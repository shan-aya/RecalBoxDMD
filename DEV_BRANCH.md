# ⚠️ `dev` branch — testing only, not production

This branch is used to stage features for **real-device testing** before they're merged into `main`. Nothing here is guaranteed to work, compile, or be complete. If you're looking for the stable release, use [`main`](https://github.com/shan-aya/RecalBoxDMD/tree/main).

## Currently on this branch (not on `main`)

### Firmware source code (`RecalBox_DMD.ino`, `web_config.h`, `clock_themes.h`)
`main` only ships pre-built binaries (`binaries/`) — the actual firmware source wasn't published anywhere on GitHub yet. It lives here first.

### 🧪 Testing: slow-flag ("L") per alphabetical subfolder — firmware v77
Ported from the local `dev/slow-flag-per-bucket` branch (forked at firmware v36, never merged) onto the current v76 firmware base.

- **What it changes**: today, a whole *system* (e.g. `mame/`) gets flagged "slow" (shows the loading mask) if its **total** file count crosses a threshold — even if only one alphabetical subfolder (e.g. `mame/S/`) is actually huge and the rest are small. This ports the flag to be computed **per alphabetical bucket** (`A`, `B`, ... `#`) instead of per whole system.
- **Status**: firmware side compiles; **not retested on the current v76 base, not tested on real hardware yet**.
- **⚠️ Incomplete**: the matching PC Toolkit change (`build_systems_cache()` writing the new 4th field to `systems_cache.dat`) does not exist yet anywhere. Without it, the firmware silently falls back to the old aggregate behavior — i.e. **this feature has no visible effect until the Toolkit side is written**.

## Workflow

Features get tested here, on real hardware, before being merged into `main`. See [`CHANGELOG.md`](CHANGELOG.md) for what's already shipped on `main`.
