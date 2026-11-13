# 好友平台协议 v1

传输为 `TcpListener` / TCP、严格 UTF-8、NUL 结尾的扁平 XML 消息。单帧最多 64 KiB、256 个属性；客户端必须处理拆包和粘包。DTD、嵌套节点、重复属性、无效 UTF-8 和超限帧均拒绝。连接有握手、半帧、空闲及发送超时；发送队列与接收事件队列都有边界。

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

`VSTATE` 是完整快照，缺失的 handN 不得沿用旧快照。`handN` 仅向本人或明牌视角提供；`hiddenN` 表示当前暗手张数，`drawnN` 仅提供给本人。`meldsN="kind:tiles;..."` 配套 `meld_infoN="source,called;..."` 与 `meld_addedN="added,..."`，顺序一致，非补杠的 added 为 -1。来源座位为绝对座位；对手暗杠实体继续隐藏为 `closed_kan:?`。

`wall_total/wall_front/wall_tail/wall_offset` 表示总牌数、前取数量、尾取数量和开口，满足 total = front + tail + wall；不传未来牌实体。108 张使用两边 14 墩、两边 13 墩，136/144 张每边分别为 17/18 墩；均由公开游标消费。客户端根据实际资源锚点绘制，第 18 墩不使用下层槽位充当上层。

`seq` 是本场 MATCHVIEW 快照序号；`event_seq` 是公共呈现事件边界。`events="seq:kind:actor:tile:source;..."` 携带最近最多 32 条连续事件，序号在本场内递增，快照包含截至 event_seq 的全部规则变更。kind 包括 start、exchange、dingque、draw、discard、chi、pon、open_kan、closed_kan、added_kan、ron、tsumo、flower；无实体/来源为 -1。draw 的 tile 只对 actor 本人可见，其他视角为 -1。首次连接或恢复只对齐边界；事件重复不重播，缺口直接用本帧完整快照重同步。

四川结算用 `receipt_count` 和 `receiptI_*`（I 从 0 开始）发送本盘不可变收据，顺序为实际和牌顺序，盘末可追加流局收据。字段为 who、from、tile、fan、patterns（分号分隔名称）、reason、base/delta（四家完整分数单位）、hand 和 melds。melds 为 `kind:tiles:source:called:added`，分号分隔；公开手牌仅用于结算，不改变活动牌桌的暗手可见性。who=-1 表示盘末查花猪、查大叫和退税的合并收据，逐家 delta 给出最终支付变化；杠税已实时包含在牌桌分数中。客户端不从名称推导计分。

基础牌 ID 与现有资源一致：`0..35`万、`36..71`筒、`72..107`索、`108..123`东南西北、`124..127`白、`128..131`发、`132..135`中；国标花牌 `136..143` 为梅兰竹菊春夏秋冬。

`phase="exchange"` 等待换三张，`missing` 等待定缺，`turn` 为摸打阶段，`claims` 等待响应，`ended` 等待 NEXTREADY，`complete` 可返回大厅。血战和牌后 `retired0..3="1"` 标记本手退场，其 `actions` 为空；尚有两家以上未和时继续对局，三家和完即进入 ended/complete。`exchange_direction` 为 clockwise/counterclockwise/opposite/none，按八局局序确定。

`CLOCK ms/base_ms` 为剩余动作时间；`PAUSED`、`RESTORE`、`RESTORED`、`READYREQUEST` 用于恢复。`EXIT table` 和 `EXITVOTE table/id/yes` 用于退出；通过后收到 `TABLEEND reason="exit"`。

`HEALTH` 可在登录前查询数量与就绪状态。`Z` 是心跳。`GETREPLAY table` 用于注册玩家下载已完成的立直 XML：REPLAYBEGIN、REPLAYCHUNK（小写十六进制 data、字节 offset）、REPLAYEND。四川与国标 JSON 记录在服务端 games 目录。
