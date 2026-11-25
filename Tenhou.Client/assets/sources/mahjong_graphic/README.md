Source: https://github.com/lietxia/mahjong_graphic

Commit: `3e275804ff58325306710bef3a7406860444bc6a`

Original transparent SVGs (`Vectors 矢量图/SVG(透明背景 Transparent background)/`), unchanged:
- 34 standard tiles (33 faces + blank white dragon `5z` rendered from the ivory tile body)
- 3 red fives (`0m.svg`, `0p.svg`, `0s.svg`)
- 8 flowers (`mei.svg`, `lan.svg`, `zu.svg`, `ju.svg`, `chun.svg`, `xia.svg`, `qiu.svg`, `dong.svg`, entities 136..143)

Baked artifacts:
- Classic flower PNG derivatives (`images/variants/flowers/*.png`): retained unchanged for Classic compatibility.
- HD tile-face atlas (`images/hd_tiles/faces.png`): baked from all 44 transparent SVGs via `Tenhou.Client/tools/svg_bake.cpp` (Windows Direct2D `ID2D1DeviceContext5`, 384x512 per cell in an 8x6 premultiplied/straight RGBA grid) with zero runtime SVG parsing.

See `LICENSE`.
