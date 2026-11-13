using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Channels;
using System.Threading.Tasks;
using Tenhou.Server.Application;
using Tenhou.Server.Infrastructure;

namespace Tenhou.Server.Protocol;

public readonly record struct ClientJoined(Peer Peer);
public readonly record struct FrameReceived(Peer Peer, string Text);
public readonly record struct ClientLeft(Peer Peer);
public readonly record struct ClockTick;
public union HostEvent(ClientJoined, FrameReceived, ClientLeft, ClockTick);

public sealed class Peer
{
    private static readonly UTF8Encoding Utf8 = new(false, true);
    private readonly TcpClient client;
    private readonly Channel<byte[]> output = Channel.CreateBounded<byte[]>(128);
    private int queuedBytes;
    public long Id { get; }
    public string Token { get; set; } = "";
    public string Login { get; set; } = "";
    public bool Hello { get; set; }
    public bool Presentation { get; set; }
    public Peer(long id, TcpClient client) { Id = id; this.client = client; client.NoDelay = true; }
    public void Send(string text)
    {
        byte[] bytes = Utf8.GetBytes(text + '\0');
        if (Interlocked.Add(ref queuedBytes, bytes.Length) > 512 * 1024 || !output.Writer.TryWrite(bytes))
        { Interlocked.Add(ref queuedBytes, -bytes.Length); Abort(); }
    }
    public void Abort() { output.Writer.TryComplete(); client.Dispose(); }
    public void Complete() => output.Writer.TryComplete();
    public async Task RunAsync(ChannelWriter<HostEvent> input, CancellationToken cancellation)
    {
        using CancellationTokenSource lifetime = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        Task writer = WriteAsync(lifetime.Token);
        try
        {
            await input.WriteAsync(new ClientJoined(this), lifetime.Token);
            using MemoryStream frame = new(); byte[] buffer = new byte[8192];
            long window = Environment.TickCount64, frameStart = 0; int count = 0;
            while (!lifetime.IsCancellationRequested)
            {
                using CancellationTokenSource timeout = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
                timeout.CancelAfter(TimeSpan.FromSeconds(Hello ? 45 : 10));
                int received = await client.GetStream().ReadAsync(buffer, timeout.Token);
                if (received == 0) break;
                for (int i = 0; i < received; ++i)
                {
                    long now = Environment.TickCount64;
                    if (frame.Length == 0) frameStart = now;
                    if (now - frameStart > 10000) throw new InvalidDataException("Frame timeout");
                    if (buffer[i] != 0)
                    {
                        if (frame.Length >= Wire.MaximumFrameBytes) throw new InvalidDataException("Frame too large");
                        frame.WriteByte(buffer[i]); continue;
                    }
                    if (now - window >= 1000) { window = now; count = 0; }
                    if (++count > 120 || frame.Length == 0) throw new InvalidDataException("Message rate or framing limit");
                    string text = Utf8.GetString(frame.GetBuffer(), 0, (int)frame.Length); frame.SetLength(0);
                    await input.WriteAsync(new FrameReceived(this, text), lifetime.Token);
                }
            }
        }
        catch (Exception e) when (e is IOException or SocketException or OperationCanceledException or ObjectDisposedException or DecoderFallbackException) { }
        finally
        {
            await lifetime.CancelAsync(); Abort();
            try { await writer; } catch (Exception e) when (e is IOException or SocketException or OperationCanceledException or ObjectDisposedException) { }
            if (!cancellation.IsCancellationRequested) await input.WriteAsync(new ClientLeft(this), cancellation);
        }
    }
    private async Task WriteAsync(CancellationToken cancellation)
    {
        try
        {
            await foreach (byte[] bytes in output.Reader.ReadAllAsync(cancellation))
            {
                using CancellationTokenSource timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
                timeout.CancelAfter(TimeSpan.FromSeconds(10));
                await client.GetStream().WriteAsync(bytes, timeout.Token); Interlocked.Add(ref queuedBytes, -bytes.Length);
            }
        }
        finally { client.Dispose(); }
    }
}

public sealed class TcpHost
{
    private readonly ServerSettings settings;
    private readonly Platform platform;
    public TcpHost(ServerSettings settings, Platform platform) { this.settings = settings; this.platform = platform; }
    public async Task RunAsync(CancellationToken cancellation)
    {
        Channel<HostEvent> input = Channel.CreateBounded<HostEvent>(new BoundedChannelOptions(1024) { SingleReader = true, FullMode = BoundedChannelFullMode.Wait });
        using CancellationTokenSource lifetime = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        TcpListener listener = new(IPAddress.Parse(settings.Listen), settings.Port);
        ConcurrentDictionary<long, Task> clients = new(); ConcurrentDictionary<long, Peer> peers = new(); long sequence = 0;
        listener.Start(); Console.WriteLine("Tenhou platform ready " + listener.LocalEndpoint);
        async Task Accept()
        {
            while (!lifetime.IsCancellationRequested)
            {
                TcpClient client = await listener.AcceptTcpClientAsync(lifetime.Token);
                if (peers.Count >= settings.MaxConnections) { client.Dispose(); continue; }
                long id = Interlocked.Increment(ref sequence); Peer peer = new(id, client); peers[id] = peer;
                Task task = peer.RunAsync(input.Writer, lifetime.Token); clients[id] = task;
                _ = task.ContinueWith(completed => { clients.TryRemove(id, out _); peers.TryRemove(id, out _); },
                    CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
            }
        }
        async Task Pulse()
        {
            using PeriodicTimer timer = new(TimeSpan.FromMilliseconds(25));
            while (await timer.WaitForNextTickAsync(lifetime.Token))
                await input.Writer.WriteAsync(new ClockTick(), lifetime.Token);
        }
        Task accept = Accept(), pulse = Pulse();
        bool orderly = false;
        try
        {
            // Accept/pulse failures must stop the reader, not leave an idle server.
            _ = accept.ContinueWith(_ => lifetime.Cancel(), CancellationToken.None, TaskContinuationOptions.OnlyOnFaulted, TaskScheduler.Default);
            _ = pulse.ContinueWith(_ => lifetime.Cancel(), CancellationToken.None, TaskContinuationOptions.OnlyOnFaulted, TaskScheduler.Default);
            await foreach (HostEvent message in input.Reader.ReadAllAsync(lifetime.Token))
                switch (message)
                {
                    case ClientJoined joined: platform.Connected(joined.Peer); break;
                    case FrameReceived received: platform.Receive(received.Peer, received.Text); break;
                    case ClientLeft left: platform.Disconnected(left.Peer); break;
                    case ClockTick: platform.Tick(); break;
                }
        }
        catch (OperationCanceledException) when (lifetime.IsCancellationRequested) { orderly = true; }
        finally
        {
            await lifetime.CancelAsync(); listener.Stop();
            foreach (Peer peer in peers.Values) peer.Abort();
            try { await Task.WhenAll(clients.Values.Append(accept).Append(pulse)); }
            catch (OperationCanceledException) when (lifetime.IsCancellationRequested) { }
            if (orderly) platform.Save();
        }
    }
}
