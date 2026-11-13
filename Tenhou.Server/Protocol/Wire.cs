using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Linq;
using Tenhou.Server.Rules;

namespace Tenhou.Server.Protocol;

public sealed record WireMessage(string Tag, Dictionary<string, string> Attributes)
{
    public string Get(string key, string fallback = "") => Attributes.GetValueOrDefault(key, fallback);
    public int Int(string key, int min = 0, int max = 100000000)
    {
        if (!int.TryParse(Get(key), NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out int result)
            || result < min || result > max) throw new ArgumentException("Invalid field: " + key);
        return result;
    }
}

public static class Wire
{
    public const int MaximumFrameBytes = 65536;
    public static WireMessage Parse(string frame)
    {
        if (frame.Length < 4 || frame.Length > MaximumFrameBytes) throw new ArgumentException("Invalid frame length");
        using StringReader text = new(frame);
        using XmlReader reader = XmlReader.Create(text, new XmlReaderSettings
        { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null, MaxCharactersInDocument = MaximumFrameBytes });
        XElement element = XElement.Load(reader);
        if (element.Name.Namespace != XNamespace.None || element.HasElements || element.Value.Length != 0
            || element.Attributes().Count() > 64 || element.Attributes().Any(a => a.IsNamespaceDeclaration || a.Name.Namespace != XNamespace.None))
            throw new ArgumentException("Expected a flat protocol message");
        return new(element.Name.LocalName, element.Attributes().ToDictionary(a => a.Name.LocalName, a => a.Value, StringComparer.Ordinal));
    }
    public static string Encode(string tag, IReadOnlyDictionary<string, string>? values = null)
        => new XElement(tag, values?.Select(e => new XAttribute(e.Key, e.Value))).ToString(SaveOptions.DisableFormatting);
    public static string Encode(string tag, params (string Key, object Value)[] values)
        => Encode(tag, values.ToDictionary(e => e.Key, e => Convert.ToString(e.Value, CultureInfo.InvariantCulture) ?? ""));
    public static string Project(MatchEvent e, int viewer, bool openHands, int window)
    {
        Dictionary<string, string> data = new(e.Data); string tag = e.Kind;
        int Rotate(int s) => (s - viewer + 4) % 4;
        if (tag == "INIT")
        {
            data["hai"] = data["hai" + viewer];
            for (int s = 0; s < 4; ++s)
            {
                if (openHands && data.TryGetValue("hai" + s, out string? hand)) data["open" + Rotate(s)] = hand;
                data.Remove("hai" + s);
            }
            data["oya"] = Rotate(int.Parse(data["oya"], CultureInfo.InvariantCulture)).ToString(CultureInfo.InvariantCulture);
            string[] ten = data["ten"].Split(','); data["ten"] = string.Join(',', Enumerable.Range(0, 4).Select(s => ten[(s + viewer) % 4]));
        }
        else if (tag.Length > 1 && char.IsAsciiDigit(tag[1]) && "TUVWDEFGtuvwdefg".Contains(tag[0], StringComparison.Ordinal))
        {
            char upper = char.ToUpperInvariant(tag[0]); bool draw = "TUVW".Contains(upper, StringComparison.Ordinal);
            string letters = draw ? "TUVW" : "DEFG"; int seat = letters.IndexOf(upper);
            char code = letters[Rotate(seat)]; if (char.IsLower(tag[0])) code = char.ToLowerInvariant(code);
            tag = code + (!draw || seat == viewer || openHands ? tag[1..] : "");
        }
        foreach (string key in new[] { "who", "fromWho" })
            if (data.TryGetValue(key, out string? seat)) data[key] = Rotate(int.Parse(seat, CultureInfo.InvariantCulture)).ToString(CultureInfo.InvariantCulture);
        foreach (string key in new[] { "sc", "owari" })
            if (data.TryGetValue(key, out string? raw))
            {
                string[] fields = raw.Split(',');
                data[key] = string.Join(',', Enumerable.Range(0, 4).SelectMany(s => fields.Skip(((s + viewer) % 4) * 2).Take(2)));
            }
        if (e.Kind == "RYUUKYOKU")
        {
            for (int s = 0; s < 4; ++s) data.Remove("hai" + s);
            for (int s = 0; s < 4; ++s) if (e.Data.TryGetValue("hai" + s, out string? hand)) data["hai" + Rotate(s)] = hand;
        }
        if (e.Hints.TryGetValue(viewer, out int hint) && hint != 0) data["t"] = hint.ToString(CultureInfo.InvariantCulture);
        data["window"] = window.ToString(CultureInfo.InvariantCulture);
        return Encode(tag, data);
    }
    public static MatchAction Decode(WireMessage m, IReadOnlyList<MatchAction> options, int declared)
    {
        if (m.Tag == "D" && m.Attributes.Count == 1) return new(declared == m.Int("p", 0, 135) ? "riichi" : "discard", [m.Int("p", 0, 135)]);
        if (m.Tag != "N") throw new ArgumentException("Expected an action");
        if (m.Attributes.Count == 0) return new("skip");
        string kind = m.Int("type", 1, 10) switch
        {
            1 => "pon", 2 => "open_kan", 3 => "chi", 4 => "closed_kan", 5 => "added_kan",
            6 => "ron", 7 => "tsumo", 9 => "draw", 10 => "nuki", _ => throw new ArgumentException("Unsupported action")
        };
        if (kind is "pon" or "chi" && m.Attributes.Count == 3)
            return new(kind, new[] { m.Int("hai0", 0, 135), m.Int("hai1", 0, 135) }.Order().ToArray());
        if (kind is "closed_kan" or "added_kan" or "nuki" && m.Attributes.Count == 2)
            return options.FirstOrDefault(a => a.Kind == kind && a.Tiles.Contains(m.Int("hai", 0, 135))) ?? throw new ArgumentException("Unavailable action");
        if (m.Attributes.Count != 1) throw new ArgumentException("Unexpected action fields");
        return kind == "open_kan" ? options.FirstOrDefault(a => a.Kind == kind) ?? throw new ArgumentException("Unavailable kong") : new(kind);
    }
}
