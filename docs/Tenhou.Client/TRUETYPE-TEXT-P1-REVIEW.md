# TrueType P1：渲染基础与人工样张

状态：P0 与 P1 样张已由用户完成人工视觉验证。仅报告 594 样张偏左及 Info 背景合成两项问题，本次已按反馈修正，**P1 完成交付**。本阶段没有替换生产牌桌或结算的图像绘制入口，P2 再接入最终合成。

修正后的构建输出位于 `build/text-p1/installed/`（三个宽度）和 `build/text-p1/private/`（1440 宽）。目录属于可重新生成的构建产物；本次重新导出不另记为人工复验。

### 两项修正记录

1. 原 `transforms-1024.png` 在 `x=50` 左对齐绘制 594，再按 `x=80` 起始的区域裁切，造成偏左及左侧缺失。这是样张的裁切示例位置，不是 594 文案或字距错误。新增共用 `TextRenderer::Caption` 入口，按可见字形中心锚定；594 与其他 Caption 完全相同，缩放围绕中心进行。样张已改用裁切框中心，生产浮字原有居中逻辑保留，后续字体迁移复用该入口。
2. Info 原来把灰度节点当作 RGB 浅灰色，配上固定 0.82 Alpha，形成浅色矩形。用户提供的 [透明参考图](asset-reference/info-566-alpha-reference.png) 表明：背景主体是黑色 RGB、Alpha 约 0→51，在白底上合成才呈现 255→204。现改为透明画布上叠加暗色渐变遮罩；底部最后一行同样处理，倒数第二行保留独立灰色高亮层及其淡入 Alpha 包络。三组原灰度节点均保留，不新增底色填充。

## 人工入口

从仓库根目录执行。构建只编译，样张工具不在 CTest 中自动运行：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests

# 开发机已安装字体：默认导出 1440、1024、1279 三种物理像素宽度
.\build\Tenhou.Client.Tests\tenhou_text_samples.exe `
  --output .\build\text-p1\installed --font-source installed

# 独立私有字体路径；读取 ResourceManager 的 TextFont，不依赖系统安装
.\build\Tenhou.Client.Tests\tenhou_text_samples.exe `
  --output .\build\text-p1\private --font-source resources `
  --resources .\build\Tenhou.Client.Tests\resources --width 1440

# 单独生成其他宽度或 DPI 元数据；物理尺寸不会再乘一次 DPI
.\build\Tenhou.Client.Tests\tenhou_text_samples.exe `
  --output .\build\text-p1\dpi144 --font-source resources `
  --resources .\build\Tenhou.Client.Tests\resources --width 1279 --dpi 144
```

每个输出目录包含：

| 文件 | 用途 |
| --- | --- |
| `README.md` | 本次字体来源、粗体实现、固定文案及动态字符缺字列表、缓存占用；生成成功不表示验收通过 |
| `overview-<宽度>.png` | 15 种样式、459、595–602 两态、最长役种、数字单位、浮字缩放与淡出 |
| `overview-<宽度>-bounds.png` | 同一内容加框；绿色为可见字形范围，橙色为设备效果栅格分配范围 |
| `transforms-<宽度>.png` | 红边风位四向变换、轻微倾斜、Info 共用宽度、裁切和 SD 换算原型 |
| `info-layer-<宽度>.png` | 独立透明 RGBA 图，可在画图等工具中直接查看棋盘格与灰度层 |
| `catalog-<宽度>-01.png` 等 | 每页 24 项，共 13 页，覆盖全部 296 个固定 Text；图像条目不混入 |
| `metrics.csv` | 每项 advance、行高、可见轮廓和效果范围，单位为 HD 设计像素 |

人工查看时以图片原始像素尺寸判断清晰度；图片查看器再次缩放会改变观感。`--dpi` 只影响元数据和缓存上下文，不能代替真实窗口的系统 DPI、跨屏与恢复验收。

## 数值校准

确认的文案、字号和灰度节点仍以 [catalog.json](../../Tenhou.Client/assets/text/catalog.json) 为准。用户已完成视觉验证，色板标记 `calibrated: true`，[rendering.json](../../Tenhou.Client/assets/text/rendering.json) 状态更新为 `human_confirmed`，保存本次交付数值。此记录不宣称完整逐像素复刻原参考：

| 参数 | 交付值 | 单位 / 含义 |
| --- | ---: | --- |
| 描边宽度 | 1.0 | pt，共用圆角描边 |
| 缺少原生粗体时的几何加粗宽度 | 0.6 | pt；先生成统一轮廓，再测量与绘制效果 |
| 阴影偏移 X / Y | 1.5 / 2.0 | pt |
| 阴影模糊 | 1.25 | pt，最终尺寸遮罩的三次盒式滤波近似 |
| 阴影 Alpha | 0.65 | 0–1 |
| Info 图层 Alpha 增益 | 1.0 | 保留原生遮罩 Alpha，不为背景填充统一的不透明度 |
| Info 水平 / 垂直边距 | 3.0 / 1.5 | pt |
| Info 底部两条渐变行的各自行高 | 0.75 | pt；三段渐变分别绘制 |
| 459 小字间隔 / 向下偏移 | 0.5 / 3.0 | pt；小字字号仍为已确认的 12 pt |

