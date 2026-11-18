"""dgc_mark -- filigrane invisible "DGC" (DMD GIF Creator / toolkit RecalBox DMD).

Un meme fichier est copie a l'identique dans trois depots (DMD GIF Creator, toolkit RecalBox DMD, bot Discord) : le
modifier partout ou nulle part.

Ce que c'est : un MARQUEUR "cree par nous", pas une preuve (sources publiques -> falsifiable ; un GIF recolore,
redimensionne ou reencode perd la marque). Sa presence est un indice fort, son absence ne prouve rien.

GIF (deux couches)
  1. bit de poids faible des octets R/G/B de la table de couleurs de chaque image (+-1/255, survit a la suppression
     des metadonnees) ;
  2. extension "Comment" du GIF : DGC1:<hex>.
  Charge utile 40 bits : magic 0xD6 | format 4 b | version appli (maj.min.patch, 3 x 4 b) | CRC-16.
  La lecture des tables de couleurs est faite a la main (pas par Pillow, qui convertit les images suivantes en RGB).

raw565 / raw565pack (8192 octets par image, sans en-tete ni palette : rien a ajouter)
  bit de poids faible du VERT (bit 5 du mot 565, +-4/255) des pixels "porteurs" uniquement (max(R5, G5, B5) >= 4 :
  jamais un noir ni un pixel tres sombre). Critere independant du bit modifie -> marquage idempotent et deterministe.
  Charge utile 32 bits SANS version (une nouvelle release ne change aucun octet) : magic 0xD6 | format | CRC-16.

Ligne de commande : python dgc_mark.py fichier.gif|.raw565|.raw565pack ...
"""

from __future__ import annotations

import binascii
import struct
import sys

# A maintenir avec APP_VERSION de DMD GIF Creator (rappel dans le script de bump).
GIF_APP_VERSION = (3, 2, 7)

_MAGIC = 0xD6
_FORMAT = 1
_GIF_BITS = 40
_RAW_BITS = 32
_COMMENT_PREFIX = b"DGC1:"
RAW565_FRAME = 8192
_RAW_MIN_CARRIERS = 2 * _RAW_BITS      # au moins 2 copies de la charge utile


def _crc16(data: bytes) -> int:
    return binascii.crc_hqx(data, 0xFFFF) & 0xFFFF


def _bits_of(value: int, n: int) -> list[int]:
    return [(value >> (n - 1 - i)) & 1 for i in range(n)]


def _gif_payload(version=GIF_APP_VERSION) -> int:
    major, minor, patch = (int(v) & 0xF for v in version)
    head = bytes([_MAGIC, (_FORMAT << 4) | major, (minor << 4) | patch])
    return (int.from_bytes(head, "big") << 16) | _crc16(head)


def _raw_payload() -> int:
    head = bytes([_MAGIC, _FORMAT << 4])
    return (int.from_bytes(head, "big") << 16) | _crc16(head)


def _vote(votes: list[list[int]], n: int):
    """votes[i] = [nb de 0, nb de 1] -> entier sur n bits, ou None si un bit n'a recu aucun vote / ex aequo."""
    out = 0
    for i in range(n):
        zeros, ones = votes[i]
        if zeros == ones:
            return None
        out = (out << 1) | (1 if ones > zeros else 0)
    return out


def _decode_gif_payload(value: int):
    head = (value >> 16).to_bytes(3, "big")
    if (value & 0xFFFF) != _crc16(head) or head[0] != _MAGIC or (head[1] >> 4) != _FORMAT:
        return None
    return (head[1] & 0xF, head[2] >> 4, head[2] & 0xF)


def _decode_raw_payload(value: int) -> bool:
    head = (value >> 16).to_bytes(2, "big")
    return (value & 0xFFFF) == _crc16(head) and head[0] == _MAGIC and (head[1] >> 4) == _FORMAT


# --------------------------------------------------------------------------------------------------------------------
# GIF
# --------------------------------------------------------------------------------------------------------------------
def mark_palette(palette: list[int], version=GIF_APP_VERSION) -> list[int]:
    """Palette Pillow (liste R,G,B,R,G,B...) -> meme palette avec la charge utile dans les bits de poids faible."""
    bits = _bits_of(_gif_payload(version), _GIF_BITS)
    return [(v & 0xFE) | bits[i % _GIF_BITS] for i, v in enumerate(palette)]


def gif_comment(version=GIF_APP_VERSION) -> bytes:
    return _COMMENT_PREFIX + b"%010x" % _gif_payload(version)


def embed_gif_frames(p_frames, version=GIF_APP_VERSION) -> bytes:
    """Marque en place la palette de chaque image (mode "P") et renvoie le commentaire GIF a passer a save(comment=)."""
    for im in p_frames:
        pal = im.getpalette()
        if pal:
            # Pillow ecrit la table arrondie a une puissance de 2 (min. 2 entrees) et bourre de zeros : on remplit nous-memes
            # jusqu'a cette taille (min. 16 entrees = 48 octets >= 40 bits) pour que tout octet de la table porte la charge.
            entries = 16
            while entries < len(pal) // 3:
                entries *= 2
            pal = list(pal) + [0] * (3 * entries - len(pal))
            im.putpalette(mark_palette(pal, version))
    return gif_comment(version)


