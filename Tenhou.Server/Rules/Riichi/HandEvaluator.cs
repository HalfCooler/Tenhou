using System;
using System.Collections.Generic;
using System.Linq;

namespace Tenhou.Server.Rules.Riichi;

// The rules are ported from the repository's native evaluator. Caches belong to
// a match, so independent tables never share mutable scoring state.
public sealed class HandEvaluator
{
    private sealed record Block(int Kind, int Size = 3, bool Sequence = false, bool Opened = false);
    private readonly Dictionary<string, int> shantenCache = [];
    private readonly Dictionary<string, int[]> waitsCache = [];
    public static bool Terminal(int kind) => kind >= 27 || kind % 9 is 0 or 8;
    public static int[] Counts(IEnumerable<int> tiles)
    {
        int[] counts = new int[34];
        foreach (int tile in tiles)
        {
            if (tile is < 0 or >= 136) throw new ArgumentException("Invalid physical tile");
            if (++counts[tile / 4] > 4) throw new ArgumentException("Too many identical tiles");
        }
        return counts;
    }
    private static void Decompose(int[] c, List<Block> path, List<List<Block>> output)
    {
        int k = Array.FindIndex(c, v => v != 0);
        if (k == -1) { output.Add([.. path]); return; }
        if (c[k] >= 3)
        {
            c[k] -= 3; path.Add(new(k)); Decompose(c, path, output); path.RemoveAt(path.Count - 1); c[k] += 3;
        }
        if (k < 27 && k % 9 < 7 && c[k + 1] > 0 && c[k + 2] > 0)
        {
            --c[k]; --c[k + 1]; --c[k + 2]; path.Add(new(k, Sequence: true));
            Decompose(c, path, output); path.RemoveAt(path.Count - 1);
            ++c[k]; ++c[k + 1]; ++c[k + 2];
        }
    }
    private static List<List<Block>> Shapes(int[] input)
    {
        int[] c = (int[])input.Clone();
        List<List<Block>> output = [];
        for (int k = 0; k < 34; ++k)
            if (c[k] >= 2)
            {
                c[k] -= 2; Decompose(c, [new(k, 2)], output); c[k] += 2;
            }
        return output;
    }
    private static bool Orphans(int[] c) => Enumerable.Range(0, 34).All(k => Terminal(k) ? c[k] > 0 : c[k] == 0)
        && Enumerable.Range(0, 34).Count(k => Terminal(k) && c[k] >= 2) == 1;
    private static bool Seven(int[] c) => c.Count(v => v == 2) == 7;
    public static bool Complete(int[] c)
    {
        int total = c.Sum();
        return total % 3 == 2 && ((total == 14 && (Orphans(c) || Seven(c))) || Shapes(c).Count > 0);
    }
    private static string Key(int[] c) => new(c.Select(v => (char)v).ToArray());
    public int Shanten(int[] input)
    {
        string key = Key(input);
        if (shantenCache.TryGetValue(key, out int cached)) return cached;
        int total = input.Sum(), best = 8;
        int[] c = (int[])input.Clone();
        void Search(int at, int meld, int partial, int pair)
        {
            while (at < 34 && c[at] == 0) ++at;
            if (at == 34)
            {
                best = Math.Min(best, 8 - 2 * meld - Math.Min(partial, 4 - meld) - pair); return;
            }
            if (c[at] >= 3) { c[at] -= 3; Search(at, meld + 1, partial, pair); c[at] += 3; }
            if (at < 27 && at % 9 < 7 && c[at + 1] > 0 && c[at + 2] > 0)
            {
                --c[at]; --c[at + 1]; --c[at + 2]; Search(at, meld + 1, partial, pair);
                ++c[at]; ++c[at + 1]; ++c[at + 2];
            }
            if (c[at] >= 2)
            {
                c[at] -= 2;
                if (pair == 0) Search(at, meld, partial, 1);
                Search(at, meld, partial + 1, pair); c[at] += 2;
            }
            if (at < 27 && at % 9 < 8 && c[at + 1] > 0)
            {
                --c[at]; --c[at + 1]; Search(at, meld, partial + 1, pair); ++c[at]; ++c[at + 1];
            }
            if (at < 27 && at % 9 < 7 && c[at + 2] > 0)
            {
                --c[at]; --c[at + 2]; Search(at, meld, partial + 1, pair); ++c[at]; ++c[at + 2];
            }
            --c[at]; Search(at, meld, partial, pair); ++c[at];
        }
        Search(0, (14 - total) / 3, 0, 0);
        if (total >= 13)
        {
            int pairs = input.Count(v => v >= 2), distinct = input.Count(v => v > 0);
            int orphans = Enumerable.Range(0, 34).Count(k => Terminal(k) && input[k] > 0);
            int orphanPair = Enumerable.Range(0, 34).Any(k => Terminal(k) && input[k] >= 2) ? 1 : 0;
            best = Math.Min(best, Math.Min(6 - pairs + Math.Max(0, 7 - distinct), 13 - orphans - orphanPair));
        }
        if (shantenCache.Count >= 100000) shantenCache.Clear();
        shantenCache[key] = best;
        return best;
    }
    public int[] Waits(IReadOnlyCollection<int> hand, int players = 4)
    {
        if (hand.Count % 3 != 1) return [];
        int[] c = Counts(hand);
        string key = Key(c) + (char)players;
        if (waitsCache.TryGetValue(key, out int[]? cached)) return cached;
        List<int> result = [];
        for (int k = 0; k < 34; ++k)
        {
            if ((players == 3 && k is >= 1 and <= 7) || c[k] >= 4) continue;
            ++c[k]; if (Complete(c)) result.Add(k); --c[k];
        }
        if (waitsCache.Count >= 100000) waitsCache.Clear();
        int[] output = result.ToArray(); waitsCache[key] = output; return output;
    }
    private static int NextDora(int k, int n) => k < 27 ? (n == 3 && k == 0 ? 8 : k / 9 * 9 + (k % 9 + 1) % 9)
        : k < 31 ? 27 + (k - 27 + 1) % 4 : 31 + (k - 31 + 1) % 3;
    public static HandValue Payments(int han, int fu, bool dealer, bool tsumo, bool natural = false)
    {
        HandValue value = new() { Han = han, Fu = fu };
        if (han <= 0) return value;
        int basis;
        if (han >= 13) { basis = 8000 * (natural ? han / 13 : 1); value.Level = 5; }
        else if (han >= 11) { basis = 6000; value.Level = 4; }
        else if (han >= 8) { basis = 4000; value.Level = 3; }
        else if (han >= 6) { basis = 3000; value.Level = 2; }
        else
        {
            basis = Math.Min(2000, han >= 5 ? 2000 : fu * (1 << (han + 2)));
            if (basis == 2000) value.Level = 1;
        }
        static int Round(int v) => (v + 99) / 100 * 100;
        value.Main = Round(basis * (tsumo ? 2 : dealer ? 6 : 4));
        value.Additional = tsumo ? Round(basis * (dealer ? 2 : 1)) : 0;
        return value;
    }

