# Mettre à jour depuis une version antérieure (v12 → v13)

[🇬🇧 English](UPGRADING.md) · 🇫🇷 **Français** · [🇪🇸 Español](UPGRADING.es.md)

Tu utilises déjà RecalBoxDMD (firmware v12 ou antérieur, boîte à outils PC plus ancienne que v6243) ? Voici ce qui change vraiment et ce qu'il y a à faire — la plupart est optionnel.

## 1. Mettre à jour la boîte à outils PC

Récupère la dernière release depuis la [page Releases](https://github.com/shan-aya/RecalBoxDMD/releases) et installe-la par-dessus l'ancienne (ou remplace l'`.exe` portable). Tes réglages (thème, langue, IP Recalbox, image de secours enregistrée...) sont conservés automatiquement — ils vivent dans un fichier séparé `RecalBoxDMD_prefs.json`, jamais touché par la mise à jour.

## 2. Flasher le nouveau firmware

Comme d'habitude — l'[installateur web en un clic](https://shan-aya.github.io/RecalBoxDMD/) (Chrome/Edge) flashe la v13 en USB en environ une minute. Pas besoin de cocher « Effacer l'appareil » cette fois (c'est réservé à une première installation ou en venant d'un autre firmware) — un reflash normal conserve ton `config.ini` et tout ce qui est déjà sur la carte SD.

## 3. Régénérer le cache — optionnel, mais recommandé

**Rien ne casse si tu sautes cette étape.** Le firmware v13 lit très bien un `systems_cache.dat` à l'ancien format — il retombe automatiquement sur l'ancien flag « lent » par système entier quand les nouvelles données par bucket sont absentes.

Mais la v13 introduit un calcul plus précis des systèmes « lents » pour le système de masque : au lieu d'un seul flag **« L »** pour tout un système (MAME, FBNeo...), il est désormais calculé **par sous-dossier alphabétique**. Un système avec un gros sous-dossier et plusieurs petits ne pénalise plus inutilement les petits — l'écran d'attente se déclenchera *moins souvent* sur les collections où c'était le cas.

Pour profiter de cette amélioration, régénère les deux fichiers de cache avec la boîte à outils PC mise à jour :

- **Le plus rapide** — onglet Avancé → **Mode 6** (cache de jeux) puis **Mode 7** (cache de systèmes) à la suite, pointés sur ta carte SD existante. Quelques minutes même sur une grosse collection, ne touche absolument pas à tes marquees/images.
- **Le plus simple** — relance simplement le **Mode 1** comme une mise à jour normale ; il reconstruit les deux caches de toute façon dans le cadre du pipeline complet.

Rien à configurer — le nouveau calcul par bucket est automatique. Le seuil « systèmes lents » (onglet Paramètres) est aussi revenu à sa valeur par défaut d'origine, **800** (fichiers convertis), pour correspondre — il n'avait été remonté à 5 000 que pour compenser l'ancien calcul par système entier ; si tu avais personnalisé cette valeur dans l'esprit des anciens défauts, reconsidère-la.

## 4. Profiter des nouveaux écrans superposés en jeu — optionnel

Hi-Score, Infos jeu, RetroAchievements et le Challenge RB mensuel (voir le [README](README.fr.md#écrans-superposés-en-jeu--hi-score-infos-jeu-succès--challenge-rb)) ont besoin de leurs scripts installés côté Recalbox. Lance **Mode 9** une fois (ou un nouveau **Mode 1**) — il les installe avec tout le reste, et nettoie automatiquement les anciens noms de scripts. Rien à configurer côté DMD ; ça se met à fonctionner dès le prochain lancement d'un jeu ayant des données disponibles.

## Ce que tu n'as *pas* besoin de faire

- Rescraper tes jeux, reconstruire ta carte SD depuis zéro, ou retélécharger le pack de 600 GIFs — rien de tout ça n'a changé.
- Recréer tes playlists — elles sont intactes.
- Supprimer manuellement les anciens scripts Recalbox avant d'installer — Mode 9/Mode 1 nettoient les anciens noms tout seuls.
