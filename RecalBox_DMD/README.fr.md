# RecalBox DMD — Firmware ESP32 pour Panneau Marquee

## Résumé

**RecalBox_DMD** est un firmware ESP32 complet pour piloter un **panneau HUB75 LED (128×32 px)** affichant les images marquee des jeux Recalbox. Le firmware supporte PNG et GIF, cache de jeux, intégration MQTT en temps réel, Telnet pour le débogage et Bluetooth.

---

## Caractéristiques

- **Support Images** : PNG et GIF avec cache intelligent
- **Optimisation Perf** : formats raw565 / raw565pack pour rendu rapide
- **Cache Jeux** : lookup bigramme indexé pour recherche rapide
- **Cache Systèmes** : détection type système et fallback automatique
- **Réseau** :
  - WiFi (statique ou DHCP)
  - MQTT (affichage en temps réel du jeu/système)
  - Telnet (débogage & contrôle manuel)
- **Audio** : support Bluetooth Serial
- **Configuration** : pilotée par `config.ini`
- **Fallback** : retour automatique aux images par défaut si manquantes

---

## Matériel Requis

### Microcontrôleur
- **ESP32** (testé sur DEVKIT-C, 4 MB de flash recommandé)

### Panneau Affichage
- **Panneau HUB75 LED** 64×32 pixels, **2 panneaux en chaîne** (128×32 visibles)

### Stockage
- **Carte SD** (microSD via adaptateur SPI, **formatée FAT32**)

