using System;
using System.IO;
using System.Net;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.Encodings.Web;
using System.Text.Unicode;

namespace Tenhou.Server.Infrastructure;

public sealed record ServerSettings
{
    public string Listen { get; init; } = "127.0.0.1";
    public int Port { get; init; } = 10080;
    public string DataDirectory { get; init; } = "data";
    public int MaxConnections { get; init; } = 64;
    public int MaxTables { get; init; } = 64;
    public int BotDelayMs { get; init; } = 350;
    public int MatchWaitMs { get; init; } = 10000;
    public QueueSettings[] Queues { get; init; } =
    [
        new(101, "立直 四人 东风", "riichi", new() { ["match_type"] = "standard4", ["length"] = "east" }),
        new(102, "立直 四人 半庄", "riichi", new() { ["match_type"] = "standard4", ["length"] = "south" }),
        new(103, "立直 三人 东风", "riichi", new() { ["match_type"] = "standard3", ["length"] = "east" }),
        new(104, "立直 三人 半庄", "riichi", new() { ["match_type"] = "standard3", ["length"] = "south" }),
        new(201, "四川 血流成河", "sichuan-bloodflow", new()),
        new(301, "国标麻将", "guobiao", new())
    ];
    public static JsonSerializerOptions Json { get; } = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase, WriteIndented = true,
        Encoder = JavaScriptEncoder.Create(UnicodeRanges.All),
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow
    };
    public static ServerSettings Load(string path)
    {
        path = Path.GetFullPath(path);
        ServerSettings settings = JsonSerializer.Deserialize<ServerSettings>(File.ReadAllText(path), Json)
            ?? throw new InvalidDataException("Empty server configuration");
        if (!IPAddress.TryParse(settings.Listen, out _) || settings.Port is < 0 or > 65535
            || settings.MaxConnections is < 1 or > 4096 || settings.MaxTables is < 1 or > 1000
            || settings.BotDelayMs is < 0 or > 60000 || settings.MatchWaitMs is < 0 or > 600000
            || string.IsNullOrWhiteSpace(settings.DataDirectory)) throw new InvalidDataException("Invalid server configuration");
        return settings with { DataDirectory = Path.GetFullPath(settings.DataDirectory, Path.GetDirectoryName(path)!) };
    }
}

public sealed record QueueSettings(int Id, string Name, string Rule, System.Collections.Generic.Dictionary<string, string> Options);
