# 好友平台协议 v1

传输为 `TcpListener` / TCP、严格 UTF-8、NUL 结尾的扁平 XML 消息。单帧最多 64 KiB、256 个属性；客户端必须处理拆包和粘包。DTD、嵌套节点、重复属性、无效 UTF-8 和超限帧均拒绝。连接有握手、半帧、空闲及发送超时；发送队列与接收事件队列都有边界。

保留 XML 与立直事件编码是为了复用现有客户端牌桌。平台版本与官方服务无关：`HELO uname="TENHOU-PLATFORM" version="1"`；不仿造官方认证。

## 登录与目录

1. 客户端发送 `HELO name="NoName" sx="M"`，或以注册账号名登录。
2. 收到平台 `HELO` 后发送 `CAPS version="1" token="..."`，账号登录附带 `password`，注册附带 `register="1" nickname="..."`。
3. 服务端返回 `SESSION token="..." rid="..."`，然后下发目录与大厅。

NoName 的 `token` 只是续接访客会话的提示：令牌有效时恢复原会话；不存在或属于注册账号时自动创建新的访客会话，并用 `SESSION` 返回新令牌，不要求用户手动清理本地文件。注册账号仍使用密码认证。

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

## 自建服务大厅

服务通过 `HELO caps` 中的 `origin-lobby` 宣告能力。客户端在 `CAPS` 加 `lobby="origin"` 进入重建大厅，收到 `SESSION`、`PROFILE` 后以 `CHAT lobby/epoch/rid/seated` 确认当前会场。会场用完整的 `L0000`、`L1000` 至 `L9999` 或 `C` 加八位数字表示。每次成功切厅递增当前连接的 `epoch`。

- 变更请求带 `lobby/epoch/rid`，分别为来源会场、会场代次和本连接内递增的请求编号。客户端一次等待一个请求。普通切厅使用 `CHAT text="%2Flobby%20房间号"`，大会切厅使用 `CS target="Cxxxxxxxx"`。成功由同一 `rid` 的 `CHAT` 确认，拒绝通过 `RESERVATIONS` 返回。
- `CS lobby/epoch/title/players/rule/premiumonly/joinfee/registered/blocked/start/end/now` 下发大会配置及报名资格。时间为 UTC Unix 秒，可预约区间含开始时刻，截止时刻即关闭。报名请求为 `CSREG`，以同一场次重复报名返回当前结果。费用是测试配置展示值。
- 普通会场用 `JOIN t="房间号,玩法掩码"`，大会用 `JOIN type="玩法掩码"`。只带 `lobby/epoch/rid` 的 `JOIN` 取消全部预约。`RESERVATIONS lobby/epoch/types/rid/code` 返回当前预约集合、请求编号及拒绝原因。到期主动撤销时 `rid` 为空。
- `LN lobby/epoch/n/j/g` 下发当前会场的十进制计数向量，长度分别为 4、113、113。当前使用 `n[0]` 连接人数、`n[2]` 预约人数、`j` 各玩法的预约人数、`g` 各玩法进行中的桌数。段位、R 值、名次、会员日期和剩余天数由 `PROFILE` 下发。
- `UN` 后的 `GO` 带 `lobby/reservation`，明确服务选择的预约。原有 `GO type` 传牌桌呈现用的玩法位，不含段位页掩码。席位一旦提交，该玩家的其他预约同时移除，晚到的取消由已发出的 `GO` 收敛。完成对局后发 `BYE`，服务以 `CHAT` 确认返回原会场。
- 断线时将未开桌预约移出真人队列。同一服务进程内，`CAPS resume="1"` 可恢复仍符合资格的预约，重新排到队尾。普通登录开始新的排队会话。已开桌时沿用牌桌存档和 `RESTORE/RESTORED`，登录确认中的 `seated="1"` 表示即将恢复牌桌。

## 对局

立直使用 `UN/GO/INIT`、摸打/副露/和牌事件，以及既有 `D/N/REACH` 操作。GOK、STARTREADY、NEXTREADY 是幂等准备屏障；READYACK 确认提交，等待其他玩家时不再触发客户端动作超时。

`READYACK table/kind` 的 kind 对应原准备命令。`GOK` 和 `STARTREADY` 的回执不得清除 VSTATE 已授予的动作；`NEXTREADY` 的回执只解除正在等待该回执的次局提交，不得清除更新的动作快照。