### Alimentation
- 5V pour panneaux LED (~2A typique)
- 5V USB pour ESP32 (peut partager l'alimentation via régulateur séparé)

---

## Configuration des Broches

### SPI (Carte SD)
| Broche | GPIO |
|--------|------|
| CS     | 5    |
| MISO   | 19   |
| MOSI   | 23   |
| SCLK   | 18   |

### Broches Panneau HUB75
| Signal | GPIO |
|--------|------|
| CLK    | 16   |
| OE     | 15   |
| LAT    | 4    |
| A      | 33   |
| B      | 32   |
| C      | 22   |
| D      | 17   |
| E      | -1   |

### Broches Couleur (Demi-Canaux)
| Moitié Haute | GPIO | Moitié Basse | GPIO |
|--------------|------|--------------|------|
| R1           | 25   | R2           | 14   |
| G1           | 26   | G2           | 12   |
| B1           | 27   | B2           | 13   |

---

## Structure de la Carte SD

```
(Racine SD)
├── config.ini                          # Configuration principale
├── systems_cache.dat                   # Index des systèmes (généré)
├── games_cache.bin                     # Cache images jeux (généré)
├── systems/
│   ├── _defaults/
│   │   ├── default.png                # Image de secours (128×32 PNG)
│   │   ├── default.raw565             # Secours rapide (128×32 raw565)
│   │   ├── <system>.png               # Logos systèmes
│   │   ├── <system>.gif
│   │   ├── <system>.raw565
│   │   ├── <system>.raw565pack        # GIF pré-encodé en raw frames
│   │   └── <system>.meta              # Métadonnées délai raw565pack
│   │
│   └── <system>/
│       ├── <game>.png                 # Images marquee jeux
│       ├── <game>.gif
│       ├── <game>.raw565              # Format rapide pré-converti
│       └── <game>.raw565pack          # Pack raw frames
│
└── playlists/
    ├── <playlist_name>.txt            # Liste fichiers GIF
    ├── <playlist_name>.cache          # Cache compilé
    ├── <playlist_name>.idx            # Index offsets
    └── <playlist_name>.sig            # Signature hash
```

---

## Bibliothèques Arduino Requises

Installer via **Sketch → Include Library → Manage Libraries** :

| Bibliothèque | Auteur | Version |
|--------------|--------|---------|
| `ESP32-HUB75-MatrixPanel-I2S-DMA` | mrfaptastic | ≥1.0.0 |
| `AnimatedGIF` | Larry Bank | ≥2.0.0 |
| `SD` | Arduino | (intégré) |
| `SPI` | Arduino | (intégré) |
| `WiFi` | Arduino | (intégré) |
| `PubSubClient` | Nick O'Leary | ≥2.8.0 |
| `BluetoothSerial` | Espressif | (noyau ESP32) |

**Note** : `pngle.h` est inclus directement dans le sketch pour décodage PNG.

---

## Configuration (`config.ini`)

Exemple :

```ini
# Playlist à afficher au démarrage
playlist=marquee.txt

# Configuration WiFi
wifi_enabled=1
wifi_ssid=MonSSID
wifi_password=motdepasse123
wifi_static_enabled=0
# IP statique optionnelle :
# wifi_static_ip=192.168.1.100
# wifi_gateway=192.168.1.1
# wifi_subnet=255.255.255.0
# wifi_dns1=8.8.8.8
# wifi_dns2=8.8.4.4

# Broker MQTT Recalbox
recalbox_ip=192.168.1.50

# Topic MQTT (reçoit événements de Recalbox)
mqtt_event_topic=marquee/event

# Bluetooth
bluetooth_enabled=0
bluetooth_name=ESP32-GIF

# Écran splash au démarrage
info=1

# Mode lecture playlist
random=1
```

---

## Compilation et Flash

### 1. Prérequis
- Arduino IDE ≥ 1.8.0
- Paquet Board ESP32 (≥2.0.0)
  - URL du gestionnaire : `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

### 2. Ouvrir le Sketch
- File → Open → `RecalBox_DMD.ino`

### 3. Configurer la Carte
- Tools → Board : Choisir **ESP32 Dev Module** (ou votre variante)
- Tools → Flash Size : Choisir **4MB** (ou la taille de votre carte)
- Tools → Upload Speed : **921600** (ou plus bas si instable)
- Tools → Port : Sélectionner le port COM

### 4. Installer les Dépendances
- Sketch → Include Library → Manage Libraries
- Chercher et installer chaque bibliothèque du tableau ci-dessus

### 5. Compiler
- Sketch → Verify (Ctrl+R)

### 6. Upload
- Sketch → Upload (Ctrl+U)
- Ou via **esptool.py** (ligne de commande) :
  ```bash
  esptool.py --chip esp32 --port COM3 --baud 921600 write_flash -z 0x10000 RecalBox_DMD.ino.bin
  ```

---

## Utilisation

### Démarrage
1. Alimenter l'ESP32
2. Si `info=1`, affichage splash (2,5 secondes)
3. Si WiFi activé, connexion et affichage du statut
4. Charger la playlist (si configurée) ou afficher par défaut
5. Prêt pour commandes MQTT / entrée Telnet

### Topics MQTT (Entrée)
- `marquee/cmd/stop` — Arrêter lecture
- `marquee/cmd/default` — Reprendre playlist
- `marquee/cmd/system <system_name>` — Afficher logo système
- `marquee/cmd/game <system>/<game>` — Afficher image jeu
- `marquee/event` — S'abonner événements Recalbox (traité automatiquement)

### Commandes Telnet (Port 23)
```
help               — Lister toutes les commandes
ip                 — Afficher adresse IP
wifi               — Afficher statut WiFi
wifiinfo           — Info WiFi détaillée
next               — GIF suivant
count              — Nombre GIF chargés
playlist           — Playlist courante
random on|off      — Activer/désactiver lecture aléatoire
reboot             — Redémarrer ESP32
show <path>        — Afficher fichier (ex: /systems/ps2/game.png)
showsys <system>   — Afficher logo système
showgame <sys/rom> — Afficher image jeu
default            — Afficher image par défaut
black              — Effacer écran
mode               — Afficher mode affichage courant
syscache           — Lister cache systèmes
rebuildcache       — Reconstruire cache systèmes
heap               — Afficher mémoire libre
```

---

## Pipeline Traitement Données

### Cache Systèmes
1. Au premier démarrage (ou `rebuildcache`), scanner dossier `/systems/`
2. Détecter types images disponibles par système (PNG/GIF/raw565pack)
3. Marquer systèmes >800 jeux comme "lents" (flag 'L')
4. Sauvegarder index dans `systems_cache.dat`

### Cache Jeux
1. Scripts Python (`tools/`) prétraitent listes jeux depuis `gamelist.xml`
2. Extraire images marquee, convertir PNG en 128×32
3. Construire `games_cache.bin` avec lookup bigramme indexé
4. Copier racine carte SD

### Flux Affichage
- **Système Rapide (N)** : décode PNG + affiche, fallback raw565, fallback défaut
- **Système Lent (L)** : vérifier cache d'abord, fallback raw565 défaut (skip décode PNG)
- **Requête Jeu MQTT** : charger mask système, puis image jeu, avec chaîne fallback

---

## Outils Python

Des scripts de préparation sont disponibles dans le dossier `tools/` :

- `RetroBoxLED_gui.py` — GUI pour extraction images, conversion et préparation SD
- `RetroBoxLED_tool3.3.py` — Processeur batch ligne de commande

Voir `tools/README.fr.md` pour instructions détaillées.

---

## Dépannage

| Problème | Solution |
|----------|----------|
| **Carte SD non détectée** | Vérifier broche CS (5), s'assurer FAT32, tester avec `dir` en Telnet |
| **Panneau affiche du bruit** | Vérifier broches HUB75 correspond config; vérifier polarité panneau |
| **Chargement images lent** | Marquer système lent (flag 'L') en config; pré-convertir PNG en raw565 |
| **WiFi / MQTT non fonctionnel** | Vérifier SSID, mot de passe, IP broker dans `config.ini`; vérifier Telnet `wifiinfo` |
| **Manque mémoire** | Réduire cache bigram ou convertir grands PNG en raw565pack |

---

## Notes Performance

- **Décodage PNG** : ~200–500ms par image (dépend taille et vitesse système)
- **Lecture GIF** : 10–50 FPS (limité par refresh panneau et vitesse SPI)
- **Blitting raw565** : ~50ms par refresh plein écran (optimisé avec lecture unique en bloc)
- **Latence MQTT** : ~100–500ms du message à affichage

---

## Licence

Ce firmware est fourni tel quel pour usage personnel et éducatif.

---

## Support & Feedback

Pour problèmes, questions ou contributions, référez-vous au dépôt du projet ou contactez le mainteneur.

---

**Version** : Raw565 Edition  
**Dernière mise à jour** : Juin 2026
