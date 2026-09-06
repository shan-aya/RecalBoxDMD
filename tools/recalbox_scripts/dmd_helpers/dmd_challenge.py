#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v3
#
# v3 - 2026-09-05 - safe-modify - BUG REEL confirme sur materiel RB1 (retour
#   utilisateur : "RB cHALLENGE ne fonctionne pas sur le challenge actuel
#   rb1"). Challenge actif au moment du signalement : "Z EVENT - Blade
#   Buster" (system=nes, core=fceumm, current.json bien present avec 100
#   entrees de classement -- le challenge tourne reellement). Cause :
#   roms[].name dans current.json valait "BladeBuster (High Level
#   Challenge)" -- un NOM D'AFFICHAGE, alors que `rom` recu ici est le nom
#   de FICHIER sans extension (bladebuster.zip -> "bladebuster", voir
#   dmd_score.sh: `basename "$game_path" | sed 's/\.[^.]*$//'`). L'egalite
#   stricte `rom not in roms` (v1/v2) echouait donc TOUJOURS pour ce
#   challenge, malgre une session active -- aucun panneau RB CHALLENGE
#   affiche. Le seul test materiel fait jusqu'ici (Blazing Star, voir
#   changelog dmd_score.sh v17/v23) fonctionnait par PURE COINCIDENCE : son
#   nom d'affichage etait identique a son nom de fichier une fois
#   normalise -- ce n'est pas garanti en general (annotations type "(High
#   Level Challenge)", "2600", editions, etc. frequentes sur les Z EVENTS).
#   Fix : comparaison normalisee (accents/casse/ponctuation retires) ET
#   tolerante -- match si le nom de fichier normalise est CONTENU DANS le
#   nom d'affichage normalise (couvre le cas annote ci-dessus) ou egal
#   (couvre le cas Blazing Star, comportement inchange). Voir
#   _normalize_id()/_rom_matches_challenge().
#
# v2 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel -- voir
#   dmd_hiscore_generic.py v4 pour le detail complet (meme investigation,
#   meme cause : appele directement par EmulationStation a chaque evenement
#   gamelistbrowsing avec la convention `-action ... -statefile ... -param
#   ...`, jamais concue pour ce fichier -- main() attend des positionnels
#   system/rom). Paie le demarrage Python + l'ouverture/parsing de
#   current.json a chaque evenement, sans la moindre limite de frequence.
#
# v1 - 2026-08-22 - safe-modify - Creation. Lit le classement communautaire
#   du "Challenge" Recalbox du mois en cours (fonctionnalite officielle RB,
#   1 seul jeu choisi par Recalbox chaque mois, score valide sur 1 credit
#   sans continue -- voir memoire projet pour l'exploration complete de
#   /usr/lib/python3.11/site-packages/configgen/challenge/). Le fichier
#   local /recalbox/share/system/challenges/current.json (ecrit par
#   ScoreWatch.py/NetCmd.py cote RB, rafraichi automatiquement pendant une
#   session de challenge active) contient DEJA le classement en JSON tout
#   pret -- AUCUN decodage de fichier de sauvegarde necessaire ici (a la
#   difference du chantier hi-score FBNeo/MAME general, toujours en
#   attente d'une reponse de l'equipe RB). Demande utilisateur explicite :
#   afficher le classement EN LIGNE (pas le score local du joueur), en
#   round-robin avec le marquee pendant la partie, MAJ suivant le fichier
#   (RB le rafraichit deja tout seul cote serveur pendant une session
#   active). Toujours actif par defaut, PAS de reglage web dedie pour ce
#   v1 (decision utilisateur explicite, "toujours actif" -- ajustable plus
#   tard si besoin, meme pattern que les autres reglages hiscore/infos/
#   description si un jour necessaire).
import json
import re
import sys
import unicodedata

CHALLENGE_FILE = "/recalbox/share/system/challenges/current.json"
# Aligne sur MAX_PAGES(3) x LINES_PER_PAGE(3) cote dmd_score.sh -- pas la
# peine de renvoyer plus d'entrees que ce que la pagination existante peut
# de toute facon afficher.
MAX_ENTRIES = 9


def strip_accents(s):
    return "".join(c for c in unicodedata.normalize("NFD", s) if unicodedata.category(c) != "Mn")


def _normalize_id(s):
    """Reduit un nom de rom/jeu a son squelette alphanumerique (accents,
    casse, espaces, parentheses et ponctuation retires) pour comparer un
    nom de FICHIER (ex. "bladebuster") a un nom d'AFFICHAGE potentiellement
    annote (ex. "BladeBuster (High Level Challenge)") -- voir changelog v3."""
    s = strip_accents(s or "").lower()
    return re.sub(r"[^a-z0-9]", "", s)


def _rom_matches_challenge(rom, display_names):
    """Vrai si `rom` (nom de fichier sans extension) designe bien l'un des
    jeux du challenge (`display_names` = roms[].name de current.json).
    Egalite normalisee (cas Blazing Star, ou nom de fichier == nom
    d'affichage) OU nom de fichier normalise CONTENU DANS le nom
    d'affichage normalise (cas Blade Buster, nom d'affichage annote)."""
    norm_rom = _normalize_id(rom)
    if not norm_rom:
        return False
    for name in display_names:
        norm_name = _normalize_id(name)
        if norm_name and (norm_rom == norm_name or norm_rom in norm_name):
            return True
    return False


def clean_name(raw):
    """Normalise un nom de joueur pour l'ecran DMD (police classique, pas
    d'accents/caracteres non geres) -- meme esprit que normalize_text()
    dans dmd_game_info.py, mais plus strict (le champ 'name' du classement
    Recalbox est libre, pas garanti alphanumerique/majuscule comme les
    tables hi-score arcade d'origine)."""
    name = strip_accents(raw or "").upper()
    name = re.sub(r"[^A-Z0-9 ]", "", name).strip()
    return name[:10] if name else "?"


def main():
    if len(sys.argv) < 3:
        return
    # v2 -- voir changelog v2 en tete de fichier : sortie immediate si
    # invoque avec la convention native ES ("-action ..."), jamais celle de
    # dmd_score.sh (positionnels, jamais prefixes par "-").
    if sys.argv[1].startswith("-"):
        return
    system, rom = sys.argv[1], sys.argv[2]

    try:
        with open(CHALLENGE_FILE, encoding="utf-8") as f:
            data = json.load(f)
    except Exception:
        return

    # Le challenge du mois ne concerne QU'UN SEUL jeu -- verifie que la
    # partie en cours correspond bien, sinon un current.json perime (mois
    # precedent, jamais nettoye) afficherait le mauvais classement sur
    # n'importe quel autre jeu.
    if data.get("system") != system:
        return
    display_names = [r.get("name") for r in data.get("roms", [])]
    if not _rom_matches_challenge(rom, display_names):
        return

    leaderboard = data.get("leaderboard", [])
    if not leaderboard:
        return

    lines = []
    for i, entry in enumerate(leaderboard[:MAX_ENTRIES], start=1):
        name = clean_name(entry.get("name") or entry.get("nickname", ""))
        score = entry.get("score", 0)
        lines.append(f"{i} {name} {score}")

    print("|".join(lines))


if __name__ == "__main__":
    main()
