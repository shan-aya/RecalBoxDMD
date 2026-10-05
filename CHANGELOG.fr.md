# Changelog

Historique de **RecalBoxDMD — RawEdition v2.0**, couvrant à la fois le **firmware ESP32** (y compris sa page de configuration web) et la **boîte à outils PC**, depuis le tout premier commit jusqu'à aujourd'hui. Les entrées sont groupées par date ; chaque puce est étiquetée avec la partie du projet qu'elle concerne.

[🇬🇧 English](CHANGELOG.md) · 🇫🇷 **Français** · [🇪🇸 Español](CHANGELOG.es.md)

Ceci est un résumé sélectionné de l'historique interne des versions du projet (185+ révisions firmware, 64+ révisions config web, 43+ révisions boîte à outils, 62+ révisions GUI) — regroupé par jalons réellement pertinents pour un utilisateur, pas un déversement brut de chaque micro-correctif.

---

## 2026-10-05 (1) — Firmware v2.46 : le DMD affiche ce que la Recalbox affiche

- **Changement** : l'ordre de choix des logos redevient celui de l'écran de la Recalbox. Pour un système, le DMD utilise, dans cet ordre : le logo du thème pour votre **langue**, puis pour votre **région**, puis le **logo de base** du thème. L'image par défaut traduite (français, espagnol) n'est utilisée que si le thème n'a **aucun logo** pour ce système. Cela annule la v2.45, qui plaçait l'image par défaut traduite avant les logos de base du thème et pouvait rendre le DMD différent de l'écran de la Recalbox.
- Conséquence : avec un thème sans variante pour votre langue, le DMD affiche le même logo (souvent en anglais) que l'écran de la Recalbox. Les auteurs de thèmes peuvent ajouter des variantes de langue (`path.fr`, `path.es`, `path.en`) et les deux écrans suivent.
- Mise à jour depuis la page de configuration web (bouton **Mettre à jour maintenant (Wi-Fi)**) ou la boîte à outils PC (**Mode 14**). Rien à réinstaller sur la carte SD. Les images par défaut de `systems/_defaults/es` et `fr` ont aussi été refaites (nouveaux logos, plus de fautes de texte) : lancez le **Mode 2** pour les actualiser.

## 2026-10-04 (9) — Firmware v2.45 : les images Favoris / Dernier joué / genres traduites passent avant celles, en anglais, du thème

- **Correction** : avec une langue de Recalbox qui a des images par défaut traduites (français, espagnol) et un thème qui fournit ses propres logos *Favoris*, *Dernier joué* ou de genres **sans variante pour cette langue** (par exemple Midnight en espagnol : il n'existe qu'une variante française), le DMD affichait le logo anglais du thème alors que l'image traduite était sur la carte SD. L'image traduite est maintenant utilisée ; les consoles gardent le logo du thème, et une variante de langue fournie par le thème reste prioritaire (Midnight en français est inchangé pour Favoris et Dernier joué).
- Mise à jour depuis la page de configuration web (bouton **Mettre à jour maintenant (Wi-Fi)**) ou la boîte à outils PC (**Mode 14**). Rien à réinstaller sur la carte SD.

## 2026-10-04 (8) — Firmware v2.44 et boîte à outils PC build 10567 : mise à jour du DMD par Wi-Fi

