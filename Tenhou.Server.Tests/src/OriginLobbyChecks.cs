using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using Tenhou.Server.Application;
using Tenhou.Server.Infrastructure;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules;
using Tenhou.Server.Rules.Riichi;

namespace Tenhou.Server.Tests;

internal static partial class Program
{
    private sealed class LobbyClock(DateTimeOffset start) : TimeProvider
    {
        private long seconds = start.ToUnixTimeSeconds();
        public override DateTimeOffset GetUtcNow() => DateTimeOffset.FromUnixTimeSeconds(Interlocked.Read(ref seconds));
        public void Advance(int delta) => Interlocked.Add(ref seconds, delta);
    }

    private static async Task OriginLobbyChecks(string? nativeClient)
    {
        string directory = Path.Combine(Path.GetTempPath(), "tenhou-origin-lobby-" + Guid.NewGuid().ToString("N"));
        TcpListener reserve = new(IPAddress.Loopback, 0);
        reserve.Start();
        int port = ((IPEndPoint)reserve.LocalEndpoint).Port;
        reserve.Stop();
        LobbyClock clock = new(DateTimeOffset.UtcNow);
        DateTimeOffset now = clock.GetUtcNow();
        ServerSettings settings = new()
        {
            Port = port, DataDirectory = directory, MatchWaitMs = 0,
            Contests =
            [
                new("C00112233", "报名大会", 0x11, now.AddSeconds(-1), now.AddMinutes(10), JoinFee: 10),
                new("C00223344", "限时大会", 0x19, now.AddSeconds(10), now.AddSeconds(40)),
                new("C00334455", "会员大会", 0x1, now.AddSeconds(-1), now.AddMinutes(10), PremiumOnly: true)
            ]
        };
        var registry = Registries();
        using JsonStore store = new(directory);
        Account member = Account.Create("origin_member", "会员", "test-password");
        member.MembershipUntil = now.AddDays(60);
        ServerState seed = new() { Accounts = [member] };
        var ending = SeedContestEnding(seed);
        store.Save("state.json", seed);

        using CancellationTokenSource stop = new();
        Task host = new TcpHost(settings, new Platform(settings, store, registry.Rules, registry.Presentations, clock)).RunAsync(stop.Token);
        Client[] players = [new(), new(), new()];
        Client[] contestPeers = [new(), new(), new(), new()];
        await using Client watcher = new();
        int rid = 0;

        async Task<int> Request(Client client, string tag, params (string Key, object Value)[] attrs)
        {
            WireMessage room = client.Seen.Last(m => m.Tag == "CHAT");
            int request = ++rid;
            await client.Send(Wire.Encode(tag, attrs.Concat(
                [("lobby", (object)room.Get("lobby")), ("epoch", room.Get("epoch")), ("rid", request)]).ToArray()));
            return request;
        }
        async Task<WireMessage> Receipt(Client client, int request)
        {
            WireMessage response;
            do { response = await client.Until("RESERVATIONS"); } while (response.Get("rid") != request.ToString());
            return response;
        }
        async Task Queue(Client client, int type, string expected, string code = "", bool contest = false)
        {
            int request = await Request(client, "JOIN", contest ? ("type", type) : ("t", "0," + type));
            WireMessage receipt = await Receipt(client, request);
            Check(receipt.Get("types") == expected && receipt.Get("code") == code, "Unexpected queue receipt " + receipt);
        }
        async Task<WireMessage> Enter(Client client, string lobby)
        {
            int move = await (lobby[0] == 'C' ? Request(client, "CS", ("target", lobby)) :
                Request(client, "CHAT", ("text", "%2Flobby%20" + int.Parse(lobby[1..]))));
            WireMessage confirmation = await client.Until("CHAT");
            Check(confirmation.Get("lobby") == lobby && confirmation.Get("rid") == move.ToString(), "Lobby confirmation mismatch");
            return confirmation;
        }

        try
        {
            for (int i = 0; i < players.Length; ++i)
            {
                await players[i].Connect(port);
                await players[i].Login(name: i == 0 ? member.Id : "origin_rank" + i, register: i != 0, origin: true);
            }
            Client first = players[0], second = players[1], third = players[2];
            await watcher.Connect(port); await watcher.Login(origin: true);
            await Queue(watcher, 1, "", "ACCOUNT_REQUIRED");
            await Queue(second, 0x81, "", "INELIGIBLE");
            await Queue(first, 101, "", "UNSUPPORTED_TYPE");

            await Queue(first, 0x11, "17");
            await Queue(first, 0x81, "17,129");
            WireMessage counts;
            do { counts = await watcher.Until("LN"); }
            while (counts.Get("j").Split(',')[64] != "1" || counts.Get("j").Split(',')[16] != "1");
            Check(counts.Get("n").Split(',')[2] == "1", "A player with two tickets was counted twice");
            Check((await Receipt(first, await Request(first, "JOIN"))).Get("types") == "", "Cancel did not clear both tiers");

            var oldRoom = watcher.Seen.Last(m => m.Tag == "CHAT");
            await Enter(watcher, "L1234");
            await Enter(watcher, "L0000");
            int stale = ++rid;
            await watcher.Send(Wire.Encode("JOIN", ("t", "0,17"), ("rid", stale), ("lobby", "L0000"), ("epoch", oldRoom.Get("epoch"))));
            Check((await Receipt(watcher, stale)).Get("code") == "STALE_LOBBY", "Old epoch changed a returned lobby");
            Check((await Receipt(watcher, await Request(watcher, "CS", ("target", "C00000001")))).Get("code") == "LOBBY_NOT_FOUND",
                "Missing contest changed confirmed lobby");

            await Enter(second, "C00112233");
            Check((await second.Until("CS")).Get("registered") == "0", "Paid contest already registered");
            await Queue(second, 0x11, "", "REGISTER_FIRST", contest: true);
            for (int i = 0; i < 2; ++i)
            {
                Check((await Receipt(second, await Request(second, "CSREG"))).Get("code") == "", "Registration rejected");
                Check(second.Seen.Last(m => m.Tag == "CS").Get("registered") == "1", "Registration was not confirmed");
            }
            await Enter(second, "C00334455");
            Check((await second.Until("CS")).Get("blocked") == "1", "Premium-only contest did not show restriction");
            await Queue(second, 1, "", "INELIGIBLE", contest: true);
            await Enter(second, "C00112233");
            Check((await second.Until("CS")).Get("registered") == "1", "Registration lost while changing lobbies");
            await Enter(second, "L0000");

            await Enter(watcher, "C00223344");
            Check((await watcher.Until("CS")).Get("players") == "3", "Wrong contest settings");
            await Queue(watcher, 0x19, "", "BEFORE_START", contest: true);
            clock.Advance(10);
            await Queue(watcher, 0x19, "25", contest: true);
            string waitingToken = watcher.Seen.First(m => m.Tag == "SESSION").Get("token");
            await watcher.DisposeAsync();
            WireMessage health;
            do { await first.Send("<HEALTH/>"); health = await first.Until("HEALTH"); } while (health.Get("connections") != "3");
            Client resumedWaiting = contestPeers[0];
            await resumedWaiting.Connect(port); await resumedWaiting.Login(waitingToken, origin: true, resume: true);
            Check(resumedWaiting.Seen.Last(m => m.Tag == "CHAT").Get("lobby") == "C00223344" &&
                (await resumedWaiting.Until("RESERVATIONS")).Get("types") == "25", "Rejoin lost valid unseated tickets");
            for (int i = 1; i < contestPeers.Length; ++i)
            {
                await contestPeers[i].Connect(port); await contestPeers[i].Login(origin: true);
                await Enter(contestPeers[i], "C00223344");
                await Queue(contestPeers[i], 0x19, "25", contest: true);
            }
            Client[] contestSeats = contestPeers[..3];
            WireMessage[] contestGames = await Task.WhenAll(contestSeats.Select(c => c.Until("GO")));
            Check(contestGames.Select(g => g.Get("table")).Distinct().Count() == 1 && contestGames.All(g =>
                g.Get("lobby") == "C00223344" && g.Get("type") == "25" && g.Get("ranked") == "0"), "Contest handoff mismatch");
            clock.Advance(30);
            Client unseated = contestPeers[3];
            WireMessage closed;
            do { closed = await unseated.Until("RESERVATIONS"); } while (closed.Get("rid") != "" || closed.Get("code") != "CLOSED");
            Check(closed.Get("types") == "", "Deadline retained a contest ticket");
            await Queue(unseated, 0x19, "", "CLOSED", contest: true);
            await Task.WhenAll(contestSeats.Select(c => c.Send("<GOK/>")));
            await Task.WhenAll(contestSeats.Select(c => c.Until("INIT")));

            // Cancel was processed before the last seat. Later the last seat is
            // processed first and must win against a cancellation on the same stream.
            await Queue(second, 0x11, "17");
            await Queue(third, 0x11, "17");
            await third.Send("<HEALTH/>");
            Check((await third.Until("HEALTH")).Get("tables") == "2", "Ranked queue filled its empty seat with a bot");
            await Queue(first, 0x81, "129");
            int winningJoin = await Request(first, "JOIN", ("t", "0,17"));
            await Request(first, "JOIN");
            Check((await Receipt(first, winningJoin)).Get("types") == "17,129", "Winning reservation was not acknowledged before GO");
            WireMessage[] games = await Task.WhenAll(players.Select(p => p.Until("GO")));
            string table = games[0].Get("table");
            Check(games.All(g => g.Get("table") == table && g.Get("lobby") == "L0000" && g.Get("reservation") == "17" &&
                g.Get("ranked") == "1" && g.Get("exit") == "0"), "Ranked match or cancel/GO ordering differed across seats");
            await Task.WhenAll(players.Select(p => p.Send("<GOK/>")));
            await Task.WhenAll(players.Select(p => p.Until("INIT")));
            await first.DisposeAsync();
            Check((await second.Until("PAUSED")).Get("show") == "1", "Disconnect did not pause the committed match");
            players[0] = new Client();
            await players[0].Connect(port); await players[0].Login(name: member.Id, origin: true);
            await players[0].Until("RESTORED");
            Check(players[0].Seen.Last(m => m.Tag == "GO").Get("table") == table &&
                players[0].Seen.Last(m => m.Tag == "RESERVATIONS").Get("types") == "", "Recovery lost the committed table or replayed other tickets");
            Check((await second.Until("PAUSED")).Get("show") == "0", "Rejoined table stayed paused");

            Console.WriteLine("PASS origin lobby TCP: cross-tier queues, stale rooms, eligibility, registration, deadline, cancel/GO and reconnect");
            if (nativeClient is not null)
            {
                foreach (int type in new[] { 1, 9, 17, 25 })
                    await Task.WhenAll(Enumerable.Range(0, (type & 16) == 0 ? 4 : 3)
                        .Select(seat => RunOriginClient(nativeClient, port, "--service", $"native_{type}_{seat}", type.ToString())));
            }
            await FinishContest(port, directory, ending.Table, ending.Action, nativeClient is null);
            if (nativeClient is not null) await RunOriginClient(nativeClient, port, "--service-return", "origin_completed0");
        }
        finally
        {
            foreach (Client player in players) await player.DisposeAsync();
            foreach (Client peer in contestPeers) await peer.DisposeAsync();
            await stop.CancelAsync();
            await host;
        }
        Platform restarted = new(settings, store, registry.Rules, registry.Presentations, clock);
        restarted.Save();
        Console.WriteLine("PASS completed contest return and restart with other seats still holding results");
    }

