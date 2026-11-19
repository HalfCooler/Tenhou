# 第 4 阶段 · 东天红和联合回归

2026-11-19，中国标准时间（UTC+8）。原版实窗使用 Windows 版 130/0005、150% DPI 和独立的 `NoName` 配置。运行副本的 `tenhou.exe` SHA-256 为 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`，HD `_res2.dat` 为 `e1994e05dae85587446d53b4da1284706600e2cce292b1ca4109e4b056b7df7f`，与仓库原版一致。重建端基于 `gdi-opti` 的 `e4ff7a3` 加本次直播房悬停修正。

现有玩法联合回归通过。东天红已有原版试打、拔牌补牌和流局终局样本。正式和牌计分、完整牌谱与正式对局恢复仍待验证，重建端官方入口继续关闭。

## 原版实窗和静态取证

### 试打、牌山与拔牌

- 原版试打菜单末项为「東天紅 ３局 サンマβ」，PE 文案地址 `0x4F8C2C`。菜单命令 `0x874` 在 `0x43B28E` 减去 100 后作为玩法参数发送，第二次试打在排名场实测出站 `JOIN t="0,2064"`，入站 `GO type="2064"`，即 `0x810`。预约首格的 `0x811` 另由 `0x43AF78` 产生。两次试打都从访客大厅进入，同桌两席为 COM。 [原版菜单](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L98866) · [试打分派](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L99062) · [试打开局](origin-stage4/original-practice-east1.jpg)
- 牌山呈现路径在三麻且 `type & 0x800` 时使用 112 张，恢复路径对 136 个可视位置取 24 张偏移。原版宝牌查表 `0x4FE198` 和 `FUN_0049C5A0` 在东天红拔牌计数中把 1 万指示牌映到 5 万、5 万映到 9 万、9 万映到 1 万。牌山的完整实体集合和补牌取牌顺序尚缺全山样本。 [牌山呈现](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L94455) · [恢复位置偏移](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L97537) · [宝牌计数映射](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L103654)
- 原版 `0x48A4AF` 由 `type & 2` 设置赤牌绘制开关。开关开启时，`FUN_00481A20` 将实体 ID 16、52、88 映到赤五万、赤五筒、赤五索；其余五牌实体走普通牌面的映射。第二次试打同屏显示普通五万 19 和赤五万 16。此前的六赤设想缺少该版证据。 [赤牌开关](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L92243) · [实体到图面](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L85945)
- P0 指令回放覆盖 1 万、5 万、9 万和北的候选生成。第一次试打实窗同时出现 5 万、9 万和北的「抜く」按钮，东二实际拔出九万 34，收到 `N who="0" m="8736"` 和补摸 `T135`。第二次试打实际拔出赤五万 16，收到 `N who="0" m="4128"` 和补摸 `T104`。两笔确认的 `m` 都是 `(实体 ID << 8) | 0x20`。补牌后实窗显示拔出的牌，赤五万样本的计数控件为 `0x1+2`。原版显示代码以拔牌数为起点，ID 16 额外加一，收齐同种四张额外加四，并加入对应宝牌计数。实际补牌样本覆盖九万和赤五万。 [拔牌候选](origin-stage4/original-practice-nuki-options.jpg) · [九万拔牌](origin-stage4/original-practice-nuki-completed.jpg) · [赤五万拔牌](origin-stage4/original-practice-red-five-nuki.jpg) · [原版显示计算](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L103568)

### 结算、牌谱与恢复

- 第一桌三席各 8000 点，东一和东二流局后点击 `OK` 继续。东三显示末局标识，流局回包带 `owari`，结算按钮变为 `END`。点击后出现三席均为 8000 点、结果均为 0.0 的终局窗，确认后返回大厅。样本覆盖三局零点变动的终局。和牌、听牌罚点、点与枚的支付，以及提前结束条件仍需样本。原版结算绘制按 `0x800` 选取点数单位并单独绘制枚数。 [东一流局](origin-stage4/original-practice-draw.jpg) · [东三流局及 END](origin-stage4/original-practice-final-draw.jpg) · [终局窗](origin-stage4/original-practice-end.jpg) · [原版结算绘制](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L120584)
- 第二桌在已开局且已拔牌后结束本次取证进程，同一测试目录经原版启动器重启，再用 `NoName` 登录。登录往返后出现大厅，观察 60 秒以上未收到 `GO`、`SAIKAI` 或 `REINIT`。此结果限于访客试打的进程中断。本次未取得完整的东天红牌谱或正式桌恢复样本。
- 将观测到的 `GO` 和一条 `INIT` 封装成最小 XML 探针，重建端按预期在首个 `GO` 拒绝特殊三麻。重建端离线试打菜单仍禁用东天红，线上入口也有同一玩法检查。 [试打菜单](../../Tenhou.Client/src/offline_lobby_view.cpp#L481) · [牌谱类型检查](../../Tenhou.Client/src/replay.cpp#L297) · [线上类型检查](../../Tenhou.Client/src/online_game.cpp#L119)

### 脱敏回包选录

出站观察原版的 Winsock `send`，入站观察原版在 `0x415F48` 的 NUL 分帧分派。探针仅记录试打标签及数字字段，省略身份、连接地址、牌谱标识和其他未列字段。表中均为原版官方试打样本。 [原版出站调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L19560) · [原版入站分帧](../../Tenhou.Client.Origin/tenhou/tenhou.exe.c#L19871)

| 时间 | 方向 | 消息或动作 |
| --- | --- | --- |
| 15:33:48.990 | 入站 | `RYUUKYOKU sc="80,0,80,0,0,0,80,0"`，东一流局。 |
| 15:33:57.219–57.360 | 双向 | `NEXTREADY` 后收到 `INIT seed="1,1,0,5,1,16" ten="80,80,0,80" oya="0"`，进入东二一本场。 |
| 15:34:12.240–12.546 | 双向 | `N type="10" hai="34"` → `N who="0" m="8736"` → `T135`。 |
| 15:35:47.500–15:35:57.123 | 双向 | 东二 `RYUUKYOKU sc="80,0,80,0,0,0,80,0"`，确认后收到 `INIT seed="2,2,0,1,3,103"`，进入东三二本场。 |
| 15:38:46.341 | 入站 | `RYUUKYOKU sc="80,0,80,0,0,0,80,0" owari="80,0.0,80,0.0,0,0.0,80,0.0"`。 |
| 15:40:11.594–12.406 | 双向 | `JOIN t="0,2064"` → `GO type="2064"` → `GOK` → `TAIKYOKU`。 |
| 15:40:23.694–24.224 | 双向 | `NEXTREADY` → `INIT seed="0,0,0,4,2,97" ten="80,80,80,0" oya="0"` → `T16 t="128"`。 |
| 15:40:35.262–35.515 | 双向 | `N type="10" hai="16"` → `N who="0" m="4128"` → `T104 t="128"`。 |
| 15:42:34.703–34.846 | 双向 | 中断后重启，访客 `HELO` 往返，随后实窗回到大厅。 |

## 现有玩法联合回归

| 检查 | 结果与覆盖 |
| --- | --- |
| Release 全量构建 | `Build-Tenhou.ps1 -NativeReference` 通过，四项目和冻结原生规则对照程序均已生成。改动后重建客户端通过。 |
| Python 与 CTest | 16 项 Python 检查、6 项客户端 CTest 通过。覆盖原版 P0 指令回放、离线大厅模型、文字、牌桌呈现及资源。 |
| 服务端和真实客户端协议 | 带 `--oracle` 与 `--native-client` 的 Release 套件通过。标准立直三麻、四麻各六个种子和逐事件规则对照，四川、国标各四个完整样本，国标 130 个计分样例。真实 C++ 客户端跑完三类快捷队列，并验证四麻、三麻各自东风／东南的排位交接。覆盖四人好友房、无效会话令牌后的访客新建、现有访客的重启恢复、排位预约竞态与大会结算和归档传输。 |
| 重建端实窗 | 根据原版 `0x43C7E6` 的零房号判断修正规则栏触发条件。离线 `L0000` 悬停超过 500 ms 后显示规则，切到 `L8141` 后同格悬停超过 500 ms 始终显示资料。标准三麻自建服归档用 `--replay` 打开，从第 0 步跳至 503/503，再回到 502/503。 [排位规则](origin-stage4/rebuild-ranked-rule.jpg) · [直播房资料](origin-stage4/rebuild-stream-profile.jpg) · [原版房号判断](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L100699) · [重建端悬停条件](../../Tenhou.Client/src/offline_lobby_view.cpp#L741) |
| 资源与声音 | 解包核对发布的 `_res2.dat`，图像 ID 401、402、440、466、589、591、592 的尺寸和存储内容均与原版一致，29 条 WAV 记录的 ID 和内容全部一致。资源测试解码两首 BGM，`--play-audio` 的六项音效均收到 waveOut 完成回调。离线实窗从直播房切到活动会场一，日志确认 `_bgm0.wav` 淡出停止，`_bgm1.wav` 从头循环。听感未验收。 |

在仓库根目录复跑联合回归。P0 指令回放使用 `unicorn==2.1.4`。

```powershell
.\Build-Tenhou.ps1 -NativeReference
python -B -m unittest discover -s scripts/tests -v
ctest --test-dir .\Tenhou.Client.Tests\obj\native\Release -C Release --output-on-failure
dotnet .\build\Tenhou.Server.Tests\Tenhou.Server.Tests.dll --oracle .\build\Tenhou.Server.Tests\tenhou_rule_oracle.exe --native-client .\build\Tenhou.Client.Tests\tenhou_protocol_check.exe
.\build\Tenhou.Client.Tests\tenhou_resource_check.exe .\build\Tenhou.Client.Tests\resources .\Tenhou.Client\assets --play-audio
```

## 官方入口

正式开放仍依赖东天红规则、牌谱与恢复的待验证项，以及重建端官方协议闭环。当前欢迎页会拦截官方登录。第 3 阶段的官方原版样本和本次原版试打均不能覆盖重建端的官方登录、会场准入、开桌及对局重连。 [官方登录闸门](../../Tenhou.Client/src/application.cpp#L380) · [第 3 阶段证据](ORIGIN-STAGE3.md)
