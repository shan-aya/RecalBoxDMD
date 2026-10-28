# Changelog

History of **RecalBoxDMD — RawEdition v3.x**, covering both the **ESP32 firmware** (including its web configuration page) and the **PC Toolkit**, from the very first commit to today. Entries are grouped by date; each bullet is tagged with the part of the project it changes.

🇬🇧 **English** · [🇫🇷 Français](CHANGELOG.fr.md) · [🇪🇸 Español](CHANGELOG.es.md)

This is a curated summary of the project's internal version history (185+ firmware revisions, 64+ web-config revisions, 43+ toolkit revisions, 62+ GUI revisions) — grouped into the milestones that actually matter if you use the project, not a raw dump of every micro-fix.

---

## 2026-10-28 (3) — Toolkit 14285: new "Customize" tab (custom game logos), Mode 3 without text cards

**PC toolkit build 14285** (firmware 3.98 and Pi daemon 3.74 unchanged)
- **New "Customize" tab**: replace a game's logo with your own image (PNG/JPG, animated GIF, or a text card). Your choices are stored in `custom/logos.json` on the SD card (with a copy in `%APPDATA%`) and **re-applied automatically after every copy to the SD card**: a new scrape no longer overwrites them. Batch import, zip packages, "All systems" list, LED preview.
- **Logs tab**: Copy and Export buttons (full log), Top/End navigation, grid scrollbars.
- **Mode 3: no more text cards.** Mode 3 extracts the raw images from `gamelist.xml` for DMD GIF Creator; the text cards generated for games without an image were converted like real images, with a catastrophic result. Games without an image are listed in `images_manquantes.txt` again. The other modes keep text cards (Mode 18 regenerates them on their own).
- **Real logos**: the `dice` logo is removed (it showed ATARI CLASSICS; DICE is an emulator, not a console, and the theme's logo is correct). The set is now 136 base logos.

## 2026-10-28 (2) — Raspberry Pi daemon v3.74 and toolkit 14184: real console logos can be sent to the Pi over Wi-Fi

**Raspberry Pi daemon v3.74 · PC toolkit build 14184**
- **Mode 20: sending the real console logos to the Raspberry Pi over Wi-Fi.** After the download, if a Pi (daemon 3.74 or later) answers on the network, the toolkit sends it the `logos-reels` folder directly (167 files, about 4 s): no need to take the SD card out. The daemon reloads its indexes and redraws the logo on screen straight away. No Pi found: a message, and the copy to the SD card as before (the ESP32 stays on SD-card copy).
- **Pi daemon**: new upload type `POST /upload?kind=reels&dir=base|r_us|r_eu|r_jp` (names, folders and sizes checked; temporary file then rename, never a truncated logo) and `POST /reels-refresh`. An older daemon is refused by the toolkit (it would store the files as GIFs).
- Tested on a Raspberry Pi 3B: 167 files sent, SHA-256 identical to the source.

## 2026-10-28 — Firmware v3.98, Raspberry Pi daemon v3.73 and toolkit 14083: real console logos

**Firmware v3.98 · Raspberry Pi daemon v3.73 · PC toolkit build 14083 (Mode 20)**
- **Real console logos**: new *Real console logos* option on the Display page (*Off / All themes / Only the ticked themes*). The DMD then shows the real logo of each console (137 systems, USA / Europe / Japan variants chosen by the region announced by the Recalbox, base = world logo) **before** the theme's region and base logos; the theme's language variant keeps priority and logos missing from the set keep the theme's logo. Works even when the theme folder is not on the SD.
- **The tick list builds itself**: themes installed on the SD (`GET /reels_installed`) + themes the Recalbox has already announced (`reels_seen` in `config.ini`, 24 max) + an option to tick new themes automatically — nothing to maintain when new themes come out.
- **Same behaviour on the Raspberry Pi** (daemon): same `config.ini` keys, same order, same page, same route. Checked on a real Raspberry Pi 3B (daemon built on the Pi, daemon log): it picks the right file (base, then `r_jp/` when the Recalbox announces the Japan region), redraws the logo on screen straight away when the region changes, and ticks the announced theme automatically.
- **Checked on the DMD (ESP32) against a Recalbox**: changing the region of the theme in Recalbox changes the real logo shown on the DMD.
- **Configuration screen (ESP32) fixed**: the background message (the page address) was never set, so “Saving…” left an empty line 2 once it expired; the address now comes back after 5 s.
- **Toolkit Mode 20 — Real console logos** (*Download from GitHub*): downloads only the `logos-reels` folder, so it can be updated on its own; Mode 1 / Mode 13 list it as a pre-ticked item while it is missing or outdated on the SD. Left column of the Advanced tab tightened (516 → 491 px) to fit the fifth mode.
- **Package**: `logos-reels` (137 base logos + `r_us` / `r_eu` / `r_jp`) is an *extra* theme of the package (`EXTRA_THEMES` in `build_theme_package.py` v10): the Monday workflow never deletes it. Logos from ScreenScraper (CC BY-NC-SA 4.0), see the folder's `LISEZMOI.txt`.
- **Plaquette no. 12** (FR/EN/ES).
- **Thanks**: **Mighty Jim** for his support (“a coffee is a coffee!”) and **Beavisx** for the idea of the realistic logos, and thanks for the maple-flavoured coffee 🇨🇦.

## 2026-10-27 (2) — Firmware v3.97, Raspberry Pi daemon v3.72 and toolkit 13982: every DMD announces its hardware, update notices adapted to the Pi

**Firmware v3.97 · Raspberry Pi daemon v3.72 · PC toolkit build 13982**
- **Every DMD announces its hardware** (`platform` field of `/load`: `esp32` or `rpi`). The web pages and the toolkit use it to compare a Raspberry Pi with the published **Pi daemon** version (the manifest's `pi` key) and an ESP32 with the **firmware** version: a Pi is no longer invited to install the ESP32 firmware.
- **Toolkit: Pi notice** — an outdated Raspberry Pi DMD is offered the daemon update (DMD Settings page or Mode 17), not the Web Installer nor Mode 14.
- **Toolkit: the Recalbox scripts notice comes back at every launch** as long as the bundle announced by the DMD is older than the repository's, naming the DMD concerned (it used to be shown only once per published version).

## 2026-10-27 — Recalbox scripts no. 7 (daphne/Singe logos), Raspberry Pi daemon 3.71

**Recalbox scripts** (bundle no. 6 → 7, install with Mode 9)
- **Fix: game logos of the `daphne` system (Singe included) were missing while browsing**, only shown once the game was launched. On this system each game is a folder named `<game>.daphne`, which the `marquee` script (v44) took for a navigation sub-folder and answered with the system logo.

**Raspberry Pi daemon 3.71** (web pages only, same binary as 3.70)
- **Fix: the Settings page offered the ESP32 firmware (3.96) to a Pi** — it now compares the Pi with the `pi` version of the manifest, like the home page badge.

## 2026-10-26 (4) — Documentation: README Diagnostic section, ULTIMATE pack tutorial, plaquette no. 11, thanks

**Documentation** (no firmware, daemon or toolkit change)
- **README (EN/FR/ES): new “Wi-Fi / Recalbox diagnostic” section** — how to start it (BOOT, action 12, web page), what it measures, how to read the 4 panel pages and the web page, typical findings, Raspberry Pi. The web-config pages table gains the **Diagnostic** page, the toolkit modes table gains **Mode 19** and sending the fallback image over Wi-Fi (**Mode 10**); the Playlist section describes sending GIF folders to the Pi over Wi-Fi, the deletion-complete pop-up and the log.
- **New “Step-by-step tutorials” section**: animated marquees and **installing the ULTIMATE GIFS DLC pack** (~11,000 GIFs by eLLuiGi / RpiTeaM), in EN/FR/ES with real screenshots: [docs/tutorials/ultimate-gif-pack.md](docs/tutorials/ultimate-gif-pack.md).
- **Plaquette no. 11** (EN/FR/ES): what's new in the latest versions (`medias/marketing/plaquette_11_nouveautes_*.png`).
- **Thanks**: **Mighty Jim** for his help debugging the diagnostic tool; **Dodoche3826** for his support (“thanks for the cup of coffee ^^”).

## 2026-10-26 (3) — Firmware v3.96, Raspberry Pi daemon v3.70 and PC toolkit build 13881: fallback image over Wi-Fi, preferences saved in the installed version, DMD detection reinforced

**Firmware v3.96 · Raspberry Pi daemon v3.70 · PC toolkit build 13881**
- **Fallback image over Wi-Fi** (ESP32 and Raspberry Pi): in the toolkit's "Fallback image" gallery, the new **"Send the chosen image to the DMD (Wi-Fi)"** button replaces `default.raw565` directly on the DMD, without taking the SD card out. The DMD writes a temporary file, checks it is exactly 8192 bytes, then replaces the old image; an interrupted or wrong-size upload leaves the old image untouched. An older firmware / daemon is detected and nothing is sent.
- **Installed version (Setup)**: the Setup installs into "Program Files", where a normal user cannot write; the preferences file was never created there, so the language, theme, Recalbox address, DMD address and fallback image were not kept between starts. Preferences are now written to `%APPDATA%\RecalBoxDMD` when the exe folder is not writable; the portable version keeps its file next to the exe. The fallback image chosen from the gallery is also remembered when the portable exe starts again (imported images are kept in `%APPDATA%\RecalBoxDMD\default_images`).
- **"DMD: not found" at startup**: the search now also tries the `dmd-marquee.local` name and scans **every** active network card (VPN, WSL, Hyper-V and several cards no longer hide the DMD's network), with longer delays and a second pass; the log explains what was tried. New **"DMD address (IP or name)"** field in the Settings tab, used first.

## 2026-10-26 (2) — Firmware v3.96, Raspberry Pi daemon v3.70: Wi-Fi / Recalbox diagnostic mode

**Firmware v3.96 · Raspberry Pi daemon v3.70**
- **New diagnostic mode** to check the Wi-Fi signal and the link with the Recalbox, started three ways: the new **action 12 of the Button page** ("Wi-Fi / Recalbox diagnostics"), a **short press on the ESP32 BOOT button** (works even without the physical button), or the new **Diagnostics page** of the web configuration (also in the home menu). It loops until you press again.
- **What it measures**, once per second over a sliding window of the last 20 tests: signal strength (dBm) and quality, channel, access point (BSSID), then a PING to the Recalbox with **loss, latency min / average / max and jitter**, the time since the last packet, UDP gaps and, on the ESP32, the **Wi-Fi reconnections with the reason of the last disconnection** and the history of the last 8 connections (which access point the DMD joined, and when). The panel shows 4 pages of 4 s; the web page shows the values, two curves and a **"How to read these values" guide** (thresholds, causes, what to do), and can generate a **text report** to copy or download for a support message.
- **Leaving diagnostic mode**: press again, leave the page, switch tab or close the browser (and, as a safety net, 8 s without a sign of life from the page).
- **Fixed (ESP32)**: the DMD joined the **first** access point that answered instead of the **strongest**. With a mesh network or a repeater it could latch onto a far access point (−75 dBm instead of −52 dBm) and then stop receiving anything while still showing "connected". It now scans all channels and picks the best signal (the scan adds a short delay at each connection).
- **Fixed (ESP32)**: **"Resume DMD"** after a start in configuration mode (first start, or `first_boot=1`) left a **black screen**: the playlist index was never loaded. It is now loaded on resume.
- **Raspberry Pi**: same diagnostic mode (action 12 of the Button page and the same web page). No BOOT button on the Pi; the Wi-Fi reconnection history only exists **while a diagnostic runs**; on Ethernet the panel shows "ETHERNET / NO WIFI".

## 2026-10-26 — PC toolkit build 13579: fallback image remembered, Playlist tab deletion check

**PC toolkit build 13579**
- **Fallback image**: the image picked in the gallery (for example `default_rgbdual2`) was not remembered the next time the portable version started, and the old Recalbox-and-ghosts logo came back. The choice is now found again at every start, imported images are kept in `%APPDATA%\RecalBoxDMD\default_images` and listed in the gallery, and if the saved image has disappeared the log says so and the default visual is applied.
- **Playlist tab**: after "Delete", a "Deletion finished" window shows the result checked on the card; every action of the tab is written to the log; deleting also removes the matching lines from `cache_master_gifs.dat`.

## 2026-10-25 (3) — Firmware v3.89, Raspberry Pi daemon v3.67: "Recalbox sleep" labels clarified; animated marquees tutorial

**Firmware v3.89 · Raspberry Pi daemon v3.67**
- **"Recalbox sleep" section (Display page)**: the labels now say that, **unchecked, the DMD shows the playlist with the widgets and the clock per your settings** (instead of "simple playlist"), for game demos and for video clips alike; checked (default): the marquee of the game being shown. No change in behaviour. ESP32 and Raspberry Pi.
- **New tutorial** (FR/EN/ES, with screenshots): make animated marquees with DMD GIF Creator (AUTO mode) and the toolkit, ending with an SD card full of `.raw565pack` files: [docs/tutorials/animated-marquees.md](docs/tutorials/animated-marquees.md).

## 2026-10-25 (2) — Firmware v3.88, Raspberry Pi daemon v3.66 and toolkit 13377: the DMD resumes by itself when the configuration page is closed, sending GIF folders to the Pi

**Firmware v3.88 · Raspberry Pi daemon v3.66 · PC toolkit build 13377**
- **The DMD no longer stays in configuration mode when the page is closed** (cross, browser closed, crash). The page tells the DMD it is leaving and the DMD resumes 10 s later; a refresh (F5) or moving to another configuration page cancels that. If the browser crashes or the Wi-Fi drops, the DMD resumes after 3 min without a sign of life. No automatic resume during the first boot, on the Wi-Fi access point screen or during a playlist generation; toolkit messages and file uploads count as activity. Same behaviour on ESP32 and Raspberry Pi.
- **Toolkit, Playlist tab: "Send to Pi (Wi-Fi)…" button** (Raspberry Pi only). Sends GIF folders from the computer into the Pi's `gifs/` folder over Wi-Fi, without taking its card out: same folder picking as "Add a PC folder…", confirmation, progress and summary. An ESP32 DMD is refused (copy to its SD card). The Pi's card only accepts ASCII: accents in folder and file names are converted (é becomes e) and the summary says so.

## 2026-10-25 — Firmware v3.87, Raspberry Pi daemon v3.65 and toolkit 13277: download link in the update notices

**Firmware v3.87 · Raspberry Pi daemon v3.65 · PC toolkit build 13277**
- **DMD web page (ESP32 and Pi)**: the "Recalbox scripts" and "PC toolkit" notices now have a **"Download the toolkit"** link next to "Release notes" (latest release page), translated like the rest of the banner.
- **Toolkit**: the new-version notice also shows **"Download the latest version"**, next to "Release notes" and "GitHub".

## 2026-10-24 (3) — Firmware v3.86, Raspberry Pi daemon v3.64 and toolkit 13177: native Recalbox mode, Visual Pinball on the Pi

**Firmware v3.86 · Raspberry Pi daemon v3.64 · PC toolkit build 13177**
- **Native Recalbox mode: back to the playlist when the Recalbox shuts down or stops answering.** The screen used to stay on EmulationStation's last image. The DMD (ESP32 and Pi) now hands the screen back to the playlist when the Recalbox is switched off (a few seconds) or misses two PINGs in a row (about 30 s), and takes EmulationStation's stream back as soon as it returns.
- **Native mode (firmware): the festive boot animation finally plays.** The date was never stored in this mode, so no holiday was detected; it is now (silent time synchronisation). The festive animation plays from the boot that follows the one that stored the date.
- **Visual Pinball on the Pi (normal and native mode): tables now display.** Two causes on the Recalbox side, fixed by the `dmd_vpx_config` script v4 (installed by Mode 9): the DMD address in VPX's configuration could be stale, and Recalbox cleared the DMD plugin option at every table launch. The script takes the address from Recalbox's ZeDMD option (or the DMD that answers) and sets the "external DMD" option back to ZeDMD before each launch; it leaves everything alone if you chose another display. The `marquee` script v55 warns the DMD on shutdown.
- **Toolkit: "GitHub" link** next to "Release notes" in the update notice.
- **Good to know — updating in native Recalbox mode.** With firmware 3.84 or older, updating from the web page fails in native mode (`echec:heure`: the clock is not synchronised). Untick *native Recalbox mode*, **Save & Restart**, run the update, then tick it again. From 3.85 (so 3.86) on, the update works directly.

## 2026-10-24 (2) — PC toolkit build 13077: translated texts

**PC toolkit build 13077**
- **Three texts that were still written in French are now translated** (French, English, Spanish): the retry dialog after a failed copy to the SD card, the error message when the help cannot be opened, and the error title of the themes window. No other change from build 12977.

## 2026-10-24 — Firmware v3.83 and Raspberry Pi daemon v3.63: time zone in "Clock settings", translated update notice, protected boot animations

**Firmware v3.83 · Raspberry Pi daemon v3.63**
- **Time zone in the clock settings.** The Clock page (opened by the **"Clock settings"** button of the Widgets page, formerly "Choose clock") now offers the time zone on its first line; it is the same setting as on the Place page.
- **Translated update notice.** The banner announcing a new version (firmware, scripts, toolkit) stayed in the language the page was loaded in, often French: it now follows the language selector.
- **Protected boot animations.** The `gifs/_boot` folder is hidden from the folder list, the web page can no longer delete it, and only the six known animation files can be sent to it (toolkit Mode 19). Future versions download their animations from the repository's `boot_anim/` folder (`binaries/` stays reserved for the firmware).

## 2026-10-23 (2) — PC toolkit build 12977: Mode 19, boot animations over Wi-Fi

**PC toolkit build 12977**
- **New Mode 19 — Boot animations (Wi-Fi)** ("DMD FIRMWARE" category): sends the default Shan_ayA animation and the five holiday ones to the DMD over Wi-Fi, **ESP32 or Raspberry Pi**, downloaded from GitHub. Each file is checked (size and checksum) after sending; those the DMD already has are not sent again; a button restarts the DMD to see the animation. Holiday animations need ESP32 firmware 3.67 or later (Raspberry Pi 3.62 or later).
- The animations now have their own `boot_anim/` folder on GitHub (`binaries/` stays reserved for the firmware; the old copy is kept for firmwares 3.63 to 3.82). Help updated. No other change from build 12876.

## 2026-10-23 — Firmware v3.82 and Raspberry Pi daemon v3.62: liquid in a tank, Sand 2, lava lamp, boot animation and alerts on the Pi

**Firmware v3.82** (v3.68 to v3.82)
- **Liquid in a tank.** The liquid now sits in a tank (floor and two walls, as wide as the panel) on a triangular bar at the bottom centre: the tank **rocks** to the liquid's maximum angle, with a phase length drawn at start-up (5 to 10 s) and a **speed drawn at each swing** (maximum angle reached then a pause, or a swing cut short). Waves hit the walls and throw **splashes** (up to 48 drops, more numerous and livelier the faster the front) for fluid liquids; thick liquids (syrup, wax, honey) **cling** to the walls (meniscus, slowly draining film).
- **Sand 2 redone** (like a real sand-art frame). Off-screen reserve, fine four-tone layers, a heavy colour that falls in clumps and a light one that drifts like a cloud (weight set per colour and per gap), a lateral current specific to each gap, gaps with a random lifetime (up to 6 at once), and layers that slide when a heavy layer lands on a light one; the panel fills to about 80%.
- **Lava lamp with a hot spot.** Heat starts at a hot spot at the centre of the bottom and spreads outwards: the centre comes alive first, then the activity reaches the sides, with a heat glow in the oil.

**Raspberry Pi daemon v3.62**
- **Same ambiances as the ESP32 DMD**: Sand 2, liquid in a tank, lava lamp with a hot spot (identical code, taken automatically from the firmware).
- **Boot animation and seasons.** The Pi plays the same Shan_ayA animation as the DMD, and its holiday version (Halloween, Christmas, New Year, Valentine's Day, Easter) depending on the date. It ships with the daemon update: nothing to download or set. `GET /boot-anim` and `GET /boot-season` routes, and Mode 19 can send it too.
- **Same status alerts as the DMD**: "no wifi / no recalbox" (network down) and "RecalBox offline" (two PINGs in a row unanswered), blinking for 5 s then back to the playlist; "RecalBox connected" without the smiley, centred on the ink of the letters.

## 2026-10-22 (4) — PC toolkit build 12876: boot animations in Mode 1

**PC toolkit build 12876**
- **Mode 1 copies the boot animations** onto the SD card (`gifs/_boot/`): the default Shan_ayA one and the five holiday ones (Halloween, Christmas, New Year, Valentine's Day, Easter), downloaded from GitHub and checked (size and checksum) after the copy. Best-effort: if GitHub cannot be reached the pipeline goes on and the DMD downloads them by itself (firmware v3.67). Not done for a Raspberry Pi card. No other change from build 12775.

## 2026-10-22 (3) — Firmware v3.67: seasonal boot animations

**Firmware v3.67**
- **Seasonal boot animations.** Around each holiday the DMD plays a themed version of the Shan_ayA boot animation (new colours, decorations, and a heart-shaped pupil for Valentine's Day), then returns to the default logo. **Halloween** (15 Oct – 2 Nov: orange, slit pupil, bats), **Christmas** (10 – 26 Dec: red, snow), **New Year** (27 Dec – 3 Jan: gold, confetti and stars), **Valentine's Day** (7 – 14 Feb: pink, hearts) and **Easter** (from 7 days before to the day after Easter Sunday, computed every year: eggs). The windows surround the date, they are not just the day itself.
- **Nothing to set up.** The DMD remembers the date at each time synchronisation (it has no clock at power-on) and downloads the seasonal files by itself, in one go, from 21 days before a holiday (2 attempts a day, silent if GitHub cannot be reached). If a file is missing the default animation plays. The animation only plays at power-on: a DMD left on continuously switches at its next restart. Mode 1 of the PC toolkit also copies the seasonal files onto a new SD card.
- **Web page.** New diagnostic routes: `GET /boot-season?d=YYYYMMDD` (which animation would play on that date) and `GET /boot-anim?s=<holiday>`.
## 2026-10-22 (2) — Firmware v3.66: a restart no longer cancels the update

**Firmware v3.66**
- **Fixed — an OTA update cancelled by a restart.** After an update, the new version stayed "to be verified" for 60 seconds, and any restart within that delay (the web page's "Save & Restart" button, a button press, a Recalbox command) brought the old version back. A **deliberate** restart now validates the new version as soon as the DMD has been running for at least 20 seconds. Real crashes at startup stay protected: the automatic return to the previous version is kept.
- **Honest update result.** If an older version is running while the last update announced a newer one, the page now shows an explicit failure ("older version in service") instead of a false "succeeded". This second point only applies from a DMD already on 3.66.

## 2026-10-22 — Firmware v3.65: boot animation and status messages

**Firmware v3.65**
- **Boot animation.** At power-on the panel plays a short animation of the **Shan_ayA logo** (the name orbits around the eye, the pupil follows it); the title appears over it for 2.5 s, then the animation runs 2 more seconds before the normal boot goes on. Nothing to install by hand: on the first boot of this firmware, if a WiFi is configured, the DMD **downloads the animation itself** from GitHub (a few seconds with the panel off; checked by SHA-256 and exact size; 2 attempts at most). Mode 1 of the PC toolkit also copies it onto a new SD card (`gifs/_boot/`). Without the file, the classic title is shown exactly as before.
- **Status messages.** The smileys of "Recalbox connected" and "offline" are gone; the French alert "Pas de wifi / ni Recalbox" now reads **"ni wifi / ni recalbox"**; the lines are centred on the letters themselves (a 2-pixel offset on "ni wifi" disappears).
- **Reliability.** A web upload now reports an error when the SD card cuts a write short, instead of answering "OK" with a truncated file; a new `GET /boot-anim` returns the size and checksum of the animation.

## 2026-10-21 (3) — PC toolkit build 12775: Shan_ayA logo and support link

**PC toolkit build 12775**
- The "GUI - Shan_ayA 2026" text at the bottom of the window is replaced by the **☕ Buy me a coffee** link (hover bubble, opens the Buy Me a Coffee page; FR/EN/ES) and the **Shan_ayA logo** is shown on the right, on top of the theme background. No other change from build 12675.

## 2026-10-21 (3) — Firmware v3.60 and Raspberry Pi daemon v3.61: web pages

**Firmware v3.60 · Raspberry Pi daemon v3.61**
- **Web pages.** The notice banner (toolkit, firmware, scripts) now starts below the language selector and the Help link, which hid the end of it on narrow screens; the "Buy me a coffee" link underline is more spaced. No logic change (the daemon binary is identical to 3.60).

## 2026-10-21 (2) — Raspberry Pi daemon v3.60: missing button, page footer

**Raspberry Pi**
- **Missing button or false press.** Same fix as firmware 3.59: the very long press fires on release only, and "restart" and "Wi-Fi recovery" actions coming from the button are ignored during the first 5 minutes after the daemon starts.
- **Web pages.** New home image, and a "© Shan_ayA 2026" credit with a "Buy me a coffee" link at the bottom of every page.
- The sources in the `raspberry-pi/` folder match this version exactly (they also include the web-page update added in 3.59).

## 2026-10-21 — Firmware v3.59: a missing button can no longer restart the DMD

**Firmware v3.59**
- **Fixed — restart loops on a DMD without a physical button.** An SD card copied from a DMD *with* a button (option on, very long press = restart) into a DMD *without* one looped on restarts: the floating button pin goes low during the "RecalBox connected" screen. The very long press now fires on **release**, the press state is cleared when the pin is stuck, and a **loop breaker** ignores the button for the whole boot that follows a button-triggered restart less than 5 minutes after boot. An OTA update is no longer cancelled by this false press.
- **Web pages.** New home image, and a "© Shan_ayA 2026" credit with a "Buy me a coffee" link at the bottom of every page (under the intro image on the home page).
## 2026-10-20 (2) — Raspberry Pi daemon v3.59: update from the web page

**Raspberry Pi**
- **Daemon update from the web page** (*Settings* page): a notice when a newer version is published (`pi-v…` release), an **Update now** button; download from GitHub, SHA-256 checked, install, service restart, **automatic rollback to the previous version** if it fails to start. `config.ini` and the card content are never touched. A Pi on 3.58 goes to 3.59 once through Mode 17.
## 2026-10-20 — Raspberry Pi version (daemon pi-v3.58)

**Raspberry Pi**
- **The DMD on a Raspberry Pi 3B** (validated) with two P5 panels: same protocol, same Recalbox scripts, same SD card, same web pages, widgets, logo themes, clock, scores, Pinball mode and Recalbox native mode as the ESP32. C++ daemon under **GPL-2.0+** (linked with rpi-rgb-led-matrix), sources in `raspberry-pi/`, binary and payload in the `pi-v3.58` release. Pi 4 / 400 / CM4 / Pi 2: `gpio_slowdown` is set automatically from the model, **untested**. See [RASPBERRY-PI.md](RASPBERRY-PI.md).

**PC toolkit build 12675**
- **Raspberry Pi target**: all-in-one **Mode 1** from a blank card, **Mode 16** (prepare the card, replaces Raspberry Pi Imager), **Mode 17** (install / update). The Pi software is downloaded from GitHub (SHA-256 checked).
- **Mode 18 — Text cards only**: regenerates only the text cards of games without a picture. Games without a logo get a text card (clean style on a dark background); non-games are excluded.
- **Conversion faithful to the original**: pixel-perfect, falling back to the exact scale when the integer divisor shrinks the logo by more than 3 %. Unreadable scrapes are repaired or reported.
- Recalbox profile **"11.x-10.x"** (formerly "10.x"). Warning before conversion: the description shown on the DMD is in the language of the Recalbox scrape. **Refresh** buttons for SD card detection, an SD copy panel that is no longer cropped, a Stop button for the copy, and a popup for serious errors.

## 2026-10-20 — Firmware v3.58: bigger overlay messages of identical size, "RecalBox connected" blinks

**Firmware v3.58**
- **Overlay messages.** Every message drawn over the picture (RecalBox connected, offline, no Wi-Fi, RetroAchievements alerts…) now has the same size in French, English and Spanish, as large as the panel allows (reduced only if a language would overflow).
- **"RecalBox connected".** The message stays 5 seconds over the default image, **blinking**, then the logo stays alone.
- **Default settings.** **Pinball mode (VPX)** and **RetroAchievements alerts** are ticked by default, like the Raspberry Pi version (a `config.ini` without the key = enabled; `feat_vpinball_dmd=0` / `feat_ra_window=0` turns them off).
## 2026-10-18 (2) — Firmware v3.56: "Update" badge on the Settings button

**Firmware v3.56**
- **"⬆ Update" badge.** When a newer firmware or Recalbox script bundle is published, an orange badge now appears on the **Settings** button of the Home page and on the Settings link of every page's menu (in French, English or Spanish), in addition to the update bar. It does not depend on how recently the page checked: the bar still has its 5-minute check from v3.55.
## 2026-10-18 — Firmware v3.55: update notices appear on every page right away

**Firmware v3.55**
- **Fixed — no update alert on the Home page.** The update bar shown on every page (firmware, Recalbox scripts, newer PC toolkit) only re-read the published version every 6 hours, so a new firmware was announced on the *Settings* page (which checks every time) but not on the others. The bar now re-checks every 5 minutes.
## 2026-10-17 (2) — Firmware v3.54 and PC toolkit build 12572: update notices no longer ask for a Mode 9

**Firmware v3.54**
- The firmware update notice (*Settings* page) no longer tells you to run a **Mode 9** after each update — the Recalbox scripts do not change with every firmware. The Mode 9 instruction now only appears in the **Recalbox scripts** notice, shown when a newer script bundle is published. That notice is now independent of the firmware notice (it used to be hidden while a newer firmware was announced).

**PC toolkit build 12572**
- Same change in the toolkit's update window: the "also run a Mode 9" sentence is no longer added to every firmware notice; only the scripts notice carries it.
## 2026-10-17 — Firmware v3.53 and PC toolkit build 12472: 10 countdown events, update notices with release notes

**Firmware v3.53**
- **Countdown: 10 events** (instead of 5), each with an **Active** tick to pause it without deleting it, and names up to 40 characters: a name longer than 14 characters now scrolls in the widget. An incomplete row (name without date, date without name, year outside 2000–2100) raises an alert instead of being silently ignored.
- **Fixed — the background did not follow the playlist.** Choosing a playlist could leave the background on *Widgets only* when a *Widgets* page was still open in another tab (it erased the choice). A page now only writes what it changed itself.
- **Fixed — same temperature for every place in the weather preview.** The kept weather values were not tied to their place, and the preview used the saved place instead of the one typed on the *Place* page. Both fixed.
- **Home and Settings.** A Home page opened by typing the address now puts the DMD in configuration mode (the browser's speculative request was ignored and the follow-up call used the wrong method). The Home page gets *Save & Restart* and *Resume DMD*; Settings gets *Resume DMD*.
- **Update notices.** Firmware and scripts notices have a **Release notes** link; hover it to read the latest changelog entries in a bubble. The web pages also show a dismissible notice when a newer PC toolkit build is published.

**PC toolkit build 12472**
- All update notices (toolkit, firmware, Recalbox scripts, quote themes) now use one window with named buttons, a **Release notes** link and the latest changelog entries in a bubble on hover. Built-in help updated.
## 2026-10-15 — Firmware v3.52 and PC toolkit build 12372: fresh install, widgets by default, playlist ⇄ background

**Firmware v3.52**
- **Fresh install fixed.** A missing `config.ini` is now created (the AP page used to say "OK" and loop forever without writing anything); a clear error is shown if the SD card is missing or read-only; the Wi-Fi network list is no longer empty on the first press.
- **First start.** The web configuration is always required. Defaults proposed when no playlist and no widget is set: **widgets only + random clock** (durations: clock 60 s, date 10, ephemeris 15, countdown 10, weather 15).
- **Boot rule.** After the first start the web configuration is forced only when there is nothing to display: *widgets only* with no widget, or *GIF playlist* with no playlist. The Recalbox IP is no longer required.
- **Playlist ⇄ background.** Choosing a playlist switches the background to *GIF playlist*; choosing *Widgets only* deselects the playlist (remembered); switching back reactivates it (the DMD restarts to load it). *Widgets only* is refused when no widget is selected.
- **Sub-menus keep your settings.** Place, events, clock colours and widgets keep unsaved changes; **one Save from any page saves everything**.
- Weather / ephemeris preview without a place offers to choose one; clicking a colour turns *Auto* off; the brightness preview no longer changes the saved setting; the Wi-Fi page says *MQTT/UDP*; the first-connection help window is rewritten.

**PC toolkit build 12372**
- **Hover previews** in the *Download from GitHub* menu (Modes 2, 11, 13, 15); **Mode 15 reports** what it copied; the **Quit button** is no longer pushed out of view in the Advanced tab; built-in help updated.

## 2026-10-15 — PC toolkit, build 12072: quote themes (Mode 15), several Recalbox, new asset names

- **Mode 15 — ready-made quote themes** (French, English, Spanish): 15 themes of 50 entries published on GitHub, listed with their state on your SD card (to install / up to date / update available / modified by you), installed on demand with a checksum check; an edited file is backed up before being replaced and nothing is deleted. **Mode 1** offers it with a preview of each theme; the toolkit announces new revisions of the pack at launch.
- Modes 1 and 9 accept **several Recalbox** (up to 4) for the firmware 3.x automatic switch.
- Built-in help updated (Modes 1 and 15, FR/EN/ES); the Mode 14 and update-choice windows were unreadable in the dark theme (fixed).
- Release assets renamed `RecalBoxDMD_Toolkit_<build>_Setup.exe` / `_Portable.exe` / `_Source.zip`; firmware binaries are now published without a version in their name (`binaries/RecalBoxDMD_app.bin`, `RecalBoxDMD_merged.bin`).

## 2026-10-15 — Firmware v3.50: widgets, ambiances, new clocks, several Recalbox, Settings page, physical button

**Added**
- **Widgets** (page *Widgets*): date, ephemeris, countdown, weather, ambiance (8 styles), quotes & quizzes and three Recalbox widgets (challenge, rewards, health), mixed with the GIF playlist — *off / in place of the clock / in addition*, trigger every N GIFs or N minutes, or **widgets only** (no GIF). A **Preview** button plays any widget at once.
- **Sky** that follows the real sunrise/sunset of your location and the live weather behind the date, weather, ephemeris and countdown widgets.
- **5 new animated clocks**: Plasma, Life, Stars, Rain, Lava.
- **Quotes & quizzes**: 4 line formats incl. a 3-choice quiz (countdown, right answer stays on screen), create / edit / import / delete themes from the web page, illustrated backgrounds (embedded in the firmware, written to the SD card if missing).
- **Recalbox widgets** fed by `dmd_rbwidgets` (Recalbox script, UDP `CMD=rbw` every 2 min), each with an illustrated background; rewards stay 8 s per page, default durations 15 / **30** / 15 s.
- **Several Recalbox, one DMD**: up to 4 IPs (`recalbox_ips`, `recalbox_target`), automatic switch to the first that answers, packets from the other listed Recalbox ignored; PC Toolkit Modes 1 and 9 take several IPs.
- **Physical button** (GPIO 21): short / double / long / very-long press, each mapped to an action from the *Button* page.
- **⚙ Settings page** (right after *Home* in the menu): brightness, silent boot, Pinball mode, RetroAchievements alerts, native Recalbox mode and the firmware version with its *Update now* (OTA) button, moved out of the home page — which is now a **pure menu**.
- **Update banner at the top of every page**: a newer firmware (link to *Settings*) and / or a newer **Recalbox-scripts bundle** (link to the PC Toolkit, which installs them: Mode 9) are announced whichever page you open. Checked by your browser — the DMD needs no Internet access — at most every 6 hours.
- **Quiz**: a long answer now wraps over **2 lines** instead of being cut.
- **Ready-made quote themes** (toolkit **Mode 15**): 15 themes of 50 entries — famous quotes, fortune cookie, jokes, riddles, retro-gaming quiz — in **French, English and Spanish**, published on GitHub (`widgets/quotes/`). Mode 15 lists them with their state on your SD card (to install / up to date / update available / modified by you), installs the ones you tick (checksum verified, a file you edited is backed up before being replaced, nothing is ever deleted), and the toolkit tells you at launch when a new revision is published. **Mode 1** now offers it with a preview of each theme.

**Changed**
- Web pages are sent with a bounded stall delay (2 s) instead of freezing the main loop for up to 19 s on a slow client.
- The *Statistics* widget was removed.
- The web pages share their style, help, translations and a request queue through two cached files (`/cfg.css`, `/cfg.js`): pages are 20–60 % lighter and the browser asks the DMD **one request at a time**.
- A web page requested while a widget or the clock is running **stops it first and frees its memory, then is served** — the web config mode unloads what it can before opening a page.
- Folder and playlist lists load **about 16× faster** (names read straight from the SD directory).
- The *Continue* shortcut of the home page was removed.

**Fixed**
- **`config.ini` reliability**: atomic writes with boot-time repair; saves verified before answering *OK*; empty Wi-Fi / playlist fields refused; Wi-Fi settings applied only by a loaded *Wi-Fi* page (stale browser drafts could switch Wi-Fi and the static IP off); **widgets-only mode, trigger, order and extras count were never written** to `config.ini` and came back to defaults at each reboot.
- Sky widgets skipped with « sky impossible (heap) » after a save: the allocation now retries with thinner bands.
- **« Unable to load the configuration » and saves that never completed**: static memory had grown past a threshold (≈ 90 KB) with the liquid / sand ambiances, after which the web server could no longer send. Widget buffers are now allocated only while a widget runs (static memory 98 → 88 KB) and freed when the web config opens.
- **Wi-Fi freezing under a burst of page loads** (a page was served from inside a running widget with too little free memory).
- The main task stack is now 12 KB (8 KB overflowed during widgets), and idle browser connections are dropped after 0.6 s instead of 5 s.

## 2026-10-12 — Firmware v2.64: choose which clock themes the random draw may pick

- **Clock: themes allowed in the draw.** When the theme is *Random*, the Clock page shows 10 tick boxes (one per theme): the clock only draws among the ticked themes (at least one). A fixed theme is unchanged; by default every theme is allowed.
- **Never the same theme twice in a row.** The draw now remembers the last theme from one clock appearance to the next (the rule did not apply between two successive appearances).

## 2026-10-11 (2) — Firmware v2.61: Recalbox 11 native mode, Visual Pinball over ZeDMD WiFi (scripts bundle 5)

- **Native Recalbox mode** (new box in the panel's web page, own section under *Display*). Recalbox 11 can drive a DMD by itself: tick the box and the panel behaves like a **ZeDMD WiFi** — Recalbox decides everything that is shown (system and game marquee, Visual Pinball tables, and, in the Recalbox 11 Patreon alpha, hi-scores and RetroAchievements). Untick it and every firmware feature comes back exactly as you had set it (idle playlist, clock, game info, RB Challenge, script commands). Enter the DMD's address in Recalbox's DMD settings (*ZeDMD WiFi*).
- **Visual Pinball on the DMD in native mode.** Recalbox's own Visual Pinball configuration does not pass the ZeDMD WiFi address to the emulator; the script `dmd_vpx_config` (v3, **scripts bundle 5 — run Mode 9 of the toolkit once**) now copies Recalbox's `dmd.zedmd.wifi` setting into it when *external DMD: zedmd* is chosen in Visual Pinball's options in Recalbox. Measured: ~100 images per second from a table.
- **Last image kept.** In native mode the panel redisplays the last image shown while browsing when it restarts (it is not saved during a game).
- **Fixes found while testing on hardware:** reception of the ZeDMD stream could freeze (replaced by a plain socket: switching between marquee and table is now immediate); the panel no longer enters configuration mode when the browser merely *prefetches* the address you are typing; the Wi-Fi page fills in the static IP, gateway, mask and DNS from the current connection and refuses an incomplete static IP (it caused *IP CFG ERR* at reboot); the web page shows its message on the panel again and restores the image afterwards.
- Also in this build: the experimental *ES RetroAchievements alerts* option (normal mode, off by default).

## 2026-10-11 — PC Toolkit build 11469: Recalbox 11 compatibility, sharper and more faithful logos

- **Logos reduced like Recalbox 11 does on its own DMD (render 21).** Recalbox 11 can drive a DMD natively: EmulationStation rasterizes each logo straight to the size of the screen, with exact anti-aliasing. The toolkit now reduces logos the same way (exact area) instead of forcing every LED fully on or fully off: letters are no longer deformed, small text and details keep their grey levels. The whole logo package (11 themes) and the default images were rebuilt.
- **Dark outlines kept dark.** In *Multi-game compilation*, *Board game*, *Rhythm & music*, *Platformer* and *Platform shooter* the outline is dark blue, purple or brown, not black; it was lightened until it matched the letters. It now stays almost black, so the letters stand out.
- **Three logos that were missing.** *recalboxtv*, *online* (Recalbox Online) and *wheel* (steering wheel), present in Recalbox 11.0-patron-1.1 but not in the archive of 11.0-patron-1, are now part of the toolkit and of the package (also as default images).

## 2026-10-11 — PC Toolkit build 11369: thin text readable, Recalbox 11 logos, GitLab fallback

- **Thin strokes no longer broken into dashes (render 19).** Strokes thinner than about half an LED (fine text such as *TECHNOLOGIES*, *MULTIPLE ARCADE MACHINE EMULATOR*, *PlayStation Portable*, outlines such as the PC Engine ellipse) are thickened a little before the reduction. Measured on 311 logos: **88 stray lit LEDs before, 16 now**. Big strokes and solid shapes are untouched.
- **Recalbox 11.0-patron-1 logos.** The logos built into Recalbox now come from 11.0-patron-1: **32 new systems** (Sega Model 1 / 2, Namco System 22, Hyper Neo Geo 64, Jaguar CD, N-Gage, Game.com, Archimedes, FM Towns…), existing logos unchanged. The default images include them.
- **GitLab fallback.** GitLab started answering the version request of Recalbox's own themes with a bot-protection page (HTTP 403, since 10/10); the toolkit now reads the version from the main branch instead, so *recalbox-next* and *recalbox-240p* stay listed in Mode 12 even without a local cache.

## 2026-10-09 — PC Toolkit build 11269: safer copy to the SD card, clearer *Languages | regions* column

- **Copy to the SD card with resume.** In Mode 12, every theme is now **prepared in the Toolkit's working folder, then copied to the SD card**. If the copy is interrupted (card removed, write error, crash), nothing is lost: the **Copy to SD (n)** button — greyed out when nothing is waiting — resumes where it stopped, and the card never shows a half-written theme as up to date (the version marker is written last and the old one removed first). Themes prepared in a previous session are listed when the window opens.
- **Languages | regions column** (was *Variants*): lists **every** language and region a theme offers, the English / US base included (for example *en, fr | us, eu, jp*).
- Docs: the home page and the README now present the **Wi-Fi firmware updates** (no more USB cable since firmware 2.44: web configuration page, phone included, and Toolkit Mode 14).
- **Logo fixes (render 16)**: **Data East Classics** was missing from the default images (it exists under a name with a non-breaking space, the one Recalbox sends) — added; the dark outline of the busy genre logos (*Platformer*, *Platform Shooter*, *Board Game*, *Multi-game Compilation*) is now almost black so the letters stand out; *SNK Classics* no longer has a white spot in the A; *Atari ST* loses the ® mark that showed up as a bright square; **Philips Videopac G7000** — the European variant of the Odyssey 2 is now found (the theme wrote it under another name than the one Recalbox sends), and the other region logos built into Recalbox (for example *Genesis* for the US) are installed too.
- **Sharper logos (render 18)**: the reduction to 128×32 was redone. Measured on 74 logos, **34 % of the edge LEDs were half-lit** (a blurry stroke) with the previous method; they are now **10 %**, while the amount of lit matter stays the same (1.02 × the plain average) and **very thin strokes are kept** instead of disappearing. Each LED now takes the colour of the lit matter of its block, and only the LEDs at the edge of a solid shape are snapped on or off. Bonus: converting a theme is about **10 times faster** (a whole theme in seconds), and the Atari symbol keeps its thin arms. The default images and the 11-theme package were regenerated.

## 2026-10-08 — PC Toolkit builds 10768 → 11169: DMD logos closer to what the Recalbox shows, faster Mode 12

- **Recalbox's own themes** (*recalbox-next*, the default theme, and *recalbox-240p*): the toolkit now also converts the logos **built into Recalbox** (genres, Favorites, Last played, Ports…) with their French variants. Genre logos follow your Recalbox language (*Multijoueur* instead of *Multiplayer*) and region-specific logos (Super Nintendo EU / JP / US) follow your region. Both themes are in the GitHub package, which now holds **11 themes**.
- **Sharper and more readable logos**: pixel-art reduction (Kopf et al. 2013) instead of a plain average; "240p TEST SUITE" readable; Theme Studio variants (`<path.fr>`, `<path.EU>`) detected. A dark **plate** behind white text stays black (Dragon 32, Lynx, Midway / Sammy *CLASSICS*, VIC-20, Textual Adventure, the BBC Micro owl); a dark **outline** stays a visible dark grey instead of turning into a white halo; the **Dashboard-X gradient** is back; true blues stay blue (Vectrex, lutro, SAM Coupé); touch-ups for EXL100, Mitchell and ScummVM.
- **Because the rendering changed (render 15), themes you already converted show as *to update* in Mode 12** — one *Update outdated* is enough.
- **Mode 12 (build 11169)**: a theme whose version equals the one in the GitHub package is now updated by **downloading the ready-made logos** (seconds) instead of converting it (minutes); it converts from your Recalbox only when the version differs, the theme is not in the package or its logos were modified. A **progress bar** shows the work, each theme turns to *UP TO DATE* as soon as it is finished, and the SD-card check message now says that themes installed on the Recalbox are compared with the Recalbox (the reference).
- **Modes 12 / 13**: the theme active on your Recalbox is offered first; one question at a time; faster check. **Mode 9** offers to restart the Recalbox once the scripts are installed; the false *scripts to update* alert is fixed (`dmd_udp_resync` v13 — reinstall the scripts with Mode 9).
- Details of the Mode 12 behaviour: Help tab (*Recalbox themes*).

## 2026-10-05 (2) — Firmware v2.47: new "Language first" option for the theme following

- **New**: web configuration page, "Recalbox theme" section: a new checkbox **Language first (theme without variant)**, **off by default** (nothing changes). When ticked and the theme has no variant for the Recalbox language (for example Midnight in Spanish), the DMD prefers the translated default image (French or Spanish, installed by the toolkit Mode 2) over the theme's base logo, which is often in English. The DMD may then differ from the Recalbox screen. A language variant provided by the theme always wins. It applies at the next system change.
- Update from the web configuration page (**Update now (Wi-Fi)** button) or the PC Toolkit (**Mode 14**). Nothing to reinstall on the SD card.

## 2026-10-05 (1) — Firmware v2.46: the DMD shows what the Recalbox shows

- **Change**: the logo selection order is back to what the Recalbox screen does. For a system, the DMD now uses, in this order: the theme's logo for your **language**, then for your **region**, then the theme's **base logo**. The translated default image (French, Spanish) is only used when the theme has **no logo at all** for that system. This reverses v2.45, which put the translated default image before the theme's base logos and could make the DMD differ from the Recalbox screen.
- Consequence: with a theme that has no variant for your language, the DMD shows the same (usually English) logo as the Recalbox screen. Theme authors can add language variants (`path.fr`, `path.es`, `path.en`) and both screens follow.
- Update from the web configuration page (**Update now (Wi-Fi)** button) or the PC Toolkit (**Mode 14**). Nothing to reinstall on the SD card. The default images in `systems/_defaults/es` and `fr` were also redone (new logos, no more text errors): run **Mode 2** to refresh them.

## 2026-10-04 (9) — Firmware v2.45: translated Favorites / Last played / genre images now win over the theme's English ones

- **Fix**: with a Recalbox language that has translated default images (French, Spanish) and a theme that provides its own *Favorites*, *Last played* or genre logos but **no variant for that language** (for example Midnight in Spanish: only a French variant exists), the DMD showed the theme's English logo even though the translated image was on the SD card. The translated image is now used; consoles keep the theme's logo, and a language variant provided by the theme still takes priority (Midnight in French is unchanged for Favorites and Last played).
- Update from the web configuration page (**Update now (Wi-Fi)** button) or the PC Toolkit (**Mode 14**). Nothing to reinstall on the SD card.

## 2026-10-04 (8) — Firmware v2.44 and PC Toolkit build 10567: update the DMD over Wi-Fi

- **New**: the DMD now updates itself from GitHub. The “new version available” notice on its web page gets an **Update now (Wi-Fi)** button, and the PC Toolkit a new **Mode 14 — DMD firmware (Wi-Fi)**, which finds the DMD, compares versions and starts the update. The DMD downloads the firmware itself (about 2 minutes, screen dark meanwhile, several restarts), checks its size and SHA-256 before installing and goes back to the previous firmware on its own if the download fails or the new one does not start.
- **One USB reinstall is required to get to 2.44** (Web Installer): the flash layout changed (two firmware slots instead of one). Your `config.ini` and the SD card are untouched; after that, no more cable. See [Upgrading](UPGRADING.md). Compiling yourself: Partition Scheme **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)**.
- **Removed**: the classic Bluetooth serial port (off by default, ~660 KB of flash and ~40 KB of RAM). The DMD now has about 58–72 KB of free memory in normal use instead of 12–24 KB.

## 2026-10-04 (7) — Firmware v2.43: Recalbox theme logos now follow on genre systems

- **Fix**: on Recalbox 10.1, genre systems (Sports, Strategy, Action platformer…) and virtual systems such as *Last Played* kept the DMD's default images even with a theme selected. EmulationStation announces them as `genre-<name>` but looks for their theme logo under `auto-<name>`. The DMD now tries `auto-<name>` when the theme has no logo under the announced name (language and region variants included). Nothing to reinstall: the theme packages already contain these logos (Midnight, for instance, ships `auto-sports`, `auto-strategy`…).
- Update from the web configuration page (“new version available” notice) or the web installer.

## 2026-10-04 (6) — Firmware v2.42, script v11: the update check now covers the Recalbox scripts

- **New**: the DMD's web page and the PC Toolkit (build `10466`, at startup) now also tell you when the **Recalbox scripts** are out of date, and what to do: **run the PC Toolkit and do a Mode 9, then restart the Recalbox**. A new firmware notice reminds you of it too.
- How: the scripts (`dmd_udp_resync` v11, reinstall with **Mode 9**) announce their number to the DMD, which shows it on its web page and gives it to the toolkit over Wi-Fi. Nothing is read on the Recalbox itself. Scripts older than v11 announce nothing: the DMD recognises them once the Recalbox has been running for a minute and a half.
- Update the firmware from the web configuration page or the web installer, then run a **Mode 9** once.

## 2026-10-04 (5) — Firmware v2.41 and script v10: “Resume DMD” shows the Recalbox's current state

- **Fix** (script `dmd_udp_resync` v10 — reinstall with **Mode 9**, then restart the Recalbox): after **Resume DMD** the DMD could stay on an empty screen until you moved in the Recalbox. When the Recalbox was showing a **system** (not a game), it answered “playlist”. It now sends the current system, so its logo appears right away.
- **Firmware** (v2.41): closing or leaving the web configuration no longer forces the configuration screen once the DMD has been resumed; a clock preview started after resuming no longer stays frozen on screen.
- Update from the web configuration page (“new version available” notice) or the web installer.

## 2026-10-04 (4) — Firmware v2.40: web configuration keeps your changes across pages

- **Fix**: on the DMD's web configuration, a setting changed on one page (for example **Pinball mode** on the home page) was lost if you pressed Save on another page. Now the changes you make on any page are kept while you move around, and **one Save / Save & Reboot (on any page) saves them all**.
- When you leave with **Resume DMD** or **Reboot** while some changes are not saved, a dialog tells you and offers **Save** / **Leave anyway** (or Cancel). Nothing else interrupts you: moving between pages stays free.
- Update from the web configuration page (“new version available” notice) or the web installer.

## 2026-10-04 (3) — Firmware v2.39 and script v9: theme changes in Recalbox reach the DMD

- **Fix** (script `dmd_udp_resync` v9 — reinstall with **Mode 9**, then restart the Recalbox): changing the theme in Recalbox could leave the DMD on the previous theme's logos. The two messages (language/region, then theme) were sent in the same millisecond and the DMD kept only one.
- **Firmware** (v2.39): theme and language/region messages are now applied the moment they arrive (none can be lost any more); the **last theme, language and region are remembered**, so after a cold start the DMD shows the right logos right away instead of the default ones; the logo on screen is **redrawn immediately** when you change the theme, language or region.
- Update the DMD firmware from the web configuration page (“new version available” notice) or the web installer.

## 2026-10-04 (2) — PC Toolkit build 10365: default (US) console logos fixed

- **Fix**: with the language / region variants, the **base logos** of a theme were taken from the theme's *neutral* path instead of its **US** variant. On themes where the two differ (for example **Midnight**: the neutral Super Nintendo logo is the Japanese *Super Famicom*), the DMD showed the Japanese logo for the default US region. Fixed in the package (**10 themes rebuilt**, a handful of console logos change per theme) and in the toolkit's own conversions (Mode 12).
- **To get it**: reinstall / update your themes once more (Mode 12 flags them as outdated; or Mode 1 / 13).
- **Reminder**: after **Mode 9** (Recalbox scripts), **restart the Recalbox** — the old scripts keep running in memory until then, so the DMD would not follow the Recalbox's language and region.

## 2026-10-04 — Firmware v2.38 and PC Toolkit build 10364: language and region follow your Recalbox, live

- **Firmware** (v2.38) + **Recalbox script `dmd_udp_resync` v8** (reinstall with **Mode 9**): the DMD now uses the **language** and the **theme region** of your Recalbox. The **region** picks the console logos (*Super Famicom* / *Super Nintendo*, *Mega Drive* / *Genesis*…), the **language** picks the translated texts (*Favoris* / *Favorites*…). Change either in Recalbox and the DMD follows within seconds — no reinstall, no reboot. Also works for the default images (English, French, Spanish) when no theme is active.
- **PC Toolkit** (build `10364`): **nothing left to choose** — the language question of Modes 1, 2 and 13 and the language / region lists of Mode 12 are gone. The toolkit installs the English / US base plus **every variant** (themes: `l_<language>/` and `r_<region>/` sub-folders; default images: `_defaults/fr/` and `_defaults/es/`), a few hundred KB more per theme.
- **Theme package** rebuilt in the new format (10 themes). **One-time reinstall needed** to get the variants: Mode 1, 12 or 13 for the themes, Mode 1 or 2 for the default images. Until then everything keeps working with the English / US logos. Older toolkits install the base US logos only.
- Reason: Recalbox's theme region is its own setting (default US, independent of the language), whereas the old package tied the language to the region — nine `recalbox-next` console logos differed from what the Recalbox showed.

## 2026-10-03 (7) — PC Toolkit build 10263: copy-to-SD panel fixed after Mode 13, SD column in Mode 12

- **Fix**: after **Mode 13** (and Modes 2 and 11) launched from the Advanced tab, the **"copy to SD card" panel** was hidden right after the download ended. It now stays visible, so you can copy the files to the SD card straight away.
- **Mode 12** (Recalbox themes management): a new **SD** column next to *Recalbox* shows whether each theme is already on the DMD's SD card (✓ present, — absent, ? if the card is not detected).

## 2026-10-03 (6) — Recalbox's default theme (recalbox-next) now works: PC Toolkit build 10163 + Recalbox script v7

- **Recalbox script `dmd_udp_resync.py` v7** (reinstall with **Mode 9**): if you never changed the theme in Recalbox, the script now announces Recalbox's default theme, **`recalbox-next`**. Before, it announced "no theme" in that case, so most users never saw the theme logos at all.
- **Logo package**: **`recalbox-next`** (default theme) and **`recalbox-240p`** join the package, now **10 themes**. They are redistributed with Recalbox's permission (see `NOTICE.txt` in the package). Install them with Mode 1, 12 or 13 (they are listed there; the default theme is pre-ticked when it is the one active in your Recalbox).
- **PC Toolkit** (build `10163`): can also download and convert the themes that ship with Recalbox straight from GitLab when they are not in the package (fallback for any future ones); the Help tab explains it.
- Nothing to flash: the firmware is unchanged (v2.37).

## 2026-10-03 (5) — PC Toolkit build 10062: the update notice compares with the firmware flashed in your DMD

- **PC Toolkit** (build `10062`): a few seconds after it opens, the toolkit reads the **firmware version of your DMD over Wi-Fi** (it finds the DMD on your network by itself — no USB needed) and tells you if a newer firmware is published, once per published version. The DMD must run firmware **2.37 or later** to report its version; an older one is recognised as "older than v2.37". DMD switched off or on another network: the notice stays informative, as before. It is gentle with the DMD (a single request at startup, its address is remembered). To turn the checks off: `"update_check": false` in `RecalBoxDMD_prefs.json`.
- **Web site**: a green **"New"** tag now marks the latest feature on the home page (Recalbox themes).

## 2026-10-03 (4) — Firmware v2.37 and PC Toolkit build 9961: you are told when a new version is out

- **Firmware** (v2.37): the home page of the web configuration now shows the **firmware version** and, when a newer one is published, a **"New version available"** notice with a link to the Web Installer. The check is made by **your browser** (the DMD needs no Internet access and uses no extra memory); offline, nothing is shown.
- **PC Toolkit** (build `9961`): a few seconds after it opens, the toolkit looks up the latest published toolkit and firmware. If a newer toolkit exists, a box offers to open the download page (once per version). The firmware notice is informative only — the toolkit cannot know which version is flashed in your DMD, so check the version on the DMD's web page. To turn the check off: `"update_check": false` in `RecalBoxDMD_prefs.json`.
- **Web Installer**: the page now shows the version it will install, and that number is updated automatically from the firmware file itself (the firmware carries its own version number).
- **Stay informed**: on GitHub, *Watch → Custom → Releases* (or the releases feed).

## 2026-10-03 (3) — PC Toolkit build 9860: choose the Recalbox themes to install

- **PC Toolkit** (build `9860`): you decide which Recalbox themes go on the DMD.
  - **Mode 1** now asks whether to enable the Recalbox theme following (the new firmware 2.35 option), then lists the themes installed on your Recalbox next to those available on GitHub, with check boxes: **only the ticked themes are copied**. A theme already **up to date on the SD card** is neither downloaded nor rewritten. Recalbox unreachable? You get the full list of available themes instead.
  - **New Mode 13 — Recalbox themes** (*DOWNLOAD FROM GITHUB* category): downloads the themes you choose into the working folder, with the same questions as Mode 1.
  - **New Mode 12 — Themes management** (its own category, launched with START): compares GitHub, the themes installed on your Recalbox and the **DMD SD card**, flags the outdated or never-converted themes, updates them and converts the ones GitHub does not offer. It opens its list on its own and has a *Refresh* button.
  - The SD copy panel now pre-selects the drive named **RECALBOXDMD**; its *Start copy* button stays visible after Mode 13; the Advanced menu is more compact (the Quit button was being cut off). The Help tab documents the new modes.
- **To get the whole feature**: firmware **2.35** (on/off switch for the theme logos) + this toolkit. The package of 8 themes is kept up to date from the Recalbox theme hub by an automatic weekly check on GitHub.

## 2026-10-03 (2) — Firmware v2.35: a switch for the Recalbox theme logos

- **Firmware** (v2.34 and v2.35): new **"Recalbox theme"** section on the web page (*Display & Playlists*) with a **"Follow the Recalbox theme"** checkbox, ticked by default (same behaviour as v2.33). Unticked, the DMD always shows the default logos. The theme announced by the Recalbox is remembered, so ticking the box again applies it at once. The setting is saved in `config.ini` (`feat_theme_follow=`).
- **Fix**: when the theme changes **while an animation is playing**, the theme's logo index (`_index.bin`) is now loaded (it used to be skipped for lack of a large enough free memory block, so a logo missing from the theme took about 1 second to fall back to the default one).
- **Nothing to change on the Recalbox**: the `dmd_udp_resync.py` v6 from v2.33 is still the right one. Installing the theme packs from the PC Toolkit comes with its next version.

## 2026-10-03 — Firmware v2.33: Recalbox theme logos on the DMD

- **Firmware** (v2.33): the DMD can now show the **system logos of the theme selected in Recalbox** (Midnight, Recalbox Next, Dashboard-X...) instead of the default logos. The Recalbox scripts send the theme name; the firmware looks for `/systems/_defaults/_themes/<theme>/<system>.raw565` on the SD card and, if the logo is not there, falls back to the default logo. A small `_index.bin` file per theme avoids slow lookups on missing logos. No theme folder on the SD card = nothing changes.
- **Recalbox scripts**: `dmd_udp_resync.py` v6 (in `tools/recalbox_scripts/dmd_helpers/`) sends the theme to the DMD at start-up, on theme change and after each DMD reboot. **Update this script on your Recalbox** for the feature to work.
- **SD card**: the default logos in `carte SD/systems/_defaults/` were regenerated from Recalbox's own vector (SVG) logos, and a **package of theme logos** (8 themes, English/French/Spanish variants) is available in `carte SD/systems/_defaults/_themes/`: copy the themes you use to the same place on your SD card. The toolkit installation of this package (Mode 1) and the update window will come with the next PC Toolkit build.

## 2026-09-30 — PC Toolkit: SD card recognised by Windows but missing from the toolkit

- **PC Toolkit** (build `8056`): fixed the SD card detection (Modes 1, 6 and 8). Some cards seen by Windows as a normal removable, FAT32 drive were **silently missing** from the drive list: the toolkit asked PowerShell for the drives, and any read failure (accented character in the volume label, slow PowerShell start-up, WMI error) produced an empty list without any message. The output is now read as UTF-8, the wait is longer, and if PowerShell still fails the toolkit **asks Windows directly** (native Win32 call) instead of giving up.
- **If your card is still missing**: click *Refresh* in the drive dialog, then report the letter, file system and size shown by Windows.

## 2026-09-27 — Firmware v2.32: no more reboot loop at startup with a very long playlist

- **Firmware** (v2.32): fixed a **reboot loop right at startup** when the selected playlist is very long (tens of thousands of lines). Rebuilding the playlist cache kept the processor busy long enough to trigger the ESP32 watchdog, which restarted the DMD before the cache was saved — and every restart began again. The rebuild now lets the system breathe regularly: tested with a 23,842-line playlist, the cache is built once (about 50 seconds with the hourglass on screen) and reused instantly on the next startups.
- **If your DMD is stuck in this loop**: flash v2.32 with the Web Installer, or, while waiting, put the SD card in a PC and point `playlist=` in `config.ini` to a shorter playlist.

## 2026-09-26 (2) — Recalbox scripts: two copies of the same script could run at once

- **Recalbox scripts** (`dmd_helpers/singleton_lock.sh`, shared by `marquee`, `dmd_score` and `dmd_achievement`): fixed a rare race in the "only one copy running" lock. During a burst of EmulationStation events (the scripts are started again on every event), a new copy could take the lock of a copy that had just started, and **both kept running** — duplicated or out-of-order displays on the DMD. Measured on a Raspberry Pi Recalbox under heavy load: 1 burst out of 40 before, 0 after. The check is also lighter (no extra process started each time EmulationStation relaunches a script).
- **To update**: run Mode 9 of the PC Toolkit (installs the scripts from GitHub), then restart the Recalbox.

## 2026-09-26 — PC Toolkit: animated game images back in quick navigation, Mode 4 on a flat folder

- **PC Toolkit** (build `8055`): **fixed a regression of build 8053** — the systems cache marked every system as "still image", so the DMD never tried the animated image (`.raw565pack`) of a game during quick navigation. The type of each system (still, animated or both) is again deduced from its **game images**, sub-folders included; images not converted yet count as what they will become (`.png` = still, `.gif` = animated), so Mode 7 still works right after Mode 3. The 2026-09-24 entry below was wrong on this point: the type does not come from `systems/_defaults/`.
- **PC Toolkit**: **Mode 4** now converts images placed directly in the chosen folder (no system sub-folder) — before, it reported "0 PNG, 0 GIF" without any error. They are written to `systems/<folder name>/`.

## 2026-09-24 — PC Toolkit: systems cache fixed, default folders for Modes 2/3/6/7/11

- **PC Toolkit** (build `8053`): fixed **Mode 7** reporting "0 systems found" on a folder whose game images are not converted yet (for example right after Mode 3). The type of each system (still image, animation or both) is now read from its default image in `systems/_defaults/`, exactly like the DMD firmware does — before, it was wrongly deduced from the game images of the system folder. Also applies to Mode 1. The systems cache now lists only the system folders actually present (plus `default`), never more than the DMD can hold.
- **PC Toolkit**: the "slow" flag of the systems cache now also counts images that are not converted yet (a `.png` counts as its future `.raw565`, a `.gif` as its future `.raw565pack` + `.meta`), so a large folder such as `mame/S` is correctly marked slow even before conversion.
- **PC Toolkit**: Modes 2, 3 and 11 open the working folder at the end; Modes 6 and 7 point to the working folder's `systems` by default when it exists (a folder you pick yourself is kept).

## 2026-09-23 (2) — PC Toolkit: Mode 9 layout fixed, script clean-up

- **PC Toolkit** (build `7951`): the Pause / Resume / Skip / Stop buttons of the Progress frame no longer disappear at the bottom of the window during Mode 9 (they were pushed out when the result text appeared). The Progress frame now always keeps its place, the Mode 9 result has a fixed height, and the Mode 9 description is shorter (it now mentions the `.hi` copy).
- **PC Toolkit**: after installing the scripts, the message now asks to **reboot the Recalbox** — restarting only EmulationStation leaves the old scripts running.
- **Recalbox scripts** (`dmd_score` v54, `dmd_achievement` v8): leftover MQTT code removed (the DMD has not used MQTT for a while); the RetroAchievements bridge log is now `dmd_achievement.log`. No change in behaviour. Re-install with Mode 9, then reboot the Recalbox.

## 2026-09-23 — Hi-scores independent of the MAME version, verified MAME tables fixed

- **Recalbox scripts** (`dmd_hiscore_verified.py` v2): fixed the hand-verified hi-score tables **never showing for MAME games** — the lookup used the system name (`mame`) while the table keys carry the MAME version (`mame0278_…`). About 2270 MAME games now show their table when no real `.hi` exists. Re-install the scripts with Mode 9.
- **Recalbox scripts** (`dmd_hiscore_generic.py` v6): the MAME hi-score folder is no longer hard-coded — the script uses the MAME core Recalbox actually runs (`mame.core` in `recalbox.conf`, otherwise the folder MAME wrote to most recently). A future MAME core needs no script update.
- **PC Toolkit** (build `7651`): the MAME `.hi` files are copied into the folder of the MAME core in use on the Recalbox (same rule), instead of a fixed `mame0278` folder. Still never overwrites an existing `.hi`.
- **Data**: in [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/), the MAME files move to `hi/mame/hiscore/` (no version) and a new `hiscore_hi_pack_v2.zip` is used by the Toolkit; the old zip stays for build 7550.

## 2026-09-22 — PC Toolkit copies hi-score files, new hi-score data folder

- **PC Toolkit** (build `7550`): **Mode 1 and Mode 9 now also copy the hi-score files (`.hi`)** — about 3000 files for FBNeo and MAME 0.278 — into `/recalbox/share/saves` on the Recalbox, so the DMD has scores to show for games that were never played. **A `.hi` already present on the Recalbox is never overwritten**; only missing ones are added. Over the network share, with SSH as a fallback. The hi-score tables (JSON) were already installed with the scripts.
- **PC Toolkit**: the Help tab now mentions the `.hi` copy (3 languages).
- **Docs / data**: new folder [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/) — 3161 `.hi` files (FBNeo 2491, MAME 0.278 670), the two hi-score JSON files, a checksum manifest and a README, ready to be handed over to the Recalbox team. The hi-score table `verified_default_scores.json` is updated (3941 entries: 2277 mame0278 + 1664 fbneo); re-install the scripts with Mode 9 to get it.

## 2026-09-20 (2) — Visual Pinball (VPX) mode without reboot, new web config home page, Recalbox standby options

- **Firmware**: new **Pinball mode (VPX)** — a Visual Pinball table launched from Recalbox is shown live on the DMD (ZeDMD-WiFi protocol, DMDUtil plugin on the Recalbox side). **No DMD reboot**: the DMD switches in place when the table starts and back to normal when the game ends (or about 5 s after the last frame). While a table runs, the playlist and the Recalbox screens ("Recalbox connected"…) are suspended so nothing is drawn over it. Off by default; the brightness is restored on exit. Only tested on 128×32 tables so far.
- **Firmware**: to make room for it, the memory budget was reworked — a minimal built-in zlib decompressor (stack-only) replaces the heavier one, buffers are allocated only when the mode is enabled, and the in-memory system/game index is right-sized (300 → 160 entries; a log line reports it if a very large collection hits the limit). Free memory is about 12 KB higher.
- **Web config**: the main menu (home page) now opens with the **Display** frame — brightness, silent boot and the **Pinball mode (VPX)** switch. Its Save button shows a confirmation on line 2 of the DMD. The Display page keeps the in-game overlay options.
- **Firmware**: the DMD boot title now shows the **real firmware version** (e.g. `RawEdition v2.31`) instead of a fixed `v2.0`; the README badge and the Web Installer show the same number.
- **Firmware + Recalbox script**: the Pinball mode option now **authorizes** a new Recalbox script, `dmd_vpx_config`, to check and write the Visual Pinball settings the DMD needs (DMDUtil / ZeDMD WiFi, AlphaDMD) in `VPinballX-configgen.ini` at every Recalbox start and after each game — never while a table is running, and never overwriting an IP you typed. Unticked, the script does nothing. A warning on the option's help bubble (web config) says so. Needs the scripts re-installed with Mode 9.
- **Firmware**: fixed Pinball mode on colorized tables (Serum) — large color updates were partly lost, giving backgrounds drawn over each other and missing texts. The sender splits a message across two UDP packets when it does not fit in 1400 bytes; the DMD now reassembles such messages instead of discarding them. Colorized tables (Diner…) now display correctly.
- **Firmware**: fixed the hi-score / game info / description overlays missing when the DMD is powered on *before* the Recalbox (only a DMD reboot fixed it) — the DMD now re-sends its settings to the Recalbox on the first reply after startup or after a lost connection.
- **Docs**: older tables with segment (alphanumeric) displays need the **AlphaDMD** VPX plugin enabled on the Recalbox side — see the README (Pinball section). Tested on 8 tables: 6 work; two (Black Hole, Farfalla) use display layouts that plugin does not support.
- **Firmware**: fixed the *Recalbox connected* screen showing while the Recalbox is switched off (then replaced by *Recalbox offline*) after *Resume DMD*, the home page Save button or leaving Pinball mode — it now only appears once the Recalbox has really answered.
- **Web config**: a **Home** button now sits first in the menu bar of every configuration page (Display, Playlist, Wi-Fi & BT, Clock, Media).
- **Web config**: new **Recalbox standby** section — during the "game demos" / "game video clips" screensavers, choose between following the game logo (as before, default) or the plain playlist, like the other screensavers.
- **Recalbox scripts** (`marquee` v54, `dmd_score` v53): the standby options above, and an immediate exit from Pinball mode at the end of a game. Re-install the scripts with Mode 9 to get them.
- **Docs**: README (3 languages) documents Pinball mode, the home page and the new `config.ini` keys (`feat_vpinball_dmd`, `feat_demo_follow`, `feat_clip_follow`).
- **PC Toolkit** (build `7449`): the Help tab is now a real user manual (first run, Mode 1 step by step, every Advanced-tab mode, Playlist tab, Recalbox scripts incl. `dmd_vpx_config`, troubleshooting) instead of a copy of the README; no more MQTT mentions.

## 2026-09-20 — DMD IP discovered automatically, PC Toolkit scales with Windows DPI, shuffle image installed, SD label rename restored

- **Recalbox scripts**: fixed the DMD staying stuck on its idle playlist while a game was running — the scripts had the DMD's IP address hard-coded (`192.168.0.51`), so they talked to nothing on any network where the DMD got a different address. They now use the address the DMD actually announces itself from (falls back to the old default if none has been seen yet). Follows a real report from a tester.
- **Firmware**: SD-card label renaming is active again — it had been left disabled after a diagnostic session.
- **PC Toolkit**: the whole interface now scales with the Windows display scaling (125%, 150%...) instead of only the fonts — at 125% on a 4K screen the bottom of the window (the Progress section) used to be cut off. Applied after the next sign-out if the scaling was just changed.
- **PC Toolkit**: the build number is now shown in the window title bar (currently `7349`), so it is easy to tell which version someone is running.
- **PC Toolkit**: Mode 9 now reminds you to restart EmulationStation after installing the user scripts — the *User scripts* menu stays greyed out until EmulationStation rescans its scripts at startup.
- **PC Toolkit**: SD card creation now also installs the shuffle-mode CRT static image (`_shuffle.raw565pack` + `_shuffle.meta`); before, these two files were skipped and the DMD showed an empty screen in shuffle mode. If your SD was created with an earlier build, copy those two files from `carte SD/systems/_defaults/` into `systems/_defaults/` on the card.
## 2026-09-14 — MQTT fully removed, watchdog crash fixed, optional static IP, FAQ page

- **Firmware**: the MQTT subsystem (connection/task code, ~800 lines) is now fully removed from the source — not just disabled by default as of the day before. If you had something wired into the DMD's old MQTT topics, see [UPGRADING.md](UPGRADING.md) for what that means.
- **Firmware**: fixed a false "Recalbox connected" screen that could appear even while Recalbox is powered off — it used to trigger on merely *sending* a UDP hello, with no confirmation it was actually received; now waits for an actual reply first.
- **Firmware**: fixed a real crash — a normal visit to the web config page could occasionally trip the ESP32's hardware watchdog and force a reboot (`WebServer::_parseRequest()` could busy-wait up to 5 seconds with no yield, right at the edge of the platform's own 5-second watchdog window). Confirmed fixed on real hardware: 300 rapid requests against the exact pattern that used to crash it, zero failures.
- **PC Toolkit**: Mode 1's WiFi setup step can now also set a static IP on the DMD (optional, off by default) — includes an explicit warning against configuring both a router-side DHCP reservation *and* a DMD-side static IP at the same time (can cause real connectivity issues), and pre-fills the network fields from this PC's own configuration to cut down on typos.
- **PC Toolkit**: fixed a false "can't reach the Recalbox" error in Mode 1 even with the correct IP entered — the reachability check only tried a raw TCP connection to the SMB port, which some firewall/antivirus rules block for an unrecognized process even though Windows Explorer's own SMB client reaches the exact same share just fine; it now falls back to a real share-access attempt (the same thing Explorer does) before giving up.
- **PC Toolkit**: attempted a fix for the bottom of the interface (the Progress section) disappearing on high-DPI displays (reported by a tester on a 4K screen at 125% scaling) — later reports showed this didn't fully cover it; a follow-up fix is in progress.
- **Docs**: new dedicated [FAQ & Troubleshooting](FAQ.md) page — USB power sensitivity (brightness vs. current draw), ESP32 chip revision notes, microSD card quality, WiFi setup loops, fixed-IP guidance, and more — linked from the Troubleshooting section of the README.

## 2026-09-13 — MQTT → UDP transport: `dev/dmd-udp-transport` merged to master

- **Firmware**: real-time link between Recalbox and the DMD switched from **MQTT to UDP** — the previous transport hit a platform-level wall inside the ESP32's TCP/IP stack (a fixed ~5.7 KB `TCP_SND_BUF`, baked into the precompiled Arduino core, no application-level fix possible) that could stall an MQTT `SUBSCRIBE` for several seconds after a reconnect; UDP has no such handshake to stall on. MQTT support was kept in the firmware at the time, disabled by default, as a rollback path — fully removed the next day, see above.
- **Firmware**: months-long hunt for a historically-reported severe UDP reception freeze (12–90s+) — never reproduced after an intensive instrumentation campaign (active gap-probing, max-per-call timing, sustained real traffic), even though roughly ten real, unrelated bugs surfaced and got fixed along the way (below). Current conclusion: the original freeze reports were most likely a benign side effect of a too-aggressive detection threshold colliding with ordinary UDP packet loss, not an actual reception stall — documented in detail in `DECISIONS.md`.
- **Firmware**: fixed a false "Recalbox offline" alert firing on a single dropped UDP packet (normal, expected for UDP) — now requires two consecutive missed pings before warning.
- **Firmware**: fixed the resync-after-reconnect logic getting stuck in the cached-fallback display mode (`MODE_PNG`) instead of catching up to Recalbox's real state.
- **Firmware**: fixed "Resume DMD" (web config page) always forcing the idle playlist regardless of what Recalbox was actually doing (in-game, demo, gameclip...) — a dead MQTT-era check made the correct branch unreachable since the UDP switch; now resyncs the real state the same way a reconnect does.
- **Firmware**: reduced SD-card `open()` calls per game change (up to 6 before, as few as 1 now) by remembering which of the two SD folder-naming conventions the card uses instead of probing both every time — also reduces exposure to a rare ESP32 heap-allocation crash inside the SD filesystem driver; a retry-once mitigation was added for the catchable variant of that same crash.
- **Recalbox scripts**: fixed a zombie hi-score round-robin process that could survive a screensaver wake-up and keep drawing an old score panel over the marquee indefinitely.
- **Recalbox scripts**: fixed `gameclip`/demo mode showing the generic playlist instead of the clip's own game marquee (a leftover from an old MQTT-era workaround that had over-disabled it).
- **Docs**: the previous master is archived as `archive/mqtt-pre-udp-transport-final` — last state of the project before this transport switch, kept for rollback.
- **Branding**: the project's firmware edition is renamed **Raw565 Edition → RawEdition v2.0** (the `raw565` pixel format itself, and everything built on it, is unchanged — only the product name/version badge changes).

## 2026-09-06 — `dev/core-reassignment` merged to master: in-game overlays, RB Challenge, Playlist tab

The biggest merge in the project's history — months of work on a separate branch, reconciled with everything shipped on master in the meantime, then tested point-by-point (each conflict resolved and re-tested individually) before landing here. Already running an earlier version? See **[UPGRADING.md](UPGRADING.md)**.

- **Firmware**: new **in-game overlay system** — while a game is actually running, the panel can automatically alternate the marquee with the real **MAME/FBNeo Hi-Score** (community manifest, ~2,758 games, decoded from the emulator's own saved score file, no live RAM reading), **Game Info** (description/genre/developer/year from `gamelist.xml`), unlocked **RetroAchievements**, and Recalbox's own monthly **Challenge** leaderboard. Fully passive on the DMD side — all timing/logic lives in the Recalbox-side scripts, the firmware just displays what it's sent and auto-reverts to the marquee on its own local timer, so a slow/failed script can never leave the panel stuck. See the [dedicated README section](README.md#in-game-overlays--hi-score-game-info-achievements--rb-challenge).
- **PC Toolkit**: **Mode 9** (and the auto-install baked into Mode 1) now also installs the Hi-Score/Game Info/Achievements/Challenge scripts (`dmd_helpers/`) and cleans up any older script names left over from a previous install — previously only Mode 1's separate staging step handled this, Mode 9 itself did not.
- **PC Toolkit**: the mask-system "slow" flag (**"L"**, triggers the wait screen on huge collections) is now computed **per alphabetical sub-folder ("bucket")** instead of per whole system — a system with one big and several small sub-folders no longer has the small ones penalized unnecessarily. The Settings-tab threshold default follows accordingly (5,000 → 800, meaningful again now that it applies per bucket).
- **PC Toolkit**: **Playlist tab** — build your own attract-mode rotations by picking any mix of the 600-GIF pack and your own GIFs (drag a PC folder in); can now also be done **mid-`Mode 1`**, before the working folder is even copied to the SD card, instead of only after the fact from an inserted card.
- **PC Toolkit**: default UI language is now the **Windows system language** on first launch (instead of always English) — an explicit choice in the Settings tab still always wins afterwards.
- **PC Toolkit**: several fixes found by testing the merge live — a leftover "copy to SD" panel could push the Progress bar outside the fixed window on some Advanced-tab modes, the random theme picker could land on the plain "default" theme, and closing the app while mid-way through adding custom GIFs (Mode 1's playlist step) now resumes the pipeline instead of quitting.
- **Firmware / MQTT**: the 12 separate `marquee/cmd/*` topics were consolidated into a single `marquee/cmd` topic (compact `CMD=/ARG=` payload) — cuts the number of MQTT subscriptions per (re)connection from 12 to 2, reducing exposure to a rare ESP32 WiFi-stack condition where a subscribe could silently never leave the device.

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
