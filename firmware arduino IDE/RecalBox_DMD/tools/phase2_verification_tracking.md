# Suivi vérification hi-score (méthode "vérité d'abord" via capture d'écran)

Méthode : lancer le jeu directement (`emulatorlauncher.pyc`), attendre le
chargement, envoyer la commande réseau RetroArch `SCREENSHOT` (UDP
127.0.0.1:55355), lire l'image réelle (score/nom affichés), chercher dans le
`.hi` réel la position exacte qui reproduit cette valeur EXACTEMENT (score
BCD ou binaire, avec ou sans facteur d'échelle ×10/×100/×1000/×10000) —
outils `build_entry_from_truth.py`/`find_correct_offset.py` (scratchpad
session, non commités — à recréer si besoin, motif documenté dans
DECISIONS.md). Bien plus fiable que l'heuristique statistique de
`phase2_static_analyze.py` seule.

## Corrigés et validés (score + nom confirmés à l'écran ou par motif propre)

- **ikari** — score ET nom confirmés pixel-pour-pixel contre une capture
  (BEST10 RANKING : 30000/28000/27000/26000/25000/24000/23000/22000/21000,
  tous "IKARI"). Structure : score 1 octet BCD ×1000 à l'offset relatif 1,
  nom 5 caractères à l'offset relatif 3, stride 8, 10 entrées.
- **cotton** — score confirmé (68000, écran titre "TOP 68000"), nom
  reconstitué par motif propre (BIG+COT+TON+PAN+TSU, cohérent). Score en
  **binaire brut** (pas BCD) 3 octets, offset relatif 1 ; nom 3 caractères
  offset relatif 6 ; stride 10.
- **ninjemak** — score confirmé (100000, écran de jeu "HI-SCORE 100000"),
  nom reconstitué par motif propre (NIN+JA+EMA+KI+TER ≈ "NINJA EMAKI",
  titre du jeu). Score 1 octet BCD ×10000 offset relatif 1 ; nom 3
  caractères offset relatif 6 ; stride 13.

## Corrigés PARTIELLEMENT — score confirmé, nom NON confirmé (a verifier)

Ces jeux affichent maintenant le bon SCORE (confirmé exact contre une vraie
capture d'écran), mais le NOM affiché est une déduction structurelle non
vérifiée visuellement (pas de tableau de classement avec noms atteint à
l'écran pendant le test) :

- **bonzeadv** — score confirmé exact (50000, écran titre "HIGH SCORE
  50000"). Nom "SSB"/"TOM"/"TET"/"KAI"/"HP" déduit par motif (même stride
  10 que le score, position symétrique à cotton) mais **jamais vu à
  l'écran** — a verifier en laissant le jeu atteindre un vrai tableau de
  classement (pas juste l'écran titre).
- **citybomb** — score confirmé exact (57300, écran titre "HI 57300").
  Aucun nom trouvé du tout (pas de motif ASCII clair à proximité) — entrée
  actuelle en **score seul** (`mode: score_only`, pas de champ NAME). A
  chercher plus avant si un tableau de classement avec noms existe pour ce
  jeu.

## Non résolus — nécessitent une investigation plus approfondie

- **starjack** — le fichier `.hi` ne contient QUE le nom par défaut
  ("STARJACKER", reparti sur 5 blocs de 6 octets) — aucun octet ne varie
  entre les rangs pour représenter un score distinct. Même mystère que
  **64street** (documenté précédemment, session du 22-23/08) : le score
  affiché à l'écran (30000) n'est simplement PAS présent dans le fichier
  sous une forme BCD/binaire directe détectée. Piste non explorée :
  peut-être un encodage plus exotique (offset+delta, table de score fixe
  non stockée par rang, etc.) — nécessiterait une lecture RAM live pour
  confirmer où vit réellement cette donnée.
- **mnight** (Mutant Night) — le lancement de test est tombé sur l'écran de
  **service/configuration** (menu operator) au lieu de l'attract/gameplay —
  jamais vu le vrai écran de score. A refaire avec un délai différent ou en
  s'assurant qu'aucun bouton "test" n'est activé.

## Reste à traiter (lot complet du 23/08, jamais repassés par cette méthode)

Tous les autres jeux de la fusion Phase 2 FBNeo de cette nuit (80 jeux
"FORTE" fusionnés le 23/08 au matin, source `phase2-static-heuristic-batch2`
dans `hiscore_manifest.json`) restent sur l'ancienne heuristique statistique
uniquement — **jamais vérifiés contre une vraie capture d'écran**. Idem pour
les ~349 "FAIBLE" et 135 "moyenne" jamais fusionnés du tout (rejetés par
l'heuristique, potentiellement récupérables avec cette méthode "vérité
d'abord" — voir le cas ikari qui aurait été raté par l'heuristique seule).
Prioriser les jeux les plus connus/joués en premier.

**Découverte généralisable importante pour la suite** : l'heuristique
statistique de `phase2_static_analyze.py` ne teste JAMAIS :
1. Un champ score étroit (1-2 octets) avec facteur d'échelle (×10/100/
   1000/10000) — cause du échec sur `ikari`/`ninjemak`.
2. Un encodage **binaire brut** (pas BCD) — cause de l'échec sur `cotton`.

Ces 2 lacunes touchent probablement une part significative des jeux classés
"FAIBLE"/"moyenne" par l'heuristique — un futur passage pourrait étendre
`phase2_static_analyze.py` pour les tester automatiquement, MAIS la méthode
"vérité d'abord" (capture réelle + recherche exhaustive contre une valeur
CONNUE) reste strictement plus fiable qu'une heuristique statistique élargie,
qui resterait probabiliste. A privilégier pour tout nouveau jeu traité.
