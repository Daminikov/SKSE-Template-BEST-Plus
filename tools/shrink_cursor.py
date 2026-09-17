#!/usr/bin/env python3
"""Shrinks a cursor PNG of any size into assets/cursor.png (21x27, the size PrismaUI expects).

    python tools/shrink_cursor.py <source.png>

PrismaUI paints the texture with its top-left corner at the cursor position, so the pointer tip has to
sit at the top-left of the image: transparent padding is cropped away first, then the artwork is
scaled to fit 21x27 (LANCZOS), keeping its aspect ratio. The untouched source is kept next to it as
assets/cursor_source.png so the cursor can be re-shrunk or re-exported later.
"""
import pathlib
import shutil
import sys

try:
    from PIL import Image
except ImportError:  # pragma: no cover - Pillow ships with the agent's python here
    raise SystemExit("Pillow is required: python -m pip install pillow")

WIDTH, HEIGHT = 21, 27


def preview(image: Image.Image) -> None:
    glyphs = " .:-=+*#%@"
    alpha = image.getchannel("A")
    print(f"  +{'-' * image.width}+")
    for y in range(image.height):
        line = ""
        for x in range(image.width):
            line += glyphs[min(9, alpha.getpixel((x, y)) * 10 // 256)]
        print(f"  |{line}|")
    print(f"  +{'-' * image.width}+")


def shrink(source: pathlib.Path, target: pathlib.Path) -> None:
    original = Image.open(source).convert("RGBA")
    print(f"source: {source} - {original.width}x{original.height}")

    alpha = original.getchannel("A")
    content = alpha.point(lambda value: 255 if value >= 8 else 0).getbbox()
    # (alpha >= 8: nearly transparent halo pixels - the soft shadow around the artwork - sit at the
    #  canvas border and would otherwise keep the crop at full size)
    if content is None:
        raise SystemExit(f"{source} is fully transparent")
    histogram = alpha.histogram()
    print(f"alpha: fully transparent {histogram[0]} px, faint (<8) {sum(histogram[1:8])} px, "
          f"opaque (255) {histogram[255]} px")
    cropped = original.crop(content)
    print(f"content box: {content} -> {cropped.width}x{cropped.height}")

    scale = min(WIDTH / cropped.width, HEIGHT / cropped.height)
    # never upscale: a small source stays as it is, only big art gets reduced
    scale = min(scale, 1.0)
    size = (max(1, round(cropped.width * scale)), max(1, round(cropped.height * scale)))
    resized = cropped.resize(size, Image.LANCZOS) if size != cropped.size else cropped
    print(f"scaled to {size[0]}x{size[1]} (fit into {WIDTH}x{HEIGHT}, aspect kept)")

    canvas = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    canvas.paste(resized, (0, 0))  # tip top-left, like a real pointer
    canvas.save(target)
    preview(canvas)


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    root = pathlib.Path(__file__).resolve().parent.parent
    source = pathlib.Path(sys.argv[1])
    if not source.exists():
        raise SystemExit(f"no such file: {source}")

    target = root / "assets" / "cursor.png"
    kept = root / "assets" / "cursor_source.png"
    target.parent.mkdir(exist_ok=True)

    shutil.copyfile(source, kept)
    shrink(kept, target)
    print(f"wrote {target} ({target.stat().st_size} bytes); source kept at {kept}")
    print("deploys to <mod>/PrismaUI/misc/cursor.png on the next build")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())