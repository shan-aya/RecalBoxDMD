"""Cartes texte pour les jeux SANS logo (2026-10-20) : le nom du jeu rendu en police 5x7 sur une plaque de marquee 128x32, style « E » (biseau + plaque).

Pourquoi : ~25 % des logos de la Recalbox manquent (jeux jamais scrapes, jeux sans logo chez ScreenScraper). Sans carte, le DMD affiche le logo generique du
systeme ; avec, chaque jeu a un visuel a son nom. Les « non-jeux » (noms en « ZZZ... » : BIOS, appareils) et les jeux caches sont exclus.

Rendu : police classique 5x7 d'Adafruit_GFX (BSD, comme le firmware), jeu de caracteres CP437 (accents), taille x2 si le titre tient sur 1-2 lignes sinon x1 sur
3 lignes, lignes equilibrees, degrade vertical + extrusion + biseau, palette tiree du nom (stable d'une conversion a l'autre). Dependances : Pillow + numpy.
"""
import hashlib
import re
import unicodedata

W, H = 128, 32

# police 5x7 d'Adafruit_GFX (glcdfont.c, licence BSD) : 256 caracteres x 5 colonnes, bit 0 = haut
_FONT_HEX = (
    "00000000003e5b4f5b3e3e6b4f6b3e1c3e7c3e1c183c7e3c181c577d571c1c5e"
    "7f5e1c00183c1800ffe7c3e7ff0018241800ffe7dbe7ff30483a060e26297929"
    "26407f050507407f05253f5a3ce73c5a7f3e1c1c08081c1c3e7f14227f22145f"
    "5f005f5f06097f017f006689956a606060606094a2ffa29408047e040810207e"
    "201008082a1c08081c2a08081e101010100c1e0c1e0c30383e3830060e3e0e06"
    "000000000000005f00000007000700147f147f14242a7f2a1223130864623649"
    "5620500008070300001c2241000041221c002a1c7f1c2a08083e080800807030"
    "000808080808000060600020100804023e5149453e00427f4000724949494621"
    "41494d331814127f1027454545393c4a49493141211109073649494936464949"
    "291e000014000000403400000008142241141414141400412214080201590906"
    "3e415d594e7c1211127c7f494949363e414141227f4141413e7f494949417f09"
    "0909013e414151737f0808087f00417f41002040413f017f081422417f404040"
    "407f021c027f7f0408107f3e4141413e7f090909063e4151215e7f0919294626"
    "4949493203017f01033f4040403f1f2040201f3f4038403f6314081463030478"
    "04036159494d43007f4141410204081020004141417f04020102044040404040"
    "000307080020545478407f284444383844444428384444287f38545454180008"
    "7e090218a4a49c787f0804047800447d40002040403d007f1028440000417f40"
    "007c047804787c080404783844444438fc1824241818242418fc7c0804040848"
    "5454542404043f44243c4040207c1c2040201c3c4030403c44281028444c9090"
    "907c4464544c4400083641000000770000004136080002010204023c2623263c"
    "1ea1a161123a4040207a38545455592155557941225454784221555478402054"
    "5579400c1e5272123955555559395454545939555454580000457c410002457d"
    "420001457c407d1211127df0282528f07c545545002054547c547c0a097f4932"
    "494949323a4444443a324a4848303a4141217a3a42402078009da0a07d3d4242"
    "423d3d4040403d3c24ff2424487e4943662b2ffc2f2bff0929f620c0887e0903"
    "20545479410000447d413048484a32384040227a007a0a0a727d0d19317d2629"
    "292f28262929292630484d4020380808080808080808382f10c8acba2f102834"
    "fa00007b000008142a142222142a14085500550055aa55aa55aaff55ff55ff00"
    "0000ff00101010ff00141414ff001010ff00ff1010f010f0141414fc001414f7"
    "00ff0000ff00ff1414f404fc141417101f10101f101f1414141f00101010f000"
    "0000001f101010101f10101010f010000000ff101010101010101010ff100000"
    "00ff140000ff00ff00001f10170000fc04f414141710171414f404f40000ff00"
    "f714141414141414f700f7141414171410101f101f141414f4141010f010f000"
    "001f101f0000001f14000000fc140000f010f01010ff10ff141414ff14101010"
    "1f00000000f010fffffffffff0f0f0f0f0ffffff0000000000ffff0f0f0f0f0f"
    "3844443844fc4a4a4a347e02020606027e027e0263554941633844443c04407e"
    "201e2006027e020299a5e7a5991c2a492a1c4c7201724c304a4d4d3030487848"
    "30bc625a463d3e494949007e0101017e2a2a2a2a2a44445f444440514a444040"
    "444a51400000ff0103e080ff000008086b6b083612362436060f090f06000018"
    "180000001010003040ff0101001f01011e00191d1712003c3c3c3c0000000000"
)
_FONT = bytes.fromhex(_FONT_HEX)

