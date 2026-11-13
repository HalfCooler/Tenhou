using System;
using System.Collections.Generic;
using System.Linq;
using Tenhou.Server.Rules.Common;

namespace Tenhou.Server.Rules.Guobiao;

public sealed record McrContext(bool SelfDrawn = false, bool LastTile = false, bool AboutKong = false,
    bool FourthTile = false, int SeatWind = 27, int RoundWind = 27, int Flowers = 0);
public sealed record FanEntry(string Name, int Points, int Count);
public sealed record McrScore(FanEntry[] Entries)
{
    public int Points => Entries.Sum(e => e.Points * e.Count);
    public int QualifyingPoints => Entries.Where(e => e.Name != "花牌").Sum(e => e.Points * e.Count);
}

// Managed implementation of the 81 MCR fans, plus the optional 明暗杠 (5).
// Pattern classifications and exclusion rules follow GB-Mahjong (MIT).
// Set associations use an acyclic maximum-weight forest: a set may be reused
// to attach an unused set once, but may not close a scoring cycle.
public static class McrScoring
{
    private sealed record Candidate(string Name, int Mask = 0);
    public static McrScore Evaluate(IReadOnlyList<int> hand, IReadOnlyList<TileSet> melds, int winning, McrContext context)
    {
        int[] c = TileMath.Counts(hand);
        if (c.Sum() != 14 - 3 * melds.Count || !TileMath.Complete(c, true)) return new([]);
        int[] all = TileMath.Counts(hand.Concat(melds.SelectMany(m => m.Tiles)));
        List<(List<ShapeBlock> Blocks, int[]? Knitted, string? Special)> forms = [];
        foreach (List<ShapeBlock> shape in TileMath.Shapes(c)) forms.Add((shape, null, null));
        if (melds.Count == 0 && TileMath.SevenPairs(c))
        {
            int[] kinds = Enumerable.Range(0, 34).Where(k => c[k] > 0).ToArray();
            bool consecutive = kinds.Length == 7 && kinds[0] < 27 && kinds.All(k => k / 9 == kinds[0] / 9) && kinds[^1] - kinds[0] == 6;
            forms.Add(([], null, consecutive ? "连七对" : "七对"));
        }
        if (melds.Count == 0 && TileMath.Orphans(c)) forms.Add(([], null, "十三幺"));
        if (melds.Count == 0 && TileMath.Unconnected(c)) forms.Add(([], null, Enumerable.Range(27, 7).All(k => c[k] > 0) ? "七星不靠" : "全不靠"));
        if (melds.Count <= 1)
            foreach (int[] pattern in TileMath.Knitted.Where(p => p.All(k => c[k] > 0)))
            {
                int[] rest = (int[])c.Clone(); foreach (int k in pattern) --rest[k];
                foreach (List<ShapeBlock> shape in TileMath.Shapes(rest)) forms.Add((shape, pattern, null));
            }
        int[] before = (int[])c.Clone(); --before[winning / 4];
        int waits = 0;
        for (int k = 0; k < 34; ++k) { ++before[k]; if (TileMath.Complete(before, true)) ++waits; --before[k]; }
        McrScore best = new([]);
        foreach ((List<ShapeBlock> original, int[]? knitted, string? special) in forms)
        {
            int concealed = original.Count;
            List<ShapeBlock> blocks = [.. original, .. melds.Select(m => new ShapeBlock(m.Tiles.Min() / 4, m.Tiles.Length, m.Kind == "chi", m.Open, true))];
            int[] interpretations = special is not null ? [-1] : Enumerable.Range(0, concealed).Where(i => blocks[i].Contains(winning / 4)).ToArray();
            if (knitted?.Contains(winning / 4) == true) interpretations = [.. interpretations, -1];
            foreach (int win in interpretations)
            {
                List<ShapeBlock> view = [.. blocks];
                if (win >= 0 && !context.SelfDrawn && !view[win].Chow && view[win].Size == 3) view[win] = view[win] with { Open = true };
                McrScore score = ScoreForm(c, all, melds, view, winning / 4, win, waits == 1, knitted, special, context);
                if (score.Points > best.Points) best = score;
            }
        }
        return best;
    }
    private static McrScore ScoreForm(int[] c, int[] all, IReadOnlyList<TileSet> melds, List<ShapeBlock> blocks,
        int winning, int win, bool singleWait, int[]? knitted, string? special, McrContext x)
    {
        List<Candidate> fans = [], excluded = [];
        void Add(string name, int mask = 0) => fans.Add(new(name, mask));
        void Exclude(string name, int mask = 0) => excluded.Add(new(name, mask));
        bool Has(string name) => fans.Any(f => f.Name == name);
        void ExcludeNames(params string[] names) { foreach (string name in names) Exclude(name); }
        int[] kinds = Enumerable.Range(0, 34).Where(k => all[k] > 0).ToArray();
        int[] suits = kinds.Where(k => k < 27).Select(k => k / 9).Distinct().ToArray();
        bool honors = kinds.Any(k => k >= 27), closed = melds.All(m => !m.Open);
        bool ordinary = special is null && knitted is null;
        if (special is not null) Add(special);
        if (knitted is not null || (special is "全不靠" or "七星不靠" && TileMath.Knitted.Any(p => p.All(k => c[k] > 0)))) Add("组合龙");
        bool completeSpecial = special is "十三幺" or "全不靠" or "七星不靠";
        if (!completeSpecial)
        {
            if (knitted is null)
            {
                if (kinds.All(k => k is 19 or 20 or 21 or 23 or 25 or 32)) { Add("绿一色"); Exclude("混一色"); }
                if (melds.Count == 0 && suits.Length == 1 && !honors)
                {
                    int[] prior = (int[])c.Clone(); --prior[winning]; int offset = suits[0] * 9;
                    if (prior[offset] == 3 && prior[offset + 8] == 3 && Enumerable.Range(1, 7).All(i => prior[offset + i] == 1))
                    {
                        Add("九莲宝灯"); ExcludeNames("清一色", "不求人", "门前清", "无字");
                        int terminal = blocks.FindIndex(b => b.Size == 3 && !b.Chow && TileMath.Terminal(b.Tile));
                        if (terminal >= 0) Exclude("幺九刻", 1 << terminal);
                    }
                }
                if (kinds.All(k => k < 27 && TileMath.Terminal(k)))
                {
                    Add("清幺九"); ExcludeNames("碰碰和", "全带幺", "无字");
                    for (int i = 0; i < blocks.Count; ++i)
                    {
                        Exclude("幺九刻", 1 << i);
                        for (int j = i + 1; j < blocks.Count; ++j)
                            if (blocks[i].Size >= 3 && blocks[j].Size >= 3 && blocks[i].Rank == blocks[j].Rank) Exclude("双同刻", (1 << i) | (1 << j));
                    }
                }
                if (suits.Length == 0) { Add("字一色"); ExcludeNames("碰碰和", "全带幺"); for (int i = 0; i < blocks.Count; ++i) Exclude("幺九刻", 1 << i); }
                if (honors && suits.Length > 0 && kinds.All(TileMath.Terminal))
                { Add("混幺九"); ExcludeNames("碰碰和", "全带幺"); for (int i = 0; i < blocks.Count; ++i) Exclude("幺九刻", 1 << i); }
                if (ordinary && blocks.All(b => !b.Chow && b.Tile < 27 && b.Rank % 2 == 0))
                { Add("全双刻"); ExcludeNames("碰碰和", "断幺", "无字"); }
                if (suits.Length == 1 && !honors) { Add("清一色"); Exclude("无字"); }
                if (kinds.All(k => k < 27 && k % 9 >= 6)) { Add("全大"); ExcludeNames("大于五", "无字"); }
                if (kinds.All(k => k < 27 && k % 9 is >= 3 and <= 5)) { Add("全中"); ExcludeNames("断幺", "无字"); }
                if (kinds.All(k => k < 27 && k % 9 <= 2)) { Add("全小"); ExcludeNames("小于五", "无字"); }
                if (ordinary && blocks.All(b => b.Kinds.All(k => k < 27) && b.Kinds.Any(k => k % 9 == 4)))
                { Add("全带五"); ExcludeNames("断幺", "无字"); }
                if (kinds.All(k => k < 27 && k % 9 >= 5)) { Add("大于五"); Exclude("无字"); }
                if (kinds.All(k => k < 27 && k % 9 <= 3)) { Add("小于五"); Exclude("无字"); }
                if (kinds.All(k => new[] { 19, 21, 22, 23, 25, 26, 9, 10, 11, 12, 13, 16, 17, 31 }.Contains(k))) { Add("推不倒"); Exclude("缺一门"); }
                if (ordinary && blocks.All(b => !b.Chow)) Add("碰碰和");
                if (suits.Length == 1 && honors) Add("混一色");
                if (ordinary && blocks.All(b => b.Kinds.Any(TileMath.Terminal))) Add("全带幺");
                if (kinds.All(k => !TileMath.Terminal(k))) { Add("断幺"); Exclude("无字"); }
                if (suits.Length == 2) Add("缺一门");
            }
            if (suits.Length == 3 && kinds.Any(k => k is >= 27 and <= 30) && kinds.Any(k => k >= 31)) Add("五门齐");
            if (special is null && blocks.Count > 0 && blocks.All(b => b.Chow || (b.Size == 2 && b.Tile < 27))) { Add("平和"); Exclude("无字"); }
            foreach (int k in kinds.Where(k => all[k] == 4 && !melds.Any(m => m.Tiles.Length == 4 && m.Tiles[0] / 4 == k))) Add("四归一");
            if (!honors) Add("无字");
        }
        if (special == "连七对") ExcludeNames("清一色", "无字", "七对");
        if (special is null)
        {
            int closedKongs = blocks.Count(b => b.Size == 4 && !b.Open), openKongs = blocks.Count(b => b.Size == 4 && b.Open);
            int concealedPungs = blocks.Count(b => b.Size >= 3 && !b.Chow && !b.Open);
            if (closedKongs + openKongs == 4) { Add("四杠"); Exclude("碰碰和"); int pair = blocks.FindIndex(b => b.Size == 2); Exclude("单钓将", 1 << pair); }
            else if (closedKongs + openKongs == 3) Add("三杠");
            else if (closedKongs == 2) { Add("双暗杠"); if (concealedPungs == 2) Exclude("双暗刻"); }
            else if (openKongs == 2) Add("双明杠");
            else if (closedKongs == 1 && openKongs == 1) Add("明暗杠");
            else if (closedKongs == 1) Add("暗杠"); else if (openKongs == 1) Add("明杠");
            if (concealedPungs == 4) { Add("四暗刻"); ExcludeNames("碰碰和", "不求人", "门前清"); }
            else if (concealedPungs == 3) Add("三暗刻"); else if (concealedPungs == 2) Add("双暗刻");
            List<Candidate> associations = Associations(blocks);
            foreach (Candidate f in BestAssociations(associations))
            {
                Add(f.Name, f.Mask);
                switch (f.Name)
                {
                    case "大四喜":
                        Exclude("碰碰和");
                        for (int i = 0; i < blocks.Count; ++i) if ((f.Mask & (1 << i)) != 0) { Exclude("圈风刻", 1 << i); Exclude("门风刻", 1 << i); Exclude("幺九刻", 1 << i); }
                        break;
                    case "大三元": case "小三元": case "双箭刻":
                        for (int i = 0; i < blocks.Count; ++i) if ((f.Mask & (1 << i)) != 0) { Exclude("箭刻", 1 << i); Exclude("幺九刻", 1 << i); }
                        break;
                    case "小四喜": case "三风刻":
                        for (int i = 0; i < blocks.Count; ++i) if ((f.Mask & (1 << i)) != 0) Exclude("幺九刻", 1 << i);
                        break;
                    case "一色双龙会": ExcludeNames("清一色", "平和", "无字"); break;
                    case "一色四同顺": Exclude("四归一"); Exclude("四归一"); Exclude("四归一"); break;
                    case "一色四节高": Exclude("碰碰和"); break;
                    case "三色双龙会": Exclude("平和"); break;
                }
            }
            for (int i = 0; i < blocks.Count; ++i)
            {
                ShapeBlock b = blocks[i]; if (b.Chow || b.Size == 2) continue;
                if (b.Tile >= 31) { Add("箭刻", 1 << i); Exclude("幺九刻", 1 << i); }
                if (b.Tile == x.RoundWind) { Add("圈风刻", 1 << i); Exclude("幺九刻", 1 << i); }
                if (b.Tile == x.SeatWind) { Add("门风刻", 1 << i); Exclude("幺九刻", 1 << i); }
                if (TileMath.Terminal(b.Tile)) Add("幺九刻", 1 << i);
            }
        }
        if (x.LastTile && x.SelfDrawn) { Add("妙手回春"); Exclude("自摸"); }
        if (x.LastTile && !x.SelfDrawn) Add("海底捞月");
        if (x.AboutKong && x.SelfDrawn) { Add("杠上开花"); Exclude("自摸"); }
        if (x.AboutKong && !x.SelfDrawn) { Add("抢杠和"); Exclude("和绝张"); }
        int pairIndex = blocks.FindIndex(b => b.Size == 2);
        if (melds.Count == 4 && melds.All(m => m.Open) && !x.SelfDrawn) { Add("全求人"); Exclude("单钓将", 1 << pairIndex); }
        if (closed && x.SelfDrawn) { Add("不求人"); ExcludeNames("门前清", "自摸"); }
        if (x.FourthTile) { Add("和绝张"); Exclude("单钓将", 1 << pairIndex); }
        if (closed) Add("门前清");
        if (special is not null) ExcludeNames("不求人", "门前清");
        if (singleWait && win >= 0 && knitted?.Contains(winning) != true)
        {
            ShapeBlock b = blocks[win];
            if (b.Size == 2) Add("单钓将", 1 << win);
            else if (b.Chow && ((b.Rank == 1 && winning % 9 == 2) || (b.Rank == 7 && winning % 9 == 6))) Add("边张", 1 << win);
            else if (b.Chow && winning == b.Tile + 1) Add("坎张", 1 << win);
        }
        if (x.SelfDrawn)
        {
            Add("自摸");
            if ((Has("九莲宝灯") || Has("四暗刻") || special is not null) && !x.LastTile && !x.AboutKong) excluded.RemoveAll(f => f.Name == "自摸");
        }
        foreach (Candidate e in excluded)
        {
            int index = fans.FindIndex(f => f.Name == e.Name && f.Mask == e.Mask); if (index >= 0) fans.RemoveAt(index);
        }
        if (fans.Count == 0) Add("无番和");
        for (int i = 0; i < x.Flowers; ++i) Add("花牌");
        return new(fans.GroupBy(f => f.Name).Select(g => new FanEntry(g.Key, FanCatalog.Points[g.Key], g.Count())).ToArray());
    }
    private static List<Candidate> Associations(List<ShapeBlock> blocks)
    {
        List<Candidate> output = [];
        void Add(string name, IEnumerable<int> ids) => output.Add(new(name, ids.Aggregate(0, (mask, i) => mask | (1 << i))));
        int[] chows = Enumerable.Range(0, blocks.Count).Where(i => blocks[i].Chow).ToArray();
        int[] pungs = Enumerable.Range(0, blocks.Count).Where(i => !blocks[i].Chow && blocks[i].Size >= 3).ToArray();
        int[] winds = pungs.Where(i => blocks[i].Tile is >= 27 and <= 30).ToArray(), dragons = pungs.Where(i => blocks[i].Tile >= 31).ToArray();
        int pair = blocks.FindIndex(b => b.Size == 2);
        if (winds.Length == 4) Add("大四喜", winds);
        if (winds.Length == 3) { Add("三风刻", winds); if (pair >= 0 && blocks[pair].Tile is >= 27 and <= 30) Add("小四喜", winds.Append(pair)); }
        if (dragons.Length == 3) Add("大三元", dragons);
        if (dragons.Length == 2) { Add("双箭刻", dragons); if (pair >= 0 && blocks[pair].Tile >= 31) Add("小三元", dragons.Append(pair)); }
        if (chows.Length == 4 && pair >= 0 && blocks[pair].Tile < 27 && blocks[pair].Rank == 5
            && chows.Count(i => blocks[i].Rank == 1) == 2 && chows.Count(i => blocks[i].Rank == 7) == 2)
        {
            if (chows.All(i => blocks[i].Suit == blocks[pair].Suit)) Add("一色双龙会", chows.Append(pair));
            else
            {
                int[] low = chows.Where(i => blocks[i].Rank == 1).Select(i => blocks[i].Suit).Order().ToArray();
                int[] high = chows.Where(i => blocks[i].Rank == 7).Select(i => blocks[i].Suit).Order().ToArray();
                if (low.Distinct().Count() == 2 && low.SequenceEqual(high) && !low.Contains(blocks[pair].Suit)) Add("三色双龙会", chows.Append(pair));
            }
        }
        foreach (int[] ids in Subsets(chows, 2, 4))
        {
            ShapeBlock[] b = ids.Select(i => blocks[i]).OrderBy(b => b.Rank).ToArray();
            bool sameSuit = b.All(s => s.Suit == b[0].Suit), sameRank = b.All(s => s.Rank == b[0].Rank);
            if (sameSuit && sameRank) Add(ids.Length == 4 ? "一色四同顺" : ids.Length == 3 ? "一色三同顺" : "一般高", ids);
            if (ids.Length == 2)
            {
                if (!sameSuit && sameRank) Add("喜相逢", ids);
                if (sameSuit && b[1].Rank - b[0].Rank == 3) Add("连六", ids);
                if (sameSuit && b[1].Rank - b[0].Rank == 6) Add("老少副", ids);
            }
            if (ids.Length >= 3 && sameSuit && b[1].Rank - b[0].Rank is 1 or 2 && Enumerable.Range(1, b.Length - 1).All(i => b[i].Rank - b[i - 1].Rank == b[1].Rank - b[0].Rank))
                Add(ids.Length == 4 ? "一色四步高" : "一色三步高", ids);
            if (ids.Length == 3)
            {
                bool threeSuits = b.Select(v => v.Suit).Distinct().Count() == 3;
                if (sameRank && threeSuits) Add("三色三同顺", ids);
                if (threeSuits && b[1].Rank - b[0].Rank == 1 && b[2].Rank - b[1].Rank == 1) Add("三色三步高", ids);
                if (b.Select(v => v.Rank).SequenceEqual(new[] { 1, 4, 7 }))
                { if (sameSuit) Add("清龙", ids); if (threeSuits) Add("花龙", ids); }
            }
        }
        foreach (int[] ids in Subsets(pungs.Where(i => blocks[i].Tile < 27).ToArray(), 2, 4))
        {
            ShapeBlock[] b = ids.Select(i => blocks[i]).OrderBy(b => b.Rank).ToArray();
            if (b.All(v => v.Rank == b[0].Rank)) Add(ids.Length == 3 ? "三同刻" : "双同刻", ids);
            if (ids.Length >= 3 && Enumerable.Range(1, b.Length - 1).All(i => b[i].Rank == b[i - 1].Rank + 1))
            {
                if (b.All(v => v.Suit == b[0].Suit)) Add(ids.Length == 4 ? "一色四节高" : "一色三节高", ids);
                if (ids.Length == 3 && b.Select(v => v.Suit).Distinct().Count() == 3) Add("三色三节高", ids);
            }
        }
        return output;
    }
    private static IEnumerable<int[]> Subsets(int[] input, int min, int max)
    {
        for (int mask = 1; mask < 1 << input.Length; ++mask)
        {
            int[] ids = Enumerable.Range(0, input.Length).Where(i => (mask & (1 << i)) != 0).Select(i => input[i]).ToArray();
            if (ids.Length >= min && ids.Length <= max) yield return ids;
        }
    }
    private sealed record Forest(int[] Roots, List<Candidate> Used, int Score);
    private static IEnumerable<Candidate> BestAssociations(List<Candidate> candidates)
    {
        Queue<Forest> queue = new(); Forest best = new([0, 1, 2, 3, 4], [], 0); queue.Enqueue(best);
        Dictionary<string, int> visited = [];
        while (queue.TryDequeue(out Forest? current))
            foreach (Candidate candidate in candidates)
            {
                int[] ids = Enumerable.Range(0, 5).Where(i => (candidate.Mask & (1 << i)) != 0).ToArray();
                if (ids.Select(i => current.Roots[i]).Distinct().Count() != ids.Length) continue;
                int[] roots = (int[])current.Roots.Clone(); HashSet<int> merged = ids.Select(i => roots[i]).ToHashSet(); int root = merged.Min();
                for (int i = 0; i < roots.Length; ++i) if (merged.Contains(roots[i])) roots[i] = root;
                string key = string.Join(',', roots); int score = current.Score + FanCatalog.Points[candidate.Name];
                if (visited.GetValueOrDefault(key) >= score) continue;
                visited[key] = score; Forest next = new(roots, [.. current.Used, candidate], score); queue.Enqueue(next);
                if (score > best.Score) best = next;
            }
        return best.Used;
    }
}
