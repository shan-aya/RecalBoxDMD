# 🎮 RetroBoxLED Toolkit GUI (Recalbox) — Aide 


## ✅ Objectif

Préparer la carte SD pour l’ESP32 **marquee HUB75 / DMD 128x32** en générant :

- `systems/<système>/...` (images par jeu/système)
- `systems/_defaults/` (fichiers de remplacement)
- `games_cache*.bin` (index des images jeux)
- `systems_cache.dat` (index des systèmes ESP32)
- (puis copie sur la carte SD)

---

## 📁 Où travaille le script ?

Le script crée un dossier temporaire (type “sd_card” de travail) dans **%LOCALAPPDATA%/Temp/RetroBoxLED** (Windows).
À la fin, le mode “copie” synchronise ce contenu vers la **racine** de la carte SD.

---

## 🎛️ Les modes (choix 1 à 8)

### Mode 1 — Extraction + Conversion + Build cache (TOUT)
- lit les `gamelist.xml` dans votre dossier ROMs
- extrait les images (PNG/GIF)
- convertit les **PNG → 128x32** (les GIF restent en l’état)
- construit `games_cache*.bin` + `systems_cache.dat`
- télécharge aussi `systems/_defaults` (GitHub)

### Mode 2 — Téléchargement `systems/_defaults` via GitHub (uniquement)
- **télécharge uniquement** `systems/_defaults/`
- **ne fait pas** extraction, conversion, ni génération des caches

> Idéal si vous voulez juste mettre à jour les remplacements `_defaults` sans toucher au reste.

### Mode 3 — Extraction gamelist uniquement
- extrait les images depuis `gamelist.xml`
- ne génère pas les caches
- ne convertit pas les PNG en **128x32**

### Mode 4 — Conversion uniquement
- convertit les PNG en **128x32**
- conserve les GIF
- ne génère pas les caches

### Mode 5 — Build `games_cache.bin` uniquement
- génère le cache des jeux à partir des images déjà présentes

### Mode 6 — Génération `systems_cache.dat` uniquement
- construit l’index systèmes ESP32
- s’appuie sur `systems/_defaults` et/ou les systèmes déjà présents

### Mode 7 — Copie sur la carte SD (robocopy)
- copie le dossier de travail (sd_card/) **vers la racine** de la carte SD
- utilise `robocopy` (rapide)

---

## 🧠 Remplacement `_defaults` (fallback)

Le toolkit utilise un dossier :
- `systems/_defaults/`

Si une image de système/jeu manque, l’ESP32 utilisera le fichier présent dans `_defaults`.

---

## 🖼️ Conversion 128x32 (PNG → 128x32)

La conversion nécessite **Pillow**.
Le script tente de l’installer automatiquement si besoin.
Si Pillow n’est pas disponible, la conversion est désactivée (mais les GIF restent utilisables).

---

## 🧪 Prérequis (carte SD & ROMs)

- Carte SD : format **FAT32**
- ROMs : contient des sous-dossiers avec un `gamelist.xml` (structure Recalbox classique)

---

## ▶️ Utilisation

### Via l’interface Tkinter
1. Lance `RetroBoxLED_gui.py`
2. Choisis le dossier ROMs
3. Sélectionne le **mode**
4. Clique **Démarrer**
5. (Mode 7) Choisis la carte SD et lance la copie



---

## ℹ️ Notes utiles
- Le script peut télécharger depuis GitHub (connexion internet requise pour les modes “_defaults”).


---

## 🧾 À vérifier après copie sur SD
Sur la carte SD (racine), vous devez voir le contenu généré, notamment :
- `systems/` (avec `_defaults/`)
- `systems_cache.dat`
- `games_cache.bin` 
