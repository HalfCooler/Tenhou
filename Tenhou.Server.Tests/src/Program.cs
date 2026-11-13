using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Tenhou.Server.Application;
using Tenhou.Server.Infrastructure;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules;
using Tenhou.Server.Rules.Riichi;
using Tenhou.Server.Rules.Sichuan;
using Tenhou.Server.Rules.Guobiao;

namespace Tenhou.Server.Tests;

internal static class Program
{
    private static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
    private static void Reject(System.Action action, string message)
    {
        try { action(); } catch (ArgumentException) { return; } throw new Exception(message);
    }
    private static void Rules(string? oracle)
    {
        RiichiRuleSet rule = new();
        Check(rule.Descriptor.Settings.Any(s => s.Key == "ancient"), "Ancient rules must belong to riichi");
        Reject(() => rule.Normalize(new Dictionary<string, string> { ["rule"] = "unknown" }), "Unknown rules accepted");
        Reject(() => rule.Normalize(new Dictionary<string, string> { ["red_count"] = "2" }), "Invalid red count accepted");
        HandValue dealer = HandEvaluator.Payments(5, 30, true, false);
        Check(dealer.Main == 12000 && dealer.Level == 1, "Mangan payment");
        HandValue child = HandEvaluator.Payments(3, 40, false, true);
        Check(child.Main == 2600 && child.Additional == 1300, "Tsumo rounding");
        int[] pairs = [0, 1, 8, 9, 36, 37, 44, 45, 72, 73, 80, 81, 124, 125];
        Check(HandEvaluator.Complete(HandEvaluator.Counts(pairs)), "Seven pairs rejected");
        Check(new HandEvaluator().Shanten(HandEvaluator.Counts(pairs)) == -1, "Complete hand shanten");
        HandValue seven = HandEvaluator.Evaluate(pairs, [], 125, new RiichiOptions(), new WinContext { Tsumo = true });
        Check(seven.Fu == 25 && seven.Yaku.Contains(22), "Seven pairs scoring");
        Check(!Wire.Project(new MatchEvent("U123"), 0, false, 2).Contains("123", StringComparison.Ordinal), "Opponent draw leaked");
        Check(Wire.Project(new MatchEvent("U123"), 1, false, 2).Contains("T123", StringComparison.Ordinal), "Own draw hidden");
        for (int players = 3; players <= 4; ++players)
            for (int seed = 0; seed < 6; ++seed)
            {
                Dictionary<string, string> values = new() { ["match_type"] = players == 3 ? "standard3" : "standard4", ["length"] = "single", ["ancient"] = seed % 2 == 0 ? "1" : "0" };
                IMatch match = rule.Create(values, new Random(seed)); IMatch replay = rule.Create(values, new Random(999));
                List<string> script = [Wire.Encode("CONFIG", RiichiOptions.From(values).ToSettings().Where(e => e.Key != "rule").ToDictionary())];
                match.Start(); replay.Start(match.Walls[0]); script.Add(Wire.Encode("START", ("wall", string.Join(',', match.Walls[0]))));
                int commands = 0;
                while (!match.IsComplete)
                {
                    bool acted = false;
                    for (int s = 0; s < players; ++s)
                    {
                        if (match.GetActions(s).Count == 0) continue;
                        MatchAction a = match.ChooseBotAction(s); match.Submit(s, a); replay.Submit(s, a);
                        script.Add(Wire.Encode("ACT", ("seat", s), ("kind", a.Kind), ("tiles", string.Join(',', a.Tiles))));
                        match.Validate(); replay.Validate(); ++commands; acted = true;
                        Check(match.Scores.SequenceEqual(replay.Scores) && match.Phase == replay.Phase, "Replay state diverged");
                        Check(JsonSerializer.Serialize(match.Events) == JsonSerializer.Serialize(replay.Events), "Replay events diverged");
                        break;
                    }
                    Check(acted && commands < 2000, "Game stuck");
                }
                Console.WriteLine($"PASS riichi {players} players seed {seed}: {commands} actions");
                if (oracle is not null) CompareOracle(oracle, script, match);
            }
    }
    private static void CompareOracle(string oracle, List<string> script, IMatch match)
    {
        using Process process = Process.Start(new ProcessStartInfo(oracle) { RedirectStandardInput = true, RedirectStandardOutput = true,
            RedirectStandardError = true, UseShellExecute = false, CreateNoWindow = true }) ?? throw new Exception("Cannot start oracle");
        foreach (string line in script) process.StandardInput.WriteLine(line);
        process.StandardInput.Close(); string output = process.StandardOutput.ReadToEnd(); string errors = process.StandardError.ReadToEnd();
        process.WaitForExit(); Check(process.ExitCode == 0, "Native rule oracle failed: " + errors);
        WireMessage[] actual = output.Split('\n', StringSplitOptions.RemoveEmptyEntries).Select(s => Wire.Parse(s.TrimEnd('\r'))).ToArray();
        Check(actual.Length == match.Events.Count + 1, "Native event count differs");
        static Dictionary<string, string> Normalize(string tag, Dictionary<string, string> input)
        {
            Dictionary<string, string> data = new(input);
            if (tag == "INIT") { string[] seed = data["seed"].Split(','); seed[3] = seed[4] = "0"; data["seed"] = string.Join(',', seed); }
            if (data.TryGetValue("owari", out string? scores)) data["owari"] = string.Join(',', scores.Split(',').Select(s => double.Parse(s, System.Globalization.CultureInfo.InvariantCulture).ToString("R", System.Globalization.CultureInfo.InvariantCulture)));
            return data;
        }
        for (int i = 0; i < match.Events.Count; ++i)
        {
            MatchEvent expected = match.Events[i]; Dictionary<string, string> attrs = new(expected.Data);
            foreach ((int seat, int hint) in expected.Hints) if (hint != 0) attrs["_h" + seat] = hint.ToString();
            var left = Normalize(expected.Kind, attrs).OrderBy(e => e.Key).ToArray();
            var right = Normalize(actual[i].Tag, actual[i].Attributes).OrderBy(e => e.Key).ToArray();
            Check(expected.Kind == actual[i].Tag && left.SequenceEqual(right), "Native event differs at " + i + " " + expected.Kind
                + "\nC#: " + JsonSerializer.Serialize(left) + "\nC++: " + JsonSerializer.Serialize(right));
        }
        Check(actual[^1].Get("scores") == string.Join(',', match.Scores) && actual[^1].Get("phase") == match.Phase, "Native final state differs");
        Console.WriteLine("PASS native rule differential: " + match.Events.Count + " events");
    }

