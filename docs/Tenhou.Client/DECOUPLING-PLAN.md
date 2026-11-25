# Tenhou.Client 解耦与 Direct2D 牌桌渲染迁移路线

本文档记录 `Tenhou.Client` 从强耦合 Win32 / GDI+ 架构向“纯 C++ 演出编排层（`tenhou_presenter`）+ 牌桌渲染接口（`ITableCanvasRenderer`）+ Direct2D 1.1 / D3D11 牌桌后端”演进的任务目标、强制约束边界、完成项与验收范围，便于后续直接接续实施。

---

## 一、任务目标

1. **逻辑 / 演出与窗口 / 绘图解耦（Stage 1 & Stage 2）**
   - 清理过时文档与失效脚本测试，移除废弃视图原型（`VariantTableView` 等）。
   - 将原版几何锚点目录、手牌理牌/手动拖牌重排、结算揭露、在线对局/离线回放演出时序与自动操作状态机、牌桌渲染帧快照统一抽离到无 `<windows.h>` / `<gdiplus.h>` 依赖的纯 C++ 静态库 `tenhou_presenter`。
   - 将在线牌桌（`OnlineTable`）与离线回放（`TableView::PaintTable`）长期并行的两套牌桌绘制管线收拢为单一 `TableRenderFrame` + `ITableCanvasRenderer`。
2. **牌桌动作按钮收拢与 Direct2D + D3D11 局部硬件加速（Stage 3.2-B，已核准方案）**
   - **保留部分**：保留登录欢迎页、设置面板、原版大厅（`OfflineLobbyView`）、匹配视图、投票退出弹窗及 `SettlementView` 等 Win32 表单/控件壳层；**完整保留已逐像素校准的 `TextRenderer`（GDI+ 私有字体排版、描边、渐变、发光与阴影栅格化引擎）**。
   - **升级部分**：将牌桌内动作按钮从零散子 `HWND` 收拢为 `TableRenderFrame::buttons` 画布自绘与矩形命中；在此基础上实现 `D2DTableRenderer`（Direct2D 1.1 `ID2D1DeviceContext` + D3D11 + DXGI Flip-Model SwapChain + 离屏读回 `RenderToDc`），将 `TextRenderer` 生成的预乘 BGRA 位图、牌面、背景与阴影常驻上传为 `ID2D1Bitmap1`，把缩混、`HandColor` 矩阵着色与飞牌透明度混合迁入 GPU。

---

## 二、强制边界与行为基准（不可破坏）

1. **原版行为基准（见 [AGENTS.md](../../AGENTS.md)）**
   - 一切资源、布局、坐标、动画、声音与协议时序以 `Tenhou.Client.Origin/tenhou/tenhou.exe`（Windows 版 `130/0005`，SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）为基准，不得因解耦改变任何演出顺序或时序常量。
   - 必须原样保留的已核验时序与交互规则包括：
     - **对局封包排队与节拍**：对手摸牌停顿 `41` tick、舍牌停顿 `15` tick（`ActionPauseTicks`），自家摸牌无人工等待；流局倒牌 14 槽位共 `28` tick 并配音效 `104`；立直自摸/摸切自动打出 `1000ms` 等待（`kAutomaticDiscardWaitMs`）；和牌 `2000ms` 结算弹出延迟（`FUN_004aad30`）；`CLOCK` 倒计时 `<= 2s` 每秒触发一次 `108` 音效；对局暂停时长平移上述截止时间。
     - **动画与音效停止顺序**：`OnlineTableSequencer` 在 `INIT` / 新局切换 / `RESTORED` 时先发出 `stop_all_sound` 并重置 `opening_` / 动画，再发新局开局与音效；`LobbyView::ApplyEffects` 必须严格先执行 `stop_all_sound`（不可误清刚生成的新局 `opening_` 或 `deferred_sounds_`），再处理 `intro`、`settlement_sync` 与 `play_sounds`。
     - **结算演化**：揭示帧步进、`0x004ad739` 里宝 `1200ms` 停顿、役满 `111 + 109` 音效、进入终局页触发 `115`（`SE_GAMEEND`）+ `BGM_GAME9`、非动画纯音效揭示结束后才开启 5 秒自动 OK 倒计时。
     - **手牌投影与拖拽**：`UpdateOriginSuitOrder` 缺门花色后移、`FUN_00488650` 手动插牌后将摸牌视觉间隙重映射到末尾槽位、以及按 `actions.revision` 取消过期按键或拖拽捕获。
