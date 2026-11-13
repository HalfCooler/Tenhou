using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Tenhou.Server.Rules.Common;

namespace Tenhou.Server.Rules.Sichuan;

public sealed record SichuanValue(int Fan, string[] Patterns);
public sealed class SichuanMatch : VariantMatch
{
    private readonly IReadOnlyDictionary<string, string> options;
    private readonly Dictionary<int, int[]> exchanges = [];
    private readonly List<(int Payer, int Recipient, int Amount)> kongLedger = [];
    private int lastKongSeat = -1, firstWinner = -1, nextDealer;
    public SichuanMatch(IReadOnlyDictionary<string, string> options, Random random) : base(random, 50000) => this.options = options;
    protected override int[] TileSet => Enumerable.Range(0, 108).ToArray();
    protected override bool CanChow => false;
    protected override bool IsMcr => false;
    protected override int TotalHands => 8;
    protected override int SelectDealer() => nextDealer;
    protected override bool Participating(int seat) => Players.Length == 0 || Players[seat].Wins == 0;
    protected override bool ShouldEndMatch => options["bankruptcy"] == "1" && Points.Any(p => p < 0);
    private int Cap => int.Parse(options["fan_cap"], CultureInfo.InvariantCulture);
    // Seats advance counterclockwise in the common turn/claim mechanics.
    private int ExchangeDirection => ((HandNumber - 1) % 4) switch { 0 => 3, 1 => 1, 2 => 2, _ => 0 };
    protected override void AddState(Dictionary<string, string> state)
        => state["exchange_direction"] = ExchangeDirection switch { 3 => "clockwise", 1 => "counterclockwise", 2 => "opposite", _ => "none" };
    protected override void Begin()
    {
        exchanges.Clear(); kongLedger.Clear(); lastKongSeat = firstWinner = -1;
        Phase = ExchangeDirection == 0 ? "missing" : "exchange";
        History.Add(ExchangeDirection switch { 3 => "本局顺时针换三张", 1 => "本局逆时针换三张", 2 => "本局对侧换三张", _ => "本局不换三张" });
    }
    protected override IReadOnlyList<MatchAction>? SpecialActions(int seat)
    {
        if (Phase == "exchange")
        {
            if (exchanges.ContainsKey(seat)) return [];
            List<MatchAction> actions = [];
            foreach (IGrouping<int, int> suit in Players[seat].Hand.GroupBy(t => t / 36))
            {
                int[] ts = suit.ToArray();
                for (int i = 0; i < ts.Length; ++i) for (int j = i + 1; j < ts.Length; ++j) for (int k = j + 1; k < ts.Length; ++k)
                    actions.Add(new("exchange", [ts[i], ts[j], ts[k]]));
            }
            return actions;
        }
        if (Phase == "missing") return Players[seat].MissingSuit >= 0 ? [] : [new("dingque", [0]), new("dingque", [1]), new("dingque", [2])];
        return null;
    }
    protected override void SpecialSubmit(int seat, MatchAction action)
    {
        if (Phase == "exchange")
        {
            exchanges[seat] = action.Tiles;
            if (exchanges.Count == 4)
            {
                for (int s = 0; s < 4; ++s) foreach (int tile in exchanges[s]) Players[s].Hand.Remove(tile);
                for (int s = 0; s < 4; ++s) Players[(s + ExchangeDirection) % 4].Hand.AddRange(exchanges[s]);
                foreach (VariantPlayer p in Players) p.Hand.Sort(); Phase = "missing";
                History.Add("换三张完成");
            }
        }
        else
        {
            Players[seat].MissingSuit = action.Tiles[0];
            if (Players.All(p => p.MissingSuit >= 0)) Draw(Dealer);
        }
    }
    protected override bool CanDiscard(int seat, int tile)
    {
        VariantPlayer p = Players[seat];
        return !p.Hand.Any(t => t / 36 == p.MissingSuit) || tile / 36 == p.MissingSuit;
    }
    protected override bool CanCall(int seat, int tile) => Participating(seat) && tile / 36 != Players[seat].MissingSuit;
    protected override bool CanKong(int seat, MatchAction action) => Participating(seat) && action.Tiles[0] / 36 != Players[seat].MissingSuit;
    private int Capped(int fan) => Cap == 0 ? fan : Math.Min(Cap, fan);
    private static int Payment(int fan) => 1000 * (1 << (fan - 1));
    public SichuanValue? Score(int seat, int tile, bool selfDrawn, bool special = true)
    {
        VariantPlayer p = Players[seat]; List<int> hand = [.. p.Hand]; if (!selfDrawn) hand.Add(tile);
        if (hand.Any(t => t / 36 == p.MissingSuit) || p.Melds.Any(m => m.Tiles.Any(t => t / 36 == p.MissingSuit))) return null;
        int[] allTiles = hand.Concat(p.Melds.SelectMany(m => m.Tiles)).ToArray();
        if (allTiles.Any(t => t < 0 || t >= 108) || allTiles.GroupBy(t => t / 4).Any(g => g.Count() > 4)) return null;
        int[] c = TileMath.Counts(hand);
        if (c.Sum() != 14 - 3 * p.Melds.Count || !TileMath.Complete(c)) return null;
        int[] all = TileMath.Counts(allTiles);
        bool pure = allTiles.Select(t => t / 36).Distinct().Count() == 1;
        int roots = all.Count(n => n == 4);
        List<(int Fan, string Name, int IncludedRoots)> forms = [(1, "底和", 0)];
        if (pure) forms.Add((3, "清一色", 0));
        if (TileMath.SevenPairs(c) && p.Melds.Count == 0)
        {
            forms.Add((pure ? 5 : 3, pure ? "清七对" : "七对", 0));
            if (roots > 0) forms.Add((pure ? 6 : 5, pure ? "清龙七对" : "龙七对", 1));
        }
        foreach (List<ShapeBlock> concealed in TileMath.Shapes(c))
        {
            List<ShapeBlock> shape = [.. concealed, .. p.Melds.Select(m => new ShapeBlock(m.Tiles[0] / 4, m.Tiles.Length, false, m.Open))];
            bool pung = shape.All(b => !b.Chow);
            if (pung) forms.Add((pure ? 4 : 2, pure ? "清对" : "对对和", 0));
            if (shape.All(b => b.Kinds.Any(k => k % 9 is 0 or 8))) forms.Add((pure ? 5 : 3, pure ? "清幺九" : "带幺九", 0));
            if (pung && shape.All(b => b.Tile % 9 is 1 or 4 or 7)) forms.Add((4, "将对", 0));
            if (p.Melds.Count == 4) forms.Add((pure ? 5 : 3, pure ? "清金钩钓" : "金钩钓", 0));
        }
        if (special && selfDrawn && !Interrupted && firstWinner < 0 && p.River.Count == 0 && p.Draws == 1)
            forms.Add((6, seat == Dealer ? "天和" : "地和", 0));
        // Choose one complete interpretation, then add only its unaccounted roots.
        var best = forms.OrderByDescending(f => f.Fan + roots - f.IncludedRoots).ThenByDescending(f => f.Fan).First();
        int fan = best.Fan + roots - best.IncludedRoots;
        List<string> patterns = [best.Name];
        if (roots > best.IncludedRoots) patterns.Add("根×" + (roots - best.IncludedRoots));
        if (special)
        {
            if (selfDrawn && Replacement) { ++fan; patterns.Add("杠上花"); }
            if (!selfDrawn && ClaimKind == "added_kan") { ++fan; patterns.Add("抢杠"); }
            if (!selfDrawn && lastKongSeat == LastActor && ClaimKind == "discard") { ++fan; patterns.Add("杠上炮"); }
        }
        return new(Capped(fan), patterns.ToArray());
    }
    protected override bool CanWin(int seat, int tile, bool selfDrawn, bool aboutKong) => Participating(seat) && Score(seat, tile, selfDrawn) is not null;
    protected override void Win(int[] winners, int source, bool selfDrawn)
    {
        int seat = winners[0], tile = selfDrawn ? Players[source].Drawn : LastTile;
        SichuanValue value = Score(seat, tile, selfDrawn) ?? throw new InvalidOperationException("Invalid Sichuan win");
        int amount = Payment(value.Fan);
        int[] before = (int[])Points.Clone();
        if (selfDrawn) { for (int payer = 0; payer < 4; ++payer) if (payer != seat && Participating(payer)) Transfer(payer, seat, amount); }
        else Transfer(source, seat, amount);
        AddReceipt(seat, source, tile, value.Fan, value.Patterns, before, "");
        Cue(selfDrawn ? "tsumo" : "ron", seat, tile, source);
        if (firstWinner < 0) firstWinner = nextDealer = seat;
        ++Players[seat].Wins;
        if (selfDrawn) Players[seat].Hand.Remove(tile);
        else if (ClaimKind == "discard") Players[source].Called.Add(tile);
        else Players[source].WinningTiles.Remove(tile);
        Players[seat].WinningTiles.Add(tile); Players[seat].Drawn = -1;
        History.Add($"座位{seat + 1} {(selfDrawn ? "自摸" : "荣和")} {value.Fan}翻，{string.Join('、', value.Patterns)}，每位付款者{amount}分；本手退场");
        lastKongSeat = -1;
        if (Players.Count(p => p.Wins > 0) == 3 || ShouldEndMatch)
        {
            Result = ShouldEndMatch ? "击飞终局" : "三家已和，本局结束";
            History.Add(Result); FinishHand(); return;
        }
        Draw((source + 1) % 4);
    }
    protected override void KongPayment(int seat, int source, string kind)
    {
        int income = 0;
        for (int payer = 0; payer < 4; ++payer)
        {
            if (payer == seat || !Participating(payer) || (kind == "open_kan" && payer != source)) continue;
            int amount = kind == "added_kan" ? 1000 : 2000;
            Transfer(payer, seat, amount); kongLedger.Add((payer, seat, amount)); income += amount;
        }
        lastKongSeat = seat; History.Add($"座位{seat + 1} 杠税收入{income}分");
    }
    protected override void AfterDiscardClaims() => lastKongSeat = -1;
    protected override void Exhausted()
    {
        int[] before = (int[])Points.Clone();
        int[] active = Enumerable.Range(0, 4).Where(Participating).ToArray();
        bool[] pigs = Players.Select(p => p.Hand.Concat(p.Melds.SelectMany(m => m.Tiles)).Any(t => t / 36 == p.MissingSuit)).ToArray();
        int[] maximum = new int[4];
        foreach (int s in active)
        {
            if (pigs[s]) continue;
            foreach (int kind in Waits(Players[s].Hand, Players[s].Melds))
            {
                int tile = Enumerable.Range(kind * 4, 4).First(t => !Players[s].Hand.Contains(t) && !Players[s].Melds.Any(m => m.Tiles.Contains(t)));
                SichuanValue? value = Score(s, tile, false, false);
                if (value is not null) maximum[s] = Math.Max(maximum[s], Payment(value.Fan));
            }
        }
        foreach (int s in active)
        {
            if (pigs[s])
            {
                foreach (int other in active) if (s != other) Transfer(s, other, Payment(Cap == 0 ? 5 : Cap));
            }
            else if (maximum[s] == 0)
            {
                foreach (int other in active) if (maximum[other] > 0) Transfer(s, other, maximum[other]);
            }
        }
        foreach ((int payer, int recipient, int amount) in kongLedger)
            if (Participating(recipient) && maximum[recipient] == 0) Transfer(recipient, payer, amount);
        Result = "牌墙摸完，未和玩家已查花猪、查大叫；未听及花猪退税。";
        AddReceipt(-1, -1, -1, 0, [], before, Result);
        History.Add(Result); FinishHand();
    }
    private void AddReceipt(int seat, int source, int tile, int fan, string[] patterns, int[] before, string reason)
    {
        Dictionary<string, string> receipt = new()
        {
            ["who"] = seat.ToString(CultureInfo.InvariantCulture), ["from"] = source.ToString(CultureInfo.InvariantCulture),
            ["tile"] = tile.ToString(CultureInfo.InvariantCulture), ["fan"] = fan.ToString(CultureInfo.InvariantCulture),
            ["patterns"] = string.Join(';', patterns), ["reason"] = reason, ["base"] = string.Join(',', before),
            ["delta"] = string.Join(',', Points.Select((p, i) => p - before[i]))
        };
        if (seat >= 0)
        {
            VariantPlayer p = Players[seat];
            receipt["hand"] = string.Join(',', (seat == source ? p.Hand : p.Hand.Append(tile)).Order());
            receipt["melds"] = string.Join(';', p.Melds.Select(m => $"{m.Kind}:{string.Join(',', m.Tiles)}:{m.Source}:{m.Called}:{m.Added}"));
        }
        Receipts.Add(receipt);
    }
    public override MatchAction ChooseBotAction(int seat)
    {
        IReadOnlyList<MatchAction> actions = GetActions(seat);
        if (Phase == "exchange") return actions.OrderBy(a => Players[seat].Hand.Count(t => t / 36 == a.Tiles[0] / 36)).ThenBy(a => a.Tiles.Sum()).First();
        if (Phase == "missing") return actions.OrderBy(a => Players[seat].Hand.Count(t => t / 36 == a.Tiles[0])).First();
        return base.ChooseBotAction(seat);
    }
}
