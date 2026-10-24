# ============================================
# RecalBoxDMD_pi.py -- toolkit : preparer et installer un DMD sur Raspberry Pi (Modes 16 et 17)
#
# v1 - 2026-10-19 - safe-modify - Creation (chantier portage Raspberry Pi, jalon 7d).
#   Mode 16 : prepare une carte SD de Raspberry Pi (remplace Raspberry Pi Imager) : telecharge l'image Raspberry Pi OS Lite 32 bits
#             (Bookworm, la version validee) et verifie son SHA-256, l'ecrit sur la carte choisie (droits administrateur : le toolkit
#             se relance lui-meme en eleve, voir helper_main()), ecrit custom.toml (nom d'hote, utilisateur, SSH, Wi-Fi, clavier,
#             fuseau) et depose sur la partition boot le dossier recalboxdmd/ (demon, pages web, service, install.sh, config.ini
#             avec first_boot=1) : le Pi peut aussi s'installer seul (sudo /boot/firmware/recalboxdmd/install.sh).
#   Mode 17 : installe / met a jour le demon sur un Pi deja demarre (SSH/SFTP) et synchronise le contenu de la carte SD du DMD
#             (copie incrementale) vers /opt/recalboxdmd/sd.
# Le charge utile (demon precompile + pages web + deploy/) vient de find_payload() : dossier pi_payload a cote du toolkit (ou de
# l'exe), dossier de l'utilisateur, ou variable RECALBOXDMD_PI_PAYLOAD. Aucun secret n'est ecrit hors de la carte / de la connexion.
# ============================================
import base64  # noqa: F401  (reserve)
import errno
import hashlib
import json
import lzma
import os
import re
import socket
import subprocess
import sys
import time
import urllib.request
from pathlib import Path

RASPIOS_BASE = "https://downloads.raspberrypi.com/raspios_oldstable_lite_armhf/images/"   # « oldstable » = Bookworm (la serie « stable » est Trixie)
IMAGE_FLAVOR = "bookworm"        # version validee avec le demon (NetworkManager, nmcli, custom.toml) : on ne prend PAS la derniere si elle est autre
DEFAULT_HOSTNAME = "rbpidmd"   # meme nom que l'etiquette de la partition media (RBPIDMD) ; l'ancien « dmdpi » memorise est remplace (OLD_HOSTNAMES)
OLD_HOSTNAMES = ("dmdpi", "dmdpi.local")
DEFAULT_USER = "pi"
ALLOWED_BUS = ("USB", "SD", "MMC")
MIN_DISK_BYTES = 7 * 1000 ** 3   # une carte de 8 Go annonce ~7,9 Go
MAX_DISK_BYTES = 260 * 1000 ** 3   # une carte SD fait au plus 256 Go : les gros disques USB (sauvegardes) ne sont jamais proposes
SECTOR = 512
CHUNK = 4 * 1024 * 1024
UA = {"User-Agent": "recalbox-dmd-toolkit"}
PAYLOAD_REPO = "shan-aya/RecalBoxDMD"   # releases « pi-v... » : charge utile du Pi (RecalBoxDMD_pi_payload.zip + .sha256), construite par la CI
PAYLOAD_ASSET = "RecalBoxDMD_pi_payload.zip"


# ---------------------------------------------------------------------------------------------------------------- dossiers
def data_dir() -> Path:
    """Dossier durable du toolkit (jamais %TEMP% : Windows / PC Manager le vident) : cache d'image, taches elevees."""
    base = os.environ.get("LOCALAPPDATA") or str(Path.home())
    d = Path(base) / "RecalBoxDMD"
    d.mkdir(parents=True, exist_ok=True)
    return d


def cache_dir() -> Path:
    d = data_dir() / "pi_cache"
    d.mkdir(parents=True, exist_ok=True)
    return d


def jobs_dir() -> Path:
    d = data_dir() / "pi_jobs"
    d.mkdir(parents=True, exist_ok=True)
    return d


# ---------------------------------------------------------------------------------------------------------------- charge utile
PAYLOAD_FILES = ("bin/dmd_daemon", "web/menu.html", "install.sh", "recalboxdmd.service")


def payload_ok(p: Path) -> bool:
    return all((p / f).is_file() for f in PAYLOAD_FILES)


def find_payload(extra: str = "") -> "Path | None":
    """Dossier de la charge utile du Pi : {bin/dmd_daemon, web/, install.sh, recalboxdmd.service, config.ini.seed}."""
    cands = []
    if extra:
        cands.append(Path(extra))
    env = os.environ.get("RECALBOXDMD_PI_PAYLOAD")
    if env:
        cands.append(Path(env))
    here = Path(__file__).resolve().parent
    cands.append(data_dir() / "pi_payload")   # telechargee depuis GitHub (fetch_payload) : la plus recente
    meipass = getattr(sys, "_MEIPASS", None)
    if meipass:
        cands.append(Path(meipass) / "pi_payload")
    cands += [here / "pi_payload", Path(sys.executable).resolve().parent / "pi_payload"]
    for c in cands:
        try:
            if payload_ok(c):
                return c
        except OSError:
            pass
    return None


