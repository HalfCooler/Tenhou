# 原版会话消息采集

使用 Windows 上的 Python 3.11 或更新版本。在仓库根目录安装 Frida 后，用原版启动器打开客户端，停在登录页。先启动脚本并输入 `start`，再在原版窗口登录或复现目标操作。

```powershell
python -m pip install frida==17.27.1
python -X utf8 -B .\scripts\Capture-OriginSession.py capture --raw
python -X utf8 -B .\scripts\Capture-OriginSession.py capture --raw --spawn
```

有多个 `tenhou.exe` 时可加 `--pid 1234`，或在提示符输入 `attach 1234`。采集断线重连（再开）时，原版在收到含 `gpid` 的 `GO` 后会把 `[STATUS] GPID` 写入 `config.ini`；下次启动时若读到该键，会立即清除它并跳过欢迎页自动发起 `<HELO ... gpid="..." .../>`（见 `tenhou.exe.c` 第 62849–63042 行、第 63570–63585 行）。此外，原版 `tenhou.exe` 启动入口会先通过 `FindWindowA("WC_TENHOU_WIN_APP_LAUNCHER", NULL)` 检查启动器窗口，缺失时会直接退出（见 `tenhou.exe.c` 第 61435–61439 行），必须由 `launcher.exe` 拉起（见 `launcher.exe.c` 第 1317–1321 行、第 2692–2696 行）。使用 `--spawn` 或在提示符中输入 `spawn` 会启动原版 `launcher.exe`（仓库内 SHA-256 `5214d0cd37f7a61c5d82962f2f2314b0215e95a47a4ed979d82c8f41c8b0a44c`）并开启 Frida 子进程门控，在启动器拉起 `tenhou.exe` 后核对 `tenhou.exe` 的 SHA-256、于首条指令执行前加载钩子并自动开启记录，再恢复 `tenhou.exe` 运行，从而完整记录再开开头。`--raw` 默认写入 `build/origin-capture/session-raw-*.jsonl`，可用 `--out` 指定新文件。省略 `--raw` 即使用原有的脱敏模式。附加或启动时都会核对原版 `tenhou.exe` 的 SHA-256，客户端更新后需要重新核对观察点。

核对个人资料时，在 `start` 后登录，等大厅资料出现，再切换需要比较的全期间和月间类别。用 `mark` 记录画面和操作，用 `pause` 或 `quit` 结束采集。原始记录及其裁剪件可能含登录 ID 和认证值，分享前请在副本中手工脱敏。

## 记录、打标和裁剪

| 命令 | 用途 |
| --- | --- |
| `start` / `pause` | 开始或暂停记录游戏消息。标记和进程附加／退出事件仍可记录。 |
| `show` | 查看最近 20 条记录及 `#序号`。 |
| `show 12 34` | 查看序号 12 到 34，包含两端及锚定在区间内的备注。 |
| `mark 已点击预约` | 给当时最后一条事件加备注。 |
| `mark @18 已显示大会报名失败` | 把备注锚定到第 18 条事件。 |
| `export 12 34 .\build\origin-capture\contest-cut.jsonl` | 把区间和对应备注保存为新文件，保留原序号和时间。 |
| `detach` / `attach [PID]` | 切换观察进程。原版停在登录页时可重新附加并输入 `start`。 |
| `spawn [启动器路径]` | 启动原版 `launcher.exe` 并通过子进程门控拦截其拉起的 `tenhou.exe`，在首条指令前开启记录并恢复运行，避免再开自动登录时漏采开头。省略路径默认使用仓库内原版 `launcher.exe`。 |
| `quit` | 结束采集，解除附加。 |

采集结束后可以重新打开源文件或裁剪件，或直接把原始 `data_b64` 记录解码为可读文本与二进制帧文件。审阅时新增的备注随 `export` 写入新文件。

```powershell
python -X utf8 -B .\scripts\Capture-OriginSession.py review .\build\origin-capture\contest-cut.jsonl
python -X utf8 -B .\scripts\Capture-OriginSession.py decode .\docs\Tenhou.Client.Origin\origin-capture\一般\ 三食断赤速\ 东风战.jsonl --redact
python -X utf8 -B .\scripts\Capture-OriginSession.py decode .\build\origin-capture\capture-raw.jsonl --out .\build\origin-capture\decoded.txt --bin-dir .\build\origin-capture\frames
```

