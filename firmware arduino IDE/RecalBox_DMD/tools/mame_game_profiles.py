#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# mame_game_profiles.py -- adaptation de fbneo_game_profiles.py (v3, deja
# valide en reel sur RB2 pour l'autopilotage fbneo/RB Challenge) au core
# mame0278, pour la campagne niveau1 sur les 195 candidats "hiscore.dat
# confirme mais aucune couverture niveau2" (2026-09-13).
#
# Reprend integralement les regles deja durement apprises sur fbneo :
#   - JAMAIS le D-pad (BTN_DPAD_*) -- lie a des raccourcis RecalBox de
#     niveau systeme, provoque des sauts de contenu vers d'autres roms.
#     Deplacement TOUJOURS via le stick analogique (ABS_X/ABS_Y).
#   - JAMAIS BTN_SOUTH en usage soutenu -- partage le meme index brut que
#     input_menu_toggle_btn de RetroArch, finit par faire sortir la
#     session vers EmulationStation. EAST/NORTH/WEST utilises a la place.
#
# Nouveaute mame0278 : le corpus cible (195 roms, tous des titres
# 1976-1983 : Galaxian/Space Invaders-likes, labyrinthes, casse-briques,
# jeux de tir fixe...) a un profil de genres tres different de la liste
# RB Challenge fbneo (surtout plateforme/beat-em-up/shmup scrolling).
# Nouvelle categorie "fixed_shooter" ajoutee -- LA plus frequente sur ce
# corpus (deja observee visuellement lors de la campagne niveau2 :
# galaxian/invaders/invadpt2/invds/invho2/invinco/invrvnge/ipminvad/
# gmgalax/cosmica/cosmo/spacefev/spaceg/spacezap/orbitron/pisces/
# azurian/ballbomb/uniwars/zigzagb/warofbug/phpython/olibochu/warp1) :
# le vaisseau reste sur une bande horizontale fixe en bas d'ecran, tir
# vertical fixe -- PAS de "shmup_horizontal" (qui suppose un defilement
# horizontal et une esquive verticale, modele inadapte ici).
#
# Resultat campagne 2026-09-13/14 : 51/195 .hi produits (49 non-nuls),
# ~25% de reussite contre ~5% pour l'ancienne methode UDP. Detail dans
# DECISIONS.md.
"""Classifie une rom mame0278 par genre (gamelist.xml officiel Recalbox)
et fournit un pattern d'inputs adapte. Meme API que fbneo_game_profiles :

    from mame_game_profiles import profile_for_rom
    category, play_fn = profile_for_rom("galaxian")
    play_fn(pad, seconds)   # pad = rb2_uinput_gamepad.VirtualGamepad deja cree()
"""
import os
import sys
import time
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rb2_uinput_gamepad import (BTN_EAST, BTN_NORTH, BTN_WEST,  # noqa: E402
                                 ABS_X, ABS_Y)
# BTN_SOUTH volontairement PAS importe/utilise -- voir commentaire d'en-tete.

STICK_MAX = 32000

GAMELIST_PATHS = (
    "/recalbox/share/roms/mame/gamelist.xml",
    "/recalbox/share/roms/mame/mame0278/gamelist.xml",
)

# v2 - 2026-09-14 - safe-modify - BUG REEL trouve en direct sur materiel
# (retour utilisateur : "le steamdeck tente d'ouvrir le clavier virtuel
# en boucle") : `game_metadata(rom)` reparsait l'INTEGRALITE de
# gamelist.xml (31 Mo) a CHAQUE rom (cache uniquement sur le resultat
# final par rom, jamais sur l'arbre XML lui-meme). `/recalbox/share`
# s'est revele etre un montage CIFS reseau (`mount` confirme
# "//192.168.0.50/RECALBOX/ROMS ... type cifs"), donc CHAQUE lecture de
# ce fichier repasse par le reseau -- une micro-coupure pendant l'une de
# ces 1900+ lectures repetees a fait passer le process Python entier en
# etat D (disk sleep, non-interruptible meme par SIGKILL, confirme via
# /proc/<pid>/stack : filemap_update_page/netfs_buffered_read_iter),
# bloquant tout (y compris nos propres gestionnaires de signal, qui ne
# s'executent qu'une fois le noyau libre de le faire) et laissant
# RetroArch orphelin -- cause tres probable de l'incident clavier.
# Fix : index CONSTRUIT UNE SEULE FOIS par process (1 lecture reseau
# pour toute la campagne au lieu d'une par rom), plus de re-parsing.
_INDEX = None  # None = pas encore construit


