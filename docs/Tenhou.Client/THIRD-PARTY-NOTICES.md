# Third-party source

`Tenhou.Client/vendor/zlib/` contains unmodified inflate-related sources from zlib **1.3.1**, by Jean-loup Gailly and Mark Adler. The
original [license](../../Tenhou.Client/vendor/zlib/LICENSE) is retained beside the source headers. Seven upstream C files are built into a static library; no
zlib DLL or network fetch is required during builds.

- Source: https://zlib.net/fossils/zlib-1.3.1.tar.gz
- Archive SHA-256: `9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23`

The repository root's `Build-Tenhou.ps1` configures local tools and dispatches the project build scripts.
Shared build functions discover MSVC through vswhere and CMake through PATH.
Edit `$LocalEnvironment` or use `-VsDevCmd` and `-CMake` to override tool paths. No toolchain downloaders or portable tool
caches are required. The application uses static CRT and system `gdiplus.dll`.
