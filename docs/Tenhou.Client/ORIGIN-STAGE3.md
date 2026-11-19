# 第 3 阶段 · 原版实窗和官方连接取证

2026-11-19，中国标准时间（UTC+8）。原版为仓库中的 Windows 版 130/0005，运行副本的 `tenhou.exe` SHA-256 为 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`，`launcher.exe` 为 `5214d0cd37f7a61c5d82962f2f2314b0215e95a47a4ed979d82c8f41c8b0a44c`，HD `_res2.dat` 为 `e1994e05dae85587446d53b4da1284706600e2cce292b1ca4109e4b056b7df7f`。地址对应此 EXE 的 PE 虚拟地址。Windows 系统区域为 zh-CN，截图中的部分 ANSI 日文显示为乱码，菜单名结合可访问性文字与 PE 中的 CP932 文案核对。

## 官方 NoName 会话

- **启动与入场**。直接运行 `tenhou.exe` 退出，因为它等待 `WC_TENHOU_WIN_APP_LAUNCHER`。切网前，启动器显示「无法获取更新信息」。切网后，`http://tenhou.net/1/resinfo-release130.txt` 返回 HTTP 200 及 `win130_0124/`，同包启动器拉起哈希一致的 EXE。用独立目录中的 `NoName` 入场，原版游戏连接到 `b.tenhou.net:10080`，收到 `HELO` 和 `LN`。官方首页也把 `NoName` 列为 Windows 版的连通性检查路径。 [启动调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L61435) · [欢迎页](origin-stage3/guest-welcome.jpg) · [官方 Windows 版说明](https://tenhou.net/)
- **排位大厅与普通房**。排位主区显示「有効期限の残りが１日以上必要です」，付款链接使用访客占位符 `ID00000000`，下方保留菜单和试打按钮。用「配信部屋β(無料)」进入 8141 后，窗口显示两列常规预约格，光桌行没有预约按钮。鼠标停在四麻东风格超过 500 ms，右栏保留资料。此时所在会场为普通房，排位规则切换分支在 `0x43C7E6` 检查房号为零。 [排位访客页](origin-stage3/guest-ranked-150.jpg) · [配信部屋](origin-stage3/guest-stream-room-150.jpg) · [原版指令](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L100699)
- **菜单**。主菜单可展开会场移动、显示等子菜单。「ロビーの移動」含排名战、活动会场 1–3、配信部屋、编辑和编号指定。「表示」实窗显示 `Full/Window ALT+Enter` 和最小化。 [主菜单](origin-stage3/guest-main-menu.jpg) · [会场菜单](origin-stage3/guest-lobby-menu.jpg) · [显示菜单](origin-stage3/guest-display-menu.jpg)
- **右栏统计**。全期间下拉显示四麻／三麻段位战、雀庄类别、四麻／三麻个室和说明；在大会中选择段位战三麻，标题实际切到 `■全期間 / 段位戦 ３人打ち`。月间按一般／上级／特上／凤凰及雀庄组织；展开一般，四麻东风／东南各有无食断、有食断、有食断赤，三麻各有一项食断赤。选三麻东南后，月间标题更新，访客数值保持横杠。右上角独立按钮 `0xE104` 打开「テキストで表示」「画像を保存」，对应命令 `0xFC2 / 0xFC3`，文案地址 `0x4F7420 / 0x4F7414`。 [全期间](origin-stage3/guest-all-period-menu.jpg) · [月间](origin-stage3/guest-month-menu.jpg) · [一般卓子菜单](origin-stage3/guest-month-ranked-options.jpg) · [右上角菜单](origin-stage3/guest-profile-menu.jpg)
- **大会与断线**。从 8141 点活动会场一，服务先以 `CHAT lobby="C00112233"` 确认会场，随后约 200 ms 回 `CS`。大厅显示「近代麻雀note杯」，样本配置为会员限定、无报名费、未报名，页面显示会期结束。对原版进程的游戏套接字执行 `shutdown(SD_SEND)` 后，弹出 `CONNECTION CLOSED`，说明重新入场需重新启动。选「否」保留窗口，菜单登出能退到欢迎页，同进程再点 `OK` 的观察期间没有恢复游戏连接。按提示重启并以 `NoName` 入场，原版主动重发同一 `CS` 请求，服务重新确认并发送配置，实窗回到该大会。原版 `STATUS/LOBBY_LAST` 的读写调用点为 `tenhou.exe.c:31741 / 39066 / 39091`。 [大会](origin-stage3/guest-event-1-150.jpg) · [断线提示](origin-stage3/guest-connection-closed.jpg) · [重启再入场](origin-stage3/guest-event-1-restored.jpg)

### 脱敏消息记录

