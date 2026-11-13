using System;
using System.Collections.Generic;
using Tenhou.Server.Rules.Common;

namespace Tenhou.Server.Rules.Sichuan;

public sealed class SichuanRuleSet : IRuleSet
{
    public RuleDescriptor Descriptor { get; } = new("sichuan-bloodbattle", "四川麻将 · 血战到底", "mahjong-v1",
    [
        RuleOptions.Time,
        new("fan_cap", "封顶翻数（0为不封顶）", "5", [], 0, 10, 1),
        RuleOptions.Toggle("bankruptcy", "击飞（负分终局）", false),
        RuleOptions.Toggle("open_hands", "明牌练习", false)
    ]);
    public IReadOnlyDictionary<string, string> Normalize(IReadOnlyDictionary<string, string> options) => RuleOptions.Normalize(Descriptor, options);
    public IMatch Create(IReadOnlyDictionary<string, string> options, Random random) => new SichuanMatch(Normalize(options), random);
}
