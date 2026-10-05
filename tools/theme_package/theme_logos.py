#!/usr/bin/env python3
"""Logos de THEME Recalbox -> .raw565 128x32 pour la SD du DMD (+ verification du theme-hub).

Moteur unique (pas de dependance a l'interface) : conversion (SVG/PNG), alias des systemes virtuels, index
_index.bin, suivi _source.json, detection des logos d'un theme (modele du theme.xml puis heuristique), lecture du
catalogue https://gitlab.com/recalbox/themes/theme-hub (nouveaux themes / mises a jour).

Dependances : pillow, numpy, resvg-py (pip install resvg-py pillow numpy). Stdlib pour le reseau et les ZIP.

Usage (ligne de commande) :
    python theme_logos.py check   [--out <dossier _themes>] [--rb <\\\\RECALBOX\\share\\themes>] [--no-hub]
    python theme_logos.py convert <theme-hub | dossier> [--source hub|rb|dir] [--out <dossier _themes>] [--rb ...]
        (<theme-hub> = nom de dossier du hub, ex. bounitos-midnight ; ou chemin d'un dossier de theme)

Sortie : <out>/<nom du theme, 31 car. max>/<systeme>.raw565 (8192 o) + _index.bin + _source.json.
Sur la SD : systems/_defaults/_themes/<nom>/ (le firmware v233 lit ce chemin, voir DECISIONS.md).

safe-modify -- v1 - 2026-10-03 - creation (prototype valide sur midnight : logos RB1 identiques au prototype).
safe-modify -- v4 - 2026-10-04 - BUG REEL corrige (retour utilisateur : « sur midnight la Super Nintendo reste en version JP sur le DMD, meme en US ou EU ») : en mode variantes, les logos de BASE
etaient convertis avec prefer=() = le chemin NEUTRE du theme, qui n'est pas la variante US (midnight : « snes - whlogo.svg » = la version japonaise), alors que les surcharges etaient calculees contre la base
US (prefer_keys('en_US','us')). Resultat : region US (defaut) = logo japonais pour tout theme dont le chemin neutre differe de sa variante US. La base utilise maintenant prefer_keys('en_US','us') ;
_source.json['prefer'] reste [] (comparaison stable avec la fenetre). PIPELINE_VERSION 3 : tout est a reconvertir / republier.
safe-modify -- v3 - 2026-10-03 - VARIANTES DE LANGUE ET DE REGION A LA VOLEE : convert_theme() ecrit, en plus des logos de BASE (US / anglais), un sous-dossier par variante
<theme>/l_<langue>/ (textes traduits, ex. l_fr) et <theme>/r_<region>/ (consoles regionales, r_eu / r_jp), chacun avec son _index.bin et SEULEMENT les logos qui
different de la base. Le firmware (v238) choisit selon la langue et la region ANNONCEES par la Recalbox (script dmd_udp_resync v8) : plus de choix de langue / region
dans le toolkit. Ordre de recherche : langue, region, base (meme priorite que find_logos). PIPELINE_VERSION 2 (reconversion de tous les themes).
safe-modify -- v2 - 2026-10-03 - THEMES SYSTEME de Recalbox (SYSTEM_THEMES, ici recalbox-next = le theme PAR DEFAUT de Recalbox 10+, absent du theme-hub et du dossier share/themes de la Recalbox) : proposes comme des themes du hub (hub_catalog) mais telecharges depuis l'archive GitLab du depot recalbox/recalbox-themes (dossier du theme seulement, ~26 Mo, mis en cache) et convertis EN LOCAL. Licence CC BY-NC-ND 4.0 : pas de redistribution des logos convertis (ils ne vont donc PAS dans le paquet GitHub du projet).
"""
import argparse
import hashlib
import io
import json
import os
import re
import shutil
import struct
import sys
import time
import urllib.request
import zipfile

W, H, SS = 128, 32, 8
PIPELINE_VERSION = 3          # a incrementer quand le rendu change (declenche "a reconvertir" dans check)
# v18 (2026-10-05) : le depot GitLab theme-hub n'est plus alimente (Recalbox abandonne GitLab) : catalogue et ZIP des themes sont publies a jour sur
# media.recalbox.com (c'est de la que la Recalbox telecharge ses themes : .installedFrom). Miroir en premier, GitLab (fige) seulement en repli.
HUB_BASE = "https://media.recalbox.com/hub/-/raw/main"
HUB_ZIP_MIRROR = "https://gitlab.com/recalbox/themes/theme-hub/-/raw/main"
DEFAULT_RB_THEMES = r"\\RECALBOX\share\themes"
# themes fournis avec Recalbox (absents du hub) : dossier -> depot GitLab, chemin du theme, licence
SYSTEM_THEMES = {
    "recalbox-next": {"project": "recalbox%2Frecalbox-themes", "path": "themes/recalbox-next",
                      "name": "Recalbox NEXT (theme par defaut)", "license": "CC BY-NC-ND 4.0"},
    "recalbox-240p": {"project": "recalbox%2Frecalbox-themes", "path": "themes/recalbox-240p",
                      "name": "Recalbox 240p", "license": "licence du depot recalbox-themes"},
    # recalbox-next-v9 (ancienne version) : aucun logo de systeme exploitable (structure d'avant la 10), volontairement absent
}
USER_AGENT = "RecalBoxDMD-Toolkit/theme_logos"


# --------------------------------------------------------------------------- noms
def version_newer(new, old):
    """True si la version `new` est strictement plus recente que `old`. Versions pointees (1.4, 2.1, 1.41) comparees numeriquement ; sinon (dates, hash) : simple difference.
    Evite qu'un hub EN RETARD (ex. Dashboard-X 1.4) soit pris pour une mise a jour d'une copie Recalbox plus recente (1.5)."""
    def parse(v):
        m = re.fullmatch(r"\s*(\d+(?:\.\d+)*)\s*", str(v))
        return tuple(int(x) for x in m.group(1).split(".")) if m else None
    a, b = parse(new), parse(old)
    if a is not None and b is not None:
        n = max(len(a), len(b))
        return a + (0,) * (n - len(a)) > b + (0,) * (n - len(b))
    return str(new) != str(old)


