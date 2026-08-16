# -*- coding: utf-8 -*-
"""
v2 : find_icon_text_split/text_bbox generalises (fractions de largeur, pas
des pixels fixes) pour fonctionner aussi bien sur les .raw565 128x32 que sur
des PNG renvoyes par l'IA a une resolution quelconque (detection auto : image
deja recadree sur le texte seul (nouveau workflow) OU image complete avec
icone a gauche (ancien workflow / fichiers deja deposes)).
"""
import struct
from pathlib import Path

TARGET_W = 128
TARGET_H = 32


def decode_raw565(src: Path):
    from PIL import Image
    data = src.read_bytes()
    img = Image.new("RGB", (TARGET_W, TARGET_H))
    px = img.load()
    idx = 0
    for y in range(TARGET_H):
        for x in range(TARGET_W):
            (v,) = struct.unpack_from("<H", data, idx)
            idx += 2
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
            px[x, y] = (r, g, b)
    return img


def encode_raw565(img, dst: Path):
    img = img.convert("RGB")
    assert img.size == (TARGET_W, TARGET_H)
    raw_bytes = img.tobytes()
    with open(dst, "wb") as f:
        for i in range(0, len(raw_bytes), 3):
            r, g, b = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            f.write(struct.pack("<H", rgb565))


def find_icon_text_split(img, thresh=18, search_from_frac=0.03, search_to_frac=0.40):
    """Retourne la colonne ou l'icone se termine, ou None si aucune coupure
    nette n'est detectee (image probablement deja recadree sur le texte seul)."""
    w, h = img.size
    px = img.load()
    search_from = max(2, int(w * search_from_frac))
    search_to = max(search_from + 1, int(w * search_to_frac))
    min_run = max(2, int(w * 0.01))

    colsum = []
    for x in range(w):
        s = 0
        for y in range(h):
            r, g, b = px[x, y]
            if r > thresh or g > thresh or b > thresh:
                s += 1
        colsum.append(s)

    seen_content = False
    run = 0
    for x in range(search_from, min(search_to, w)):
        if colsum[x] > 0:
            seen_content = True
            run = 0
        else:
            if seen_content:
                run += 1
                if run >= min_run:
                    return x - run + 1
    return None


def text_bbox(img, x_start: int = 0, thresh=18):
    w, h = img.size
    px = img.load()
    min_x, max_x, min_y, max_y = w, x_start, h, 0
    for x in range(x_start, w):
        for y in range(h):
            r, g, b = px[x, y]
            if r > thresh or g > thresh or b > thresh:
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)
    if max_x < min_x:
        return (x_start, 0, w, h)
    pad = max(1, w // 128)
    return (max(x_start, min_x - pad), max(0, min_y - pad), min(w, max_x + 1 + pad), min(h, max_y + 1 + pad))


def export_text_crop(src_raw565: Path, dst_png: Path, scale: int = 8):
    from PIL import Image
    img = decode_raw565(src_raw565)
    split_x = find_icon_text_split(img, search_from_frac=6 / 128, search_to_frac=64 / 128) or 34
    bbox = text_bbox(img, split_x)
    crop = img.crop(bbox)
    crop = crop.resize((crop.width * scale, crop.height * scale), Image.NEAREST)
    dst_png.parent.mkdir(parents=True, exist_ok=True)
    crop.save(dst_png)
    return bbox


def extract_ai_text_region(ai_img):
    """Detection auto : si l'image renvoyee par l'IA contient encore une
    icone a gauche (ancien workflow plein cadre), on la retire et on ne
    garde que la zone de texte. Sinon (nouveau workflow, deja un crop de
    texte), l'image est utilisee telle quelle."""
    split_x = find_icon_text_split(ai_img)
    if split_x is None:
        return ai_img
    bbox = text_bbox(ai_img, split_x)
    return ai_img.crop(bbox)


# Repli quand find_icon_text_split() ne detecte aucune coupure nette :
# la plupart du temps ca veut dire qu'il n'y a PAS d'icone du tout (juste
# une marge avant le texte, ex: ports/lastplayed/allgames-FR) -- 0 est donc
# un repli bien plus sur que l'ancien "34" fixe, qui coupait a l'aveugle en
# plein milieu du texte d'origine sur ces cas-la (residu visible, ex:
# "LAS" avant "ÚLTIMOS JUGADOS"). Seule exception connue : "favorites" a un
# vrai coeur dont le degrade ne redescend jamais a un noir pur (creux a une
# valeur proche de 0 mais jamais 0), donc la detection echoue alors qu'une
# icone existe bel et bien -- coupure fixee a la main pour ce cas precis.
_SPLIT_X_OVERRIDES = {"favorites": 31}


def compose_final(base_raw565: Path, ai_png: Path, dst_raw565: Path):
    from PIL import Image
    base = decode_raw565(base_raw565)
    override = _SPLIT_X_OVERRIDES.get(base_raw565.stem)
    if override is not None:
        split_x = override
    else:
        split_x = find_icon_text_split(base, search_from_frac=6 / 128, search_to_frac=64 / 128) or 0
    bbox = text_bbox(base, split_x)
    bx0, by0, bx1, by1 = bbox
    box_w, box_h = bx1 - bx0, by1 - by0

    out = Image.new("RGB", (TARGET_W, TARGET_H), (0, 0, 0))
    icon = base.crop((0, 0, split_x, TARGET_H))
    out.paste(icon, (0, 0))

    with Image.open(ai_png) as ai_img:
        ai_img = ai_img.convert("RGB")
        # Workflow actuel : la reference envoyee au modele est deja un crop
        # texte seul (text_reference/), donc le retour est traite tel quel --
        # PAS de re-detection icone/texte ici (ça a deja fait sauter le mot
        # "TIR" en le prenant a tort pour une icone, un espace entre mots
        # pouvant depasser le seuil de coupure). Repasse par
        # extract_ai_text_region() seulement si un jour un vieux fichier
        # plein-cadre doit etre re-traite a la main.
        text_region = ai_img.resize((box_w, box_h), Image.LANCZOS)

    out.paste(text_region, (bx0, by0))
    encode_raw565(out, dst_raw565)
    return out
