using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using Tenhou.Server.Rules;
using Tenhou.Server.Rules.Riichi;

namespace Tenhou.Server.Protocol;

// A ruleset's game messages are adapted independently of room/session hosting.
// A different client renderer can provide another presentation for the same rules.
public interface IMatchPresentation
{
    string GameTag => "GO";
    bool SupportsAnimation => true;
    string ReplayExtension => "xml";
    int DisplayFlags { get; }
    string RankCategory { get; }
    Dictionary<string, string> GameInfo { get; }
    string Project(MatchEvent e, int viewer, int window);
    MatchAction Decode(WireMessage message, IReadOnlyList<MatchAction> options, int declared);
    string Replay(IMatch match, string[] names);
}

public sealed class VariantPresentation : IMatchPresentation
{
    private readonly IReadOnlyDictionary<string, string> options;
    public VariantPresentation(IReadOnlyDictionary<string, string> options) => this.options = options;
    public string GameTag => "VGO";
    public bool SupportsAnimation => false;
    public string ReplayExtension => "json";
    public int DisplayFlags => 0;
    public string RankCategory => "";
    public Dictionary<string, string> GameInfo => new() { ["rule"] = options["rule"], ["players"] = "4" };
    public string Project(MatchEvent e, int viewer, int window)
    {
        Dictionary<string, string> data = new(e.Data);
        for (int s = 0; s < 4; ++s)
        {
            data["hidden" + s] = data["hand" + s].Split(',', StringSplitOptions.RemoveEmptyEntries).Length.ToString(CultureInfo.InvariantCulture);
            if (s != viewer && options.GetValueOrDefault("open_hands") != "1") data.Remove("hand" + s);
            if (s != viewer)
                data["melds" + s] = string.Join(';', data["melds" + s].Split(';').Select(m => m.StartsWith("closed_kan:", StringComparison.Ordinal) ? "closed_kan:?" : m));
            data.Remove("actions" + s);
            // Dingque selections remain private until everyone has committed.
            if (s != viewer && data["phase"] == "missing") data["missing" + s] = "-1";
        }
        data["actions"] = e.Data["actions" + viewer];
        data["seat"] = viewer.ToString(CultureInfo.InvariantCulture); data["window"] = window.ToString(CultureInfo.InvariantCulture);
        return Wire.Encode("VSTATE", data);
    }
    public MatchAction Decode(WireMessage message, IReadOnlyList<MatchAction> actions, int declared)
    {
        if (message.Tag != "VACT" || message.Attributes.Keys.Any(k => k is not ("kind" or "tiles" or "table"))) throw new ArgumentException("Invalid variant action");
        return new(message.Get("kind"), message.Get("tiles").Split(',', StringSplitOptions.RemoveEmptyEntries).Select(s => int.Parse(s, CultureInfo.InvariantCulture)).ToArray());
    }
    public string Replay(IMatch match, string[] names) => System.Text.Json.JsonSerializer.Serialize(new
    { version = 1, rule = options["rule"], options, names, scores = match.Scores, events = match.Events }, Infrastructure.ServerSettings.Json);
}

public sealed class RiichiPresentation : IMatchPresentation
{
    private readonly RiichiOptions options;
    public RiichiPresentation(IReadOnlyDictionary<string, string> values) => options = RiichiOptions.From(values);
    public int DisplayFlags => options.DisplayFlags;
    public string RankCategory => "riichi/" + options.Players;
    public Dictionary<string, string> GameInfo => new()
    {
        ["type"] = DisplayFlags.ToString(CultureInfo.InvariantCulture), ["open_hands"] = options.OpenHands ? "1" : "0",
        ["red_tiles"] = string.Join(',', options.RedTiles), ["room_length"] = options.Length,
        ["local_time"] = (options.Time.Split('+').Sum(int.Parse) * 1000).ToString(CultureInfo.InvariantCulture)
    };
    public string Project(MatchEvent e, int viewer, int window) => Wire.Project(e, viewer, options.OpenHands, window);
    public MatchAction Decode(WireMessage message, IReadOnlyList<MatchAction> actions, int declared) => Wire.Decode(message, actions, declared);
    public string Replay(IMatch match, string[] names)
    {
        StringBuilder text = new("<mjloggm ver=\"2.3\">\n");
        text.AppendLine(Wire.Encode("GO", ("type", DisplayFlags), ("red_tiles", string.Join(',', options.RedTiles))));
        text.AppendLine(Wire.Encode("UN", Enumerable.Range(0, 4).ToDictionary(s => "n" + s, s => Uri.EscapeDataString(names[s]))));
        foreach (MatchEvent e in match.Events) if (e.Kind != "PROF") text.AppendLine(Wire.Encode(e.Kind, e.Data));
        return text.AppendLine("</mjloggm>").ToString();
    }
}

public sealed class PresentationRegistry
{
    private readonly Dictionary<string, Func<IReadOnlyDictionary<string, string>, IMatchPresentation>> factories = [];
    public void Register(string id, Func<IReadOnlyDictionary<string, string>, IMatchPresentation> factory) => factories.Add(id, factory);
    public IMatchPresentation Create(RuleDescriptor descriptor, IReadOnlyDictionary<string, string> options)
        => factories.TryGetValue(descriptor.Presentation, out Func<IReadOnlyDictionary<string, string>, IMatchPresentation>? factory)
            ? factory(options) : throw new ArgumentException("Unsupported presentation: " + descriptor.Presentation);
}
