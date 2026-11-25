# 文字规格目录

`catalog.json`（文案、样式、番型映射）与 `rendering.json`（绘制效果参数）是客户端 TrueType 文字的唯一维护真源。CMake 配置阶段将其生成为 `obj/native/<配置>/generated/text_catalog.hpp` 与 `text_render_profile.hpp`，运行时不解析 JSON。

## 关键约定

- `labels` / `texts`：296 条语义定义，空格与字形原样保留；普通/悬停、checked/unchecked 共用文案、分引样式。
- `styles`：字号与字距单位为 pt，字体统一 `HGSeikaishotaiPRO`。481 使用通常役种 `yaku` 样式（24 pt、近白、无粗体/描边/阴影）；仅 567–588、704、705 继承 566。
- `images`：仅含 7 个保留非文字图形（401、402、440、466、589、591、592）。
- `info_decoration` 与 `rendering.json`：定义 Info 三段灰度渐变、6.5 基准字符宽、Alpha 包络与 459 的 12 pt 小字偏移。
- `rule_patterns`：四川/国标 101 个服务端名称到显示键与缺省番值的映射（如 `龙七对` -> `竜七対`），未知名称回退系统字体。
- 按钮边框：普通黑边（450–458 / 599–602 / 459）、悬停橙边（441–449 / 595–598）；自动选项按 checked/unchecked 用棕边/黑边。

## 维护与验证

直接编辑两个 JSON 文件，勿改生成的 `.hpp`；增改语义键时同步更新 C++ 调用方，改动番型映射时核对服务端名称与番值。

构建与 CTest 见 [Tenhou.Client.Tests](../../../docs/Tenhou.Client.Tests/README.md)（关注 `client_text_catalog`、`client_text_composition`、`client_presentation` 及手动样张工具 `tenhou_text_samples.exe`）。
