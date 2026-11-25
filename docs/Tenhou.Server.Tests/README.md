# Tenhou.Server.Tests

.NET 11 / C# 15 服务端测试项目（失败返回非零退出码），默认仅需 .NET SDK。

| 文件（`Tenhou.Server.Tests/` 下） | 说明 |
| --- | --- |
| `src/Program.cs` | 立直规则/重放、TCP 目录与房间、幂等、恢复、同步动作、退出投票、真实进程启停 |
| `src/VariantChecks.cs`、`src/SichuanChecks.cs`、`McrFixtures.json` | 四川血战与国标完整对局、重放、边界规则及计分样例 |
| `src/native_rule_oracle.cpp`、`src/native-riichi-reference/` | 冻结旧 C++ 立直规则参考，用于可选逐事件差异检查 |

## 运行

```powershell
.\Build-Tenhou -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll
# 或：dotnet run --project Tenhou.Server.Tests -c Release
```

界面夹具生成于测试程序旁的 `ui-fixtures/`。如需联跑客户端协议栈或旧规则对照（`-NativeReference` 需 MSVC x86、Windows SDK、CMake）：

```powershell
.\Build-Tenhou -Target Tenhou.Client.Tests
.\Build-Tenhou -Target Tenhou.Server.Tests -NativeReference
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --oracle .\build\Tenhou.Server.Tests\tenhou_rule_oracle.exe --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```
