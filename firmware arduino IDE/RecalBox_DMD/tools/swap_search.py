#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v2
#
# v2 - 2026-08-30 - safe-modify - Ajout du format "digits" (1 chiffre
#   decimal brut par octet -- format officiel ScoreWatch.py::decode_score,
#   commentaire du code "Osman & co") + tailles etendues 5-8 octets pour
#   l'accommoder. Retestee sur les outils officiels RecalBox telecharges
#   et lus en entier (ramSearch.py/ScoreWatch.py, /usr/lib/python3.11/
#   site-packages/configgen/challenge/ sur RB2) : confirme que notre
#   variante "wordLE_be" est mathematiquement equivalente a l'algorithme
#   byteSwap officiel (raw[i^1], echange les 2 octets ADJACENTS de chaque
#   mot -- pas les 2 mots entiers comme suppose en v1) une fois combinee
#   a endian=big. `ramSearch.py` officiel NE teste PAS byteSwap du tout --
#   notre recherche va plus loin que l'outil stock RecalBox sur ce point.
#
# v1 - 2026-08-30 - safe-modify - Creation initiale. Recherche EXACTE
#   (avec tolerance optionnelle) d'une adresse score dans des dumps
#   READ_CORE_RAM deja captures (rb2_fbneo_manual_wide_capture.py),
#   en testant BCD et binaire en BE/LE classiques PLUS le "byteSwap"
#   (les 2 mots de 16 bits d'une valeur 32 bits inverses entre eux --
#   artefact 68000/bus 16 bits) -- detail retrouve dans le format
#   recipe officiel RecalBox (/recalbox/share/system/challenges/
#   current.json, champ score.byteSwap) et jamais teste avant cette
#   session. A debloque 4 jeux d'un coup la ou la recherche BCD/binaire
#   classique (tolerant_search.py) donnait 0 candidat malgre des
#   donnees de verite propres -- voir DECISIONS.md "Suite immediate (19)".
#
#   Adresses confirmees avec cet outil (2026-08-30) : pzloop2=0x8508
#   (binaire), mtwins=0x1530 (BCD), willow=0x834a (BCD),
#   gogomile=0x4c8 (binaire) -- toutes en variante mot-swap.
"""Recherche exacte (ou tolerante) d'une adresse score, avec variantes
de byteSwap (mots 16 bits inverses) en plus de BCD/binaire BE/LE
classique -- sur des dumps deja captures par rb2_fbneo_manual_wide_capture.py.

Usage: swap_search.py <run_dir> <tolerance> "phase1=val1" "phase2=val2" ...
Ex.  : swap_search.py pzloop2/20260829-173818 0 obs04=0 obs08=159100
"""
import glob
import sys


def bcd_value(raw):
    digits = []
    for byte in raw:
        hi, lo = byte >> 4, byte & 0xF
        if hi > 9 or lo > 9:
            return None
        digits.append(str(hi))
        digits.append(str(lo))
    return int("".join(digits)) if digits else None


def digits_value(raw):
    """1 chiffre decimal par octet -- format officiel ScoreWatch.py
    fmt=="digits", commentaire "Osman & co"."""
    digits = []
    for byte in raw:
        if byte > 9:
            return None
        digits.append(str(byte))
    return int("".join(digits)) if digits else None


def variants(raw):
    """Toutes les permutations plausibles d'un bloc de 2 ou 4 octets."""
    n = len(raw)
    out = {}
    out["be"] = raw
    out["le"] = raw[::-1]
    if n == 4:
        # word-swap : les 2 mots de 16 bits sont inverses entre eux
        w0, w1 = raw[0:2], raw[2:4]
        out["wordswap_be"] = w1 + w0
        out["wordswap_le"] = (w1 + w0)[::-1]
        # chaque mot individuellement en LE, mots dans l'ordre normal
        out["wordLE_be"] = w0[::-1] + w1[::-1]
        out["wordLE_wordswap"] = w1[::-1] + w0[::-1]
    return out


def main():
    run_dir = sys.argv[1]
    tol = int(sys.argv[2])
    obs = []
    for arg in sys.argv[3:]:
        phase, val = arg.split("=")
        obs.append((phase, int(val)))

    dumps = {}
    for phase, _ in obs:
        path = glob.glob("{}/{}-*.ram.bin".format(run_dir, phase))[0]
        dumps[phase] = open(path, "rb").read()

    length = min(len(d) for d in dumps.values())
    hits = []
    for size in (2, 4, 5, 6, 7, 8):
        for offset in range(length - size + 1):
            raws = {p: dumps[p][offset:offset + size] for p, _ in obs}
            for variant_name in variants(raws[obs[0][0]]).keys():
                for fmt in ("bcd", "binary", "digits"):
                    ok = True
                    decoded = {}
                    for phase, visible in obs:
                        v = variants(raws[phase])[variant_name]
                        if fmt == "bcd":
                            val = bcd_value(v)
                        elif fmt == "digits":
                            val = digits_value(v)
                        else:
                            val = int.from_bytes(v, "big")
                        if val is None or abs(val - visible) > tol:
                            ok = False
                            break
                        decoded[phase] = val
                    if ok:
                        hits.append((offset, size, variant_name, fmt, decoded))
    for h in hits:
        print("addr=0x{:x} size={} variant={} fmt={} decoded={}".format(*h))
    print("total: {}".format(len(hits)))


if __name__ == "__main__":
    main()
