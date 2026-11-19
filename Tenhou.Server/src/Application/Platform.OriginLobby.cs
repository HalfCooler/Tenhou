using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Tenhou.Server.Infrastructure;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules.Riichi;

namespace Tenhou.Server.Application;

public sealed partial class Platform
{
    private const string OriginRankedLobby = "L0000";
    private const int TierBits = 0xA0;
    private sealed record RankedRule(int Slot, int Tier, RiichiOptions Options);
    private static readonly Dictionary<int, RankedRule> OriginTypes = CreateOriginTypes();
    private readonly Dictionary<string, ContestSettings> contests;
    private readonly TimeProvider originClock;
    private long nextLobbyCounts;

    private static Dictionary<int, RankedRule> CreateOriginTypes()
    {
        (int Type, int Slot, int Stride)[] slots =
        [
            (0x7, 3, 16), (0xF, 7, 16), (0x1, 0, 16), (0x9, 4, 16),
            (0x41, 8, 16), (0x49, 12, 16),
            (0x11, 64, 2), (0x19, 65, 2), (0x51, 72, 2), (0x59, 73, 2)
        ];
        int[] tiers = [0, 0x80, 0x20, 0xA0];
        Dictionary<int, RankedRule> result = [];
        for (int tier = 0; tier < tiers.Length; ++tier)
        foreach (var (type, slot, stride) in slots)
        {
            bool three = (type & 0x10) != 0;
            result.Add(type | tiers[tier], new(slot + tier * stride, tier, new RiichiOptions
            {
                MatchType = three ? "standard3" : "standard4",
                Length = (type & 8) != 0 ? "south" : "east",
                Time = (type & 0x40) != 0 ? "3+5" : "5+20",
                Fast = (type & 0x40) != 0,
                OpenTanyao = (type & 4) == 0,
                RedCount = (type & 2) != 0 ? 0 : three ? 2 : 3,
                Starting = three ? 35000 : 25000,
                Target = three ? 40000 : 30000
            }));
        }
        return result;
    }

    private static bool IsContest(string id) => id.Length == 9 && id[0] == 'C' && id.Skip(1).All(char.IsAsciiDigit);
    private static bool IsOriginLobby(string id) => IsContest(id) || id == OriginRankedLobby ||
        (id.Length == 5 && id[0] == 'L' && id[1] != '0' && id.Skip(1).All(char.IsAsciiDigit));
    private static int MembershipDays(Account? account, DateTimeOffset now) =>
        account?.MembershipUntil is DateTimeOffset until ? (int)Math.Max(0, Math.Ceiling((until - now).TotalDays)) : 0;
    private Account? LobbyAccount(Session session) => accounts.GetValueOrDefault(session.Account);
    private static bool IsRegistered(Session session, ContestSettings contest) => contest.JoinFee == 0 ||
        (session.ContestEntries.TryGetValue(contest.Id, out DateTimeOffset startsAt) && startsAt == contest.StartsAt);
    private static bool ContestBlocked(ContestSettings contest, Account? account, DateTimeOffset now) =>
        (contest.PremiumOnly && MembershipDays(account, now) == 0) || (contest.JoinFee > 0 && account is null);

    private string ReservationRejection(Session session, int type, DateTimeOffset now)
    {
        if (!OriginTypes.TryGetValue(type, out RankedRule? rule)) return "UNSUPPORTED_TYPE";
        string lobby = session.OriginLobby;
        Account? account = LobbyAccount(session);
        if (IsContest(lobby))
        {
            if (!contests.TryGetValue(lobby, out ContestSettings? contest) || now >= contest.EndsAt) return "CLOSED";
            if (contest.Type != type) return "UNSUPPORTED_TYPE";
            if (ContestBlocked(contest, account, now)) return "INELIGIBLE";
            if (!IsRegistered(session, contest)) return "REGISTER_FIRST";
            return now < contest.StartsAt ? "BEFORE_START" : "";
        }
        if (lobby != OriginRankedLobby)
            return rule.Tier == 0 ? "" : "UNSUPPORTED_TYPE";
        if (account is null) return "ACCOUNT_REQUIRED";
        Rank rank = account.Ranks[rule.Options.Players == 3 ? 1 : 0];
        bool special = rank.Level >= 13 && rank.Rating >= 1800;
        bool phoenix = rank.Level >= 16 && rank.Rating >= 2000;
        bool eligible = rule.Tier switch
        {
            0 => !special,
            1 => (rank.Level >= 9 || MembershipDays(account, now) >= 60) && !phoenix,
            2 => special,
            3 => phoenix,
            _ => false
        };
        return eligible ? "" : "INELIGIBLE";
    }

