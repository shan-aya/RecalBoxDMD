# -*- coding: utf-8 -*-
"""Surveille la version du theme PAR DEFAUT que Recalbox integre a ses images (recalbox-next, « Recalbox NEXT 2025 », signe Bounitos).

Les images officielles de Recalbox recuperent ce theme dans un depot PRIVE (recalbox-themes-prime, jeton CI) ; le depot public recalbox-themes reste
fige (febce03c, fevrier 2026) : le workflow « Update theme logos » ne peut donc pas voir les nouveautes (nouveaux systemes, logos, vues).
Le fichier PUBLIC package/recalbox-themes/recalbox-themes.mk du depot recalbox/recalbox donne en revanche la version privee utilisee par chaque branche
(RECALBOX_THEMES_VERSION du bloc « ifneq ($(GITLAB_TOKEN_THEMES),) »). Ce script compare ces versions a celles notees dans recalbox_theme_pin.json et
ecrit un compte rendu Markdown quand l'une a change : le workflow « Watch Recalbox theme pin » en fait une Issue GitHub.

Ensuite, le mainteneur (seul a avoir une Recalbox alpha qui contient le theme) lance en local check_recalbox_system_theme.py pour reconstruire le paquet.

Usage : python watch_recalbox_theme_pin.py [--report fichier.md] [--update]
  --update : ecrit les versions lues dans recalbox_theme_pin.json (a faire une fois le theme traite).
Code retour : 0 toujours (une panne reseau ne doit pas faire echouer le workflow chaque semaine) ; la sortie dit « CHANGED » / « UNCHANGED » / « UNREACHABLE »."""
import argparse
import json
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
PIN_FILE = HERE / "recalbox_theme_pin.json"
PROJECT = "recalbox%2Frecalbox"
MK = "package/recalbox-themes/recalbox-themes.mk"
UA = "RecalBoxDMD-theme-watch"


def fetch_mk(branch: str) -> str:
    url = f"https://gitlab.com/api/v4/projects/{PROJECT}/repository/files/{urllib.parse.quote(MK, safe='')}/raw?ref={urllib.parse.quote(branch, safe='')}"
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": UA}), timeout=30) as r:
        return r.read().decode("utf-8", "replace")


def private_pin(mk: str):
    """Version du bloc « jeton present » (depot prive) et du bloc public."""
    m = re.search(r"ifneq \(\$\(GITLAB_TOKEN_THEMES\),\)\s*RECALBOX_THEMES_VERSION\s*=\s*([0-9a-f]{7,40})", mk)
    p = re.search(r"else\s*RECALBOX_THEMES_VERSION\s*=\s*([0-9a-f]{7,40})", mk)
    return (m.group(1) if m else None), (p.group(1) if p else None)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--report", default=None)
    ap.add_argument("--update", action="store_true")
    a = ap.parse_args()
    state = json.loads(PIN_FILE.read_text(encoding="utf-8-sig"))
    branches = state["branches"]
    now, errors = {}, []
    for br in branches:
        try:
            priv, pub = private_pin(fetch_mk(br))
            if not priv:
                errors.append(f"{br} : version privee introuvable dans {MK} (format change ?)")
                continue
            now[br] = {"private": priv, "public": pub}
        except Exception as e:   # noqa: BLE001
            errors.append(f"{br} : {type(e).__name__}: {e}")
    changed = [(br, branches[br], now[br]) for br in now if now[br]["private"] != branches[br].get("private") or now[br]["public"] != branches[br].get("public")]
    if a.update and now:
        for br, v in now.items():
            branches[br] = v
        PIN_FILE.write_text(json.dumps(state, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    for e in errors:
        print("UNREACHABLE" if not now else "WARN", e)
    if not changed:
        print("UNCHANGED" if now else "UNREACHABLE")
        return 0
    lines = ["## Le theme par défaut de Recalbox (recalbox-next) a changé dans une image officielle", "",
             "Version **privée** (`recalbox-themes-prime`) référencée par le dépôt public `recalbox/recalbox` :", "",
             "| Branche | Avant | Maintenant |", "|---|---|---|"]
    for br, old, new in changed:
        lines.append(f"| `{br}` | `{(old.get('private') or '?')[:8]}` | `{new['private'][:8]}`" + (f" (public : `{new['public'][:8]}`)" if new.get("public") else "") + " |")
    lines += ["", "Ce depot est inaccessible depuis GitHub : le workflow « Update theme logos » ne le voit pas.",
              "**À faire** (sur le PC du mainteneur, Recalbox alpha à jour allumée) :", "",
              "1. `python tools/theme_package/check_recalbox_system_theme.py --host <IP de la Recalbox>` : compare le thème installé au paquet.",
              "2. Si le thème a changé : `... --build \"carte SD/systems/_defaults/_themes\" --known \"carte SD/systems/_defaults\"`, vérifier, publier.",
              "3. `python tools/theme_package/watch_recalbox_theme_pin.py --update` puis commiter `recalbox_theme_pin.json`."]
    text = "\n".join(lines) + "\n"
    if a.report:
        Path(a.report).write_text(text, encoding="utf-8")
    print("CHANGED", ", ".join(f"{br}:{new['private'][:8]}" for br, _o, new in changed))
    return 0


if __name__ == "__main__":
    sys.exit(main())
