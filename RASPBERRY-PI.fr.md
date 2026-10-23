# RecalBoxDMD sur Raspberry Pi

[🇬🇧 English](RASPBERRY-PI.md) · 🇫🇷 **Français** · [🇪🇸 Español](RASPBERRY-PI.es.md)

Le même DMD, avec un **Raspberry Pi** à la place de l'ESP32 : mêmes scripts Recalbox, même contenu de carte SD, mêmes pages de configuration web, mêmes widgets et mêmes thèmes. Le Pi pilote deux dalles P5 64×32 (soit 128×32) par une carte d'interface HUB75.

> ✅ **Validé : Raspberry Pi 3B** (Raspberry Pi OS Lite 32 bits « Bookworm »).
> ⚠️ **Prévu mais non testé : Pi 4 / 400 / CM4, Pi 2, Zero 2.** Le logiciel détecte le modèle et règle `gpio_slowdown` en conséquence (Pi 4 : 4 ; Pi 2/3 : 2 ; Pi 1/Zero : 1), mais seul le Pi 3B a été essayé sur de vraies dalles. Si votre Pi scintille, ajustez `gpio_slowdown=1..4` dans `config.ini`.

## Installer (le plus simple : la boîte à outils PC)

1. Téléchargez la **boîte à outils PC** (page *Releases*) et lancez-la. En haut de la fenêtre, choisissez la cible **Raspberry Pi**.
2. **Mode 1** : l'assistant enchaîne tout, depuis une **carte SD vierge** jusqu'à une carte prête à insérer dans le Pi — formatage, Raspberry Pi OS Lite, votre Wi-Fi / utilisateur / SSH, logiciel du DMD, et vos logos convertis dans une partition `RBPIDMD`. Windows demande une autorisation administrateur (le disque est écrit en accès direct) ; le contenu de la carte est effacé après confirmation.
3. Insérez la carte dans le Pi, alimentez-le, attendez 2 à 3 minutes. Le DMD affiche **Page de configuration** et son adresse (ou ouvrez `http://rbpidmd.local/`). Sans Wi-Fi valide, le Pi ouvre le réseau **RecalBox-DMD-Config** (adresse 192.168.4.1) pour le configurer depuis un téléphone.
4. Renseignez l'adresse IP de votre Recalbox dans les réglages Wi-Fi, puis lancez le **Mode 9** de la boîte à outils : les scripts Recalbox sont **les mêmes** que pour l'ESP32.

Les modes séparés restent disponibles : **Mode 16** (préparer une carte, remplace Raspberry Pi Imager) et **Mode 17** (installer ou mettre à jour le logiciel sur un Pi déjà démarré, et copier le contenu de la carte SD du DMD). Le logiciel du DMD pour le Pi est **téléchargé depuis GitHub** (release `pi-v…`, empreinte SHA-256 vérifiée) ; sans réseau, la copie déjà présente sur le PC est utilisée.

## Matériel

- Raspberry Pi 3B (validé) et une carte d'interface HUB75 (cartes de type RPI2DMD : mapping GPIO « regular », horloge RTC DS1307 optionnelle).
- Deux dalles P5 64×32 en chaîne (128×32), balayage 1/16 : réglages par défaut `--led-cols=64 --led-rows=32 --led-chain=2 --led-pwm-bits=11 --led-slowdown-gpio=<selon le modèle>`.
- Un bouton physique optionnel entre GPIO 21 et GND.
- Le son du Pi est désactivé et un cœur est réservé à l'affichage (`isolcpus=3`) : c'est fait par l'installation.

## Mettre à jour

- **Depuis la page web du DMD (v3.59 et suivantes)** : la page **Réglages** affiche un avis quand une nouvelle version du démon est publiée, avec un bouton **Mettre à jour maintenant**. Le Pi télécharge la dernière release `pi-v…` sur GitHub, vérifie son empreinte SHA-256, installe, redémarre le service et **revient tout seul à la version précédente** si le démarrage échoue. Le DMD affiche « Mise à jour… » pendant l'opération (environ 1 minute). Un Pi resté en 3.58 passe une première fois en 3.59 par le Mode 17.
- **Mode 17** de la boîte à outils, ou
- à la main : récupérez `RecalBoxDMD_pi_payload.zip` dans la dernière release `pi-v…` et lancez `sudo bash install.sh` depuis le dossier décompressé sur le Pi.

La configuration (`config.ini`) et le contenu de la carte ne sont jamais écrasés.

**Animations de démarrage (v3.62)** : le démon joue l'animation Shan_ayA au démarrage, et sa version de fête autour d'Halloween, Noël, Nouvel An, Saint-Valentin et Pâques. Les six fichiers (`gifs/_boot/` de la carte de contenu) sont livrés avec la mise à jour du démon, copiés seulement s'ils manquent ou changent ; le Mode 19 de la boîte à outils peut aussi les envoyer.

## Compiler soi-même

Le démon est dans [`raspberry-pi/rpi_daemon/`](raspberry-pi/rpi_daemon/) (C++17, CMake). Il se lie à [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) de Henner Zeller :

```
git clone https://github.com/hzeller/rpi-rgb-led-matrix ~/rpi-rgb-led-matrix && make -C ~/rpi-rgb-led-matrix/lib
sudo apt install cmake g++ libpng-dev libgif-dev zlib1g-dev libcpp-httplib-dev
cd raspberry-pi/rpi_daemon && cmake -B build && cmake --build build -j3
```

Compter de 10 à 25 minutes sur un Pi 3B. `deploy/install.sh --build` fait tout cela et installe le service.

## Licence

Le dossier [`raspberry-pi/rpi_daemon/`](raspberry-pi/rpi_daemon/) est sous **GPL-2.0-or-later** (le démon est lié à rpi-rgb-led-matrix, GPL-2.0+) : texte dans `raspberry-pi/rpi_daemon/LICENSE`, détails dans `LICENSE.md`. Le binaire distribué est construit à partir des sources de ce dépôt. Le reste du dépôt (firmware ESP32, boîte à outils) reste sous licence MIT.

## Limites connues

- Quelques LED défectueuses des dalles restent visibles quel que soit le réglage (matériel).
- Mode natif Recalbox / Visual Pinball : validé au simulateur et à l'œil sur les dalles ; pas de campagne de test prolongée.
