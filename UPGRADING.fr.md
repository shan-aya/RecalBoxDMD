# Mettre à jour depuis une version antérieure (v13 → v2.0)

[🇬🇧 English](UPGRADING.md) · 🇫🇷 **Français** · [🇪🇸 Español](UPGRADING.es.md)

Tu utilises déjà RecalBoxDMD (firmware v13 "Raw565 Edition", n'importe quelle version de la boîte à outils PC) ? Voici ce qui change vraiment et ce qu'il y a à faire — la plupart est optionnel, et rien sur ta carte SD ou ta config Recalbox n'a besoin de changer.

## 1. Mets à jour la boîte à outils PC

Récupère la dernière version depuis la [page Releases](https://github.com/shan-aya/RecalBoxDMD/releases) et installe-la par-dessus l'ancienne (ou remplace le `.exe` portable). Tes réglages (thème, langue, IP Recalbox, image de repli enregistrée...) sont conservés automatiquement — ils vivent dans un `RecalBoxDMD_prefs.json` séparé, non touché par la mise à jour.

## 2. Flashe le nouveau firmware

Comme d'habitude — le [Web Installer en un clic](https://shan-aya.github.io/RecalBoxDMD/) (Chrome/Edge) flashe la v2.0 par USB en environ une minute. Pas besoin de cocher "Effacer l'appareil" — un reflash normal conserve ton `config.ini` et tout ce qui est déjà sur la carte SD, y compris le champ `recalbox_ip`, qui identifie désormais le correspondant UDP au lieu du broker MQTT ; rien à changer de ce côté.

## 3. Réinstalle les scripts Recalbox — obligatoire

La liaison temps réel entre Recalbox et le DMD passe en coulisses de **MQTT à UDP** (voir le [Changelog](CHANGELOG.md) pour le pourquoi). Les scripts côté Recalbox (`marquee[...].sh`, `dmd_score[...].sh`, et le reste de `dmd_helpers/`) ont été mis à jour pour parler UDP au lieu de publier sur un broker MQTT — **lance le Mode 9** une fois (ou un Mode 1 complet) depuis la boîte à outils PC pour les réinstaller ; il nettoie aussi automatiquement les anciennes versions de scripts de l'ère MQTT. Rien à configurer : pas de broker, pas de port, pas d'identifiants à saisir — le DMD écoute sur le même `recalbox_ip` qu'il utilisait déjà.

## 4. Tu avais branché quelque chose sur les anciens topics MQTT ?

Le support MQTT a été entièrement retiré du firmware depuis la v209 (2026-09-14) — pas seulement désactivé par un indicateur comme le disait une version antérieure de cette page. `MQTT_ENABLED=true` n'a plus aucun effet : le code de connexion/tâche lui-même a disparu des sources, pas juste désactivé. Si tu avais branché quelque chose d'externe sur les anciens topics MQTT du DMD, il faudrait récupérer ce code depuis l'historique git antérieur à la v209 et recompiler à partir de là. L'UDP est désormais le seul chemin temps réel pris en charge.

## 5. Logos du thème Recalbox (firmware 2.33 et suivants) — optionnel, mais trois étapes si tu les veux

Depuis le firmware **2.33**, le DMD peut afficher les logos de tes consoles dans le style du thème actif dans Recalbox (Midnight, CRT Color, Neoretro…). Rien ne se passe tant que ces trois étapes ne sont pas faites — flasher le firmware seul **ne suffit pas** :

1. **Réinstalle les scripts Recalbox avec le Mode 9** (ou un Mode 1 complet) : le `dmd_udp_resync.py` mis à jour (**v6**) est celui qui indique au DMD le thème utilisé par Recalbox. Avec l'ancien script, le DMD ne connaît jamais le thème et garde ses logos par défaut.
2. **Mets les logos de thème sur la carte SD avec le Mode 12 ou le Mode 13** de la boîte à outils PC — les logos vivent dans `systems/_defaults/_themes/<thème>/` sur la carte SD et n'y sont pas après une simple mise à jour du firmware :
   - **Mode 13** télécharge le paquet de thèmes prêt à l'emploi depuis GitHub (11 thèmes, dont le thème par défaut de Recalbox et recalbox-240p) et copie sur la carte SD ceux que tu coches — le plus rapide ;
   - **Mode 12** (« Gestion des thèmes Recalbox ») liste les thèmes de ta Recalbox, convertit ceux qui manquent ou sont périmés et garde la carte SD à jour.

   Le Mode 1 pose aussi la question des thèmes (il propose ceux trouvés sur ta Recalbox) : un Mode 1 complet couvre donc aussi cette étape. Les thèmes déjà à jour sur la carte SD ne sont pas réécrits.
3. **Vérifie la case** : section *Thème Recalbox* (page Affichage) de la page de config web du DMD (`feat_theme_follow`, cochée par défaut). Décoche-la pour garder les logos par défaut.

Un thème ou un logo absent de la carte SD est sans conséquence : le DMD retombe sur le logo par défaut. Passer du firmware 2.33 au 2.35 ne demande rien de plus qu'un reflash.

**Depuis le firmware 2.38 — la langue et la région sont automatiques.** La carte SD contient maintenant toutes les variantes (thèmes : sous-dossiers `l_<langue>/` et `r_<région>/` ; images par défaut : `_defaults/fr/` et `_defaults/es/`) et le DMD prend la langue et la région du thème de votre Recalbox, même après un changement. Pour en profiter : flashez la **2.38**, réinstallez les scripts Recalbox avec le **Mode 9** (`dmd_udp_resync` v8) et **réinstallez une fois vos thèmes** (Mode 1, 12 ou 13) ainsi que les images par défaut (Mode 1 ou 2) pour que la carte SD reçoive les variantes — la boîte à outils ne demande plus de langue. D'ici là, tout continue de fonctionner avec les logos anglais / US.

**Depuis le firmware 2.42 — tu es prévenu quand les scripts ne sont pas à jour.** La page d'accueil du DMD et la boîte à outils PC (build 10466 et suivants, au démarrage) vérifient aussi les scripts Recalbox et disent quoi faire : lance la boîte à outils PC et fais un **Mode 9**, puis redémarre la Recalbox. Rien n'est lu sur la Recalbox (pas de SSH) : les scripts (`dmd_udp_resync` v11) annoncent leur numéro au DMD, qui l'affiche sur sa page web et le donne à la boîte à outils en Wi-Fi. Les scripts plus anciens n'annoncent rien — une fois la Recalbox allumée depuis une minute et demie, le DMD le constate et demande le Mode 9. Règle simple après toute mise à jour : **firmware → Mode 9 → redémarrer la Recalbox**.

## Ce que tu *n'as pas* besoin de faire

- Re-scraper tes jeux, reconstruire ta carte SD, régénérer un cache, ou toucher à tes playlists — rien de tout ça n'a changé.
- Reconfigurer l'IP Recalbox, le WiFi, ou quoi que ce soit d'autre sur la page de config web — mêmes champs, mêmes valeurs.
- Faire quoi que ce soit au sujet du broker MQTT (Mosquitto) qui tourne toujours sur Recalbox — le DMD ne lui parle simplement plus ; laisse-le tourner ou retire-le, comme tu préfères.

## 6. Firmware 2.44 et suivants — mise à jour par Wi-Fi (une réinstallation USB d'abord)

À partir du firmware **2.44**, le DMD se met à jour tout seul par Wi-Fi : bouton **Mettre à jour maintenant (Wi-Fi)** dans l'avis « nouvelle version disponible » de sa page web, ou **Mode 14** de la boîte à outils PC (build 10567 et suivants). Le DMD télécharge lui-même le nouveau firmware sur GitHub (environ 2 minutes, écran éteint pendant ce temps), vérifie sa taille et son SHA-256 et revient seul à l'ancien firmware en cas de problème ou si le nouveau ne démarre pas.

**Pour passer à la 2.44 depuis un firmware plus ancien, il faut réinstaller une fois par USB** avec le [Web Installer](https://shan-aya.github.io/RecalBoxDMD/) : la répartition de la mémoire flash a changé (deux emplacements de firmware au lieu d'un), ce qui rend possibles les mises à jour suivantes par Wi-Fi. Votre `config.ini` et la carte SD ne sont pas touchés. Ne flashez pas `RecalBoxDMD_v2.0_app.bin` seul sur un DMD qui n'a pas été réinstallé ainsi. Vous compilez vous-même ? Schéma de partition **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)**.

Le port série Bluetooth classique a été retiré dans la 2.44 (désactivé par défaut, il occupait ~660 Ko de flash et ~40 Ko de RAM) : le DMD dispose maintenant de beaucoup plus de mémoire libre en fonctionnement normal. Les lignes `bluetooth_*` restées dans `config.ini` sont simplement ignorées.
