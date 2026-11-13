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
                Dictionary<string, string> options = new() { ["hands"] = "1" };
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
                    bool acted = false;
                    for (int seat = 0; seat < 4; ++seat)
                    {
                        if (match.GetActions(seat).Count == 0) continue;
                        MatchAction action = match.ChooseBotAction(seat); match.Submit(seat, action); replay.Submit(seat, action);
                        match.Validate(); replay.Validate(); acted = true; ++commands;
                        if (JsonSerializer.Serialize(match.Events) != JsonSerializer.Serialize(replay.Events)) throw new Exception("Variant replay mismatch");
                        break;
                    }
                    if (!acted || commands > 2000) throw new Exception("Variant game stuck");
                }
                Console.WriteLine($"PASS {rule.Descriptor.Id} seed {seed}: {commands} actions, scores {string.Join(',', match.Scores)}");
            }
        BloodflowContinuation();
    }
    private static void BloodflowContinuation()
    {
        SichuanRuleSet rule = new();
        IMatch match = rule.Create(new Dictionary<string, string> { ["hands"] = "1", ["exchange"] = "0" }, new Random(5));
        int[] hand = [0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 16]; int[] draws = [17, 36, 37, 38, 19];
        List<int> available = Enumerable.Range(0, 108).Except(hand).Except(draws).ToList();
        List<int>[] hands = [hand.ToList(), available.Take(13).ToList(), available.Skip(13).Take(13).ToList(), available.Skip(26).Take(13).ToList()];
        int[] wall = Enumerable.Range(0, 13).SelectMany(i => Enumerable.Range(0, 4).Select(s => hands[s][i]))
            .Concat(draws).Concat(available.Skip(39)).ToArray();
        match.Start(wall);
        for (int seat = 0; seat < 4; ++seat) match.Submit(seat, new("dingque", [seat == 0 ? 2 : 0]));
        match.Submit(0, new("tsumo"));
        SichuanMatch bloodflow = (SichuanMatch)match;
        if (bloodflow.IsComplete || bloodflow.Players[0].Wins != 1 || bloodflow.Players[0].Hand.Count != 13 || !bloodflow.Players[0].Locked)
            throw new Exception("First bloodflow win ended the hand or failed to lock the wait");
        for (int seat = 1; seat < 4; ++seat)
        {
            match.Submit(seat, match.GetActions(seat).First(a => a.Kind == "discard"));
            while (match.Phase == "claims")
            {
                int claimant = Enumerable.Range(0, 4).First(s => match.GetActions(s).Count > 0);
                match.Submit(claimant, match.GetActions(claimant).First(a => a.Kind == "skip"));
            }
        }
        if (bloodflow.Turn != 0 || bloodflow.Players[0].Drawn != 19) throw new Exception("Bloodflow rotation lost its wall order");
        match.Submit(0, new("tsumo")); match.Validate();
        if (bloodflow.Players[0].Wins != 2 || bloodflow.Players[0].Hand.Count != 13 || bloodflow.IsComplete)
            throw new Exception("Repeated bloodflow win failed");
        Console.WriteLine("PASS bloodflow repeated wins, locked wait, continued play and tile conservation");
        bloodflow.Players[0].Hand.Clear();
        bloodflow.Players[0].Hand.AddRange(new[] { 4, 8, 12, 16, 20, 24, 28, 32, 36, 37 });
        bloodflow.Players[0].Melds.Add(new("closed_kan", [0, 1, 2, 3], 0, 0));
        if (bloodflow.Score(0, 0, false, false) is not null) throw new Exception("A fifth copy in a declared kong scored as a ready-hand win");
        Console.WriteLine("PASS Sichuan virtual fifth-copy wait after a kong is rejected");
        SichuanMatch alternate = (SichuanMatch)rule.Create(new Dictionary<string, string> { ["hands"] = "1", ["fan_cap"] = "10" }, new Random(1));
        alternate.Start(); alternate.Players[0].MissingSuit = 2; alternate.Players[0].Hand.Clear();
        alternate.Players[0].Hand.AddRange(Enumerable.Range(0, 12).Concat(new[] { 32, 33 }));
        SichuanValue? alternateValue = alternate.Score(0, 33, true, false);
        if (alternateValue?.Fan != 7 || (alternateValue.Patterns.Contains("七对") && alternateValue.Patterns.Contains("带幺九")))
            throw new Exception("Sichuan combined mutually exclusive decompositions");
        Console.WriteLine("PASS Sichuan scoring selects one decomposition before adding shared bonuses");
    }
}
