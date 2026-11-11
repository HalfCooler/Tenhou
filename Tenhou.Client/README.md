# 重建客户端源码 0.27.0

请解压到纯英文工程路径（例如 C:/Tenhou/client-src 或 C:/Tenhou/server-src），当前 NMake 不支持中文工程路径。

统一使用仓库根目录的 `toolchain.local.json` 和构建脚本。在根目录执行 `build.cmd Release`（或 `Debug`）构建全部项目，
只构建客户端可执行 `build.cmd Release -Target tenhou_rebuild`。生成 `build-Release/Tenhou.Client/tenhou_rebuild.exe`
（Debug 对应 `build-Debug`）。资源已放在本目录 `assets/`，构建会复制到 EXE 旁的 `resources/`，无需下载。
完整构建参数见[根目录说明](../readme.md)。

先启动单独交付的本机服务，再运行客户端。默认 127.0.0.1:10080。可注册本地账号，或以 NoName 访客登录。多人窗口用不同
--config-dir。BGM 支持 WAV；牌谱设置在 LOG/PATH。大厅战等 10 秒补人机，仅全注册真人局计排位。

本目录只包含生产工程，不包含阶段测试入口、脚本或文档。边界和验收详见另行交付的收尾文档。
