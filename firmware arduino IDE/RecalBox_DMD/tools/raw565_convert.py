"""Petits utilitaires pour visualiser/regenerer les .raw565 (128x32, RGB565 LE)."""
import struct
import sys
from pathlib import Path

TARGET_W = 128
TARGET_H = 32


def raw565_to_png(src: Path, dst: Path, scale: int = 6):
    from PIL import Image

    data = src.read_bytes()
    assert len(data) == TARGET_W * TARGET_H * 2, f"taille inattendue: {len(data)}"
    img = Image.new("RGB", (TARGET_W, TARGET_H))
    px = img.load()
    idx = 0
    for y in range(TARGET_H):
        for x in range(TARGET_W):
            (v,) = struct.unpack_from("<H", data, idx)
            idx += 2
            r = (v >> 11) & 0x1F
            g = (v >> 5) & 0x3F
            b = v & 0x1F
            r = (r * 255) // 31
            g = (g * 255) // 63
            b = (b * 255) // 31
            px[x, y] = (r, g, b)
    if scale > 1:
        img = img.resize((TARGET_W * scale, TARGET_H * scale), Image.NEAREST)
    img.save(dst)


def png_to_raw565(src: Path, dst: Path):
    from PIL import Image

    with Image.open(src) as img:
        img = img.convert("RGB")
        if img.size != (TARGET_W, TARGET_H):
            img = img.resize((TARGET_W, TARGET_H), Image.NEAREST)
        raw_bytes = img.tobytes()
        with open(dst, "wb") as f:
            for i in range(0, len(raw_bytes), 3):
                r, g, b = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack("<H", rgb565))


if __name__ == "__main__":
    mode = sys.argv[1]
    if mode == "decode":
        raw565_to_png(Path(sys.argv[2]), Path(sys.argv[3]))
    elif mode == "encode":
        png_to_raw565(Path(sys.argv[2]), Path(sys.argv[3]))