Info 基准 advance 暂用同一字体、字号和粗体策略下的「国」，列宽为基准 advance × 6.5 加水平边距；所有 Info 共用该宽度。文案超长时不会自动缩小字号，交由调用层裁切并在样张中核对。

主体与最后一行使用黑色遮罩 `Alpha = (255 − 白底合成灰度) / 255`；主体从完全透明逐渐到约 20% 黑色。倒数第二行高亮的 Alpha 包络从参考图提取，节点为 `(0,0)、(0.05,0.016)、(0.15,0.235)、(0.30,0.60)、(0.45,0.894)、(0.48,0.925)、(1,0.925)`；颜色由同一白底合成灰度及 Alpha 反解，维持高亮与透明底的独立性。这不把灰度数值直接当作 Alpha。

修改 JSON 或目录色板后重新构建、导出即可；日常构建不运行 Python 或解析 Excel。P0 的 `null` 是原规格在 P0 时尚未定值的记录，实际交付参数保存于独立 P1 配置，不修改原表追溯字段。

## 人工验收记录

- [x] 用户完成人工样张视觉验证，除以下两项外未报告其他问题；现有色板、粗体、字距、描边、阴影、459 偏移及尺寸参数据此登记。
- [x] 594 偏左的样张原因已定位，已与其他 Caption 共用居中入口。
- [x] Info 按用户透明参考图改为透明底与独立灰度图层合成，并提供透明 PNG 导出。
- [x] 当前交付配置、参考图及输出路径已登记。

验收依据：用户在本次会话明确说明已完成人工视觉验证，仅发现上述两项，修正后 P1 可交付。记录日期：2026-11-17。样张路径：`build/text-p1/installed/`、`build/text-p1/private/`。P1 剩余问题：无；实际窗口、局部重绘与 DPI 验收按 P2 / P6 继续。

## 实现与边界

- [text_renderer.hpp](../../Tenhou.Client/src/text_renderer.hpp) / `.cpp` 提供 `TextFontService`、`TextLayout`、`TextRenderContext`、`SceneTextRun`、`TextRenderer`。GDI+ 生命周期仍由应用或工具拥有，字体服务先于渲染器创建、后于渲染器销毁；接口按 UI 线程使用。
- 私有字体从 `ResourceManager::Read(AssetId::TextFont)` 读入内存并保留到字体集合销毁；只使用仓库已有文件，不自动复制系统字体。找不到指定字体时明确报错，不静默换字体。当前将已有字体登记进资源清单；发布来源和随包使用条件仍须在打包前落实。
- 字号和字距使用 `pt × 96 / 72` 转成 HD 设计像素。上下文分别记录原生布局比例、最终视口比例、物理原点、DPI 和设备裁切；SD 只乘一次 `1 / 1.4`。
- 测量与绘制共用文字轮廓，分别提供 advance、可见 ink 与含效果范围；不以带默认 padding 的字符串矩形充当视觉中心。原生粗体不存在时采用共用几何加粗，样张中记录采用的路径。
- 非零字距由 Uniscribe 字符停止位置分簇，代理对、组合字符、变体选择与 ZWJ 不按单个 UTF-16 码元拆分；簇位置来自整串测量。零字距直接生成整串轮廓。当前范围为目录内中日文及数字混排；非零字距的跨簇复杂连写/连字不作为本阶段完整排版能力保证。输入是单行，最多 4096 UTF-16 码元。
- 阴影遮罩按最终设备尺寸生成；填充、描边、阴影和装饰先合成到透明位图，最后统一应用动画 Alpha。缓存与背景无关，调用层必须先恢复背景再绘制；P2 接入 retained/dirty 层时落实这一顺序。
- 布局 LRU 最多 256 项，栅格 LRU 最多 64 项且不超过 32 MiB；栅格键包含布局、完整设备仿射矩阵的子像素相位、输出尺寸和 DPI。整数平移可复用，裁切每次合成时应用，Alpha 不需重建缓存。单次大图也有分配上限，超出保留预算的栅格绘制后释放。
- `SceneTextRun` 可以按原场景顺序与图像命令交错；本阶段只提供最终尺寸原型。生产层的分层合成、局部重绘、恢复、窗口 DPI 消息和性能回归属于 P2 / P6，不能据此宣布通过。

API 依据：[GDI+ AddString](https://learn.microsoft.com/en-us/windows/win32/api/gdipluspath/nf-gdipluspath-graphicspath-addstring%28constwchar_int_constfontfamily_int_real_constpointf__conststringformat%29)、[私有内存字体](https://learn.microsoft.com/en-us/windows/win32/api/gdiplusheaders/nf-gdiplusheaders-privatefontcollection-addmemoryfont)、[Uniscribe 字符停止分析](https://learn.microsoft.com/en-us/windows/win32/api/usp10/nf-usp10-scriptstringanalyse)。
