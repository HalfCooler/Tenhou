# Tenhou.Client 解耦与 Direct2D 牌桌渲染迁移路线

本文档记录 `Tenhou.Client` 从强耦合 Win32 / GDI+ 架构向“纯 C++ 演出编排层（`tenhou_presenter`）+ 牌桌渲染接口（`ITableCanvasRenderer`）+ Direct2D 1.1 / D3D11 牌桌后端”演进的任务目标、强制约束边界、已完成项与后续待推进项，便于后续直接接续实施。

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

## 三、已完成项（截至 2026-11-25）

### 1. Stage 1：文档、测试与废弃原型清理（提交 `f2d6d58`）
- 移除失效脚本与废弃视图原型：`scripts/tests/test_text_catalog.py`、`scripts/tests/test_original_lobby_p0.py`、`__pycache__`、[variant_table_view](../../Tenhou.Client/CMakeLists.txt)（原 `variant_table_view.hpp/.cpp`）及不可达 `text_details_` 分支。
- 修复仓库文档断裂链接，新增 [Tenhou.Client 总览](README.md) 与 [RENDERING-MIGRATION.md](RENDERING-MIGRATION.md)。

### 2. Stage 2 Step 2.1：几何目录与手牌投影（提交 `f2d6d58`）
- 新增 [`tile_geometry.hpp`](../../Tenhou.Client/src/tile_geometry.hpp) / [`tile_geometry.cpp`](../../Tenhou.Client/src/tile_geometry.cpp)（`TileGeometryCatalog`，直接解析 `_obj` 索引头获取精灵矩形、`variant = 34` 基准矩形、偏移、深度排序及 1-bit Alpha 命中掩码）。
- 新增 [`hand_projection.hpp`](../../Tenhou.Client/src/hand_projection.hpp) / [`hand_projection.cpp`](../../Tenhou.Client/src/hand_projection.cpp)（`HandProjection`、`TableInteraction`、`HandColor` 分类、花色排序与插牌槽位重映射）。

### 3. Stage 2 Step 2.2：结算演出编排器（提交 `f2d6d58`）
- 新增 [`settlement_sequencer.hpp`](../../Tenhou.Client/src/settlement_sequencer.hpp) / [`settlement_sequencer.cpp`](../../Tenhou.Client/src/settlement_sequencer.cpp)（`SettlementSequencer`），剥离 [`settlement_view.hpp`](../../Tenhou.Client/src/settlement_view.hpp) / [`settlement_view.cpp`](../../Tenhou.Client/src/settlement_view.cpp) 中的纯计时与声音状态机；保留隐藏父窗口下的 `WS_VISIBLE` / `host_visible` 兼容语义以支持离屏 `WM_PRINT`。

### 4. Stage 2 Step 2.3：在线与回放牌桌演出编排器（当前工作区）
- 新增纯 C++ 模块：[`wall_geometry.hpp`](../../Tenhou.Client/src/wall_geometry.hpp) / [`wall_geometry.cpp`](../../Tenhou.Client/src/wall_geometry.cpp)、[`action_caption_state.hpp`](../../Tenhou.Client/src/action_caption_state.hpp)；将 [`action_selection_scene.hpp`](../../Tenhou.Client/src/action_selection_scene.hpp) / [`action_selection_scene.cpp`](../../Tenhou.Client/src/action_selection_scene.cpp) 与 [`replay_effects.hpp`](../../Tenhou.Client/src/replay_effects.hpp) / [`replay_effects.cpp`](../../Tenhou.Client/src/replay_effects.cpp) 解耦迁入 `tenhou_presenter`。
- 新增 [`table_sequencer.hpp`](../../Tenhou.Client/src/table_sequencer.hpp) / [`table_sequencer.cpp`](../../Tenhou.Client/src/table_sequencer.cpp)（`ReplayTableSequencer`、`OnlineTableSequencer`、`OnlineSequencerEffects`）。
- 将 [`table_view.hpp`](../../Tenhou.Client/src/table_view.hpp) / [`table_view.cpp`](../../Tenhou.Client/src/table_view.cpp) 与 [`lobby_view.hpp`](../../Tenhou.Client/src/lobby_view.hpp) / [`lobby_view.cpp`](../../Tenhou.Client/src/lobby_view.cpp) 收薄为 Win32 消息/定时器适配器；更新 [`presentation_check.cpp`](../../Tenhou.Client.Tests/src/presentation_check.cpp)、[`text_table_check.cpp`](../../Tenhou.Client.Tests/src/text_table_check.cpp) 并扩展无头 [`presenter_check.cpp`](../../Tenhou.Client.Tests/src/presenter_check.cpp)。

