# Tenhou.Client 现代化路线与阶段交付记录

> 行为基准：`Tenhou.Client.Origin/tenhou/tenhou.exe`（Windows `130/0005`，SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）。  
> 约束原则：所有现代化改造严守**“零影响已有演出逻辑、时序与手感体验”**的红线（与 GDI+ → Direct2D 重构相同；仅 Phase C 明确授权的高清无损缩放作为特例，且经典资源包完整保留）。

---

## 四阶段总览

| 阶段 | 核心目标 | 当前状态 |
| --- | --- | --- |
| **Phase A** | 基础设施全面现代化：原生 x64、核心层去平台/架构类型（UTF-8 与 `<cstdint>`）、主窗口统一 `W` + 滚动日志、进程级 `SharedAssetStore`、音频迁移至 `miniaudio`（WASAPI）、网络栈独立 I/O 线程（`getaddrinfo` + `WSAPoll`）与纯 C++ `ini_store` | **已完成并验收** |
| **Phase B** | 渲染性能专项优化：建立 `tenhou_render_bench` 基准、消除主循环 1440×1080 Host DIB 往返、`ID2D1StrokeStyle` 焦点框、静态底图缓存、消除阴影 `O(N²)` 扫描与增量阴影上传、O(1) LRU 与结算背景缓存 | **已完成并验收** |
| **Phase C** | 高清化与自由缩放：基于 `lietxia/mahjong_graphic` 的构建期 SVG 烘焙管线、GPU 四点透视牌面模型、7 张残留 Sprites（`401/402/440/466/589/591/592`）重绘与**人工视觉验收门禁**、解除 `1440×1080` 固定窗口限制 | **已完成，待人工验视签收** |
| **Phase D** | 跨平台结构解耦：拆分大文件、提取声明式 `SettingsSchema`（移除 `104.bin`–`114.bin` 对话框依赖）与视图 ViewModel、清理最后 Win32 尾项（`RCDATA`/`bcrypt`/`ITextRasterizer`），再替换 UI 壳层 | 待开始 |

---

## Phase A 已确认的取证与实施细节

### 1. 原生 x64 构建切换
- 构建脚本与解决方案统一切换为 `-arch=x64 -host_arch=x64`（`scripts/Build.Common.ps1`、`scripts/Tenhou.Native.targets`、`Tenhou.slnx`、`Tenhou.Client/Tenhou.Client.vcxproj`、`Tenhou.Client.Tests/Tenhou.Client.Tests.vcxproj`、`Tenhou.Client/Build-Tenhou.Client.ps1`）。
- 产物验证：`build/Tenhou.Client/tenhou_rebuild.exe` PE Machine 字段为 `0x8664`（AMD64），全部 4 个项目（`Tenhou.Client`、`Tenhou.Client.Tests`、`Tenhou.Server`、`Tenhou.Server.Tests` 含 `tenhou_rule_oracle`）在 MSVC `/W4 /WX /utf-8 /EHsc` 下零警告编译通过。
- 原版浮点配色取证核对：`object_pack.cpp` 与 `background.cpp` 均保留 `double` 中间量以匹配原版 x87 计算精度，经 `client_resources` 与 `client_presentation` 在 x64 下验证无偏差。

### 2. 核心层去除平台与架构相关类型
- 新增跨平台基础头文件：
  - `Tenhou.Client/src/monotonic_clock.hpp`：基于 `std::chrono::steady_clock` 的 `MonotonicMs()`，替代 `GetTickCount64()`；`animation_clock.hpp` 同步移除 `<windows.h>` / `QueryPerformanceCounter`。
  - `Tenhou.Client/src/win32_utf.hpp`：Win32 边界唯一 UTF-8 ↔ UTF-16 转换与 `PathFromUtf8` / `PathToUtf8` 工具。
  - `Tenhou.Client/src/ini_store.hpp/.cpp`：纯 C++17 INI 读写模块，兼容 UTF-8、UTF-8 BOM 与历史 UTF-16 LE BOM 配置文件，ASCII 节/键大小写不敏感，保留未知行与注释，内存 mtime 缓存与临时文件原子替换。
