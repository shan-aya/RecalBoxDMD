#!/usr/bin/env python3
"""zedmd_check.py -- compare le PNG du demon (--png-out, zoom 4) a l'image attendue apres une suite d'actions de zedmd_send.py.

  zedmd_check.py image.png [--zoom 4] ACTION...   (memes actions que zedmd_send.py : full565:S full888:S zones888:i,j clearzone:i,j clear)
Code de sortie 0 = identique ; sinon affiche le nombre de pixels differents et le premier.
"""
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import zedmd_send as z  # noqa: E402


def q565(r, g, b):
    v = z.rgb565_from_888(r, g, b)
    return ((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31


def put_zone(img, zone, rgb_bytes):
    x0, y0 = (zone % z.ZPR) * z.ZW, (zone // z.ZPR) * z.ZH
    for y in range(z.ZH):
        for x in range(z.ZW):
            i = (y * z.ZW + x) * 3
            img[(x0 + x, y0 + y)] = q565(rgb_bytes[i], rgb_bytes[i + 1], rgb_bytes[i + 2])


def expected(actions, base=None):
    img = base if base is not None else Image.new('RGB', (z.W, z.H), (0, 0, 0))
    px = img.load()
    for a in actions:
        name, _, val = a.partition(':')
        if name in ('full565', 'full888'):
            seed = int(val or 1)
            for zone in range(128):
                put_zone(px, zone, z.zone_rgb888(seed, zone))
        elif name == 'zones888':
            for zone in [int(x) for x in val.split(',')]:
                put_zone(px, zone, bytes([(zone * 37) % 256, (zone * 91) % 256, 200]) * (z.ZW * z.ZH))
        elif name == 'clearzone':
            for zone in [int(x) for x in val.split(',')]:
                put_zone(px, zone, bytes(z.ZW * z.ZH * 3))
        elif name == 'clear':
            px_clear = Image.new('RGB', (z.W, z.H), (0, 0, 0))
            img.paste(px_clear)
        elif name in ('render', 'keepalive', 'sleep', 'bright', 'lost565'):
            pass
        else:
            raise SystemExit('action inconnue : ' + a)
    return img


def main():
    args = sys.argv[1:]
    png = Path(args[0])
    zoom = 4
    acts = []
    i = 1
    while i < len(args):
        if args[i] == '--zoom':
            zoom = int(args[i + 1]); i += 2
        else:
            acts.append(args[i]); i += 1
    got = Image.open(png).convert('RGB')
    want = expected(acts).resize((z.W * zoom, z.H * zoom), Image.NEAREST)
    if got.size != want.size:
        print('%s: tailles differentes %s / %s' % (png.name, got.size, want.size))
        return 1
    g, w = got.load(), want.load()
    bad = [(x, y) for y in range(got.size[1]) for x in range(got.size[0]) if g[x, y] != w[x, y]]
    if bad:
        x, y = bad[0]
        print('%s: %d pixels differents (premier : %d,%d dalle %d,%d : obtenu %s attendu %s)' % (
            png.name, len(bad), x, y, x // zoom, y // zoom, g[x, y], w[x, y]))
        return 1
    print('%s: identique (%s)' % (png.name, ' '.join(acts) or 'noir'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
