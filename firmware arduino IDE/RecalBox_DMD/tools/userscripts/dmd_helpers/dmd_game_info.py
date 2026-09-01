#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v10
#
# v10 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel -- voir
#   dmd_hiscore_generic.py v4 pour le detail complet (meme investigation,
#   meme cause : appele directement par EmulationStation a chaque evenement
#   gamelistbrowsing avec la convention `-action ... -statefile ... -param
#   ...`, jamais concue pour ce fichier -- main() attend des positionnels
#   system/game_path). Ce fichier echoue deja relativement vite dans ce cas
#   (os.path.isfile() sur un chemin gamelist.xml invalide, pas de parsing
#   XML gaspille) mais paie quand meme le demarrage Python a chaque
#   evenement, sans la moindre limite de frequence -- meme fix applique par
#   coherence/defense en profondeur.
#
# v9 - 2026-08-20 - safe-modify - Retour utilisateur : le libelle
#   "Developpeur:" (12 caracteres) est abrege en "Dev:" -- l'ecran INFOS
#   cote firmware n'enveloppe pas la valeur (elle demarre juste apres le
#   libelle sur la meme ligne, jamais coupee sur plusieurs lignes comme
#   DESCRIPTION), donc un libelle long mangeait l'espace disponible pour
#   le nom du studio qui suit, le coupant net en fin d'ecran. Voir
#   field_infos_combined().
#
# v8 - 2026-08-20 - safe-modify - BUG REEL corrige (latent depuis v4/v5) :
#   field_infos_combined() joignait ses lignes avec "\n", convention de
#   l'ANCIEN systeme d'overlay (startGameInfoOverlay(), RecalBox_DMD.ino
#   v92, retire depuis v104) -- jamais mise a jour vers "|" (convention du
#   systeme CMD_SCORE actuel, v110+). Invisible tant qu'aucune coupure de
#   ligne n'etait tentee cote firmware sur ce champ, mais aurait empeche
#   toute pagination correcte de fonctionner dessus (dmd_score.sh v6,
#   send_paginated_lines() -- decoupe explicitement sur "|"). Fix : "|".
#
# v7 - 2026-08-15 - safe-modify - Demande utilisateur : le titre affiche
#   sur le DMD pour ce champ passe de "DESC" a "DESCRIPTION" (label
#   utilise tel quel comme titre par startGameInfoOverlay(), aucun code
#   firmware ne compare sur la valeur "DESC" -- verifie, seul "INFOS" est
#   teste explicitement -- rien d'autre a changer cote firmware).
#
# v6 - 2026-08-15 - safe-modify - Bug reel confirme sur materiel (log
#   dmd_score_mqtt.log, systeme 3do, "Flashback") : find_game_element()
#   ne trouvait AUCUNE correspondance pour un jeu pourtant present dans
#   gamelist.xml -- les ROM ont ete renommees sur disque (espaces retires)
#   sans mise a jour de gamelist.xml, qui garde donc les anciens noms AVEC
#   espaces. Fix : repli en 2e recours (comparaison exacte toujours
#   prioritaire) ignorant les espaces des 2 cotes -- voir find_game_element().
#
# v5 - 2026-08-15 - safe-modify - 3e retour utilisateur : "developpeur" et
#   "editeur" ne doivent JAMAIS etre fusionnes meme s'ils sont identiques
#   ("ce sont 2 infos differentes contenues dans le xml") -- retrait de la
#   dedup ajoutee en v3/v4 ("Dev/Editeur: X" sur 1 seule ligne quand les 2
#   valeurs etaient egales), toujours 2 lignes separees desormais.
#
# v4 - 2026-08-15 - safe-modify - 2e retour utilisateur suite au test
#   materiel de v3 : le champ INFOS combine ("Konami - 1988 - Joueurs:
#   1-2 - Note: 3/5" sur une seule ligne " - ") manquait de retours a la
#   ligne -- chaque element (developpeur, editeur, annee, joueurs, note)
#   doit apparaitre sur SA PROPRE ligne avec son intitule. Fix :
#   field_infos_combined() joint desormais les lignes avec "\n" (au lieu
#   de " - "), chacune prefixee de son intitule ("Developpeur: Konami",
#   "Annee: 1988", etc.) -- le firmware correspondant (RecalBox_DMD.ino
#   v92) reconnait ce "\n" comme une coupure de ligne EXPLICITE dans
#   startGameInfoOverlay(), distincte du retour a la ligne par largeur
#   (seul mecanisme existant avant ce fix, ne respectait aucune coupure
#   voulue).
#
# v3 - 2026-08-15 - safe-modify - Retours utilisateur suite au 1er test
#   materiel de la rotation multi-champs : GENRE supprime (pas assez
#   utile a l'affichage, valeurs souvent tres longues et peu lisibles en
#   scroll DMD -- ex. "Action,Sport / Multisports,Sport,Sport / Course a
#   pied") ; DEV/EDITEUR/ANNEE/JOUEURS/NOTE regroupes en UN SEUL champ
#   combine ("INFOS", ex. "Konami - 1988 - Joueurs: 1-2 - Note: 3/5") au
#   lieu de 4 cartes separees qui se succedaient une par une -- l'effet
#   observe sur materiel reel etait "plusieurs infos d'affilee" (chaque
#   petit fait individuel prenait un tour complet du cycle marquee/hi-score
#   a lui seul). Dev/editeur dedupliques si identiques (cas frequent). Le
#   firmware correspondant (RecalBox_DMD.ino v91) fait tourner DESC et
#   INFOS comme 2 cartes distinctes desormais, plus 7.
#
# v2 - 2026-08-15 - safe-modify - Renomme dmd_synopsis.py -> dmd_game_info.py
#   (demande utilisateur : "synopsis" -> "game_info" en interne, l'afficheur
#   doit faire tourner PLUSIEURS informations du gamelist.xml, pas
#   seulement <desc>). Extrait desormais aussi <genre>, <developer>,
#   <publisher>, <releasedate> (annee seule), <players>, <rating> (converti
#   en note sur 5) -- chaque champ non-vide devient une "carte" que le
#   firmware fait tourner en rotation (meme mecanisme de scroll que le
#   <desc> seul avant). Un seul payload MQTT (marquee/cmd/game_info,
#   renomme depuis marquee/cmd/synopsis) transporte TOUS les champs
#   disponibles pour ce jeu -- le firmware choisit lequel afficher a
#   chaque declenchement, pas ce script (evite une republication MQTT a
#   chaque rotation, le jeu ne change pas entre 2 rotations).
#
# v1 - 2026-08-14 - safe-modify - Creation initiale (dmd_synopsis.py) --
#   voir historique complet dans _backups/ si besoin de le consulter.
#
# Usage : dmd_game_info.py <system> <game_path_absolu>
#   <system>            : nom du systeme RB (ex. "fbneo"), = SystemId de
#                          /tmp/es_state.inf
#   <game_path_absolu>  : chemin ABSOLU de la ROM (ex.
#                          /recalbox/share/roms/fbneo/arcade/1941.zip),
#                          = GamePath de /tmp/es_state.inf
#
# Sortie : sur stdout, un payload "LABEL|contenu|LABEL|contenu|..." (un
#   couple par champ non-vide trouve, dans l'ordre FIELD_ORDER ci-dessous)
#   si au moins un champ est trouve, RIEN (stdout vide) sinon. Code de
#   sortie 0/1 en consequence. Meme convention "affiche TEL QUEL, langue
#   du scraper Recalbox, jamais traduit" que dmd_synopsis.py v1 (choix
#   utilisateur 2026-08-14, toujours valable).
# ============================================

