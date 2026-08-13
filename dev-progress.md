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

### ⚠️ Known gap — blocks the feature from doing anything yet
The PC Toolkit counterpart is **missing entirely** (checked `main`, not found anywhere): `build_systems_cache()` in `RecalBoxDMD_tool.py` needs to write a new 4th field to `systems_cache.dat` (27 characters, one `L`/`N` per alphabetical bucket). Without it, every `systems_cache.dat` on an SD card only has the old 2-3 fields, so the firmware's new per-bucket parsing always falls back to the old aggregate system-level flag — **no visible behavior change** until this is written.

## ⏭️ Next steps

1. Write the PC Toolkit side (`build_systems_cache()` + a bucket-letter helper, mirroring `bucketLetterForFilename()`/`sysBucketSlowFlag()` on the firmware side).
2. Recompile the firmware against current `web_config.h`/`clock_themes.h` and confirm 0 errors.
3. Test on real hardware: a system with an unevenly-distributed alphabet (e.g. `mame/S/` huge, `mame/G/` small) should now show the mask only when launching a game from the genuinely slow bucket.
4. Once confirmed working on hardware → merge `dev` into `main`.
