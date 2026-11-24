# -*- coding: utf-8 -*-
"""GIFs du jeu (2026-11-15, v424) : voir quel GIF le DMD jouera pour chaque jeu, copier / importer des GIFs dans le DOSSIER DEDIE /gifs_jeux, les renommer.

Fonctionnement (decision utilisateur) :
  - pas de dossier /gifs_jeux sur la SD -> MODE AUTO : le DMD cherche dans /gifs avec la regle STRICTE (nom du GIF identique au titre, hors prefixes XXX_ et numero de serie 01) ;
  - le dossier /gifs_jeux existe -> le DMD ne lit QUE lui (aucun repli), meme regle stricte. Il contient des COPIES (les originaux de /gifs ne sont jamais touches), nommees
    d'apres le titre du jeu, rangees en sous-dossiers par PREMIERE LETTRE du nom (A-Z, « # » sinon) ; /gifs_jeux/_index.dat liste leurs chemins relatifs (le firmware ESP32 ne parcourt
    jamais de repertoire) et DOIT etre reecrit a chaque changement (write_index).
La regle est dans gifmatch.py (copie exacte du firmware v424 et du demon Pi 3.86). Ce module (sans Tk, testable seul : test_game_gifs.py) fait le travail ; la fenetre est RecalBoxDMD_game_gifs_gui.py.

Bibliotheque de 500 000 roms : la detection est faite SYSTEME PAR SYSTEME, a la demande ; la regle stricte s'applique par INDEX (cle normalisee -> GIFs), sans double parcours jeux x GIFs.
"""
import bisect
import json
import os
import re
import shutil
import unicodedata
import zlib
from collections import defaultdict
from pathlib import Path

import gifmatch as gm
from game_gifs_store import as_store

GIFS_DIR = "gifs"
PLAYLIST_DIR = "playlists"
FOLDER = "gifs_jeux"                       # dossier dedie (racine de la SD, hors /gifs)
INDEX_NAME = "_index.dat"
STATE_REL = Path("custom") / "game_gifs_state.json"
PROTECTED_DIRS = {"_boot"}                 # animations de demarrage : jamais touchees (comme le firmware / les routes web)
_BAD_NAME = re.compile(r'[\\/:*?"<>|\x00-\x1f]')


# --------------------------------------------------------------------------- index des GIFs de /gifs (originaux)
def list_gif_files(sd_root) -> list:
    """GIFs de <SD>/gifs/<dossier>/*.gif (un seul niveau, comme le cache maitre du firmware), hors dossiers proteges :
    [{"sd": "/gifs/Arcade/x.gif", "file": Path (SD locale) ou None (Pi en Wi-Fi), "folder": "Arcade", "name": "x.gif", "size": octets, "key": cle normalisee, "wb": debuts de mots}].
    Les noms sans cle exploitable (< 3 car.) sont gardes avec key == ""."""
    st = as_store(sd_root)
    ok, dirs = st.ls("/gifs")
    out = []
    if not ok:
        return out
    for dname, isd, _sz in sorted(dirs, key=lambda d: d[0].lower()):
        if not isd or dname.lower() in PROTECTED_DIRS:
            continue
        _ok, files = st.ls(f"/{GIFS_DIR}/{dname}")
        for fname, fisd, size in sorted(files, key=lambda d: d[0].lower()):
            if not fisd and fname.lower().endswith(".gif"):
                info = gm.gif_info(fname)
                sd = f"/{GIFS_DIR}/{dname}/{fname}"
                out.append({"sd": sd, "file": st.local_file(sd) if st.kind == "sd" else None, "folder": dname, "name": fname, "size": size,
                            "key": info[0] if info else "", "wb": info[1] if info else []})
    return out


