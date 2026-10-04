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