    private void OriginSnapshot(Session session, string rid)
    {
        Presence current = presence[session.Token];
        Profile(session);
        Send(session.Token, Wire.Encode("CHAT", ("lobby", session.OriginLobby), ("epoch", current.LobbyEpoch), ("rid", rid),
            ("seated", session.Table.Length == 0 ? 0 : 1)));
        SendContest(session);
        SendReservations(session);
        SendLobbyCounts(session.OriginLobby, [session]);
    }

    private void SendContest(Session session)
    {
        if (!contests.TryGetValue(session.OriginLobby, out ContestSettings? contest)) return;
        bool blocked = ContestBlocked(contest, LobbyAccount(session), originClock.GetUtcNow());
        Send(session.Token, Wire.Encode("CS", ("lobby", contest.Id), ("epoch", presence[session.Token].LobbyEpoch),
            ("title", contest.Title), ("players", OriginTypes[contest.Type].Options.Players), ("rule", contest.Type),
            ("premiumonly", contest.PremiumOnly ? 1 : 0), ("joinfee", contest.JoinFee),
            ("registered", IsRegistered(session, contest) ? 1 : 0), ("blocked", blocked ? 1 : 0),
            ("start", contest.StartsAt.ToUnixTimeSeconds()), ("end", contest.EndsAt.ToUnixTimeSeconds()),
            ("now", originClock.GetUtcNow().ToUnixTimeSeconds())));
    }

    private void SendReservations(Session session, string rid = "", string code = "")
    {
        Presence current = presence[session.Token];
        Send(session.Token, Wire.Encode("RESERVATIONS", ("lobby", session.OriginLobby), ("epoch", current.LobbyEpoch),
            ("types", string.Join(',', current.Reservations.Keys.Order())), ("rid", rid), ("code", code)));
    }

    private bool OriginCommand(Session session, WireMessage message)
    {
        string tag = message.Tag;
        if (tag is not ("CHAT" or "CS" or "JOIN" or "CSREG" or "ROOMCREATE" or "ROOMJOIN" or "ROOMLEAVE" or "ROOMCLOSE" or "SETROOM"))
            return false;
        // Identity/GO is already enqueued if the final seat won the race with cancellation.
        if (session.Table.Length > 0) return true;
        Presence current = presence[session.Token];
        string rid = message.Int("rid", 1).ToString(CultureInfo.InvariantCulture);
        string code;
        if (message.Get("lobby") != session.OriginLobby || message.Int("epoch", 1) != current.LobbyEpoch)
            code = "STALE_LOBBY";
        else if (tag is "CHAT" or "CS")
        {
            const string prefix = "%2Flobby%20";
            string text = message.Get("text"), room = message.Get("target");
            if (tag == "CHAT")
                room = text.StartsWith(prefix, StringComparison.Ordinal) &&
                    int.TryParse(text.AsSpan(prefix.Length), NumberStyles.None, CultureInfo.InvariantCulture, out int n)
                    && n is 0 or (>= 1000 and <= 9999) ? "L" + n.ToString("D4", CultureInfo.InvariantCulture) : "";
            if (current.Reservations.Count > 0) code = "BUSY";
            else if (!IsOriginLobby(room) || (tag == "CS" && !contests.ContainsKey(room))) code = "LOBBY_NOT_FOUND";
            else
            {
                session.OriginLobby = room;
                ++current.LobbyEpoch;
                Save();
                OriginSnapshot(session, rid);
                return true;
            }
        }
        else if (tag == "CSREG") code = RegisterContest(session);
        else if (tag == "JOIN")
        {
            if (!message.Attributes.ContainsKey("type") && !message.Attributes.ContainsKey("t"))
            {
                current.Reservations.Clear(); code = "";
            }
            else
            {
                int type;
                if (IsContest(session.OriginLobby)) type = message.Int("type", 1, 65535);
                else
                {
                    string[] fields = message.Get("t").Split(',');
                    int room = int.Parse(session.OriginLobby.AsSpan(1), CultureInfo.InvariantCulture);
                    if (fields.Length != 2 || fields[0] != room.ToString(CultureInfo.InvariantCulture) ||
                        !int.TryParse(fields[1], NumberStyles.None, CultureInfo.InvariantCulture, out type))
                    { SendReservations(session, rid, "STALE_LOBBY"); return true; }
                }
                code = ReservationRejection(session, type, originClock.GetUtcNow());
                if (code.Length == 0 && !current.Reservations.ContainsKey(type))
                    current.Reservations.Add(type, ++order);
            }
        }
        else code = "UNSUPPORTED_COMMAND";
        SendReservations(session, rid, code);
        if (tag == "JOIN") MatchmakeOrigin();
        return true;
    }