    private sealed class Client : IAsyncDisposable
    {
        private readonly TcpClient tcp = new();
        private readonly Queue<WireMessage> pending = [];
        private readonly List<byte> partial = [];
        public List<WireMessage> Seen { get; } = [];
        public async Task Connect(int port) => await tcp.ConnectAsync(IPAddress.Loopback, port);
        public async Task Send(string text)
        { await tcp.GetStream().WriteAsync(Encoding.UTF8.GetBytes(text + '\0')); }
        public async Task<WireMessage> Until(string tag)
        {
            using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(12));
            while (true)
            {
                if (pending.TryDequeue(out WireMessage? item))
                {
                    Seen.Add(item); if (item.Tag == tag) return item; continue;
                }
                byte[] buffer = new byte[8192]; int count = await tcp.GetStream().ReadAsync(buffer, timeout.Token);
                Check(count > 0, "Connection closed while awaiting " + tag);
                for (int i = 0; i < count; ++i)
                    if (buffer[i] == 0) { pending.Enqueue(Wire.Parse(Encoding.UTF8.GetString(partial.ToArray()))); partial.Clear(); }
                    else partial.Add(buffer[i]);
            }
        }
        public async Task Login(string? token = null, string name = "NoName", bool register = false)
        {
            // Deliberately split a UTF-8 frame across two writes.
            await Send(Wire.Encode("HELO", ("name", name), ("sx", "M"))); WireMessage hello = await Until("HELO");
            Check(hello.Get("uname") == "TENHOU-PLATFORM" && hello.Get("version") == "1", "Platform handshake");
            await Send(Wire.Encode("CAPS", ("version", "1"), ("token", token ?? ""), ("sx", "M"),
                ("password", "test-password"), ("nickname", "测试玩家"), ("register", register ? "1" : "0")));
            await Until("SESSION"); await Until("CATALOGEND"); await Until("CS");
        }
        public ValueTask DisposeAsync() { tcp.Dispose(); return ValueTask.CompletedTask; }
    }
    private static (RuleRegistry Rules, PresentationRegistry Presentations) Registries()
    {
        RuleRegistry rules = new(); rules.Register(new RiichiRuleSet()); rules.Register(new SichuanRuleSet()); rules.Register(new GuobiaoRuleSet());
        PresentationRegistry presentations = new(); presentations.Register("riichi-v1", v => new RiichiPresentation(v));
        presentations.Register("mahjong-v1", v => new VariantPresentation(v)); return (rules, presentations);
    }
    private static async Task Integration(string? nativeClient)
    {
        string directory = Path.Combine(Path.GetTempPath(), "tenhou-dotnet-test-" + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(directory);
        TcpListener reserve = new(IPAddress.Loopback, 0); reserve.Start(); int port = ((IPEndPoint)reserve.LocalEndpoint).Port; reserve.Stop();
        ServerSettings settings = new() { Port = port, DataDirectory = directory, BotDelayMs = 0, MatchWaitMs = 0 };
        settings.Queues[0].Options["length"] = "single";
        settings.Queues[4].Options["hands"] = "4";
        settings.Queues[5].Options["hands"] = "4";
        var registry = Registries(); string token;
        using (JsonStore store = new(directory))
        using (CancellationTokenSource stop = new())
        {
            Platform platform = new(settings, store, registry.Rules, registry.Presentations);
            Task host = new TcpHost(settings, platform).RunAsync(stop.Token);
            if (nativeClient is not null)
            foreach (int queue in new[] { 101, 201, 301 })
            {
                ProcessStartInfo start = new(nativeClient) { RedirectStandardOutput = true, RedirectStandardError = true,
                    UseShellExecute = false, CreateNoWindow = true };
                start.ArgumentList.Add(port.ToString());
                start.ArgumentList.Add(queue.ToString());
                using Process probe = Process.Start(start) ?? throw new Exception("Cannot start native client check");
                Task<string> output = probe.StandardOutput.ReadToEndAsync(), errors = probe.StandardError.ReadToEndAsync();
                using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(50));
                try { await probe.WaitForExitAsync(timeout.Token); }
                catch { probe.Kill(true); throw; }
                Check(probe.ExitCode == 0, "Native client failed: " + await errors); Console.Write(await output);
            }
            await using (Client client = new())
            {
                await client.Connect(port); await client.Login(); token = client.Seen.First(m => m.Tag == "SESSION").Get("token");
                Check(client.Seen.Count(m => m.Tag == "QUEUEDEF") == 6, "Server queue catalog missing");
                Check(client.Seen.Any(m => m.Tag == "RULEFIELD" && m.Get("key") == "ancient"), "Rule schema missing");
                string create = "<ROOMCREATE rid=\"1\" rule=\"riichi\" length=\"single\"/>";
                await client.Send(create); WireMessage accepted = await client.Until("ACCEPT"); string room = accepted.Get("room"); await client.Until("ROOM");
                await client.Send(create); WireMessage repeated = await client.Until("ACCEPT");
                Check(repeated.Get("room") == room, "Idempotent room create allocated twice"); await client.Until("ROOM");
                await client.Send("<SETROOM rid=\"2\" revision=\"1\" rule=\"riichi\" length=\"single\"/>"); await client.Until("ACCEPT");
                Check((await client.Until("ROOM")).Get("revision") == "2", "Room setting revision did not advance");
                await client.Send("<SETROOM rid=\"3\" revision=\"1\" rule=\"riichi\" length=\"single\"/>");
                Check((await client.Until("REJECT")).Get("code") == "REVISION_CONFLICT", "Stale room edit accepted"); await client.Until("ROOM");
                await client.Send($"<JOIN t=\"{room},0\" revision=\"2\"/>"); await client.Until("GO");
                await client.Send("<GOK/>"); await client.Until("INIT"); await client.Until("CLOCK");
                Check(File.ReadAllText(Path.Combine(directory, "state.json")).Contains("\"wall\"", StringComparison.Ordinal), "Wall not persisted as JSON");
            }
            // Let the disconnect event pause the table before the orderly shutdown.
            await using (Client health = new())
            {
                await health.Connect(port); await health.Send("<HEALTH/>"); await health.Until("HEALTH");
            }
            await stop.CancelAsync(); await host;
        }
        using (JsonStore store = new(directory))
        using (CancellationTokenSource stop = new())
        {
            Platform platform = new(settings, store, registry.Rules, registry.Presentations);
            Task host = new TcpHost(settings, platform).RunAsync(stop.Token);
            await using (Client client = new())
            {
                await client.Connect(port); await client.Login(token); await client.Until("RESTORED");
                WireMessage go = client.Seen.Last(m => m.Tag == "GO");
                await client.Send(Wire.Encode("EXIT", ("table", go.Get("table")))); await client.Until("TABLEEND");
                await client.Until("CS"); Check(client.Seen.Any(m => m.Tag == "INIT"), "Recovered hand missing");
            }
            await stop.CancelAsync(); await host;
        }
        Console.WriteLine("PASS TCP catalog, idempotent rooms, match start, JSON persistence, restart recovery, exit");
        // Keep the temporary fixture available if a later assertion fails.
    }
    private static async Task Multiplayer()
    {
        string directory = Path.Combine(Path.GetTempPath(), "tenhou-multiplayer-" + Guid.NewGuid().ToString("N"));
        TcpListener reserve = new(IPAddress.Loopback, 0); reserve.Start(); int port = ((IPEndPoint)reserve.LocalEndpoint).Port; reserve.Stop();
        ServerSettings settings = new() { Port = port, DataDirectory = directory, MatchWaitMs = 10000 };
        var registry = Registries();
        using JsonStore store = new(directory); using CancellationTokenSource stop = new();
        Platform platform = new(settings, store, registry.Rules, registry.Presentations);
        Task host = new TcpHost(settings, platform).RunAsync(stop.Token);
        Client[] clients = Enumerable.Range(0, 4).Select(_ => new Client()).ToArray();
        try
        {
            for (int i = 0; i < clients.Length; ++i)
            { await clients[i].Connect(port); await clients[i].Login(name: i == 0 ? "test_owner" : "NoName", register: i == 0); }
            await clients[0].Send("<ROOMCREATE rid=\"1\" rule=\"sichuan-bloodflow\" hands=\"1\"/>");
            string room = (await clients[0].Until("ACCEPT")).Get("room"); await clients[0].Until("ROOM");
            for (int i = 1; i < clients.Length; ++i)
            { await clients[i].Send($"<ROOMJOIN rid=\"1\" room=\"{room}\"/>"); await clients[i].Until("ROOM"); }
            await Task.WhenAll(clients.Select(c => c.Send($"<JOIN t=\"{room},0\" revision=\"1\"/>")));
            WireMessage[] games = await Task.WhenAll(clients.Select(c => c.Until("VGO")));
            Check(games.Select(g => g.Get("table")).Distinct().Count() == 1, "Four humans did not share a table");
            await Task.WhenAll(clients.Select(c => c.Send("<GOK/>")));
            WireMessage[] state = await Task.WhenAll(clients.Select(c => c.Until("VSTATE")));
            Check(state.All(s => s.Get("phase") == "exchange"), "Missing exchange phase");
            async Task Act(Client client, WireMessage view, string? overrideAction = null)
            {
                string[] fields = (overrideAction ?? view.Get("actions").Split(';')[0]).Split(':');
                await client.Send(Wire.Encode("VACT", ("table", games[0].Get("table")), ("window", view.Get("window")), ("kind", fields[0]), ("tiles", fields[1])));
            }
            await Task.WhenAll(Enumerable.Range(0, 4).Select(i => Act(clients[i], state[i])));
            async Task<WireMessage> Phase(Client client, string phase)
            {
                while (true) { WireMessage next = await client.Until("VSTATE"); if (next.Get("phase") == phase) return next; }
            }
            state = await Task.WhenAll(clients.Select(c => Phase(c, "missing")));
            Check(clients.All(c => c.Seen.All(m => m.Tag != "RESTORE")), "Another player's exchange invalidated a legal simultaneous action");
            await Task.WhenAll(Enumerable.Range(0, 4).Select(i => Act(clients[i], state[i], "dingque:" + (i % 3))));
            await Task.WhenAll(clients.Select(c => Phase(c, "turn")));
            for (int round = 1; round <= 2; ++round)
            {
                await clients[0].Send(Wire.Encode("EXIT", ("table", games[0].Get("table"))));
                WireMessage[] votes = await Task.WhenAll(clients.Select(c => c.Until("EXITVOTE")));
                Check(votes.All(v => v.Get("show") == "1" && v.Get("id") == round.ToString()), "Vote generation mismatch");
                int yes = round == 1 ? 2 : 3;
                await Task.WhenAll(Enumerable.Range(0, 4).Select(i => clients[i].Send(Wire.Encode("EXITVOTE", ("table", games[0].Get("table")),
                    ("id", round), ("yes", i < yes ? 1 : 0)))));
                if (round == 1)
                {
                    foreach (Client client in clients)
                    { WireMessage result; do { result = await client.Until("EXITVOTE"); } while (result.Get("show") != "0"); Check(result.Get("result") == "continue", "Half the votes ended the table"); }
                }
                else await Task.WhenAll(clients.Select(c => c.Until("TABLEEND")));
            }
            Console.WriteLine("PASS four-client room, account registration, simultaneous exchange/dingque, strict-majority exit voting");
        }
        finally
        {
            foreach (Client client in clients) await client.DisposeAsync();
            await stop.CancelAsync(); await host;
        }
    }
    private static async Task Lifecycle()
    {
        string directory = Path.Combine(Path.GetTempPath(), "Tenhou 启停 test " + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(directory);
        string config = Path.Combine(directory, "服务器.json");
        File.WriteAllText(config, JsonSerializer.Serialize(new ServerSettings { Port = 0, DataDirectory = "数据" }, ServerSettings.Json));
        string binary = typeof(Tenhou.Server.Program).Assembly.Location;
        ProcessStartInfo start = new("dotnet") { RedirectStandardOutput = true, RedirectStandardError = true, UseShellExecute = false, CreateNoWindow = true };
        start.ArgumentList.Add(binary); start.ArgumentList.Add("--config"); start.ArgumentList.Add(config);
        using Process server = Process.Start(start) ?? throw new Exception("Cannot start service");
        using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(12));
        try
        {
            string? ready = await server.StandardOutput.ReadLineAsync(timeout.Token);
            Check(ready?.StartsWith("Tenhou platform ready", StringComparison.Ordinal) == true, "Server did not become ready: " + ready);
            ServerSettings settings = ServerSettings.Load(config); await ControlEndpoint.StopAsync(settings);
            await server.WaitForExitAsync(timeout.Token);
            Check(server.ExitCode == 0, "Graceful shutdown failed: " + await server.StandardError.ReadToEndAsync());
            Check(File.Exists(Path.Combine(directory, "数据", "state.json")), "Shutdown did not persist state");
            File.WriteAllText(Path.Combine(directory, "数据", "state.json"), "{broken");
            using Process corrupt = Process.Start(start) ?? throw new Exception("Cannot restart service");
            await corrupt.WaitForExitAsync(timeout.Token);
            Check(corrupt.ExitCode != 0 && File.ReadAllText(Path.Combine(directory, "数据", "state.json")) == "{broken", "Corrupt data silently replaced");
            Console.WriteLine("PASS real process lifecycle, local stop pipe, Unicode paths, corrupt-state refusal");
        }
        finally { if (!server.HasExited) { server.Kill(true); await server.WaitForExitAsync(); } }
    }
    public static async Task<int> Main(string[] args)
    {
        string? oracle = null, nativeClient = null;
        for (int i = 0; i < args.Length; i += 2)
        {
            if (args[i] == "--oracle") oracle = args[i + 1];
            else if (args[i] == "--native-client") nativeClient = args[i + 1];
            else throw new ArgumentException("Unknown test option");
        }
        try { VariantChecks.Scoring(); VariantChecks.Games(); Rules(oracle); await Integration(nativeClient); await Multiplayer(); await Lifecycle(); Console.WriteLine("All checks passed."); return 0; }
        catch (Exception e) { Console.Error.WriteLine(e); return 1; }
    }
}
