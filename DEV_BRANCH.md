# ⚠️ Branche `dev` — tests uniquement, pas la production

Cette branche sert à préparer des fonctionnalités pour des **tests sur matériel réel** avant leur fusion dans `main`. Rien ici n'est garanti fonctionnel, compilable ou complet. Si vous cherchez la version stable, utilisez [`main`](https://github.com/shan-aya/RecalBoxDMD/tree/main).

## Actuellement sur cette branche (absent de `main`)

### Code source du firmware (`RecalBox_DMD.ino`, `web_config.h`, `clock_themes.h`)
`main` ne propose que les binaires précompilés (`binaries/`) — le code source du firmware n'était encore publié nulle part sur GitHub. Il se trouve ici en premier.

### 🧪 En test : flag lent (« L ») par sous-dossier alphabétique — firmware v77 + outil PC v35
Porté depuis la branche locale `dev/slow-flag-per-bucket` (forkée au firmware v36, jamais fusionnée) sur le firmware v76 actuel, plus la contrepartie côté outil PC (`RecalBoxDMD_tool.py`, v34 → v35).

- **Ce que ça change** : aujourd'hui, un *système* entier (ex. `mame/`) est marqué « lent » (affiche l'écran masque de chargement) si son nombre **total** de fichiers dépasse un seuil — même si un seul sous-dossier alphabétique (ex. `mame/S/`) est réellement volumineux et que les autres sont petits. Le flag est désormais calculé **par sous-dossier alphabétique** (`A`, `B`, ... `#`) plutôt que par système entier.
- **Statut firmware** : compile ; **pas retesté sur la base v76 actuelle, pas testé sur matériel réel**.
- **Statut outil PC** : `build_systems_cache()` écrit désormais le 4e champ (27 caractères `L`/`N`) dans `systems_cache.dat`, en réutilisant le seuil réglable existant (`slow_threshold`). Compile proprement (`py_compile`) ; **pas testé** — aucune conversion réelle, aucune carte SD réelle.
- **Prochaine étape** : générer un vrai `systems_cache.dat` avec l'outil, recompiler le firmware contre `web_config.h`/`clock_themes.h` actuels, puis tester sur matériel réel avant toute fusion dans `main`.

Voir [`dev-progress.md`](dev-progress.md) pour le journal détaillé des changements.

## Fonctionnement

Les fonctionnalités sont testées ici, sur matériel réel, avant d'être fusionnées dans `main`. Voir [`CHANGELOG.md`](CHANGELOG.md) pour ce qui est déjà livré et stable sur `main`.