def parse_gif(data: bytes, max_images: int = 16):
    """-> (tables, comments) : tables de couleurs (globale puis locales, octets bruts) et blocs Comment. Tolere les troncatures."""
    tables: list[bytes] = []
    comments: list[bytes] = []
    if data[:3] != b"GIF" or len(data) < 13:
        return tables, comments
    try:
        flags = data[10]
        pos = 13
        if flags & 0x80:
            size = 3 * (2 << (flags & 7))
            tables.append(data[pos:pos + size])
            pos += size
        images = 0
        while pos < len(data):
            block = data[pos]
            if block == 0x3B:
                break
            if block == 0x21:
                label = data[pos + 1]
                pos += 2
                chunks = []
                while True:
                    sz = data[pos]
                    pos += 1
                    if sz == 0:
                        break
                    chunks.append(data[pos:pos + sz])
                    pos += sz
                if label == 0xFE:
                    comments.append(b"".join(chunks))
            elif block == 0x2C:
                lflags = data[pos + 9]
                pos += 10
                if lflags & 0x80:
                    size = 3 * (2 << (lflags & 7))
                    tables.append(data[pos:pos + size])
                    pos += size
                pos += 1                      # taille de code LZW minimale
                while data[pos]:
                    pos += 1 + data[pos]
                pos += 1
                images += 1
                if images >= max_images:
                    break
            else:
                break
    except IndexError:
        pass
    return tables, comments


def _table_votes(tables) -> list[list[int]]:
    votes = [[0, 0] for _ in range(_GIF_BITS)]
    for t in tables:
        for i, v in enumerate(t):
            votes[i % _GIF_BITS][v & 1] += 1
    return votes


def detect_gif(data: bytes):
    """-> {"layers": [...], "version": "3.2.7"} ou None."""
    tables, comments = parse_gif(data)
    layers = []
    version = None

    for c in comments:
        if c.startswith(_COMMENT_PREFIX):
            try:
                v = _decode_gif_payload(int(c[len(_COMMENT_PREFIX):].decode("ascii"), 16))
            except ValueError:
                v = None
            if v:
                layers.append("comment")
                version = v
                break

    # chaque table seule (un bourrage de zeros ne doit pas masquer une table marquee), puis toutes ensemble
    attempts = [[t] for t in tables if len(t) >= 3 * 16] + ([tables] if tables else [])
    for group in attempts:
        value = _vote(_table_votes(group), _GIF_BITS)
        v = _decode_gif_payload(value) if value is not None else None
        if v:
            layers.append("palette")
            version = version or v
            break

    if not layers:
        return None
    return {"layers": layers, "version": ".".join(str(n) for n in version)}


# --------------------------------------------------------------------------------------------------------------------
# raw565 / raw565pack
# --------------------------------------------------------------------------------------------------------------------
def _is_carrier(w: int) -> bool:
    return max(w >> 11, (w >> 6) & 0x1F, w & 0x1F) >= 4


def mark_raw565(data: bytes) -> bytes:
    """Marque chaque image de 8192 octets (raw565 ou raw565pack). Autre taille ou image quasi noire : inchangee."""
    if not data or len(data) % RAW565_FRAME:
        return data
    return b"".join(_mark_raw_frame(data[i:i + RAW565_FRAME]) for i in range(0, len(data), RAW565_FRAME))


def _mark_raw_frame(frame: bytes) -> bytes:
    words = list(struct.unpack("<%dH" % (RAW565_FRAME // 2), frame))
    carriers = [i for i, w in enumerate(words) if _is_carrier(w)]
    if len(carriers) < _RAW_MIN_CARRIERS:
        return frame
    bits = _bits_of(_raw_payload(), _RAW_BITS)
    for k, i in enumerate(carriers):
        words[i] = (words[i] & ~0x20) | (bits[k % _RAW_BITS] << 5)
    return struct.pack("<%dH" % len(words), *words)


def _raw_frame_marked(frame: bytes) -> bool:
    words = struct.unpack("<%dH" % (RAW565_FRAME // 2), frame)
    votes = [[0, 0] for _ in range(_RAW_BITS)]
    k = 0
    for w in words:
        if _is_carrier(w):
            votes[k % _RAW_BITS][(w >> 5) & 1] += 1
            k += 1
    if k < _RAW_MIN_CARRIERS:
        return False
    value = _vote(votes, _RAW_BITS)
    return value is not None and _decode_raw_payload(value)


def detect_raw565(data: bytes):
    """-> {"layers": ["pixels"], "frames": n_marquees, "total": n} ou None. Au moins une image marquee."""
    if not data or len(data) % RAW565_FRAME:
        return None
    total = len(data) // RAW565_FRAME
    marked = sum(1 for i in range(0, len(data), RAW565_FRAME) if _raw_frame_marked(data[i:i + RAW565_FRAME]))
    if not marked:
        return None
    return {"layers": ["pixels"], "frames": marked, "total": total}


def detect(data: bytes, name: str = ""):
    """Detecte selon le contenu (GIF) ou l'extension (.raw565 / .raw565pack)."""
    if data[:3] == b"GIF":
        return detect_gif(data)
    if name.lower().endswith((".raw565", ".raw565pack")):
        return detect_raw565(data)
    return None


def _main(argv) -> int:
    if not argv:
        print(__doc__.strip().splitlines()[-1])
        return 2
    for path in argv:
        with open(path, "rb") as f:
            res = detect(f.read(), path)
        print(f"{path}: " + (f"DGC {res}" if res else "pas de marque DGC"))
    return 0


if __name__ == "__main__":
    sys.exit(_main(sys.argv[1:]))
