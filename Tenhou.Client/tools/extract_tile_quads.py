"""Extract exact 4-point perspective face quads (TL, TR, BR, BL) for all 694
face-up tile slots (variant == 34) in _obj2.dat / _obj3.dat and emit
Tenhou.Client/src/hd_tile_quads.hpp.

Each quad is stored in local pixel coordinates of the corresponding 1440-canvas
base tile sprite (origin at top-left pixel corner of the base sprite).
"""
from __future__ import annotations

import argparse
import pathlib
import struct
import zlib
import numpy as np


def load_obj(path: pathlib.Path):
    data = path.read_bytes()
    sig = data[:4].decode("ascii")
    header_bytes, _ = struct.unpack_from("<II", data, 4)
    prefix = 12 if sig in ("obj0", "obj2") else 24
    entries = {}
    for pos in range(prefix, header_bytes, 4):
        off = struct.unpack_from("<I", data, pos)[0]
        if not off:
            continue
        x, y, w, h = struct.unpack_from("<hhhh", data, off)
        key = struct.unpack_from("<BBBB", data, off + 8)
        _, stored_bytes = struct.unpack_from("<II", data, off + 12)
        entries[key] = (x, y, w, h, off + 20, stored_bytes)
    return data, entries


def decode_tile(d2, e2, d3, e3, k_base, variant: int) -> np.ndarray:
    bx, by, bw, bh, bo, bsb = e3[k_base]
    base = np.frombuffer(zlib.decompress(d3[bo : bo + bsb]), dtype=np.uint8).reshape(bh, bw, 4).copy()
    if variant == 34:
        return base
    vk = (k_base[0], k_base[1], variant, k_base[3])
    rx, ry, rw, rh, ro, rsb = e2[vk]
    dec = zlib.decompress(d2[ro : ro + rsb])
    pc = dec[0]
    pal = np.frombuffer(dec[1 : 1 + pc * 4], dtype=np.uint8).reshape(pc, 4)
    idx = np.frombuffer(dec[1 + pc * 4 :], dtype=np.uint8).reshape(rh, rw)
    base[ry - by : ry - by + rh, rx - bx : rx - bx + rw] ^= pal[idx]
    return base


def decode_delta_mask(d2, e2, e3, k_base, variant: int) -> np.ndarray:
    bx, by, bw, bh, _, _ = e3[k_base]
    vk = (k_base[0], k_base[1], variant, k_base[3])
    rx, ry, rw, rh, ro, rsb = e2[vk]
    dec = zlib.decompress(d2[ro : ro + rsb])
    pc = dec[0]
    pal = np.frombuffer(dec[1 : 1 + pc * 4], dtype=np.uint32)
    idx = np.frombuffer(dec[1 + pc * 4 :], dtype=np.uint8).reshape(rh, rw)
    mask = np.zeros((bh, bw), dtype=bool)
    mask[ry - by : ry - by + rh, rx - bx : rx - bx + rw] = pal[idx] != 0
    return mask


def convex_hull(pts: list[tuple[int, int]]) -> list[tuple[int, int]]:
    pts = sorted(set(pts))
    if len(pts) <= 2:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower: list[tuple[int, int]] = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper: list[tuple[int, int]] = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def line_intersect(n1: np.ndarray, c1: float, n2: np.ndarray, c2: float) -> np.ndarray:
    det = float(n1[0] * n2[1] - n1[1] * n2[0])
    if abs(det) < 1e-8:
        raise RuntimeError("Degenerate quad side intersection")
    x = (c1 * n2[1] - n1[1] * c2) / det
    y = (n1[0] * c2 - c1 * n2[0]) / det
    return np.array([x, y], dtype=float)


def fit_quad(d2, e2, d3, e3, k_base) -> np.ndarray:
    _, _, bw, bh, _, _ = e3[k_base]

    # Use 1m (black 'ichi' at top, red 'man' at bottom) to establish exact face-down direction.
    im1m = decode_tile(d2, e2, d3, e3, k_base, 1)
    m1m = decode_delta_mask(d2, e2, e3, k_base, 1)
    b, g, r = im1m[:, :, 0].astype(int), im1m[:, :, 1].astype(int), im1m[:, :, 2].astype(int)
    red_m = m1m & (r - b > 40) & (r - g > 40)
    blk_m = m1m & (r < 120) & (g < 120) & (b < 120) & (~red_m)
    ry, rx = np.where(red_m)
    ky, kx = np.where(blk_m)
    down_vec = np.array([rx.mean() - kx.mean(), ry.mean() - ky.mean()], dtype=float)
    down_vec /= np.linalg.norm(down_vec)
    right_vec = np.array([down_vec[1], -down_vec[0]], dtype=float)

    # Union of all 35 printed face variants gives the exact ink footprint of the tile face.
    union = np.zeros((bh, bw), dtype=bool)
    for v in range(37):
        if v == 34:
            continue
        if (k_base[0], k_base[1], v, k_base[3]) in e2:
            union |= decode_delta_mask(d2, e2, e3, k_base, v)
    ys, xs = np.where(union)
    hull = np.array(convex_hull(list(zip(xs.tolist(), ys.tolist()))), dtype=float)

    dirs = [-down_vec, right_vec, down_vec, -right_vec]  # Top, Right, Bottom, Left
    side_edges: list[list[tuple[float, np.ndarray]]] = [[], [], [], []]
    n_h = len(hull)
    for i in range(n_h):
        p = hull[i]
        q = hull[(i + 1) % n_h]
        edge = q - p
        length = float(np.linalg.norm(edge))
        if length < 1e-6:
            continue
        out_n = np.array([edge[1], -edge[0]], dtype=float) / length
        dots = [float(np.dot(out_n, d)) for d in dirs]
        best = int(np.argmax(dots))
        if dots[best] > 0.75:
            side_edges[best].append((length, out_n))

    normals = []
    offsets = []
    for s in range(4):
        if side_edges[s]:
            wsum = np.zeros(2, dtype=float)
            for length, out_n in side_edges[s]:
                wsum += out_n * (length * length)
            n = wsum / np.linalg.norm(wsum)
        else:
            n = dirs[s]
        c = float(np.max(hull @ n))
        normals.append(n)
        offsets.append(c)

    # Pixel centers sit at (x + 0.5, y + 0.5); shift by +0.5 so quad covers full ink pixel extent.
    tl = line_intersect(normals[0], offsets[0], normals[3], offsets[3]) + 0.5
    tr = line_intersect(normals[0], offsets[0], normals[1], offsets[1]) + 0.5
    br = line_intersect(normals[2], offsets[2], normals[1], offsets[1]) + 0.5
    bl = line_intersect(normals[2], offsets[2], normals[3], offsets[3]) + 0.5
    return np.array([tl, tr, br, bl], dtype=float)


