#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-31 - safe-modify - Creation initiale. Reprend un principe
#   deja amorce cote Codex (rb2_fbneo_recipe_capture.py::PLAY_PROFILES/
#   profile_for(), jamais committe dans tools/) sur demande explicite de
#   l'utilisateur : "on avait commence a creer un guide des regles du
#   jeu pour chaque type de jeu (selon gamelist.xml) pour t'aider a
#   scorer correctement selon le type de jeu et le deplacement
#   (horizontal-vertical)". Categories affinees et validees par
#   observation DIRECTE cette meme nuit (pas juste devinees) :
#   - kamenrid ("Course, Conduite") : PAS un jeu de combat -- accelerer
#     + diriger, confirme par capture ecran reelle (vue circuit/vitesse).
#   - dynagear ("Fighter Scrolling") : score reste a 0 avec un pattern
#     generique trop pauvre en boutons d'attaque (verifie en reel,
#     obs05 : boss combattu, TIME 85, score encore 00) -- corrige avec
#     un pattern beat-em-up dedie (attaque quasi continue).
#   - pzloop2/msgogo ("Puzzle-Game / Lancer") : PAS un jeu de
#     deplacement continu -- viser (gauche/droite bref) puis tirer,
#     cycle repete. Genre "Lancer" (= lancer/tirer une bille) confirme
#     par gamelist.xml, coherent avec le gameplay Puzzle Bobble-like
#     deja observe sur pzloop2 cette session.
#   Genres officiels releves via gamelist.xml (2026-08-31, liste RB
#   Challenge complete) -- voir DECISIONS.md "Suite immediate (22)"
#   pour le detail brut.
"""Classifie une rom fbneo par genre (gamelist.xml) et fournit un
pattern d'inputs adapte -- evite le piege d'un pattern generique unique
qui ne fait PAS avancer le score sur la moitie des genres (confirme en
reel : un beat-em-up a besoin d'attaque quasi continue, un puzzle a
besoin de viser puis tirer par a-coups, PAS de mouvement continu, une
course a besoin d'accelerer+diriger, PAS de bouton d'attaque).

Usage (import) :
    from fbneo_game_profiles import profile_for_rom
    category, play_fn = profile_for_rom("dynagear")
    play_fn(send, seconds)   # send = fonction qui envoie 1 commande UDP
"""
import os
import time
import xml.etree.ElementTree as ET

GAMELIST_PATHS = (
    "/recalbox/share/roms/fbneo/gamelist.xml",
    "/recalbox/share/roms/fbneo/arcade/gamelist.xml",
)

# Genres officiels observes sur la liste RB Challenge (2026-08-31) --
# reference brute pour ajuster les mots-cles ci-dessous si un nouveau
# genre inconnu apparait :
#   pzloop2  : "Puzzle-Game,Puzzle-Game / Lancer"
#   mtwins   : "Plateforme / Fighter Scrolling,Plateforme"
#   willow   : "Plateforme,Plateforme / Shooter Scrolling"
#   gogomile : "Action / Labyrinthe,Action"
#   kamenrid : "Course, Conduite,Course, Conduite / Moto"
#   dynagear : "Plateforme,Plateforme / Fighter Scrolling"
#   jjsquawk : "Plateforme,Plateforme / Shooter Scrolling"
#   nemo     : "Plateforme,Plateforme / Fighter Scrolling"
#   inthunt  : "Tir,Tir / Horizontal,Shoot'em up / Horizontal,..."
#   msgogo   : "Puzzle-Game,Puzzle-Game / Lancer"
#   progear  : "Shoot'em up / Horizontal,Shoot'em Up"
#   joemacr  : "Plateforme,Plateforme / Run & Jump"
#   gbusters : "Tir,Tir / Run and Gun"
#   osman    : "Plateforme,Plateforme / Fighter Scrolling"


def game_metadata(rom):
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
            if name != rom:
                continue
            return {"name": game.findtext("name") or rom,
                    "genre": game.findtext("genre") or ""}
    return {"name": rom, "genre": ""}


def classify(genre_text):
    text = genre_text.casefold()
    # ordre important : du plus specifique au plus generique
    if "course" in text or "conduite" in text or "moto" in text:
        return "racing"
    if "lancer" in text or "puzzle" in text:
        return "puzzle_aim_throw"
    if "labyrinthe" in text:
        return "maze_action"
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
        return "shmup_horizontal"
    return "generic"


# --- patterns d'inputs par categorie -----------------------------------
# Chaque fonction recoit (send, seconds) et boucle jusqu'a expiration.
# `send(cmd)` envoie UNE commande UDP fire-and-forget (ex. "PLAYER1_B").

