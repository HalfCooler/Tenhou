# 文字规格目录

`catalog.json` 保存生产客户端的 TrueType 文案、样式和番型映射，`rendering.json` 保存绘制效果参数。历史规格源于 `Sprites.xlsx` 的 `Sprites705` 工作表及用户逐像素确认的修正，当前 JSON 为唯一维护真源。

## 数据约定

- `source` 记录原始工作表的 SHA-256、来源路径及规格摘要。规则映射的来源校验值是首次从旧资源清单提取时的追溯记录。
- `labels` 保存显示文案；`texts` 保存 296 个语义定义，引用 `label_key` 与 `style`。普通/悬停、checked/unchecked 共用文案，两态分别引用样式。空格和字形原样保留，不进行简繁转换或规则名替换。
- `styles` 的字号、字距单位均为 pt，字体统一为 `HGSeikaishotaiPRO`。颜色描述已修正为黑色、白色，原始 Size / Color / Style / Note 仍保存在来源记录中供追溯。
- `images` 只含 401、402、440、466、589、591、592 共 7 个保留非文字图形条目；434 和 593 未登记。
- 481 使用通常役种的完整 `yaku` 样式：**24 pt、近白、非粗体、无描边、无阴影、零额外字距**。源表 56 pt 及粗体、阴影记录仅供追溯，不参与渲染样式选择。
- 只有 567–588、704、705 的空白格式继承 566；`inherits_origin_id` 和 `inherited_fields` 显式记录继承关系。其他类别不前向填充。
- `info_decoration` 分别保存普通背景、倒数第二行、最后一行的灰度渐变（白底合成参考对应的黑色 Alpha 遮罩与独立灰色高亮层）。Info 共用 6.5 个基准字符 advance 的宽度；Alpha 包络、边距及 459 的 12 pt 小字偏移登记于 `rendering.json`。
- `rule_patterns` 保存四川/国标 101 个服务端匹配名称、显示文字语义键和缺省分值。`龙七对` 匹配到显示 `竜七対`，未知名称沿用系统字体回退。
- 普通按钮为黑边、悬停按钮为橙边。441–449 / 595–598 对应悬停，450–458 / 599–602 对应普通；459 选择按钮使用黑边。自动选项仍按 checked/unchecked 使用棕边/黑边，不跟随 hover 规则。

## 维护与构建检查

直接编辑 `catalog.json` 与 `rendering.json` 维护规格；增加或修改语义键时同步调整 C++ 调用方，修改规则映射时同步核对服务端匹配名和分值。不要直接编辑构建期生成的 C++ 头文件。

CMake 配置阶段读取已登记 JSON，生成 `obj/native/<配置>/generated/text_catalog.hpp` 与 `text_render_profile.hpp`（包含 `TextKey`、UTF-16 / UTF-8 共享文案、`TextStyle`、`ImageDefinition` 和 `RulePatternDefinition`），客户端运行时无需解析 JSON。

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release --output-on-failure
```

- `client_text_catalog` 检查 296 文字 / 7 图像分类、样式、字形存在性、共享文案、Info 装饰、番型映射、生产按钮状态及缺图回退。
- `client_text_composition` 与 `client_presentation` 检查最终尺寸合成、局部/完整重绘逐像素一致性及 296 个文字编号的资源隔离。
- 手动样张导出工具 `tenhou_text_samples.exe` 随 `Tenhou.Client.Tests` 一并编译（不随 CTest 自动运行），用于人工抽检全部 296 条文案与透明 Info 图层。

## 生产接入范围

- **牌桌与开局**：风位/局数、右上角 Info、自动选项、振听提示、动作按钮、四家浮字（共用 `TextRenderer::Caption` 视觉中心锚点）及开局/恢复确认面板直接使用文字目录，在线与回放共用 `FinalScene` 最终尺寸合成。
- **结算**：使用文字目录绘制役种、番型、标题、风位、次数和带单位的数值；四川和国标按服务端原名匹配 `rule_patterns` 取得显示文字和缺省番值。
