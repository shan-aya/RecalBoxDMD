# 🧪 Alpha test plan

Report each result with **`/alphatest`** on Discord (see the [README](README.md)). ⚠️ = precaution.

## T01 · Panel colour order (ESP32)  —  *ESP32*

**Steps**

1. DMD web page → **Settings** → *Colour order*.
2. Pick another order (e.g. GRB) → *Apply and restart*.
3. Watch the boot title / a logo: do the colours match the order?
4. Go back to the order that shows the right colours; restart once more.

**Expected** : The DMD restarts, colours change with the order, the chosen order is kept after another restart.

## T02 · Panel colour order (Raspberry Pi)  —  *Raspberry Pi*

**Steps**

1. Pi web page → **Settings** → *Colour order*.
2. Pick another order → apply (the service restarts).
3. Check the colours on the panel, then restore the right order.

**Expected** : The panel comes back with the new colour order; the setting survives a Pi reboot.

## T03 · Settings page layout  —  *ESP32 / Pi / Toolkit*

**Steps**

1. Open **Settings** on the ESP32 and on the Pi (phone and PC).
2. Check the sections: *Firmware channel*, *Display* (colour order above brightness), *Startup and modes*.
3. Check that the three buttons (switch channel / apply and restart / configure) have the same size and are aligned.
4. Switch the page language FR / EN / ES.

**Expected** : Same look on both devices, no overlap, texts translated, all controls usable on a phone.

## T04 · Switch channel alpha ↔ stable (ESP32)  —  *ESP32*

**Wait at least 60 seconds of normal running between two switches.**

**Steps**

1. **Settings** → *Switch to channel STABLE…* → confirm.
2. Wait about 1.5 minutes (download, restarts).
3. Check the title on the panel (`STABLE`) and the version on the page.
4. Wait 60 s, then switch back to ALPHA.

**Expected** : Both switches succeed, the web pages come back by themselves, Wi-Fi and settings are kept.

## T05 · Switch channel alpha ↔ stable (Raspberry Pi)  —  *Raspberry Pi*

**Steps**

1. Pi **Settings** → *Switch to channel STABLE…* → confirm.
2. Wait for the service to restart; check the title `Pi STABLE` and the version.
3. Switch back to ALPHA.

**Expected** : The daemon of the chosen channel is installed each time; the Pi never changes channel by itself.

## T06 · Update notices stay in the alpha channel  —  *ESP32 / Pi / Toolkit*

**Steps**

1. Open the web pages (ESP32, Pi) and the toolkit.
2. Read the update banners (firmware, Pi, toolkit).
3. Follow the links.

**Expected** : Banners only mention alpha versions (pre-releases); never a stable number or a stable download.

## T07 · Web pages come back after a firmware update  —  *ESP32*

**Steps**

1. Update the firmware by Wi-Fi (web page or toolkit Mode 14).
2. Wait 1 to 2 minutes without touching anything.
3. Reload the web page.

**Expected** : The pages are back by themselves. If you see the fallback page, report the cause line it shows (screenshot).

## T08 · Settings survive restarts  —  *ESP32*

**Steps**

1. Change a few settings (brightness, language, a widget) and save.
2. Restart the DMD three times (button / power).
3. Check the settings and that it rejoins your Wi-Fi.

**Expected** : Everything is kept; the DMD never falls back to its own access point (AP mode).

## T09 · Change the Wi-Fi network  —  *ESP32*

**Steps**

1. **Network** page → choose or re-enter your Wi-Fi → save.
2. Wait for the reconnection.
3. Reload the Settings page.

**Expected** : The DMD reconnects and the pages load; nothing else is lost.

## T10 · Game GIFs while playing  —  *ESP32 / Pi / Toolkit*

**Steps**

1. Web page *Basic* → enable *Game GIFs while playing*.
2. Re-run **Mode 9** (Recalbox scripts) of the toolkit once.
3. Launch games that have a GIF with the same name as the game (put them in `/gifs_jeux` on the SD card, or let auto mode work).
4. Watch: 1 to 5 GIFs, then the logo; leave a game in demo mode.

**Expected** : Only sure matches are played (never the wrong game); 1 GIF per pass in demo mode; an exclusion file really excludes.

## T11 · Logo customization  —  *Toolkit*

**Steps**

1. Toolkit → *Customize* tab → pick a game.
2. Replace its logo with an image, then with an animated GIF, then with a text card.
3. Use *Cancel* (back to the previous version, then the original), *Restore my logos*, and the zip backup / restore.
4. Repeat on the SD card and on a Raspberry Pi over Wi-Fi if you have one.

**Expected** : The logo changes on the DMD, every cancel returns what it should, nothing is lost.

## T12 · Target badge and Playlist over Wi-Fi  —  *Toolkit*

**Steps**

1. Open the toolkit with a card inserted, then with a Pi on the network.
2. Check the target badge (ESP32 / Pi) and the greyed-out modes.
3. *Playlist* tab: write to the card, then over Wi-Fi to a Pi without a card (seasonal playlists too).

**Expected** : The right target is preselected, inapplicable modes are greyed out, playlists arrive on the device.

## T13 · Installation tab (Mode 1)  —  *Toolkit*

**Steps**

1. Toolkit → *Installation* tab → page *Without a Recalbox*, then *With a Recalbox*.
2. Go through Start / Next / Continue: ROMs folder first, then Recalbox version.
3. Read the warnings about the Recalbox version and scraped media.

**Expected** : Next / Continue refuse an invalid ROMs folder; the warnings are clear; the copy completes.

## T14 · Format an SD card as FAT32  —  *Toolkit*

**⚠️ This ERASES the card. Use a spare card, never one you need.**

**Steps**

1. Insert a spare card (ideally larger than 32 GB).
2. Toolkit → Mode 1 → SD card window → *Format as FAT32…*.
3. Accept the administrator prompt and the two confirmations that name the drive.
4. Put the card in the DMD and check that it boots.

**Expected** : Only removable cards are offered; the card is formatted in FAT32 and works in the DMD.

## T15 · Channel mismatch alert in the toolkit  —  *Toolkit*

**Steps**

1. Put one DMD on the STABLE channel (T04).
2. Open the alpha toolkit with that DMD reachable.

**Expected** : The toolkit warns about the difference and offers the toolkit of the DMD's channel.

## T16 · Pi update button stays on alpha  —  *Raspberry Pi*

**Steps**

1. On the Pi (alpha) web page, use the update button when a new alpha daemon is announced.
2. Check the version and the title afterwards.

**Expected** : The Pi stays on alpha (`Pi ALPHA`), never installs a stable daemon.

## T17 · Recalbox themes (Modes 12 / 13)  —  *Toolkit*

**Steps**

1. Run Mode 12 or 13 on a Recalbox alpha.
2. Check that the default Recalbox theme logos are not overwritten by an old conversion.
3. Re-download a theme: stale files should disappear.

**Expected** : The default theme stays up to date with the Recalbox alpha; no stale leftovers.
