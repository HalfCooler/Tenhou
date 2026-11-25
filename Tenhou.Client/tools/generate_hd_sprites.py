"""Generate crisp vector SVG masters for the 7 remaining non-text UI sprites
(401, 402, 440, 466, 589, 591, 592) matching Tenhou.Client.Origin geometry,
colors, and visual language, ready for baking via svg_bake.exe.
"""
from __future__ import annotations

import pathlib


def svg_401() -> str:
    # Original 7x13: 100-point stick icon (vertical ivory stick with 2x3 = 6 black dots, soft edge shadow).
    return """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 7 13">
  <defs>
    <linearGradient id="body401" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#e8e8df"/>
      <stop offset="45%" stop-color="#d9d9cc"/>
      <stop offset="78%" stop-color="#c4c4b6"/>
      <stop offset="100%" stop-color="#96968b"/>
    </linearGradient>
    <linearGradient id="shade401" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#ffffff" stop-opacity="0.22"/>
      <stop offset="14%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="76%" stop-color="#000000" stop-opacity="0"/>
      <stop offset="92%" stop-color="#000000" stop-opacity="0.22"/>
      <stop offset="100%" stop-color="#000000" stop-opacity="0.46"/>
    </linearGradient>
  </defs>
  <!-- Soft outer drop shadow matching original 1px halo -->
  <rect x="0.25" y="0.25" width="6.5" height="12.5" rx="0.9" fill="#000000" fill-opacity="0.24"/>
  <!-- Stick body -->
  <rect x="0.85" y="0.45" width="5.3" height="11.55" rx="0.55" fill="url(#body401)"/>
  <rect x="0.85" y="0.45" width="5.3" height="11.55" rx="0.55" fill="url(#shade401)"/>
  <!-- Subtle top bevel highlight -->
  <rect x="1.0" y="0.55" width="4.9" height="0.85" rx="0.3" fill="#f6f6ef" fill-opacity="0.65"/>
  <!-- 6 black pips (2 columns x 3 rows, matching original rows y=3,5,7 cols x=2,4) -->
  <circle cx="2.55" cy="3.55" r="0.58" fill="#141414"/>
  <circle cx="4.45" cy="3.55" r="0.58" fill="#080808"/>
  <circle cx="2.55" cy="5.55" r="0.58" fill="#141414"/>
  <circle cx="4.45" cy="5.55" r="0.58" fill="#080808"/>
  <circle cx="2.55" cy="7.55" r="0.58" fill="#141414"/>
  <circle cx="4.45" cy="7.55" r="0.58" fill="#080808"/>
</svg>
"""


def svg_402() -> str:
    # Original 7x13: 1000-point riichi stick icon (1px left transparent pad, vertical ivory stick with center red dot).
    return """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 7 13">
  <defs>
    <linearGradient id="body402" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#e8e8df"/>
      <stop offset="45%" stop-color="#d9d9cc"/>
      <stop offset="78%" stop-color="#c4c4b6"/>
      <stop offset="100%" stop-color="#96968b"/>
    </linearGradient>
    <linearGradient id="shade402" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#ffffff" stop-opacity="0.22"/>
      <stop offset="14%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="76%" stop-color="#000000" stop-opacity="0"/>
      <stop offset="92%" stop-color="#000000" stop-opacity="0.22"/>
      <stop offset="100%" stop-color="#000000" stop-opacity="0.46"/>
    </linearGradient>
    <radialGradient id="reddot402" cx="40%" cy="35%" r="65%">
      <stop offset="0%" stop-color="#f22929"/>
      <stop offset="55%" stop-color="#d40e0e"/>
      <stop offset="100%" stop-color="#960000"/>
    </radialGradient>
  </defs>
  <!-- Soft outer drop shadow, preserving original 1px left transparent column -->
  <rect x="1.15" y="0.25" width="5.7" height="12.5" rx="0.85" fill="#000000" fill-opacity="0.24"/>
  <!-- Stick body -->
  <rect x="1.65" y="0.45" width="4.65" height="11.55" rx="0.5" fill="url(#body402)"/>
  <rect x="1.65" y="0.45" width="4.65" height="11.55" rx="0.5" fill="url(#shade402)"/>
  <rect x="1.80" y="0.55" width="4.25" height="0.85" rx="0.3" fill="#f6f6ef" fill-opacity="0.65"/>
  <!-- Centered red riichi pip (matching y=5..6, x=3..4) -->
  <circle cx="3.95" cy="5.95" r="1.05" fill="url(#reddot402)"/>
</svg>
"""


