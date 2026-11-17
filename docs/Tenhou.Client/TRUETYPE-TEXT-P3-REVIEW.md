# TrueType P3：牌桌、按钮与开局接入

状态：2026-11-17 已完成生产入口接入、客户端自动检查及三玩法协议烟测。离屏截图已检查；实际鼠标操作、多 DPI 与跨屏拖动仍属于 P6 人工验收，不由离屏结果替代。结算文字及资源清单收尾分别留在 P4、P5。

## 接入内容

- `original_display.hpp/.cpp`：`OriginalHudLayout::text` 使用语义 `TextKey` 表达中央风位、局数及四座风位；真实立直棒继续使用 `DisplaySprite`。`AppendHud` 保留四座旋转、轻微倾斜及原生锚点。Info、玩法标签、All Last、振听与自动选项通过 `AppendChrome` / `AppendStatus` 生成最终尺寸文字命令，行高和宽度来自文字测量，生产入口不再读取对应文字 PNG/DAT 记录。
- `lobby_view.cpp`：动作控件通过 `ActionButtonText` 选择普通黑边或悬停橙边，涵盖吃、碰、杠、立直、荣和、自摸、流局、跳过、拔北、交换和三种定缺。保留 440 底板、原点击区域、按下位移和禁用遮罩；自动选项按 checked/unchecked 选样式并保留未选中 Alpha。
- `action_captions.hpp`：事件映射到语义文字键，新增 `AcceptText`；`AcceptResource` 仅作为 OriginId 过渡适配器。固定文案使用目录，四座锚点、100 tick 生命周期、出现缩放、淡入淡出及调用层事件去重保持原有逻辑。动画帧按当前变换栅格化文字轮廓，交给最终合成缓存恢复下层画面。
- `online_table.cpp` / `table_view.cpp`：在线与回放牌桌把文字和浮字交给 P2 的 `FinalScene` / `RetainedScene`。在线入口可传入窗口 DPI，离屏 DC 默认 96；目标尺寸已是物理像素，不再次乘 DPI。删除在线原生尺寸单独覆画旧文字图片的分支，统一处理缓存、曝光恢复和浮字结束。
- `match_intro_view.cpp`：403–423、463/464 改为目录文字。背景中央文字先于确认框遮罩，标题及座位风位与卡片按顺序交错合成，避免被统一放到最顶层。昵称、段位、倒计时及 OK 保留各自的现有字体策略。

字体服务按资源目录延迟创建，从仓库已有 `TextFont` 读取私有字体；更换资源配置时先销毁渲染器再释放字体。固定文字复用 P1 的有界布局/栅格缓存，不生成文字 PNG，不在运行时解析 Excel。

## 验证记录

构建及检查命令：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure
.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```

客户端及客户端测试 Release 构建通过；CTest **5/5 通过**，包括 P2 合成、文字目录、变体、生产呈现与资源检查。`git diff --check` 通过。

`text_table_check.cpp` 随 `client_presentation` 执行，直接驱动生产 OnlineTable、LobbyView 按钮、MatchIntroView 与 TableView 回放绘制：

- 三玩法、1440 原生画布，分别输出到 1024、1197 和 1440 宽度；包含非整数缩放及有偏移的目标矩形。
- 完整重绘与缓存/局部重绘逐像素比较，覆盖自动选项及 hover 边框、轮转风位、分数变化、四座浮字、浮字淡出/清空、振听消失、清空目标后的曝光恢复。
- 原生 1024/1440 的文字层分别验证锚点和 SD 的一次 `1 / 1.4` 换算；静止文字层连续绘制不会新增布局或栅格。
- 在临时资源副本中移除全部登记文字 PNG，再写入损坏 PNG；每轮均新建生产渲染器。三玩法牌桌、动作按钮、开局/恢复窗口及回放的输出与完整资源逐像素一致。真实图像 440 损坏仍触发原资源错误。原始资源未被修改。
- 按钮截图覆盖普通、按下、禁用三列；普通/悬停文字键和样式映射继续由 `client_text_catalog` 检查。

截图由测试输出到 `<RESOURCES 的父目录>/text-p3/`，例如 CTest 的 `Tenhou.Client.Tests/bin/Release/text-p3/`。本次另复制到 `build/text-p3/` 方便查看：18 张三玩法牌桌图，另有 `buttons.png`、`intro-match.png`、`intro-resume.png`、`replay.png`。已查看非整数尺寸牌桌的四座文字，以及按钮、开局/恢复和回放截图。

真实协议检查通过原生客户端队列 101、201、301 完整对局（分别 26、154、116 个动作），服务端测试报告 `All checks passed`。显示迁移未改变协议、规则、选牌实体或提交屏障。

## 验证边界

仓库只随附 HD 牌面；完整原生 1024 牌桌仍需要原有可选 SD 对象包及相匹配的显示包。此次 1024 输出牌桌使用 1440 原生画布缩放，原生 SD 单独验证文字层，不能据此宣布缺少 SD 图片资源的完整牌桌已通过。

实际 hover/按下期间状态变化、单击出牌、拖动理牌、多个杠候选、换牌/定缺鼠标操作、窗口遮挡恢复及 100%/125%/150%/200% DPI 与跨屏拖动，继续按 P6 矩阵人工验收。本阶段验证静止文字缓存复用，未进行迁移前后同条件性能基准，不宣称性能验收完成。

P4 结算仍使用现有番型/数字图像入口；P5 前继续保留旧 PNG 登记和原始 DAT 包。字体发布来源与随包使用条件仍须在正式打包前落实。
