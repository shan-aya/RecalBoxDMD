#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-24 - safe-modify - Creation initiale. PIEGE OPERATIONNEL
#   REEL trouve ce soir (voir DECISIONS.md "Récolte MAME0278") : le
#   chemin `/dev/input/eventN` de la manette "Steam Deck" utilisee par
#   direct_harvest_mame0278_rb2.py, rb2_ram_truth_capture.py et
#   rb2_smart_harvest.py etait code en dur (event14) dans les 3 scripts.
#   Apres un redemarrage physique de RB2, la numerotation kernel des
#   peripheriques d'entree s'est decalee : "Steam Deck" (le vrai
#   controleur, seul avec de vraies capacites bouton EV_KEY non vides)
#   est passe de event14 a event13, et event14 pointait desormais vers
#   "Steam Deck Motion Sensors" (gyroscope, AUCUNE capacite bouton) --
#   emulatorlauncher.pyc plantait alors systematiquement (`KeyError: 1`
#   dans `LibretroControllers._MapUdevControllerButtons` ->
#   `controller.HasKey()`, capacites clavier totalement absentes pour ce
#   peripherique), bloquant TOUT lancement de jeu quel qu'il soit.
#
#   Ce module centralise la detection DYNAMIQUE du bon chemin (au lieu
#   de le deviner/coder en dur) en analysant /proc/bus/input/devices a
#   chaque demarrage de script -- le numero eventN peut a nouveau changer
#   a un prochain reboot, mais le nom "Steam Deck" et ses capacites
#   bouton restent un identifiant stable pour le retrouver.
#
#   Le "physicalpath" (`-p1physicalpath`, utilise tel quel par
#   emulatorlauncher.pyc dans le LAUNCH_TEMPLATE des 3 scripts) N'EST
#   PAS touche par ce bug -- valide en reel le 2026-08-24 : il reflete le
#   chemin USB PHYSIQUE (pci 04:00.4, port 3, interface 2), stable a
#   travers un reboot puisque rien n'a change physiquement, contrairement
#   au numero eventN qui depend de l'ORDRE d'enumeration kernel au boot.
#   Reste donc une constante dans les 3 scripts, seul le devicepath est
#   desormais resolu dynamiquement via ce module.
"""Detecte dynamiquement /dev/input/eventN pour la manette "Steam Deck"
sur RB2, en remplacement d'un chemin code en dur qui casse a chaque
redemarrage (numerotation kernel non stable -- voir DECISIONS.md).
"""
import re


def detect_steam_deck_device(devices_path="/proc/bus/input/devices"):
    """Retourne '/dev/input/eventN' pour le VRAI peripherique "Steam Deck"
    (pas "Steam Deck Motion Sensors", pas "Valve Software Steam
    Controller") -- identifie par son nom EXACT et la presence d'une
    capacite bouton (ligne B: KEY= non vide). Leve RuntimeError si
    introuvable (plutot que de deviner/retomber sur un defaut silencieux
    -- mieux vaut un echec explicite qu'un plantage confus plus loin dans
    emulatorlauncher.pyc, comme ce soir)."""
    with open(devices_path) as f:
        content = f.read()

    for block in content.split("\n\n"):
        if 'N: Name="Steam Deck"' not in block:
            continue
        key_lines = [l for l in block.splitlines() if l.startswith("B: KEY=")]
        if not key_lines:
            continue
        m = re.search(r"\bevent(\d+)\b", block)
        if m:
            return "/dev/input/event{}".format(m.group(1))

    raise RuntimeError(
        "Steam Deck (avec capacites bouton) introuvable dans {} -- "
        "verifier manuellement 'cat /proc/bus/input/devices' (voir "
        "DECISIONS.md 'piege event14/reboot RB2')".format(devices_path))


if __name__ == "__main__":
    print(detect_steam_deck_device())
