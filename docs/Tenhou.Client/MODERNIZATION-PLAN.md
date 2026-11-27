# Tenhou.Client 现代化路线与阶段交付记录

> 行为基准：`Tenhou.Client.Origin/tenhou/tenhou.exe`（Windows `130/0005`，SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）。  
> 约束原则：所有现代化改造严守**“零影响已有演出逻辑、时序与手感体验”**的红线（与 GDI+ → Direct2D 重构相同；仅 Phase C 明确授权的高清无损缩放作为特例，且经典资源包完整保留）。

---

## 四阶段总览

| 阶段 | 核心目标 | 当前状态 |
| --- | --- | --- |
| **Phase A** | 基础设施全面现代化：原生 x64、核心层去平台/架构类型（UTF-8 与 `<cstdint>`）、主窗口统一 `W` + 滚动日志、进程级 `SharedAssetStore`、音频迁移至 `miniaudio`（WASAPI）、网络栈独立 I/O 线程（`getaddrinfo` + `WSAPoll`）与纯 C++ `ini_store` | **已完成并验收** |
| **Phase B** | 渲染性能专项优化：建立 `tenhou_render_bench` 基准、消除主循环 1440×1080 Host DIB 往返、`ID2D1StrokeStyle` 焦点框、静态底图缓存、消除阴影 `O(N²)` 扫描与增量阴影上传、O(1) LRU 与结算背景缓存 | **已完成并验收** |
| **Phase C** | 4 档原生分辨率材质与原版分辨率选择恢复：回滚 SVG 烘焙牌面与 7 张重绘精灵至原版视觉风格；接入原版 `1024×768` 与 C-EGG 授权 CDN `1920×1440` / `2640×1980` 原生 3D 透视牌面及配套阴影包；恢复原版设置页 `画面`（Dialog `114`）与 `[STATUS] SCREENSIZE` / `FULLSCREENSIZE` 分辨率选择逻辑，并保留自由窗口缩放与无边框全屏 | **已完成并验收** |
| **Phase D** | 跨平台结构解耦：拆分大文件、提取声明式 `SettingsSchema` 与 4 个纯 C++ ViewModel、清理核心层最后 Win32 尾项（`RCDATA`/`bcrypt`/`ITextRasterizer`），为最终跨平台 UI 壳层替换（D.3）铺平道路 | **D.1–D.2 已完成，D.3 待选型** |

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

## Phase C 已确认的取证与实施细节（2026-11-26 方案切换为 4 档原生材质）

1. **回滚 SVG 烘焙与 7 张重绘精灵**
   - 经视觉对比确认 SVG 重绘牌面与原版 3D 透视预渲染质感存在明显差异，按用户决策完整回滚构建期 SVG 烘焙管线与 `HdArtwork` 路径（移除 `hd_tile_*`、`svg_bake.cpp`、`extract_tile_quads.py`、`generate_hd_sprites.py`、`scripts/Bake-HdAssets.ps1`、`assets/images/hd_tiles/`、`assets/images/sprites/hd/`、`assets/sources/sprites/` 及非花牌 SVG，保留 `mahjong_graphic` 8 张花牌与许可）。
   - 保留 Phase A/B 全部基础设施与 Direct2D 渲染优化、自由窗口缩放、系统菜单尺寸预设与无边框全屏（`Alt+Enter` / `F11`）。
