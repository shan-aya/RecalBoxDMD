#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-31 - safe-modify - Creation initiale. Decouverte majeure
#   (retour utilisateur : "envisager un pilotage de fbneo comme mame
#   pour jouer des vraies parties tout seul") : le fichier
#   /recalbox/share/system/configs/retroarch/cores/retroarch-core-
#   options.cfg contient deja, PAR JEU, les dip switches FBNeo au
#   format texte simple `fbneo-dipswitch-<rom>-<Nom_Switch> = "<valeur>"`
#   -- POPULE AUTOMATIQUEMENT par RetroArch/le core au premier lancement
#   de chaque rom (observe : tous nos jeux testes cette session y sont
#   deja, avec leurs vrais choix de dip -- Coin_A/Coin_B/Coinage/
#   Free_Play/Lives/Difficulty/etc., valeurs LISIBLES EN CLAIR).
#
#   Certains jeux exposent un boolean `Free_Play` dedie (dynagear,
#   mtwins, nemo confirmes) ; d'autres seulement `Coinage` ou
#   `Coin_A`/`Coin_B` (motif MAME standard : "Free Play" est souvent une
#   valeur possible de la meme liste que "1 Coin 1 Credit" etc., pas un
#   switch separe).
#
#   VALIDE EN REEL sur `dynagear` (2026-08-31) : Free_Play mis a "On",
#   relance, UN SEUL appui PLAYER1_START (sans PLAYER1_SELECT ni aucune
#   simulation de piece) -> VRAIE PARTIE demarree ("AREA 1 JUNGLE GO!",
#   gameplay reel avec inputs generiques). Resout DEFINITIVEMENT le
#   probleme d'automatisation credit qui avait fait echouer 3 tentatives
#   differentes plus tot dans la nuit (voir DECISIONS.md "Suite
#   immediate 20") -- le probleme n'etait jamais la simulation d'input
#   START elle-meme, mais l'ABSENCE DE CREDIT PREALABLE (le driver ne
#   demarre jamais sur un simple START sans credit reel enregistre, et
#   la simulation de piece via PLAYER1_SELECT ne fonctionne
#   structurellement pas sur ce core -- Free Play contourne le probleme
#   entierement en rendant le credit inutile).
#
#   Test sur `gogomile` (Coinage, pas de Free_Play dedie) : INCONCLUANT
#   (intro trop longue pour le delai de test utilise, jamais reconfirme
#   avec un delai plus long) -- ne PAS presumer que `Coinage="Free Play"`
#   fonctionne pour tous les jeux sans reverifier au cas par cas.
"""Bascule le dip switch de credit d'un jeu fbneo sur Free Play, en
editant directement le fichier texte retroarch-core-options.cfg AVANT
le lancement -- essaie Free_Play (boolean dedie) en premier, puis
Coinage, puis Coin_A+Coin_B (valeur "Free Play", motif MAME standard).

Usage : set_freeplay.py <rom> [--revert]
  --revert : remet la valeur d'origine (Off / "1 Coin  1 Credit(s)")
             plutot que d'activer free play -- PAS automatique, il faut
             connaitre/redonner la valeur d'origine manuellement pour
             l'instant (ce script ne sauvegarde pas l'ancienne valeur).
"""
import sys

PATH = "/recalbox/share/system/configs/retroarch/cores/retroarch-core-options.cfg"
CANDIDATE_FIELDS = ["Free_Play", "Coinage", "Coin_A", "Coin_B"]


def main():
    rom = sys.argv[1]
    with open(PATH, encoding="utf-8") as f:
        lines = f.readlines()

    changed = []
    for field in CANDIDATE_FIELDS:
        key = "fbneo-dipswitch-{}-{} = ".format(rom, field)
        for i, line in enumerate(lines):
            if line.startswith(key):
                value = "On" if field == "Free_Play" else "Free Play"
                old = line[len(key):].strip()
                lines[i] = '{}"{}"\n'.format(key, value)
                changed.append((field, old, value))

    if not changed:
        print("AUCUN dip switch de credit trouve pour '{}' (jeu jamais lance ? "
              "verifier le fichier manuellement)".format(rom))
        return 1

    with open(PATH, "w", encoding="utf-8") as f:
        f.writelines(lines)

    for field, old, new in changed:
        print("{}: {} -> {}".format(field, old, new))
    print("OK -- {} champ(s) modifie(s) pour '{}'".format(len(changed), rom))
    return 0


if __name__ == "__main__":
    sys.exit(main())
