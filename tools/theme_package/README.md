# Theme logos package — maintainer tools

Builds and keeps up to date the package of Recalbox theme logos published in
`carte SD/systems/_defaults/_themes/` (converted to 128×32 `.raw565` for the DMD).
Users get it through the PC Toolkit (Mode 1 and the "Recalbox theme logos" window).

| File | Role |
|---|---|
| `theme_logos.py` | Engine (theme-hub catalogue, logo detection, SVG/PNG → `.raw565`, `_index.bin`). |
| `build_theme_package.py` | Builds the package; `--incremental` only rebuilds themes whose hub version changed. |
| `verify_package.py` | Rebuilds everything in a temp folder and compares each theme `rev` with the published manifest. |
| `verify_package_semantics.py` | For every theme, system, language and region: checks that the logo the DMD picks (language variant, region variant, base) is the one EmulationStation would pick (rules read from the theme XML). Run by the workflow before any Pull Request. |
| `requirements.txt` | Pinned versions (the rendering must stay reproducible). |

The GitHub Action `.github/workflows/update-theme-logos.yml` runs the incremental build and opens a Pull Request
(`auto/update-theme-logos`) with the list of new / updated / removed themes. Review the PR, then merge.

> The reference copy of `theme_logos.py` and `build_theme_package.py` lives in the toolkit sources (`master`);
> keep these two files in sync when the engine changes (bump `PIPELINE_VERSION` to force a full rebuild).

## Default theme (`recalbox-next`) — follow each Recalbox alpha

Official Recalbox images take the default theme from a **private** repository (`recalbox-themes-prime`); the public
`recalbox-themes` stays frozen, so the weekly workflow above cannot see new systems / logos (e.g. the new « favorites »).
The theme (by Bounitos) is converted for the DMD from an installed **Recalbox alpha** (Bounitos agreed, 2026-11-16).

| File | Role |
|---|---|
| `watch_recalbox_theme_pin.py` + `.github/workflows/watch-recalbox-theme-pin.yml` | Weekly: reads the public `recalbox-themes.mk` of `recalbox/recalbox` (private version used per branch, listed in `recalbox_theme_pin.json`) and opens an Issue when it changes. |
| `check_recalbox_system_theme.py` | On the maintainer's PC: reads the theme **and** `/recalbox/system/resources/logos` over SSH (read-only), compares with the last processed state, and with `--build` rebuilds only `recalbox-next` in the package (other themes kept). |

When an Issue appears: switch the alpha Recalbox on, run `python check_recalbox_system_theme.py --host <IP> --build "carte SD/systems/_defaults/_themes_rb11" --known "carte SD/systems/_defaults"`
(password via `RB_SSH_PASSWORD` or prompt; needs `pip install paramiko`), review, commit, then `python watch_recalbox_theme_pin.py --update`.
The embedded logos are read from the Recalbox through `RBDMD_EMBEDDED_LOGOS_DIR` (set by the script, see `theme_logos.py`).
**The target is `_themes_rb11`, never `_themes`** (see below). `--force` rebuilds even when the fingerprint is the one already processed.

## Two packages, one per Recalbox profile (since 2026-11-24)

The logos follow the **profile of the Recalbox the DMD is plugged into** (`rb10` = Recalbox 10.x, `rb11` = Recalbox ≥ 11: patron / alpha / 11.0 stable), not the toolkit channel.
A Recalbox 10 user must not receive logos drawn for the alpha (e.g. a different `favorites`): the DMD would not show what the screen shows.

| Folder | Profile | Content | Built from |
|---|---|---|---|
| `_themes/` | `rb10` | every theme, `recalbox-next` as in the public archive (`2026-02-05-febce03`) | the weekly workflow (theme hub) |
| `_themes_rb11/` | `rb11` | **only** the themes that differ (today `recalbox-next`) + its own `manifest.json` | `check_recalbox_system_theme.py --build` on an installed Recalbox 11 |

The toolkit reads `_themes/manifest.json`, then — for an `rb11` target — overlays `_themes_rb11/manifest.json` (each theme keeps the address of its own folder). Missing or unreachable
overlay: the `rb10` package alone, no error. An unknown profile falls back to the toolkit build channel (alpha build → `rb11`, stable build → `rb10`).
Code: `fetch_theme_package_manifest`, `merge_theme_manifests`, `theme_profile_for` in `RecalBoxDMD_tool.py`; tests `PaquetThemesParProfil` in `dev/test_compat.py`.

- Never rebuild `recalbox-next` from a Recalbox into `_themes/`: the weekly workflow and `verify_package.py` expect it to match the public archive (otherwise « version du hub changée » → a Pull Request deleting logos).
- Publication order: **toolkit first** (the one that reads `_themes_rb11`), then the data. Toolkits already installed only read `_themes/` and would lose the new logos.
- When Recalbox 11 becomes stable nothing changes: `rb11` already covers it. When Recalbox 10 stops being supported, `_themes_rb11` can replace `_themes` for `recalbox-next`.

