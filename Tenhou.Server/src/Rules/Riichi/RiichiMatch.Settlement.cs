using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using static Tenhou.Server.Rules.Riichi.RiichiOptions;
using static Tenhou.Server.Rules.Riichi.HandEvaluator;

namespace Tenhou.Server.Rules.Riichi;

public sealed partial class RiichiMatch
{
    private int[] Transfers(int seat, int source, HandValue value, bool tsumo)
    {
        int[] delta = new int[4];
        (int Seat, int Units)[] liable = value.Yakuman.Where(Players[seat].Pao.ContainsKey)
            .Select(k => (Players[seat].Pao[k], k == 49 ? 2 : 1)).ToArray();
        int main = value.Main, additional = value.Additional;
        if (liable.Length > 0)
        {
            int remaining = value.Han / 13, unit = seat == Dealer ? 48000 : 32000;
            foreach ((int payer, int units) in liable)
            {
                remaining -= units;
                if (tsumo || source == payer) delta[payer] -= unit * units;
                else { delta[source] -= unit * units / 2; delta[payer] -= unit * units / 2; }
            }
            main = (tsumo ? 16000 : unit) * remaining; additional = (seat == Dealer ? 16000 : 8000) * remaining;
        }
        if (tsumo)
        {
            for (int s = 0; s < PlayerCount; ++s)
                if (s != seat) delta[s] -= seat == Dealer || s == Dealer ? main : additional;
            if (liable.Length > 0) delta[liable[0].Seat] -= (PlayerCount - 1) * 100 * Honba;
            else for (int s = 0; s < PlayerCount; ++s) if (s != seat) delta[s] -= 100 * Honba;
        }
        else delta[source] -= main + 300 * Honba;
        delta[seat] = -delta.Sum(); return delta;
    }
    private void Win(int[] winners, int source, bool tsumo)
    {
        int tile = tsumo ? Players[source].Drawn : lastTile;
        HandValue[] values = winners.Select(s => Value(s, tile, tsumo, !tsumo && claimKind != "discard", !tsumo && claimKind == "closed_kan")).ToArray();
        if (values.Any(v => v.Han == 0)) throw new InvalidOperationException("Unvalidated win");
        int first = events.Count;
        for (int i = 0; i < winners.Length; ++i)
        {
            int seat = winners[i]; Player p = Players[seat]; HandValue value = values[i];
            int[] before = (int[])scores.Clone(), delta = Transfers(seat, source, value, tsumo);
            int award = i == 0 ? Sticks : 0; delta[seat] += award * 1000; Sticks -= award;
            List<int> sc = [];
            for (int s = 0; s < 4; ++s) { scores[s] += delta[s]; sc.Add(before[s] / 100); sc.Add(delta[s] / 100); }
            List<int> hand = [.. p.Hand]; if (!tsumo) hand.Add(tile); hand.Sort();
            Dictionary<string, string> attrs = new()
            {
                ["who"] = Text(seat), ["fromWho"] = Text(source), ["machi"] = Text(tile), ["hai"] = List(hand),
                ["ten"] = List([value.Fu, delta[seat] - award * 1000 - Honba * (tsumo ? (PlayerCount - 1) * 100 : 300), value.Level]),
                ["doraHai"] = List(Dora()), ["ba"] = List([Honba, award]), ["sc"] = List(sc),
                [value.Yakuman.Count == 0 ? "yaku" : "yakuman"] = List(value.Yakuman.Count == 0 ? value.Yaku : value.Yakuman)
            };
            int[] melds = p.Melds.Select(m => m.Encode()).Concat(p.Nuki.Select(t => (t << 8) | 32)).ToArray();
            if (melds.Length > 0) attrs["m"] = List(melds);
            if (p.Riichi) attrs["doraHaiUra"] = List(Dora(true));
            Emit("AGARI", attrs);
        }
        End(winners.Contains(Dealer), false, first);
    }
    private void EndDraw(string reason = "")
    {
        int[] before = (int[])scores.Clone(), delta = new int[4];
        Dictionary<string, string> attrs = [];
        if (reason.Length > 0) attrs["type"] = reason;
        int[] ready = Enumerable.Range(0, PlayerCount).Where(s => evaluator.Waits(Players[s].Hand, PlayerCount).Length > 0).ToArray();
        int[] nagashi = Enumerable.Range(0, PlayerCount).Where(s => Players[s].River.Count > 0 && Players[s].Called.Count == 0
            && Players[s].River.All(t => Terminal(t / 4))).ToArray();
        if (reason.Length == 0)
        {
            if (Options.Minimum is "haneman" or "baiman" or "sanbaiman" or "yakuman") nagashi = [];
            if (nagashi.Length > 0)
            {
                attrs["type"] = "nm";
                foreach (int s in nagashi)
                {
                    HandValue v = new() { Han = 5, Fu = 30, Main = 4000, Additional = s == Dealer ? 4000 : 2000 };
                    int[] transfer = Transfers(s, s, v, true);
                    for (int j = 0; j < 4; ++j) delta[j] += transfer[j];
                }
                int recipient = nagashi.OrderBy(s => (s - Dealer + 4) % 4).First();
                delta[recipient] += Sticks * 1000; Sticks = 0;
            }
            else if (ready.Length > 0 && ready.Length < PlayerCount)
            {
                int pool = PlayerCount == 3 ? 2000 : 3000;
                for (int s = 0; s < PlayerCount; ++s) delta[s] = ready.Contains(s) ? pool / ready.Length : -pool / (PlayerCount - ready.Length);
            }
            foreach (int s in ready) attrs["hai" + s] = List(Players[s].Hand);
        }
        List<int> sc = [];
        for (int s = 0; s < 4; ++s) { scores[s] += delta[s]; sc.Add(before[s] / 100); sc.Add(delta[s] / 100); }
        attrs["sc"] = List(sc); int first = events.Count; Emit("RYUUKYOKU", attrs);
        End(reason.Length > 0 || (nagashi.Length == 0 ? ready : nagashi).Contains(Dealer), true, first);
    }
    private void End(bool stays, bool drawn, int first)
    {
        nextRound = stays ? Round : Round + 1; nextHonba = drawn || stays ? Honba + 1 : 0;
        int top = Enumerable.Range(0, PlayerCount).OrderByDescending(s => scores[s]).First();
        bool bankrupt = Options.Bankruptcy && scores.Take(PlayerCount).Any(v => v < 0);
        bool finish = bankrupt || Options.Length == "single";
        if (Round >= (Options.Length == "south" ? 2 : 1) * PlayerCount - 1 && scores[top] >= Options.Target)
            finish |= !stays || top == Dealer;
        if (drawn && events[first].Data.TryGetValue("type", out string? type) && type != "nm") finish = bankrupt || Options.Length == "single";
        Phase = finish ? "complete" : "ended";
        if (finish)
        {
            MatchEvent last = events[^1];
            if (Sticks > 0)
            {
                scores[top] += Sticks * 1000; int[] sc = last.Data["sc"].Split(',').Select(int.Parse).ToArray();
                sc[top * 2 + 1] += Sticks * 10; last.Data["sc"] = List(sc); Sticks = 0;
            }
            int[] order = Enumerable.Range(0, PlayerCount).OrderByDescending(s => scores[s]).ToArray();
            int[] uma = PlayerCount == 3 ? [20, 0, -20] : [20, 10, -10, -20]; double[] points = new double[4];
            for (int rank = 0; rank < PlayerCount; ++rank)
                points[order[rank]] = (scores[order[rank]] - Options.Target) / 1000.0 + uma[rank]
                    + (rank == 0 ? PlayerCount * (Options.Target - Options.Starting) / 1000.0 : 0);
            string owari = string.Join(',', Enumerable.Range(0, 4).SelectMany(s => new[]
                { Text(scores[s] / 100), points[s].ToString("0.######", CultureInfo.InvariantCulture) }));
            for (int i = first; i < events.Count; ++i) events[i].Data["owari"] = owari;
            Emit("PROF");
        }
        claims.Clear(); responses.Clear();
    }
    public void Validate()
    {
        if (dead.Count == 0) return;
        List<int> used = [.. live, .. dead.Skip(replacements), .. sealedTiles];
        foreach (Player p in Players)
        {
            used.AddRange(p.Hand); used.AddRange(p.Melds.SelectMany(m => m.Tiles));
            used.AddRange(p.River.Where(t => !p.Called.Contains(t))); used.AddRange(p.Nuki);
        }
        if (!used.Order().SequenceEqual(TileSet())) throw new InvalidOperationException("Physical tile conservation");
        if (scores.Sum() + Sticks * 1000 != Options.Starting * PlayerCount) throw new InvalidOperationException("Point conservation");
    }
    public MatchAction ChooseBotAction(int seat)
    {
        IReadOnlyList<MatchAction> options = GetActions(seat);
        foreach (string kind in new[] { "ron", "tsumo", "nuki" })
        {
            MatchAction? win = options.FirstOrDefault(a => a.Kind == kind); if (win is not null) return win;
        }
        if (Phase == "claims") return new("skip");
        Player p = Players[seat]; int[] visible = Counts(p.Hand);
        foreach (Player q in Players)
            foreach (int t in q.Nuki.Concat(q.River.Where(t => !q.Called.Contains(t))).Concat(q.Melds.SelectMany(m => m.Tiles))) ++visible[t / 4];
        Dictionary<int, (int Shanten, int Improving)> evaluated = [];
        (int, int, int, int, int) best = (99, 0, 1, 1, 999); MatchAction? chosen = null;
        foreach (MatchAction a in options)
        {
            if (a.Kind is not ("discard" or "riichi")) continue;
            int tile = a.Tiles[0], kind = tile / 4;
            if (!evaluated.TryGetValue(kind, out (int Shanten, int Improving) rank))
            {
                List<int> rest = [.. p.Hand]; Remove(rest, tile); int[] c = Counts(rest);
                int basis = evaluator.Shanten(c), improving = 0;
                for (int k = 0; k < 34; ++k)
                {
                    if ((PlayerCount == 3 && k is >= 1 and <= 7) || visible[k] >= 4) continue;
                    ++c[k]; if (evaluator.Shanten(c) < basis) improving += 4 - visible[k]; --c[k];
                }
                rank = (basis, -improving); evaluated[kind] = rank;
            }
            (int, int, int, int, int) key = (rank.Shanten, rank.Improving, a.Kind == "riichi" ? 0 : 1, Options.RedTiles.Contains(tile) ? 1 : 0, tile);
            if (key.CompareTo(best) < 0) { best = key; chosen = a; }
        }
        return chosen ?? throw new InvalidOperationException("No bot action");
    }
}