### 5. Stage 2 Step 2.4：统一牌桌帧数据与渲染后端接口（当前工作区）
- 新增 [`table_render_frame.hpp`](../../Tenhou.Client/src/table_render_frame.hpp) / [`table_render_frame.cpp`](../../Tenhou.Client/src/table_render_frame.cpp)（`TableRenderFrame`、`TableActionButton` 数据模型、`BuildOnlineTableRenderFrame`、`BuildReplayTableRenderFrame`）。
- 新增 [`table_canvas_renderer.hpp`](../../Tenhou.Client/src/table_canvas_renderer.hpp)（`ITableCanvasRenderer`）及 [`gdi_table_renderer.hpp`](../../Tenhou.Client/src/gdi_table_renderer.hpp) / [`gdi_table_renderer.cpp`](../../Tenhou.Client/src/gdi_table_renderer.cpp)（`GdiTableRenderer`）。
- 重构 [`online_table.hpp`](../../Tenhou.Client/src/online_table.hpp) / [`online_table.cpp`](../../Tenhou.Client/src/online_table.cpp) 与 [`table_view.cpp`](../../Tenhou.Client/src/table_view.cpp) 共用 `ITableCanvasRenderer`，消除双份重复 GDI+ 牌桌绘制实现。

### 6. 当前基线最近一次验证结果
- `Build-Tenhou.ps1`（`Tenhou.Client`、`Tenhou.Client.Tests`、`Tenhou.Server`、`Tenhou.Server.Tests`）：0 警告 0 错误。
- CTest 全套 8/8 通过（`client_presenter`、`client_offline_lobby`、`client_official_captures`、`client_text_composition`、`client_text_catalog`、`client_variants`、`client_presentation`、`client_resources`）。
- 服务端与原生客户端协议端到端联调（`Tenhou.Server.Tests.dll --native-client tenhou_protocol_check.exe`）及 `git diff --check` 全项通过。

---

## 四、待推进项（按顺序继续执行）

### 1. Stage 2 Step 2.5：下沉离线大厅格式化与计时状态
- **目标文件**：[`offline_lobby.hpp`](../../Tenhou.Client/src/offline_lobby.hpp)、[`offline_lobby.cpp`](../../Tenhou.Client/src/offline_lobby.cpp)、[`offline_lobby_view.cpp`](../../Tenhou.Client/src/offline_lobby_view.cpp)、[`offline_lobby_check.cpp`](../../Tenhou.Client.Tests/src/offline_lobby_check.cpp)。
- **实施要点**：
  - 将个人战绩与月度榜单格式化函数（`FormatFixedTrunc`、`FormatRate`、`FormatSignedAdaptive`、`FormatExpiry`、`RankGradeName`、`LifetimeProfileLabel`、`RankedMonthlyLabel`、`MonthlyProfileCaption`、`MonthlyCorrespondingRating`、`FormatProfileClipboardText`）以及切厅延迟 / 规则悬浮 / 认定弹窗动画计时逻辑下沉到 `offline_lobby.hpp/.cpp`（无 `<windows.h>` / GDI+ 依赖）。
  - 在 `offline_lobby_check.cpp` 补充无头断言，覆盖上述格式化精度与截断/自适应小数行为（如 `FUN_00434290` / `FUN_00434650` / `FUN_004346b0`）。

### 2. Stage 3.2-B Part 1：收拢牌桌动作按钮至画布自绘
- **目标文件**：[`table_render_frame.hpp`](../../Tenhou.Client/src/table_render_frame.hpp)、[`table_render_frame.cpp`](../../Tenhou.Client/src/table_render_frame.cpp)、[`table_sequencer.hpp`](../../Tenhou.Client/src/table_sequencer.hpp)、[`table_sequencer.cpp`](../../Tenhou.Client/src/table_sequencer.cpp)、[`lobby_view.hpp`](../../Tenhou.Client/src/lobby_view.hpp)、[`lobby_view.cpp`](../../Tenhou.Client/src/lobby_view.cpp)、[`gdi_table_renderer.cpp`](../../Tenhou.Client/src/gdi_table_renderer.cpp)。
- **实施要点**：
  - 将牌桌内全部动作与状态按钮——`5121..5128`（吃/碰/杠/立直/荣/自摸/流局/跳过）、`5151`（拔北）、`5170..5173`（万/筒/索/换三张确认）、`5129`（下一盘）、`5174`（结算）、`5137`（返回大厅）、`0xbc6`（EXIT）——统一纳入 `TableRenderFrame::buttons`（已在 `table_render_frame.hpp` 预定义 `TableActionButton` 与 `TableActionButtonStyle`）。
  - 移除 [`lobby_view.hpp`](../../Tenhou.Client/src/lobby_view.hpp) / [`lobby_view.cpp`](../../Tenhou.Client/src/lobby_view.cpp) 中上述 `controls_` 子 `HWND` 按钮（保留 `exit_vote_` 对话框、`match_intro_`、`action_view_` 与 `settlement_view_` 自身控件）；在画布指针悬停、按下（记录 `button_revision_` 并 `SetCapture`）、抬起及键盘激活路径直接命中 `buttons` 列表并派发 `Command(id)`；使 `RenderToDc` 直接将可见按钮绘制进牌桌画布，消除 `Capture(HDC)` 中的 `WM_PRINT` 子窗口循环。
  - **测试同步约束**：需同步更新 [`presentation_check.cpp`](../../Tenhou.Client.Tests/src/presentation_check.cpp) 第 319–374 行（原检查 `view.controls_.at(5173)` / `5170` 与 `WM_APP + 26`）与 [`text_table_check.cpp`](../../Tenhou.Client.Tests/src/text_table_check.cpp) `TextTableCheck::Buttons`（原构造 `DRAWITEMSTRUCT` 调用 `PaintAction`），改为通过公开或友元测试辅助查询画布按钮状态与调用 `ITableCanvasRenderer::RenderButtonToDc` 验证全部 13 个动作按钮 × 3 态视觉效果。