    private string RegisterContest(Session session)
    {
        DateTimeOffset now = originClock.GetUtcNow();
        if (!contests.TryGetValue(session.OriginLobby, out ContestSettings? contest) || now >= contest.EndsAt) return "CLOSED";
        if (ContestBlocked(contest, LobbyAccount(session), now)) return "INELIGIBLE";
        if (!IsRegistered(session, contest))
        {
            foreach (string id in session.ContestEntries.Keys.Where(id => !contests.ContainsKey(id)).ToArray())
                session.ContestEntries.Remove(id);
            session.ContestEntries[contest.Id] = contest.StartsAt;
            Save();
        }
        SendContest(session);
        return "";
    }

    private IEnumerable<Session> OriginVisitors() => connections.Values
        .Where(peer => peer.Token.Length > 0 && presence[peer.Token].Origin)
        .Select(peer => sessions[peer.Token]).Where(s => s.Table.Length == 0);

    private string PruneReservations(Session session, DateTimeOffset now)
    {
        string rejection = "";
        var reservations = presence[session.Token].Reservations;
        foreach (int type in reservations.Keys.ToArray())
        {
            string code = ReservationRejection(session, type, now);
            if (code.Length == 0) continue;
            reservations.Remove(type);
            rejection = code;
        }
        return rejection;
    }

    private void MatchmakeOrigin()
    {
        DateTimeOffset now = originClock.GetUtcNow();
        var visitors = OriginVisitors().ToArray();
        foreach (Session session in visitors)
        {
            string code = PruneReservations(session, now);
            if (code.Length == 0) continue;
            SendContest(session);
            SendReservations(session, code: code);
        }
        var groups = visitors.SelectMany(s => presence[s.Token].Reservations.Select(ticket =>
                (Session: s, Type: ticket.Key, Order: ticket.Value)))
            .GroupBy(t => (Lobby: t.Session.OriginLobby, t.Type)).OrderBy(g => g.Min(t => t.Order));
        foreach (var group in groups)
        {
            if (tables.Count >= settings.MaxTables) break;
            RiichiOptions options = OriginTypes[group.Key.Type].Options;
            Session[] waiting = group.OrderBy(t => t.Order).Select(t => t.Session).ToArray();
            int offset = 0;
            while (tables.Count < settings.MaxTables)
            {
                Session[] seats = waiting.Skip(offset).Where(s => s.Table.Length == 0).Take(options.Players).ToArray();
                if (seats.Length < options.Players) break;
                offset = Array.IndexOf(waiting, seats[^1], offset) + 1;
                Table table = CreateTable(new()
                {
                    Id = Guid.NewGuid().ToString("N"), Rule = "riichi", RuleVersion = rules.Get("riichi").Descriptor.Version,
                    Options = options.ToSettings(), Humans = seats.Select(s => s.Token).ToList(),
                    Ranked = group.Key.Lobby == OriginRankedLobby, OriginLobby = group.Key.Lobby, ReservationType = group.Key.Type
                });
                CommitTable(table);
            }
        }
    }

    private void TickOrigin()
    {
        MatchmakeOrigin();
        if (Now < nextLobbyCounts) return;
        nextLobbyCounts = Now + 1000;
        foreach (var room in OriginVisitors().GroupBy(s => s.OriginLobby))
            SendLobbyCounts(room.Key, room);
    }

    private void SendLobbyCounts(string lobby, IEnumerable<Session> recipients)
    {
        int[] n = new int[4], j = new int[113], g = new int[113];
        foreach (Session visitor in OriginVisitors().Where(s => s.OriginLobby == lobby))
        {
            ++n[0];
            var requests = presence[visitor.Token].Reservations;
            if (requests.Count > 0) ++n[2];
            foreach (int type in requests.Keys) ++j[OriginTypes[type].Slot];
        }
        foreach (Table table in tables.Values.Where(t => t.Record.OriginLobby == lobby && !t.Match.IsComplete))
        {
            ++g[OriginTypes[table.Record.ReservationType].Slot];
            n[0] += table.Record.Humans.Count(t => presence[t].Peer is not null);
        }
        string totals = string.Join(',', n), waiting = string.Join(',', j), games = string.Join(',', g);
        foreach (Session session in recipients)
            Send(session.Token, Wire.Encode("LN", ("lobby", lobby), ("epoch", presence[session.Token].LobbyEpoch),
                ("n", totals), ("j", waiting), ("g", games)));
    }
}