def _beat_em_up(send, seconds):
    """Marche + attaque QUASI CONTINUE (retour d'observation directe :
    un pattern trop pauvre en boutons laisse le score a 0 meme en plein
    combat de boss)."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_B")
        send("PLAYER1_A")
        phase = (tick // 20) % 4
        if phase == 0:
            send("PLAYER1_RIGHT")
        elif phase == 1:
            send("PLAYER1_RIGHT")
            send("PLAYER1_UP")
        elif phase == 2:
            send("PLAYER1_LEFT")
        else:
            send("PLAYER1_RIGHT")
            send("PLAYER1_DOWN")
        tick += 1
        time.sleep(0.1)


def _platform_shooter(send, seconds):
    """Marche + tir a distance quasi continu (willow/jjsquawk :
    scrolling horizontal, arme a portee)."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_B")
        if tick % 15 < 12:
            send("PLAYER1_RIGHT")
        else:
            send("PLAYER1_LEFT")
        if tick % 40 < 5:
            send("PLAYER1_A")  # saut ponctuel (obstacles/plateformes)
        tick += 1
        time.sleep(0.1)


def _platform_pure(send, seconds):
    """Plateforme pure (joemacr, run & jump) : avance + saut regulier,
    attaque ponctuelle."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_RIGHT")
        if tick % 10 < 3:
            send("PLAYER1_A")  # saut
        if tick % 25 < 3:
            send("PLAYER1_B")  # attaque/action ponctuelle
        tick += 1
        time.sleep(0.15)


def _shmup_horizontal(send, seconds):
    """Shoot'em up horizontal (inthunt/progear/gbusters) : tir CONTINU
    (indispensable, un shmup sans tir continu ne marque jamais de
    points), deplacement vertical dominant (esquive), le scrolling
    horizontal est automatique donc pas besoin d'aller vers la droite."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_B")  # tir -- envoye a CHAQUE tick, pas en option
        phase = (tick // 15) % 4
        if phase == 0:
            send("PLAYER1_UP")
        elif phase == 1:
            send("PLAYER1_DOWN")
        elif phase == 2:
            send("PLAYER1_UP")
            send("PLAYER1_RIGHT")
        else:
            send("PLAYER1_DOWN")
            send("PLAYER1_RIGHT")
        tick += 1
        time.sleep(0.08)


def _run_and_gun(send, seconds):
    """Run and gun (gbusters) : avance + tir continu, saut ponctuel."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_B")
        send("PLAYER1_RIGHT")
        if tick % 20 < 4:
            send("PLAYER1_A")
        tick += 1
        time.sleep(0.1)


def _puzzle_aim_throw(send, seconds):
    """Puzzle-Game / Lancer (pzloop2/msgogo) : PAS de deplacement
    continu -- vise (gauche/droite bref) puis tire, cycle repete.
    Un pattern de "marche" ici ne sert a rien (le personnage/canon est
    fixe, seul l'angle de visee bouge)."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        aim_phase = tick % 6
        if aim_phase < 2:
            send("PLAYER1_LEFT")
        elif aim_phase < 4:
            send("PLAYER1_RIGHT")
        else:
            send("PLAYER1_B")  # tir/lancer
        tick += 1
        time.sleep(0.2)


def _racing(send, seconds):
    """Course/Conduite (kamenrid) : accelerer (maintenu) + diriger --
    PAS de bouton d'attaque au sens classique, confirme par observation
    directe (vue circuit/vitesse, pas de combat)."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_UP")  # accelerer, maintenu en continu
        phase = (tick // 15) % 3
        if phase == 1:
            send("PLAYER1_LEFT")
        elif phase == 2:
            send("PLAYER1_RIGHT")
        if tick % 10 == 0:
            send("PLAYER1_B")  # arme/objet ponctuel si le jeu en a un
        tick += 1
        time.sleep(0.1)


def _maze_action(send, seconds):
    """Action/Labyrinthe (gogomile) : deplacement omnidirectionnel +
    attaque, pas de direction dominante claire (labyrinthe)."""
    deadline = time.time() + seconds
    dirs = ("PLAYER1_RIGHT", "PLAYER1_DOWN", "PLAYER1_LEFT", "PLAYER1_UP")
    tick = 0
    while time.time() < deadline:
        send(dirs[(tick // 15) % 4])
        send("PLAYER1_B")
        tick += 1
        time.sleep(0.1)


def _generic(send, seconds):
    """Repli pour un genre non reconnu -- melange large, moins efficace
    que les patterns dedies mais evite de rester totalement passif."""
    deadline = time.time() + seconds
    tick = 0
    while time.time() < deadline:
        send("PLAYER1_B")
        if tick % 20 < 12:
            send("PLAYER1_RIGHT")
        else:
            send("PLAYER1_LEFT")
        if tick % 30 < 3:
            send("PLAYER1_A")
        tick += 1
        time.sleep(0.1)


PROFILES = {
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
    """Retourne (categorie, fonction_de_jeu) pour une rom donnee."""
    meta = game_metadata(rom)
    category = classify(meta["genre"])
    return category, PROFILES[category]


if __name__ == "__main__":
    import sys
    for rom in sys.argv[1:]:
        meta = game_metadata(rom)
        category, _ = profile_for_rom(rom)
        print("{}: genre={!r} -> categorie={}".format(rom, meta["genre"], category))
