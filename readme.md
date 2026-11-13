# Tenhou

服务端管理玩法目录、房间、合法动作、计分和恢复；客户端根据服务器提供的规则和设置创建房间。

当前提供立直麻将、四川血流成河和国标麻将。

`Tenhou.slnx` 统一登记四个项目：

| 项目 | 内容 | 构建产物 |
| --- | --- | --- |
| [Tenhou.Client](docs/Tenhou.Client/README.md) | Windows C++17 / GDI 客户端 | `build/Tenhou.Client/` |
| [Tenhou.Client.Tests](docs/Tenhou.Client.Tests/README.md) | 客户端资源、协议栈和通用牌桌检查 | `build/Tenhou.Client.Tests/` |
| [Tenhou.Server](docs/Tenhou.Server/README.md) | .NET 11 / C# 15 服务端 | `build/Tenhou.Server/` |
| [Tenhou.Server.Tests](docs/Tenhou.Server.Tests/README.md) | 规则、计分、TCP 集成、存储恢复和进程启停测试 | `build/Tenhou.Server.Tests/` |

规则、协议和运行时设计见 [规则约定](docs/Tenhou.Server/RULES.md)、[协议](docs/Tenhou.Server/PROTOCOL.md)、[架构](docs/Tenhou.Server/ARCHITECTURE.md)。完整文档入口见 [docs](docs/README.md)。

## 目录约定

四个项目直接放在解决方案根目录，每个项目的代码放在自身的 `src/` 下：

```text
Tenhou/
├─ Build-Tenhou.ps1              # 本地环境配置与项目调度
├─ Tenhou.slnx
├─ Tenhou.Client/                # 其他三个项目采用同样的布局
│  ├─ Build-Tenhou.Client.ps1     # 本项目编译、清理及部署
│  ├─ src/                      # 项目代码
│  ├─ bin/                      # 编译结果（忽略）
│  └─ obj/                      # 中间文件（忽略）
├─ scripts/Build.Common.ps1      # 共用构建函数
├─ docs/<项目名>/               # 项目说明、设计和验证文档
└─ build/<项目名>/              # 部署目录（忽略）
```

项目文件、构建定义、资源和运行配置保留在各项目中；项目文档统一在 `docs/` 维护。

## 统一构建

在仓库根目录执行：

```powershell
.\Build-Tenhou.ps1                                   # 默认 Release，构建全部四个项目
.\Build-Tenhou.ps1 -Configuration Debug              # 构建全部 Debug
.\Build-Tenhou.ps1 -Target Tenhou.Server              # 单独构建服务端
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests        # 单独构建客户端测试
.\Build-Tenhou.ps1 -Clean                            # 清理全部项目 bin、obj 和部署目录
.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests -Clean   # 只清理指定项目
.\Build-Tenhou.ps1 -Help                             # 查看参数与示例
Get-Help .\Build-Tenhou.ps1 -Full
```

构建脚本使用 PowerShell 单横线命名参数，例如 `-Target Tenhou.Server -Configuration Debug`。

根目录 `Build-Tenhou.ps1` 配置本地工具路径并调度各项目的 `Build-<项目名>.ps1`；项目脚本负责本项目的编译、清理和部署，共用函数位于 `scripts/Build.Common.ps1`。每个项目的编译结果保留在自己的 `bin/`，中间文件保留在自己的 `obj/`；成功后将运行所需文件复制到 `build/<项目名>/`。此部署目录对应最近一次构建的配置，Release 与 Debug 不分目录。

完整构建需要 .NET 11 SDK、MSVC x86、Windows SDK 和 CMake 3.24+。本机工具路径集中在 `Build-Tenhou.ps1` 的 `$LocalEnvironment` 中，留空时从 PATH 查找 CMake / .NET，通过 `vswhere` 探测 Visual Studio / Build Tools。也可以通过 `-CMake`、`-VsDevCmd`、`-DotNet` 临时覆盖：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client -VsDevCmd C:/BuildTools/Common7/Tools/VsDevCmd.bat -CMake 'C:/Program Files/CMake/bin/cmake.exe'
```

本地环境直接在脚本中配置，无需额外的配置文件。唯一的原生 `CMakeLists.txt` 位于 `Tenhou.Client/`，测试项目通过各自的 `.cmake` 文件登记目标；NMake 源码及构建路径需要使用 ASCII 字符。更新工具链后可传 `-Fresh` 重新探测。

两个 `.proj` 文件将客户端和客户端测试接入 MSBuild 解决方案，调用根脚本和客户端 CMake 工程。Windows 上也可以执行 `dotnet build Tenhou.slnx -c Release`。单独构建两个 C# 项目只需要 .NET SDK：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Server
.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests
```

构建只编译测试程序，测试运行命令见下文。

## 启停与联机

```powershell
.\Tenhou.Server\start-server.cmd
.\Tenhou.Server\stop-server.cmd
# 或前台运行分发目录中的服务
dotnet .\build\Tenhou.Server\Tenhou.Server.dll --config .\Tenhou.Server\server.json
```

默认监听 `127.0.0.1:10080`。启停脚本默认使用源码项目中的 `server.json`，数据目录相对配置文件解析。分发目录也包含服务配置和启停脚本，可部署整个 `build/Tenhou.Server/`。清理分发目录会同时删除其中的运行数据，需保留的数据应通过配置放在构建目录之外。

好友联机可将服务端配置 `listen` 改为适当的 IPv4 地址，客户端配置中 `[REBUILD] NativeHost` / `NativePort` 指向好友服务器。不同客户端窗口使用不同的 `--config-dir`。

## 验证

```powershell
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```

可选构建冻结的旧规则对照程序：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests -NativeReference
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --oracle .\build\Tenhou.Server.Tests\tenhou_rule_oracle.exe --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```

`Tenhou.Server.Tests/src/native-riichi-reference/` 只用于迁移差异检查，不参与生产服务。详细范围见各测试项目文档和 [验证记录](docs/Tenhou.Server/VALIDATION.md)。
