# 🎮 RecalBoxDMD Toolkit v3.5 — Aide complète

> **RecalBoxDMD** = Recalbox + panneau LED DMD 128x32 pour votre borne d'arcade !

---

## 📋 Table des matières

1. [Présentation](#presentation)
2. [🆕 Nouveautés](#nouveautes)
3. [Prérequis matériel](#prerequis-materiel)
4. [Prérequis logiciel](#prerequis-logiciel)
5. [Installation rapide](#installation-rapide)
6. [Interface graphique (GUI)](#interface-graphique-gui)
   - [Onglet Main](#onglet-main)
   - [Onglet Avancé](#onglet-avance)
   - [Onglet Logs](#onglet-logs)
   - [Onglet Paramètres](#onglet-parametres)
7. [Détail des modes](#detail-des-modes)
8. [Workflow complet (recommandé)](#workflow-complet-recommande)
9. [Tutoriel scraping Recalbox](#tutoriel-scraping-recalbox)
10. [🔧 Assemblage du panneau DMD (DMDos)](#assemblage-du-panneau-dmd-dmdos)
11. [Firmware ESP32 — Raw565 Edition](#firmware-esp32-raw565-edition)
    - [Pourquoi le raw565 ?](#pourquoi-le-raw565)
    - [Compilation avec Arduino IDE](#compilation-avec-arduino-ide)
    - [Configuration (config.ini)](#configuration-configini)
    - [Installation via navigateur (WebInstaller)](#installation-via-navigateur-webinstaller)
12. [Le format raw565 en détail](#le-format-raw565-en-detail)
    - [.raw565 (image fixe PNG)](#raw565-image-fixe-png)
    - [.raw565pack + .meta (GIF animé)](#raw565pack-meta-gif-anime)
    - [Cache bigramme — indexation accélérée](#cache-bigramme-indexation-acceleree)
    - [Système de masque (mask)](#systeme-de-masque-mask)
13. [Structure de la carte SD](#structure-de-la-carte-sd)
14. [Dépannage](#depannage)
15. [Fichiers générés par le script](#fichiers-generes-par-le-script)
16. [Crédits](#credits)

---

## 🎯 Présentation

**RecalBoxDMD Toolkit** est un fork optimisé de **RetroBoxLED** par Jamyz, spécialement conçu pour résoudre les problèmes de **lenteur d'affichage** sur les systèmes contenant beaucoup de fichiers (fullset MAME, FBNeo, etc.).

### 🔥 Le problème résolu

La version originale de Jamyz lisait directement les fichiers **PNG et GIF** depuis la carte SD. Sur des systèmes comme **MAME fullset** (30 000+ jeux), cette approche provoquait :

```
❌ Problème original :
   - Ouverture du dossier systems/mame/ avec 30 000 fichiers
   - L'ESP32 liste tous les fichiers → freeze de plusieurs secondes
   - Décodage PNG sur l'ESP32 → lent, mémoire insuffisante
   - Entre chaque jeu : "sabre noir" ou latence visible
```

**La solution « Raw565 Edition »** pré-convertit toutes les images sur le PC (via le script Python) en un format **brut RGB565** directement affichable par l'ESP32 :

```
✅ Solution raw565 :
   1. Le script convertit chaque PNG → fichier .raw565 (8192 octets, prêt à l'emploi)
   2. Chaque GIF → fichier .raw565pack (trames concaténées) + .meta (timings)
   3. L'ESP32 lit le fichier raw565 et l'envoie directement au panneau LED
   4. Pas de décodage, pas de lenteur : affichage plus rapide
   5. possibilité d utiliser des images fixe (PNG-->raw565) ou animé (GIF-->RAW565pack) pour les jeux
```

| Métrique | Original PNG/GIF | Raw565 Edition |
|----------|-----------------|----------------|
| Temps d'affichage | 500ms – 3s+ | **5–15ms** |
| RAM nécessaire | 50-100 Ko | **8 Ko** |
| Latence MAME fullset | Freeze 5-10s | **0** |
| Compatibilité | Tous systèmes | **Systèmes « L » marqués** |

### Le système de masque (mask)

Pour les très gros systèmes marqués **« L »** (Large / Lent, comme MAME ou FBNeo), le firmware utilise un mécanisme de masque :

1. Le script détecte les systèmes avec >800 fichiers individuels
2. Il marque ces systèmes comme **« L »** dans `systems_cache.dat`
3. Quand l'ESP32 reçoit un jeu de ce système :
   - Il affiche  le **raw565** du système de jeu
   - En parallèle, une tâche asynchrone controle lexistence du raw565pack ou du raw565  spécifique
   - Quand la verification et la localisation sur la carte SD  est fini, l'image spécifique remplace le masque
   - En cas d absence de fichier image, le systeme retourne l image **default.raw565** immediatement car en cahe RAM
4. Résultat : **l'utilisateur ne voit jamais d'écran noir**, le panneau reste toujours actif

> **💡 Résultat** : Finies les longues secondes d'attente entre chaque jeu. L'affichage est bien plus rapide ( dépend de la rapidité de la SD ) même avec un fullset MAME de 30 000 fichiers !

> **💡 Contraintes** : Il faut imperativement passer par le script pour préparer la SD. Vous ne pouvez pas rajouter des images sur la SD à la volée

### 🔄 Comment tout s'articule

```
┌─────────────────────────────────────────────────────────────┐
│                     RECALBOX                                 │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Lance un jeu → MQTT envoie "mame/kof98" à l'ESP32   │   │
│  └──────────────┬───────────────────────────────────────┘   │
│                 │                                           │
│          Événement MQTT                                     │
│                 ▼                                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │            ESP32 + Panneau LED 128x32                  │   │
│  │                                                       │   │
│  │  Reçoit "mame/kof98" :                                │   │
│  │  1. Cherche /systems/mame/kof98.raw565 (instantané)   │   │
│  │  2. Pas trouvé ? Cherche dans games_cache.bin         │   │
│  │  3. Pas trouvé ? Affiche _defaults/mame.raw565        │   │
│  │  4. Pas trouvé ? Affiche _defaults/default.raw565     │   │
│  │                                                       │   │
│  │  ⏱️ Temps total : 5 à 15 ms au lieu de 500ms-3s !     │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────┐
         │     RecalBoxDMD Toolkit (Prépare la SD)       │
         │  Extrait les images des gamelists.xml          │
         │  Convertit PNG → .raw565 (format brut RGB565)  │
         │  Convertit GIF → .raw565pack + .meta           │
         │  Construit le cache bigramme (games_cache.bin) │
         │  Marque les systèmes lents (flag "L")          │
         │  Copie le tout sur la carte SD                 │
         └─────────────────────────────────────────────┘
```

---

## 🆕 Nouveautés

### 🎯 Gestion des versions Recalbox (Mode 1 & Mode 3)

Recalbox stocke les images marquee/logo différemment selon la version installée **et** selon les options choisies dans l'onglet Scraper de Recalbox. Pour éviter de récupérer le mauvais visuel (jaquette au lieu du logo, par exemple), les modes 1 et 3 proposent maintenant un panneau **« Version Recalbox »** :

- Un sélecteur **10.x / 9.x / legacy** qui détermine automatiquement la bonne balise du `gamelist.xml` (`<logo>`, `<thumbnail>` ou `<image>`) et le bon dossier média (`media/wheels/`, `media/thumbnails/` ou `media/images/`).
- Un bouton **« Comment scraper ? »** qui affiche, pour la version choisie, une capture d'écran annotée de l'onglet Scraper de Recalbox indiquant exactement quel champ régler (voir [Tutoriel scraping Recalbox](#tutoriel-scraping-recalbox)).
- Un bouton **« Nettoyer les dossiers avant scrape »** : supprime (avec confirmation et aperçu du nombre de fichiers) les images déjà présentes dans le dossier média correspondant, système par système, pour repartir sur un scrape Recalbox propre et complet.
- Le choix de version est mémorisé (fichier de préférences) et partagé entre le Mode 1 (onglet Main) et le Mode 3 (onglet Avancé).

### 🔎 Mode 8 — Vérification des images manquantes

Nouveau mode dans l'onglet Avancé : parcourt les `gamelist.xml` du dossier ROMs et signale, pour chaque système et chaque jeu, si l'image attendue (selon le profil de version choisi) existe réellement sur le disque. Un rapport est généré, et une option complémentaire permet de comparer avec le dossier de travail temporaire (et, en option, avec la carte SD physique) pour distinguer une image manquante côté ROMs, non convertie, ou non copiée sur la SD.

### 💾 Reprise de la copie SD après interruption

La copie vers la carte SD physique (bouton clignotant après un traitement) résiste maintenant à une interruption (débranchement, crash) : écriture atomique des fichiers, détection d'une copie précédente incomplète au prochain lancement, et bouton dédié pour ne relancer que les fichiers en échec plutôt que toute la copie.

---

## 🧰 Prérequis matériel

| Composant | Référence | Prix indicatif |
|-----------|-----------|----------------|
| 🧠 Microcontrôleur | ESP32 DevKit V1 USB-C (38 broches) | ~5€ |
| 🖥️ Panneau LED | 2× panneaux matriciels RGB HUB75 **P4, 64x32, 256x128mm**, à assembler côte à côte (→ 128x32) | ~15-25€/panneau |
| 💾 Lecteur SD | Module adaptateur Micro SD (SPI) — intégré à la DMDos Board | ~2€ |
| 🔌 Carte de connexion | **DMDos Board V3** (recommandée, simplifie le câblage et intègre le lecteur SD) | ~15€ |
| ⚡ Alimentation | 5V 4A+ (selon taille du panneau) | ~10€ |

> **💡 Astuce** : La DMDos Board de [Mortaca](https://www.mortaca.com/) intègre le lecteur SD et évite la soudure ! Les liens d'achat pour chaque composant (ESP32, DMDos Board, microSD, panneaux P4) sont centralisés sur la page [Hardware de dmdos.net](https://www.dmdos.net/).
>
> Voir aussi la section [🔧 Assemblage du panneau DMD (DMDos)](#assemblage-du-panneau-dmd-dmdos) pour le guide de montage complet (traduit du site officiel).

---

## 💻 Prérequis logiciel

- **Python 3.8+** installé sur votre PC (uniquement pour lancer depuis les sources)
- **Pillow** (installé automatiquement si absent)
- **Arduino IDE** (uniquement pour recompiler le firmware)
- Navigateur Chrome/Edge (pour le WebInstaller)

---

## 🚀 Installation rapide

Prenez le build de votre choix sur la **[page Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (les `.exe`/`.msi` compilés ne sont pas dans le dépôt, seulement publiés là-bas) :

**Option A — Installateur Windows (recommandé)**

```
1. Téléchargez RecalBoxDMD_Toolkit_Setup.exe depuis la page Releases
2. Lancez-le — raccourci menu Démarrer, icône bureau optionnelle, vrai désinstalleur
3. Lancez « RecalBoxDMD Toolkit » depuis le menu Démarrer
```

**Option B — Exécutable portable (sans installation)**

```
1. Téléchargez RecalBoxDMD_GUI.exe depuis la page Releases
2. Lancez-le directement — aucune installation, aucun Python requis, fichier unique
```

**Option C — .msi (pour un déploiement scripté/GPO)**

```
1. Téléchargez le .msi depuis la page Releases
2. msiexec /i "RecalBoxDMD Toolkit-1.0.0-win64.msi"   (ou double-clic)
```

**Option D — Depuis les sources Python**

```
1. Téléchargez les fichiers du dossier tools/
2. Double-cliquez sur install_and_run.bat — installe Python (via winget,
   si absent), Pillow et Markdown, puis lance la GUI
   (ou manuellement : pip install Pillow Markdown && python run_gui.py)
```

---

## 🖥️ Interface graphique (GUI)

L'interface graphique se lance automatiquement. Elle est organisée en **5 onglets** :

### 📌 Onglet Main

> Ne contient que le **Mode 1** — l'essentiel pour la plupart des utilisateurs.

```
┌────────────────────────────────────────────────────────────┐
│ [Main]  [Avancé]  [Logs]  [Paramètres]  [AIDE]            │
├────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 📋 Mode                                               │ │
│ │                                                       │ │
│ │ ◉ MODE 1 — Extraction + Conversion + Build Cache      │ │
│ │   (TOUT en un clic !)                                 │ │
│ │                                                       │ │
│ │ ┌──────────────────────────────────────────────────┐  │ │
│ │ │ 📂 Choisir dossier ROMs                         │  │ │
│ │ │ D:\Recalbox\share\roms                          │  │ │
│ │ └──────────────────────────────────────────────────┘  │ │
│ │                                                       │ │
│ │ [ 🟢 Démarrer ]                                       │ │
│ │ [ ❌ Quitter ]                                        │ │
│ └────────────────────────────────────────────────────────┘ │
│                                                            │
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 🎛️ Version Recalbox (scrape marquee/logo)              │ │
│ │ [ 10.x ▾ ]                                            │ │
│ │ [ Comment scraper ? ]                                 │ │
│ │ [ Nettoyer les dossiers avant scrape ]                │ │
│ └────────────────────────────────────────────────────────┘ │
│                                                            │
│ ┌────────────────────────────────────────────────────────┐ │
│ │ Progression : ████████████░░░░░░ 75%                   │ │
│ │ Extraction de : mame/king_of_fighters.png              │ │
│ │ ⏸️ Pause  ▶️ Reprise  ⏭️ Passer  ⏹️ Stop               │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

**👉 Mode 1 = le mode complet** qui enchaîne :
0. Choix de la **version Recalbox** (10.x / 9.x / legacy) — détermine la balise du `gamelist.xml` et le dossier média utilisés
1. Extraction des images depuis vos `gamelist.xml`
2. Conversion PNG → raw565 (format RGB565 brut, 8192 octets)
3. Conversion GIF → raw565pack + meta (trames concaténées)
4. Téléchargement de `systems/_defaults` depuis GitHub
5. Construction de `games_cache.bin` (index bigramme)
6. Génération de `systems_cache.dat` (index systèmes + flag lent/rapide)
7. Détection automatique des gros systèmes (flag `L` pour MAME, FBNeo...)

Une fois le traitement terminé, un bouton clignote pour proposer la **copie directe vers la carte SD physique** (détection automatique des lecteurs amovibles).

### 🔬 Onglet Avancé

> Contient les **modes 2 à 8** pour des opérations ciblées.

```
┌────────────────────────────────────────────────────────────┐
│ [Main]  [Avancé]  [Logs]  [Paramètres]  [AIDE]            │
├────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 📋 Mode                                               │ │
│ │                                                       │ │
│ │ ○ MODE 2 — Téléchargement _defaults via GitHub        │ │
│ │ ○ MODE 3 — Extraction Gamelist uniquement             │ │
│ │ ○ MODE 4 — Conversion PNG→raw565 + GIF→raw565pack     │ │
│ │ ○ MODE 5 — Conversion 128x32 uniquement               │ │
│ │ ○ MODE 6 — Build Games Cache uniquement               │ │
│ │ ○ MODE 7 — Génération systems_cache.dat               │ │
│ │ ○ MODE 8 — Vérifier les images manquantes             │ │
│ │                                                       │ │
│ │ [ 🟢 Démarrer ]                                       │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

> Le **Mode 3** affiche lui aussi le panneau **« Version Recalbox »** (même sélecteur, mêmes boutons d'aide/nettoyage que le Mode 1), puisqu'il extrait également les images depuis les `gamelist.xml`.
> Le **Mode 8** affiche un panneau dédié avec son propre sélecteur de version, le bouton d'ouverture du rapport, et un bouton de comparaison avec le support final (dossier temporaire / carte SD).

### 📝 Onglet Logs

Affiche toute la sortie du script en temps réel. Utile pour suivre la progression et détecter les erreurs.

Boutons de contrôle :
- **⏸️ Pause** — Met le traitement en pause
- **▶️ Reprise** — Reprend le traitement
- **⏭️ Passer** — Passe à l'étape suivante
- **⏹️ Stop** — Arrête le script

### ⚙️ Onglet Paramètres

Permet de changer la langue de l'interface :
- 🇫🇷 Français
- 🇬🇧 English
- 🇪🇸 Español

Permet aussi de changer le thème visuel, et de choisir la **« Version Recalbox »** par défaut (10.x / 9.x / legacy) — la même variable que celle des panneaux Mode 1/3/8 : un changement ici (ou dans n'importe lequel de ces modes) est immédiatement répercuté partout et sauvegardé dans `RecalBoxDMD_prefs.json`.

### 📖 Onglet AIDE

Affiche ce README directement dans l'interface.

---

## 📖 Détail des modes

| Mode | Nom | Action | Durée estimée |
|------|-----|--------|---------------|
| **1** | **TOUT** | Version Recalbox + Extraction + Conversion raw565 + Cache + _defaults | Longue (5-30 min) |
| **2** | Téléchargement _defaults | Récupère les images systèmes depuis GitHub | Courte (~1 min) |
| **3** | Extraction uniquement | Version Recalbox + lit les gamelist.xml, copie les images | Variable |
| **4** | Conversion raw565 | PNG→raw565 + GIF→raw565pack/meta | Moyenne |
| **5** | Conversion 128x32 | Redimensionne les PNG en 128x32 (format image) | Moyenne |
| **6** | Build Games Cache | Génère games_cache.bin (index bigramme) | Rapide |
| **7** | Génération systems_cache.dat | Index des systèmes + flag lent/rapide | Rapide |
| **8** | **Vérification images manquantes** | Compare gamelist.xml ↔ images présentes (+ comparaison support final en option) | Variable |

> **📎 Copie sur la carte SD** : ce n'est pas un mode à part — après un traitement (Mode 1, 3, 4, 5...), un bouton clignote automatiquement dans l'onglet Main pour proposer la copie vers un lecteur amovible détecté. La copie résiste à une interruption et propose de ne relancer que les fichiers en échec.

---

## ✅ Workflow complet (recommandé)

```
1. 🔄 Scrapez vos jeux dans Recalbox (voir le Tutoriel scraping ci-dessous
   selon votre version Recalbox : logo dédié, marquee ou logo détouré)
   ↓
2. 📂 Lancez RecalBoxDMD Toolkit (raccourci installé, RecalBoxDMD_GUI.exe portable, ou python run_gui.py) → onglet Main
   ↓
3. 🎛️ Choisissez votre « Version Recalbox » (10.x / 9.x / legacy)
      Besoin d'aide ? Cliquez « Comment scraper ? »
      Dossier déjà scrapé avec une ancienne config ? « Nettoyer les dossiers avant scrape »
   ↓
4. 🗂️ Choisissez votre dossier ROMs (ex: D:\Recalbox\share\roms)
   ↓
5. 🟢 Cliquez Démarrer (Mode 1)
   ↓
6. ⏳ Laissez le script travailler, il va :
      - Extraire les images des gamelist.xml (selon la version choisie)
      - Convertir PNG → .raw565 (format RGB565 rapide)
      - Convertir GIF → .raw565pack + .meta
      - Télécharger les _defaults depuis GitHub
      - Construire le cache bigramme (indexation jeux)
      - Marquer les gros systèmes (flag "L" pour MAME, FBNeo)
   ↓
7. ✅ Terminé ! Le dossier sd_card/ est prêt dans %TEMP%/RecalBoxDMD
   ↓
8. 💾 Insérez votre carte SD, le bouton clignotant propose de copier
      (optionnel : Mode 8 pour vérifier qu'aucune image n'est manquante)
   ↓
9. 🎮 Branchez la SD dans l'ESP32 et allumez !
      - Affichage quasi instantané même avec 30 000 jeux MAME
```

---

## 🎨 Tutoriel scraping Recalbox

Pour que le panneau LED affiche les bons visuels, vous devez d'abord **scraper** vos ROMs dans Recalbox — et bien configurer l'onglet Scraper **selon votre version**. C'est exactement ce que fait le bouton **« Comment scraper ? »** du Mode 1/3 (captures d'écran annotées directement dans l'appli), résumé ici :

### Recalbox 10.x (champ logo dédié)

Depuis Recalbox 10.x, le menu **SCRAPEUR → SCRAPE NOW** possède un champ dédié **« SELECT LOGO TYPE »** (parfois encore affiché en anglais sur les versions alpha/bêta) :

1. Réglez **« SELECT LOGO TYPE »** sur **« CLEAR »**
2. Ne réglez surtout pas cette valeur sur **« Sélectionnez le type d'image »** — ce champ sert au visuel principal (écran de jeu, jaquette...), pas au logo
3. Lancez le scrape

Les fichiers sont stockés dans `media/wheels/` de chaque système, référencés par la balise `<logo>` du `gamelist.xml`. → Choisissez le profil **10.x** dans l'outil.

### Recalbox 9.x (pas de champ logo dédié)

Sur les versions de Recalbox sans champ « logo » dédié :

1. Réglez **« Sélectionnez le type de vignette »** sur **« MARQUEE »**
2. Ne réglez pas **« Sélectionnez le type d'image »** sur cette valeur — il sert à un autre visuel (jaquette, écran titre...)
3. Lancez le scrape

Les fichiers sont stockés dans `media/thumbnails/` de chaque système, référencés par la balise `<thumbnail>`. → Choisissez le profil **9.x** (recommandé) dans l'outil.

### Profil « legacy » (via le champ image)

Si votre configuration Recalbox scrape le logo/la marquee via **« Sélectionnez le type d'image »** (réglé sur **« LOGO DÉTOURÉ »** ou **« MARQUEE »**), les fichiers sont stockés dans `media/images/`, référencés par la balise `<image>`. → Choisissez le profil **legacy**. À réserver aux configurations qui n'utilisent pas le champ vignette pour le logo.

```
📁 /recalbox/share/roms/
   ├── 📁 mame/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 media/
   │   │   ├── 📁 wheels/         ← profil 10.x (<logo>)
   │   │   ├── 📁 thumbnails/     ← profil 9.x (<thumbnail>)
   │   │   └── 📁 images/         ← profil legacy (<image>)
   │   └── ...
   ├── 📁 snes/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 media/
   │   │   └── 📁 wheels/
   │   │       └── 🖼️ supermetroid.png
   └── ...
```

> **💡 Conseil** : Utilisez Screenscraper.fr ou TheGamesDB pour de meilleurs résultats !
>
> **💡 Scrape déjà fait avec une autre config ?** Utilisez le bouton **« Nettoyer les dossiers avant scrape »** (Mode 1/3) avant de relancer le scrape Recalbox, pour repartir sur un dossier média propre et éviter de mélanger d'anciens visuels avec les nouveaux.
>
> **💡 Un doute sur les images récupérées ?** Lancez le **Mode 8** pour générer un rapport des images manquantes ou incohérentes par rapport au profil choisi.

---

## 🔧 Assemblage du panneau DMD (DMDos)

Le montage matériel (panneaux LED + DMDos Board + ESP32 + microSD) est identique à celui décrit sur le site officiel **[dmdos.net](https://www.dmdos.net/)** de Mortaca. Instructions traduites ci-dessous — pour les images du guide et les liens d'achat à jour, consultez directement les pages [Hardware](https://www.dmdos.net/) et [Montage](https://www.dmdos.net/) du site.

> ⚠️ **DMDos** (le site) propose son propre système d'exploitation/firmware pour ce même matériel. **Ne flashez pas le firmware DMDos** sur votre ESP32 si vous souhaitez utiliser le firmware **RecalBoxDMD** de ce dépôt — seul le **matériel** (panneaux, DMDos Board, boîtier) et le **guide de montage physique** sont réutilisés ici. Le firmware et la carte SD doivent venir de ce Toolkit (voir [Firmware ESP32](#firmware-esp32-raw565-edition)).

### 🛒 Matériel (liens d'achat)

| Composant | Description |
|-----------|-------------|
| **Carte ESP32 USB-C** | ESP32 DevKit V1, connexion USB-C |
| **DMDos Board V3** | Conçue par **MORTACA** — connecte l'ESP32 au panneau DMD, intègre le lecteur microSD et l'alimentation des panneaux |
| **MicroSD 32 Go** | Type carte RaspberryPi recommandé (évite les corruptions), mais n'importe quelle carte convient |
| **2× Panneaux P4 64x32** | À assembler horizontalement pour obtenir un panneau 128x32 (256x128 mm chacun) |

👉 Liens d'achat à jour (AliExpress, Mortaca) sur la page **[Hardware de dmdos.net](https://www.dmdos.net/)**.

### 🪛 Étapes de montage

**Étape 1 — Assembler les deux panneaux**

En suivant ces étapes, vous pourrez monter votre panneau DMD en moins de cinq minutes, même sans expérience en bricolage. Commencez par assembler les deux panneaux à l'aide des pièces de jonction fournies avec la DMDos Board. Les vis ne sont pas incluses, mais n'importe quelle vis M3 que vous avez chez vous convient — par exemple récupérée sur une multiprise électrique.

**Étape 2 — Positionner la DMDos Board**

Assemblez les panneaux en veillant à ce que l'orientation des composants à l'arrière soit identique des deux côtés. Une fois assemblés, vous verrez deux connecteurs identiques où positionner la DMDos Board : l'un est une **entrée**, l'autre une **sortie**. Placez la carte sur le connecteur d'**entrée**, dans l'orientation qui permet de la positionner facilement sans qu'elle ne heurte le support en plastique. ⚠️ Elle ne fonctionne que sur le connecteur d'entrée.

**Étape 3 — Câblage de l'alimentation**

Une fois la carte connectée, et **avant** de poser l'ESP32 dessus, reliez les câbles d'alimentation rouge et noir des deux panneaux aux bornes de la carte DMDos, comme indiqué par la sérigraphie : rouge d'un côté, noir de l'autre. Vous pouvez garder les connecteurs fournis avec les câbles et ne visser qu'une seule broche ; vous pouvez aussi couper le surplus ou dénuder tout le câble pour qu'il rentre entièrement dans la borne. Reliez également les deux panneaux entre eux avec la nappe plate fournie.

**Étape 4 — SD, ESP32 et alimentation**

Pour terminer, insérez la carte micro-SD préalablement préparée (voir [Workflow complet](#workflow-complet-recommande)) et emboîtez par-dessus l'ESP32 déjà flashé avec le firmware **RecalBoxDMD** (voir [Firmware ESP32](#firmware-esp32-raw565-edition)). Il ne reste plus qu'à le brancher à l'alimentation via le port USB-C.

### 📦 Boîtier / Mueble

- **Impression 3D** : modèle (carré ou arrondi) créé par **Janibol** de [Retromojones](https://www.youtube.com/@retromojones), disponible sur [Thingiverse](https://www.thingiverse.com/thing:6704880). Nécessite une imprimante 3D grand format (plateau ≥ 300×300×400 mm). Un kit imprimé (pièces + caches arrière + vis + interrupteur + connecteur USB-C externe, hors verre/plexiglas) peut aussi être commandé directement — voir la page **[Mueble de dmdos.net](https://www.dmdos.net/)**.
- **Boîtier bois sur mesure** : des artisans proposent des meubles « old school » avec vitre parsol incluse — voir les contacts sur la page **[Mueble de dmdos.net](https://www.dmdos.net/)**.

---

## ⚡ Firmware ESP32 — Raw565 Edition

### 💡 Pourquoi le raw565 ?

Le format **raw565** est l'élément clé de cette version. Voici pourquoi :

```
┌────────────────────────────────────────────────────────────────┐
│                       CARTE SD                                  │
│                                                                │
│  PNG (50 Ko)  →  Conversion PC →  .raw565 (8 192 octets)      │
│  GIF (200 Ko) →  Conversion PC →  .raw565pack + .meta          │
│                                                                │
│  L'ESP32 n'a plus qu'à faire :                                 │
│    1. Ouvrir le fichier .raw565 (8 Ko)                         │
│    2. Lire 8192 octets en RAM                                  │
│    3. drawRGBBitmap() → affichage direct                        │
│                                                                │
│  🚀 Temps total : 5-15 ms (vs 500ms-3s pour PNG/GIF)          │
└────────────────────────────────────────────────────────────────┘
```

Les formats sont décrits en détail [dans la section dédiée](#-le-format-raw565-en-détail).

### 🔧 Compilation avec Arduino IDE

1. Ouvrez `RecalBox_DMD.ino` dans l'Arduino IDE
2. Installez les bibliothèques suivantes :

| Bibliothèque | Lien | Utilité |
|---|---|---|
| ESP32-HUB75-MatrixPanel-I2S-DMA | [GitHub](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) | Contrôle DMA du panneau LED |
| AnimatedGIF | [GitHub](https://github.com/bitbank2/AnimatedGIF) | Décodage des GIFs (fallback) |
| pngle | [GitHub](https://github.com/kikuchan/pngle) | Décodage des PNGs (fallback) |
| WiFiManager | [GitHub](https://github.com/tzapu/WiFiManager) | Configuration Wi-Fi |
| Adafruit GFX Library | [GitHub](https://github.com/adafruit/Adafruit-GFX-Library) | Affichage texte et formes |
| ArduinoJson | [GitHub](https://github.com/bblanchon/ArduinoJson) | Configuration et web |

3. Sélectionnez la carte **ESP32 Dev Module** dans le menu **Outils → Type de carte**
4. Branchez l'ESP32, sélectionnez le bon port COM
5. Cliquez sur **Téléverser** (⚡)

### ⚙️ Configuration (config.ini)

Placez ce fichier à la racine de la carte SD :

```ini
# Info
info=0                     # 0 = pas d'info au démarrage

# Playlist
playlist=TODO.txt          # Playlist à lire dans /playlist
random=1                   # 0 = ordre, 1 = aléatoire

# Wi-Fi
wifi_enabled=1
wifi_ssid=mon_wifi
wifi_password=mon_mdp
wifi_static_enabled=1
wifi_static_ip=192.168.1.240
wifi_gateway=192.168.1.1
wifi_subnet=255.255.255.0

# MQTT
recalbox_ip=192.168.1.104   # IP fixe de votre Recalbox
image_folder=logo_detoure   # ou "marquee"
```

### 🌐 Installation via navigateur

> [👉 Installer depuis la page WebInstaller](https://jamyz.github.io/RetroBoxLED/)

1. Utilisez Chrome ou Edge
2. Branchez l'ESP32 en USB
3. Cliquez sur **Install** et sélectionnez le port COM
4. **Important** : Cochez **« Erase device »** pour effacer complètement la mémoire !

### 📡 MQTT — Le cerveau du système

MQTT permet à Recalbox de communiquer avec l'ESP32 en temps réel :

```
Recalbox → marquee[rungame,...].sh → MQTT → ESP32 → Panneau LED

1. Vous lancez "King of Fighters '98"
2. Le script bash détecte l'événement → envoie "mame/kof98" via MQTT
3. L'ESP32 reçoit → cherche dans l'ordre :
   a. /systems/mame/kof98.raw565      ← instantané (5 ms)
   b. games_cache.bin → bigramme      ← indexation accélérée
   c. /systems/_defaults/mame.raw565  ← fallback système
   d. /systems/_defaults/default.raw565 ← fallback global
4. Affiché en moins de 15 ms !
```

Placez `marquee[rungame,endgame,systembrowsing,...].sh` dans :
```
/recalbox/share/userscripts/
```

### 🔌 Telnet

Le firmware inclut un terminal Telnet pour tester l'ESP32. Connectez-vous avec :
```
telnet 192.168.1.240
```
Tapez `help` pour la liste des commandes.

---

## 🗂️ Le format raw565 en détail

### 📄 .raw565 (image fixe PNG)

```
Taille : 128 × 32 × 2 = 8 192 octets exactement
Format : RGB565 brut (16 bits par pixel)
         Bit R: bits 15-11 (5 bits)
         Bit G: bits 10-5  (6 bits)
         Bit B: bits 4-0   (5 bits)

Lecture ESP32 :
  f.read(buffer, 8192);
  drawRGBBitmap(0, 0, buffer, 128, 32);
  // 1 seule opération SD + 1 seul draw → 5 ms
```

### 🎞️ .raw565pack + .meta (GIF animé)

Le fichier **`.raw565pack`** concatène toutes les trames du GIF :

```
[NomFichier].raw565pack
├── Frame 0  →  8 192 octets (raw565)
├── Frame 1  →  8 192 octets
├── Frame 2  →  8 192 octets
├── ...
└── Frame N  →  8 192 octets

[NomFichier].meta
├── delay_0  →  2 octets (uint16, millisecondes)
├── delay_1  →  2 octets
├── delay_2  →  2 octets
└── ...
```

**Avantages** :
- **1 seule ouverture SD** pour lire une frame (seek + read bulk)
- **Pas de décodage GIF** sur l'ESP32
- Lecture accélérée via le cache RAM des delays (`.meta` chargé en une fois)
- Contrôle de vitesse intégré (`GIF_RAW_PACK_SPEED_PERCENT`)

### ⚡ Cache bigramme — indexation accélérée

Pour éviter de lister les dossiers SD (très lent avec 30 000 fichiers), le script construit un **index bigramme** :

```
games_cache.bin
├── [Entête]  → nombre de systèmes indexés
├── [Système 0] → nom + offset table bigramme
├── [Système 1] → nom + offset
└── ...

Table bigramme (703 entrées = 2812 octets par système)
├── Index 0   → '#'  (chiffres, symboles)
├── Index 1   → 'A'  (jeux commençant par A)
├── Index 2   → 'AA' (jeux commençant par AA)
├── Index 3   → 'AB'
├── ...
└── Index 702 → 'ZZ'

Chaque entrée = offset dans le fichier cache vers la liste des jeux
```

Quand l'ESP32 cherche `kof98` :
1. Calcule l'index bigramme : `K` → index `11*27+1=298` (préfixe `KO`)
2. Charge la tranche correspondante en RAM (lecture bulk)
3. Parcourt les noms de jeux dans cette tranche
4. Trouve `kof98` et son type (`p` pour raw565) → **instantané**

### 🎭 Système de masque (mask)

Pour les très gros systèmes (MAME, FBNeo, etc.), le mécanisme de masque évite les écrans noirs :

```
┌─────────────────────────────────────────────────────────────┐
│  Étape 1 : Script Python analyse le système                │
│  → Compte les fichiers individuels                          │
│  → >800 fichiers ? Flag "L" dans systems_cache.dat         │
│                                                             │
│  Étape 2 : ESP32 reçoit "mame/kof98"                       │
│  → Système MAME flagué "L" ?                               │
│    OUI → Affiche IMMÉDIATEMENT le default raw565            │
│           (préchargé en RAM, 0 ms d'attente)               │
│                                                                   │
│  Étape 3 : En parallèle, tâche asynchrone :                │
│  → Cherche le .raw565 spécifique                           │
│  → Pas trouvé ? Décode le PNG en tâche de fond             │
│  → Une fois prêt : remplace le masque par l'image réelle   │
│                                                             │
│  Résultat : L'utilisateur ne voit jamais d'écran noir !     │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Structure de la carte SD

```
📁 SD CARD (FAT32)
   │
   ├── 📄 config.ini              ← Configuration ESP32
   │
   ├── 📁 systems/
   │   ├── 📁 mame/
   │   │   ├── 🖼️ kof98.raw565          ← PNG converti (8 Ko)
   │   │   ├── 🖼️ sf2.raw565
   │   │   ├── 🖼️ intro.raw565pack      ← GIF converti (trames)
   │   │   ├── 🖼️ intro.meta            ← Timings GIF
   │   │   └── ...
   │   ├── 📁 snes/
   │   │   ├── 🖼️ supermetroid.raw565
   │   │   └── ...
   │   └── 📁 _defaults/
   │       ├── 🖼️ default.raw565        ← Image de remplacement global
   │       ├── 🖼️ mame.raw565           ← Fallback pour système MAME
   │       ├── 🖼️ snes.raw565
   │       └── ...
   │
   ├── 📁 gifs/                   ← Playlists d'animations
   │   ├── 📁 Arcade/
   │   ├── 📁 BEST_OF_TOP_30/
   │   └── 📁 Pixel_Art/
   │
   ├── 📁 playlists/
   │   ├── 📄 Arcade.txt
   │   └── 📄 TODO.txt
   │
   ├── 📄 games_cache.bin         ← Cache bigramme (indexation jeux)
   └── 📄 systems_cache.dat       ← Index systèmes + flag lent/rapide
```

---

## 🔧 Dépannage

### ❌ Problème : "Pillow n'est pas installé"
- Le script tente de l'installer automatiquement
- Si ça échoue, lancez manuellement : `pip install Pillow`

### ❌ Problème : "API GitHub inaccessible"
- Vérifiez votre connexion internet
- Les modes _defaults nécessitent une connexion

### ❌ Problème : "Aucun lecteur amovible détecté"
- Insérez votre carte SD
- Vérifiez qu'elle est détectée par Windows (Explorateur → Ce PC)
- Réessayez

### ❌ Problème : L'ESP32 n'affiche rien
- Vérifiez l'alimentation (5V 4A minimum)
- Vérifiez que config.ini est bien à la racine de la SD
- Vérifiez le cablage HUB75 (brochage ESP32)
- Essayez le Telnet : tapez `help` pour tester

### ❌ Problème : "ESP32 non détecté" (port COM manquant)
Installez les drivers USB :

| Puce USB | Pilotes |
|----------|---------|
| CP2102 | [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| CH340/CH341 | [SparkFun](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all) |

### ❌ Problème : Affichage lent ou écran noir entre les jeux
- Vérifiez que vous utilisez le **Mode 1** (conversion raw565 complète)
- Les gros systèmes (MAME, FBNeo) doivent être flagués **« L »** — vérifiez dans `systems_cache.dat`
- Si le flag est `N` (rapide) mais le système est lent, augmentez le seuil `SPRITE_LIMIT` dans le .ino
- Vérifiez que les fichiers `.raw565` existent bien dans `systems/<sys>/`

### ❌ Problème : L'image ne s'affiche pas sur le panneau
- Vérifiez que l'image est bien au format 128x32
- Les PNG doivent être en RGB (pas RGBA) pour la conversion
- Les raw565 sont prioritaires sur les raw565pack

### ❌ Problème : La mauvaise image apparaît (jaquette au lieu du logo/marquee)
- C'est généralement un mauvais réglage du champ Scraper de Recalbox — vérifiez le profil **« Version Recalbox »** (Mode 1/3) et cliquez **« Comment scraper ? »** pour la marche à suivre exacte selon votre version
- Utilisez **« Nettoyer les dossiers avant scrape »** puis relancez un scrape complet dans Recalbox avec le bon réglage
- Lancez le **Mode 8** pour vérifier quelles images sont réellement présentes

---

## 📁 Fichiers générés par le script

| Fichier | Format | Utilité |
|---------|--------|---------|
| `sd_card/systems/.../*.raw565` | 8192 octets RGB565 | Image PNG convertie (affichage 5ms) |
| `sd_card/systems/.../*.raw565pack` | Trames concaténées | GIF animé converti |
| `sd_card/systems/.../*.meta` | uint16[] × nb_frames | Timings des trames GIF |
| `sd_card/systems/_defaults/*.raw565` | 8192 octets | Images de remplacement par système |
| `sd_card/games_cache.bin` | Index bigramme 703 entrées | Cache des jeux (recherche accélérée) |
| `sd_card/systems_cache.dat` | Texte (val + nom + flag) | Index systèmes + flag L/N |
| `images_manquantes.txt` | Texte | Liste des images manquantes (Mode 1/2/3) |
| `reports/mode8_report_*.txt` | Texte | Rapport du Mode 8 (vérification gamelist ↔ images) |

---

## 🤝 Crédits

- **Projet original RetroBoxLED** : [Jamyz](https://github.com/Jamyz/RetroBoxLED) — le firmware ESP32 et l'idée de base
- **Raw565 Edition** : Shan_ayA — optimisation du format raw565, cache bigramme, système de masque, interface graphique, gestion des versions Recalbox
- **Inspiration** : [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) par fjgordillo86
- **Communauté** : [Recalbox](https://www.recalbox.com/)
- **Matériel & guide de montage** : [Mortaca - DMDos Board](https://www.mortaca.com/) et [dmdos.net](https://www.dmdos.net/)
- **Boîtier 3D** : Janibol — [Retromojones](https://www.youtube.com/@retromojones)

---

## ☕ Soutien

Si ce projet vous est utile :
👉 [Faire un don via PayPal](https://www.paypal.com/paypalme/felysaya)

---

> **RecalBoxDMD Raw565 Edition** = Recalbox + Panneau DMD LED + Affichage rapide même avec 30 000 jeux MAME ! 🎮⚡