def theme_dir_name(folder):
    """Meme assainissement que le firmware (CMD=theme) : [A-Za-z0-9._-], 31 car. max."""
    clean = re.sub(r"[^A-Za-z0-9._-]", "", folder or "")[:31]
    return "" if clean in ("", ".", "..") else clean


def fnv1a32(name):
    """Hash des noms du fichier _index.bin (identique au firmware : FNV-1a 32 bits sur le nom en minuscules)."""
    h = 2166136261
    for b in name.lower().encode("ascii", "replace"):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


# --------------------------------------------------------------------------- rendu
def _imports():
    import numpy as np
    from PIL import Image
    return np, Image


def _render_svg(svg_bytes, height):
    import resvg_py
    np, Image = _imports()
    txt = svg_bytes.decode("utf-8", errors="replace")

    def fix_root(m):
        tag = m.group(0)
        if "viewBox" in tag:   # unites mixtes (ex. height="29.01mm") : on repart du viewBox
            tag = re.sub(r"\s(width|height)=\"[^\"]*\"", "", tag)
        return tag

    txt = re.sub(r"<svg\b[^>]*>", fix_root, txt, count=1)
    png = resvg_py.svg_to_bytes(svg_string=txt, height=height)
    return Image.open(io.BytesIO(bytes(png))).convert("RGBA")


def _lighten_dark(img, thr=0.10, bigfrac=0.18, lift=0.38):
    """Quasi-noir -> blanc (sauf grand aplat clair, ex. cadre blanc 3DO) puis releve les couleurs sombres (teinte gardee)."""
    np, Image = _imports()
    a = np.asarray(img, dtype=np.float32).copy()
    lum = (0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]) / 255.0
    if ((a[..., 3] > 200) & (lum > 0.80)).mean() >= bigfrac:
        return img
    m = (lum < thr) & (a[..., 3] > 0)
    a[m, 0:3] = 255.0
    lum2 = (0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]) / 255.0
    dk = (a[..., 3] > 0) & (lum2 >= thr) & (lum2 < lift)
    f = np.minimum(lift / np.maximum(lum2[dk], 1e-3), 3.0)
    a[dk, 0:3] = np.minimum(a[dk, 0:3] * f[:, None], 255.0)
    return Image.fromarray(a.astype(np.uint8), "RGBA")


