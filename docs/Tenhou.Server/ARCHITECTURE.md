# 规则与运行时架构

服务端是一个进程。`TcpHost` 负责异步 `TcpListener` 接入、UTF-8 分帧、发送队列和生命周期；所有房间与牌桌状态由有界 `Channel<HostEvent>` 的单消费者修改。`HostEvent` 使用 C# 15 联合类型表达接入、收到帧、断开和时钟事件。

下列代码路径均相对 `Tenhou.Server/src/`。`Rules/Contracts.cs` 定义两个主要接口：

- `IRuleSet`：稳定规则 ID、设置描述、参数校验及 `IMatch` 创建。规则目录通过注册表下发。
- `IMatch`：一场对局的状态、合法动作、操作提交、人机选择、事件、牌山和不变量校验。和牌是否结束当前手牌由实现决定。

立直的三/四人、局数、古役等是同一个 `RiichiRuleSet` 的选项。四川与国标分别实现 `SichuanRuleSet`、`GuobiaoRuleSet`。四川与国标共享不含立直语义的 `Common/VariantMatch` 牌张机制；各自决定特殊阶段、和牌资格、计分、和牌退场或终局。国标算番器独立于网络和文件系统。

`Application/Platform` 负责账号、房间归属、排队、准备屏障、计时、断线暂停、退出投票和结算提交。它通过 `IMatch` 推进对局，不直接访问某个规则的玩家结构。规则实现不依赖 Socket、存储或真实时钟。

`Protocol/IMatchPresentation` 将领域事件投影为某种客户端呈现。`riichi-v1` 适配现有 GDI 牌桌；`mahjong-v1` 下发私有视角快照与合法动作，供通用牌桌操作。隐藏手牌、暗杠与未公开定缺决定在投影时过滤。存储中的完整状态不直接发给客户端。

动作带桌号与操作窗口。通用玩法的窗口按每个玩家的动作集合分别更新，其他人提交换牌或定缺不会使尚未改变的合法选择失效。客户端不自行推导四川或国标的和牌资格。

`Infrastructure/JsonStore` 保存完整 JSON 快照，`TableRecord` 保存规则 ID、规则版本、归一化参数和实际牌山/动作。重放调用规则实现的同一入口；未知规则版本拒绝加载，避免规则升级后误读历史。没有 DPAPI、旧洗牌兼容器、P/Invoke 或 NativeAOT 要求。

扩展新规则时，实现并注册 `IRuleSet`、`IMatch`，提供设置与完整规则测试。能使用 `mahjong-v1` 的玩法可以复用通用牌桌；需要新牌具或新视觉语义时添加独立 presentation 与相应客户端 renderer。仅完成接口、未具备可玩实现的规则不应放入目录。

`Tenhou.Server.Tests/src/native-riichi-reference` 是冻结的旧规则比较样本，仅构建为可选测试程序。生产构建不编译这些源文件。
