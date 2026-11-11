# 源码构建

请解压到纯英文工程路径（例如 C:/Tenhou/client-src 或 C:/Tenhou/server-src），当前 NMake 不支持中文工程路径。

修改仓库根目录 `toolchain.local.json` 后，在根目录执行 `build.cmd Release -Target tenhou_server`（或 `Debug`），
生成 `build-Release/Tenhou.Server/tenhou_server.exe`（Debug 对应 `build-Debug`）。省略 `-Target` 会构建全部项目。
完整构建参数见[根目录说明](../readme.md)。本目录 `start-server.cmd` 按本目录 `server.ini` 后台启动，`stop-server.cmd` 优雅停止。正常运行只依赖 Windows 系统 DLL，不需要
Python 或外部数据库。

本目录不包含账户、会话、存档、牌谱或测试样例。运行配置、DPAPI 备份与升级规则见 README.md，完整验收与适配范围见另行交付的收尾文档。