def _fit(big):
    """RGBA grande taille -> canvas RGB 128x32 (rognage des marges, ajustement proportionnel centre, filtre BOX)."""
    np, Image = _imports()
    bb = big.split()[3].point(lambda v: 255 if v > 8 else 0).getbbox()
    if bb:
        big = big.crop(bb)
    big = _lighten_dark(big)
    bg = Image.new("RGBA", big.size, (0, 0, 0, 255))
    bg.alpha_composite(big)
    rgb = bg.convert("RGB")
    sc = min(W / rgb.width, H / rgb.height)
    nw, nh = max(1, round(rgb.width * sc)), max(1, round(rgb.height * sc))
    small = rgb.resize((nw, nh), Image.BOX)
    canvas = Image.new("RGB", (W, H), (0, 0, 0))
    canvas.paste(small, ((W - nw) // 2, (H - nh) // 2))
    return canvas


def convert_bytes(data, is_svg):
    """Octets d'un logo (SVG ou PNG) -> 8192 octets RGB565 little-endian."""
    np, Image = _imports()
    if is_svg:
        big = _render_svg(data, H * SS)
    else:
        big = Image.open(io.BytesIO(data)).convert("RGBA")
    a = np.asarray(_fit(big), dtype=np.uint16)
    v = ((a[..., 0] & 0xF8) << 8) | ((a[..., 1] & 0xFC) << 3) | (a[..., 2] >> 3)
    return v.astype("<u2").tobytes()


# --------------------------------------------------------------------------- apercu
PREVIEW_SYSTEMS = ["snes", "megadrive", "mame", "fbneo", "neogeo", "psx", "n64", "dreamcast", "arcade",
                   "lastplayed", "favorites", "atari2600", "amiga600", "gba", "pcengine", "mastersystem"]


def raw565_to_image(data):
    """8192 octets RGB565 little-endian (ou chemin) -> image PIL RGB 128x32."""
    np, Image = _imports()
    if isinstance(data, (str, os.PathLike)):
        with open(data, "rb") as f:
            data = f.read()
    v = np.frombuffer(data, dtype="<u2").reshape(H, W)
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return Image.fromarray(np.stack([r * 255 // 31, g * 255 // 63, b * 255 // 31], -1).astype(np.uint8))


def preview_sheet(theme_dir, default_dir, names=None, scale=3):
    """Planche comparative : logo par defaut (gauche) / logo du theme (droite), pour les systemes presents dans le theme.
    Retourne (image PIL, liste des systemes montres)."""
    np, Image = _imports()
    from PIL import ImageDraw
    names = [n for n in (names or PREVIEW_SYSTEMS) if os.path.exists(os.path.join(theme_dir, n + ".raw565"))]
    if not names:   # aucun des systemes habituels : les premiers du theme
        names = sorted(os.path.splitext(f)[0] for f in os.listdir(theme_dir) if f.endswith(".raw565"))[:12]
    cw, row = W * scale + 10, H * scale + 6
    sheet = Image.new("RGB", (cw * 2 + 4, 18 + row * max(1, len(names))), (45, 45, 45))
    d = ImageDraw.Draw(sheet)
    d.text((6, 3), "defaut", fill=(255, 220, 90))
    d.text((cw + 6, 3), "theme", fill=(255, 220, 90))
    for i, n in enumerate(names):
        y = 18 + i * row
        dp = os.path.join(default_dir, n + ".raw565")
        if os.path.exists(dp):
            sheet.paste(raw565_to_image(dp).resize((W * scale, H * scale), Image.NEAREST), (4, y))
        sheet.paste(raw565_to_image(os.path.join(theme_dir, n + ".raw565")).resize((W * scale, H * scale), Image.NEAREST), (cw + 4, y))
        d.text((8, y + 1), n, fill=(120, 255, 120))
    return sheet, names

# --------------------------------------------------------------------------- sources (dossier / ZIP)
class DirSource:
    kind = "dir"

    def __init__(self, root):
        self.root = root

    def files(self):
        out = []
        for base, _dirs, names in os.walk(self.root):
            for n in names:
                out.append(os.path.relpath(os.path.join(base, n), self.root).replace("\\", "/"))
        return out

    def read(self, rel):
        with open(os.path.join(self.root, rel), "rb") as f:
            return f.read()

    def size(self, rel):
        try:
            return os.path.getsize(os.path.join(self.root, rel))
        except OSError:
            return 0


class RangeFile(io.RawIOBase):
    """Fichier distant en lecture seule via requetes HTTP Range (blocs mis en cache) : permet a zipfile de lire l'index
    d'un ZIP de plusieurs centaines de Mo et seulement les entrees voulues, sans tout telecharger."""

    def __init__(self, urls, block=512 * 1024, max_blocks=64):
        super().__init__()
        self.urls = [urls] if isinstance(urls, str) else list(urls)
        self.url = None
        self.block, self.max_blocks = block, max_blocks
        self.cache, self.order = {}, []
        self.pos = 0
        self.fetched = 0
        self.requests = 0
        err = None
        for u in self.urls:        # 1er miroir qui repond (taille via Content-Range)
            try:
                req = urllib.request.Request(u, headers={"Range": "bytes=-1", "User-Agent": USER_AGENT})
                with urllib.request.urlopen(req, timeout=30) as r:
                    cr = r.headers.get("Content-Range", "")
                    self.size = int(cr.rsplit("/", 1)[1]) if "/" in cr else int(r.headers.get("Content-Length", 0))
                    self.url = u
                    break
            except Exception as e:
                err = e
        if not self.url:
            raise RuntimeError(f"archive inaccessible : {err}")

    def readable(self):
        return True

    def seekable(self):
        return True

    def tell(self):
        return self.pos

    def seek(self, offset, whence=0):
        self.pos = offset if whence == 0 else self.pos + offset if whence == 1 else self.size + offset
        self.pos = max(0, min(self.pos, self.size))
        return self.pos

    def _block(self, i):
        if i in self.cache:
            return self.cache[i]
        start = i * self.block
        end = min(start + self.block, self.size) - 1
        req = urllib.request.Request(self.url, headers={"Range": f"bytes={start}-{end}", "User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=60) as r:
            data = r.read()
        self.requests += 1
        self.fetched += len(data)
        self.cache[i] = data
        self.order.append(i)
        if len(self.order) > self.max_blocks:
            self.cache.pop(self.order.pop(0), None)
        return data

    def read(self, n=-1):
        if n is None or n < 0:
            n = self.size - self.pos
        n = min(n, self.size - self.pos)
        out = []
        while n > 0:
            i, off = divmod(self.pos, self.block)
            chunk = self._block(i)[off: off + n]
            if not chunk:
                break
            out.append(chunk)
            self.pos += len(chunk)
            n -= len(chunk)
        return b"".join(out)

    def readinto(self, b):
        data = self.read(len(b))
        b[: len(data)] = data
        return len(data)


class ZipSource:
    kind = "zip"

    def __init__(self, path):
        """`path` : chemin d'un ZIP local ou objet fichier (ex. RangeFile pour un ZIP distant)."""
        self.zf = zipfile.ZipFile(path)
        names = [n for n in self.zf.namelist() if not n.endswith("/")]
        # un dossier racine unique contenant theme.xml : on le traite comme la racine du theme
        self.prefix = ""
        cands = [n for n in names if n.lower().endswith("theme.xml")]
        if cands:
            best = min(cands, key=lambda n: n.count("/"))
            self.prefix = best[: -len("theme.xml")]
        self._names = names

    def files(self):
        return [n[len(self.prefix):] for n in self._names if n.startswith(self.prefix)]

    def read(self, rel):
        return self.zf.read(self.prefix + rel)

    def size(self, rel):
        try:
            return self.zf.getinfo(self.prefix + rel).file_size
        except KeyError:
            return 0


# --------------------------------------------------------------------------- detection des logos
IMG_EXT = (".svg", ".png")
# noms qui ne sont pas des systemes (images de manettes, copies...) meme s'ils tombent dans le modele de chemin
_NOISE = re.compile(r"[()\s@]|_(controller|console|consolegame|game|bg|background)$", re.I)   # « @ » exclu : x@fr.svg = variante de langue (Dashboard-X 1.5) non utilisee par la Recalbox 10.1.1, pas un systeme
# suffixes de decoration courants autour du nom de systeme dans les themes
_STRIP = [r"\s*-\s*whlogo$", r"[-_ ]?logo$", r"^logo[-_ ]?"]


def known_systems_from_dir(path):
    """Noms de systemes connus = noms des .raw565 par defaut (sd_card/systems/_defaults)."""
    out = set()
    if path and os.path.isdir(path):
        for n in os.listdir(path):
            if n.lower().endswith(".raw565"):
                out.add(os.path.splitext(n)[0].lower())
    return out


def _aspect(src, rel):
    """Rapport largeur/hauteur d'une image du theme (SVG : viewBox ; PNG : taille), None si illisible."""
    try:
        data = src.read(rel)
        if rel.lower().endswith(".svg"):
            head = data[:2048].decode("utf-8", errors="replace")
            m = re.search(r"viewBox=\"\s*[-\d.]+[ ,]+[-\d.]+[ ,]+([\d.]+)[ ,]+([\d.]+)\s*\"", head)
            if m and float(m.group(2)) > 0:
                return float(m.group(1)) / float(m.group(2))
            w = re.search(r"\swidth=\"([\d.]+)", head)
            h = re.search(r"\sheight=\"([\d.]+)", head)
            return float(w.group(1)) / float(h.group(1)) if w and h and float(h.group(1)) > 0 else None
        _np, Image = _imports()
        with Image.open(io.BytesIO(data)) as im:
            return im.width / im.height if im.height else None
    except Exception:
        return None


def _looks_like_logos(src, files, sample=12, min_ratio=1.6):
    """Un dossier de logos de systeme contient des images LARGES (mediane largeur/hauteur >= 1,6)."""
    if len(files) < 25:
        return True   # pas assez pour juger ; le recouvrement des noms tranchera
    step = max(1, len(files) // sample)
    ratios = sorted(r for r in (_aspect(src, f) for f in files[::step][:sample]) if r)
    return bool(ratios) and ratios[len(ratios) // 2] >= min_ratio


XML_BUDGET = 4 * 1024 * 1024   # octets de XML lus au plus par theme (les gros ZIP du hub sont lus a distance)
XML_MAX_FILES = 40             # et 40 fichiers au plus : des centaines de petits XML disperses dans un ZIP distant = trop de requetes


def _xml_logo_templates(src):
    """Modeles de chemin des images `name="logo"` contenant $system, dans theme.xml PUIS les autres XML du theme (budget
    XML_BUDGET). Trois syntaxes de variantes sont reconnues : attributs `path.EU` / `path.fr` dans la balise, balises
    separees `<image name="logo" region="eu" path="...">` (ou lang=/language=), chemin en element enfant `<path>`.
    Retourne (modeles de base, {cle de variante: [modeles]}). Attributs `path` dupliques tolere (regex)."""
    xml_files = [f for f in src.files() if f.lower().endswith(".xml")]
    if not xml_files:
        return [], {}
    xml_files.sort(key=lambda f: (f.lower() != "theme.xml", f.count("/") > 0, src.size(f), f))
    xml_files = xml_files[:XML_MAX_FILES]   # theme.xml, puis les XML de la racine (theme-crt.xml...), puis les plus petits

    def compile_tpl(p):
        rel = re.sub(r"^\$\{root\}/|^\./", "", p.strip())
        rx = re.escape(rel).replace(re.escape("$system"), r"(?P<sid>[^/]+?)")
        try:
            return re.compile("^" + rx + "$", re.I)
        except re.error:
            return None

    has_system = re.compile(r"\$system(?!\w)")
    base, variants, budget = [], {}, XML_BUDGET
    for f in xml_files:
        sz = src.size(f)
        if sz > budget:
            continue
        try:
            xml = src.read(f).decode("utf-8", errors="replace")
        except Exception:
            continue
        budget -= sz
        for m in re.finditer(r"<image\b(?P<attrs>[^>]*?)(?:/>|>(?P<body>.*?)</image>)", xml, flags=re.S | re.I):
            attrs = m.group("attrs")
            if not re.search(r"\bname=\"logo\"", attrs, re.I):
                continue
            pairs = re.findall(r"([\w.]+)=\"([^\"]*)\"", attrs)
            key = next((v.lower() for k, v in pairs if k.lower() in ("region", "lang", "language")), None)
            paths = [(None, v) for k, v in pairs if k == "path"]                       # attribut path (peut etre duplique)
            paths += [(k[5:].lower(), v) for k, v in pairs if k.lower().startswith("path.")]   # path.EU / path.fr
            body = m.group("body") or ""
            child = re.search(r"<path>\s*([^<]*?)\s*</path>", body, re.I)
            if child:
                paths.append((None, child.group(1)))
            for vkey, p in paths:
                if not has_system.search(p):
                    continue
                rx = compile_tpl(p)
                if not rx:
                    continue
                k2 = vkey or key
                if k2:
                    variants.setdefault(k2, []).append(rx)
                else:
                    base.append(rx)
    return base, variants

def prefer_keys(language, region="us"):
    """Ordre de preference des variantes d'un logo : [langue_PAYS, langue, region] (ex. fr_FR + us -> ['fr_fr', 'fr', 'us']).
    SOURCE = ES de la Recalbox STABLE (gitlab.com/recalbox/recalbox, projects/frontend/es-core/src/themes/ThemeData.cpp) :
    un element de theme est retenu s'il est neutre ou s'il correspond a la LANGUE de `system.language` (code 2 lettres ou
    langue+pays) ou a la REGION `emulationstation.theme.region` (jp / eu, sinon US) -- la region est un reglage a part,
    PAS deduite de la langue, defaut us (recalbox.conf : RecalboxConf.h sThemeRegion = "us"). Confirme sur une Recalbox de
    test (midnight, en_US et fr_FR : `-fr` pour les systemes virtuels, `-us` pour les consoles). L'ordre de priorite quand
    plusieurs variantes correspondent n'est pas lu dans le code (hypothese : langue avant region)."""
    m = re.match(r"^([a-z]{2})_([A-Z]{2})$", language or "")
    if not m:
        return []
    region = (region or "us").lower()
    region = region if region in ("us", "eu", "jp") else "us"
    return [f"{m.group(1)}_{m.group(2)}".lower(), m.group(1), region]

def find_logos(src, known=None, prefer=()):
    """Retourne ({systeme: chemin relatif}, methode). Methode 'modele' (theme.xml) ou 'heuristique' (dossier).
    `prefer` = cles de variantes (langue/region) : le logo de BASE reprend la variante que le theme definit pour elle
    (ES a pris 'megadrive - whlogo-us.svg' pour une Recalbox en en_US)."""
    files = [f for f in src.files() if f.lower().endswith(IMG_EXT)]
    found = {}
    base_tpl, var_tpl = _xml_logo_templates(src)
    for rx in base_tpl:
        for f in files:
            m = rx.match(f)
            if m:
                sid = m.group("sid").lower()
                if sid not in found or (f.lower().endswith(".svg") and not found[sid].lower().endswith(".svg")):
                    found[sid] = f
    found = {k: v for k, v in found.items() if not _NOISE.search(k)}
    if len(found) >= 20:
        chosen = {}   # systeme -> (rang de preference, fichier) ; rang le plus bas = prefere
        for rank, key in enumerate(prefer):
            for rx in var_tpl.get(key, []):
                for f in files:
                    m = rx.match(f)
                    if m:
                        sid = m.group("sid").lower()
                        if sid not in chosen or rank < chosen[sid][0]:
                            chosen[sid] = (rank, f)
        used = 0
        for sid, (_rank, f) in chosen.items():
            if found.get(sid) != f:
                found[sid] = f
                used += 1
        return found, "modele" + (f"+variantes({','.join(prefer)}:{used})" if used else "")
    if not known:
        return found, "modele" if found else "aucune"
    # heuristique : le dossier dont les noms recouvrent le plus les systemes connus ET dont les images ressemblent a des
    # logos (larges ; les vignettes carrees d'un theme ne sont pas des logos)
    best, best_score = {}, 0
    by_dir = {}
    for f in files:
        by_dir.setdefault(f.rsplit("/", 1)[0] if "/" in f else "", []).append(f)
    for d, fl in by_dir.items():
        if not _looks_like_logos(src, fl):
            continue
        cur = {}
        for f in fl:
            stem = os.path.splitext(f.rsplit("/", 1)[-1])[0].lower()
            cands = [stem] + [re.sub(p, "", stem) for p in _STRIP]
            for c in cands:
                if c in known:
                    if c not in cur or (f.lower().endswith(".svg") and not cur[c].lower().endswith(".svg")):
                        cur[c] = f
                    break
        if len(cur) > best_score:
            best, best_score = cur, len(cur)
    # motif "un dossier par systeme" : <systeme>/<nom commun>.svg|png (ex. Mukashi : logo.svg ; Haigenco : system.png).
    # Le nom du systeme est celui du dossier ; on retient le nom de fichier commun couvrant le plus de systemes, a condition
    # que les images soient larges (logos) ; 'logo' est prefere.
    groups = {}
    for f in files:
        if "/" not in f:
            continue
        sid = f.rsplit("/", 2)[-2].lower()
        if sid not in known:
            continue
        stem = os.path.splitext(f.rsplit("/", 1)[-1])[0].lower()
        cur = groups.setdefault(stem, {})
        if sid not in cur or (f.lower().endswith(".svg") and not cur[sid].lower().endswith(".svg")):
            cur[sid] = f
    per_system, per_stem, per_score = {}, "", 0
    for stem, cur in groups.items():
        if len(cur) >= 25 and _looks_like_logos(src, list(cur.values())):
            score = len(cur) + (1000 if stem == "logo" else 0)
            if score > per_score:
                per_system, per_stem, per_score = cur, stem, score
    if len(per_system) >= 25 and len(per_system) > best_score:
        return per_system, f"dossier-par-systeme({per_stem})"
    if best_score >= 25:
        return best, "heuristique"
    return found, "modele" if found else "aucune"


# --------------------------------------------------------------------------- conversion d'un theme
def detect_variants(src, known=None):
    """Variantes proposees par un theme, parmi celles que le toolkit sait choisir : {"lang": ["fr", "es"], "region": ["eu", "jp"]}.
    lang = textes des logos (Favoris, Dernier joue...) qui different de la base en_US/us ; region = logos de consoles (eu/jp) qui different de us."""
    base = find_logos(src, known, prefer_keys("en_US", "us"))[0]
    out = {"lang": [], "region": []}
    for code, lang in (("fr", "fr_FR"), ("es", "es_ES")):
        if find_logos(src, known, prefer_keys(lang, "us"))[0] != base:
            out["lang"].append(code)
    for reg in ("eu", "jp"):
        if find_logos(src, known, prefer_keys("en_US", reg))[0] != base:
            out["region"].append(reg)
    return out

REGION_CODES = ("eu", "jp")          # regions proposees par les themes en plus de la base (us)


def overlay_dirs(src, known=None):
    """Variantes d'un theme sous forme de surcharges : {"l_fr": {systeme: fichier}, "r_eu": {...}} -- SEULEMENT les logos qui different de
    la base (US / anglais). l_<langue> : langues trouvees dans le theme (attributs path.fr / path.fr_FR...), r_<region> : eu / jp.
    La base est celle de prefer_keys("en_US", "us") ; une variante est calculee seule (langue sans region, region sans langue)."""
    base = find_logos(src, known, prefer_keys("en_US", "us"))[0]
    keys = set(_xml_logo_templates(src)[1])
    langs = {}
    for k in keys:
        if k in ("us",) + REGION_CODES:
            continue
        if re.fullmatch(r"[a-z]{2}", k):
            langs.setdefault(k, k.upper())
        elif re.fullmatch(r"[a-z]{2}_[a-z]{2}", k):
            langs.setdefault(k[:2], k[3:].upper())
    out = {}
    for lang, cc in sorted(langs.items()):
        if lang == "en":
            continue
        cur = find_logos(src, known, prefer_keys(f"{lang}_{cc}", "us"))[0]
        diff = {sid: f for sid, f in cur.items() if base.get(sid) != f}
        if diff:
            out[f"l_{lang}"] = diff
    for reg in REGION_CODES:
        cur = find_logos(src, known, prefer_keys("en_US", reg))[0]
        diff = {sid: f for sid, f in cur.items() if base.get(sid) != f}
        if diff:
            out[f"r_{reg}"] = diff
    return out


def write_overlay(src, diff, base_ids, out_dir):
    """Ecrit une surcharge (logos qui different + alias 'auto-X' -> 'X' comme la base) et son _index.bin. Retourne les noms de fichiers .raw565."""
    os.makedirs(out_dir, exist_ok=True)
    written = []
    for sid, rel in sorted(diff.items()):
        data = convert_bytes(src.read(rel), rel.lower().endswith(".svg"))
        names = [sid]
        if sid.startswith("auto-") and len(sid) > 5 and sid[5:] not in base_ids:
            names.append(sid[5:])
        for n in names:
            with open(os.path.join(out_dir, n + ".raw565"), "wb") as f:
                f.write(data)
            written.append(n + ".raw565")
    write_index(out_dir)
    return sorted(written)


def convert_theme(src, out_dir, known=None, log=print, hub_info=None, source_label="dir", prefer=(), variants=True):
    """Convertit les logos de `src` dans out_dir. Retourne le dict _source.json ecrit.
    variants=True (defaut) : en plus de la base US / anglais, ecrit les surcharges l_<langue>/ et r_<region>/ (voir overlay_dirs) -- ignore si `prefer` est donne."""
    # v4 : en mode variantes (aucune preference imposee), la BASE est la variante US / anglais -- pas le chemin neutre du theme (qui peut etre la version japonaise)
    base_prefer = tuple(prefer) if prefer else (prefer_keys("en_US", "us") if variants else ())
    logos, method = find_logos(src, known, base_prefer)
    if not logos:
        raise RuntimeError("aucun logo de systeme detecte dans ce theme")
    os.makedirs(out_dir, exist_ok=True)
    made, errors = {}, []
    for sid, rel in sorted(logos.items()):
        try:
            data = convert_bytes(src.read(rel), rel.lower().endswith(".svg"))
        except Exception as e:  # un logo defectueux ne bloque pas le theme
            errors.append((sid, str(e)[:80]))
            continue
        with open(os.path.join(out_dir, sid + ".raw565"), "wb") as f:
            f.write(data)
        made[sid] = rel
    # alias : 'auto-X' (systemes virtuels nommes par le theme) -> 'X' (SystemId envoye par ES), sans ecraser un vrai 'X'
    alias = []
    for sid in list(made):
        if sid.startswith("auto-") and len(sid) > 5 and sid[5:] not in made:
            a = sid[5:]
            with open(os.path.join(out_dir, sid + ".raw565"), "rb") as f, open(os.path.join(out_dir, a + ".raw565"), "wb") as g:
                g.write(f.read())
            alias.append(a)
    n_idx = write_index(out_dir)
    overlays = {}
    if os.path.isdir(out_dir):
        for d in os.listdir(out_dir):      # anciennes surcharges : retirees (une reconversion remplace tout)
            if re.fullmatch(r"[lr]_[a-z]{2}", d) and os.path.isdir(os.path.join(out_dir, d)):
                shutil.rmtree(os.path.join(out_dir, d), ignore_errors=True)
    if variants and not prefer:
        try:
            for code, diff in overlay_dirs(src, known).items():
                overlays[code] = write_overlay(src, diff, set(logos), os.path.join(out_dir, code))
        except Exception as e:
            log(f"variantes de langue/region non ecrites : {type(e).__name__}: {e}")
    sig = logo_signature(logos, src)
    info = {
        "pipeline": PIPELINE_VERSION,
        "source": source_label,
        "method": method,
        "prefer": list(prefer),
        "logo_count": len(made),
        "alias_count": len(alias),
        "index_count": n_idx,
        "errors": errors[:20],
        "overlays": {k: len(v) for k, v in overlays.items()},
        "signature": sig,
        "converted": time.strftime("%Y-%m-%d %H:%M:%S"),
    }
    try:   # v2 : variantes langue/region proposees par le theme (colonne "Variantes" de la fenetre du toolkit)
        info["variants"] = detect_variants(src, known)
    except Exception:
        pass
    if hub_info:
        info.update(hub_info)
    with open(os.path.join(out_dir, "_source.json"), "w", encoding="utf-8") as f:
        json.dump(info, f, indent=2, ensure_ascii=False)
    log(f"{len(made)} logos ({method}) + {len(alias)} alias, index {n_idx}, {len(errors)} echec(s)"
        + ("" if not overlays else " ; variantes : " + ", ".join(f"{k}={len(v)}" for k, v in sorted(overlays.items()))))
    return info


def write_index(out_dir):
    stems = [os.path.splitext(n)[0] for n in os.listdir(out_dir) if n.lower().endswith(".raw565")]
    hashes = sorted({fnv1a32(s) for s in stems})
    with open(os.path.join(out_dir, "_index.bin"), "wb") as f:
        f.write(b"".join(struct.pack("<I", h) for h in hashes))
    return len(hashes)


def logo_signature(logos, src):
    """Empreinte du contenu des logos (noms + contenu) : detecte une mise a jour du theme sans reconvertir."""
    h = hashlib.sha1()
    for sid, rel in sorted(logos.items()):
        h.update(sid.encode())
        try:
            h.update(hashlib.sha1(src.read(rel)).digest())
        except Exception:
            pass
    return h.hexdigest()


# --------------------------------------------------------------------------- theme-hub
def _http_get(url, timeout=30):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


def _hub_json(rel, log=print):
    """JSON du hub : miroir media.recalbox.com d'abord, GitLab (fige) en repli."""
    last = None
    for base in (HUB_BASE, HUB_ZIP_MIRROR):
        try:
            return json.loads(_http_get(f"{base}/{rel}").decode("utf-8-sig"))
        except Exception as e:
            last = e
            log(f"hub {base.split('/')[2]} : {rel} injoignable ({str(e)[:60]})")
    raise last


def hub_catalog(log=print):
    """[{folder, active, name, version, zips:[...], error?}] depuis list.json + descriptor.json de chaque theme."""
    out = []
    lst = _hub_json("list.json", log)
    for t in lst.get("themes", []):
        row = {"folder": t.get("folder", ""), "active": bool(t.get("active", True))}
        try:
            d = _hub_json(f"{row['folder']}/descriptor.json", log)
            row.update(name=d.get("name", row["folder"]), version=d.get("version"),
                       zips=[f.get("file") for f in d.get("files", []) if f.get("file")])
        except Exception as e:
            row["error"] = str(e)[:80]
        out.append(row)
    out.extend(system_catalog(log))
    return out


def system_catalog(log=print):
    """Themes fournis avec Recalbox, presentes comme des lignes du hub : {folder, active, name, version, zips, system, license, ref}.
    version = date + debut du dernier commit touchant le dossier du theme (change quand le theme change)."""
    out = []
    for folder, s in SYSTEM_THEMES.items():
        try:
            c = json.loads(_http_get(f"https://gitlab.com/api/v4/projects/{s['project']}/repository/commits?path={s['path']}&per_page=1").decode("utf-8"))[0]
            out.append({"folder": folder, "active": True, "name": s["name"], "version": f"{str(c['committed_date'])[:10]}-{c['id'][:7]}",
                        "zips": [f"{folder}.zip"], "system": True, "license": s["license"], "ref": c["id"]})
        except Exception as e:
            log(f"theme systeme {folder} injoignable : {str(e)[:80]}")
    return out


def system_download(h, cache_dir, log=print):
    """Archive GitLab du dossier d'un theme systeme (cache <dossier>__<version>.zip). Retourne le chemin local."""
    s = SYSTEM_THEMES[h["folder"]]
    os.makedirs(cache_dir, exist_ok=True)
    dst = os.path.join(cache_dir, f"{h['folder']}__{h['version']}.zip")
    if os.path.exists(dst) and os.path.getsize(dst) > 1 << 20:
        log("theme systeme deja en cache")
        return dst
    url = f"https://gitlab.com/api/v4/projects/{s['project']}/repository/archive.zip?path={s['path']}&sha={h['ref']}"
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    tmp = dst + ".part"
    got = 0
    with urllib.request.urlopen(req, timeout=120) as r, open(tmp, "wb") as f:
        while True:
            chunk = r.read(1 << 20)
            if not chunk:
                break
            f.write(chunk)
            got += len(chunk)
    os.replace(tmp, dst)
    log(f"theme systeme telecharge ({got / 1e6:.0f} Mo)")
    return dst


def open_hub_source(h, cache_dir=None, log=print):
    """Source de conversion d'une ligne du catalogue : (ZipSource, RangeFile|None). Theme du hub = ZIP distant lu par morceaux ;
    theme systeme = archive GitLab complete du dossier du theme (pas de lecture partielle possible)."""
    if h.get("system"):
        return ZipSource(system_download(h, cache_dir or default_cache_dir(), log)), None
    rf = RangeFile(hub_zip_urls(h["folder"], h["zips"][0]))
    return ZipSource(rf), rf


def hub_zip_urls(folder, zipname):
    """Adresses du ZIP d'un theme du hub : GitLab d'abord, miroir media.recalbox.com en secours."""
    q = urllib.request.quote(zipname)
    return [f"{HUB_BASE}/{folder}/{q}", f"{HUB_ZIP_MIRROR}/{folder}/{q}"]


def hub_download(folder, zipname, cache_dir, log=print):
    """Telecharge le ZIP du hub (cache, reprise non geree). Retourne le chemin local."""
    os.makedirs(cache_dir, exist_ok=True)
    dst = os.path.join(cache_dir, f"{folder}__{zipname}")
    url = f"{HUB_BASE}/{folder}/{urllib.request.quote(zipname)}"
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as r:
        total = int(r.headers.get("Content-Length") or 0)
        if os.path.exists(dst) and total and os.path.getsize(dst) == total:
            log(f"ZIP deja en cache ({total/1e6:.0f} Mo)")
            return dst
        tmp = dst + ".part"
        got, last = 0, 0
        with open(tmp, "wb") as f:
            while True:
                chunk = r.read(1 << 20)
                if not chunk:
                    break
                f.write(chunk)
                got += len(chunk)
                if got - last >= 20 << 20:
                    last = got
                    log(f"  telecharge {got/1e6:.0f}/{total/1e6:.0f} Mo")
    os.replace(tmp, dst)
    return dst


def default_cache_dir():
    base = os.environ.get("LOCALAPPDATA") or os.path.expanduser("~")
    return os.path.join(base, "RecalBoxDMD", "theme_cache")


def _read_source_json(out_root, folder):
    p = os.path.join(out_root, theme_dir_name(folder), "_source.json")
    try:
        with open(p, encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return None


def read_rb_setting(rb_themes, key, default):
    """Reglage de recalbox.conf lu a cote du dossier des themes (<share>/system/recalbox.conf) ; `default` si absent."""
    try:
        conf = os.path.join(os.path.dirname(os.path.normpath(rb_themes)), "system", "recalbox.conf")
        with open(conf, encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if line.startswith(key + "="):
                    return line.split("=", 1)[1].strip() or default
    except Exception:
        pass
    return default


def read_rb_language(rb_themes):
    """system.language de la Recalbox (ex. en_US) ; en_US par defaut."""
    return read_rb_setting(rb_themes, "system.language", "en_US")


def read_rb_region(rb_themes):
    """emulationstation.theme.region de la Recalbox (us / eu / jp) ; us par defaut (valeur par defaut d ES)."""
    return read_rb_setting(rb_themes, "emulationstation.theme.region", "us")


def _has_converted_logos(out_root, folder):
    """True si le dossier du theme contient des logos .raw565 (meme sans _source.json : copie manuelle)."""
    try:
        return any(n.endswith(".raw565") for n in os.listdir(os.path.join(out_root, theme_dir_name(folder))))
    except OSError:
        return False


def check_status(out_root, rb_themes=None, use_hub=True, known=None, log=print, prefer=()):
    """Etat de chaque theme : NOUVEAU (jamais converti), MAJ (version du hub / logos du theme installe / rendu), A JOUR."""
    rows = []
    hub = []
    if use_hub:
        try:
            hub = hub_catalog(log)
        except Exception as e:
            log(f"theme-hub injoignable : {e}")
    seen = set()
    for h in hub:
        if not h.get("active", True):
            continue
        seen.add(h["folder"])
        local = _read_source_json(out_root, h["folder"])
        st, why = "NOUVEAU", "jamais converti"
        if local:
            if local.get("pipeline", 0) < PIPELINE_VERSION:
                st, why = "MAJ", f"rendu v{local.get('pipeline')} -> v{PIPELINE_VERSION}"
            elif local.get("source") in ("hub", "package") and local.get("hub_version") is not None and version_newer(h.get("version"), local.get("hub_version")):
                st, why = "MAJ", f"hub {local.get('hub_version')} -> {h.get('version')}"
            else:
                if local.get("source") == "hub" and prefer and local.get("prefer") and list(local["prefer"]) != list(prefer):
                    st, why = "MAJ", f"langue/region des logos {local.get('prefer')} -> {list(prefer)}"
                else:
                    st, why = "A JOUR", f"converti {local.get('converted')} (source {local.get('source')})"
        if not local and _has_converted_logos(out_root, h["folder"]):
            st, why = "MAJ", "version inconnue (copie manuelle)"
        rows.append((h["folder"], st, why, h.get("version"), "hub"))
    # themes installes sur la Recalbox (copie utilisee par ES) : comparaison de l'empreinte des logos
    if rb_themes and os.path.isdir(rb_themes):
        for name in sorted(os.listdir(rb_themes)):
            p = os.path.join(rb_themes, name)
            if not os.path.isdir(p) or not os.path.exists(os.path.join(p, "theme.xml")):
                continue
            local = _read_source_json(out_root, name)
            try:
                src = DirSource(p)
                logos, _m = find_logos(src, known, prefer)
                sig = logo_signature(logos, src) if logos else None
            except Exception as e:
                rows.append((name, "ERREUR", str(e)[:60], None, "rb"))
                continue
            if not logos:
                rows.append((name, "SANS LOGOS", "aucun logo de systeme detecte", None, "rb"))
            elif not local and _has_converted_logos(out_root, name):
                rows.append((name, "MAJ", "version inconnue (copie manuelle)", None, "rb"))
            elif not local:
                rows.append((name, "NOUVEAU", "installe sur la Recalbox, jamais converti", None, "rb"))
            elif local.get("pipeline", 0) < PIPELINE_VERSION:
                rows.append((name, "MAJ", f"rendu v{local.get('pipeline')} -> v{PIPELINE_VERSION}", None, "rb"))
            elif local.get("source") == "package":
                # installe depuis le paquet GitHub (toutes langues, pas d'empreinte) : a jour tant que le paquet n'a pas change
                rows.append((name, "A JOUR", f"paquet GitHub ({local.get('converted')})", None, "rb"))
            elif list(local.get("prefer", [])) != list(prefer):
                rows.append((name, "MAJ", f"langue/region des logos {local.get('prefer')} -> {list(prefer)}", None, "rb"))
            elif local.get("signature") != sig and local.get("source") == "rb":
                rows.append((name, "MAJ", "logos modifies sur la Recalbox depuis la conversion", None, "rb"))
            elif local.get("source") == "hub" and local.get("signature") != sig:
                rows.append((name, "DIFFERENT", "converti depuis le hub, la copie de la Recalbox a d'autres logos", None, "rb"))
            else:
                rows.append((name, "A JOUR", f"converti {local.get('converted')} (source {local.get('source')})", None, "rb"))
    return rows


# --------------------------------------------------------------------------- ligne de commande
def _cli():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("check", "convert"):
        p = sub.add_parser(name)
        p.add_argument("--out", default=os.path.join("sd_card", "systems", "_defaults", "_themes"))
        p.add_argument("--rb", default=DEFAULT_RB_THEMES)
        p.add_argument("--known", default=os.path.join("sd_card", "systems", "_defaults"),
                       help="dossier des logos par defaut (noms de systemes connus)")
        p.add_argument("--lang", default=None, help="langue de la Recalbox (ex. en_US) ; defaut : lue dans recalbox.conf")
        p.add_argument("--region", default=None, help="region des themes us/eu/jp ; defaut : lue dans recalbox.conf (emulationstation.theme.region), sinon us")
        if name == "check":
            p.add_argument("--no-hub", action="store_true")
        else:
            p.add_argument("theme")
            p.add_argument("--source", choices=("auto", "hub", "rb", "dir"), default="auto")
            p.add_argument("--cache", default=default_cache_dir())
            p.add_argument("--full-download", action="store_true",
                           help="telecharge le ZIP du hub en entier au lieu de ne lire que les logos (Range)")
    a = ap.parse_args()
    known = known_systems_from_dir(a.known)
    prefer = prefer_keys(a.lang or read_rb_language(a.rb), a.region or read_rb_region(a.rb))
    if a.cmd == "check":
        for folder, st, why, ver, src in check_status(a.out, a.rb, not a.no_hub, known, prefer=prefer):
            print(f"{st:10s} {folder:34s} [{src}] {why}")
        return 0
    # convert
    theme = a.theme
    is_path = os.path.isdir(theme)
    folder = os.path.basename(os.path.normpath(theme)) if is_path else theme
    out_dir = os.path.join(a.out, theme_dir_name(folder))
    rb_dir = os.path.join(a.rb, folder)
    source = a.source
    if source == "auto":   # reference = theme-hub (stable/public) ; copie de la Recalbox seulement si le theme n y est pas
        if is_path:
            source = "dir"
        else:
            try:
                in_hub = any(h["folder"] == folder and h.get("zips") for h in hub_catalog())
            except Exception:
                in_hub = False
            source = "hub" if in_hub else ("rb" if os.path.isdir(rb_dir) else "hub")
    hub_info = None
    if source == "dir":
        src = DirSource(theme if is_path else rb_dir)
    elif source == "rb":
        src = DirSource(rb_dir)
    else:
        cat = {h["folder"]: h for h in hub_catalog()}
        h = cat.get(folder)
        if not h or not h.get("zips"):
            print(f"theme '{folder}' absent du hub"); return 2
        hub_info = {"hub_version": h.get("version"), "zip": h["zips"][0]}
        rf = None
        if h.get("system"):
            if h.get("license"):
                print(f"licence {h['license']} : conversion locale uniquement, ne pas redistribuer les logos convertis")
            src, rf = open_hub_source(h, a.cache)
        elif a.full_download:
            src = ZipSource(hub_download(folder, h["zips"][0], a.cache))
        else:   # lecture partielle : seuls l'index du ZIP et les logos sont telecharges
            src, rf = open_hub_source(h, a.cache)
    convert_theme(src, out_dir, known, hub_info=hub_info, source_label=source, prefer=prefer)
    if source == "hub" and rf is not None:
        print(f"telecharge : {rf.fetched / 1e6:.1f} Mo sur {rf.size / 1e6:.0f} Mo ({rf.requests} requetes)")
    print("->", out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(_cli())