def latest_payload_release(timeout: int = 10):
    """Derniere release « pi-v... » du depot (charge utile du Pi) : (tag, url du zip, url du .sha256) ou None."""
    req = urllib.request.Request(f"https://api.github.com/repos/{PAYLOAD_REPO}/releases?per_page=30", headers={**UA, "Accept": "application/vnd.github+json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        rels = json.load(r)
    for rel in rels:
        tag = str(rel.get("tag_name", ""))
        if not tag.startswith("pi-v") or rel.get("draft") or rel.get("prerelease"):
            continue
        assets = {a.get("name"): a.get("browser_download_url") for a in rel.get("assets", [])}
        if PAYLOAD_ASSET in assets and PAYLOAD_ASSET + ".sha256" in assets:
            return tag, assets[PAYLOAD_ASSET], assets[PAYLOAD_ASSET + ".sha256"]
    return None


def fetch_payload(log=None) -> "Path | None":
    """Telecharge la derniere charge utile publiee sur GitHub (release « pi-v... »), verifie son SHA-256 et l'installe dans
    data_dir()/pi_payload. Rien a faire si elle est deja a jour. Retourne le dossier, ou None (hors ligne / aucune release) : le
    toolkit garde alors la charge utile deja presente."""
    import shutil
    import zipfile

    say = log or (lambda m: None)
    try:
        info = latest_payload_release()
    except Exception as e:   # noqa: BLE001
        say(f"Charge utile : GitHub injoignable ({type(e).__name__})")
        return None
    if not info:
        say("Charge utile : aucune release pi-v... trouvee sur GitHub")
        return None
    tag, url_zip, url_sha = info
    dest = data_dir() / "pi_payload"
    tag_file = dest / "GITHUB_TAG"
    try:
        if payload_ok(dest) and tag_file.read_text(encoding="utf-8").strip() == tag:
            return dest
    except OSError:
        pass
    say(f"Telechargement de la charge utile {tag}...")
    req = urllib.request.Request(url_sha, headers=UA)
    with urllib.request.urlopen(req, timeout=20) as r:
        want = r.read().decode("ascii", "replace").split()[0].strip().lower()
    zpath = cache_dir() / f"payload_{tag}.zip"
    h = hashlib.sha256()
    req = urllib.request.Request(url_zip, headers=UA)
    with urllib.request.urlopen(req, timeout=60) as r, open(zpath, "wb") as f:
        while True:
            chunk = r.read(CHUNK)
            if not chunk:
                break
            h.update(chunk)
            f.write(chunk)
    if h.hexdigest() != want:
        zpath.unlink()
        raise RuntimeError(f"SHA-256 de la charge utile {tag} invalide : telechargement rejete")
    staging = data_dir() / "pi_payload_new"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    root = staging.resolve()
    with zipfile.ZipFile(zpath) as z:
        for m in z.namelist():
            tgt = (staging / m).resolve()
            if root != tgt and root not in tgt.parents:
                raise RuntimeError("archive de la charge utile : chemin suspect, rejet")
        z.extractall(staging)
    (staging / "GITHUB_TAG").write_text(tag + "\n", encoding="utf-8")
    if not payload_ok(staging):
        shutil.rmtree(staging)
        raise RuntimeError(f"charge utile {tag} incomplete")
    if dest.exists():
        shutil.rmtree(dest)
    staging.rename(dest)
    say(f"Charge utile {tag} installee : {dest}")
    return dest


def payload_version(p: Path) -> str:
    try:
        v = (p / "VERSION").read_text(encoding="utf-8").strip()
        try:
            return f"{(p / 'GITHUB_TAG').read_text(encoding='utf-8').strip()} ({v})"
        except OSError:
            return v
    except OSError:
        return ""


def iter_payload_files(p: Path):
    """(chemin relatif en '/', chemin local) de tout ce qui doit partir sur le Pi."""
    for root, _dirs, files in os.walk(p):
        for f in files:
            full = Path(root) / f
            yield full.relative_to(p).as_posix(), full


def make_config_seed(recalbox_ip: str = "", language: str = "fr") -> str:
    """config.ini de depart : first_boot=1 (le DMD affiche l'ecran de configuration jusqu'a une sauvegarde conforme)."""
    lines = ["first_boot=1"]
    lang = language if language in ("fr", "en", "es") else "fr"
    lines.append(f"language={lang}")
    ips = [x.strip() for x in re.split(r"[,; ]+", recalbox_ip or "") if x.strip()]
    if len(ips) == 1:
        lines.append(f"recalbox_ip={ips[0]}")
    elif len(ips) > 1:
        lines.append("recalbox_ips=" + ",".join(ips[:4]))
        lines.append(f"recalbox_ip={ips[0]}")
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------------------------------------------- mot de passe
_B64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"


def _b64_24(b2: int, b1: int, b0: int, n: int) -> str:
    w = (b2 << 16) | (b1 << 8) | b0
    out = ""
    for _ in range(n):
        out += _B64[w & 0x3F]
        w >>= 6
    return out


def sha512_crypt(password: str, salt: str = "", rounds: int = 5000) -> str:
    """Hash « $6$ » (SHA-512 crypt, algorithme de Drepper) : le format que custom.toml attend (password_encrypted = true).
    Verifie contre `openssl passwd -6` (voir tests). Pas de module `crypt` : retire de Python 3.13."""
    if not salt:
        salt = "".join(_B64[b & 63] for b in os.urandom(16))
    salt = salt[:16]
    pw = password.encode("utf-8")
    sb = salt.encode("ascii")
    h = hashlib.sha512
    b = h(pw + sb + pw).digest()
    a = pw + sb
    n = len(pw)
    while n > 64:
        a += b
        n -= 64
    a += b[:n]
    n = len(pw)
    while n > 0:
        a += b if (n & 1) else pw
        n >>= 1
    a = h(a).digest()
    dp = h(pw * len(pw)).digest()
    p = (dp * (len(pw) // 64 + 1))[:len(pw)]
    ds = h(sb * (16 + a[0])).digest()
    s = (ds * (len(sb) // 64 + 1))[:len(sb)]
    c = a
    for i in range(rounds):
        ctx = (p if i & 1 else c)
        if i % 3:
            ctx += s
        if i % 7:
            ctx += p
        ctx += (c if i & 1 else p)
        c = h(ctx).digest()
    order = [(0, 21, 42), (22, 43, 1), (44, 2, 23), (3, 24, 45), (25, 46, 4), (47, 5, 26), (6, 27, 48), (28, 49, 7), (50, 8, 29),
             (9, 30, 51), (31, 52, 10), (53, 11, 32), (12, 33, 54), (34, 55, 13), (56, 14, 35), (15, 36, 57), (37, 58, 16),
             (59, 17, 38), (18, 39, 60), (40, 61, 19), (62, 20, 41)]
    out = "".join(_b64_24(c[x], c[y], c[z], 4) for x, y, z in order) + _b64_24(0, 0, c[63], 2)
    pre = "$6$" + (f"rounds={rounds}$" if rounds != 5000 else "") + salt + "$"
    return pre + out


# ---------------------------------------------------------------------------------------------------------------- custom.toml
def _toml_str(s: str) -> str:
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", " ").replace("\r", "") + '"'


def build_custom_toml(hostname: str, user: str, pw_hash: str, ssid: str = "", psk: str = "", country: str = "FR",
                      keymap: str = "fr", timezone: str = "Europe/Paris") -> str:
    """custom.toml de Raspberry Pi OS (meme contenu que Raspberry Pi Imager) : nom d'hote, utilisateur, SSH par mot de passe, Wi-Fi, locale."""
    t = ["config_version = 1", "", "[system]", f"hostname = {_toml_str(hostname)}", "",
         "[user]", f"name = {_toml_str(user)}", f"password = {_toml_str(pw_hash)}", "password_encrypted = true", "",
         "[ssh]", "enabled = true", "password_authentication = true", ""]
    if ssid:
        t += ["[wlan]", f"ssid = {_toml_str(ssid)}", f"password = {_toml_str(psk)}", "password_encrypted = false", "hidden = false",
              f"country = {_toml_str(country)}", ""]
    t += ["[locale]", f"keymap = {_toml_str(keymap)}", f"timezone = {_toml_str(timezone)}", ""]
    return "\n".join(t)


def valid_hostname(h: str) -> bool:
    return bool(re.fullmatch(r"[a-z0-9]([a-z0-9-]{0,30}[a-z0-9])?", h or ""))


def valid_username(u: str) -> bool:
    return bool(re.fullmatch(r"[a-z_][a-z0-9_-]{0,30}", u or ""))


# ---------------------------------------------------------------------------------------------------------------- image
def _http_text(url: str, timeout: float = 30.0) -> str:
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read().decode("utf-8", "replace")


def find_latest_image(base: str = RASPIOS_BASE, flavor: str = IMAGE_FLAVOR, max_dirs: int = 12) -> dict:
    """{name, url, sha_url, date} de la plus recente image Raspberry Pi OS Lite 32 bits de la version `flavor` (Bookworm).
    Parcourt les dossiers du plus recent au plus ancien ; leve RuntimeError si introuvable."""
    idx = _http_text(base)
    dirs = sorted(set(re.findall(r'href="(raspios_[a-z_]*lite_armhf-\d{4}-\d{2}-\d{2})/"', idx)), reverse=True)
    for d in dirs[:max_dirs]:
        try:
            sub = _http_text(f"{base}{d}/")
        except Exception:
            continue
        for name in re.findall(r'href="([^"]+\.img\.xz)"', sub):
            if flavor in name:
                url = f"{base}{d}/{name}"
                return {"name": name, "url": url, "sha_url": url + ".sha256", "date": d.split("-", 1)[1]}
    raise RuntimeError("image Raspberry Pi OS introuvable")


def sha256_file(path: Path, progress=None) -> str:
    h = hashlib.sha256()
    size = path.stat().st_size or 1
    done = 0
    with open(path, "rb") as f:
        while True:
            b = f.read(CHUNK)
            if not b:
                break
            h.update(b)
            done += len(b)
            if progress:
                progress(done, size)
    return h.hexdigest()


def download_image(info: dict, progress=None, log=None) -> Path:
    """Telecharge l'image (.img.xz) dans le cache durable et verifie son SHA-256 publie. Reutilise un fichier deja verifie."""
    dest = cache_dir() / info["name"]
    ok_mark = cache_dir() / (info["name"] + ".verified")
    want = _http_text(info["sha_url"]).split()[0].strip().lower()
    if dest.is_file() and ok_mark.is_file() and ok_mark.read_text().strip() == want:
        if log:
            log("image deja telechargee et verifiee : " + dest.name)
        return dest
    part = dest.with_suffix(dest.suffix + ".part")
    req = urllib.request.Request(info["url"], headers=UA)
    with urllib.request.urlopen(req, timeout=60) as r, open(part, "wb") as f:
        total = int(r.headers.get("Content-Length") or 0)
        done = 0
        while True:
            b = r.read(1024 * 256)
            if not b:
                break
            f.write(b)
            done += len(b)
            if progress and total:
                progress(done, total)
    got = sha256_file(part)
    if got != want:
        part.unlink(missing_ok=True)
        raise RuntimeError(f"somme SHA-256 de l'image incorrecte ({got[:12]}... au lieu de {want[:12]}...)")
    part.replace(dest)
    ok_mark.write_text(want)
    return dest


# ---------------------------------------------------------------------------------------------------------------- disques (Windows)
def _ps(cmd: str, timeout: float = 60.0) -> str:
    full = "[Console]::OutputEncoding=[Text.Encoding]::UTF8; $ErrorActionPreference='Stop'; " + cmd
    r = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", full], capture_output=True, timeout=timeout,
                       creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    if r.returncode != 0:
        raise RuntimeError((r.stderr or r.stdout).decode("utf-8", "replace").strip()[:400])
    return r.stdout.decode("utf-8", "replace")


def list_target_disks() -> list:
    """Disques candidats : USB / SD / MMC, ni systeme ni boot, 7..260 Go. [{number, model, size, bus}]"""
    out = _ps("Get-Disk | Select-Object Number,FriendlyName,Size,@{n='BusType';e={$_.BusType.ToString()}},IsBoot,IsSystem | ConvertTo-Json -Compress")
    data = json.loads(out) if out.strip() else []
    if isinstance(data, dict):
        data = [data]
    res = []
    for d in data:
        bus = str(d.get("BusType") or "")
        size = int(d.get("Size") or 0)
        if int(d["Number"]) == 0 or d.get("IsBoot") or d.get("IsSystem"):
            continue
        if bus.upper() not in ALLOWED_BUS:   # jamais NVMe / SATA / virtuel : seulement les supports amovibles
            continue
        if not (MIN_DISK_BYTES <= size <= MAX_DISK_BYTES):
            continue
        res.append({"number": int(d["Number"]), "model": str(d.get("FriendlyName") or "").strip(), "size": size, "bus": bus})
    return res


def fmt_size(n: int) -> str:
    return f"{n / 1e9:.1f} Go"


def wifi_networks() -> list:
    """Reseaux Wi-Fi 2,4 GHz visibles depuis ce PC (le Pi 3B ne sait pas le 5 GHz). Reutilise le scan du toolkit ; liste vide en cas d'echec."""
    try:
        import RecalBoxDMD_tool as _t
        return list(_t.scan_wifi_networks_24ghz())
    except Exception:
        return []


def _local_ipv4() -> str:
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("10.255.255.255", 1))
        return s.getsockname()[0]
    except OSError:
        return ""
    finally:
        s.close()


def scan_recalbox_lan(log=None, max_hosts: int = 4) -> list:
    """Cherche les Recalbox du reseau local (/24 de ce PC) : SSH (22) + partage Windows (445) ouverts, puis page web (80) qui parle de Recalbox.
    Renvoie [(ip, confirmee)] : les confirmees d'abord. Aucune authentification, aucun envoi de donnees."""
    import socket
    from concurrent.futures import ThreadPoolExecutor
    me = _local_ipv4()
    if not me:
        return []
    base = me.rsplit(".", 1)[0]

    def open_(ip, port, to=0.5):
        try:
            with socket.create_connection((ip, port), timeout=to):
                return True
        except OSError:
            return False

    def probe(ip):
        if ip == me or not open_(ip, 445) or not open_(ip, 22):
            return None
        ok = False
        try:
            import urllib.request
            with urllib.request.urlopen(f"http://{ip}/", timeout=2) as r:
                ok = b"recalbox" in r.read(60000).lower()
        except Exception:
            pass
        return (ip, ok)

    with ThreadPoolExecutor(64) as ex:
        found = [x for x in ex.map(probe, [f"{base}.{i}" for i in range(1, 255)]) if x]
    found.sort(key=lambda x: (not x[1], tuple(int(p) for p in x[0].split("."))))
    if log:
        log(f"{len(found)} Recalbox possible(s) sur {base}.0/24")
    return found[:max_hosts]


def current_wifi_ssid() -> str:
    """SSID du Wi-Fi auquel ce PC est connecte (pre-remplissage ; jamais le mot de passe)."""
    try:
        r = subprocess.run(["netsh", "wlan", "show", "interfaces"], capture_output=True, timeout=10,
                           creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        txt = r.stdout.decode("cp850", "replace")
        m = re.search(r"^\s*SSID\s*:\s*(.+)$", txt, re.M)
        return m.group(1).strip() if m else ""
    except Exception:
        return ""


# ---------------------------------------------------------------------------------------------------------------- disposition de la carte (3 partitions)
# Carte du Pi preparee sur le PC :  [1] bootfs FAT32 512 Mo (image)  |  [3] media FAT32 « RECALBOXDMD » (<= 30 Gio, Windows ne formate pas du FAT32 plus gros)  |  [2] rootfs ext4
# (image, physiquement EN DERNIER). Les numeros de partition de l'image ne changent pas (root=PARTUUID=...-02, fstab) ; l'entree 3 (media) est ajoutee dans
# la table MBR, entre bootfs et rootfs. Au 1er demarrage l'initramfs de Raspberry Pi OS agrandit la racine (`parted resizepart 2 <fin du disque>` puis
# resize2fs) : il faut donc de la place libre APRES la racine, et rien apres elle.
FAT32_MAX_BYTES = 30 * 1024 ** 3   # au-dela, la partition media est formatee en exFAT (Windows ne formate pas de FAT32 plus gros)
MEDIA_CAP_BYTES = None            # pas de plafond : tout l'espace hors racine va au media
ROOT_MIN_BYTES = 5 * 1024 ** 3     # la racine doit pouvoir atteindre au moins cette taille apres agrandissement (l'OS en occupe ~2,5 Gio)
MEDIA_MIN_BYTES = 1024 ** 3
ALIGN_SECTORS = 2048               # 1 Mio
MEDIA_LABEL = "RBPIDMD"          # etiquette de la partition media du Pi (la carte DMD de l'ESP32 : RECALBOXDMD) ; <= 11 caracteres (FAT32)


def mbr_parse(sec0: bytes) -> list:
    out = []
    for i in range(4):
        e = sec0[446 + 16 * i:462 + 16 * i]
        out.append({"slot": i + 1, "raw": bytes(e), "type": e[4], "start": int.from_bytes(e[8:12], "little"), "size": int.from_bytes(e[12:16], "little")})
    return out


def _mbr_entry(ptype: int, start: int, size: int) -> bytes:
    return bytes([0, 0xFE, 0xFF, 0xFF, ptype, 0xFE, 0xFF, 0xFF]) + start.to_bytes(4, "little") + size.to_bytes(4, "little")


def plan_media_layout(total_sectors: int, e1: dict, e2: dict, media_cap=MEDIA_CAP_BYTES) -> dict:
    """Place bootfs (e1, inchangee), media, rootfs (e2) ; leve ValueError si la carte est trop petite."""
    up = lambda x: (x + ALIGN_SECTORS - 1) // ALIGN_SECTORS * ALIGN_SECTORS   # noqa: E731
    data_start = up(e1["start"] + e1["size"])
    room = total_sectors - data_start - ROOT_MIN_BYTES // SECTOR
    data_size = (room if media_cap is None else min(media_cap // SECTOR, room)) // ALIGN_SECTORS * ALIGN_SECTORS
    if data_size < MEDIA_MIN_BYTES // SECTOR:
        raise ValueError("carte trop petite : il faut au moins 8 Go pour le systeme et la partition media")
    root_start = data_start + data_size
    if root_start + e2["size"] > total_sectors:
        raise ValueError("carte trop petite pour le systeme")
    fs = "FAT32" if data_size * SECTOR <= FAT32_MAX_BYTES else "exFAT"   # FAT32 jusqu'a 30 Gio (Windows), exFAT au-dela
    return {"boot": (e1["start"], e1["size"]), "media": (data_start, data_size), "root": (root_start, e2["size"]), "root_orig_start": e2["start"], "fs": fs}


def build_mbr(sec0: bytes, e1: dict, e2: dict, plan: dict) -> bytes:
    m = bytearray(sec0)
    m[446:462] = e1["raw"]
    m[462:478] = _mbr_entry(e2["type"], plan["root"][0], plan["root"][1])
    m[478:494] = _mbr_entry(0x0C, plan["media"][0], plan["media"][1])
    m[494:510] = b"\0" * 16
    m[510:512] = b"\x55\xaa"
    return bytes(m)


def _read_exact(src, n: int) -> bytes:
    buf = b""
    while len(buf) < n:
        b = src.read(n - len(buf))
        if not b:
            break
        buf += b
    return buf


import contextlib


def existing_media(n: int):
    """Partition MEDIA (n°3) deja presente sur la carte, lue SANS droits administrateur : {'start','size' (secteurs),'label','fs'} ou None."""
    out = _ps(f"$p = Get-Partition -DiskNumber {n} -PartitionNumber 3 -ErrorAction SilentlyContinue; if (-not $p) {{ 'NONE'; return }}; "
              f"$v = $p | Get-Volume -ErrorAction SilentlyContinue; [pscustomobject]@{{Offset=[int64]$p.Offset; Size=[int64]$p.Size; Label=[string]$v.FileSystemLabel; FS=[string]$v.FileSystem}} | ConvertTo-Json -Compress").strip()
    if not out or out == "NONE":
        return None
    d = json.loads(out)
    return {"start": int(d["Offset"]) // SECTOR, "size": int(d["Size"]) // SECTOR, "label": d.get("Label") or "", "fs": d.get("FS") or ""}


@contextlib.contextmanager
def _locked_volumes(letters, log=None):
    """Verrouille et demonte les volumes Windows `letters` (lettres seules) pendant l'ecriture brute : Windows refuse d'ecrire dans les secteurs d'un volume monte
    (PermissionError). Les poignees restent ouvertes pendant tout le bloc ; a la fermeture le volume redevient accessible."""
    import ctypes
    from ctypes import wintypes
    k = ctypes.windll.kernel32
    k.CreateFileW.restype = ctypes.c_void_p
    k.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
    k.DeviceIoControl.argtypes = [ctypes.c_void_p, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
    k.CloseHandle.argtypes = [ctypes.c_void_p]
    invalid = ctypes.c_void_p(-1).value
    handles = []
    try:
        for L in letters:
            h = k.CreateFileW(rf"\\.\{L}:", 0xC0000000, 3, None, 3, 0, None)
            if h in (None, invalid):
                raise RuntimeError(f"impossible d'ouvrir le volume {L}: (erreur {ctypes.GetLastError()})")
            handles.append(h)
            ret = wintypes.DWORD(0)
            if not k.DeviceIoControl(h, 0x00090018, None, 0, None, 0, ctypes.byref(ret), None):     # FSCTL_LOCK_VOLUME
                raise RuntimeError(f"le volume {L}: est utilise (fermez l'Explorateur, l'antivirus ou toute fenetre ouverte sur la carte) puis reessayez")
            k.DeviceIoControl(h, 0x00090020, None, 0, None, 0, ctypes.byref(ret), None)              # FSCTL_DISMOUNT_VOLUME
            if log:
                log(f"volume {L}: verrouille et demonte pour l'ecriture")
        yield
    finally:
        for h in handles:
            k.CloseHandle(h)


class _RawWriter:
    """Ecriture sequentielle sur un disque brut, avec reprise : EBADF (poignee invalidee), EACCES ou EIO pendant l'ecriture => attente, reouverture
    du disque et reecriture du bloc fautif au meme offset (jusqu'a RETRIES fois). Constate le 2026-10-20 : « Bad file descriptor » en plein
    ecrit (le lecteur USB / Windows invalide la poignee), puis PermissionError a 62 % lors d'un essai precedent."""
    RETRIES = 6

    def __init__(self, target: str, log=None):
        self.target, self.log, self.f, self.pos = target, log, None, 0
        self._open()

    def _open(self):
        self.f = open(self.target, "r+b", buffering=0)
        self.f.seek(self.pos)

    def seek(self, off: int):
        self.pos = off
        self.f.seek(off)

    def write(self, b: bytes):
        for attempt in range(self.RETRIES + 1):
            try:
                self.f.seek(self.pos)
                self.f.write(b)
                self.pos += len(b)
                return
            except OSError as e:
                if attempt >= self.RETRIES or e.errno not in (errno.EBADF, errno.EACCES, errno.EIO, errno.EPERM):
                    raise
                if self.log:
                    self.log(f"ecriture interrompue a {self.pos} ({type(e).__name__} {e.errno} winerror={getattr(e, 'winerror', None)}) : nouvelle tentative {attempt + 1}/{self.RETRIES}")
                try:
                    self.f.close()
                except OSError:
                    pass
                time.sleep(2 + attempt * 2)
                for _ in range(15):      # le disque peut mettre quelques secondes a reapparaitre
                    try:
                        self._open()
                        break
                    except OSError:
                        time.sleep(2)
                else:
                    raise

    def close(self):
        try:
            self.f.close()
        except OSError:
            pass


def write_image_layout(img: Path, target: str, total_bytes: int, log, progress=None, media_cap=MEDIA_CAP_BYTES, keep_media=None) -> dict:
    """Ecrit l'image sur `target` (disque brut ou fichier pre-dimensionne) avec la disposition a 3 partitions, puis RELIT et verifie (SHA-256 des deux zones + MBR).
    progress(pct 0..100) pendant l'ecriture. Renvoie le plan (secteurs)."""
    opener = (lambda p: lzma.LZMAFile(open(p, "rb"))) if img.suffix.lower() == ".xz" else (lambda p: open(p, "rb"))
    csize = max(1, img.stat().st_size)
    src = opener(img)
    src_f = getattr(src, "_fp", None)
    sec0 = _read_exact(src, SECTOR)
    if len(sec0) != SECTOR or sec0[510:512] != b"\x55\xaa":
        raise RuntimeError("image invalide : pas de table de partitions MBR")
    ents = mbr_parse(sec0)
    e1, e2 = ents[0], ents[1]
    plan = plan_media_layout(total_bytes // SECTOR, e1, e2, media_cap)
    mbr = build_mbr(sec0, e1, e2, plan)
    if keep_media and (keep_media["start"] != plan["media"][0] or keep_media["size"] != plan["media"][1]):
        raise RuntimeError(f"la partition media existante (debut {keep_media['start']}, taille {keep_media['size']} secteurs) ne correspond pas a la disposition prevue "
                           f"(debut {plan['media'][0]}, taille {plan['media'][1]}) : conservation impossible, relancez sans l'option « conserver la partition media »")
    lenA = e2["start"] * SECTOR - SECTOR           # de la fin du MBR au debut de la racine d'origine (bootfs compris)
    root_off = plan["root"][0] * SECTOR
    log(f"disposition : bootfs {plan['boot'][1] * SECTOR / 1e6:.0f} Mo | media {plan['media'][1] * SECTOR / 1073741824:.1f} Gio ({plan['fs']}) | rootfs a partir de {root_off / 1073741824:.1f} Gio")
    hA, hB = hashlib.sha256(), hashlib.sha256()
    lenB = 0
    last = -1

    def pos_pct() -> int:
        p = src_f.tell() if src_f is not None else 0
        return min(70, int(p * 70 / csize))

    dst = _RawWriter(target, log)
    try:
        dst.seek(0)
        dst.write(mbr)
        left = lenA
        dst.seek(SECTOR)
        while left > 0:
            b = _read_exact(src, min(CHUNK, left))
            if not b:
                raise RuntimeError("image tronquee (zone bootfs)")
            hA.update(b)
            dst.write(b)
            left -= len(b)
        dst.seek(root_off)
        while True:
            b = src.read(CHUNK)
            if not b:
                break
            hB.update(b)
            lenB += len(b)
            if len(b) % SECTOR:
                b += b"\0" * (SECTOR - len(b) % SECTOR)
            dst.write(b)
            pc = pos_pct()
            if pc != last:
                last = pc
                if progress:
                    progress(pc)
    finally:
        dst.close()
    src.close()
    # relecture
    with open(target, "rb", buffering=0) as rd:
        if rd.read(SECTOR) != mbr:
            raise RuntimeError("verification ECHOUEE : table de partitions")
        for name, off, ln, h in (("bootfs", SECTOR, lenA, hA), ("rootfs", root_off, lenB, hB)):
            rd.seek(off)
            hh = hashlib.sha256()
            left = ln
            while left > 0:
                want = min(CHUNK, left)
                b = rd.read(want + (-want % SECTOR))
                if not b:
                    raise RuntimeError(f"lecture interrompue pendant la verification ({name})")
                hh.update(b[:want])
                left -= want
            if hh.hexdigest() != h.hexdigest():
                raise RuntimeError(f"verification ECHOUEE : {name} different de ce qui a ete ecrit (carte defectueuse ?)")
    log("verification OK (table de partitions, bootfs, rootfs)")
    return plan


# ---------------------------------------------------------------------------------------------------------------- ecriture (eleve)
def is_admin() -> bool:
    try:
        import ctypes
        return bool(ctypes.windll.shell32.IsUserAnAdmin())
    except Exception:
        return False


def write_job(job: dict) -> Path:
    """Ecrit la tache (contient le hash du mot de passe et le mot de passe Wi-Fi) dans le dossier durable de l'utilisateur ; le
    processus eleve la supprime des qu'il l'a lue."""
    p = jobs_dir() / f"job_{int(time.time())}_{os.getpid()}.json"
    p.write_text(json.dumps(job), encoding="utf-8")
    return p


def launch_elevated(job_path: Path) -> bool:
    """Relance le toolkit en administrateur (UAC) avec --pi-write-job <fichier>. False si l'utilisateur refuse."""
    import ctypes
    if getattr(sys, "frozen", False):
        exe, params = sys.executable, f'--pi-write-job "{job_path}"'
    else:
        exe = sys.executable
        params = f'"{Path(__file__).resolve().parent / "run_gui.py"}" --pi-write-job "{job_path}"'
    rc = ctypes.windll.shell32.ShellExecuteW(None, "runas", exe, params, None, 0)
    return rc > 32


def read_progress(job_path: Path, pos: int = 0):
    """(nouvelles lignes, nouvelle position, fini?, resultat). Le processus eleve ecrit <job>.log ; <job>.done contient « OK » ou « ERR: ... »."""
    lg = Path(str(job_path) + ".log")
    lines = []
    if lg.is_file():
        with open(lg, "rb") as f:
            f.seek(pos)
            data = f.read()
            pos += len(data)
        lines = [x for x in data.decode("utf-8", "replace").splitlines() if x]
    done = Path(str(job_path) + ".done")
    res = done.read_text(encoding="utf-8").strip() if done.is_file() else None
    return lines, pos, res is not None, res


class _Log:
    def __init__(self, path: Path):
        self.path = path

    def __call__(self, line: str):
        with open(self.path, "a", encoding="utf-8") as f:
            f.write(line.replace("\n", " ") + "\n")


def helper_main(job_path: str) -> int:
    """Point d'entree du processus ELEVE : ecrit l'image, puis les fichiers de la partition boot. Journal dans <job>.log, resultat dans <job>.done."""
    jp = Path(job_path)
    log = _Log(Path(str(jp) + ".log"))
    done = Path(str(jp) + ".done")
    try:
        job = json.loads(jp.read_text(encoding="utf-8"))
        try:
            jp.unlink()   # secrets : supprimes des qu'ils sont en memoire
        except OSError:
            pass
        _write_card(job, log)
        done.write_text("OK", encoding="utf-8")
        return 0
    except Exception as e:   # noqa: BLE001
        log(f"ERREUR: {type(e).__name__}: {e}")
        try:
            import traceback
            log("trace : " + " | ".join(l.strip() for l in traceback.format_exc().splitlines()[-8:]))
        except Exception:
            pass
        done.write_text(f"ERR: {type(e).__name__}: {e}", encoding="utf-8")
        return 1


def _write_card(job: dict, log) -> None:
    n = int(job["disk_number"])
    dry = bool(job.get("dry_run"))
    if not is_admin() and not dry:
        raise RuntimeError("droits administrateur requis")
    # --- garde-fous : le disque doit etre exactement celui que l'utilisateur a confirme (le numero peut avoir change entre-temps)
    out = _ps(f"Get-Disk -Number {n} | Select-Object Number,FriendlyName,Size,@{{n='BusType';e={{$_.BusType.ToString()}}}},IsBoot,IsSystem | ConvertTo-Json -Compress")
    d = json.loads(out)
    log(f"cible : disque {d['Number']} {d['FriendlyName']} {int(d['Size']):,} octets ({d['BusType']})".replace(",", " "))
    if n == 0 or d.get("IsBoot") or d.get("IsSystem"):
        raise RuntimeError("refus : disque systeme")
    if str(d.get("BusType") or "").upper() not in ALLOWED_BUS:
        raise RuntimeError(f"refus : le disque n'est pas un support amovible ({d.get('BusType')})")
    if str(d["FriendlyName"]).strip() != job["expected_model"] or int(d["Size"]) != int(job["expected_size"]):
        raise RuntimeError("refus : le disque n'est plus celui qui a ete confirme (modele ou taille differents)")
    if not (MIN_DISK_BYTES <= int(d["Size"]) <= MAX_DISK_BYTES):
        raise RuntimeError("refus : taille de disque inattendue")
    img = Path(job["image_path"])
    if not img.is_file():
        raise RuntimeError(f"image introuvable : {img}")
    if dry:
        log("DRY-RUN : controles OK, rien n'est ecrit")
        return
    log("PCT 0")
    keep = bool(job.get("keep_media"))
    media_keep = None
    boot_letters = []
    if keep:
        em = existing_media(n)
        if not em or em["fs"] not in ("FAT32", "exFAT") or em["label"].upper() != MEDIA_LABEL:
            raise RuntimeError(f"conservation impossible : pas de partition media « {MEDIA_LABEL} » valide sur cette carte (trouve : {em or 'aucune'})")
        media_keep = {"start": em["start"], "size": em["size"]}
        log(f"partition media CONSERVEE : {em['label']} {em['fs']} {em['size'] * SECTOR / 1073741824:.1f} Gio (seuls le systeme et la partition de demarrage sont reecrits)")
        s = _ps(f"(Get-Partition -DiskNumber {n} -PartitionNumber 1 -ErrorAction SilentlyContinue).DriveLetter").strip()
        boot_letters = [s.upper()] if re.fullmatch(r"[A-Za-z]", s or "") else []
    else:
        log("effacement de la table de partitions")
        _ps(f"Clear-Disk -Number {n} -RemoveData -RemoveOEM -Confirm:$false", timeout=180)
        time.sleep(2)
    log("ecriture de l'image (disposition a 3 partitions)")
    last_pct = [-1]

    def prog(p):
        if p != last_pct[0]:
            last_pct[0] = p
            log(f"PCT {p}")
    with _locked_volumes(boot_letters, log):
        plan = write_image_layout(img, rf"\\.\PhysicalDrive{n}", int(d["Size"]), log, prog, keep_media=media_keep)
    log("PCT 96")
    # --- partitions : bootfs = n°1 (FAT32 de l'image), media = n°3 (a formater : Windows ne la connait pas encore)
    _ps(f"Update-Disk -Number {n}")
    time.sleep(3)

    def letter_of(num: int) -> "str | None":
        s = _ps(f"$p = Get-Partition -DiskNumber {n} -PartitionNumber {num}; if (-not $p.DriveLetter) {{ Add-PartitionAccessPath -DiskNumber {n} -PartitionNumber {num} -AssignDriveLetter; $p = Get-Partition -DiskNumber {n} -PartitionNumber {num} }}; $p.DriveLetter").strip()
        return (s.upper() + ":\\") if re.fullmatch(r"[A-Za-z]", s or "") else None

    boot = None
    for _ in range(10):
        try:
            boot = letter_of(1)
        except RuntimeError:
            boot = None
        if boot:
            break
        time.sleep(2)
    if not boot or not (Path(boot) / "cmdline.txt").is_file():
        raise RuntimeError("partition boot introuvable apres l'ecriture")
    if not keep:
        log(f"formatage de la partition media ({plan['fs']}, {MEDIA_LABEL})")
        _ps(f"Get-Partition -DiskNumber {n} -PartitionNumber 3 | Format-Volume -FileSystem {plan['fs']} -NewFileSystemLabel {MEDIA_LABEL} -Confirm:$false | Out-Null", timeout=900)
        time.sleep(2)
    media = None
    for _ in range(10):
        try:
            media = letter_of(3)
        except RuntimeError:
            media = None
        if media:
            break
        time.sleep(2)
    if not media:
        raise RuntimeError("partition media introuvable apres le formatage")
    log(f"MEDIA {media[0]}:")
    if job.get("config_ini") and not keep:      # conservation : le config.ini de la carte (reglages deja faits) n'est pas touche
        (Path(media) / "config.ini").write_text(job["config_ini"].replace("\r\n", "\n"), encoding="utf-8", newline="\n")
        log("config.ini (premier demarrage) ecrit sur la partition media")
    # --- fichiers de premier demarrage sur la partition boot
    (Path(boot) / "custom.toml").write_text(job["custom_toml"].replace("\r\n", "\n"), encoding="utf-8", newline="\n")
    (Path(boot) / "ssh").write_text("")
    log("custom.toml et ssh ecrits")
    pl = job.get("payload_dir")
    if pl:
        dst = Path(boot) / "recalboxdmd"
        dst.mkdir(exist_ok=True)
        for rel, full in iter_payload_files(Path(pl)):
            t = dst / rel
            t.parent.mkdir(parents=True, exist_ok=True)
            data = full.read_bytes()
            if rel.endswith((".sh", ".service", ".seed", ".ini")):
                data = data.replace(b"\r\n", b"\n")
            t.write_bytes(data)
        if job.get("config_ini"):
            (dst / "config.ini.seed").write_text(job["config_ini"], encoding="utf-8", newline="\n")
        log("charge utile deposee sur la carte (recalboxdmd/)")
        cmd = Path(boot) / "cmdline.txt"
        line = cmd.read_text(encoding="utf-8").strip()
        line = re.sub(r"\s+systemd\.(run|run_success_action|run_failure_action|unit)=\S+", "", line)
        cmd.write_text(line + " systemd.run=/boot/firmware/recalboxdmd/firstrun.sh systemd.run_success_action=reboot systemd.run_failure_action=reboot systemd.unit=kernel-command-line.target\n", encoding="utf-8", newline="\n")
        log("crochet de premier demarrage ajoute a cmdline.txt (installation du DMD hors ligne)")
    log("PCT 100")
    log("TERMINE")


# ---------------------------------------------------------------------------------------------------------------- Pi : SSH / SFTP
def _paramiko():
    import paramiko
    return paramiko


def pi_connect(host: str, user: str, password: str, timeout: float = 8.0):
    pm = _paramiko()
    c = pm.SSHClient()
    c.set_missing_host_key_policy(pm.AutoAddPolicy())
    c.connect(host, username=user, password=password, timeout=timeout, banner_timeout=timeout, auth_timeout=timeout,
              look_for_keys=False, allow_agent=False)
    return c


def local_prefix() -> str:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
        s.close()
        return ip.rsplit(".", 1)[0]
    except OSError:
        return ""


def _port_open(ip: str, port: int, timeout: float) -> bool:
    try:
        with socket.create_connection((ip, port), timeout=timeout):
            return True
    except OSError:
        return False


def default_hostname(saved: str = "") -> str:
    """Nom a proposer : celui memorise, sauf l'ancien nom par defaut (remplace par le nouveau)."""
    return DEFAULT_HOSTNAME if not saved or saved.lower() in OLD_HOSTNAMES else saved


def find_pi(host: str, user: str, password: str, log=None):
    """(client, adresse) : essaie `host` (par defaut dmdpi.local), puis balaie le /24 du PC (port 22) et tente l'identification.
    Leve RuntimeError si le Pi n'est pas trouve ; PermissionError si le mot de passe est refuse."""
    from concurrent.futures import ThreadPoolExecutor
    pm = _paramiko()
    tried = []
    last = None
    for h in [host or f"{DEFAULT_HOSTNAME}.local"]:
        try:
            tried.append(h)
            return pi_connect(h, user, password), h
        except pm.AuthenticationException:
            raise PermissionError("mot de passe refuse")
        except Exception as e:   # noqa: BLE001
            last = e
    pre = local_prefix()
    if pre:
        if log:
            log(f"{host or DEFAULT_HOSTNAME} injoignable : balayage de {pre}.0/24 (port 22)…")
        hosts = [f"{pre}.{i}" for i in range(1, 255)]
        with ThreadPoolExecutor(max_workers=64) as ex:
            opened = [h for h, ok in zip(hosts, ex.map(lambda x: _port_open(x, 22, 0.4), hosts)) if ok]
        for h in opened:
            try:
                c = pi_connect(h, user, password, 6.0)
                _, o, _ = c.exec_command("test -d /boot/firmware -o -d /boot && uname -m", timeout=6)
                if o.read().decode().strip().startswith("arm"):
                    return c, h
                c.close()
            except pm.AuthenticationException:
                continue
            except Exception:   # noqa: BLE001
                continue
    raise RuntimeError(f"Pi introuvable ({last})")


def _sftp_mkdirs(sftp, path: str) -> None:
    parts = [p for p in path.split("/") if p]
    cur = ""
    for p in parts:
        cur += "/" + p
        try:
            sftp.stat(cur)
        except IOError:
            sftp.mkdir(cur)


def run_remote(c, cmd: str, log=None, sudo_pw: str = "", timeout: float = 1800.0) -> tuple:
    """Execute une commande (avec sudo -S si sudo_pw), renvoie (code, sortie) ; chaque ligne est passee a log()."""
    if sudo_pw:
        cmd = f"sudo -S -p '' bash -c {json.dumps(cmd)}"
    chan = c.get_transport().open_session()
    chan.get_pty()   # sans pty, sudo -S melange mal l'invite ; stdout et stderr arrivent ensemble
    chan.exec_command(cmd)
    if sudo_pw:
        chan.send(sudo_pw + "\n")
    out = ""
    buf = ""
    t0 = time.time()
    while True:
        if chan.recv_ready():
            d = chan.recv(4096).decode("utf-8", "replace")
            out += d
            buf += d
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.rstrip("\r")
                if line and log and line.strip() != sudo_pw:
                    log(line)
        elif chan.exit_status_ready():
            break
        else:
            time.sleep(0.1)
        if time.time() - t0 > timeout:
            chan.close()
            return -1, out
    if buf.strip() and log:
        log(buf.strip())
    return chan.recv_exit_status(), out


def upload_payload(c, payload: Path, stage: str, config_seed: str = "", log=None) -> None:
    sftp = c.open_sftp()
    try:
        _sftp_mkdirs(sftp, stage)
        for rel, full in iter_payload_files(payload):
            r = f"{stage}/{rel}"
            _sftp_mkdirs(sftp, r.rsplit("/", 1)[0])
            data = full.read_bytes()
            if rel.endswith((".sh", ".service", ".seed", ".ini")):
                data = data.replace(b"\r\n", b"\n")
            with sftp.open(r, "wb") as f:
                f.write(data)
        if config_seed:
            with sftp.open(f"{stage}/config.ini.seed", "wb") as f:
                f.write(config_seed.replace("\r\n", "\n").encode("utf-8"))
        if log:
            log("charge utile envoyee")
    finally:
        sftp.close()


SKIP_DIRS = {"system volume information", "$recycle.bin", ".trash-1000", ".spotlight-v100"}


SYNC_DIRS = ("systems", "gifs", "playlists", "widgets")                                   # dossiers du contenu du DMD copies en entier
SYNC_ROOT_FILES = ("config.ini", "games_cache.bin", "systems_cache.dat", "systems_cache.dat.bucket")   # fichiers de la racine (config.ini : jamais ecrase)


# Elements attendus dans un dossier de contenu = ce que le MODE 1 ecrit dans le dossier de travail pour preparer une carte SD complete (voir DECISIONS.md) :
#   systems/<systeme>/...      images des jeux converties (raw565 / raw565pack / meta), un dossier par systeme (hors _defaults)
#   systems/_defaults/         images par defaut (default.raw565, logos de systemes / genres)
#   systems/_defaults/_themes/ logos des themes Recalbox (Modes 1 / 12 / 13)
#   systems_cache.dat          index des systemes (Mode 7)          games_cache.bin   cache des jeux (Mode 6)
#   gifs/                      pack de GIF (Mode 11)                playlists/        playlists (ALL.txt, playlist du pack)
#   widgets/quotes/            themes de citations (Mode 15)        config.ini        reglages (langue, IP Recalbox, playlist)
# (recalbox_userscripts/, images_manquantes.txt, logs/ ne font pas partie d'une carte : jamais attendus ni copies.)
EXPECTED_CONTENT = (   # (libelle, chemin, facultatif)
    ("systems/<systeme>/", "systems_games", False),
    ("systems/_defaults/", "systems/_defaults", False),
    ("systems/_defaults/_themes/", "systems/_defaults/_themes", False),
    ("systems_cache.dat", "systems_cache.dat", False),
    ("games_cache.bin", "games_cache.bin", False),
    ("playlists/", "playlists", False),
    ("config.ini", "config.ini", False),
    ("gifs/", "gifs", True),             # pack de GIF : telechargement a la demande (Mode 11)
    ("widgets/quotes/", "widgets/quotes", True),   # themes de citations : Mode 15, directement sur la carte
)


def _result(flags: dict) -> dict:
    """flags : {libelle: present?} -> {found, missing (obligatoires absents), optional_missing, flags}."""
    opt = {lab: o for lab, _rel, o in EXPECTED_CONTENT}
    return {"found": [l for l, ok in flags.items() if ok],
            "missing": [l for l, ok in flags.items() if not ok and not opt[l]],
            "optional_missing": [l for l, ok in flags.items() if not ok and opt[l]],
            "flags": flags}


def check_content(root: Path) -> dict:
    """Quels elements attendus (EXPECTED_CONTENT) contient ce dossier local. Un dossier compte s'il contient au moins un fichier."""
    def has_files(p: Path) -> bool:
        if p.is_file():
            return True
        if not p.is_dir():
            return False
        for _r, _d, f in os.walk(p):
            if f:
                return True
        return False

    flags = {}
    for label, rel, _opt in EXPECTED_CONTENT:
        if rel == "systems_games":
            ok = False
            sysdir = root / "systems"
            if sysdir.is_dir():
                for d in sysdir.iterdir():
                    if d.is_dir() and d.name != "_defaults" and has_files(d):
                        ok = True
                        break
        else:
            ok = has_files(root / rel)
        flags[label] = ok
    return _result(flags)


def remote_check(c, remote_root: str) -> dict:
    """Meme controle sur le dossier de contenu du Pi (une seule commande SSH) : carte des dossiers presents / absents."""
    cmds = []
    for i, (label, rel, _opt) in enumerate(EXPECTED_CONTENT):
        if rel == "systems_games":
            t = f'[ -n "$(find {remote_root}/systems -mindepth 2 -type f -not -path "{remote_root}/systems/_defaults/*" -print -quit 2>/dev/null)" ]'
        elif "." in rel.rsplit("/", 1)[-1]:
            t = f"[ -f {remote_root}/{rel} ]"
        else:
            t = f'[ -n "$(find {remote_root}/{rel} -type f -print -quit 2>/dev/null)" ]'
        cmds.append(f"{t} && echo R{i}=1 || echo R{i}=0")
    _code, out = run_remote(c, "; ".join(cmds), None, timeout=60)
    flags = {}
    for i, (label, _rel, _opt) in enumerate(EXPECTED_CONTENT):
        flags[label] = f"R{i}=1" in out
    return _result(flags)


def content_plan(root: Path) -> list:
    """Ce que le Mode 17 copie : [(chemin relatif en '/', chemin local, octets)]. UNIQUEMENT le contenu utile au DMD : les dossiers systems / gifs /
    playlists / widgets (sous-dossiers compris) et les fichiers de la racine config.ini, games_cache.bin, systems_cache.dat(.bucket). Le reste de
    la carte (logs, stats.dat, native_last.raw565, sauvegardes .back, FOUND.*, System Volume Information...) n'est pas copie."""
    out = []
    for name in SYNC_ROOT_FILES:
        f = root / name
        if f.is_file():
            out.append((name, f, f.stat().st_size))
    for d in SYNC_DIRS:
        base = root / d
        if not base.is_dir():
            continue
        for r, dirs, fnames in os.walk(base):
            dirs[:] = [x for x in dirs if x.lower() not in SKIP_DIRS]
            for fn in fnames:
                full = Path(r) / fn
                try:
                    out.append((full.relative_to(root).as_posix(), full, full.stat().st_size))
                except OSError:
                    pass
    return out


def sync_content(c, local_root: Path, remote_root: str, log=None, progress=None, cancelled=lambda: False,
                 replace: bool = False, overwrite_config: bool = False) -> dict:
    """Copie du contenu utile (voir content_plan) vers le Pi, journal detaille.
    replace=False : un fichier DEJA present sur le Pi n'est jamais touche (seuls les fichiers manquants sont ajoutes) ;
    replace=True  : un fichier present est remplace s'il differe (taille ou date). config.ini n'est ecrase que si overwrite_config.
    Chaque fichier est ecrit sous un nom .part puis renomme : une coupure ne laisse jamais un fichier tronque a la place d'un bon.
    Renvoie {sent, kept, skipped, bytes, errors, seconds, total}."""
    log = log or (lambda m: None)
    plan = content_plan(local_root)
    total_bytes = sum(s for _r, _f, s in plan)
    stats = {"sent": 0, "kept": 0, "skipped": 0, "bytes": 0, "errors": 0, "seconds": 0.0, "total": len(plan)}
    per_dir = {}
    for rel, _f, s in plan:
        k = rel.split("/", 1)[0] if "/" in rel else "(racine)"
        a = per_dir.setdefault(k, [0, 0])
        a[0] += 1
        a[1] += s
    log(f"Contenu : {local_root} -> {remote_root}")
    log(f"A examiner : {len(plan)} fichier(s), {total_bytes / 1e9:.2f} Go")
    for k, (n, s) in sorted(per_dir.items()):
        log(f"  {k} : {n} fichier(s), {s / 1e6:.1f} Mo")
    log("Regle : " + ("fichiers deja presents REMPLACES s'ils different" if replace else "fichiers deja presents CONSERVES (seuls les manquants sont ajoutes)")
        + " ; config.ini " + ("ECRASE" if overwrite_config else "conserve s'il existe"))
    if not plan:
        log("Rien a copier : aucun fichier dans systems / gifs / playlists / widgets (ni cache) dans ce dossier.")
        return stats
    t0 = time.time()
    sftp = c.open_sftp()
    try:
        made = set()
        last_log = t0
        for i, (rel, full, size) in enumerate(plan):
            if cancelled():
                log("Copie interrompue.")
                break
            r = f"{remote_root}/{rel}"
            try:
                st = full.stat()
                try:
                    rs = sftp.stat(r)
                    exists = True
                except IOError:
                    rs, exists = None, False
                if exists and rel.lower() == "config.ini" and not overwrite_config:
                    stats["kept"] += 1
                elif exists and not replace and rel.lower() != "config.ini":
                    stats["kept"] += 1
                elif exists and rs.st_size == st.st_size and abs(int(rs.st_mtime) - int(st.st_mtime)) <= 2:
                    stats["skipped"] += 1   # identique (taille et date)
                else:
                    d = r.rsplit("/", 1)[0]
                    if d not in made:
                        _sftp_mkdirs(sftp, d)
                        made.add(d)
                    part = r + ".part"
                    sftp.put(str(full), part)
                    try:
                        sftp.utime(part, (int(st.st_atime), int(st.st_mtime)))
                    except IOError:
                        pass
                    sftp.posix_rename(part, r)
                    stats["sent"] += 1
                    stats["bytes"] += st.st_size
            except Exception as e:   # noqa: BLE001
                stats["errors"] += 1
                if stats["errors"] <= 20:
                    log(f"ERREUR sur {rel} : {e}")
            now = time.time()
            if progress and (i % 25 == 0 or i == len(plan) - 1):
                progress(i + 1, len(plan))
            if now - last_log >= 10:   # une ligne de suivi toutes les 10 s
                last_log = now
                sp = stats["bytes"] / max(now - t0, 0.1) / 1e6
                log(f"  {i + 1}/{len(plan)} examines, {stats['sent']} envoyes ({stats['bytes'] / 1e6:.0f} Mo, {sp:.1f} Mo/s), {stats['kept'] + stats['skipped']} deja presents, {stats['errors']} erreur(s)")
    finally:
        sftp.close()
    stats["seconds"] = time.time() - t0
    log(f"Termine en {stats['seconds']:.0f} s : {stats['sent']} envoye(s) ({stats['bytes'] / 1e6:.0f} Mo), {stats['kept']} conserve(s), {stats['skipped']} identique(s), {stats['errors']} erreur(s)")
    return stats


def wait_pi_back(host: str, user: str, password: str, seconds: float = 240.0, log=None):
    """Attend qu'un Pi redemarre ; renvoie un client SSH ou None."""
    t0 = time.time()
    time.sleep(8)
    while time.time() - t0 < seconds:
        try:
            return pi_connect(host, user, password, 5.0)
        except PermissionError:
            raise
        except Exception:   # noqa: BLE001
            time.sleep(4)
    return None


def ascii_fold(s: str) -> str:
    """Police 5x7 du DMD : pas d'accents (meme repli que la page web, stripAccents)."""
    import unicodedata
    return unicodedata.normalize("NFKD", s).encode("ascii", "replace").decode("ascii")


def dmd_say(host: str, msg: str, title: str = "", color: str = "1", timeout: float = 3.0) -> bool:
    """Affiche un message sur le DMD du Pi (POST /dmd-pause, comme la page web) : sans effet, sans erreur, si le demon ne tourne pas
    encore. title = ligne 1 (ignore par un demon plus ancien, qui garde « WEB DMD CONFIG »), msg = ligne 2, color 1 vert / 2 rouge."""
    import urllib.parse
    try:
        body = {"msg": ascii_fold(msg)[:60], "color": color}
        if title:
            body["title"] = ascii_fold(title)[:20]
        req = urllib.request.Request(f"http://{host}/dmd-pause", data=urllib.parse.urlencode(body).encode(), method="POST", headers=UA)
        with urllib.request.urlopen(req, timeout=timeout) as r:
            r.read(200)
        return True
    except Exception:
        return False


def dmd_resume(host: str, timeout: float = 3.0) -> bool:
    """Rend l'ecran au DMD (POST /dmd-resume) ; en premier demarrage le demon reste sur l'ecran de configuration."""
    try:
        req = urllib.request.Request(f"http://{host}/dmd-resume", data=b"x=1", method="POST", headers=UA)
        with urllib.request.urlopen(req, timeout=timeout) as r:
            r.read(200)
        return True
    except Exception:
        return False


def folder_stats(root: Path, limit_files: int = 400000) -> tuple:
    """(nombre de fichiers, octets) d'un dossier de contenu ; s'arrete a limit_files."""
    n = size = 0
    for r, _d, files in os.walk(root):
        for f in files:
            n += 1
            try:
                size += os.path.getsize(os.path.join(r, f))
            except OSError:
                pass
            if n >= limit_files:
                return n, size
    return n, size


def pi_web_check(host: str, timeout: float = 6.0) -> "dict | None":
    """GET /load du demon : {ok, first_boot} si le DMD du Pi repond."""
    try:
        d = json.loads(_http_text(f"http://{host}/lang", timeout))
        return {"ok": "language" in d, "first_boot": str(d.get("first_boot", ""))}
    except Exception:
        return None
