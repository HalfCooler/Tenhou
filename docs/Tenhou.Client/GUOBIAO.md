# 国标客户端与图片接入

2026-11-16：国标通过 `VariantState → OnlineState → OnlineTable` 使用共用牌桌。单击出牌、拖动理牌、吃牌候选、多杠手选、和牌、四个自动功能、暂停/恢复及结算窗口均使用共用路径。合法动作、番数和付款以服务器为准，无补花按钮、报听动作或立直标记。

## 文字和图像

番种文案、玩法标签、最后一局提示和补花浮字统一使用 [TrueType 文字目录](../../Tenhou.Client/assets/text/README.md)。历史编号 622–703、705、578 和 480 可在目录数据中核对。

`ResourceManager` 与 `VariantArtwork` 按固定目录加载及缓存八花牌面 PNG。番型以玩法和服务端名称匹配文字目录，显示字形按目录文案，例如服务器的「一色四步高」显示为「一色四歩高」。

## 八花与牌山

实体顺序为 `136梅、137兰、138竹、139菊、140春、141夏、142秋、143冬`。固定上游版本的 SVG、许可和来源位于 `Tenhou.Client/assets/sources/mahjong_graphic/`；已有派生 PNG 位于 `images/variants/flowers/`，可由 `scripts/Generate-FlowerArtwork.py` 重建。运行时不需要 Python 或 SVG 库，发布附带来源许可。

花牌复用白牌框，在牌面平面内合成图案并按座位旋转。每家展示第一张花与 ×N，副露从下一个槽位开始，不开启三麻玩法位。开局补花在开局演出结束时按座位浮显一次 480；同一快照连续补花按座位合并，恢复不重播。普通摸牌消耗前端，补花及杠补消耗尾端，连续补花后的普通牌动画从最后实际消耗的位置出发。

144 张每边 18 墩，136 张每边 17 墩。第 18 墩复用第 17 墩的图块并平移，整边反移半步并向桌心收进 HD 8px；单层、双层、阴影和动画起点共用实际墩位置。108/136 张不使用这组位移。

## 结算

番型名称、次数、逐项分值、总飜与和牌收入按结算窗口的目标尺寸绘制。每项分值由服务器提供，旧快照缺失分值时按登记的番种目录回退。和牌收入包含起和底分，四家分数卡显示实际付款明细。

每页最多 14 项，超过 6 项用两列，长名称与数字分别按可用区域等比缩放；超过 14 项分页，演出跨页推进。演出结束后可用上一页/下一页或 PageUp/PageDown 浏览，确认先翻页，最后一页再确认下一盘。流局使用 461“流局”和原分数变化面板。标准 16 局在北 4 显示 578，其他房间长度按 total_hands 处理。

## 验证

客户端检查覆盖八花四向缓存、首花计数与副露占位、18 墩布局与阴影、连续补花摸牌起点、82 项番种、两列分页及恢复不重播。原生协议检查覆盖服务端结算与完整对局。结算的离屏检查见 [P4 记录](TRUETYPE-TEXT-P4-REVIEW.md)。

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure
.\Build-Tenhou.ps1 -Target Tenhou.Server.Tests
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
```
