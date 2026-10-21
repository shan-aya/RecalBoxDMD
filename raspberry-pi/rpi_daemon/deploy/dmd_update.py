#!/usr/bin/env python3
"""dmd_update.py -- met a jour le demon RecalBox DMD depuis la derniere release GitHub « pi-vM.mm » (bouton « Mettre a jour » de la page web).

Lance en root par le demon, DETACHE du service (systemd-run) : le service est redemarre pendant la mise a jour. Ce script :
  1. ecrit « en-cours » dans <prefix>/ota_last (la page web le lit dans /load et attend le resultat) ;
  2. cherche la release pi-v... la plus recente (release publiee, pas un brouillon) et la compare a <prefix>/PI_VERSION ;
  3. telecharge RecalBoxDMD_pi_payload.zip, verifie son SHA-256 (fichier .sha256 de la release), l'extrait sans jamais sortir du dossier de travail ;
  4. sauvegarde le demon actuel (<prefix>/backup/), lance l'install.sh de la charge utile (qui redemarre le service et verifie qu'il reste actif) ;
  5. en cas d'echec, REMET l'ancienne version et relance le service. Resultat final dans ota_last : « ok:<version> », « ok:deja <version> » ou « echec:<raison> ».
config.ini et le contenu de la carte ne sont jamais touches. Variables de test : DMD_UPDATE_API (URL de la liste des releases), DMD_UPDATE_PREFIX.
"""
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

PREFIX = os.environ.get("DMD_UPDATE_PREFIX", "/opt/recalboxdmd")
REPO = "shan-aya/RecalBoxDMD"
API = os.environ.get("DMD_UPDATE_API", f"https://api.github.com/repos/{REPO}/releases?per_page=30")
ASSET = "RecalBoxDMD_pi_payload.zip"
STATE = os.path.join(PREFIX, "ota_last")
UA = {"User-Agent": "recalboxdmd-updater"}
TEST = "DMD_UPDATE_PREFIX" in os.environ     # essai hors du Pi : ni systemctl ni apt


def state(text: str) -> None:
    try:
        with open(STATE, "w", encoding="utf-8") as f:
            f.write(text + "\n")
    except OSError:
        pass


def vtuple(s: str):
    m = re.fullmatch(r"(\d+)\.(\d+)", (s or "").strip())
    return (int(m.group(1)), int(m.group(2))) if m else (0, 0)


def read(path: str, default: str = "") -> str:
    try:
        with open(path, encoding="utf-8") as f:
            return f.read().strip()
    except OSError:
        return default


def restart_service() -> None:
    if not TEST:
        # reset-failed : un service tombe en « failed » apres des demarrages rates en rafale (limite de demarrages de systemd) et refuserait le relancement
        subprocess.run(["systemctl", "reset-failed", "recalboxdmd"], timeout=30)
        subprocess.run(["systemctl", "restart", "recalboxdmd"], timeout=60)


def latest_release():
    req = urllib.request.Request(API, headers={**UA, "Accept": "application/vnd.github+json"})
    with urllib.request.urlopen(req, timeout=20) as r:
        rels = json.load(r)
    best = None
    for rel in rels:
        m = re.fullmatch(r"pi-v(\d+\.\d+)", str(rel.get("tag_name", "")))
        if not m or rel.get("draft") or rel.get("prerelease"):
            continue
        assets = {a.get("name"): a.get("browser_download_url") for a in rel.get("assets", [])}
        if ASSET not in assets or ASSET + ".sha256" not in assets:
            continue
        if best is None or vtuple(m.group(1)) > vtuple(best[0]):
            best = (m.group(1), assets[ASSET], assets[ASSET + ".sha256"])
    return best


def download(url: str, dest: str) -> str:
    h = hashlib.sha256()
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=60) as r, open(dest, "wb") as f:
        while True:
            chunk = r.read(1 << 20)
            if not chunk:
                break
            h.update(chunk)
            f.write(chunk)
    return h.hexdigest()


def extract(zpath: str, dest: str) -> None:
    root = os.path.realpath(dest)
    with zipfile.ZipFile(zpath) as z:
        for name in z.namelist():
            tgt = os.path.realpath(os.path.join(dest, name))
            if tgt != root and not tgt.startswith(root + os.sep):
                raise RuntimeError("archive : chemin suspect")
        z.extractall(dest)


