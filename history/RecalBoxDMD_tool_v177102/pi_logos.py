"""Logos de jeux PERSONNALISES -> Raspberry Pi par Wi-Fi (2026-10-29). Module sans Tk, testable seul (test_pi_logos.py).

Le demon Raspberry Pi >= 3.75 expose cinq routes sur le port 80 (contrat : raspberry-pi/rpi_daemon/LOGOS_ROUTES_375.md) :
  POST /upload?kind=logo&dir=<systeme>   (multipart, un fichier <jeu>.raw565 de 8192 octets) -> ecrit le logo la ou celui du jeu est deja (lettre ou a plat)
  GET  /logo-info?dir=&name=             -> {"exists", "where", "size", "fnv"} (FNV-1a 32 bits du fichier)
  GET  /logo?dir=&name=                  -> les 8192 octets (404 si absent)
  POST /logo-delete?dir=&name=           -> supprime ce fichier (jamais un dossier)
  POST /logos-refresh                    -> UNE fois en fin de lot : le Pi complete son games_cache.bin et redessine l'ecran
Ici : l'envoi des entrees du store (custom_logos) avec comparaison d'empreinte (rien n'est renvoye si le Pi a deja le bon fichier), la lecture de
l'original avant le premier remplacement, et la restauration. Demon >= 3.82 (PI_ANIM_MIN) : ANIMATIONS aussi, en <jeu>.raw565pack + <jeu>.meta (memes routes ; logo-info donne en plus "frames") ; le logo fixe du jeu est alors
supprime (le demon dessine le logo fixe AVANT l'animation) et l'original reste dans le store (orig_b64). Un demon plus ancien ne recoit que les images fixes (animations : SD).
Aucun etat global : l'adresse du Pi est un parametre. Jamais d'envoi vers un demon plus ancien que PI_LOGOS_MIN (il rangerait le fichier comme un GIF).
"""
import json
import os
import threading
import urllib.parse
import urllib.request
import uuid

PI_LOGOS_MIN = 375          # premier demon Raspberry Pi qui connait kind=logo (3.75)
PI_ANIM_MIN = 382           # premier demon qui recoit <jeu>.raw565pack / <jeu>.meta par kind=logo (3.82)
RAW_SIZE = 128 * 32 * 2
MAX_PACK_BYTES = 600 * RAW_SIZE
_FORBIDDEN = set('\\/:*?"<>|')


def _tool():
    import RecalBoxDMD_tool as tool
    return tool


def _cl():
    import custom_logos as cl
    return cl


def fnv32(data: bytes) -> str:
    """FNV-1a 32 bits en hexadecimal minuscule sur 8 chiffres : meme valeur que GET /logo-info."""
    h = 0x811C9DC5
    for b in data:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return "%08x" % h


def system_ok(system: str) -> bool:
    return bool(system) and len(system) <= 48 and system[0] not in "_." and all(c.isascii() and (c.isalnum() or c in "_-") for c in system)


def name_ok(stem: str) -> bool:
    """Meme regle que logoNameOk() du demon sur « <stem>.raw565 » : ASCII imprimable sans espace, sans \\ / : * ? \" < > |, pas de « . » ni « _ » en tete, 127 car. au plus."""
    name = f"{stem}.raw565"
    if not stem or len(name) > 127 or stem[0] in "._":
        return False
    return all(0x20 < ord(c) < 0x7F and c not in _FORBIDDEN for c in stem)


# --------------------------------------------------------------------------- HTTP
def _request(ip: str, method: str, path: str, body: bytes = None, headers: dict = None, timeout: float = 20.0):
    """(statut HTTP, octets). Une erreur HTTP renvoie son statut et son corps ; une erreur reseau leve OSError."""
    req = urllib.request.Request(f"http://{ip}{path}", data=body, method=method, headers=dict({"User-Agent": "recalbox-toolkit"}, **(headers or {})))
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.read(RAW_SIZE * 4)
    except urllib.error.HTTPError as e:
        try:
            return e.code, e.read(20000)
        finally:
            e.close()
    except urllib.error.URLError as e:
        raise OSError(str(e.reason)) from None


