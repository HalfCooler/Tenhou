# Third-party source

`Tenhou.Client/vendor/zlib/` contains unmodified inflate-related sources from zlib **1.3.1**, by Jean-loup Gailly and Mark Adler. The
original [license](../../Tenhou.Client/vendor/zlib/LICENSE) is retained beside the source headers. Seven upstream C files are built into a static library; no
zlib DLL or network fetch is required during builds.

- Source: https://zlib.net/fossils/zlib-1.3.1.tar.gz
- Archive SHA-256: `9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23`

`Tenhou.Client/vendor/miniaudio/` contains the unmodified single-header audio library **miniaudio 0.11.21** by David Reid, dual-licensed under **Public Domain (Unlicense)** and **MIT-0**. The original [license](../../Tenhou.Client/vendor/miniaudio/LICENSE) is retained beside `miniaudio.h` and staged into `licenses/miniaudio-LICENSE.txt`. It is compiled via `miniaudio_impl.c` into a static library (`miniaudio_impl`) providing WASAPI playback and built-in PCM / Microsoft ADPCM / IMA ADPCM / MP3 / FLAC / Vorbis decoding with zero runtime DLL dependencies or network fetches during builds.

- Source: https://github.com/mackron/miniaudio/releases/tag/0.11.21
- `miniaudio.h` SHA-256: `6b2029714f8634c4d7c70cc042f45074e0565766113fc064f20cd27c986be9c9`

`Tenhou.Client/assets/sources/mahjong_graphic/` contains unmodified transparent SVG flower faces (`mei`, `lan`, `zu`, `ju`, `chun`, `xia`, `qiu`, `dong`) from **mahjong_graphic** by lietxia, freely licensed for commercial and non-commercial use. The original [license](../../Tenhou.Client/assets/sources/mahjong_graphic/LICENSE) is retained beside the SVG masters and staged into `licenses/mahjong_graphic-LICENSE.txt`. PNG derivatives in `assets/images/variants/flowers/` are used for flower tile overlays.

- Source: https://github.com/lietxia/mahjong_graphic
- Commit: `3e275804ff58325306710bef3a7406860444bc6a`

Native 4-tier pre-rendered 3D mahjong tile and shadow packs (`1024×768`, `1440×1080`, `1920×1440`, `2640×1980`) in `Tenhou.Client/assets/packs/` and `Tenhou.Client/assets/shadows/` are bundled under C-EGG authorization granted to the project owner.

The repository root's `Build-Tenhou.ps1` configures local tools and dispatches the project build scripts.
Shared build functions discover MSVC through vswhere and CMake through PATH.

Edit `$LocalEnvironment` or use `-VsDevCmd` and `-CMake` to override tool paths. No toolchain downloaders or portable tool
caches are required. The application uses static CRT, native x64 (`AMD64`), Direct3D 11 / Direct2D 1.1, and system `gdiplus.dll`.
