# 第三方参考与许可

国标麻将番名、番值、算番分类、排除规则及测试样例参考：

- GB-Mahjong，Copyright (c) 2019-2020 Zheng Fan <i@fanzheng.org>。
- 仓库：https://github.com/zheng-fan/GB-Mahjong
- 固定提交：`ace2c52f13e1ae4473fc267d8a86504167708334`。
- MIT 许可完整文本：[licenses/GB-Mahjong-MIT.txt](licenses/GB-Mahjong-MIT.txt)。

相关文件为 `Rules/Guobiao/FanCatalog.cs`、`Rules/Guobiao/McrScoring.cs` 和测试工程的 `McrFixtures.json`。服务端运行时使用本项目的托管 C# 实现，不加载该项目的 C++ 代码。

四川规则还参考竞技世界的公开[血流成河说明](https://www.jj.cn/news/320/20120925153300025557.shtml)，项目内可配置的计分约定以 RULES.md 为准。
