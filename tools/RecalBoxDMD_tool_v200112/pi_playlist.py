# -*- coding: utf-8 -*-
"""pi_playlist.py -- onglet Playlist du toolkit par Wi-Fi pour un Raspberry Pi (demon >= 3.92).

Principe : le Pi est lu comme une carte SD. On en garde un « SQUELETTE » local (dossier `<miroir>/<ip>_playlists/`) : gifs/<dossier>/<fichier>.gif en fichiers VIDES (les noms
suffisent pour lister, compter et cocher) et playlists/<nom>.txt avec leur vrai contenu. L'onglet travaille sur ce squelette avec le meme code que pour une carte ; apres chaque
action il envoie le resultat au Pi (ecriture de playlist, suppression, televersement) puis relit le Pi (`pull`) : le Pi fait foi, jamais le squelette.
Routes du demon utilisees : /lsgifdirs, /gg-ls, /lsplaylists, /playlist-get, /playlist-write (3.92), /delete-playlist, /delete-folders, /delete-files (3.92), /create-folder, /upload,
/add-to-playlists-batch, /gg-get (apercu d'un GIF)."""
import json
import os
import shutil
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

PL_MIN = 392          # premier demon qui a /playlist-write et /delete-files
GIF_PREVIEW_MAX = 12 * 1024 * 1024


def _tool():
    import RecalBoxDMD_tool as t
    return t


def _pil():
    import pi_logos as p
    return p


def route_exists(ip: str, path: str = "/playlist-get?name=", present=(200, 400), timeout: float = 4.0):
    """v111 -- SONDE de capacite : True = la route existe (le demon repond, meme par une erreur de parametre : 400), False = absente (404), None = injoignable (ne conclut pas).
    `/playlist-get?name=` (nom vide) repond 400 sur un demon qui a la route, 404 sinon : les numeros de version divergent entre les canaux (un demon stable plus recent
    qu'un alpha peut ne pas avoir la fonction), la route elle-meme ne ment pas."""
    try:
        with urllib.request.urlopen(urllib.request.Request(f"http://{ip}{path}", headers={"User-Agent": "recalbox-toolkit"}), timeout=timeout) as r:
            return r.status in present
    except urllib.error.HTTPError as e:
        return e.code in present
    except Exception:   # noqa: BLE001
        return None


def supported(info) -> bool:
    """Le demon sait-il ecrire une playlist par Wi-Fi ? Raspberry Pi >= 3.92 ET route presente (sonde) ; reseau muet : on s'en tient a la version."""
    if not info or not info.get("is_pi"):
        return False
    if _tool().fw_version_num(info.get("version")) < PL_MIN:
        return False
    return route_exists(info.get("ip")) is not False


def skeleton_root(ip: str) -> Path:
    base = _pil().mirror_root(ip)
    return base.with_name(base.name + "_playlists")


def is_skeleton(root) -> bool:
    return root is not None and Path(root).name.endswith("_playlists") and (Path(root) / ".pi_ip").exists()


def _get_text(ip: str, path: str, timeout: float = 20.0) -> str:
    """GET complet (jusqu'a 8 Mo) : pi_logos._request tronque a 32 Ko (fait pour les logos), trop court pour une playlist ou un gros dossier."""
    req = urllib.request.Request(f"http://{ip}{path}", headers={"User-Agent": "recalbox-toolkit"})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.read(8 * 1024 * 1024).decode("utf-8", "replace")
    except urllib.error.HTTPError as e:
        e.close()
        raise OSError(f"HTTP {e.code} {path}") from None
    except urllib.error.URLError as e:
        raise OSError(str(e.reason)) from None


def _q(s: str) -> str:
    return urllib.parse.quote(s, safe="")


