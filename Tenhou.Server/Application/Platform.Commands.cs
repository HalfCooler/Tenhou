using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules;

namespace Tenhou.Server.Application;

public sealed partial class Platform
{
    private void Command(Session session, WireMessage message, string raw)
    {
        string tag = message.Tag; Presence current = presence[session.Token];
        if (tag is "ROOMCREATE" or "ROOMJOIN" or "ROOMLEAVE" or "ROOMCLOSE" or "SETROOM")
        { RoomCommand(session, message, raw); return; }
        if (tag == "PRESENTATION") { current.Presentation = true; return; }
        if (tag == "GETREPLAY") { SendReplay(session, message); return; }
        if (tag == "CHAT")
        {
            const string prefix = "%2Flobby%20";
            string text = message.Get("text");
            if (!text.StartsWith(prefix, StringComparison.Ordinal) || text[prefix.Length..] != session.Room.ToString(CultureInfo.InvariantCulture))
                throw new ArgumentException("Use confirmed room membership");
            if (session.Table.Length == 0) Lobby(session); return;
        }
        if (tag == "JOIN")
        {
            if (message.Attributes.Count == 0) { if (session.Table.Length == 0) current.Queued = false; return; }
            if (session.Table.Length != 0 || current.Queued) throw new ArgumentException("Already queued or playing");
            int[] fields = message.Get("t").Split(',').Select(s => int.Parse(s, CultureInfo.InvariantCulture)).ToArray();
            if (fields.Length != 2 || fields[0] != session.Room) throw new ArgumentException("Room mismatch");
            int flags;
            if (session.Room != 0)
            {
                Room room = rooms[session.Room];
                if (message.Int("revision", 1) != room.Revision) { Send(session.Token, Wire.Encode("QUEUEREJECT", ("code", "REVISION_CONFLICT"))); RoomSnapshot(session); return; }
                if (sessions.Values.Any(s => s.Room == session.Room && s.Table.Length > 0))
                { Send(session.Token, Wire.Encode("QUEUEREJECT", ("code", "IN_PROGRESS"))); return; }
                flags = presentations.Create(rules.Get(room.Rule).Descriptor, room.Options).DisplayFlags;
            }
            else
            {
                if (!queues.TryGetValue(fields[1], out QueueDefinition? queue)) throw new ArgumentException("Unknown queue");
                flags = presentations.Create(rules.Get(queue.Rule).Descriptor, queue.Options).DisplayFlags;
            }
            current.Queue = fields[1]; current.Queued = true; current.QueuedAt = Now; current.Order = ++order;
            Send(session.Token, Wire.Encode("QUEUED", ("room", session.Room), ("type", flags))); Matchmake(); return;
        }
        if (tag == "BYE")
        {
            if (session.Table.Length == 0 || !tables[session.Table].Match.IsComplete) throw new ArgumentException("Match is not complete");
            string id = session.Table; session.Table = "";
            if (sessions.Values.All(s => s.Table != id)) tables.Remove(id);
            Save(); return;
        }
        if (session.Table.Length == 0 && current.Exited && tag is "D" or "N" or "REACH" or "GOK" or "STARTREADY" or "NEXTREADY") return;
        if (session.Table.Length == 0) throw new ArgumentException("No active table");
        Table table = tables[session.Table]; int seat = table.Record.Humans.IndexOf(session.Token);
        if (tag is "EXIT" or "EXITVOTE") { ExitCommand(table, session, message); return; }
        // Idempotent barriers tolerate a late animation completion after recovery.
        if (tag is "GOK" or "STARTREADY" or "NEXTREADY")
        {
            if (tag == "NEXTREADY" && table.Match.HandEnded && table.Record.Stage == "play")
            { table.Record.Stage = "next"; table.Ready.Clear(); }
            if ((tag == "GOK" && table.Record.Stage == "gok") || (tag == "STARTREADY" && table.Record.Stage == "prepare")
                || (tag == "NEXTREADY" && table.Record.Stage == "next")) table.Ready.Add(session.Token);
            Send(session.Token, Wire.Encode("READYACK", ("table", table.Record.Id), ("kind", tag)));
            return;
        }
        if (table.Paused) { Restore(table, session.Token); return; }
        if (table.Record.Stage != "play") throw new ArgumentException("Table not accepting actions");
        if (table.Presentation.GameTag == "VGO" && message.Get("table") != table.Record.Id) return;
        if (message.Int("window", 1) != table.Windows[seat]) { table.Declared[seat] = -1; Restore(table, session.Token); return; }
        Dictionary<string, string> attributes = new(message.Attributes); attributes.Remove("window");
        WireMessage actionMessage = new(tag, attributes); IReadOnlyList<MatchAction> actions = table.Match.GetActions(seat);
        if (tag == "REACH" && attributes.Count == 1 && table.Declared[seat] < 0)
        {
            int tile = actionMessage.Int("hai", 0, 135);
            if (!actions.Any(a => a.Kind == "riichi" && a.Tiles.SequenceEqual(new[] { tile }))) throw new ArgumentException("Illegal riichi");
            table.Declared[seat] = tile; return;
        }
        MatchAction action = table.Presentation.Decode(actionMessage, actions, table.Declared[seat]);
        if (table.Declared[seat] >= 0 && action.Kind != "riichi") throw new ArgumentException("Riichi requires selected discard");
        if (!actions.Any(a => a.Equivalent(action))) throw new ArgumentException("Illegal or stale action");
        table.Declared[seat] = -1;
        if (table.Started[seat] != 0)
        {
            int spent = table.Record.Elapsed[seat] + (int)(Now - table.Started[seat]);
            table.Record.Reserve[seat] = Math.Max(0, table.Record.Reserve[seat] - Math.Max(0, spent - BaseTime(table)));
        }
        Apply(table, seat, action); Save(); Flush(table);
    }
    private void RoomCommand(Session session, WireMessage message, string raw)
    {
        long rid = message.Int("rid", 1);
        if (rid == session.LastRequest)
        {
            if (session.RequestText != raw) throw new ArgumentException("Request id reused");
            Send(session.Token, session.RequestReply); Lobby(session, rid.ToString(CultureInfo.InvariantCulture)); return;
        }
        if (rid < session.LastRequest) throw new ArgumentException("Stale request id");
        if (session.Table.Length > 0 || presence[session.Token].Queued)
        { Send(session.Token, Wire.Encode("REJECT", ("rid", rid), ("code", "BUSY"))); return; }
        string? rejection = null; List<Session> changed = [];
        Dictionary<string, string> options = new(message.Attributes);
        options.Remove("rid"); options.Remove("revision");
        if (message.Tag == "ROOMCREATE")
        {
            if (session.Room != 0) rejection = "LEAVE_FIRST";
            else if (rooms.Count >= 9000) rejection = "ROOM_CAPACITY";
            else
            {
                string rule = message.Get("rule", "riichi");
                Dictionary<string, string> normalized = new(rules.Get(rule).Normalize(options));
                int number = Random.Shared.Next(1000, 10000);
                while (rooms.ContainsKey(number)) number = 1000 + (number - 999) % 9000;
                rooms.Add(number, new() { Number = number, Owner = session.Token, Rule = rule, Options = normalized }); session.Room = number;
            }
        }
        else if (message.Tag == "ROOMJOIN")
        {
            int number = message.Int("room", 1000, 9999);
            if (session.Room != 0) rejection = "LEAVE_FIRST";
            else if (!rooms.TryGetValue(number, out Room? room)) rejection = "NOT_FOUND";
            else if (sessions.Values.Any(s => s.Room == number && s.Table.Length > 0)) rejection = "IN_PROGRESS";
            else if (sessions.Values.Count(s => s.Room == number) >= rules.Get(room.Rule).Create(room.Options, new Random()).PlayerCount) rejection = "FULL";
            else session.Room = number;
        }
        else if (session.Room == 0 || !rooms.TryGetValue(session.Room, out Room? room)) rejection = "NOT_FOUND";
        else if (sessions.Values.Any(s => s.Room == session.Room && (s.Table.Length > 0 || presence[s.Token].Queued))) rejection = "IN_PROGRESS";
        else if (message.Tag == "ROOMLEAVE")
        {
            if (room.Owner == session.Token)
            {
                Session? successor = sessions.Values.FirstOrDefault(s => s.Room == session.Room && s.Token != session.Token);
                if (successor is null) rooms.Remove(room.Number); else { room.Owner = successor.Token; changed.Add(successor); }
            }
            session.Room = 0;
        }
        else if (room.Owner != session.Token) rejection = "OWNER_REQUIRED";
        else if (message.Tag == "ROOMCLOSE")
        {
            foreach (Session member in sessions.Values.Where(s => s.Room == room.Number)) { member.Room = 0; changed.Add(member); }
            rooms.Remove(room.Number);
        }
        else if (message.Tag == "SETROOM")
        {
            if (message.Int("revision", 1) != room.Revision) rejection = "REVISION_CONFLICT";
            else
            {
                string rule = message.Get("rule", room.Rule); Dictionary<string, string> normalized = new(rules.Get(rule).Normalize(options));
                int players = rules.Get(rule).Create(normalized, new Random()).PlayerCount;
                if (sessions.Values.Count(s => s.Room == room.Number) > players) rejection = "FULL";
                else { room.Options = normalized; room.Rule = rule; ++room.Revision; changed.AddRange(sessions.Values.Where(s => s.Room == room.Number)); }
            }
        }
        session.LastRequest = rid; session.RequestText = raw;
        session.RequestReply = rejection is null ? Wire.Encode("ACCEPT", ("rid", rid), ("room", session.Room))
            : Wire.Encode("REJECT", ("rid", rid), ("code", rejection));
        Save(); Send(session.Token, session.RequestReply); Lobby(session, rid.ToString(CultureInfo.InvariantCulture));
        foreach (Session member in changed.Distinct().Where(s => s.Token != session.Token)) Lobby(member);
    }
    private void SendReplay(Session session, WireMessage message)
    {
        if (session.Account.Length == 0 || session.Table.Length == 0 || message.Get("table") != session.Table || !tables[session.Table].Match.IsComplete)
        { Send(session.Token, Wire.Encode("REPLAYERROR", ("text", "仅注册玩家可下载本人已完成的对局"))); return; }
        Table table = tables[session.Table]; byte[] bytes = Encoding.UTF8.GetBytes(table.Presentation.Replay(table.Match, Names(table)));
        if (bytes.Length > 8 * 1024 * 1024) throw new ArgumentException("Replay exceeds limit");
        // Feed one chunk per tick so a large archive cannot overflow the output budget.
        replayTransfers[session.Token] = (table.Record.Id, bytes, 0);
        Send(session.Token, Wire.Encode("REPLAYBEGIN", ("table", table.Record.Id), ("bytes", bytes.Length)));
    }
    private readonly Dictionary<string, (string Id, byte[] Bytes, int Offset)> replayTransfers = [];
    private void TransferReplays()
    {
        foreach ((string token, (string id, byte[] bytes, int offset)) in replayTransfers.ToArray())
        {
            if (presence[token].Peer is null) { replayTransfers.Remove(token); continue; }
            int count = Math.Min(1400, bytes.Length - offset);
            Send(token, Wire.Encode("REPLAYCHUNK", ("table", id), ("offset", offset), ("data", Convert.ToHexStringLower(bytes.AsSpan(offset, count)))));
            if (offset + count == bytes.Length) { Send(token, Wire.Encode("REPLAYEND", ("table", id))); replayTransfers.Remove(token); }
            else replayTransfers[token] = (id, bytes, offset + count);
        }
    }
    private void ExitCommand(Table table, Session session, WireMessage message)
    {
        TableRecord r = table.Record;
        if (message.Get("table") != r.Id || table.Match.IsComplete) return;
        if (r.Ranked) return;
        int seat = r.Humans.IndexOf(session.Token);
        if (r.VoteUntil > 0 && DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() >= r.VoteUntil) { ResolveExit(table); return; }
        if (message.Tag == "EXIT")
        {
            if (r.Humans.Count == 1) { EndExit(table); return; }
            if (r.VoteUntil == 0)
            { ++r.VoteRevision; r.VoteUntil = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() + 30000; r.Votes = [-1, -1, -1, -1]; Pause(table); Save(); }
        }
        else
        {
            if (r.VoteUntil == 0 || message.Int("id", 1) != r.VoteRevision) return;
            if (r.Votes[seat] < 0) { r.Votes[seat] = message.Int("yes", 0, 1); Save(); }
            if (r.Votes.Take(r.Humans.Count).All(v => v >= 0)) { ResolveExit(table); return; }
        }
        foreach (string token in r.Humans) ExitSnapshot(table, token);
    }
    private void ExitSnapshot(Table table, string token)
    {
        TableRecord r = table.Record;
        Send(token, Wire.Encode("EXITVOTE", ("table", r.Id), ("show", "1"), ("id", r.VoteRevision),
            ("ms", Math.Clamp(r.VoteUntil - DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(), 0, 30000)), ("total", r.Humans.Count),
            ("yes", r.Votes.Take(r.Humans.Count).Count(v => v == 1)), ("no", r.Votes.Take(r.Humans.Count).Count(v => v == 0)),
            ("choice", r.Votes[r.Humans.IndexOf(token)])));
    }
    private void ResolveExit(Table table)
    {
        if (table.Record.Votes.Take(table.Record.Humans.Count).Count(v => v == 1) * 2 > table.Record.Humans.Count) { EndExit(table); return; }
        table.Record.VoteUntil = 0; Save();
        Broadcast(table, Wire.Encode("EXITVOTE", ("table", table.Record.Id), ("show", "0"), ("result", "continue"))); Resume(table);
    }
    private void EndExit(Table table)
    {
        foreach (string token in table.Record.Humans) { sessions[token].Table = ""; presence[token].Exited = true; presence[token].Queued = false; }
        tables.Remove(table.Record.Id); Save(); Broadcast(table, Wire.Encode("TABLEEND", ("table", table.Record.Id), ("reason", "exit")));
        foreach (string token in table.Record.Humans) Lobby(sessions[token]);
    }
}