---

# Paquet de logos de thèmes — outils du mainteneur

Construit et met à jour le paquet de logos de thèmes Recalbox publié dans
`carte SD/systems/_defaults/_themes/` (convertis en `.raw565` 128×32 pour le DMD).
Les utilisateurs le récupèrent via la boîte à outils PC (Mode 1 et fenêtre « Logos de thème Recalbox »).

Le workflow `.github/workflows/update-theme-logos.yml` lance la construction incrémentale (seuls les thèmes dont la version
du hub a changé) et ouvre une Pull Request `auto/update-theme-logos` avec la liste des thèmes nouveaux / mis à jour / retirés.
Relis la PR, puis fusionne.

> Les copies de `theme_logos.py` et `build_theme_package.py` doivent rester synchronisées avec celles des sources du toolkit
> (`master`). Augmenter `PIPELINE_VERSION` force une reconstruction complète.

## Thème par défaut (`recalbox-next`) — suivre chaque Recalbox alpha

Les images officielles de Recalbox prennent le thème par défaut dans un dépôt **privé** (`recalbox-themes-prime`) ; le dépôt public
`recalbox-themes` reste figé : le workflow hebdomadaire ci-dessus ne voit donc ni les nouveaux systèmes ni les nouveaux logos
(ex. le nouveau « favoris »). Le thème (de Bounitos, accord de conversion du 2026-11-16) est converti pour le DMD depuis une **Recalbox alpha** installée.

- `watch_recalbox_theme_pin.py` + `watch-recalbox-theme-pin.yml` : chaque lundi, lit le `recalbox-themes.mk` public de `recalbox/recalbox` (version privée par branche,
  notée dans `recalbox_theme_pin.json`) et ouvre une Issue si elle change.
- `check_recalbox_system_theme.py` : sur le PC du mainteneur, lit en SSH (lecture seule) le thème **et** `/recalbox/system/resources/logos`, compare au dernier état traité ;
  avec `--build`, reconstruit seulement `recalbox-next` dans le paquet (les autres thèmes sont conservés).

Quand une Issue apparaît : allumer la Recalbox alpha, lancer `python check_recalbox_system_theme.py --host <IP> --build "carte SD/systems/_defaults/_themes_rb11" --known "carte SD/systems/_defaults"`
(mot de passe via `RB_SSH_PASSWORD` ou saisie ; `pip install paramiko`), vérifier, commiter, puis `python watch_recalbox_theme_pin.py --update`.
**La cible est `_themes_rb11`, jamais `_themes`** (voir ci-dessous). `--force` reconstruit même si l'empreinte est déjà celle traitée.

## Deux paquets, un par profil de Recalbox (depuis le 2026-11-24)

Les logos suivent le **profil de la Recalbox à laquelle le DMD est branché** (`rb10` = Recalbox 10.x, `rb11` = Recalbox ≥ 11 : patron / alpha / 11.0 stable), pas le canal du toolkit.
Un utilisateur de Recalbox 10 ne doit pas recevoir des logos dessinés pour l'alpha (ex. un `favorites` différent) : le DMD n'afficherait pas ce que montre l'écran.

| Dossier | Profil | Contenu | Construit depuis |
|---|---|---|---|
| `_themes/` | `rb10` | tous les thèmes, `recalbox-next` comme dans l'archive publique (`2026-02-05-febce03`) | le workflow hebdomadaire (hub des thèmes) |
| `_themes_rb11/` | `rb11` | **seulement** les thèmes qui diffèrent (aujourd'hui `recalbox-next`) + son propre `manifest.json` | `check_recalbox_system_theme.py --build` sur une Recalbox 11 installée |

Le toolkit lit `_themes/manifest.json` puis, pour une cible `rb11`, y superpose `_themes_rb11/manifest.json` (chaque thème garde l'adresse de son dossier). Surcharge absente ou
injoignable : le paquet `rb10` seul, sans erreur. Profil inconnu : repli sur le canal du build du toolkit (build alpha → `rb11`, build stable → `rb10`).
Code : `fetch_theme_package_manifest`, `merge_theme_manifests`, `theme_profile_for` dans `RecalBoxDMD_tool.py` ; tests `PaquetThemesParProfil` dans `dev/test_compat.py`.

- Ne jamais reconstruire `recalbox-next` depuis une Recalbox dans `_themes/` : le workflow hebdomadaire et `verify_package.py` attendent qu'il soit identique à l'archive publique (sinon « version du hub changée » → une Pull Request qui supprime des logos).
- Ordre de publication : **le toolkit d'abord** (celui qui lit `_themes_rb11`), puis les données. Les toolkits déjà installés ne lisent que `_themes/` et perdraient les nouveaux logos.
- Quand la Recalbox 11 passera en stable, rien ne change : `rb11` la couvre déjà. Quand la Recalbox 10 ne sera plus supportée, `_themes_rb11` pourra remplacer `_themes` pour `recalbox-next`.
