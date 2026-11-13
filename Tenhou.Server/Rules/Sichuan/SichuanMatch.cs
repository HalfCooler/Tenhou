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
    private int lastKongSeat = -1, lastKongIncome;
    public SichuanMatch(IReadOnlyDictionary<string, string> options, Random random) : base(random, 1000) => this.options = options;
    protected override int[] TileSet => Enumerable.Range(0, 108).ToArray();
    protected override bool CanChow => false;
    protected override bool IsMcr => false;
    protected override int TotalHands => int.Parse(options["hands"], CultureInfo.InvariantCulture);
    private int Basis => int.Parse(options["base_points"], CultureInfo.InvariantCulture);
    private int Cap => int.Parse(options["fan_cap"], CultureInfo.InvariantCulture);
    protected override void Begin()
    {
        exchanges.Clear(); kongLedger.Clear(); lastKongSeat = -1; lastKongIncome = 0;
        Phase = options["exchange"] == "1" ? "exchange" : "missing";
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
                int direction = WallLog[^1][0] % 3 + 1;
                for (int s = 0; s < 4; ++s) foreach (int tile in exchanges[s]) Players[s].Hand.Remove(tile);
                for (int s = 0; s < 4; ++s) Players[(s + direction) % 4].Hand.AddRange(exchanges[s]);
                foreach (VariantPlayer p in Players) p.Hand.Sort(); Phase = "missing";
                History.Add("换三张完成：" + (direction == 1 ? "传下家" : direction == 2 ? "传对家" : "传上家"));
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
        if (p.Hand.Any(t => t / 36 == p.MissingSuit)) return tile / 36 == p.MissingSuit;
        return base.CanDiscard(seat, tile);
    }
    protected override bool CanCall(int seat, int tile) => tile / 36 != Players[seat].MissingSuit && base.CanCall(seat, tile);
    protected override bool CanKong(int seat, MatchAction action) => action.Tiles[0] / 36 != Players[seat].MissingSuit && base.CanKong(seat, action);
    protected override bool MustWin(int seat) => options["last_four_must_win"] == "1" && Wall.Count < 4;
    public SichuanValue? Score(int seat, int tile, bool selfDrawn, bool special = true)
    {
        VariantPlayer p = Players[seat]; List<int> hand = [.. p.Hand]; if (!selfDrawn) hand.Add(tile);
        if (hand.Any(t => t / 36 == p.MissingSuit) || p.Melds.Any(m => m.Tiles.Any(t => t / 36 == p.MissingSuit))) return null;
        int[] c = TileMath.Counts(hand);
        if (c.Sum() != 14 - 3 * p.Melds.Count || !TileMath.Complete(c)) return null;
        int[] allTiles = hand.Concat(p.Melds.SelectMany(m => m.Tiles)).ToArray();
        if (allTiles.GroupBy(t => t / 4).Any(g => g.Count() > 4)) return null;
        int[] all = TileMath.Counts(allTiles);
        List<(int Fan, string Name)> bestForm = [];
        if (TileMath.SevenPairs(c) && p.Melds.Count == 0) bestForm.Add((2, "七对"));
        List<List<ShapeBlock>> shapes = TileMath.Shapes(c).Select(s => s.Concat(p.Melds.Select(m => new ShapeBlock(m.Tiles[0] / 4, m.Tiles.Length, false, m.Open))).ToList()).ToList();
        foreach (List<ShapeBlock> shape in shapes)
        {
            List<(int Fan, string Name)> form = [];
            if (shape.All(b => !b.Chow)) form.Add((1, "碰碰胡"));
            if (shape.All(b => b.Kinds.Any(k => k % 9 is 0 or 8))) form.Add((2, "带幺九"));
            if (shape.All(b => !b.Chow && b.Tile % 9 is 1 or 4 or 7)) form.Add((3, "将对"));
            if (p.Melds.Count == 4) form.Add((1, "金钩钓"));
            if (form.Sum(f => f.Fan) > bestForm.Sum(f => f.Fan)) bestForm = form;
        }
        List<(int Fan, string Name)> patterns = [.. bestForm];
        if (Enumerable.Range(0, 27).Where(k => all[k] > 0).Select(k => k / 9).Distinct().Count() == 1) patterns.Add((2, "清一色"));
        int roots = all.Count(n => n == 4); if (roots > 0) patterns.Add((roots, "根×" + roots));
        if (options["menqing_zhongzhang"] == "1")
        {
            if (p.Melds.All(m => !m.Open)) patterns.Add((1, "门清"));
            if (Enumerable.Range(0, 27).All(k => all[k] == 0 || k % 9 is not (0 or 8))) patterns.Add((1, "中张"));
        }
        if (special)
        {
            if (selfDrawn && Replacement) patterns.Add((1, "杠上花"));
            if (!selfDrawn && ClaimKind == "added_kan") patterns.Add((1, "抢杠胡"));
            if (!selfDrawn && lastKongSeat == LastActor && ClaimKind == "discard") patterns.Add((1, "杠上炮"));
            if (Wall.Count == 0) patterns.Add((1, "海底"));
            if (selfDrawn && !Interrupted && p.River.Count == 0 && p.Draws == 1) patterns.Add((3, seat == Dealer ? "天胡" : "地胡"));
            if (selfDrawn && options["self_draw"] == "add_fan") patterns.Add((1, "自摸加番"));
        }
        return new(Math.Min(Cap, patterns.Sum(p => p.Fan)), patterns.Count == 0 ? ["平胡"] : patterns.Select(p => p.Name).ToArray());
    }
    protected override bool CanWin(int seat, int tile, bool selfDrawn, bool aboutKong) => Score(seat, tile, selfDrawn) is not null;
    protected override void Win(int[] winners, int source, bool selfDrawn)
    {
        int tile = selfDrawn ? Players[source].Drawn : LastTile;
        foreach (int seat in winners)
        {
            SichuanValue value = Score(seat, tile, selfDrawn) ?? throw new InvalidOperationException("Invalid Sichuan win");
            int amount = Basis * (1 << value.Fan) + (selfDrawn && options["self_draw"] == "add_base" ? Basis : 0);
            if (selfDrawn) { for (int payer = 0; payer < 4; ++payer) if (payer != seat) Transfer(payer, seat, amount); }
            else Transfer(source, seat, amount);
            if (!selfDrawn && ClaimKind == "discard" && lastKongSeat == source && options["call_transfer"] == "1") Transfer(source, seat, lastKongIncome);
            Players[seat].Locked = true; ++Players[seat].Wins;
            History.Add($"座位{seat + 1} {(selfDrawn ? "自摸" : "胡牌")} {value.Fan}番，{string.Join('、', value.Patterns)}，每家{amount}分");
        }
        if (selfDrawn) { Players[source].Hand.Remove(tile); Players[source].WinningTiles.Add(tile); Players[source].Drawn = -1; }
        lastKongSeat = -1; lastKongIncome = 0; Draw((source + 1) % 4);
    }
    protected override void KongPayment(int seat, int source, string kind)
    {
        int income = 0;
        for (int payer = 0; payer < 4; ++payer)
        {
            if (payer == seat || (kind == "open_kan" && payer != source)) continue;
            int amount = Basis * (kind == "added_kan" ? 1 : 2); Transfer(payer, seat, amount);
            kongLedger.Add((payer, seat, amount)); income += amount;
        }
        lastKongSeat = seat; lastKongIncome = income; History.Add($"座位{seat + 1} 杠牌收入{income}分");
    }
    protected override void AfterDiscardClaims() { lastKongSeat = -1; lastKongIncome = 0; }
    protected override void Exhausted()
    {
        bool[] pigs = Players.Select(p => p.Hand.Any(t => t / 36 == p.MissingSuit)).ToArray(); int[] maximum = new int[4];
        for (int s = 0; s < 4; ++s)
        {
            if (pigs[s]) continue;
            foreach (int kind in Waits(Players[s].Hand, Players[s].Melds))
            {
                int tile = Enumerable.Range(kind * 4, 4).First(t => !Players[s].Hand.Contains(t));
                SichuanValue? value = Score(s, tile, false, false);
                if (value is not null) maximum[s] = Math.Max(maximum[s], Basis * (1 << value.Fan));
            }
        }
        for (int s = 0; s < 4; ++s)
        {
            if (pigs[s]) { for (int other = 0; other < 4; ++other) if (!pigs[other]) Transfer(s, other, Basis * (1 << Cap)); }
            else if (maximum[s] == 0) { for (int other = 0; other < 4; ++other) if (maximum[other] > 0) Transfer(s, other, maximum[other]); }
        }
        foreach ((int payer, int recipient, int amount) in kongLedger)
            if (maximum[recipient] == 0) Transfer(recipient, payer, amount);
        Result = "牌墙摸完，已查花猪、查大叫并退还未听牌者杠分。";
        History.Add(Result); FinishHand();
    }
    public override MatchAction ChooseBotAction(int seat)
    {
        IReadOnlyList<MatchAction> actions = GetActions(seat);
        if (Phase == "exchange") return actions.OrderBy(a => Players[seat].Hand.Count(t => t / 36 == a.Tiles[0] / 36)).ThenBy(a => a.Tiles.Sum()).First();
        if (Phase == "missing") return actions.OrderBy(a => Players[seat].Hand.Count(t => t / 36 == a.Tiles[0])).First();
        return base.ChooseBotAction(seat);
    }
}