def _q(system: str, stem: str, ext: str = ".raw565") -> str:
    return "dir=" + urllib.parse.quote(system) + "&name=" + urllib.parse.quote(f"{stem}{ext}")


def _err(body: bytes) -> str:
    return body.decode("utf-8", "replace").strip()[:200] or "reponse vide"


def probe(ip: str):
    """Reponse de GET /load si l'hote est un DMD (voir RecalBoxDMD_tool._dmd_probe), sinon None."""
    return _tool()._dmd_probe(ip, 6.0)


def anim_supported(info) -> bool:
    """Le demon (probe) recoit-il les animations ? Raspberry Pi >= PI_ANIM_MIN."""
    ok, _why = supported(info)
    return ok and _tool().fw_version_num(info.get("version")) >= PI_ANIM_MIN


def supported(info) -> tuple:
    """(ok, detail) : `info` = probe(). Seul un demon Raspberry Pi >= PI_LOGOS_MIN accepte les routes de logos."""
    if not info:
        return False, "no_dmd"
    if not info.get("is_pi"):
        return False, "not_pi"
    if _tool().fw_version_num(info.get("version")) < PI_LOGOS_MIN:
        return False, "too_old"
    return True, "ok"


def logo_info(ip: str, system: str, stem: str):
    """{"exists": False} ou {"exists": True, "where", "size", "fnv"} ; None si la requete echoue (nom refuse, demon ancien...)."""
    if not (system_ok(system) and name_ok(stem)):
        return None
    st, body = _request(ip, "GET", "/logo-info?" + _q(system, stem), timeout=10.0)
    if st != 200:
        return None
    try:
        return json.loads(body.decode("utf-8", "replace"))
    except ValueError:
        return None