2. **4 档原生 3D 透视牌面与阴影资源接入（C-EGG 授权打包）**
   - 静态二进制核对确认：`tenhou.net/4/`（Desktop4K 版）通过 `wss://b-wk.mjv.jp` 下发 `.ini` 清单并从 `https://cdn.tenhou.net/4/res/` 分发与 Windows 原版完全同构的 `obj0`（25,928 条记录，头长 `0x19564`）、`obj1`（944 条记录，头长 `0xf10`）和 `sdw0`（1,504 个唯一阴影蒙版，头长 `0x18ac`）资源包；其中 `9/_obj0_{W}x{H}_{8hex}.dat` 分块按 `.ini` 顺序字节拼接即为标准连续 `obj0` 包。
   - 全部 4 档资源纳入 `Tenhou.Client/assets/catalog.cmake` 作为 `REQUIRED` 发布资产：
     - **`1024×768`**：`_obj0.dat`（20,236,526 B，源自原版 `130/0005`）、`_obj1.dat`（1,724,607 B，源自原版 `130/0005`）、`_sdw0_1024x768.dat`（864,152 B，SHA-256 前缀 `96c937f14cf579a4`）。
     - **`1440×1080`**：`_obj2.dat`（30,210,614 B）、`_obj3.dat`（2,581,907 B）、内嵌 `shadow-hd.dat`（`sdw2`，915,664 B）。
     - **`1920×1440`**：`_obj0_1920x1440.dat`（43,583,771 B，SHA-256 前缀 `f86709d7a835751f`）、`_obj1_1920x1440.dat`（3,624,779 B，SHA-256 前缀 `f6ff7e96174b2ce2`）、`_sdw0_1920x1440.dat`（1,757,510 B，SHA-256 前缀 `eb63f151a7d4784d`）。
     - **`2640×1980`**：`_obj0_2640x1980.dat`（66,751,696 B，SHA-256 前缀 `c329ec6247a2d953`）、`_obj1_2640x1980.dat`（5,300,074 B，SHA-256 前缀 `2e7411cb53331c39`）、`_sdw0_2640x1980.dat`（2,553,534 B，SHA-256 前缀 `24b42681dfe04593`）。
   - 渲染与几何扩展：`TableArtwork`、`TileGeometryCatalog`、`ShadowPack`/`TileShadowLayer`、`DisplayArtwork`、`D2DTableRenderer`、`GdiTableRenderer`、`SettlementScene`/`SettlementView`、`MatchIntroView` 与 `ActionSelectionScene` 全面支持 4 档画布宽度；`1024×768` 与 `1440×1080` 保持原版字面偏移表与像素对齐，`1920×1440` / `2640×1980` 沿 `1440` HD 设计空间等比映射，所有文字通过 `FinalScene` 直接在目标视口像素上矢量化绘制。
3. **原版 `画面`（Dialog `114`）与 `[STATUS] SCREENSIZE` 逻辑恢复**
   - 原版取证（`Tenhou.Client.Origin/tenhou/tenhou.exe.c:20358-20372`、`24382-24482`、`61519-61548`、`70140`，`tenhou.exe` SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）：
     - 设置属性页顺序为 `104(牌), 105(卓), 106(SE), 109(BGM), 110(牌譜), 114(画面), 107(その他)`；
     - `Dialog 114`（`assets/dialogs/114.bin`）包含 `0x426`（`画面サイズ`：`自動` / `1024 x 768` / `1440 x 1080`，重建端按授权扩展增加 `1920 x 1440` / `2640 x 1980`）与 `0x429`（`フルスクリーン`：`自動` + 显示器 32bpp 枚举模式）；
     - 持久化键遵循原版 `[STATUS] SCREENSIZE`（`default` / `1024x768` / `1440x1080` / `1920x1440` / `2640x1980`）与 `[STATUS] FULLSCREENSIZE`（`W,H`）。
   - 项目差异（用户明确授权）：`default`（自动）按当前客户区 4:3 适配宽度在 4 档中自动切档（`≤1024 → 1024`、`≤1440 → 1440`、`≤1920 → 1920`、`>1920 → 2640`），修改 `0x426` 立即热切换材质档位与窗口尺寸，无需强制重启应用。
4. **6 张非文字实用精灵高清替换与逻辑尺寸解耦（2026-11-26 增补）**
   - 原版取证（`_res2.dat` 7 张保留非文字精灵）：`401`/`402`（`7×13`）、`440`（`140×51`）、`466`（`114×14`，棒身 `x=1..113, y=2..13` 近直角、红点 `cx=56, cy=7` 受顶光内凹着色）、`589`（`244×172`，3D 透视梯形外黑框 + 更大圆角内盘形成四角新月斜面倒角与银灰反光弧，顶边 2 px / 侧边 3.15 px / 底边 4.2 px）、`591`（`728×330`）、`592`（`590×430`）。
   - 按用户明确授权定案落盘至 `Tenhou.Client/assets/images/sprites/`：
     - `401.png` / `402.png`（`56×104`，8× 矢量烘焙，2,550 B / 2,428 B）；
     - `440` 不外置，继续走 `_res2.dat` 原版位图运行时缩放；
     - `466.png`（`456×56`，4× 8×超采样重建，2,920 B，四角收紧至 `r=0.30 px` + 凹面漆光红点，1× 回缩绿底 `MAE=1.78 / P95=4.0`）；
     - `589.png`（`976×688`，4× 8×超采样透视拟合重建，35,948 B，还原四角新月银灰倒角并修补原版四角透明漏底，1× 回缩绿底 `MAE=2.72 / P95=9.2`）；
     - `591.png` / `592.png`（`2912×1320` / `2360×1720`，4× 锐化重建回纹边框，50,195 B / 51,021 B）。
   - 渲染与硬编码解除：[original_display.hpp](../../Tenhou.Client/src/original_display.hpp) / [original_display.cpp](../../Tenhou.Client/src/original_display.cpp) 新增 `OriginalDisplaySpriteSize`、`DisplayArtwork::LogicalSize` 与分离式预乘 Alpha Area/Box 预缩放（按当前画布档位 `1024/1440/1920/2640` 精确下采样，不放大 1× 图），同步解除 [d2d_table_renderer.cpp](../../Tenhou.Client/src/d2d_table_renderer.cpp)、[welcome_view.cpp](../../Tenhou.Client/src/welcome_view.cpp)、[matchmaking_view.cpp](../../Tenhou.Client/src/matchmaking_view.cpp)、[offline_lobby_view.cpp](../../Tenhou.Client/src/offline_lobby_view.cpp) 与 [settlement_view.cpp](../../Tenhou.Client/src/settlement_view.cpp) 的像素尺寸硬编码假设。

