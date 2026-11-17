# 好友麻将桌面客户端

Windows / C++17 / GDI。项目位于 `Tenhou.Client/`，代码位于 `Tenhou.Client/src/`，CMake 工程和 `Build-Tenhou.Client.ps1` 位于项目根目录。本地工具路径由仓库根目录 `Build-Tenhou.ps1` 配置。在仓库根目录执行：

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client
```

四川血战到底和国标已接入共用四面图形牌桌：四川换三张单击选牌并确认、点击万筒索定缺，两种玩法共用单击出牌、拖动理牌、手选多个杠、和牌扣牌和结算。规则和合法动作由服务端提供。国标花牌暂以中央 HUD 的文字显示，补花有浮字和音效。详细状态见 [变体交互设计](VARIANT-INTERACTION.md)。

默认构建 Release，产物复制到 `build/Tenhou.Client/tenhou_rebuild.exe`；使用 `-Configuration Debug` 切换配置。资源统一维护在 `Tenhou.Client/assets/`，以 [资源清单](../../Tenhou.Client/assets/catalog.cmake) 为唯一登记入口（下表路径相对项目根目录）：

| 目录 | 内容 | 发布方式 |
| --- | --- | --- |
| `assets/images/` | 默认背景 BMP | 复制至 EXE 相邻的 `resources/images/` |
| `assets/audio/` | 默认背景音乐 WAV | 复制至 `resources/audio/` |
| `assets/packs/` | 图片、音效及牌面原始 DAT 资源包 | 复制至 `resources/packs/` |
| `assets/dialogs/` | 设置页的原始二进制模板 | 内嵌至 EXE 的 RCDATA |
| `assets/shadows/` | 压缩的牌面阴影数据 | 内嵌至 EXE 的 RCDATA |
| `assets/fonts/` | P1 私有字体文件，来源与随包条件待发布前落实 | 通过 `TextFont` 登记，复制至 `resources/fonts/` |

清单统一生成 `AssetId`、路径元数据和 Win32 RC，并驱动构建复制与安装。新增资源时，在对应目录放入文件并添加一条 `tenhou_asset(...)`；业务代码通过 `ResourceManager` 的 `Path`、`Read` 或 `Embedded` 访问，格式解码仍由原有图片、音频和资源包模块负责。构建时会检查必需文件及重复 ID；仅修改外置资源后重新构建，也会同步更新输出目录。

自行制作四川、国标素材时，参见[图片制作规范与清单](ASSET-AUTHORING.md)：包含原始按钮、动作浮字、役种图片的透明PNG、精确尺寸、字体信息边界和逐项制作CSV。参考图可通过 `scripts/Export-DisplayReferences.ps1` 重新导出；新增PNG仍需登记资源并接入绘制代码。

表内文字图片迁移见 [TrueType 文字渲染实施计划](TRUETYPE-TEXT-PLAN.md)。P0 验证已确认完成，[文字目录与显式导入工具](../../Tenhou.Client/assets/text/README.md) 覆盖 296 Text、7 Image、481 完整样式修正与 Info 继承；代码文字回退已同步目录，普通/悬停按钮按黑边/橙边选择。P1 已经用户人工视觉验证，594 居中与 Info 透明图层两项反馈已修正并完成交付，见 [校准及交付记录](TRUETYPE-TEXT-P1-REVIEW.md)。P2 已接入最终尺寸有序合成与缓存失效，见 [接入记录](TRUETYPE-TEXT-P2-REVIEW.md)。P3 已接入生产牌桌、按钮、四座浮字与开局介绍，见 [P3 接入及验证记录](TRUETYPE-TEXT-P3-REVIEW.md)；结算与资源清理按 P4–P5 继续，实际操作、多 DPI 和跨屏验收按 P6 进行。

`--resources <目录>` 和配置中的 `[REBUILD] ResourceDirectory` 继续指定外置资源根目录。优先查找清单中的分类路径，找不到时兼容旧版平铺文件名，例如 `_res2.dat`。`TENHOU_ASSET_DIRECTORY` 也接受这两种目录布局；内嵌模板始终取自仓库。自定义背景及音频仍按原配置解析。仓库只提供 HD 牌面，清单中的 SD 牌面条目为可选兼容项。

默认大厅 BGM 每次重新开始播放时，从 `_bgm0.wav`、`_bgm1.wav` 等概率独立选择，允许连续重复；大厅状态刷新不重选。`BGM_LOBBY0/1` 的自定义文件和 `none` 继续按对应大厅类型生效，设置页的默认曲目试听保持固定。对局 BGM 沿用 `BGM_GAME*` 配置，未配置时没有默认对局背景音乐。

联网音效统一由设置页“音效音量”（`[RESOURCE] SE_VOL`）控制，0 为静音；不再读取或保存没有界面入口的旧 `[REBUILD] NetworkSound` 开关，已有 `NetworkSound=0` 不会再阻止播放。`SE_NOVOICE` 和单项音效自定义/`none` 保持有效。资源包中的 29 个 WAV 通过别名提供摸打、碰杠、和牌等效果；`SE_GAMEEND`、`SE_LOGIN`、`SE_LOGOUT` 没有内置 WAV，默认试听静音，仍可指定自定义文件。

可单独检查实际资源解码、内嵌模板、旧目录兼容和资源字节一致性：

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client.Tests
./build/Tenhou.Client.Tests/tenhou_resource_check.exe ./build/Tenhou.Client.Tests/resources ./Tenhou.Client/assets
```

