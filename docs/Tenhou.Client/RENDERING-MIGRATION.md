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