- **Nouveau** : le DMD se met désormais à jour tout seul depuis GitHub. L'avis « nouvelle version disponible » de sa page web propose un bouton **Mettre à jour maintenant (Wi-Fi)**, et la boîte à outils PC un nouveau **Mode 14 — Firmware du DMD (Wi-Fi)**, qui trouve le DMD, compare les versions et lance la mise à jour. Le DMD télécharge lui-même le firmware (environ 2 minutes, écran éteint pendant ce temps, plusieurs redémarrages), vérifie sa taille et son SHA-256 avant de l'installer et revient seul à l'ancien firmware si le téléchargement échoue ou si le nouveau ne démarre pas.
- **Une réinstallation USB est nécessaire pour passer à la 2.44** (Web Installer) : la répartition de la mémoire flash a changé (deux emplacements de firmware au lieu d'un). Votre `config.ini` et la carte SD ne sont pas touchés ; ensuite, plus de câble. Voir [Mise à jour](UPGRADING.fr.md). Compilation personnelle : schéma de partition **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)**.
- **Retiré** : le port série Bluetooth classique (désactivé par défaut, ~660 Ko de flash et ~40 Ko de RAM). Le DMD dispose maintenant d'environ 58–72 Ko de mémoire libre en fonctionnement normal au lieu de 12–24 Ko.

## 2026-10-04 (7) — Firmware v2.43 : les logos de thème Recalbox suivent aussi sur les systèmes de genre

- **Correctif** : sur Recalbox 10.1, les systèmes de genre (Sports, Stratégie, Plateforme action…) et des systèmes virtuels comme *Derniers joués* gardaient les images par défaut du DMD même avec un thème sélectionné. EmulationStation les annonce sous le nom `genre-<nom>` mais cherche leur logo de thème sous `auto-<nom>`. Le DMD essaie maintenant `auto-<nom>` quand le thème n'a pas de logo sous le nom annoncé (variantes de langue et de région comprises). Rien à réinstaller : les paquets de thèmes contiennent déjà ces logos (Midnight fournit par exemple `auto-sports`, `auto-strategy`…).
- Mise à jour depuis la page de configuration web (avis « nouvelle version disponible ») ou l'installeur web.

## 2026-10-04 (6) — Firmware v2.42, script v11 : la vérification de mise à jour couvre aussi les scripts Recalbox

- **Nouveau** : la page web du DMD et la boîte à outils PC (build `10466`, au démarrage) vous préviennent aussi quand les **scripts Recalbox** ne sont pas à jour, et vous disent quoi faire : **lancez la boîte à outils PC et faites un Mode 9, puis redémarrez la Recalbox**. L'avis de nouveau firmware le rappelle aussi.
- Comment : les scripts (`dmd_udp_resync` v11, à réinstaller avec le **Mode 9**) annoncent leur numéro au DMD, qui l'affiche sur sa page web et le donne à la boîte à outils en Wi-Fi. Rien n'est lu sur la Recalbox elle-même. Les scripts antérieurs à la v11 n'annoncent rien : le DMD les reconnaît une fois la Recalbox allumée depuis une minute et demie.
- Mettez à jour le firmware depuis la page de configuration web ou l'installeur web, puis faites un **Mode 9** une fois.

## 2026-10-04 (5) — Firmware v2.41 et script v10 : « Reprendre DMD » affiche l'état courant de la Recalbox

- **Correctif** (script `dmd_udp_resync` v10 — à réinstaller avec le **Mode 9**, puis redémarrer la Recalbox) : après **Reprendre DMD**, le DMD pouvait rester sur un écran vide tant que vous ne bougiez pas dans la Recalbox. Quand la Recalbox affichait un **système** (et non un jeu), elle répondait « playlist ». Elle envoie maintenant le système courant : son logo s'affiche tout de suite.
- **Firmware** (v2.41) : fermer ou quitter la configuration web ne force plus l'écran de configuration une fois le DMD repris ; un aperçu d'horloge lancé après la reprise ne reste plus figé à l'écran.
- Mise à jour depuis la page de configuration web (avis « nouvelle version disponible ») ou l'installeur web.

## 2026-10-04 (4) — Firmware v2.40 : la configuration web garde vos modifications d'une page à l'autre

- **Correctif** : dans la configuration web du DMD, un réglage modifié sur une page (par exemple le **mode Pinball** sur l'accueil) était perdu si l'on appuyait sur Enregistrer depuis une autre page. Désormais les modifications faites sur n'importe quelle page sont conservées pendant que vous naviguez, et **un seul Enregistrer / Enreg. & Redémarrer (depuis n'importe quelle page) les enregistre toutes**.
- Quand vous quittez avec **Reprendre DMD** ou **Redémarrer** alors que des modifications ne sont pas enregistrées, une boîte de dialogue vous le signale et propose **Enregistrer** / **Quitter quand même** (ou Annuler). Rien d'autre ne vous interrompt : passer d'une page à l'autre reste libre.
- Mise à jour depuis la page de configuration web (avis « nouvelle version disponible ») ou l'installeur web.

## 2026-10-04 (3) — Firmware v2.39 et script v9 : le changement de thème dans Recalbox arrive bien au DMD

- **Correctif** (script `dmd_udp_resync` v9 — à réinstaller avec le **Mode 9**, puis redémarrer la Recalbox) : changer de thème dans Recalbox pouvait laisser le DMD sur les logos de l'ancien thème. Les deux messages (langue/région, puis thème) partaient dans la même milliseconde et le DMD n'en gardait qu'un.
- **Firmware** (v2.39) : les messages de thème et de langue/région sont maintenant appliqués dès leur arrivée (plus aucun ne peut se perdre) ; le **dernier thème, la dernière langue et la dernière région sont mémorisés**, donc après un démarrage à froid le DMD affiche tout de suite les bons logos au lieu des logos par défaut ; le logo à l'écran est **redessiné immédiatement** quand on change de thème, de langue ou de région.
- Mettez à jour le firmware du DMD depuis la page de configuration web (avis « nouvelle version disponible ») ou l'installeur web.

## 2026-10-04 (2) — Boîte à outils PC build 10365 : logos de consoles par défaut (US) corrigés

- **Correctif** : avec les variantes de langue / région, les **logos de base** d'un thème étaient pris sur le chemin *neutre* du thème au lieu de sa variante **US**. Sur les thèmes où les deux diffèrent (par exemple **Midnight** : le logo neutre de la Super Nintendo est le *Super Famicom* japonais), le DMD affichait le logo japonais pour la région US par défaut. Corrigé dans le paquet (**10 thèmes reconstruits**, quelques logos de consoles changent par thème) et dans les conversions propres de la boîte à outils (Mode 12).
- **Pour en profiter** : réinstallez / mettez à jour vos thèmes une fois de plus (le Mode 12 les signale comme périmés ; ou Mode 1 / 13).
- **Rappel** : après le **Mode 9** (scripts Recalbox), **redémarrez la Recalbox** — les anciens scripts continuent de tourner en mémoire jusque-là, donc le DMD ne suivrait pas la langue et la région de la Recalbox.

## 2026-10-04 — Firmware v2.38 et boîte à outils PC build 10364 : la langue et la région suivent votre Recalbox, à la volée

- **Firmware** (v2.38) + **script Recalbox `dmd_udp_resync` v8** (à réinstaller avec le **Mode 9**) : le DMD prend maintenant la **langue** et la **région du thème** de votre Recalbox. La **région** choisit les logos de consoles (*Super Famicom* / *Super Nintendo*, *Mega Drive* / *Genesis*…), la **langue** choisit les textes traduits (*Favoris* / *Favorites*…). Changez l'une ou l'autre dans la Recalbox et le DMD suit en quelques secondes — sans réinstaller, sans redémarrer. Fonctionne aussi pour les images par défaut (anglais, français, espagnol) quand aucun thème n'est actif.
- **Boîte à outils PC** (build `10364`) : **plus rien à choisir** — la question de langue des Modes 1, 2 et 13 et les listes langue / région du Mode 12 disparaissent. La boîte à outils installe la base anglais / US plus **toutes les variantes** (thèmes : sous-dossiers `l_<langue>/` et `r_<région>/` ; images par défaut : `_defaults/fr/` et `_defaults/es/`), quelques centaines de Ko de plus par thème.
- **Paquet de thèmes** reconstruit dans le nouveau format (10 thèmes). **Une réinstallation est nécessaire une fois** pour obtenir les variantes : Mode 1, 12 ou 13 pour les thèmes, Mode 1 ou 2 pour les images par défaut. D'ici là, tout continue de fonctionner avec les logos anglais / US. Les anciennes boîtes à outils n'installent que les logos US de base.
- Pourquoi : la région du thème est un réglage propre à la Recalbox (US par défaut, indépendant de la langue), alors que l'ancien paquet liait la langue à la région — neuf logos de consoles de `recalbox-next` différaient de ce que montrait la Recalbox.

## 2026-10-03 (7) — Boîte à outils PC build 10263 : panneau de copie vers la SD corrigé après le Mode 13, colonne SD dans le Mode 12

- **Correctif** : après le **Mode 13** (et les Modes 2 et 11) lancé depuis l'onglet Avancé, le **panneau « copier sur la carte SD »** était masqué juste après la fin du téléchargement. Il reste maintenant visible : vous pouvez copier les fichiers sur la carte SD tout de suite.
- **Mode 12** (Gestion des thèmes Recalbox) : une nouvelle colonne **SD**, à côté de *Recalbox*, indique si chaque thème est déjà sur la carte SD du DMD (✓ présent, — absent, ? si la carte n'est pas détectée).

## 2026-10-03 (6) — Le thème par défaut de Recalbox (recalbox-next) fonctionne : boîte à outils PC build 10163 + script Recalbox v7

- **Script Recalbox `dmd_udp_resync.py` v7** (à réinstaller avec le **Mode 9**) : si vous n'avez jamais changé de thème dans Recalbox, le script annonce maintenant le thème par défaut de Recalbox, **`recalbox-next`**. Avant, il annonçait « aucun thème » dans ce cas : la plupart des utilisateurs ne voyaient donc jamais les logos de thème.
- **Paquet de logos** : **`recalbox-next`** (thème par défaut) et **`recalbox-240p`** rejoignent le paquet, qui compte maintenant **10 thèmes**. Ils sont redistribués avec l'accord de Recalbox (voir `NOTICE.txt` dans le paquet). Installez-les avec les Modes 1, 12 ou 13 (ils y sont listés ; le thème par défaut est pré-coché quand c'est celui de votre Recalbox).
- **Boîte à outils PC** (build `10163`) : sait aussi télécharger et convertir directement depuis GitLab les thèmes livrés avec Recalbox quand ils ne sont pas dans le paquet (solution de repli pour d'éventuels futurs thèmes) ; l'onglet Aide l'explique.
- Rien à flasher : le firmware est inchangé (v2.37).

## 2026-10-03 (5) — Boîte à outils PC build 10062 : l'avis de mise à jour compare avec le firmware flashé dans votre DMD

- **Boîte à outils PC** (build `10062`) : quelques secondes après l'ouverture, la boîte à outils lit la **version du firmware de votre DMD par le Wi-Fi** (elle retrouve le DMD sur votre réseau toute seule — pas besoin d'USB) et vous prévient si un firmware plus récent est publié, une fois par version publiée. Le DMD doit avoir le firmware **2.37 ou plus récent** pour annoncer sa version ; un firmware plus ancien est reconnu comme « antérieur à la v2.37 ». DMD éteint ou sur un autre réseau : l'avis reste informatif, comme avant. Elle ménage le DMD (une seule requête au démarrage, son adresse est mémorisée). Pour désactiver les vérifications : `"update_check": false` dans `RecalBoxDMD_prefs.json`.
- **Site web** : une pastille verte **« Nouveauté »** signale désormais la dernière fonctionnalité sur la page d'accueil (thèmes Recalbox).

## 2026-10-03 (4) — Firmware v2.37 et boîte à outils PC build 9961 : vous êtes prévenu quand une nouvelle version sort

- **Firmware** (v2.37) : la page d'accueil de la configuration web affiche maintenant la **version du firmware** et, quand une version plus récente est publiée, un avis **« Nouvelle version disponible »** avec un lien vers le Web Installer. La vérification est faite par **votre navigateur** (le DMD n'a besoin d'aucun accès Internet et n'utilise pas de mémoire en plus) ; hors ligne, rien ne s'affiche.
- **Boîte à outils PC** (build `9961`) : quelques secondes après l'ouverture, la boîte à outils consulte la dernière version publiée de la boîte à outils et du firmware. Si une boîte à outils plus récente existe, une fenêtre propose d'ouvrir la page de téléchargement (une seule fois par version). L'avis firmware est informatif : la boîte à outils ne peut pas savoir quelle version est flashée dans votre DMD, regardez la version sur la page web du DMD. Pour désactiver la vérification : `"update_check": false` dans `RecalBoxDMD_prefs.json`.
- **Web Installer** : la page affiche maintenant la version qu'elle va installer, et ce numéro est mis à jour automatiquement à partir du fichier du firmware lui-même (le firmware porte son propre numéro de version).
- **Rester informé** : sur GitHub, *Watch → Custom → Releases* (ou le flux des releases).

