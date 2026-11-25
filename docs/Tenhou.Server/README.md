# Tenhou.Server

.NET 11 / C# 15 跨平台服务端（显式 `using`、nullable、无 `unsafe`、无原生/Windows API 依赖），代码位于 `Tenhou.Server/src/`。

## 构建与运行

```powershell
.\Build-Tenhou -Target Tenhou.Server
dotnet run --project Tenhou.Server -c Release -- --config Tenhou.Server/server.json

# 优雅停止（也可 Ctrl+C 或 SIGTERM）
dotnet run --project Tenhou.Server -c Release -- --config Tenhou.Server/server.json --stop

# 独立发布
dotnet publish Tenhou.Server -c Release -r win-x64 --self-contained true -o build/server-win-x64
```

- 省略 `--config` 时读取程序目录 `server.json`；`dataDirectory` 相对配置文件目录解析（支持中文路径）。
- `--stop` 通过按数据目录区分的本机命名管道通知对应实例保存并退出；Windows 另提供 `start-server.cmd` / `stop-server.cmd`（支持 `-ConfigurationFile`、`-Configuration Debug`）。
- 源码无 Windows 依赖，但当前仅在 Windows 完成验收，其他平台需自行跑测。

## 配置

`server.json` 支持 `listen`（默认 `127.0.0.1`）、`port`（默认 `10080`）、`dataDirectory`、`maxConnections`、`maxTables`、`botDelayMs`、`matchWaitMs`、`queues`、`contests`。

```json
{
  "listen": "127.0.0.1",
  "port": 10080,
  "dataDirectory": "data",
  "queues": [
    { "id": 1, "name": "好友立直四人一局", "rule": "riichi", "options": { "length": "single" } },
    { "id": 2, "name": "血战到底半庄", "rule": "sichuan-bloodbattle", "options": {} },
    { "id": 3, "name": "国标练习", "rule": "guobiao", "options": { "hands": "1" } }
  ]
}
```

- **匹配规则**：快捷练习为单真人 + 人机；好友房满员即开桌，否则等待 `matchWaitMs` 后补人机。队列不跨重启恢复。
- **自建服务大厅**：客户端带 `--service-lobby` 连接时进入原版布局大厅，按会场 ID 与玩法掩码排队，满 3/4 真人开桌；`contests` 可配置大会（详见 [PROTOCOL.md](PROTOCOL.md#自建服务大厅)）。

## 数据与状态

- `state.json`：账号、房间、恢复会话、进行中对局与结算标记。
- `games/<桌号>.xml` / `.json`：已完成牌谱（立直 XML，四川/国标 JSON）。
- `game-<桌号>.json`：含牌山与动作的可重放记录。
- `events.jsonl`：启动、连接与牌桌生命周期日志。
- `server.lock`：数据目录独占锁。

存档均为明文（密码存 PBKDF2-HMAC-SHA256 哈希），写入经临时文件刷盘替换；若 JSON 损坏或无法重放则启动失败，不自动清空。真人断线会暂停整桌直至恢复；退出规则为单真人直接退、多真人 30 秒过半投票，立直排位禁退；仅全注册真人立直好友局计入段位。

## 相关文档

- [架构](ARCHITECTURE.md) · [规则](RULES.md) · [协议](PROTOCOL.md) · [验证记录](VALIDATION.md) · [第三方许可](THIRD-PARTY-NOTICES.md)
