# 🎮 RetroBoxLED 

Firmware **ESP32** pour **Recalbox** LED marquee (HUB75/DMD 128x32 P4 panels).

✅Compatible Recalbox 10.0.5

## ✨ Fonctionnalités

- **GIF playback** : Lecture de GIFS et de PNG (/Arcade, /BEST_OF_TOP_30, /Pixel_Art, etc...)
- **Fallbacks** : /systems/_defaults/_default.png 
- **Playlists** : Arcade.txt, Favories.txt, Consoles.txt
- **MQTT** : EmulationStation events (rungame, shutdown...)
- **Recalbox** : Auto Arcade mode


## ⭐ **Fonctionnement**

Par défaut le ESP32 jouera une playlist de gifs.
Une fois qu'il reçoit les infos de MQQT. Il se mettra automatiquement en mode ARCADE.
Une fois que la Recalbox est éteinte. Le ESP32 jouera à nouveau la playlist.
Si il manque un gif ou png. Il jouera le gif ou png dans le dossier que vous aurez mis dans /systems/_defaults.

## 📁 Structure SD

La carte SD devra être formaté en FAT32.
En  ayant la structure suivante.
Copier le dossier _defaults dans le repertoire systems de votre carte SD.

```
RetroBoxLED SD Card/
├── gifs/
│   ├── Arcade/, BEST_OF_TOP_30/, Pixel_Art/| GIFs
├── systems/
│   ├── mame/, neogeo/, snes/               | GIFs
│   ├── _defaults/                          | fallback
├── playlists
│   ├── Arcade.txt, Favories.txt, Consoles.txt

```

## 🚀 Installation

Avant utilisation.

1. **Configuration** : Configuration du fichier config.ini
2. **Playlists** : Création de playlists
3. **Outils** : Utilisation des scripts
4. **Flash** : ESP32 firmware
5. **MQTT** : Explication et fonctionnement
6. **Telnet** : Terminal Telnet pour test



## 1 - ⚙️ Configuration

Vous devez avoir le fichier config.ini à la racine de la carte SD.
Dans celui-ci vous pourrez configurer

```
#Info
info=0 #0=aucune info au boot, 1=affiche les infos au boot

#Playlist
playlist=TODO.txt #Joue la playlist indiqué dans /playlist
random=1 #0=pour jouer la playlist par ordre, 1=pour jouer la playlist aleatoirement

#Wifi & Bluetooth
wifi_enabled=1 #0=Desactive le wifi, 1=Active le wifi. Laissez sur 1
wifi_ssid=monwifi #Nom de votre reseau wifi
wifi_password=monpasse #Mot de passe de votre reseau wifi
bluetooth_enabled=0 #0=Desactive le bluetooth, 1=Active le bluetooth. Laissez sur 0. Cela depends si vous avez des interferences, par exemple manette 8Bitdo Pro 3
bluetooth_name=ESP32-GIF #Nom Bluetooth

wifi_static_enabled=1 #0=DHCP, 1=IP fixe. Preferez une IP fixe.
wifi_static_ip=192.168.20.240 #À renseigner seulement si wifi_static_enabled=1
wifi_gateway=192.168.20.1  #À renseigner seulement si wifi_static_enabled=1
wifi_subnet=255.255.255.0  #À renseigner seulement si wifi_static_enabled=1
wifi_dns1=1.1.1.1  #À renseigner seulement si wifi_static_enabled=1
wifi_dns2=8.8.8.8  #À renseigner seulement si wifi_static_enabled=1

#MQQT
recalbox_ip=192.168.20.104 #Adresse IP fixe de votre Recalbox
mqtt_timeout=30 #Temps d'attente avant de lancer la playlist si MQQT ne reponds pas. L'ideal est de mettre comme l'economiseur d'écran.
```

## 2 - ▶️ Playlists

Vous pouvez créer vous même vos playlists.
Pour cela vous retrouverez l'outil **Generador de Playlists v1.0.1.bat** modifié de https://github.com/fjgordillo86/RetroPixelLED dans le dossier **tools** de ce dépôt.
Il listera tout les dossiers dans le répertoire **gifs**.
Si vous avez un dossier Arcade, BEST_OF_TOP_30, Pixel_Art, etc...., avec des gifs dans ces dossiers.
Vous pourrez choisir les dossiers à mettre dans votre playlist (par exemple les dossiers 1,3,5).
Si vous voulez tout mettre dans une playlist. Mettez "TODO".


## 🛠️ - 💾 Outils

Deux Scripts sont à votre disposition.

**extract_gamelist_images** : Extrait les images de vos dossiers medias et les renomme selon le path du gamelist.xml
**convert_128x32** : Converti les images en 128x32

L'ideal est de placer ces deux fichiers dans un repertoire pour avoir tout sous la main. Il copiera les images dans le dossier où se trouvent les scripts.
La meilleure option pour le panneau est de faire un scrapping avec le type d'image **LOGO DETOURE** ou **MARQUEE** qui sont ideals pour le panneau LED.
Une fois les images convertis. Copier tout les dossiers et coller tout dans /systems de votre carte SD.