import sys
import os
import re
import unicodedata
import xml.etree.ElementTree as ET

# Plafond du payload COMPLET (tous champs confondus) -- marge sous le
# buffer MQTT firmware (mqttClient.setBufferSize(1024), voir
# RecalBox_DMD.ino v81), qui doit aussi couvrir le topic + les entetes
# MQTT. Les champs sont ajoutes dans l'ordre de FIELD_ORDER jusqu'a
# atteindre ce plafond -- un champ qui ferait deborder est simplement
# omis (pas de coupure au milieu), les champs suivants dans l'ordre sont
# tentes quand meme (ex. DESC omis pour cause de taille n'empeche pas
# GENRE/DEV d'etre inclus s'ils tiennent).
MAX_TOTAL_LEN = 700
# <desc> plafonne INDIVIDUELLEMENT avant assemblage (c'est de loin le
# champ le plus long) -- laisse de la place aux autres champs plutot que
# de tout lui reserver comme avant (dmd_synopsis.py v1, MAX_LEN=600 pour
# lui seul).
MAX_DESC_LEN = 450


def strip_accents(text):
    """Translitteration ASCII (é -> e, ç -> c, œ -> oe approx, etc.) --
    pas d'iconv disponible sur cette Recalbox (verifie), unicodedata
    (stdlib) suffit pour du texte latin standard (FR/EN/ES). Appliquee a
    TOUS les champs texte (pas seulement desc) -- un nom d'editeur/genre
    peut aussi contenir des accents."""
    nfkd = unicodedata.normalize('NFKD', text)
    return nfkd.encode('ascii', 'ignore').decode('ascii')