def backup() -> str:
    b = os.path.join(PREFIX, "backup")
    if os.path.isdir(b):
        shutil.rmtree(b)
    os.makedirs(b)
    for name in ("bin", "web", "lib"):
        src = os.path.join(PREFIX, name)
        if os.path.isdir(src):
            shutil.copytree(src, os.path.join(b, name))
    for name in ("PI_VERSION",):
        if os.path.isfile(os.path.join(PREFIX, name)):
            shutil.copy2(os.path.join(PREFIX, name), os.path.join(b, name))
    return b


def restore(b: str) -> None:
    for name in ("bin", "web", "lib"):
        src = os.path.join(b, name)
        if os.path.isdir(src):
            dst = os.path.join(PREFIX, name)
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
    if os.path.isfile(os.path.join(b, "PI_VERSION")):
        shutil.copy2(os.path.join(b, "PI_VERSION"), os.path.join(PREFIX, "PI_VERSION"))


def sd_dir() -> str:
    """Dossier du contenu : DMD_SD de /etc/default/recalboxdmd (carte preparee par le toolkit : partition media), sinon <prefix>/sd."""
    env = os.environ.get("DMD_UPDATE_ENVFILE", "/etc/default/recalboxdmd")
    for line in read(env).splitlines():
        m = re.match(r"\s*DMD_SD=(.+)", line)
        if m:
            return m.group(1).strip().strip('"')
    return os.path.join(PREFIX, "sd")


def main() -> int:
    state("en-cours")
    cur = read(os.path.join(PREFIX, "PI_VERSION"), "0.0")
    work = tempfile.mkdtemp(prefix="recalboxdmd-update-", dir=os.environ.get("DMD_UPDATE_TMP", "/var/tmp"))
    restarted = False
    try:
        try:
            rel = latest_release()
        except Exception as e:   # noqa: BLE001
            state(f"echec:GitHub injoignable ({type(e).__name__})")
            return 1
        if rel is None:
            state("echec:aucune version publiee")
            return 1
        ver, url_zip, url_sha = rel
        if vtuple(ver) <= vtuple(cur):
            state(f"ok:deja {cur}")
            return 0
        try:
            with urllib.request.urlopen(urllib.request.Request(url_sha, headers=UA), timeout=20) as r:
                want = r.read().decode("ascii", "replace").split()[0].strip().lower()
            zpath = os.path.join(work, ASSET)
            got = download(url_zip, zpath)
        except Exception as e:   # noqa: BLE001
            state(f"echec:telechargement impossible ({type(e).__name__})")
            return 1
        if got != want:
            state("echec:empreinte SHA-256 invalide, telechargement rejete")
            return 1
        stage = os.path.join(work, "payload")
        try:
            os.makedirs(stage)
            extract(zpath, stage)
        except Exception as e:   # noqa: BLE001
            state(f"echec:archive illisible ({type(e).__name__})")
            return 1
        for need in ("install.sh", "bin/dmd_daemon", "recalboxdmd.service", "PI_VERSION"):
            if not os.path.isfile(os.path.join(stage, need)):
                state(f"echec:charge utile incomplete ({need})")
                return 1
        if read(os.path.join(stage, "PI_VERSION")) != ver:
            state("echec:version de la charge utile incoherente")
            return 1
        b = backup()
        if TEST:
            cmd = ["bash", os.path.join(stage, "install.sh"), "--offline", "--sd", sd_dir()]
        else:
            cmd = ["bash", os.path.join(stage, "install.sh"), "--sd", sd_dir()]
        res = subprocess.run(cmd, cwd=stage, capture_output=True, text=True, timeout=900)
        restarted = True
        if res.returncode != 0 or read(os.path.join(PREFIX, "PI_VERSION")) != ver:
            tail = (res.stderr or res.stdout or "").strip().splitlines()[-1:] or ["?"]
            restore(b)
            restart_service()
            state(f"echec:installation refusee, version precedente remise ({tail[0][:80]})")
            return 1
        state(f"ok:{ver}")
        return 0
    except Exception as e:   # noqa: BLE001
        state(f"echec:{type(e).__name__}")
        return 1
    finally:
        shutil.rmtree(work, ignore_errors=True)
        if not restarted:
            restart_service()   # l'ecran « Mise a jour... » est efface par le relancement du service


if __name__ == "__main__":
    sys.exit(main())
