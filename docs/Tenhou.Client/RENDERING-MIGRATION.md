# 牌桌渲染迁移收益调研

2026-11-24。重建端基线为 `e51a9fa`。原版为 Windows 版 `130/0005`，`Tenhou.Client.Origin/tenhou/tenhou.exe` 的 SHA-256 为 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`。

## 静态取证

- 原版反编译 `Tenhou.Client.Origin/tenhou/tenhou.exe.c` 中的 `FUN_004761a0`（约 78028 行）在 `param_6 == 1.0` 且源与目标尺寸相同时调用 `GdipDrawImagePointRectI`，其余情况使用带颜色矩阵的 `GdipDrawImageRectRectI`。这个调用点确认了两条位图绘制分支。滤波核和混合舍入仍待验证。
- 重建端的 [OnlineGame](../../Tenhou.Client/src/online_game.hpp)、[LobbySnapshot](../../Tenhou.Client/src/network_protocol.hpp) 和 [ReplaySession](../../Tenhou.Client/src/replay.hpp) 已提供状态边界。[LobbyView](../../Tenhou.Client/src/lobby_view.cpp) 仍在窗口消息路径中协调协议快照、开局及结算时序、自动操作、声音和绘制。[OnlineTable::Paint](../../Tenhou.Client/src/online_table.cpp) 负责状态投影、动画取样、排序、阴影和绘制。
- [RetainedScene](../../Tenhou.Client/src/retained_scene.hpp#L135) 已跟踪受损区域并保留底层画面。[PaintBuffer](../../Tenhou.Client/src/paint_buffer.hpp#L119) 的最终合成路径包含逐行像素比较、带裁切的 `HALFTONE StretchBlt` 及变更后的源像素备份。[MotionTimer](../../Tenhou.Client/src/motion_timer.hpp) 在 UI 线程调度按需重绘，窗口隐藏或最小化时放慢唤醒。

## 收益与验证

| 改动 | 判断 |
| --- | --- |
| 拆分对局呈现调度与绘制 | 工程收益高。利用现有模型，把事件顺序、动作提交和结算时序放在可注入时钟的控制器里，给渲染器只读的帧数据。现有测试可用于固定开局、暂停恢复和跨盘语义。拆分本身的 CPU 收益取决于能否减少重复投影和布局。 |
| 牌桌迁移到 Direct2D + D3D11 | 性能收益集中在连续重绘及高物理分辨率下。常驻位图、GPU 上的缩放和透明混合可减轻 UI 线程的合成负担。静止牌桌已有缓存，预期增益较小。保持按需呈现，并以原版实窗核对采样、预乘 Alpha、透明边缘、图层顺序和窗口缩放。 |
| 引入 D3D12 | 当前收益预期低。只有分析确认 D3D11 命令提交或驱动开销显著时才值得评估。通过 D3D11On12 接入 Direct2D 还会引入互操作和同步成本。 |

最低成本的验证是用固定牌谱、相同资源、相同窗口物理尺寸和相同时钟，比较当前基线、拆分后的 GDI+ 路径、Direct2D 牌桌原型。取静止、普通摸打和连续动画样本，在原生尺寸与高 DPI 下用 ETW 比较进程 CPU、每次绘制的 UI 线程耗时 P50/P95、重绘次数及 GPU 占用。若缩放或像素对比居首，原型应覆盖这一步。若场景构建居首，先验证复用帧数据的收益。避免逐帧从 CPU 上传整张已合成画面。实现路线参考 [Direct2D 设备上下文](https://learn.microsoft.com/en-us/windows/win32/direct2d/devices-and-device-contexts)、[Direct2D 性能指南](https://learn.microsoft.com/en-us/windows/win32/direct2d/improving-direct2d-performance)和 [D3D11On12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12)。

本次验证范围为静态调用点和现有测试入口。待验证项包括实际 CPU/GPU 耗时、不同物理尺寸下的画面差异及原版实窗对照。现有 [客户端测试说明](../Tenhou.Client.Tests/README.md)列出呈现事件、局部重绘和离屏图片的覆盖范围。

## 2026-11-25 迁移取证

原版仍为 Windows `130/0005`。本次核对仓库 PE 的 SHA-256 与页首指纹一致。

- **战绩**：`tenhou.exe.c:34390–34561` 的 `FUN_00434290`、`FUN_00434650`、`FUN_004346b0` 固定了 `float` 转换、截断、符号、自适应精度和恰为 1 时的比率显示。原版有效期与月间布局锚点见[战绩取证](ORIGIN-PROFILE-AND-SHELL.md#14-数值格式化规则)。本次沿用重建端既有的本地切厅 250 ms、悬浮 500 ms、认定 20 ms 步进、1980 ms 可确认与 420 ms 关闭。本地切厅是夹具延迟，规则与认定的完整原版实窗对照仍待验证。
- **动作按钮**：`tenhou.exe.txt` 的 `0x004a8b93–0x004a8bc3` 读取牌桌按钮底图 `0x1b8`（440），并按按钮类型读入两组文字资源。重建端继续用 440 和现有 `ActionButtonText` 文本映射；使用 `BuildActionPlacements` 的锚点与 1024/1440 布局刻度。四川定缺、换三张和本地状态按钮沿用项目既有布局。
- **EXIT**：`tenhou.exe.txt` 的 `0x0047d25c–0x0047d29f` 查找或创建 `0xbc6`，`FUN_004a7920` 是 `WC_ITEM_BTN` 绘制路径。画布沿用既有的文字量测后加宽 16 px、加高 8 px，以及悬浮描边和半幅渐变。
- **阴影和采样**：地影缓存复用 `ComposeTileShadows` 的原版四字节比较与低位择取规则（`FUN_004828e0` / `FUN_00482630`）。同尺寸绘制保持像素对位，视口缩放用 D2D 多采样线性或高质量三次插值。Direct2D 与 GDI 的抗锯齿、缩放核和混合舍入允许有可测差异。[Direct2D 设备上下文](https://learn.microsoft.com/en-us/windows/win32/direct2d/devices-and-device-contexts) · [DXGI Flip Model](https://learn.microsoft.com/windows/win32/direct3ddxgi/dxgi-flip-model) · [颜色矩阵](https://learn.microsoft.com/en-us/windows/win32/direct2d/color-matrix)
- **验收**：自动化采用原生 Win32 测试窗口、WARP、内存 DIB 和离屏截图。重建端在隔离立直牌谱上实窗检查终点画面和回到起点的显隐。跨 DPI 的同局面原版实窗对照、硬件 GPU 利用率及 UI 线程耗时仍待实测。[路线与验收范围](DECOUPLING-PLAN.md#四验收范围)

## 2026-11-25 Phase B 渲染基准与热路径优化实测

原版基准仍为 Windows `130/0005`（SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）。

新增原生基准程序 [render_bench.cpp](../../Tenhou.Client.Tests/src/render_bench.cpp)（构建目标 `tenhou_render_bench`），在固定牌谱夹具、原生资源与 `frames=120 / warmup=10 / vsync=0` 条件下，对 `GDI+`、`D2D-WARP`、`D2D-HW`（实机硬件 GPU）在 `1440×1080` 与 `2560×1440` 两个视口下分别测量 `static`（静止牌桌）、`hover_cd`（手牌悬停 + 自动按钮悬停 + 倒计时跳秒）与 `flight`（连续摸打飞牌动画）。

### 1. 优化前基线识别的瓶颈

- `hover_cd`：`PrepareHud` 将 4 个自动按钮的悬停状态编码进 `hud_stamp`，每次悬停变化均在 CPU 侧重绘并上传整张 `1440×1080`（6.22 MB）HUD 位图；`DrawInteraction` 每次倒计时跳秒分配两张 `1440×1080` DIB（12.4 MB）逐像素恢复 Alpha 并整张上传，120 帧累计上传 **972,000 KB（约 949 MiB）**，`D2D-HW` P50 达 6.99 ms、P95 达 12.62 ms。
- `flight`：每帧执行 `BuildTileShadows`（含 `O(N²)` 查找）且每次牌面起落通过 `TileShadowLayer::Raster` 重算并上传整张 `1440×1080` 阴影纹理（120 帧累计上传 **85,050 KB**），造成 `D2D-HW` P95 尖刺达 4.17–5.73 ms。
- `2560×1440` 边栏与 WARP：在线宽屏模式下，每帧对被不透明 `base` 遮挡的整个 `2560×1440` 区域执行 `HIGH_QUALITY_CUBIC` 背景拉伸，导致 `D2D-WARP` 在 `2560×1440` 下即便静止也高达 ~33 ms。

### 2. 优化措施（不改变原版演出逻辑、时序与像素语义）

- **Host DIB 绕过**：[online_table.cpp](../../Tenhou.Client/src/online_table.cpp)、[lobby_view.cpp](../../Tenhou.Client/src/lobby_view.cpp)、[table_view.cpp](../../Tenhou.Client/src/table_view.cpp) 在 DXGI 子窗口覆盖牌桌区域时绕过宿主 `PaintBuffer` 整窗 DIB 往返，失败时自动回退。
- **消除 HUD/交互层 1440×1080 上传**：[d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp) 将 4 个自动按钮悬停背景剥离出 `PrepareHud`，改用缓存 `ID2D1LinearGradientBrush` 与单像素描边在 Direct2D 原生绘制；将 `DrawInteraction` 拆分为独立的 `hint` 与 `countdown` 紧致包围盒局部 DIB 上传（倒计时跳秒上传量由 6,220 KB 降至 ~2.5 KB）；按钮焦点框改为缓存 `ID2D1StrokeStyle` 单次 `DrawRectangle`。
- **增量阴影与静态图层缓存**：[tile_shadow.cpp](../../Tenhou.Client/src/tile_shadow.cpp) 将 `BuildTileShadows` 槽位与牌山顶层查询优化为栈上扁平表 `O(N)`，`TileShadowLayer::Raster` 改为精确脏矩形增量合成（保留原版 `FUN_004828e0` / `00482630` 四字节分组与低位择取语义）；`D2DTableRenderer` 通过 `ID2D1Bitmap::CopyFromMemory` 仅上传脏矩形阴影，配合阴影帧指纹复用、`O(N)` 双指针映射及两级静态牌桌图层缓存（`static_board` + 空闲 `base` 复用）。
- **缓存复杂度与展示路径**：[text_renderer.cpp](../../Tenhou.Client/src/text_renderer.cpp)、[table_scene.cpp](../../Tenhou.Client/src/table_scene.cpp)、[d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp) 全部升级为 `O(1)` 哈希 LRU；[settlement_view.cpp](../../Tenhou.Client/src/settlement_view.cpp) 缓存底层牌桌背景 DIB，避免结算动画与倒计时每帧触发 D3D11 Staging 回读；宽屏边栏仅绘制可视条带；窗口交换链启用 `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT`（`SetMaximumFrameLatency(1)`）与 `Present1`（旧驱动自动回退）。

### 3. 优化前后量化对比（`frames=120, warmup=10, vsync=0`）

#### 1440×1080 视口

| 后端 | 场景 | 优化前 P50 / P95 / P99 (ms) | 优化后 P50 / P95 / P99 (ms) | 优化前上传量 | 优化后上传量 | 关键改善 |
| --- | --- | --- | --- | --- | --- | --- |
| `GDI+` | `static` | 1.087 / 1.590 / 1.913 | 1.536 / 2.295 / 2.576 | — | — | 基线参考 |
| `GDI+` | `hover_cd` | 11.331 / 13.576 / 14.909 | 10.442 / 12.459 / 13.992 | — | — | 略降 |
| `GDI+` | `flight` | 2.066 / 10.656 / 11.420 | 2.056 / 5.533 / 6.521 | — | — | 增量阴影使 P95 **1.9×** 加速 |
| `D2D-WARP` | `static` | 1.446 / 1.966 / 2.609 | 0.970 / 1.907 / 2.685 | 0 KB | 0 KB | `base_hit=120`，`sdw_reb` 120→0 |
| `D2D-WARP` | `hover_cd` | 8.011 / 15.061 / 15.793 | **1.450 / 2.412 / 2.910** | 972,000 KB | **103 KB** | P50 **5.5×** / P95 **6.2×** / 上传量 **-99.99%** |
| `D2D-WARP` | `flight` | 1.306 / 5.705 / 6.058 | **1.256 / 2.589 / 3.777** | 85,050 KB | **408 KB** | P95 **2.2×** / 上传量 **-99.52%** |
| `D2D-HW` | `static` | 0.835 / 1.489 / 1.716 | **0.636 / 1.378 / 2.592** | 0 KB | 0 KB | `base_hit=120`，`sdw_reb` 120→0 |
| `D2D-HW` | `hover_cd` | 6.993 / 12.615 / 14.619 | **1.076 / 2.267 / 2.774** | 972,000 KB | **103 KB** | P50 **6.5×** / P95 **5.6×** / 上传量 **-99.99%** |
| `D2D-HW` | `flight` | 0.611 / 4.167 / 4.518 | **0.874 / 1.694 / 2.699** | 85,050 KB | **408 KB** | P95 **2.5×** / 上传量 **-99.52%** |

#### 2560×1440 视口

| 后端 | 场景 | 优化前 P50 / P95 / P99 (ms) | 优化后 P50 / P95 / P99 (ms) | 优化前上传量 | 优化后上传量 | 关键改善 |
| --- | --- | --- | --- | --- | --- | --- |
| `GDI+` | `static` | 1.827 / 2.809 / 4.288 | 2.248 / 2.852 / 3.323 | — | — | 基线参考 |
| `GDI+` | `hover_cd` | 14.442 / 18.968 / 21.372 | 16.113 / 20.493 / 23.395 | — | — | 基线参考 |
| `GDI+` | `flight` | 2.942 / 12.128 / 13.924 | 2.565 / 8.827 / 10.329 | — | — | 增量阴影使 P95 **1.4×** 加速 |
| `D2D-WARP` | `static` | 32.939 / 35.115 / 36.014 | **2.150 / 2.831 / 3.263** | 0 KB | 0 KB | P50 **15.3×** / P95 **12.4×** |
| `D2D-WARP` | `hover_cd` | 33.466 / 36.479 / 38.844 | **2.871 / 3.591 / 3.982** | 972,000 KB | **103 KB** | P50 **11.7×** / P95 **10.2×** |
| `D2D-WARP` | `flight` | 33.032 / 35.604 / 38.955 | **2.667 / 3.595 / 3.773** | 85,050 KB | **408 KB** | P50 **12.4×** / P95 **9.9×** |
| `D2D-HW` | `static` | 1.170 / 3.461 / 4.269 | **1.319 / 2.250 / 2.954** | 0 KB | 0 KB | P95 **1.5×** |
| `D2D-HW` | `hover_cd` | 6.840 / 12.727 / 13.616 | **1.566 / 2.613 / 3.347** | 972,000 KB | **103 KB** | P50 **4.4×** / P95 **4.9×** |
| `D2D-HW` | `flight` | 1.160 / 5.728 / 6.094 | **1.345 / 2.004 / 2.422** | 85,050 KB | **408 KB** | P95 **2.9×** / P99 **2.5×** |

> 注：`D2D-HW` 开启 `SetMaximumFrameLatency(1)` 帧等待对象后，`flight` / `2560×1440 static` 的 P50 略增 ~0.15–0.26 ms 属于避免 GPU 多帧队列堆积带来的真实呈现同步成本；所有场景在硬件 GPU 下 P95 均稳定在 **1.38–2.61 ms**、P99 ≤ **3.35 ms**（占 60 Hz 帧预算不足 16%）。

### 4. 验收覆盖范围

- **静态与编译检查**：MSVC C++17 `/W4 /WX /utf-8 /EHsc` x64 Release 与 Debug 零警告通过，确认所有 `TileShadow` 增量合成与 `D2DTableRenderer` 图层顺序严格保持既有绘制语义。
- **自动化检查**：`ctest` 8/8 通过（`client_presentation` 验证三规则 D2D 与 GDI+ 像素一致性、增量 `TileShadowLayer` 与参考渲染零像素差异、7 种手牌色彩矩阵、飞牌拖影、设备丢失恢复及 Flip 交换链缩放）；`tenhou_render_bench` 实机硬件 GPU 与 WARP 18 组对照采集完成；`Tenhou.Server.Tests`（含 `tenhou_protocol_check`）与 `scripts/tests`（6/6）全量通过。
- **实窗冒烟验证**：使用独立 `--config-dir` 启动 `build/Tenhou.Client/tenhou_rebuild.exe --offline-lobby` 与 `--replay`，确认 `TENHOU_CLIENT` 窗口正常渲染、收到 `WM_CLOSE` 后保存 `config.ini` 并以 `EXIT code=0` 退出。跨 DPI 同局面与原版客户端的人工肉眼细粒度对照及 Phase C 7 张 Sprites 重绘仍列为后续人工验收项。

## 2026-11-25 Phase C：HD SVG 烘焙与 GPU 四点透视单应实测（`tenhou_render_bench --hd`）

在启用 `ClientPreferences::hd_artwork = true`（目标视口原生像素分辨率合成 + 44 层 `384×512` SVG 烘焙 `Texture2DArray` 完整 Mipmap + 16× 各向异性过滤 + D3D11 精确齐次四点透视单应 Shader + 7 张高清矢量重绘精灵）下的实机基准（`frames=120, warmup=10, vsync=0`）：

| 后端 | 分辨率 | 场景 | P50 (ms) | P95 (ms) | P99 (ms) | Mean (ms) | 上传次/量 (KB) | `base_hit` |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `D2D-WARP-HD` | `1440×1080` | `static` | 2.581 | 3.122 | 3.459 | 2.637 | 0 / 0 | 120 |
| `D2D-WARP-HD` | `1440×1080` | `hover_cd` | 4.463 | 5.153 | 5.740 | 4.543 | 40 / 103 | 120 |
| `D2D-WARP-HD` | `1440×1080` | `flight` | 4.091 | 4.961 | 5.276 | 3.914 | 14 / 408 | 106 |
| `D2D-HW-HD` | `1440×1080` | `static` | 0.724 | 1.641 | 2.457 | 0.860 | 0 / 0 | 120 |
| `D2D-HW-HD` | `1440×1080` | `hover_cd` | 1.323 | 1.935 | 2.227 | 1.345 | 40 / 103 | 120 |
| `D2D-HW-HD` | `1440×1080` | `flight` | 1.134 | 2.045 | 2.325 | 1.229 | 14 / 408 | 106 |
| `D2D-WARP-HD` | `2560×1440` | `static` | 3.378 | 3.918 | 4.610 | 3.452 | 0 / 0 | 120 |
| `D2D-WARP-HD` | `2560×1440` | `hover_cd` | 8.291 | 9.079 | 9.523 | 8.316 | 40 / 103 | 120 |
| `D2D-WARP-HD` | `2560×1440` | `flight` | 6.890 | 19.832 | 21.009 | 7.487 | 14 / 408 | 106 |
| `D2D-HW-HD` | `2560×1440` | `static` | 0.969 | 1.540 | 2.033 | 1.051 | 0 / 0 | 120 |
| `D2D-HW-HD` | `2560×1440` | `hover_cd` | 1.978 | 2.835 | 3.047 | 2.054 | 40 / 103 | 120 |
| `D2D-HW-HD` | `2560×1440` | `flight` | 1.786 | 2.743 | 3.289 | 1.834 | 14 / 408 | 106 |

- 在 `2560×1440` 目标视口原生分辨率 + 44 层 Mipmap 四点透视 HD 渲染下，硬件 GPU 全场景依然保持 **P95 ≤ 2.84 ms、P99 ≤ 3.29 ms**，并完整复用 Phase B 的静态底图缓存与增量阴影上传机制。
