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

## Ce que tu *n'as pas* besoin de faire

- Re-scraper tes jeux, reconstruire ta carte SD, régénérer un cache, ou toucher à tes playlists — rien de tout ça n'a changé.
- Reconfigurer l'IP Recalbox, le WiFi, ou quoi que ce soit d'autre sur la page de config web — mêmes champs, mêmes valeurs.
- Faire quoi que ce soit au sujet du broker MQTT (Mosquitto) qui tourne toujours sur Recalbox — le DMD ne lui parle simplement plus ; laisse-le tourner ou retire-le, comme tu préfères.
