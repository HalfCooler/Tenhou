# 好友麻将桌面客户端

Windows / C++17 / GDI。项目位于 `Tenhou.Client/`，代码位于 `Tenhou.Client/src/`，CMake 工程和 `Build-Tenhou.Client.ps1` 位于项目根目录。本地工具路径由仓库根目录 `Build-Tenhou.ps1` 配置。在仓库根目录执行：

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client
```

默认构建 Release，产物复制到 `build/Tenhou.Client/tenhou_rebuild.exe`；使用 `-Configuration Debug` 切换配置。资源统一维护在 `Tenhou.Client/assets/`，以 [资源清单](../../Tenhou.Client/assets/catalog.cmake) 为唯一登记入口（下表路径相对项目根目录）：

| 目录 | 内容 | 发布方式 |
| --- | --- | --- |
| `assets/images/` | 默认背景 BMP | 复制至 EXE 相邻的 `resources/images/` |
| `assets/audio/` | 默认背景音乐 WAV | 复制至 `resources/audio/` |
| `assets/packs/` | 图片、音效及牌面原始 DAT 资源包 | 复制至 `resources/packs/` |
| `assets/dialogs/` | 设置页的原始二进制模板 | 内嵌至 EXE 的 RCDATA |
| `assets/shadows/` | 压缩的牌面阴影数据 | 内嵌至 EXE 的 RCDATA |

清单统一生成 `AssetId`、路径元数据和 Win32 RC，并驱动构建复制与安装。新增资源时，在对应目录放入文件并添加一条 `tenhou_asset(...)`；业务代码通过 `ResourceManager` 的 `Path`、`Read` 或 `Embedded` 访问，格式解码仍由原有图片、音频和资源包模块负责。构建时会检查必需文件及重复 ID；仅修改外置资源后重新构建，也会同步更新输出目录。

`--resources <目录>` 和配置中的 `[REBUILD] ResourceDirectory` 继续指定外置资源根目录。优先查找清单中的分类路径，找不到时兼容旧版平铺文件名，例如 `_res2.dat`。`TENHOU_ASSET_DIRECTORY` 也接受这两种目录布局；内嵌模板始终取自仓库。自定义背景及音频仍按原配置解析。仓库只提供 HD 牌面，清单中的 SD 牌面条目为可选兼容项。

可单独检查实际资源解码、内嵌模板、旧目录兼容和资源字节一致性：

```powershell
./Build-Tenhou.ps1 -Target Tenhou.Client.Tests
./build/Tenhou.Client.Tests/tenhou_resource_check.exe ./build/Tenhou.Client.Tests/resources ./Tenhou.Client/assets
```

先启动 `Tenhou.Server`。默认连接 `127.0.0.1:10080`；可通过配置文件 `[REBUILD] NativeHost`、`NativePort` 连接其他 IPv4 服务器。多人窗口使用不同的 `--config-dir`。

“快捷练习”的玩法来自服务端队列目录。“好友房间”的规则、配置字段、默认值和选项来自服务端；客户端没有预定义官方规则房。服务器目前提供立直麻将、四川血流成河和国标麻将。

立直使用现有 GDI 牌桌。四川和国标使用通用牌桌：左侧显示玩家、手牌、牌河、副露、花牌和分数；右侧列表显示服务器授予的合法操作，选择后点击执行或双击。换三张的选项仅包含同一花色的合法组合；定缺时选择花色。四川和牌后继续对局，国标自动补花并校验八番起和。

注册账号的密码通过服务端哈希保存。访客恢复令牌使用 `platform-session-<端口>-<服务器与账号摘要>.txt` 明文文件，旧 DPAPI 会话文件不导入。注册玩家的立直牌谱可通过 `LOG/PATH` 自动保存；四川和国标完整 JSON 牌谱保存在服务端数据目录的 `games/` 中。

当前通用牌桌采用文字牌面和操作列表，尚未使用立直牌桌的动画及语音资源。排行榜沿用现有立直段位；四川和国标只记录场次与对局结果，不混入立直段位。
