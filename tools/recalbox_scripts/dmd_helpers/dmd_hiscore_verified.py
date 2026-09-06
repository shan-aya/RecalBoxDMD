#!/usr/bin/env python3
"""dmd_hiscore_verified.py v1 -- 2026-09-04

Niveau 2 de la cascade hi-score (voir DECISIONS.md et la fiche memoire
du chantier) :

  1. .hi REEL, decode par dmd_hiscore_generic.py (prioritaire, toujours
     verifie EN PREMIER par build_score_payload() dans dmd_score.sh)
  2. table VERIFIEE MANUELLEMENT ici (ce script) -- pour un jeu dont le
     .hi n'a jamais pu etre obtenu (aucune adresse hiscore.dat ne
     s'arme, meme apres une vraie partie credit+jouee) MAIS dont
     l'ecran hi-score du jeu a ete lu directement (capture d'ecran,
     methode "verite d'abord") -- affiche les vraies valeurs du jeu
     sans dependre du mecanisme .hi qui ne fonctionne pas pour ce jeu.
  3. PLACEHOLDER generique shan_aya/RecalBox (dmd_score.sh, repli final)

Ce script est purement un lookup en lecture -- l'ajout d'entrees se
fait via add_verified_score.py (garde le format coherent, evite les
erreurs de saisie a la main dans le JSON).

Convention CLI et convention de sortie IDENTIQUES a
dmd_hiscore_generic.py (positionnels system/rom, une ligne "N NOM
SCORE" par entree jointe par "|", silencieux -- aucune sortie -- si
rien a afficher ou en cas d'erreur) : dmd_score.sh peut donc appeler
les deux scripts de la meme facon.
"""
import json
import os
import sys

DATA_PATH = os.path.join(os.path.dirname(__file__), "verified_default_scores.json")


def load_data():
    with open(DATA_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def main():
    if len(sys.argv) < 3:
        return
    if sys.argv[1].startswith("-"):
        return
    system, rom = sys.argv[1], sys.argv[2]
    try:
        data = load_data()
    except (OSError, json.JSONDecodeError):
        return
    entry = data.get(f"{system}_{rom}")
    if not entry:
        return
    lines = entry.get("lines")
    if not lines:
        return
    print("|".join(lines))


if __name__ == "__main__":
    main()