def _build_index():
    global _INDEX
    index = {}
    for path in GAMELIST_PATHS:
        if not os.path.exists(path):
            continue
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError:
            continue
        for game in root.findall(".//game"):
            raw_path = (game.findtext("path") or "").replace("\\", "/")
            name = os.path.splitext(os.path.basename(raw_path))[0]
            genre = game.findtext("genre") or ""
            existing = index.get(name)
            if existing is None or (not existing["genre"] and genre):
                index[name] = {"name": game.findtext("name") or name, "genre": genre}
    _INDEX = index


def game_metadata(rom):
    if _INDEX is None:
        _build_index()
    return _INDEX.get(rom, {"name": rom, "genre": ""})


def classify(genre_text):
    text = genre_text.casefold()
    # ordre important : du plus specifique au plus generique
    if "space invaders like" in text:
        return "fixed_shooter"
    if "casse" in text or "breakout" in text or "brique" in text:
        return "breakout"
    if "labyrinthe" in text or "pac-man" in text:
        return "maze_action"
    if "course" in text or "conduite" in text or "moto" in text:
        return "racing"
    if "lancer" in text or "puzzle" in text:
        return "puzzle_aim_throw"
    if "fighter scrolling" in text:
        return "beat_em_up"
    if "shooter scrolling" in text:
        return "platform_shooter"
    if "run & jump" in text or ("plateforme" in text and "shooter" not in text
                                 and "fighter" not in text):
        return "platform_pure"
    if "run and gun" in text:
        return "run_and_gun"
    if "horizontal" in text and ("shoot" in text or "tir" in text):
        return "shmup_horizontal"
    if "tir" in text or "shoot" in text:
        # generalite "Tir" sans precision -- beaucoup de ce corpus sont en
        # fait des jeux de tir fixe (cible/periscope/mortier) plutot que
        # des shmups a defilement -- fixed_shooter est un choix plus sur
        # ici (tir continu + balayage lateral leger fonctionne aussi bien
        # sur une cible fixe que sur un vaisseau qui bouge peu).
        return "fixed_shooter"
    return "generic"


def _stick(pad, dx, dy):
    pad.abs_move(ABS_X, dx * STICK_MAX)
    pad.abs_move(ABS_Y, dy * STICK_MAX)


def _stick_center(pad):
    pad.abs_move(ABS_X, 0)
    pad.abs_move(ABS_Y, 0)


def _fixed_shooter(pad, seconds):
    """Galaxian/Space Invaders-like : vaisseau sur une bande horizontale
    fixe en bas d'ecran, tir vertical fixe. Tir QUASI CONTINU
    (indispensable -- sans tir, aucun score) + balayage lateral regulier
    (esquive + couverture de toute la largeur)."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        pad.tap(BTN_EAST, hold=0.03)  # tir a chaque tick
        phase = (tick // 12) % 4
        if phase == 0:
            _stick(pad, 1, 0)
        elif phase == 1:
            _stick(pad, 0, 0)
        elif phase == 2:
            _stick(pad, -1, 0)
        else:
            _stick(pad, 0, 0)
        tick += 1
        time.sleep(0.08)
    _stick_center(pad)


def _breakout(pad, seconds):
    """Casse-briques : PAS de bouton de tir (la balle rebondit seule) --
    seul le mouvement lateral du paddle compte. Balayage regulier gauche/
    droite pour intercepter la balle."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        phase = (tick // 8) % 2
        _stick(pad, 1 if phase == 0 else -1, 0)
        tick += 1
        time.sleep(0.12)
    _stick_center(pad)


