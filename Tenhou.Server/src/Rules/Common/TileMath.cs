using System;
using System.Collections.Generic;
using System.Linq;

namespace Tenhou.Server.Rules.Common;

public sealed record TileSet(string Kind, int[] Tiles, int Who, int Source, int Called = -1)
{
    public bool Open => Kind != "closed_kan";
}
public sealed record ShapeBlock(int Tile, int Size = 3, bool Chow = false, bool Open = false, bool Declared = false)
{
    public int Suit => Tile / 9;
    public int Rank => Tile % 9 + 1;
    public bool Contains(int kind) => Chow ? kind >= Tile && kind <= Tile + 2 : kind == Tile;
    public int[] Kinds => Chow ? [Tile, Tile + 1, Tile + 2] : Enumerable.Repeat(Tile, Size).ToArray();
}

public static class TileMath
{
    public static int[] Counts(IEnumerable<int> tiles)
    {
        int[] c = new int[34];
        foreach (int tile in tiles)
        {
            if (tile is < 0 or >= 136 || ++c[tile / 4] > 4) throw new ArgumentException("Invalid tile set");
        }
        return c;
    }
    public static bool Terminal(int k) => k >= 27 || k % 9 is 0 or 8;
    public static IEnumerable<List<ShapeBlock>> Shapes(int[] counts)
    {
        List<List<ShapeBlock>> output = []; int[] c = (int[])counts.Clone();
        void Search(List<ShapeBlock> path)
        {
            int k = Array.FindIndex(c, n => n > 0);
            if (k == -1) { output.Add([.. path]); return; }
            if (c[k] >= 3)
            {
                c[k] -= 3; path.Add(new(k)); Search(path); path.RemoveAt(path.Count - 1); c[k] += 3;
            }
            if (k < 27 && k % 9 < 7 && c[k + 1] > 0 && c[k + 2] > 0)
            {
                --c[k]; --c[k + 1]; --c[k + 2]; path.Add(new(k, Chow: true)); Search(path); path.RemoveAt(path.Count - 1);
                ++c[k]; ++c[k + 1]; ++c[k + 2];
            }
        }
        for (int pair = 0; pair < 34; ++pair)
            if (c[pair] >= 2) { c[pair] -= 2; Search([new(pair, 2)]); c[pair] += 2; }
        return output;
    }
    public static bool SevenPairs(int[] c) => c.Sum() == 14 && c.All(n => n % 2 == 0);
    public static bool Orphans(int[] c) => c.Sum() == 14 && Enumerable.Range(0, 34).All(k => Terminal(k) ? c[k] > 0 : c[k] == 0);
    public static readonly int[][] Knitted =
        (from a in Enumerable.Range(0, 3) from b in Enumerable.Range(0, 3) where b != a
         let d = 3 - a - b select Enumerable.Range(0, 3).SelectMany(i => new[] { a * 9 + i * 3, b * 9 + i * 3 + 1, d * 9 + i * 3 + 2 }).Order().ToArray()).ToArray();
    public static bool Unconnected(int[] c) => c.Sum() == 14 && c.All(n => n <= 1)
        && Knitted.Any(pattern => Enumerable.Range(0, 27).All(k => c[k] == 0 || pattern.Contains(k)));
    public static bool Complete(int[] c, bool mcr = false)
    {
        if (c.Sum() % 3 != 2) return false;
        if (Shapes(c).Any() || SevenPairs(c) || (mcr && (Orphans(c) || Unconnected(c)))) return true;
        if (!mcr || c.Sum() < 11) return false;
        foreach (int[] pattern in Knitted)
            if (pattern.All(k => c[k] > 0))
            {
                int[] rest = (int[])c.Clone(); foreach (int k in pattern) --rest[k];
                if (Shapes(rest).Any()) return true;
            }
        return false;
    }
}
