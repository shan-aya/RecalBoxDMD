# -*- coding: utf-8 -*-
"""Controle (et mise a jour) du theme PAR DEFAUT de Recalbox (recalbox-next) depuis une Recalbox ALPHA, a lancer par le mainteneur a chaque nouvelle version.

Pourquoi : les images officielles embarquent ce theme depuis un depot prive (recalbox-themes-prime) ; le depot public est fige. La seule source accessible
est une Recalbox installee. Auteur du theme : Bounitos (accord de conversion pour le DMD depuis l'alpha, confirme par le mainteneur le 2026-11-16).

Le script LIT la Recalbox en SSH (lecture seule : un tar du dossier du theme, rien n'est ecrit sur la Recalbox) :
  1. version de la Recalbox (/recalbox/recalbox.version) et empreinte du theme ;
  2. comparaison avec le dernier etat traite (system_theme_state.json) et avec le paquet GitHub (fichiers nouveaux / modifies) ;
  3. avec --build : reconstruit le seul theme recalbox-next du paquet depuis cette copie (build_theme_package.py --rb --prefer-rb --only recalbox-next)
     puis note l'etat dans system_theme_state.json. Les logos embarques (/recalbox/system/resources/logos : favorites, allgames, lastplayed, nouveaux systemes)
     sont lus sur la Recalbox aussi (variable RBDMD_EMBEDDED_LOGOS_DIR vue par theme_logos.py), pas dans l'archive publique 11.0-patron-1.

Mot de passe SSH : variable d'environnement RB_SSH_PASSWORD, sinon saisie masquee. Jamais ecrit sur disque. Necessite : pip install paramiko.

Usage : python check_recalbox_system_theme.py --host 192.168.0.35 [--build <dossier _themes> --known <dossier _defaults>]"""
import argparse
import getpass
import hashlib
import io
import json
import os
import subprocess
import sys
import tarfile
import tempfile
from datetime import date
from pathlib import Path

HERE = Path(__file__).resolve().parent
STATE_FILE = HERE / "system_theme_state.json"
THEME_DIR = "/recalbox/share_init/system/.emulationstation/themes"
THEME = "recalbox-next"
LOGOS_PARENT = "/recalbox/system/resources"       # logos embarques (${system.logo}) : favorites, allgames, lastplayed, genres, nouveaux systemes...


def pull(host, user, password, dest: Path):
    try:
        import paramiko
    except ImportError:
        sys.exit("paramiko absent : pip install paramiko")
    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(host, username=user, password=password, timeout=15, allow_agent=False, look_for_keys=False)

    def run(cmd, binary=False, timeout=600):
        _i, o, _e = c.exec_command(cmd, timeout=timeout)
        data = o.read()
        return data if binary else data.decode("utf-8", "replace").strip()
    version = run("cat /recalbox/recalbox.version")
    raw = run(f"tar cf - -C {THEME_DIR} {THEME}", binary=True)
    raw_logos = run(f"tar cf - -C {LOGOS_PARENT} logos", binary=True)
    c.close()
    if not raw or not raw_logos:
        sys.exit(f"theme ou logos introuvables ({THEME_DIR}/{THEME}, {LOGOS_PARENT}/logos)")
    tarfile.open(fileobj=io.BytesIO(raw)).extractall(dest, filter="data")
    tarfile.open(fileobj=io.BytesIO(raw_logos)).extractall(dest / "_embedded", filter="data")      # -> dest/_embedded/logos (sans theme.xml : ignore par le constructeur)
    return version


def tree_hashes(root: Path) -> dict:
    return {str(p.relative_to(root)).replace("\\", "/"): hashlib.md5(p.read_bytes()).hexdigest() for p in sorted(root.rglob("*")) if p.is_file()}


def tree_fingerprint(h: dict) -> str:
    return hashlib.sha256("\n".join(f"{k} {v}" for k, v in sorted(h.items())).encode()).hexdigest()[:16]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--host", required=True)
    ap.add_argument("--user", default="root")
    ap.add_argument("--build", default=None, metavar="DOSSIER_THEMES", help="reconstruit recalbox-next dans ce dossier _themes si le theme a change")
    ap.add_argument("--known", default=os.path.join("sd_card", "systems", "_defaults"))
    ap.add_argument("--force", action="store_true", help="reconstruit meme si l'empreinte est celle deja traitee")
    a = ap.parse_args()
    pw = os.environ.get("RB_SSH_PASSWORD") or getpass.getpass(f"Mot de passe SSH {a.user}@{a.host} : ")
    tmp = Path(tempfile.mkdtemp(prefix="rbtheme_"))
    version = pull(a.host, a.user, pw, tmp)
    root = tmp / THEME
    hashes = tree_hashes(root)
    hashes.update({"logos/" + k: v for k, v in tree_hashes(tmp / "_embedded" / "logos").items()})     # le theme ET les logos embarques comptent dans l'empreinte
    fp = tree_fingerprint(hashes)
    state = json.loads(STATE_FILE.read_text(encoding="utf-8")) if STATE_FILE.exists() else {}
    print(f"Recalbox {version} : {len(hashes)} fichiers, empreinte {fp}")
    if state:
        print(f"Dernier etat traite : Recalbox {state.get('recalbox_version')} le {state.get('date')}, empreinte {state.get('fingerprint')}, {state.get('files')} fichiers")
    changed = fp != state.get("fingerprint")
    print("-> theme CHANGE depuis le dernier traitement" if changed else "-> theme identique au dernier traitement")
    if a.build is None:
        print("(ajouter --build <dossier _themes> --known <dossier _defaults> pour reconstruire le paquet)")
        return 0
    if not changed and not a.force:
        return 0
    # --incremental conserve les autres themes du paquet ; mais le constructeur SUPPRIME ceux qui ne sont ni au hub ni dans --rb (themes ajoutes depuis une Recalbox,
    # ex. beudbeud-astro). On les declare donc a la constructeur par un dossier vide « theme.xml » dans la racine --rb : presents = conserves, jamais reconstruits (--only).
    for d in Path(a.build).iterdir():
        if d.is_dir() and d.name != THEME and not (tmp / d.name).exists():
            (tmp / d.name).mkdir()
            (tmp / d.name / "theme.xml").write_text("<theme/>", encoding="utf-8")
    cmd = [sys.executable, str(HERE / "build_theme_package.py"), a.build, "--known", a.known, "--only", THEME, "--rb", str(tmp), "--prefer-rb", "--incremental"]
    print("Construction :", " ".join(cmd))
    env = dict(os.environ, RBDMD_EMBEDDED_LOGOS_DIR=str(tmp / "_embedded" / "logos"))      # logos embarques = ceux de CETTE Recalbox (theme_logos.py)
    r = subprocess.run(cmd, env=env)
    if r.returncode:
        sys.exit(f"echec de la construction (code {r.returncode}) : etat non mis a jour")
    STATE_FILE.write_text(json.dumps({"recalbox_version": version, "date": date.today().isoformat(), "fingerprint": fp, "files": len(hashes)}, indent=2) + "\n", encoding="utf-8")
    print(f"Etat note dans {STATE_FILE.name}. A faire : verifier le paquet, commiter, puis `watch_recalbox_theme_pin.py --update`.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
