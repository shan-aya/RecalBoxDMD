# Changelog

History of **RecalBoxDMD — Raw565 Edition**, covering both the **ESP32 firmware** (including its web configuration page) and the **PC Toolkit**, from the very first commit to today. Entries are grouped by date; each bullet is tagged with the part of the project it changes.

🇬🇧 **English** · [🇫🇷 Français](CHANGELOG.fr.md) · [🇪🇸 Español](CHANGELOG.es.md)

This is a curated summary of the project's internal version history (76+ firmware revisions, 58+ web-config revisions, 38+ toolkit revisions, 54+ GUI revisions) — grouped into the milestones that actually matter if you use the project, not a raw dump of every micro-fix.

---

## 2026-08-19 — Removable-drive detection & popup positioning fixes

- **PC Toolkit**: fixed SD-card detection silently failing on recent Windows 11 builds — drive listing relied entirely on `wmic.exe`, which Microsoft removed by default starting with recent Windows 11 releases; a user's SD card was visible in Windows Explorer but never showed up in the toolkit (Mode 1/6/8), with no error message. Detection now goes through PowerShell's `Get-CimInstance` instead, with the old `wmic` call kept only as a last-resort fallback for unusual environments.
- **PC Toolkit**: fixed several popups (end-of-copy dialog, SD-card drive picker, quit-confirmation dialog) appearing off-screen or outside the main window, especially on multi-monitor setups. Root cause was two-fold: the main window never had an explicit launch position (now explicitly centered on the primary screen at startup), and the compiled `.exe`/`.msi` builds lacked a Windows DPI-awareness manifest — present natively when running from Python source, but absent by default from PyInstaller/cx_Freeze output, which could make Windows misreport window coordinates. Popup centering also now clamps to the real monitor Windows reports for the main window, instead of Tk's primary-monitor-only screen size.

## 2026-08-16 — Multi-language system/genre images (FR/ES)

- **PC Toolkit**: the `systems/_defaults` fallback pack (genre badges, pseudo-systems like Favorites/Last Played/Ports/All Games) is now available in **French and Spanish**, 60/60 each — icon kept pixel-identical (vectorized, not just upscaled), only the text re-rendered and translated. Untranslated genres transparently fall back to English, never missing.
- **PC Toolkit**: new **system images language** picker (EN/FR/ES, with a side-by-side comparison preview) in both Mode 1 (auto pipeline) and Mode 2 (Advanced tab, `_defaults`-only download) — `download_defaults()` always grabs the English base set first (guaranteed fallback), then overlays the chosen language's translated files on top.
- **PC Toolkit**: Mode 2 now always offers the fallback-image gallery (closing without picking reverts to the project default) instead of a yes/no prompt gated on "not already set"; the confirmation popups after picking one were also removed (the choice is already visible/applied immediately).
- **PC Toolkit**: fixed a real slowdown bug in `_parallel_download_batch()` — `urlretrieve()` had no timeout, so a single stalled connection inside the 16-thread pool could block its slot indefinitely; a bounded socket timeout is now set for the duration of the batch.
- **Firmware assets**: 15 system/genre logos added to `_defaults` — 10 missing versus the official Recalbox logo set, plus 5 very recent additions from Recalbox's own alpha channel (Cassette Vision, EXL 100, ST-V, Vircon32, and the new **Challenges** pseudo-system).

## 2026-08-13 — Public release prep