- 核心数据结构与 API 全面切换为 UTF-8 `std::string` / `std::string_view`、`<cstdint>` 固定宽度整型及 `CanvasRect`：
  - `text_catalog.hpp.in` / `TextCatalog.cmake`、`ClientAssets.cmake` / `resource_manager.cpp`、`client_preferences`、`music_preferences`、`offline_lobby`、`network_protocol`、`network_client`、`replay`、`replay_archive`、`replay_audio`、`hand_projection`、`table_render_frame`、`action_caption_state`、`table_sequencer`、`tile_shadow`、`text_renderer`。

### 3. 主窗口统一 Unicode `W` 与滚动日志
- `application.cpp` / `main.cpp` 将 `RegisterClassExA` / `CreateWindowExA` / `GetWindowLongPtrA` / `DefWindowProcA` / `IsDialogMessageA` / `DispatchMessageA` / `GetModuleFileNameA` / `CreateMutexA` / `GetPropA` / `SetPropA` / `RemovePropA` / `CreateFontA` / `MessageBoxA` 统一迁移至 Unicode `W` 接口，移除 `PlaySoundA` 与 `winmm` 依赖。
- 启动日志更新为 `START x64; 本机麻将客户端 0.27.0`，增加 4 MiB 上限单份备份滚动（`rebuild.log` → `rebuild.log.1`），仅在关键事件/错误/退出时主动 flush。

### 4. 进程级 `SharedAssetStore`
- 新增 `Tenhou.Client/src/shared_assets.hpp/.cpp`，按规范路径 + 文件大小/mtime（或配置指纹）在进程内共享：
  - `ObjectPack`（`TableArtwork::Load` 复用于回放、在线牌桌、结算、开局介绍）
  - `ResourcePack`（`DisplayArtwork::Get` 复用于所有视图）
  - `TextFontService`（以 `std::weak_ptr` 共享 `HGSeikaishotaiPRO.TTF`，确保在 `GdiplusShutdown` 前自动析构；`Application::~Application` 亦显式 `Clear()`）
  - 首选背景 `Image`（LRU 上限 4 份，复用于欢迎页、离线大厅、回放与在线牌桌）
  - `ReplaySoundBank`（按包与覆盖项指纹缓存，跨 `TableView` / `LobbyView` / `SettingsView` 复用）

### 5. 音频后端迁移至内置 `miniaudio 0.11.21`（WASAPI）
- 音频资源取证：内置 `Tenhou.Client/assets/audio/_bgm0.wav` 与 `_bgm1.wav` 均为 **Microsoft ADPCM（`fmtTag = 0x0002`，44100 Hz，2ch，4-bit，blockAlign = 2048）**，原版通过 32 位 `msacm32`（ACM）解码，64 位下存在系统编解码器兼容隐患。
- 引入 `Tenhou.Client/vendor/miniaudio/miniaudio.h`（SHA-256 `6b2029714f8634c4d7c70cc042f45074e0565766113fc064f20cd27c986be9c9`）与 `miniaudio_impl.c`，彻底移除 `winmm` 与 `msacm32` 链接依赖；许可文件同步打包至 `licenses/miniaudio-LICENSE.txt`。
- 保留原版全部行为契约：
  - `DecodeSound` 维持 8/16-bit 8000..96000 Hz PCM WAV 严格校验与 SHA-256 记录；
  - `DecodeMusic` 对非 PCM 音频通过 `ma_decoder_init_memory` 直接解码（原生支持 MS-ADPCM / IMA-ADPCM / MP3 / FLAC / OGG）；
  - `BackgroundMusic` 使用单 `ma_device`（44.1 kHz 立体声 float32，Windows 默认 WASAPI），淡入/淡出包络步长严格维持原版 `00403f90` 的 `5e-6 * 88200 / (sr * ch)`，电平表改由回调原子峰值读取；
  - `ReplayAudio` 使用单 `ma_device` 4 声道软件 float32 混音，严格保持声道替换优先级（空闲 → `id == 104` 同类替换 → 最旧 serial）、136 条延迟队列、`id == 104` 25 ms 防突发间隔与零拷贝实时增益。

