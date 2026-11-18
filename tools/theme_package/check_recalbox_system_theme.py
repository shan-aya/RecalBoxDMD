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

  4. SET PAR DEFAUT (systems/_defaults, --known) : les logos embarques de la Recalbox servent aussi aux images par defaut (base + fr/es). Le script les convertit (meme moteur) et les
     compare au dossier --known : identiques / proches (< 2 %, rien a faire) / DIFFERENTS / NOUVEAUX (systemes ajoutes). Avec --update-defaults (ou --build), les images differentes ou nouvelles
     sont copiees dans --known (base) et dans <known>/<langue>/ quand la Recalbox fournit la variante de langue ; les traductions DEJA presentes dont le logo de base a change mais que la
     Recalbox ne fournit pas (ex. favoris / tous les jeux / dernier joue en espagnol) sont listees « A RETRADUIRE » : a refaire a la main.
     Sans SSH : --defaults-only --embedded-dir <dossier logos deja lu> (ex. un dossier deja lu par une execution precedente).

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


DEFAULTS_NEAR = 0.02          # ecart moyen (0..1) en dessous duquel une image est « la meme » (arrondis de conversion)
DEFAULTS_LANGS = ("fr", "es")  # langues qui ont un dossier <known>/<langue>/ (le DMD n'en lit pas d'autres)


