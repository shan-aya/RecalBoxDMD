# -*- coding: utf-8 -*-
"""Correspondance titre de jeu <-> nom de GIF : COPIE EXACTE de la regle du firmware (RecalBox_DMD.ino v422 : ggNormW / ggScanMaster) et du demon Pi (src/media/gifmatch.h).

Le DMD recoit le titre du jeu (gamelist) et cherche dans /playlists/cache_master_gifs.dat les GIFs dont le nom correspond.
Le toolkit PC applique la MEME regle pour montrer ce que le DMD jouera. test_gifmatch.py (Python), rpi_daemon/tools/test_gifmatch.cpp (Pi) et le firmware partagent les memes
cas ; toute modification de la regle doit etre faite PARTOUT.

Regle (v422, decision utilisateur : « privilegier des correspondances SURES plutot que d'afficher le mauvais jeu ») :
  - normalisation : minuscules, lettres/chiffres ASCII seuls, accents latins courants replies sur la lettre de base (e, a, o, u... ; ae, oe, ss), texte entre (...) et [...]
    ignore, tout autre caractere ignore ;
  - MOTS d'un nom : coupes aux separateurs (tout caractere ignore ci-dessus), aux passages minuscule -> Majuscule (AlienStorm), lettre <-> chiffre (Storm01) et en fin de sigle
    (NEOGEOMetal -> NEOGEO + Metal) ;
  - rang 3 : nom du GIF (normalise) identique au titre ;
  - rang 2 : le nom du GIF est contenu dans le titre (4 caracteres au moins, strictement plus court) ;
  - rang 1 : le titre est contenu dans le nom du GIF ET couvre des MOTS ENTIERS (il commence et finit a une frontiere de mot du GIF) : « Aliens » ne correspond plus a
    ARCADE_AlienStorm01, « Tron » a Robotron, « Darius » a Gdarius ; « Mortal Kombat » correspond toujours a ARCADE_MortalKombat05SubZero (titre >= 4 caracteres, strictement plus court) ;
  - un GIF dont le nom normalise a moins de 3 caracteres est ignore ; seul le meilleur rang trouve est retenu (le DMD tire au hasard parmi eux).
"""
import re
import unicodedata

_FOLD = {  # U+00C0..U+00FF (comme la table du firmware)
    **{c: "a" for c in "àáâãäåÀÁÂÃÄÅ"}, "æ": "ae", "Æ": "ae", "ç": "c", "Ç": "c",
    **{c: "e" for c in "èéêëÈÉÊË"}, **{c: "i" for c in "ìíîïÌÍÎÏ"}, "ð": "d", "Ð": "d", "ñ": "n", "Ñ": "n",
    **{c: "o" for c in "òóôõöøÒÓÔÕÖØ"}, **{c: "u" for c in "ùúûüÙÚÛÜ"}, **{c: "y" for c in "ýÿÝ"},
    "ß": "ss", "×": "", "÷": "", "þ": "", "Þ": "", "œ": "oe", "Œ": "oe",
}

_NONE, _LOWER, _UPPER, _DIGIT = 0, 1, 2, 3


