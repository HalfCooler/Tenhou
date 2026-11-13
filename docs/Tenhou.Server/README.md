# Tenhou.Server

项目目录 `Tenhou.Server/`，代码集中在 `Tenhou.Server/src/`，项目构建入口为 `Tenhou.Server/Build-Tenhou.Server.ps1`。根命名空间与程序集名 `Tenhou.Server`。使用 .NET 11 / C# 15，显式 `using`、启用 nullable、禁止 unsafe，不加载 C++ DLL，不依赖 Windows 专用 API。

## 构建和运行

在仓库根目录执行：

```powershell
.\Build-Tenhou.ps1 -Target Tenhou.Server
dotnet run --project Tenhou.Server -c Release -- --config Tenhou.Server/server.json
```

配置文件路径及数据目录支持中文；数据目录相对配置文件所在目录解析。省略 `--config` 时读取程序目录中的 `server.json`。监听地址为显式 IPv4，默认 `127.0.0.1:10080`。

前台 Ctrl+C、Unix SIGTERM 或下面的命令会停止接收并保存状态：

```powershell
dotnet run --project Tenhou.Server -c Release -- --config Tenhou.Server/server.json --stop
```

`--stop` 通过当前操作系统用户的本机命名管道请求停止，管道按数据目录区分实例。Windows 还提供 `start-server.cmd`、`stop-server.cmd`，可传 `-ConfigurationFile <绝对路径>`、`-Configuration Debug`。

独立发布示例：

```powershell
dotnet publish Tenhou.Server -c Release -r win-x64 --self-contained true -o build/server-win-x64
```

纯服务端源码不依赖 Windows，但本次验证环境为 Windows；其他操作系统需要在目标环境运行测试。

## 配置

`server.json` 支持 `listen`、`port`、`dataDirectory`、`maxConnections`、`maxTables`、`botDelayMs`、`matchWaitMs`。

可通过 `queues` 数组替换快捷练习目录，每项包含 `id`、`name`、`rule`、`options`。ID 是服务器自己的队列 ID，与客户端呈现用的立直位标志分离。例如：

```json
{
  "listen": "127.0.0.1",
  "port": 10080,
  "dataDirectory": "data",
  "queues": [
    { "id": 1, "name": "好友立直四人一局", "rule": "riichi", "options": { "length": "single" } },
    { "id": 2, "name": "血流成河一盘", "rule": "sichuan-bloodflow", "options": { "hands": "1" } },
    { "id": 3, "name": "国标练习", "rule": "guobiao", "options": { "hands": "1" } }
  ]
}
```

快捷练习为单真人加人机。好友房满员立即开桌；未满员则从最早排队者开始等待 `matchWaitMs` 后补人机。队列不跨服务重启恢复。

## 数据

- `state.json`：账号、房间、恢复会话、进行中的对局和结算标记。
- `games/<桌号>.xml`：完成的立直牌谱；`games/<桌号>.json`：完成的四川/国标牌谱。
- `game-<桌号>.json`：包含牌山和动作的可重放记录。
- `events.jsonl`：启动、连接和牌桌生命周期事件。
- `server.lock`：实例对数据目录的独占锁。

所有存档都是明文。密码保留 PBKDF2-HMAC-SHA256 哈希，凭据盐由标准密码学 API 产生；游戏洗牌、房间分配使用 `System.Random`。保存实际牌山，恢复不依赖运行时随机数序列。

状态写入临时文件，刷盘后替换主文件。损坏的 JSON、未知版本或无法重放的记录会令启动失败；不会自动清空。旧 version26/27 加密测试状态与旧客户端会话不迁移。

真人断线会暂停整桌，登录恢复后继续。退出采用单真人直接退出、多真人30秒严格过半投票；立直排位桌禁止中途退出。全注册真人立直好友局计入现有段位，其余玩法不计立直段位。结算标记与账号场次在同一快照提交。

服务端部署目录中的 `docs/` 随附这些文档及参考实现许可。详细设计见 [ARCHITECTURE.md](ARCHITECTURE.md)，玩法约定见 [RULES.md](RULES.md)，消息见 [PROTOCOL.md](PROTOCOL.md)，验证记录见 [VALIDATION.md](VALIDATION.md)，参考实现许可见 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)。