### Phase C 验收范围

| 验收维度 | 覆盖内容 | 结果 |
| --- | --- | --- |
| **静态取证与构建检查** | 核对原版 `tenhou.exe` `130/0005` `RT_DIALOG 114`、`FUN_0041d...` 与 `70140` 阴影签名逻辑；二进制校验全部 4 档 `obj0/obj1/sdw0` 记录数、连续布局与 zlib 解码；MSVC C++17 `/W4 /WX /utf-8 /EHsc` x64 Release 零警告通过 | 全部通过 |
| **自动化测试与基准** | `ctest` 8/8 全量通过（`client_presentation` 新增 `MultiResolutionArtworkAndScalingCheck` 覆盖 4 档 `SCREENSIZE` 解析/自动选档、4 档原生渲染与阴影包、4 档 `RiichiFlights` 与 `2640×1980` 设备丢失恢复；`client_resources` 逐字节校验全部 4 档发布资产）；`tenhou_render_bench` 硬件 GPU 实测 `1920` 档 `2560×1440` P95 ≤ 2.33 ms、`2640` 档 `2560×1440` P95 ≤ 2.84 ms | 全部通过 |
| **实窗冒烟验证** | 独立 `--config-dir` 实窗验证 `SCREENSIZE=1024x768`、`SCREENSIZE=default`（`1920×1440` 自动选档）与 `SCREENSIZE=2640x1980` 三个场景，确认窗口标题、日志、`config.ini` 持久化与 `EXIT code=0` 正常退出；高分屏下各档位人工肉眼对照与设置页交互细检列为后续人工验收项 | 自动实窗通过 |

---

## Phase D（D.1 + D.2）已确认的取证与实施细节

> 本阶段在保持 100% 演出逻辑、时序、INI 配置键位及 4 档原生分辨率行为不变的前提下，完成全部结构解耦与核心层 Win32 尾项清理；最终 UI 壳层替换（Phase D.3）待用户确认跨平台框架选型后推进。

### D.1 大文件拆分、声明式设置 Schema 与纯 C++ ViewModel
1. **大文件职责拆分与具名枚举**
   - [table_sequencer.cpp](../../Tenhou.Client/src/table_sequencer.hpp) 拆分为独立的 [replay_sequencer.cpp](../../Tenhou.Client/src/replay_sequencer.cpp)（回放编排）与 [online_sequencer.cpp](../../Tenhou.Client/src/online_sequencer.cpp)（在线牌局编排）。
   - [network_protocol.cpp](../../Tenhou.Client/src/network_protocol.cpp) 拆分为 [network_protocol_internal.hpp](../../Tenhou.Client/src/network_protocol_internal.hpp)、[network_protocol_wire.cpp](../../Tenhou.Client/src/network_protocol_wire.cpp)（XML 解析/组帧/认证/服务端错误与 JSON 工具）、[network_protocol.cpp](../../Tenhou.Client/src/network_protocol.cpp)（`LobbySession` 核心会话状态机）与 [network_protocol_lobby.cpp](../../Tenhou.Client/src/network_protocol_lobby.cpp)（官方/原版大厅画像、统计与提交）。
   - [lobby_view.cpp](../../Tenhou.Client/src/lobby_view.cpp) 将全部裸数字定时器（`2..12`）与 `WM_APP + 16/17/18/25` 消息字面量替换为强语义具名枚举 `LobbyTimerId` 与 `LobbyAppMessage`。
