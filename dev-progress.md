# Dev branch — work in progress log

Living log of what's being worked on in the `dev` branch and why. Updated as local changes get committed. See [`DEV_BRANCH.md`](DEV_BRANCH.md) for the branch's overall purpose, and [`CHANGELOG.md`](CHANGELOG.md) for what's already shipped and stable on `main`.

---

## 🎯 Current goal

Test the **slow-flag ("L") per alphabetical subfolder** feature on real hardware before merging it into `main`.

**Why**: today, a whole *system* (e.g. `mame/`) is flagged "slow" (shows the loading mask on the DMD) if its **total** file count crosses a threshold — even when only one alphabetical subfolder (e.g. `mame/S/`) is actually huge and the rest are small (e.g. `mame/G/`, 15 files). This wastes the mask on systems/games that would actually load fast. The fix computes the flag **per alphabetical bucket** (`A`, `B`, ... `#`) instead of per whole system, so only genuinely slow buckets show the mask.

## 📋 Changes so far

### 2026-08-13 — firmware source published + feature ported (commit `360284d`)
- **Published** `RecalBox_DMD.ino`, `web_config.h`, `clock_themes.h` to GitHub for the first time — they weren't here at all before (only compiled `binaries/`).
- **Ported** the per-bucket flag logic from the local `dev/slow-flag-per-bucket` branch (single commit, forked from firmware v36, never merged) onto the current v76 firmware base → **v77**. 3-way merge was clean; only the changelog header needed manual resolution.
- **Status**: compiles conceptually (based on the original v37 compile check); **not yet re-verified on the v76 base, not yet tested on real hardware**.

### 2026-08-13 — PC Toolkit counterpart ported (commit `<this push>`)
- **Correction to the note below**: the Toolkit side wasn't actually missing — it was already written, just sitting **uncommitted** in the local `dev-slow-flag-per-bucket` worktree (`RecalBoxDMD_tool.py` v29, untracked on that branch's own git history). Committed there as-is.
- **Ported** the same per-bucket logic onto the current v34 Toolkit (which has diverged a lot since — Recalbox-version profiles, Mode 9-11, adjustable `slow_threshold`, etc.) → **v35**. `build_systems_cache()` now writes the 4th field (27-char `L`/`N` string) to `systems_cache.dat`, reusing the existing adjustable `slow_threshold` (v33) instead of the worktree's original hard-coded `800`.
- New helper `_bucket_letter_for_stem()`, same rule as `_alpha_subdir()`.
- **Status**: compiles clean (`py_compile`). **Not yet tested** — no real conversion run, no real SD card, no hardware.

~~### ⚠️ Known gap — blocks the feature from doing anything yet~~ *(resolved above)*
~~The PC Toolkit counterpart is missing entirely...~~

## ⏭️ Next steps

1. ~~Write the PC Toolkit side~~ ✅ done (see above).
2. Run Mode 1/7 against a real ROMs folder, inspect the generated `systems_cache.dat` — confirm the 4th field is well-formed (27 chars, valid `L`/`N`) and matches what the firmware expects.
3. Recompile the firmware against current `web_config.h`/`clock_themes.h` and confirm 0 errors.
4. Test on real hardware: a system with an unevenly-distributed alphabet (e.g. `mame/S/` huge, `mame/G/` small) should now show the mask only when launching a game from the genuinely slow bucket.
5. Once confirmed working on hardware → merge `dev` into `main`.
