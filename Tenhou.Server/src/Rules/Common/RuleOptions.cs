using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace Tenhou.Server.Rules.Common;

public static class RuleOptions
{
    public static SettingChoice[] Choices(params string[] values) => Enumerable.Range(0, values.Length / 2).Select(i => new SettingChoice(values[2 * i], values[2 * i + 1])).ToArray();
    public static RuleSetting Toggle(string key, string label, bool initial) => new(key, label, initial ? "1" : "0", Choices("0", "关", "1", "开"));
    public static RuleSetting Time { get; } = new("time", "思考时间", "10+20", Choices("5+20", "5+20", "10+20", "10+20", "60+0", "60+0", "300+0", "300+0"));
    public static Dictionary<string, string> Normalize(RuleDescriptor descriptor, IReadOnlyDictionary<string, string> input)
    {
        if (input.Keys.Any(k => k != "rule" && descriptor.Settings.All(f => f.Key != k)) || input.GetValueOrDefault("rule", descriptor.Id) != descriptor.Id)
            throw new ArgumentException("Unknown rule option");
        Dictionary<string, string> output = new() { ["rule"] = descriptor.Id };
        foreach (RuleSetting field in descriptor.Settings)
        {
            string value = input.GetValueOrDefault(field.Key, field.Default);
            if (field.Choices.Length > 0)
            { if (!field.Choices.Any(c => c.Value == value)) throw new ArgumentException("Invalid option: " + field.Label); }
            else if (!int.TryParse(value, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out int n)
                || n < field.Minimum || n > field.Maximum || (n - field.Minimum) % field.Step != 0) throw new ArgumentException("Invalid number: " + field.Label);
            output[field.Key] = value;
        }
        return output;
    }
}
