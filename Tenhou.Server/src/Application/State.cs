using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using Tenhou.Server.Rules;

namespace Tenhou.Server.Application;

public sealed class Account
{
    public string Id { get; set; } = "";
    public string Nickname { get; set; } = "";
    public string Salt { get; set; } = "";
    public string PasswordHash { get; set; } = "";
    public int Played { get; set; }
    public Rank[] Ranks { get; set; } = [new(), new()];
    public static string NormalizeId(string id)
    {
        id = id.ToLowerInvariant();
        if (id.Length is < 3 or > 32 || id == "noname" || id.Any(c => !char.IsAsciiLetterOrDigit(c) && c is not ('_' or '-')))
            throw new ArgumentException("账号须为3～32位英文字母、数字、下划线或短横线");
        return id;
    }
    public static Account Create(string id, string nickname, string password)
    {
        if (Encoding.UTF8.GetByteCount(password) is < 8 or > 128) throw new ArgumentException("密码须为8～128字节");
        if (string.IsNullOrWhiteSpace(nickname) || Encoding.UTF8.GetByteCount(nickname) > 48 || nickname.Any(char.IsControl))
            throw new ArgumentException("昵称不能为空，最多48个UTF-8字节且不能含控制字符");
        byte[] salt = RandomNumberGenerator.GetBytes(32);
        return new() { Id = NormalizeId(id), Nickname = nickname, Salt = Convert.ToHexString(salt),
            PasswordHash = Convert.ToHexString(Rfc2898DeriveBytes.Pbkdf2(Encoding.UTF8.GetBytes(password), salt, 210000, HashAlgorithmName.SHA256, 32)) };
    }
    public bool Verify(string password)
    {
        if (Encoding.UTF8.GetByteCount(password) is < 8 or > 128) return false;
        byte[] actual = Rfc2898DeriveBytes.Pbkdf2(Encoding.UTF8.GetBytes(password), Convert.FromHexString(Salt), 210000, HashAlgorithmName.SHA256, 32);
        return CryptographicOperations.FixedTimeEquals(actual, Convert.FromHexString(PasswordHash));
    }
}

public sealed class Rank
{
    public int Level { get; set; }
    public int Points { get; set; }
    public int Games { get; set; }
    public double Rating { get; set; } = 1500;
    public int[] Places { get; set; } = new int[4];
    public string Name => Level < 10 ? (Level == 0 ? "新人" : (10 - Level) + "级") : (Level - 9) + "段";
    public int Target => new[] { 20, 20, 20, 20, 40, 40, 80, 80, 100, 100, 400, 800, 1200, 1600, 2000, 2400, 2800, 3200, 3600 }[Level];
    public void Update(int place, int players, bool south, double average)
    {
        int delta = players == 3 ? (place == 0 ? 30 : 0) : (place == 0 ? 20 : place == 1 ? 10 : 0);
        if (place == players - 1) delta = Level < 8 ? 0 : Level == 8 ? -10 : Level == 9 ? -20 : -(Level - 7) * 10;
        if (south) delta = delta * 3 / 2;
        Points += delta;
        if (Level < 18 && Points >= Target) { ++Level; Points = Level < 10 ? 0 : (Level - 9) * 200; }
        else if (Level >= 10 && Points < 0) { --Level; Points = Level < 10 ? 0 : (Level - 9) * 200; }
        else Points = Math.Clamp(Points, 0, Target);
        int finish = players == 3 ? (place == 0 ? 30 : place == 1 ? 0 : -30) : 30 - 20 * place;
        Rating = Math.Ceiling((Rating + Math.Max(.2, 1 - Games * .002) * (finish + (Math.Max(1500, average) - Rating) / 40)) * 100 - 1e-8) / 100;
        ++Games; ++Places[place];
    }
}

public sealed class Session
{
    public string Token { get; set; } = "";
    public string Account { get; set; } = "";
    public string Gender { get; set; } = "M";
    public string Table { get; set; } = "";
    public int Room { get; set; }
    public long LastRequest { get; set; }
    public string RequestText { get; set; } = "";
    public string RequestReply { get; set; } = "";
}
public sealed class Room
{
    public int Number { get; set; }
    public string Owner { get; set; } = "";
    public string Rule { get; set; } = "riichi";
    public Dictionary<string, string> Options { get; set; } = [];
    public int Revision { get; set; } = 1;
}
public sealed record MatchCommand(string Kind, int Seat = 0, MatchAction? Action = null, int[]? Wall = null);
public sealed class TableRecord
{
    public int RuleVersion { get; set; } = 1;
    public string Id { get; set; } = "";
    public string Rule { get; set; } = "";
    public Dictionary<string, string> Options { get; set; } = [];
    public List<string> Humans { get; set; } = [];
    public List<MatchCommand> Commands { get; set; } = [];
    public string Stage { get; set; } = "gok";
    public bool Ranked { get; set; }
    public bool Settled { get; set; }
    public int[] Reserve { get; set; } = new int[4];
    public int[] Remaining { get; set; } = [-1, -1, -1, -1];
    public int[] Elapsed { get; set; } = new int[4];
    public int VoteRevision { get; set; }
    public long VoteUntil { get; set; }
    public int[] Votes { get; set; } = [-1, -1, -1, -1];
}
public sealed class ServerState
{
    public int Version { get; set; } = 1;
    public List<Account> Accounts { get; set; } = [];
    public List<Session> Sessions { get; set; } = [];
    public List<Room> Rooms { get; set; } = [];
    public List<TableRecord> Tables { get; set; } = [];
}