# --------------------------------------------------------------------------- detection LARGE (candidats : noms approximatifs a verifier)
def detect(games: list, gifs: list, exclusions_text: str = "") -> dict:
    """Avec `exclusions_text` (contenu de /playlists/game_gifs_exclude.txt) : applique les exclusions COMME LE DMD (un GIF exclu est ecarte AVANT le choix du rang).
    Pour chaque jeu ({"name"|"stem"}), le meilleur rang et les GIFs que le DMD jouerait : {indice du jeu: (rang, [gif, ...])} ; les jeux sans GIF sont ABSENTS.
    Meme resultat que gifmatch.best_matches(titre, tous les chemins) (regle v422 : rang 1 = MOTS ENTIERS), calcule sans le double parcours.
    Chaque gif porte "key" et "wb" (debuts de mots, voir gifmatch.norm_words)."""
    if exclusions_text and exclusions_text.strip():
        return _detect_with_exclusions(games, gifs, exclusions_text)
    by_key = defaultdict(list)
    for g in gifs:
        if g.get("key"):
            by_key[g["key"]].append(g)
    titles = defaultdict(list)                 # titre normalise -> indices de jeux
    for i, game in enumerate(games):
        tn = gm.norm(game.get("name") or game.get("stem") or "")
        if len(tn) >= 3:
            titles[tn].append(i)
    best = {}                                  # titre normalise -> (rang, [gifs])
    for tn in titles:
        if tn in by_key:
            best[tn] = (3, list(by_key[tn]))
    t_list = sorted(titles)                    # parcours deterministe
    # rang 2 : le nom du GIF est contenu dans le titre (nom >= 4 car., strictement plus court)
    if t_list:
        big = "\x00".join(t_list)
        starts, pos = [], 0
        for t in t_list:
            starts.append(pos)
            pos += len(t) + 1
        rank2 = defaultdict(list)
        for gk in by_key:
            if len(gk) < 4:
                continue
            p = big.find(gk)
            while p != -1:
                tn = t_list[bisect.bisect_right(starts, p) - 1]
                if len(gk) < len(tn) and gk not in rank2[tn]:
                    rank2[tn].append(gk)
                p = big.find(gk, p + 1)
        for tn, keys in rank2.items():
            if tn not in best:             # un nom identique (rang 3) l'emporte
                best[tn] = (2, [g for k in keys for g in by_key[k]])
    # rang 1 : le titre est contenu dans le nom du GIF en MOTS ENTIERS (titre >= 4 car., strictement plus court), seulement si rien de mieux
    gkeys = sorted(by_key)
    if gkeys:
        gbig = "\x00".join(gkeys)
        gstarts, pos = [], 0
        for k in gkeys:
            gstarts.append(pos)
            pos += len(k) + 1
        for tn in t_list:
            if len(tn) < 4 or (tn in best and best[tn][0] >= 2):
                continue
            tl = len(tn)
            hits, seen = [], set()
            p = gbig.find(tn)
            while p != -1:
                gi = bisect.bisect_right(gstarts, p) - 1
                k, off = gkeys[gi], p - gstarts[gi]
                if tl < len(k):
                    for gif in by_key[k]:
                        wb = gif["wb"]
                        if wb[off] and (off + tl == len(k) or wb[off + tl]) and id(gif) not in seen:
                            seen.add(id(gif))
                            hits.append(gif)
                p = gbig.find(tn, p + 1)
            if hits:
                best[tn] = (1, hits)
    out = {}
    for tn, idxs in titles.items():
        if tn in best:
            for i in idxs:
                out[i] = best[tn]
    return out

# --------------------------------------------------------------------------- exclusions (v423)
EXCL_REL = Path(PLAYLIST_DIR) / gm.EXCLUDE_FILE
EXCL_HEADER = ("# Exclusions de GIFs du jeu (cree par le toolkit, Mode 21).\n"
               "# titre;GIF  : ce GIF n'est jamais joue pour ce titre   |   titre;*  : aucun GIF pour ce titre   |   *;GIF  : ce GIF n'est jamais joue en partie\n")


def exclusions_path(sd_root) -> Path:
    return Path(sd_root) / EXCL_REL


def read_exclusions_text(sd_root) -> str:
    return as_store(sd_root).read_excl()


def write_exclusions_text(sd_root, text: str) -> None:
    """Ecrit le fichier d'exclusions (UTF-8, fins de ligne LF) de facon atomique ; texte vide = fichier supprime."""
    as_store(sd_root).write_excl(text)


def parse_entries(text: str) -> list:
    """Regles du fichier : [{"title", "gif", "line"}] ('*' = tous / tous les jeux) ; commentaires et lignes invalides ignores."""
    out = []
    for i, raw in enumerate(text.splitlines()):
        ln = raw.strip()
        if not ln or ln.startswith("#") or ";" not in ln:
            continue
        left, right = (s.strip() for s in ln.split(";", 1))
        if left and right:
            out.append({"title": left, "gif": right, "line": i})
    return out


def _same_rule(e, title: str, gif: str) -> bool:
    gk = lambda s: s.replace("\\", "/").rsplit("/", 1)[-1].lower()
    t_e = e["title"] if e["title"] == "*" else gm.norm(e["title"])
    t_n = title if title == "*" else gm.norm(title)
    return t_e == t_n and (e["gif"] == "*" and gif == "*" or (e["gif"] != "*" and gif != "*" and gk(e["gif"]).removesuffix(".gif") == gk(gif).removesuffix(".gif")))