## 2026-10-03 (3) — Boîte à outils PC build 9860 : choisir les thèmes Recalbox à installer

- **Boîte à outils PC** (build `9860`) : vous décidez quels thèmes Recalbox vont sur le DMD.
  - Le **Mode 1** demande maintenant s'il faut activer le suivi des thèmes Recalbox (la nouvelle option du firmware 2.35), puis liste les thèmes installés sur votre Recalbox à côté de ceux disponibles sur GitHub, avec des cases à cocher : **seuls les thèmes cochés sont copiés**. Un thème déjà **à jour sur la carte SD** n'est ni téléchargé ni réécrit. Recalbox injoignable ? Vous obtenez la liste complète des thèmes disponibles.
  - **Nouveau Mode 13 — Thèmes Recalbox** (catégorie *DOWNLOAD FROM GITHUB*) : télécharge les thèmes que vous choisissez dans le dossier de travail, avec les mêmes questions qu'au Mode 1.
  - **Nouveau Mode 12 — Gestion des thèmes** (catégorie à part, lancé avec DÉMARRER) : compare GitHub, les thèmes installés sur votre Recalbox et la **carte SD du DMD**, signale les thèmes périmés ou jamais convertis, les met à jour et convertit ceux que GitHub ne propose pas. La liste se remplit toute seule à l'ouverture, avec un bouton *Actualiser*.
  - Le panneau de copie sur la carte SD présélectionne désormais le lecteur nommé **RECALBOXDMD** ; son bouton *Démarrer la copie* reste visible après le Mode 13 ; le menu Avancé est plus compact (le bouton Quitter était coupé). L'onglet Aide documente les nouveaux modes.