def _img_diff(a: bytes, b: bytes) -> float:
    """Ecart moyen normalise (0..1) entre deux images raw565 128x32."""
    n = 0
    for i in range(0, 8192, 2):
        va, vb = a[i] | (a[i + 1] << 8), b[i] | (b[i + 1] << 8)
        n += abs(((va >> 11) & 31) - ((vb >> 11) & 31)) + abs(((va >> 5) & 63) // 2 - ((vb >> 5) & 63) // 2) + abs((va & 31) - (vb & 31))
    return n / (4096 * 3 * 31)


def compare_defaults(embedded_dir: Path, known: Path, apply: bool = False) -> dict:
    """Compare les logos embarques de la Recalbox (convertis par theme_logos) au set par defaut `known` (= systems/_defaults). -> {"changed", "new", "lang_changed", "retranslate", "applied"}."""
    sys.path.insert(0, str(HERE))
    import shutil
    import theme_logos as tl
    os.environ["RBDMD_EMBEDDED_LOGOS_DIR"] = str(embedded_dir)
    out = Path(tempfile.mkdtemp(prefix="rbdef_"))
    tl.apply_embedded_logos(str(out), log=lambda *a: None)
    res = {"changed": [], "new": [], "lang_changed": {}, "lang_new": {}, "retranslate": {}, "applied": 0, "same": 0}
    changed_names = set()
    for f in sorted(out.glob("*.raw565")):
        old = known / f.name
        new = f.read_bytes()
        if not old.exists():
            res["new"].append(f.stem)
        else:
            d = _img_diff(old.read_bytes(), new)
            if d >= DEFAULTS_NEAR:
                res["changed"].append((f.stem, round(d, 3)))
                changed_names.add(f.stem)
            else:
                res["same"] += 1
    updated_lang = {}
    for lg in DEFAULTS_LANGS:
        src, dst = out / f"l_{lg}", known / lg
        res["lang_changed"][lg], res["lang_new"][lg], updated_lang[lg] = [], [], set()
        if not dst.is_dir():
            continue
        for f in sorted(src.glob("*.raw565")) if src.is_dir() else []:
            old = dst / f.name
            if not old.exists():
                res["lang_new"][lg].append(f.stem)      # variante fournie mais pas dans notre serie traduite : signalee, non ajoutee
            elif _img_diff(old.read_bytes(), f.read_bytes()) >= DEFAULTS_NEAR:
                res["lang_changed"][lg].append(f.stem)
                updated_lang[lg].add(f.stem)
    for lg in DEFAULTS_LANGS:
        dst = known / lg
        if dst.is_dir():
            res["retranslate"][lg] = sorted(n for n in changed_names if (dst / f"{n}.raw565").exists() and n not in updated_lang[lg])
    if apply:
        for n in [x[0] for x in res["changed"]] + res["new"]:
            shutil.copyfile(out / f"{n}.raw565", known / f"{n}.raw565")
            res["applied"] += 1
        for lg in DEFAULTS_LANGS:
            for n in res["lang_changed"].get(lg, []):
                shutil.copyfile(out / f"l_{lg}" / f"{n}.raw565", known / lg / f"{n}.raw565")
                res["applied"] += 1
    shutil.rmtree(out, ignore_errors=True)
    return res


def print_defaults_report(r: dict, applied: bool) -> None:
    print(f"== SET PAR DEFAUT (_defaults) : {r['same']} images identiques ou proches ; {len(r['changed'])} DIFFERENTES ; {len(r['new'])} NOUVELLES")
    if r["changed"]:
        print("   differentes :", ", ".join(f"{n} ({d})" for n, d in r["changed"]))
    if r["new"]:
        print("   nouvelles   :", ", ".join(r["new"]))
    for lg in DEFAULTS_LANGS:
        if r["lang_changed"].get(lg) is not None:
            print(f"   {lg} fournies par la Recalbox et differentes : {', '.join(r['lang_changed'][lg]) or '-'}" + (f" | variantes non reprises (absentes de notre serie) : {', '.join(r['lang_new'][lg])}" if r["lang_new"].get(lg) else ""))
    for lg, names in r["retranslate"].items():
        if names:
            print(f"   A RETRADUIRE ({lg}) : {', '.join(names)}  -- le logo de base a change et la Recalbox n'en fournit pas la traduction")
    print(f"   -> {r['applied']} fichier(s) copie(s) dans le dossier par defaut." if applied else "   (rien modifie : ajouter --update-defaults ou --build pour copier les images differentes / nouvelles)")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--host", default=None, help="adresse de la Recalbox alpha (SSH) ; inutile avec --defaults-only --embedded-dir")
    ap.add_argument("--user", default="root")
    ap.add_argument("--build", default=None, metavar="DOSSIER_THEMES", help="reconstruit recalbox-next dans ce dossier _themes si le theme a change")
    ap.add_argument("--known", default=os.path.join("sd_card", "systems", "_defaults"))
    ap.add_argument("--force", action="store_true", help="reconstruit meme si l'empreinte est celle deja traitee")
    ap.add_argument("--update-defaults", action="store_true", help="copie dans --known les images par defaut differentes / nouvelles (voir 4. ci-dessus)")
    ap.add_argument("--defaults-only", action="store_true", help="ne controle que le set par defaut (pas le theme)")
    ap.add_argument("--embedded-dir", default=None, metavar="DOSSIER_LOGOS", help="dossier de logos embarques DEJA lus (evite le SSH) ; avec --defaults-only")
    a = ap.parse_args()
    if a.defaults_only:
        emb = a.embedded_dir
        if not emb:
            if not a.host:
                sys.exit("--defaults-only demande --embedded-dir ou --host")
            pwd = os.environ.get("RB_SSH_PASSWORD") or getpass.getpass(f"Mot de passe SSH {a.user}@{a.host} : ")
            tmp0 = Path(tempfile.mkdtemp(prefix="rbtheme_"))
            print(f"Recalbox {pull(a.host, a.user, pwd, tmp0)} lue en SSH")
            emb = str(tmp0 / "_embedded" / "logos")
        known = Path(a.known)
        if not known.is_dir():
            sys.exit(f"dossier par defaut introuvable : {known}")
        print_defaults_report(compare_defaults(Path(emb), known, apply=a.update_defaults), a.update_defaults)
        return 0
    if not a.host:
        sys.exit("--host est obligatoire (sauf avec --defaults-only --embedded-dir)")
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
    known_dir = Path(a.known)
    if known_dir.is_dir():          # set par defaut : toujours compare (les logos embarques changent avec la Recalbox, pas seulement le theme)
        apply_def = bool(a.update_defaults or a.build is not None)
        print_defaults_report(compare_defaults(tmp / "_embedded" / "logos", known_dir, apply=apply_def), apply_def)
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
