using System;
using System.Collections.Generic;
using System.Linq;

namespace Tenhou.Server.Rules;

// A ruleset owns its options, state machine, legal actions and settlement.
// The host must not assume riichi phases, tile sets, or that a win ends a hand.
public interface IRuleSet
{
    RuleDescriptor Descriptor { get; }
    IReadOnlyDictionary<string, string> Normalize(IReadOnlyDictionary<string, string> options);
    IMatch Create(IReadOnlyDictionary<string, string> options, Random random);
}

public interface IMatch
{
    int PlayerCount { get; }
    string Phase { get; }
    bool HandEnded { get; }
    bool IsComplete { get; }
    IReadOnlyList<int> Scores { get; }
    IReadOnlyList<MatchEvent> Events { get; }
    IReadOnlyList<int[]> Walls { get; }
    void Start(int[]? wall = null);
    IReadOnlyList<MatchAction> GetActions(int seat);
    void Submit(int seat, MatchAction action);
    MatchAction ChooseBotAction(int seat);
    void Validate();
}

public sealed record MatchAction(string Kind, int[] Tiles)
{
    public MatchAction(string kind) : this(kind, []) { }
    public bool Equivalent(MatchAction other) => Kind == other.Kind && Tiles.SequenceEqual(other.Tiles);
}

// Values are immutable after publication, except settlement can annotate its own
// just-created events before Submit returns. No wire XML is generated in rules.
public sealed record MatchEvent(string Kind, Dictionary<string, string> Data, Dictionary<int, int> Hints)
{
    public MatchEvent(string kind, Dictionary<string, string>? data = null)
        : this(kind, data ?? [], []) { }
}

public sealed record SettingChoice(string Value, string Label);
public sealed record RuleSetting(string Key, string Label, string Default, SettingChoice[] Choices,
    int? Minimum = null, int? Maximum = null, int Step = 1);
public sealed record RuleDescriptor(string Id, string Name, string Presentation, RuleSetting[] Settings, int Version = 1);
public sealed record QueueDefinition(int Id, string Name, string Rule, Dictionary<string, string> Options);

public sealed class RuleRegistry
{
    private readonly Dictionary<string, IRuleSet> rules = new(StringComparer.Ordinal);
    public IEnumerable<IRuleSet> Rules => rules.Values;
    public void Register(IRuleSet rule)
    {
        if (!rules.TryAdd(rule.Descriptor.Id, rule))
            throw new ArgumentException("Duplicate rule id: " + rule.Descriptor.Id);
    }
    public IRuleSet Get(string id) => rules.TryGetValue(id, out IRuleSet? rule)
        ? rule : throw new ArgumentException("Unknown ruleset: " + id);
}