def clean_text(text):
    text = text.strip()
    text = strip_accents(text)
    return re.sub(r'\s+', ' ', text).strip()


def find_game_element(gamelist_path, target_abs_path):
    try:
        tree = ET.parse(gamelist_path)
    except (ET.ParseError, OSError):
        return None
    root = tree.getroot()
    roms_dir = os.path.dirname(gamelist_path)
    # v6 (2026-08-15, safe-modify) -- bug reel confirme sur materiel (log
    # dmd_score_mqtt.log) : "GAME_INFO skip .../Flashback-TheQuestforIdentity(...).cue
    # (pas de champ exploitable)" alors que gamelist.xml CONTIENT bien une
    # entree pour ce jeu -- <path>Flashback- The Quest for Identity - 38617/
    # Flashback - The Quest for Identity (1994)(U.S. Gold)(EU)(en-fr).cue</path>
    # (AVEC espaces) alors que le fichier reel sur disque (et donc GamePath,
    # lu depuis es_state.inf par EmulationStation) n'en a plus AUCUN --
    # "Flashback-TheQuestforIdentity-38617/Flashback-TheQuestforIdentity(1994)
    # (U.S.Gold)(EU)(en-fr).cue". Les ROM ont ete renommees (espaces retires)
    # sans que gamelist.xml soit mis a jour -- meme phenomene deja documente
    # ailleurs dans ce projet (outil PC RecalBoxDMD_tool.py, sanitize_filename()).
    # Fix : comparaison exacte EN PRIORITE (rapide, fiable), repli en 2e
    # recours seulement (si aucune correspondance exacte trouvee dans tout
    # le fichier) en ignorant les espaces des 2 cotes.
    target_no_space = target_abs_path.replace(' ', '')
    fallback = None
    for game in root.findall('game'):
        path_el = game.find('path')
        if path_el is None or not path_el.text:
            continue
        # <path> est relatif au dossier du gamelist.xml (ex. "arcade/1941.zip"
        # ou "./1941.zip" selon le systeme) -- realpath pour comparer
        # fiablement avec le chemin absolu recu (GamePath de es_state.inf).
        rel = path_el.text
        candidate_abs = os.path.realpath(os.path.join(roms_dir, rel))
        if candidate_abs == target_abs_path:
            return game
        if fallback is None and candidate_abs.replace(' ', '') == target_no_space:
            fallback = game
    return fallback


def field_desc(game):
    el = game.find('desc')
    if el is None or not el.text:
        return None
    text = clean_text(el.text)
    if len(text) > MAX_DESC_LEN:
        cut = text.rfind(' ', 0, MAX_DESC_LEN)
        if cut <= 0:
            cut = MAX_DESC_LEN
        text = text[:cut] + "..."
    return text or None


def field_simple(game, tag):
    el = game.find(tag)
    if el is None or not el.text:
        return None
    text = clean_text(el.text)
    return text or None


def field_year(game):
    el = game.find('releasedate')
    if el is None or not el.text or len(el.text) < 4:
        return None
    year = el.text[:4]
    return year if year.isdigit() else None


