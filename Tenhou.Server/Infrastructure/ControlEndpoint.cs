using System;
using System.IO;
using System.IO.Pipes;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Tenhou.Server.Infrastructure;

public static class ControlEndpoint
{
    private static string Name(ServerSettings settings)
    {
        string directory = Path.GetFullPath(settings.DataDirectory);
        if (OperatingSystem.IsWindows()) directory = directory.ToUpperInvariant();
        return "tenhou-" + Convert.ToHexStringLower(SHA256.HashData(Encoding.UTF8.GetBytes(directory)))[..24];
    }
    public static async Task StopAsync(ServerSettings settings)
    {
        using NamedPipeClientStream pipe = new(".", Name(settings), PipeDirection.Out, PipeOptions.Asynchronous);
        using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(3));
        await pipe.ConnectAsync(timeout.Token);
        using StreamWriter writer = new(pipe, new UTF8Encoding(false), leaveOpen: true);
        await writer.WriteLineAsync("stop".AsMemory(), timeout.Token); await writer.FlushAsync(timeout.Token);
    }
    public static async Task ListenAsync(ServerSettings settings, CancellationTokenSource shutdown)
    {
        try
        {
            while (!shutdown.IsCancellationRequested)
            {
                using NamedPipeServerStream pipe = new(Name(settings), PipeDirection.In, 1, PipeTransmissionMode.Byte,
                    PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
                await pipe.WaitForConnectionAsync(shutdown.Token);
                using CancellationTokenSource timeout = CancellationTokenSource.CreateLinkedTokenSource(shutdown.Token);
                timeout.CancelAfter(TimeSpan.FromSeconds(3));
                using StreamReader reader = new(pipe, new UTF8Encoding(false, true), leaveOpen: true);
                try { if (await reader.ReadLineAsync(timeout.Token) == "stop") await shutdown.CancelAsync(); }
                catch (Exception error) when (error is IOException or OperationCanceledException or DecoderFallbackException) { }
            }
        }
        catch (OperationCanceledException) when (shutdown.IsCancellationRequested) { }
    }
}
