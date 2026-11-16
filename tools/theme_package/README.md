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

When an Issue appears: switch the alpha Recalbox on, run `python check_recalbox_system_theme.py --host <IP> --build "carte SD/systems/_defaults/_themes" --known "carte SD/systems/_defaults"`
(password via `RB_SSH_PASSWORD` or prompt; needs `pip install paramiko`), review, commit, then `python watch_recalbox_theme_pin.py --update`.
The embedded logos are read from the Recalbox through `RBDMD_EMBEDDED_LOGOS_DIR` (set by the script, see `theme_logos.py`).

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

Quand une Issue apparaît : allumer la Recalbox alpha, lancer `python check_recalbox_system_theme.py --host <IP> --build "carte SD/systems/_defaults/_themes" --known "carte SD/systems/_defaults"`
(mot de passe via `RB_SSH_PASSWORD` ou saisie ; `pip install paramiko`), vérifier, commiter, puis `python watch_recalbox_theme_pin.py --update`.