Vous trouverez un dossier avec tout les systemes et leur images déjà convertie.
Vous aurez aussi un systeme nommé **_defaults**. Si vous placez un fichier _default.gif ou _default.png sera celui par defaut si aucune image systeme est trouvée.
Par défaut les gifs sont prioritaires sur les png.

## 4 -⚡Flash

Avant de débuter. Assurez vous que votre PC reconnait votre ESP32.

## 💡 ESP32 non détecté ?

**Si "Install" ne trouve pas le port COM** :

| Chip USB | Drivers |
|----------|---------|
| **CP2102** | [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| **CH340/CH341** | [SparkFun](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all) |

### **[👉 Installation depuis la page web de RETRO PIXEL LED](https://jamyz.github.io/RetroBoxLED/)**

Étapes pour l’installation :

- Utilise un navigateur compatible (Google Chrome ou Microsoft Edge).
- Connecte ton ESP32 au port USB de l’ordinateur.
- Clique sur le bouton "Install" sur la page web et sélectionne le port COM correspondant.
- IMPORTANT : Assure-toi de cocher la case "Erase device" dans l’assistant afin d’effectuer un effacement complet de la mémoire et éviter les erreurs de fragmentation.

## 5 -🧠 MQTT - Cerveau RetroBoxLED

Ce que fait MQTT c'est dire au ESP32 ce qu'il doit lancer.

**Recalbox → "Je lance MAME" → MQTT → ESP32 → "Affiche GIF ou PNG mame !"**

- Synchronisation : LED affiche exactement le jeu lancé
- Réseau local : 192.168.XXX.XXX (WiFi arcade)

Voici un exemple
```
1. Tu lances King of Fighters (mame/kof98)
2. marquee[rungame,endgame,shutdown,reboot,systembrowsing,start](permanent).sh détecte → MQTT "mame/kof98"  
3. ESP32 reçoit → /systems/mame/kof98.gif
4. Pas de GIF ? → /systems/_defaults/_default.gif
```
Le fichier [`marquee[rungame,endgame,shutdown,reboot,systembrowsing,start.sh]`](https://example.com)
doit etre dans /recalbox/share/userscripts/ de votre Recalbox

## 6 - >_ Telnet

Le firmware contient un Telnet pour tester le ESP32.
Tapez help pour afficher la liste des commandes.
Envoyé des commandes pour modifier les gifs, etc.....
Il sera eliminé plus tard. Une fois que le code fonctionnera correctement et liberer de l'espace sur le ESP32.

## 📚 Librairies Nécessaires

Pour compiler et programmer correctement le projet depuis l'IDE Arduino, vous devez installer les librairies suivantes. Vous pouvez les rechercher dans le Gestionnaire de Librairies d'Arduino ou les télécharger depuis leurs dépôts officiels :

-   **[ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA)** : Contrôle haute performance du panneau LED via DMA.
-   **[AnimatedGIF](https://github.com/bitbank2/AnimatedGIF)** : Décodeur efficace pour la lecture de fichiers GIF depuis la carte SD.
-   **[pngle](https://github.com/kikuchan/pngle)** : Pour la lecture de fichiers PNG gerant l'alpha depuis la carte SD.
-   **[WiFiManager](https://github.com/tzapu/WiFiManager)** : Gestion de la connexion Wi-Fi via un portail captif.
-   **[Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)** : Librairie de base pour dessiner du texte et des formes géométriques.    
-   **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)** : Pour la gestion des fichiers de configuration et la communication web.

## 🛒 **Liste des Matériaux**

Pour garantir la compatibilité, l'utilisation des composants testés pendant le développement est recommandée :
-   **Microcontrôleur** : [ESP32 DevKit V1 (38 broches)](https://es.aliexpress.com/item/1005005704190069.html)
-   **Panneau LED Matrix (HUB75)** : [Panneau RGB P2.5 / P4](https://es.aliexpress.com/item/1005007439017560.html)
-   **Lecteur de cartes** : [Module adaptateur Micro SD (SPI)](https://es.aliexpress.com/item/1005005591145849.html)
-   **Carte de connexion ESP32-Panneau LED** : [DMDos Board V3 - Mortaca](https://www.mortaca.com/) _(Optionnel : pas de soudure + lecteur SD intégré)_
-   **Alimentation** : Source 5V (Minimum 4A recommandé pour panneaux 64x32)

## 🤝 Credits

- [RetroPixelLED original](https://github.com/fjgordillo86/RetroPixelLED)
- [Recalbox community](https://www.recalbox.com/fr/)
- [Systems logos  are released under Creative Commons Attribution-NonCommercial-NoDerivs 4.0 License CC BY-NC-ND](https://creativecommons.org/licenses/by-nc-nd/4.0/)
- [Bounitos](https://github.com/BenoitBounar)

## ☕ Soutenir le projet

Si ce projet t’a aidé, tu peux m’inviter à un café :

👉 [☕ Donate via PayPal](https://www.paypal.com/paypalme/jamyz77)

**RetroBoxLED** = Recalbox + Pixel LED perfection ! 😎
