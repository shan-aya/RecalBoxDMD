# -*- coding: utf-8 -*-
"""Mode 21 : ou vivent les GIFs. Deux « magasins » qui exposent les MEMES primitives, pour que game_gifs.py marche indifferemment sur une CARTE SD (dossier local)
ou sur un RASPBERRY PI en Wi-Fi (routes /gg-* du demon >= 3.87) :

    ls(path) -> (existe, [(nom, est_dossier, taille)])      get(path) -> octets        put(path, octets)        put_file(chemin_local, path)
    cp(src, dst)   mv(src, dst)   rm(path)   free() -> octets libres   read_excl() / write_excl(texte)   local_file(path) -> Path local (telecharge si besoin)

Les chemins sont ceux de la SD (« /gifs/Arcade/x.gif », « /gifs_jeux/A/x.gif »). L'ESP32 reste en SD seulement (sa carte, hors du DMD).
Ecritures : seulement /gifs_jeux, /custom/game_gifs_state.json et le fichier d'exclusions ; les originaux de /gifs ne sont jamais modifies (le demon Pi l'impose aussi).
"""
import json
import os
import shutil
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

EXCL_SD_PATH = "/playlists/game_gifs_exclude.txt"


class SdStore:
    kind = "sd"

    def __init__(self, root):
        self.root = Path(root)

    def label(self):
        return str(self.root)

    def p(self, path: str) -> Path:
        return self.root / path.lstrip("/")

    def ls(self, path):
        d = self.p(path)
        if not d.is_dir():
            return False, []
        out = []
        for e in sorted(d.iterdir(), key=lambda q: q.name.lower()):
            try:
                out.append((e.name, e.is_dir(), 0 if e.is_dir() else e.stat().st_size))
            except OSError:
                continue
        return True, out

    def exists(self, path):
        return self.p(path).exists()

    def get(self, path):
        return self.p(path).read_bytes()

    def put(self, path, data: bytes):
        f = self.p(path)
        f.parent.mkdir(parents=True, exist_ok=True)
        tmp = f.with_name(f.name + ".tmp")
        tmp.write_bytes(data)
        os.replace(tmp, f)

    def put_file(self, local, path):
        f = self.p(path)
        f.parent.mkdir(parents=True, exist_ok=True)
        tmp = f.with_name(f.name + ".tmp")
        shutil.copyfile(local, tmp)
        os.replace(tmp, f)

    def cp(self, src, dst):
        self.put_file(self.p(src), dst)

    def mv(self, src, dst):
        d = self.p(dst)
        d.parent.mkdir(parents=True, exist_ok=True)
        s = self.p(src)
        if d.exists() and d.name.lower() == s.name.lower() and d.parent == s.parent:   # meme nom a la casse pres (FAT) : passage par un nom temporaire
            tmp = s.with_name(s.name + ".renaming")
            os.replace(s, tmp)
            os.replace(tmp, d)
        else:
            os.replace(s, d)
        self._prune(s.parent)

    def rm(self, path):
        f = self.p(path)
        f.unlink()
        self._prune(f.parent)

    def _prune(self, d: Path):
        """Supprime un sous-dossier de lettre devenu vide (jamais /gifs_jeux lui-meme)."""
        try:
            if d.parent.name == "gifs_jeux" and d.parent.parent == self.root and d.is_dir() and not any(d.iterdir()):
                d.rmdir()
        except OSError:
            pass

    def free(self):
        try:
            return shutil.disk_usage(self.root).free
        except OSError:
            return None

    def read_excl(self) -> str:
        try:
            return self.p(EXCL_SD_PATH).read_bytes().decode("utf-8", errors="replace").replace("\r\n", "\n")
        except OSError:
            return ""

    def write_excl(self, text: str):
        text = text.replace("\r\n", "\n")
        f = self.p(EXCL_SD_PATH)
        if not text.strip():
            try:
                f.unlink()
            except OSError:
                pass
            return
        self.put(EXCL_SD_PATH, text.encode("utf-8"))

    def local_file(self, path):
        return self.p(path)


class PiStore:
    kind = "pi"

    def __init__(self, ip, timeout=20.0):
        self.ip = ip
        self.timeout = timeout
        self._cache = None

    def label(self):
        return self.ip

    def _url(self, route, **q):
        return f"http://{self.ip}{route}" + ("?" + urllib.parse.urlencode(q, quote_via=urllib.parse.quote) if q else "")

    def _req(self, method, route, body=None, timeout=None, **q):
        req = urllib.request.Request(self._url(route, **q), data=body, method=method, headers={"Content-Type": "application/octet-stream"} if body is not None else {})
        try:
            with urllib.request.urlopen(req, timeout=timeout or self.timeout) as r:
                return r.read()
        except urllib.error.HTTPError as e:
            raise OSError(f"{e.code} {e.read().decode('utf-8', 'replace')[:120]}") from None
        except (urllib.error.URLError, TimeoutError) as e:
            raise OSError(f"Pi injoignable ({e})") from None

    def ls(self, path):
        d = json.loads(self._req("GET", "/gg-ls", path=path).decode("utf-8"))
        return bool(d.get("exists")), [(n, bool(isd), int(sz)) for n, isd, sz in d.get("items", [])]

    def exists(self, path):
        try:
            self._req("GET", "/gg-get", path=path)
            return True
        except OSError:
            return False

    def get(self, path):
        return self._req("GET", "/gg-get", timeout=60, path=path)

    def put(self, path, data: bytes):
        self._req("POST", "/gg-put", body=data, timeout=120, path=path)

    def put_file(self, local, path):
        self.put(path, Path(local).read_bytes())

    def cp(self, src, dst):
        self._req("POST", "/gg-cp", body=b"", src=src, dst=dst)

    def mv(self, src, dst):
        self._req("POST", "/gg-mv", body=b"", src=src, dst=dst)

    def rm(self, path):
        self._req("POST", "/gg-rm", body=b"", path=path)

    def free(self):
        try:
            return int(json.loads(self._req("GET", "/gg-info").decode("utf-8")).get("free", 0)) or None
        except (OSError, ValueError):
            return None

    def read_excl(self) -> str:
        try:
            return self._req("GET", "/game-gifs-exclude").decode("utf-8", errors="replace").replace("\r\n", "\n")
        except OSError:
            return ""

    def write_excl(self, text: str):
        self._req("POST", "/game-gifs-exclude", body=text.replace("\r\n", "\n").encode("utf-8"))

    def snapshot_bytes(self):
        """custom/games.json du Pi (GET /games-snapshot) ou None."""
        try:
            return self._req("GET", "/games-snapshot", timeout=60)
        except OSError:
            return None

    def local_file(self, path):
        """Telecharge le fichier dans un cache temporaire (apercu, copie vers une autre cible) et retourne son chemin local."""
        if self._cache is None:
            self._cache = Path(tempfile.mkdtemp(prefix="gifs_pi_"))
        f = self._cache / path.strip("/").replace("/", "__")
        if not f.is_file():
            f.write_bytes(self.get(path))
        return f

    def close(self):
        if self._cache is not None:
            shutil.rmtree(self._cache, ignore_errors=True)
            self._cache = None


def as_store(x):
    """Un chemin de carte SD (str / Path) ou deja un magasin -> magasin."""
    return x if isinstance(x, (SdStore, PiStore)) else SdStore(x)
