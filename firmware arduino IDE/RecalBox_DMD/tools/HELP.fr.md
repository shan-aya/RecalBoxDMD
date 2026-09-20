# 🎮 Boîte à outils PC RecalBoxDMD — Aide

🇫🇷 **Français** · 🇬🇧 English (`HELP.md`) · 🇪🇸 Español (`HELP.es.md`) — la langue de cette page suit la langue choisie dans l'onglet **Paramètres**.

> Ceci est le **manuel d'utilisation de la boîte à outils PC** (l'application Windows que vous utilisez en ce moment). Pour le projet lui-même — firmware, page de configuration web, support Visual Pinball, matériel — voir la [page du projet sur GitHub](https://github.com/shan-aya/RecalBoxDMD) et sa [FAQ](https://github.com/shan-aya/RecalBoxDMD/blob/main/FAQ.fr.md).

---

## 📋 Sommaire

1. [Ce que fait la boîte à outils](#1-ce-que-fait-la-boîte-à-outils)
2. [Avant de commencer](#2-avant-de-commencer)
3. [La fenêtre en un coup d'œil](#3-la-fenêtre-en-un-coup-dœil)
4. [Première utilisation — la méthode simple (Mode 1)](#4-première-utilisation--la-méthode-simple-mode-1)
5. [Le Mode 1 en détail](#5-le-mode-1-en-détail)
6. [L'onglet Avancé — Modes 2 à 11](#6-longlet-avancé--modes-2-à-11)
7. [L'onglet Playlist](#7-longlet-playlist)
8. [Les onglets Paramètres, Logs et Aide](#8-les-onglets-paramètres-logs-et-aide)
9. [Scripts Recalbox (Mode 9)](#9-scripts-recalbox-mode-9)
10. [Après la copie — premier démarrage du DMD](#10-après-la-copie--premier-démarrage-du-dmd)
11. [Dépannage](#11-dépannage)

---

## 1. Ce que fait la boîte à outils

Le DMD est un panneau LED 128 × 32 piloté par un ESP32. Il lit ses images sur une carte microSD, dans un format compact qui ne demande aucun décodage sur le panneau. La boîte à outils **construit cette carte SD pour vous** :

- elle lit les fichiers `gamelist.xml` de votre Recalbox et récupère l'image **marquee / logo** de chaque jeu (celle que vous avez scrapée) ;
- elle convertit tout au format du panneau (`.raw565` pour les images fixes, `.raw565pack` + `.meta` pour les GIFs) et construit les deux fichiers d'index du firmware (`games_cache.bin`, `systems_cache.dat`) ;
- elle télécharge les images par défaut (logos des systèmes, genres, image de secours) et, si vous le souhaitez, le pack gratuit d'environ 600 GIFs pour les playlists de veille ;
- elle peut régler le WiFi du DMD, installer les **scripts Recalbox** qui relient le jeu que vous lancez au panneau, et tout copier sur la carte SD.

Vous n'avez **pas** besoin de connaître les formats de fichiers : le **Mode 1** automatique fait tout et vous pose quelques questions en chemin.

---

## 2. Avant de commencer

- **Windows 10 ou 11.** La boîte à outils existe en version portable ou installée ; les deux fonctionnent pareil.
- **Une carte microSD d'au moins 8 Go, formatée en FAT32** (la boîte à outils vérifie les deux et refuse le reste).
- **Votre dossier de ROMs Recalbox**, accessible depuis ce PC — un partage réseau (`\\RECALBOX\share\roms`, ou votre NAS) ou une copie sur un disque. Les systèmes doivent avoir été **scrapés** pour que chaque jeu ait une image marquee/logo (voir [Scraping](#scraping--récupérer-les-images-marquee)).
- **Votre Recalbox allumée et sur le même réseau** si vous voulez que la boîte à outils installe les scripts à votre place (sinon vous pouvez les copier à la main, voir le [Mode 9](#9-scripts-recalbox-mode-9)).
- **Le nom et le mot de passe de votre WiFi** — un réseau **2,4 GHz** ; l'ESP32 ne sait pas utiliser le 5 GHz.

### Scraping — récupérer les images marquee

La boîte à outils lit l'image déclarée dans chaque `gamelist.xml`. L'endroit où Recalbox la range dépend de votre version — choisissez le profil correspondant dans **Version Recalbox** (onglet Main ou Paramètres), et utilisez le bouton **Comment scraper ?** pour voir les captures exactes du menu :

| Profil | Réglage du scraper Recalbox | Dossier / balise lus |
|---|---|---|
| **10.x** (recommandé) | champ **SÉLECTIONNEZ LE TYPE DE LOGO** = **CLEAR** | `media/wheels/`, balise `<logo>` |
| **9.x** | type de vignette = **MARQUEE** | `media/thumbnails/`, balise `<thumbnail>` |
| **legacy** | type d'image = **LOGO DÉTOURÉ** (Clear Logo) ou **MARQUEE** | `media/images/`, balise `<image>` |

Le bouton **Nettoyer les dossiers avant scrape** supprime les images déjà scrapées des systèmes sélectionnés (uniquement ces images — jamais les ROMs ni les `gamelist.xml`), utile avant de re-scraper avec un autre réglage. Il demande confirmation.

---

## 3. La fenêtre en un coup d'œil

La fenêtre comporte **six onglets** :

| Onglet | À quoi il sert |
|---|---|
| **Main** | Le **Mode 1** automatique : choisir le dossier de ROMs, les systèmes, la version Recalbox, et appuyer sur **DÉMARRER**. |
| **Playlist** | Construire vos propres playlists de veille à partir des GIFs de la carte SD ou de dossiers de GIFs de votre PC. |
| **Avancé** | Les modes 2 à 11 séparément, regroupés par thème (téléchargements GitHub, gamelist, outils d'images, caches, scripts). |
| **Logs** | Toute la sortie texte de ce que fait la boîte à outils, avec un filtre de niveau. |
| **Paramètres** | Langue (français / anglais / espagnol), thème de couleurs, profil de version Recalbox, seuil flag L (systèmes lents). |
| **Aide** | Cette page. |

En bas, le panneau **Progression** montre l'étape en cours avec quatre boutons : **Pause / Reprise**, **Passe** (saute l'étape en cours) et **Stop**. Le numéro dans la barre de titre est le **numéro de build** de votre boîte à outils — indiquez-le quand vous demandez de l'aide.

Un dossier de travail temporaire (`sd_card`) sert à tout préparer avant la copie sur la carte SD. Quand vous quittez, la boîte à outils propose de le supprimer (cochez *Conserver le dossier temporaire* pour le garder).

---

## 4. Première utilisation — la méthode simple (Mode 1)

1. Ouvrez l'onglet **Main**.
2. **Choisir dossier ROMs** — le dossier qui contient un sous-dossier par système (`snes`, `mame`, …). Si les ROMs sont sur un NAS qui demande un identifiant, une boîte **Identifiants NAS** apparaît : saisissez l'utilisateur et le mot de passe.
3. Cliquez sur **Détection des systèmes (gamelist.xml)**. La liste *Systèmes à traiter* se remplit. Cliquez pour sélectionner les systèmes voulus (ou **Tout sélectionner**). *Si vous ne sélectionnez rien, la boîte à outils vous le dit et s'arrête.*
4. Vérifiez le profil **Version Recalbox** (voir [Scraping](#scraping--récupérer-les-images-marquee)).
5. Cliquez sur **DÉMARRER** et répondez aux questions (elles sont toutes posées d'emblée, pour que vous puissiez ensuite laisser le PC travailler).
6. Quand c'est **Terminé**, choisissez **Explorer la carte SD** pour voir le résultat, mettez la carte dans le DMD et allumez-le (voir [Premier démarrage](#10-après-la-copie--premier-démarrage-du-dmd)).

---

## 5. Le Mode 1 en détail

### Les questions posées quand vous appuyez sur DÉMARRER

Dans cet ordre :

1. **WiFi du DMD (facultatif)** — choisissez le réseau **2,4 GHz**, saisissez le mot de passe et appuyez sur **Vérifier** : le PC se connecte brièvement à ce réseau pour prouver que le mot de passe est bon. *Ignorer (configurer plus tard)* est possible : le DMD proposera alors son propre point d'accès au premier démarrage.
   - **Définir aussi une IP fixe pour le DMD (avancé)** — uniquement si votre box/routeur ne donne *pas* déjà une adresse fixe au DMD. Utilisez **une seule** des deux méthodes (une réservation sur le routeur, **ou** cette IP fixe), jamais les deux. Les champs sont pré-remplis d'après le réseau de votre PC, à titre indicatif : vérifiez-les.
2. **Recalbox** — la boîte à outils cherche votre Recalbox sur le réseau et affiche son **adresse IP** pour confirmation (important si plusieurs Recalbox sont allumées). *Non* / rien trouvé → saisissez son IP ou son nom à la main ; si elle est injoignable, vous pouvez **ressaisir l'IP** ou choisir **Mode 9 plus tard**.
3. **Image de secours** — l'image affichée sur le panneau quand un jeu n'en a pas. Oui → choisissez une des propositions ou importez la vôtre (elle est redimensionnée automatiquement). Non → garde l'actuelle.
4. **Langue des images système** — français, anglais ou espagnol pour les badges de systèmes/genres (Favoris, Derniers joués, …).
5. **Carte SD** — choisissez le lecteur (**Actualiser** s'il n'apparaît pas). FAT32 et 8 Go minimum.
6. **Pack de GIFs gratuit** — télécharge les ~600 GIFs (arcade, consoles, ordinateurs, flipper, Halloween, Noël, …) depuis GitHub pour les playlists de veille. Le téléchargement démarre tout de suite en arrière-plan.
7. **GIFs personnalisés** — *Oui* vous emmène dans l'onglet **Playlist** en mode temporaire : **Ajouter un dossier PC…**, **Copier la sélection**, éventuellement **Construire la playlist**, puis appuyez sur le bouton orange clignotant **Continuer** pour reprendre.

### Ce que fait ensuite le pipeline automatique

1. **Prépare** le dossier de travail et écrit la langue et l'indicateur de « premier démarrage » dans le `config.ini` du DMD.
2. **Installe les scripts Recalbox** (voir [Mode 9](#9-scripts-recalbox-mode-9)) — d'abord une copie locale dans `recalbox_userscripts`, puis sur la Recalbox elle-même si elle a été confirmée. L'IP de la Recalbox est écrite dans le `config.ini` pour que la page web du DMD soit pré-remplie.
3. **Extrait** l'image marquee de chaque jeu depuis les `gamelist.xml` (la liste des images manquantes est enregistrée dans `images_manquantes.txt`).
4. **Convertit** au format brut 128 × 32, puis supprime les `.png`/`.gif` d'origine qui ont été convertis.
5. **Construit `games_cache.bin`**, télécharge les images **`_defaults`** (avec votre langue et l'image de secours), le **pack de GIFs** et les **playlists** (une playlist par défaut plus `ALL.txt`), puis construit **`systems_cache.dat`**.
6. **Copie sur la carte SD.** Si des fichiers existent déjà, on vous demande de les *écraser* ou de les *ignorer* ; une copie interrompue peut être **reprise** la fois suivante.

Selon la taille de votre collection, cela va de quelques minutes à un long moment (un set MAME de 30 000 jeux est le cas lent). Vous pouvez **mettre en pause**, **passer** une étape ou **arrêter** à tout moment.

---

## 6. L'onglet Avancé — Modes 2 à 11

La colonne de gauche regroupe les modes en cinq catégories dépliables ; le centre montre les options du mode choisi ; **Détails du mode selectionné** l'explique. Servez-vous-en pour refaire une seule partie du travail.

| Catégorie | Mode | Ce qu'il fait |
|---|---|---|
| **DOWNLOAD FROM GITHUB** | **2 — `_defaults`** | Extrait les marquees *et* télécharge les images par défaut (`systems/_defaults`) depuis GitHub. Demande s'il faut écraser les fichiers existants. |
| | **11 — Pack 600 GIFs** | Télécharge le pack de GIFs gratuit dans `gifs/`. |
| **GAMELIST.XML** | **3 — Extraction** | Extrait seulement les images marquee depuis les `gamelist.xml` (choix du dossier de ROMs et des systèmes). |
| | **8 — Images manquantes** | Vérifie quels jeux n'ont **aucune** image et écrit un rapport (`mode8_report_*.txt`) ; un second bouton compare ce rapport au dossier de travail et à la carte SD (`mode8_final_report_*.csv/.txt`). |
| **IMAGES TOOLS** | **4 — PNG/GIF → raw565** | Convertit les PNG (→ `.raw565`) et GIF (→ `.raw565pack` + `.meta`) d'un dossier de votre choix. |
| | **5 — 128×32** | Redimensionne/convertit seulement les images en 128 × 32. |
| | **10 — Image de secours** | Choisit (ou réinitialise) l'image affichée quand rien d'autre ne correspond. |
| **CACHES** | **6 — `games_cache.bin`** | Reconstruit l'index des jeux depuis le dossier `systems`. |
| | **7 — `systems_cache.dat`** | Reconstruit l'index des systèmes (demande le dossier `systems` qui contient `_defaults`). |
| **SCRIPTS RECALBOX** | **9 — Installer les scripts** | Installe / met à jour les scripts Recalbox (voir plus bas). |

Chaque mode a son propre bouton **Choisir le dossier …** et son propre bouton **DÉMARRER**. Les modes 3, 4, 5, 6 et 7 travaillent sur le dossier de travail courant sauf si vous les dirigez ailleurs.

**Seuil flag L (systèmes lents)** (onglet Paramètres) : les systèmes qui ont plus de fichiers convertis que ce nombre sont marqués *lents* ; le panneau affiche alors une image par défaut pendant que la vraie se charge. Augmentez-le si votre carte SD est rapide, baissez-le si elle est lente.

---

## 7. L'onglet Playlist

Une **playlist** est la liste des GIFs que le DMD joue quand rien ne se passe sur la Recalbox (veille / mode attraction).

- **Carte SD** — choisissez le lecteur (🔄 *Actualiser*). L'onglet montre les dossiers `gifs/` et leurs fichiers.
- **Cochez un dossier entier** pour prendre tous ses GIFs, ou **cliquez sur son nom** pour cocher les fichiers un par un (survol d'un fichier = aperçu animé). Une ligne **orange** indique une sélection partielle ; le dossier affiché est souligné.
- **Ajouter un dossier PC…** importe un ou plusieurs dossiers de GIFs de votre ordinateur (plusieurs à la fois possible). Décochez les fichiers dont vous ne voulez pas, puis **Copier la sélection**.
- **Supprimer** retire les dossiers/fichiers cochés (définitif, avec confirmation ; les playlists qui les utilisaient sont mises à jour).
- **Nom de la playlist** + **Construire la playlist** enregistre votre sélection sous ce nom (existant ou nouveau).
- **Régénérer le cache playlist** (bouton orange) reconstruit `cache_master_gifs.dat`, un récapitulatif de tous les GIFs de la carte (usage interne).

C'est dans la page web du DMD (page Playlist) que l'on choisit **quelle** playlist est active.

---

## 8. Les onglets Paramètres, Logs et Aide

- **Paramètres** — *Langue* (Français / English / Español, s'applique à toute l'application), *Thème* (couleurs de la fenêtre), *Version Recalbox* (le profil de scraping, partagé avec l'onglet Main) et le *Seuil flag L (systèmes lents)*. Vos choix sont mémorisés.
- **Logs** — tout ce que la boîte à outils écrit pendant son travail. Le filtre **Niveau** affiche *Tout*, *Alertes+Erreurs* ou *Erreurs* seulement. En cas de problème, c'est le premier endroit à regarder, et ce qu'il faut copier pour signaler un souci.
- **Aide** — ce manuel. **Ouvrir dans le navigateur** l'affiche dans votre navigateur web.

---

## 9. Scripts Recalbox (Mode 9)

Le DMD n'affiche que ce que la Recalbox lui dit. Le lien est un ensemble de petits **scripts** que Recalbox exécute à ses évènements (jeu sélectionné, jeu lancé, jeu terminé, économiseur d'écran…). **Le Mode 1 les installe pour vous** ; le **Mode 9** les installe ou les **met à jour** à tout moment (faites-le après chaque mise à jour de la boîte à outils).

**Comment :** onglet *Avancé* → *SCRIPTS RECALBOX* → **Mode 9**, saisissez l'**IP ou le nom réseau** de la Recalbox, appuyez sur **Installer / Mettre à jour**. La boîte à outils copie les fichiers dans le dossier `share/userscripts` de la Recalbox — par le partage réseau, ou automatiquement en SSH si le partage est bloqué.

**Ce qui est installé**

- **Scripts d'évènements** (se lancent tout seuls) : le pont *marquee*, le script *hi-score / infos jeu / Challenge RB*, le script *RetroAchievements*, et `dmd_vpx_config`, qui règle les paramètres DMD de Visual Pinball **uniquement si** l'option **Mode Pinball (VPX)** est cochée sur la page web du DMD. Leurs fichiers d'aide vont dans `dmd_helpers/`.
- **Scripts manuels** (dans le menu Recalbox **Scripts utilisateur**) : *DMD Config Web*, *DMD Luminosité +10 % / −10 %*, *DMD Reboot* et *DMD WiFi Recovery*.

> **Redémarrez EmulationStation** (ou la Recalbox) après l'installation, sinon le menu **Scripts utilisateur** reste grisé : Recalbox ne cherche les scripts qu'à son démarrage.

**Si la Recalbox est injoignable** (éteinte, mauvaise IP…), la boîte à outils garde une version prête à copier dans le dossier `recalbox_userscripts` du dossier de travail : copiez son contenu vous-même dans `share/userscripts` de votre Recalbox.

---

## 10. Après la copie — premier démarrage du DMD

1. Mettez la carte dans le DMD et allumez-le. Il affiche son titre (`RawEdition v…`) puis le panneau lance la playlist de veille.
2. Si vous avez saisi le WiFi dans le Mode 1, le DMD rejoint votre réseau tout seul. Sinon il ouvre son propre point d'accès WiFi : connectez-vous-y et suivez la page (choix du réseau et IP de la Recalbox).
3. Ouvrez la **page de configuration web** du DMD (son IP s'affiche sur le panneau) pour régler la luminosité, la playlist, l'horloge et le lien avec la Recalbox. La page du projet explique chaque option.
4. Lancez un jeu sur la Recalbox : le panneau doit passer sur le marquee de ce jeu.

---

## 11. Dépannage

| Problème | Que faire |
|---|---|
| **La carte SD n'est pas listée / refusée** | Elle doit être en **FAT32** et faire **≥ 8 Go**. Cliquez sur **Actualiser**. Reformatez en FAT32 si besoin (une carte de 64 Go ou plus doit être formatée avec un outil FAT32). |
| **« Aucun système détecté »** | Le dossier de ROMs est mauvais (choisissez celui qui contient les dossiers de systèmes) ou les dossiers d'images sont absents. Pour un NAS, saisissez **utilisateur / mot de passe NAS** puis recliquez sur *Détecter les systèmes*. |
| **Beaucoup de jeux sans image** | Le profil de scraping ne correspond pas à la façon dont vous avez scrapé (voir [Scraping](#scraping--récupérer-les-images-marquee)) — changez la **Version Recalbox** et utilisez **Comment scraper ?**. Le Mode 8 liste les manquantes. |
| **« Recalbox injoignable »** | Vérifiez qu'elle est allumée et sur le même réseau, ainsi que son IP. Réessayez avec l'IP, ou terminez puis utilisez le **Mode 9** plus tard, ou copiez `recalbox_userscripts` à la main. Sans les scripts, le DMD n'affiche que la playlist et l'horloge. |
| **Le menu Scripts utilisateur est grisé** | Redémarrez EmulationStation (ou la Recalbox) — voir [Mode 9](#9-scripts-recalbox-mode-9). |
| **Le DMD ne se connecte pas au WiFi** | Utilisez un réseau **2,4 GHz** et vérifiez le mot de passe (le Mode 1 peut le vérifier). Si vous avez défini une IP fixe, assurez-vous que le routeur n'en réserve pas une aussi (une seule méthode). |
| **Le DMD n'affiche rien quand un jeu démarre** | Scripts absents ou anciens → relancez le **Mode 9**, puis redémarrez EmulationStation. Vérifiez l'IP de la Recalbox dans la page web du DMD. |
| **La fenêtre est coupée / texte trop petit (écran haute densité)** | L'interface suit la mise à l'échelle de Windows ; déconnectez-vous puis reconnectez-vous après avoir changé l'échelle. Le numéro de build dans la barre de titre nous aide à reproduire un problème. |
| **Une copie sur la carte SD s'est arrêtée** | Relancez la copie du Mode 1 : la boîte à outils propose de **reprendre** là où elle s'est arrêtée. |

**Toujours bloqué ?** Copiez le contenu de l'onglet **Logs** et le numéro de build de la barre de titre, et ouvrez un ticket sur la [page GitHub](https://github.com/shan-aya/RecalBoxDMD) — ou lisez d'abord la [FAQ](https://github.com/shan-aya/RecalBoxDMD/blob/main/FAQ.fr.md) (alimentation USB, qualité de la carte SD, boucles WiFi, IP fixe…).
