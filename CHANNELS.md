# Canaux STABLE / ALPHA — règles de publication

Deux branches publiques, **deux canaux verrouillés par le build** (jamais un réglage de l'utilisateur) :

| | `main` = **STABLE** | `alpha` = **ALPHA** |
|---|---|---|
| Pour | Recalbox stable | Recalbox alpha (11-patron…), testeurs |
| Annonces / alertes | manifeste `docs/install/manifest.json` (Pages) | manifeste de la branche `alpha` (raw), page `docs/install-alpha/` |
| Releases GitHub | normales | **pre-release** |
| Titre sur la dalle | `STABLE v4.28` / `Pi STABLE v3.93` | `ALPHA v4.28` (rouge) / `Pi ALPHA v3.93` (rouge) |
| Titre du toolkit | `build N` | `build N ALPHA` |

Le **numéro** de version ne change jamais de format (`M.mm`, `pi-vM.mm`, build du toolkit) : seul l'affichage porte la mention du canal.

## Où vit le canal (tout est déduit de la branche)

| Élément | Marqueur | Verrouillé par |
|---|---|---|
| Firmware ESP32 | compilation `-DFW_ALPHA=1` (`compile.ps1 -Channel alpha`) ; marqueur `RBDMD_CHAN=<canal>;` dans le binaire | **le build** ; le workflow refuse un binaire du mauvais canal |
| Démon Pi | fichier `raspberry-pi/rpi_daemon/PI_CHANNEL` | workflow `sync-installer-version` |
| Boîte à outils PC | constante `BUILD_CHANNEL` de `RecalBoxDMD_tool.py` | workflow `sync-installer-version` |
| Manifeste | chemin du binaire `…/<branche>/binaries/…` | workflow `sync-installer-version` |

Le workflow `.github/workflows/sync-installer-version.yml` est **identique** sur `main` et `alpha`.

## RÈGLE DE BASCULE : alpha → stable (« renommer alpha en stable »)

Quand l'alpha devient la stable, les mentions ALPHA doivent devenir STABLE. Rien ne se renomme à la main : on **reconstruit**.

1. **Valider** l'alpha (retours des testeurs, essai sur Recalbox stable).
2. **Fusionner le contenu** de `alpha` dans `main` (cherry-pick ou merge). Le workflow est identique, le chemin du binaire du manifeste et les constantes `BUILD_CHANNEL` / `PI_CHANNEL` sont remis automatiquement sur `stable` au push suivant.
3. **Recompiler le firmware sans `-Channel alpha`** (`compile.ps1`) : le titre devient « STABLE », le marqueur `RBDMD_CHAN=stable;`. Pousser les binaires sur `main`. **Un binaire alpha poussé sur `main` fait échouer le workflow** (et inversement).
4. **Reconstruire** la charge utile du Pi et la boîte à outils **depuis `main`** (leurs fichiers de canal sont alors `stable`).
5. Publier les releases (`pi-vM.mm`, toolkit) en **release normale** (pas pre-release) : seul ce cas met à jour le manifeste stable.
6. Annoncer sur Discord dans `#announcements-annonces` (sans étiquette ALPHA).
7. **Repartir** : recréer `alpha` à partir de `main` pour le cycle suivant (`git branch -f alpha main` puis push, avec précaution : c'est un push forcé sur `alpha`).

## À ne jamais faire
- Publier une pre-release sur `main`, ou un binaire/outil de test sur `main`.
- Modifier à la main `BUILD_CHANNEL`, `PI_CHANNEL` ou le chemin `…/main|alpha/binaries/…` du manifeste (réécrits par le workflow).

## Changement de canal par l'utilisateur (page Réglages du DMD / du Pi)

- Le canal du **build** reste verrouillé (titre `STABLE` / `ALPHA`, alertes, manifeste). Les **alertes de mise à jour** (firmware, démon Pi, toolkit) ne portent que sur le canal du build.
- La page Réglages propose en plus **« Passer au canal ALPHA / STABLE »** : le DMD télécharge et installe **le firmware de l'autre canal** (mise à jour forcée, la version cible peut être plus ancienne), puis redémarre. Après la bascule, le DMD *est* un build de l'autre canal.
  - Firmware : `POST /ota/start?chan=alpha|stable` → `config.ini ota_chan=` (consommé au démarrage) ; deux jeux d'adresses **fixes** codés dans le firmware, aucune adresse n'est fournie par le client.
  - Démon Pi : `dmd_update.py --channel alpha|stable` ; le fichier `PI_CHANNEL` est réécrit après l'installation.
- La **boîte à outils** lit le canal du firmware (`fw_channel` de `/load`). En cas d'écart (firmware stable / toolkit alpha, ou l'inverse) elle le signale, propose de télécharger la boîte à outils **du canal du DMD**, supprime l'avis de mise à jour du firmware et désactive le Mode 14 (mise à jour Wi-Fi).

## Contenu des releases GitHub par canal

| | Release **stable** | **Pre-release alpha** |
|---|---|---|
| Boîte à outils PC | `Setup.exe`, `Portable.exe`, `Source.zip` | **`Portable.exe` UNIQUEMENT** (`gh release create … --prerelease`, jamais `--latest`) |
| Démon Pi | `RecalBoxDMD_pi_payload.zip` + `.sha256` (`PI_CHANNEL` = stable) | idem, `PI_CHANNEL` = alpha, **marquée pre-release** |
| Firmware | binaires sur `main` | binaires sur la branche `alpha` |