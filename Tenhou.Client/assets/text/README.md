# 文字规格目录

`catalog.json` 是 TrueType 迁移的工程数据。设计来源是 `docs/Tenhou.Client/asset-reference/Sprites.xlsx` 的 `Sprites705` 及用户确认的修正，实施边界见 [计划](../../../docs/Tenhou.Client/TRUETYPE-TEXT-PLAN.md)。P0 已建立目录与生成链路，文案和修正规格已确认，见 [核对记录](../../../docs/Tenhou.Client/TRUETYPE-TEXT-P0-REVIEW.md)；字体绘制从 P1 开始接入。

## 数据约定

- `source` 记录 XLSX 和工作表 XML 的 SHA-256、来源路径及规格摘要。规则映射的来源校验值是首次从旧资源清单提取时的记录。
- `labels` 保存显示文案；`texts` 保存 296 个语义定义，引用 `label_key` 与 `style`。普通/悬停、checked/unchecked 共用文案，两态分别引用样式。空格和字形原样保留，不进行简繁转换或规则名替换。
- `styles` 的字号、字距单位均为 pt，字体统一为 `HGSeikaishotaiPRO`。颜色描述已修正为黑色、白色，原始 Size / Color / Style / Note 仍保存在来源记录中供追溯。色板标为 `calibrated: false` 的 RGBA 是初值。
- `images` 只含 401、402、440、466、589、591、592，466 残留的字号/颜色仍只作来源记录。434 和 593 未登记。
- 481 使用通常役种的完整 `yaku` 样式：**24 pt、近白、非粗体、无描边、无阴影、零额外字距**。源表 56 pt 及粗体、阴影记录仅供追溯，不参与渲染样式选择。
- 只有 567–588、704、705 的空白格式继承 566；`inherits_origin_id` 和 `inherited_fields` 显式记录继承关系。其他类别不前向填充。
- `info_decoration` 分别保存普通背景、倒数第二行、最后一行的灰度渐变，用户已逐像素确认。灰度不表示透明度。Info 共用 6.5 个基准字符 advance 的宽度；Alpha、边距、基准测量留 P1。459 右下角小字已确认为 **12 pt**，仅偏移留 P1；未确定值用 `null` 保存。
- `rule_patterns` 保存四川/国标 101 个服务端匹配名称、显示文字语义键和缺省分值。`龙七对` 匹配到显示 `竜七対`，未知名由后续调用方沿用文本回退；不从显示名反推规则。
- 普通按钮为黑边、悬停按钮为橙边。441–449 / 595–598 对应悬停，450–458 / 599–602 对应普通；459 选择按钮使用黑边。自动选项仍按 checked/unchecked 使用棕边/黑边，不跟随 hover 规则。

## 主动更新规格

从仓库根目录执行；只需要 Python 3 标准库，不需要 Excel 或第三方 Python 包：

```powershell
# 只读核对：一致为 0，有差异为 1，无效输入为 2
python -B scripts/Import-TextCatalog.py --check

# 输出差异和独立候选文件；显式请求时另生成核对表
python -B scripts/Import-TextCatalog.py `
  --output build/text-catalog.candidate.json `
  --report build/text-catalog-review.md
```

工具不改写 XLSX 或已登记 JSON。候选保留现有语义键、规则映射和校准参数，按 OriginId 更新 Label、来源记录、样式引用和校验值。`normalized_style` 始终应用用户确认的颜色错字修正和 481 完整样式修正，重新导入不会恢复旧设定。如果出现未登记样式或共享文案的两态不一致，导入失败并要求先明确映射。新 OriginId 或分类范围变化需要先更新目录和 P0 完整性约束。

人工审阅候选差异后，将候选登记为 `catalog.json`，更新核对记录。增加或修改语义键时同步调整调用方；修改规则映射时同时核对服务端匹配名和分值。不要编辑构建生成的 C++ 头文件。

## 构建与检查

CMake 配置仅读取已登记 JSON，生成 `obj/native/<配置>/generated/text_catalog.hpp`，包含 `TextKey`、UTF-16 / UTF-8 共享文案、`TextStyle`、独立 `ImageDefinition` 和 `RulePatternDefinition`。已接入按钮文字、玩法标签、完成浮字、立直役种与四川/国标番型的代码回退，已知名称从目录读取，未知名称原样显示。JSON 改动会触发重新配置；`configure_file` 在内容不变时保留输出时间戳。客户端运行不解析 XLSX/JSON，日常构建不调用 Python。

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client
./Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir ./Tenhou.Client.Tests/obj/native/Release --output-on-failure

# 可选的规格导入/错误输入检查；不属于日常构建依赖
python -B scripts/tests/test_text_catalog.py -v
```

`client_text_catalog` 检查分类、样式、字形字符串、共享文案、Info 装饰和全部旧番型映射的名称/分值/OriginId 一致性，并检查生产按钮状态选择及缺图时的已知/未知番型文字回退。显式 Python 检查使用临时副本测试导入和 CMake 拒绝错误数据，不修改用户资源。现有 PNG/DAT 入口保留，普通按钮两态选择已按确认结果调整；字体绘制入口分别在 P1–P5 接入。
