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
def push_store(ip: str, store: dict, keys=None, log=print, progress_cb=None) -> dict:
    """Envoie les entrees du store (images FIXES) vers le Pi. keys : limiter a ces cles.
    Modifie les entrees (orig_b64, pi_orig_done, pi_sent) : l'appelant enregistre le store ensuite.
    Rapport : sent / unchanged / skipped_anim (animations : SD seulement) / errors [(cle, code)] / refresh (reponse de /logos-refresh ou None).
    Une entree dont l'empreinte est deja celle du Pi n'est pas renvoyee. Un seul /logos-refresh en fin de lot, meme si tout a ete ignore."""
    cl = _cl()
    rep = {"sent": [], "unchanged": [], "skipped_anim": [], "errors": [], "refresh": None}
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
            rep["errors"].append((key, "name"))
            log(f"   ✖ {key} : nom refuse par le Pi (ASCII sans espace uniquement)")
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


def mirror_root(ip: str):
    """Dossier miroir d'un Pi sur ce PC : il a la forme d'une carte SD (systems/<systeme>/<Lettre|#>/<jeu>.raw565, custom/logos.json) pour que l'onglet lise le Pi avec le meme code
    que pour une carte. Dans %APPDATA% (jamais Temp). Les logos y sont copies a la demande, systeme par systeme (sync_system)."""
    from pathlib import Path
    base = os.environ.get("APPDATA") or str(Path.home())
    safe = "".join(c if (c.isalnum() or c in ".-") else "_" for c in str(ip))
    return Path(base) / "RecalBoxDMD" / "pi_mirror" / safe


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

def sync_system(ip: str, root, system: str, stems, progress_cb=None, workers: int = 4) -> dict:
    """Copie dans le miroir `root` les logos du Pi pour les jeux `stems` d'un systeme : GET /logo-info (existence + empreinte) puis GET /logo seulement si le fichier local est absent
    ou different. Un logo que le Pi n'a plus est retire du miroir (le Pi fait foi). Un jeu dont le nom n'est pas accepte par le Pi est ignore. Reseau en panne pour un jeu : erreur, le
    fichier local est laisse tel quel. progress_cb(fait, total) facultatif. -> {"present", "downloaded", "unchanged", "absent", "removed", "skipped", "errors"} (compteurs)."""
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
            info = logo_info(ip, system, stem)
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
