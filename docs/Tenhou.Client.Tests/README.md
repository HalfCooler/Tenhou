# Tenhou.Client.Tests

客户端专项检查的代码集中在 `Tenhou.Client.Tests/src/`，复用 `Tenhou.Client/CMakeLists.txt` 定义的客户端库及工具链。项目根目录的 `Build-Tenhou.Client.Tests.ps1` 负责构建与部署，`Tenhou.Client.Tests.proj` 将这些检查登记到 `Tenhou.slnx`；`Tests.cmake` 负责测试目标定义。下表文件位于项目的 `src/` 中。

| 文件 | 功能 |
| --- | --- |
| `resource_check.cpp` | 外置及内嵌资源字节、背景/资源包解码、旧版平铺路径兼容、全部音效别名和双 BGM 解码、播放路径与覆盖配置 |
| `client_protocol_check.cpp` | 使用真实客户端协议栈连接测试服务，跑完立直、四川及国标对局 |
| `variant_ui_check.cpp` | 从服务端测试生成的状态夹具渲染离屏 GDI 牌桌截图 |
| `variant_check.cpp` | 有类型快照、实体动作、换牌候选、多杠、退场、定缺装饰、牌山游标、恢复事件及提交屏障 |
| `lobby_check.cpp` | 用生产 LobbyView 和本地访客会话打开独立验收窗口；仅手动运行 |

在仓库根目录构建和运行：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
.\build\Tenhou.Client.Tests\tenhou_variant_check.exe
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets
# 可选：通过真实 waveOut 设备顺序播放移牌、摸牌、打牌、开场、碰、荣和
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets --play-audio
# 同一资源检查也登记到 CTest
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure

.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe

.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.bmp
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\guobiao.txt .\build\Tenhou.Server.Tests\ui-fixtures\guobiao.bmp

# 服务端测试还会生成四川各阶段（含 retired）的独立夹具；四川使用生产 OnlineTable 渲染
Get-ChildItem .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle-*.txt | ForEach-Object {
    & .\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe $_.FullName ([IO.Path]::ChangeExtension($_.FullName, '.bmp'))
}
# 单独检查首张和牌收据；资源目录之后加 --settlement
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle-retired.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-settlement.bmp .\build\Tenhou.Client.Tests\resources --settlement

# 解锁桌面后手动运行；仅连接本地测试服务，配置与日志写入独立目录
.\build\Tenhou.Client.Tests\tenhou_lobby_check.exe 10080 .\build\manual-sichuan-check
```

协议检查由服务端测试驱动，它会创建临时服务、分配端口并管理生命周期。UI 检查生成截图，仍需人工检查布局。

2026-11-13：四川第一轮实现已通过原生控制器、资源和真实协议完整对局检查，并检查阶段离屏截图。桌面锁定期间未运行实际窗口点击；单击/拖动、多个杠的真实候选操作、按下期间换 revision、暂停恢复以及立直窗口回归均保留为手工验收项。144 张牌山目前只通过游标及数量测试，18 墩布局不能据此视为通过。