- **Docs**: full rewrite of the README in English/French/Spanish — screenshots, real device footage, mode reference, hardware guide.
- **Firmware**: [Web Installer](https://shan-aya.github.io/RecalBoxDMD/) — flash the ESP32 straight from Chrome/Edge, no Arduino IDE.
- **PC Toolkit**: Windows installer (`.exe` via Inno Setup) and `.msi` (via cx_Freeze), plus a one-click `install_and_run.bat` for running from source.

## 2026-08-11 — Live previews, GIF pack, and the Advanced-tab accordion

- **Firmware / Web config**: picking a clock theme or dragging the brightness slider on the web config page now **previews instantly on the physical panel**, before you save.
- **Firmware**: fix for "Resume DMD" being ignored while a clock-theme preview was still playing; boot-time diagnostic logging of the last reset reason.
- **PC Toolkit**: the Advanced tab's 8 flat mode radios were reorganized into **5 collapsible categories** (GitHub downloads / Gamelist / Images / Caches / Scripts); **Mode 10** (set/generate the global fallback image) and **Mode 11** (one-click download of the ~600-GIF pack) added; the slow-system "L" threshold became a user-adjustable Settings-tab value instead of a hard-coded constant.

## 2026-08-09 – 2026-08-10 — Real-device stability pass

- **Firmware**: several fixes found only through direct hardware testing around the slow-system mask and the fast-path game lookup.
- **PC Toolkit**: the flag-"L" threshold work began here (see above), driven by real SD-card speed differences reported by users.

## 2026-08-06 – 2026-08-07 — Heap stability

- **Firmware**: two independent heap-fragmentation fixes (a dedicated playlist-generation step, disabling WiFi auto-reconnect) — no incidents afterward under intensive real-world testing, including a router power-cycle mid-use.

## 2026-08-05 — `dev/tous-txt-filter` merge

- **PC Toolkit**: playlist tooling and GitHub GIF-bank groundwork merged into the main line.

## 2026-08-03 — First-boot flow overhaul

- **Firmware / Web config**: the first-boot / WiFi access-point setup page was substantially reworked based on real first-run testing.
- **PC Toolkit**: matching updates to the fallback-image picker and popups around first-run/reboot messaging.

## 2026-08-01 – 2026-08-02 — The `cache_master_gifs` rewrite

- **Firmware + Web config + PC Toolkit**: three-part rewrite of the GIF-playlist pipeline around `cache_master_gifs.dat`, a master index of every GIF already on the SD card — speeds up folder browsing in the web Media page and playlist building in the PC Toolkit's Playlist tab, and made large-batch uploads through the web page far more reliable (buffer-size tuning, upload serialization to avoid `ERR_INVALID_CHUNKED_ENCODING`).

## 2026-07-26 – 2026-07-29 — Real-device debugging pass

- **Firmware**: heap-usage and MQTT-connection investigations on real hardware; several regressions found and fixed this way.
- **PC Toolkit**: Mode 9 (install Recalbox scripts) hardened after a real SMB/guest-login failure mode was diagnosed on an actual Recalbox.

## 2026-07-22 – 2026-07-23 — Mode 1 pipeline & network detection

- **PC Toolkit**: `detect_recalbox_share()` (NetBIOS auto-detection of `\\RECALBOX\share`) and `resolve_recalbox_ip()`; the install flow for Recalbox scripts was reworked end-to-end after real-device testing.

## 2026-07-20 – 2026-07-21 — Translation audit & scripts installer

- **PC Toolkit**: full FR/EN/ES translation audit with strict key parity across all three languages; **Mode 9** shipped — install the Recalbox userscripts (marquee bridge, WiFi recovery, web-config sync) directly over the Recalbox's network share, replacing an earlier FTP-based approach the target Recalbox didn't actually support.

## 2026-07-14 — 10th clock theme

- **Firmware**: "Level 1‑1" — a scrolling recreation of Super Mario Bros' first level — added as the 10th clock theme.

## 2026-07-13 — Trilingual interface

- **Firmware + Web config + PC Toolkit**: French/English/Spanish added throughout — the DMD's web config page and the Windows toolkit share the same language, pushed to the DMD automatically at the start of Mode 1.

## 2026-07-11 — Fallback images & Recalbox-version awareness

- **PC Toolkit**: custom fallback-image picker (choose what shows when nothing else matches); the **"Recalbox version" selector** (10.x / 9.x / legacy) is introduced, so the toolkit reads the right `gamelist.xml` tag (`<logo>`/`<thumbnail>`/`<image>`) and media folder for your setup.

## 2026-07-10 — The GUI arrives

- **PC Toolkit**: `RecalBoxDMD_GUI.py` v1 — a Tkinter interface wrapping the console tool; resumable SD-card copy after an interruption; steady layout/UX refinement over the following days (Advanced tab, progress panel, SD-card explorer popup).

## 2026-07-08 — The PC Toolkit is born

- **PC Toolkit**: base version of `RecalBoxDMD_tool.py` (console) — `gamelist.xml` extraction, PNG→raw565/GIF→raw565pack conversion, cache building. Mode 8 (missing-image check) shipped on day one.

## 2026-07-02 — The web configuration page is born

- **Firmware / Web config**: first version of the browser-based config page — FR/EN/ES with browser-language auto-detect, tooltips on every field, GIF upload/multi-upload/deletion, automatic playlist regeneration, and the DMD pausing to a status message during SD operations. A dense string of same-day reliability fixes followed: watchdog-timeout avoidance in long SD loops, `mkdir`/`rmdir` workarounds for read-only FAT32 quirks, a persistent floating status message.

## 2026-07-01 — The clock themes arrive

- **Firmware**: `retro_clock` integration — 9 pixel-art clock themes (Super Mario, Tetris, Pac-Man, Space Invaders, Pong, Neon, Matrix, Fire, Rainbow), replacing the old plain digit renderer.

## 2026-06-11 – 2026-06-29 — Early hardening

- **Firmware**: raw565/raw565pack rendering optimizations; alphabetical `A..Z/#` subfolders added specifically to work around FAT32 slowdowns past ~800 files per folder; first multi-style clock with configurable brightness; a playlist-freeze bug fixed.

## 2026-06-10 — Project born: the Raw565 fork

- **Firmware**: forked from [Jamyz's RetroBoxLED](https://github.com/Jamyz/RetroBoxLED). The original PNG/GIF-decoding pipeline is replaced by a custom **raw565**/**raw565pack** format, a **bigram-indexed game cache** (`games_cache.bin`), and the slow-system **"L" mask** — the foundation that lets a 30,000-game MAME fullset display in milliseconds with no black screen between games.

---

*Dates come from the version headers kept at the top of each source file (`RecalBox_DMD.ino`, `web_config.h`, `RecalBoxDMD_tool.py`, `RecalBoxDMD_GUI.py`) — the project's own internal changelog convention, condensed here for readability.*
