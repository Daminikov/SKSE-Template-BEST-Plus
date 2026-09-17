#!/usr/bin/env python3
"""Generates the pointer PrismaUI draws (assets/cursor.png).

PrismaUI loads its cursor from a fixed path - `Data/PrismaUI/misc/cursor.png` (Core.cpp) - and paints
that texture instead of the game's own cursor whenever input capture is active. Shipping our own copy
inside the mod makes MO2 resolve that path to this file, so the pointer is the game's gold arrow
instead of the framework's white one.

Geometry: the classic arrow silhouette (tip top-left, notched right wing, short tail), 21x27 to match
the framework's texture so nothing shifts on screen. Rendered 8x supersampled with a 1px outline and a
soft drop shadow, all in stdlib (zlib + struct) - no Pillow needed on a modding box.

Run from the template root:  python tools/make_cursor.py
"""
import pathlib
import struct
import zlib

WIDTH, HEIGHT = 21, 27
SUPERSAMPLE = 8

FILL = (233, 216, 168)     # Skyrim-ish pale gold
OUTLINE = (26, 21, 15)     # near-black warm outline
SHADOW = (0, 0, 0, 120)    # soft drop shadow

# Classic arrow, normalised to the 21x27 box (u right, v down, tip at the origin corner).
ARROW = [
    (0.00, 0.00),   # tip
    (0.00, 0.78),   # left edge, bottom
    (0.21, 0.58),   # notch, left
    (0.34, 0.96),   # tail, bottom-left
    (0.50, 0.93),   # tail, bottom-right
    (0.38, 0.55),   # notch, right
    (0.68, 0.55),   # right wing
]
MARGIN_X, MARGIN_Y = 0.6, 0.6


def to_pixels(polygon):
    width, height = WIDTH - 2 * MARGIN_X, HEIGHT - 2 * MARGIN_Y
    return [(MARGIN_X + u * width, MARGIN_Y + v * height) for u, v in polygon]


def inside(x, y, polygon):
    result = False
    for index in range(len(polygon)):
        x1, y1 = polygon[index]
        x2, y2 = polygon[(index + 1) % len(polygon)]
        if (y1 > y) != (y2 > y):
            if x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
                result = not result
    return result


def shrink(polygon, factor):
    cx = sum(p[0] for p in polygon) / len(polygon)
    cy = sum(p[1] for p in polygon) / len(polygon)
    return [(cx + (x - cx) * factor, cy + (y - cy) * factor) for x, y in polygon]


def coverage(px, py, polygon):
    hits = 0
    for sx in range(SUPERSAMPLE):
        for sy in range(SUPERSAMPLE):
            if inside(px + (sx + 0.5) / SUPERSAMPLE, py + (sy + 0.5) / SUPERSAMPLE, polygon):
                hits += 1
    return hits / (SUPERSAMPLE * SUPERSAMPLE)


def build():
    shape = to_pixels(ARROW)
    solid = [[coverage(x, y, shape) for x in range(WIDTH)] for y in range(HEIGHT)]
    inner = [[coverage(x, y, shrink(shape, 0.80)) for x in range(WIDTH)] for y in range(HEIGHT)]
    shadow = [[coverage(x, y, [(x + 1.6, y + 1.6) for x, y in shape]) for x in range(WIDTH)] for y in range(HEIGHT)]

    raw = bytearray()
    for y in range(HEIGHT):
        raw.append(0)  # PNG filter: none
        for x in range(WIDTH):
            cover, core = solid[y][x], inner[y][x]
            if cover <= 0.001:
                alpha = int(SHADOW[3] * shadow[y][x])
                raw += bytes((SHADOW[0], SHADOW[1], SHADOW[2], alpha))
                continue
            colour = FILL if core > 0.35 else OUTLINE
            raw += bytes((colour[0], colour[1], colour[2], int(255 * cover)))
    return raw


def write_png(path, raw):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) % 2**32)

    header = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 6, 0, 0, 0)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) +
                     chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def preview(raw):
    glyphs = " .:-=+*#%@"
    print(f"  {'-' * WIDTH}")
    for y in range(HEIGHT):
        line = ""
        for x in range(WIDTH):
            offset = y * (WIDTH * 4 + 1) + 1 + x * 4
            line += glyphs[min(9, raw[offset + 3] // 26)]
        print(f"  {line}")
    print(f"  {'-' * WIDTH}")


def main():
    root = pathlib.Path(__file__).resolve().parent.parent
    target = root / "assets" / "cursor.png"
    target.parent.mkdir(exist_ok=True)
    raw = build()
    write_png(target, raw)
    print(f"wrote {target} ({WIDTH}x{HEIGHT} RGBA, {target.stat().st_size} bytes)")
    preview(raw)
    print("deploys to <mod>/PrismaUI/misc/cursor.png (overrides the framework's texture)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())