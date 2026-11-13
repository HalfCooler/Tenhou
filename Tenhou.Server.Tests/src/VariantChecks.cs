using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.RegularExpressions;
using Tenhou.Server.Rules;
using Tenhou.Server.Rules.Common;
using Tenhou.Server.Rules.Guobiao;
using Tenhou.Server.Rules.Sichuan;

namespace Tenhou.Server.Tests;

internal static class VariantChecks
{
    private sealed record Fixture(string Hand, string[] Fans);
    private static int[] Kinds(string text)
    {
        List<int> output = []; List<int> numbers = [];
        foreach (char ch in text)
        {
            if (ch is >= '1' and <= '9') { numbers.Add(ch - '1'); continue; }
            if (ch is 'm' or 'p' or 's')
            { int offset = ch == 'm' ? 0 : ch == 'p' ? 9 : 18; output.AddRange(numbers.Select(n => n + offset)); numbers.Clear(); continue; }
            int honor = "ESWNPFC".IndexOf(ch);
            if (honor >= 0) output.Add(27 + honor);
        }
        return output.ToArray();
    }
    public static void Scoring()
    {
        Fixture[] fixtures = JsonSerializer.Deserialize<Fixture[]>(File.ReadAllText(Path.Combine(AppContext.BaseDirectory, "McrFixtures.json")), new JsonSerializerOptions { PropertyNameCaseInsensitive = true })!;
        int failures = 0; HashSet<string> covered = [];
        foreach (Fixture fixture in fixtures)
        {
            string[] segments = fixture.Hand.Split('|'); string raw = segments[0]; int[] used = new int[34];
            int Physical(int k) => k * 4 + used[k]++;
            List<TileSet> melds = [];
            foreach (Match m in Regex.Matches(raw, @"\[([^\]]+)\]"))
            {
                string[] parts = m.Groups[1].Value.Split(','); int[] kinds = Kinds(parts[0]); int offer = parts.Length > 1 ? int.Parse(parts[1]) : 0;
                string kind = kinds.Length == 4 ? (offer == 0 ? "closed_kan" : "open_kan") : kinds.Distinct().Count() == 1 ? "pon" : "chi";
                melds.Add(new(kind, kinds.Select(Physical).ToArray(), 0, offer));
            }
            int[] hand = Kinds(Regex.Replace(raw, @"\[[^\]]+\]", "")).Select(Physical).ToArray();
            string flags = segments.Length > 1 ? segments[1] : "EE0000";
            int flowers = segments.Length > 2 ? (int.TryParse(segments[2], out int n) ? n : segments[2].Length) : 0;
            McrContext context = new(flags[2] == '1', flags[4] == '1', flags[5] == '1', flags[3] == '1',
                27 + "ESWN".IndexOf(flags[1]), 27 + "ESWN".IndexOf(flags[0]), flowers);
            McrScore actual = McrScoring.Evaluate(hand, melds, hand[^1], context);
            int expected = fixture.Fans.Sum(f => FanCatalog.Points[f]);
            covered.UnionWith(fixture.Fans);
            if (actual.Points != expected)
            {
                ++failures; Console.WriteLine($"MCR DIFFERENCE {fixture.Hand}: expected {expected} ({string.Join('、', fixture.Fans)}), got {actual.Points} ({string.Join('、', actual.Entries.Select(e => e.Name + "×" + e.Count))})");
            }
        }
        Console.WriteLine($"MCR fixture coverage: {fixtures.Length} hands, {covered.Count} fan types, {failures} differences");
        if (failures != 0) throw new Exception("MCR reference fixtures differ");
    }
    public static void Games()
    {
        foreach (IRuleSet rule in new IRuleSet[] { new SichuanRuleSet(), new GuobiaoRuleSet() })
            for (int seed = 0; seed < 4; ++seed)
            {
                Dictionary<string, string> options = rule is SichuanRuleSet ? new() : new() { ["hands"] = "1" };
                IMatch match = rule.Create(options, new Random(seed)), replay = rule.Create(options, new Random(seed + 200));
                match.Start(); replay.Start(match.Walls[0]); int commands = 0;
                if (seed == 0)
                {
                    string fixtureDirectory = Path.Combine(AppContext.BaseDirectory, "ui-fixtures");
                    Directory.CreateDirectory(fixtureDirectory);
                    Dictionary<string, string> identity = new() { ["rule"] = rule.Descriptor.Id, ["title"] = rule.Descriptor.Name,
                        ["n0"] = "测试玩家", ["n1"] = "好友甲", ["n2"] = "好友乙", ["n3"] = "好友丙" };
                    Protocol.VariantPresentation presentation = new(rule.Normalize(options));
                    File.WriteAllLines(Path.Combine(fixtureDirectory, rule.Descriptor.Id + ".txt"), [Protocol.Wire.Encode("VGO", identity), presentation.Project(match.Events[^1], 0, 1)]);
                }
                while (!match.IsComplete)
                {
                    if (match.HandEnded) { match.Start(); replay.Start(match.Walls[^1]); }
                    bool acted = false;
                    for (int seat = 0; seat < 4; ++seat)
                    {
                        if (match.GetActions(seat).Count == 0) continue;
                        MatchAction action = match.ChooseBotAction(seat); match.Submit(seat, action); replay.Submit(seat, action);
                        match.Validate(); replay.Validate(); acted = true; ++commands;
                        if (match.Events.Count != replay.Events.Count || JsonSerializer.Serialize(match.Events[^1]) != JsonSerializer.Serialize(replay.Events[^1])) throw new Exception("Variant replay mismatch");
                        break;
                    }
                    if (!acted || commands > 20000) throw new Exception("Variant game stuck");
                }
                Console.WriteLine($"PASS {rule.Descriptor.Id} seed {seed}: {commands} actions, scores {string.Join(',', match.Scores)}");
            }
        SichuanChecks.Run();
    }
}