2. **文件与架构边界**
   - `tenhou_presenter` 目标（位于 [Tenhou.Client/CMakeLists.txt](../../Tenhou.Client/CMakeLists.txt)）禁止引入 `<windows.h>` 或 `<gdiplus.h>`。
   - 必须保留 `ITableCanvasRenderer::RenderToDc` 离屏绘制能力（Direct2D 下通过 D3D11 离屏渲染目标 + Staging Texture 读回 DIB），确保 `LobbyView::Capture(HDC)`、`SettlementView` 背景采样、[`variant_ui_check.cpp`](../../Tenhou.Client.Tests/src/variant_ui_check.cpp) 及现有像素回归测试无断档继续工作。
   - 禁止凭印象改动字体轮廓或替换 `TextRenderer`；`D2DTableRenderer` 必须复用 `TextRenderer` 输出的预乘 ARGB 光栅。
   - [ORIGIN.md](ORIGIN.md) 末尾的 `## 随笔` 章节不得删改。
   - MSVC C++17 编译策略始终为 `/W4 /WX /utf-8 /EHsc`，零警告零错误。

---

## 三、实施记录

### 1. Stage 1：文档、测试与废弃原型清理（提交 `f2d6d58`）
- 移除失效脚本与废弃视图原型：`scripts/tests/test_text_catalog.py`、`scripts/tests/test_original_lobby_p0.py`、`__pycache__`、[variant_table_view](../../Tenhou.Client/CMakeLists.txt)（原 `variant_table_view.hpp/.cpp`）及不可达 `text_details_` 分支。
- 修复仓库文档断裂链接，新增 [Tenhou.Client 总览](README.md) 与 [RENDERING-MIGRATION.md](RENDERING-MIGRATION.md)。

### 2. Stage 2 Step 2.1：几何目录与手牌投影（提交 `f2d6d58`）
- 新增 [`tile_geometry.hpp`](../../Tenhou.Client/src/tile_geometry.hpp) / [`tile_geometry.cpp`](../../Tenhou.Client/src/tile_geometry.cpp)（`TileGeometryCatalog`，直接解析 `_obj` 索引头获取精灵矩形、`variant = 34` 基准矩形、偏移、深度排序及 1-bit Alpha 命中掩码）。
- 新增 [`hand_projection.hpp`](../../Tenhou.Client/src/hand_projection.hpp) / [`hand_projection.cpp`](../../Tenhou.Client/src/hand_projection.cpp)（`HandProjection`、`TableInteraction`、`HandColor` 分类、花色排序与插牌槽位重映射）。

### 3. Stage 2 Step 2.2：结算演出编排器（提交 `f2d6d58`）
- 新增 [`settlement_sequencer.hpp`](../../Tenhou.Client/src/settlement_sequencer.hpp) / [`settlement_sequencer.cpp`](../../Tenhou.Client/src/settlement_sequencer.cpp)（`SettlementSequencer`），剥离 [`settlement_view.hpp`](../../Tenhou.Client/src/settlement_view.hpp) / [`settlement_view.cpp`](../../Tenhou.Client/src/settlement_view.cpp) 中的纯计时与声音状态机；保留隐藏父窗口下的 `WS_VISIBLE` / `host_visible` 兼容语义以支持离屏 `WM_PRINT`。

### 4. Stage 2 Step 2.3：在线与回放牌桌演出编排器（提交 `7ee743a`）
- 新增纯 C++ 模块：[`wall_geometry.hpp`](../../Tenhou.Client/src/wall_geometry.hpp) / [`wall_geometry.cpp`](../../Tenhou.Client/src/wall_geometry.cpp)、[`action_caption_state.hpp`](../../Tenhou.Client/src/action_caption_state.hpp)；将 [`action_selection_scene.hpp`](../../Tenhou.Client/src/action_selection_scene.hpp) / [`action_selection_scene.cpp`](../../Tenhou.Client/src/action_selection_scene.cpp) 与 [`replay_effects.hpp`](../../Tenhou.Client/src/replay_effects.hpp) / [`replay_effects.cpp`](../../Tenhou.Client/src/replay_effects.cpp) 解耦迁入 `tenhou_presenter`。
- 新增 [`table_sequencer.hpp`](../../Tenhou.Client/src/table_sequencer.hpp) / [`table_sequencer.cpp`](../../Tenhou.Client/src/table_sequencer.cpp)（`ReplayTableSequencer`、`OnlineTableSequencer`、`OnlineSequencerEffects`）。
- 将 [`table_view.hpp`](../../Tenhou.Client/src/table_view.hpp) / [`table_view.cpp`](../../Tenhou.Client/src/table_view.cpp) 与 [`lobby_view.hpp`](../../Tenhou.Client/src/lobby_view.hpp) / [`lobby_view.cpp`](../../Tenhou.Client/src/lobby_view.cpp) 收薄为 Win32 消息/定时器适配器；更新 [`presentation_check.cpp`](../../Tenhou.Client.Tests/src/presentation_check.cpp)、[`text_table_check.cpp`](../../Tenhou.Client.Tests/src/text_table_check.cpp) 并扩展无头 [`presenter_check.cpp`](../../Tenhou.Client.Tests/src/presenter_check.cpp)。