    private static (string Table, MatchCommand Action) SeedContestEnding(ServerState state)
    {
        var options = RiichiOptions.From(new Dictionary<string, string> { ["match_type"] = "standard3" });
        var table = new TableRecord
        {
            Id = Guid.NewGuid().ToString("N"), Rule = "riichi", Options = options.ToSettings(), OriginLobby = "C00112233",
            ReservationType = 0x11, Stage = "play"
        };
        for (int seat = 0; seat < 3; ++seat)
        {
            string id = "origin_completed" + seat, token = Guid.NewGuid().ToString("N") + Guid.NewGuid().ToString("N");
            state.Accounts.Add(Account.Create(id, id, "test-password"));
            state.Sessions.Add(new() { Token = token, Account = id, Table = table.Id, OriginLobby = table.OriginLobby });
            table.Humans.Add(token);
        }
        var match = new RiichiMatch(options, new Random(1));
        while (!match.IsComplete && table.Commands.Count < 3000)
        {
            if (match.HandEnded || table.Commands.Count == 0)
            {
                match.Start();
                table.Commands.Add(new("start", Wall: match.Walls[^1]));
            }
            int seat = Enumerable.Range(0, match.PlayerCount).First(s => match.GetActions(s).Count > 0);
            MatchAction action = match.ChooseBotAction(seat);
            match.Submit(seat, action);
            table.Commands.Add(new("action", seat, action));
        }
        Check(match.IsComplete, "Could not generate a completed contest fixture");
        MatchCommand final = table.Commands[^1];
        table.Commands.RemoveAt(table.Commands.Count - 1);
        state.Tables.Add(table);
        return (table.Id, final);
    }

