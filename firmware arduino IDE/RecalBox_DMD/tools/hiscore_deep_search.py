#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-25 - safe-modify - Creation initiale. Formalise en outil
#   reutilisable les 3 strategies de recherche developpees cette session
#   pendant la chasse (infructueuse) au score `inthunt` (voir
#   DECISIONS.md "Suite immediate (15)") -- utile pour tout futur jeu ou
#   `hiscore_recipe_builder.py` (recoupement simple, deja documente
#   faillible sur au moins 1 faux positif reel) ne suffit pas.
#
#   Contrairement a hiscore_recipe_builder.py (qui suppose une adresse
#   CONTIGUE avec un format/multiplicateur parmi un petit jeu de
#   candidats), cet outil est volontairement plus exhaustif :
#     --mode value   : comme le builder, mais balaie TOUTE la zone (pas
#                      besoin de --zone), formats binary/bcd/digits,
#                      2 endians, longueurs 1-8, multiplicateurs 1/10/100.
#     --mode delta   : NE PRESUPPOSE AUCUN multiplicateur -- calcule le
#                      ratio delta_reel/delta_brut entre observations
#                      consecutives et ne garde que les candidats ou ce
#                      ratio est EXACTEMENT constant (cross-multiplication,
#                      pas de tolerance flottante). Revele des encodages
#                      inhabituels (biais, multiplicateur non standard).
#     --mode digits_scattered : teste l'hypothese "chaque chiffre du
#                      score est stocke a une adresse INDEPENDANTE, pas
#                      contigue" -- recherche par POSITION de chiffre
#                      (unites/dizaines/centaines/...) sur toute la zone.
#
#   IMPORTANT (retour d'experience direct, `inthunt`) : le score change
#   EN CONTINU pendant le jeu actif (contrairement au credit, rarement
#   modifie) -- une capture ecran asynchrone peut donc afficher une
#   valeur DIFFERENTE de celle reellement en RAM au moment du snapshot,
#   meme sans aucune erreur de methode. `manager.machine:pause()` (Lua)
#   s'est revele un no-op silencieux dans le contexte libretro testé --
#   utiliser PAUSE_TOGGLE (commande reseau RetroArch externe, deja fiable
#   pour SCREENSHOT/QUIT) pour garantir la synchronisation avant de
#   fournir des observations a cet outil (voir
#   tools/mame_plugin_hiscore_probe -- test de reference
#   test_demo_capture_pause.sh en scratchpad de session, a porter ici si
#   ce chantier reprend).
"""Recherche approfondie d'une adresse (score ou autre) dans un snapshot
hiscore_probe, au-dela du simple recoupement de hiscore_recipe_builder.py.

Usage:
  hiscore_deep_search.py <snapshot.txt> --mode value \\
      --observe PLAY_3=900 --observe PLAY_5=1901 --observe PLAY_6=2401 \\
      --observe PLAY_7=2902

  hiscore_deep_search.py <snapshot.txt> --mode delta --observe ... (idem)

  hiscore_deep_search.py <snapshot.txt> --mode digits_scattered --observe ...

Au moins 3 --observe recommandes (2 minimum accepte mais peu fiable, voir
DECISIONS.md sur le faux positif 0xe09d5/0xe0bd5 -- et desormais aussi le
cas inthunt score, ou meme 4 observations SANS synchronisation garantie
avaient produit un faux positif).
"""
import argparse
import sys


def load(path):
    zones = {}
    snaps = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("ZONE"):
                cols = line.split("\t")
                zones[int(cols[1])] = {"astart": int(cols[4], 16), "aend": int(cols[5], 16),
                                        "label": cols[6] if len(cols) > 6 else ""}
            elif line.startswith("SNAP"):
                cols = line.split("\t")
                phase, zidx, hexdata = cols[1], int(cols[2]), cols[3]
                if hexdata == "ERROR":
                    continue
                snaps[(phase, zidx)] = bytes.fromhex(hexdata)
    return zones, snaps


def encode(value, fmt, endian, length):
    if fmt == "bcd":
        digits = "{:0{}d}".format(value, length * 2)
        if len(digits) > length * 2:
            return None
        raw = bytes(int(digits[i:i + 2], 16) for i in range(0, len(digits), 2))
    elif fmt == "digits":
        digits = "{:0{}d}".format(value, length)
        if len(digits) > length:
            return None
        raw = bytes(int(c) for c in digits)
    else:
        try:
            raw = value.to_bytes(length, "big")
        except OverflowError:
            return None
    return raw[::-1] if endian == "little" else raw


def zone_datas(zones, snaps, obs):
    """Pour chaque zone ayant toutes les phases observees, {phase: bytes}."""
    out = {}
    for zone_idx in zones:
        datas = {}
        ok = True
        for phase, _ in obs:
            d = snaps.get((phase, zone_idx))
            if d is None:
                ok = False
                break
            datas[phase] = d
        if ok:
            out[zone_idx] = datas
    return out


