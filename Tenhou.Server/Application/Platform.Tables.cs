using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules;

namespace Tenhou.Server.Application;

public sealed partial class Platform
{
    private static string Offer(Table table, int seat) => string.Join(';', table.Match.GetActions(seat).Select(a => a.Kind + ":" + string.Join(',', a.Tiles)));
    private static int BaseTime(Table table) => int.Parse(table.Record.Options.GetValueOrDefault("time", "5+20").Split('+')[0], CultureInfo.InvariantCulture) * 1000;
    private static int ReserveTime(Table table) => int.Parse(table.Record.Options.GetValueOrDefault("time", "5+20").Split('+')[1], CultureInfo.InvariantCulture) * 1000;
    private void Matchmake()
    {
        foreach (IGrouping<(int Room, int Queue), Session> group in sessions.Values.Where(s => presence[s.Token].Queued && presence[s.Token].Peer is not null)
            .GroupBy(s => (s.Room, s.Room == 0 ? presence[s.Token].Queue : 0)).ToArray())
        {
            List<Session> waiting = group.OrderBy(s => presence[s.Token].Order).ToList();
            string rule = group.Key.Room == 0 ? queues[group.Key.Queue].Rule : rooms[group.Key.Room].Rule;
            Dictionary<string, string> options = new(group.Key.Room == 0 ? queues[group.Key.Queue].Options : rooms[group.Key.Room].Options);
            while (waiting.Count > 0 && tables.Count < settings.MaxTables)
            {
                Table table = CreateTable(new() { Id = Guid.NewGuid().ToString("N"), Rule = rule, RuleVersion = rules.Get(rule).Descriptor.Version, Options = options });
                int players = table.Match.PlayerCount;
                if (group.Key.Room != 0 && waiting.Count < players && Now - presence[waiting[0].Token].QueuedAt < settings.MatchWaitMs) break;
                int humans = group.Key.Room == 0 ? 1 : Math.Min(players, waiting.Count);
                table.Record.Humans = waiting.Take(humans).Select(s => s.Token).ToList();
                table.Record.Ranked = group.Key.Room != 0 && humans == players && table.Presentation.RankCategory.StartsWith("riichi/", StringComparison.Ordinal)
                    && table.Record.Humans.All(t => sessions[t].Account.Length > 0);
                foreach (string token in table.Record.Humans)
                { sessions[token].Table = table.Record.Id; presence[token].Queued = false; presence[token].Exited = false; }
                waiting.RemoveRange(0, humans); tables.Add(table.Record.Id, table); Save();
                store.Log("table_created", humans);
                foreach (string token in table.Record.Humans) Identity(table, token);
            }
        }
    }
    private string[] Names(Table table) => Enumerable.Range(0, 4).Select(s => s >= table.Match.PlayerCount ? ""
        : s < table.Record.Humans.Count ? Name(table.Record.Humans[s]) : "人机" + (s + 1)).ToArray();
    private void Identity(Table table, string token)
    {
        int viewer = table.Record.Humans.IndexOf(token); string[] names = Names(table);
        Dictionary<string, string> attributes = [];
        for (int j = 0; j < 4; ++j) attributes["n" + j] = Uri.EscapeDataString(names[(j + viewer) % 4]);
        attributes["sx"] = string.Concat(Enumerable.Range(0, 4).Select(j => (j + viewer) % 4 < table.Record.Humans.Count
            ? sessions[table.Record.Humans[(j + viewer) % 4]].Gender : "M"));
        int category = table.Match.PlayerCount == 3 ? 1 : 0;
        Rank GetRank(int j)
        {
            int s = (j + viewer) % 4;
            return s < table.Record.Humans.Count && sessions[table.Record.Humans[s]].Account.Length > 0
                ? accounts[sessions[table.Record.Humans[s]].Account].Ranks[category] : new();
        }
        attributes["dan"] = string.Join(',', Enumerable.Range(0, 4).Select(j => GetRank(j).Level));
        attributes["rate"] = string.Join(',', Enumerable.Range(0, 4).Select(j => GetRank(j).Rating.ToString(CultureInfo.InvariantCulture)));
        Send(token, Wire.Encode("UN", attributes));
        Dictionary<string, string> go = table.Presentation.GameInfo;
        go["table"] = table.Record.Id; go["ranked"] = table.Record.Ranked ? "1" : "0"; go["exit"] = table.Record.Ranked ? "0" : "1";
        if (table.Presentation.GameTag == "VGO")
        {
            go["seat"] = viewer.ToString(CultureInfo.InvariantCulture);
            for (int s = 0; s < 4; ++s) go["n" + s] = names[s];
        }
        Send(token, Wire.Encode(table.Presentation.GameTag, go));
    }
    private void Start(Table table)
    {
        table.Ready.Clear(); table.Deadlines = new long[4]; table.Started = new long[4];
        table.Record.Elapsed = new int[4]; table.Offers = ["", "", "", ""]; table.Declared = [-1, -1, -1, -1];
        table.Record.Reserve = Enumerable.Repeat(ReserveTime(table), 4).ToArray();
        table.Match.Start(); table.Record.Commands.Add(new("start", Wall: table.Match.Walls[^1]));
        bool presentation = table.Presentation.SupportsAnimation && table.Record.Humans.Any(t => presence[t].Presentation);
        table.Record.Stage = presentation ? "prepare" : "play"; table.Due = presentation ? Now + 10000 : 0;
        Save();
        if (presentation) foreach (string token in table.Record.Humans.Where(t => presence[t].Presentation)) Send(token, Wire.Encode("PREPARE"));
        Flush(table, presentation);
    }
    private void Apply(Table table, int seat, MatchAction action)
    {
        if (table.Record.Commands.Count >= 100000) throw new InvalidOperationException("Command capacity reached");
        table.Match.Submit(seat, action); table.Record.Commands.Add(new("action", seat, action));
        table.Offers[seat] = ""; table.Deadlines[seat] = table.Started[seat] = 0; table.Record.Elapsed[seat] = 0; table.Declared[seat] = -1;
    }
    private void Flush(Table table, bool opening = false)
    {
        RefreshWindows(table);
        if (table.Match.IsComplete && !table.Record.Settled) Settle(table);
        bool paced = table.Presentation.SupportsAnimation && table.Record.Humans.Any(t => presence[t].Presentation);
        while (table.Cursor < table.Match.Events.Count)
        {
            MatchEvent e = table.Match.Events[table.Cursor];
            bool movement = e.Kind == "N" || (e.Kind.Length > 1 && char.IsAsciiDigit(e.Kind[1]) && "TUVWDEFGtuvwdefg".Contains(e.Kind[0], StringComparison.Ordinal));
            if (paced && movement && Now < table.NextEvent) return;
            ++table.Cursor;
            for (int s = 0; s < table.Record.Humans.Count; ++s)
                Send(table.Record.Humans[s], table.Presentation.Project(e, s, table.Presentation.GameTag == "VGO" ? table.Windows[s] : table.Match.Events.Count));
            if (paced && movement) table.NextEvent = Now + (e.Kind == "N" ? 400 : 280);
            if (opening && e.Kind == "INIT") break;
        }
        if (table.Presentation.GameTag != "VGO")
            for (int s = 0; s < table.Record.Humans.Count; ++s) table.Windows[s] = table.Match.GetActions(s).Count == 0 ? 0 : table.Match.Events.Count;
    }
    private static void RefreshWindows(Table table)
    {
        if (table.Presentation.GameTag != "VGO") return;
        for (int s = 0; s < table.Record.Humans.Count; ++s)
        {
            string offer = Offer(table, s);
            if (offer.Length == 0) table.Windows[s] = 0;
            else if (offer != table.WindowOffers[s] || table.Windows[s] == 0) table.Windows[s] = table.Match.Events.Count;
            table.WindowOffers[s] = offer;
        }
    }
    private void Pause(Table table)
    {
        if (table.Paused) return;
        table.Paused = true; table.PausedAt = Now; Broadcast(table, Wire.Encode("PAUSED", ("show", "1")));
    }
    private void Resume(Table table)
    {
        if (!table.Paused || table.Record.VoteUntil > 0 || table.Record.Humans.Any(t => presence[t].Peer is null)) return;
        long now = Now, delay = now - table.PausedAt;
        if (table.Due != 0) table.Due += delay;
        if (table.NextEvent != 0) table.NextEvent += delay;
        for (int s = 0; s < 4; ++s)
        {
            if (table.Deadlines[s] != 0) table.Deadlines[s] += delay;
            if (table.Started[s] != 0) table.Started[s] += delay;
        }
        if (table.Record.Stage is "gok" or "prepare") table.Due = now + 10000;
        table.Paused = false; Broadcast(table, Wire.Encode("PAUSED", ("show", "0")));
        foreach (string token in table.Record.Humans) Clock(table, token); Save();
    }
    private void Clock(Table table, string token)
    {
        int seat = table.Record.Humans.IndexOf(token);
        if (table.Paused || table.Deadlines[seat] == 0 || table.Match.GetActions(seat).Count == 0) return;
        Send(token, Wire.Encode("CLOCK", ("ms", Math.Max(0, table.Deadlines[seat] - Now)),
            ("base_ms", Math.Max(0, BaseTime(table) - table.Record.Elapsed[seat] - (Now - table.Started[seat])))));
    }
    private void Restore(Table table, string token)
    {
        Send(token, Wire.Encode("RESTORE")); Identity(table, token); int seat = table.Record.Humans.IndexOf(token);
        RefreshWindows(table);
        int begin = 0;
        for (int i = 0; i < table.Cursor; ++i) if (table.Match.Events[i].Kind is "INIT" or "MATCHVIEW") begin = i;
        for (int i = begin; i < table.Cursor; ++i) Send(token, table.Presentation.Project(table.Match.Events[i], seat,
            table.Presentation.GameTag == "VGO" ? table.Windows[seat] : table.Match.Events.Count));
        if (table.Presentation.GameTag != "VGO") table.Windows[seat] = table.Match.GetActions(seat).Count == 0 ? 0 : table.Match.Events.Count;
        Send(token, Wire.Encode("RESTORED"));
        if (table.Paused) Send(token, Wire.Encode("PAUSED", ("show", "1"))); else Clock(table, token);
        if (table.Record.Stage == "gok") Send(token, Wire.Encode("READYREQUEST", ("kind", "gok")));
        if (table.Record.Stage == "prepare" && presence[token].Presentation) Send(token, Wire.Encode("READYREQUEST", ("kind", "start")));
        if (table.Record.VoteUntil > 0) ExitSnapshot(table, token);
    }
    private void Settle(Table table)
    {
        int[] seats = Enumerable.Range(0, table.Match.PlayerCount).OrderByDescending(s => table.Match.Scores[s]).ToArray();
        int category = table.Match.PlayerCount == 3 ? 1 : 0;
        double average = table.Record.Ranked ? table.Record.Humans.Average(t => accounts[sessions[t].Account].Ranks[category].Rating) : 1500;
        for (int s = 0; s < table.Record.Humans.Count; ++s)
        {
            Session session = sessions[table.Record.Humans[s]]; if (session.Account.Length == 0) continue;
            Account account = accounts[session.Account]; ++account.Played;
            if (table.Record.Ranked) account.Ranks[category].Update(Array.IndexOf(seats, s), table.Match.PlayerCount,
                table.Record.Options.GetValueOrDefault("length") == "south", average);
        }
        table.Record.Settled = true;
        // Archives are idempotent by table id. Write them before committing the
        // marker and account totals together, so recovery can retry a failed archive.
        store.SaveText("games/" + table.Record.Id + "." + table.Presentation.ReplayExtension, table.Presentation.Replay(table.Match, Names(table)));
        store.Save("game-" + table.Record.Id + ".json", table.Record);
        Save();
        foreach (string token in table.Record.Humans) Profile(sessions[token]);
        store.Log("table_complete", table.Match.PlayerCount);
    }
    public void Tick()
    {
        TransferReplays();
        foreach (Table table in tables.Values.ToArray())
        {
            if (table.Record.VoteUntil > 0 && DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() >= table.Record.VoteUntil) ResolveExit(table);
            if (!tables.ContainsKey(table.Record.Id) || table.Paused || table.Record.Stage == "complete") continue;
            TickTable(table);
        }
        Matchmake();
        if (Now - checkpoint >= 1000) { if (tables.Count > 0) Save(); checkpoint = Now; }
    }
    private void TickTable(Table table)
    {
        TableRecord r = table.Record; long now = Now;
        if (r.Stage == "gok")
        {
            if (r.Humans.All(table.Ready.Contains)) Start(table);
            else if (now >= table.Due) { Pause(table); Broadcast(table, Wire.Encode("TABLEERROR", ("code", "READY_TIMEOUT"))); }
            return;
        }
        if (r.Stage == "prepare")
        {
            if (r.Humans.Where(t => presence[t].Presentation).All(table.Ready.Contains))
            { r.Stage = "deal"; table.Due = now + 3200; Save(); Broadcast(table, Wire.Encode("START")); }
            else if (now >= table.Due) { Pause(table); Broadcast(table, Wire.Encode("TABLEERROR", ("code", "READY_TIMEOUT"))); }
            return;
        }
        if (r.Stage == "deal") { if (now < table.Due) return; r.Stage = "play"; table.Due = 0; }
        if (r.Stage == "next") { if (r.Humans.All(table.Ready.Contains)) Start(table); return; }
        Flush(table); if (table.Cursor < table.Match.Events.Count) return;
        if (table.Match.IsComplete) { r.Stage = "complete"; Save(); return; }
        if (table.Match.HandEnded) { r.Stage = "next"; table.Ready.Clear(); Save(); return; }
        for (int s = 0; s < r.Humans.Count; ++s)
        {
            IReadOnlyList<MatchAction> actions = table.Match.GetActions(s); string offer = Offer(table, s);
            if (offer.Length == 0) { table.Offers[s] = ""; table.Deadlines[s] = table.Started[s] = 0; r.Elapsed[s] = 0; continue; }
            if (table.Offers[s] != offer)
            {
                table.Offers[s] = offer; table.Started[s] = now; r.Elapsed[s] = 0; table.Deadlines[s] = now + BaseTime(table) + r.Reserve[s];
                Save(); Clock(table, r.Humans[s]);
            }
            if (now >= table.Deadlines[s])
            {
                MatchAction fallback = actions.FirstOrDefault(a => a.Kind == "skip") ?? actions.FirstOrDefault(a => a.Kind == "discard") ?? actions[0];
                r.Reserve[s] = 0; Apply(table, s, fallback); Save(); Flush(table); return;
            }
        }
        for (int s = r.Humans.Count; s < table.Match.PlayerCount; ++s)
        {
            if (table.Match.GetActions(s).Count == 0) continue;
            if (table.Due == 0) { table.Due = now + Math.Max(table.Presentation.SupportsAnimation && r.Humans.Any(t => presence[t].Presentation) ? 900 : 0, settings.BotDelayMs); return; }
            if (now < table.Due) return;
            Apply(table, s, table.Match.ChooseBotAction(s)); table.Due = 0; Save(); Flush(table); return;
        }
    }
}