先启动 `Tenhou.Server`。默认连接 `127.0.0.1:10080`；可通过配置文件 `[REBUILD] NativeHost`、`NativePort` 连接其他 IPv4 服务器。多人窗口使用不同的 `--config-dir`。

“快捷练习”的玩法来自服务端队列目录。“好友房间”的规则、配置字段、默认值和选项来自服务端；客户端没有预定义官方规则房。服务器目前提供立直麻将、四川血战到底和国标麻将。

三种玩法共用 GDI 牌桌、语音设置、动作播放队列、发牌及摸打动画。四川和国标的打牌后摸牌、杠后补牌、血战退场后继续摸牌使用独立的公开快照，防止后续动作覆盖动画；中间快照不提供可提交操作。Bot 等待及开局准备沿用立直流程。换三张仅允许服务端授予的同花色合法组合。四川和牌后本手退场，其余玩家继续对局至三家和完或牌墙耗尽，国标自动补花并校验八番起和。

注册账号的密码通过服务端哈希保存。访客恢复令牌使用 `platform-session-<端口>-<服务器与账号摘要>.txt` 明文文件，旧 DPAPI 会话文件不导入。注册玩家的立直牌谱可通过 `LOG/PATH` 自动保存；四川和国标完整 JSON 牌谱保存在服务端数据目录的 `games/` 中。

NoName 可直接登录：本地恢复令牌损坏时忽略；服务器找不到旧访客会话、或令牌属于注册账号时，会自动创建新访客会话。有效的访客令牌继续用于断线续局。血战到底与国标的盘末结算在动画完成后，每份收据、每页番种均有 5 秒 OK 倒计时，依次确认后才进入下一盘或返回大厅。

和牌后逐张扣牌并保留和牌张，侧家和牌张与手牌留出独立间隔；盘结束后等待再显示结算。流局先按服务端公开信息逐张亮牌或扣牌，保留先前退场赢家的和牌张。结算开窗和每个番种均有音效，静音、无语音、分座位语音及单项音效覆盖仍生效。恢复快照直接重建当前牌桌，不重播历史动作。排行榜沿用现有立直段位；四川和国标只记录场次与对局结果，不混入立直段位。

四川、国标的图形牌桌阶段交互、资源复用和剩余验收项见[交互草案](VARIANT-INTERACTION.md)。