def list_logos(ip: str, system: str, timeout: float = 30.0):
    """GET /logos-list?dir=<systeme> (demon >= 3.91) -> (le Pi possede ce systeme?, {stems .raw565}, {stems .raw565pack}) EN MINUSCULES, ou None (route absente = demon plus ancien, reseau
    en panne, reponse illisible). Un seul aller-retour pour tout un systeme au lieu d'un /logo-info par jeu. Lecture complete de la reponse (plusieurs centaines de Ko pour mame)."""
    try:
        req = urllib.request.Request(f"http://{ip}/logos-list?dir=" + urllib.parse.quote(system), headers={"User-Agent": "recalbox-toolkit"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            if r.status != 200:
                return None
            d = json.loads(r.read(16 * 1024 * 1024).decode("utf-8", "replace"))
        return bool(d.get("sys")), {str(s).lower() for s in d.get("static", [])}, {str(s).lower() for s in d.get("anim", [])}
    except (OSError, ValueError):
        return None


# Noms de systemes Recalbox connus : le demon n'a pas de route « liste des systemes » (seulement /logos-list?dir=<systeme>, qui dit si UN systeme existe sur le Pi). Sans dossier de ROMs
# configure et avec un miroir vide (version fraichement INSTALLEE, 2026-11-17) l'onglet ne connaissait que les systemes de l'instantane du Pi (2 sur 86) : on interroge donc aussi cette liste
# (123 noms, ~2 s) et on garde ceux que le Pi possede. Un systeme hors liste est ajoute par le dossier de ROMs, le miroir ou l'instantane.
KNOWN_SYSTEMS = tuple("""3do 64dd amiga1200 amiga600 amigacd32 amstradcpc apple2 apple2gs arcadia2001 archimedes atari2600 atari5200 atari7800 atari800 atarist atomiswave bk c64 cavestory cdi
channelf colecovision cplus4 daphne dos dragon dreamcast fbneo fds gamecom gamecube gamegear gb gba gbc gw gx4000 hatari hng64 intellivision jaguar jaguarcd lowresnx lutro lynx mame mastersystem
megadrive megaduck model1 model2 model3 moonlight mrboom msx1 msx2 msxturbor multivision n-gage n64 namcos22 naomi naomi2 naomigd nds neogeo neogeocd nes ngp ngpc o2em openbor oricatmos palm pc88
pc98 pcengine pcenginecd pcfx pcv2 pet pico pokemini prboom ps2 psp psx pv1000 satellaview scummvm scv sega32x segacd sg1000 snes solarus spectravideo stv sufami supergrafx supervision switch
thomson ti994a tic80 triforce uzebox vc4000 vectrex vic20 videopacplus virtualboy vpinball vsmile wasm4 wswan wswanc x1 x68000 xbox zx81 zxspectrum""".split())


def list_many(ip: str, systems, workers: int = 12, progress=None) -> dict:
    """{systeme: (existe, statiques, animations)} pour chaque systeme dont la liste a ete lue ; {} si le demon n'a pas la route (le 1er essai echoue : on n'insiste pas).
    progress(fait, total) : appele au fil de l'eau (thread de fond) pour afficher une progression (123 interrogations = ~18 s a 6 en parallele, 2026-11-17)."""
    from concurrent.futures import ThreadPoolExecutor
    systems = [s for s in systems if system_ok(s)]
    if not systems:
        return {}
    first = list_logos(ip, systems[0])
    if first is None:
        return {}
    out = {systems[0]: first}
    rest = systems[1:]
    total = len(systems)
    if progress:
        progress(1, total)
    if rest:
        with ThreadPoolExecutor(max_workers=max(1, workers)) as ex:
            for n, (s, res) in enumerate(zip(rest, ex.map(lambda x: list_logos(ip, x), rest)), 2):
                if res is not None:
                    out[s] = res
                if progress:
                    progress(n, total)
    return out


def get_logo(ip: str, system: str, stem: str):
    """Les 8192 octets du logo actuel, ou None s'il n'y en a pas."""
    if not (system_ok(system) and name_ok(stem)):
        return None
    st, body = _request(ip, "GET", "/logo?" + _q(system, stem), timeout=10.0)
    return body if st == 200 and len(body) == RAW_SIZE else None


def put_logo(ip: str, system: str, stem: str, raw: bytes, retries: int = 2) -> tuple:
    """(ok, detail). Le demon ecrit un .tmp puis renomme : jamais de logo tronque. Une reprise si le reseau a flanche."""
    if len(raw) != RAW_SIZE:
        return False, "size"
    if not (system_ok(system) and name_ok(stem)):
        return False, "name"
    return _put_file(ip, system, f"{stem}.raw565", raw, retries, 30.0)


def anim_info(ip: str, system: str, stem: str):
    """Comme logo_info, pour l'animation du jeu (<jeu>.raw565pack) : {"exists", "where", "size", "fnv", "frames"} ; None si la requete echoue (demon < 3.82...)."""
    if not (system_ok(system) and name_ok(stem)):
        return None
    st, body = _request(ip, "GET", "/logo-info?" + _q(system, stem, ".raw565pack"), timeout=10.0)
    if st != 200:
        return None
    try:
        return json.loads(body.decode("utf-8", "replace"))
    except ValueError:
        return None


def put_anim(ip: str, system: str, stem: str, pack: bytes, meta: bytes, retries: int = 2) -> tuple:
    """(ok, detail) : l'animation d'un jeu = <jeu>.raw565pack (2 a 600 images) PUIS <jeu>.meta (2 octets de delai par image). Chaque fichier est ecrit en .tmp puis renomme."""
    if not pack or len(pack) % RAW_SIZE or len(pack) < 2 * RAW_SIZE or len(pack) > MAX_PACK_BYTES or len(meta) != 2 * (len(pack) // RAW_SIZE):
        return False, "size"
    if not (system_ok(system) and name_ok(stem)):
        return False, "name"
    ok, detail = _put_file(ip, system, f"{stem}.raw565pack", pack, retries, 120.0)
    if not ok:
        return False, detail
    return _put_file(ip, system, f"{stem}.meta", meta, retries, 30.0)


def delete_anim(ip: str, system: str, stem: str) -> tuple:
    """(ok, detail) : supprime l'animation du jeu (le demon retire aussi son .meta). Deja absente = reussi."""
    if not (system_ok(system) and name_ok(stem)):
        return False, "name"
    try:
        st, ans = _request(ip, "POST", "/logo-delete?" + _q(system, stem, ".raw565pack"), b"", None, 15.0)
    except OSError as e:
        return False, str(e)
    if st == 200 or st == 404:
        return True, "ok" if st == 200 else "absent"
    return False, _err(ans)


def _put_file(ip: str, system: str, filename: str, raw: bytes, retries: int, timeout: float) -> tuple:
    boundary = uuid.uuid4().hex
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="{filename}"\r\nContent-Type: application/octet-stream\r\n\r\n').encode("ascii") + raw + f"\r\n--{boundary}--\r\n".encode("ascii")
    last = "net"
    for _ in range(max(1, retries)):
        try:
            st, ans = _request(ip, "POST", "/upload?kind=logo&dir=" + urllib.parse.quote(system), body, {"Content-Type": f"multipart/form-data; boundary={boundary}"}, timeout)
            if st == 200:
                files = (json.loads(ans.decode("utf-8", "replace")).get("files") or [])
                if files and files[0].get("ok"):
                    return True, "ok"
                last = str((files[0].get("err") if files else "") or "reponse vide")
            else:
                last = _err(ans)
        except (OSError, ValueError) as e:
            last = f"{type(e).__name__}: {e}"
    return False, last


def delete_logo(ip: str, system: str, stem: str) -> tuple:
    """(ok, detail). Un logo deja absent compte comme reussi."""
    if not (system_ok(system) and name_ok(stem)):
        return False, "name"
    try:
        st, ans = _request(ip, "POST", "/logo-delete?" + _q(system, stem), b"", None, 15.0)
    except OSError as e:
        return False, str(e)
    if st == 200 or st == 404:
        return True, "ok" if st == 200 else "absent"
    return False, _err(ans)


def refresh(ip: str, games, dry: bool = False):
    """POST /logos-refresh : `games` = [(systeme, jeu)]. -> dict du demon {"added","known","nologo","invalid","cache","identical"} ou None."""
    items = [f"{s}/{g}" for s, g in games if system_ok(s) and name_ok(g)]
    form = {"games": "|".join(items)}
    if dry:
        form["dry"] = "1"
    try:
        st, ans = _request(ip, "POST", "/logos-refresh", urllib.parse.urlencode(form).encode("ascii"), {"Content-Type": "application/x-www-form-urlencoded"}, 30.0)
        return json.loads(ans.decode("utf-8", "replace")) if st == 200 else None
    except (OSError, ValueError):
        return None


# --------------------------------------------------------------------------- store -> Pi
def _drop_mirror_static(mirror, key) -> bool:
    """Le demon retire le .raw565 FIXE d'un jeu quand il recoit son animation (le fixe est dessine avant l'animation). Le miroir local (image du Pi, lue par l'onglet comme
    une carte SD ou le .raw565 l'emporte sur le .raw565pack) doit faire pareil, sinon l'apercu « Existant » montre l'ancien logo fixe au lieu de l'animation du Pi.
    True si un fichier a ete supprime."""
    if mirror is None:
        return False
    try:
        _cl().target_path(mirror, key).unlink()
        return True
    except OSError:
        return False


def push_store(ip: str, store: dict, keys=None, log=print, progress_cb=None, mirror=None) -> dict:
    """Envoie les entrees du store (images FIXES) vers le Pi. keys : limiter a ces cles.
    Modifie les entrees (orig_b64, pi_orig_done, pi_sent) : l'appelant enregistre le store ensuite.
    Rapport : sent / unchanged / skipped_anim (animations : SD seulement) / errors [(cle, code)] / refresh (reponse de /logos-refresh ou None) / mirror_changed (cles dont le
    .raw565 fixe a ete retire du miroir `mirror` parce que le Pi n'a plus que l'animation).
    Une entree dont l'empreinte est deja celle du Pi n'est pas renvoyee. Un seul /logos-refresh en fin de lot, meme si tout a ete ignore."""
    cl = _cl()
    rep = {"sent": [], "unchanged": [], "skipped_anim": [], "skipped_name": [], "errors": [], "refresh": None, "mirror_changed": []}
    wanted = {k.lower() for k in keys} if keys is not None else None
    todo = [it for it in store.get("items", []) if wanted is None or it.get("key", "").lower() in wanted]
    anim_ok = anim_supported(probe(ip)) if todo else False      # un seul GET /load : ce demon recoit-il les animations ?
    for n, it in enumerate(todo, 1):
        key = it.get("key", "")
        if progress_cb:
            progress_cb(n, len(todo), key)
        try:
            system, stem = cl.split_key(key)
        except ValueError as e:
            rep["errors"].append((key, "key"))
            log(f"   ✖ {key} : {e}")
            continue
        anim = cl.is_anim(it)
        if anim:
            if not anim_ok:
                rep["skipped_anim"].append(key)
                log(f"   ? {key} : animation, non envoyee (ce demon ne recoit que les images fixes : mise a jour >= 3.82 requise)")
                continue
        if not (system_ok(system) and name_ok(stem)):
            # NOM que le Pi ne sait pas enregistrer (ASCII imprimable sans espace ni \ / : * ? " < > |) : ce n'est pas une panne d'envoi. Ex. vpinball : « Checkpoint<U+00A0>(Data East 1991).vpx »,
            # l'espace INSECABLE du nom de ROM devient « ? » dans le nom du fichier sur la carte du Pi (iocharset=ascii) : le demon refuse « ? » a l'envoi. Compte a part, pas dans les erreurs.
            rep["skipped_name"].append(key)
            log(f"   ? {key} : nom non enregistrable par le Pi (caractere non ASCII, espace ou interdit) : a ecrire par la carte SD")
            continue
        try:
            if anim:
                (_e1, pack), (_e2, meta) = cl.item_files(it)
                raw = pack
            else:
                raw = cl.item_raw(it)
        except (ValueError, KeyError, OSError) as e:
            rep["errors"].append((key, "item"))
            log(f"   ✖ {key} : {e}")
            continue
        info = logo_info(ip, system, stem)                   # logo FIXE actuel du Pi
        ainfo = anim_info(ip, system, stem) if anim_ok else None   # animation actuelle (demon >= 3.82 seulement)
        has_static = bool(info and info.get("exists"))
        has_anim = bool(ainfo and ainfo.get("exists"))
        if anim:
            same = has_anim and not has_static and ainfo.get("fnv") == fnv32(pack)
        else:
            same = has_static and not has_anim and info.get("fnv") == fnv32(raw)
        if same:
            rep["unchanged"].append(key)
            it["pi_sent"] = True
            if anim and _drop_mirror_static(mirror, key):
                rep["mirror_changed"].append(key)
            continue
        if not it.get("orig_b64") and not it.get("pi_orig_done") and has_static:
            # premier envoi : garder le visuel que le Pi affiche AVANT de l'ecraser, pour pouvoir le rendre (« Retirer la perso »)
            old = get_logo(ip, system, stem)
            if old:
                try:
                    it["orig_b64"] = cl.image_to_png_b64(cl.raw565_to_image(old))
                except (ValueError, OSError):
                    pass
        it["pi_orig_done"] = True
        if anim:
            ok, detail = put_anim(ip, system, stem, pack, meta)
            if ok and has_static:
                ok, detail = delete_logo(ip, system, stem)   # le demon dessine le logo fixe AVANT l'animation : il faut le retirer (l'original est dans le store)
        else:
            ok, detail = put_logo(ip, system, stem, raw)
            if ok and has_anim:
                ok, detail = delete_anim(ip, system, stem)   # l'ancienne animation masquerait le nouveau logo fixe
        if ok:
            rep["sent"].append(key)
            it["pi_sent"] = True
            if anim and _drop_mirror_static(mirror, key):
                rep["mirror_changed"].append(key)
            log(f"   ✔ {key}")
        else:
            rep["errors"].append((key, detail))
            log(f"   ✖ {key} : {detail}")
    games = [cl.split_key(k) for k in rep["sent"] + rep["unchanged"]]
    rep["refresh"] = refresh(ip, games) if (games or rep["sent"]) else None
    return rep


def restore_on_pi(ip: str, item: dict, log=print) -> str:
    """Retire une personnalisation sur le Pi : remet l'original garde dans l'entree (orig_b64), sinon supprime le fichier perso.
    -> 'restored' | 'deleted' | 'error'. Appeler ensuite refresh(). Les originaux animes (orig_pack_z) ne sont pas rendables par Wi-Fi : on supprime."""
    cl = _cl()
    system, stem = cl.split_key(item["key"])
    if cl.is_anim(item):                                   # une animation du Pi : retiree d'abord (le demon retire aussi son .meta)
        ok_a, detail_a = delete_anim(ip, system, stem)
        if not ok_a:
            log(f"   ✖ {item['key']} : {detail_a}")
            return "error"
    if item.get("orig_b64"):
        try:
            raw = cl.rgb_to_raw565(cl.png_b64_to_image(item["orig_b64"]))
        except (ValueError, OSError) as e:
            log(f"   ✖ {item['key']} : original illisible ({e})")
            return "error"
        ok, detail = put_logo(ip, system, stem, raw)
        if not ok:
            log(f"   ✖ {item['key']} : {detail}")
        return "restored" if ok else "error"
    ok, detail = delete_logo(ip, system, stem)
    if not ok:
        log(f"   ✖ {item['key']} : {detail}")
    return "deleted" if ok else "error"
# --------------------------------------------------------------------------- lecture du Pi « comme une carte SD »
def find_supported_pi(known_ip=None, scan: bool = True):
    """Info (voir probe) d'un Raspberry Pi dont le demon accepte les routes de logos, ou None. known_ip : essaye d'abord ; scan=False : jamais de balayage du reseau."""
    info = probe(known_ip) if known_ip else None
    if not (info and info.get("is_pi")) and scan:
        info = _tool().find_pi_dmd(known_ip=known_ip or None)
    return info if info and info.get("is_pi") and supported(info)[0] else None


def _app_dir():
    """Dossier de l'application : a cote de l'EXE (version portable) ou du module en developpement -- meme convention que les preferences (RecalBoxDMD_prefs.py)."""
    import sys
    from pathlib import Path
    return Path(sys.executable).parent if getattr(sys, "frozen", False) else Path(__file__).resolve().parent


def _dir_writable(folder) -> bool:
    import tempfile
    try:
        with tempfile.TemporaryFile(dir=str(folder)):
            pass
        return True
    except Exception:   # noqa: BLE001
        return False


def _safe_ip(ip: str) -> str:
    return "".join(c if (c.isalnum() or c in ".-") else "_" for c in str(ip))


def local_mirror_root(ip: str):
    """Repli quand le dossier de l'application n'est pas modifiable (version INSTALLEE dans Program Files) : %LOCALAPPDATA%\\RecalBoxDMD\\pi_mirror\\<ip>. LOCAL et non Roaming : c'est un cache de
    centaines de Mo, recreable, qui ne doit pas suivre un profil itinerant."""
    from pathlib import Path
    base = os.environ.get("LOCALAPPDATA") or os.environ.get("APPDATA") or str(Path.home())
    return Path(base) / "RecalBoxDMD" / "pi_mirror" / _safe_ip(ip)


def legacy_mirror_root(ip: str):
    """Ancien emplacement du miroir (avant le 2026-11-17) : %APPDATA% (Roaming)\\RecalBoxDMD\\pi_mirror\\<ip>."""
    from pathlib import Path
    base = os.environ.get("APPDATA") or str(Path.home())
    return Path(base) / "RecalBoxDMD" / "pi_mirror" / _safe_ip(ip)


def mirror_root(ip: str):
    """Dossier miroir d'un Pi sur ce PC : il a la forme d'une carte SD (systems/<systeme>/<Lettre|#>/<jeu>.raw565, custom/logos.json) pour que l'onglet lise le Pi avec le meme code
    que pour une carte. DANS LE DOSSIER DE L'APPLICATION (`pi_mirror/<ip>/`, comme les preferences) pour que le remplissage de fond reprenne a chaque lancement en ne telechargeant que ce qui
    manque (jamais Temp, que Windows vide) ; repli %LOCALAPPDATA%\\RecalBoxDMD\\pi_mirror si ce dossier n'est pas modifiable (version installee dans Program Files). RBDMD_MIRROR_DIR (variable
    d'environnement) impose un autre dossier de base : tests et bancs d'essai."""
    from pathlib import Path
    forced = os.environ.get("RBDMD_MIRROR_DIR")
    if forced:
        return Path(forced) / _safe_ip(ip)
    app = _app_dir() / "pi_mirror" / _safe_ip(ip)
    local = local_mirror_root(ip)
    if app.exists():
        return app
    if local.exists():
        return local          # deja utilise par la version installee
    return app if _dir_writable(_app_dir()) else local         # portable / developpement : a cote de l'application ; installee (Program Files) : %LOCALAPPDATA%


def migrate_mirror(ip: str, log=print) -> str:
    """Deplace l'ancien miroir (%APPDATA%) vers le dossier de l'application, une seule fois. -> 'moved' | 'none' (rien a deplacer / deja fait) | 'error'. Ne perd jamais de fichiers :
    copie complete, verification du nombre de fichiers, PUIS suppression de l'ancien dossier."""
    import shutil
    forced = os.environ.get("RBDMD_MIRROR_DIR")
    new, old = mirror_root(ip), legacy_mirror_root(ip)
    if forced or new == old or not old.exists() or (new.exists() and any(new.iterdir())):
        return "none"
    try:
        n_old = sum(1 for p in old.rglob("*") if p.is_file())
        new.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(old, new, dirs_exist_ok=True)          # le dossier cible peut exister VIDE (cree par l'onglet avant la migration)
        n_new = sum(1 for p in new.rglob("*") if p.is_file())
        if n_new < n_old:
            log(f"[Miroir] migration incomplete ({n_new}/{n_old} fichiers) : ancien dossier conserve")
            return "error"
        shutil.rmtree(old, ignore_errors=True)
        log(f"[Miroir] {n_new} fichier(s) deplaces vers {new}")
        return "moved"
    except OSError as e:
        log(f"[Miroir] migration impossible : {e}")
        return "error"


def _atomic_write(path, data: bytes) -> None:
    from pathlib import Path
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    tmp = p.with_suffix(p.suffix + ".tmp")
    tmp.write_bytes(data)
    os.replace(tmp, p)


SNAP_MAX = 16 * 1024 * 1024         # meme plafond que le demon (GET /games-snapshot)


def fetch_snapshot(ip: str, root, timeout: float = 60.0) -> str:
    """Copie dans le miroir `root` l'instantane du dernier Mode 1 du Pi (GET /games-snapshot, demon >= 3.81 : la liste des jeux du scrape et le drapeau « c » des cartes texte,
    que rien d'autre ne distingue d'un logo). -> "ok" | "absent" (404 : demon plus ancien ou pas d'instantane) | "invalid" (pas un instantane : un demon ancien peut repondre 200 avec sa
    page web) | "error" (reseau). Ne leve jamais ; n'ecrit RIEN hors "ok" (un instantane local deja present est conserve)."""
    import tempfile
    from pathlib import Path
    req = urllib.request.Request(f"http://{ip}/games-snapshot", headers={"User-Agent": "recalbox-toolkit"})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            body = r.read(SNAP_MAX + 1)
    except urllib.error.HTTPError as e:
        e.close()
        return "absent" if e.code in (400, 404, 405, 501) else "error"
    except (urllib.error.URLError, OSError, ValueError):
        return "error"
    if not body or len(body) > SNAP_MAX:
        return "invalid"
    cl = _cl()
    with tempfile.TemporaryDirectory() as d:
        probe_file = Path(d) / "games.json"
        probe_file.write_bytes(body)
        if not cl.load_snapshot(probe_file)["systems"]:
            return "invalid"
    try:
        _atomic_write(Path(root) / cl.SNAP_REL, body)
    except OSError:
        return "error"
    return "ok"

def sync_system(ip: str, root, system: str, stems, progress_cb=None, workers: int = 4, present=None) -> dict:
    """Copie dans le miroir `root` les logos du Pi pour les jeux `stems` d'un systeme : GET /logo-info (existence + empreinte) puis GET /logo seulement si le fichier local est absent
    ou different. Un logo que le Pi n'a plus est retire du miroir (le Pi fait foi). Un jeu dont le nom n'est pas accepte par le Pi est ignore. Reseau en panne pour un jeu : erreur, le
    fichier local est laisse tel quel. progress_cb(fait, total) facultatif. present : ensemble (minuscules) des stems .raw565 du Pi, s'il est connu (list_logos) : les autres jeux sont
    declares absents sans requete. -> {"present", "downloaded", "unchanged", "absent", "removed", "skipped", "errors"} (compteurs)."""
    from concurrent.futures import ThreadPoolExecutor
    cl = _cl()
    rep = dict(present=0, downloaded=0, unchanged=0, absent=0, removed=0, skipped=0, errors=0)
    todo = []
    for stem in stems:
        if system_ok(system) and name_ok(stem):
            todo.append(stem)
        else:
            rep["skipped"] += 1
    lock = threading.Lock()
    done = [0]

    def one(stem):
        out = "error"
        try:
            local = cl.target_path(root, f"{system}/{stem}")
            # present = stems .raw565 que le Pi possede (liste /logos-list, demon >= 3.91) : un jeu qui n'y figure pas est absent SANS requete (la plupart des jeux d'un gros systeme)
            info = {"exists": False} if (present is not None and stem.lower() not in present) else logo_info(ip, system, stem)
            if info is None:
                out = "error"
            elif not info.get("exists"):
                out = "absent"
                if local.exists():
                    local.unlink()
                    out = "removed"
            elif local.exists() and fnv32(local.read_bytes()) == info.get("fnv"):
                out = "unchanged"
            else:
                raw = get_logo(ip, system, stem)
                if raw is None:
                    out = "error"
                else:
                    _atomic_write(local, raw)
                    out = "downloaded"
        except (OSError, ValueError):
            out = "error"
        with lock:
            if out == "error":
                rep["errors"] += 1
            else:
                if out in ("downloaded", "unchanged"):
                    rep["present"] += 1
                rep[out] += 1
            done[0] += 1
            n = done[0]
        if progress_cb and (n % 10 == 0 or n == len(todo)):
            progress_cb(n, len(todo))

    with ThreadPoolExecutor(max_workers=max(1, workers)) as ex:
        list(ex.map(one, todo))
    return rep
