# 牌桌渲染迁移收益调研

2026-11-24。重建端基线为 `e51a9fa`。原版为 Windows 版 `130/0005`，`Tenhou.Client.Origin/tenhou/tenhou.exe` 的 SHA-256 为 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`。

## 静态取证

- 原版反编译 `Tenhou.Client.Origin/tenhou/tenhou.exe.c` 中的 `FUN_004761a0`（约 78028 行）在 `param_6 == 1.0` 且源与目标尺寸相同时调用 `GdipDrawImagePointRectI`，其余情况使用带颜色矩阵的 `GdipDrawImageRectRectI`。这个调用点确认了两条位图绘制分支。滤波核和混合舍入仍待验证。
- 重建端的 [OnlineGame](../../Tenhou.Client/src/online_game.hpp)、[LobbySnapshot](../../Tenhou.Client/src/network_protocol.hpp) 和 [ReplaySession](../../Tenhou.Client/src/replay.hpp) 已提供状态边界。[LobbyView](../../Tenhou.Client/src/lobby_view.cpp) 仍在窗口消息路径中协调协议快照、开局及结算时序、自动操作、声音和绘制。[OnlineTable::Paint](../../Tenhou.Client/src/online_table.cpp#L164) 负责状态投影、动画取样、排序、阴影和绘制。
- [RetainedScene](../../Tenhou.Client/src/retained_scene.hpp#L135) 已跟踪受损区域并保留底层画面。[PaintBuffer](../../Tenhou.Client/src/paint_buffer.hpp#L119) 的最终合成路径包含逐行像素比较、带裁切的 `HALFTONE StretchBlt` 及变更后的源像素备份。[MotionTimer](../../Tenhou.Client/src/motion_timer.hpp) 在 UI 线程调度按需重绘，窗口隐藏或最小化时放慢唤醒。

## 收益与验证

| 改动 | 判断 |
| --- | --- |
| 拆分对局呈现调度与绘制 | 工程收益高。利用现有模型，把事件顺序、动作提交和结算时序放在可注入时钟的控制器里，给渲染器只读的帧数据。现有测试可用于固定开局、暂停恢复和跨盘语义。拆分本身的 CPU 收益取决于能否减少重复投影和布局。 |
| 牌桌迁移到 Direct2D + D3D11 | 性能收益集中在连续重绘及高物理分辨率下。常驻位图、GPU 上的缩放和透明混合可减轻 UI 线程的合成负担。静止牌桌已有缓存，预期增益较小。保持按需呈现，并以原版实窗核对采样、预乘 Alpha、透明边缘、图层顺序和窗口缩放。 |
| 引入 D3D12 | 当前收益预期低。只有分析确认 D3D11 命令提交或驱动开销显著时才值得评估。通过 D3D11On12 接入 Direct2D 还会引入互操作和同步成本。 |

最低成本的验证是用固定牌谱、相同资源、相同窗口物理尺寸和相同时钟，比较当前基线、拆分后的 GDI+ 路径、Direct2D 牌桌原型。取静止、普通摸打和连续动画样本，在原生尺寸与高 DPI 下用 ETW 比较进程 CPU、每次绘制的 UI 线程耗时 P50/P95、重绘次数及 GPU 占用。若缩放或像素对比居首，原型应覆盖这一步。若场景构建居首，先验证复用帧数据的收益。避免逐帧从 CPU 上传整张已合成画面。实现路线参考 [Direct2D 设备上下文](https://learn.microsoft.com/en-us/windows/win32/direct2d/devices-and-device-contexts)、[Direct2D 性能指南](https://learn.microsoft.com/en-us/windows/win32/direct2d/improving-direct2d-performance)和 [D3D11On12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12)。

本次验证范围为静态调用点和现有测试入口。待验证项包括实际 CPU/GPU 耗时、不同物理尺寸下的画面差异及原版实窗对照。现有 [客户端测试说明](../Tenhou.Client.Tests/README.md)列出呈现事件、局部重绘和离屏图片的覆盖范围。
