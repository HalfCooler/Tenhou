# Tenhou.Client.Tests

客户端专项检查的代码集中在 `Tenhou.Client.Tests/src/`，复用 `Tenhou.Client/CMakeLists.txt` 定义的客户端库及工具链。项目根目录的 `Build-Tenhou.Client.Tests.ps1` 负责构建与部署，`Tenhou.Client.Tests.proj` 将这些检查登记到 `Tenhou.slnx`；`Tests.cmake` 负责测试目标定义。下表文件位于项目的 `src/` 中。

| 文件 | 功能 |
| --- | --- |
| `resource_check.cpp` | 外置及内嵌资源字节、背景/资源包解码、旧版平铺路径兼容、全部音效别名和双 BGM 解码、播放路径与覆盖配置 |
| `text_catalog_check.cpp` | P0 的 296 文字 / 7 图像分类、共享文案、481 通常役种完整样式、459 小字 12 pt、Info 三段装饰；生产按钮两态与全部立直/变体番型文字回退 |
| `text_samples.cpp` | P1 人工样张导出工具 `tenhou_text_samples`；全部 296 文案、样式、组合与最终尺寸变换，不登记 CTest 自动运行 |
| `text_composition_check.cpp` | P2 最终层合成；六组原生/物理尺寸下完整/局部重绘逐像素比较、图文顺序、淡出、裁切、曝光/尺寸/DPI 恢复及缓存上限，登记 `client_text_composition` |
| `client_protocol_check.cpp` | 使用真实客户端协议栈连接测试服务，跑完立直、四川及国标对局 |
| `variant_ui_check.cpp` | 从服务端测试生成的状态夹具渲染离屏 GDI 牌桌截图 |
| `variant_check.cpp` | 有类型快照、实体动作、换牌候选、多杠、退场、定缺装饰、牌山游标、恢复事件及提交屏障 |
| `text_table_check.cpp` | P3 生产三玩法牌桌、按钮、开局/恢复及回放；非整数输出下完整/缓存逐像素比较、静止缓存复用及缺失/损坏文字 PNG 隔离验证，随 `client_presentation` 执行 |
| `presentation_check.cpp` | 生产 LobbyView 的开局准备、连续快照排队、摸牌等待、暂停与恢复、跨盘准备；实际牌面资源的侧家和牌间距；结算开窗及逐项音效、静音和隐藏取消 |
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

2026-11-17 TrueType P0：新增 `client_text_catalog`，日常构建只读已登记 JSON 并由 CMake 生成类型化目录，不依赖 Excel/Python。显式导入和错误输入检查可运行 `python -B scripts/tests/test_text_catalog.py -v`；仅在临时副本中修改数据。目录维护说明见 [文字目录](../../Tenhou.Client/assets/text/README.md)，逐项人工验收见 [核对表](../Tenhou.Client/TRUETYPE-TEXT-P0-REVIEW.md)。P0 尚未接入字体绘制，不进行后续阶段的窗口视觉或协议回归。

P0 规格确认补充：普通按钮选择黑边、悬停选择橙边，595–598 / 599–602 两组已确认；481 复用通常役种完整样式，459 小字登记为 12 pt，566 渐变已由用户逐像素确认。`client_text_catalog` 通过生产按钮选择与缺图结算场景检查目录回退，覆盖 55 个立直役种、101 个变体匹配名、未知名、重复次数及回执数据不被显示映射修改。

2026-11-17 TrueType P1：用户确认 P0 验证完成。新增字体渲染基础与 `tenhou_text_samples`，随本项目构建；人工样张命令及核对记录见 [P1 说明](../Tenhou.Client/TRUETYPE-TEXT-P1-REVIEW.md)。P1 的视觉验证、效果数值校准交给人工；不以编译或导出成功代替验收。