出站观察原版的 Winsock `send`，入站观察原版从 TCP 重组、按 NUL 分帧后在 `0x415F48` 发送的 `0x8209`。只保留涉及入场和公开会场的字段、计数存在性和时间。以下时间来自同一主机的进程观测，精度为毫秒。未记录客户端源地址、端口、聊天正文、认证值及玩家账号资料。 [出站调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L19560) · [入站分帧](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L19871)

| 时间 | 方向 | 选录 |
| --- | --- | --- |
| 14:44:39.892 | 客户端 → 官方 | `CHAT text="%2Flobby%208141"`。实窗聊天状态随后显示 `#ENTER LOBBY 8141`。该次入站的抓取点尚未挂上。 |
| 14:47:05.853 | 客户端 → 官方 | `CS lobby="C00112233"` |
| 14:47:06.026 | 官方 → 客户端 | `CHAT lobby="C00112233"` |
| 14:47:06.030 | 客户端 → 官方 | `PXR V="-1"` |
| 14:47:06.226 | 官方 → 客户端 | `CS players="0" premiumonly="1" joinfee="0" join="0"`，含 `title/rule/csrule`。 |
| 14:47:06.227 | 官方 → 客户端 | `LN` 含 `n`，`j/g` 为空。 |
| 14:47:15.345 | 本地 | 对当前游戏套接字执行 `shutdown(SD_SEND)`，返回 0，随后显示断线提示。 |
| 14:50:01.639 | 重启客户端 → 官方 | `HELO name="NoName" sx="M" tid="w0"` |
| 14:50:01.781 | 官方 → 客户端 | `HELO uname="NoName"`。此轮访客入场的记录中无 `AUTH`。 |
| 14:50:02.519 | 客户端 → 官方 | `CS lobby="C00112233"` |
| 14:50:03.653–03.694 | 双向 | 入站 `LN`、入站 `CHAT lobby="C00112233"`、出站 `PXR V="-1"`、入站 `CS`、入站 `LN`。 |
| 14:50:52.251–52.554 | 双向 | 出站 `CHAT text="%2Flobby%200"`、入站 `CHAT lobby="0000"`、出站 `PXR V="1"`、入站 `LN`。 |

期间观察到空 `PXR` 大约每 15 秒发送一次，数次紧随其后收到 `LN`。上表中的 `PXR V` 来自访客及会场切换样本，尚未核实它与注册账号、访问权限或统计类别的全部关系。

## 官方注册账号会话

同日使用同一指纹的 Windows 版 130/0005 和 HD 资源，在独立运行目录以既有注册账号完成两次登录。系统缩放 150%（144 DPI），正常窗口的 `WC_LOBBY` 为 590×430 物理像素，位于客户区 (425,325)。欢迎页按原版 `STATUS/HIDDENID` 遮盖 ID，由用户点击 `OK`。截图裁到大厅和断线提示所在区域，遮盖玩家名及有效期。回包记录只保留字段名、公开会场配置及客户端类型值。 [ID 遮盖调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L110565)

- **入场与认证**。两次均由 `HELO name=… sx="M" tid="w0"` 发起，约 150 ms 和 185 ms 后收到含 `uname/PF4/PF3/expire/expiredays/ratingscale/opt` 的 `HELO`，随后收到 `LN` 并进入大厅。两轮均未出现 `HELO auth`、出站 `AUTH` 或显式版本拒绝。原版在 `0x462D3C` 读取 `auth`，该分支仍需带挑战值的样本。 [认证调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L63838)
- **排位资格与规则栏**。此账号在一般和上级卓显示预约格，在特上和凤凰卓只显示资格提示。一般卓四麻食断赤东风悬停设 `0x66` 为 500 ms，实测约 506 ms 后显示规则。横移到同行东南格时约 14 ms 后清掉离开计时器 `0x67`，规则内容切换且未重设 `0x66`。移出预约区约 505 ms 后恢复资料。此样本确认的是官方会话中本地预约界面的显隐与规则切换。 [凤凰卓资格提示](origin-stage3/registered-ranked-ineligible-150.png) · [四麻东风规则](origin-stage3/registered-ranked-rule-150.png)
- **全期间与月间**。全期间从四麻切换到三麻，段位、R、名次分布和比率随类别更新。月间从一般卓四麻东风食断赤切到一般卓三麻东南食断赤，标题更新，数据区保留横杠。切月间时发大写 `PXR V="25"`（`0x19`），后续约每 15 秒发 `Z` 和小写 `PXR v="25"`。保持三麻东南，点击上级、特上、凤凰时依次发 `V=153/57/185`，对应叠加 `0x80/0x20/0xA0`。重启回大会后返回排名战，原版恢复三麻全期间、凤凰卓三麻东南月间及凤凰页，恢复后的 `PXR V="185"` 与配置中的 `PROF_LASTTAB` 选择一致。实窗出现全期间非空数值，两轮线上记录均未收到 `RANK v2`。静态分支在 `0x432516–0x432562` 按逗号切分，仅接受 15 或 21 项。 [全期间及空月间](origin-stage3/registered-ranked-profile-150.png) · [月间 PXR 构造](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L34879) · [RANK v2 项数检查](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L80791)
- **大会、断线与返回**。活动会场一先由 `CHAT lobby="C00112233"` 确认，再收到 `CS`。该账号样本的 `premiumonly=1`、`joinfee=0`、`join=1`，实窗显示会员限定和会期结束。进入会场时原版自行发出无属性的 `JOIN`，随后收到大会榜单 `RANKING rankingcs=…`。两次进入均未操作报名或预约，也未发出带玩法的 `JOIN`。对大厅套接字执行 `shutdown(SD_SEND)` 后显示 `CONNECTION CLOSED`。选择「是」后旧进程退出，由启动器重新启动，用户再次登录。客户端根据 `STATUS/LOBBY_LAST` 主动请求同一会场，收到确认后实窗回到该大会。 [大会](origin-stage3/registered-event-1-150.png) · [断线提示](origin-stage3/registered-connection-closed.png) · [重启回场](origin-stage3/registered-event-1-restored-150.png)

