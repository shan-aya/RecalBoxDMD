#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-09-03 - safe-modify - Creation initiale. Generalise la
#   methode utilisee pour trouver l'adresse `lives` de `dynagear` cette
#   nuit (pilotage autonome uinput -> session longue avec captures
#   rapprochees -> lecture manuelle du HUD sur chaque screenshot pour
#   etablir la valeur attendue -> recherche par correlation exacte sur
#   toute la fenetre RAM). Reutilisable pour `lives`/`credit`/`ingame`
#   sur n'importe quel jeu, des que rb2_fbneo_autopilot.py peut generer
#   une session avec au moins une transition d'etat observable a l'ecran
#   (perte de vie, insertion credit, debut/fin de partie).
"""Cherche un octet unique dans une fenetre RAM fbneo dont la valeur
correspond EXACTEMENT a une sequence attendue sur plusieurs observations
d'une meme session (ou de plusieurs sessions passees en argument).

La sequence attendue se lit manuellement sur les screenshots (obsNN.png)
de chaque session -- ce script ne fait pas d'OCR, juste la correlation
RAM une fois les valeurs HUD identifiees a l'oeil.

Usage :
    search_state_byte.py <session_dir> <obs=valeur> [<obs=valeur> ...] \
        [--session2 <dir> <obs=valeur> ...]

Exemple (lives dynagear, cf. hiscore_recipes/dynagear.json) :
    search_state_byte.py /path/session1 obs01=2 obs02=2 obs03=2 \
        obs04=1 obs05=1 obs06=1 obs08=2 obs09=2

Pour croiser 2 sessions independantes (recommande, discipline "2/2" de
ce projet) : lancer une 1ere fois par session, comparer les candidats
trouves a la main (ou avec --intersect, voir plus bas).
"""
import argparse
import os
import re
import sys

PAIR_RE = re.compile(r"^(obs\d+)=(-?\d+)$")


def parse_pairs(items):
    out = {}
    for item in items:
        m = PAIR_RE.match(item)
        if not m:
            raise ValueError("format invalide (attendu obsNN=valeur) : {!r}".format(item))
        out[m.group(1)] = int(m.group(2))
    return out


def search(session_dir, expected):
    data = {}
    for phase in expected:
        path = os.path.join(session_dir, "{}.ram.bin".format(phase))
        with open(path, "rb") as f:
            data[phase] = f.read()
    size = min(len(b) for b in data.values())
    candidates = []
    for i in range(size):
        if all(data[phase][i] == val for phase, val in expected.items()):
            candidates.append(i)
    return candidates, size


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("session_dir")
    parser.add_argument("pairs", nargs="+", help="obsNN=valeur (valeur attendue en RAM a cet octet)")
    parser.add_argument("--session2-dir", help="2e session independante, pour croiser (--session2-pairs requis)")
    parser.add_argument("--session2-pairs", nargs="+", default=None)
    args = parser.parse_args()

    expected1 = parse_pairs(args.pairs)
    print("session 1 : {} -- attendu {}".format(args.session_dir, expected1))
    candidates1, size1 = search(args.session_dir, expected1)
    print("fenetre comparee : {} octets -- {} candidat(s)".format(size1, len(candidates1)))
    for addr in candidates1[:200]:
        print("  0x{:x} ({})".format(addr, addr))
    if len(candidates1) > 200:
        print("  ... ({} de plus, tronque)".format(len(candidates1) - 200))

    if args.session2_dir:
        if not args.session2_pairs:
            print("!! --session2-dir fourni sans --session2-pairs", file=sys.stderr)
            sys.exit(1)
        expected2 = parse_pairs(args.session2_pairs)
        print("\nsession 2 (croisement) : {} -- attendu {}".format(args.session2_dir, expected2))
        candidates2, size2 = search(args.session2_dir, expected2)
        print("{} candidat(s) sur la 2e session".format(len(candidates2)))
        common = sorted(set(candidates1) & set(candidates2))
        print("\n=== candidats COMMUNS aux 2 sessions ({}) ===".format(len(common)))
        for addr in common:
            print("  0x{:x} ({})".format(addr, addr))


if __name__ == "__main__":
    main()