def mode_value(zones, snaps, obs):
    results = []
    datas_by_zone = zone_datas(zones, snaps, obs)
    for zone_idx, datas in datas_by_zone.items():
        astart = zones[zone_idx]["astart"]
        zone_len = len(datas[obs[0][0]])
        for fmt in ("binary", "bcd", "digits"):
            for endian in ("big", "little"):
                lengths = (1, 2, 3, 4, 5, 6, 7, 8) if fmt == "digits" else (1, 2, 3, 4)
                for length in lengths:
                    for mult in (1, 10, 100):
                        patterns = []
                        ok = True
                        for phase, val in obs:
                            if val % mult != 0:
                                ok = False
                                break
                            raw = encode(val // mult, fmt, endian, length)
                            if raw is None:
                                ok = False
                                break
                            patterns.append((phase, raw))
                        if not ok:
                            continue
                        first_phase, first_raw = patterns[0]
                        first_data = datas[first_phase]
                        cand = [i for i in range(zone_len - length + 1)
                                if first_data[i:i + length] == first_raw]
                        for phase, raw in patterns[1:]:
                            data = datas[phase]
                            cand = [off for off in cand if data[off:off + length] == raw]
                            if not cand:
                                break
                        for off in cand:
                            results.append({"zone": zone_idx, "addr": astart + off,
                                             "format": fmt, "endian": endian, "len": length,
                                             "multiplier": mult})
    return results


def mode_delta(zones, snaps, obs):
    real_deltas = [obs[i + 1][1] - obs[i][1] for i in range(len(obs) - 1)]
    results = []
    datas_by_zone = zone_datas(zones, snaps, obs)
    for zone_idx, datas in datas_by_zone.items():
        astart = zones[zone_idx]["astart"]
        zone_len = len(datas[obs[0][0]])
        for endian in ("big", "little"):
            for length in (1, 2, 3, 4):
                for off in range(zone_len - length + 1):
                    raw_vals = [int.from_bytes(datas[phase][off:off + length], endian) for phase, _ in obs]
                    raw_deltas = [raw_vals[i + 1] - raw_vals[i] for i in range(len(raw_vals) - 1)]
                    if all(d == 0 for d in raw_deltas):
                        continue
                    ok = True
                    for i in range(len(real_deltas)):
                        for j in range(i + 1, len(real_deltas)):
                            if real_deltas[i] * raw_deltas[j] != real_deltas[j] * raw_deltas[i]:
                                ok = False
                                break
                        if not ok:
                            break
                    if not ok:
                        continue
                    ratio = next((real_deltas[i] / raw_deltas[i] for i in range(len(real_deltas))
                                  if raw_deltas[i] != 0), None)
                    results.append({"zone": zone_idx, "addr": astart + off, "endian": endian,
                                     "len": length, "ratio": ratio, "raw_vals": raw_vals})
    return results


def mode_digits_scattered(zones, snaps, obs):
    results = []
    datas_by_zone = zone_datas(zones, snaps, obs)
    max_val = max(v for _, v in obs)
    n_digits = len(str(max_val))
    for digit_pos in range(n_digits):
        target_digits = [(v // (10 ** digit_pos)) % 10 for _, v in obs]
        if len(set(target_digits)) == 1:
            continue
        for zone_idx, datas in datas_by_zone.items():
            astart = zones[zone_idx]["astart"]
            zone_len = len(datas[obs[0][0]])
            for off in range(zone_len):
                if all(datas[phase][off] == digit for (phase, _), digit in zip(obs, target_digits)):
                    results.append({"zone": zone_idx, "addr": astart + off, "digit_position": digit_pos})
    return results


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("snapshot")
    ap.add_argument("--mode", choices=["value", "delta", "digits_scattered"], required=True)
    ap.add_argument("--observe", action="append", required=True,
                     help="PHASE=valeur -- au moins 3 recommandes (l'ordre compte pour --mode delta)")
    args = ap.parse_args()

    zones, snaps = load(args.snapshot)
    obs = []
    for o in args.observe:
        phase, val = o.split("=", 1)
        obs.append((phase, int(val)))
    if len(obs) < 2:
        print("au moins 2 --observe requis (3+ fortement recommande)", file=sys.stderr)
        sys.exit(1)

    print("mode={} observations={}".format(args.mode, obs))
    if args.mode == "value":
        results = mode_value(zones, snaps, obs)
        print("candidats: {}".format(len(results)))
        seen = {}
        for r in results:
            seen.setdefault((r["zone"], r["addr"]), []).append(r)
        for (zone, addr), variants in sorted(seen.items()):
            v = variants[0]
            print("  zone={} addr=0x{:x} len={} format={} endian={} multiplier={} ({} variantes)".format(
                zone, addr, v["len"], v["format"], v["endian"], v["multiplier"], len(variants)))
    elif args.mode == "delta":
        results = mode_delta(zones, snaps, obs)
        print("candidats: {}".format(len(results)))
        for r in results[:60]:
            print("  zone={} addr=0x{:x} len={} endian={} ratio={:.4f} raw={}".format(
                r["zone"], r["addr"], r["len"], r["endian"], r["ratio"], r["raw_vals"]))
    else:
        results = mode_digits_scattered(zones, snaps, obs)
        print("candidats: {}".format(len(results)))
        for r in results[:60]:
            print("  zone={} addr=0x{:x} position_chiffre={}".format(
                r["zone"], r["addr"], r["digit_position"]))

    if not results:
        print("\nAucun candidat -- essayer un autre --mode, ou considerer que la")
        print("valeur n'est peut-etre pas stockee en RAM sous une forme testable")
        print("(cf DECISIONS.md, cas inthunt : compteur materiel dedie suspecte).")


if __name__ == "__main__":
    main()
