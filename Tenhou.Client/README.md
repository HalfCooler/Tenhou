# 重建客户端源码 0.27.0

请解压到纯英文工程路径（例如 C:/Tenhou/client-src 或 C:/Tenhou/server-src），当前 NMake 不支持中文工程路径。

修改本目录 toolchain.local.json，指向已安装的 MSVC VsDevCmd.bat 和 CMake。执行 build.cmd Release 或 Debug，生成 build-Release/tenhou_rebuild.exe（或 build-Debug）。资源已放在 assets/，构建会复制到 EXE 旁的 resources/，无需下载。也可在 x86 MSVC 环境直接用 CMake 构建。

先启动单独交付的本机服务，再运行客户端。默认 127.0.0.1:10080。可注册本地账号，或以 NoName 访客登录。多人窗口用不同 --config-dir。BGM 支持 WAV；牌谱设置在 LOG/PATH。大厅战等 10 秒补人机，仅全注册真人局计排位。

本目录只包含生产工程，不包含阶段测试入口、脚本或文档。边界和验收详见另行交付的收尾文档。
