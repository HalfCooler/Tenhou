# Tenhou.Client

Windows 原生 C++17 客户端，以仓库内冻结的原版客户端 `Tenhou.Client.Origin/tenhou/tenhou.exe`（Windows 版 `130/0005`，SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）为行为与时序基准，支持官方/自建大厅、标准三/四人立直麻将、四川血战到底、国标麻将及本地牌谱回放。

## 构建与启动参数

在仓库根目录执行：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client
.\build\Tenhou.Client\tenhou_rebuild.exe
```

支持以下命令行参数：

| 参数 | 说明 |
| --- | --- |
| `--config-dir <目录>` | 指定配置与日志目录（读写 `config.ini`、`session.ini`、`rebuild.log`），同时隔离单实例互斥锁以支持本机多开 |
| `--resources <目录>` | 指定资源根目录（默认读取配置项或可执行文件旁的 `resources/`） |
| `--replay <文件>` | 启动后直接进入离线牌谱回放视图并打开指定 XML / `.mjlog` / `.gz` 文件（主窗口亦支持直接拖入牌谱文件） |
| `--offline-lobby` | 使用本地固定战绩夹具进入原版布局大厅检视页 |
| `--service-lobby` | 登录自建服务器时启用原版布局会场、预约与大会大厅（协议见 [自建服务大厅](../Tenhou.Server/PROTOCOL.md#自建服务大厅)） |

## 模块结构与演进路线

唯一原生 CMake 工程位于 `Tenhou.Client/CMakeLists.txt`，按职责划分为单向依赖的静态库与可执行目标：

- `tenhou_resources`：资源包（`_obj2.dat`、`_obj3.dat`、`_res2.dat`）、变体花牌覆盖、内嵌阴影包与字体资产读取。
- `tenhou_replay`：立直牌谱解析、牌桌回放状态机与结算数据结构（无窗口/图形依赖）。
- `tenhou_network`：网络分帧、官方与自建大厅会话协议（`LobbySession`）、在线牌局状态（`OnlineGame`）、变体快照（`VariantState`）与大厅模型（`OfflineLobby`）。
- `tenhou_presenter`（解耦演进中）：纯 C++ 几何锚点目录（`TileGeometryCatalog`）、手牌理牌/拖拽/命中投影（`HandProjection`）、结算与牌桌演出编排器（`SettlementSequencer` / `TableSequencer`），完全脱离 `HWND`、`HDC` 与 GDI+。
- `tenhou_text`：[TrueType 文字规格目录](../../Tenhou.Client/assets/text/README.md) 驱动的私有字体排版、描边/渐变/发光栅格化与最终尺寸合成层（`FinalScene`）。
- `tenhou_table`：牌桌场景几何（`BuildTableScene`）、飞牌与砌墙发牌过渡动画（`TablePresentation` / `OpeningPresentation`）、牌面阴影及音频调度。
- `tenhou_rebuild`：Win32 主窗口壳层、登录欢迎页、在线/离线大厅、牌桌与结算视图。

### 既定渲染与解耦路线

详细任务目标、行为边界、已完成模块与待推进清单见 [解耦与 Direct2D 迁移路线](DECOUPLING-PLAN.md)，总体分阶段如下：

1. **逻辑与演出全面下沉（Stage 2）**：已将几何目录（`TileGeometryCatalog`）、手牌投影（`HandProjection`）、结算编排（`SettlementSequencer`）、在线与回放牌桌编排（`OnlineTableSequencer` / `ReplayTableSequencer`）及统一牌桌帧构建（`TableRenderFrame` + `ITableCanvasRenderer` + `GdiTableRenderer`）落入 `tenhou_presenter` / `tenhou_table`；待完成离线大厅战绩格式化与计时下沉。
2. **牌桌画布局部硬件加速（Stage 3.2-B）**：保持多原生子控件的表单/大厅壳层（`WelcomeView`、`OfflineLobbyView`、`SettingsView` 等）及已逐像素校准的 `TextRenderer` 字形栅格化内核不变；将牌桌动作按钮收归 `TableRenderFrame::buttons` 自绘与命中，再为 `ITableCanvasRenderer` 接入 **Direct2D 1.1 + D3D11 Flip SwapChain**（`D2DTableRenderer`），从而移除纯 CPU 多级缩放缓冲与脏矩形合成负担，降低后续扩展新玩法的渲染成本。

## 相关文档

- [解耦与 Direct2D 迁移路线（目标、边界与待办）](DECOUPLING-PLAN.md)
- [牌桌渲染迁移收益调研](RENDERING-MIGRATION.md)
- [原版对局采集清单](ORIGIN.md)
- [个人战绩面板、错误弹窗与主窗口壳层取证](ORIGIN-PROFILE-AND-SHELL.md)
- [原版会话消息采集工具说明](ORIGIN-SESSION-CAPTURE.md)
- [TrueType 文字规格目录](../../Tenhou.Client/assets/text/README.md)
- [客户端专项检查与验收边界](../Tenhou.Client.Tests/README.md)
- [第三方源码与许可](THIRD-PARTY-NOTICES.md)
