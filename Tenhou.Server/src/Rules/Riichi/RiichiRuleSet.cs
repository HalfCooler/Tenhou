using System;
using System.Collections.Generic;

namespace Tenhou.Server.Rules.Riichi;

public sealed class RiichiRuleSet : IRuleSet
{
    private static SettingChoice[] Choices(params string[] items)
    {
        SettingChoice[] choices = new SettingChoice[items.Length / 2];
        for (int i = 0; i < choices.Length; ++i) choices[i] = new(items[2 * i], items[2 * i + 1]);
        return choices;
    }
    private static RuleSetting Toggle(string key, string label, bool value) => new(key, label, value ? "1" : "0",
        Choices("0", "关", "1", "开"));
    public RuleDescriptor Descriptor { get; } = new("riichi", "立直麻将", "riichi-v1",
    [
        new("match_type", "人数", "standard4", Choices("standard4", "四人", "standard3", "三人")),
        new("length", "局数", "east", Choices("single", "一局", "east", "东风", "south", "半庄")),
        new("time", "思考时间", "5+20", Choices("3+5", "3+5", "5+10", "5+10", "5+20", "5+20", "60+0", "60+0", "300+0", "300+0")),
        new("starting_points", "起始点数", "25000", [], 0, 200000, 100),
        new("target_points", "终局目标点数", "30000", [], 0, 100000000, 100),
        new("red_count", "赤宝牌（四人0/3/4，三人0/2/3）", "3", Choices("0", "0", "2", "2", "3", "3", "4", "4")),
        Toggle("bankruptcy", "击飞", true), Toggle("ancient", "古役", false), Toggle("open_tanyao", "食断", true),
        new("minimum", "翻缚", "1", Choices("1", "1翻", "2", "2翻", "4", "4翻", "mangan", "满贯", "haneman", "跳满", "baiman", "倍满", "sanbaiman", "三倍满", "yakuman", "役满")),
        Toggle("open_hands", "明牌", false)
    ]);
    public IReadOnlyDictionary<string, string> Normalize(IReadOnlyDictionary<string, string> options)
        => RiichiOptions.From(options).ToSettings();
    public IMatch Create(IReadOnlyDictionary<string, string> options, Random random)
        => new RiichiMatch(RiichiOptions.From(options), random);
}