### 3. Stage 3.2-B Part 2：实装 Direct2D + D3D11 牌桌渲染后端
- **目标文件**：[`text_renderer.hpp`](../../Tenhou.Client/src/text_renderer.hpp)、[`text_renderer.cpp`](../../Tenhou.Client/src/text_renderer.cpp)、新增 `d2d_table_renderer.hpp/.cpp`、[`CMakeLists.txt`](../../Tenhou.Client/CMakeLists.txt)、[`online_table.hpp`](../../Tenhou.Client/src/online_table.hpp) / [`online_table.cpp`](../../Tenhou.Client/src/online_table.cpp)、[`table_view.cpp`](../../Tenhou.Client/src/table_view.cpp)、[`LobbyView`](../../Tenhou.Client/src/lobby_view.cpp)。
- **实施要点**：
  - 在 `TextRenderer` 暴露按 `TextRenderContext` + `SceneTextRun` 获取已光栅化预乘 BGRA 缓冲与设备像素包围盒的只读接口（沿用既有 `rasters_` LRU 缓存与完全一致的字形描边/阴影/渐变路径，零视觉偏差）。
  - 新增 `D2DTableRenderer : public ITableCanvasRenderer`，链接 `d3d11.lib`、`dxgi.lib`、`d2d1.lib`：
    - 创建 `ID3D11Device`（`D3D11_CREATE_DEVICE_BGRA_SUPPORT`，硬件优先，自动回退 `D3D_DRIVER_TYPE_WARP` 以兼容无硬件适配器或离屏测试环境）、`ID2D1Factory1`、`ID2D1Device` 与 `ID2D1DeviceContext`。
    - 缓存 `TableArtwork` 精灵、`DisplayArtwork` 原版 UI 贴图（`401/402/440/466/589/591/592` 等）、阴影图与 `TextRenderer` 文本光栅对应的 GPU `ID2D1Bitmap1` 纹理。
    - 使用内置 `CLSID_D2D1ColorMatrix` 实现 `HandColor` 7 态变色，使用 `DrawBitmap` 透明通道实现飞牌残影、文字淡入淡出与禁用遮罩，使用 `D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC` / `MULTI_SAMPLE_LINEAR` 替代 CPU `HALFTONE StretchBlt`。
    - 实现窗口路径（`IDXGISwapChain1` + `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` + 尺寸变化重建）与离屏兼容路径（`RenderToDc`：渲染至 D3D11 `BGRA8` 纹理后通过 `D3D11_USAGE_STAGING` 拷贝回目标 `HDC`，并在 `DXGI_ERROR_DEVICE_REMOVED` / `RESET` 时完整重建资源）。
  - 接入 `OnlineTable` 与 `TableView` 默认使用 `D2DTableRenderer`（按项目兼容需要评估保留 `GdiTableRenderer` 作参考对比或测试对照），并在 `Tenhou.Client.Tests` 补充 Direct2D 渲染自动化测试（覆盖三玩法、飞牌、阴影、TrueType 文字、画布按钮及设备重置恢复）。

### 4. 最终文档收口与全量验收
- 更新本文件、[README.md](README.md)、[RENDERING-MIGRATION.md](RENDERING-MIGRATION.md) 与 [`docs/Tenhou.Client.Tests/README.md`](../Tenhou.Client.Tests/README.md)，按 [AGENTS.md](../../AGENTS.md) 要求写明**静态取证、自动化检查与实窗对照各自的覆盖范围**。
- 在最终交付前运行全套构建（`Build-Tenhou.ps1`）、`ctest --output-on-failure`、`Tenhou.Server.Tests.dll --native-client tenhou_protocol_check.exe`、`tenhou_variant_ui_check.exe`、Python `unittest`、文档断链检查与 `git diff --check`。