def norm_words(text):
    """(texte normalise, debuts de mots) : `wb[i]` vaut 1 si un mot commence au caractere i du texte normalise. Meme algorithme octet par octet que le firmware (ggNormW)."""
    out, wb, depth, prev, prev2 = [], [], 0, _NONE, _NONE
    for ch in unicodedata.normalize("NFC", text):
        if ch in "([":
            depth += 1
            prev = prev2 = _NONE
            continue
        if ch in ")]":
            depth = max(0, depth - 1)
            prev = prev2 = _NONE
            continue
        if depth:
            continue
        if ch in _FOLD:
            folded, cls = _FOLD[ch], _LOWER          # lettre accentuee : minuscule, continue le mot en cours
        elif ord(ch) >= 0x80:
            prev = prev2 = _NONE                     # autre caractere non ASCII : separateur
            continue
        else:
            if "A" <= ch <= "Z":
                folded, cls = ch.lower(), _UPPER
            elif "a" <= ch <= "z":
                folded, cls = ch, _LOWER
            elif "0" <= ch <= "9":
                folded, cls = ch, _DIGIT
            else:
                prev = prev2 = _NONE                 # separateur
                continue
        for k, c in enumerate(folded):
            c_cls = cls
            start = False
            if prev == _NONE:
                start = True
            elif c_cls == _DIGIT and prev != _DIGIT:
                start = True                         # lettre -> chiffre
            elif c_cls != _DIGIT and prev == _DIGIT:
                start = True                         # chiffre -> lettre
            elif c_cls == _UPPER and prev == _LOWER:
                start = True                         # minuscule -> Majuscule
            elif c_cls == _LOWER and prev == _UPPER and prev2 == _UPPER and out:
                wb[len(out) - 1] = 1                 # fin de sigle : la derniere majuscule ouvre le mot suivant
            out.append(c)
            wb.append(1 if start else 0)
            prev2, prev = prev, c_cls
    return "".join(out), wb


def norm(text):
    return norm_words(text)[0]


def gif_info(path):
    """(cle normalisee, debuts de mots) du nom de fichier d'un GIF (sans dossier ni .gif) ; None si ce n'est pas un .gif exploitable (nom normalise < 3 caracteres)."""
    base = path.replace("\\", "/").rsplit("/", 1)[-1]
    if len(base) < 5 or base[-4:].lower() != ".gif":
        return None
    g, wb = norm_words(base[:-4])
    return (g, wb) if len(g) >= 3 else None


def gif_key(path):
    """Cle normalisee d'un chemin de GIF ; None si ce n'est pas un .gif exploitable."""
    info = gif_info(path)
    return info[0] if info else None


def whole_words(title_norm, gif_norm, gif_wb):
    """Le titre apparait dans le nom du GIF en commencant ET en finissant a une frontiere de mot (au moins une de ses occurrences)."""
    tl, gl = len(title_norm), len(gif_norm)
    p = gif_norm.find(title_norm)
    while p != -1:
        if gif_wb[p] and (p + tl == gl or gif_wb[p + tl]):
            return True
        p = gif_norm.find(title_norm, p + 1)
    return False


def rank(title_norm, gif_norm, gif_wb):
    """3 identique, 2 le GIF est contenu dans le titre, 1 le titre est contenu dans le GIF en MOTS ENTIERS, 0 aucun."""
    tl, gl = len(title_norm), len(gif_norm)
    if gl == tl and gif_norm == title_norm:
        return 3
    if gl >= 4 and gl < tl and gif_norm in title_norm:
        return 2
    if tl >= 4 and tl < gl and title_norm in gif_norm and whole_words(title_norm, gif_norm, gif_wb):
        return 1
    return 0


def best_matches(title, gif_paths):
    """(rang, [chemins]) : les GIFs du meilleur rang pour ce titre (le DMD en tire un au hasard)."""
    t = norm(title)
    if len(t) < 3:
        return 0, []
    best, found = 0, []
    for p in gif_paths:
        info = gif_info(p)
        if info is None:
            continue
        r = rank(t, info[0], info[1])
        if r == 0 or r < best:
            continue
        if r > best:
            best, found = r, []
        found.append(p)
    return best, found

# --------------------------------------------------------------------------- exclusions (v423)
EXCLUDE_FILE = "game_gifs_exclude.txt"      # /playlists/game_gifs_exclude.txt sur la SD, cree et gere par le toolkit (Mode 21)


