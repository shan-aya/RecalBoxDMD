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
