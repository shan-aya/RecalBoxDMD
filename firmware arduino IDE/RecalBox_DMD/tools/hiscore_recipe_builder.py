#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-25 - safe-modify - Creation initiale. Formalise en outil
#   reutilisable la methode de croisement developpee manuellement cette
#   session (find_real_score.py/cross_check_score.py, scratchpad) :
#   prend un fichier produit par le plugin hiscore_probe (snapshots RAM
#   par phase) + une liste d'observations (phase, valeur affichee a
#   l'ecran, lue par capture "verite d'abord") et produit une recette
#   au format du module Challenge OFFICIEL de RecalBox (decouvert cette
#   session : /usr/lib/python3.11/site-packages/configgen/challenge/,
#   voir DECISIONS.md "Suite immediate (10)") -- meme schema que
#   ramSearch.py::manifest_snippet() (addr/sysramOffset/len/format/
#   endian/multiplier), directement reutilisable par leur infrastructure
#   de consommation existante (HiscoreTable.py).
#
#   Objectif explicite utilisateur : "fournir un outil qui produit des
#   recettes VERIFIEES pour chaque jeu" -- "verifiee" ici signifie
#   recoupee sur AU MOINS 2 observations independantes (pas une seule
#   valeur, insuffisant -- vu cette session avec le faux positif
#   0xe09d5/0xe0bd5 qui matchait 400 puis 600 par coincidence).
#
#   Champ additionnel "read_core_ram_ok" (au-dela du format officiel) :
#   teste si l'adresse trouvee est lisible via READ_CORE_RAM/
#   READ_CORE_MEMORY (mecanisme que ScoreWatch.py utilise en session
#   reelle) -- decouverte cette session que ce n'est PAS garanti (ex.
#   inthunt/mame0278 : les deux echouent au-dela de l'adresse 0, "no
#   memory map defined" pour READ_CORE_MEMORY meme a l'adresse 0). Une
#   recette avec read_core_ram_ok=false est correcte mais PAS
#   utilisable telle quelle par ScoreWatch.py sans qu'ils adoptent un
#   mecanisme de lecture alternatif pour ce driver -- a signaler
#   explicitement, pas a cacher.
"""Construit une recette de score verifiee (format RecalBox Challenge)
a partir d'un snapshot hiscore_probe et d'observations vérité d'abord.

Usage:
  hiscore_recipe_builder.py <snapshot.txt> --zone 2 \\
      --observe DEMO_5=1200 --observe DEMO_6=2900 [--observe ...] \\
      [--min-distinct 3] [--out recipe.json]

Au moins 2 --observe sont requis pour qu'une recette soit consideree
verifiee (sinon avertissement explicite, pas de recette produite).
"""
import argparse
import json
import sys


def load(path):
    zones = {}
    snaps = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("ZONE"):
                cols = line.split("\t")
                zones[int(cols[1])] = {
                    "tag": cols[2], "space": cols[3],
                    "astart": int(cols[4], 16), "aend": int(cols[5], 16),
                    "label": cols[6] if len(cols) > 6 else "",
                }
            elif line.startswith("SNAP"):
                cols = line.split("\t")
                phase, zidx, hexdata = cols[1], int(cols[2]), cols[3]
                if hexdata == "ERROR":
                    continue
                snaps[(phase, zidx)] = bytes.fromhex(hexdata)
    return zones, snaps