def pull(ip: str, log=print) -> dict:
    """Relit le Pi et remet le squelette local a l'identique (dossiers, noms de GIF, playlists). -> {"dirs", "gifs", "playlists"}. Leve OSError si le Pi ne repond pas."""
    root = skeleton_root(ip)
    (root / "gifs").mkdir(parents=True, exist_ok=True)
    (root / "playlists").mkdir(parents=True, exist_ok=True)
    (root / ".pi_ip").write_text(ip, encoding="utf-8")
    dirs = json.loads(_get_text(ip, "/lsgifdirs"))
    total = 0
    for old in (root / "gifs").iterdir():
        if old.is_dir() and old.name not in dirs:
            shutil.rmtree(old, ignore_errors=True)
    for d in dirs:
        ans = json.loads(_get_text(ip, "/gg-ls?path=" + _q("/gifs/" + d), 30.0))
        names = {it[0] for it in ans.get("items", []) if not it[1] and str(it[0]).lower().endswith(".gif")}
        folder = root / "gifs" / d
        folder.mkdir(parents=True, exist_ok=True)
        for f in folder.iterdir():
            if f.is_file() and f.name not in names:
                f.unlink()
        for n in names:
            p = folder / n
            if not p.exists():
                p.write_bytes(b"")
        total += len(names)
    pl_names = json.loads(_get_text(ip, "/lsplaylists"))
    keep = set()
    for fn in pl_names:
        stem = fn[:-4] if fn.lower().endswith(".txt") else fn
        try:
            body = _get_text(ip, "/playlist-get?name=" + _q(stem), 30.0)
        except OSError as e:
            log(f"[Pi playlist] {stem} : {e}")
            continue
        (root / "playlists" / f"{stem}.txt").write_text(body, encoding="utf-8")
        keep.add(stem.lower())
    for f in (root / "playlists").glob("*.txt"):
        if f.stem.lower() not in keep:
            f.unlink()
    return {"dirs": len(dirs), "gifs": total, "playlists": len(keep)}


def push_playlist(ip: str, root, name: str) -> tuple:
    """Envoie <squelette>/playlists/<name>.txt au Pi (/playlist-write). -> (ok, detail)."""
    path = Path(root) / "playlists" / f"{name}.txt"
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    full = ""
    if lines and lines[0].startswith("# FULL:"):
        full = lines[0][len("# FULL:"):].strip()
        lines = lines[1:]
    body = ("\n".join(lines) + "\n").encode("utf-8")
    q = "/playlist-write?name=" + _q(name) + ("&full=" + _q(full) if full else "")
    st, ans = _pil()._request(ip, "POST", q, body, {"Content-Type": "text/plain; charset=utf-8"}, 60.0)
    msg = ans.decode("utf-8", "replace").strip()
    return st == 200 and msg.startswith("OK"), msg


def _post_form(ip: str, path: str, form: dict, timeout: float = 60.0) -> tuple:
    data = urllib.parse.urlencode(form).encode("utf-8")
    st, ans = _pil()._request(ip, "POST", path, data, {"Content-Type": "application/x-www-form-urlencoded"}, timeout)
    msg = ans.decode("utf-8", "replace").strip()
    return st == 200 and msg.startswith("OK"), msg


def delete_playlist(ip: str, name: str) -> tuple:
    return _post_form(ip, "/delete-playlist", {"name": name + ".txt"})


def delete_folders(ip: str, folders) -> tuple:
    return _post_form(ip, "/delete-folders", {"dirs": ",".join(folders)}, 120.0)


def delete_files(ip: str, folder: str, files) -> tuple:
    """Supprime des .gif de gifs/<dossier>/ sur le Pi (/delete-files, sep=nl : un nom par ligne, les noms avec virgule passent). -> (ok, detail)."""
    files = [f for f in files if f]
    if not files:
        return False, "aucun fichier"
    return _post_form(ip, "/delete-files", {"dir": folder, "files": "\n".join(files), "sep": "nl"}, 120.0)


def add_to_playlists(ip: str, folder: str, files) -> int:
    """Apres un televersement : ajoute les nouveaux GIF aux playlists qui referencent deja le dossier (par paquets de 64). -> nombre de lignes ajoutees."""
    n = 0
    files = list(files)
    for i in range(0, len(files), 64):
        ok, msg = _post_form(ip, "/add-to-playlists-batch", {"dir": folder, "files": "\n".join(files[i:i + 64]), "sep": "nl"}, 60.0)
        if ok and ":" in msg:
            try:
                n += int(msg.split(":", 1)[1].strip().split()[0])
            except ValueError:
                pass
    return n


def fetch_gif(ip: str, folder: str, fname: str) -> Path:
    """Apercu d'un GIF du Pi : telecharge /gifs/<dossier>/<fichier> une seule fois dans le squelette (cache `.preview/`). Leve OSError si indisponible."""
    cache = skeleton_root(ip) / ".preview" / folder
    target = cache / fname
    if target.exists() and target.stat().st_size > 0:
        return target
    cache.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(f"http://{ip}/gg-get?path=" + _q(f"/gifs/{folder}/{fname}"), headers={"User-Agent": "recalbox-toolkit"})
    try:
        with urllib.request.urlopen(req, timeout=20.0) as r:
            data = r.read(GIF_PREVIEW_MAX + 1)
    except Exception as e:   # noqa: BLE001
        raise OSError(str(e)) from None
    if len(data) > GIF_PREVIEW_MAX:
        raise OSError("GIF trop gros pour l'apercu")
    target.write_bytes(data)
    return target