### 5. Stage 2 Step 2.4：统一牌桌帧数据与渲染后端接口（提交 `7ee743a`）
- 新增 [`table_render_frame.hpp`](../../Tenhou.Client/src/table_render_frame.hpp) / [`table_render_frame.cpp`](../../Tenhou.Client/src/table_render_frame.cpp)（`TableRenderFrame`、`TableActionButton` 数据模型、`BuildOnlineTableRenderFrame`、`BuildReplayTableRenderFrame`）。
- 新增 [`table_canvas_renderer.hpp`](../../Tenhou.Client/src/table_canvas_renderer.hpp)（`ITableCanvasRenderer`）及 [`gdi_table_renderer.hpp`](../../Tenhou.Client/src/gdi_table_renderer.hpp) / [`gdi_table_renderer.cpp`](../../Tenhou.Client/src/gdi_table_renderer.cpp)（`GdiTableRenderer`）。
- 重构 [`online_table.hpp`](../../Tenhou.Client/src/online_table.hpp) / [`online_table.cpp`](../../Tenhou.Client/src/online_table.cpp) 与 [`table_view.cpp`](../../Tenhou.Client/src/table_view.cpp) 共用 `ITableCanvasRenderer`，消除双份重复 GDI+ 牌桌绘制实现。

### 6. Stage 2 Step 2.5：大厅战绩与计时

- [离线大厅模型](../../Tenhou.Client/src/offline_lobby.cpp) 提供全期间与月间标题、固定精度与自适应小数、会籍有效期、对应 Rate、剪贴板文本及 `OfflineLobbyTiming`。视图提供本地时间、调用 Win32 定时器并应用焦点与可见性；服务大厅依服务器回执完成切厅。
- [无头检查](../../Tenhou.Client.Tests/src/offline_lobby_check.cpp) 覆盖格式、月份类别映射、导出内容、切厅与规则悬浮截止时间、认定揭示和关闭。

### 7. Stage 3.2-B Part 1：画布动作按钮

- [在线编排器](../../Tenhou.Client/src/table_sequencer.cpp) 用原有选项、条件和锚点生成视口矩形与按钮状态。牌桌帧包含 13 个动作及定缺/换三张按钮，以及下一盘、结算、返回大厅和 EXIT。鼠标悬浮、按压、焦点和键盘激活由 [LobbyView](../../Tenhou.Client/src/lobby_view.cpp) 处理。动作版本变化、禁用、失焦或失去捕获时取消按压。
- `RenderToDc` 直接输出画布上的按钮。Win32 子窗口继续承载候选牌组、入场、结算和退出投票弹窗。

### 8. Stage 3.2-B Part 2：Direct2D 牌桌

- [Direct2D 后端](../../Tenhou.Client/src/d2d_table_renderer.cpp) 默认用于在线牌桌和牌谱回放。D3D11 优先使用硬件，创建失败时尝试 WARP。窗口牌桌通过独立的子 HWND 和 flip swap chain 呈现，鼠标输入穿透回牌桌视图，壳层继续用 GDI 绘制。在暂停、截图或结算取底图时，使用 BGRA 离屏纹理、staging 读回和 DIB 输出。
- 牌面、背景、原版 HUD 图片、阴影和文本光栅按需上传。GPU 执行牌面颜色矩阵、飞牌透明混合、牌桌缩放与动作按钮合成。保留原版阴影掩码的 CPU 合并和逐像素位运算；私有字体继续由 `TextRenderer` 光栅化。旧式 Win32 按钮与系统字体标注复用原有画法，结果作为纹理参与合成。
- 设备移除、重置或 D2D 目标失效时重建设备和纹理；保留 `GdiTableRenderer` 供兼容和回归基准使用。

## 四、验收范围

- **静态取证**见[渲染迁移记录](RENDERING-MIGRATION.md)。原版常量、复用的重建端参数与项目新增按钮均标明来源。
- **自动化**见[客户端测试说明](../Tenhou.Client.Tests/README.md)。WARP 测试覆盖三玩法帧、六种非默认牌面着色、飞牌、阴影、TrueType、画布按钮、读回、swap chain 改尺寸与模拟设备重置。GDI 缓存与完整重绘仍作逐像素比较。四项目构建通过、CTest 8/8、Python 6/6，协议端到端及变体牌桌／结算离屏截图均通过。
- **重建端实窗**使用隔离配置载入 11 个事件的立直牌谱，检查跳到终点显示牌山、手牌和 HUD，以及回到起点时收起 DXGI 子窗口。高 DPI 缩放、动画透明边缘、在线拖牌与焦点仍需与原版使用同一局面实窗对照，硬件显卡上的性能收益仍待实测。
