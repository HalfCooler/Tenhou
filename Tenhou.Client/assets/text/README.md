# 文字规格目录

`catalog.json` 保存生产客户端的 TrueType 文案、样式和番型映射，`rendering.json` 保存绘制效果参数。设计来源是 `docs/Tenhou.Client/asset-reference/Sprites.xlsx` 的 `Sprites705` 及用户确认的修正。详见 [实施计划](../../../docs/Tenhou.Client/TRUETYPE-TEXT-PLAN.md) 和 [规格核对记录](../../../docs/Tenhou.Client/TRUETYPE-TEXT-P0-REVIEW.md)。

## 数据约定

- `source` 记录 XLSX 和工作表 XML 的 SHA-256、来源路径及规格摘要。规则映射的来源校验值是首次从旧资源清单提取时的记录。
- `labels` 保存显示文案；`texts` 保存 296 个语义定义，引用 `label_key` 与 `style`。普通/悬停、checked/unchecked 共用文案，两态分别引用样式。空格和字形原样保留，不进行简繁转换或规则名替换。
- `styles` 的字号、字距单位均为 pt，字体统一为 `HGSeikaishotaiPRO`。颜色描述已修正为黑色、白色，原始 Size / Color / Style / Note 仍保存在来源记录中供追溯。色板标为 `calibrated: false` 的 RGBA 是初值。
- `images` 只含 401、402、440、466、589、591、592，466 残留的字号/颜色仍只作来源记录。434 和 593 未登记。
- 481 使用通常役种的完整 `yaku` 样式：**24 pt、近白、非粗体、无描边、无阴影、零额外字距**。源表 56 pt 及粗体、阴影记录仅供追溯，不参与渲染样式选择。
- 只有 567–588、704、705 的空白格式继承 566；`inherits_origin_id` 和 `inherited_fields` 显式记录继承关系。其他类别不前向填充。
- `info_decoration` 分别保存普通背景、倒数第二行、最后一行的灰度渐变，用户已逐像素确认。P1 根据用户透明参考图明确灰度为白底合成结果：主体和最后一行为黑色 Alpha 遮罩，倒数第二行为独立灰色高亮层。Info 共用 6.5 个基准字符 advance 的宽度；Alpha 包络、边距及 459 的 12 pt 小字偏移已登记于 P1 `rendering.json`；P0 追溯字段中的 `null` 不作为运行时参数。
- `rule_patterns` 保存四川/国标 101 个服务端匹配名称、显示文字语义键和缺省分值。`龙七对` 匹配到显示 `竜七対`，未知名称沿用系统字体回退。
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

CMake 配置仅读取已登记 JSON，生成 `obj/native/<配置>/generated/text_catalog.hpp`，包含 `TextKey`、UTF-16 / UTF-8 共享文案、`TextStyle`、独立 `ImageDefinition` 和 `RulePatternDefinition`。JSON 改动会触发重新配置，`configure_file` 在内容不变时保留输出时间戳。客户端运行不解析 XLSX/JSON，日常构建不调用 Python。

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client
./Build-Tenhou.ps1 -Target Tenhou.Client.Tests
ctest --test-dir ./Tenhou.Client.Tests/obj/native/Release --output-on-failure

# 可选的规格导入/错误输入检查；不属于日常构建依赖
python -B scripts/tests/test_text_catalog.py -v
```

`client_text_catalog` 检查分类、样式、字形字符串、共享文案、Info 装饰、番型映射、生产按钮状态及已知和未知番型的显示。显式 Python 检查在临时副本上验证导入和 CMake 对错误数据的处理。

## P1 字体与样张

P0 验证已完成。P1 新增 `text_renderer.hpp/.cpp` 和独立 `rendering.json` 效果参数；CMake 生成 `text_render_profile.hpp`，无需运行时解析。确认的文案/字号/灰度继续来自 `catalog.json`，Alpha、边距与偏移保存在独立 P1 配置。私有字体通过 ResourceManager 的 `TextFont` 读取仓库已有 TTF，也可显式选用已安装字体。用户已完成人工视觉验证，两项反馈已修正；目录色板及渲染配置标记为已确认。

构建测试项目会生成 `tenhou_text_samples.exe`，用于手动导出全部 296 条文案、各样式与最终尺寸原型及独立透明 Info PNG；不会由 CTest 自动执行。命令、交付参数、人工验收记录和接口边界见 [P1 说明](../../../docs/Tenhou.Client/TRUETYPE-TEXT-P1-REVIEW.md)。浮字通过共用 `TextRenderer::Caption` 居中，594 没有独立偏移规则。

## 生产接入

牌桌风位/局数、Info、自动选项、振听、动作按钮、四座浮字及开局介绍已直接使用文字目录。在线和回放复用 P2 最终尺寸合成，见 [P3 记录](../../../docs/Tenhou.Client/TRUETYPE-TEXT-P3-REVIEW.md)。

结算使用文字目录绘制役种、番型、标题、风位、次数和带单位的数值。四川和国标按服务端原名匹配 `rule_patterns`，从同一条映射取得显示文字和缺省番值。接入和检查结果见 [P4 记录](../../../docs/Tenhou.Client/TRUETYPE-TEXT-P4-REVIEW.md)。

296 个文案 OriginId 用于追溯规格。编号 PNG 只为 7 个 Image 条目登记，旧文字样图保存在源码资源目录供对照。图像和打包检查见 [P5 记录](../../../docs/Tenhou.Client/TRUETYPE-TEXT-P5-REVIEW.md)。
