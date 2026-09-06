# Upgrading from an earlier version (v12 → v13)

🇬🇧 **English** · [🇫🇷 Français](UPGRADING.fr.md) · [🇪🇸 Español](UPGRADING.es.md)

Already running RecalBoxDMD (firmware v12 or earlier, PC Toolkit older than v6243)? Here's what actually changes and what you need to do — most of it is optional.

## 1. Update the PC Toolkit

Grab the latest release from the [Releases page](https://github.com/shan-aya/RecalBoxDMD/releases) and install it over the old one (or replace the portable `.exe`). Your settings (theme, language, Recalbox IP, saved fallback image...) carry over automatically — they live in a separate `RecalBoxDMD_prefs.json`, untouched by the update.

## 2. Flash the new firmware

Same as always — the [one-click Web Installer](https://shan-aya.github.io/RecalBoxDMD/) (Chrome/Edge) flashes v13 over USB in about a minute. No need to tick "Erase device" this time (that's only for a first install or when coming from a different firmware) — a normal reflash preserves your `config.ini` and everything already on the SD card.

## 3. Regenerate the cache — optional, but recommended

**Nothing breaks if you skip this.** Firmware v13 reads an old-format `systems_cache.dat` just fine — it automatically falls back to the previous per-system "slow" flag when the newer per-bucket data isn't present.

But v13 introduces a more accurate way of flagging "slow" systems for the mask system: instead of one **"L"** flag for an entire system (MAME, FBNeo...), it's now computed **per alphabetical sub-folder**. A system with one huge sub-folder and several small ones no longer has the small ones penalized unnecessarily — you'll see the wait-mask trigger *less often* on collections where that used to happen.

To get this improvement, regenerate the two cache files with the updated PC Toolkit:

- **Fastest** — Advanced tab → **Mode 6** (games cache) and **Mode 7** (systems cache) back to back, pointed at your existing SD card. A couple of minutes even on a big collection, doesn't touch your marquees/images at all.
- **Simplest** — just run **Mode 1** again like a normal update; it rebuilds both caches as part of the full pipeline anyway.

Nothing to configure — the new per-bucket calculation is automatic. The slow-system threshold (Settings tab) also came back down to its original default of **800** (converted files) to match, since it was only raised to 5,000 to compensate for the old per-whole-system calculation; if you'd customized this value in the old defaults' spirit, revisit it.

## 4. Get the new in-game overlays — optional

Hi-Score, Game Info, RetroAchievements and the monthly RB Challenge (see the [README](README.md#in-game-overlays--hi-score-game-info-achievements--rb-challenge)) need their scripts installed on the Recalbox side. Run **Mode 9** once (or a fresh **Mode 1**) — it installs them alongside everything else, and cleans up any older script names automatically. Nothing to configure on the DMD; it starts working the next time you launch a game with data available.

## What you *don't* need to do

- Re-scrape your games, rebuild your SD card from scratch, or re-download the 600-GIF pack — none of that changed.
- Re-create your playlists — they're untouched.
- Manually delete old Recalbox scripts before installing — Mode 9/Mode 1 clean up outdated names on their own.