2. **声明式 `SettingsSchema` 与纯 C++ `SettingsStore`**
   - 新增 [settings_schema.hpp](../../Tenhou.Client/src/settings_schema.hpp) / [settings_schema.cpp](../../Tenhou.Client/src/settings_schema.cpp)，完整声明全部 7 个原版设置页（`104 牌`、`105 卓`、`106 SE`、`109 BGM`、`110 牌谱`、`114 画面`、`107 其他`）的字段范围、INI 节/键、`FLAGS0` 位掩码与反转语义，以及牌背/桌布/画面尺寸/音效模式预设集合；提供零 Win32 依赖的无头读写器 `SettingsStore`。
   - [settings_view.cpp](../../Tenhou.Client/src/settings_view.cpp) 已将 `画面`（`InitScreen`/`ScreenCommand`）与 `牌/卓` 预设及持久化接入 `SettingsSchema` 与 `SettingsStore`；将 `client_preferences.cpp` 上移至 `tenhou_resources` 静态库以便无头复用。
3. **4 个零 Win32/HWND 依赖的纯 C++ ViewModel**
   - [welcome_view_model.hpp](../../Tenhou.Client/src/welcome_view_model.hpp) / [.cpp](../../Tenhou.Client/src/welcome_view_model.cpp)：登录偏好加载/保存、密码永不落盘、隐藏 ID 掩码（`ID########-########`）保真、19 位官方 ID 与端口范围校验及 `ConnectionOptions` 构造；已接入 [welcome_view.cpp](../../Tenhou.Client/src/welcome_view.cpp)。
   - [room_settings_view_model.hpp](../../Tenhou.Client/src/room_settings_view_model.hpp) / [.cpp](../../Tenhou.Client/src/room_settings_view_model.cpp)：规则集过滤（`riichi-v1`/`mahjong-v1`）、8 项分页、选项与数值步长校验及切规则清理旧键；已完整接入 [room_settings_view.cpp](../../Tenhou.Client/src/room_settings_view.cpp)。
   - [matchmaking_view_model.hpp](../../Tenhou.Client/src/matchmaking_view_model.hpp) / [.cpp](../../Tenhou.Client/src/matchmaking_view_model.cpp)：人机队列选择、1000–9999 房号校验、`Lxxxx` 房主/成员确认、按钮可见/可用态与服务端错误本地化；已接入 [matchmaking_view.cpp](../../Tenhou.Client/src/matchmaking_view.cpp)。
   - [offline_lobby_view_model.hpp](../../Tenhou.Client/src/offline_lobby_view_model.hpp) / [.cpp](../../Tenhou.Client/src/offline_lobby_view_model.cpp)：离线大厅 250 ms 移动节拍、服务大厅快照同步与命令构造、预约切换与大会室报名/入场/取消状态机。
   - `matchmaking.cpp`、`settings_schema.cpp` 与 4 个 ViewModel 统一归入零 GDI+/D3D11/Win32 UI 依赖的 `tenhou_presenter` 目标。

### D.2 核心层最后 Win32 尾项清理
1. **移除 `bcrypt.dll` 依赖（纯 C++ FIPS 180-4 SHA-256）**
   - [resource_pack.cpp](../../Tenhou.Client/src/resource_pack.cpp) 移除 `<windows.h>` / `<bcrypt.h>` 与 `BCrypt*` 调用，替换为无外部依赖的标准 C++17 SHA-256 实现，并通过 NIST FIPS 180-4 测试向量校验；[CMakeLists.txt](../../Tenhou.Client/CMakeLists.txt) 移除 `tenhou_resources` 的 `bcrypt` 链接项。
   - 静态 PE 导入表校验确认最终 `build/Tenhou.Client/tenhou_rebuild.exe` 已零 `bcrypt.dll` 导入。