def svg_440() -> str:
    # Original 140x51: translucent glossy action-button plate (alpha ~77/255 = 0.302) with
    # subtle upper curved glass sheen and light-to-dark 1px border gradient.
    return """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 140 51">
  <defs>
    <linearGradient id="base440" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#525252" stop-opacity="0.302"/>
      <stop offset="50%" stop-color="#080808" stop-opacity="0.302"/>
      <stop offset="100%" stop-color="#000000" stop-opacity="0.302"/>
    </linearGradient>
    <linearGradient id="gloss440" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#a8a8a8" stop-opacity="0.14"/>
      <stop offset="65%" stop-color="#606060" stop-opacity="0.06"/>
      <stop offset="100%" stop-color="#101010" stop-opacity="0.0"/>
    </linearGradient>
    <linearGradient id="border440" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#aaaaaa" stop-opacity="0.32"/>
      <stop offset="50%" stop-color="#7c7c7c" stop-opacity="0.30"/>
      <stop offset="100%" stop-color="#4e4e4e" stop-opacity="0.32"/>
    </linearGradient>
  </defs>
  <rect x="0" y="0" width="140" height="51" fill="url(#base440)"/>
  <path d="M 1 1 L 139 1 L 139 15 C 102 27, 42 33, 1 32 Z" fill="url(#gloss440)"/>
  <rect x="0.75" y="0.75" width="138.5" height="49.5" fill="none" stroke="url(#border440)" stroke-width="1.4"/>
</svg>
"""


def svg_466() -> str:
    # Original 114x14: horizontal 1000-point riichi stick on the table.
    return """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 114 14">
  <defs>
    <linearGradient id="stick466" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#f8f8ee"/>
      <stop offset="18%" stop-color="#f5f5e8"/>
      <stop offset="64%" stop-color="#efefe0"/>
      <stop offset="78%" stop-color="#dedecc"/>
      <stop offset="90%" stop-color="#b9b9aa"/>
      <stop offset="100%" stop-color="#929287"/>
    </linearGradient>
    <linearGradient id="endshade466" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#000000" stop-opacity="0.16"/>
      <stop offset="2.5%" stop-color="#000000" stop-opacity="0"/>
      <stop offset="97.5%" stop-color="#000000" stop-opacity="0"/>
      <stop offset="100%" stop-color="#000000" stop-opacity="0.16"/>
    </linearGradient>
    <radialGradient id="reddot466" cx="44%" cy="38%" r="60%">
      <stop offset="0%" stop-color="#df1818"/>
      <stop offset="55%" stop-color="#b40404"/>
      <stop offset="100%" stop-color="#7c0000"/>
    </radialGradient>
  </defs>
  <rect x="0.4" y="1.0" width="113.2" height="12.6" rx="1.4" fill="#000000" fill-opacity="0.22"/>
  <rect x="1.0" y="1.75" width="112.0" height="10.8" rx="1.15" fill="url(#stick466)"/>
  <rect x="1.0" y="1.75" width="112.0" height="10.8" rx="1.15" fill="url(#endshade466)"/>
  <rect x="2.2" y="2.25" width="109.6" height="1.2" rx="0.6" fill="#ffffff" fill-opacity="0.58"/>
  <circle cx="56.5" cy="6.95" r="3.85" fill="#d4d4c6" fill-opacity="0.60"/>
  <circle cx="56.5" cy="6.95" r="3.45" fill="#f7f7ee" fill-opacity="0.75"/>
  <circle cx="56.5" cy="6.95" r="3.05" fill="url(#reddot466)"/>
</svg>
"""


