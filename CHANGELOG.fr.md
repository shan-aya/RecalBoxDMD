# Changelog

Historique de **RecalBoxDMD — RawEdition v2.0**, couvrant à la fois le **firmware ESP32** (y compris sa page de configuration web) et la **boîte à outils PC**, depuis le tout premier commit jusqu'à aujourd'hui. Les entrées sont groupées par date ; chaque puce est étiquetée avec la partie du projet qu'elle concerne.

[🇬🇧 English](CHANGELOG.md) · 🇫🇷 **Français** · [🇪🇸 Español](CHANGELOG.es.md)

Ceci est un résumé sélectionné de l'historique interne des versions du projet (185+ révisions firmware, 64+ révisions config web, 43+ révisions boîte à outils, 62+ révisions GUI) — regroupé par jalons réellement pertinents pour un utilisateur, pas un déversement brut de chaque micro-correctif.

---

## 2026-09-13 — MQTT → UDP : fusion de `dev/dmd-udp-transport` sur master

- **Firmware** : la liaison temps réel entre Recalbox et le DMD passe de **MQTT à UDP** — l'ancien transport se heurtait à un mur au niveau de la plateforme, dans la pile TCP/IP de l'ESP32 (un `TCP_SND_BUF` fixe d'environ 5,7 Ko, figé dans le core Arduino précompilé, sans levier applicatif possible) qui pouvait bloquer un `SUBSCRIBE` MQTT plusieurs secondes après une reconnexion ; l'UDP n'a pas cette poignée de main à bloquer. Le support MQTT reste présent dans le firmware, désactivé par défaut, comme filet de repli — voir [UPGRADING.md](UPGRADING.md).
- **Firmware** : chasse de plusieurs mois à un gel de réception UDP sévère historiquement signalé (12 à 90s+) — jamais reproduit après une campagne d'instrumentation intensive (sonde active de gel, mesure du pire cas par appel, trafic réel soutenu), bien qu'une dizaine de bugs réels sans rapport aient été trouvés et corrigés au passage (ci-dessous). Conclusion actuelle : les signalements initiaux étaient très probablement un effet de bord bénin d'un seuil de détection trop agressif face à une perte de paquet UDP ordinaire, pas un véritable gel de réception — détaillé dans `DECISIONS.md`.
- **Firmware** : corrige une fausse alerte "Recalbox hors ligne" déclenchée par la perte d'un seul paquet UDP (normal, attendu en UDP) — exige désormais deux pertes consécutives avant d'alerter.
- **Firmware** : corrige la resynchronisation après reconnexion qui pouvait rester bloquée sur le mode d'affichage de repli en cache (`MODE_PNG`) au lieu de rattraper l'état réel de Recalbox.
- **Firmware** : corrige "Reprendre DMD" (page de config web) qui forçait systématiquement la playlist d'attente quoi que fasse Recalbox (en jeu, démo, gameclip...) — une vérification obsolète datant de l'ère MQTT rendait la bonne branche inatteignable depuis la bascule UDP ; resynchronise désormais l'état réel, comme le fait une reconnexion.
- **Firmware** : réduit le nombre d'ouvertures SD par changement de jeu (jusqu'à 6 auparavant, 1 seule désormais dans le meilleur des cas) en mémorisant laquelle des deux conventions de nommage de dossiers la carte SD utilise, au lieu de tester les deux à chaque fois — réduit aussi l'exposition à un rare crash d'allocation mémoire dans le pilote de système de fichiers SD ; une nouvelle tentative automatique a été ajoutée pour la variante rattrapable de ce même crash.
- **Scripts Recalbox** : corrige un processus zombie du round-robin hi-score qui pouvait survivre à un réveil de l'écran de veille et continuer à afficher un vieux score par-dessus le marquee indéfiniment.
- **Scripts Recalbox** : corrige le mode `gameclip`/démo qui affichait la playlist générique au lieu du marquee du jeu du clip (reliquat d'un contournement de l'ère MQTT qui l'avait trop largement désactivé).
- **Docs** : l'ancien master est archivé sous `archive/mqtt-pre-udp-transport-final` — dernier état du projet avant cette bascule de transport, conservé pour pouvoir revenir en arrière.
- **Image de marque** : l'édition firmware du projet est renommée **Raw565 Edition → RawEdition v2.0** (le format pixel `raw565` lui-même, et tout ce qui repose dessus, ne change pas — seuls le nom produit/le badge de version changent).

## 2026-09-06 — Fusion de `dev/core-reassignment` sur master : écrans superposés en jeu, Challenge RB, onglet Playlist

La plus grosse fusion de l'histoire du projet — plusieurs mois de travail sur une branche séparée, réconciliés avec tout ce qui était sorti sur master entretemps, puis testés point par point (chaque conflit résolu et retesté individuellement) avant d'atterrir ici. Tu utilises déjà une version antérieure ? Voir **[UPGRADING.fr.md](UPGRADING.fr.md)**.

- **Firmware** : nouveau **système d'écrans superposés en jeu** — pendant qu'un jeu tourne réellement, le panneau peut alterner automatiquement le marquee avec le vrai **Hi-Score MAME/FBNeo** (manifeste communautaire, ~2 758 jeux, décodé depuis le fichier de score sauvegardé par l'émulateur lui-même, aucune lecture RAM en direct), les **Infos jeu** (description/genre/développeur/année depuis `gamelist.xml`), les **RetroAchievements** débloqués, et le classement mensuel du **Challenge** officiel de Recalbox. Entièrement passif côté DMD — toute la logique de timing vit dans les scripts côté Recalbox, le firmware se contente d'afficher ce qu'on lui envoie et revient tout seul au marquee sur son propre minuteur local, pour qu'un script lent ou en échec ne puisse jamais bloquer le panneau. Voir la [section dédiée du README](README.fr.md#écrans-superposés-en-jeu--hi-score-infos-jeu-succès--challenge-rb).
- **Boîte à outils PC** : le **Mode 9** (et l'installation automatique intégrée au Mode 1) installe désormais aussi les scripts Hi-Score/Infos jeu/Succès/Challenge (`dmd_helpers/`) et nettoie les anciens noms de scripts d'une installation précédente — auparavant, seule l'étape de mise en scène séparée du Mode 1 le gérait, pas le Mode 9 lui-même.
- **Boîte à outils PC** : le flag « lent » du système de masque (**« L »**, déclenche l'écran d'attente sur les grosses collections) est désormais calculé **par sous-dossier alphabétique (« bucket »)** au lieu de par système entier — un système avec un gros sous-dossier et plusieurs petits ne pénalise plus inutilement les petits. Le seuil par défaut de l'onglet Paramètres suit (5 000 → 800, à nouveau pertinent maintenant qu'il s'applique par bucket).
- **Boîte à outils PC** : **onglet Playlist** — créez vos propres rotations en mode attente en combinant le pack de 600 GIFs et vos propres GIFs (glissez un dossier PC) ; possible désormais aussi **en plein Mode 1**, avant même la copie du dossier de travail sur la carte SD, et plus seulement après coup depuis une carte insérée.
- **Boîte à outils PC** : la langue de l'interface par défaut est désormais celle du **système Windows** au premier lancement (au lieu de toujours l'anglais) — un choix explicite dans l'onglet Paramètres reste toujours prioritaire ensuite.
- **Boîte à outils PC** : plusieurs correctifs trouvés en testant la fusion en direct — un cadre « copie SD » resté affiché pouvait pousser la barre de Progression hors de la fenêtre fixe sur certains modes de l'onglet Avancé, le tirage aléatoire de thème pouvait tomber sur le thème « default » tout nu, et fermer l'appli en pleine ajout de GIFs perso (étape playlist du Mode 1) reprend désormais le pipeline au lieu de quitter.
- **Firmware / MQTT** : les 12 topics séparés `marquee/cmd/*` ont été fusionnés en un seul topic `marquee/cmd` (payload compact `CMD=/ARG=`) — réduit le nombre de souscriptions MQTT par (re)connexion de 12 à 2, limitant l'exposition à une condition rare de la pile WiFi de l'ESP32 où une souscription pouvait silencieusement ne jamais quitter l'appareil.

## 2026-08-19 — Détection des lecteurs amovibles & positionnement des popups

- **Boîte à outils PC** : correction de la détection de carte SD qui échouait silencieusement sur les builds récentes de Windows 11 — la liste des lecteurs reposait entièrement sur `wmic.exe`, retiré par défaut par Microsoft sur les versions récentes de Windows 11 ; un utilisateur voyait sa carte SD dans l'Explorateur Windows mais elle n'apparaissait jamais dans l'outil (Mode 1/6/8), sans aucun message d'erreur. La détection passe désormais par `Get-CimInstance` (PowerShell), avec l'ancien appel `wmic` conservé uniquement en dernier recours pour les environnements inhabituels.
- **Boîte à outils PC** : correction de plusieurs popups (fin de copie, sélection du lecteur carte SD, confirmation de fermeture) qui apparaissaient hors écran ou hors de la fenêtre principale, surtout sur les configurations multi-écrans. Double cause : la fenêtre principale n'avait jamais de position explicite au lancement (désormais centrée explicitement sur l'écran primaire au démarrage), et les builds `.exe`/`.msi` compilées n'avaient pas de manifeste Windows déclarant la conscience DPI — présent nativement en lançant depuis les sources Python, mais absent par défaut des sorties PyInstaller/cx_Freeze, ce qui pouvait fausser les coordonnées de fenêtre rapportées par Windows. Le centrage des popups se confine désormais aussi au vrai moniteur que Windows rapporte pour la fenêtre principale, plutôt qu'à la taille d'écran primaire uniquement connue de Tk.

## 2026-08-16 — Images systèmes/genres multilingues (FR/ES)

- **Boîte à outils PC** : le pack de secours `systems/_defaults` (badges de genre, pseudo-systèmes comme Favoris/Derniers Jeux Joués/Portages/Tous Jeux) est désormais disponible en **français et espagnol**, 60/60 chacun — icône conservée pixel pour pixel (vectorisée, pas juste agrandie), seul le texte a été re-rendu et traduit. Les genres pas encore traduits basculent simplement en anglais, jamais d'absence.
- **Boîte à outils PC** : nouveau sélecteur de **langue des images système** (EN/FR/ES, avec aperçu comparatif côte à côte) dans le Mode 1 (pipeline auto) et le Mode 2 (onglet Avancé, téléchargement `_defaults` seul) — `download_defaults()` récupère toujours d'abord le jeu anglais de base (repli garanti), puis surcharge avec les fichiers traduits de la langue choisie.
- **Boîte à outils PC** : le Mode 2 propose désormais systématiquement la galerie d'image de secours (fermer sans choisir revient au visuel par défaut du projet) au lieu d'une question oui/non conditionnée à "pas déjà défini" ; les popups de confirmation après sélection ont aussi été retirées (le choix est déjà visible/appliqué immédiatement).
- **Boîte à outils PC** : correction d'un vrai bug de lenteur dans `_parallel_download_batch()` — `urlretrieve()` n'avait aucun timeout, une seule connexion figée dans le pool de 16 threads pouvait bloquer son slot indéfiniment ; un timeout socket borné est maintenant posé le temps du lot.
- **Assets firmware** : 15 logos systèmes/genres ajoutés à `_defaults` — 10 manquants par rapport au jeu de logos officiel Recalbox, plus 5 ajouts très récents de la branche alpha de Recalbox (Cassette Vision, EXL 100, ST-V, Vircon32, et le nouveau pseudo-système **Challenges**).

## 2026-08-13 — Préparation de la publication

- **Docs** : réécriture complète du README en anglais/français/espagnol — captures d'écran, vraies images de l'appareil, référence des modes, guide matériel.
- **Firmware** : [installateur Web](https://shan-aya.github.io/RecalBoxDMD/) — flashez l'ESP32 directement depuis Chrome/Edge, sans Arduino IDE.
- **Boîte à outils PC** : installateur Windows (`.exe` via Inno Setup) et `.msi` (via cx_Freeze), plus un `install_and_run.bat` en un clic pour lancer depuis les sources.

## 2026-08-11 — Aperçus en direct, pack de GIFs, et accordéon de l'onglet Avancé

- **Firmware / Config web** : choisir un thème horloge ou déplacer le curseur de luminosité sur la page de config web **s'affiche instantanément sur le panneau physique**, avant même d'enregistrer.
- **Firmware** : correction du bug « Reprendre DMD » ignoré pendant qu'un aperçu de thème horloge tournait encore ; journalisation diagnostique de la raison du dernier reset au démarrage.
- **Boîte à outils PC** : les 8 radios à plat de l'onglet Avancé réorganisées en **5 catégories repliables** (téléchargements GitHub / Gamelist / Images / Caches / Scripts) ; **Mode 10** (définir/générer l'image de secours globale) et **Mode 11** (téléchargement en un clic du pack ~600 GIFs) ajoutés ; le seuil « L » des systèmes lents devient une valeur réglable dans l'onglet Paramètres au lieu d'une constante codée en dur.

## 2026-08-09 – 2026-08-10 — Passe de stabilité sur matériel réel

- **Firmware** : plusieurs correctifs trouvés uniquement via des tests directs sur matériel, autour du masque des systèmes lents et de la recherche rapide de jeu.
- **Boîte à outils PC** : le travail sur le seuil du flag « L » a commencé ici (voir ci-dessus), motivé par des différences réelles de vitesse de carte SD signalées par les utilisateurs.

## 2026-08-06 – 2026-08-07 — Stabilité du tas mémoire (heap)

- **Firmware** : deux correctifs indépendants de fragmentation du tas (une étape dédiée de génération de playlist, désactivation de la reconnexion WiFi automatique) — aucun incident ensuite lors de tests intensifs réels, y compris une coupure/redémarrage du routeur en cours d'utilisation.

## 2026-08-05 — Fusion `dev/tous-txt-filter`

- **Boîte à outils PC** : outillage playlist et base du système de banque de GIFs GitHub fusionnés dans la branche principale.

## 2026-08-03 — Refonte du flux de premier démarrage

- **Firmware / Config web** : la page de configuration premier démarrage / point d'accès WiFi largement retravaillée suite à des tests réels de premier lancement.
- **Boîte à outils PC** : mises à jour correspondantes du sélecteur d'image de secours et des popups autour des messages de premier lancement/redémarrage.

## 2026-08-01 – 2026-08-02 — La refonte `cache_master_gifs`

- **Firmware + Config web + Boîte à outils PC** : refonte en trois parties du pipeline de playlists GIF autour de `cache_master_gifs.dat`, un index maître de tous les GIFs déjà présents sur la carte SD — accélère la navigation dans les dossiers de la page web Médias et la construction de playlists dans l'onglet Playlist de la boîte à outils, et a rendu les envois en gros volume depuis la page web bien plus fiables (ajustement de taille de buffer, sérialisation des envois pour éviter `ERR_INVALID_CHUNKED_ENCODING`).

## 2026-07-26 – 2026-07-29 — Passe de débogage sur matériel réel

- **Firmware** : investigations sur l'usage du tas mémoire et la connexion MQTT en conditions réelles ; plusieurs régressions trouvées et corrigées de cette façon.
- **Boîte à outils PC** : le Mode 9 (installer les scripts Recalbox) fiabilisé après le diagnostic d'un vrai cas d'échec SMB/connexion invité sur une Recalbox réelle.

## 2026-07-22 – 2026-07-23 — Pipeline du Mode 1 & détection réseau

- **Boîte à outils PC** : `detect_recalbox_share()` (détection NetBIOS automatique de `\\RECALBOX\share`) et `resolve_recalbox_ip()` ; le flux d'installation des scripts Recalbox entièrement retravaillé après des tests réels.

## 2026-07-20 – 2026-07-21 — Audit de traduction & installateur de scripts

- **Boîte à outils PC** : audit complet de traduction FR/EN/ES avec parité stricte des clés entre les trois langues ; **Mode 9** livré — installe les scripts utilisateur Recalbox (pont marquee, récupération WiFi, synchro config web) directement via le partage réseau de la Recalbox, remplaçant une approche FTP antérieure que la Recalbox cible ne supportait en réalité pas.

## 2026-07-14 — 10e thème horloge

- **Firmware** : « Level 1-1 » — une recréation défilante du premier niveau de Super Mario Bros — ajouté comme 10e thème horloge.

## 2026-07-13 — Interface trilingue

- **Firmware + Config web + Boîte à outils PC** : français/anglais/espagnol ajoutés partout — la page de config web du DMD et la boîte à outils Windows partagent la même langue, poussée automatiquement au DMD au tout début du Mode 1.

## 2026-07-11 — Images de secours & prise en compte de la version Recalbox

- **Boîte à outils PC** : sélecteur d'image de secours personnalisée (choisir ce qui s'affiche quand rien d'autre ne correspond) ; le **sélecteur « Version Recalbox »** (10.x / 9.x / legacy) est introduit, pour que l'outil lise la bonne balise `gamelist.xml` (`<logo>`/`<thumbnail>`/`<image>`) et le bon dossier média selon votre configuration.

## 2026-07-10 — L'interface graphique arrive

- **Boîte à outils PC** : `RecalBoxDMD_GUI.py` v1 — une interface Tkinter enveloppant l'outil console ; copie SD reprenable après interruption ; affinement constant de la mise en page/UX les jours suivants (onglet Avancé, panneau de progression, popup d'exploration de la carte SD).

## 2026-07-08 — La boîte à outils PC est née

- **Boîte à outils PC** : version de base de `RecalBoxDMD_tool.py` (console) — extraction `gamelist.xml`, conversion PNG→raw565/GIF→raw565pack, construction du cache. Le Mode 8 (vérification images manquantes) livré dès le premier jour.

## 2026-07-02 — La page de configuration web est née

- **Firmware / Config web** : première version de la page de config dans le navigateur — FR/EN/ES avec détection automatique de la langue du navigateur, infobulles sur chaque champ, upload/upload multiple/suppression de GIFs, régénération automatique des playlists, et le DMD qui se met en pause avec un message de statut pendant les opérations SD. Une série dense de correctifs de fiabilité le même jour a suivi : évitement des timeouts watchdog dans les longues boucles SD, contournements `mkdir`/`rmdir` pour les particularités FAT32 en lecture seule, message de statut flottant persistant.

## 2026-07-01 — Les thèmes horloge arrivent

- **Firmware** : intégration de `retro_clock` — 9 thèmes horloge pixel-art (Super Mario, Tetris, Pac-Man, Space Invaders, Pong, Neon, Matrix, Fire, Rainbow), remplaçant l'ancien rendu de chiffres brut.

## 2026-06-11 – 2026-06-29 — Premiers durcissements

- **Firmware** : optimisations du rendu raw565/raw565pack ; sous-dossiers alphabétiques `A..Z/#` ajoutés spécifiquement pour contourner les ralentissements FAT32 au-delà d'environ 800 fichiers par dossier ; première horloge multi-style avec luminosité configurable ; un bug de gel de playlist corrigé.

## 2026-06-10 — Naissance du projet : le fork Raw565

- **Firmware** : fork de [RetroBoxLED de Jamyz](https://github.com/Jamyz/RetroBoxLED). Le pipeline original de décodage PNG/GIF est remplacé par un format maison **raw565**/**raw565pack**, un **cache de jeux indexé par bigrammes** (`games_cache.bin`), et le **masque « L »** des systèmes lents — la fondation qui permet à un fullset MAME de 30 000 jeux de s'afficher en quelques millisecondes, sans écran noir entre deux jeux.

---

*Les dates proviennent des en-têtes de version conservés en haut de chaque fichier source (`RecalBox_DMD.ino`, `web_config.h`, `RecalBoxDMD_tool.py`, `RecalBoxDMD_GUI.py`) — la convention de changelog interne du projet, condensée ici pour la lisibilité.*
