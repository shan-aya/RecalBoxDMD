"""Logos de jeux PERSONNALISES (2026-10-28) : importer une image, la controler, la convertir au format de la dalle et la rejouer apres tout ecrasement.

Pourquoi : un logo faux/manquant ne se corrigeait qu'en retouchant le scrape puis en relancant les Modes 1/8 ; aucune retouche manuelle ne survivait a un
nouveau scrape destructeur. Ici chaque remplacement est consigne dans UN SEUL fichier `custom/logos.json` a la racine de la SD (FAT32 : un petit fichier par
logo gaspillerait un cluster de 4-32 Ko chacun ; le PNG 128x32 est donc integre en base64, ~1-2 Ko par entree) et dans un miroir sur le PC
(%APPDATA%\\RecalBoxDMD\\custom\\logos.json, jamais dans Temp que Windows vide). `apply_store()` reecrit les .raw565 voulus : absents, ecrases par un nouveau
scrape, ou remplaces par un Mode 8 qui saute les fichiers de meme taille (tous les .raw565 font 8192 octets).

Cle d'une entree = `<systeme>/<stem>` ou stem = nom de fichier ROM sans extension, espaces supprimes, `\\/:*?"<>|` -> `_` : meme regle que marquee.sh, le
firmware et `sanitize_filename()` du toolkit. Cible = `systems/<systeme>/<Lettre|#>/<stem>.raw565`.

Module sans Tk : testable seul (test_custom_logos.py). Le moteur de conversion (`_fit_logo_rgb`) et le cache bigramme (`build_cache`) viennent du toolkit,
importes a l'usage. Dependances : Pillow + numpy.
"""
import base64
import hashlib
import io
import json
import os
import re
import zlib
from datetime import datetime
from pathlib import Path

W, H = 128, 32
RAW_SIZE = W * H * 2
STORE_VERSION = 1
STORE_REL = Path("custom") / "logos.json"
MODES = ("pixel", "fit", "fill", "crop")
MAX_ANIM_FRAMES = 600          # 600 images = 4,9 Mo de raw565pack sur la SD ; ~250 Ko compresse dans le store. Le firmware lit les images une a une sur la SD (2 o/image en heap) : pas de plafond cote ESP32 ; 94 % du pack Ultimate GIFs tient dans 600
TYPE_STATIC, TYPE_ANIM = 0x70, 0x67   # octet « type » du games_cache.bin (EXTENSIONS_CACHE du toolkit)
# jeu de noms de systeme Recalbox : lettres/chiffres/_/- (le `custom` du JSON peut venir d'un paquet partage : rien d'autre ne doit atteindre le disque)
_SYSTEM_RE = re.compile(r"^[A-Za-z0-9_-]+$")
# un logo est « presque noir » si rien n'est allume au-dessus de ce seuil de luminance, ou si moins de 1 % des LED le sont
DARK_MAX_LUMA = 60
DARK_LIT_LUMA = 24
DARK_LIT_SHARE = 0.01
DARK_DIM_SHARE = 0.10   # un logo dont le maximum est < DARK_MAX_LUMA n'est « noir » que si moins de 10 % des LED sont allumees


def _tool():
    import RecalBoxDMD_tool as tool
    return tool


# --------------------------------------------------------------------------- cles / chemins
def rom_basename(rom: str) -> str:
    """Dernier composant d'un chemin de ROM, / ou \\ : pas de pathlib (sous Windows « a:b.zip » y devient lecteur « a: » + « b.zip »)."""
    return re.split(r"[\\/]", str(rom))[-1]


def stem_for_rom(rom: str) -> str:
    """Nom de fichier du visuel d'une ROM : exactement `basename | sed 's/\\.[^.]*$//; s/ //g'` de marquee.sh, puis sanitize_filename() du toolkit."""
    return _tool().sanitize_filename(re.sub(r"\.[^.]*$", "", rom_basename(rom)))


def make_key(system: str, rom: str) -> str:
    return f"{system}/{stem_for_rom(rom)}"


def split_key(key: str):
    """(systeme, stem) d'une cle, ou ValueError : refuse tout ce qui pourrait sortir de systems/<systeme>/ (« .. », separateurs, _defaults)."""
    if not isinstance(key, str) or key.count("/") != 1:
        raise ValueError(f"cle invalide : {key!r}")
    system, stem = key.split("/")
    if not _SYSTEM_RE.match(system) or system.lower() == "_defaults":
        raise ValueError(f"systeme invalide : {system!r}")
    if not stem or set(stem) <= {"."} or any(c in stem for c in '\\/:*?"<>|\x00') or stem != stem.strip():
        raise ValueError(f"nom de jeu invalide : {stem!r}")
    return system, stem


def target_path(sd_root, key: str) -> Path:
    system, stem = split_key(key)
    bucket = _tool()._bucket_letter_for_stem(stem)
    return Path(sd_root) / "systems" / system / bucket / f"{stem}.raw565"


def store_path(sd_root) -> Path:
    return Path(sd_root) / STORE_REL


def mirror_path() -> Path:
    base = os.environ.get("APPDATA") or str(Path.home())
    return Path(base) / "RecalBoxDMD" / "custom" / "logos.json"


# --------------------------------------------------------------------------- conversion
def rgb_to_raw565(img_rgb) -> bytes:
    """Image RGB 128x32 -> 8192 octets RGB565 petit-boutiste, ligne par ligne (meme encodage que le toolkit)."""
    import numpy as np

    if img_rgb.size != (W, H):
        raise ValueError(f"taille {img_rgb.size}, {W}x{H} attendu")
    a = np.asarray(img_rgb.convert("RGB"), dtype=np.uint16)
    v = ((a[..., 0] & 0xF8) << 8) | ((a[..., 1] & 0xFC) << 3) | (a[..., 2] >> 3)
    return v.astype("<u2").tobytes()


def _flatten(img_rgba, bg):
    from PIL import Image

    base = Image.new("RGBA", img_rgba.size, (*bg, 255))
    base.alpha_composite(img_rgba)
    return base