def svg_589() -> str:
    # Original 244x172: perspective-tapered center HUD plate with ivory matte interior
    # and glossy black/silver metallic rounded bevel rim.
    return """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 244 172">
  <defs>
    <linearGradient id="face589" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#dcdcd4"/>
      <stop offset="50%" stop-color="#e0e0d8"/>
      <stop offset="100%" stop-color="#e5e5dd"/>
    </linearGradient>
    <linearGradient id="rimOuter589" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#2b2b2b"/>
      <stop offset="18%" stop-color="#141414"/>
      <stop offset="82%" stop-color="#191919"/>
      <stop offset="100%" stop-color="#070707"/>
    </linearGradient>
    <linearGradient id="rimSheen589" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#b8b8b8" stop-opacity="0.85"/>
      <stop offset="12%" stop-color="#585858" stop-opacity="0.55"/>
      <stop offset="50%" stop-color="#303030" stop-opacity="0.25"/>
      <stop offset="88%" stop-color="#727272" stop-opacity="0.55"/>
      <stop offset="100%" stop-color="#a8a8a8" stop-opacity="0.75"/>
    </linearGradient>
    <linearGradient id="innerEdge589" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#ffffff" stop-opacity="0.45"/>
      <stop offset="25%" stop-color="#ffffff" stop-opacity="0.08"/>
      <stop offset="85%" stop-color="#000000" stop-opacity="0.06"/>
      <stop offset="100%" stop-color="#000000" stop-opacity="0.18"/>
    </linearGradient>
  </defs>
  <path d="M 30 0.5
           L 214 0.5
           C 231.5 0.5, 237.8 8.0, 239.0 24.5
           L 243.5 142.5
           C 243.9 162.5, 234.5 171.5, 217 171.5
           L 27 171.5
           C 9.5 171.5, 0.1 162.5, 0.5 142.5
           L 5.0 24.5
           C 6.2 8.0, 12.5 0.5, 30 0.5 Z"
        fill="url(#rimOuter589)"/>
  <path d="M 30.5 1.8
           L 213.5 1.8
           C 230.2 1.8, 236.2 9.0, 237.4 25.0
           L 241.9 142.2
           C 242.3 161.2, 233.4 169.8, 216.5 169.8
           L 27.5 169.8
           C 10.6 169.8, 1.7 161.2, 2.1 142.2
           L 6.6 25.0
           C 7.8 9.0, 13.8 1.8, 30.5 1.8 Z"
        fill="none" stroke="url(#rimSheen589)" stroke-width="1.35"/>
  <path d="M 10.2 22.0 C 11.4 10.2, 17.5 4.4, 29.5 3.6" fill="none" stroke="#d8d8d8" stroke-opacity="0.72" stroke-width="1.15" stroke-linecap="round"/>
  <path d="M 233.8 22.0 C 232.6 10.2, 226.5 4.4, 214.5 3.6" fill="none" stroke="#d8d8d8" stroke-opacity="0.72" stroke-width="1.15" stroke-linecap="round"/>
  <path d="M 5.6 148.5 C 7.0 161.2, 14.8 167.2, 27.0 167.8" fill="none" stroke="#b6b6b6" stroke-opacity="0.58" stroke-width="1.1" stroke-linecap="round"/>
  <path d="M 238.4 148.5 C 237.0 161.2, 229.2 167.2, 217.0 167.8" fill="none" stroke="#b6b6b6" stroke-opacity="0.58" stroke-width="1.1" stroke-linecap="round"/>
  <path d="M 31.5 4.0
           L 212.5 4.0
           C 228.0 4.0, 233.8 10.6, 235.0 25.8
           L 239.3 141.6
           C 239.7 159.4, 231.4 167.4, 215.5 167.4
           L 28.5 167.4
           C 12.6 167.4, 4.3 159.4, 4.7 141.6
           L 9.0 25.8
           C 10.2 10.6, 16.0 4.0, 31.5 4.0 Z"
        fill="url(#face589)" stroke="#242424" stroke-width="0.9"/>
  <path d="M 32.0 4.9
           L 212.0 4.9
           C 227.2 4.9, 232.8 11.3, 234.0 26.2
           L 238.2 141.2
           C 238.6 158.6, 230.5 166.4, 215.0 166.4
           L 29.0 166.4
           C 13.5 166.4, 5.4 158.6, 5.8 141.2
           L 10.0 26.2
           C 11.2 11.3, 16.8 4.9, 32.0 4.9 Z"
        fill="none" stroke="url(#innerEdge589)" stroke-width="1.0"/>
</svg>
"""