2. **移除 Win32 `RCDATA`（构建期静态字节数组生成）**
   - 新增纯 CMake 生成器 [EmbedAssets.cmake](../../Tenhou.Client/cmake/EmbedAssets.cmake)，修改 [ClientAssets.cmake](../../Tenhou.Client/cmake/ClientAssets.cmake) 与 [resource_manager.cpp](../../Tenhou.Client/src/resource_manager.cpp)，将全部 9 份 `EMBEDDED` 资产（`Dialog104`–`Dialog114` 与 `TileShadows`）生成为 `alignas(16) constexpr std::uint8_t` 静态只读数组（`embedded_assets.cpp`），彻底移除 `client_assets.rc` 与 `FindResourceW`/`LoadResource`/`LockResource`。
   - **实窗取证记录（`COMCTL32.dll` `5.82.26100.8328`）**：实窗打开设置页时发现经典 `PropertySheetW`（`PSP_DLGINDIRECT`）会在原地修改 `DLGTEMPLATE::style`，直接传入 `.rdata` 只读数组会触发 `0xC0000005`。已在 [settings_view.cpp](../../Tenhou.Client/src/settings_view.cpp) 中于 `SettingsView::Open()` 将 7 份模板字节拷贝至实例成员可写缓冲（`page_templates_`）后再传入 `pResource`，既保持 `.rdata` 只读零启动拷贝，又彻底兼容 `COMCTL32.dll`。
3. **`tenhou_text` 平台无关接口 `ITextRasterizer`**
   - 新增零 `<windows.h>` / `<gdiplus.h>` 依赖的 [text_rasterizer.hpp](../../Tenhou.Client/src/text_rasterizer.hpp)（`TextPointF`/`TextRectF`/`TextRectI`/`TextRasterPixels`/`TextCacheStats`/`PortableTextLayout`/`PortableTextContext`/`PortableTextRun`/`PortableTextRaster` 与纯虚接口 `ITextRasterizer`）。
   - Windows 端 [text_renderer.hpp](../../Tenhou.Client/src/text_renderer.hpp) / [text_renderer.cpp](../../Tenhou.Client/src/text_renderer.cpp) 继承并实现 `ITextRasterizer`，保留 GDI+ 精确像素参考实现，为后续 FreeType + HarfBuzz 后端提供可直接替换的契约。

### Phase D（D.1–D.2）验收范围

| 验收维度 | 覆盖内容 | 结果 |
| --- | --- | --- |
| **静态与构建检查** | MSVC C++17 `/W4 /WX /utf-8 /EHsc` x64 Release（全 4 项目）与 Debug（`Tenhou.Client`）零警告通过；PE x64 导入表校验确认零 `bcrypt.dll` 导入；源码审计确认 `tenhou_resources` 零 `FindResourceW`/`LoadResource`/`LockResource`/`BCrypt*` 残留；`text_rasterizer.hpp`、`settings_schema.hpp` 与 4 个 ViewModel 头文件零 `<windows.h>` 依赖 | 全部通过 |
| **自动化测试** | `ctest` **9/9** 全量通过，含新增无头套件 `client_phase_d`（[phase_d_check.cpp](../../Tenhou.Client.Tests/src/phase_d_check.cpp)：FIPS 180-4 SHA-256 向量、全部 9 份静态内嵌资产与磁盘逐字节 SHA-256 一致性、跨平台 Mock `ITextRasterizer` 多态契约、`SettingsSchema` 7 页顺序与 `SettingsStore` 全键位/位掩码持久化、4 个纯 C++ ViewModel 状态机与校验）；`Tenhou.Server.Tests`（含 `tenhou_protocol_check`）全部通过；`scripts/tests` 6/6 通过 | 全部通过 |
| **实窗冒烟验证** | 独立 `--config-dir` 启动 `build/Tenhou.Client/tenhou_rebuild.exe --offline-lobby` 并通过 `WM_COMMAND 4023` 实窗打开/关闭设置属性页（验证从纯 C++ 静态数组加载 `104..114` 模板及 `COMCTL32` 可写缓冲修复），再启动 `--replay` 实窗验证内嵌 `TileShadows` 加载与 D2D 渲染；两次均无 `ERROR` 日志并以 `EXIT code=0` 正常退出 | 全部通过 |

---

## 待验证项（后续阶段）
- **4 档实窗细检**：在 2K/4K 实机上人工打开设置页 `画面` 切档，核对 `1024 / 1440 / 1920 / 2640` 下的牌局、立直飞牌、鸣牌偏移、结算界面与全屏切换体验，以及未来对背景板/底板高分材质的重新规划。
- **Phase D.3（跨平台 UI 壳层替换）**：在确认最终跨平台框架选型（如 `SDL3 + SDL_GPU + RmlUi` / `Qt6` / `Slint`）与文字后端（FreeType + HarfBuzz 实现 `ITextRasterizer`）后，基于已就绪的 `SettingsSchema`、4 个 ViewModel、`miniaudio` 与独立网络线程替换 Win32 视图与 `D2DTableRenderer` 壳层。