def field_rating(game):
    el = game.find('rating')
    if el is None or not el.text:
        return None
    try:
        val = float(el.text)
    except ValueError:
        return None
    stars = round(val * 5)
    if stars < 0 or stars > 5:
        return None
    return f"{stars}/5"


def field_infos_combined(game):
    """Regroupe dev/editeur/annee/joueurs/note en UNE seule carte (v3,
    demande utilisateur suite au 1er test materiel -- voir en-tete). v4
    (2026-08-15, 2e retour utilisateur) : chaque element sur SA PROPRE
    ligne avec son intitule (au lieu d'une seule ligne " - " -- illisible,
    pas de retour a la ligne cote firmware a ce moment-la) -- separateur
    "\\n" reconnu par startGameInfoOverlay() (RecalBox_DMD.ino v92) comme
    une coupure de ligne EXPLICITE, distincte du retour a la ligne par
    largeur utilise pour DESC. v5 (2026-08-15, 3e retour utilisateur) :
    dev/editeur NE SONT PLUS dedupliques meme si identiques -- "ce sont 2
    infos differentes contenues dans le xml", toujours affichees separement
    (retrait de la dedup ajoutee en v3/v4, jugee a tort utile)."""
    dev = field_simple(game, 'developer')
    pub = field_simple(game, 'publisher')
    year = field_year(game)
    players = field_simple(game, 'players')
    rating = field_rating(game)

    lines = []
    if dev:
        # v9 -- "Developpeur:" (12 caracteres) abrege en "Dev:" (retour
        # utilisateur : le nom du studio qui suit etait coupe -- l'ecran
        # INFOS (RecalBox_DMD.ino) n'enveloppe PAS la valeur, elle demarre
        # juste apres le libelle sur la MEME ligne (x = 1 + len(libelle)*6+4
        # en police classique) -- "Developpeur:" a lui seul ne laissait que
        # ~51px (~8 caracteres) pour le nom du studio avant le bord droit
        # de l'ecran 128px. Les autres libelles (Editeur/Annee/Joueurs/Note)
        # sont deja assez courts, inchanges.
        lines.append(f"Dev: {dev}")
    if pub:
        lines.append(f"Editeur: {pub}")
    if year:
        lines.append(f"Annee: {year}")
    if players:
        lines.append(f"Joueurs: {players}")
    if rating:
        lines.append(f"Note: {rating}")

    return "|".join(lines) if lines else None


# Ordre d'inclusion dans le payload -- aussi l'ordre de rotation cote
# firmware (voir startGameInfoOverlay()/RecalBox_DMD.ino). DESC en premier
# (contenu principal), INFOS ensuite (v3 -- GENRE supprime, dev/editeur/
# annee/joueurs/note regroupes en une seule carte, voir en-tete).
FIELD_ORDER = [
    ("DESCRIPTION", field_desc),
    ("INFOS", field_infos_combined),
]


def build_payload(game):
    parts = []
    total = 0
    for label, extractor in FIELD_ORDER:
        value = extractor(game)
        if not value:
            continue
        chunk = f"{label}|{value}|"
        if total + len(chunk) > MAX_TOTAL_LEN:
            continue  # ce champ ferait deborder -- omis, mais les suivants sont quand meme tentes
        parts.append(chunk)
        total += len(chunk)
    return "".join(parts)


def main():
    if len(sys.argv) < 3:
        return 1
    # v10 -- voir changelog v10 en tete de fichier : sortie immediate si
    # invoque avec la convention native ES ("-action ..."), jamais celle de
    # dmd_score.sh (positionnels, jamais prefixes par "-").
    if sys.argv[1].startswith("-"):
        return 1
    system = sys.argv[1]
    game_path = sys.argv[2]

    gamelist_path = f"/recalbox/share/roms/{system}/gamelist.xml"
    if not os.path.isfile(gamelist_path):
        return 1

    target_abs = os.path.realpath(game_path)
    game = find_game_element(gamelist_path, target_abs)
    if game is None:
        return 1

    payload = build_payload(game)
    if not payload:
        return 1

    print(payload)
    return 0


if __name__ == '__main__':
    sys.exit(main())