### 注册会话脱敏消息记录

沿用出站 Winsock `send` 与入站 `0x415F48` 的观察点。以下时刻为中国标准时间，省略账号、会员有效期、排名内容及认证值。

| 时间 | 方向 | 选录 |
| --- | --- | --- |
| 15:13:16.663–16.813 | 双向 | 出站注册账号 `HELO`，入站含上述字段的 `HELO`。 |
| 15:13:17.598–17.932 | 双向 | 出站 `PXR V="1"`、入站 `LN`、出站 `Z` 和 `PXR v="1"`。 |
| 15:14:24.328–32.540 | 双向 | 切月间后出站 `PXR V="25"`，下一轮轮询发送 `Z` 和 `PXR v="25"`，收到 `LN`。 |
| 15:15:11.825–19.430 | 客户端 → 官方 | 切排位页时依次出站 `PXR V="153"`、`V="57"`、`V="185"`。 |
| 15:15:33.550–35.641 | 双向 | 出站 `CS lobby="C00112233"`、入站 `CHAT lobby="C00112233"`、出站 `PXR V="-1"`、入站 `CS` 与 `LN`。 |
| 15:15:35.642–36.246 | 双向 | 出站 `DATE` 和无属性的 `JOIN`、入站 `RANKING rankingcs=…` 与 `DATE`。 |
| 15:16:12.169 | 本地 | `shutdown(SD_SEND)` 返回 0，随后显示断线提示。 |
| 15:17:18.205–19.527 | 双向 | 重启后 `HELO` 往返、客户端主动发送 `CS lobby="C00112233"`、服务以 `CHAT lobby` 确认并下发 `CS`。 |
| 15:17:50.559–51.669 | 双向 | 手动返回排名战，出站 `CHAT text="%2Flobby%200"`、入站 `CHAT lobby="0000"`、出站 `PXR V="185"`、入站 `LN`。 |

### 2026-11-20 官方注册账号握手

