# Changelog

Historique de **RecalBoxDMD — Raw565 Edition**, couvrant à la fois le **firmware ESP32** (y compris sa page de configuration web) et la **boîte à outils PC**, depuis le tout premier commit jusqu'à aujourd'hui. Les entrées sont groupées par date ; chaque puce est étiquetée avec la partie du projet qu'elle concerne.

[🇬🇧 English](CHANGELOG.md) · 🇫🇷 **Français** · [🇪🇸 Español](CHANGELOG.es.md)

Ceci est un résumé sélectionné de l'historique interne des versions du projet (76+ révisions firmware, 58+ révisions config web, 34+ révisions boîte à outils, 48+ révisions GUI) — regroupé par jalons réellement pertinents pour un utilisateur, pas un déversement brut de chaque micro-correctif.

---

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
