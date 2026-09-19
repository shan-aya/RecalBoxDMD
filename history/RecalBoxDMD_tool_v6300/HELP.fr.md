# 🎮 RecalBoxDMD Toolkit — Aide complète

> **RecalBoxDMD** = Recalbox + une vraie enseigne LED 128×32 pour ta borne d'arcade, pilotée depuis ton PC par cet outil.

---

## 📋 Sommaire

1. [Vue d'ensemble](#-vue-densemble)
2. [Matériel nécessaire](#-matériel-nécessaire)
3. [Logiciels nécessaires](#-logiciels-nécessaires)
4. [Démarrage rapide](#-démarrage-rapide)
5. [Interface graphique (GUI)](#️-interface-graphique-gui)
   - [Onglet Main](#-onglet-main)
   - [Onglet Playlist](#-onglet-playlist)
   - [Onglet Avancé](#-onglet-avancé)
   - [Onglet Logs](#-onglet-logs)
   - [Onglet Paramètres](#️-onglet-paramètres)
   - [Onglet Aide](#-onglet-aide)
6. [Référence des modes](#-référence-des-modes)
7. [Déroulé recommandé](#-déroulé-recommandé)
8. [Tutoriel de scraping Recalbox](#-tutoriel-de-scraping-recalbox)
9. [Assemblage du panneau DMD](#-assemblage-du-panneau-dmd)
10. [Firmware ESP32](#-firmware-esp32)
    - [Compilation depuis les sources](#-compilation-depuis-les-sources)
    - [Configuration (config.ini)](#️-configuration-configini)
    - [Installeur Web](#-installeur-web)
    - [Liaison UDP & Telnet](#-liaison-udp--telnet)
11. [Écrans superposés en jeu — Hi-Score, infos jeu, succès & RB Challenge](#-écrans-superposés-en-jeu--hi-score-infos-jeu-succès--rb-challenge)
12. [Le format raw565 en détail](#-le-format-raw565-en-détail)
13. [Structure de la carte SD](#-structure-de-la-carte-sd)
14. [Dépannage](#-dépannage)
15. [Fichiers générés par le script](#-fichiers-générés-par-le-script)
16. [Crédits](#-crédits)

---

## 🎯 Vue d'ensemble

**RecalBoxDMD Toolkit** est l'application Windows qui prépare tout ce dont une vraie enseigne LED a besoin, à partir de ta collection de ROMs Recalbox existante, jusqu'à une carte SD prête à l'emploi — en un clic, même sur un fullset MAME de 30 000 jeux.

### 🔥 Le problème résolu

Un firmware naïf qui lit des PNG/GIF directement depuis la carte SD étouffe sur les grosses collections :

```
❌ Sans cet outil :
   - L'ESP32 ouvre /systems/mame/ (30 000+ fichiers) → fige plusieurs secondes
   - Décodage PNG/GIF sur l'ESP32 lui-même → lent, gourmand en mémoire
   - Entre chaque jeu : écran noir ou lag visible
```

L'outil convertit tout à l'avance sur ton PC, dans un format que l'ESP32 peut afficher **sans rien décoder** :

```
✅ Avec cet outil :
   1. PNG → .raw565 (8 192 octets, prêt à afficher)
   2. GIF → .raw565pack (frames concaténées) + .meta (timings)
   3. L'ESP32 lit le fichier et l'envoie directement au panneau
   4. Aucun décodage, aucun lag : 5-15 ms par affichage
```

| Mesure | Sans conversion | Avec RecalBoxDMD Toolkit |
|--------|--------------------|---------------------------|
| Temps d'affichage | 500 ms – 3 s+ | **5-15 ms** |
| RAM nécessaire sur l'ESP32 | 50-100 Ko | **8 Ko** |
| Fullset MAME (30 000 jeux) | fige 5-10 s | **aucun gel, aucun écran noir** |
| Mise en place | manuelle, fichier par fichier | **un clic sur « Démarrer »** |

### Le système de masque, en bref

Les très gros systèmes (MAME, FBNeo...) sont marqués **« L »** par l'outil. Quand le firmware reçoit un jeu de ce type, il affiche **instantanément** l'image par défaut du système en cache pendant que la vraie image se décode en arrière-plan — le panneau ne devient jamais noir, même en parcourant rapidement une grosse collection. Détail complet dans [Le format raw565 en détail](#-le-format-raw565-en-détail).

### 🔄 Comment tout s'articule

```
┌─────────────────────────────────────────────────────────────┐
│                          RECALBOX                            │
│   Lance un jeu → marquee[...].sh envoie "mame/kof98"          │
│                          via UDP                              │
└──────────────────────────────┬────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│               ESP32 + Panneau LED HUB75 128×32                 │
│                                                                 │
│  Reçoit "mame/kof98" :                                          │
│   1. /systems/mame/kof98.raw565 (ou .raw565pack) → instantané  │
│   2. pas trouvé ? recherche dans games_cache.bin (index bigram)│
│   3. toujours pas ? affiche /systems/_defaults/mame.raw565     │
│   4. toujours pas ? affiche /systems/_defaults/default.raw565  │
│                                                                 │
│   ⏱️ 5-15 ms au total, quelle que soit la taille de la collection│
└─────────────────────────────────────────────────────────────┘

           ┌───────────────────────────────────────────────┐
           │   RecalBoxDMD Toolkit (cette appli) — prépare la SD│
           │  Extrait les enseignes depuis gamelist.xml        │
           │  PNG → .raw565   /   GIF → .raw565pack + .meta   │
           │  Construit le cache bigram des jeux               │
           │  Marque les systèmes lents (« L ») pour le masque │
           │  Télécharge les assets gratuits (_defaults + 600 GIFs) │
           │  Installe les scripts Recalbox (Mode 9)            │
           │  Copie tout sur la carte SD (reprise possible)     │
           └───────────────────────────────────────────────┘
```

---

## 🧰 Matériel nécessaire

| Composant | Référence | Prix approx. |
|-----------|-----------|---------------|
| 🧠 Microcontrôleur | ESP32 DevKit V1 USB‑C (38 broches) | ~5 $ |
| 🖥️ Panneau LED | 2× panneaux HUB75 RGB **P4, 64×32, 256×128 mm**, joints côte à côte (→ 128×32) | ~15-25 $/panneau |
| 🔌 Carte de connexion | **DMDos Board V3** (recommandée — inclut le lecteur SD, pas de soudure) | ~15 $ |
| 💾 Lecteur SD | Adaptateur Micro SD SPI (intégré à la DMDos Board) | ~2 $ |
| ⚡ Alimentation | 5V 4A+ | ~10 $ |

> **💡 Astuce** : La DMDos Board de [Mortaca](https://www.mortaca.com/) inclut le lecteur SD et ne demande aucune soudure. Liens d'achat à jour centralisés sur [dmdos.net](https://www.dmdos.net/). Voir [Assemblage du panneau DMD](#-assemblage-du-panneau-dmd) pour les étapes complètes.

---

## 💻 Logiciels nécessaires

- **Windows 10/11** (cet outil est réservé à Windows)
- **Python 3.8+** — uniquement pour lancer depuis les sources (l'installeur/le portable embarquent le leur)
- Chrome ou Edge — pour l'installeur firmware par navigateur
- Connexion internet — pour le pack `_defaults`, le pack de 600 GIFs, et l'installation des scripts Recalbox (Mode 9)

---

## 🚀 Démarrage rapide

Prends la version que tu préfères sur la **[page Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (les `.exe`/`.msi` compilés ne sont pas dans le dépôt lui-même, seulement publiés là-bas) :

**Option A — Installateur Windows (recommandé)**
```
1. Télécharge RecalBoxDMD_Toolkit_Setup.exe depuis la page Releases
2. Lance-le — raccourci menu Démarrer, icône bureau optionnelle, désinstalleur propre
3. Lance « RecalBoxDMD Toolkit » depuis le menu Démarrer
```

**Option B — .exe portable (sans installation)**
```
1. Télécharge RecalBoxDMD-<build>-portable.exe depuis la page Releases
2. Lance-le directement — pas d'installation, pas besoin de Python, un seul fichier
```

**Option C — .msi (pour déploiement scripté/stratégie de groupe)**
```
1. Télécharge le .msi depuis la page Releases
2. msiexec /i "RecalBoxDMD Toolkit-<version>-win64.msi"   (ou double-clic)
```

**Option D — Depuis les sources Python**
```
1. Récupère le dossier tools/
2. Double-clique sur install_and_run.bat — installe Python (via winget si
   absent), Pillow et Markdown, puis lance la GUI
   (ou manuellement : pip install Pillow Markdown && python run_gui.py)
```

---

## 🖥️ Interface graphique (GUI)

La GUI s'ouvre directement sur **6 onglets** : **Main · Playlist · Avancé · Logs · Paramètres · Aide**.

### 📌 Onglet Main

Contient le **Mode 1** — celui dont la plupart des utilisateurs ont besoin. Pointe-le vers ton dossier ROMs et clique sur **Démarrer** ; tout le reste est automatique.

Avant de lancer le traitement, le Mode 1 propose deux étapes optionnelles :

- **Configuration WiFi du DMD** — choisis ton réseau 2,4 GHz dans une liste scannée en direct, saisis le mot de passe, et l'outil le **vérifie réellement** (connecte brièvement ton PC à ce réseau) avant de l'écrire sur la carte SD. Le DMD peut ainsi rejoindre ton WiFi dès son tout premier démarrage, sans jamais passer par son écran de secours en point d'accès. Étape ignorable — sans elle, le DMD retombe sur sa propre page de configuration WiFi au premier démarrage.
- **IP fixe (avancé, optionnel, décochée par défaut)** — permet au DMD d'utiliser une IP statique plutôt qu'une IP attribuée par DHCP. Un avertissement est affiché : utilise **soit** une réservation DHCP sur ton routeur, **soit** une IP fixe ici, **jamais les deux en même temps** — les combiner peut causer de vrais problèmes de connexion. Si tu coches cette case, les champs passerelle/masque/DNS sont pré-remplis à partir de la configuration réseau de ce PC comme point de départ (à vérifier quand même).

Vient ensuite le panneau **Version Recalbox** (10.x / 9.x / legacy — voir [Tutoriel de scraping Recalbox](#-tutoriel-de-scraping-recalbox)), ton **dossier ROMs**, puis **Démarrer**.

Une fois le traitement terminé, un bouton clignote dans cet onglet pour proposer une **copie directe vers un lecteur amovible détecté** — la copie résiste à une interruption (débranchement/plantage) et peut ne reprendre que les fichiers en échec.

### 🎬 Onglet Playlist

Construis tes propres rotations de GIFs en veille/attract-mode : mélange n'importe quelle combinaison du **pack de 600 GIFs** (Mode 11, un clic pour le télécharger) et de **tes propres GIFs** (glisse un dossier PC), nomme la playlist, et elle est prête à être sélectionnée comme active. Fonctionne directement sur une carte SD déjà préparée, ou **en plein Mode 1**, sur le dossier de travail avant même qu'il soit copié sur la carte.

### 🔬 Onglet Avancé

Regroupe chaque opération individuelle en 5 catégories, pour quand tu ne veux pas lancer tout le pipeline du Mode 1 — voir le tableau [Référence des modes](#-référence-des-modes) ci-dessous pour le détail de chacun :

```
📥 GitHub     — Mode 2 (_defaults), Mode 11 (pack de 600 GIFs)
🗂️ Gamelist   — Mode 3 (extraction seule), Mode 8 (vérification images manquantes)
🖼️ Images     — Mode 4 (conversion raw565), Mode 5 (redimensionnement 128×32), Mode 10 (image de secours)
🧮 Caches     — Mode 6 (cache des jeux), Mode 7 (cache des systèmes)
📜 Scripts    — Mode 9 (installation des scripts Recalbox)
```

Les modes 3 et 8 affichent aussi le panneau **Version Recalbox** (même sélecteur que le Mode 1), car ils travaillent tous deux à partir de `gamelist.xml`.

### 📝 Onglet Logs

Affiche toute la sortie du script en temps réel. Boutons de contrôle pendant qu'un mode tourne :

- **⏸️ Pause** / **▶️ Reprendre** — met en pause et reprend le traitement
- **⏭️ Passer** — passe à l'étape suivante
- **⏹️ Arrêter** — arrête le script

### ⚙️ Onglet Paramètres

- Langue de l'interface : 🇫🇷 Français · 🇬🇧 Anglais · 🇪🇸 Espagnol
- Thème visuel (9 habillages : SNES, Mega Drive, Dreamcast, PlayStation, N64, Neo Geo, Game Boy, Atari 2600, ou Aléatoire)
- **Version Recalbox** par défaut (10.x / 9.x / legacy) — la même valeur partagée utilisée partout ailleurs (Mode 1/3/8) ; la changer ici ou dans l'un de ces modes la met à jour partout et l'enregistre dans `RecalBoxDMD_prefs.json`
- Seuil de basculement système lent (nombre de fichiers au-delà duquel un système est marqué **« L »**, par défaut **5 000**) — augmente-le si ta carte SD est assez rapide pour ne pas avoir besoin du système de masque aussi tôt

### 📖 Onglet Aide

Tu es en train de le lire — cette page est directement générée depuis ce fichier.

---

## 📖 Référence des modes

| Mode | Catégorie | Nom | Action |
|------|----------|------|---------------|
| **1** | *(onglet Main)* | **AUTO — tout** | Config WiFi/IP → détection version Recalbox → extraction gamelist → conversion raw565 → cache bigram → téléchargement `_defaults` → installation scripts Recalbox → copie SD |
| 2 | 📥 GitHub | Télécharger `_defaults` | Récupère les images de secours par défaut pour chaque système connu |
| 11 | 📥 GitHub | Pack de 600 GIFs | Téléchargement en un clic de la collection de GIFs gratuits sélectionnés (Arcade, Consoles, Ordinateurs, Flippers, Halloween, Noël, Logo, et plus) |
| 3 | 🗂️ Gamelist | Extraction seule | Lit `gamelist.xml`, copie la bonne enseigne/logo selon ton profil de version Recalbox |
| 8 | 🗂️ Gamelist | Vérification images manquantes | Signale, par système/jeu, si l'image attendue existe réellement (dossier ROMs / dossier de travail / carte SD) |
| 4 | 🖼️ Images | Conversion raw565 | PNG → `.raw565`, GIF → `.raw565pack` + `.meta` |
| 5 | 🖼️ Images | Redimensionnement 128×32 | Redimensionne les PNG à la résolution du panneau (format image, sans conversion raw565) |
| 10 | 🖼️ Images | Image de secours | Définit/génère l'image par défaut globale affichée quand rien d'autre ne correspond |
| 6 | 🧮 Caches | Cache des jeux | Construit `games_cache.bin` (index bigram, 703 entrées par système) |
| 7 | 🧮 Caches | Cache des systèmes | Construit `systems_cache.dat` (index des systèmes + drapeaux lent/rapide **« L »/« N »**) |
| 9 | 📜 Scripts | Installer les scripts Recalbox | Copie le pont enseigne + les scripts [Hi-Score/Infos jeu/Succès/Challenge](#-écrans-superposés-en-jeu--hi-score-infos-jeu-succès--rb-challenge), ainsi que les scripts manuels de récupération WiFi/config web/reboot/luminosité, sur le partage réseau Recalbox — nettoie au passage les anciens noms de scripts laissés par une installation précédente |

> **📎 Copie sur carte SD** : pas un mode séparé — après n'importe quel traitement (Mode 1, 3, 4, 5...), un bouton clignote dans l'onglet Main pour proposer la copie vers un lecteur amovible détecté.

---

## ✅ Déroulé recommandé

```
1. 🔄 Scrape tes jeux dans Recalbox (voir le Tutoriel de scraping ci-dessous,
   selon ta version de Recalbox)
   ↓
2. 📂 Lance RecalBoxDMD Toolkit → onglet Main
   ↓
3. 📶 (Optionnel) Configure le WiFi du DMD, et une IP fixe si tu en veux une
   ↓
4. 🎛️ Choisis ta « Version Recalbox » (10.x / 9.x / legacy)
      Besoin d'aide ? Clique sur « Comment scraper ? »
      Dossier déjà scrapé avec une ancienne config ? « Nettoyer les dossiers avant scraping »
   ↓
5. 🗂️ Choisis ton dossier ROMs (ex. D:\Recalbox\share\roms)
   ↓
6. 🟢 Clique sur Démarrer (Mode 1) et laisse-le tourner
   ↓
7. ✅ Terminé ! Le dossier de travail est prêt
   ↓
8. 💾 Insère ta carte SD — le bouton clignotant propose de la copier
      (optionnel : Mode 8 pour vérifier qu'il ne manque rien)
   ↓
9. 🎮 Insère la carte SD dans le DMD et allume-le !
```

---

## 🎨 Tutoriel de scraping Recalbox

Pour que le panneau affiche la bonne enseigne, il faut d'abord **scraper** tes ROMs dans Recalbox en configurant correctement l'onglet Scraper **pour ta version**. Le bouton **« Comment scraper ? »** des onglets Main/Avancé montre des captures d'écran annotées pour ça ; résumé ici :

### Recalbox 10.x (champ logo dédié)

1. Règle **« SELECT LOGO TYPE »** sur **« CLEAR »**
2. Ne mets PAS cette même valeur sur **« Select image type »** — ce champ sert au visuel principal (capture, jaquette...), pas au logo
3. Lance le scrape

Stocké dans `media/wheels/`, référencé par la balise `<logo>`. → choisis le profil **10.x**.

### Recalbox 9.x (pas de champ logo dédié)

1. Règle **« Select thumbnail type »** sur **« MARQUEE »**
2. Ne mets PAS cette même valeur sur **« Select image type »**
3. Lance le scrape

Stocké dans `media/thumbnails/`, référencé par la balise `<thumbnail>`. → choisis le profil **9.x** (recommandé).

### Profil « legacy »

Si ta configuration scrape le logo/l'enseigne via **« Select image type »** (réglé sur **« Clear Logo »** ou **« Marquee »**), les fichiers atterrissent dans `media/images/`, référencés par la balise `<image>`. → choisis le profil **legacy**.

> **💡 Astuces** : Screenscraper.fr ou TheGamesDB donnent les meilleurs résultats. Déjà scrapé avec une config différente ? Utilise d'abord **« Nettoyer les dossiers avant scraping »**. Pas sûr de ce que tu as vraiment récupéré ? Lance le **Mode 8**.

---

## 🔧 Assemblage du panneau DMD

Le montage matériel (2 panneaux + DMDos Board + ESP32 + microSD) est rapide et ne demande aucune soudure :

1. **Joins les deux panneaux** avec les pièces d'union fournies avec la DMDos Board (n'importe quelle vis M3 convient).
2. **Positionne la DMDos Board** sur le connecteur **entrée** (pas sortie) — garde l'orientation des composants arrière identique des deux côtés.
3. **Câble l'alimentation** (rouge/noir selon le sérigraphie) *avant* de poser l'ESP32 dessus, et relie les deux panneaux avec le câble ruban fourni.
4. **Insère la carte SD** préparée avec cet outil, branche l'ESP32 déjà flashé, alimente le tout via le port USB-C.

> ⚠️ Le site **DMDos** ([dmdos.net](https://www.dmdos.net/)) propose son propre firmware séparé. Ne réutilise que son **matériel et son guide de montage** — le firmware et le contenu de la carte SD doivent venir de cet outil, pas de DMDos.

Guide illustré complet avec photos : [dmdos.net → Montaje/Assembly](https://www.dmdos.net/#montaje). Cadre imprimable en 3D par Janibol ([Retromojones](https://www.youtube.com/@retromojones)) sur [Thingiverse](https://www.thingiverse.com/thing:6704880).

---

## ⚡ Firmware ESP32

### 🔧 Compilation depuis les sources

1. Ouvre `RecalBox_DMD.ino` dans l'**Arduino IDE**.
2. Installe ces bibliothèques (Croquis → Inclure une bibliothèque → Gérer les bibliothèques) :

| Bibliothèque | Rôle |
|---------|---------|
| ESP32-HUB75-MatrixPanel-I2S-DMA | Pilotage DMA du panneau LED |
| AnimatedGIF | Décodage GIF (chemin de secours) |
| pngle | Décodage PNG (chemin de secours, embarqué) |
| WiFiManager | Configuration WiFi |
| Adafruit GFX Library | Rendu texte/formes |
| PubSubClient | MQTT (gardé dormant — l'UDP est le transport par défaut) |
| ArduinoJson | (Dé)sérialisation config & page web |

3. Outils → Carte : **ESP32 Dev Module**, taille flash **4 Mo**, schéma de partition **Huge APP**.
4. Sélectionne le bon port COM, puis **Téléverser**.

### ⚙️ Configuration (config.ini)

Tu n'as jamais besoin d'écrire ce fichier à la main — le Mode 1 le fait pour toi, et la page web du DMD permet ensuite de modifier chaque valeur en direct. Pour référence :

```ini
# Affichage
brightness=40                 # luminosite du panneau 0-100 %

# Playlist
playlist=RecalBox_intros.txt  # lue depuis /playlist
random=1                      # 0 = sequentiel, 1 = aleatoire

# WiFi
wifi_enabled=1
wifi_ssid=monwifi
wifi_password=monmotdepasse
wifi_static_enabled=1         # 1 = utilise les champs IP fixe ci-dessous au lieu du DHCP
wifi_static_ip=192.168.1.240
wifi_gateway=192.168.1.1
wifi_subnet=255.255.255.0

# Liaison Recalbox (UDP)
recalbox_ip=192.168.1.104     # IP fixe de ta Recalbox

# Horloge (thèmes d'horloge rétro)
[CLOCK]
CLOCK_ENABLED=1
CLOCK_THEME=-1                # -1=aleatoire, 0=Mario ... 9=Level 1-1
CLOCK_INTERVAL=5              # nombre de GIFs avant d'afficher l'horloge
CLOCK_DURATION=60             # secondes d'affichage de l'horloge
```

### 🌐 Installeur Web

> [👉 Installer depuis l'installeur Web par navigateur](https://shan-aya.github.io/RecalBoxDMD/)

Chrome ou Edge, branche l'ESP32 en USB, clique sur **Install**, choisis le port COM — environ une minute, pas besoin d'Arduino IDE. Coche **« Erase device »** lors d'une première installation, ou en venant d'un autre firmware.

### 📡 Liaison UDP & Telnet

```
Recalbox → marquee[...].sh → UDP → ESP32 → Panneau LED

1. Tu lances "King of Fighters '98"
2. Le script bash detecte l'evenement → envoie "mame/kof98"
3. L'ESP32 recherche, dans l'ordre :
   a. /systems/mame/kof98.raw565 (ou .raw565pack)   ← instantane
   b. index bigram de games_cache.bin                ← accelere
   c. /systems/_defaults/mame.raw565                 ← secours systeme
   d. /systems/_defaults/default.raw565               ← secours global
4. Affiche en moins de 15 ms
```

Installe le script avec le **Mode 9**, ou copie `marquee[...].sh` manuellement dans `/recalbox/share/userscripts/`.

**Contrôle manuel depuis le menu Recalbox** — le Mode 9 installe aussi des scripts déclenchables à la main (**START → Paramètres avancés → Scripts utilisateur**) : WiFi Recovery DMD (repasse en mode point d'accès de secours), Config Web DMD (ouvre la page de config web), Reboot DMD, et luminosité ±10%.

Une console **Telnet** est intégrée pour le débogage sur l'appareil : `telnet <ip-esp32>` puis `help`.

---

## 🏆 Écrans superposés en jeu — Hi-Score, infos jeu, succès & RB Challenge

Pendant qu'un jeu tourne réellement (jamais en mode veille/playlist), le panneau peut automatiquement alterner l'enseigne avec jusqu'à **quatre** écrans communautaires — toujours autolimités, revenant à l'enseigne sur une minuterie qui vit entièrement sur le DMD lui-même :

- 🏆 **Hi-Score (MAME/FBNeo)** — décode le fichier de score sauvegardé par l'émulateur lui-même à partir d'un manifeste communautaire (des milliers de jeux couverts) et affiche le vrai classement.
- ℹ️ **Infos jeu** — description, genre, développeur et année, directement depuis ton `gamelist.xml` existant.
- 🎖️ **RetroAchievements** — s'affiche dès qu'un succès est débloqué en pleine partie.
- 📅 **RB Challenge** — le classement officiel du Challenge communautaire mensuel de Recalbox.

**Aucune configuration côté DMD.** Installe les scripts Recalbox une fois — **Mode 9** (ou l'installation automatique intégrée au **Mode 1**) — et chaque écran superposé se met à fonctionner tout seul pour tout jeu/système ayant des données à afficher.

---

## 🗂️ Le format raw565 en détail

### 📄 .raw565 (image fixe depuis un PNG)

```
Taille : 128 × 32 × 2 = 8 192 octets exactement
Format : RGB565 brut (16 bits par pixel)

Lecture ESP32 :
  f.read(buffer, 8192);
  drawRGBBitmap(0, 0, buffer, 128, 32);
  // 1 operation SD + 1 affichage → 5 ms
```

### 🎞️ .raw565pack + .meta (GIF animé)

```
[Nom].raw565pack                [Nom].meta
├── Frame 0 → 8 192 octets       ├── delay_0 → 2 octets (uint16, ms)
├── Frame 1 → 8 192 octets       ├── delay_1 → 2 octets
└── ...                          └── ...
```

Une ouverture SD + un seek par frame, aucun décodage GIF sur l'appareil.

### ⚡ Cache bigram — indexation accélérée

`games_cache.bin` indexe chaque système par préfixe de 2 lettres (703 entrées, `#`, `A`, `AA`, `AB`... `ZZ`) — une recherche saute directement à la bonne tranche du cache au lieu de parcourir un dossier de dizaines de milliers de fichiers.

### 🎭 Système de masque

Les systèmes marqués **« L »** (au-dessus du seuil configurable, onglet Paramètres — 5 000 par défaut) affichent **immédiatement** leur image par défaut en cache pendant qu'une tâche en arrière-plan décode et remplace par la vraie image. Le panneau n'est jamais laissé noir.

---

## 📁 Structure de la carte SD

```
📁 CARTE SD (FAT32)
├── config.ini
├── systems/
│   ├── <systeme>/
│   │   ├── <jeu>.raw565             ← enseigne fixe
│   │   ├── <jeu>.raw565pack         ← enseigne animee (frames)
│   │   └── <jeu>.meta               ← enseigne animee (timings)
│   └── _defaults/
│       ├── default.raw565           ← secours global
│       └── <systeme>.raw565         ← secours par systeme
├── gifs/                            ← playlists attract-mode (le pack de 600 GIFs atterrit ici)
│   ├── Arcade/  Consoles/  Computers/  Pinball_Short/  Pinball_Story/
│   └── Halloween/  XMAS/  Logo/  Other/ ...
├── playlists/
│   └── <nom_playlist>.txt
├── games_cache.bin                  ← index bigram
└── systems_cache.dat                ← index systemes + drapeaux L/N
```

---

## 🔧 Dépannage

Pour les gels, la corruption d'affichage, les boucles de configuration WiFi, ou un DMD qui semble désynchronisé de Recalbox, consulte d'abord la page dédiée **[FAQ & Dépannage](FAQ.fr.md)** sur GitHub — elle couvre les causes les plus courantes (alimentation USB, qualité de la carte microSD, révision du chip ESP32) plus en détail que ce qui tient ici.

| Problème | Solution |
|---|---|
| « Pillow n'est pas installé » | Installé automatiquement au premier lancement ; si ça échoue : `pip install Pillow` |
| « API GitHub injoignable » | Les téléchargements `_defaults`/pack 600 GIFs ont besoin d'internet ; réessaie plus tard (limite de débit) |
| Aucun lecteur amovible détecté | Insère/revérifie que la carte SD est visible dans l'Explorateur Windows |
| L'ESP32 n'affiche rien | Vérifie l'alimentation (5V 4A min.), `config.ini` à la racine de la SD, le câblage HUB75 ; essaie Telnet `help` |
| ESP32 non détecté (pas de port COM) | Installe les pilotes USB : CP2102 (Silicon Labs) ou CH340/CH341 |
| Affichage lent / écran noir entre les jeux | Confirme que tu as lancé le **Mode 1** ; vérifie que le système est marqué `L` dans `systems_cache.dat` ; augmente le seuil de basculement (onglet Paramètres) si ta carte SD est rapide |
| Mauvaise image affichée (jaquette au lieu du logo) | Vérifie le profil **Version Recalbox** et utilise **« Comment scraper ? »** ; lance le **Mode 8** pour vérifier ce qui est réellement présent |
| Impossible de configurer le WiFi pendant le Mode 1 | Étape optionnelle — ignore-la et utilise la page WiFi de premier démarrage du DMD à la place |
| Problèmes de connexion après avoir configuré une IP fixe | Assure-toi de ne pas utiliser **en même temps** une réservation DHCP pour le DMD sur ton routeur — n'utilise qu'une seule des deux méthodes |

---

## 📁 Fichiers générés par le script

| Fichier | Format | Rôle |
|------|--------|---------|
| `systems/.../*.raw565` | 8 192 octets RGB565 | PNG converti (affichage 5 ms) |
| `systems/.../*.raw565pack` | Frames concaténées | GIF animé converti |
| `systems/.../*.meta` | uint16[] × nb de frames | Timings des frames du GIF |
| `systems/_defaults/*.raw565` | 8 192 octets | Images de secours par système |
| `games_cache.bin` | Index bigram, 703 entrées/système | Cache des jeux (recherche accélérée) |
| `systems_cache.dat` | Texte | Index des systèmes + drapeaux `L`/`N` |
| `images_manquantes.txt` | Texte | Liste des images manquantes (Mode 1/2/3) |
| `reports/mode8_report_*.txt` | Texte | Rapport du Mode 8 (vérification gamelist ↔ images) |
| `config.ini` | Texte | Configuration du DMD (WiFi, playlist, luminosité...) |

---

## 🤝 Crédits

- **Projet original RetroBoxLED** : [Jamyz](https://github.com/Jamyz/RetroBoxLED) — la base firmware ESP32 et l'idée
- **RawEdition** : **Shan_ayA** — format raw565, cache bigram, système de masque, toolkit PC, thèmes d'horloge, gestion des versions Recalbox, aperçu live web
- **Inspiration** : [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) par fjgordillo86
- **Pack de 600 GIFs** : **eLLuiGi** / [RpiTeaM](https://rpiteam.carrd.co/) — échantillon gratuit de leur collection de GIFs rétro
- **Manifeste Hi-Score** : format communautaire **hi2txt-xml**, construit autour du projet `hiscore.dat` de MAME lui-même
- **Matériel & guide de montage** : [Mortaca — DMDos Board](https://www.mortaca.com/) / [dmdos.net](https://www.dmdos.net/)
- **Cadre 3D** : Janibol — [Retromojones](https://www.youtube.com/@retromojones)
- **Communauté** : [Recalbox](https://www.recalbox.com/)

Page complète du projet, captures d'écran et historique des versions : [github.com/shan-aya/RecalBoxDMD](https://github.com/shan-aya/RecalBoxDMD)

---

> **RecalBoxDMD Toolkit** — Recalbox + une vraie enseigne LED, instantanée même avec 30 000 jeux MAME ! 🎮⚡
