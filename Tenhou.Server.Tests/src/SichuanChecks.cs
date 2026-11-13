using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Tenhou.Server.Rules;
using Tenhou.Server.Rules.Common;
using Tenhou.Server.Rules.Sichuan;

namespace Tenhou.Server.Tests;

internal static class SichuanChecks
{
    private static void Check(bool ok, string message) { if (!ok) throw new Exception(message); }
    private static int[] Kinds(string text)
    {
        List<int> result = [], digits = [];
        foreach (char ch in text)
            if (ch is >= '1' and <= '9') digits.Add(ch - '1');
            else if (ch is 'm' or 'p' or 's')
            { int offset = ch == 'm' ? 0 : ch == 'p' ? 9 : 18; result.AddRange(digits.Select(n => n + offset)); digits.Clear(); }
        return result.ToArray();
    }
    private static void Set(SichuanMatch match, string name, object value)
    {
        for (Type? type = match.GetType(); type is not null; type = type.BaseType)
        {
            FieldInfo? field = type.GetField(name, BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            if (field is not null) { field.SetValue(match, value); return; }
        }
        typeof(VariantMatch).GetProperty(name)!.SetValue(match, value);
    }
    private static void Invoke(SichuanMatch match, string name, params object[] args)
        => typeof(SichuanMatch).GetMethod(name, BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(match, args);
    private static SichuanMatch New(Dictionary<string, string>? options = null)
    {
        SichuanMatch match = (SichuanMatch)new SichuanRuleSet().Create(options ?? new(), new Random(13));
        Set(match, "HandNumber", 3); match.Start(); return match; // Fourth hand deliberately has no exchange.
    }
    private static SichuanValue Value(string hand, params string[] melds)
    {
        SichuanMatch match = New(new() { ["fan_cap"] = "0" }); VariantPlayer player = match.Players[0];
        int[] used = new int[27]; int Physical(int k) => k * 4 + used[k]++;
        player.Hand.Clear(); player.Hand.AddRange(Kinds(hand).Select(Physical));
        foreach (string text in melds)
        {
            int[] tiles = Kinds(text).Select(Physical).ToArray();
            player.Melds.Add(new(tiles.Length == 4 ? "closed_kan" : "pon", tiles, 0, 1));
        }
        player.MissingSuit = Enumerable.Range(0, 3).First(s => player.Hand.Concat(player.Melds.SelectMany(m => m.Tiles)).All(t => t / 36 != s));
        return match.Score(0, player.Hand[^1], true, false) ?? throw new Exception("Fixture did not win: " + hand);
    }
    private static void Fan(string hand, int fan, string pattern, params string[] melds)
    {
        SichuanValue value = Value(hand, melds);
        Check(value.Fan == fan && value.Patterns.Contains(pattern), $"{hand}: expected {fan}/{pattern}, got {value.Fan}/{string.Join(',', value.Patterns)}");
    }
    private static SichuanMatch Rig(string[] texts, string draws, Dictionary<string, string>? options = null)
    {
        int[] used = new int[27]; int Physical(int k) { Check(used[k] < 4, "Rig exceeds four copies"); return k * 4 + used[k]++; }
        List<int>[] hands = texts.Select(t => Kinds(t).Select(Physical).ToList()).ToArray();
        int[] first = Kinds(draws).Select(Physical).ToArray();
        HashSet<int> occupied = hands.SelectMany(h => h).Concat(first).ToHashSet();
        Queue<int> remaining = new(Enumerable.Range(0, 108).Where(t => !occupied.Contains(t)));
        foreach (List<int> hand in hands) { Check(hand.Count <= 13, "Oversize rig"); while (hand.Count < 13) hand.Add(remaining.Dequeue()); }
        int[] wall = Enumerable.Range(0, 13).SelectMany(i => hands.Select(h => h[i])).Concat(first).Concat(remaining).ToArray();
        SichuanMatch match = (SichuanMatch)new SichuanRuleSet().Create(options ?? new(), new Random(3));
        Set(match, "HandNumber", 3); match.Start(wall);
        for (int seat = 0; seat < 4; ++seat)
        {
            int missing = Enumerable.Range(0, 3).OrderBy(s => match.Players[seat].Hand.Count(t => t / 36 == s)).First();
            match.Submit(seat, new("dingque", [missing]));
        }
        Set(match, "Interrupted", true); return match;
    }
    private static void Responses(SichuanMatch match, params int[] ron)
    {
        for (int seat = 3; seat >= 0 && match.Phase == "claims"; --seat)
        {
            IReadOnlyList<MatchAction> actions = match.GetActions(seat);
            if (actions.Count > 0) match.Submit(seat, actions.First(a => a.Kind == (ron.Contains(seat) ? "ron" : "skip")));
        }
    }
    private static void ReadyHand(SichuanMatch match, int seat, string text, int missing = 2)
    {
        int[] used = new int[27];
        match.Players[seat].Hand.Clear(); match.Players[seat].Melds.Clear();
        match.Players[seat].Hand.AddRange(Kinds(text).Select(k => k * 4 + used[k]++)); match.Players[seat].MissingSuit = missing;
    }
    public static void Run()
    {
        Scoring(); Continuation(); Claims(); TaxesAndDraw(); ScheduleAndEnd();
    }
    private static void Scoring()
    {
        Fan("123456789m23455p", 1, "底和");
        Fan("111123m45678955p", 2, "根×1");
        Fan("111222m33344455p", 2, "对对和");
        Fan("12345678923455m", 3, "清一色");
        Fan("112233m44556677p", 3, "七对");
        Fan("123789m12378999p", 3, "带幺九");
        Fan("55p", 3, "金钩钓", "111m", "222m", "333p", "444p");
        Fan("11122233344455m", 4, "清对");
        Fan("222555m22288855p", 4, "将对");
        Fan("111122m33445566p", 5, "龙七对");
        Fan("11112222m334455p", 6, "根×1");
        Fan("11223344556677m", 5, "清七对");
        Fan("11112233789999m", 7, "清幺九");
        Fan("55m", 5, "清金钩钓", "111m", "222m", "333m", "444m");
        Fan("11112233445566m", 6, "清龙七对");
        Fan("11112222334455m", 7, "根×1");
        Fan("55p", 4, "根×1", "1111m", "222m", "333p", "444p");
        SichuanMatch match = Rig(["123456789m2345p", "", "", ""], "5p", new() { ["fan_cap"] = "0" });
        Set(match, "Interrupted", false);
        Check(match.Score(0, match.Players[0].Drawn, true)?.Patterns.Contains("天和") == true, "Heavenly hand missing");
        Check(match.Score(0, match.Players[0].Drawn, true)?.Fan == 6, "Heavenly hand wrong tier");
        Set(match, "Interrupted", true); Set(match, "Replacement", true);
        Check(match.Score(0, match.Players[0].Drawn, true)?.Fan == 2, "Kong replacement bonus missing");
        Set(match, "Replacement", false);
        ReadyHand(match, 1, "123456789m23455p"); match.Players[1].Draws = 1;
        Set(match, "Interrupted", false);
        Check(match.Score(1, 0, true)?.Patterns.Contains("地和") == true, "Earthly hand missing");
        match.Players[1].River.Add(107);
        Check(match.Score(1, 0, true)?.Patterns.Contains("地和") == false, "Earthly hand repeated after discard");
        ReadyHand(match, 0, "23456789m11p"); match.Players[0].Melds.Add(new("closed_kan", [0, 1, 2, 3], 0, 0));
        Check(match.Score(0, 0, false, false) is null, "Virtual fifth-copy wait accepted");
        Console.WriteLine("PASS Sichuan fan tiers, corrected pure outside hand, roots, decomposition, heavenly/earthly and replacement bonuses");
    }
    private static void Continuation()
    {
        SichuanMatch match = Rig(["123456789m2345p", "123456789s2345m", "111222p666777s8p", ""], "5p5m8p");
        Check(match.Scores.All(p => p == 50000), "Starting points changed");
        match.Submit(0, new("tsumo"));
        string fixtureDirectory = Path.Combine(AppContext.BaseDirectory, "ui-fixtures"); Directory.CreateDirectory(fixtureDirectory);
        File.WriteAllLines(Path.Combine(fixtureDirectory, "sichuan-bloodbattle-retired.txt"),
        [Protocol.Wire.Encode("VGO", new Dictionary<string, string> { ["rule"] = "sichuan-bloodbattle", ["title"] = "四川麻将 · 血战到底",
            ["n0"] = "测试玩家", ["n1"] = "好友甲", ["n2"] = "好友乙", ["n3"] = "好友丙" }),
            new Protocol.VariantPresentation(new SichuanRuleSet().Normalize(new Dictionary<string, string>())).Project(match.Events[^1], 0, 1)]);
        Check(match.Turn == 1 && match.Players[0].Wins == 1 && match.GetActions(0).Count == 0 && !match.HandEnded, "Winner did not retire");
        Check(match.Scores.SequenceEqual(new[] { 53000, 49000, 49000, 49000 }), "Self draw base payment differs from ron");
        match.Submit(1, new("tsumo"));
        Check(match.Scores[0] == 53000 && match.Turn == 2, "Retired player paid or drew again");
        match.Submit(2, new("tsumo")); match.Validate();
        Check(match.HandEnded && match.Scores.SequenceEqual(new[] { 53000, 51000, 50000, 46000 }), "Three winners did not finish with active-only payments");
        match.Start(); Check(match.Dealer == 0 && match.HandNumber == 5, "First winner did not retain dealer");
        SichuanMatch skip = Rig(["123456789m2345p", "", "", ""], "5p");
        skip.Submit(0, new("tsumo"));
        for (int turn = 1; turn <= 3; ++turn)
        {
            int seat = skip.Turn; Check(seat != 0, "Retired seat regained a turn");
            skip.Submit(seat, skip.GetActions(seat).First(a => a.Kind == "discard")); Responses(skip);
        }
        Check(skip.Turn == 1 && skip.Players[0].Draws == 1, "Turn rotation did not skip retired player"); skip.Validate();
        Console.WriteLine("PASS blood battle retirement, skipped draws/claims/payments, three-win finish and tile conservation");
    }
    private static void Claims()
    {
        SichuanMatch match = Rig(["", "123456789m2345p", "234567m1237895p", ""], "5p");
        int tile = match.Players[0].Drawn; match.Players[0].MissingSuit = 2;
        match.Submit(0, new("discard", [tile]));
        Check(match.GetActions(1).Any(a => a.Kind == "ron") && match.GetActions(2).Any(a => a.Kind == "ron"), "Multiple ron fixture failed");
        Responses(match, 1, 2); match.Validate();
        Check(match.Players[1].Wins == 1 && match.Players[2].Wins == 0 && match.Turn == 2, "Counterclockwise head bump failed");
        Check(match.Scores.SequenceEqual(new[] { 49000, 51000, 50000, 50000 }), "Ron charged more than the discarder or used wrong rate");
        Invoke(match, "Exhausted"); match.Start(); Check(match.Dealer == 1, "Nondealer first winner was not next dealer");
        SichuanMatch robbed = Rig(["111234567m2345p", "23m123456789p55m", "", ""], "1m");
        VariantPlayer actor = robbed.Players[0];
        int[] pon = actor.Hand.Where(t => t / 4 == 0 && t != actor.Drawn).ToArray();
        foreach (int t in pon) actor.Hand.Remove(t); actor.Melds.Add(new("pon", pon, 0, 3));
        robbed.Submit(0, new("added_kan", [actor.Drawn])); Responses(robbed, 1); robbed.Validate();
        Check(robbed.Players[1].Wins == 1 && actor.Melds[0].Kind == "pon" && robbed.Scores[0] == 48000 && robbed.Scores[1] == 52000,
            "Robbed added kong paid tax, completed, or missed its bonus");
        SichuanMatch afterKong = Rig(["111234567m2345p", "123456s123789p9s", "", ""], "1m");
        afterKong.Submit(0, afterKong.GetActions(0).First(a => a.Kind == "closed_kan"));
        afterKong.Submit(0, new("discard", [afterKong.Players[0].Drawn])); Responses(afterKong, 1); afterKong.Validate();
        Check(afterKong.Scores.SequenceEqual(new[] { 54000, 50000, 48000, 48000 }), "Kong-discard win missed bonus or transferred kong income");
        // A third win ends immediately and must not trigger any refund of that tax.
        afterKong.Players[2].Wins = 1;
        ReadyHand(afterKong, 3, "123456789m23455p"); afterKong.Players[3].Drawn = afterKong.Players[3].Hand[^1];
        Invoke(afterKong, "Win", new int[] { 3 }, 3, true);
        Check(afterKong.HandEnded && afterKong.Scores[0] == 53000 && afterKong.Scores[3] == 49000, "Three-win finish refunded kong taxes");
        Console.WriteLine("PASS reverse-response-order head bump, first-winner dealer, robbing an added kong without tax");
    }
    private static void TaxesAndDraw()
    {
        foreach (string kind in new[] { "closed_kan", "open_kan", "added_kan" })
        {
            SichuanMatch tax = New(); tax.Players[3].Wins = 1;
            Invoke(tax, "KongPayment", 0, 1, kind);
            int[] expected = kind switch { "closed_kan" => [54000, 48000, 48000, 50000], "open_kan" => [52000, 48000, 50000, 50000], _ => [52000, 49000, 49000, 50000] };
            Check(tax.Scores.SequenceEqual(expected), "Wrong active-only tax: " + kind);
        }
        SichuanMatch match = New(new() { ["fan_cap"] = "0" });
        ReadyHand(match, 0, "123456789m2345p"); // base wait: 1 fan, 1000
        ReadyHand(match, 1, "111222m3334445p"); // triplet wait: 2 fan, 2000
        ReadyHand(match, 2, "147m147p147s258m1p"); // pig
        ReadyHand(match, 3, "123456789m2345p"); match.Players[3].Wins = 1;
        Invoke(match, "KongPayment", 2, 2, "closed_kan");
        Invoke(match, "Exhausted");
        Check(match.Scores.SequenceEqual(new[] { 66000, 66000, 18000, 50000 }), "Pig cap fallback, retired exclusion or tax refund wrong");
        SichuanMatch ready = New();
        ReadyHand(ready, 0, "123456789m2345p");
        ReadyHand(ready, 1, "111222m3334445p");
        ReadyHand(ready, 2, "147m147p258m258p1m");
        ReadyHand(ready, 3, "123456789m2345p"); ready.Players[3].Wins = 1;
        Invoke(ready, "KongPayment", 2, 2, "added_kan"); Invoke(ready, "Exhausted");
        Check(ready.Scores.SequenceEqual(new[] { 51000, 52000, 47000, 50000 }), "Maximum wait compensation or nonready tax refund wrong");
        SichuanMatch keep = New();
        ReadyHand(keep, 0, "123456789m2345p"); ReadyHand(keep, 1, "111222m3334445p");
        ReadyHand(keep, 2, "147m147p258m258p1m"); keep.Players[3].Wins = 1;
        Invoke(keep, "KongPayment", 0, 0, "closed_kan"); Invoke(keep, "Exhausted");
        Check(keep.Scores.SequenceEqual(new[] { 55000, 50000, 45000, 50000 }), "Ready kong recipient incorrectly refunded taxes");
        SichuanMatch pigs = New();
        for (int s = 0; s < 4; ++s) ReadyHand(pigs, s, "147m147p147s258m1p");
        pigs.Players[3].Wins = 1; Invoke(pigs, "Exhausted");
        Check(pigs.Scores.All(p => p == 50000), "Mutual pigs did not pay each other equally");
        Console.WriteLine("PASS all three taxes, active-only draw settlement, maximum waits, pig fallback cap, mutual pigs and refunds");
    }
    private static void ScheduleAndEnd()
    {
        SichuanRuleSet rule = new();
        Check(rule.Normalize(new Dictionary<string, string>())["fan_cap"] == "5", "Default fan cap changed");
        try { rule.Normalize(new Dictionary<string, string> { ["last_four_must_win"] = "1" }); throw new Exception("Legacy forced-win option accepted"); }
        catch (ArgumentException) { }
        SichuanMatch match = (SichuanMatch)rule.Create(new Dictionary<string, string>(), new Random(5));
        string[] directions = ["clockwise", "counterclockwise", "opposite", "none"];
        for (int hand = 1; hand <= 8; ++hand)
        {
            match.Start(); Check(match.Events[^1].Data["exchange_direction"] == directions[(hand - 1) % 4], "Exchange schedule drifted");
            if (hand % 4 != 0)
            {
                int[][] original = match.Players.Select(p => p.Hand.ToArray()).ToArray();
                MatchAction[] choices = Enumerable.Range(0, 4).Select(s => match.GetActions(s).First()).ToArray();
                for (int s = 0; s < 4; ++s) match.Submit(s, choices[s]);
                int direction = hand % 4 == 1 ? 3 : hand % 4 == 2 ? 1 : 2;
                for (int s = 0; s < 4; ++s)
                    Check(match.Players[s].Hand.Order().SequenceEqual(original[s].Except(choices[s].Tiles).Concat(choices[(s - direction + 4) % 4].Tiles).Order()), "Exchange recipient wrong");
            }
            Check(match.Phase == "missing", "No-exchange hand failed to enter dingque");
            // Exercise the hand-count boundary without tying the schedule check to bot strategy.
            for (int s = 0; s < 4; ++s) ReadyHand(match, s, "123456789m2345p");
            Invoke(match, "Exhausted");
            Check(match.IsComplete == (hand == 8), "Fixed eight-hand match ended at wrong boundary");
        }
        SichuanMatch optional = Rig(["123456789m2345p", "", "", ""], "5p");
        List<int> wall = (List<int>)typeof(VariantMatch).GetField("Wall", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(optional)!;
        wall.RemoveRange(0, wall.Count - 3);
        Check(optional.GetActions(0).Any(a => a.Kind == "discard") && optional.GetActions(0).Any(a => a.Kind == "tsumo"), "Last four tiles force a win");
        SichuanMatch flying = Rig(["111234567m2345p", "", "", ""], "1m", new() { ["bankruptcy"] = "1" });
        Set(flying, "Points", new int[] { 50000, 1000, 50000, 99000 }); // Tax takes seat 1 below zero.
        flying.Submit(0, flying.GetActions(0).First(a => a.Kind == "closed_kan")); flying.Validate();
        Check(flying.IsComplete, "Enabled bankruptcy did not finish");
        SichuanMatch noFlying = New(); Set(noFlying, "Points", new int[] { 50000, -1000, 50000, 101000 });
        for (int s = 0; s < 4; ++s) ReadyHand(noFlying, s, "123456789m2345p");
        Invoke(noFlying, "Exhausted"); Check(noFlying.HandEnded && !noFlying.IsComplete, "Default bankruptcy unexpectedly ended the match");
        Console.WriteLine("PASS eight-hand exchange cycle, exact final hand, optional bankruptcy and optional last-four wins");
    }
}