def encode(value, fmt, endian, length):
    """Meme logique que ramSearch.py::encode() (module Challenge RecalBox)
    -- garde la compatibilite exacte avec leur outil."""
    if fmt == "bcd":
        digits = "{:0{}d}".format(value, length * 2)
        if len(digits) > length * 2:
            return None
        raw = bytes(int(digits[i:i + 2], 16) for i in range(0, len(digits), 2))
    else:
        try:
            raw = value.to_bytes(length, "big")
        except OverflowError:
            return None
    return raw[::-1] if endian == "little" else raw


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("snapshot", help="fichier produit par hiscore_probe (mame_lua_snapshot.txt)")
    ap.add_argument("--zone", type=int, required=True, help="indice de zone (voir lignes ZONE du fichier)")
    ap.add_argument("--observe", action="append", required=True,
                     help="PHASE=valeur (ex: DEMO_5=1200) -- au moins 2 requis pour une recette verifiee")
    ap.add_argument("--out", help="fichier de sortie JSON (sinon affiche sur stdout)")
    ap.add_argument("--romname", default="", help="nom du romset, inclus dans la recette pour tracabilite")
    ap.add_argument("--read-core-live", choices=["yes", "no", "unknown"], default="unknown",
                     help="l'adresse est-elle lisible en direct via READ_CORE_RAM/READ_CORE_MEMORY "
                          "(mecanisme utilise par ScoreWatch.py en vraie session de challenge) -- "
                          "a tester manuellement (voir DECISIONS.md, pas garanti meme a l'adresse 0 "
                          "pour certains drivers), PAS deduit automatiquement par cet outil")
    args = ap.parse_args()

    zones, snaps = load(args.snapshot)
    if args.zone not in zones:
        print("zone {} introuvable dans {} (zones disponibles: {})".format(
            args.zone, args.snapshot, sorted(zones)), file=sys.stderr)
        sys.exit(1)
    zone = zones[args.zone]
    astart = zone["astart"]

    observations = []
    for obs in args.observe:
        if "=" not in obs:
            print("observation invalide (attendu PHASE=valeur): {}".format(obs), file=sys.stderr)
            sys.exit(1)
        phase, val = obs.split("=", 1)
        observations.append((phase, int(val)))

    if len(observations) < 2:
        print("ATTENTION : moins de 2 observations -- une recette basee sur 1 seule valeur")
        print("n'est PAS consideree verifiee (risque de faux positif, voir DECISIONS.md")
        print("le cas 0xe09d5/0xe0bd5 sur inthunt qui matchait 2 valeurs par coincidence).")
        print("Fournir au moins --observe PHASE1=val1 --observe PHASE2=val2.")
        sys.exit(1)

    # verifie que toutes les phases observees existent bien pour cette zone
    length_ref = None
    for phase, _ in observations:
        data = snaps.get((phase, args.zone))
        if data is None:
            print("phase '{}' absente pour la zone {} -- verifier le nom exact (voir".format(phase, args.zone), file=sys.stderr)
            print("les lignes SNAP du fichier)", file=sys.stderr)
            sys.exit(1)
        if length_ref is None:
            length_ref = len(data)
        elif len(data) != length_ref:
            print("tailles de zone incoherentes entre phases -- fichier corrompu ?", file=sys.stderr)
            sys.exit(1)

    formats = ["bcd", "binary"]
    endians = ["big", "little"]
    lengths = [1, 2, 3, 4]
    multipliers = [1, 10, 100]

    matches = []
    for fmt in formats:
        for endian in endians:
            for length in lengths:
                for mult in multipliers:
                    patterns = []
                    ok = True
                    for phase, val in observations:
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
                    # cherche les offsets ou TOUTES les observations matchent au meme offset
                    first_phase, first_raw = patterns[0]
                    first_data = snaps[(first_phase, args.zone)]
                    candidate_offsets = [i for i in range(len(first_data) - length + 1)
                                         if first_data[i:i + length] == first_raw]
                    for phase, raw in patterns[1:]:
                        data = snaps[(phase, args.zone)]
                        candidate_offsets = [off for off in candidate_offsets if data[off:off + length] == raw]
                        if not candidate_offsets:
                            break
                    for off in candidate_offsets:
                        matches.append({"offset": off, "format": fmt, "endian": endian,
                                         "len": length, "multiplier": mult})

    if not matches:
        print("Aucune recette ne matche TOUTES les {} observations simultanement.".format(len(observations)))
        print("Verifier les valeurs lues (erreur de lecture visuelle ?) ou elargir les")
        print("observations (l'adresse peut etre correcte mais l'encodage/multiplicateur")
        print("teste ici insuffisant -- voir DECISIONS.md pour les cas dejà rencontres).")
        sys.exit(2)

    print("{} recette(s) verifiee(s) (matchent {} observations independantes) :".format(
        len(matches), len(observations)))
    recipes = []
    for m in matches:
        addr = astart + m["offset"]
        recipe = {
            "romname": args.romname,
            "addr": "{:x}".format(addr),
            "sysramOffset": "{:x}".format(m["offset"]),
            "len": m["len"],
            "format": m["format"],
            "endian": m["endian"],
            "multiplier": m["multiplier"],
            "verified_on": ["{}={}".format(p, v) for p, v in observations],
            "read_core_live": args.read_core_live,
        }
        recipes.append(recipe)
        print("  addr=0x{:x} format={} endian={} len={} multiplier={}".format(
            addr, m["format"], m["endian"], m["len"], m["multiplier"]))

    if len(recipes) > 1:
        print("\nATTENTION : plusieurs recettes distinctes matchent -- ajouter une")
        print("observation supplementaire (--observe) pour departager, ou verifier")
        print("manuellement laquelle est semantiquement correcte (score vs un autre")
        print("compteur qui progresse pareil par coincidence).")

    if args.out:
        with open(args.out, "w") as f:
            json.dump(recipes, f, indent=2)
        print("\nEcrit dans {}".format(args.out))


if __name__ == "__main__":
    main()