- 会话来自 Windows 版 `130/0005`，`tenhou.exe` SHA-256 为 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`。记录第 3–8 条依次为出站 `HELO`、入站 `HELO`、出站 `PXR V="9"`、入站 `LN`、入站 `RANKING`、出站 `BYE`。入站 `HELO` 保留了 `uname/PF4/PF3/expire/expiredays/ratingscale/opt`，其中 `opt="3"`。采集对 `uname`、`expire` 和该次 `ratingscale` 只保留存在性。重建版日志在发出 `HELO` 后、记录任何入站标签前报 `Unterminated XML entity`，触发这一错误的实际属性仍待确认。 [原版会话](../../build/origin-capture/session-20261120-000258-395948.jsonl)
- 原版的 `HELO` 处理在 `0x431F15` 取 `ratingscale`，在 `0x4323BE` 取 `uname`，两处均调用 `0x4C48C0` 将属性起始标记至结束引号间的内容原样取出。`uname` 随后在 `0x4323F6` 调用百分号解码。重建版的官方入站 `HELO` 按此保留属性原值，再按字段处理。自建服务、出站帧及官方其他标签继续使用 XML 实体解码。此次样本的 `PXR V="9"` 与上文另一注册会话的 `V="1"` 均为实测值，当前修复沿用重建版既有的统计请求值。 [字段提取](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L131317) · [用户名调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L80701)
- 自动化用合成注册回包复现旧版报错。回环以 NUL 分帧，经生产收包路径发送含裸 `&` 和百分号编码的分段 `HELO`，核对出站 `PXR`，随后用 `LN` 和 `RANKING` 验证进入大厅及账号资料。另核对自建 `HELO` 和官方 `CHAT` 的 XML 实体校验，以及标准 `&amp;` 的解码。客户端展示与离线大厅检查通过，生产网络栈用 `NoName` 连接真实官方服务并到达大厅。实窗证据来自上述原版会话，重建版的真实注册账号入口仍待复测。

## 原版排位规则栏与 DPI

官方访客的排位格被有效期提示页覆盖，所以用另一份隔离配置让原版游戏连接指向 `127.0.0.1:18080`。仅把该进程在 `0x416410` 发起的游戏 TCP 连接导向同机监听；原版启动器和 EXE 文件保持原哈希。系统连接表确认游戏套接字连本机。监听端用 `HELO uname="NoName" expire="20991231" expiredays="30"`、`CHAT lobby="0000"`、`LN` 驱动窗口。截图中的日期和计数来自夹具，部分计数因夹具编码显示异常，仅取规则栏和窗体尺寸。

| 实窗动作 | 所见与计时 |
| --- | --- |
| 从资料栏状态移鼠标到四麻东风格 | `SetTimer(0x66, 500)`，约 491 ms 后进入该定时分支，右栏改显示规则。 |
| 从该格跨到另一格 | 离开旧格时设 `0x67`，约 7 ms 后进新格并取消，未重新设 `0x66`。规则标题随新格刷新。 |
| 移出预约区 | `SetTimer(0x67, 500)`，约 498 ms 后进入分支并切回资料。 |

计时取同一进程中 `SetTimer / KillTimer` 回调的时间差，以上为一次实测样本。对应静态位置为 `0x43C845 / 0x43C8CB` 和 `0x43C86D / 0x43C889`，跨格直刷为 `0x43C822–0x43C82E`。 [规则栏](origin-stage3/fixture-ranked-rule-150.jpg) · [移出后资料栏](origin-stage3/fixture-ranked-profile-restored-150.jpg)

本机单屏 3840×2160，系统缩放 150%（144 DPI）。`GetDpiForWindow` 返回 144，窗口 DPI awareness 枚举值为 2（per-monitor aware）。以下是 DPI 感知探针读取的物理像素，原点为顶层客户区左上角。正常窗口的工具截图为 963×746，图中的大厅约 393×287，约为实测物理尺寸的 2/3。

| 状态 | 顶层客户区 | `WC_VIEW` 位置与尺寸 | `WC_LOBBY` 位置与尺寸 |
| --- | --- | --- | --- |
| 正常窗口 | 1440×1080 | (0,0)，1440×1080 | (425,325)，590×430 |
| 最大化 | 3840×2054 | (1200,487)，1440×1080 | (1625,812)，590×430 |

资料与规则两个子窗在两种尺寸下均相对大厅位于 (320,30)，物理尺寸 240×370。最大化时内容画布维持原尺寸并居中，四周留黑。实测覆盖同一 DPI 下的尺寸变化。

## 重建端对照

用 `e4ff7a3` 的重建端在同一 150% 桌面启动 `--offline-lobby`。大厅在截图中同样约 393×287，重建端增加顶层原生菜单，窗口中的大厅整体下移约 25 个截图像素。排位页悬停显示规则摘要，跨格后摘要更新。 [重建端资料栏](origin-stage3/rebuild-offline-profile-150.jpg) · [重建端规则栏](origin-stage3/rebuild-offline-rule-150.jpg)

重建端进入离线 `L8141` 后，悬停同一四麻东风格会显示规则摘要。原版官方 `L8141` 实窗保留资料栏。重建端的按钮悬停条件检查外层页签、切厅状态和按钮可用性，原版条件还检查房号。 [重建端 L8141](origin-stage3/rebuild-offline-stream-rule-150.jpg) · [重建端按钮处理](../../Tenhou.Client/src/offline_lobby_view.cpp#L736)

## 验收边界

- **静态取证**核对了启动器依赖、NUL 分帧、原版会场确认路径、资料菜单命令及规则栏定时器调用点。
- **自动化检查**注册会话取证后重跑原版 P0 的 11 项 Unicorn 指令回放，全部通过，范围包括排位映射、资格显隐、计数槽、跨页预约、切厅确认及右栏定时分支。访客会话取证时运行的重建端 6 项 CTest 通过，覆盖离线大厅、文字合成与目录、玩法变体、对局呈现和资源。
- **实窗与官方回包**覆盖 Windows 原版 150% DPI 的访客及注册账号入场、菜单、注册账号全期间实绩和空月间、排位资格显示、免费普通房、大会确认、受控关闭连接和重启回场。原版排位规则时序同时由回环夹具和官方注册会话覆盖。带 `auth` 的认证挑战、版本不兼容及登录失败分支、官方 `RANK v2` 及非空月间样本、付费和开放会期的实际报名／预约、对局断线恢复以及其他 DPI 仍待验证。
