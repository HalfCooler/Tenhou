using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Tenhou.Server.Application;
using Tenhou.Server.Infrastructure;
using Tenhou.Server.Protocol;
using Tenhou.Server.Rules;
using Tenhou.Server.Rules.Riichi;
using Tenhou.Server.Rules.Sichuan;
using Tenhou.Server.Rules.Guobiao;

namespace Tenhou.Server;

public static class Program
{
    public static async Task<int> Main(string[] args)
    {
        try
        {
            string config = Path.Combine(AppContext.BaseDirectory, "server.json");
            bool stop = false;
            for (int i = 0; i < args.Length; ++i)
            {
                if (args[i] == "--config" && i + 1 < args.Length) config = args[++i];
                else if (args[i] == "--stop") stop = true;
                else throw new ArgumentException("Usage: Tenhou.Server [--config server.json] [--stop]");
            }
            ServerSettings settings = ServerSettings.Load(config);
            if (stop) { await ControlEndpoint.StopAsync(settings); Console.WriteLine("Shutdown requested."); return 0; }
            RuleRegistry rules = new(); rules.Register(new RiichiRuleSet()); rules.Register(new SichuanRuleSet()); rules.Register(new GuobiaoRuleSet());
            PresentationRegistry presentations = new(); presentations.Register("riichi-v1", values => new RiichiPresentation(values));
            presentations.Register("mahjong-v1", values => new VariantPresentation(values));
            using JsonStore store = new(settings.DataDirectory);
            Platform platform = new(settings, store, rules, presentations);
            using CancellationTokenSource shutdown = new();
            ConsoleCancelEventHandler cancel = (_, e) => { e.Cancel = true; shutdown.Cancel(); };
            Console.CancelKeyPress += cancel;
            using PosixSignalRegistration? termination = OperatingSystem.IsWindows() ? null : PosixSignalRegistration.Create(PosixSignal.SIGTERM,
                context => { context.Cancel = true; shutdown.Cancel(); });
            Task host = new TcpHost(settings, platform).RunAsync(shutdown.Token);
            Task control = ControlEndpoint.ListenAsync(settings, shutdown);
            try
            {
                Task completed = await Task.WhenAny(host, control); await completed;
            }
            finally
            {
                await shutdown.CancelAsync();
                Console.CancelKeyPress -= cancel;
                await Task.WhenAll(host, control);
            }
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine("Tenhou server failed: " + error.Message); return 1; }
    }
}