# (haut du degrade, bas du degrade, extrusion, fond)
PALETTES = [
    ((255, 245, 170), (255, 150, 30), (150, 60, 10), (36, 18, 4)),
    ((200, 240, 255), (40, 140, 255), (20, 60, 150), (4, 14, 40)),
    ((255, 210, 210), (240, 50, 70), (140, 20, 40), (40, 6, 12)),
    ((210, 255, 200), (40, 200, 90), (20, 110, 50), (4, 32, 14)),
    ((240, 215, 255), (170, 80, 255), (90, 40, 160), (22, 8, 44)),
    ((255, 235, 255), (255, 90, 190), (150, 30, 100), (40, 8, 28)),
    ((220, 255, 255), (30, 200, 200), (15, 105, 110), (3, 30, 32)),
    ((255, 255, 255), (170, 180, 200), (80, 90, 120), (14, 16, 24)),
]

_ARTICLES = ("The", "A", "An", "Les", "Le", "La", "L'", "Der", "Die", "Das", "El", "Il", "Lo")


def is_non_game(name: str, stem: str = "", hidden: bool = False) -> bool:
    """Entrees a ne pas habiller : jeux caches, « ZZZ(notgame):... » (BIOS, appareils), « ZZZ-UNK-... »."""
    if hidden:
        return True
    return any((s or "").strip().upper().startswith("ZZZ") for s in (name, stem))


def clean_title(name: str) -> str:
    t = re.sub(r"\([^)]*\)|\[[^\]]*\]", "", name or "").strip(" -:")
    m = re.match(r"^(.*), (%s)$" % "|".join(re.escape(a) for a in _ARTICLES if not a.endswith("'")), t)
    if m:
        t = m.group(2) + " " + m.group(1)                      # « Chaos Engine, The » -> « The Chaos Engine »
    return re.sub(r"\s+", " ", t).strip() or (name or "?").strip()


def _glyph(ch: str) -> bytes:
    try:
        i = ch.encode("cp437")[0]
    except UnicodeEncodeError:
        b = unicodedata.normalize("NFKD", ch).encode("ascii", "ignore")
        i = b[0] if b else 63
    return _FONT[i * 5:i * 5 + 5]


def _tw(s: str, sc: int) -> int:
    return len(s) * 6 * sc - sc if s else 0


def _balanced(words, sc, maxw, nlines):
    """Repartit les mots sur `nlines` lignes en minimisant l'irregularite (evite un mot orphelin sur la derniere ligne). None si impossible."""
    n = len(words)
    INF = float("inf")
    best = {}

    def cost(i, j):
        w = _tw(" ".join(words[i:j]), sc)
        return INF if w > maxw else (maxw - w) ** 2

    def solve(i, k):
        if k == 1:
            return (cost(i, n), [n]) if i < n else (INF, [])
        key = (i, k)
        if key in best:
            return best[key]
        res = (INF, [])
        for j in range(i + 1, n - k + 2):
            c = cost(i, j)
            if c == INF:
                break
            rest, cuts = solve(j, k - 1)
            if c + rest < res[0]:
                res = (c + rest, [j] + cuts)
        best[key] = res
        return res

    c, cuts = solve(0, nlines)
    if c == INF:
        return None
    out, a = [], 0
    for b in cuts:
        out.append(" ".join(words[a:b])); a = b
    return out


