# Tenhou

客户端与本机服务共用根目录的构建脚本和工具链配置。要求 Windows、MSVC x86 和 CMake 3.24 或更高版本，工程及构建目录使用纯英文路径。

修改根目录 `toolchain.local.json` 中的 `vsdevcmd`、`cmake`，使其指向本机的 `VsDevCmd.bat` 和 `cmake.exe`，然后在根目录执行：

```bat
build.cmd Release
build.cmd Debug
```

不传参数时默认构建 Release，统一配置并编译所有子项目。也可以指定 CMake target 或构建目录：

```bat
build.cmd Release -Target tenhou_rebuild
build.cmd Release -Target tenhou_server
build.cmd Debug -BuildDirectory "out/debug"
```

PowerShell 中使用 `./build.ps1 -Configuration Release -Target tenhou_server`。相对构建目录按仓库根目录解析，从其他工作目录调用脚本也可使用。

默认产物：

- 客户端：`build-Release/Tenhou.Client/tenhou_rebuild.exe`
- 客户端资源：`build-Release/Tenhou.Client/resources/`（从 `Tenhou.Client/assets/` 自动复制）
- 服务端：`build-Release/Tenhou.Server/tenhou_server.exe`
- 服务端配置：`build-Release/Tenhou.Server/server.ini`

Debug 产物位于对应的 `build-Debug/` 目录。子项目只需在 CMake 中声明自己的目标，并由根目录 `CMakeLists.txt` 引入，无需另写编译脚本。

构建 Release 后，可用 `Tenhou.Server/start-server.cmd` 后台启动服务，使用 `Tenhou.Server/server.ini`；`Tenhou.Server/stop-server.cmd` 优雅停止。客户端和服务运行说明见各子项目 README。