复现场景时，记录原版窗口上的动作和结果，例如「提交报名」「出现拒绝」「断线后重启」「恢复后可以出牌」。将关键画面或牌谱另存，并在备注里写入文件名。分享前复核备注、截图及另存牌谱中的身份信息。

## 原始记录格式

原始 JSONL 的头部 `format` 为 `tenhou-origin-session-raw/1`。每个 `frame` 事件的 `data_b64` 是完整的原始字节，`byte_length` 是解码后的长度。消息中的字段顺序、空值、引号、百分号编码、高位字节和末尾 `00` 均随字节保留。出站 `send.requested` 和 `send.accepted` 记录请求长度和返回值；短写时 `data_b64` 仍含整个请求缓冲区，失败的返回值为 `-1`。

`show` 以转义字节串显示内容，`export` 原样保留选中帧的 Base64。可在 PowerShell 中把选中的原始帧还原为二进制文件。将示例中的文件名和 `seq` 换成实际值。

```powershell
$frame = Get-Content -Encoding utf8 -LiteralPath .\build\origin-capture\capture-raw.jsonl |
    ForEach-Object { ConvertFrom-Json -InputObject $_ } |
    Where-Object { $_.seq -eq 4 -and $_.kind -eq 'frame' }
[IO.File]::WriteAllBytes((Join-Path $PWD 'build\origin-capture\frame-4.bin'), [Convert]::FromBase64String($frame.data_b64))
```

## 观察点和证据范围

- 原版为 Windows 版 `130/0005`。`tenhou.exe` SHA-256 为 `96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6`，与第 3、4 阶段一致。出站观察原版游戏套接字的 `send`，`0x415ADF–0x415AE8` 扫描 NUL 并以含 NUL 的长度发起调用。入站观察原版完成 NUL 分帧后从 `0x415F48` 发出的 `SendMessageA(0x8209)`。`0x415F12` 比较 `buf[ESI]` 与零，分派返回后 `0x415F51` 用 `ESI + 1` 推进接收缓冲区。原始模式据此复制双向消息及其 NUL。记录包含本机带时区的毫秒时间、方向、序号和进程附加批次。 [出站调用点](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L36925) · [入站分帧与分派](../../Tenhou.Client.Origin/tenhou/tenhou.exe.txt#L37371)
- 脱敏模式在原版进程内先按标签和字段筛选再写出。账号、认证值、玩家名、会员有效期、牌谱标识等仅记录是否存在。`CHAT` 保留房号与普通房切换命令，聊天正文省略。`LN` 保留计数向量编码，`RANKING v2` 和 `RANK v2` 保留数字列表，`RANKING rankingcs` 仅记录是否存在。牌局消息保留牌实体、分数、拔牌、副露及恢复相关数字字段。未识别的消息记为 `UNKNOWN`。两种模式均将读取异常记为 `gap`。
- 出站覆盖原版游戏套接字的发送请求。入站覆盖原版已完成 NUL 分帧并分派的消息。`detach` 表示观察进程退出或解除附加。断线提示和恢复动作可用 `mark` 记录。

## 验证记录

- Python 套件共 19 项通过。采集测试覆盖来源过滤、启停、脱敏模式、原始模式的发送短写和超过 64 KiB 的合成帧，以及原始记录的审阅和带标记裁剪。
- 原版实窗使用同一指纹的 EXE 和隔离的 `NoName` 配置接入本机回环。原始模式按字节对照了双向 `HELO` 和分两次送达的 `LN`，验证额外属性、裸 `&`、百分号编码、高位字节和末尾 NUL。两种模式都通过了暂停及裁剪检查。官方注册账号的非空资料回包仍待用原始模式采集。

已有原版样本见[第 3 阶段官方会话](ORIGIN-STAGE3.md)、[第 4 阶段东天红](ORIGIN-STAGE4.md)和[四麻东南牌局](ORIGIN-RIICHI-SESSION.md)。
