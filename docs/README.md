# 项目文档

解决方案的项目文档集中在本目录，按所属项目分类。构建和运行命令默认从仓库根目录执行。

| 项目 | 文档 |
| --- | --- |
| Tenhou.Client | [客户端与资源](Tenhou.Client/README.md)、[第三方源码和许可](Tenhou.Client/THIRD-PARTY-NOTICES.md) |
| Tenhou.Client.Tests | [客户端专项检查](Tenhou.Client.Tests/README.md) |
| Tenhou.Server | [构建与运行](Tenhou.Server/README.md)、[架构](Tenhou.Server/ARCHITECTURE.md)、[规则](Tenhou.Server/RULES.md)、[协议](Tenhou.Server/PROTOCOL.md)、[验证记录](Tenhou.Server/VALIDATION.md)、[第三方参考与许可](Tenhou.Server/THIRD-PARTY-NOTICES.md) |
| Tenhou.Server.Tests | [规则和集成测试](Tenhou.Server.Tests/README.md) |

目录约定、本地环境配置和统一构建入口见[仓库说明](../readme.md)。各项目的代码位于项目根目录的 `src/` 中；构建脚本、项目文件、资源、配置和测试数据保留在所属项目中。第三方源码自带的许可证继续随源码保存。
