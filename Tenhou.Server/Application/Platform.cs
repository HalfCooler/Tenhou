using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using Tenhou.Server.Infrastructure;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules;

namespace Tenhou.Server.Application;

// All methods run on the host's single consumer. Network tasks only enqueue.
public sealed partial class Platform
{
    private sealed class Presence
    {
        public Peer? Peer;
        public bool Queued, Presentation, Exited;
        public int Queue;
        public long QueuedAt, Order;
    }
    private sealed class Table
    {
        public required TableRecord Record;
        public required IMatch Match;
        public required IMatchPresentation Presentation;
        public HashSet<string> Ready = [];
        public int Cursor;
        public bool Paused;
        public long PausedAt, Due, NextEvent;
        public long[] Deadlines = new long[4], Started = new long[4];
        public string[] Offers = ["", "", "", ""];
        public string[] WindowOffers = ["", "", "", ""];
        public int[] Windows = new int[4], Declared = [-1, -1, -1, -1];
    }
    private readonly ServerSettings settings;
    private readonly JsonStore store;
    private readonly RuleRegistry rules;
    private readonly PresentationRegistry presentations;
    private readonly Dictionary<int, QueueDefinition> queues;
    private readonly Dictionary<long, Peer> connections = [];
    private readonly Dictionary<string, Session> sessions = [];
    private readonly Dictionary<string, Presence> presence = [];
    private readonly Dictionary<string, Account> accounts = [];
    private readonly Dictionary<int, Room> rooms = [];
    private readonly Dictionary<string, Table> tables = [];
    private long order, checkpoint;
    private static long Now => Environment.TickCount64;
    private static string Token() => Guid.NewGuid().ToString("N") + Guid.NewGuid().ToString("N");
    public Platform(ServerSettings settings, JsonStore store, RuleRegistry rules, PresentationRegistry presentations)
    {
        this.settings = settings; this.store = store; this.rules = rules; this.presentations = presentations;
        queues = settings.Queues.Select(q => new QueueDefinition(q.Id, q.Name, q.Rule, new(rules.Get(q.Rule).Normalize(q.Options))))
            .ToDictionary(q => q.Id);
        if (queues.Count > 64 || queues.Keys.Any(k => k <= 0)) throw new InvalidDataException("Invalid queue catalog");
        Load(); checkpoint = Now;
        store.Log("ready", tables.Count);
    }
    private Table CreateTable(TableRecord record)
    {
        IRuleSet rule = rules.Get(record.Rule);
        return new() { Record = record, Match = rule.Create(record.Options, new Random()),
            Presentation = presentations.Create(rule.Descriptor, record.Options), Due = Now + 10000 };
    }
    private void Load()
    {
        ServerState? state = store.Load<ServerState>("state.json"); if (state is null) return;
        if (state.Version != 1 || state.Sessions.Count > 10000 || state.Accounts.Count > 10000 || state.Tables.Count > settings.MaxTables)
            throw new InvalidDataException("Unsupported/oversized state");
        foreach (Account account in state.Accounts)
        {
            if (Account.NormalizeId(account.Id) != account.Id || account.Ranks.Length != 2) throw new InvalidDataException("Invalid account");
            accounts.Add(account.Id, account);
        }
        foreach (Session session in state.Sessions)
        {
            if (session.Token.Length != 64 || session.Token.Any(c => !char.IsAsciiHexDigit(c))
                || (session.Account.Length > 0 && !accounts.ContainsKey(session.Account))) throw new InvalidDataException("Invalid session");
            sessions.Add(session.Token, session); presence.Add(session.Token, new());
        }
        foreach (Room room in state.Rooms)
        {
            if (room.Number is < 1000 or > 9999 || !sessions.ContainsKey(room.Owner)) throw new InvalidDataException("Invalid room");
            room.Options = new(rules.Get(room.Rule).Normalize(room.Options)); rooms.Add(room.Number, room);
        }
        foreach (TableRecord record in state.Tables)
        {
            if (record.RuleVersion != rules.Get(record.Rule).Descriptor.Version) throw new InvalidDataException("Unsupported rule version");
            if (record.Id.Length != 32 || record.Id.Any(c => !char.IsAsciiHexDigit(c)) || record.Commands.Count > 100000
                || record.Stage is not ("gok" or "prepare" or "deal" or "play" or "next" or "complete")
                || record.Humans.Count == 0 || record.Humans.Distinct().Count() != record.Humans.Count
                || record.Remaining.Length != 4 || record.Elapsed.Length != 4 || record.Reserve.Length != 4 || record.Votes.Length != 4)
                throw new InvalidDataException("Invalid table");
            Table table = CreateTable(record);
            if (record.Humans.Count > table.Match.PlayerCount) throw new InvalidDataException("Too many seats");
            foreach (MatchCommand command in record.Commands) Replay(table.Match, command);
            table.Match.Validate(); table.Cursor = table.Match.Events.Count; table.Paused = true; table.PausedAt = Now;
            if (record.Stage == "deal") record.Stage = "play";
            for (int s = 0; s < 4; ++s)
                if (record.Remaining[s] >= 0)
                { table.Deadlines[s] = table.PausedAt + record.Remaining[s]; table.Started[s] = table.PausedAt; table.Offers[s] = Offer(table, s); }
            tables.Add(record.Id, table);
            foreach (string token in record.Humans)
                if (!sessions.TryGetValue(token, out Session? session) || session.Table != record.Id) throw new InvalidDataException("Invalid seat/session reference");
        }
        foreach (Session session in sessions.Values)
        {
            if ((session.Room != 0 && !rooms.ContainsKey(session.Room)) || (session.Table.Length > 0
                && (!tables.TryGetValue(session.Table, out Table? table) || !table.Record.Humans.Contains(session.Token))))
                throw new InvalidDataException("Broken session reference");
        }
    }
    private static void Replay(IMatch match, MatchCommand command)
    {
        if (command.Kind == "start" && command.Wall is not null) match.Start(command.Wall);
        else if (command.Kind == "action" && command.Action is not null) match.Submit(command.Seat, command.Action);
        else throw new InvalidDataException("Invalid match command");
    }
    public void Save()
    {
        foreach (Table table in tables.Values)
        {
            long now = table.Paused ? table.PausedAt : Now;
            for (int s = 0; s < 4; ++s)
            {
                table.Record.Remaining[s] = table.Deadlines[s] == 0 ? -1 : (int)Math.Max(0, table.Deadlines[s] - now);
                // Runtime elapsed is kept separate to avoid counting it repeatedly at checkpoints.
            }
        }
        List<TableRecord> records = [];
        foreach (Table table in tables.Values)
        {
            // Serialization copy; live clock baselines must not be mutated by a save.
            TableRecord r = table.Record;
            records.Add(new() { Id = r.Id, Rule = r.Rule, RuleVersion = r.RuleVersion, Options = r.Options, Humans = r.Humans, Commands = r.Commands,
                Stage = r.Stage, Ranked = r.Ranked, Settled = r.Settled, Reserve = r.Reserve, Remaining = r.Remaining,
                Elapsed = Enumerable.Range(0, 4).Select(s => r.Elapsed[s] + (table.Started[s] == 0 ? 0 : (int)((table.Paused ? table.PausedAt : Now) - table.Started[s]))).ToArray(),
                VoteRevision = r.VoteRevision, VoteUntil = r.VoteUntil, Votes = r.Votes });
        }
        store.Save("state.json", new ServerState { Accounts = accounts.Values.ToList(), Sessions = sessions.Values.ToList(),
            Rooms = rooms.Values.ToList(), Tables = records });
    }
    public void Connected(Peer peer) { connections.Add(peer.Id, peer); store.Log("connected", connections.Count); }
    public void Disconnected(Peer peer)
    {
        connections.Remove(peer.Id);
        if (peer.Token.Length == 0 || !presence.TryGetValue(peer.Token, out Presence? current) || current.Peer != peer) return;
        current.Peer = null; current.Queued = false;
        Session session = sessions[peer.Token];
        if (session.Table.Length > 0 && tables.TryGetValue(session.Table, out Table? table)) Pause(table);
        Save();
        store.Log("disconnected", connections.Count);
    }
    private void Send(string token, string text) => presence[token].Peer?.Send(text);
    private void Broadcast(Table table, string text) { foreach (string token in table.Record.Humans) Send(token, text); }
    public void Receive(Peer peer, string raw)
    {
        if (!connections.ContainsKey(peer.Id)) return;
        try
        {
            WireMessage message = Wire.Parse(raw);
            if (message.Tag == "HEALTH")
            {
                peer.Send(Wire.Encode("HEALTH", ("version", "1"), ("ready", "1"), ("connections", connections.Count),
                    ("sessions", sessions.Count), ("rooms", rooms.Count), ("tables", tables.Count))); return;
            }
            if (message.Tag == "Z") { peer.Send(Wire.Encode("Z")); return; }
            if (message.Tag == "HELO" && !peer.Hello && peer.Token.Length == 0)
            {
                peer.Login = message.Get("name") == "NoName" ? "NoName" : Account.NormalizeId(message.Get("name"));
                peer.Hello = true; peer.Send(Wire.Encode("HELO", ("uname", "TENHOU-PLATFORM"), ("version", "1"), ("caps", "catalog,rooms,queue,recovery,presentation"))); return;
            }
            if (message.Tag == "CAPS" && peer.Hello && peer.Token.Length == 0) { Authenticate(peer, message); return; }
            if (peer.Token.Length == 0) throw new ArgumentException("Authentication required");
            Session session = sessions[peer.Token];
            if (message.Tag == "CATALOG") { Catalog(peer); return; }
            Command(session, message, raw);
        }
        catch (Exception error) when (error is ArgumentException or KeyNotFoundException or FormatException or XmlException or OverflowException)
        {
            peer.Send(Wire.Encode("ERR", ("code", "400"), ("text", error.Message)));
            // Invalid input cannot leave a connected seat with speculative state.
            Disconnected(peer); peer.Complete();
        }
    }
    private void Authenticate(Peer peer, WireMessage message)
    {
        if (message.Get("version") != "1") throw new ArgumentException("Protocol version mismatch");
        string token = message.Get("token"); bool register = message.Get("register") == "1";
        if (peer.Login != "NoName")
        {
            if (register)
            {
                if (accounts.ContainsKey(peer.Login)) throw new ArgumentException("账号已存在");
                if (accounts.Count >= 10000) throw new ArgumentException("账号容量已满");
                // Account creation follows admission checks so a rejected login cannot reserve an id.
                if (sessions.Count >= 10000) throw new ArgumentException("会话容量已满");
                accounts.Add(peer.Login, Account.Create(peer.Login, message.Get("nickname"), message.Get("password")));
            }
            else if (!accounts.TryGetValue(peer.Login, out Account? account) || !account.Verify(message.Get("password")))
                throw new ArgumentException("账号或密码错误");
            token = sessions.Values.FirstOrDefault(s => s.Account == peer.Login)?.Token ?? "";
        }
        else if (register) throw new ArgumentException("NoName不能注册为账号");
        else if (token.Length > 0 && (!sessions.TryGetValue(token, out Session? guest) || guest.Account.Length > 0))
            throw new ArgumentException("访客会话无效，请清除本地会话后重试");
        if (token.Length == 0)
        {
            if (sessions.Count >= 10000) throw new ArgumentException("会话容量已满");
            token = Token(); sessions.Add(token, new() { Token = token, Account = peer.Login == "NoName" ? "" : peer.Login,
                Gender = message.Get("sx") == "F" ? "F" : "M" }); presence.Add(token, new());
        }
        Presence current = presence[token];
        if (current.Peer is not null) throw new ArgumentException("该账号已在其他窗口登录");
        peer.Token = token; peer.Presentation = message.Get("presentation") == "1";
        current.Peer = peer; current.Presentation = peer.Presentation; current.Exited = false;
        Save(); peer.Send(Wire.Encode("SESSION", ("token", token), ("rid", sessions[token].LastRequest)));
        Catalog(peer); Lobby(sessions[token]);
        if (sessions[token].Table.Length > 0)
        { Table table = tables[sessions[token].Table]; Restore(table, token); Resume(table); }
    }
    private void Catalog(Peer peer)
    {
        peer.Send(Wire.Encode("CATALOGBEGIN", ("version", "1")));
        foreach (IRuleSet rule in rules.Rules)
        {
            RuleDescriptor descriptor = rule.Descriptor;
            peer.Send(Wire.Encode("RULE", ("id", descriptor.Id), ("name", descriptor.Name), ("presentation", descriptor.Presentation), ("version", descriptor.Version)));
            foreach (RuleSetting field in descriptor.Settings)
            {
                peer.Send(Wire.Encode("RULEFIELD", ("rule", descriptor.Id), ("key", field.Key), ("label", field.Label),
                    ("default", field.Default), ("min", field.Minimum?.ToString(CultureInfo.InvariantCulture) ?? ""),
                    ("max", field.Maximum?.ToString(CultureInfo.InvariantCulture) ?? ""), ("step", field.Step)));
                foreach (SettingChoice choice in field.Choices)
                    peer.Send(Wire.Encode("RULECHOICE", ("rule", descriptor.Id), ("key", field.Key), ("value", choice.Value), ("label", choice.Label)));
            }
        }
        foreach (QueueDefinition queue in queues.Values)
        {
            IMatchPresentation presentation = presentations.Create(rules.Get(queue.Rule).Descriptor, queue.Options);
            peer.Send(Wire.Encode("QUEUEDEF", ("id", queue.Id), ("name", queue.Name), ("rule", queue.Rule), ("type", presentation.DisplayFlags)));
        }
        peer.Send(Wire.Encode("CATALOGEND"));
    }
    private void Profile(Session session)
    {
        Dictionary<string, string> attrs = new() { ["account"] = session.Account, ["nickname"] = Name(session.Token) };
        if (session.Account.Length > 0)
        {
            Account account = accounts[session.Account]; attrs["played"] = account.Played.ToString(CultureInfo.InvariantCulture);
            for (int i = 0; i < 2; ++i)
            {
                string n = i == 0 ? "4" : "3"; Rank rank = account.Ranks[i];
                attrs["grade" + n] = rank.Name; attrs["points" + n] = rank.Points.ToString(CultureInfo.InvariantCulture);
                attrs["target" + n] = rank.Target.ToString(CultureInfo.InvariantCulture); attrs["games" + n] = rank.Games.ToString(CultureInfo.InvariantCulture);
                attrs["rating" + n] = rank.Rating.ToString(CultureInfo.InvariantCulture);
            }
        }
        Send(session.Token, Wire.Encode("PROFILE", attrs));
    }
    private string Name(string token) => sessions[token].Account.Length == 0 ? "NoName" : accounts[sessions[token].Account].Nickname;
    private void Lobby(Session session, string rid = "")
    {
        Send(session.Token, Wire.Encode("CS", ("lobby", "L" + session.Room.ToString("D4", CultureInfo.InvariantCulture)), ("rid", rid)));
        if (session.Room != 0) RoomSnapshot(session, rid); Profile(session);
    }
    private void RoomSnapshot(Session session, string rid = "")
    {
        Room room = rooms[session.Room]; Dictionary<string, string> attrs = new(room.Options)
        {
            ["rule"] = room.Rule, ["room"] = room.Number.ToString(CultureInfo.InvariantCulture), ["revision"] = room.Revision.ToString(CultureInfo.InvariantCulture),
            ["owner"] = room.Owner == session.Token ? "1" : "0", ["rid"] = rid,
            ["type"] = presentations.Create(rules.Get(room.Rule).Descriptor, room.Options).DisplayFlags.ToString(CultureInfo.InvariantCulture)
        };
        Send(session.Token, Wire.Encode("ROOM", attrs));
    }
}
