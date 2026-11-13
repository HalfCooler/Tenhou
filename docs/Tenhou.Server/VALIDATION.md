# 首轮迁移验证记录

验证日期：2026-11-13。环境为 Windows x64，服务端 .NET SDK 11.0.100 / Runtime 11.0.0；桌面客户端及参考程序为 MSVC x86，最终工具集14.52.37025（编译器19.52.37029.0），系统 Windows SDK 10.0.26100.0。

## 已通过

- .NET Debug / Release 构建，nullable 与 warnings-as-errors 开启，零警告。
- MSVC x86 Debug 构建；通过根目录脚本的完整 Release 构建。生产服务不编译原生规则参考程序。
- 国标130个参考算番样例，覆盖81个标准番种及明暗杠扩展，总番数与参考样例相同。
- 立直三人、四人各6个种子，包含古役开关；完成完整对局、逐操作牌张/点数守恒、独立牌山重放。与冻结 C++ 参考程序逐事件、役种、得分和终局对比通过，比较时只归一化装饰性骰子和数值格式。
- 四川与国标各4组完整对局及重放。额外验证血流连续两次自摸后仍保留13张锁定手牌、查叫时排除暗杠耗尽的第五张虚拟牌、不同和型不混算。
- 真实 C++ `LobbySession` / `OnlineGame` / `OnlineActions` 协议栈连接 .NET 服务：立直完整对局、四川四盘、国标四盘，含下一盘准备和返回大厅。
- TCP 规则/队列目录、重复开房幂等、设置 revision 更新与冲突、JSON 保存与服务重启恢复、单真人退出。
- 四个 TCP 客户端进入同一好友房、注册账号、同时换三张与定缺；相同合法选择不会因其他人的提交失效。退出投票2票对2票继续，3票对1票终止。
- 真实独立服务进程、命名管道优雅停止、中文路径、损坏状态拒绝启动且保留原文件。
- 后台 start/stop 脚本使用含空格的配置路径启动和优雅停止，通过。
- 四川换牌、国标摸打的离屏 GDI 牌桌截图，人工检查文字、花牌、操作列表和布局。
- `git diff --check`。

## 复现

```powershell
dotnet run --project Tenhou.Server.Tests -c Release
./Build-Tenhou.ps1 -Fresh -NativeReference
dotnet ./build/Tenhou.Server.Tests/Tenhou.Server.Tests.dll --oracle ./build/Tenhou.Server.Tests/tenhou_rule_oracle.exe --native-client ./build/Tenhou.Client.Tests/tenhou_protocol_check.exe
./build/Tenhou.Client.Tests/tenhou_variant_ui_check.exe build/Tenhou.Server.Tests/ui-fixtures/sichuan-bloodbattle.txt build/Tenhou.Server.Tests/ui-fixtures/sichuan-bloodbattle.bmp
./build/Tenhou.Client.Tests/tenhou_variant_ui_check.exe build/Tenhou.Server.Tests/ui-fixtures/guobiao.txt build/Tenhou.Server.Tests/ui-fixtures/guobiao.bmp
```

Debug 构建时，本机 MSVC / SDK 正在升级，曾临时使用工作区内官方 Windows SDK NuGet 包10.0.28000.2705完成隔离构建。系统 SDK 恢复后，最终 Release 已通过普通仓库构建入口。隔离 SDK、下载的参考仓库和测试状态均不纳入源码。

## 验证范围

本次没有进行 Linux/macOS 运行验证、长时间压力测试或全部牌山穷举。参考样例覆盖全部番种，不能消除所有罕见组合及规则解释分歧。

四川与国标目前使用可操作的通用文字牌桌，立直保留原有 GDI 牌桌；通用牌桌未接入立直动画、音效和牌谱浏览器。四川/国标的 JSON 牌谱保存在服务器，客户端自动下载 XML 仅用于立直。

## 项目结构整理验证（2026-11-13）

四个项目保持在解决方案根目录，项目代码统一位于各自的 `src/`，文档统一位于根目录 `docs/`。37 个迁移的代码文件与迁移前内容一致；两个 C# 工程显式只编译 `src/**/*.cs`。

本次通过：

- `./Build-Tenhou.ps1 -Clean` 后执行 `./Build-Tenhou.ps1 -NativeReference -Fresh`，四项目 Release 编译、部署及可选原生规则对照程序均通过。
- `dotnet build Tenhou.slnx -c Release`，验证 `.proj` 经 Windows PowerShell 调用新入口，零警告、零错误。
- 上文带 `--oracle`、`--native-client` 的完整服务端测试，包含规则差异、真实客户端联机、TCP 持久化与恢复、四客户端房间及真实进程启停。
- `ctest --test-dir ./Tenhou.Client.Tests/obj/native/Release --output-on-failure` 及部署目录中的资源检查程序。
- PowerShell 语法、帮助、无效目标和原生参考目标约束检查；源码与部署文档的本地链接检查；`git diff --check`。

此次结构整理未重新执行 Debug 全量构建、GUI 人工检查或跨平台运行验证。

## 四川血战村规修订验证（2026-11-13）

本节替代上文旧血流规则的验证口径。新增 `SichuanChecks.cs`，使用定制牌山验证和牌退场、跳过已和玩家、有效付款人数、三家和完、逆时针截和（反序提交）、首和坐庄、抢加杠不收税、杠上炮加翻且不转移杠收入。覆盖基本翻种、根与龙七对扣根、清幺九按清一色带幺九、天和/地和、杠上花、最大听牌赔付、花猪、无封顶按5翻赔付、退税与保留杠税、八局换牌顺序、固定八局终局、默认不击飞及可选击飞、末四张仍可不和。

通过 .NET Release 测试：四川四组完整八局与逐事件重放、国标四组对局与130个计分样例、立直三/四人各六组、TCP房间和重启恢复、四人同步换牌/定缺与退出投票、独立进程启停。MSVC Release 客户端及客户端测试构建、CTest资源检查通过。真实 C++ 协议栈联机血战八局，含 NEXTREADY 与返回大厅。

界面夹具新增 `sichuan-bloodbattle-retired.txt`，展示已和退场与等待提示；初始换牌夹具更新为 `sichuan-bloodbattle.txt`。
