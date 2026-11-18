# 好友麻将桌面客户端

Windows / C++17 / GDI。项目位于 `Tenhou.Client/`，代码位于 `Tenhou.Client/src/`，CMake 工程和 `Build-Tenhou.Client.ps1` 位于项目根目录。本地工具路径由仓库根目录 `Build-Tenhou.ps1` 配置。在仓库根目录执行：

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client
```

四川血战到底和国标已接入共用四面图形牌桌。四川换三张单击选牌并确认、点击万筒索定缺，两种玩法共用单击出牌、拖动理牌、手选多个杠、和牌扣牌和结算。规则和合法动作由服务端提供。国标花牌显示在牌桌四角，补花有浮字和音效。详见 [国标与八花](GUOBIAO.md)。

默认构建 Release，产物复制到 `build/Tenhou.Client/tenhou_rebuild.exe`；使用 `-Configuration Debug` 切换配置。资源统一维护在 `Tenhou.Client/assets/`，以 [资源清单](../../Tenhou.Client/assets/catalog.cmake) 为唯一登记入口（下表路径相对项目根目录）：

| 目录 | 内容 | 发布方式 |
| --- | --- | --- |
| `assets/images/` | 默认背景 BMP、八花 PNG 和 7 个图像编号的可选 PNG 覆盖 | 复制至 EXE 相邻的 `resources/images/` |
| `assets/audio/` | 默认背景音乐 WAV | 复制至 `resources/audio/` |
| `assets/packs/` | 图形与音效 DAT、原始牌面 DAT | 复制至 `resources/packs/` |
| `assets/dialogs/` | 设置页的原始二进制模板 | 内嵌至 EXE 的 RCDATA |
| `assets/shadows/` | 压缩的牌面阴影数据 | 内嵌至 EXE 的 RCDATA |
| `assets/fonts/` | 私有字体文件，来源与随包条件待发布前落实 | 通过 `TextFont` 登记，复制至 `resources/fonts/` |

清单统一生成 `AssetId`、路径元数据和 Win32 RC，并驱动构建复制与安装。新增资源时，在对应目录放入文件并添加一条 `tenhou_asset(...)`；业务代码通过 `ResourceManager` 的 `Path`、`Read` 或 `Embedded` 访问，格式解码仍由原有图片、音频和资源包模块负责。构建时会检查必需文件及重复 ID；仅修改外置资源后重新构建，也会同步更新输出目录。

修改按钮、动作浮字、玩法信息或番型时，按 [TrueType 文字目录维护说明](../../Tenhou.Client/assets/text/README.md) 编辑文案、字号和样式。目录登记 296 个文字条目，界面按需绘制。

随客户端发布的 `packs/_res2.dat` 包含 29 个音效和 401、402、440、466、589、591、592 七个图形。可在 `images/sprites/` 提供同编号 PNG 作为可选资源。增量构建及安装会清除输出目录中旧清单登记的文字 PNG。迁移和检查记录见 [TrueType 实施计划](TRUETYPE-TEXT-PLAN.md) 和 [P5 记录](TRUETYPE-TEXT-P5-REVIEW.md)。

`--resources <目录>` 和配置中的 `[REBUILD] ResourceDirectory` 继续指定外置资源根目录。优先查找清单中的分类路径，找不到时兼容旧版平铺文件名，例如 `_res2.dat`。`TENHOU_ASSET_DIRECTORY` 也接受这两种目录布局；内嵌模板始终取自仓库。自定义背景及音频仍按原配置解析。仓库只提供 HD 牌面，清单中的 SD 牌面条目为可选兼容项。

默认大厅 BGM 每次重新开始播放时，从 `_bgm0.wav`、`_bgm1.wav` 等概率独立选择，允许连续重复；大厅状态刷新不重选。`BGM_LOBBY0/1` 的自定义文件和 `none` 继续按对应大厅类型生效，设置页的默认曲目试听保持固定。对局 BGM 沿用 `BGM_GAME*` 配置，未配置时没有默认对局背景音乐。

联网音效统一由设置页“音效音量”（`[RESOURCE] SE_VOL`）控制，0 为静音；不再读取或保存没有界面入口的旧 `[REBUILD] NetworkSound` 开关，已有 `NetworkSound=0` 不会再阻止播放。`SE_NOVOICE` 和单项音效自定义/`none` 保持有效。资源包中的 29 个 WAV 通过别名提供摸打、碰杠、和牌等效果；`SE_GAMEEND`、`SE_LOGIN`、`SE_LOGOUT` 没有内置 WAV，默认试听静音，仍可指定自定义文件。

可单独检查实际资源解码、内嵌模板、旧目录兼容和资源字节一致性：

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client.Tests
./build/Tenhou.Client.Tests/tenhou_resource_check.exe ./build/Tenhou.Client.Tests/resources ./Tenhou.Client/assets
```

先启动 `Tenhou.Server`，在登录界面选择“第三方服务器”。默认地址为 `localhost:10080`，可以填写其他主机和端口。配置沿用 `[REBUILD] NativeHost`、`NativePort`。多人窗口使用不同的 `--config-dir`。

“天凤官方”入口接受 `IDxxxxxxxx-xxxxxxxx` 格式的 19 位 ID，官方连接和对局大厅留待后续实现。

“人机场”的玩法来自服务端队列目录。“房间场”的规则、配置字段、默认值和选项来自服务端。服务器目前提供立直麻将、四川血战到底和国标麻将。两个场共用 `_bgm0.wav`，新开播时从曲内偏移淡入。

三种玩法共用 GDI 牌桌、语音设置、动作播放队列、发牌及摸打动画。四川和国标的打牌后摸牌、杠后补牌、血战退场后继续摸牌使用独立的公开快照，防止后续动作覆盖动画；中间快照不提供可提交操作。Bot 等待及开局准备沿用立直流程。换三张仅允许服务端授予的同花色合法组合。四川和牌后本手退场，其余玩家继续对局至三家和完或牌墙耗尽，国标自动补花并校验八番起和。

注册账号的密码通过服务端哈希保存。访客恢复令牌使用 `platform-session-<端口>-<服务器与账号摘要>.txt` 明文文件，旧 DPAPI 会话文件不导入。注册玩家的立直牌谱可通过 `LOG/PATH` 自动保存；四川和国标完整 JSON 牌谱保存在服务端数据目录的 `games/` 中。

NoName 可直接登录：本地恢复令牌损坏时忽略；服务器找不到旧访客会话、或令牌属于注册账号时，会自动创建新访客会话。有效的访客令牌继续用于断线续局。血战到底与国标的盘末结算在动画完成后，每份收据、每页番种均有 5 秒 OK 倒计时，依次确认后才进入下一盘或返回大厅。

和牌后逐张扣牌并保留和牌张，侧家和牌张与手牌留出独立间隔；盘结束后等待再显示结算。流局先按服务端公开信息逐张亮牌或扣牌，保留先前退场赢家的和牌张。结算开窗和每个番种均有音效，静音、无语音、分座位语音及单项音效覆盖仍生效。恢复快照直接重建当前牌桌，不重播历史动作。排行榜沿用现有立直段位；四川和国标只记录场次与对局结果，不混入立直段位。
