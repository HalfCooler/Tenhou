using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using static Tenhou.Server.Rules.Riichi.RiichiOptions;
using static Tenhou.Server.Rules.Riichi.HandEvaluator;

namespace Tenhou.Server.Rules.Riichi;

public sealed partial class RiichiMatch : IMatch
{
    private readonly Random random;
    private readonly HandEvaluator evaluator = new();
    private readonly List<MatchEvent> events = [];
    private readonly List<int[]> walls = [];
    private readonly List<int> live = [], dead = [], sealedTiles = [];
    private readonly Dictionary<int, List<MatchAction>> claims = [];
    private readonly Dictionary<int, MatchAction> responses = [];
    private int lastActor = -1, lastTile = -1, pendingRiichi = -1, pendingDora, doraCount = 1,
        replacements, nextRound, nextHonba;
    private bool rinshan, interrupted, kanDiscard, kanFlip;
    private string claimKind = "discard";
    private readonly int[] scores = new int[4];
    public RiichiOptions Options { get; }
    public Player[] Players { get; private set; } = Enumerable.Range(0, 4).Select(_ => new Player()).ToArray();
    public int Round { get; private set; }
    public int Honba { get; private set; }
    public int Sticks { get; private set; }
    public int Dealer { get; private set; }
    public int Turn { get; private set; }
    public int HandNumber { get; private set; }
    public string Phase { get; private set; } = "new";
    public int PlayerCount => Options.Players;
    public bool HandEnded => Phase == "ended";
    public bool IsComplete => Phase == "complete";
    public IReadOnlyList<int> Scores => scores;
    public IReadOnlyList<MatchEvent> Events => events;
    public IReadOnlyList<int[]> Walls => walls;
    public RiichiMatch(RiichiOptions options, Random random)
    {
        Options = options; this.random = random;
        for (int s = 0; s < PlayerCount; ++s) scores[s] = options.Starting;
    }
    private void Emit(string kind, Dictionary<string, string>? data = null, Dictionary<int, int>? hints = null)
        => events.Add(new(kind, data ?? [], hints ?? []));
    public List<int> Dora(bool ura = false) => Enumerable.Range(0, doraCount)
        .Select(i => dead[Options.ReplacementLimit + 2 * i + (ura ? 1 : 0)]).ToList();
    private int[] TileSet() => Enumerable.Range(0, 136).Where(t => PlayerCount != 3 || t / 4 is < 1 or > 7).ToArray();
    public void Start(int[]? wall = null)
    {
        if (Phase is not ("new" or "ended")) throw new InvalidOperationException("Invalid start phase");
        if (Phase == "ended") { Round = nextRound; Honba = nextHonba; }
        int[] tiles = TileSet();
        if (wall is not null)
        {
            if (!wall.Order().SequenceEqual(tiles)) throw new ArgumentException("Invalid physical wall");
            tiles = (int[])wall.Clone();
        }
        else random.Shuffle(tiles);
        walls.Add((int[])tiles.Clone());
        Dealer = Round % PlayerCount;
        Players = Enumerable.Range(0, 4).Select(_ => new Player()).ToArray();
        Dictionary<string, string> attrs = [];
        for (int i = 0; i < 13 * PlayerCount; ++i) Players[(Dealer + i % PlayerCount) % PlayerCount].Hand.Add(tiles[i]);
        for (int s = 0; s < PlayerCount; ++s)
        {
            attrs["hai" + s] = List(Players[s].Hand); Players[s].Hand.Sort();
        }
        int split = tiles.Length - (Options.ReplacementLimit + 10);
        live.Clear(); live.AddRange(tiles[ (13 * PlayerCount)..split ]);
        dead.Clear(); dead.AddRange(tiles[split..]); sealedTiles.Clear();
        replacements = pendingDora = 0; doraCount = 1; interrupted = kanDiscard = false;
        pendingRiichi = -1; claims.Clear(); responses.Clear(); ++HandNumber;
        // Dice are cosmetic. Derive from the persisted wall so recovery is exact.
        attrs["seed"] = List([(Round / PlayerCount * 4 + Round % PlayerCount) % 16, Honba, Sticks, tiles[0] % 6, tiles[1] % 6, Dora()[0]]);
        attrs["ten"] = List(scores.Select(v => v / 100)); attrs["oya"] = Text(Dealer);
        Emit("INIT", attrs); Draw(Dealer); Validate();
    }
    private void Draw(int seat, bool replacement = false, bool nuki = false)
    {
        if (live.Count == 0) { EndDraw(); return; }
        int tile;
        if (replacement)
        {
            if (replacements >= Options.ReplacementLimit) throw new InvalidOperationException("Replacement exhausted");
            tile = dead[replacements++]; sealedTiles.Add(live[^1]); live.RemoveAt(live.Count - 1);
        }
        else { tile = live[0]; live.RemoveAt(0); }
        Player p = Players[seat]; p.Hand.Add(tile); p.Hand.Sort(); p.Drawn = tile; ++p.Draws;
        p.TemporaryFuriten = false; p.Forbidden.Clear(); Turn = seat; Phase = "turn";
        rinshan = replacement; kanDiscard = replacement && !nuki;
        int hint = 0;
        foreach (MatchAction a in GetActions(seat)) hint |= a.Kind switch
        {
            "tsumo" => 16, "riichi" => 32, "closed_kan" or "added_kan" => 2,
            "draw" => 64, "nuki" => 128, _ => 0
        };
        Emit("TUVW"[seat] + Text(tile), hints: new() { [seat] = hint });
    }
    public HandValue Value(int seat, int tile, bool tsumo = false, bool chankan = false, bool closedRob = false)
    {
        Player p = Players[seat]; List<int> hand = [.. p.Hand];
        if (!tsumo)
        {
            int[] waits = evaluator.Waits(hand, PlayerCount);
            if (p.TemporaryFuriten || p.RiichiFuriten || p.River.Any(t => waits.Contains(t / 4))) return new();
            hand.Add(tile);
        }
        bool first = p.Draws == 1 && p.River.Count == 0 && !interrupted;
        WinContext context = new()
        {
            Tsumo = tsumo, Dealer = seat == Dealer, SeatWind = 27 + (seat - Dealer + PlayerCount) % PlayerCount,
            RoundWind = 27 + Round / PlayerCount % 4, Riichi = p.Riichi, DoubleReach = p.DoubleReach, Ippatsu = p.Ippatsu,
            Rinshan = tsumo && rinshan, Chankan = chankan && !closedRob && claimKind != "nuki",
            Haitei = tsumo && live.Count == 0 && !rinshan, Houtei = !tsumo && !chankan && live.Count == 0 && !rinshan,
            Tenhou = tsumo && first && seat == Dealer, Chiihou = tsumo && first && seat != Dealer,
            Dora = Dora(), Ura = p.Riichi ? Dora(true) : [], Nuki = [.. p.Nuki],
            Renhou = !tsumo && seat != Dealer && p.Draws == 0 && !interrupted,
            Swallow = !tsumo && claimKind == "discard" && pendingRiichi == lastActor,
            KanDiscard = !tsumo && claimKind == "discard" && kanDiscard
        };
        HandValue value = Evaluate(hand, p.Melds, tile, Options, context);
        return closedRob && !value.Yakuman.Contains(47) && !value.Yakuman.Contains(48) ? new() : value;
    }
    private static void Remove(List<int> hand, int tile)
    {
        if (!hand.Remove(tile)) throw new InvalidOperationException("Missing physical tile");
    }
    private int KanCount => Players.Sum(p => p.Melds.Count(m => m.Kind.Contains("kan", StringComparison.Ordinal)));
    private static HashSet<int> Kuikae(int called, int low)
    {
        HashSet<int> result = [called];
        if (called == low && low % 9 < 6) result.Add(low + 3);
        if (called == low + 2 && low % 9 > 0) result.Add(low - 1);
        return result;
    }
    public IReadOnlyList<MatchAction> GetActions(int seat)
    {
        if (seat < 0 || seat >= PlayerCount) return [];
        if (Phase == "claims") return claims.TryGetValue(seat, out List<MatchAction>? options) && !responses.ContainsKey(seat) ? options : [];
        if (Phase != "turn" || seat != Turn) return [];
        Player p = Players[seat]; List<MatchAction> result = [];
        foreach (int t in p.Hand)
            if ((!p.Riichi || t == p.Drawn) && !p.Forbidden.Contains(t / 4)) result.Add(new("discard", [t]));
        if (p.Drawn < 0) return result;
        if (Value(seat, p.Drawn, true).Han > 0) result.Add(new("tsumo"));
        if (p.River.Count == 0 && !interrupted && p.Hand.Select(t => t / 4).Where(Terminal).Distinct().Count() >= 9) result.Add(new("draw"));
        if (!p.Riichi && p.Closed && scores[seat] >= 1000 && live.Count >= PlayerCount)
            foreach (MatchAction a in result.ToArray())
                if (a.Kind == "discard")
                {
                    List<int> rest = [.. p.Hand]; Remove(rest, a.Tiles[0]);
                    if (evaluator.Waits(rest, PlayerCount).Length > 0) result.Add(new("riichi", a.Tiles));
                }
        if (PlayerCount == 3 && live.Count > 0 && replacements < Options.ReplacementLimit)
            foreach (int t in p.Hand)
                if (t / 4 == 30 && (!p.Riichi || t == p.Drawn)) result.Add(new("nuki", [t]));
        if (live.Count > 0 && replacements < Options.ReplacementLimit && KanCount < 4)
        {
            int[] c = Counts(p.Hand);
            for (int k = 0; k < 34; ++k)
            {
                if (c[k] == 0) continue;
                int[] same = p.Hand.Where(t => t / 4 == k).ToArray();
                if (c[k] == 4)
                {
                    bool allowed = !p.Riichi;
                    if (p.Riichi && p.Drawn / 4 == k)
                    {
                        List<int> before = [.. p.Hand], rest = [.. p.Hand]; Remove(before, p.Drawn);
                        foreach (int t in same) Remove(rest, t);
                        int[] waits = evaluator.Waits(before, PlayerCount);
                        allowed = waits.Length > 0 && waits.SequenceEqual(evaluator.Waits(rest, PlayerCount));
                    }
                    if (allowed) result.Add(new("closed_kan", same));
                }
                if (!p.Riichi && p.Melds.Any(m => m.Kind == "pon" && m.Tiles[0] / 4 == k))
                    foreach (int t in same) result.Add(new("added_kan", [t]));
            }
        }
        return result;
    }
    private static int Priority(MatchAction a) => a.Kind switch { "ron" => 0, "pon" or "open_kan" => 1, "chi" => 2, _ => 3 };
    public void Submit(int seat, MatchAction action)
    {
        IReadOnlyList<MatchAction> options = GetActions(seat);
        if (!options.Any(a => a.Equivalent(action))) throw new ArgumentException("Illegal or stale action");
        if (Phase == "claims")
        {
            responses[seat] = action;
            if (options.Any(a => a.Kind == "ron") && action.Kind != "ron")
            { Players[seat].TemporaryFuriten = true; Players[seat].RiichiFuriten = Players[seat].Riichi; }
            (int, int) Order(int s, MatchAction a) => (Priority(a), a.Kind == "ron" ? 0 : (s - lastActor + 4) % 4);
            (int, int) best = (4, 4);
            foreach ((int s, MatchAction a) in responses)
                if (a.Kind != "skip" && Order(s, a).CompareTo(best) < 0) best = Order(s, a);
            if (best.Item1 < 4)
                foreach (int s in claims.Keys.ToArray())
                    if (!responses.ContainsKey(s) && !claims[s].Any(a => a.Kind != "skip" && Order(s, a).CompareTo(best) <= 0)) claims.Remove(s);
            if (claims.Keys.All(responses.ContainsKey)) ResolveClaims();
        }
        else if (action.Kind is "discard" or "riichi") Discard(seat, action.Tiles[0], action.Kind == "riichi");
        else if (action.Kind == "tsumo") Win([seat], seat, true);
        else if (action.Kind == "draw") EndDraw("yao9");
        else if (action.Kind == "nuki")
        {
            Player p = Players[seat]; Remove(p.Hand, action.Tiles[0]); p.Nuki.Add(action.Tiles[0]); p.Drawn = -1;
            lastActor = seat; lastTile = action.Tiles[0]; claimKind = "nuki"; PrepareClaims();
            Emit("N", new() { ["who"] = Text(seat), ["m"] = Text((lastTile << 8) | 32) }, Hints());
            if (claims.Count == 0) { Interrupt(); Draw(seat, true, true); }
        }
        else Kan(seat, action);
        Validate();
    }
    private void Discard(int seat, int tile, bool reach)
    {
        Player p = Players[seat]; if (p.Riichi) p.Ippatsu = false;
        if (reach)
        {
            pendingRiichi = seat; p.DoubleReach = p.River.Count == 0 && !interrupted;
            Emit("REACH", new() { ["who"] = Text(seat), ["step"] = "1" });
        }
        bool tsumogiri = p.Drawn == tile;
        Remove(p.Hand, tile); p.River.Add(tile); p.Drawn = -1; p.Forbidden.Clear();
        lastActor = seat; lastTile = tile; claimKind = "discard"; PrepareClaims();
        char code = "DEFG"[seat]; if (tsumogiri) code = char.ToLowerInvariant(code);
        Emit(code + Text(tile), hints: Hints());
        if (claims.Count == 0) ResolveClaims();
    }
    private void PrepareClaims()
    {
        Phase = "claims"; claims.Clear(); responses.Clear(); int kind = lastTile / 4;
        for (int s = 0; s < PlayerCount; ++s)
        {
            if (s == lastActor) continue;
            Player p = Players[s]; List<MatchAction> options = [];
            if (Value(s, lastTile, chankan: claimKind != "discard", closedRob: claimKind == "closed_kan").Han > 0) options.Add(new("ron"));
            else if (claimKind == "discard" && evaluator.Waits(p.Hand, PlayerCount).Contains(kind)) p.TemporaryFuriten = true;
            if (claimKind == "discard" && !p.Riichi && live.Count > 0)
            {
                int[] same = p.Hand.Where(t => t / 4 == kind).ToArray();
                for (int i = 0; i < same.Length; ++i)
                    for (int j = i + 1; j < same.Length; ++j)
                        if (p.Hand.Any(t => t / 4 != kind)) options.Add(new("pon", [same[i], same[j]]));
                if (same.Length == 3 && replacements < Options.ReplacementLimit && KanCount < 4) options.Add(new("open_kan", same));
                if (PlayerCount == 4 && s == (lastActor + 1) % PlayerCount && kind < 27)
                    for (int low = Math.Max(kind / 9 * 9, kind - 2); low <= Math.Min(kind, kind / 9 * 9 + 6); ++low)
                    {
                        int[] need = Enumerable.Range(low, 3).Where(k => k != kind).ToArray();
                        foreach (int t in p.Hand.Where(t => t / 4 == need[0]))
                            foreach (int u in p.Hand.Where(t => t / 4 == need[1]))
                                if (p.Hand.Any(v => v != t && v != u && !Kuikae(kind, low).Contains(v / 4)))
                                    options.Add(new("chi", [Math.Min(t, u), Math.Max(t, u)]));
                    }
            }
            if (options.Count > 0) { options.Add(new("skip")); claims[s] = options; }
        }
    }
    private Dictionary<int, int> Hints() => claims.ToDictionary(e => e.Key, e => e.Value.Aggregate(0,
        (h, a) => h | (a.Kind switch { "ron" => 8, "pon" => 1, "open_kan" => 2, "chi" => 4, _ => 0 })));
    private void ResolveClaims()
    {
        int[] winners = responses.Where(e => e.Value.Kind == "ron").Select(e => e.Key).OrderBy(s => (s - lastActor + 4) % 4).ToArray();
        if (winners.Length == 3) { EndDraw("ron3"); return; }
        if (winners.Length > 0) { Win(winners, lastActor, false); return; }
        if (claimKind != "discard")
        {
            if (claimKind == "nuki") { Interrupt(); Draw(lastActor, true, true); } else FinishKan();
            return;
        }
        if (pendingRiichi >= 0)
        {
            int s = pendingRiichi; Players[s].Riichi = Players[s].Ippatsu = true; scores[s] -= 1000; ++Sticks;
            Emit("REACH", new() { ["who"] = Text(s), ["step"] = "2" }); pendingRiichi = -1;
        }
        while (pendingDora > 0) { Flip(); --pendingDora; }
        if (Players.Take(PlayerCount).All(p => p.Riichi)) { EndDraw("reach4"); return; }
        if (KanCount == 4 && Players.Max(p => p.Melds.Count(m => m.Kind.Contains("kan", StringComparison.Ordinal))) != 4) { EndDraw("kan4"); return; }
        if (PlayerCount == 4 && !interrupted && Players.All(p => p.River.Count == 1))
        {
            int k = Players[0].River[0] / 4;
            if (k is >= 27 and < 31 && Players.All(p => p.River[0] / 4 == k)) { EndDraw("kaze4"); return; }
        }
        KeyValuePair<int, MatchAction>[] calls = responses.Where(e => e.Value.Kind is "chi" or "pon" or "open_kan")
            .OrderBy(e => (Priority(e.Value), (e.Key - lastActor + 4) % 4)).ToArray();
        if (calls.Length > 0) Call(calls[0].Key, calls[0].Value); else Draw((lastActor + 1) % PlayerCount);
    }
    private void Interrupt() { interrupted = true; foreach (Player p in Players) p.Ippatsu = false; }
    private void Call(int seat, MatchAction action)
    {
        Player p = Players[seat]; foreach (int t in action.Tiles) Remove(p.Hand, t);
        Players[lastActor].Called.Add(lastTile);
        Meld meld = new() { Kind = action.Kind, Tiles = action.Tiles.Append(lastTile).Order().ToList(), Who = seat, Source = lastActor, Called = lastTile };
        p.Melds.Add(meld); Interrupt();
        if (meld.Opened && meld.Kind != "chi")
        {
            HashSet<int> kinds = p.Melds.Where(m => m.Kind != "chi").Select(m => m.Tiles[0] / 4).ToHashSet();
            if (meld.Tiles[0] / 4 >= 31 && new[] { 31, 32, 33 }.All(kinds.Contains)) p.Pao.TryAdd(39, lastActor);
            if (meld.Tiles[0] / 4 is >= 27 and < 31 && new[] { 27, 28, 29, 30 }.All(kinds.Contains)) p.Pao.TryAdd(49, lastActor);
        }
        Turn = seat; Phase = "turn"; rinshan = false; p.Drawn = -1;
        p.Forbidden = action.Kind == "pon" ? [lastTile / 4] : action.Kind == "chi" ? Kuikae(lastTile / 4, meld.Tiles[0] / 4) : [];
        Emit("N", new() { ["who"] = Text(seat), ["m"] = Text(meld.Encode()) });
        if (action.Kind == "open_kan") { ++pendingDora; Draw(seat, true); }
    }
    private void Kan(int seat, MatchAction action)
    {
        Player p = Players[seat]; Meld meld;
        if (action.Kind == "closed_kan")
        {
            meld = new() { Kind = action.Kind, Tiles = [.. action.Tiles], Who = seat, Source = seat };
            foreach (int t in action.Tiles) Remove(p.Hand, t);
            p.Melds.Add(meld); lastTile = action.Tiles.Contains(p.Drawn) ? p.Drawn : action.Tiles[0];
        }
        else
        {
            lastTile = action.Tiles[0]; meld = p.Melds.Single(m => m.Kind == "pon" && m.Tiles[0] / 4 == lastTile / 4);
            meld.Kind = action.Kind; meld.Tiles.Add(lastTile); meld.Tiles.Sort(); meld.Added = lastTile; Remove(p.Hand, lastTile);
        }
        p.Drawn = -1; lastActor = seat; claimKind = action.Kind; PrepareClaims();
        Emit("N", new() { ["who"] = Text(seat), ["m"] = Text(meld.Encode()) }, Hints());
        kanFlip = action.Kind == "closed_kan"; if (claims.Count == 0) FinishKan();
    }
    private void FinishKan()
    {
        Interrupt(); while (pendingDora > 0) { Flip(); --pendingDora; }
        if (kanFlip) Flip(); else ++pendingDora; Draw(lastActor, true);
    }
    private void Flip()
    {
        if (doraCount >= 5) throw new InvalidOperationException("Too many indicators");
        ++doraCount; Emit("DORA", new() { ["hai"] = Text(Dora()[^1]) });
    }
}
