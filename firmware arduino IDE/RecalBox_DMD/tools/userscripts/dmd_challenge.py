#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
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
    roms = [r.get("name") for r in data.get("roms", [])]
    if rom not in roms:
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