def add_exclusion(sd_root, title: str, gif: str = "*") -> bool:
    """Ajoute « titre;gif » (gif = nom de fichier du GIF, ou '*' = aucun GIF pour ce titre ; title = '*' + gif = jamais ce GIF en partie). False si la regle existait deja."""
    title = title.replace(";", " ").replace("\n", " ").strip()
    gif = gif.replace(";", " ").replace("\n", " ").strip().replace("\\", "/").rsplit("/", 1)[-1] if gif != "*" else "*"
    if not title or not gif or (title == "*" and gif == "*") or (title != "*" and len(gm.norm(title)) < 3):
        raise ValueError("regle invalide")
    text = read_exclusions_text(sd_root)
    if any(_same_rule(e, title, gif) for e in parse_entries(text)):
        return False
    if not text.strip():
        text = EXCL_HEADER
    if not text.endswith("\n"):
        text += "\n"
    write_exclusions_text(sd_root, text + f"{title};{gif}\n")
    return True


def remove_exclusion(sd_root, title: str, gif: str) -> bool:
    """Retire la regle « titre;gif » ; True si une ligne a ete retiree (les commentaires sont conserves)."""
    text = read_exclusions_text(sd_root)
    ents = [e for e in parse_entries(text) if _same_rule(e, title, gif)]
    if not ents:
        return False
    drop = {e["line"] for e in ents}
    lines = text.split("\n")
    kept = [ln for i, ln in enumerate(lines) if i not in drop]
    rest = "\n".join(kept)
    # il ne reste que l'en-tete et des commentaires : le fichier est supprime
    write_exclusions_text(sd_root, rest if parse_entries(rest) else "")
    return True


def _detect_with_exclusions(games: list, gifs: list, text: str) -> dict:
    """detect() avec exclusions : exclusions globales (« *;GIF ») retirees de la liste, puis detection rapide ; les titres qui ont une regle propre sont recalcules avec la
    reference gifmatch.best_matches sur les GIFs restants (un GIF exclu ne bloque pas un GIF de rang inferieur, comme le DMD)."""
    base = lambda s: s.replace("\\", "/").rsplit("/", 1)[-1].lower()
    glob_names, by_title = set(), {}
    for e in parse_entries(text):
        gname = e["gif"] if e["gif"] == "*" else base(e["gif"])
        if gname != "*" and not gname.endswith(".gif"):
            gname += ".gif"
        if e["title"] == "*":
            if gname != "*":
                glob_names.add(gname)
            continue
        tn = gm.norm(e["title"])
        if len(tn) < 3:
            continue
        ent = by_title.setdefault(tn, {"all": False, "names": set()})
        if gname == "*":
            ent["all"] = True
        else:
            ent["names"].add(gname)
    kept = [g for g in gifs if g["name"].lower() not in glob_names]
    out = detect(games, kept)
    if not by_title:
        return out
    by_path = {g["sd"]: g for g in kept}
    paths = list(by_path)
    for i, game in enumerate(games):
        tn = gm.norm(game.get("name") or game.get("stem") or "")
        ent = by_title.get(tn)
        if ent is None:
            continue
        if ent["all"]:
            out.pop(i, None)
            continue
        allowed = [p for p in paths if base(p) not in ent["names"]]
        rank, found = gm.best_matches(game.get("name") or game.get("stem") or "", allowed)
        if rank:
            out[i] = (rank, [by_path[p] for p in found])
        else:
            out.pop(i, None)
    return out


