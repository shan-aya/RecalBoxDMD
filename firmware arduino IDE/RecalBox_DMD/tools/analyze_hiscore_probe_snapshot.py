#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-25 - safe-modify - Creation initiale. Portage des scripts
#   d'analyse ad-hoc scratchpad (analyze_snapshot.py/summarize_v2.py,
#   session precedente) vers un outil versionne. Analyse un ou
#   plusieurs fichiers produits par rb2_hiscore_probe_harvest.py
#   (mame_plugin_hiscore_probe) : diff BOOT_SETTLE->POST_CREDIT
#   (candidats credit/etat) et recherche de candidats "score" sur le
#   dwell PLAY_1..8 -- MONOTONE croissant ET au moins MIN_DISTINCT
#   valeurs distinctes (filtre v2 : un simple saut 0->255 en une seule
#   etape n'est PAS un vrai score progressif, juste du bruit -- trouve
#   en observant des centaines de faux positifs de ce type sur
#   jjsquawk/mtwins/msgogo avec un filtre moins strict, voir
#   DECISIONS.md).
"""Analyse un ou plusieurs fichiers .snapshot.txt (plugin hiscore_probe).

Usage: analyze_hiscore_probe_snapshot.py <fichier.snapshot.txt | dossier> [--min-distinct 3] [--detail]
"""
import glob
import os
import sys

MIN_DISTINCT_DEFAULT = 3


def load(path):
    zones = {}
    snaps = {}
    phase_order = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("ZONE"):
                cols = line.split("\t")
                zones[int(cols[1])] = (cols[2], cols[3], cols[4], cols[5], cols[6])
            elif line.startswith("SNAP"):
                cols = line.split("\t")
                phase, zidx, hexdata = cols[1], int(cols[2]), cols[3]
                if hexdata == "ERROR":
                    continue
                snaps[(phase, zidx)] = bytes.fromhex(hexdata)
                if phase not in phase_order:
                    phase_order.append(phase)
    return zones, snaps, phase_order


def score_candidates(zones, snaps, phases, min_distinct):
    play_phases = [p for p in phases if p.startswith("PLAY_")]
    candidates = []
    for zidx in zones:
        seqs = [snaps.get((p, zidx)) for p in play_phases]
        seqs = [s for s in seqs if s is not None]
        if len(seqs) < 3:
            continue
        astart = int(zones[zidx][2], 16)
        for off in range(len(seqs[0])):
            vals = [s[off] for s in seqs]
            if all(vals[i] <= vals[i + 1] for i in range(len(vals) - 1)) and len(set(vals)) >= min_distinct:
                candidates.append((astart + off, vals))
    candidates.sort(key=lambda c: -(max(c[1]) - min(c[1])))
    return candidates


def credit_diff(zones, snaps):
    # v2 - 2026-08-25 - safe-modify - BUG REEL trouve par "verite d'abord"
    # (captures reelles sur progear) : ne comparait QUE la zone 1, alors
    # qu'un jeu a plusieurs zones (ex. progear en a 4) et que le vrai
    # changement peut etre dans n'importe laquelle -- credit_diff=0
    # rapporte a tort sur progear (le credit fonctionnait en realite,
    # 69 octets changes en zone 4/mainram, invisibles avec l'ancien
    # code qui ne regardait que la zone 1). Parcourt maintenant TOUTES
    # les zones.
    diffs = []
    for zidx in zones:
        a = snaps.get(("BOOT_SETTLE", zidx))
        b = snaps.get(("POST_CREDIT", zidx))
        if a is None or b is None or len(a) != len(b):
            continue
        astart = int(zones[zidx][2], 16)
        diffs.extend((astart + i, a[i], b[i]) for i in range(len(a)) if a[i] != b[i])
    return diffs


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    target = sys.argv[1]
    min_distinct = MIN_DISTINCT_DEFAULT
    if "--min-distinct" in sys.argv:
        min_distinct = int(sys.argv[sys.argv.index("--min-distinct") + 1])
    detail = "--detail" in sys.argv

    paths = sorted(glob.glob(os.path.join(target, "*.snapshot.txt"))) if os.path.isdir(target) else [target]

    for path in paths:
        rom = os.path.basename(path).split(".")[0]
        zones, snaps, phases = load(path)
        if not zones:
            print("{:12s} zones=0".format(rom))
            continue
        cd = credit_diff(zones, snaps)
        cands = score_candidates(zones, snaps, phases, min_distinct)
        print("{:12s} zones={} credit_diff={:3d} candidats_score(>={} valeurs)={:3d}  {}".format(
            rom, len(zones), len(cd), min_distinct, len(cands),
            "meilleur: addr=0x{:x} {}".format(cands[0][0], cands[0][1]) if cands else "-"))
        if detail:
            for addr, old, new in cd[:15]:
                print("    credit +0x{:x}: {} -> {}".format(addr, old, new))
            for addr, vals in cands[:10]:
                print("    score  +0x{:x}: {}".format(addr, vals))


if __name__ == "__main__":
    main()
