# Décisions actées — RecalBox_DMD (firmware + outils)

Registre des décisions techniques/architecturales transversales, à consulter
AVANT de re-explorer un sujet qui a déjà été tranché. Contrairement aux
changelogs `safe-modify` en tête de chaque fichier (qui expliquent "pourquoi
CE bout de code est comme ça"), ce fichier rassemble les décisions qui
comptent au-delà d'un seul fichier/commit — celles qu'une session future
(sur n'importe quelle branche/worktree) risque de redécouvrir à la dure si
elles ne sont pas notées ici.

**Règle d'usage** : ce fichier est versionné avec le code (visible sur
toutes les branches qui le contiennent, contrairement à la mémoire
Claude qui est scopée par worktree) — c'est la source de vérité en cas de
doute. Ajouter une entrée ici à chaque décision transversale actée, pas
seulement documenter dans un commentaire de code isolé. Rester CONCIS et
daté (renvoyer vers le commit/fichier concerné plutôt que dupliquer le
raisonnement complet).

---

## Page web de configuration (ESP32 WebServer)

- **Pas de limite de taille de page "dure" — c'est un risque probabiliste lié à la navigation rapide entre onglets.**
  `NetworkClient::write()` (bibliothèque Arduino ESP32, `cores/esp32/../Network/src/NetworkClient.cpp`) retente en interne jusqu'à 10x avec un `select()` d'1s chacun (~10-20s de blocage possible) si la connexion est abandonnée en plein transfert (ex. changement d'onglet avant la fin du chargement) — comportement **non configurable depuis le sketch** (`setTimeout()` testé et confirmé sans effet, `send()` utilise `MSG_DONTWAIT`). Pendant ce blocage, `loop()` entier est gelé (y compris le rendu DMD) — symptôme observé : affichage "parasité"/figé pendant ~20s après une navigation rapide entre pages de config.
  Historique : chassé en profondeur sur `master` (v45→v50, tests réels iOS/Nagle/heap), palier final = chunked-send (blocs 1Ko) + détection rapide d'échec partiel + garde heap `maxalloc<4096`. Le seuil **"~12.5 Ko à risque"** noté à l'époque est une heuristique empirique (page plus petite = fenêtre de transfert plus courte = moins de chance d'être interrompue), PAS un seuil technique dur. Reproduit à l'identique le 2026-08-20 sur `dev/core-reassignment` (page MEDIA, 13926o, bloquée 18.5s après navigation rapide) — confirme que cette limitation existe encore, inchangée, sur toutes les branches qui héritent de `sendGzipHtml()`.
  **Options non tranchées à ce jour** (voir mémoire projet `dev-core-reassignment` du 2026-08-20 pour le détail) : accepter tel quel / réduire la taille des pages / patcher les constantes de retry de la lib vendue (risque : casse la tolérance mobile déjà durement acquise en v47-v49) / déplacer le traitement web sur une tâche séparée (gros refactor, jamais tenté).

- **Patch vendored `Stream::timedRead()`/`timedPeek()` (2026-08-20, `dev/core-reassignment`)** : ces fonctions (Arduino core `cores/esp32/Stream.cpp`) busy-spinnaient sans `yield()`/`delay()`, causant un crash TASK_WDT réel quand `WebServer::_parseRequest()` les enchaîne sur un client HTTP lent. Fix : `delay(1)` ajouté dans la boucle. **CE PATCH EST HORS DU DÉPÔT GIT** (installation Arduino partagée du poste de dev, `C:\Users\...\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.11\...`) — invisible sur toute autre machine/réinstallation du core ESP32. Si le crash `TASK_WDT` dans `WebServer::_parseRequest()`/`Stream::timedRead()` réapparaît sur une autre machine, c'est probablement que ce patch n'y a jamais été appliqué — le refaire (voir l'entête du fichier Stream.cpp pour le patch exact, ou la mémoire projet `dev-core-reassignment` du 2026-08-20).

## Architecture commandes MQTT firmware

- **`g_cmdPending[]`/tableau générique indexé par type de commande : DÉLIBÉRÉMENT ÉVITÉ.**
  Fortement suspecté de corruption mémoire (écriture hors-limites ailleurs dans le firmware atterrissant sur cette zone adjacente) sur la branche `dev/mame-score-mqtt-bridge` — expliquerait le bug "CMD_STARTCLIP fantôme" (déclenchement sans aucun message MQTT réel prouvable côté broker), jamais élucidé autrement malgré plusieurs sessions d'investigation. `dev/core-reassignment` utilise à la place `pendingCmd` (UN SEUL slot partagé, pas un tableau) + des variables dédiées nommées individuellement pour les commandes les plus sensibles (`g_pendingDefault`/`g_pendingSystem`+Arg/`g_pendingGame`+Arg) — motif à reproduire pour toute nouvelle commande MQTT plutôt que d'étendre un tableau générique.

- **rc=-4 / gels de connexion MQTT chassés sur plusieurs sessions : cause principale = overclock RPi5 + canicule, PAS le hi-score/LoopCore.**
  Confirmé le 2026-08-19 par test décisif (vitesse de navigation max, coupe-circuit anti-rafale désactivé, zéro rc=-4 une fois l'overclock retiré). Ne pas re-suspecter le portage hi-score ou la réassignation de cœur (`LoopCore=0`) pour CE symptôme précis sans nouvelle preuve — c'est un dossier distinct du bug "CMD_STARTCLIP fantôme" ci-dessus (qui, lui, reproduit indépendamment de l'overclock).

## Philosophie "DMD bête" (hi-score/infos/description/RA, depuis 2026-08-19/20)

- **Le DMD reste 100% passif — toute décision de timing/logique vit côté script Recalbox (RB), jamais côté firmware.**
  `CMD_GAME`/l'affichage du jeu restent gérés par le DMD comme avant (inchangés). Le contenu hi-score/infos/description/RA est ajouté par-dessus via un canal unique (`marquee/cmd/score`) sans aucune dépendance au canal `marquee/event`/`CMD_STARTCLIP`/`CMD_RESUMESYS` (zone à risque, voir ci-dessus) — retour automatique au jeu par un timer purement LOCAL au DMD (aucun message RB requis pour revenir), garantie anti-blocage explicite. Toute nouvelle fonctionnalité de ce sous-système (répétition périodique, dwell navigation, etc.) doit suivre ce même principe : complexité/état côté RB, DMD strictement dumb-display + auto-revert. Un réglage exprimé en unités "sûres par construction" (ex. répétition en CYCLES de slideshow plutôt qu'en secondes) est préférable à un réglage brut qui oblige l'utilisateur à calculer une valeur sûre à la main.

## Mémoire Claude — limitation connue

- La mémoire Claude est scopée PAR WORKTREE (dossier différent par chemin) — une décision actée sur `master` n'est PAS automatiquement visible depuis un worktree `dev/...`, et inversement. Ce fichier (`DECISIONS.md`, versionné avec le code) est le seul registre qui traverse fiablement les branches/worktrees. En cas de doute sur une décision déjà prise ailleurs, chercher explicitement dans les dossiers mémoire des AUTRES worktrees (`C:\Users\...\.claude\projects\<slug-du-worktree>\memory\`) plutôt que de supposer qu'un sujet n'a jamais été traité.
