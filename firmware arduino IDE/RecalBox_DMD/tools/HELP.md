# 🎮 RecalBoxDMD PC Toolkit — Help

🇬🇧 **English** · 🇫🇷 Français (`HELP.fr.md`) · 🇪🇸 Español (`HELP.es.md`) — the language of this page follows the language chosen in the **Settings** tab.

> This is the **user manual of the PC Toolkit** (the Windows application you are using right now). For the project itself — firmware, web configuration page, Visual Pinball support, hardware — see the [project page on GitHub](https://github.com/shan-aya/RecalBoxDMD) and its [FAQ](https://github.com/shan-aya/RecalBoxDMD/blob/main/FAQ.md).

---

## 📋 Contents

1. [What the Toolkit does](#1-what-the-toolkit-does)
2. [Before you start](#2-before-you-start)
3. [The window at a glance](#3-the-window-at-a-glance)
4. [First run — the easy way (Mode 1)](#4-first-run--the-easy-way-mode-1)
5. [Mode 1 in detail](#5-mode-1-in-detail)
6. [The Advanced tab — Modes 2 to 11](#6-the-advanced-tab--modes-2-to-11)
7. [The Playlist tab](#7-the-playlist-tab)
8. [Settings, Logs and Help tabs](#8-settings-logs-and-help-tabs)
9. [Recalbox scripts (Mode 9)](#9-recalbox-scripts-mode-9)
10. [After the copy — first boot of the DMD](#10-after-the-copy--first-boot-of-the-dmd)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. What the Toolkit does

The DMD is a 128 × 32 LED panel driven by an ESP32. It reads its pictures from a microSD card, in a compact format that needs no decoding on the panel. The Toolkit **builds that SD card for you**:

- it reads your Recalbox's `gamelist.xml` files and takes each game's **marquee / logo** picture (the one you scraped);
- it converts everything to the panel's format (`.raw565` for still images, `.raw565pack` + `.meta` for GIFs) and builds the two index files the firmware uses (`games_cache.bin`, `systems_cache.dat`);
- it downloads the default pictures (system logos, genres, fallback) and, if you want, the free pack of ~600 GIFs for the idle playlists;
- it can set the DMD's WiFi, install the **Recalbox scripts** that link the game you launch to the panel, and copy everything to the SD card.

You do **not** need to know the file formats: the automatic **Mode 1** does everything and asks you a few questions on the way.

---

## 2. Before you start

- **Windows 10 or 11.** The Toolkit is a portable program or an installed one; both work the same.
- **A microSD card of at least 8 GB, formatted FAT32** (the Toolkit checks both and refuses anything else).
- **Your Recalbox ROM folder**, reachable from this PC — a network share (`\\RECALBOX\share\roms`, or your NAS) or a copy on a disk. The systems must have been **scraped** so that each game has a marquee/logo picture (see [Scraping](#scraping--getting-the-marquee-pictures)).
- **Your Recalbox switched on and on the same network** if you want the Toolkit to install the scripts for you (otherwise you can copy them by hand, see [Mode 9](#9-recalbox-scripts-mode-9)).
- **Your WiFi name and password** — a **2.4 GHz** network; the ESP32 cannot use 5 GHz.

### Scraping — getting the marquee pictures

The Toolkit reads the picture declared in each `gamelist.xml`. Where Recalbox stores it depends on your version — pick the matching profile under **Recalbox version** (Main tab or Settings tab), and use the **How to scrape?** button for the exact menu screenshots:

| Profile | Recalbox scraper setting | Folder / tag read |
|---|---|---|
| **10.x** (recommended) | field **SELECT LOGO TYPE** = **CLEAR** | `media/wheels/`, tag `<logo>` |
| **9.x** | thumbnail type = **MARQUEE** | `media/thumbnails/`, tag `<thumbnail>` |
| **legacy** | image type = **LOGO DÉTOURÉ** (Clear Logo) or **MARQUEE** | `media/images/`, tag `<image>` |

The button **Clean folders before scraping** deletes the already-scraped pictures of the selected systems (only those pictures — never the ROMs nor the `gamelist.xml`), which is useful before re-scraping with another setting. It asks for confirmation.

---

## 3. The window at a glance

The window has **six tabs**:

| Tab | What it is for |
|---|---|
| **Main** | The automatic **Mode 1**: choose the ROM folder, the systems, the Recalbox version, and press **START**. |
| **Playlist** | Build your own idle playlists from the GIFs on the SD card or from GIF folders on your PC. |
| **Advanced** | The individual modes 2 to 11, grouped by theme (GitHub downloads, gamelist, image tools, caches, scripts). |
| **Logs** | The full text output of what the Toolkit is doing, with a level filter. |
| **Settings** | Language (French / English / Spanish), colour theme, Recalbox version profile, "slow" threshold. |
| **Help** | This page. |

At the bottom, the **Progress** panel shows the current step with four buttons: **Pause / Resume**, **Skip** (skip the current step) and **Stop**. The number in the window's title bar is the **build number** of your Toolkit — quote it when you ask for help.

A temporary working folder (`sd_card`) is used to prepare everything before the copy to the SD card. When you quit, the Toolkit offers to delete it (tick *Keep the temporary folder* to keep it).

---

## 4. First run — the easy way (Mode 1)

1. Open the **Main** tab.
2. **Choose ROMs folder** — the folder that contains one sub-folder per system (`snes`, `mame`, …). If the ROMs are on a NAS that asks for a login, a **NAS credentials** box appears: enter the user and password.
3. Click **Detect systems (gamelist.xml)**. The list *Systems to process* fills in. Click to select the systems you want (or **Select all**). *If you select nothing, the Toolkit tells you and stops.*
4. Check the **Recalbox version** profile (see [Scraping](#scraping--getting-the-marquee-pictures)).
5. Click **START** and answer the questions (they are all asked up front, so you can then leave the PC working).
6. When it says **Done**, choose **Browse SD card** to see the result, put the card in the DMD and power it on (see [First boot](#10-after-the-copy--first-boot-of-the-dmd)).

---

## 5. Mode 1 in detail

### The questions asked when you press START

In this order:

1. **DMD WiFi (optional)** — pick the **2.4 GHz** network, type the password and press **Verify**: the PC briefly connects to that network to prove the password is right. *Skip (configure later)* is fine: the DMD then offers its own access point on first boot.
   - **Also set a fixed IP for the DMD (advanced)** — only if your router does *not* already give the DMD a fixed address. Do **one** of the two methods (a reservation on the router, **or** this fixed IP), never both. The fields are pre-filled from your PC's network for reference: check them.
2. **Recalbox** — the Toolkit looks for your Recalbox on the network and shows its **IP address** for confirmation (important if several Recalbox units are switched on). *No* / nothing found → type its IP or name by hand; if it cannot be reached you can **re-enter the IP** or choose **Mode 9 later**.
3. **Fallback image** — the picture shown on the panel when a game has none. Yes → choose one of the proposals or import your own (it is resized automatically). No → keeps the current one.
4. **System images language** — French, English or Spanish for the system/genre badges (Favorites, Last played, …).
5. **SD card** — pick the drive (**Refresh** if it does not show up). It must be FAT32 and at least 8 GB.
6. **Free GIF pack** — download the ~600 GIFs (arcade, consoles, computers, pinball, Halloween, Xmas, …) from GitHub for the idle playlists. The download starts immediately in the background.
7. **Custom GIFs** — *Yes* takes you to the **Playlist** tab in a temporary mode: **Add a PC folder…**, **Copy selection**, optionally **Build playlist**, then press the blinking orange **Continue** button to resume.

### What the automatic pipeline then does

1. **Prepares** the working folder and writes the language and the "first boot" flag into the DMD's `config.ini`.
2. **Installs the Recalbox scripts** (see [Mode 9](#9-recalbox-scripts-mode-9)) — first a local copy in `recalbox_userscripts`, then onto the Recalbox itself if it was confirmed. The Recalbox IP is written to `config.ini` so the DMD web page is pre-filled. It then copies the hi-score files (see [Mode 9](#9-recalbox-scripts-mode-9)).
3. **Extracts** each game's marquee picture from the `gamelist.xml` files (a list of missing pictures is saved as `images_manquantes.txt`).
4. **Converts** to 128 × 32 raw format, then removes the original `.png`/`.gif` that were converted.
5. **Builds `games_cache.bin`**, downloads the **`_defaults`** pictures (with your language and fallback image), the **GIF pack** and the **playlists** (a default playlist plus `ALL.txt`), then builds **`systems_cache.dat`**.
6. **Copies to the SD card.** If files already exist you are asked to *overwrite* or *skip*; an interrupted copy can be **resumed** the next time.

Depending on the size of your collection this can take from a few minutes to a long while (a 30,000-game MAME set is the slow case). You can **Pause**, **Skip** a step or **Stop** at any time.

---

## 6. The Advanced tab — Modes 2 to 11

The left column groups the modes in five collapsible categories; the centre shows the options of the chosen mode; **Mode details** explains it. Use these to redo only one part of the job.

| Category | Mode | What it does |
|---|---|---|
| **DOWNLOAD FROM GITHUB** | **2 — `_defaults`** | Extracts the marquees *and* downloads the default pictures (`systems/_defaults`) from GitHub. Asks whether to overwrite existing files. |
| | **11 — 600 GIFs pack** | Downloads the free GIF pack into `gifs/`. |
| **GAMELIST.XML** | **3 — Extraction** | Only extracts the marquee pictures from `gamelist.xml` (choose the ROM folder and the systems). |
| | **8 — Missing images** | Checks which games have **no** picture and writes a report (`mode8_report_*.txt`); a second button compares that report with the working folder and the SD card (`mode8_final_report_*.csv/.txt`). |
| **IMAGES TOOLS** | **4 — PNG/GIF → raw565** | Converts the PNG (→ `.raw565`) and GIF (→ `.raw565pack` + `.meta`) of any folder you choose. |
| | **5 — 128×32** | Only resizes/converts pictures to 128 × 32. |
| | **10 — Fallback image** | Chooses (or resets) the picture shown when nothing else matches. |
| **CACHES** | **6 — `games_cache.bin`** | Rebuilds the game index from the `systems` folder. |
| | **7 — `systems_cache.dat`** | Rebuilds the system index (needs the `systems` folder that contains `_defaults`). |
| **SCRIPTS RECALBOX** | **9 — Install scripts** | Installs / updates the Recalbox scripts (see below). |

Each mode has its own **Choose … folder** button and its own **START** button. Modes 3, 4, 5, 6 and 7 work on the current working folder unless you point them elsewhere.

**Slow-flag threshold** (Settings tab): systems with more converted files than this number are marked *slow*, so the panel shows a default picture while the real one loads. Raise it if your SD card is fast, lower it if it is slow.

---

## 7. The Playlist tab

A **playlist** is the list of GIFs the DMD plays when nothing is happening on the Recalbox (idle / attract mode).

- **SD card** — pick the drive (🔄 *Refresh*). The tab shows the `gifs/` folders and their files.
- **Check a whole folder** to take all its GIFs, or **click its name** to check files one by one (hover a file = animated preview). An **orange** row means a partial selection; the folder being displayed is underlined.
- **Add a PC folder…** imports one or more GIF folders from your computer (several at once are possible). Uncheck the files you do not want, then **Copy selection**.
- **Delete** removes the checked folders/files (permanent, with a confirmation; playlists that used them are updated).
- **Playlist name** + **Build playlist** saves your selection under that name (existing or new).
- **Regenerate playlist cache** (orange button) rebuilds `cache_master_gifs.dat`, a summary of all GIFs on the card (internal use).

The DMD's web page (Playlist page) is where you pick **which** playlist is active.

---

## 8. Settings, Logs and Help tabs

- **Settings** — *Language* (Français / English / Español, applies to the whole application), *Theme* (colour skin of the window), *Recalbox version* (the scraping profile, shared with the Main tab) and the *Slow-flag threshold*. Your choices are remembered.
- **Logs** — everything the Toolkit prints while it works. The **Level** filter shows *All*, *Warnings+Errors* or *Errors* only. When something goes wrong, this is the first place to look, and what to copy when reporting a problem.
- **Help** — this manual. **Open in browser** shows it in your web browser.

---

## 9. Recalbox scripts (Mode 9)

The DMD only shows what the Recalbox tells it. The link is a set of small **scripts** that Recalbox runs on its events (game selected, game started, game ended, screensaver…). **Mode 1 installs them for you**; **Mode 9** installs or **updates** them at any time (do it after every Toolkit update).

**How:** *Advanced* tab → *Scripts Recalbox* → **Mode 9**, type the Recalbox **IP or network name**, press **Install / Update**. The Toolkit copies the files to the Recalbox's `share/userscripts` folder — through the network share, or over SSH automatically if the share is blocked.

**What is installed**

- **Event scripts** (run by themselves): the *marquee* bridge, the *hi-score / game info / RB Challenge* script, the *RetroAchievements* script, and `dmd_vpx_config`, which sets up Visual Pinball's DMD settings **only if** the **Pinball mode (VPX)** option is ticked on the DMD's web page. Their helper files go in `dmd_helpers/`.
- **Manual scripts** (in the Recalbox menu **Userscripts**): *DMD Config Web*, *DMD Brightness +10 % / −10 %*, *DMD Reboot* and *DMD WiFi Recovery*.
- **Hi-score files (`.hi`)**: Mode 1 and Mode 9 also copy about 3000 hi-score files (FBNeo and MAME 0.278) into the Recalbox's `share/saves` folder, so the DMD has scores to show for games you have not played yet. **A `.hi` that already exists on your Recalbox is never overwritten** — only the missing ones are added. The log ends with a line such as *N .hi copied, M already present*. The hi-score tables (JSON) travel with the scripts, in `dmd_helpers/`.

> **Restart EmulationStation** (or reboot the Recalbox) after installing, otherwise the **Userscripts** menu stays greyed out: Recalbox only looks for scripts when it starts.

**If the Recalbox cannot be reached** (switched off, wrong IP…), the Toolkit keeps a ready-to-copy version in the `recalbox_userscripts` folder of the working directory: copy its content to `share/userscripts` on your Recalbox yourself.

---

## 10. After the copy — first boot of the DMD

1. Put the card in the DMD and power it on. It shows its title (`RawEdition v…`) and the panel starts the idle playlist.
2. If you entered the WiFi in Mode 1, the DMD joins your network by itself. Otherwise it opens its own WiFi access point: connect to it and follow the page (choose your network and give the Recalbox IP).
3. Open the DMD's **web configuration page** (its IP is shown on the panel) to set brightness, playlist, clock, and the Recalbox link. The project page explains each option.
4. Start a game on the Recalbox: the panel should switch to that game's marquee.

---

## 11. Troubleshooting

| Problem | What to do |
|---|---|
| **The SD card is not listed / refused** | It must be **FAT32** and **≥ 8 GB**. Click **Refresh**. Reformat as FAT32 if needed (a 64 GB+ card must be formatted with a FAT32 tool). |
| **"No system detected"** | The ROM folder is wrong (choose the one that holds the system folders) or the images folders are missing. For a NAS, enter the **NAS user / password** and click *Detect systems* again. |
| **Many games have no picture** | The scraping profile does not match how you scraped (see [Scraping](#scraping--getting-the-marquee-pictures)) — change the **Recalbox version** and use **How to scrape?**. Mode 8 lists the missing ones. |
| **"Recalbox unreachable"** | Check that it is on and on the same network, and its IP. Retry with the IP, or finish and use **Mode 9** later, or copy `recalbox_userscripts` by hand. Without the scripts the DMD only shows the playlist and the clock. |
| **The Userscripts menu is greyed out** | Restart EmulationStation (or reboot) — see [Mode 9](#9-recalbox-scripts-mode-9). |
| **The DMD does not connect to WiFi** | Use a **2.4 GHz** network and check the password (Mode 1 can verify it). If you set a fixed IP, make sure the router does not also reserve one (use only one method). |
| **The DMD shows nothing when a game starts** | Scripts missing or outdated → run **Mode 9** again, then restart EmulationStation. Check the Recalbox IP on the DMD web page. |
| **The window is cut off / text too small (high-DPI screen)** | The interface follows Windows' display scaling; sign out and back in after changing the scale. The build number in the title bar helps us reproduce a problem. |
| **A copy to the SD card stopped** | Start Mode 1's copy again: the Toolkit offers to **resume** where it stopped. |

**Still stuck?** Copy the content of the **Logs** tab and the build number from the title bar, and open an issue on the [GitHub page](https://github.com/shan-aya/RecalBoxDMD) — or read the [FAQ](https://github.com/shan-aya/RecalBoxDMD/blob/main/FAQ.md) first (USB power, SD card quality, WiFi loops, fixed IP…).
