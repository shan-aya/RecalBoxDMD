# Kit de localisation FR/ES -- systems/_defaults (logos genres DMD)

Chaque logo fait 128x32 px (RGB565 sur la carte SD). L'icone a gauche NE DOIT PAS changer -- seule la zone de texte a droite doit etre recreee dans la langue cible, en gardant le style graphique (police, couleurs, degrade, contour, effet -- flammes/cuir/pierre/neon selon le genre) visible sur la reference EN (et FR quand disponible).

## Format de sortie attendu
- PNG 128x32 (ou plus grand puis redimensionne), fond noir pur (#000000) hors des sujets.
- Nommer le fichier `<stem>.png` (voir colonne "Fichier").
- Une fois les PNG recus : les convertir en `.raw565` (RGB565 little-endian, 128x32, sans en-tete) et les deposer dans :
  - FR -> `sd_card/systems/_defaults/fr/<stem>.raw565`
  - ES -> `sd_card/systems/_defaults/es/<stem>.raw565`
  (script de conversion : `python ../../raw565_convert.py encode <src.png> <dst.raw565>` -- voir `firmware arduino IDE/RecalBox_DMD/tools/raw565_convert.py`)

## Table de traduction

| Fichier | Ref. EN | Ref. FR (si dispo) | FR a creer ? | Texte FR | Texte ES |
|---|---|---|---|---|---|
| `genre-action` | en_reference/genre-action.png | — | OUI | Action | Acción |
| `genre-actionbattleroyale` | en_reference/genre-actionbattleroyale.png | — | OUI | Battle Royale | Battle Royale |
| `genre-actionbeatemup` | en_reference/genre-actionbeatemup.png | — | OUI | Beat'em up | Beat'em up |
| `genre-actionfighting` | en_reference/genre-actionfighting.png | — | OUI | Combat | Lucha |
| `genre-actionfirstpersonshooter` | en_reference/genre-actionfirstpersonshooter.png | — | OUI | Tir à la première personne | Disparos en primera persona |
| `genre-actionplatformer` | en_reference/genre-actionplatformer.png | — | OUI | Plateforme | Plataformas |
| `genre-actionplatformshooter` | en_reference/genre-actionplatformshooter.png | — | OUI | Plateforme-Tir | Plataformas de disparos |
| `genre-actionrythm` | en_reference/genre-actionrythm.png | — | OUI | Rythme & Musique | Ritmo y Música |
| `genre-actionshootemup` | en_reference/genre-actionshootemup.png | — | OUI | Shoot'em up | Shoot'em up |
| `genre-actionshootwithgun` | en_reference/genre-actionshootwithgun.png | — | OUI | Tir au pistolet | Disparo con pistola |
| `genre-actionstealth` | en_reference/genre-actionstealth.png | — | OUI | Infiltration | Sigilo |
| `genre-adventure` | en_reference/genre-adventure.png | fr_reference/genre-adventure.png | non (déjà fait) | Aventure | Aventura |
| `genre-adventuregraphics` | en_reference/genre-adventuregraphics.png | — | OUI | Aventure graphique | Aventura gráfica |
| `genre-adventureinteractivemovie` | en_reference/genre-adventureinteractivemovie.png | — | OUI | Film interactif | Película interactiva |
| `genre-adventurerealtime3d` | en_reference/genre-adventurerealtime3d.png | fr_reference/genre-adventurerealtime3d.png | non (déjà fait) | Aventure 3D Temps Réel | Aventura 3D en Tiempo Real |
| `genre-adventuresurvivalhorror` | en_reference/genre-adventuresurvivalhorror.png | — | OUI | Survival Horror | Terror de supervivencia |
| `genre-adventuretext` | en_reference/genre-adventuretext.png | — | OUI | Aventure textuelle | Aventura textual |
| `genre-adventurevisualnovels` | en_reference/genre-adventurevisualnovels.png | — | OUI | Roman visuel | Novela visual |
| `genre-board` | en_reference/genre-board.png | — | OUI | Jeu de plateau | Juego de mesa |
| `genre-casino` | en_reference/genre-casino.png | — | OUI | Casino | Casino |
| `genre-casual` | en_reference/genre-casual.png | — | OUI | Jeu occasionnel | Juego casual |
| `genre-compilation` | en_reference/genre-compilation.png | — | OUI | Compilation multi-jeux | Compilación multijuego |
| `genre-demoscene` | en_reference/genre-demoscene.png | — | OUI | Scène (démo) | Escena (demo) |
| `genre-digitalcard` | en_reference/genre-digitalcard.png | — | OUI | Cartes numériques | Cartas digitales |
| `genre-educative` | en_reference/genre-educative.png | — | OUI | Éducatif | Educativo |
| `genre-party` | en_reference/genre-party.png | — | OUI | Jeu de soirée | Juego de fiesta |
| `genre-pinball` | en_reference/genre-pinball.png | — | OUI | Flipper | Pinball |
| `genre-puzzleandlogic` | en_reference/genre-puzzleandlogic.png | — | OUI | Puzzle & Logique | Puzle y lógica |
| `genre-rpg` | en_reference/genre-rpg.png | — | OUI | RPG | RPG |
| `genre-rpgaction` | en_reference/genre-rpgaction.png | — | OUI | RPG d'action | RPG de acción |
| `genre-rpgdungeoncrawler` | en_reference/genre-rpgdungeoncrawler.png | — | OUI | Dungeon Crawler | Dungeon Crawler |
| `genre-rpgfirstpersonpartybased` | en_reference/genre-rpgfirstpersonpartybased.png | — | OUI | RPG de groupe (1re pers.) | RPG de grupo (1ª persona) |
| `genre-rpgjapanese` | en_reference/genre-rpgjapanese.png | — | OUI | JRPG | JRPG |
| `genre-rpgmmo` | en_reference/genre-rpgmmo.png | — | OUI | MMORPG | MMORPG |
| `genre-rpgtactical` | en_reference/genre-rpgtactical.png | — | OUI | RPG tactique | RPG táctico |
| `genre-simulation` | en_reference/genre-simulation.png | — | OUI | Simulation | Simulación |
| `genre-simulationbuildandmanagement` | en_reference/genre-simulationbuildandmanagement.png | — | OUI | Construction et gestion | Construcción y gestión |
| `genre-simulationfishandhunt` | en_reference/genre-simulationfishandhunt.png | — | OUI | Chasse et pêche | Caza y pesca |
| `genre-simulationlife` | en_reference/genre-simulationlife.png | — | OUI | Simulation de vie | Simulación de vida |
| `genre-simulationscifi` | en_reference/genre-simulationscifi.png | — | OUI | Simulation SF | Simulación de ciencia ficción |
| `genre-simulationvehicle` | en_reference/genre-simulationvehicle.png | — | OUI | Simulation de véhicule | Simulación de vehículos |
| `genre-sportcompetitive` | en_reference/genre-sportcompetitive.png | — | OUI | Sport compétitif | Deporte competitivo |
| `genre-sportfight` | en_reference/genre-sportfight.png | — | OUI | Sport de combat | Deporte de combate |
| `genre-sportracing` | en_reference/genre-sportracing.png | — | OUI | Course | Carreras |
| `genre-sports` | en_reference/genre-sports.png | — | OUI | Sport | Deporte |
| `genre-sportsimulation` | en_reference/genre-sportsimulation.png | — | OUI | Simulation sportive | Simulación deportiva |
| `genre-strategy` | en_reference/genre-strategy.png | — | OUI | Stratégie | Estrategia |
| `genre-strategy4x` | en_reference/genre-strategy4x.png | — | OUI | Explorer Étendre Exploiter Exterminer | Explorar Expandir Explotar Exterminar |
| `genre-strategyartillery` | en_reference/genre-strategyartillery.png | — | OUI | Artillerie | Artillería |
| `genre-strategyautobattler` | en_reference/genre-strategyautobattler.png | — | OUI | Auto-Battler | Auto-Battler |
| `genre-strategymoba` | en_reference/genre-strategymoba.png | — | OUI | Arène de bataille en ligne (MOBA) | Arena de batalla en línea (MOBA) |
| `genre-strategyrts` | en_reference/genre-strategyrts.png | — | OUI | Stratégie temps réel | Estrategia en tiempo real |
| `genre-strategytbs` | en_reference/genre-strategytbs.png | — | OUI | Stratégie tour par tour | Estrategia por turnos |
| `genre-strategytowerdefense` | en_reference/genre-strategytowerdefense.png | — | OUI | Défense de tour | Defensa de torre |
| `genre-strategywargame` | en_reference/genre-strategywargame.png | — | OUI | Stratégie de guerre | Estrategia bélica |
| `genre-trivia` | en_reference/genre-trivia.png | — | OUI | Quiz | Trivial |
| `allgames` | en_reference/allgames.png | fr_reference/allgames.png | non (déjà fait) | (voir fr_reference) | Todos los juegos |
| `favorites` | en_reference/favorites.png | fr_reference/favorites.png | non (déjà fait) | (voir fr_reference) | Favoritos |
| `lastplayed` | en_reference/lastplayed.png | fr_reference/lastplayed.png | non (déjà fait) | (voir fr_reference) | Últimos jugados |
| `ports` | en_reference/ports.png | fr_reference/ports.png | non (déjà fait) | (voir fr_reference) | Portados |