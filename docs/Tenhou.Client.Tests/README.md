# Tenhou.Client.Tests

客户端专项检查的代码集中在 `Tenhou.Client.Tests/src/`，复用 `Tenhou.Client/CMakeLists.txt` 定义的客户端库及工具链。项目根目录的 `Build-Tenhou.Client.Tests.ps1` 负责构建与部署，`Tenhou.Client.Tests.proj` 将这些检查登记到 `Tenhou.slnx`；`Tests.cmake` 负责测试目标与 CTest 定义。

## 测试目标清单

下表源文件均位于 `Tenhou.Client.Tests/src/`：

| 文件 | CTest 名称 | 覆盖范围 |
| --- | --- | --- |
| `presenter_check.cpp` | `client_presenter` | 纯内存无窗口/无 GDI+ 验证：`TileGeometryCatalog`（精灵包围盒、偏移、1-bit Alpha 命中掩码）、`HandProjection`（缺门花色排序、手动插牌槽位重映射、非整数视口命中与高亮分类）及 `SettlementSequencer`（立直里宝 1200ms 停顿、役满音效、终局页 BGM/115 音效、暂停时间平移与 5 秒倒计时） |
| `offline_lobby_check.cpp` | `client_offline_lobby` | 自建原版大厅 `LobbySession` 切厅、大会资格、多票预约、并发 `GO` 收敛；官方 `HELO`/`RANKING v2`（15/21 项）/琥珀厅战绩解码与 `<ERR>` 致命分类 |
| `official_capture_check.cpp` | `client_official_captures` | 回放 `docs/Tenhou.Client.Origin/origin-capture/` 中的真实官方对局与断线重连原始帧样本，核对客户端协议与状态收敛 |
| `text_catalog_check.cpp` | `client_text_catalog` | 296 文字 / 7 图像分类、共享文案、481 役种完整样式、459 小字 12 pt、Info 三段装饰、生产按钮两态及全部立直/变体番型文字回退 |
| `text_composition_check.cpp` | `client_text_composition` | `FinalScene` 最终层合成；六组原生/物理尺寸下完整与局部重绘逐像素一致性、图文交错遮挡、淡出、裁切、曝光/DPI 恢复及缓存上限 |
| `variant_check.cpp` | `client_variants` | 四川与国标类型化快照、实体动作、换三张候选、多杠、血战退场、定缺装饰、108/136/144 张牌山游标、恢复事件及准备屏障 |
| `presentation_check.cpp` + `text_table_check.cpp` | `client_presentation` | 牌面几何与手牌理牌/拖拽投影、结算演出编排器（`SettlementSequencer`）与窗口级回归；生产 `LobbyView` 开局屏障、连续快照排队、摸牌等待、暂停恢复与跨盘准备；三玩法非整数缩放逐像素比对及 296 文字 PNG 资源隔离 |
| `resource_check.cpp` | `client_resources` | 外置及内嵌资源字节、背景/资源包解码、7 个保留编号图像 DAT 回退与 PNG 覆盖、全部音效别名和双 BGM 解码及播放路径 |
| `client_protocol_check.cpp` | （随服务端测试调用） | 使用真实客户端协议栈连接临时测试服务，跑完立直、四川及国标完整对局 |
| `variant_ui_check.cpp` | （手动截图工具） | 从服务端测试生成的 `ui-fixtures/*.txt` 渲染离屏 `OnlineTable` 与 `SettlementView` 牌桌截图（支持 `--settlement` 与 `--settlement-page=N`） |
| `text_samples.cpp` | （手动样张工具） | 导出全部 296 条 TrueType 文案、样式组合与透明 `info-layer` PNG 供人工视觉抽检 |
| `lobby_check.cpp` | （手动实窗工具） | 用生产 `LobbyView` 和本地隔离访客会话打开独立验收窗口连接指定本地端口 |

## 构建与运行

在仓库根目录执行：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure

# 联机协议端到端回归（由服务端测试驱动客户端协议检查器）
.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe

# 可选：通过真实 waveOut 设备顺序播放音效做听感抽检
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets --play-audio

# 可选：渲染变体牌桌与结算离屏截图
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle.bmp
.\build\Tenhou.Client.Tests\tenhou_variant_ui_check.exe .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-bloodbattle-retired.txt .\build\Tenhou.Server.Tests\ui-fixtures\sichuan-settlement.bmp .\build\Tenhou.Client.Tests\resources --settlement

# 可选：原版采集脚本与 Frida 钩子单元测试
python -B -m unittest discover -s scripts/tests -v
```

## 自动化覆盖与实窗验收边界

- **纯内存无头检查**：协议解析、大厅状态机、手牌理牌/手动重排/命中投影、结算逐帧揭示与确认倒计时状态机、文字规格目录均在无窗口或纯内存路径中确定性验证，不受桌面锁屏影响。
- **离屏光栅回归**：`client_text_composition` 与 `client_presentation` 通过内存 DIB 对局部脏区缓存合成与完整全帧渲染做逐像素等价断言，并输出 `text-p2/`、`text-p3/`、`text-p4/` 样张备查。
- **仍需实窗人工确认的范围**：真实鼠标拖拽手感、窗口焦点切换、高刷显示器动态观感及 `waveOut` 实际外放听感，需在解锁桌面下运行 `tenhou_rebuild.exe` 或 `tenhou_lobby_check.exe` 抽检。