def parse_exclusions(text, title):
    """Exclusions qui s'appliquent a ce TITRE : (exclure_tout, {noms de GIF en minuscules}).
    Fichier : une regle par ligne « titre;GIF », `#` = commentaire, espaces autour ignores.
      Kong;ARCADE_DonkeyKong06.gif   ce GIF n'est jamais joue pour ce titre (compare sur le titre NORMALISE : tous les clones / regions)
      Kong;*                         aucun GIF pour ce titre
      *;ARCADE_1Player.gif           ce GIF n'est jamais joue en partie, pour aucun jeu
    Le nom du GIF est celui du FICHIER (sans dossier), sans casse ; « .gif » facultatif ; « *;* » est ignore."""
    tn = norm(title)
    names, everything = set(), False
    for raw in text.splitlines():
        ln = raw.strip()
        if not ln or ln.startswith("#") or ";" not in ln:
            continue
        left, right = (s.strip() for s in ln.split(";", 1))
        if not left or not right:
            continue
        if left != "*" and not (len(tn) >= 3 and norm(left) == tn):
            continue
        if right == "*":
            if left != "*":
                everything = True
            continue
        right = right.replace("\\", "/").rsplit("/", 1)[-1].lower()
        names.add(right if right.endswith(".gif") else right + ".gif")
    return everything, names


def best_matches_excl(title, gif_paths, exclusions_text=""):
    """Comme best_matches, en ecartant les GIFs exclus pour ce titre AVANT le choix du rang (comme le firmware : un GIF exclu ne bloque pas un GIF de rang inferieur)."""
    everything, names = parse_exclusions(exclusions_text, title)
    if everything:
        return 0, []
    kept = [p for p in gif_paths if p.replace("\\", "/").rsplit("/", 1)[-1].lower() not in names]
    return best_matches(title, kept)

# --------------------------------------------------------------------------- MODE AUTO STRICT (v424)
# Sans dossier dedie /gifs_jeux, le DMD ne joue que les correspondances SURES : nom du GIF IDENTIQUE au titre une fois retires les prefixes « XXX_ » (jusqu'a 3) et un numero de serie final
# de 2 ou 3 chiffres commencant par 0 (01, 02 ; pas « Alien3 » ni « Sonic2 » : ce sont des suites). Les GIFs de collection (premier prefixe HALLOWEEN_, XMAS_, QUIZ_, LOGO_, MOVIES_, XXX_, HAPPY_) ne sont jamais joues automatiquement.
# Avec le dossier dedie, la regle large (rank / best_matches ci-dessus) s'applique, mais seulement a ce dossier.
_PREFIX_RE = re.compile(r"^[A-Za-z0-9]{1,10}_")
_TAIL_NUM_RE = re.compile(r"\s*0\d{1,2}$")      # numero de SERIE : 2 ou 3 chiffres commencant par 0 (01, 02, 003)
COLLECTION_PREFIXES = ("halloween", "xmas", "quiz", "logo", "movies", "xxx", "happy")


def strict_keys(gif_name):
    """Cles normalisees sous lesquelles un GIF correspond SUREMENT a un titre (mode auto strict) : son nom, et ce nom apres retrait de 1 a 3 prefixes « XXX_ », chacun avec et sans numero
    de serie final. Ensemble VIDE pour un GIF de collection (premier prefixe HALLOWEEN_, XMAS_...). Un titre correspond au GIF si son titre normalise est dans cet ensemble."""
    base = gif_name.replace("\\", "/").rsplit("/", 1)[-1]
    if base[-4:].lower() == ".gif":
        base = base[:-4]
    cands, cur = [base], base
    for i in range(3):
        m = _PREFIX_RE.match(cur)
        if not m:
            break
        if i == 0 and m.group(0)[:-1].lower() in COLLECTION_PREFIXES:
            return set()
        cur = cur[m.end():]
        cands.append(cur)
    keys = set()
    for c in cands:
        if not c:
            continue
        keys.add(norm(c))
        t = _TAIL_NUM_RE.sub("", c)
        if t and t != c:
            keys.add(norm(t))
    return keys


def strict_match(title_norm, gif_name):
    """True si le GIF `gif_name` (nom de fichier ou chemin) est une correspondance SURE du titre normalise `title_norm` (mode auto strict, dossier dedie)."""
    return len(title_norm) >= 3 and title_norm in strict_keys(gif_name)


def best_matches_strict(title, gif_paths):
    """Mode auto strict : les GIFs qui correspondent SUREMENT a ce titre (tous de meme rang ; le DMD en tire un au hasard)."""
    t = norm(title)
    return [p for p in gif_paths if gif_info(p) is not None and strict_match(t, p)]