def pack_slot_key(family: int, slot: int, direction: int) -> int:
    return (family & 0xFF) | ((slot & 0xFF) << 8) | ((direction & 0xFF) << 16)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--obj2", default="Tenhou.Client/assets/packs/_obj2.dat")
    parser.add_argument("--obj3", default="Tenhou.Client/assets/packs/_obj3.dat")
    parser.add_argument("--out", default="Tenhou.Client/src/hd_tile_quads.hpp")
    args = parser.parse_args()

    d2, e2 = load_obj(pathlib.Path(args.obj2))
    d3, e3 = load_obj(pathlib.Path(args.obj3))
    bases = sorted(k for k in e3 if k[2] == 34)

    rows = []
    for k in bases:
        quad = fit_quad(d2, e2, d3, e3, k)
        v1 = quad[1] - quad[0]
        v2 = quad[3] - quad[0]
        area = float(v1[0] * v2[1] - v1[1] * v2[0])
        if area <= 20.0 or np.any(np.isnan(quad)):
            raise RuntimeError(f"Invalid quad for {k}: area={area}")
        rows.append((pack_slot_key(k[0], k[1], k[3]), k, quad))

    rows.sort(key=lambda r: r[0])

    lines = [
        "// Generated by Tenhou.Client/tools/extract_tile_quads.py from _obj2.dat / _obj3.dat.",
        "// 4-point perspective face quads (TL, TR, BR, BL) in 1440-canvas base-sprite pixel coordinates.",
        "#pragma once",
        "#include <algorithm>",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        "namespace tenhou {",
        "    struct HdTileFaceQuad {",
        "        std::uint32_t slot_key; // family | (slot << 8) | (direction << 16)",
        "        float x0, y0; // Top-Left",
        "        float x1, y1; // Top-Right",
        "        float x2, y2; // Bottom-Right",
        "        float x3, y3; // Bottom-Left",
        "    };",
        "",
        "    inline constexpr std::uint32_t HdTileSlotKey(char family, int slot, int direction) {",
        "        return static_cast<std::uint32_t>(static_cast<std::uint8_t>(family)) |",
        "               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(slot)) << 8) |",
        "               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(direction)) << 16);",
        "    }",
        "",
        f"    inline constexpr HdTileFaceQuad kHdTileFaceQuads[{len(rows)}]{{",
    ]
    for sk, k, q in rows:
        lines.append(
            f"        {{0x{sk:06x}u, {q[0,0]:.2f}f, {q[0,1]:.2f}f, {q[1,0]:.2f}f, {q[1,1]:.2f}f, "
            f"{q[2,0]:.2f}f, {q[2,1]:.2f}f, {q[3,0]:.2f}f, {q[3,1]:.2f}f}}, // {chr(k[0])}/{k[1]}/d{k[3]}"
        )
    lines += [
        "    };",
        "",
        "    inline const HdTileFaceQuad *FindHdTileFaceQuad(char family, int slot, int direction) {",
        "        const std::uint32_t key = HdTileSlotKey(family, slot, direction);",
        "        const auto *begin = kHdTileFaceQuads;",
        "        const auto *end = kHdTileFaceQuads + (sizeof(kHdTileFaceQuads) / sizeof(kHdTileFaceQuads[0]));",
        "        const auto *it = std::lower_bound(begin, end, key,",
        "            [](const HdTileFaceQuad &entry, std::uint32_t target) { return entry.slot_key < target; });",
        "        return (it != end && it->slot_key == key) ? it : nullptr;",
        "    }",
        "} // namespace tenhou",
        "",
    ]

    out_path = pathlib.Path(args.out)
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {out_path} with {len(rows)} perspective quads.")


if __name__ == "__main__":
    main()