def render(img_rgba, mode: str = "pixel", bg=(0, 0, 0)):
    """Image RGBA quelconque -> RGB 128x32.
    pixel : moteur du toolkit (echelle entiere, NEAREST/BOX, fidele a l'original ; letterbox noir) ;
    fit : tient en entier (LANCZOS, bandes de la couleur de fond) ; fill : etire aux 128x32 ; crop : remplit la dalle et rogne le surplus."""
    from PIL import Image, ImageOps

    if mode not in MODES:
        raise ValueError(f"mode inconnu : {mode!r}")
    flat = _flatten(img_rgba.convert("RGBA"), bg)
    if mode == "pixel":
        return _tool()._fit_logo_rgb(flat)
    rgb = flat.convert("RGB")
    if mode == "fill":
        return rgb.resize((W, H), Image.LANCZOS)
    if mode == "crop":
        return ImageOps.fit(rgb, (W, H), Image.LANCZOS)
    ratio = min(W / rgb.width, H / rgb.height)
    small = rgb.resize((max(1, round(rgb.width * ratio)), max(1, round(rgb.height * ratio))), Image.LANCZOS)
    canvas = Image.new("RGB", (W, H), bg)
    canvas.paste(small, ((W - small.width) // 2, (H - small.height) // 2))
    return canvas


def is_dark(img_rgb) -> bool:
    """Logo « presque noir » sur la dalle : moins de 1 % des LED allumees (> DARK_LIT_LUMA), ou rien au-dessus de DARK_MAX_LUMA ET moins de DARK_DIM_SHARE des LED
    allumees. 2026-11-16 : l'ancienne regle « rien au-dessus de DARK_MAX_LUMA » seule classait « noir » un logo sombre mais lisible (3DO Striker - World Cup Special :
    maximum 49, 28 % des LED allumees) ; un logo sombre n'est noir que s'il n'allume presque rien."""
    import numpy as np

    luma = np.asarray(img_rgb.convert("L"), dtype=np.float32)
    lit = float((luma > DARK_LIT_LUMA).mean())
    return bool(lit < DARK_LIT_SHARE or (luma.max() < DARK_MAX_LUMA and lit < DARK_DIM_SHARE))


CARD_MAX_DIFF = 14.0   # ecart moyen (0-255 par canal) sous lequel un visuel est la carte texte du jeu ; une carte rendue de nouveau tombe a ~1-3, un vrai logo reste > 35


def looks_like_card(img_rgb, name: str) -> bool:
    """Le visuel est-il vraiment la CARTE TEXTE de `name` ? Le drapeau « c » de l'instantane date du scrape (Mode 1 / Mode 18) : si un vrai logo a remplace la carte depuis
    (Mode 1 relance, logos reels envoyes au Pi...), le drapeau reste. On rend donc la carte du nom (text_card.render_card, deterministe) et on la compare au fichier."""
    import numpy as np

    try:
        import text_card
        ref = text_card.render_card(name)
    except Exception:   # noqa: BLE001
        return True      # impossible de verifier : on garde l'etat annonce par l'instantane
    a = np.asarray(img_rgb.convert("RGB"), dtype=np.int16)
    b = np.asarray(ref.convert("RGB"), dtype=np.int16)
    return a.shape == b.shape and float(np.abs(a - b).mean()) < CARD_MAX_DIFF


def analyze(src_rgba, out_rgb) -> dict:
    """Controles affiches avant validation. `warnings` = codes (le texte est a la charge de l'interface) :
    dark (presque noir sur la dalle), transparent (aplati sur le fond choisi), upscaled (source plus petite que la dalle), narrow (ratio tres eloigne de 4:1)."""
    import numpy as np

    out = np.asarray(out_rgb.convert("L"), dtype=np.float32)
    lit = float((out > DARK_LIT_LUMA).mean())
    max_luma = float(out.max())
    dark = is_dark(out_rgb)
    sw, sh = src_rgba.size
    alpha_min = int(np.asarray(src_rgba.getchannel("A")).min()) if "A" in src_rgba.getbands() else 255
    warnings = []
    if dark:
        warnings.append("dark")
    if alpha_min < 255:
        warnings.append("transparent")
    if sw < W and sh < H:
        warnings.append("upscaled")
    if sw / sh > 12 or sw / sh < 1.2:
        warnings.append("narrow")
    return {"src_size": (sw, sh), "ratio": round(sw / sh, 3), "lit_share": round(lit, 4), "max_luma": round(max_luma, 1), "warnings": warnings}


def make_item(system: str, rom: str, src_path, name: str = "", mode: str = "pixel", bg=(0, 0, 0), stem: str = None):
    """Image sur disque -> (entree du store, rapport d'analyse, apercu RGB 128x32). Une image animee donne sa premiere image.
    stem : nom de visuel deja connu (jeu issu de la SD) -- evite de re-retirer une « extension » a un nom comme « foo.bar »."""
    key = f"{system}/{stem}" if stem else make_key(system, rom)
    split_key(key)
    tool = _tool()
    with tool._open_image_tolerant(Path(src_path)) as im:
        try:
            im.seek(0)
        except EOFError:
            pass
        src = im.convert("RGBA")
    out = render(src, mode, tuple(bg))
    report = analyze(src, out)
    buf = io.BytesIO()
    out.save(buf, "PNG", optimize=True)
    item = {
        "key": key,
        "rom": rom_basename(rom),
        "name": name or (stem if stem else re.sub(r"\.[^.]*$", "", rom_basename(rom))),
        "mode": mode,
        "bg": "#%02x%02x%02x" % tuple(bg),
        "png_b64": base64.b64encode(buf.getvalue()).decode("ascii"),
        "sha1": hashlib.sha1(rgb_to_raw565(out)).hexdigest(),
        "src": Path(str(src_path)).name,
        "date": datetime.now().isoformat(timespec="seconds"),
    }
    return item, report, out


def is_anim(item: dict) -> bool:
    return bool(item.get("pack_z"))


def item_files(item: dict) -> list:
    """Fichiers a ecrire pour une entree : [(extension, octets)]. Image fixe -> .raw565 ; animation -> .raw565pack + .meta (delais uint16 LE par image).
    ValueError si l'entree est alteree (empreinte, nombre d'images, taille)."""
    if not is_anim(item):
        return [(".raw565", item_raw(item))]
    try:
        pack = zlib.decompress(base64.b64decode(item["pack_z"]), 15, MAX_ANIM_FRAMES * RAW_SIZE + 1)
        delays = [int(d) for d in item["delays"]]
    except (zlib.error, ValueError, KeyError, TypeError) as e:
        raise ValueError(f"animation illisible pour {item.get('key')} : {e}") from None
    if not (2 <= len(delays) <= MAX_ANIM_FRAMES) or len(pack) != len(delays) * RAW_SIZE or not all(1 <= d <= 0xFFFF for d in delays):
        raise ValueError(f"animation incoherente pour {item.get('key')}")
    if item.get("sha1") and hashlib.sha1(pack).hexdigest() != item["sha1"]:
        raise ValueError(f"empreinte differente pour {item.get('key')}")
    import struct
    return [(".raw565pack", pack), (".meta", struct.pack(f"<{len(delays)}H", *delays))]


def item_raw(item: dict) -> bytes:
    """Octets raw565 d'une entree, reconstruits depuis son PNG ; ValueError si l'empreinte ne correspond plus (entree alteree).
    Pour une animation : sa 1re image (apercu), apres verification de l'ensemble."""
    from PIL import Image

    if is_anim(item):
        return item_files(item)[0][1][:RAW_SIZE]
    with Image.open(io.BytesIO(base64.b64decode(item["png_b64"]))) as im:
        raw = rgb_to_raw565(im.convert("RGB"))
    if item.get("sha1") and hashlib.sha1(raw).hexdigest() != item["sha1"]:
        raise ValueError(f"empreinte differente pour {item.get('key')}")
    return raw


# --------------------------------------------------------------------------- store JSON
def new_store() -> dict:
    return {"version": STORE_VERSION, "panel": [W, H], "items": []}


def load_store(path) -> dict:
    """Store absent -> store vide. Store illisible -> ValueError : on ne l'ecrase JAMAIS en silence (il contient le travail de l'utilisateur)."""
    p = Path(path)
    if not p.exists():
        return new_store()
    try:
        data = json.loads(p.read_text(encoding="utf-8"))
    except (OSError, ValueError) as e:
        raise ValueError(f"{p} illisible : {e}") from None
    if not isinstance(data, dict) or not isinstance(data.get("items"), list):
        raise ValueError(f"{p} : structure inattendue")
    if data.get("version", 0) > STORE_VERSION:
        raise ValueError(f"{p} : version {data.get('version')} plus recente que ce toolkit")
    data.setdefault("version", STORE_VERSION)
    data.setdefault("panel", [W, H])
    return data


def _atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    part = path.with_name(path.name + ".part")
    with open(part, "wb") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    os.replace(part, path)


def save_store(path, store: dict) -> None:
    store["items"] = sorted(store["items"], key=lambda it: it["key"].lower())
    _atomic_write(Path(path), json.dumps(store, ensure_ascii=False, indent=1).encode("utf-8"))


def find_item(store: dict, key: str):
    for it in store["items"]:
        if it.get("key", "").lower() == key.lower():
            return it
    return None


def put_item(store: dict, item: dict) -> list:
    """Ajoute ou remplace une entree. Garde UNE version precedente (`prev`) pour annuler. Renvoie des codes d'avertissement :
    collision = une autre ROM (autre nom de fichier) donne la meme cle une fois les espaces retires."""
    warnings = []
    old = find_item(store, item["key"])
    if old is not None:
        if old.get("rom") and old["rom"] != item["rom"]:
            warnings.append("collision")
        item["prev"] = {k: old[k] for k in ("png_b64", "sha1", "date", "mode", "bg", "src", "pack_z", "delays") if k in old}
        for k in ("orig_b64", "orig_pack_z", "orig_delays"):   # le logo d'AVANT la 1re personnalisation reste celui a rendre
            if k in old and k not in item:
                item[k] = old[k]
        store["items"].remove(old)
    store["items"].append(item)
    return warnings


_PREV_FIELDS = ("png_b64", "sha1", "date", "mode", "bg", "src", "pack_z", "delays")      # champs que put_item() range dans `prev`


def has_previous(item) -> bool:
    """L'entree a une version precedente (un seul niveau) a laquelle revenir."""
    return bool(item and item.get("prev"))


def undo_previous(sd_root, store: dict, key: str, log=print) -> bool:
    """Revient a la version PRECEDENTE de `key` (un seul niveau : `prev`, rempli par put_item a chaque remplacement d'une personnalisation existante). L'entree retrouve
    l'image (ou l'animation) d'avant avec la date d'AUJOURD'HUI (la fusion SD / miroir PC garde l'entree la plus recente : une date ancienne perdrait contre la version
    annulee), l'original sauvegarde (orig_*) est conserve et `prev` est vide : un second appel ne « refait » pas, le bouton propose ensuite l'original. Ecrit sur la SD
    comme une validation (une animation remplacee par une fixe, ou l'inverse, est geree par stage_item). Renvoie False si rien a annuler. N'enregistre pas le store."""
    cur = find_item(store, key)
    if not has_previous(cur):
        return False
    new = {k: v for k, v in cur.items() if k not in _PREV_FIELDS and k != "prev"}
    new.update(cur["prev"])
    new["date"] = datetime.now().isoformat(timespec="seconds")
    commit_item(sd_root, store, new, log=log)
    done = find_item(store, key)
    if done is not None:
        done.pop("prev", None)          # put_item y avait range la version annulee : un seul niveau, pas de « refaire »
    return True


def remove_item(store: dict, key: str) -> bool:
    it = find_item(store, key)
    if it is None:
        return False
    store["items"].remove(it)
    return True


def merge_stores(a: dict, b: dict) -> dict:
    """Fusion par cle, l'entree la plus RECENTE gagne (egalite : a). Une suppression faite d'un seul cote reapparait : l'interface ecrit toujours les deux."""
    out = new_store()
    best = {}
    for it in a["items"] + b["items"]:
        k = it["key"].lower()
        if k not in best or it.get("date", "") > best[k].get("date", ""):
            best[k] = it
    out["items"] = list(best.values())
    return out


def sync_stores(sd_root) -> dict:
    """Fusionne le store de la SD et le miroir PC, ecrit le resultat des deux cotes (si la SD n'est pas disponible, seul le miroir l'est)."""
    sd_p, mir_p = store_path(sd_root), mirror_path()
    merged = merge_stores(load_store(sd_p), load_store(mir_p))
    save_store(mir_p, merged)
    if Path(sd_root).is_dir():
        save_store(sd_p, merged)
    return merged


# --------------------------------------------------------------------------- application sur la SD
def _cache_lacks(sd: Path, pairs) -> bool:   # pairs : (systeme, stem) ou (systeme, stem, type attendu)
    """True si games_cache.bin existe et ne liste pas l'un des (systeme, stem) : le firmware ecarte alors ce jeu sans lire la SD. Pas de cache = rien a
    contourner (False). Cache illisible = True (on reconstruit). Format : voir _write_cache_binary() du toolkit (entete u32 + 36 o/systeme, table de
    NB_IDX u32 par systeme, listes [type][nom minuscule]\\0 delimitees par l'offset non nul suivant)."""
    import struct

    try:
        data = (sd / "games_cache.bin").read_bytes()
    except OSError:
        return False
    tool = _tool()
    try:
        total = struct.unpack_from("<I", data, 0)[0]
        blocks = {}
        starts = []
        for i in range(total):
            base = 4 + i * 36
            name = data[base:base + 32].split(b"\0", 1)[0].decode("utf-8", "replace")
            off = struct.unpack_from("<I", data, base + 32)[0]
            blocks[name] = off
            starts.append(off)
        starts.sort()
        for pair in pairs:
            system, stem = pair[0], pair[1]
            want_type = pair[2] if len(pair) > 2 else None
            off = blocks.get(system)
            if off is None:
                return True
            table = struct.unpack_from(f"<{tool.NB_IDX}I", data, off)
            start = table[tool._calc_bigram_idx(stem)]
            if not start:
                return True
            nxt = [o for o in table if o > start] + [o for o in starts if o > off] + [len(data)]
            end = min(o for o in nxt if o > start)
            pos, want = start, stem.lower().encode("utf-8")
            found = False
            while pos < end:
                z = data.index(b"\0", pos + 1)
                if data[pos + 1:z] == want and (want_type is None or data[pos] == want_type):   # un jeu qui a .raw565 ET .raw565pack figure 2 fois (un type chacun)
                    found = True
                    break
                pos = z + 1
            if not found:
                return True
        return False
    except (struct.error, ValueError, IndexError):
        return True


def apply_on_drive(drive_root, log=print):
    """Rejoue les personnalisations sur une SD (appele apres chaque copie du dossier de travail vers la SD, qui ecrase des logos).
    Fusionne le store de la SD et le miroir PC (la plus recente gagne) puis ecrit les .raw565. Ne cree rien si l'utilisateur n'a aucune personnalisation ;
    ne leve jamais : un souci ici ne doit pas faire echouer la copie. Renvoie le rapport d'apply_store(), ou None."""
    try:
        sd = Path(drive_root)
        sd_store, mirror = load_store(store_path(sd)), load_store(mirror_path())
        if not sd_store["items"] and not mirror["items"]:
            return None
        merged = sync_stores(sd)
        log(f"[Logos perso] {len(merged['items'])} personnalisation(s) a reappliquer sur {sd}")
        rep = apply_store(sd, merged, log=log)
        log(f"[Logos perso] reecrits {len(rep['written'])} - deja bons {len(rep['unchanged'])} - orphelins {len(rep['orphans'])} - "
            f"erreurs {len(rep['errors'])} - cache {'reconstruit' if rep['cache'] else 'inchange'}")
        for key, ext in rep["shadows"]:
            log(f"   ⚠ {key} : un fichier {ext} voisin peut masquer le logo personnalise")
        return rep
    except Exception as e:   # noqa: BLE001
        log(f"[Logos perso] reapplication impossible : {e}")
        return None


SYSC_NAME = "systems_cache.dat"


def _sysc_update(sd: Path, system: str, anim: bool) -> bool:
    """Type p/g/B du systeme dans systems_cache.dat (ligne « <type> <systeme> <L|N> <27 drapeaux de bucket> », voir build_systems_cache() du toolkit).
    Le firmware en tire l'ordre d'essai des images de jeux : un systeme 'p' (que des images fixes) ne lit JAMAIS un .raw565pack, un 'g' ne lit jamais un
    .raw565 fixe. Une animation dans un systeme 'p' le fait donc passer a 'B' (les deux), une image fixe dans un 'g' aussi ; un systeme absent du fichier y est
    ajoute. Mise a jour de la seule ligne (pas de rescan de la SD). Pas de fichier = rien (le firmware deduit les types des dossiers au demarrage).
    Un type 'B' devenu inutile apres un retrait n'est pas ramene a 'p'/'g' : plus lent a l'essai, jamais faux. True si le fichier a change."""
    p = sd / SYSC_NAME
    try:
        lines = [ln for ln in p.read_text(encoding="utf-8").split("\n") if ln.strip()]
    except OSError:
        return False
    changed, found = False, False
    for i, ln in enumerate(lines):
        parts = ln.split(" ", 2)
        if len(parts) >= 3 and parts[1].lower() == system.lower():
            found = True
            new = "B" if parts[0] == ("p" if anim else "g") else parts[0]
            if new != parts[0]:
                lines[i] = " ".join([new] + parts[1:])
                changed = True
            break
    if not found:
        lines.append(f"{'g' if anim else 'p'} {system} N {'N' * 27}")
        lines.sort(key=lambda ln: ln.split(" ", 2)[1].lower())
        changed = True
    if changed:
        _atomic_write(p, ("\n".join(lines) + "\n").encode("utf-8"))
    return changed


def apply_store(sd_root, store: dict, keys=None, rebuild_cache: bool = True, log=print) -> dict:
    """Reecrit sur la SD les .raw565 du store qui manquent ou different. keys : limiter a ces cles.
    Rapport : written / unchanged / orphans (systeme absent de la SD) / errors (entree invalide ou alteree) / shadows (fichiers freres que le firmware
    pourrait afficher a la place : .png, .gif, .raw565pack) / cache (games_cache.bin reconstruit).
    games_cache.bin n'est reconstruit que si un jeu SANS visuel en recoit un : le firmware ecarte sans lire la SD un jeu absent du cache."""
    sd = Path(sd_root)
    rep = {"written": [], "unchanged": [], "orphans": [], "errors": [], "shadows": [], "cache": False, "sysc": []}
    wanted = {k.lower() for k in keys} if keys is not None else None
    new_files = 0
    present = []
    kinds = set()
    for it in store["items"]:
        key = it.get("key", "")
        if wanted is not None and key.lower() not in wanted:
            continue
        try:
            system, _stem = split_key(key)
            dst = target_path(sd, key)
            files = item_files(it)
        except (ValueError, KeyError, OSError) as e:
            rep["errors"].append((key, str(e)))
            log(f"   ✖ {key} : {e}")
            continue
        if not (sd / "systems" / system).is_dir():
            rep["orphans"].append(key)
            log(f"   ? {key} : systeme absent de la SD")
            continue
        anim = is_anim(it)
        existed = all(dst.with_suffix(ext).exists() for ext, _b in files)
        if existed and all(dst.with_suffix(ext).read_bytes() == data for ext, data in files):
            rep["unchanged"].append(key)
        else:
            for ext, data in files:
                _atomic_write(dst.with_suffix(ext), data)
            rep["written"].append(key)
            if not existed:
                new_files += 1
            log(f"   ✔ {key}" + (f" (animation, {len(files[1][1]) // 2} images)" if anim else ""))
        present.append((system, dst.stem, TYPE_ANIM) if anim else (system, dst.stem))
        kinds.add((system, anim))
        for ext in ((".png", ".gif") if anim else (".png", ".gif", ".raw565pack")):
            if dst.with_suffix(ext).exists():
                rep["shadows"].append((key, ext))
    for system, anim in sorted(kinds):      # type du systeme (p/g/B) : sinon le firmware n'essaie jamais ce type d'image dans ce systeme
        if rebuild_cache and _sysc_update(sd, system, anim):
            rep["sysc"].append(system)
            log(f"   ✔ systems_cache.dat : type de {system} mis a jour ({'animation' if anim else 'image fixe'} ajoutee)")
    # le .raw565 peut deja exister (ecrit par un passage precedent) alors que le games_cache.bin, lui, vient d'etre recopie depuis le dossier de travail
    # et ignore ce jeu : on verifie donc le cache lui-meme, pas seulement les fichiers ecrits
    if rebuild_cache and present and (new_files or _cache_lacks(sd, present)):
        try:
            _tool().build_cache(sd / "systems", sd)
            rep["cache"] = True
        except Exception as e:   # noqa: BLE001
            rep["errors"].append(("games_cache.bin", str(e)))
            log(f"   ✖ games_cache.bin : {e}")
    return rep


# --------------------------------------------------------------------------- navigation (interface)
def raw565_to_image(raw: bytes):
    """8192 octets RGB565 petit-boutiste -> image RGB 128x32 (pour une animation .raw565pack : sa 1re image)."""
    import numpy as np
    from PIL import Image

    if len(raw) < RAW_SIZE:
        raise ValueError(f"{len(raw)} octets, {RAW_SIZE} attendus")
    a = np.frombuffer(raw[:RAW_SIZE], dtype="<u2").reshape(H, W).astype(np.uint16)
    r, g, b = (a >> 11) & 0x1F, (a >> 5) & 0x3F, a & 0x1F
    rgb = np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)], axis=-1).astype(np.uint8)
    return Image.fromarray(rgb, "RGB")


