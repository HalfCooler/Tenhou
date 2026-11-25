# Tenhou

`Tenhou.Client` 已基本完成天凤官方对局的重建，并进行了现代化改造：

- 完整的图像式字体嵌入（TrueType 渲染替代原版位图文字）
- 硬编码字符串提取与国际化支持
- 模型与视图解耦，牌桌呈现由 GDI+ 迁移至 Direct2D + D3D11

客户端提供两个入口：
- **天凤官方**：支持 `NoName` 与 19 位 ID，已重建对局大厅和个室（大会场有 Bug）；玩法支持除光速、雀庄战（祝仪）和东天红外的大部分桌型。
- **第三方服务器**（搭配 `Tenhou.Server`）：支持立直麻将、血战到底和国标麻将，当前未作维护主力。

## 项目结构

`Tenhou.slnx` 挂载四个项目（.NET 使用 `.csproj`，C++ 使用 Visual C++ Makefile 工程）；项目文档集中在 [docs/](docs/README.md)。

| 项目 | 说明 | SDK | 产物 |
| --- | --- | --- | --- |
| [Tenhou.Client](docs/Tenhou.Client/README.md) | Windows C++17 / Direct2D + GDI 客户端 | MSVC x86 | `build/Tenhou.Client/` |
| [Tenhou.Client.Tests](docs/Tenhou.Client.Tests/README.md) | 客户端资源、协议栈与牌桌检查 | MSVC x86 | `build/Tenhou.Client.Tests/` |
| [Tenhou.Server](docs/Tenhou.Server/README.md) | .NET 11 / C# 15 服务端 | .NET 11 | `build/Tenhou.Server/` |
| [Tenhou.Server.Tests](docs/Tenhou.Server.Tests/README.md) | 规则、计分、TCP 集成与恢复测试 | .NET 11 | `build/Tenhou.Server.Tests/` |

服务端设计见 [规则](docs/Tenhou.Server/RULES.md)、[协议](docs/Tenhou.Server/PROTOCOL.md)、[架构](docs/Tenhou.Server/ARCHITECTURE.md)。

## 构建

前置依赖：.NET 11 SDK、MSVC x86（VS「使用 C++ 的桌面开发」+ Windows SDK）、CMake 3.24+（NMake 路径需纯 ASCII）。在仓库根目录执行：

```powershell
.\Build-Tenhou                                    # 构建全部（默认 Release）
.\Build-Tenhou -Configuration Debug               # 构建 Debug
.\Build-Tenhou -Target Tenhou.Server              # 单独构建指定项目（C# 项目仅需 .NET SDK）
.\Build-Tenhou -Clean                             # 清理全部 bin、obj 与 build/
.\Build-Tenhou -Target Tenhou.Server.Tests -Clean # 清理指定项目
.\Build-Tenhou -Help                              # 查看完整参数
```

- 产物部署到 `build/<ProjectName>/`（Release 与 Debug 共用目录，保留最近一次构建）。
- 本地工具路径可在 `Build-Tenhou.ps1` 的 `$LocalEnvironment` 配置，或传 `-CMake`、`-VsDevCmd`、`-DotNet` 临时覆盖。
- 更换工具链后，请用 `-Fresh` 刷新缓存。
- 也可在 Visual Studio 中打开 `Tenhou.slnx`（C++ 项目对应 `Win32`），或执行 `dotnet build Tenhou.slnx -c Release`。
- 新增 C++ 源文件时，需要同步更新 `Tenhou.Client/CMakeLists.txt` 或对应 `.cmake`。

## 服务端启停与联机

```powershell
.\Tenhou.Server\start-server.cmd
.\Tenhou.Server\stop-server.cmd
# 或前台运行：
dotnet .\build\Tenhou.Server\Tenhou.Server.dll --config .\Tenhou.Server\server.json
```

默认监听 `127.0.0.1:10080`，数据目录相对配置文件解析（需保留的数据请放在 `build/` 之外）。联机时将 `server.json` 的 `listen` 改为对应 IPv4 地址，客户端选择“第三方服务器”连接；本机多开请使用不同的 `--config-dir`。

## 验证

```powershell
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```

可选构建冻结的旧规则对照程序（不参与生产服务）：

```powershell
.\Build-Tenhou -Target Tenhou.Server.Tests -NativeReference
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --oracle .\build\Tenhou.Server.Tests\tenhou_rule_oracle.exe --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```

详细测试项见各测试项目文档与 [验证记录](docs/Tenhou.Server/VALIDATION.md)。

## TODO

- 将 `Tenhou.Server` XML 通信逻辑回归原版；
- 精进第三方对局演出。
- 跨平台...？

## AI 使用声明

本项目的代码基本由 `GPT-6.2 Sol` 和 `Gemini 4 Neon/Carbon` 制作。