四川和国标使用：

- `VGO table/rule/seat/n0..n3/exit`：对局身份与绝对座位。
- `VSTATE`：当前视角的手牌、公开区域、分数、阶段、结果历史和 `actions`。
- `actions="kind:tile,tile;kind:tile;..."`：仅包含该玩家可提交的操作。`dingque` 参数是花色0万/1筒/2索，其余参数是实体牌 ID。
- `VACT table/window/kind/tiles`：选择服务器授予的操作。

`VSTATE` 是完整快照，缺失的 handN 不得沿用旧快照。`handN` 仅向本人或明牌视角提供；`hiddenN` 表示当前暗手张数，`drawnN` 仅提供给本人。`meldsN="kind:tiles;..."` 配套 `meld_infoN="source,called;..."` 与 `meld_addedN="added,..."`，顺序一致，非补杠的 added 为 -1。来源座位为绝对座位；对手暗杠实体继续隐藏为 `closed_kan:?`。

首个开局快照可附带 `dealN`：该家十三张普通牌的原始发牌顺序（含自动补花所得的替代牌），客户端用于先发牌再理牌；实际 `handN` 继续使用规则状态中的顺序。`dealN` 的可见范围与 `handN` 一致，对手暗手不披露；后续快照不携带、不沿用。

`wall_total/wall_front/wall_tail/wall_offset` 表示总牌数、前取数量、尾取数量和开口，满足 total = front + tail + wall；不传未来牌实体。108 张使用两边 14 墩、两边 13 墩，136/144 张每边分别为 17/18 墩；均由公开游标消费。客户端根据实际资源锚点绘制，第 18 墩不使用下层槽位充当上层。

`seq` 是本场 MATCHVIEW 快照序号；`event_seq` 是公共呈现事件边界。`events="seq:kind:actor:tile:source;..."` 携带最近最多 32 条连续事件，序号在本场内递增，快照包含截至 event_seq 的全部规则变更。kind 包括 start、exchange、dingque、draw、discard、chi、pon、open_kan、closed_kan、added_kan、ron、tsumo、flower；无实体/来源为 -1。draw 的 tile 只对 actor 本人可见，其他视角为 -1。首次连接或恢复只对齐边界；事件重复不重播，缺口直接用本帧完整快照重同步。

四川结算用 `receipt_count` 和 `receiptI_*`（I 从 0 开始）发送本盘不可变收据，顺序为实际和牌顺序，盘末可追加流局收据。字段为 who、from、tile、fan、patterns（分号分隔名称）、reason、base/delta（四家完整分数单位）、hand 和 melds。melds 为 `kind:tiles:source:called:added`，分号分隔；公开手牌仅用于结算，不改变活动牌桌的暗手可见性。who=-1 表示盘末查花猪、查大叫和退税的合并收据，逐家 delta 给出最终支付变化；杠税已实时包含在牌桌分数中。客户端不从名称推导计分。

基础牌 ID 与现有资源一致：`0..35`万、`36..71`筒、`72..107`索、`108..123`东南西北、`124..127`白、`128..131`发、`132..135`中；国标花牌 `136..143` 为梅兰竹菊春夏秋冬。

`phase="exchange"` 等待换三张，`missing` 等待定缺，`turn` 为摸打阶段，`claims` 等待响应，`ended` 等待 NEXTREADY，`complete` 可返回大厅。血战和牌后 `retired0..3="1"` 标记本手退场，其 `actions` 为空；尚有两家以上未和时继续对局，三家和完即进入 ended/complete。`exchange_direction` 为 clockwise/counterclockwise/opposite/none，按八局局序确定。

`CLOCK ms/base_ms` 为剩余动作时间；`PAUSED`、`RESTORE`、`RESTORED`、`READYREQUEST` 用于恢复。`EXIT table` 和 `EXITVOTE table/id/yes` 用于退出；通过后收到 `TABLEEND reason="exit"`。

`HEALTH` 可在登录前查询数量与就绪状态。`Z` 是心跳。`GETREPLAY table` 用于注册玩家下载已完成的立直 XML：REPLAYBEGIN、REPLAYCHUNK（小写十六进制 data、字节 offset）、REPLAYEND。四川与国标 JSON 记录在服务端 games 目录。
