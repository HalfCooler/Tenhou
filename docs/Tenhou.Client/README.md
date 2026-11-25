# Tenhou.Client

Windows C++17 客户端，以原版 `Tenhou.Client.Origin/tenhou/tenhou.exe`（Windows `130/0005`，SHA-256 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`）为行为与时序基准，支持官方/自建大厅、三/四人立直、四川血战到底、国标麻将与本地牌谱回放。

## 构建与启动

```powershell
.\Build-Tenhou -Target Tenhou.Client
.\build\Tenhou.Client\tenhou_rebuild.exe
```

也可通过 [Tenhou.slnx](../../Tenhou.slnx)（`Win32` 配置）构建与调试。

| 参数 | 说明 |
| --- | --- |
| `--config-dir <目录>` | 指定配置与日志目录（`config.ini` / `session.ini` / `rebuild.log`），并隔离单实例互斥锁以支持多开 |
| `--resources <目录>` | 指定资源根目录（默认读取可执行文件旁 `resources/`） |
| `--replay <文件>` | 启动即打开指定 XML / `.mjlog` / `.gz` 牌谱（窗口也支持拖入） |
| `--offline-lobby` | 使用本地固定夹具进入原版布局大厅检视页 |
| `--service-lobby` | 登录自建服务器时启用原版布局会场、预约与大会大厅（见 [自建服务大厅](../Tenhou.Server/PROTOCOL.md#自建服务大厅)） |

## 模块结构

原生 CMake 工程位于 `Tenhou.Client/CMakeLists.txt`，按单向依赖划分：

- `tenhou_resources`：`_obj2.dat` / `_obj3.dat` / `_res2.dat`、花牌覆盖、内嵌阴影与字体资产加载。
- `tenhou_replay`：立直牌谱解析、回放状态机与结算数据（无 UI 依赖）。
- `tenhou_network`：网络分帧、官方/自建大厅会话、在线牌局、变体快照及纯 C++ 大厅模型与计时。
- `tenhou_presenter`：牌面几何目录、手牌理牌/拖拽/命中投影、结算与牌桌演出编排（脱离 `HWND`/`HDC`/GDI+）。
- `tenhou_text`：[TrueType 文字目录](../../Tenhou.Client/assets/text/README.md) 驱动的私有字体排版、特效栅格化与最终尺寸合成（`FinalScene`）。
- `tenhou_table`：牌桌场景几何、发牌/飞牌动画、牌面阴影、Direct2D / GDI 后端与音频调度。在线与回放共用 `TableRenderFrame` 和 `ITableCanvasRenderer`；牌桌默认走 Direct2D 1.1 + D3D11（截图/结算取样经 staging 纹理回读），字体沿用 `TextRenderer` 预乘 BGRA 光栅，保留 GDI 兼容路径。
- `tenhou_rebuild`：Win32 主窗口壳层、欢迎页、大厅、牌桌与结算视图。

## 相关文档

- [解耦与 Direct2D 迁移路线](DECOUPLING-PLAN.md)
- [牌桌渲染迁移收益调研](RENDERING-MIGRATION.md)
- [原版对局采集清单](ORIGIN.md)
- [原版会话采集工具](ORIGIN-SESSION-CAPTURE.md)
- [TrueType 文字目录](../../Tenhou.Client/assets/text/README.md)
- [客户端专项检查](../Tenhou.Client.Tests/README.md)
- [第三方许可](THIRD-PARTY-NOTICES.md)
