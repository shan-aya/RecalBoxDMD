#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Methode "verite
#   d'abord" (voir DECISIONS.md, section hi-score generique MAME/FBNeo) :
#   au lieu de deviner statistiquement puis corriger a posteriori (Phase 2,
#   phase2_static_analyze.py), on capture d'abord une VRAIE valeur affichee
#   a l'ecran (score + eventuellement nom, via la commande reseau RetroArch
#   SCREENSHOT, UDP 127.0.0.1:55355 -- voir tools/phase2_verification_tracking.md
#   pour le protocole complet), puis on cherche EXHAUSTIVEMENT dans le vrai
#   fichier .hi la position/taille/echelle/format (BCD ou binaire brut) qui
#   reproduit EXACTEMENT cette valeur -- beaucoup plus fiable qu'une
#   heuristique de plausibilite statistique. A deja permis de corriger
#   ikari/cotton/ninjemak/bonzeadv/citybomb (voir phase2_verification_tracking.md)
#   et de reveler 2 lacunes de l'heuristique statistique : champs score
#   etroits (1-2 octets) avec facteur d'echelle, et encodage binaire brut
#   (jamais testes par phase2_static_analyze.py).
"""Construit une entree hiscore_manifest.json a partir d'une VERITE TERRAIN
(score + nom reellement vus a l'ecran, via capture RetroArch) -- cherche
TOUTES les positions ou le score decode (BCD ou binaire brut, avec ou sans
facteur d'echelle x1/x10/x100/x1000/x10000) correspond EXACTEMENT, localise
le nom a proximite si fourni, propose un stride en cherchant la repetition
du nom un peu plus loin dans le fichier.

Usage: build_entry_from_truth.py <rom> <hi_file> <target_score> [target_name]
"""
import sys
import os
import json


def bcd_decode(raw):
    digits = ""
    for b in raw:
        hi, lo = b >> 4, b & 0xF
        if hi > 9 or lo > 9:
            return None
        digits += str(hi) + str(lo)
    return int(digits) if digits else 0


def binary_decode(raw):
    v = 0
    for b in raw:
        v = v * 256 + b
    return v


def find_score_hits(data, target_score, sizes=(1, 2, 3, 4), scales=(1, 10, 100, 1000, 10000)):
    hits = []
    for size in sizes:
        for off in range(0, len(data) - size + 1):
            raw = data[off:off + size]
            for fmt, decoder in (("bcd", bcd_decode), ("binary", binary_decode)):
                v = decoder(raw)
                if v is None:
                    continue
                for scale in scales:
                    if v * scale == target_score:
                        # evite les doublons binaire/bcd qui donnent la meme
                        # valeur par coincidence (rare mais possible sur de
                        # petits nombres) -- garde bcd en priorite si les 2
                        # matchent, plus courant dans ce contexte.
                        if fmt == "binary" and any(
                                h[:3] == (off, size, scale) for h in hits):
                            continue
                        hits.append((off, size, scale, fmt))
    return hits


def find_name_near(data, off, score_size, target_name):
    name_bytes = target_name.encode("ascii", "ignore")
    if not name_bytes:
        return None
    window_start = max(0, off - 12)
    window_end = min(len(data), off + score_size + 20)
    window = data[window_start:window_end]
    idx = window.find(name_bytes)
    if idx < 0:
        return None
    abs_off = window_start + idx
    return abs_off - off  # relatif au debut du champ score


def guess_stride(data, score_off, name_off, name_bytes):
    """Cherche la PROCHAINE occurrence des memes octets de nom plus loin
    dans le fichier -- bien plus fiable qu'un simple "BCD valide" (les
    lettres ASCII collisionnent facilement avec des paires BCD valides,
    voir le motif texte-ASCII-pris-pour-score deja documente dans
    phase2_static_analyze.py v4)."""
    if not name_bytes:
        return None
    idx = data.find(name_bytes, name_off + 1)
    if idx < 0:
        return None
    return idx - name_off


def main():
    if len(sys.argv) < 4:
        print("usage: build_entry_from_truth.py <rom> <hi_file> <target_score> [target_name]")
        return
    rom = sys.argv[1]
    hi_file = sys.argv[2]
    target_score = int(sys.argv[3])
    target_name = sys.argv[4] if len(sys.argv) > 4 else None

    with open(hi_file, "rb") as f:
        data = f.read()

    hits = find_score_hits(data, target_score)
    if not hits:
        print(json.dumps({"rom": rom, "status": "no_score_match"}))
        return

    results = []
    for off, size, scale, fmt in hits:
        name_rel = find_name_near(data, off, size, target_name) if target_name else None
        entry = {"score_offset": off, "score_size": size, "scale": scale,
                  "format": fmt, "name_rel_offset": name_rel}
        if name_rel is not None:
            name_off = off + name_rel
            stride = guess_stride(data, off, name_off, target_name.encode("ascii", "ignore"))
            entry["stride_guess"] = stride
        results.append(entry)

    print(json.dumps({"rom": rom, "file_size": len(data), "status": "ok", "candidates": results}, indent=2))


if __name__ == "__main__":
    main()
