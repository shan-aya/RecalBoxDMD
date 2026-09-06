#!/usr/bin/env python3
"""add_verified_score.py v1 -- 2026-09-04

Outil d'ajout pour verified_default_scores.json (niveau 2 de la
cascade hi-score, voir dmd_hiscore_verified.py). A utiliser a la main,
au cas par cas, pour un jeu dont le .hi ne peut pas etre obtenu
(verifie au prealable : vraie partie credit+jouee jusqu'a game over,
toujours pas de .hi -- voir DECISIONS.md) mais dont l'ecran hi-score a
ete lu directement sur une capture d'ecran reelle (methode "verite
d'abord" -- ne JAMAIS inventer les valeurs).

Usage :
    python3 add_verified_score.py <system> <rom> \
        --entry "NOM=SCORE" [--entry "NOM=SCORE" ...] \
        --source "capture ecran apres partie credit reel, game over" \
        [--date AAAA-MM-JJ]

Exemple (1941, mame0278, lu sur une capture d'ecran hi-score reelle) :
    python3 add_verified_score.py mame0278 1941 \
        --entry "1WD=91500" --entry "CAP=84300" \
        --source "capture ecran table hi-score interne, partie credit reelle"

Trie les entrees par score decroissant, renumerote 1..N, ecrit une
sauvegarde .bak avant modification (meme prudence que les autres
outils du chantier qui touchent le manifeste).
"""
import argparse
import datetime
import json
import os
import shutil
import sys

DATA_PATH = os.path.join(os.path.dirname(__file__), "verified_default_scores.json")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("system")
    ap.add_argument("rom")
    ap.add_argument("--entry", action="append", required=True,
                     help="NOM=SCORE, repetable")
    ap.add_argument("--source", required=True)
    ap.add_argument("--date", default=datetime.date.today().isoformat())
    args = ap.parse_args()

    parsed = []
    for raw in args.entry:
        if "=" not in raw:
            print(f"!! entree invalide (attendu NOM=SCORE) : {raw}", file=sys.stderr)
            sys.exit(1)
        name, score_s = raw.rsplit("=", 1)
        name = name.strip()
        try:
            score = int(score_s.strip())
        except ValueError:
            print(f"!! score non entier : {raw}", file=sys.stderr)
            sys.exit(1)
        parsed.append((score, name))

    parsed.sort(key=lambda t: -t[0])
    lines = [f"{i} {name} {score}" for i, (score, name) in enumerate(parsed, start=1)]

    with open(DATA_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    key = f"{args.system}_{args.rom}"
    if key in data:
        print(f"-- entree existante pour {key} remplacee (etait : {data[key].get('lines')})")

    data[key] = {"lines": lines, "source": args.source, "date": args.date}

    shutil.copy2(DATA_PATH, DATA_PATH + ".bak")
    with open(DATA_PATH, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")

    print(f"OK {key} -> {' | '.join(lines)}")


if __name__ == "__main__":
    main()
