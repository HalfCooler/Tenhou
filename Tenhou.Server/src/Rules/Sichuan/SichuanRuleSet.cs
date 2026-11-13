using System;
using System.Collections.Generic;
using Tenhou.Server.Rules.Common;

namespace Tenhou.Server.Rules.Sichuan;

public sealed class SichuanRuleSet : IRuleSet
{
    public RuleDescriptor Descriptor { get; } = new("sichuan-bloodflow", "四川麻将 · 血流成河", "mahjong-v1",
    [
        new("hands", "盘数", "4", RuleOptions.Choices("1", "一盘", "4", "四盘", "8", "八盘", "16", "十六盘")),
        RuleOptions.Time, RuleOptions.Toggle("exchange", "开局换三张", true),
        new("base_points", "底分", "1", [], 1, 100, 1),
        new("fan_cap", "封顶番数", "5", [], 1, 10, 1),
        new("self_draw", "自摸计分", "add_base", RuleOptions.Choices("add_base", "自摸加底", "add_fan", "自摸加番")),
        RuleOptions.Toggle("menqing_zhongzhang", "门清、中张各加一番", false),
        RuleOptions.Toggle("call_transfer", "呼叫转移（一炮多响分别转移）", true),
        RuleOptions.Toggle("last_four_must_win", "最后四张能和必和", true),
        RuleOptions.Toggle("open_hands", "明牌练习", false)
    ]);
    public IReadOnlyDictionary<string, string> Normalize(IReadOnlyDictionary<string, string> options) => RuleOptions.Normalize(Descriptor, options);
    public IMatch Create(IReadOnlyDictionary<string, string> options, Random random) => new SichuanMatch(Normalize(options), random);
}