    private static async Task FinishContest(int port, string directory, string table, MatchCommand final, bool returnHere)
    {
        Client[] seats = [new(), new(), new()];
        try
        {
            for (int i = 0; i < seats.Length; ++i)
            {
                await seats[i].Connect(port);
                await seats[i].Login(name: "origin_completed" + i, origin: true);
                await seats[i].Until("RESTORED");
            }
            foreach (Client seat in seats)
            {
                WireMessage state;
                do { state = await seat.Until("PAUSED"); } while (state.Get("show") != "0");
            }
            Client actor = seats[final.Seat];
            string window = actor.Seen.Last(m => m.Attributes.ContainsKey("window")).Get("window");
            MatchAction action = final.Action!;
            await actor.Send(action.Kind switch
            {
                "discard" => Wire.Encode("D", ("p", action.Tiles[0]), ("window", window)),
                "skip" => Wire.Encode("N", ("window", window)),
                "ron" => Wire.Encode("N", ("type", 6), ("window", window)),
                "tsumo" => Wire.Encode("N", ("type", 7), ("window", window)),
                _ => throw new Exception("Unexpected final action in contest fixture: " + action.Kind)
            });
            await Task.WhenAll(seats.Select(c => c.Until("PROF")));
            Check(seats.All(c => c.Seen.Last(m => m.Tag == "PROFILE").Get("played") == "1"), "Contest did not settle each seat");
            await seats[0].Send(Wire.Encode("GETREPLAY", ("table", table)));
            Check((await seats[0].Until("REPLAYEND")).Get("table") == table, "Contest replay transfer did not complete");
            string replay = File.ReadAllText(Path.Combine(directory, "games", table + ".xml"));
            Check(replay.Contains("owari=", StringComparison.Ordinal), "Contest archive omitted the final result");
            if (returnHere)
            {
                await seats[0].Send("<BYE/>");
                Check((await seats[0].Until("CHAT")).Get("lobby") == "C00112233", "Completed match failed to return to its contest");
            }
            Console.WriteLine("PASS resumed contest final action, settlement, and replay transfer");
        }
        finally { foreach (Client seat in seats) await seat.DisposeAsync(); }
    }

    private static async Task RunOriginClient(string executable, int port, params string[] args)
    {
        ProcessStartInfo start = new(executable) { RedirectStandardOutput = true, RedirectStandardError = true,
            UseShellExecute = false, CreateNoWindow = true };
        start.ArgumentList.Add(port.ToString());
        foreach (string arg in args) start.ArgumentList.Add(arg);
        using Process probe = Process.Start(start) ?? throw new Exception("Cannot start native lobby client");
        Task<string> output = probe.StandardOutput.ReadToEndAsync(), errors = probe.StandardError.ReadToEndAsync();
        using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(25));
        try { await probe.WaitForExitAsync(timeout.Token); }
        catch { if (!probe.HasExited) probe.Kill(true); throw; }
        Check(probe.ExitCode == 0, string.Join(' ', args) + ": " + await errors);
        Console.Write(await output);
    }
}
