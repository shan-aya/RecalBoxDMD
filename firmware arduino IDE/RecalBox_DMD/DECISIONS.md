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
  **Décision actée (2026-08-20, commit `edfd012`)** : réduire la taille des pages plutôt que patcher la lib vendue ou déplacer le traitement web. `BASIC` (19256o gzip, devenue la plus grosse après l'ajout hi-score) scindée en 2 : "Affichage" (luminosité + hi-score/infos/RA, 10691o) et "Playlist" (playlist + génération, 10004o) — nouvelle route `/config/playlist`. `MEDIA` (13928o) reste la page la plus grosse, non touchée cette fois — si un blocage s'y reproduit, candidate suivante pour une scission (ex. séparer upload GIF / gestion dossiers).

- **Patch vendored `Stream::timedRead()`/`timedPeek()` (2026-08-20, `dev/core-reassignment`) — TENTÉ PUIS ANNULÉ, NE PAS REFAIRE tel quel.**
  Constat initial : ces fonctions (Arduino core `cores/esp32/Stream.cpp`) busy-spinnaient sans `yield()`/`delay()`, causant un crash TASK_WDT réel quand `WebServer::_parseRequest()` les enchaîne sur un client HTTP lent. Fix tenté : `delay(1)` ajouté dans la boucle.
  **Effet réel constaté sur matériel : RÉGRESSION GRAVE.** Avant le patch, un blocage prolongé finissait par déclencher le TASK_WDT → crash + reboot automatique (perturbant mais auto-guérisseur, ~18-25s). Le `delay(1)` nourrit le watchdog en continu PENDANT le blocage, donc un blocage réseau qui aurait dépassé la tolérance de l'ancien mécanisme fait maintenant **geler le DMD indéfiniment**, sans auto-reboot — nécessitant un débranchement/rebranchement USB physique (survenu 2x le 2026-08-20 avant identification de la cause). Patch **REVERTÉ** (restauration depuis `_backups/Stream_2026-08-20_16-03-05.cpp.bak`), recompilé, reflashé, confirmé sain (brightness live testé OK).
  **Conclusion actée** : le crash TASK_WDT d'origine, bien que perturbant, jouait un rôle protecteur (auto-recovery) qu'un simple `delay()` supprime sans le remplacer par une vraie sortie de blocage. Ne pas réappliquer ce patch sans un mécanisme de timeout/abandon de connexion explicite en complément (pas juste nourrir le watchdog). Ce patch reste de toute façon HORS DU DÉPÔT GIT (installation Arduino partagée du poste de dev) — sans objet sur une autre machine tant qu'il n'est pas repensé.

- **`sendGzipHtml()` (pages de config web) n'envoyait AUCUN en-tete `Cache-Control` jusqu'au 2026-08-20.** Cause probable d'un symptome confus rencontre ce jour-la ("impossible de charger la config", listes non peuplees a l'ouverture) : un navigateur peut mettre en cache heuristiquement une reponse GET sans directive explicite -- sur un projet reflashe tres frequemment en developpement actif, une ancienne version de page (JS perime) peut alors rester servie depuis le cache alors que le format `/load` cote firmware a deja change, echouant silencieusement. Fix : `Cache-Control: no-store` ajoute a `sendGzipHtml()` (toutes les pages) -- a garder pendant toute la phase de developpement actif ; a reconsiderer seulement si/quand le rythme de changement de ces pages ralentit durablement.

## Architecture commandes MQTT firmware

- **`g_cmdPending[]`/tableau générique indexé par type de commande : DÉLIBÉRÉMENT ÉVITÉ.**
  Fortement suspecté de corruption mémoire (écriture hors-limites ailleurs dans le firmware atterrissant sur cette zone adjacente) sur la branche `dev/mame-score-mqtt-bridge` — expliquerait le bug "CMD_STARTCLIP fantôme" (déclenchement sans aucun message MQTT réel prouvable côté broker), jamais élucidé autrement malgré plusieurs sessions d'investigation. `dev/core-reassignment` utilise à la place `pendingCmd` (UN SEUL slot partagé, pas un tableau) + des variables dédiées nommées individuellement pour les commandes les plus sensibles (`g_pendingDefault`/`g_pendingSystem`+Arg/`g_pendingGame`+Arg) — motif à reproduire pour toute nouvelle commande MQTT plutôt que d'étendre un tableau générique.