### 6. 网络栈独立 I/O 线程化（`getaddrinfo` + 非阻塞 socket + `WSAPoll`）
- `Tenhou.Client/src/network_client.cpp` 移除 `_WINSOCK_DEPRECATED_NO_WARNINGS`、`WSAAsyncGetHostByName`、`WSAAsyncSelect` 与 50 ms `WM_TIMER`。
- 每个连接周期由独立 `std::thread` 执行 `getaddrinfo`（IPv4/IPv6 兼容、优先 IPv4）、非阻塞 `connect`、`TCP_NODELAY` 与 `WSAPoll`（搭配环回 UDP 唤醒套接字实现微秒级发送/退出响应，50 ms 心跳/超时节拍）。
- 所有事件通过带 `generation` 代际编号的线程安全队列 + `PostMessageW` 投递回宿主线程执行 `LobbySession` 与观察者回调，彻底消除重连竞态，同时完整兼容现有 UI 与消息泵测试。
- 配置读写全面切换至 `ini_store`，修复 `Queue()` 中重复调用 `ParseNetworkMessage()` 的冗余开销。

---

## Phase A 验收范围

| 验收维度 | 覆盖内容 | 结果 |
| --- | --- | --- |
| **静态与构建检查** | MSVC C++17 `/W4 /WX /utf-8 /EHsc` 原生 x64 Release（全 4 项目 + `tenhou_rule_oracle`）与 Debug（`Tenhou.Client`）；PE Machine 校验（`0x8664`）；`miniaudio-LICENSE.txt` 打包校验；核心目录零 `winmm`/`msacm32`/`WSAAsync*`/`GetPrivateProfile*` 残留 | 全部通过 |
| **自动化测试** | `ctest` 8/8（`client_presenter`、`client_offline_lobby`、`client_official_captures`、`client_text_composition`、`client_text_catalog`、`client_variants`、`client_presentation`、`client_resources` 含 `SharedAssetStore` 与 `ini_store` UTF-16 LE BOM 回归）；`tenhou_resource_check --play-audio` 实机 WASAPI 播放；`Tenhou.Server.Tests` 含 `tenhou_protocol_check` 端到端协议测试；`scripts/tests` Python 单测 6/6 | 全部通过 |
| **实窗冒烟验证** | 独立临时 `--config-dir` 启动 `build/Tenhou.Client/tenhou_rebuild.exe --offline-lobby`，确认记录 `START x64`、自动加载并播放 `_bgm0.wav`（MS-ADPCM via miniaudio）、收到 `WM_CLOSE` 后写入 `config.ini` 并以 `EXIT code=0` 干净退出 | 通过 |

---

## Phase B 已确认的取证与实施细节