- **Pour avoir toute la fonction** : firmware **2.35** (case pour activer ou non les logos de thèmes) + cette boîte à outils. Le paquet de 8 thèmes est tenu à jour depuis le hub de thèmes Recalbox par une vérification hebdomadaire automatique sur GitHub.

## 2026-10-03 (2) — Firmware v2.35 : une case pour les logos des thèmes Recalbox

- **Firmware** (v2.34 et v2.35) : nouvelle section **« Thème Recalbox »** sur la page web (*Affichage & Playlists*) avec une case **« Suivre le thème Recalbox »**, cochée par défaut (même comportement qu'en v2.33). Décochée, le DMD affiche toujours les logos par défaut. Le thème annoncé par la Recalbox est mémorisé : recocher la case l'applique aussitôt. Le réglage est enregistré dans `config.ini` (`feat_theme_follow=`).
- **Correction** : quand le thème change **pendant qu'une animation tourne**, l'index des logos du thème (`_index.bin`) est maintenant chargé (il était ignoré faute d'un bloc de mémoire libre assez grand, et un logo absent du thème mettait environ 1 seconde à retomber sur le logo par défaut).
- **Rien à changer côté Recalbox** : le `dmd_udp_resync.py` v6 de la v2.33 est toujours le bon. L'installation des paquets de thèmes par la boîte à outils PC arrivera avec sa prochaine version.

## 2026-10-03 — Firmware v2.33 : logos des thèmes Recalbox sur le DMD

- **Firmware** (v2.33) : le DMD peut désormais afficher les **logos système du thème sélectionné dans Recalbox** (Midnight, Recalbox Next, Dashboard-X...) à la place des logos par défaut. Les scripts Recalbox envoient le nom du thème ; le firmware cherche `/systems/_defaults/_themes/<thème>/<système>.raw565` sur la carte SD et, si le logo n'y est pas, retombe sur le logo par défaut. Un petit fichier `_index.bin` par thème évite les recherches lentes sur les logos absents. Pas de dossier de thème sur la SD = rien ne change.
- **Scripts Recalbox** : `dmd_udp_resync.py` v6 (dans `tools/recalbox_scripts/dmd_helpers/`) envoie le thème au DMD au démarrage, au changement de thème et après chaque redémarrage du DMD. **Mettez ce script à jour sur votre Recalbox** pour que la fonction marche.
- **Carte SD** : les logos par défaut de `carte SD/systems/_defaults/` ont été régénérés à partir des logos vectoriels (SVG) de Recalbox, et un **paquet de logos de thèmes** (8 thèmes, variantes anglais/français/espagnol) est disponible dans `carte SD/systems/_defaults/_themes/` : copiez les thèmes que vous utilisez au même endroit sur votre carte SD. L'installation de ce paquet par la boîte à outils (Mode 1) et la fenêtre de mise à jour arriveront avec la prochaine version de la boîte à outils PC.

## 2026-09-30 — Outil PC : carte SD reconnue par Windows mais absente de l'outil

- **Outil PC** (build `8056`) : correction de la détection de la carte SD (Modes 1, 6 et 8). Certaines cartes vues par Windows comme un lecteur amovible FAT32 normal étaient **absentes de la liste sans aucun message** : l'outil demandait la liste des lecteurs à PowerShell, et toute erreur de lecture (caractère accentué dans le nom du volume, démarrage lent de PowerShell, erreur WMI) donnait une liste vide en silence. La sortie est désormais lue en UTF-8, l'attente est plus longue, et si PowerShell échoue quand même l'outil **interroge directement Windows** (appel Win32 natif) au lieu d'abandonner.
- **Si votre carte manque encore** : cliquez sur *Rafraîchir* dans la fenêtre des lecteurs, puis signalez la lettre, le système de fichiers et la taille affichés par Windows.

## 2026-09-27 — Firmware v2.32 : plus de redémarrage en boucle au démarrage avec une très longue playlist

- **Firmware** (v2.32) : correction d'un **redémarrage en boucle dès le démarrage** quand la playlist choisie est très longue (plusieurs dizaines de milliers de lignes). La reconstruction du cache de la playlist occupait le processeur assez longtemps pour déclencher le chien de garde de l'ESP32, qui redémarrait le DMD avant que le cache soit enregistré — et chaque redémarrage recommençait. La reconstruction laisse maintenant le système respirer régulièrement : testé avec une playlist de 23 842 lignes, le cache est construit une fois (une cinquantaine de secondes, sablier à l'écran) puis réutilisé instantanément aux démarrages suivants.
- **Si votre DMD est bloqué dans cette boucle** : flashez la v2.32 avec le Web Installer, ou en attendant, mettez la carte SD dans un PC et faites pointer `playlist=` dans `config.ini` vers une playlist plus courte.

## 2026-09-26 (2) — Scripts Recalbox : deux copies du même script pouvaient tourner en même temps

- **Scripts Recalbox** (`dmd_helpers/singleton_lock.sh`, partagé par `marquee`, `dmd_score` et `dmd_achievement`) : correction d'une course rare dans le verrou « une seule copie à la fois ». Pendant une rafale d'événements EmulationStation (les scripts sont relancés à chaque événement), une nouvelle copie pouvait prendre le verrou d'une copie qui venait de démarrer, et **les deux continuaient de tourner** — affichages en double ou dans le désordre sur le DMD. Mesuré sur une Recalbox Raspberry Pi très chargée : 1 rafale sur 40 avant, 0 après. La vérification est aussi plus légère (plus aucun processus lancé à chaque relance par EmulationStation).
- **Pour mettre à jour** : lancez le Mode 9 du Toolkit PC (installe les scripts depuis GitHub), puis redémarrez la Recalbox.

## 2026-09-26 — Toolkit PC : images de jeux animées de retour en navigation rapide, Mode 4 sur un dossier à plat

- **Toolkit PC** (build `8055`) : **correction d'une régression du build 8053** — le cache des systèmes marquait tous les systèmes « image fixe », si bien que le DMD n'essayait plus jamais l'image animée (`.raw565pack`) d'un jeu en navigation rapide. Le type de chaque système (fixe, animé ou les deux) est de nouveau déduit de ses **images de jeux**, sous-dossiers compris ; les images pas encore converties comptent comme ce qu'elles deviendront (`.png` = fixe, `.gif` = animée), le Mode 7 fonctionne donc toujours juste après un Mode 3. L'entrée du 2026-09-24 ci-dessous était fausse sur ce point : le type ne vient pas de `systems/_defaults/`.
- **Toolkit PC** : le **Mode 4** convertit maintenant les images posées directement dans le dossier choisi (sans sous-dossier de système) — il annonçait jusqu'ici « 0 PNG, 0 GIF » sans erreur. Elles sont écrites dans `systems/<nom du dossier>/`.

## 2026-09-24 — Toolkit PC : cache des systèmes corrigé, dossiers par défaut des Modes 2/3/6/7/11

- **Toolkit PC** (build `8053`) : correction du **Mode 7** qui annonçait « 0 système trouvé » sur un dossier dont les images de jeux ne sont pas encore converties (par exemple juste après un Mode 3). Le type de chaque système (image fixe, animation ou les deux) est maintenant lu dans son image par défaut de `systems/_defaults/`, exactement comme le fait le firmware du DMD — il était jusqu'ici déduit à tort des images de jeux du dossier du système. Vaut aussi pour le Mode 1. Le cache des systèmes ne liste plus que les dossiers de systèmes réellement présents (plus `default`), jamais plus que ce que le DMD peut garder.
- **Toolkit PC** : le drapeau « lent » du cache des systèmes compte aussi les images pas encore converties (un `.png` compte comme son futur `.raw565`, un `.gif` comme son futur `.raw565pack` + `.meta`) : un gros dossier comme `mame/S` est bien marqué lent même avant la conversion.
- **Toolkit PC** : les Modes 2, 3 et 11 ouvrent le dossier de travail à la fin ; les Modes 6 et 7 pointent par défaut sur le dossier `systems` du dossier de travail s'il existe (un dossier choisi à la main est conservé).

## 2026-09-23 (2) — Toolkit PC : affichage du Mode 9 corrigé, nettoyage des scripts

- **Toolkit PC** (build `7951`) : les boutons Pause / Reprise / Passe / Stop du cadre Progression ne disparaissent plus en bas de la fenêtre pendant le Mode 9 (ils étaient repoussés quand le texte de résultat apparaissait). Le cadre Progression garde toujours sa place, le résultat du Mode 9 a une hauteur fixe et la description du Mode 9 est plus courte (elle mentionne maintenant la copie des `.hi`).
- **Toolkit PC** : après l'installation des scripts, le message demande de **redémarrer la Recalbox** — redémarrer seulement EmulationStation laisse tourner les anciens scripts.
- **Scripts Recalbox** (`dmd_score` v54, `dmd_achievement` v8) : restes de code MQTT retirés (le DMD n'utilise plus MQTT) ; le journal du pont RetroAchievements s'appelle maintenant `dmd_achievement.log`. Aucun changement de comportement. Réinstallez avec le Mode 9, puis redémarrez la Recalbox.

## 2026-09-23 — Hi-scores indépendants de la version de MAME, tables vérifiées MAME corrigées

- **Scripts Recalbox** (`dmd_hiscore_verified.py` v2) : correction des tables de hi-score vérifiées à la main **qui ne s'affichaient jamais pour les jeux MAME** — la recherche utilisait le nom du système (`mame`) alors que les clés de la table portent la version de MAME (`mame0278_…`). Environ 2270 jeux MAME affichent maintenant leur table quand aucun vrai `.hi` n'existe. Réinstallez les scripts avec le Mode 9.
- **Scripts Recalbox** (`dmd_hiscore_generic.py` v6) : le dossier de hi-score MAME n'est plus écrit en dur — le script utilise le cœur MAME réellement utilisé par Recalbox (`mame.core` dans `recalbox.conf`, sinon le dossier où MAME a écrit en dernier). Un futur cœur MAME ne demande aucune mise à jour du script.
- **Toolkit PC** (build `7651`) : les `.hi` MAME sont copiés dans le dossier du cœur MAME utilisé sur la Recalbox (même règle), au lieu d'un dossier `mame0278` fixe. Un `.hi` existant n'est toujours jamais écrasé.
- **Données** : dans [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/), les fichiers MAME passent dans `hi/mame/hiscore/` (sans version) et un nouveau `hiscore_hi_pack_v2.zip` est utilisé par le Toolkit ; l'ancien zip reste pour le build 7550.

## 2026-09-22 — Le Toolkit PC copie les fichiers de hi-score, nouveau dossier de données hi-score

- **Toolkit PC** (build `7550`) : **le Mode 1 et le Mode 9 copient maintenant aussi les fichiers de hi-score (`.hi`)** — environ 3000 fichiers pour FBNeo et MAME 0.278 — dans `/recalbox/share/saves` sur la Recalbox, pour que le DMD ait des scores à afficher pour des jeux jamais joués. **Un `.hi` déjà présent sur la Recalbox n'est jamais écrasé** ; seuls les manquants sont ajoutés. Par le partage réseau, avec SSH en repli. Les tables de hi-score (JSON) étaient déjà installées avec les scripts.
- **Toolkit PC** : l'onglet Aide mentionne maintenant la copie des `.hi` (3 langues).
- **Docs / données** : nouveau dossier [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/) — 3161 fichiers `.hi` (FBNeo 2491, MAME 0.278 670), les deux fichiers JSON de hi-score, un manifeste de sommes de contrôle et un README, prêts à être transmis à l'équipe Recalbox. La table de hi-score `verified_default_scores.json` est mise à jour (3941 entrées : 2277 mame0278 + 1664 fbneo) ; réinstallez les scripts avec le Mode 9 pour l'obtenir.

## 2026-09-20 (2) — Mode Visual Pinball (VPX) sans redémarrage, nouvelle page d'accueil web config, options de veille Recalbox

- **Firmware** : nouveau **Mode Pinball (VPX)** — une table Visual Pinball lancée depuis Recalbox s'affiche en direct sur le DMD (protocole ZeDMD-WiFi, plugin DMDUtil côté Recalbox). **Aucun redémarrage du DMD** : il bascule sur place au lancement de la table et revient à la normale à la fin de la partie (ou environ 5 s après la dernière image). Pendant la table, la playlist et les écrans Recalbox (« RecalBox connectée »…) sont suspendus pour ne rien dessiner par-dessus. Désactivé par défaut ; la luminosité est restaurée à la sortie. Testé pour l'instant uniquement sur des tables 128×32.
- **Firmware** : pour lui faire de la place, le budget mémoire a été retravaillé — un décompresseur zlib minimal intégré (uniquement en pile) remplace le précédent, plus lourd, les tampons ne sont alloués que si le mode est activé, et l'index système/jeux en mémoire est redimensionné (300 → 160 entrées ; une ligne de log le signale si une très grosse collection atteint la limite). La mémoire libre gagne environ 12 Ko.
- **Config web** : le menu principal (page d'accueil) s'ouvre désormais sur le cadre **Affichage** — luminosité, démarrage silencieux et interrupteur **Mode Pinball (VPX)**. Son bouton Enregistrer affiche une confirmation sur la ligne 2 du DMD. La page Affichage conserve les options des écrans superposés en jeu.
- **Firmware** : le titre de démarrage du DMD affiche désormais la **vraie version du firmware** (ex. `RawEdition v2.31`) au lieu d'un `v2.0` figé ; le badge du README et le Web Installer affichent le même numéro.
- **Firmware + script Recalbox** : l'option Mode Pinball **autorise** désormais un nouveau script Recalbox, `dmd_vpx_config`, à vérifier et écrire les réglages Visual Pinball dont le DMD a besoin (DMDUtil / ZeDMD WiFi, AlphaDMD) dans `VPinballX-configgen.ini` à chaque démarrage de la Recalbox et après chaque partie — jamais pendant qu'une table tourne, et sans jamais écraser une IP que vous avez saisie. Décochée, le script ne fait rien. Un avertissement dans la bulle d'aide de l'option (config web) le précise. Nécessite de réinstaller les scripts avec le Mode 9.
- **Firmware** : corrige le mode Pinball sur les tables colorisées (Serum) — les grosses mises à jour de couleur étaient en partie perdues, d'où des fonds dessinés les uns sur les autres et des textes absents. L'émetteur coupe un message sur deux paquets UDP quand il dépasse 1400 octets ; le DMD recolle désormais ces messages au lieu de les jeter. Les tables colorisées (Diner…) s'affichent correctement.
- **Firmware** : corrige l'absence des overlays hi-score / infos jeu / description quand le DMD est allumé *avant* la Recalbox (seul un redémarrage du DMD y remédiait) — le DMD renvoie désormais ses réglages à la Recalbox à la première réponse après son démarrage ou après une coupure.
- **Docs** : les tables anciennes à afficheur à segments (alphanumérique) demandent d'activer le plugin VPX **AlphaDMD** côté Recalbox — voir le README (section Pinball). Testé sur 8 tables : 6 fonctionnent ; deux (Black Hole, Farfalla) ont une disposition d'afficheur que ce plugin ne gère pas.
- **Firmware** : corrige l'écran *RecalBox connectée* qui s'affichait alors que la Recalbox était éteinte (puis remplacé par *RecalBox hors ligne*) après « Reprendre DMD », le bouton Enregistrer de la page d'accueil ou la sortie du mode Pinball — il n'apparaît désormais qu'une fois la Recalbox réellement joignable.
- **Config web** : un bouton **Accueil** figure désormais en premier dans le bandeau de menu de toutes les pages de configuration (Affichage, Playlist, Wi-Fi & BT, Horloge, Médias).
- **Config web** : nouvelle section **Veille Recalbox** — pendant les économiseurs d'écran « démos de jeux » / « clips vidéo de jeux », choisissez entre suivre le logo du jeu (comme avant, par défaut) ou la playlist simple, comme les autres veilles.
- **Scripts Recalbox** (`marquee` v54, `dmd_score` v53) : les options de veille ci-dessus, et une sortie immédiate du Mode Pinball à la fin d'une partie. Réinstallez les scripts avec le Mode 9 pour en bénéficier.
- **Docs** : le README (3 langues) documente le Mode Pinball, la page d'accueil et les nouvelles clés `config.ini` (`feat_vpinball_dmd`, `feat_demo_follow`, `feat_clip_follow`).
- **Toolkit PC** (build `7449`) : l'onglet Aide est maintenant un vrai manuel d'utilisation (premier lancement, Mode 1 pas a pas, chaque mode de l'onglet Avance, onglet Playlist, scripts Recalbox dont `dmd_vpx_config`, depannage) au lieu d'une copie du README ; plus de mentions de MQTT.

## 2026-09-20 — IP du DMD découverte automatiquement, Toolkit PC adapté au DPI Windows, image shuffle installée, renommage d'étiquette SD rétabli

- **Scripts Recalbox** : corrige le DMD qui restait bloqué sur sa playlist de veille pendant une partie — les scripts avaient l'adresse IP du DMD codée en dur (`192.168.0.51`) et parlaient donc dans le vide sur tout réseau où le DMD avait une autre adresse. Ils utilisent désormais l'adresse depuis laquelle le DMD s'annonce réellement (repli sur l'ancienne valeur tant qu'aucune n'a été vue). Fait suite à un retour réel d'un testeur.
- **Firmware** : le renommage d'étiquette de la carte SD est de nouveau actif — il était resté désactivé depuis une session de diagnostic.
- **Toolkit PC** : toute l'interface suit maintenant la mise à l'échelle d'affichage de Windows (125 %, 150 %...) et pas seulement les polices — à 125 % sur un écran 4K, le bas de la fenêtre (la section Progression) était coupé. Pris en compte à la prochaine ouverture de session si l'échelle vient d'être changée.
- **Toolkit PC** : le numéro de build est maintenant affiché dans le bandeau de la fenêtre (actuellement `7349`), pour savoir facilement quelle version quelqu'un utilise.
- **Toolkit PC** : le Mode 9 rappelle désormais de redémarrer EmulationStation après l'installation des scripts utilisateur — le menu *Scripts utilisateur* reste grisé tant qu'EmulationStation n'a pas relu ses scripts au démarrage.
- **Toolkit PC** : la création de carte SD installe aussi l'image de brouillage CRT du mode shuffle (`_shuffle.raw565pack` + `_shuffle.meta`) ; auparavant ces deux fichiers étaient ignorés et le DMD affichait un écran vide en mode shuffle. Si ta SD a été créée avec un build antérieur, copie ces deux fichiers depuis `carte SD/systems/_defaults/` vers `systems/_defaults/` sur la carte.
## 2026-09-14 — MQTT entièrement retiré, crash watchdog corrigé, IP fixe optionnelle, page FAQ

- **Firmware** : le sous-système MQTT (code de connexion/tâche, ~800 lignes) est désormais entièrement retiré des sources — pas seulement désactivé par défaut comme la veille. Si tu avais branché quelque chose sur les anciens topics MQTT du DMD, voir [UPGRADING.md](UPGRADING.fr.md) pour ce que ça implique.
- **Firmware** : corrige un écran « RecalBox connectée » affiché à tort même quand la Recalbox est éteinte — il se déclenchait auparavant au simple ENVOI d'un message UDP, sans confirmation qu'il avait bien été reçu ; attend désormais une vraie réponse avant de s'afficher.
- **Firmware** : corrige un vrai crash — une simple visite normale de la page de configuration web pouvait occasionnellement déclencher le watchdog matériel de l'ESP32 et forcer un redémarrage (`WebServer::_parseRequest()` pouvait attendre activement jusqu'à 5 secondes sans jamais rendre la main, tout près de la limite des 5 secondes du watchdog de la plateforme elle-même). Corrigé et confirmé sur matériel réel : 300 requêtes rapprochées reproduisant exactement le déclencheur d'origine, aucun échec.
- **PC Toolkit** : l'étape de configuration WiFi du Mode 1 peut désormais aussi définir une IP fixe sur le DMD (optionnel, décoché par défaut) — inclut un avertissement explicite contre la configuration simultanée d'une réservation DHCP côté routeur ET d'une IP statique côté DMD (peut causer de vrais problèmes de connexion), et pré-remplit les champs réseau à partir de la configuration de ce PC pour réduire le risque de faute de frappe.
- **PC Toolkit** : corrige une fausse erreur « impossible de joindre la Recalbox » dans le Mode 1 malgré une IP correcte — le test de joignabilité ne faisait qu'une connexion TCP brute au port SMB, que certaines règles pare-feu/antivirus bloquent pour un processus non reconnu, alors que l'Explorateur Windows accède normalement au même partage via son propre client SMB ; l'outil retente désormais un vrai accès au partage (le même mécanisme que l'Explorateur) avant d'abandonner.
- **PC Toolkit** : tentative de correctif pour la disparition du bas de l'interface (la section Progression) sur les écrans haute résolution (signalé par un testeur sur un écran 4K à 125 % de mise à l'échelle) — des retours ultérieurs ont montré que ça ne couvrait pas tous les cas ; un correctif complémentaire est en cours.
- **Docs** : nouvelle page dédiée [FAQ et dépannage](FAQ.fr.md) — sensibilité à l'alimentation USB (luminosité vs appel de courant), remarques sur la révision du chip ESP32, qualité de la carte microSD, boucles de configuration WiFi, conseils pour l'IP fixe, et plus — reliée depuis la section Dépannage du README.

## 2026-09-13 — MQTT → UDP : fusion de `dev/dmd-udp-transport` sur master

- **Firmware** : la liaison temps réel entre Recalbox et le DMD passe de **MQTT à UDP** — l'ancien transport se heurtait à un mur au niveau de la plateforme, dans la pile TCP/IP de l'ESP32 (un `TCP_SND_BUF` fixe d'environ 5,7 Ko, figé dans le core Arduino précompilé, sans levier applicatif possible) qui pouvait bloquer un `SUBSCRIBE` MQTT plusieurs secondes après une reconnexion ; l'UDP n'a pas cette poignée de main à bloquer. Le support MQTT était encore présent dans le firmware à ce moment-là, désactivé par défaut, comme filet de repli — entièrement retiré le lendemain, voir ci-dessus.
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
