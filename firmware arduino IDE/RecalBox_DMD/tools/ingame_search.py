#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-30 - safe-modify - Creation initiale. Cherche un octet
#   'gate ingame' : une adresse CONSTANTE sur toutes les observations
#   "hors partie" (boot/titre/attract) ET CONSTANTE mais DIFFERENTE sur
#   toutes les observations "en partie" (gameplay reel), sans
#   presupposer de valeur -- contrairement a swap_search.py qui cherche
#   une valeur precise deja connue. Teste sur kamenrid (2026-08-30) :
#   avec seulement 1 observation par etat, bruit enorme (2414 candidats
#   sur pzloop2) ; avec plusieurs observations diversifiees de chaque
#   cote (reutiliser une session de chasse au credit, qui capture deja
#   plusieurs instants differents de l'attract-loop), 185 candidats sur
#   kamenrid -- encore trop bruite pour isoler un flag unique fiable
#   (beaucoup de blocs d'adresses consecutives partageant la meme
#   valeur, probablement des donnees graphiques/palette qui changent
#   avec la scene affichee). Chantier NON ABOUTI -- voir DECISIONS.md
#   "Suite immediate (21)", section ingame.
"""Cherche un octet 'gate ingame' : une adresse dont la valeur est
CONSTANTE sur toutes les observations 'hors partie' (boot/titre/attract)
ET CONSTANTE (mais differente) sur toutes les observations 'en partie'
(gameplay reel) -- sans presupposer les valeurs.

Usage: ingame_search.py --notingame DIR p1 p2 ... --ingame DIR p3 p4 ...
(DIR peut differer entre les 2 groupes -- 2 sessions distinctes du meme jeu)
Ex.  : ingame_search.py --notingame kamenrid/20260830-135445 obs01 obs20 obs25 \
           --ingame kamenrid/20260830-135445 obs05 obs08 obs10
"""
import glob
import sys


def load(run_dir, phases):
    dumps = {}
    for phase in phases:
        path = glob.glob("{}/{}-*.ram.bin".format(run_dir, phase))[0]
        dumps[phase] = open(path, "rb").read()
    return dumps


def main():
    args = sys.argv[1:]
    groups = {"--notingame": None, "--ingame": None}
    i = 0
    while i < len(args):
        if args[i] in groups:
            key = args[i]
            run_dir = args[i + 1]
            phases = []
            j = i + 2
            while j < len(args) and not args[j].startswith("--"):
                phases.append(args[j])
                j += 1
            groups[key] = (run_dir, phases)
            i = j
        else:
            i += 1

    not_dir, not_phases = groups["--notingame"]
    in_dir, in_phases = groups["--ingame"]
    not_dumps = load(not_dir, not_phases)
    in_dumps = load(in_dir, in_phases)

    length = min(min(len(d) for d in not_dumps.values()),
                 min(len(d) for d in in_dumps.values()))
    hits = []
    for offset in range(length):
        not_vals = set(d[offset] for d in not_dumps.values())
        in_vals = set(d[offset] for d in in_dumps.values())
        if len(not_vals) == 1 and len(in_vals) == 1 and not_vals != in_vals:
            hits.append((offset, list(not_vals)[0], list(in_vals)[0]))

    print("total candidats: {}".format(len(hits)))
    for offset, nv, iv in hits[:40]:
        print("addr=0x{:x} hors_partie={} en_partie={}".format(offset, nv, iv))
    if len(hits) > 40:
        print("... ({} de plus, non affiches)".format(len(hits) - 40))


if __name__ == "__main__":
    main()
