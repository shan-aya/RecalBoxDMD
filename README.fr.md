<p align="center"><img src="medias/branding/shan_aya_logo.png" width="260" alt="Logo Shan_ayA"></p>

<h1 align="center">RecalBoxDMD — RawEdition</h1>

<p align="center">
  <b>Un vrai panneau marquee LED pour votre borne d'arcade Recalbox.<br>Le logo de chaque jeu s'allume en 15 ms — même avec 30 000 jeux MAME.</b>
</p>

<p align="center">
  <img alt="Firmware : v4.33" src="https://img.shields.io/badge/firmware-v4.33-blueviolet.svg">
  <img alt="Outil PC : v199112" src="https://img.shields.io/badge/outil%20PC-v199112-blueviolet.svg">
  <a href="LICENSE"><img alt="Licence : MIT" src="https://img.shields.io/badge/licence-MIT-green.svg"></a>
  <img alt="Plateformes : ESP32 et Raspberry Pi" src="https://img.shields.io/badge/plateformes-ESP32%20%7C%20Raspberry%20Pi-blue.svg">
  <img alt="Panneau : HUB75 128x32" src="https://img.shields.io/badge/panneau-HUB75%20128x32-blue.svg">
  <img alt="Recalbox : 11 / 10.x / 9.x / legacy" src="https://img.shields.io/badge/recalbox-11%20%7C%2010.x%20%7C%209.x%20%7C%20legacy-orange.svg">
  <img alt="Langues : FR EN ES" src="https://img.shields.io/badge/UI-FR%20%7C%20EN%20%7C%20ES-purple.svg">
  <a href="https://discord.gg/CYAyGJSvp4"><img alt="Rejoindre le Discord : aide et communauté" src="https://img.shields.io/badge/Discord-rejoindre-5865F2?logo=discord&logoColor=white"></a>
  <a href="https://buymeacoffee.com/shan_aya"><img alt="Soutenir le projet : Buy Me a Coffee" src="https://img.shields.io/badge/Buy%20Me%20a%20Coffee-support-FFDD00?logo=buymeacoffee&logoColor=black"></a>
</p>

<p align="center">
  <a href="README.md">🇬🇧 English</a> · 🇫🇷 <b>Français</b> · <a href="README.es.md">🇪🇸 Español</a>
</p>

> ## 🧪 Branche ALPHA — versions de test
> Cette branche contient les versions **alpha** (firmware, démon Raspberry Pi, boîte à outils PC) pour les **testeurs de la Recalbox alpha**. Les fonctions peuvent être inachevées ou changer sans préavis. **Ne les installez pas sur une Recalbox stable.** Le manuel utilisateur complet est sur la [branche stable](https://github.com/shan-aya/RecalBoxDMD/blob/main/README.fr.md).

## 🔭 Les nouveautés à tester

- 📟 **Firmware ESP32**
  - **Ordre des couleurs** de la dalle (6 ordres) — T01, **page Réglages** remise en ordre — T03
  - **Changement de canal** alpha ↔ stable — T04, avis de mise à jour limités au canal alpha — T06
  - **GIF du jeu pendant la partie** (option *Basic*, `/gifs_jeux`, exclusions) — T10
  - Pages web qui reviennent toutes seules après une mise à jour — T07, **réglages qui survivent aux redémarrages** (`config.ini` protégé) — T08, **changement de Wi-Fi** — T09
- 🍓 **Démon Raspberry Pi**
  - Ordre des couleurs — T02, page Réglages — T03, changement de canal — T05, bouton de mise à jour qui reste en alpha — T16
  - GIF du jeu et **Playlist en Wi-Fi** — T10, T12
- 🧰 **Boîte à outils PC**
  - **Personnalisation des logos** — T11, **pastille de cible** et Playlist en Wi-Fi — T12
  - Onglet **Installation** — T13, **Formater en FAT32** — T14
  - Alerte d'écart de canal — T15, thèmes Recalbox (Modes 12 / 13) — T17

## 🙋 Participer aux tests alpha

1. Rejoignez le Discord : https://discord.gg/CYAyGJSvp4
2. Appuyez sur le bouton 🧪 du message alpha (ou tapez `/alpha`) pour obtenir le rôle **Tester** — il ouvre `#support-alpha` et `#alpha-tests`.
3. Installez l'alpha : https://shan-aya.github.io/RecalBoxDMD/install-alpha/ (ou changez de canal dans la page web *Réglages*).
4. Choisissez des tests dans `#alpha-tests` (ou le tableau ci-dessous), faites-les, signalez avec `/alphatest`.

## 📮 Signaler un résultat (sur Discord)

- Tapez **`/alphatest`** dans n'importe quel salon et renseignez : **test** (T01…), **résultat** (✅ fonctionne · ⚠️ fonctionne avec un défaut · ❌ ne fonctionne pas · ⏭️ impossible à tester), **appareil**, un **commentaire** (obligatoire sauf ✅ : versions, ce que vous avez fait, ce qui s'est passé) et une **capture / un journal** facultatif.
- Le bot poste votre résultat dans le post du test de `#alpha-tests` et le comptabilise : **`/alphaprogress`** montre ce que vous avez fait.
- Un bug qui dépasse un test : ouvrez un post dans `#support-alpha` avec les versions (firmware / Pi / toolkit / Recalbox), les étapes, l'attendu, l'observé (un journal `/diag` aide).

