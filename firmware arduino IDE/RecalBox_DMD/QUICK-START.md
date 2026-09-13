# Quick Start — RecalBoxDMD RawEdition v2.0

🇬🇧 **English** · [🇫🇷 Français](QUICK-START.fr.md) · [🇪🇸 Español](QUICK-START.es.md)

> This page pulls out the "Quick start" section of the full **[README.md](README.md)** on its own — handy to link or print. For features, hardware details, configuration and everything else, see the full README.

<p align="center"><b>🚀 Zero to a working marquee in 4 steps 🚀</b></p>

<table align="center">
<tr>
<td align="center" width="70"><h2>1️⃣</h2></td>
<td>

**[Install the PC Toolkit](#install-the-pc-toolkit) + first run**
Scrape your games in Recalbox, point the toolkit at your ROMs folder, click **Start**.

</td>
</tr>
<tr>
<td align="center"><h2>2️⃣</h2></td>
<td>

**[Assemble the DMD](README.md#hardware)**
Join the two panels, mount the DMDos board, wire it up — **~5 minutes, no soldering**.

</td>
</tr>
<tr>
<td align="center"><h2>3️⃣</h2></td>
<td>

**[Flash the firmware](README.md#firmware--compiling--flashing)**
One-click browser installer — **no Arduino IDE required**.

</td>
</tr>
<tr>
<td align="center"><h2>4️⃣</h2></td>
<td>

**Insert the SD card, power on**
First boot walks you through Wi-Fi setup, then the **[web configuration page](README.md#web-configuration--live-in-your-browser)** takes over for everything else (brightness, playlists, clock themes...).

</td>
</tr>
</table>

## Install the PC Toolkit

Ships four ways — grab whichever you prefer from the **[Releases page](https://github.com/shan-aya/RecalBoxDMD/releases)** (the built `.exe`/`.msi` files aren't committed to the repo itself, only released there):

**Option A — Windows installer (recommended)**

```
1. Download RecalBoxDMD_Toolkit_Setup.exe from the Releases page
2. Run it — Start Menu shortcut, optional desktop icon, proper uninstaller
3. Launch "RecalBoxDMD Toolkit" from the Start Menu
```

**Option B — Portable .exe (no install)**

```
1. Download RecalBoxDMD_GUI.exe from the Releases page
2. Run it directly — no install, no Python required, single file
```

**Option C — .msi (for scripted/group-policy deployment)**

```
1. Download the .msi from the Releases page
2. msiexec /i "RecalBoxDMD Toolkit-1.0.0-win64.msi"   (or double-click)
```

**Option D — From Python source**

```
1. Grab the tools/ folder
2. Double-click install_and_run.bat — installs Python (via winget, if
   missing), Pillow and Markdown, then launches the GUI
   (or manually: pip install Pillow Markdown && python run_gui.py)
```

## First run

```
1. Scrape your games in Recalbox (see "How to scrape?" in the tool,
   depends on your Recalbox version — logo, marquee, or cut-out logo)
2. Launch the toolkit → Main tab
3. Pick your Recalbox version (10.x / 9.x / legacy)
4. Pick your ROMs folder (e.g. D:\Recalbox\share\roms)
5. Click Start — MODE 1 runs the full pipeline automatically
6. Insert the SD card → the blinking button offers to copy it for you
```

Next: [assemble the hardware](README.md#hardware) and [flash the firmware](README.md#firmware--compiling--flashing) — then insert that SD card and power on.

---

📖 Full documentation: **[README.md](README.md)** · Upgrading from an earlier version: **[UPGRADING.md](UPGRADING.md)** · Version history: **[CHANGELOG.md](CHANGELOG.md)**
