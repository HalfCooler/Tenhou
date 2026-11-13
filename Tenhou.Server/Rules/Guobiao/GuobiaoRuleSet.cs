using System;
using System.Collections.Generic;
using Tenhou.Server.Rules.Common;

namespace Tenhou.Server.Rules.Guobiao;

public sealed class GuobiaoRuleSet : IRuleSet
{
    public RuleDescriptor Descriptor { get; } = new("guobiao", "国标麻将", "mahjong-v1",
    [
        new("hands", "盘数（标准赛制16盘）", "16", RuleOptions.Choices("1", "一盘", "4", "四盘", "8", "八盘", "16", "十六盘")),
        RuleOptions.Time,
        RuleOptions.Toggle("flowers", "使用八张花牌", true),
        RuleOptions.Toggle("open_hands", "明牌练习", false)
    ]);
    public IReadOnlyDictionary<string, string> Normalize(IReadOnlyDictionary<string, string> options) => RuleOptions.Normalize(Descriptor, options);
    public IMatch Create(IReadOnlyDictionary<string, string> options, Random random) => new GuobiaoMatch(Normalize(options), random);
}
