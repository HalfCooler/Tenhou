using System;
using System.IO;
using System.Text.Json;

namespace Tenhou.Server.Infrastructure;

// Plain JSON; no DPAPI, machine key or encrypted archive. Replacement still
// protects an acknowledged snapshot against a partial file write.
public sealed class JsonStore : IDisposable
{
    private readonly string directory;
    private readonly FileStream owner;
    private readonly StreamWriter events;
    public JsonStore(string directory)
    {
        this.directory = directory; Directory.CreateDirectory(directory);
        owner = new FileStream(Path.Combine(directory, "server.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        try { events = new StreamWriter(Path.Combine(directory, "events.jsonl"), append: true) { AutoFlush = true }; }
        catch { owner.Dispose(); throw; }
    }
    public T? Load<T>(string name)
    {
        string path = Path.Combine(directory, name);
        if (!File.Exists(path)) return default;
        if (new FileInfo(path).Length > 64 * 1024 * 1024) throw new InvalidDataException("State exceeds capacity");
        return JsonSerializer.Deserialize<T>(File.ReadAllText(path), ServerSettings.Json)
            ?? throw new InvalidDataException("Empty state");
    }
    public void Save<T>(string name, T value)
    {
        string path = Path.Combine(directory, name), temporary = path + ".tmp";
        using (FileStream file = new(temporary, FileMode.Create, FileAccess.Write, FileShare.None))
        {
            JsonSerializer.Serialize(file, value, ServerSettings.Json);
            if (file.Length > 64 * 1024 * 1024) throw new IOException("State exceeds capacity");
            file.Flush(true);
        }
        File.Move(temporary, path, true);
    }
    public void SaveText(string name, string value)
    {
        string path = Path.Combine(directory, name); Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        string temporary = path + ".tmp"; File.WriteAllText(temporary, value); File.Move(temporary, path, true);
    }
    public void Log(string category, int count = 0) => events.WriteLine(JsonSerializer.Serialize(new
        { time = DateTimeOffset.UtcNow, category, count }));
    public void Dispose() { events.Dispose(); owner.Dispose(); }
}