def _layout(title: str):
    words = title.split()
    for sc, nl in ((2, 1), (2, 2), (1, 2), (1, 3)):
        maxw = W - 8
        lines = _balanced(words, sc, maxw, nl) if len(words) >= nl else None
        if lines and len(lines) == nl and all(_tw(l, sc) <= maxw for l in lines):
            # preferer 1x sur 2 lignes seulement si 2x sur 2 lignes ne tient pas
            return sc, lines
    # trop long meme en 1x sur 3 lignes : coupe sans casser les mots, finit par « .. »
    sc, maxw, lines, cur = 1, W - 8, [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if _tw(t, 1) <= maxw:
            cur = t
        else:
            if cur:
                lines.append(cur)
            cur = w[: (maxw + 1) // 6] if _tw(w, 1) > maxw else w
    lines.append(cur)
    lines = lines[:3]
    lines[-1] = lines[-1][: max(0, (maxw + 1) // 6 - 2)].rstrip() + ".."
    return sc, lines


def render_card(name: str):
    """Image PIL RGB 128x32 : nom du jeu, style E (biseau + plaque de marquee)."""
    import numpy as np
    from PIL import Image, ImageDraw

    title = clean_title(name)
    hi, lo, ext, bg = PALETTES[int(hashlib.md5(title.lower().encode("utf-8")).hexdigest(), 16) % len(PALETTES)]
    sc, lines = _layout(title)
    nl = len(lines)
    lh = 7 * sc + (2 if sc == 2 else 1)
    total = nl * lh - (2 if sc == 2 else 1)
    y0 = (H - total) // 2
    m = np.zeros((H, W), bool)
    for i, ln in enumerate(lines):
        x0 = (W - _tw(ln, sc)) // 2
        for ci, ch in enumerate(ln):
            g = _glyph(ch)
            for cx in range(5):
                for cy in range(7):
                    if g[cx] >> cy & 1:
                        for dx in range(sc):
                            for dy in range(sc):
                                X, Y = x0 + (ci * 6 + cx) * sc + dx, y0 + i * lh + cy * sc + dy
                                if 0 <= X < W and 0 <= Y < H:
                                    m[Y, X] = True

    def shift(a, dx, dy):
        o = np.zeros_like(a)
        o[max(dy, 0):H + min(dy, 0), max(dx, 0):W + min(dx, 0)] = a[max(-dy, 0):H + min(-dy, 0), max(-dx, 0):W + min(-dx, 0)]
        return o

    def lerp(a, b, t):
        return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

    img = Image.new("RGB", (W, H))
    px = img.load()
    for y in range(H):
        c = lerp((0, 0, 0), bg, 0.08 + 0.22 * (1 - abs(y - H / 2) / (H / 2)))      # fond tres sombre (v2 : etait 0,35..1,0, trop clair pour lire le titre) ; le cadre (filets) garde sa luminosite
        for x in range(W):
            px[x, y] = c
    d = ImageDraw.Draw(img)
    edge = lerp(ext, hi, 0.4)
    d.line([(0, 0), (W - 1, 0)], fill=edge)
    d.line([(0, H - 1), (W - 1, H - 1)], fill=edge)
    if total <= 24:                                  # filet interieur seulement s'il reste de la place
        inner = lerp((0, 0, 0), ext, 0.5)
        d.line([(0, 2), (W - 1, 2)], fill=inner)
        d.line([(0, H - 3), (W - 1, H - 3)], fill=inner)
    depth = 1 if (sc == 2 and nl == 2) or (sc == 1 and nl == 3) else 2
    ys = np.where(m.any(axis=1))[0]
    top, bot = int(ys.min()), int(ys.max())
    ext_any = np.zeros_like(m)
    for k in range(depth, 0, -1):
        e = shift(m, k, k)
        ext_any |= e
        for (y, x) in zip(*np.where(e & ~m)):
            px[int(x), int(y)] = lerp((0, 0, 0), ext, 0.55 + 0.45 * (depth - k + 1) / depth)
    if sc == 2:                                       # contour sombre seulement sur les grandes lettres
        dil = m.copy()
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                dil |= shift(m, dx, dy)
        for (y, x) in zip(*np.where(dil & ~m & ~ext_any)):
            px[int(x), int(y)] = (0, 0, 0)
    for (y, x) in zip(*np.where(m)):
        y, x = int(y), int(x)
        c = lerp(hi, lo, (y - top) / max(1, bot - top))
        up = not m[y - 1, x] if y > 0 else True
        left = not m[y, x - 1] if x > 0 else True
        down = not m[y + 1, x] if y < H - 1 else True
        right = not m[y, x + 1] if x < W - 1 else True
        if up or left:                                # biseau : liseré clair haut/gauche, sombre bas/droite
            c = lerp(c, (255, 255, 255), 0.55)
        elif down or right:
            c = lerp(c, ext, 0.55)
        px[x, y] = c
    return img


def make_text_card(name: str, dst_png) -> None:
    """Ecrit la carte 128x32 du jeu `name` dans `dst_png` (PNG ; la conversion la repasse telle quelle en raw565)."""
    render_card(name).save(dst_png, "PNG", optimize=False)
