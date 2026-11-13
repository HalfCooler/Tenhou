# Tenhou.Client.Tests

客户端专项检查的代码集中在 `Tenhou.Client.Tests/src/`，复用 `Tenhou.Client/CMakeLists.txt` 定义的客户端库及工具链。项目根目录的 `Build-Tenhou.Client.Tests.ps1` 负责构建与部署，`Tenhou.Client.Tests.proj` 将这些检查登记到 `Tenhou.slnx`；`Tests.cmake` 负责测试目标定义。下表文件位于项目的 `src/` 中。

| 文件 | 功能 |
| --- | --- |
| `resource_check.cpp` | 外置及内嵌资源字节、背景/资源包解码、旧版平铺路径兼容、全部音效别名和双 BGM 解码、播放路径与覆盖配置 |
| `client_protocol_check.cpp` | 使用真实客户端协议栈连接测试服务，跑完立直、四川及国标对局 |
| `variant_ui_check.cpp` | 从服务端测试生成的状态夹具渲染离屏 GDI 牌桌截图 |

在仓库根目录构建和运行：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets
# 可选：通过真实 waveOut 设备顺序播放移牌、摸牌、打牌、开场、碰、荣和
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets --play-audio
# 同一资源检查也登记到 CTest
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure

.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe

.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.bmp
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\guobiao.txt .\build\Tenhou.Server.Tests\ui-fixtures\guobiao.bmp
```

协议检查由服务端测试驱动，它会创建临时服务、分配端口并管理生命周期。UI 检查生成截图，仍需人工检查布局。
