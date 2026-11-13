# Tenhou.Server.Tests

.NET 11 / C# 15 控制台测试项目，使用失败即返回非零退出码的现有测试运行器。构建只需要 .NET SDK。

| 文件 | 功能 |
| --- | --- |
| `Program.cs` | 立直规则/重放、TCP 目录和房间、幂等、恢复、多人同步动作、退出投票、真实进程启停 |
| `VariantChecks.cs`、`McrFixtures.json` | 四川与国标完整对局、重放、规则边界和国标计分参考样例 |
| `native_rule_oracle.cpp`、`native-riichi-reference/` | 冻结旧 C++ 规则参考样本，可选逐事件差异检查 |

在仓库根目录运行：

```powershell
.\build.ps1 --Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll
```

也可直接执行 `dotnet run --project Tenhou.Server.Tests -c Release`。界面夹具生成在测试程序相邻的 `ui-fixtures/` 下。

需要真实客户端协议栈或旧规则对照时，显式构建辅助程序：

```powershell
.\build.ps1 --Target Tenhou.Client.Tests
.\build.ps1 --Target Tenhou.Server.Tests --NativeReference
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --oracle .\build\Tenhou.Server.Tests\tenhou_rule_oracle.exe --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```

`--NativeReference` 需要 MSVC x86、Windows SDK 和 CMake；由客户端内部的唯一 CMake 工程加载本目录 `NativeReference.cmake`，不增加生产 C++ 项目，也不链接到 C# 服务。
