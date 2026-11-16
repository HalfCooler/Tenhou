"""Rebuild flower PNGs from the checked-in, pinned upstream SVGs.

Build-time dependency: python -m pip install resvg-py==0.5.0
The Windows client uses GDI+ to load PNGs and needs no SVG renderer.
"""
from pathlib import Path
import resvg_py

assets = Path(__file__).resolve().parents[1] / "Tenhou.Client" / "assets"
source = assets / "sources" / "mahjong_graphic"
output = assets / "images" / "variants" / "flowers"
output.mkdir(parents=True, exist_ok=True)
for stem, name in zip(
    ("mei", "lan", "zu", "ju", "chun", "xia", "qiu", "dong"),
    ("梅", "兰", "竹", "菊", "春", "夏", "秋", "冬"),
):
    png = resvg_py.svg_to_bytes(
        svg_string=(source / f"{stem}.svg").read_text(encoding="utf-8"),
        width=160, height=200,
    )
    (output / f"{name}.png").write_bytes(png)