    public static HandValue Evaluate(IReadOnlyList<int> hand, IReadOnlyList<Meld> melds, int tile, RiichiOptions cfg, WinContext x)
    {
        int[] c = Counts(hand);
        if (!Complete(c)) return new();
        bool closed = melds.All(m => !m.Opened);
        int[] tiles = hand.Concat(melds.SelectMany(m => m.Tiles)).ToArray();
        int[] all = Counts(tiles);
        bool chiitoi = melds.Count == 0 && Seven(c), orphan = melds.Count == 0 && Orphans(c);
        List<List<Block>> decompositions = Shapes(c);
        if (chiitoi) decompositions.Add(Enumerable.Range(0, 34).Where(k => c[k] == 2).Select(k => new Block(k, 2)).ToList());
        if (orphan) decompositions.Add([new(tile / 4, 2)]);
        HandValue best = new();
        foreach (List<Block> original in decompositions)
        {
            List<Block> shape = [.. original];
            int concealed = shape.Count;
            bool pairs = concealed == 7, isOrphan = orphan && concealed == 1;
            shape.AddRange(melds.Select(m => new Block(m.Tiles.Min() / 4, m.Tiles.Count, m.Kind == "chi", m.Opened)));
            for (int win = 0; win < concealed; ++win)
            {
                Block winning = shape[win];
                if (!(winning.Sequence ? tile / 4 >= winning.Kind && tile / 4 < winning.Kind + 3 : tile / 4 == winning.Kind)) continue;
                List<(int Id, int Han)> ys = [], yak = [];
                void Add(int id, int han) { if (han > 0) ys.Add((id, han)); }
                void Natural(int id, int units = 1) => yak.Add((id, units));
                void Erase(int id) => ys.RemoveAll(y => y.Id == id);
                int pair = -1, sequences = 0, triplets = 0, kans = 0, concealedSets = 0, extraFu = 0;
                HashSet<int> trips = [], seqs = [], suits = [];
                Dictionary<int, int> seqCount = [];
                bool allTerminal = true, hasHonor = false, hasTerminal = false, hasSimple = false;
                for (int k = 0; k < 34; ++k)
                    if (all[k] > 0)
                    {
                        if (k >= 27) hasHonor = true; else suits.Add(k / 9);
                        if (Terminal(k)) hasTerminal = true; else hasSimple = true;
                    }
                for (int j = 0; j < shape.Count; ++j)
                {
                    Block b = shape[j];
                    if (b.Size == 2) { pair = b.Kind; if (!Terminal(b.Kind)) allTerminal = false; continue; }
                    if (b.Sequence)
                    {
                        ++sequences; seqs.Add(b.Kind); seqCount[b.Kind] = seqCount.GetValueOrDefault(b.Kind) + 1;
                        if (b.Kind % 9 is not (0 or 6)) allTerminal = false;
                        continue;
                    }
                    ++triplets; trips.Add(b.Kind); if (b.Size == 4) ++kans;
                    bool open = b.Opened || (!x.Tsumo && j == win);
                    if (!open) ++concealedSets;
                    extraFu += (open ? 2 : 4) * (Terminal(b.Kind) ? 2 : 1) * (b.Size == 4 ? 4 : 1);
                    if (!Terminal(b.Kind)) allTerminal = false;
                }
                if (x.Tenhou) Natural(37);
                if (x.Chiihou) Natural(38);
                if (isOrphan)
                {
                    int[] before = (int[])c.Clone(); --before[tile / 4];
                    bool thirteen = Enumerable.Range(0, 34).Where(Terminal).All(k => before[k] == 1);
                    Natural(thirteen ? 48 : 47, thirteen ? 2 : 1);
                }
                else
                {
                    if (new[] { 31, 32, 33 }.All(trips.Contains)) Natural(39);
                    if (concealedSets == 4) Natural(winning.Size == 2 ? 41 : 40, winning.Size == 2 ? 2 : 1);
                    if (new[] { 27, 28, 29, 30 }.All(trips.Contains)) Natural(49, 2);
                    else if (pair is >= 27 and <= 30 && trips.Count(k => k is >= 27 and <= 30) == 3) Natural(50);
                    if (suits.Count == 0) Natural(42);
                    if (!hasHonor && !hasSimple) Natural(44);
                    if (tiles.All(t => t / 4 is 19 or 20 or 21 or 23 or 25 or 32)) Natural(43);
                    if (kans == 4) Natural(51);
                    if (closed && melds.Count == 0 && suits.Count == 1 && !hasHonor && tiles.Length == 14)
                    {
                        int offset = suits.Single() * 9;
                        bool gates = all[offset] >= 3 && all[offset + 8] >= 3 && Enumerable.Range(1, 7).All(k => all[offset + k] >= 1);
                        if (gates)
                        {
                            int[] before = (int[])all.Clone(); --before[tile / 4];
                            bool pure = before[offset] == 3 && before[offset + 8] == 3 && Enumerable.Range(1, 7).All(k => before[offset + k] == 1);
                            Natural(pure ? 46 : 45, pure ? 2 : 1);
                        }
                    }
                }
                if (!isOrphan)
                {
                    if (x.Tsumo && closed) Add(0, 1);
                    if (x.DoubleReach) Add(21, 2); else if (x.Riichi) Add(1, 1);
                    if (x.Ippatsu && (x.Riichi || x.DoubleReach)) Add(2, 1);
                    if (x.Chankan) Add(3, 1);
                    if (x.Rinshan) Add(4, 1);
                    if (x.Haitei) Add(5, 1);
                    if (x.Houtei) Add(6, 1);
                    if (!hasTerminal && (closed || cfg.OpenTanyao)) Add(8, 1);
                    if (pairs) Add(22, 2);
                    if (!pairs)
                    {
                        foreach (int k in trips)
                        {
                            if (k >= 31) Add(18 + k - 31, 1);
                            if (k == x.SeatWind) Add(10 + k - 27, 1);
                            if (k == x.RoundWind) Add(14 + k - 27, 1);
                        }
                        if (triplets == 4) Add(28, 2);
                        if (concealedSets == 3) Add(29, 2);
                        if (kans == 3) Add(27, 2);
                        if (pair >= 31 && trips.Count(k => k is >= 31 and <= 33) == 2) Add(30, 2);
                        int doubled = closed ? seqCount.Values.Sum(v => v / 2) : 0;
                        if (doubled >= 2) Add(32, 3); else if (doubled > 0) Add(9, 1);
                        if (Enumerable.Range(0, 9).Any(k => trips.Contains(k) && trips.Contains(k + 9) && trips.Contains(k + 18))) Add(26, 2);
                        if (Enumerable.Range(0, 7).Any(k => seqs.Contains(k) && seqs.Contains(k + 9) && seqs.Contains(k + 18))) Add(25, closed ? 2 : 1);
                        if (new[] { 0, 9, 18 }.Any(k => seqs.Contains(k) && seqs.Contains(k + 3) && seqs.Contains(k + 6))) Add(24, closed ? 2 : 1);
                        if (allTerminal && sequences > 0) Add(hasHonor ? 23 : 33, hasHonor ? (closed ? 2 : 1) : (closed ? 3 : 2));
                    }
                    if (!hasSimple) Add(31, 2);
                    if (suits.Count == 1) Add(hasHonor ? 34 : 35, hasHonor ? (closed ? 3 : 2) : (closed ? 6 : 5));
                }
                bool badWait = winning.Size == 2;
                if (winning.Sequence) badWait = tile / 4 == winning.Kind + 1 || (winning.Kind % 9 == 0 && tile / 4 == winning.Kind + 2)
                    || (winning.Kind % 9 == 6 && tile / 4 == winning.Kind);
                int pairFu = pair < 0 ? 0 : (pair >= 31 ? 2 : 0) + (pair == x.SeatWind ? 2 : 0) + (pair == x.RoundWind ? 2 : 0);
                bool pinfu = !pairs && !isOrphan && sequences == 4 && pairFu == 0 && !badWait;
                if (pinfu && closed) Add(7, 1);
                int fu = pairs ? 25 : 20 + extraFu + pairFu + (badWait ? 2 : 0);
                if (!pairs)
                {
                    if (x.Tsumo && !(pinfu && closed)) fu += 2;
                    if (!closed && fu == 20) fu += 2;
                    if (closed && !x.Tsumo) fu += 10;
                    fu = (fu + 9) / 10 * 10;
                }
                if (isOrphan) fu = 0;
                if (cfg.Ancient)
                {
                    if (x.Swallow) Add(100, 1);
                    if (x.KanDiscard) Add(101, 1);
                    if (melds.Count == 4 && melds.All(m => m.Opened)) Add(102, 1);
                    if (suits.Count == 3 && Enumerable.Range(27, 4).Any(k => all[k] > 0) && Enumerable.Range(31, 3).Any(k => all[k] > 0)) Add(103, 2);
                    if (trips.Any(k => k < 27 && k % 9 <= 6 && trips.Contains(k + 1) && trips.Contains(k + 2))) Add(104, 2);
                    if (seqCount.Values.Any(v => v >= 3)) { Erase(9); Add(105, closed ? 3 : 2); }
                    if (x.Haitei && tile / 4 == 9) { Erase(5); Add(106, 5); }
                    if (x.Houtei && tile / 4 == 17) { Erase(6); Add(107, 5); }
                    if (x.Renhou) Natural(36);
                    if (chiitoi)
                    {
                        for (int suit = 0; suit < 3; ++suit)
                            if (Enumerable.Range(0, 34).All(k => all[k] == (k >= suit * 9 + 1 && k <= suit * 9 + 7 ? 2 : 0)))
                                Natural(suit == 0 ? 111 : suit == 1 ? 109 : 110);
                        if (suits.Count == 0) { yak.RemoveAll(y => y.Id == 42); Natural(113, 2); }
                    }
                    if (x.DoubleReach && (x.Haitei || x.Houtei)) Natural(112);
                }
                int baseHan = ys.Sum(y => y.Han);
                if (yak.Count == 0 && baseHan == 0) continue;
                int han;
                if (yak.Count > 0) han = yak.Sum(y => 13 * y.Han);
                else
                {
                    int dora = x.Dora.Sum(d => all[NextDora(d / 4, cfg.Players)]);
                    int ura = x.Riichi || x.DoubleReach ? x.Ura.Sum(d => all[NextDora(d / 4, cfg.Players)]) : 0;
                    dora += x.Nuki.Count * (1 + x.Dora.Count(t => t / 4 == 29)
                        + (x.Riichi || x.DoubleReach ? x.Ura.Count(t => t / 4 == 29) : 0));
                    int red = tiles.Count(cfg.RedTiles.Contains);
                    Add(52, dora); Add(53, ura); Add(54, red);
                    han = baseHan + dora + ura + red;
                }
                HandValue value = Payments(han, fu, x.Dealer, x.Tsumo, yak.Count > 0);
                if (yak.Count == 0)
                {
                    if (cfg.Minimum is "1" or "2" or "4" && baseHan < RiichiOptions.Number(cfg.Minimum)) continue;
                    int limit = Array.IndexOf(new[] { "mangan", "haneman", "baiman", "sanbaiman", "yakuman" }, cfg.Minimum);
                    if (limit >= 0 && value.Level < limit + 1) continue;
                    foreach ((int id, int h) in ys) { value.Yaku.Add(id); value.Yaku.Add(h); }
                }
                value.Yakuman.AddRange(yak.Select(y => y.Id));
                static (int, int, int) Rank(HandValue v) => (v.Main + 2 * v.Additional, v.Han, v.Fu);
                if (Rank(value).CompareTo(Rank(best)) > 0) best = value;
            }
        }
        return best;
    }
}