def send_exclusions(ip: str, text: str, timeout: float = 8.0):
    """Envoie le fichier d'exclusions au DMD (ESP32 >= 4.23 ou demon Pi >= 3.85) : POST /game-gifs-exclude. Retourne (ok, detail)."""
    import urllib.request
    try:
        req = urllib.request.Request(f"http://{ip}/game-gifs-exclude", data=text.encode("utf-8"), method="POST", headers={"Content-Type": "text/plain; charset=utf-8"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            body = r.read().decode("utf-8", "replace")
            return (200 <= r.status < 300), body[:200]
    except Exception as e:   # noqa: BLE001
        return False, f"{type(e).__name__}: {e}"


# --------------------------------------------------------------------------- noms de fichiers
def clean_title_filename(title: str) -> str:
    """Nom de fichier propose pour un GIF de ce titre : accents replies (le DMD / le Pi lisent la FAT en ASCII), caracteres interdits retires, espaces normalises, sans .gif."""
    s = unicodedata.normalize("NFKD", title or "")
    s = "".join(c for c in s if not unicodedata.combining(c))
    s = s.replace("’", "'").replace("–", "-").replace("—", "-")
    s0 = s
    s = re.sub(r"\([^)]*\)|\[[^\]]*\]", " ", s)         # etiquettes (USA), [!]... : ignorees par la regle, inutiles dans un nom
    if len(gm.norm(s)) < 3:
        s = s0
    s = _BAD_NAME.sub(" ", s)
    s = "".join(c for c in s if 32 <= ord(c) < 127)
    s = re.sub(r"\s+", " ", s).strip(" .")
    return s[:80]


def check_new_name(new_name: str) -> str:
    """'' si le nom convient, sinon la raison (FR) : .gif obligatoire, ASCII, caracteres interdits, longueur."""
    if not new_name.lower().endswith(".gif") or len(new_name) < 5:
        return "le nom doit se terminer par .gif"
    if _BAD_NAME.search(new_name):
        return "caracteres interdits : \\ / : * ? \" < > |"
    if any(ord(c) > 126 for c in new_name):
        return "caracteres accentues interdits (la carte SD est lue en ASCII)"
    if len(new_name) > 100:
        return "nom trop long"
    return ""


def all_titles(snap) -> dict:
    """Titres de TOUS les systemes de l'instantane : {titre normalise: (titre affiche, systeme)}. Un GIF est cherche par le DMD pour n'importe quel jeu, pas seulement
    pour ceux du systeme affiche : le controle des faux positifs doit porter sur toute la bibliotheque."""
    out = {}
    for system, ent in (snap or {}).get("systems", {}).items():
        for g in ent.get("games", []):
            if g.get("h"):
                continue
            name = str(g.get("n") or re.sub(r"\.[^.]*$", "", str(g.get("r", ""))))
            tn = gm.norm(name)
            if len(tn) >= 3 and tn not in out:
                out[tn] = (name, system)
    return out


# --------------------------------------------------------------------------- dossier dedie /gifs_jeux
def folder_dir(sd_root) -> Path:
    return Path(sd_root) / FOLDER


def has_folder(sd_root) -> bool:
    """Le dossier dedie est ACTIF s'il existe ET contient au moins un GIF ; un dossier vide = mode auto (comme le firmware v425 / le demon Pi 3.88)."""
    st = as_store(sd_root)
    ok, top = st.ls("/" + FOLDER)
    if not ok:
        return False
    for name, isd, _sz in top:
        if isd:
            if any(not d and n.lower().endswith(".gif") for n, d, _s in st.ls(f"/{FOLDER}/{name}")[1]):
                return True
        elif name.lower().endswith(".gif"):
            return True
    return False


def reset_if_empty(sd_root) -> bool:
    """Plus aucun GIF dans le dossier dedie : on le supprime (index compris) pour revenir au mode AUTO. Retourne True si c'est fait / deja le cas. Sur le Pi (pas de suppression de
    dossier par l'API) seul l'index est retire : un dossier sans .gif vaut le mode auto pour le demon."""
    st = as_store(sd_root)
    ok, top = st.ls("/" + FOLDER)
    if not ok:
        return True
    if has_folder(st):
        return False
    try:
        for name, isd, _sz in top:
            if not isd and name == INDEX_NAME:
                st.rm(f"/{FOLDER}/{name}")
        if st.kind == "sd":
            d = st.p("/" + FOLDER)
            for sub in list(d.iterdir()):
                if sub.is_dir() and not any(sub.iterdir()):
                    sub.rmdir()
            if not any(d.iterdir()):
                d.rmdir()
    except OSError:
        return False
    return True


def letter_dir(name: str) -> str:
    """Sous-dossier d'un fichier : sa premiere lettre en majuscule (A-Z), « # » sinon (meme regle que les logos du firmware : alphaSubdirPath)."""
    c = (name[:1] or "#").upper()
    return c if "A" <= c <= "Z" else "#"


def list_folder_gifs(sd_root) -> list:
    """GIFs de <SD>/gifs_jeux (sous-dossiers par lettre + a plat), meme format que list_gif_files ; "folder" = lettre ('' si a plat)."""
    st = as_store(sd_root)
    ok, top = st.ls("/" + FOLDER)
    out = []
    if not ok:
        return out

    def add(fname, sub, size):
        info = gm.gif_info(fname)
        sd = f"/{FOLDER}/{sub}/{fname}" if sub else f"/{FOLDER}/{fname}"
        out.append({"sd": sd, "file": st.local_file(sd) if st.kind == "sd" else None, "folder": sub, "name": fname, "size": size,
                    "key": info[0] if info else "", "wb": info[1] if info else []})
    for name, isd, size in sorted(top, key=lambda d: d[0].lower()):
        if isd:
            _ok, files = st.ls(f"/{FOLDER}/{name}")
            for fname, fisd, fsize in sorted(files, key=lambda d: d[0].lower()):
                if not fisd and fname.lower().endswith(".gif"):
                    add(fname, name, fsize)
        elif name.lower().endswith(".gif"):
            add(name, "", size)
    return out


def write_index(sd_root) -> int:
    """Reecrit /gifs_jeux/_index.dat (un chemin RELATIF par ligne, « A/Alien Storm.gif ») ; sans dossier, ne fait rien. Retourne le nombre de lignes.
    Le firmware ESP32 ne lit QUE cet index (il ne parcourt jamais le dossier) : toute modification du dossier doit finir par cet appel. (Le demon Pi parcourt le dossier ; l'index lui sert si la carte change d'appareil.)"""
    st = as_store(sd_root)
    if not has_folder(st):
        return 0
    rels = [g["sd"][len(f"/{FOLDER}/"):] for g in list_folder_gifs(st)]
    st.put(f"/{FOLDER}/{INDEX_NAME}", ("\n".join(rels) + ("\n" if rels else "")).encode("utf-8"))
    return len(rels)


def active_gifs(sd_root) -> list:
    """GIFs que le DMD lit REELLEMENT : ceux du dossier dedie s'il existe (aucun repli), sinon ceux de /gifs."""
    return list_folder_gifs(sd_root) if has_folder(sd_root) else list_gif_files(sd_root)


# --------------------------------------------------------------------------- regle stricte (mode auto ET dossier dedie)
def _excl_tables(text: str):
    """(noms de GIF exclus pour tous les jeux, {titre normalise: {"all": bool, "names": {noms}}}) du fichier d'exclusions."""
    base = lambda s: s.replace("\\", "/").rsplit("/", 1)[-1].lower()
    glob_names, by_title = set(), {}
    for e in parse_entries(text):
        gname = e["gif"] if e["gif"] == "*" else base(e["gif"])
        if gname != "*" and not gname.endswith(".gif"):
            gname += ".gif"
        if e["title"] == "*":
            if gname != "*":
                glob_names.add(gname)
            continue
        tn = gm.norm(e["title"])
        if len(tn) < 3:
            continue
        ent = by_title.setdefault(tn, {"all": False, "names": set()})
        if gname == "*":
            ent["all"] = True
        else:
            ent["names"].add(gname)
    return glob_names, by_title


def detect_strict(games: list, gifs: list, exclusions_text: str = "") -> dict:
    """Ce que le DMD jouera : {indice du jeu: (3, [gif, ...])} (les jeux sans GIF sont ABSENTS), regle stricte (gifmatch.strict_keys) et exclusions appliquees."""
    glob_names, by_title = _excl_tables(exclusions_text) if exclusions_text.strip() else (set(), {})
    idx = defaultdict(list)
    for g in gifs:
        if g["name"].lower() in glob_names:
            continue
        for k in gm.strict_keys(g["name"]):
            if len(k) >= 3:
                idx[k].append(g)
    out = {}
    for i, game in enumerate(games):
        tn = gm.norm(game.get("name") or game.get("stem") or "")
        lst = idx.get(tn) if len(tn) >= 3 else None
        if not lst:
            continue
        ent = by_title.get(tn)
        if ent is not None:
            if ent["all"]:
                continue
            lst = [g for g in lst if g["name"].lower() not in ent["names"]]
        seen, uniq = set(), []
        for g in lst:
            if id(g) not in seen:
                seen.add(id(g))
                uniq.append(g)
        if uniq:
            out[i] = (3, uniq)
    return out


def export_name(title: str, taken) -> str:
    """Nom de fichier d'un GIF du dossier pour ce jeu : le titre (« Alien Storm.gif »), puis « Alien Storm 01.gif », « 02 »... jusqu'a un nom libre (`taken` = noms deja pris, en minuscules).
    Retourne '' si aucun nom ne correspond (titre trop court). Le nom correspond TOUJOURS au titre pour la regle stricte (verifie)."""
    stem = clean_title_filename(title).replace("_", " ").strip()
    tn = gm.norm(title)
    if len(gm.norm(stem)) < 3 or len(tn) < 3:
        return ""
    cands = [stem] + [f"{stem} {n:02d}" for n in range(1, 100)]
    for c in cands:
        name = c + ".gif"
        if name.lower() in taken or check_new_name(name):
            continue
        if gm.strict_match(tn, name):
            return name
    return ""


def _copy_atomic(src: Path, dest: Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name + ".tmp")
    shutil.copyfile(src, tmp)
    os.replace(tmp, dest)


def add_many(sd_root, items: list) -> dict:
    """Copie des GIFs dans le dossier dedie (cree si besoin) : items = [{"src": fichier local (Path) OU "src_sd": chemin dans la SD / le Pi (copie sur place), "title": titre du jeu,
    "name": nom voulu (facultatif)}]. Les originaux ne sont jamais modifies. Un nom voulu doit correspondre au titre (regle stricte), sinon l'element est refuse. L'index est reecrit UNE fois.
    Retourne {"done": [(item, new_sd)], "failed": [(item, raison)]}."""
    st = as_store(sd_root)
    taken = {g["name"].lower() for g in list_folder_gifs(st)}
    done, failed = [], []
    for it in items:
        title = it["title"]
        src_sd = it.get("src_sd")
        if not src_sd:
            src = Path(it["src"])
            if not src.is_file() or src.suffix.lower() != ".gif":
                failed.append((it, "fichier .gif introuvable"))
                continue
        name = it.get("name") or ""
        if name:
            if not name.lower().endswith(".gif"):
                name += ".gif"
            why = check_new_name(name)
            if why:
                failed.append((it, why))
                continue
            if not gm.strict_match(gm.norm(title), name):
                failed.append((it, "ce nom ne correspond pas au titre du jeu (il doit etre identique au titre, a un numero 01, 02 pres)"))
                continue
            if name.lower() in taken:
                failed.append((it, "un GIF porte deja ce nom dans le dossier"))
                continue
        else:
            name = export_name(title, taken)
            if not name:
                failed.append((it, "titre trop court pour un nom fiable"))
                continue
        dest = f"/{FOLDER}/{letter_dir(name)}/{name}"
        try:
            if src_sd:
                st.cp(src_sd, dest)
            else:
                st.put_file(src, dest)
        except OSError as e:
            failed.append((it, str(e)))
            continue
        taken.add(name.lower())
        done.append((it, dest))
    try:
        write_index(st)
    except OSError as e:
        failed.append(({"title": "", "name": INDEX_NAME}, f"index non ecrit : {e}"))
    return {"done": done, "failed": failed}


def create_folder(sd_root) -> bool:
    """Cree le dossier dedie (vide, avec un index vide) sur la SD ou, en Wi-Fi, sur le Pi. Retourne False s'il existe deja. Un dossier SANS GIF = mode auto (firmware >= 4.25 /
    demon >= 3.88) : le mode dossier ne s'active qu'au premier GIF ajoute."""
    st = as_store(sd_root)
    if st.ls("/" + FOLDER)[0]:
        return False
    st.put(f"/{FOLDER}/{INDEX_NAME}", b"")
    return True


DMD_SIZE = (128, 32)


def native_ok(path) -> bool:
    """Le fichier est deja un GIF au format de la dalle (128x32) et ne depasse pas le maximum d'images : copie telle quelle, sans re-conversion."""
    import custom_logos as cl
    from PIL import Image
    try:
        with Image.open(path) as im:
            return im.format == "GIF" and im.size == DMD_SIZE and int(getattr(im, "n_frames", 1)) <= cl.MAX_ANIM_FRAMES
    except Exception:   # noqa: BLE001
        return False


def convert_gif(src, dst, mode: str = "pixel", bg=(0, 0, 0), start: int = 0, end=None, still=None) -> dict:
    """MEME MOTEUR que l'onglet de remplacement de logo (custom_logos.render : pixel-perfect / ajuster / remplir / recadrer, fond) : chaque image du fichier (GIF, WebP, PNG, JPEG...)
    est rendue en 128x32 puis ecrite en GIF anime. start / end : extrait (images depuis 0, `end` inclus) ; still : numero d'UNE image a capturer (GIF fixe). Au-dela de
    MAX_ANIM_FRAMES : 1 image sur k, delais SOMMES (duree totale conservee, jamais de coupe). Retourne {"frames", "duration_ms", "resampled"}."""
    import custom_logos as cl
    from PIL import Image, ImageSequence
    frames, delays = [], []
    n = cl.gif_frame_count(src)
    if still is not None:
        a = b = max(0, min(int(still), n - 1))
    else:
        a = max(0, int(start or 0))
        b = n - 1 if end is None else min(n - 1, int(end))
    if a > b:
        raise ValueError(f"extrait vide ({a} > {b})")
    count = b - a + 1
    step = -(-count // cl.MAX_ANIM_FRAMES) if count > cl.MAX_ANIM_FRAMES else 1
    with cl._tool()._open_image_tolerant(Path(src)) as im:
        fallback = im.info.get("duration", 100)
        for i, fr in enumerate(ImageSequence.Iterator(im)):
            if i < a:
                continue
            if i > b:
                break
            try:
                d = int(fr.info.get("duration", fallback))
            except (TypeError, ValueError):
                d = 100
            d = max(20, d if d > 0 else 100)
            if (i - a) % step:
                delays[-1] = min(0xFFFF, delays[-1] + d)
                continue
            frames.append(cl.render(fr.convert("RGBA"), mode, tuple(bg)))
            delays.append(min(0xFFFF, d))
    if not frames:
        raise ValueError("aucune image lisible")
    if len(frames) == 1:
        delays = [1000]
    pal = [f.convert("RGB").quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE) for f in frames]
    pal[0].save(str(dst), save_all=True, append_images=pal[1:], duration=delays, loop=0, optimize=False, disposal=1)
    return {"frames": len(frames), "duration_ms": sum(delays), "resampled": step if step > 1 else 0}


def import_gif(sd_root, src_file, title: str, name: str = "") -> dict:
    """Importe un GIF du PC dans le dossier dedie pour ce jeu (nom = titre du jeu, ou `name` s'il correspond au titre). Retourne {"ok", "error", "new_sd"}."""
    r = add_many(sd_root, [{"src": src_file, "title": title, "name": name}])
    if r["done"]:
        return {"ok": True, "error": "", "new_sd": r["done"][0][1]}
    return {"ok": False, "error": r["failed"][0][1] if r["failed"] else "echec"}


def rename_in_folder(sd_root, sd_path: str, new_name: str, title: str) -> dict:
    """Renomme un GIF DU DOSSIER dedie (il peut changer de sous-dossier) ; le nouveau nom doit correspondre au titre du jeu. Les exclusions qui citent l'ancien nom suivent.
    Retourne {"ok", "error", "new_sd"}."""
    st = as_store(sd_root)
    m = re.fullmatch(rf"/{FOLDER}/(?:([^/]+)/)?([^/]+)", sd_path)
    if not m:
        return {"ok": False, "error": "GIF hors du dossier " + FOLDER}
    old_name = m.group(2)
    why = check_new_name(new_name)
    if why:
        return {"ok": False, "error": why}
    if not gm.strict_match(gm.norm(title), new_name):
        return {"ok": False, "error": "ce nom ne correspond pas au titre du jeu (identique au titre, a un numero 01, 02 pres)"}
    existing = {g["sd"] for g in list_folder_gifs(st)}
    if sd_path not in existing:
        return {"ok": False, "error": "fichier introuvable"}
    dst = f"/{FOLDER}/{letter_dir(new_name)}/{new_name}"
    if dst.lower() in {e.lower() for e in existing} and dst.lower() != sd_path.lower():
        return {"ok": False, "error": "un GIF porte deja ce nom dans le dossier"}
    try:
        st.mv(sd_path, dst)
        _rename_in_exclusions(st, old_name, new_name)
        write_index(st)
    except OSError as e:
        return {"ok": False, "error": str(e)}
    return {"ok": True, "error": "", "new_sd": dst}


def _rename_in_exclusions(sd_root, old_name: str, new_name: str) -> int:
    """Les regles d'exclusion citent le GIF par son nom de fichier : elles suivent un renommage. Retourne le nombre de lignes changees."""
    text = read_exclusions_text(sd_root)
    if not text:
        return 0
    base = lambda s: s.replace("\\", "/").rsplit("/", 1)[-1].lower().removesuffix(".gif")
    n, out = 0, []
    for ln in text.split("\n"):
        s = ln.strip()
        if s and not s.startswith("#") and ";" in s:
            left, right = (x.strip() for x in s.split(";", 1))
            if right != "*" and base(right) == base(old_name):
                ln = f"{left};{new_name}"
                n += 1
        out.append(ln)
    if n:
        write_exclusions_text(sd_root, "\n".join(out))
    return n


def remove_from_folder(sd_root, sd_path: str) -> dict:
    """Retire un GIF du dossier dedie (la copie seulement : l'original de /gifs n'est pas touche) et reecrit l'index."""
    st = as_store(sd_root)
    if not re.fullmatch(rf"/{FOLDER}/(?:[^/]+/)?[^/]+", sd_path):
        return {"ok": False, "error": "GIF hors du dossier " + FOLDER}
    try:
        st.rm(sd_path)
        if has_folder(st):
            write_index(st)
        else:
            reset_if_empty(st)                 # dernier GIF retire : retour au mode auto
    except OSError as e:
        return {"ok": False, "error": str(e)}
    return {"ok": True, "error": ""}


def clear_folder(sd_root) -> int:
    """Vide le dossier dedie : supprime tous ses GIFs (les originaux de /gifs ne sont pas touches), puis le dossier et son index (retour au mode AUTO ; sur le Pi seul l'index
    est retire, un dossier sans GIF vaut deja le mode auto). Retourne le nombre de GIFs supprimes. Les exclusions ne sont pas touchees."""
    st = as_store(sd_root)
    gifs = list_folder_gifs(st)
    for g in gifs:
        st.rm(g["sd"])
    reset_if_empty(st)
    return len(gifs)


# --------------------------------------------------------------------------- nouveautes + synchronisation (export des originaux vers le dossier)
def read_state(sd_root) -> dict:
    """Etat memorise dans <SD>/custom/game_gifs_state.json : {"exported": {"<chemin source>|<titre normalise>": chemin dans le dossier} (un meme GIF peut servir plusieurs titres),
    "seen_gifs": [...], "seen_titles": [crc32 du titre normalise]}."""
    try:
        d = json.loads(as_store(sd_root).get("/" + STATE_REL.as_posix()).decode("utf-8"))
        return {"exported": dict(d.get("exported", {})), "seen_gifs": set(d.get("seen_gifs", [])), "seen_titles": set(d.get("seen_titles", [])), "known": True}
    except (OSError, ValueError):
        return {"exported": {}, "seen_gifs": set(), "seen_titles": set(), "known": False}


def write_state(sd_root, st: dict) -> None:
    as_store(sd_root).put("/" + STATE_REL.as_posix(), json.dumps({"exported": st["exported"], "seen_gifs": sorted(st["seen_gifs"]), "seen_titles": sorted(st["seen_titles"])}).encode("utf-8"))


def _thash(tn: str) -> int:
    return zlib.crc32(tn.encode("utf-8"))


def news(sd_root, snap, src_gifs: list) -> dict:
    """Nouveautes depuis le dernier controle : {"gifs": n, "games": n, "first": True si aucun controle encore}. `src_gifs` = list_gif_files (originaux de /gifs)."""
    st = read_state(sd_root)
    titles = all_titles(snap)
    return {"gifs": sum(1 for g in src_gifs if g["sd"] not in st["seen_gifs"]), "games": sum(1 for tn in titles if _thash(tn) not in st["seen_titles"]), "first": not st["known"]}


def sync_plan(sd_root, snap, src_gifs: list, exclusions_text: str = "") -> list:
    """Copies proposees vers le dossier : pour chaque jeu de la bibliotheque (tous systemes), les GIFs de /gifs qui correspondent SUREMENT a son titre (regle stricte) et qui n'ont pas
    deja ete copies. [{"src": gif, "title", "system", "new": nom propose, "new_gif": bool, "new_game": bool, "size": octets}] ; les noms sont provisoires (add_many les recalcule)."""
    st = read_state(sd_root)
    titles = all_titles(snap)
    items = list(titles.items())
    games = [{"name": d, "system": s} for _tn, (d, s) in items]
    det = detect_strict(games, src_gifs, exclusions_text)
    folder = list_folder_gifs(sd_root)
    taken = {g["name"].lower() for g in folder}
    existing = {g["sd"] for g in folder}
    plan = []
    for i, (_rank, gl) in sorted(det.items()):
        tn, (disp, system) = items[i]
        for g in gl:
            dest = st["exported"].get(f"{g['sd']}|{tn}")
            if dest and dest in existing:
                continue                       # deja copie (et toujours la)
            name = export_name(disp, taken)
            if not name:
                continue
            taken.add(name.lower())
            plan.append({"src": g, "title": disp, "system": system, "new": name, "new_gif": g["sd"] not in st["seen_gifs"], "new_game": _thash(tn) not in st["seen_titles"], "size": g.get("size", 0)})
    return plan


def apply_sync(sd_root, snap, src_gifs: list, rows: list) -> dict:
    """Copie les lignes choisies du plan dans le dossier, memorise ce qui a ete copie et marque TOUT l'etat courant comme « vu ». Retourne {"done", "failed"}."""
    res = add_many(sd_root, [{"src_sd": r["src"]["sd"], "title": r["title"]} for r in rows])
    st = read_state(sd_root)
    for it, dest in res["done"]:
        st["exported"][f"{it['src_sd']}|{gm.norm(it['title'])}"] = dest
    st["seen_gifs"] = {g["sd"] for g in src_gifs}
    st["seen_titles"] = {_thash(tn) for tn in all_titles(snap)}
    write_state(sd_root, st)
    return {"done": len(res["done"]), "failed": res["failed"]}


def mark_seen(sd_root, snap, src_gifs: list) -> None:
    """Marque les GIFs et jeux actuels comme « vus » (sans rien copier)."""
    st = read_state(sd_root)
    st["seen_gifs"] = {g["sd"] for g in src_gifs}
    st["seen_titles"] = {_thash(tn) for tn in all_titles(snap)}
    write_state(sd_root, st)

def record_export(sd_root, src_sd: str, dest_sd: str, title: str) -> None:
    """Memorise qu'un GIF de /gifs a ete copie dans le dossier pour ce titre (pour ne pas le reproposer a la prochaine mise a jour)."""
    st = read_state(sd_root)
    st["exported"][f"{src_sd}|{gm.norm(title)}"] = dest_sd
    write_state(sd_root, st)


def local_file(sd_root, gif: dict) -> Path:
    """Fichier local d'un GIF de la liste (apercu, copie) : le fichier meme sur une SD, ou une copie telechargee depuis le Pi."""
    return gif["file"] or as_store(sd_root).local_file(gif["sd"])