def led_preview(img_rgb, scale: int = 3, grid: bool = True):
    """Agrandit une image 128x32 en NEAREST ; avec grid, assombrit la 1re ligne/colonne de chaque bloc (effet de LED separees)."""
    import numpy as np
    from PIL import Image

    big = np.asarray(img_rgb.convert("RGB").resize((W * scale, H * scale), Image.NEAREST)).copy()
    if grid and scale >= 3:
        big[::scale] //= 3
        big[:, ::scale] //= 3
    return Image.fromarray(big, "RGB")


def led_style(img_rgb, scale: int = 3, brightness: float = 0.5, led_ratio: float = 0.525, glow: bool = True, supersample: int = 4):
    """Rendu d'un panneau de LED : chaque pixel devient un point rond separe de ses voisins, avec halo. Port a l'identique de
    DMDEngine.render_led_style() de DMD GIF Creator (dmd_engine.py v18, meme auteur) pour que l'apercu soit celui que l'utilisateur connait deja.
    led_ratio 0.525 = LED SMD 2121 sur un HUB75 P4 ; brightness 0..1 (0.5 = neutre) fait grossir les points et leur halo comme sur un vrai panneau."""
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter

    brightness = max(0.0, min(1.0, brightness))
    color_mult = 0.7 + 0.6 * brightness
    if brightness >= 0.5:
        bloom_delta = (brightness - 0.5) * 1.15
        glow_radius_factor = 0.15 + (brightness - 0.5) * 0.8
        glow_blend = min(0.85, 0.25 + (brightness - 0.5) * 1.0)
    else:
        bloom_delta = (brightness - 0.5) * 0.5
        glow_radius_factor = max(0.02, 0.15 + (brightness - 0.5) * 0.4)
        glow_blend = max(0.02, 0.25 + (brightness - 0.5) * 0.6)
    effective_led_ratio = max(0.15, min(1.3, led_ratio + bloom_delta))

    src = np.clip(np.array(img_rgb.convert("RGB")).astype(np.float32) * color_mult, 0, 255).astype(np.uint8)
    h, w, _ = src.shape
    big = scale * supersample
    canvas = Image.new("RGB", (w * big, h * big), (0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    dot_r = (big * effective_led_ratio) / 2
    ys, xs = np.nonzero(src.max(axis=2) >= 8)          # LED eteintes (quasi noires) : non dessinees, fond bien noir entre les points
    for y, x in zip(ys.tolist(), xs.tolist()):
        r, g, b = src[y, x]
        cx, cy = x * big + big / 2, y * big + big / 2
        draw.ellipse([cx - dot_r, cy - dot_r, cx + dot_r, cy + dot_r], fill=(int(r), int(g), int(b)))
    canvas = canvas.resize((w * scale, h * scale), Image.Resampling.LANCZOS)
    if glow:
        canvas = Image.blend(canvas, canvas.filter(ImageFilter.GaussianBlur(radius=scale * glow_radius_factor)), glow_blend)
    return canvas


def png_b64_to_image(b64: str):
    from PIL import Image

    with Image.open(io.BytesIO(base64.b64decode(b64))) as im:
        return im.convert("RGB")


def image_to_png_b64(img_rgb) -> str:
    buf = io.BytesIO()
    img_rgb.save(buf, "PNG", optimize=True)
    return base64.b64encode(buf.getvalue()).decode("ascii")


def list_systems_sd(sd_root) -> list:
    """Systemes presents sur la SD (dossiers de systems/ hors _defaults), tries sans casse."""
    base = Path(sd_root) / "systems"
    if not base.is_dir():
        return []
    return sorted((p.name for p in base.iterdir() if p.is_dir() and _SYSTEM_RE.match(p.name) and p.name.lower() != "_defaults"), key=str.lower)


def roms_systems(roms) -> dict:
    """{systeme: gamelist.xml} d'un dossier ROMs : ses sous-dossiers qui ont un gamelist.xml (cas normal, ex. //NAS/recalbox/ROMS) ou, si le dossier est lui-meme
    un systeme (ex. .../ROMS/3do avec son gamelist.xml), ce seul systeme. Noms hors [A-Za-z0-9_-] et _defaults ignores. Illisible = vide."""
    out, base = {}, Path(roms)
    try:
        if _SYSTEM_RE.match(base.name) and (base / "gamelist.xml").is_file():
            out[base.name] = base / "gamelist.xml"
        if base.is_dir():
            for d in base.iterdir():
                if _SYSTEM_RE.match(d.name) and d.name.lower() != "_defaults" and (d / "gamelist.xml").is_file():
                    out[d.name] = d / "gamelist.xml"
    except OSError:
        pass
    return out


def existing_logos(sd_root, system: str, prefer_anim=frozenset()) -> dict:
    """{stem en minuscules: (chemin, 'raw'|'anim')} des visuels d'un systeme sur la SD ; par defaut .raw565 l'emporte sur .raw565pack. `prefer_anim` (stems en minuscules) : l'animation
    l'emporte pour ces jeux -- ceux dont la personnalisation EST une animation : dans un systeme de type g / B le firmware ESP32 tente d'abord le .raw565pack (RecalBox_DMD.ino, `sysT == 'g' ||
    sysT == 'B'` -> `cached = 'g'`), et le Pi n'a plus de fixe (push_store le retire) ; sans cela « Existant » montrait l'ancien logo fixe voisin alors que le DMD joue l'animation."""
    out = {}
    base = Path(sd_root) / "systems" / system
    if not base.is_dir():
        return out
    for p in base.rglob("*"):
        ext = p.suffix.lower()
        if ext not in (".raw565", ".raw565pack") or not p.is_file():
            continue
        kind = "raw" if ext == ".raw565" else "anim"
        k = p.stem.lower()
        if k in prefer_anim:
            if kind == "anim" or k not in out:
                out[k] = (p, kind)          # animation PERSONNALISEE : c'est elle que le DMD joue (voir ci-dessous), pas le .raw565 voisin
        elif k not in out or kind == "raw":
            out[k] = (p, kind)
    return out


def games_from_gamelist(xml_path) -> list:
    """Jeux d'un gamelist.xml : [{rom, name, hidden}], sans doublon de cle."""
    from urllib.parse import unquote

    out, seen = [], set()
    for g in _tool().parse_gamelist(Path(xml_path)):
        p = g.find("path")
        if p is None or not (p.text or "").strip():
            continue
        rom = rom_basename(unquote(p.text.strip()))
        stem = stem_for_rom(rom)
        if not stem or stem.lower() in seen:
            continue
        seen.add(stem.lower())
        n, h = g.find("name"), g.find("hidden")
        name = (n.text or "").strip() if n is not None else ""
        out.append({"rom": rom, "stem": stem, "name": name or re.sub(r"\.[^.]*$", "", rom),
                    "hidden": h is not None and (h.text or "").strip().lower() == "true"})
    return out


def games_from_sd(sd_root, system: str) -> list:
    """Repli sans gamelist.xml : les jeux qui ont un visuel sur la SD (nom = stem)."""
    return [{"rom": p.stem, "stem": p.stem, "name": p.stem, "hidden": False} for p, _k in sorted(existing_logos(sd_root, system).values(), key=lambda t: t[0].stem.lower())]


def _non_game_fn():
    """Predicat « non-jeu » (ZZZ…, caches) de text_card.py ; repli : seulement les jeux caches."""
    try:
        import text_card
        return text_card.is_non_game
    except Exception:   # noqa: BLE001
        return lambda name, stem="", hidden=False: hidden


# --------------------------------------------------------------------------- instantane du Mode 1
# La liste des jeux telle qu'elle etait au dernier scrape reel (Mode 1), rangee SUR la SD avec les images qu'elle decrit : le gamelist.xml du dossier ROMs
# peut avoir change depuis (jeux ajoutes, renommes), l'instantane reste coherent avec ce que la SD contient. Un seul fichier pour tous les systemes.
SNAP_REL = Path("custom") / "games.json"
SNAP_STAGE_DIR = "_snapshot"       # dans le dossier de travail : exclu de la copie vers la SD (voir _SD_COPY_EXCLUDED_DIRS), fusionne par merge_snapshot_to_drive()
SNAP_VERSION = 1


def snapshot_path(sd_root) -> Path:
    return Path(sd_root) / SNAP_REL


def new_snapshot() -> dict:
    return {"version": SNAP_VERSION, "systems": {}}


def load_snapshot(path) -> dict:
    """{"version", "systems": {systeme: {date, images, games: [{r: rom, n: nom, h: 1 si cache}]}}}. Absent ou illisible = vide : l'instantane se regenere au
    prochain Mode 1, il ne doit jamais bloquer l'onglet."""
    out = new_snapshot()
    try:
        data = json.loads(Path(path).read_text(encoding="utf-8"))
        for system, ent in (data.get("systems") or {}).items():
            if not (isinstance(system, str) and _SYSTEM_RE.match(system) and system.lower() != "_defaults" and isinstance(ent, dict) and isinstance(ent.get("games"), list)):
                continue
            out["systems"][system] = {"date": str(ent.get("date", "")), "images": int(ent.get("images", 0)),
                                      "games": [g for g in ent["games"] if isinstance(g, dict) and isinstance(g.get("r"), str)]}
            if ent.get("add_cards"):
                out["systems"][system]["add_cards"] = True      # apport du Mode 18 : voir merge_snapshots
    except (OSError, ValueError, AttributeError, TypeError):
        return new_snapshot()
    return out


def save_snapshot(path, snap: dict) -> None:
    _atomic_write(Path(path), json.dumps(snap, ensure_ascii=False, separators=(",", ":")).encode("utf-8"))


def merge_snapshots(a: dict, b: dict) -> dict:
    """Fusion par systeme, l'entree la plus RECENTE gagne (egalite : b, l'apport qu'on fusionne dans a). Un Mode 1 sur quelques systemes ne fait pas
    disparaitre les autres."""
    out = new_snapshot()
    out["systems"] = dict(a["systems"])
    for system, ent in b["systems"].items():
        if ent.get("add_cards") and system in out["systems"]:
            # apport du Mode 18 (cartes texte regenerees) : il AJOUTE ses marques « c » a l'entree existante au lieu de la remplacer (la liste des jeux et la date
            # du scrape ne changent pas, les cartes deja marquees par un Mode 1 restent marquees)
            old = dict(out["systems"][system])
            carded = {g["r"] for g in ent["games"] if g.get("c")}
            old["games"] = [dict(g, c=1) if g["r"] in carded else g for g in old["games"]]
            out["systems"][system] = old
        elif system not in out["systems"] or ent["date"] >= out["systems"][system]["date"]:
            out["systems"][system] = {k: v for k, v in ent.items() if k != "add_cards"}
    return out


def _images_info(system_dir):
    """(nombre de .raw565/.raw565pack, date de modification la plus recente) d'un dossier de systeme."""
    n, newest = 0, 0.0
    base = Path(system_dir)
    if base.is_dir():
        for p in base.rglob("*"):
            if p.suffix.lower() in (".raw565", ".raw565pack") and p.is_file():
                n += 1
                try:
                    newest = max(newest, p.stat().st_mtime)
                except OSError:
                    pass
    return n, newest


def _count_images(system_dir) -> int:
    return _images_info(system_dir)[0]


def _snap_games(games: list, cards=None) -> list:
    """Jeux d'un instantane : {r: rom, n: nom, h: 1 si cache, c: 1 si le visuel ecrit au scrape est une CARTE TEXTE (nom de fichier dans `cards`, en minuscules)}."""
    cards = cards or ()
    out = []
    for g in games:
        stem = (g.get("stem") or stem_for_rom(g["rom"])).lower()
        d = {"r": g["rom"], "n": g["name"]}
        if g["hidden"]:
            d["h"] = 1
        if stem in cards or _safe_stem(stem) in cards:
            d["c"] = 1
        out.append(d)
    return out


def _safe_stem(stem: str) -> str:
    try:
        return _tool().sanitize_filename(stem).lower()
    except Exception:   # noqa: BLE001
        return stem.lower()


def snapshot_missing_systems(sd_root, sd_systems, roms_map: dict, snap: dict) -> list:
    """Systemes de la SD (avec des images) qui ont un gamelist.xml dans le dossier ROMs mais aucune entree dans l'instantane : SD scrapee avant l'instantane."""
    return sorted(s for s in sd_systems if s in roms_map and s not in snap["systems"] and _count_images(Path(sd_root) / "systems" / s) > 0)


def convert_snapshot(sd_root, roms_map: dict, systems: list, log=print) -> list:
    """Cree l'instantane d'une ancienne SD (scrapee avant l'instantane) pour `systems` : liste des jeux = gamelist.xml du dossier ROMs LU MAINTENANT (le scrape
    d'origine n'est plus consultable), date = fichier le plus recent du systeme sur la SD (proche du scrape reel). N'ecrase aucune entree existante et ne
    touche a aucune image. Renvoie les systemes ajoutes."""
    snap = load_snapshot(snapshot_path(sd_root))
    added = []
    for system in systems:
        gl = roms_map.get(system)
        if gl is None or system in snap["systems"] or not _SYSTEM_RE.match(system) or system.lower() == "_defaults":
            continue
        n, newest = _images_info(Path(sd_root) / "systems" / system)
        if not n:
            continue
        try:
            games = games_from_gamelist(gl)
        except Exception as e:   # noqa: BLE001
            log(f"[Instantane] {system} : gamelist.xml illisible ({e}), non cree")
            continue
        snap["systems"][system] = {"date": datetime.fromtimestamp(newest).isoformat(timespec="seconds"), "images": n, "games": _snap_games(games)}
        added.append(system)
    if added:
        save_snapshot(snapshot_path(sd_root), snap)
    return added


def build_snapshot_from_roms(roms_map: dict, systems=None, log=print) -> dict:
    """Instantane recree SANS Mode 1 : liste des jeux = gamelist.xml du dossier ROMs lus MAINTENANT (le scrape d'origine n'est plus consultable), date = maintenant.
    roms_map : {systeme: gamelist.xml} (roms_systems) ; systems : restriction facultative (None = tous). Aucune image n'est lue ni touchee (images = 0). Systemes sans
    jeu ou au gamelist illisible ignores. Renvoie un instantane (a fusionner avec merge_snapshots ou a envoyer tel quel)."""
    snap = new_snapshot()
    now = datetime.now().isoformat(timespec="seconds")
    for system in sorted(roms_map, key=str.lower):
        if systems is not None and system not in systems:
            continue
        if not _SYSTEM_RE.match(system) or system.lower() == "_defaults":
            continue
        try:
            games = games_from_gamelist(roms_map[system])
        except Exception as e:   # noqa: BLE001
            log(f"[Instantane] {system} : gamelist.xml illisible ({e}), non cree")
            continue
        if games:
            snap["systems"][system] = {"date": now, "images": 0, "games": _snap_games(games)}
    return snap


def snapshot_bytes(snap: dict) -> bytes:
    return json.dumps(snap, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def write_snapshot(work_dir, roms_root, systems=None, log=print, cards=None, add_cards=False) -> int:
    """Fin du Mode 1 : note, dans le dossier de travail, la liste des jeux de chaque systeme traite (gamelist.xml lu a l'instant du scrape) et le nombre
    d'images produites. systems : dossiers de ROMs traites (None = tous ceux qui ont un gamelist.xml). Renvoie le nombre de systemes notes."""
    work, roms = Path(work_dir), Path(roms_root)
    dirs = list(systems) if systems is not None else [d for d in roms.iterdir() if d.is_dir() and (d / "gamelist.xml").is_file()]
    stage = work / SNAP_STAGE_DIR / "games.json"
    snap = load_snapshot(stage)
    now = datetime.now().isoformat(timespec="seconds")
    done = 0
    made = getattr(_tool(), "CARDS_MADE", {}) if cards is None else cards      # cartes texte ecrites par ce Mode 1 (voir CARDS_MADE de l'outil)
    for d in dirs:
        d = Path(d)
        if not _SYSTEM_RE.match(d.name) or d.name.lower() == "_defaults":
            continue
        if add_cards and not made.get(d.name):
            continue            # Mode 18 : seuls les systemes dont des cartes ont ete regenerees comptent
        try:
            games = games_from_gamelist(d / "gamelist.xml")
        except Exception as e:   # noqa: BLE001
            log(f"[Instantane] {d.name} : gamelist.xml illisible ({e}), non note")
            continue
        snap["systems"][d.name] = {"date": now, "images": _count_images(work / "systems" / d.name),
                                   "games": _snap_games(games, made.get(d.name))}
        if add_cards:
            snap["systems"][d.name]["add_cards"] = True
        made.pop(d.name, None)       # consomme : un Mode 1 suivant repart de zero
        done += 1
    if done:
        save_snapshot(stage, snap)
    return done


def merge_snapshot_to_drive(work_dir, drive_root, log=print) -> bool:
    """Apres une copie du dossier de travail vers la SD : fusionne l'instantane prepare par write_snapshot() dans custom/games.json de la SD (par systeme,
    le plus recent gagne). Ne retient que les systemes dont le dossier de travail contient des images : le Mode 1 vide systems/ a chaque lancement, un
    instantane plus ancien d'un systeme qui n'a pas ete recopie decrirait des images absentes de la SD. Ne leve jamais."""
    try:
        work = Path(work_dir)
        staged = load_snapshot(work / SNAP_STAGE_DIR / "games.json")
        staged["systems"] = {s: e for s, e in staged["systems"].items() if _count_images(work / "systems" / s) > 0}
        if not staged["systems"]:
            return False
        merged = merge_snapshots(load_snapshot(snapshot_path(drive_root)), staged)
        save_snapshot(snapshot_path(drive_root), merged)
        log(f"[Instantane] liste des jeux notee sur la SD : {len(merged['systems'])} systeme(s)")
        return True
    except Exception as e:   # noqa: BLE001
        log(f"[Instantane] ecriture impossible : {e}")
        return False


def games_from_snapshot(snap: dict, system: str):
    """Jeux d'un systeme d'apres l'instantane ([{rom, stem, name, hidden}] comme games_from_gamelist), ou None si le systeme n'y figure pas."""
    ent = snap["systems"].get(system)
    if ent is None:
        return None
    out, seen = [], set()
    for g in ent["games"]:
        rom = g["r"]
        stem = stem_for_rom(rom)
        if not stem or stem.lower() in seen:
            continue
        seen.add(stem.lower())
        out.append({"rom": rom, "stem": stem, "name": str(g.get("n") or re.sub(r"\.[^.]*$", "", rom)), "hidden": bool(g.get("h")), "card": bool(g.get("c"))})
    return out


def rb_logos(xml_path, tag: str = "logo") -> dict:
    """Logos que la RECALBOX possede AUJOURD'HUI : {stem en minuscules: Path du fichier}, d'apres la balise `tag` du profil Recalbox (« logo », « thumbnail » ou
    « image », comme le Mode 1) du gamelist.xml, fichier existant seulement. Sert a signaler un jeu dont la SD affiche une carte texte (ou rien) alors que la
    Recalbox a maintenant son logo : le remplacant est alors tout trouve. La liste des jeux, elle, reste celle de l'EXISTANT (instantane de la SD) : un jeu
    absent de la SD veut dire un nouveau systeme, donc un nouveau Mode 1 / 3, pas un remplacement ici."""
    from urllib.parse import unquote

    base = Path(xml_path).parent
    tool = _tool()
    cand = {}                                  # stem en minuscules -> fichier candidat (le 1er jeu de ce nom gagne, comme avant)
    for g in tool.parse_gamelist(Path(xml_path)):
        p, img = g.find("path"), g.find(tag)
        if p is None or img is None or not (p.text or "").strip() or not (img.text or "").strip():
            continue
        stem = stem_for_rom(rom_basename(unquote(p.text.strip())))
        if not stem or stem.lower() in cand:
            continue
        try:
            cand[stem.lower()] = Path(tool.resolve_image_path(base, unquote(img.text.strip())))
        except Exception:   # noqa: BLE001
            continue

    # 2026-11-17 : un test d'existence PAR jeu sur le NAS (~1,2 ms chacun) = 83 s pour « Tous les systemes » (70 936 jeux). On LISTE chaque dossier d'images UNE fois
    # (os.listdir : un aller-retour reseau par dossier au lieu d'un par jeu) et on teste l'appartenance ; repli sur is_file() pour un dossier illisible.
    listings = {}

    def names_in(folder):
        if folder not in listings:
            try:
                listings[folder] = {n.lower() for n in os.listdir(folder)}
            except OSError:
                listings[folder] = None
        return listings[folder]

    def exists(src):
        names = names_in(src.parent)
        if names is None:
            try:
                return src.is_file()
            except OSError:
                return False
        return src.name.lower() in names           # le systeme de fichiers de Windows ne distingue pas les majuscules

    return {k: v for k, v in cand.items() if exists(v)}


def snapshot_extras(sd_root, system: str, games: list, store: dict) -> list:
    """Stems presents sur la SD mais absents de la liste (hors personnalisations) : la SD a change depuis le Mode 1."""
    listed = {(g.get("stem") or stem_for_rom(g["rom"])).lower() for g in games}
    custom = {it.get("key", "").lower() for it in store["items"]}
    return sorted(k for k in existing_logos(sd_root, system) if k not in listed and f"{system}/{k}".lower() not in custom)


def compare_games(old: list, new: list):
    """(nouveaux, disparus) : stems des vrais jeux (hors non-jeux et caches) presents seulement dans `new` / seulement dans `old`."""
    ng = _non_game_fn()

    def vis(games):
        out = set()
        for g in games:
            stem = g.get("stem") or stem_for_rom(g["rom"])
            if not ng(g["name"], stem, g.get("hidden", False)):
                out.add(stem.lower())
        return out
    a, b = vis(old), vis(new)
    return sorted(b - a), sorted(a - b)


def system_rows(sd_root, system: str, games: list, store: dict, rb_logos: dict = None) -> list:
    """Lignes de la liste : etat custom (dans le store) / missing (aucun visuel) / anim (seulement une animation) / ok. « dark » est ajoute apres coup
    par scan_dark() (lire 8 Ko par jeu est trop lent pour la 1re apparition de la liste). Non-jeux (ZZZ…) et jeux caches exclus."""
    non_game = _non_game_fn()
    anim_stems = {it["key"].split("/", 1)[1].lower() for it in store["items"] if it.get("key", "").lower().startswith(system.lower() + "/") and "/" in it["key"] and is_anim(it)}
    ex = existing_logos(sd_root, system, prefer_anim=anim_stems) if sd_root else {}
    custom = {it.get("key", "").lower() for it in store["items"]}
    rows = []
    for g in games:
        stem = g.get("stem") or stem_for_rom(g["rom"])
        if non_game(g["name"], stem, g.get("hidden", False)):
            continue
        key = f"{system}/{stem}"
        hit = ex.get(stem.lower())
        if key.lower() in custom:
            state = "custom"
        elif hit is None:
            state = "missing"                                  # connu du scrape mais aucun visuel sur la SD
        elif hit[1] == "anim":
            state = "anim"
        else:
            state = "card" if g.get("card") else "ok"         # card = carte texte ecrite par le Mode 1 (drapeau « c » de l'instantane)
        rb = rb_logos.get(stem.lower()) if (rb_logos and state != "custom") else None
        if rb is not None and state in ("card", "missing"):
            state = "avail"                                    # la Recalbox a maintenant un logo pour ce jeu : remplacant tout trouve (un logo NOIR le devient aussi, apres coup : voir scan_dark et la GUI)
        rows.append({"key": key, "system": system, "rom": g["rom"], "stem": stem, "name": g["name"], "state": state, "path": hit[0] if hit else None,
                     "rb_logo": str(rb) if rb is not None else None, "was_card": bool(g.get("card")) and hit is not None})
    rows.sort(key=lambda r: r["name"].lower())
    return rows


def read_logo_image(path):
    """Image RGB 128x32 d'un .raw565 / .raw565pack de la SD, ou None si illisible."""
    try:
        with open(path, "rb") as f:
            return raw565_to_image(f.read(RAW_SIZE))
    except (OSError, ValueError):
        return None


def preview_logo(path, mode: str = "pixel", bg=(0, 0, 0)):
    """Image 128x32 RGB que donnerait le fichier `path` une fois convertie comme un logo (1re image d'un GIF), ou None s'il est illisible. Pour les apercus
    (revue des logos que la Recalbox possede) : rien n'est ecrit."""
    try:
        with _tool()._open_image_tolerant(Path(path)) as im:
            return render(im.convert("RGBA"), mode, tuple(bg))
    except Exception:   # noqa: BLE001
        return None


def preview_frame(path, index: int, mode: str = "pixel", bg=(0, 0, 0)):
    """Image 128x32 RGB de l'image `index` (0 = la 1re) d'un GIF / WebP anime, convertie comme un logo ; None si illisible ou hors limites. Pour l'apercu du choix d'UNE image."""
    try:
        return render(gif_frame(path, index), mode, tuple(bg))
    except Exception:   # noqa: BLE001
        return None


def pack_frame_count(path) -> int:
    """Nombre d'images d'un .raw565 / .raw565pack (taille / 8192) ; 0 si la taille est incoherente ou le fichier illisible."""
    try:
        size = Path(path).stat().st_size
    except OSError:
        return 0
    return size // RAW_SIZE if size and not size % RAW_SIZE else 0


def pack_frame_image(path, index: int):
    """Image RGB 128x32 de l'image `index` d'un .raw565 / .raw565pack, ou None."""
    try:
        with open(path, "rb") as f:
            f.seek(index * RAW_SIZE)
            data = f.read(RAW_SIZE)
        return raw565_to_image(data) if len(data) == RAW_SIZE else None
    except (OSError, ValueError):
        return None


def make_raw_frame_item(system: str, rom: str, path, index: int, name: str = "", stem: str = None):
    """UNE image (`index`, 0 = la 1re) d'un .raw565pack deja au format de la dalle -> image FIXE (entree, rapport, image), octets repris tels quels. Le nom de la source garde
    le numero (« x.raw565pack#12 »). ValueError si l'index est hors du paquet."""
    import tempfile

    p = Path(path)
    n = pack_frame_count(p)
    if not 0 <= index < n:
        raise ValueError(f"image {index + 1} hors de 1..{n}")
    with open(p, "rb") as f:
        f.seek(index * RAW_SIZE)
        data = f.read(RAW_SIZE)
    fd, tmp = tempfile.mkstemp(suffix=".raw565")
    os.close(fd)
    try:
        Path(tmp).write_bytes(data)
        item, rep, out = make_raw_item(system, rom, tmp, name=name, stem=stem)
    finally:
        try:
            os.unlink(tmp)
        except OSError:
            pass
    item["src"] = f"{p.name}#{index + 1}"
    return item, rep, out


def scan_dark(rows: list, extra_keys=()):
    """Generateur (ligne, est_noir) pour les lignes « ok »/« anim » qui ont un fichier (+ les lignes de `extra_keys`, ex. fausses cartes redevenues « ok ») : a appeler
    hors thread d'interface."""
    for r in rows:
        if (r["state"] in ("ok", "anim") or r["key"] in extra_keys) and r["path"]:
            img = read_logo_image(r["path"])
            if img is not None:
                yield r, is_dark(img)


def scan_cards(rows: list):
    """Generateur (ligne, est_vraie_carte) pour les lignes que l'instantane donne pour une carte texte (`was_card`, etat « card » ou « avail ») et qui ont un fichier.
    Une ligne dont le fichier n'est PAS la carte du jeu porte un drapeau perime : l'appelant la remet a « ok » (voir apply_card_check). A appeler hors thread d'interface."""
    for r in rows:
        if r.get("was_card") and r["state"] in ("card", "avail") and r["path"]:
            img = read_logo_image(r["path"])
            if img is not None:
                yield r, looks_like_card(img, r["name"])


def apply_card_check(rows: list, not_card_keys) -> int:
    """Remet a « ok » (etat reel : un vrai logo est la) les lignes de `not_card_keys` ; renvoie leur nombre. Thread d'interface."""
    n = 0
    for r in rows:
        if r["key"] in not_card_keys and r["state"] in ("card", "avail"):
            r["state"], r["was_card"] = "ok", False
            n += 1
    return n


def capture_original(sd_root, system: str, stem: str):
    """PNG base64 du visuel actuellement sur la SD (a garder avant la 1re personnalisation pour pouvoir le rendre), ou None."""
    hit = existing_logos(sd_root, system).get(stem.lower())
    img = read_logo_image(hit[0]) if hit else None
    return image_to_png_b64(img) if img is not None else None


def drop_anim_files(sd_root, key: str) -> bool:
    """Supprime .raw565pack + .meta de `key` (apres une animation qu'on remplace ou retire). True si quelque chose a ete supprime."""
    dst = target_path(sd_root, key)
    gone = False
    for ext in (".raw565pack", ".meta"):
        try:
            dst.with_suffix(ext).unlink()
            gone = True
        except FileNotFoundError:
            pass
    return gone


def restore_original(sd_root, store: dict, key: str) -> str:
    """Retire la personnalisation `key`. Rend l'original sauvegarde s'il existe ('restored') : image fixe (orig_b64) ou animation (orig_pack_z), sinon supprime
    le(s) fichier(s) perso pour que le firmware retombe sur le logo du systeme/par defaut ('deleted'). 'none' si la cle est inconnue.
    Une animation perso ne touche jamais le .raw565 voisin : le retirer = supprimer pack+meta (et rebuild du cache, dont le type 'g' n'a plus lieu d'etre)."""
    item = find_item(store, key)
    if item is None:
        return "none"
    dst = target_path(sd_root, key)
    result = "deleted"
    if is_anim(item):
        drop_anim_files(sd_root, key)
        if item.get("orig_pack_z"):
            for ext, data in item_files({"key": key, "pack_z": item["orig_pack_z"], "delays": item["orig_delays"]}):
                _atomic_write(dst.with_suffix(ext), data)
            result = "restored"
        elif item.get("orig_b64"):
            _atomic_write(dst, rgb_to_raw565(png_b64_to_image(item["orig_b64"])))
            result = "restored"
        elif dst.exists():
            result = "restored"          # un .raw565 d'origine reste en place
        store["items"].remove(item)
        try:
            _tool().build_cache(Path(sd_root) / "systems", Path(sd_root))
        except Exception:   # noqa: BLE001
            pass
        return result
    if item.get("orig_pack_z"):          # l'original etait une animation (remplacee ensuite par une image fixe)
        for ext, data in item_files({"key": key, "pack_z": item["orig_pack_z"], "delays": item["orig_delays"]}):
            _atomic_write(dst.with_suffix(ext), data)
        result = "restored"
        _rebuild_cache(sd_root)
    elif item.get("orig_b64"):
        _atomic_write(dst, rgb_to_raw565(png_b64_to_image(item["orig_b64"])))
        result = "restored"
    else:
        try:
            dst.unlink()
        except FileNotFoundError:
            pass
    store["items"].remove(item)
    return result


IMG_EXTS = (".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".svg")


def plan_batch(folder, rows: list) -> dict:
    """Associe les images d'un dossier aux jeux d'un systeme : nom du fichier (sans extension) = nom de la ROM, espaces et casse ignores.
    Renvoie {matched: [(Path, row)], ambiguous: [(Path, [row,...])], unknown: [Path]}. Les lignes viennent de system_rows()."""
    by_stem, by_name = {}, {}
    for r in rows:
        by_stem.setdefault(re.sub(r"[\s_]+", "", r["stem"]).lower(), []).append(r)
        by_name.setdefault(re.sub(r"[\s_]+", "", r["name"]).lower(), []).append(r)
    out = {"matched": [], "ambiguous": [], "unknown": []}
    used = {}
    for p in sorted(Path(folder).iterdir(), key=lambda q: q.name.lower()):
        if not p.is_file() or p.suffix.lower() not in IMG_EXTS:
            continue
        k = re.sub(r"[\s_]+", "", p.stem).lower()
        cands = by_stem.get(k) or by_name.get(k) or []
        uniq = {c["key"].lower(): c for c in cands}
        if len(uniq) == 1:
            row = next(iter(uniq.values()))
            if row["key"].lower() in used:     # deux fichiers pour le meme jeu : le 1er (ordre alphabetique) l'emporte
                out["ambiguous"].append((p, [row]))
            else:
                used[row["key"].lower()] = p
                out["matched"].append((p, row))
        elif uniq:
            out["ambiguous"].append((p, list(uniq.values())))
        else:
            out["unknown"].append(p)
    return out


def import_batch(sd_root, store: dict, matched: list, mode: str = "pixel", bg=(0, 0, 0), log=print) -> dict:
    """Importe les (fichier, ligne) de plan_batch dans le store et les ecrit sur la SD. N'enregistre pas le store (a la charge de l'appelant).
    Une image illisible est comptee en erreur sans arreter le lot. Renvoie {done, errors: [(nom, message)], dark: [cles]}."""
    res = {"done": 0, "errors": [], "dark": []}
    keys = []
    dropped_any = False
    for path, row in matched:
        try:
            item, rep, _img = make_best_item(row["system"], row["rom"], path, name=row["name"], mode=mode, bg=bg, stem=row["stem"])
            dropped_any = stage_item(sd_root, store, item)[1] or dropped_any
        except Exception as e:   # noqa: BLE001
            res["errors"].append((Path(path).name, str(e)))
            continue
        if "dark" in rep["warnings"]:
            res["dark"].append(item["key"])
        keys.append(item["key"])
        res["done"] += 1
    if keys:
        apply_store(sd_root, store, keys=keys, log=log)
        if dropped_any:
            _rebuild_cache(sd_root)
    return res


PACK_MAX_BYTES = 64 * 1024 * 1024
_PACK_KEEP = ("key", "rom", "name", "mode", "bg", "png_b64", "sha1", "src", "date", "pack_z")


def export_pack(store: dict, path, keys=None) -> int:
    """Ecrit un paquet de logos partageable (zip : logos.json + LISEZMOI.txt). Sans `orig_b64`/`prev` : ce sont les logos d'origine de CETTE carte SD,
    sans interet (et plus lourds) pour quelqu'un d'autre. keys=None : tout. Renvoie le nombre d'entrees."""
    import zipfile

    want = None if keys is None else {k.lower() for k in keys}
    items = [{k: it[k] for k in _PACK_KEEP + ("delays",) if k in it} for it in store["items"] if want is None or it["key"].lower() in want]
    data = {"version": STORE_VERSION, "panel": [W, H], "items": sorted(items, key=lambda it: it["key"].lower())}
    readme = ("Paquet de logos RecalBoxDMD (dalle 128x32).\nImportez-le avec l'onglet Personnalisation > Importer un paquet.\n"
              f"{len(items)} logo(s) : cle = <systeme>/<nom du visuel>.\n\n"
              "Contenu du zip :\n"
              "- logos.json : les logos (images PNG encodees en base64, delais et images des animations). C'est CE fichier que lit « Importer un paquet » : ne le modifiez pas.\n"
              "- apercus/<systeme>/<nom>.png : les memes logos en images, agrandis x4, pour les VOIR et les partager (une animation y est representee par sa 1re image).\n"
              "  Ces apercus ne sont pas lus a l'import.\n")
    part = Path(str(path) + ".part")
    with zipfile.ZipFile(part, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("logos.json", json.dumps(data, ensure_ascii=False, indent=1))
        z.writestr("LISEZMOI.txt", readme)
        for it in items:                      # apercus lisibles : le zip ne contenait que du JSON, on n'y « voyait » aucun logo
            try:
                import io
                from PIL import Image

                system, stem = split_key(it["key"])
                img = png_b64_to_image(it["png_b64"]).resize((W * 4, H * 4), Image.NEAREST)
                buf = io.BytesIO()
                img.save(buf, "PNG")
                z.writestr(f"apercus/{system}/{stem}.png", buf.getvalue())
            except Exception:   # noqa: BLE001 - un apercu manquant ne doit jamais empecher l'export
                continue
    os.replace(part, path)
    return len(items)


def read_pack(path) -> tuple:
    """Lit et VALIDE un paquet (peut venir d'un inconnu) : renvoie (items valides, [(cle, raison)] rejetes). Cle verifiee (split_key : pas de
    traversee de dossier), PNG 128x32 decodable, empreinte coherente. ValueError si ce n'est pas un paquet."""
    import zipfile

    try:
        with zipfile.ZipFile(path) as z:
            info = z.getinfo("logos.json")
            if info.file_size > PACK_MAX_BYTES:
                raise ValueError("logos.json trop gros")
            with z.open(info) as f:
                data = json.loads(f.read(PACK_MAX_BYTES + 1).decode("utf-8"))
    except (OSError, KeyError, zipfile.BadZipFile) as e:
        raise ValueError(f"paquet illisible : {e}") from None
    except (UnicodeDecodeError, json.JSONDecodeError) as e:
        raise ValueError(f"logos.json illisible : {e}") from None
    if not isinstance(data, dict) or not isinstance(data.get("items"), list) or data.get("version", 0) > STORE_VERSION:
        raise ValueError("structure de paquet inattendue")
    ok, bad, seen = [], [], set()
    for raw in data["items"]:
        key = str(raw.get("key", "")) if isinstance(raw, dict) else ""
        try:
            split_key(key)
            if key.lower() in seen:
                raise ValueError("doublon")
            it = {k: raw[k] for k in _PACK_KEEP if k in raw and isinstance(raw[k], str)}
            it["key"] = key
            if "pack_z" in it:
                it["delays"] = raw.get("delays")
            img = png_b64_to_image(it["png_b64"])
            if img.size != (W, H):
                raise ValueError(f"taille {img.size[0]}x{img.size[1]}")
            if not it.get("sha1"):
                it["sha1"] = hashlib.sha1(rgb_to_raw565(img)).hexdigest()
            item_raw(it)
        except Exception as e:   # noqa: BLE001
            bad.append((key or "?", str(e)))
            continue
        seen.add(key.lower())
        it.setdefault("rom", key.split("/", 1)[1])
        it.setdefault("name", key.split("/", 1)[1])
        it.setdefault("date", datetime.now().isoformat(timespec="seconds"))
        ok.append(it)
    return ok, bad


BACKUP_KIND = "recalboxdmd-logos-backup"


def export_backup(store: dict, path) -> int:
    """SAUVEGARDE COMPLETE de toutes mes personnalisations dans un zip (backup.json + logos.json = le store entier, avec les logos d'ORIGINE `orig_*` et la
    version precedente `prev`, contrairement au paquet partageable d'export_pack). Relue par read_backup() / restore_backup(). Renvoie le nombre d'entrees."""
    import zipfile

    items = sorted(store["items"], key=lambda it: it["key"].lower())
    data = {"version": STORE_VERSION, "panel": [W, H], "items": items}
    meta = {"kind": BACKUP_KIND, "date": datetime.now().isoformat(timespec="seconds"), "count": len(items), "version": STORE_VERSION}
    readme = ("Sauvegarde RecalBoxDMD de mes personnalisations de logos (dalle 128x32).\n"
              "Restaurez-la avec l'onglet Personnalisation > Outils > Restaurer depuis une sauvegarde.\n"
              f"{len(items)} personnalisation(s). logos.json contient les images (PNG en base64), les animations, les logos d'origine et la version precedente : ne le modifiez pas.\n")
    part = Path(str(path) + ".part")
    with zipfile.ZipFile(part, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("backup.json", json.dumps(meta, ensure_ascii=False, indent=1))
        z.writestr("logos.json", json.dumps(data, ensure_ascii=False, indent=1))
        z.writestr("LISEZMOI.txt", readme)
    os.replace(part, path)
    return len(items)


def read_backup(path) -> tuple:
    """Lit et VALIDE une sauvegarde faite par export_backup : (items complets valides, [(cle, raison)] rejetes). ValueError si ce n'est pas une sauvegarde
    (un paquet partageable n'en est pas une : il n'a pas backup.json). Cle verifiee (pas de traversee de dossier), PNG 128x32 decodable."""
    import zipfile

    try:
        with zipfile.ZipFile(path) as z:
            meta = json.loads(z.read("backup.json").decode("utf-8"))
            if not isinstance(meta, dict) or meta.get("kind") != BACKUP_KIND:
                raise ValueError("ce zip n'est pas une sauvegarde de personnalisations")
            info = z.getinfo("logos.json")
            if info.file_size > PACK_MAX_BYTES:
                raise ValueError("logos.json trop gros")
            with z.open(info) as f:
                data = json.loads(f.read(PACK_MAX_BYTES + 1).decode("utf-8"))
    except (OSError, KeyError, zipfile.BadZipFile) as e:
        raise ValueError(f"sauvegarde illisible : {e}") from None
    except (UnicodeDecodeError, json.JSONDecodeError) as e:
        raise ValueError(f"sauvegarde illisible : {e}") from None
    if not isinstance(data, dict) or not isinstance(data.get("items"), list) or data.get("version", 0) > STORE_VERSION:
        raise ValueError("structure de sauvegarde inattendue")
    ok, bad, seen = [], [], set()
    for raw in data["items"]:
        key = str(raw.get("key", "")) if isinstance(raw, dict) else ""
        try:
            split_key(key)
            if key.lower() in seen:
                raise ValueError("doublon")
            it = dict(raw)
            it["key"] = key
            img = png_b64_to_image(it["png_b64"])
            if img.size != (W, H):
                raise ValueError(f"taille {img.size[0]}x{img.size[1]}")
            if not it.get("sha1"):
                it["sha1"] = hashlib.sha1(rgb_to_raw565(img)).hexdigest()
            item_raw(it)
        except Exception as e:   # noqa: BLE001
            bad.append((key or "?", str(e)))
            continue
        seen.add(key.lower())
        ok.append(it)
    return ok, bad


def restore_backup(sd_root, store: dict, items: list, log=print) -> dict:
    """Remet les entrees de read_backup() dans le store (la sauvegarde PREND LE PAS sur une entree de meme cle, les autres entrees sont conservees) et les ecrit
    sur la SD / le miroir. N'enregistre pas le store. Renvoie {restored, replaced}."""
    res = {"restored": 0, "replaced": 0}
    keys = []
    for it in items:
        old = find_item(store, it["key"])
        if old is not None:
            store["items"].remove(old)
            res["replaced"] += 1
        store["items"].append(it)
        keys.append(it["key"])
        res["restored"] += 1
    if keys:
        apply_store(sd_root, store, keys=keys, log=log)
    return res


def import_pack(sd_root, store: dict, items: list, overwrite: bool = False, log=print) -> dict:
    """Ajoute les entrees validees de read_pack() au store et les ecrit sur la SD. overwrite=False : une cle deja personnalisee est conservee.
    N'enregistre pas le store. Renvoie {added, replaced, skipped: [cles]}."""
    res = {"added": 0, "replaced": 0, "skipped": []}
    keys = []
    dropped_any = False
    for it in items:
        old = find_item(store, it["key"])
        if old is not None and not overwrite:
            res["skipped"].append(it["key"])
            continue
        dropped_any = stage_item(sd_root, store, it)[1] or dropped_any
        res["replaced" if old is not None else "added"] += 1
        keys.append(it["key"])
    if keys:
        apply_store(sd_root, store, keys=keys, log=log)
        if dropped_any:
            _rebuild_cache(sd_root)
    return res


def make_card_item(system: str, rom: str, name: str, stem: str = None):
    """Carte texte (nom du jeu, text_card.py) comme remplacant : meme sortie que make_item, mode « card ». Pour un logo noir ou absent."""
    import tempfile

    import text_card

    key = f"{system}/{stem}" if stem else make_key(system, rom)
    split_key(key)
    fd, tmp = tempfile.mkstemp(suffix=".png")
    os.close(fd)
    try:
        text_card.make_text_card(name or key.split("/", 1)[1], tmp)
        item, rep, img = make_item(system, rom, tmp, name=name, mode="pixel", bg=(0, 0, 0), stem=stem)
    finally:
        try:
            os.unlink(tmp)
        except OSError:
            pass
    item["mode"], item["src"] = "card", "(carte texte)"
    return item, rep, img


def import_cards(sd_root, store: dict, rows: list, states=("missing",), dark_keys=(), log=print) -> dict:
    """Cartes texte pour les lignes dont l'etat est dans `states` (ex. 'missing') OU dont la cle est dans `dark_keys` (logos noirs reperes par
    scan_dark). Les jeux deja personnalises sont ignores. N'enregistre pas le store. Renvoie {done, errors}."""
    res = {"done": 0, "errors": []}
    dark = {k.lower() for k in dark_keys}
    keys = []
    dropped_any = False
    for r in rows:
        if r["state"] == "custom" or not (r["state"] in states or r["key"].lower() in dark):
            continue
        try:
            item, _rep, _img = make_card_item(r["system"], r["rom"], r["name"], stem=r["stem"])
            dropped_any = stage_item(sd_root, store, item)[1] or dropped_any
        except Exception as e:   # noqa: BLE001
            res["errors"].append((r["key"], str(e)))
            continue
        keys.append(item["key"])
        res["done"] += 1
    if keys:
        apply_store(sd_root, store, keys=keys, log=log)
        if dropped_any:
            _rebuild_cache(sd_root)
    return res


def import_cards_for(sd_root, store: dict, rows: list, log=print) -> dict:
    """Carte texte pour CHAQUE ligne donnee (selection multiple de l'interface), meme deja personnalisee : l'ancien remplacant passe en `prev` (annulable).
    N'enregistre pas le store. Renvoie {done, errors}."""
    res = {"done": 0, "errors": []}
    keys = []
    dropped_any = False
    for r in rows:
        try:
            item, _rep, _img = make_card_item(r["system"], r["rom"], r["name"], stem=r["stem"])
            dropped_any = stage_item(sd_root, store, item)[1] or dropped_any
        except Exception as e:   # noqa: BLE001
            res["errors"].append((r["key"], str(e)))
            continue
        keys.append(item["key"])
        res["done"] += 1
    if keys:
        apply_store(sd_root, store, keys=keys, log=log)
        if dropped_any:
            _rebuild_cache(sd_root)
    return res


# --------------------------------------------------------------------------- animations (GIF -> .raw565pack + .meta)
def _anim_pack(frames_rgb: list) -> bytes:
    return b"".join(rgb_to_raw565(f) for f in frames_rgb)


def make_raw_item(system: str, rom: str, path, name: str = "", stem: str = None):
    """Visuel DEJA au format de la dalle (.raw565 ou .raw565pack [+ .meta] : un logo d'un autre jeu de la SD, un fichier du dossier gifs) -> (entree, rapport, 1re image),
    sans re-conversion : les octets sont repris tels quels (clone exact). Delais : .meta voisin (uint16 LE par image), sinon 100 ms. ValueError si le fichier est
    incoherent ou depasse MAX_ANIM_FRAMES."""
    import struct
    import tempfile

    p = Path(path)
    data = p.read_bytes()
    if not data or len(data) % RAW_SIZE:
        raise ValueError(f"{p.name} : taille incoherente ({len(data)} octets, multiple de {RAW_SIZE} attendu)")
    n = len(data) // RAW_SIZE
    if n > MAX_ANIM_FRAMES:
        raise ValueError(f"{n} images : maximum {MAX_ANIM_FRAMES} (heap/SD du DMD)")
    first = raw565_to_image(data[:RAW_SIZE])
    fd, tmp = tempfile.mkstemp(suffix=".png")
    os.close(fd)
    try:
        first.save(tmp, "PNG")
        item, rep, out = make_item(system, rom, tmp, name=name, mode="pixel", bg=(0, 0, 0), stem=stem)
    finally:
        try:
            os.unlink(tmp)
        except OSError:
            pass
    item["src"] = p.name
    if n == 1:
        return item, rep, out
    delays = [100] * n
    meta = p.with_suffix(".meta")
    if meta.is_file():
        raw = meta.read_bytes()
        if len(raw) == 2 * n:
            delays = [max(1, d) for d in struct.unpack(f"<{n}H", raw)]
    item["pack_z"] = base64.b64encode(zlib.compress(data, 9)).decode("ascii")
    item["delays"] = delays
    item["sha1"] = hashlib.sha1(data).hexdigest()
    rep["frames"], rep["duration_ms"] = n, sum(delays)
    return item, rep, out


def list_gifs(sd_root) -> list:
    """Visuels du dossier gifs/ de la SD (playlist) : [{"label": chemin relatif sans extension, "path", "kind": 'raw'|'anim'}], tries. .raw565 l'emporte sur .raw565pack
    de meme nom. Vide si le dossier n'existe pas (ou si la source est un Pi : son dossier gifs n'est pas lu)."""
    base = Path(sd_root) / "gifs"
    out = {}
    if base.is_dir():
        for p in base.rglob("*"):
            ext = p.suffix.lower()
            if ext not in (".raw565", ".raw565pack") or not p.is_file():
                continue
            label = p.relative_to(base).with_suffix("").as_posix()
            kind = "raw" if ext == ".raw565" else "anim"
            if label.lower() not in out or kind == "raw":
                out[label.lower()] = {"label": label, "path": p, "kind": kind}
    return sorted(out.values(), key=lambda d: d["label"].lower())

def gif_frame_count(path) -> int:
    """Nombre d'images d'un GIF/WebP animé (1 si fixe ou illisible)."""
    try:
        with _tool()._open_image_tolerant(Path(path)) as im:
            return max(1, int(getattr(im, "n_frames", 1)))
    except Exception:   # noqa: BLE001
        return 1


def gif_frame(path, index: int):
    """Image `index` (0 = la 1re) d'un GIF/WebP animé, composee, en RGBA. IndexError hors limites."""
    from PIL import ImageSequence

    with _tool()._open_image_tolerant(Path(path)) as im:
        n = max(1, int(getattr(im, "n_frames", 1)))
        if not 0 <= index < n:
            raise IndexError(f"image {index} hors de 0..{n - 1}")
        if n == 1:
            return im.convert("RGBA")
        im.seek(index)
        return im.convert("RGBA")


def _item_from_frame(system, rom, path, index, name, mode, bg, stem):
    """Image FIXE tiree de l'image `index` d'un GIF (capture) : meme chemin que les fixes, le nom de la source garde le numero (« x.gif#12 »)."""
    import tempfile

    fd, tmp = tempfile.mkstemp(suffix=".png")
    os.close(fd)
    try:
        gif_frame(path, index).save(tmp, "PNG")
        item, rep, out = make_item(system, rom, tmp, name=name, mode=mode, bg=bg, stem=stem)
    finally:
        try:
            os.unlink(tmp)
        except OSError:
            pass
    item["src"] = f"{Path(path).name}#{index + 1}"
    return item, rep, out


def make_gif_item(system: str, rom: str, path, name: str = "", mode: str = "pixel", bg=(0, 0, 0), stem: str = None, start: int = 0, end: int = None, fit: bool = False):
    """GIF animé -> (entree, rapport, 1re image RGB). Chaque image passe par le meme moteur que les fixes (render : pixel-perfect...). Delai de chaque image
    = sa duree dans le GIF (comme convert_gif_to_raw565pack_meta du toolkit : defaut 10 ms si absente, plafonne a 65535). Une seule image = image fixe.
    start / end : EXTRAIT (images numerotees depuis 0, `end` inclus ; None = jusqu'a la fin) ; un extrait d'une seule image est une image fixe.
    ValueError au-dela de MAX_ANIM_FRAMES (comptees sur l'extrait) SAUF fit=True : l'extrait est alors ECHANTILLONNE (1 image sur `k`, k = ceil(images / MAX_ANIM_FRAMES)) et le
    delai de chaque image gardee est la SOMME des delais du groupe : la DUREE TOTALE est conservee (rep["resampled"] = k, rep["src_frames"] = images de l'extrait) ; jamais de coupe."""
    from PIL import ImageSequence

    n = gif_frame_count(path)
    if n <= 1:
        return make_item(system, rom, path, name=name, mode=mode, bg=bg, stem=stem)
    a = max(0, int(start or 0))
    b = n - 1 if end is None else min(n - 1, int(end))
    if a > b:
        raise ValueError(f"extrait vide ({a} > {b})")
    if a == b:
        return _item_from_frame(system, rom, path, a, name, mode, bg, stem)
    count = b - a + 1
    step = 1
    if count > MAX_ANIM_FRAMES:
        if not fit:
            raise ValueError(f"{count} images : maximum {MAX_ANIM_FRAMES} (heap/SD du DMD)")
        step = -(-count // MAX_ANIM_FRAMES)             # plafond entier : 1 image gardee sur `step`
    if a == 0:
        item, rep, first = make_item(system, rom, path, name=name, mode=mode, bg=bg, stem=stem)   # 1re image : apercu + cle + controles de base
    else:
        item, rep, first = _item_from_frame(system, rom, path, a, name, mode, bg, stem)
        item["src"] = Path(path).name
    frames, delays, lit = [], [], 0
    with _tool()._open_image_tolerant(Path(path)) as im:
        fallback = im.info.get("duration", 50)
        for i, fr in enumerate(ImageSequence.Iterator(im)):
            if i < a:
                continue
            if i > b:
                break
            try:
                d = int(fr.info.get("duration", fallback))
            except (TypeError, ValueError):
                d = 10
            d = max(1, d if d > 0 else 10)
            if (i - a) % step:                          # image ecartee par l'echantillonnage : son delai s'ajoute a la derniere image gardee (duree totale conservee)
                delays[-1] = min(0xFFFF, delays[-1] + d)
                continue
            out = render(fr.convert("RGBA"), mode, tuple(bg))
            frames.append(out)
            lit += 0 if is_dark(out) else 1
            delays.append(min(0xFFFF, d))
    pack = _anim_pack(frames)
    item["pack_z"] = base64.b64encode(zlib.compress(pack, 9)).decode("ascii")
    item["delays"] = delays
    item["sha1"] = hashlib.sha1(pack).hexdigest()
    if lit and "dark" in rep["warnings"]:
        rep["warnings"].remove("dark")      # la 1re image peut etre noire (fondu) : seul compte « toutes noires »
    rep["frames"] = len(frames)
    rep["duration_ms"] = sum(delays)
    if step > 1:
        rep["resampled"], rep["src_frames"] = step, count
    return item, rep, first


def make_best_item(system: str, rom: str, path, name: str = "", mode: str = "pixel", bg=(0, 0, 0), stem: str = None, animate: bool = True, start: int = 0, end: int = None, fit: bool = False):
    """Image fixe, ou animation si le fichier en contient une et que animate est vrai (start / end : extrait, fit : echantillonnage au-dela du maximum, voir make_gif_item)."""
    if animate and gif_frame_count(path) > 1:
        return make_gif_item(system, rom, path, name=name, mode=mode, bg=bg, stem=stem, start=start, end=end, fit=fit)
    return make_item(system, rom, path, name=name, mode=mode, bg=bg, stem=stem)


def attach_original(sd_root, item: dict) -> None:
    """Garde sur l'entree le visuel actuellement sur la SD (image fixe ; animation aussi si elle tient dans MAX_ANIM_FRAMES) pour que « retirer » le rende."""
    system, stem = split_key(item["key"])
    orig = capture_original(sd_root, system, stem)
    if orig:
        item["orig_b64"] = orig
    hit = existing_logos(sd_root, system).get(stem.lower())
    if hit and hit[1] == "anim":
        try:
            pack = Path(hit[0]).read_bytes()
            meta = Path(hit[0]).with_suffix(".meta").read_bytes()
            delays = list(__import__("struct").unpack(f"<{len(meta) // 2}H", meta[:len(meta) // 2 * 2]))
            if 2 <= len(delays) <= MAX_ANIM_FRAMES and len(pack) == len(delays) * RAW_SIZE:
                item["orig_pack_z"] = base64.b64encode(zlib.compress(pack, 9)).decode("ascii")
                item["orig_delays"] = delays
        except OSError:
            pass


def stage_item(sd_root, store: dict, item: dict):
    """Prepare l'ajout de `item` : garde l'original au 1er remplacement ; si une ANIMATION perso est remplacee par une image fixe, supprime pack+meta (sinon
    le .raw565pack masquerait la nouvelle image). Met l'entree dans le store (pas d'ecriture du .raw565). Renvoie (avertissements, animation_supprimee)."""
    old = find_item(store, item["key"])
    dropped = False
    if old is None:
        attach_original(sd_root, item)
    elif is_anim(old) and not is_anim(item):
        dropped = drop_anim_files(sd_root, item["key"])
    return put_item(store, item), dropped


def _rebuild_cache(sd_root) -> None:
    try:
        _tool().build_cache(Path(sd_root) / "systems", Path(sd_root))
    except Exception:   # noqa: BLE001
        pass


def commit_item(sd_root, store: dict, item: dict, log=print) -> list:
    """stage_item + ecriture sur la SD (apply_store) ; le cache est reconstruit si une animation a disparu (type 'g' devenu faux). N'enregistre pas le store."""
    warns, dropped = stage_item(sd_root, store, item)
    apply_store(sd_root, store, keys=[item["key"]], log=log)
    if dropped:
        _rebuild_cache(sd_root)
    return warns