P1 交付补充：用户已完成视觉验证，仅报告 594 样张偏左和 Info 背景合成两项问题。已改为共用浮字居中入口及透明底上的独立灰度图层；`info-layer-<宽度>.png` 保留原生 Alpha，可在图像编辑器检查棋盘格。交付记录已更新，实际窗口验收随 P2 / P6 继续。

2026-11-17 TrueType P2：新增 `client_text_composition`，复用生产 `PaintBuffer` / `RetainedScene` / `CachedHud` 检查最终尺寸文字、交错图像和缓存失效。独立运行 `build/Tenhou.Client.Tests/tenhou_text_composition_check.exe build/Tenhou.Client.Tests/resources build/text-p2` 可导出 18 张初始/淡出/清空图；CTest 默认输出在其构建目录的 `text-p2/`。接口、覆盖及实际窗口验证边界见 [P2 记录](../Tenhou.Client/TRUETYPE-TEXT-P2-REVIEW.md)。

2026-11-13：四川第一轮实现已通过原生控制器、资源和真实协议完整对局检查，并检查阶段离屏截图。桌面锁定期间未运行实际窗口点击；单击/拖动、多个杠的真实候选操作、按下期间换 revision、暂停恢复以及立直窗口回归均保留为手工验收项。144 张牌山目前只通过游标及数量测试，18 墩布局不能据此视为通过。

2026-11-16：新增国标八花四向合成、首花与副露占位、18 墩不透明像素遮挡、下层阴影、连续补花摸牌起点、82 项图片映射、PNG 覆盖/回退、分页与开局/恢复浮字检查。国标补花与番种图片路径见 [接入说明](../Tenhou.Client/GUOBIAO.md)。离屏结算可用 `--settlement-page=1` 检查第二页；当前验证不替代实际鼠标及听感验收。

2026-11-16 编号图接入检查覆盖换牌和定缺按钮、玩法标签和最终局提示、逐项翻数、流局合并分数卡，以及盘末逐份确认。对应文案现已迁移到 [TrueType 文字目录](../../Tenhou.Client/assets/text/README.md)。

2026-11-16 会话与跨盘回归：`client_variants` 检查开局回执保留动作、NEXTREADY 只释放对应提交、迟到回执与失效候选；`client_presentation` 将真实 LobbySession 的 PREPARE/VSTATE/READYACK/START 序列送入生产 LobbyView，覆盖四川和国标的首盘与次盘。结算检查覆盖每位和牌者、流局收据、番种分页、末盘自动确认、隐藏取消、禁用后重新计时与历史浏览；测试推进倒计时截止时间，不依赖真实等待。服务端 TCP 检查覆盖失效及账号令牌下的 NoName 自动新建访客，以及已有访客的重启续局。

开局呈现补充：首盘准备标题使用 463，首盘确认期间发牌计时不推进，START 不取消本地准备窗口，跨盘不重复首盘介绍；确认换牌只在发牌结束、选齐三张合法牌后显示。`dealN` 保留原始发牌顺序，检查首批四张不预先排序、发完后统一理牌及隐藏对手发牌实体；四川东1～南4、国标东1～北4均检查现有局数图片映射。

2026-11-17 TrueType P3：`client_presentation` 新增生产文字接入检查，截图输出到所用资源根目录父级的 `text-p3/`；本次另复制到 `build/text-p3/`。覆盖范围、三玩法真实协议检查结果和原生 SD/实际窗口验收边界见 [P3 记录](../Tenhou.Client/TRUETYPE-TEXT-P3-REVIEW.md)。

2026-11-18 TrueType P5：`client_resources` 检查打包目录及 7 个编号图像的 DAT 回退、PNG 覆盖和 SD 缩放。`client_text_catalog` 核对运行清单和生产按钮映射。`client_presentation` 验证 296 个文字编号的资源隔离，并通过生产 Info 层检查玩法标签和最终局提示。详见 [P5 记录](../Tenhou.Client/TRUETYPE-TEXT-P5-REVIEW.md)。
