#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Meme algorithme de
#   recherche que build_entry_from_truth.py (find_score_hits -- BCD/binaire,
#   tailles 1-4 octets, echelles x1/x10/x100/x1000/x10000), mais applique a
#   un dump RAM brut (produit par rb2_ram_truth_capture.py) au lieu d'un
#   .hi deja ecrit par le plugin -- utile quand le plugin hiscore n'a
#   JAMAIS ecrit de .hi (entree hiscore.dat absente ou fausse, ex. reel
#   trouve ce soir : batlzone rattache par erreur au bloc mayday). Affiche
#   en plus un hexdump de contexte autour de chaque correspondance pour
#   verification visuelle rapide, et calcule l'adresse absolue (base_addr
#   + offset) directement exploitable pour ecrire une ligne hiscore.dat.
"""Cherche une valeur (score) dans un dump RAM brut, a toutes les tailles/
echelles/formats plausibles -- meme methode que build_entry_from_truth.py.

Usage: find_value_in_ramdump.py <dump_file> <target_value> [base_addr_hex]
  base_addr_hex : adresse de depart du dump (defaut 0x0) -- necessaire
  pour convertir un offset trouve en adresse absolue exploitable dans
  hiscore.dat.
"""
import sys


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


def binary_decode_le(raw):
    v = 0
    for b in reversed(raw):
        v = v * 256 + b
    return v


def find_score_hits(data, target_score, sizes=(1, 2, 3, 4), scales=(1, 10, 100, 1000, 10000)):
    hits = []
    for size in sizes:
        for off in range(0, len(data) - size + 1):
            raw = data[off:off + size]
            for fmt, decoder in (("bcd", bcd_decode), ("binary_be", binary_decode),
                                  ("binary_le", binary_decode_le)):
                v = decoder(raw)
                if v is None:
                    continue
                for scale in scales:
                    if v * scale == target_score:
                        if fmt != "bcd" and any(
                                h[:3] == (off, size, scale) for h in hits):
                            continue
                        hits.append((off, size, scale, fmt))
    return hits


def hexdump_context(data, off, size, radius=8):
    start = max(0, off - radius)
    end = min(len(data), off + size + radius)
    chunk = data[start:end]
    hexstr = " ".join("{:02x}".format(b) for b in chunk)
    marker_start = (off - start) * 3
    marker = " " * marker_start + "^" + "^" * max(0, (size * 3) - 2)
    return "  addr_dump 0x{:x}: {}\n  {}".format(start, hexstr, marker)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    dump_file = sys.argv[1]
    target = int(sys.argv[2])
    base_addr = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0

    with open(dump_file, "rb") as f:
        data = f.read()

    print("dump: {} ({} octets), cible={}, base_addr=0x{:x}".format(
        dump_file, len(data), target, base_addr))

    hits = find_score_hits(data, target)
    if not hits:
        print("AUCUNE correspondance -- essayer un autre score visible, ou une autre "
              "fenetre RAM (--extra-addr lors de la capture).")
        return

    print("{} correspondance(s) :".format(len(hits)))
    for off, size, scale, fmt in hits:
        abs_addr = base_addr + off
        # calcule les octets de declenchement (wait_first/wait_last) pour
        # une eventuelle entree hiscore.dat -- ce sont litteralement les
        # octets observes ICI, puisque l'etat est deja initialise (le jeu
        # tourne et affiche deja ce score a l'ecran).
        wait_first = data[off]
        wait_last = data[off + size - 1]
        print("- offset=0x{:x} (addr absolue=0x{:x}) taille={} format={} echelle={}".format(
            off, abs_addr, size, fmt, scale))
        print("  hiscore.dat candidat: @:maincpu,program,{:x},{:x},{:02x},{:02x}".format(
            abs_addr, size, wait_first, wait_last))
        print(hexdump_context(data, off, size))


if __name__ == "__main__":
    main()
