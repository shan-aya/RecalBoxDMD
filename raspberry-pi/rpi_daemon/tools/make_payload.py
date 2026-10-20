"""Assemble la charge utile du Pi (consommee par les Modes 16 / 17 du toolkit, voir tools/RecalBoxDMD_pi.py) :

    <sortie>/bin/dmd_daemon     binaire ARM compile sur le Pi (recupere par SFTP, ou --binary <fichier>)
    <sortie>/web/               pages de la configuration (rpi_daemon/web)
    <sortie>/install.sh, recalboxdmd.service, config.ini.seed, README.md (rpi_daemon/deploy)
    <sortie>/LICENSE, LICENSE.md   GPL-2.0-or-later (le binaire lie rpi-rgb-led-matrix)
    <sortie>/VERSION            « <date> <commit git> »

Usage :  python make_payload.py --out F:\\RPI_PORT_WORK\\payload --host dmdpi.local --user pi --password-env PI_PW
         python make_payload.py --out ... --binary dmd_daemon        (binaire deja recupere)
Le mot de passe n'est jamais passe en parametre : variable d'environnement. Pour une publication, le binaire doit venir d'un
build propre (ce script ne compile rien)."""
import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

here = Path(__file__).resolve().parent
rpi = here.parent
ap = argparse.ArgumentParser()
ap.add_argument("--out", required=True)
ap.add_argument("--binary")
ap.add_argument("--reuse", help="charge utile precedente dont on reprend bin/ et lib/ (quand le Pi de build n'est pas joignable)")
ap.add_argument("--host", default="dmdpi.local")
ap.add_argument("--user", default="pi")
ap.add_argument("--password-env", default="PI_PW")
ap.add_argument("--remote-binary", default="/home/pi/dmd/rpi_daemon/build/dmd_daemon")
a = ap.parse_args()

out = Path(a.out)
if out.exists():
    shutil.rmtree(out)
(out / "bin").mkdir(parents=True)
if a.reuse:
    reuse = Path(a.reuse)
    shutil.copy2(reuse / "bin" / "dmd_daemon", out / "bin" / "dmd_daemon")
    shutil.copytree(reuse / "lib", out / "lib")
elif a.binary:
    shutil.copy2(a.binary, out / "bin" / "dmd_daemon")
else:
    import paramiko
    pw = os.environ.get(a.password_env, "")
    if not pw:
        sys.exit(f"variable d'environnement {a.password_env} vide")
    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(a.host, username=a.user, password=pw, timeout=15, look_for_keys=False, allow_agent=False)
    s = c.open_sftp()
    s.get(a.remote_binary, str(out / "bin" / "dmd_daemon"))
    # bibliotheques que le demon utilise et que Raspberry Pi OS neuf n'a pas (livrees dans lib/, LD_LIBRARY_PATH dans le service) : premier demarrage HORS LIGNE
    (out / "lib").mkdir()
    for src_name, dst_name in (("libgif.so.7.2.0", "libgif.so.7"), ("libcpp-httplib.so.0.11.4", "libcpp-httplib.so.0.11")):
        s.get("/usr/lib/arm-linux-gnueabihf/" + src_name, str(out / "lib" / dst_name))
    s.close()
    c.close()
shutil.copytree(rpi / "web", out / "web")
for f in ("install.sh", "firstrun.sh", "check_media.sh", "recalboxdmd.service", "config.ini.seed"):
    shutil.copy2(rpi / "deploy" / f, out / f)
for f in ("LICENSE", "LICENSE.md"):
    shutil.copy2(rpi / f, out / f)
try:
    commit = subprocess.run(["git", "-C", str(rpi), "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip()
except OSError:
    commit = "?"
(out / "VERSION").write_text(f"{time.strftime('%Y-%m-%d')} {commit}\n", encoding="utf-8")
n = sum(1 for _ in out.rglob("*") if _.is_file())
print(f"charge utile : {out} ({n} fichiers, binaire {(out / 'bin' / 'dmd_daemon').stat().st_size} octets)")