- **rc=-4 / gels de connexion MQTT chassés sur plusieurs sessions : cause principale = overclock RPi5 + canicule, PAS le hi-score/LoopCore.**
  Confirmé le 2026-08-19 par test décisif (vitesse de navigation max, coupe-circuit anti-rafale désactivé, zéro rc=-4 une fois l'overclock retiré). Ne pas re-suspecter le portage hi-score ou la réassignation de cœur (`LoopCore=0`) pour CE symptôme précis sans nouvelle preuve — c'est un dossier distinct du bug "CMD_STARTCLIP fantôme" ci-dessus (qui, lui, reproduit indépendamment de l'overclock).

- **Coupe-circuit anti-rafale `marquee.sh` (`BURST_THRESHOLD`) — débit réel plafonne à ~5-7 survols/seconde sur ce matériel, PAS plus haut.**
  Mesuré à plusieurs reprises (2026-08-18 puis re-confirmé le 2026-08-22, `marquee_mqtt.log` agrégé + timestamps millisecondes serial DMD) : la navigation la plus rapide possible dans RB (y compris le mode "QuickJump" alphabétique) n'a jamais dépassé 6-7 survols dans la même seconde d'horloge. Un seuil fixé au-delà (`10`, testé le 2026-08-22) est structurellement inatteignable — le mécanisme (`!SHUFFLE`) ne se déclenche alors JAMAIS, symptôme silencieux (pas d'erreur, juste un no-op permanent). Valeur stable retenue : **`BURST_THRESHOLD=5`**, avec en plus (v16) une exigence de **durée soutenue** (`BURST_SUSTAIN_SECONDS=2`, secondes consécutives au-dessus du seuil) plutôt qu'un déclenchement instantané dès la 1ère seconde — évite un déclenchement visuellement trop précoce sur un simple pic isolé. Toute réintroduction/retouche future de ce mécanisme doit repartir de ces deux valeurs, pas de zéro.

## Philosophie "DMD bête" (hi-score/infos/description/RA, depuis 2026-08-19/20)

- **Le DMD reste 100% passif — toute décision de timing/logique vit côté script Recalbox (RB), jamais côté firmware.**
  `CMD_GAME`/l'affichage du jeu restent gérés par le DMD comme avant (inchangés). Le contenu hi-score/infos/description/RA est ajouté par-dessus via un canal unique (`marquee/cmd/score`) sans aucune dépendance au canal `marquee/event`/`CMD_STARTCLIP`/`CMD_RESUMESYS` (zone à risque, voir ci-dessus) — retour automatique au jeu par un timer purement LOCAL au DMD (aucun message RB requis pour revenir), garantie anti-blocage explicite. Toute nouvelle fonctionnalité de ce sous-système (répétition périodique, dwell navigation, etc.) doit suivre ce même principe : complexité/état côté RB, DMD strictement dumb-display + auto-revert. Un réglage exprimé en unités "sûres par construction" (ex. répétition en CYCLES de slideshow plutôt qu'en secondes) est préférable à un réglage brut qui oblige l'utilisateur à calculer une valeur sûre à la main.

## Page web de configuration — pièges JS récurrents (2026-08-20)

- **Ne jamais placer un élément interactif (icône, bouton) À L'INTÉRIEUR d'un élément porteur de `data-i18n`.** `applyLang()` fait `el.innerHTML=tr(el.dataset.i18n)` sur chaque élément marqué — ça efface silencieusement tout enfant présent, y compris ajouté après coup. Piège vécu : icône d'aide "?" Bluetooth insérée dans un `<h2 data-i18n="sec_bt">`, effacée à chaque chargement de page. Toujours mettre l'élément interactif en **sibling** (à côté, pas dedans), quitte à wrapper le texte traduit dans un `<span data-i18n>` séparé.
- **Un popup/tooltip en `position:fixed` plein écran (`inset:0`) qui apparaît AU-DESSUS d'une icône hover lui fait perdre son `:hover` CSS** → `mouseleave` se déclenche sans que la souris ait bougé → fermeture → l'icône redevient survolable → `mouseenter` → réouverture → clignotement. Fix : `pointer-events:none` sur le conteneur plein écran, `pointer-events:auto` uniquement sur la boîte de contenu visible (les événements souris traversent jusqu'à l'icône en dessous, qui garde son survol).
- **Le motif brouillon localStorage `g(k,dv)=(draft&&draft[k]!==undefined)?draft[k]:dv`** (4+ pages) doit exclure la chaîne vide, pas seulement `undefined` — un champ nombre momentanément vidé pendant une frappe est sauvegardé tel quel par `saveDraft()` (déclenché sur CHAQUE `input`), et une chaîne vide écrase alors silencieusement la vraie valeur serveur à chaque rechargement. Condition correcte : `draft[k]!==undefined&&draft[k]!==''`.

## CHANTIER OUVERT (2026-08-20) — Le DMD ne se resynchronise pas sur l'état réel de la RB au (re)branchement

**Symptôme rapporté** : au démarrage (après connexion MQTT validée) et après "Reprendre DMD" (bouton web), le DMD ne reflète pas l'état RÉEL de la RB au moment du (re)branchement — il retombe sur la playlist alors que la RB est en jeu ou dans une liste, et/ou reste bloqué sur l'écran "RecalBox connectée" indéfiniment au lieu d'un temps limité.

**Diagnostic établi (pas encore corrigé)** : `MQTT_WAITING_GRACE_MS = 1500` (`RecalBox_DMD.ino`) — fenêtre de grâce qui ignore DÉLIBÉRÉMENT les messages MQTT retenus (`marquee/cmd/system`/`marquee/cmd/game`, publiés avec `-r` par `marquee.sh`) à chaque connexion, pour éviter d'afficher un jeu "périmé" resté accroché d'une session précédente. Problème : cette fenêtre s'arme au moment où `mqttClient.connect()` réussit — qui survient déjà plusieurs secondes après le début du boot (WiFi, NTP, chargement caches, observé ~15-30s dans les logs de cette session) — donc le message retenu arrive quasi systématiquement DANS cette fenêtre de 1.5s et se fait filtrer. Le DMD attend alors passivement un NOUVEAU message MQTT, que la RB n'envoie que sur une navigation active — si la RB est immobile (en jeu ou en liste sans bouger), rien n'arrive jamais.

**Nature du problème** : ce n'est pas un bug ponctuel mais un trou de conception — le DMD ne fait que réagir passivement aux événements RB, il ne demande/reçoit jamais activement "quel est ton état actuel ?" au (re)branchement.

**Pistes à explorer (aucune tranchée)** :

1. Réduire/retirer la fenêtre de grâce sur `system`/`game` spécifiquement (risque : réintroduit l'affichage d'un jeu périmé après une longue coupure DMD — le problème d'origine que cette fenêtre visait à corriger).
2. RB republie activement son état courant à un moment identifiable côté DMD (ex. topic dédié `marquee/status/rb_state` publié par un watcher marquee.sh, lu par le DMD APRÈS l'expiration de sa propre fenêtre de grâce plutôt qu'au moment du retenu initial).
3. Le DMD publie un message "je viens de me connecter" que marquee.sh écoute et auquel il répond immédiatement par l'état courant (round-trip explicite, pas de dépendance au timing du retenu).

**Décision utilisateur (2026-08-20)** : reporté à une prochaine session (pas traité ce soir, 2 crashs TASK_WDT déjà essuyés — voir plus haut). Budget de test dédié sur matériel nécessaire, ne pas traiter à la légère.

## Scripts RB (userscripts) — verrou anti-relance

- **Verrou fichier PID (check-then-write) PROUVÉ INSUFFISANT — verrou `mkdir` atomique désormais le standard.**
  Motif historique `if [ -f "$PIDFILE" ]; then ...; fi; echo $$ > "$PIDFILE"` : la vérification et l'écriture sont deux opérations séparées, donc PAS atomiques. EmulationStation peut déclencher plusieurs invocations du même script dans la même seconde (rafale d'événements, ex. au boot) — toutes peuvent lire "pas de verrou" avant qu'aucune n'ait écrit le sien. Reproduit deux fois sur `dev/core-reassignment` malgré un verrou PID censé déjà corriger le problème : `dmd_achievement[watch].sh` retrouvé en **4 instances simultanées**, `marquee[...].sh` en 2 (2026-08-20).
  **Fix acté** : remplacer par un verrou `mkdir "$LOCKDIR"` (atomique sur ce filesystem/tmpfs — un seul appelant concurrent peut réussir), avec PID écrit à l'intérieur (`$LOCKDIR/pid`) pour détecter un verrou orphelin (process mort) et le récupérer via `rmdir` + nouvelle tentative. Appliqué aux 3 scripts RB : `marquee[...].sh` (v12), `dmd_score[...].sh` (v5), `dmd_achievement[watch].sh` (v3). **Tout nouveau script RB avec verrou anti-relance doit utiliser ce motif dès la création, pas le motif PID-file.**

## Pagination hi-score/infos/description (dmd_score.sh, depuis 2026-08-20)

- **DESCRIPTION : pagination par PHRASES ENTIERES, jamais par découpe fixe de lignes.**
  `send_paginated()` découpe le texte sur `.`/`!`/`?` (les paragraphes, eux, sont déjà détruits en amont par `dmd_game_info.py` qui normalise tous les retours à la ligne du XML en simple espace — seule la fin de phrase reste détectable) puis empile les phrases une à une sur la page en cours tant que ça tient (3 lignes/page, `LINES_PER_PAGE`). Une phrase n'est **jamais** coupée, sauf le seul cas inévitable où elle est à elle seule trop longue pour une page entière (alors étalée sur plusieurs pages consécutives). Plafond de sécurité dédié `MAX_PAGES_DESCRIPTION=5` (séparé de `MAX_PAGES=3` qui reste pour INFOS/hi-score, pagination à taille fixe inchangée) — ellipse `...` ajoutée à la dernière ligne de la dernière page si troncature réelle. Décision explicite utilisateur après itération : d'abord une version "recule vers la ponctuation précédente dans la fenêtre" (rejetée, coupait les pages trop court), puis "avance vers la ponctuation suivante, quitte à dépasser la fenêtre" (retenue).
  **Ne pas revenir à un découpage par nombre de lignes fixe pour DESCRIPTION sans revalider avec l'utilisateur** — c'est un choix de design délibéré, pas un oubli.

- **Interruption inter-pages sur changement de contexte (`state_still_valid()`, dmd_score.sh v13).**
  `round_robin()` ne vérifiait l'état de référence (state_file/expected) qu'ENTRE 2 tours de types de contenu différents, jamais ENTRE LES PAGES d'un même contenu multi-pages en cours d'envoi (chaque fonction de pagination a son propre `sleep()` interne, invisible à `round_robin()`). Symptôme : lancer/quitter un jeu en pleine séquence multi-pages laissait l'ancienne séquence continuer jusqu'à sa fin, mélangée avec la nouvelle. Fix : `state_file`/`expected` transmis en cascade jusque dans les fonctions de pagination, vérifié après chaque `sleep()` interne. **Tout nouveau point d'envoi multi-pages ajouté à ce fichier doit suivre ce même motif** (accepter `sf`/`exp` en derniers paramètres, vérifier après chaque `sleep`) sous peine de réintroduire ce bug.

## Déploiement RB via plink — piège `pkill -f` auto-destructeur

- **`pkill -f '<motif>'` envoyé dans la MÊME commande plink multi-lignes qu'une ligne contenant le nom de fichier LITTÉRAL (sans backslash) se tue lui-même.** Le shell distant qui exécute tout le bloc (`sh -c "<bloc complet>"`) a ce texte dans son PROPRE `argv` — `pkill -f` matche n'importe quel processus dont l'argv contient le motif, y compris son propre parent invocateur. Symptôme : `plink` retourne un code de sortie ~128, **aucune sortie du tout** (le shell distant meurt avant le premier `echo`). Reproduit 2x sur `dev/core-reassignment` (2026-08-20/21) avec `dmd_score[...].sh`.
  **Fix qui marche** : ne jamais combiner `pkill -f <motif-du-script>` et le nom de fichier littéral dans le même appel `plink` — toujours séparer en appels distincts (1: pkill+cleanup seul, 2: cd+relance seul, 3: vérification `ps` seule).

- **Même famille de bug, variante `ps | grep` : `ps -o args -ww | grep -q -- '<motif>'` peut s'auto-matcher.** Trouvé dans `dmd_score.sh` (`challenge_session_active()`, v24, 2026-08-22) : `grep -q -- '--challenge '` voit, dans la sortie de `ps`, SA PROPRE ligne de commande (qui contient littéralement la chaîne cherchée dans ses arguments) — la fonction retournait donc TOUJOURS vrai, même sans processus cible réel, bloquant silencieusement une fonctionnalité entière (round-robin hi-score/infos/description scotché sur un type erroné pour TOUS les jeux). Piège d'autant plus vicieux qu'un test avec un vrai match concurrent (vraie session active en même temps) masque le bug.
  **Fix qui marche** : casser la chaîne littérale dans l'invocation du grep sans changer ce qu'elle matche réellement, ex. `grep -- '[-]-challenge '` au lieu de `grep -- '--challenge '` (bracket expression regex : matche toujours le même texte cible, mais absent littéralement des arguments du grep lui-même). Réflexe à avoir sur TOUT `ps | grep -f`/`pgrep -f` dont le motif recherché pourrait apparaître dans la commande qui fait la recherche elle-même (nom de script, flag caractéristique, etc.).

## Mémoire Claude — limitation connue

- La mémoire Claude est scopée PAR WORKTREE (dossier différent par chemin) — une décision actée sur `master` n'est PAS automatiquement visible depuis un worktree `dev/...`, et inversement. Ce fichier (`DECISIONS.md`, versionné avec le code) est le seul registre qui traverse fiablement les branches/worktrees. En cas de doute sur une décision déjà prise ailleurs, chercher explicitement dans les dossiers mémoire des AUTRES worktrees (`C:\Users\...\.claude\projects\<slug-du-worktree>\memory\`) plutôt que de supposer qu'un sujet n'a jamais été traité.
