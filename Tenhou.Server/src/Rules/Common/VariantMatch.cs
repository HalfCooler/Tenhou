using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace Tenhou.Server.Rules.Common;

public sealed class VariantPlayer
{
    public List<int> Hand { get; } = [];
    public List<int> River { get; } = [];
    public List<int> Flowers { get; } = [];
    public List<int> WinningTiles { get; } = [];
    public List<TileSet> Melds { get; } = [];
    public HashSet<int> Called { get; } = [];
    public int Drawn { get; set; } = -1;
    public int MissingSuit { get; set; } = -1;
    public bool Locked { get; set; }
    public int Wins { get; set; }
    public int Draws { get; set; }
}

// Shared tile mechanics, without riichi winds, dora, furiten or win semantics.
// Rules own win scoring, continuation, special phases and end-of-wall policy.
public abstract class VariantMatch : IMatch
{
    protected readonly Random Random;
    protected readonly List<int> Wall = [];
    protected readonly List<MatchEvent> EventLog = [];
    protected readonly List<int[]> WallLog = [];
    protected readonly Dictionary<int, List<MatchAction>> Claims = [];
    protected readonly Dictionary<int, MatchAction> Responses = [];
    protected readonly int[] Points = new int[4];
    protected int LastActor = -1, LastTile = -1;
    protected string ClaimKind = "discard";
    protected MatchAction? PendingKong;
    protected bool Replacement, Interrupted;
    protected string Result = "";
    protected readonly List<string> History = [];
    protected int WallFront, WallTail;
    protected readonly List<Dictionary<string, string>> Receipts = [];
    private readonly List<(long Seq, string Kind, int Actor, int Tile, int Source)> cues = [];
    private long cueSeq;
    protected void Cue(string kind, int actor, int tile = -1, int source = -1)
    {
        cues.Add((++cueSeq, kind, actor, tile, source));
        if (cues.Count > 32) cues.RemoveAt(0);
    }
    public VariantPlayer[] Players { get; protected set; } = [];
    public int Turn { get; protected set; }
    public int Dealer { get; protected set; }
    public int HandNumber { get; protected set; }
    public string Phase { get; protected set; } = "new";
    public int PlayerCount => 4;
    public bool HandEnded => Phase == "ended";
    public bool IsComplete => Phase == "complete";
    public IReadOnlyList<int> Scores => Points;
    public IReadOnlyList<MatchEvent> Events => EventLog;
    public IReadOnlyList<int[]> Walls => WallLog;
    protected abstract int[] TileSet { get; }
    protected abstract bool CanChow { get; }
    protected abstract bool IsMcr { get; }
    protected abstract int TotalHands { get; }
    protected virtual int SelectDealer() => (HandNumber - 1) % 4;
    protected virtual bool Participating(int seat) => true;
    protected virtual bool ShouldEndMatch => false;
    protected virtual void AddState(Dictionary<string, string> state) { }
    protected VariantMatch(Random random, int starting) { Random = random; Array.Fill(Points, starting); InitialTotal = starting * 4; }
    private int InitialTotal { get; }
    public void Start(int[]? wall = null)
    {
        if (Phase is not ("new" or "ended")) throw new InvalidOperationException("Invalid start phase");
        int[] tiles = TileSet;
        if (wall is not null)
        {
            if (!wall.Order().SequenceEqual(tiles)) throw new ArgumentException("Invalid wall"); tiles = (int[])wall.Clone();
        }
        else Random.Shuffle(tiles);
        WallLog.Add((int[])tiles.Clone()); Wall.Clear(); Wall.AddRange(tiles); WallFront = WallTail = 0; ++HandNumber; Dealer = SelectDealer(); Turn = Dealer;
        Players = Enumerable.Range(0, 4).Select(_ => new VariantPlayer()).ToArray();
        Claims.Clear(); Responses.Clear(); PendingKong = null; Replacement = Interrupted = false; Result = ""; History.Clear(); Receipts.Clear();
        LastActor = LastTile = -1;
        for (int i = 0; i < 52; ++i)
        {
            int seat = (Dealer + i % 4) % 4; int tile = TakeTile(seat, false);
            if (tile < 0) throw new InvalidOperationException("Wall exhausted while dealing"); Players[seat].Hand.Add(tile);
        }
        foreach (VariantPlayer player in Players) player.Hand.Sort();
        Begin(); Cue("start", Dealer); Publish(); Validate();
    }
    protected virtual void Begin() => Draw(Dealer);
    protected int TakeTile(int seat, bool replacement)
    {
        while (Wall.Count > 0)
        {
            int index = replacement ? Wall.Count - 1 : 0; int tile = Wall[index]; Wall.RemoveAt(index);
            if (replacement) ++WallTail; else ++WallFront;
            if (tile < 136) return tile;
            Players[seat].Flowers.Add(tile); replacement = true;
            Cue("flower", seat, tile);
        }
        return -1;
    }
    protected void Draw(int seat, bool replacement = false)
    {
        if (ShouldEndMatch) { Result = "击飞终局"; History.Add(Result); FinishHand(); return; }
        while (!Participating(seat)) seat = (seat + 1) % 4;
        int tile = TakeTile(seat, replacement);
        if (tile < 0) { Exhausted(); return; }
        VariantPlayer p = Players[seat]; p.Hand.Add(tile); p.Hand.Sort(); p.Drawn = tile; ++p.Draws;
        Turn = seat; Phase = "turn"; Replacement = replacement;
        Cue("draw", seat, tile);
    }
    protected abstract bool CanWin(int seat, int tile, bool selfDrawn, bool aboutKong);
    protected abstract void Win(int[] winners, int source, bool selfDrawn);
    protected virtual bool CanDiscard(int seat, int tile)
        => !Players[seat].Locked || tile == Players[seat].Drawn;
    protected virtual bool CanCall(int seat, int tile) => !Players[seat].Locked;
    protected virtual bool CanKong(int seat, MatchAction action)
    {
        if (!Players[seat].Locked) return true;
        if (!action.Tiles.Contains(Players[seat].Drawn)) return false;
        List<int> before = [.. Players[seat].Hand]; before.Remove(Players[seat].Drawn);
        List<int> after = [.. Players[seat].Hand]; foreach (int t in action.Tiles) after.Remove(t);
        List<TileSet> future = [.. Players[seat].Melds];
        if (action.Kind == "closed_kan") future.Add(new("closed_kan", action.Tiles, seat, seat));
        else
        {
            int index = future.FindIndex(m => m.Kind == "pon" && m.Tiles[0] / 4 == action.Tiles[0] / 4);
            future[index] = future[index] with { Kind = "added_kan", Tiles = future[index].Tiles.Concat(action.Tiles).ToArray() };
        }
        return Waits(before, Players[seat].Melds).SequenceEqual(Waits(after, future));
    }
    protected int[] Waits(List<int> hand, IEnumerable<TileSet>? melds = null)
    {
        int[] c = TileMath.Counts(hand), declared = TileMath.Counts((melds ?? []).SelectMany(m => m.Tiles)); List<int> result = [];
        for (int k = 0; k < (IsMcr ? 34 : 27); ++k)
        {
            if (c[k] + declared[k] >= 4) continue; ++c[k]; if (TileMath.Complete(c, IsMcr)) result.Add(k); --c[k];
        }
        return result.ToArray();
    }
    protected virtual IReadOnlyList<MatchAction>? SpecialActions(int seat) => null;
    protected virtual void SpecialSubmit(int seat, MatchAction action) => throw new ArgumentException("No special action");
    public IReadOnlyList<MatchAction> GetActions(int seat)
    {
        if (seat is < 0 or >= 4 || !Participating(seat)) return [];
        IReadOnlyList<MatchAction>? special = SpecialActions(seat); if (special is not null) return Unique(special);
        if (Phase == "claims") return !Responses.ContainsKey(seat) && Claims.TryGetValue(seat, out List<MatchAction>? options) ? Unique(options) : [];
        if (Phase != "turn" || seat != Turn) return [];
        VariantPlayer p = Players[seat]; List<MatchAction> actions = p.Hand.Where(t => CanDiscard(seat, t)).Select(t => new MatchAction("discard", [t])).ToList();
        if (p.Drawn < 0) return Unique(actions);
        if (CanWin(seat, p.Drawn, true, Replacement)) actions.Add(new("tsumo"));
        if (MustWin(seat) && actions.Any(a => a.Kind == "tsumo")) return [new("tsumo")];
        if (Wall.Count > 0)
        {
            foreach (IGrouping<int, int> same in p.Hand.GroupBy(t => t / 4))
            {
                if (same.Count() == 4)
                {
                    MatchAction kong = new("closed_kan", same.ToArray()); if (CanKong(seat, kong)) actions.Add(kong);
                }
                if (p.Melds.Any(m => m.Kind == "pon" && m.Tiles[0] / 4 == same.Key))
                    foreach (int tile in same)
                    { MatchAction kong = new("added_kan", [tile]); if (CanKong(seat, kong)) actions.Add(kong); }
            }
        }
        return Unique(actions);
    }
    private static IReadOnlyList<MatchAction> Unique(IEnumerable<MatchAction> actions) => actions
        .DistinctBy(a => (a.Kind, string.Join(',', a.Tiles.Select(t => a.Kind is "dingque" or "discard" or "exchange" ? t : t / 4)))).ToArray();
    public void Submit(int seat, MatchAction action)
    {
        if (!GetActions(seat).Any(a => a.Equivalent(action))) throw new ArgumentException("Illegal or stale action");
        if (Phase is not ("turn" or "claims")) { SpecialSubmit(seat, action); Cue(action.Kind, seat); }
        else if (Phase == "claims")
        {
            Responses[seat] = action;
            if (Claims.Keys.All(Responses.ContainsKey)) Resolve();
        }
        else if (action.Kind == "discard")
        {
            VariantPlayer p = Players[seat]; p.Hand.Remove(action.Tiles[0]); p.River.Add(action.Tiles[0]); p.Drawn = -1;
            LastActor = seat; LastTile = action.Tiles[0]; Cue("discard", seat, LastTile); ClaimKind = "discard"; PrepareClaims(); if (Claims.Count == 0) Resolve();
        }
        else if (action.Kind == "tsumo") Win([seat], seat, true);
        else if (action.Kind == "added_kan")
        {
            LastActor = seat; LastTile = action.Tiles[0]; ClaimKind = "added_kan"; PendingKong = action;
            PrepareClaims(); if (Claims.Count == 0) Resolve();
        }
        else CompleteKong(seat, action);
        Publish(); Validate();
    }
    protected virtual bool MustWin(int seat) => false;
    protected virtual void AfterDiscardClaims() { }
    private void PrepareClaims()
    {
        Phase = "claims"; Claims.Clear(); Responses.Clear(); int kind = LastTile / 4;
        for (int s = 0; s < 4; ++s)
        {
            if (s == LastActor || !Participating(s)) continue;
            VariantPlayer p = Players[s]; List<MatchAction> actions = [];
            if (CanWin(s, LastTile, false, ClaimKind == "added_kan")) actions.Add(new("ron"));
            if (ClaimKind == "discard" && CanCall(s, LastTile) && Wall.Count > 0)
            {
                int[] same = p.Hand.Where(t => t / 4 == kind).ToArray();
                for (int i = 0; i < same.Length; ++i) for (int j = i + 1; j < same.Length; ++j) actions.Add(new("pon", [same[i], same[j]]));
                if (same.Length == 3) actions.Add(new("open_kan", same));
                if (CanChow && s == (LastActor + 1) % 4 && kind < 27)
                    for (int low = Math.Max(kind - 2, kind / 9 * 9); low <= Math.Min(kind, kind / 9 * 9 + 6); ++low)
                    {
                        int[] need = Enumerable.Range(low, 3).Where(k => k != kind).ToArray();
                        foreach (int a in p.Hand.Where(t => t / 4 == need[0])) foreach (int b in p.Hand.Where(t => t / 4 == need[1])) actions.Add(new("chi", new[] { a, b }.Order().ToArray()));
                    }
            }
            if (MustWin(s) && actions.Any(a => a.Kind == "ron")) actions = [new("ron")];
            else if (actions.Count > 0) actions.Add(new("skip"));
            if (actions.Count > 0) Claims[s] = actions;
        }
    }
    private void Resolve()
    {
        int[] winners = Responses.Where(e => e.Value.Kind == "ron").Select(e => e.Key).OrderBy(s => (s - LastActor + 4) % 4).ToArray();
        if (winners.Length > 0)
        {
            if (ClaimKind == "added_kan")
            {
                Players[LastActor].Hand.Remove(LastTile); Players[LastActor].WinningTiles.Add(LastTile); Players[LastActor].Drawn = -1; PendingKong = null;
            }
            Win(winners.Take(1).ToArray(), LastActor, false); return;
        }
        if (ClaimKind == "added_kan") { CompleteKong(LastActor, PendingKong ?? throw new InvalidOperationException()); PendingKong = null; return; }
        AfterDiscardClaims();
        KeyValuePair<int, MatchAction>[] calls = Responses.Where(e => e.Value.Kind is "pon" or "open_kan" or "chi")
            .OrderBy(e => (e.Value.Kind == "chi" ? 1 : 0, (e.Key - LastActor + 4) % 4)).ToArray();
        if (calls.Length == 0) { Draw((LastActor + 1) % 4); return; }
        int seat = calls[0].Key; MatchAction action = calls[0].Value; VariantPlayer p = Players[seat];
        foreach (int t in action.Tiles) p.Hand.Remove(t);
        Players[LastActor].Called.Add(LastTile); p.Melds.Add(new(action.Kind, action.Tiles.Append(LastTile).Order().ToArray(), seat, LastActor, LastTile));
        Interrupted = true; Turn = seat; Phase = "turn"; Replacement = false; p.Drawn = -1;
        Cue(action.Kind, seat, LastTile, LastActor);
        if (action.Kind == "open_kan") { KongPayment(seat, LastActor, "open_kan"); Draw(seat, true); }
    }
    private void CompleteKong(int seat, MatchAction action)
    {
        VariantPlayer p = Players[seat]; foreach (int t in action.Tiles) p.Hand.Remove(t);
        if (action.Kind == "closed_kan") p.Melds.Add(new(action.Kind, action.Tiles, seat, seat));
        else
        {
            int index = p.Melds.FindIndex(m => m.Kind == "pon" && m.Tiles[0] / 4 == action.Tiles[0] / 4);
            TileSet old = p.Melds[index]; p.Melds[index] = old with { Kind = "added_kan", Tiles = old.Tiles.Concat(action.Tiles).Order().ToArray(), Added = action.Tiles[0] };
        }
        p.Drawn = -1; Interrupted = true; KongPayment(seat, seat, action.Kind);
        Cue(action.Kind, seat, action.Kind == "closed_kan" ? -1 : action.Tiles[0]); Draw(seat, true);
    }
    protected virtual void KongPayment(int seat, int source, string kind) { }
    protected virtual void Exhausted() { Result = "荒牌流局"; FinishHand(); }
    protected void FinishHand()
    {
        Phase = HandNumber >= TotalHands || ShouldEndMatch ? "complete" : "ended"; Claims.Clear(); Responses.Clear();
    }
    protected void Transfer(int payer, int recipient, int amount)
    {
        if (payer == recipient || amount < 0) throw new InvalidOperationException("Invalid transfer");
        Points[payer] -= amount; Points[recipient] += amount;
    }
    protected void Publish()
    {
        Dictionary<string, string> state = new()
        {
            ["phase"] = Phase, ["turn"] = Turn.ToString(CultureInfo.InvariantCulture), ["dealer"] = Dealer.ToString(CultureInfo.InvariantCulture),
            ["hand_number"] = HandNumber.ToString(CultureInfo.InvariantCulture), ["total_hands"] = TotalHands.ToString(CultureInfo.InvariantCulture),
            ["scores"] = string.Join(',', Points), ["wall"] = Wall.Count.ToString(CultureInfo.InvariantCulture), ["result"] = Result,
            ["history"] = string.Join("\n", History.TakeLast(12))
        };
        state["seq"] = (EventLog.Count + 1).ToString(CultureInfo.InvariantCulture);
        state["event_seq"] = cueSeq.ToString(CultureInfo.InvariantCulture);
        state["events"] = string.Join(';', cues.Select(e => $"{e.Seq}:{e.Kind}:{e.Actor}:{e.Tile}:{e.Source}"));
        state["receipt_count"] = Receipts.Count.ToString(CultureInfo.InvariantCulture);
        for (int i = 0; i < Receipts.Count; ++i)
            foreach (var field in Receipts[i]) state[$"receipt{i}_{field.Key}"] = field.Value;
        state["wall_total"] = TileSet.Length.ToString(CultureInfo.InvariantCulture);
        state["wall_front"] = WallFront.ToString(CultureInfo.InvariantCulture);
        state["wall_tail"] = WallTail.ToString(CultureInfo.InvariantCulture);
        state["wall_offset"] = "0";
        state["last_actor"] = LastActor.ToString(CultureInfo.InvariantCulture);
        state["last_tile"] = LastTile.ToString(CultureInfo.InvariantCulture);
        for (int s = 0; s < 4; ++s)
        {
            VariantPlayer p = Players[s]; state["hand" + s] = string.Join(',', p.Hand); state["river" + s] = string.Join(',', p.River.Where(t => !p.Called.Contains(t)));
            state["melds" + s] = string.Join(';', p.Melds.Select(m => m.Kind + ":" + string.Join(',', m.Tiles)));
            state["meld_info" + s] = string.Join(';', p.Melds.Select(m => $"{m.Source},{m.Called}"));
            state["meld_added" + s] = string.Join(',', p.Melds.Select(m => m.Added));
            state["drawn" + s] = p.Drawn.ToString(CultureInfo.InvariantCulture);
            state["flowers" + s] = string.Join(',', p.Flowers); state["winning" + s] = string.Join(',', p.WinningTiles);
            state["missing" + s] = p.MissingSuit.ToString(CultureInfo.InvariantCulture); state["wins" + s] = p.Wins.ToString(CultureInfo.InvariantCulture);
            state["retired" + s] = Participating(s) ? "0" : "1";
            state["actions" + s] = string.Join(';', GetActions(s).Select(a => a.Kind + ":" + string.Join(',', a.Tiles)));
        }
        AddState(state); EventLog.Add(new("MATCHVIEW", state));
    }
    public void Validate()
    {
        if (WallLog.Count == 0) return;
        int[] used = Wall.Concat(Players.SelectMany(p => p.Hand.Concat(p.Flowers).Concat(p.WinningTiles)
            .Concat(p.River.Where(t => !p.Called.Contains(t))).Concat(p.Melds.SelectMany(m => m.Tiles)))).Order().ToArray();
        if (!used.SequenceEqual(TileSet) || Points.Sum() != InitialTotal) throw new InvalidOperationException("Tile or point conservation failed");
    }
    public virtual MatchAction ChooseBotAction(int seat)
    {
        IReadOnlyList<MatchAction> actions = GetActions(seat);
        foreach (string kind in new[] { "ron", "tsumo", "closed_kan", "added_kan", "open_kan" })
        { MatchAction? action = actions.FirstOrDefault(a => a.Kind == kind); if (action is not null) return action; }
        MatchAction? skip = actions.FirstOrDefault(a => a.Kind == "skip"); if (skip is not null) return skip;
        if (actions.All(a => a.Kind != "discard")) return actions[0];
        return actions.Where(a => a.Kind == "discard").OrderBy(a => DiscardCost(seat, a.Tiles[0])).First();
    }
    private int DiscardCost(int seat, int tile)
    {
        int[] c = TileMath.Counts(Players[seat].Hand); int k = tile / 4;
        int cost = (c[k] - 1) * 4;
        if (k < 27)
            for (int step = -2; step <= 2; ++step)
            {
                int j = k + step; if (step == 0 || j < 0 || j >= 27 || j / 9 != k / 9) continue;
                cost += c[j] * (Math.Abs(step) == 1 ? 2 : 1);
            }
        return cost * 100 + tile;
    }
}
