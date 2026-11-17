# TrueType P2：最终尺寸合成与缓存

状态：P2 合成基础已接入 `PaintBuffer`、`RetainedScene`、`CachedHud`；生产牌桌/按钮/开局文字与结算文字分别在 P3、P4 迁移。本阶段交付最终层接口、缓存失效与可重复的离屏合成检查，不将底层接入等同于全部文字迁移。

## 合成路径

新增 [final_scene.hpp](../../Tenhou.Client/src/final_scene.hpp) / `.cpp`，使用 P1 的 `SceneTextRun`，按提交顺序保存文字与图像回调。坐标、锚点和逐命令裁切统一使用 HD 设计像素；SD 只乘一次 `1 / 1.4`。最终物理目标尺寸直接来自 `PaintBuffer` 的 destination，DPI 参与缓存身份，不再乘字号。

绘制顺序为：原生图像画布 → HALFTONE 缩放的干净底图 → 有序最终层 → 一次呈现。最终层允许 `Image → Text → Image → Text`，不会统一把文字推到所有图像之上。后续业务迁移时，需要遮挡文字的上层图形应按原有顺序提交 `FinalScene::Image`；已经画进底图的图形无法再遮挡最终文字。

- `PaintBuffer::RenderScaled` 和 `RenderDirtyScaled` 增加可选 `const FinalScene *` 参数。没有最终层的既有入口保留原有路径。
- `RetainedScene::Render` 末尾转发最终层参数；文字变更可以独立失效，不要求伪造牌实体或修改鼠标坐标映射。
- `CachedHud::AppendText` 按显式状态键缓存 HD `SceneTextRun`，复用布局，不在 HUD 原生位图中提前栅格化。其位置就是最终层提交顺序中的当前位置。
- `TextRenderer::Bounds` 和 `Draw` 共用设备仿射几何与描边/阴影保护边界；合成失效不另用一份估算字号矩形。

`FinalScene` 是单帧对象，持有的渲染器及图像资源必须存活到本帧绘制结束。最终缓存只保留命令身份、边界和布局共享引用，不保存捕获业务状态的绘制回调。字体服务仍须早于渲染器创建、晚于渲染器销毁。

## 失效和缓存

缩放底图与最终合成缓冲分开保存。发生变化时，先从干净底图恢复旧/新效果范围，再按顺序重放相交命令。淡出和移除都会恢复原背景，透明文字、描边和阴影不反复叠加。

最终层比较覆盖布局身份（含文案/字体/样式）、锚点、对齐、完整仿射矩阵、Alpha、命令顺序、裁切、原生尺寸、物理目标尺寸和 DPI。旧布局持有共享引用，避免分配器复用地址后误判为同一文字。图像回调必须提供包含全部图像/动画变化的 revision key；`AppendText` 的键也须包含其构建结果的全部状态。

底图脏区映射至设备像素，再与命令变化的旧/新范围求保守并集；相交命令按完整效果范围扩展为原子重绘组，避免旋转图形在局部裁切边界改变采样。当前将这些区域合并为一个保守矩形，优先保证完整/局部结果一致。无变化时复用最终帧，不调用图像回调或重建文字栅格。窗口曝光直接从完整缓存复制，并遵守目标 DC 的更新区裁切。

布局、栅格仍使用 P1 的 256 项、64 项 / 32 MiB 上限；最终合成缓冲额外占用一张目标尺寸的 32 位位图，不建立随倒计时或昵称无限增长的帧缓存。新增 `TextCacheStats` 累计构建/命中计数，用于验证静止帧。失败的最终合成会丢弃合成帧，下一次恢复底图并完整重建。

## DPI 与窗口

[application.cpp](../../Tenhou.Client/src/application.cpp) 从系统 DPI 感知改为 Per Monitor V2。主窗 `WM_DPICHANGED` 按新 DPI 重新计算非客户区，并保持既有固定 **1440×1080 物理客户区**，触发子窗重绘；不把此物理尺寸再次乘 DPI。原有禁止拖动缩放/最大化的窗口约束继续有效。

后续生产 `FinalScene` 应传入所属窗口的 `GetDpiForWindow`；不要通过内存 DC 推断窗口 DPI。上下文输出尺寸变化负责重新栅格化，DPI 身份变化也会使栅格失效。

## 自动检查和样图

新增 [text_composition_check.cpp](../../Tenhou.Client.Tests/src/text_composition_check.cpp)，登记 CTest 名称 `client_text_composition`。命令：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure

# 单独运行，并生成易查找的离屏样图
.\build\Tenhou.Client.Tests\tenhou_text_composition_check.exe `
  .\build\Tenhou.Client.Tests\resources .\build\text-p2
```

CTest 默认样图写入 `Tenhou.Client.Tests/obj/native/Release/text-p2/`。手动命令写入 `build/text-p2/`；每种原生/物理宽度组合各有初始、淡出、清空三个 PNG，共 18 张。文件名为 `composition-<原生宽度>-<物理宽度>-<阶段>.png`，阶段 `0 / 5 / 14`。外圈留白用于检查目标偏移，蓝色图形有意位于浮字上方，用于检查遮挡；左下役种有意裁切，用于检查裁切变化。

检查覆盖：

| 项目 | 检查方式 |
| --- | --- |
| 1440 / 1024 原生画布，1440 / 1024 / 1279 物理输出 | 六组逐像素比较 retained/dirty 与完整参考重绘 |
| 文字变化 | 普通/悬停、数字变化、四向旋转、倾斜、子像素平移、逐命令裁切 |
| 背景与层次 | 文字下方图像局部变化、文字/图像互换顺序、实色图形遮挡先前文字 |
| 动画与恢复 | 缩放、淡出、浮字移除、整个最终层移除、目标 DC 局部曝光、完整曝光、视口缩小与恢复 |
| DPI | 96 / 120 / 144 / 192 上下文；保持物理几何不变但重新建立 DPI 对应栅格 |
| 绘制阶段 | 非整数比例下与目标尺寸直接绘字逐像素相同，且与先绘原生文字再缩放的结果不同 |
| 缓存 | 静止帧不重建栅格/重放图像；300 个动态数值后布局/栅格仍有界；失败后可恢复 |

## 本次验证与边界

记录日期：2026-11-17。

- 客户端与客户端测试 Release 构建成功；最终 CTest **5/5 通过**，包括 `client_text_composition`、`client_text_catalog`、`client_variants`、`client_presentation`、`client_resources`。
- 已导出上述 18 张 PNG，并检查 1279 宽的初始合成图及 SD 路径的淡出图：四向红边文字、Info、459 小字、浮字上层遮挡和裁切显示符合该检查场景。
- 使用独立配置目录 `build/text-p2/startup-smoke/` 启动并正常关闭客户端；在本机 144 DPI 环境查询到 Per Monitor V2 与 1440×1080 物理客户区，日志正常退出。查询线程也切换至 PMv2，避免调用方 DPI 虚拟化导致尺寸读数失真。
- 离屏 DPI 元数据和启动检查不能替代真实跨屏拖动、各显示器下的交互与视觉验收。尚未对生产文字迁移后的整局动画性能做前后对比，这些随 P3–P6 继续。

P2 不修改玩法协议和规则，三玩法完整协议烟测按计划在显示迁移集成后统一执行。P0/P1 已确认的文案、色板和效果参数不在本阶段重新校准。
