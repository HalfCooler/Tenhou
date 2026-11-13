# 好友平台协议 v1

传输为 `TcpListener` / TCP、严格 UTF-8、NUL 结尾的扁平 XML 消息。单帧最多 64 KiB；客户端必须处理拆包和粘包。DTD、嵌套节点、重复属性、无效 UTF-8 和超限帧均拒绝。连接有握手、半帧、空闲及发送超时；发送队列与接收事件队列都有边界。

保留 XML 与立直事件编码是为了复用现有客户端牌桌。平台版本与官方服务无关：`HELO uname="TENHOU-PLATFORM" version="1"`；不仿造官方认证。

## 登录与目录

1. 客户端发送 `HELO name="NoName" sx="M"`，或以注册账号名登录。
2. 收到平台 `HELO` 后发送 `CAPS version="1" token="..."`，账号登录附带 `password`，注册附带 `register="1" nickname="..."`。
3. 服务端返回 `SESSION token="..." rid="..."`，然后下发目录与大厅。

目录以 `CATALOGBEGIN` 开始、`CATALOGEND` 结束，中间为：

- `RULE id/name/presentation/version`。
- `RULEFIELD rule/key/label/default/min/max/step`。
- `RULECHOICE rule/key/value/label`。
- `QUEUEDEF id/name/rule/type`。

`type` 只服务于旧立直呈现的位标志，不能用作队列 ID。新增玩法使用 `type="0"`。客户端可发送 `CATALOG` 重新请求目录。

## 房间与排队

`ROOMCREATE rid/rule/...选项`、`ROOMJOIN rid/room`、`ROOMLEAVE rid`、`ROOMCLOSE rid`、`SETROOM rid/revision/rule/...选项`。

`rid` 在会话内单调递增。重复的相同请求返回原 `ACCEPT` 或 `REJECT`；同一 ID 携带不同内容无效。房间号由服务器分配，`ROOM` 包含规则、归一化设置、revision、owner 和呈现 type。修改需匹配 revision。

`JOIN t="0,队列ID"` 进入快捷练习；`JOIN t="房间号,0" revision="..."` 在好友房排队。收到 `QUEUED room/type` 后等待开桌。`JOIN` 无参数取消排队；随后的 `CHAT text="%2Flobby%20房间号"` 请求大厅确认。

## 对局

立直使用 `UN/GO/INIT`、摸打/副露/和牌事件，以及既有 `D/N/REACH` 操作。GOK、STARTREADY、NEXTREADY 是幂等准备屏障；READYACK 确认提交，等待其他玩家时不再触发客户端动作超时。

四川和国标使用：

- `VGO table/rule/seat/n0..n3/exit`：对局身份与绝对座位。
- `VSTATE`：当前视角的手牌、公开区域、分数、阶段、结果历史和 `actions`。
- `actions="kind:tile,tile;kind:tile;..."`：仅包含该玩家可提交的操作。`dingque` 参数是花色0万/1筒/2索，其余参数是实体牌 ID。
- `VACT table/window/kind/tiles`：选择服务器授予的操作。

基础牌 ID 与现有资源一致：`0..35`万、`36..71`筒、`72..107`索、`108..123`东南西北、`124..127`白、`128..131`发、`132..135`中；国标花牌 `136..143` 为梅兰竹菊春夏秋冬。

`phase="exchange"` 等待换三张，`missing` 等待定缺，`turn` 为摸打阶段，`claims` 等待响应，`ended` 等待 NEXTREADY，`complete` 可返回大厅。血战和牌后 `retired0..3="1"` 标记本手退场，其 `actions` 为空；尚有两家以上未和时继续对局，三家和完即进入 ended/complete。`exchange_direction` 为 clockwise/counterclockwise/opposite/none，按八局局序确定。

`CLOCK ms/base_ms` 为剩余动作时间；`PAUSED`、`RESTORE`、`RESTORED`、`READYREQUEST` 用于恢复。`EXIT table` 和 `EXITVOTE table/id/yes` 用于退出；通过后收到 `TABLEEND reason="exit"`。

`HEALTH` 可在登录前查询数量与就绪状态。`Z` 是心跳。`GETREPLAY table` 用于注册玩家下载已完成的立直 XML：REPLAYBEGIN、REPLAYCHUNK（小写十六进制 data、字节 offset）、REPLAYEND。四川与国标 JSON 记录在服务端 games 目录。
