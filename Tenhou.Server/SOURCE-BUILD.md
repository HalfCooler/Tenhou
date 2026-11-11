# 独立源码构建

请解压到纯英文工程路径（例如 C:/Tenhou/client-src 或 C:/Tenhou/server-src），当前 NMake 不支持中文工程路径。

修改 toolchain.local.json 后执行 build.cmd Release 或 Debug，生成 build-Release/tenhou_server.exe（或 build-Debug）。start-server.cmd 按本目录 server.ini 后台启动，stop-server.cmd 优雅停止。正常运行只依赖 Windows 系统 DLL，不需要 Python 或外部数据库。

本目录不包含账户、会话、存档、牌谱或测试样例。运行配置、DPAPI 备份与升级规则见 README.md，完整验收与适配范围见另行交付的收尾文档。