def svg_frame_meander(w: int, h: int) -> str:
    # Shared vector generator for 591 (728x330) and 592 (590x430).
    # Note: Direct2D SVG requires userSpaceOnUse gradients on filled rects for thin lines
    # (1D lines have zero height/width in objectBoundingBox space).
    # Top-left meander ornament directly joins the double border bars at x=24.5 (y=14.5) and y=20.5 (x=14.5).
    tl_meander = (
        "M 14.5 20.5 L 14.5 17.5 L 11.5 17.5 L 11.5 8.5 L 24.5 8.5 L 24.5 14.5 "
        "M 24.5 14.5 L 21.5 14.5 L 21.5 11.5 L 14.5 11.5 L 14.5 14.5 L 17.5 14.5 L 17.5 17.5 L 21.5 17.5 L 21.5 20.5 L 14.5 20.5"
    )
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}">
  <defs>
    <linearGradient id="topVig" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="0" y2="26">
      <stop offset="0%" stop-color="#bcbcbc" stop-opacity="0.72"/>
      <stop offset="52%" stop-color="#dadada" stop-opacity="0.35"/>
      <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>
    <linearGradient id="botVig" gradientUnits="userSpaceOnUse" x1="0" y1="{h}" x2="0" y2="{h - 26}">
      <stop offset="0%" stop-color="#bcbcbc" stop-opacity="0.72"/>
      <stop offset="52%" stop-color="#dadada" stop-opacity="0.35"/>
      <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>
    <linearGradient id="leftVig" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="26" y2="0">
      <stop offset="0%" stop-color="#bcbcbc" stop-opacity="0.72"/>
      <stop offset="52%" stop-color="#dadada" stop-opacity="0.35"/>
      <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>
    <linearGradient id="rightVig" gradientUnits="userSpaceOnUse" x1="{w}" y1="0" x2="{w - 26}" y2="0">
      <stop offset="0%" stop-color="#bcbcbc" stop-opacity="0.72"/>
      <stop offset="52%" stop-color="#dadada" stop-opacity="0.35"/>
      <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>

    <linearGradient id="hFade" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="{w}" y2="0">
      <stop offset="0%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="28%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="50%" stop-color="#ffffff" stop-opacity="0.96"/>
      <stop offset="72%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>
    <linearGradient id="vFade" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="0" y2="{h}">
      <stop offset="0%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="26%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="50%" stop-color="#ffffff" stop-opacity="0.96"/>
      <stop offset="74%" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>

    <linearGradient id="hBarMain" gradientUnits="userSpaceOnUse" x1="24" y1="0" x2="{w - 24}" y2="0">
      <stop offset="0%" stop-color="#686868" stop-opacity="1"/>
      <stop offset="24%" stop-color="#848484" stop-opacity="0.72"/>
      <stop offset="38%" stop-color="#b4b4b4" stop-opacity="0"/>
      <stop offset="62%" stop-color="#b4b4b4" stop-opacity="0"/>
      <stop offset="76%" stop-color="#848484" stop-opacity="0.72"/>
      <stop offset="100%" stop-color="#686868" stop-opacity="1"/>
    </linearGradient>
    <linearGradient id="hBarSub" gradientUnits="userSpaceOnUse" x1="24" y1="0" x2="{w - 24}" y2="0">
      <stop offset="0%" stop-color="#a2a2a2" stop-opacity="0.72"/>
      <stop offset="22%" stop-color="#b8b8b8" stop-opacity="0.40"/>
      <stop offset="35%" stop-color="#d0d0d0" stop-opacity="0"/>
      <stop offset="65%" stop-color="#d0d0d0" stop-opacity="0"/>
      <stop offset="78%" stop-color="#b8b8b8" stop-opacity="0.40"/>
      <stop offset="100%" stop-color="#a2a2a2" stop-opacity="0.72"/>
    </linearGradient>
    <linearGradient id="vBarMain" gradientUnits="userSpaceOnUse" x1="0" y1="20" x2="0" y2="{h - 20}">
      <stop offset="0%" stop-color="#686868" stop-opacity="1"/>
      <stop offset="22%" stop-color="#848484" stop-opacity="0.72"/>
      <stop offset="36%" stop-color="#b4b4b4" stop-opacity="0"/>
      <stop offset="64%" stop-color="#b4b4b4" stop-opacity="0"/>
      <stop offset="78%" stop-color="#848484" stop-opacity="0.72"/>
      <stop offset="100%" stop-color="#686868" stop-opacity="1"/>
    </linearGradient>
    <linearGradient id="vBarSub" gradientUnits="userSpaceOnUse" x1="0" y1="20" x2="0" y2="{h - 20}">
      <stop offset="0%" stop-color="#a2a2a2" stop-opacity="0.72"/>
      <stop offset="20%" stop-color="#b8b8b8" stop-opacity="0.40"/>
      <stop offset="34%" stop-color="#d0d0d0" stop-opacity="0"/>
      <stop offset="66%" stop-color="#d0d0d0" stop-opacity="0"/>
      <stop offset="80%" stop-color="#b8b8b8" stop-opacity="0.40"/>
      <stop offset="100%" stop-color="#a2a2a2" stop-opacity="0.72"/>
    </linearGradient>
  </defs>

  <rect x="0" y="0" width="{w}" height="{h}" fill="#ffffff"/>

  <!-- Soft grey perimeter framing bands -->
  <rect x="0" y="0" width="{w}" height="26" fill="url(#topVig)"/>
  <rect x="0" y="{h - 26}" width="{w}" height="26" fill="url(#botVig)"/>
  <rect x="0" y="0" width="26" height="{h}" fill="url(#leftVig)"/>
  <rect x="{w - 26}" y="0" width="26" height="{h}" fill="url(#rightVig)"/>

  <!-- Edge-center white fade -->
  <rect x="0" y="1" width="{w}" height="28" fill="url(#hFade)"/>
  <rect x="0" y="{h - 29}" width="{w}" height="28" fill="url(#hFade)"/>
  <rect x="1" y="0" width="28" height="{h}" fill="url(#vFade)"/>
  <rect x="{w - 29}" y="0" width="28" height="{h}" fill="url(#vFade)"/>

  <!-- Connected double inner border bars (filled rects with userSpaceOnUse gradients) -->
  <rect x="24" y="14.0" width="{w - 48}" height="1.15" fill="url(#hBarMain)"/>
  <rect x="24" y="17.2" width="{w - 48}" height="0.85" fill="url(#hBarSub)"/>
  <rect x="24" y="{h - 15.15}" width="{w - 48}" height="1.15" fill="url(#hBarMain)"/>
  <rect x="24" y="{h - 18.05}" width="{w - 48}" height="0.85" fill="url(#hBarSub)"/>

  <rect x="14.0" y="20" width="1.15" height="{h - 40}" fill="url(#vBarMain)"/>
  <rect x="17.2" y="20" width="0.85" height="{h - 40}" fill="url(#vBarSub)"/>
  <rect x="{w - 15.15}" y="20" width="1.15" height="{h - 40}" fill="url(#vBarMain)"/>
  <rect x="{w - 18.05}" y="20" width="0.85" height="{h - 40}" fill="url(#vBarSub)"/>

  <!-- Four symmetrical meander (雷纹) corner ornaments joined to border bars -->
  <g fill="none" stroke="#686868" stroke-width="1.15" stroke-linecap="square" stroke-linejoin="miter">
    <path d="{tl_meander}"/>
    <g transform="translate({w}, 0) scale(-1, 1)"><path d="{tl_meander}"/></g>
    <g transform="translate(0, {h}) scale(1, -1)"><path d="{tl_meander}"/></g>
    <g transform="translate({w}, {h}) scale(-1, -1)"><path d="{tl_meander}"/></g>
  </g>

  <!-- Crisp 1px outer frame border -->
  <rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="none" stroke="#424242" stroke-width="1"/>
</svg>
"""


def main() -> None:
    out_dir = pathlib.Path("Tenhou.Client/assets/sources/sprites")
    out_dir.mkdir(parents=True, exist_ok=True)
    specs = {
        401: svg_401(),
        402: svg_402(),
        440: svg_440(),
        466: svg_466(),
        589: svg_589(),
        591: svg_frame_meander(728, 330),
        592: svg_frame_meander(590, 430),
    }
    for sid, content in specs.items():
        path = out_dir / f"{sid}.svg"
        path.write_text(content, encoding="utf-8")
        print(f"Wrote {path} ({len(content)} bytes)")


if __name__ == "__main__":
    main()
