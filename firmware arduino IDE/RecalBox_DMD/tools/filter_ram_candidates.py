#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-24 - safe-modify - Creation initiale. Filtre un (ou tous
#   les) fichier(s) produit(s) par dump_full_memmap.lua/
#   rb2_lua_memmap_harvest.py pour ne garder que les zones RAM
#   (read_type=="ram" ou write_type=="ram") -- les candidats plausibles
#   pour score/vies/credits/continue (demande explicite utilisateur :
#   "profite pour recolter l'adressage d'un maximum d'element par jeu").
#   Volontairement PAS de tentative de deviner "ceci EST le score" --
#   juste la LISTE des zones RAM avec leur nom de share si present (les
#   shares nommes, ex. "highscore"/"credits"/etc. sur d'autres jeux que
#   ceux testes ce soir, sont le signal le plus direct disponible sans
#   avoir a jouer -- la confirmation reelle reste la methode "verite
#   d'abord", ce script n'est qu'un pre-tri).
"""Filtre les zones RAM d'un ou plusieurs dumps produits par
dump_full_memmap.lua.

Usage: filter_ram_candidates.py <fichier.memmap.txt | dossier>
"""
import glob
import os
import sys


def filter_ram(path):
    """Retourne (romname, [ (cpu_tag, space, addr_start, addr_end, share, region, ctx), ... ])"""
    romname = "?"
    rows = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("#game"):
                romname = line.split("\t", 1)[1] if "\t" in line else "?"
                continue
            if line.startswith("#"):
                continue
            cols = line.split("\t")
            if len(cols) < 10:
                continue
            cpu_tag, space, astart, aend, rtype, rname, wtype, wname, share, region = cols[:10]
            if rtype == "ram" or wtype == "ram":
                ctx = share or region or rname or wname or ""
                rows.append((cpu_tag, space, astart, aend, share, region, ctx))
    return romname, rows


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    target = sys.argv[1]
    if os.path.isdir(target):
        paths = sorted(glob.glob(os.path.join(target, "*.memmap.txt")))
    else:
        paths = [target]

    for path in paths:
        romname, rows = filter_ram(path)
        print("=== {} ({} zones RAM) ===".format(romname, len(rows)))
        for cpu_tag, space, astart, aend, share, region, ctx in rows:
            size = int(aend, 16) - int(astart, 16) + 1
            label = share if share else (region if region else "")
            print("  {}:{}  0x{}-0x{} ({} octets){}".format(
                cpu_tag, space, astart, aend, size,
                "  share={}".format(label) if label else ""))


if __name__ == "__main__":
    main()
