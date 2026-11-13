using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace Tenhou.Server.Rules.Riichi;

public sealed record RiichiOptions
{
    public string MatchType { get; init; } = "standard4";
    public string Length { get; init; } = "east";
    public string Time { get; init; } = "5+20";
    public string Minimum { get; init; } = "1";
    public bool Fast { get; init; }
    public bool Ancient { get; init; }
    public bool Bankruptcy { get; init; } = true;
    public bool OpenTanyao { get; init; } = true;
    public bool OpenHands { get; init; }
    public int Starting { get; init; } = 25000;
    public int Target { get; init; } = 30000;
    public int RedCount { get; init; } = 3;
    public int Players => MatchType == "standard3" ? 3 : 4;
    public int ReplacementLimit => Players == 3 ? 8 : 4;
    // Presentation flags for the existing riichi renderer, not a ruleset id.
    public int DisplayFlags => 1 | (Players == 3 ? 16 : 0) | (Length == "south" ? 8 : 0)
        | (Fast ? 64 : 0) | (!OpenTanyao ? 4 : 0) | (RedCount == 0 ? 2 : 0);
    public int[] RedTiles => RedCount == 0 ? [] : (Players == 3 ? new[] { 52, 88 } : new[] { 16, 52, 88 })
        .Concat(RedCount > (Players == 3 ? 2 : 3) ? new[] { 53 } : []).ToArray();

    public Dictionary<string, string> ToSettings() => new()
    {
        ["rule"] = "riichi", ["match_type"] = MatchType, ["length"] = Length, ["time"] = Time,
        ["fast"] = Fast ? "1" : "0", ["advanced"] = "1", ["minimum"] = Minimum,
        ["starting_points"] = Text(Starting), ["target_points"] = Text(Target), ["red_count"] = Text(RedCount),
        ["bankruptcy"] = Bankruptcy ? "1" : "0", ["ancient"] = Ancient ? "1" : "0",
        ["open_tanyao"] = OpenTanyao ? "1" : "0", ["open_hands"] = OpenHands ? "1" : "0"
    };

    public static RiichiOptions From(IReadOnlyDictionary<string, string> values)
    {
        string Get(string key, string fallback) => values.TryGetValue(key, out string? v) ? v : fallback;
        bool Flag(string key, bool fallback) => Number(Get(key, fallback ? "1" : "0"), 0, 1) == 1;
        bool three = Get("match_type", "standard4") == "standard3";
        RiichiOptions result = new()
        {
            MatchType = Get("match_type", "standard4"), Length = Get("length", "east"),
            Time = Flag("fast", false) ? "3+5" : Get("time", "5+20"),
            Minimum = Get("minimum", "1"), Ancient = Flag("ancient", false),
            Bankruptcy = Flag("bankruptcy", true), OpenTanyao = Flag("open_tanyao", true),
            OpenHands = Flag("open_hands", false),
            Starting = Number(Get("starting_points", three ? "35000" : "25000"), 0, 200000),
            Target = Number(Get("target_points", three ? "40000" : "30000"), 0, 100000000),
            RedCount = Number(Get("red_count", three ? "2" : "3"), 0, 4)
        };
        result = result with { Fast = result.Time == "3+5" };
        foreach (string key in values.Keys)
            if (!result.ToSettings().ContainsKey(key)) throw new ArgumentException("Unknown setting: " + key);
        if (Get("rule", "riichi") != "riichi" || result.MatchType is not ("standard3" or "standard4")
            || result.Length is not ("single" or "east" or "south")
            || result.Time is not ("3+5" or "5+10" or "5+20" or "60+0" or "300+0")
            || result.Minimum is not ("1" or "2" or "4" or "mangan" or "haneman" or "baiman" or "sanbaiman" or "yakuman")
            || result.Starting % 100 != 0 || result.Target % 100 != 0 || result.Target < result.Starting
            || !(three ? new[] { 0, 2, 3 } : new[] { 0, 3, 4 }).Contains(result.RedCount))
            throw new ArgumentException("Invalid riichi options");
        return result;
    }

    internal static string Text(int value) => value.ToString(CultureInfo.InvariantCulture);
    internal static int Number(string value, int min = 0, int max = 100000000)
    {
        if (!int.TryParse(value, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out int n)
            || n < min || n > max) throw new ArgumentException("Invalid integer");
        return n;
    }
    internal static string List(IEnumerable<int> values) => string.Join(',', values);
}

public sealed class Meld
{
    public string Kind { get; set; } = "";
    public List<int> Tiles { get; set; } = [];
    public int Who { get; set; }
    public int Source { get; set; }
    public int Called { get; set; } = -1;
    public int Added { get; set; } = -1;
    public bool Opened => Kind != "closed_kan";
    public int Encode()
    {
        int[] ts = Tiles.Order().ToArray();
        int relative = (Source - Who + 4) % 4;
        if (Kind == "chi")
        {
            int low = ts[0] / 4;
            int packed = (low / 9 * 7 + low % 9) * 3 + Array.IndexOf(ts, Called);
            return (packed << 10) | ((ts[0] % 4) << 3) | ((ts[1] % 4) << 5) | ((ts[2] % 4) << 7) | 4 | relative;
        }
        if (Kind is "pon" or "added_kan")
        {
            int first = ts[0] / 4 * 4;
            int excluded = Kind == "pon" ? Enumerable.Range(first, 4).Single(t => !ts.Contains(t)) : Added;
            int[] pon = Enumerable.Range(first, 4).Where(t => t != excluded).ToArray();
            int packed = first / 4 * 3 + Array.IndexOf(pon, Called);
            return (packed << 9) | ((excluded % 4) << 5) | (Kind == "pon" ? 8 : 16) | relative;
        }
        return ((Opened ? Called : ts[0]) << 8) | relative;
    }
}

public sealed class Player
{
    public List<int> Hand { get; } = [];
    public List<int> River { get; } = [];
    public List<int> Nuki { get; } = [];
    public List<Meld> Melds { get; } = [];
    public HashSet<int> Called { get; } = [];
    public HashSet<int> Forbidden { get; set; } = [];
    public Dictionary<int, int> Pao { get; } = [];
    public bool Riichi { get; set; }
    public bool DoubleReach { get; set; }
    public bool Ippatsu { get; set; }
    public bool TemporaryFuriten { get; set; }
    public bool RiichiFuriten { get; set; }
    public int Drawn { get; set; } = -1;
    public int Draws { get; set; }
    public bool Closed => Melds.All(m => !m.Opened);
}

public sealed class HandValue
{
    public int Han { get; set; }
    public int Fu { get; set; }
    public int Main { get; set; }
    public int Additional { get; set; }
    public int Level { get; set; }
    public List<int> Yaku { get; } = [];
    public List<int> Yakuman { get; } = [];
}

public sealed class WinContext
{
    public bool Tsumo, Dealer, Riichi, DoubleReach, Ippatsu, Rinshan, Chankan, Haitei, Houtei, Tenhou, Chiihou,
        Renhou, Swallow, KanDiscard;
    public int SeatWind = 28, RoundWind = 27;
    public List<int> Dora = [], Ura = [], Nuki = [];
}