## ✅ Plan de test

Détail de chaque test (étapes et résultat attendu) : [ALPHA-TESTS.fr.md](ALPHA-TESTS.fr.md).

| Test | Zone | Quoi |
|---|---|---|
| **T01** | ESP32 | Ordre des couleurs de la dalle (ESP32) |
| **T02** | Raspberry Pi | Ordre des couleurs de la dalle (Raspberry Pi) |
| **T03** | ESP32 / Pi / Toolkit | Mise en page de la page Réglages |
| **T04** | ESP32 | Changement de canal alpha ↔ stable (ESP32) |
| **T05** | Raspberry Pi | Changement de canal alpha ↔ stable (Raspberry Pi) |
| **T06** | ESP32 / Pi / Toolkit | Les avis de mise à jour restent dans le canal alpha |
| **T07** | ESP32 | Les pages web reviennent après une mise à jour du firmware |
| **T08** | ESP32 | Les réglages survivent aux redémarrages |
| **T09** | ESP32 | Changer de réseau Wi-Fi |
| **T10** | ESP32 / Pi / Toolkit | GIF du jeu pendant la partie |
| **T11** | Toolkit | Personnalisation des logos |
| **T12** | Toolkit | Pastille de cible et Playlist en Wi-Fi |
| **T13** | Toolkit | Onglet Installation (Mode 1) |
| **T14** | Toolkit | Formater une carte SD en FAT32 |
| **T15** | Toolkit | Alerte d'écart de canal dans le toolkit |
| **T16** | Raspberry Pi | Le bouton de mise à jour du Pi reste en alpha |
| **T17** | Toolkit | Thèmes Recalbox (Modes 12 / 13) |

## ⚠️ À savoir

- Attendez **60 secondes** de fonctionnement normal entre deux changements de canal ou mises à jour (une mise à jour qui n'a pas tourné 60 s est annulée par le chargeur de démarrage).
- T14 **efface** la carte SD : utilisez une carte de rechange.
- Les versions alpha peuvent être remplacées plusieurs fois par jour ; les numéros sont dans les badges ci-dessus et dans le [CHANGELOG](CHANGELOG.fr.md).

## 📚 Documents de cette branche

[ALPHA-TESTS](ALPHA-TESTS.fr.md) (plan de test) · [CHANGELOG](CHANGELOG.fr.md) (ce qui change) · [CHANNELS](CHANNELS.md) (stable ou alpha) · [QUICK-START](QUICK-START.fr.md) · [UPGRADING](UPGRADING.fr.md) · [FAQ](FAQ.fr.md) · [RASPBERRY-PI](RASPBERRY-PI.fr.md) · manuel complet : [README stable](https://github.com/shan-aya/RecalBoxDMD/blob/main/README.fr.md)
