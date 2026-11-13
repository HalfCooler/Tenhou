using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Tenhou.Server.Rules.Common;

namespace Tenhou.Server.Rules.Guobiao;

public sealed class GuobiaoMatch : VariantMatch
{
    private readonly IReadOnlyDictionary<string, string> options;
    public GuobiaoMatch(IReadOnlyDictionary<string, string> options, Random random) : base(random, 0) => this.options = options;
    protected override int[] TileSet => Enumerable.Range(0, options["flowers"] == "1" ? 144 : 136).ToArray();
    protected override bool CanChow => true;
    protected override bool IsMcr => true;
    protected override int TotalHands => int.Parse(options["hands"], CultureInfo.InvariantCulture);
    public McrScore Score(int seat, int tile, bool selfDrawn, bool aboutKong)
    {
        VariantPlayer p = Players[seat]; List<int> hand = [.. p.Hand]; if (!selfDrawn) hand.Add(tile);
        int visible = Players.Sum(q => q.River.Count(t => t / 4 == tile / 4 && !q.Called.Contains(t))
            + q.Melds.Where(m => m.Open).Sum(m => m.Tiles.Count(t => t / 4 == tile / 4)));
        McrContext context = new(selfDrawn, Wall.Count == 0, aboutKong,
            visible - (!selfDrawn && ClaimKind == "discard" ? 1 : 0) == 3,
            27 + (seat - Dealer + 4) % 4, 27 + (HandNumber - 1) / 4, p.Flowers.Count);
        return McrScoring.Evaluate(hand, p.Melds, tile, context);
    }
    protected override bool CanWin(int seat, int tile, bool selfDrawn, bool aboutKong)
        => Score(seat, tile, selfDrawn, aboutKong).QualifyingPoints >= 8;
    protected override void Win(int[] winners, int source, bool selfDrawn)
    {
        int seat = winners[0], tile = selfDrawn ? Players[seat].Drawn : LastTile;
        McrScore score = Score(seat, tile, selfDrawn, selfDrawn ? Replacement : ClaimKind == "added_kan");
        if (score.QualifyingPoints < 8) throw new InvalidOperationException("Under-eight-fan win");
        for (int payer = 0; payer < 4; ++payer)
            if (payer != seat) Transfer(payer, seat, 8 + (selfDrawn || payer == source ? score.Points : 0));
        ++Players[seat].Wins;
        Result = $"座位{seat + 1} {(selfDrawn ? "自摸" : "和牌")} {score.Points}番：" + string.Join("、", score.Entries.Select(e => e.Name + (e.Count > 1 ? "×" + e.Count : "")));
        History.Add(Result); FinishHand();
    }
}