1. **量化渲染基准（`tenhou_render_bench`）**
   - 新增 [render_bench.cpp](../../Tenhou.Client.Tests/src/render_bench.cpp) 并接入 [Tests.cmake](../../Tenhou.Client.Tests/Tests.cmake)，同时为 [d2d_table_renderer.hpp](../../Tenhou.Client/src/d2d_table_renderer.hpp) 增加 `BenchStats`（上传次数/字节、HUD/交互/阴影重建次数、静态底图命中数）、`UsingHardware()` 与 `SetSyncInterval()` 接口。
   - 在固定牌谱夹具下完成优化前后 18 组（`GDI+` / `D2D-WARP` / `D2D-HW` × `1440×1080` / `2560×1440` × `static` / `hover_cd` / `flight`）完整对照采集，完整数据表见 [RENDERING-MIGRATION.md](RENDERING-MIGRATION.md#2026-11-25-phase-b-渲染基准与热路径优化实测)。
2. **消除主循环与交互层 1440×1080 DIB 往返**
   - [online_table.cpp](../../Tenhou.Client/src/online_table.cpp)、[lobby_view.cpp](../../Tenhou.Client/src/lobby_view.cpp)、[table_view.cpp](../../Tenhou.Client/src/table_view.cpp)：当 DXGI 牌桌窗口覆盖绘制区域时直接调用 `PaintToWindow` / `PaintTableToWindow`，绕过宿主 `PaintBuffer` 整窗 DIB 往返（失败时无损回退）。
   - [d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp)：4 个自动按钮悬停背景剥离出 `PrepareHud` 改由缓存 `ID2D1LinearGradientBrush` 在 D2D 原生绘制；`DrawInteraction` 拆分为独立的 `hint` 与 `countdown` 紧致包围盒 DIB 上传（`hover_cd` 120 帧上传量由 **972,000 KB → 103 KB**，降幅 **99.99%**）；按钮焦点框改为缓存 `ID2D1StrokeStyle` 单次 `DrawRectangle`。
3. **增量阴影合成与两级静态牌桌图层缓存**
   - [tile_shadow.cpp](../../Tenhou.Client/src/tile_shadow.cpp)：`BuildTileShadows` 将槽位与牌山顶层查询改为栈上扁平表 `O(N)`；`TileShadowLayer::Raster` 改为精确脏矩形增量合成（保留原版四字节比较与低位择取语义）；`D2DTableRenderer` 通过 `CopyFromMemory` 仅上传脏矩形（`flight` 120 帧上传量由 **85,050 KB → 408 KB**，降幅 **99.52%**）。
   - [d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp)：新增阴影帧指纹复用与 `O(N)` 指针索引、两级静态图层缓存（`static_board` 缓存背景+阴影+非自家手牌静态区，`base` 在无飞牌且全帧未变时直接复用），以及 2560×1440 宽屏仅绘制边栏条带与 DXGI waitable frame latency（`SetMaximumFrameLatency(1)`）+ `Present1`。
4. **全路径 O(1) LRU 与结算背景缓存**
   - [text_renderer.cpp](../../Tenhou.Client/src/text_renderer.cpp)（`layouts`/`rasters`）、[table_scene.cpp](../../Tenhou.Client/src/table_scene.cpp)（`TableArtwork::cache_`）、[d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp)（`tile_textures`/`sprites`/`glyphs`）全部切换为 `O(1)` 哈希 LRU。
   - [settlement_view.cpp](../../Tenhou.Client/src/settlement_view.cpp)：缓存底层未变暗的牌桌背景 DIB，在 `Configure`/`Show`/`Set`/`SetContext`/`SetTileColor`/`SetBackgroundPainter`/`WM_SIZE` 时失效或释放，避免结算动画与倒计时每帧触发 D3D11 Staging 回读。

### Phase B 验收范围

| 验收维度 | 覆盖内容 | 结果 |
| --- | --- | --- |
| **静态与构建检查** | MSVC C++17 `/W4 /WX /utf-8 /EHsc` x64 Release（`Tenhou.Client`、`Tenhou.Client.Tests`、`Tenhou.Server`、`Tenhou.Server.Tests`）与 Debug（`Tenhou.Client`）零警告通过；核对原版 `FUN_004828e0` / `00482630` 阴影分组与绘制 Z-order 不变 | 全部通过 |
| **自动化测试与基准** | `ctest` 8/8 全量通过（含 `client_presentation` 三规则 D2D/GDI+ 像素一致性、增量阴影与参考渲染零差异、设备重置恢复与 Flip 交换链缩放）；`tenhou_render_bench` 优化前后 18 组实机 HW+WARP 数据闭环（`D2D-HW` 全部场景 P95 ≤ 2.61 ms，P99 ≤ 3.35 ms）；`Tenhou.Server.Tests`（含 `tenhou_protocol_check`）与 `scripts/tests` 6/6 全量通过 | 全部通过 |
| **实窗冒烟验证** | 独立 `--config-dir` 启动 `build/Tenhou.Client/tenhou_rebuild.exe --offline-lobby` 与 `--replay`，确认 `TENHOU_CLIENT` 正常启动渲染，收到 `WM_CLOSE` 后写入 `config.ini` 并以 `EXIT code=0` 退出 | 通过 |

---

## Phase C 已确认的取证与实施细节

1. **构建期 SVG 烘焙管线（零运行时 SVG 解析，零外部依赖）**
   - 将 `lietxia/mahjong_graphic`（commit `3e275804ff58325306710bef3a7406860444bc6a`，自由商用许可）全部 44 张透明背景 SVG 母版（33 张标准牌面 + 3 张赤宝 + 8 张花牌；白板 `5z` 复用空白象牙白牌体）纳入 `Tenhou.Client/assets/sources/mahjong_graphic/`，同步更新 `README.md` 与 `THIRD-PARTY-NOTICES.md`。
   - 新增原生零依赖构建期工具 [svg_bake.cpp](../../Tenhou.Client/tools/svg_bake.cpp)（Windows 10+ Direct2D `ID2D1DeviceContext5` + 内置 `<style>` 类内联展开 + WIC PNG 编码），与一键重烘焙脚本 [Bake-HdAssets.ps1](../../scripts/Bake-HdAssets.ps1)，将 44 张牌面批量烘焙为 `3072×3072`（`8×6` 网格，单格 `384×512`）预乘/直通图集 `Tenhou.Client/assets/images/hd_tiles/faces.png`（约 910 KB），并通过 [catalog.cmake](../../Tenhou.Client/assets/catalog.cmake) 自动发布至 `resources/images/hd_tiles/faces.png`。
2. **原版 694 槽位透视反演与 GPU 四点透视单应（Homography）管线**
   - 通过 [extract_tile_quads.py](../../Tenhou.Client/tools/extract_tile_quads.py) 对原版 `_obj2.dat` / `_obj3.dat` 中全部 694 个朝上牌槽位（`variant == 34`）的 35 种牌面像素并集凸包拟合与一万红黑质心朝向标定，生成精确 4 顶点透视表 [hd_tile_quads.hpp](../../Tenhou.Client/src/hd_tile_quads.hpp)（`694/694` 全部一次通过，自动精确区分 4 家方位与横置立直/鸣牌 `n/20`, `n/60`, `k/40`）。
   - 新增 [hd_tile_shader.hlsl](../../Tenhou.Client/src/hd_tile_shader.hlsl) 与离线 `fxc.exe` 预编译字节码 [hd_tile_shader_bc.hpp](../../Tenhou.Client/src/hd_tile_shader_bc.hpp)（零 `d3dcompiler` 运行时依赖），以及 [hd_tile_pipeline.hpp](../../Tenhou.Client/src/hd_tile_pipeline.hpp) / [hd_tile_pipeline.cpp](../../Tenhou.Client/src/hd_tile_pipeline.cpp)：上传 44 层 `Texture2DArray` + 硬件 `GenerateMips` 完整 Mipmap 链 + 16× 各向异性过滤，由对角线交点解析齐次权重 `(q0..q3)` 实现数学精确无折痕四点透视单应投影。
   - [d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp) 保留 Classic 模式为默认（零像素偏差兼容全部既有回归）；开启 `HdArtwork` 后，`static_board` / `base`、牌张纹理与 HUD 路径文字均直接在目标视口原生像素分辨率合成，并支持 `D2D1_COMPOSITE_MODE_SOURCE_ATOP` 精确接收阴影与设备丢失恢复。
3. **7 张剩余 UI 精灵高清矢量重绘（人工视觉验收门禁）**
   - 核对 `_res2.dat` 原始尺寸、色值与轮廓后，通过 [generate_hd_sprites.py](../../Tenhou.Client/tools/generate_hd_sprites.py) 产出 7 张 SVG 源稿（`Tenhou.Client/assets/sources/sprites/{401,402,440,466,589,591,592}.svg`）并烘焙至 `Tenhou.Client/assets/images/sprites/hd/{id}.png`：`401/402` (`7×13 → 56×104`)、`440` (`140×51 → 560×204`)、`466` (`114×14 → 456×56`)、`589` (`244×172 → 976×688`)、`591` (`728×330 → 1456×660`)、`592` (`590×430 → 1180×860`)。
   - 接入 `DisplayArtwork::GetHd`、`D2DTableRenderer`、`WelcomeView`、`MatchmakingView` 与 `OfflineLobbyView`；Classic 模式继续保持 `_res2.dat` 原版路径不变，待**人工实窗逐张验视签收**后再决定是否提升为默认。
4. **解除固定 `1440×1080` 窗口限制与画质/全屏热切换**
   - 更新 [window_layout.hpp](../../Tenhou.Client/src/window_layout.hpp)、[application.hpp](../../Tenhou.Client/src/application.hpp)、[application.cpp](../../Tenhou.Client/src/application.cpp) 与 [table_view.cpp](../../Tenhou.Client/src/table_view.cpp) / [welcome_view.cpp](../../Tenhou.Client/src/welcome_view.cpp) / [offline_lobby_view.cpp](../../Tenhou.Client/src/offline_lobby_view.cpp)：
     - 恢复 `WS_THICKFRAME | WS_MAXIMIZEBOX`，`WM_GETMINMAXINFO` 仅保留最小客户区 `640×480`，解除最大尺寸锁，首次启动自动安全夹取至显示器工作区；
     - 窗口系统菜单提供常用尺寸预设（`1024×768` / `1280×960` / `1440×1080` / `1920×1440` / `2560×1440`）、无边框全屏（`Alt+Enter` / `F11`）与 Classic ↔ HD 热切换（`F6`），命令行新增 `--hd`、`--classic`、`--width`、`--height`，配置持久化至 `config.ini` `[REBUILD]`（`Width`/`Height`/`Maximized`/`Fullscreen`/`HdArtwork`）。

### Phase C 验收范围

| 验收维度 | 覆盖内容 | 结果 |
| --- | --- | --- |
| **静态与构建检查** | MSVC C++17 `/W4 /WX /utf-8 /EHsc` x64 Release（全 4 项目 + `tenhou_rule_oracle`）与 Debug（`Tenhou.Client`）零警告通过；694/694 原版透视槽位拟合零异常；`fxc.exe` 离线着色器预编译零运行时 `d3dcompiler` 依赖；44 张 SVG 与 7 张 HD 精灵打包至 `resources/images/` | 全部通过 |
| **自动化测试与基准** | `ctest` 8/8 全量通过（`client_presentation` 新增 `HdArtworkAndScalingCheck` 覆盖 44 切片映射、694 Quad 校验、7 张 HD 精灵尺寸校验、Classic 零回归、HD `1440×1080` / `2560×1440` 渲染与设备丢失恢复）；`tenhou_render_bench --hd` 硬件 GPU 2560×1440 全场景 P95 ≤ 2.84 ms / P99 ≤ 3.29 ms；`Tenhou.Server.Tests` 与 `scripts/tests` 6/6 全量通过 | 全部通过 |
| **实窗验证与人工验视** | 独立 `--config-dir` 实窗启动 `--offline-lobby --hd --width 1280 --height 960` 及 `--replay --classic/--hd --width 1600 --height 1000`，验证自由缩放客户区尺寸、标题 `[HD]`、配置持久化与 `EXIT code=0`，并导出 7 精灵并排对照板与实窗牌桌对比截图待用户人工验视签收 | 自动实窗通过；**待用户人工验视签收** |

---

## 待验证项（后续阶段）
- **Phase C 人工验视门禁**：7 张重绘精灵（`401, 402, 440, 466, 589, 591, 592`）与 HD 模式实窗牌面/缩放效果需由用户人工签收（签收后可按意愿将 `HdArtwork` 保持可选或设为默认）。
- **Phase D**：设置页 `SettingsSchema` 与原版 `104.bin`–`114.bin` 控件项逐项对照，以及跨平台 UI 壳层替换后的实窗行为核对。
