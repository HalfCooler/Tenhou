# Tenhou.Client.Tests

客户端专项检查代码位于 `Tenhou.Client.Tests/src/`，复用 `Tenhou.Client/CMakeLists.txt`，测试目标由 `Tests.cmake` 定义。

## 测试目标

| 文件（`src/` 下） | CTest | 说明 |
| --- | --- | --- |
| `presenter_check.cpp` | `client_presenter` | 纯内存验证几何目录、手牌理牌/拖拽/命中投影、结算与在线/回放演出编排器 |
| `offline_lobby_check.cpp` | `client_offline_lobby` | 自建大厅会话（切厅/大会/预约/`GO`）、官方 `HELO`/`RANKING v2`/`<ERR>` 解码与大厅计时 |
| `official_capture_check.cpp` | `client_official_captures` | 回放 `docs/Tenhou.Client.Origin/origin-capture/` 官方原始帧，核对协议与断线重连收敛 |
| `text_catalog_check.cpp` | `client_text_catalog` | 296 文字 / 7 图像分类、样式、共享文案、481 役种、Info 装饰、按钮两态与番型回退 |
| `text_composition_check.cpp` | `client_text_composition` | `FinalScene` 多尺寸全帧与局部重绘逐像素一致性、遮挡/淡出/裁切/DPI 恢复与缓存上限 |
| `variant_check.cpp` | `client_variants` | 四川与国标类型化快照、换三张/定缺/血战退场、108/136/144 牌山游标与恢复屏障 |
| `presentation_check.cpp` + `text_table_check.cpp` | `client_presentation` | 牌桌与结算窗口级回归、GDI 缓存逐像素比对、Direct2D WARP 离屏与 flip swap chain、画布按钮交互与文字资源隔离 |
| `resource_check.cpp` | `client_resources` | 外置/内嵌资源包解码、7 保留图像回退/覆盖、全部音效与双 BGM 播放路径 |
| `client_protocol_check.cpp` | 随服务端测试调用 | 用真实客户端协议栈连接临时服务，跑完立直、四川与国标对局 |
| `variant_ui_check.cpp` | 手动工具 | 将服务端生成的 `ui-fixtures/*.txt` 渲染为离屏牌桌/结算截图（支持 `--settlement`、`--settlement-page=N`） |
| `text_samples.cpp` | 手动工具 | 导出全部 296 条 TrueType 文案与透明 `info-layer` 样张 |
| `lobby_check.cpp` | 手动工具 | 用生产 `LobbyView` 打开隔离访客窗口连接指定本地端口 |

## 运行

```powershell
.\Build-Tenhou -Target Tenhou.Client.Tests
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure

# 联机协议端到端回归
.\Build-Tenhou -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe

# 可选：真实 waveOut 听感抽检
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets --play-audio

# 可选：渲染变体离屏截图
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.bmp
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle-retired.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-settlement.bmp .\Tenhou.Client.Tests\resources --settlement

# 可选：采集脚本单元测试
python -B -m unittest discover -s scripts/tests -v
```

## 验收边界

- **无头与离屏回归**：协议、大厅、投影/演出状态机、文字目录在纯内存验证；`client_text_composition` 与 `client_presentation` 做 DIB 逐像素比对及 Direct2D（WARP）对照 GDI 检查，样张输出至 `text-p2/`、`text-p3/`、`text-p4/`。
- **Direct2D 窗口路径**：测试窗口验证 flip swap chain 创建、调整与设备重置恢复。
- **实窗人工抽检**：真实鼠标拖拽手感、焦点切换、高刷观感与 `waveOut` 外放效果，需在解锁桌面下用 `tenhou_rebuild.exe` 或 `tenhou_lobby_check.exe` 确认。