def _beat_em_up(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        pad.tap(BTN_EAST, hold=0.04)
        pad.tap(BTN_WEST, hold=0.04)
        phase = (tick // 20) % 4
        if phase == 0:
            _stick(pad, 1, 0)
        elif phase == 1:
            _stick(pad, 1, -1)
        elif phase == 2:
            _stick(pad, -1, 0)
        else:
            _stick(pad, 1, 1)
        tick += 1
        time.sleep(0.1)
    _stick_center(pad)


def _platform_shooter(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        pad.tap(BTN_EAST, hold=0.04)
        _stick(pad, 1 if tick % 15 < 12 else -1, 0)
        if tick % 40 < 5:
            pad.tap(BTN_NORTH, hold=0.04)
        tick += 1
        time.sleep(0.1)
    _stick_center(pad)


def _platform_pure(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        _stick(pad, 1, 0)
        if tick % 10 < 3:
            pad.tap(BTN_EAST, hold=0.04)
        if tick % 25 < 3:
            pad.tap(BTN_WEST, hold=0.04)
        tick += 1
        time.sleep(0.15)
    _stick_center(pad)


def _shmup_horizontal(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        pad.tap(BTN_EAST, hold=0.03)
        phase = (tick // 15) % 4
        if phase == 0:
            _stick(pad, 0, -1)
        elif phase == 1:
            _stick(pad, 0, 1)
        elif phase == 2:
            _stick(pad, 1, -1)
        else:
            _stick(pad, 1, 1)
        tick += 1
        time.sleep(0.08)
    _stick_center(pad)


def _run_and_gun(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        pad.tap(BTN_EAST, hold=0.04)
        _stick(pad, 1, 0)
        if tick % 20 < 4:
            pad.tap(BTN_NORTH, hold=0.04)
        tick += 1
        time.sleep(0.1)
    _stick_center(pad)


def _puzzle_aim_throw(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        aim_phase = tick % 6
        if aim_phase < 2:
            _stick(pad, -1, 0)
        elif aim_phase < 4:
            _stick(pad, 1, 0)
        else:
            _stick_center(pad)
            pad.tap(BTN_EAST, hold=0.04)
        tick += 1
        time.sleep(0.2)
    _stick_center(pad)


def _racing(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        phase = (tick // 15) % 3
        dx = -1 if phase == 1 else (1 if phase == 2 else 0)
        _stick(pad, dx, -1)
        if tick % 10 == 0:
            pad.tap(BTN_EAST, hold=0.04)
        tick += 1
        time.sleep(0.1)
    _stick_center(pad)


def _maze_action(pad, seconds):
    deadline = time.time() + seconds
    dirs = ((1, 0), (0, 1), (-1, 0), (0, -1))
    tick = 0
    while time.time() < deadline:
        dx, dy = dirs[(tick // 15) % 4]
        _stick(pad, dx, dy)
        pad.tap(BTN_EAST, hold=0.04)
        tick += 1
        time.sleep(0.1)
    _stick_center(pad)


def _generic(pad, seconds):
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        pad.tap(BTN_EAST, hold=0.04)
        _stick(pad, 1 if tick % 20 < 12 else -1, 0)
        if tick % 30 < 3:
            pad.tap(BTN_NORTH, hold=0.04)
        tick += 1
        time.sleep(0.1)
    _stick_center(pad)


PROFILES = {
    "fixed_shooter": _fixed_shooter,
    "breakout": _breakout,
    "beat_em_up": _beat_em_up,
    "platform_shooter": _platform_shooter,
    "platform_pure": _platform_pure,
    "shmup_horizontal": _shmup_horizontal,
    "run_and_gun": _run_and_gun,
    "puzzle_aim_throw": _puzzle_aim_throw,
    "racing": _racing,
    "maze_action": _maze_action,
    "generic": _generic,
}


def profile_for_rom(rom):
    meta = game_metadata(rom)
    category = classify(meta["genre"])
    return category, PROFILES[category]


if __name__ == "__main__":
    for rom in sys.argv[1:]:
        meta = game_metadata(rom)
        category, _ = profile_for_rom(rom)
        print("{}: genre={!r} -> categorie={}".format(rom, meta["genre"], category))
