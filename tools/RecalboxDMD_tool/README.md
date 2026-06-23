# 🎮 RecalBoxDMD Toolkit v3.4 — Full Help

> **RecalBoxDMD** = Recalbox + DMD LED panel 128x32 for your arcade cabinet!

---

## 📋 Table of Contents

1. [Overview](#-overview)
2. [Hardware Requirements](#-hardware-requirements)
3. [Software Requirements](#-software-requirements)
4. [Quick Start](#-quick-start)
5. [Graphical Interface (GUI)](#️-graphical-interface-gui)
   - [Main Tab](#-main-tab)
   - [Advanced Tab](#-advanced-tab)
   - [Logs Tab](#-logs-tab)
   - [Settings Tab](#-settings-tab)
6. [Mode Details](#-mode-details)
7. [Recommended Workflow](#-recommended-workflow)
8. [Recalbox Scraping Tutorial](#-recalbox-scraping-tutorial)
9. [ESP32 Firmware — Raw565 Edition](#-esp32-firmware--raw565-edition)
   - [Why raw565?](#-why-raw565)
   - [Arduino IDE Compilation](#-arduino-ide-compilation)
   - [Configuration (config.ini)](#️-configuration-configini)
   - [WebInstaller](#-web-installer)
10. [The raw565 Format in Detail](#-the-raw565-format-in-detail)
    - [.raw565 (still image from PNG)](#-raw565-still-image-from-png)
    - [.raw565pack + .meta (animated GIF)](#-raw565pack--meta-animated-gif)
    - [Bigram Cache — accelerated indexing](#-bigram-cache--accelerated-indexing)
    - [Mask system](#-mask-system)
11. [SD Card Structure](#-sd-card-structure)
12. [Troubleshooting](#-troubleshooting)
13. [Credits](#-credits)

---

## 🎯 Overview

**RecalBoxDMD Toolkit** is an optimized fork of **RetroBoxLED** by Jamyz, specifically designed to solve **display lag** issues on systems with a large number of files (MAME fullset, FBNeo, etc.).

### 🔥 The problem solved

The original version by Jamyz read **PNG and GIF** files directly from the SD card. On systems like **MAME fullset** (30,000+ games), this approach caused:

```
❌ Original problem:
   - Opening /systems/mame/ folder with 30,000 files
   - ESP32 lists all files → freezes for several seconds
   - PNG decoding on ESP32 → slow, insufficient memory
   - Between each game: black screen or visible lag
```

**The "Raw565 Edition"** pre-converts all images on the PC (via the Python script) into a **raw RGB565 format** directly displayable by the ESP32:

```
✅ raw565 solution:
   1. Script converts each PNG → .raw565 file (8192 bytes, ready to use)
   2. Each GIF → .raw565pack (concatenated frames) + .meta (timings)
   3. ESP32 reads the raw565 file and sends it directly to the LED panel
   4. No decoding, no lag: near-instant display
```

| Metric | Original PNG/GIF | Raw565 Edition |
|--------|-----------------|----------------|
| Display time | 500ms – 3s+ | **5–15ms** |
| RAM needed | 50-100 KB | **8 KB** |
| MAME fullset lag | Freeze 5-10s | **0** |
| Compatibility | All systems | **"L"-flagged systems** |

### The mask system

For very large systems flagged **"L"** (Large / Slow, like MAME or FBNeo), the firmware uses a mask mechanism:

1. The script detects systems with >800 individual files
2. It marks these systems as **"L"** in `systems_cache.dat`
3. When the ESP32 receives a game from this system:
   - It immediately displays the **system default raw565** from RAM cache
   - In parallel, an async task decodes the specific PNG
   - When decoding is done, the specific image replaces the mask
4. Result: **the user never sees a black screen**, the panel stays active

> **💡 Result**: No more seconds of waiting between games. Display is near-instant even with a 30,000-file MAME fullset!

### 🔄 How everything fits together

```
┌─────────────────────────────────────────────────────────────┐
│                     RECALBOX                                 │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Launches a game → MQTT sends "mame/kof98" to ESP32  │   │
│  └──────────────┬───────────────────────────────────────┘   │
│                 │                                           │
│          MQTT Event                                        │
│                 ▼                                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │            ESP32 + LED Panel 128x32                    │   │
│  │                                                       │   │
│  │  Receives "mame/kof98":                               │   │
│  │  1. Look for /systems/mame/kof98.raw565 (instant)     │   │
│  │  2. Not found? Look in games_cache.bin                │   │
│  │  3. Not found? Display _defaults/mame.raw565          │   │
│  │  4. Not found? Display _defaults/default.raw565       │   │
│  │                                                       │   │
│  │  ⏱️ Total time: 5 to 15 ms instead of 500ms-3s!      │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────┐
         │     RecalBoxDMD_tool3.4.py (Prepares SD)     │
         │  Extracts images from gamelists.xml           │
         │  Converts PNG → .raw565 (raw RGB565 format)   │
         │  Converts GIF → .raw565pack + .meta           │
         │  Builds the bigram cache (games_cache.bin)    │
         │  Marks slow systems (flag "L")                │
         │  Copies everything to the SD card             │
         └─────────────────────────────────────────────┘
```

---

## 🧰 Hardware Requirements

| Component | Reference | Approx. Price |
|-----------|-----------|---------------|
| 🧠 Microcontroller | ESP32 DevKit V1 (38 pins) | ~$5 |
| 🖥️ LED Panel | RGB HUB75 P4 64x32 or 128x32 | ~$15-25 |
| 💾 SD Reader | Micro SD SPI adapter module | ~$2 |
| 🔌 Connection board | DMDos Board V3 (optional, simplifies wiring) | ~$15 |
| ⚡ Power Supply | 5V 4A+ (depending on panel size) | ~$10 |

> **💡 Tip**: The DMDos Board by [Mortaca](https://www.mortaca.com/) includes the SD reader and eliminates soldering!

---

## 💻 Software Requirements

- **Python 3.8+** installed on your PC
- **Pillow** (installed automatically if missing)
- **Arduino IDE** (only for recompiling the firmware)
- Chrome/Edge browser (for WebInstaller)

---

## 🚀 Quick Start

```
1. Download the files from the tools/ folder
2. Run RecalBoxDMD_tool3.4.py
3. Follow the on-screen instructions or use the GUI
```

---

## 🖥️ Graphical Interface (GUI)

The GUI launches automatically. It is organized into **5 tabs**:

### 📌 Main Tab

> Contains only **Mode 1** — the essential for most users.

```
┌────────────────────────────────────────────────────────────┐
│ [Main]  [Advanced]  [Logs]  [Settings]  [HELP]            │
├────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 📋 Mode                                               │ │
│ │                                                       │ │
│ │ ◉ MODE 1 — Extraction + Conversion + Build Cache      │ │
│ │   (ALL in one click!)                                 │ │
│ │                                                       │ │
│ │ ┌──────────────────────────────────────────────────┐  │ │
│ │ │ 📂 Choose ROMs folder                          │  │ │
│ │ │ D:\Recalbox\share\roms                          │  │ │
│ │ └──────────────────────────────────────────────────┘  │ │
│ │                                                       │ │
│ │ [ 🟢 Start ]                                          │ │
│ │ [ ❌ Quit ]                                           │ │
│ └────────────────────────────────────────────────────────┘ │
│                                                            │
│ ┌────────────────────────────────────────────────────────┐ │
│ │ Progress: ████████████░░░░░░ 75%                       │ │
│ │ Extracting: mame/king_of_fighters.png                  │ │
│ │ ⏸️ Pause  ▶️ Resume  ⏭️ Skip  ⏹️ Stop                  │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

**👉 Mode 1 = the full mode** that chains:
1. Image extraction from your `gamelist.xml` files
2. PNG → raw565 conversion (raw RGB565 format, 8192 bytes)
3. GIF → raw565pack + meta conversion (concatenated frames)
4. Download `systems/_defaults` from GitHub
5. Build `games_cache.bin` (bigram index)
6. Generate `systems_cache.dat` (system index + slow/fast flags)
7. Automatic detection of large systems (flag `L` for MAME, FBNeo...)

### 🔬 Advanced Tab

> Contains **modes 2 to 7** for targeted operations.

```
┌────────────────────────────────────────────────────────────┐
│ [Main]  [Advanced]  [Logs]  [Settings]  [HELP]            │
├────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 📋 Mode                                               │ │
│ │                                                       │ │
│ │ ○ MODE 2 — Download _defaults from GitHub             │ │
│ │ ○ MODE 3 — Gamelist Extraction only                   │ │
│ │ ○ MODE 4 — PNG→raw565 + GIF→raw565pack conversion     │ │
│ │ ○ MODE 5 — 128x32 conversion only                     │ │
│ │ ○ MODE 6 — Build Games Cache only                     │ │
│ │ ○ MODE 7 — Generate systems_cache.dat                 │ │
│ │                                                       │ │
│ │ [ 🟢 Start ]                                          │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

### 📝 Logs Tab

Displays all script output in real time. Useful for tracking progress and detecting errors.

Control buttons:
- **⏸️ Pause** — Pauses processing
- **▶️ Resume** — Resumes processing
- **⏭️ Skip** — Skips to next step
- **⏹️ Stop** — Stops the script

### ⚙️ Settings Tab

Allows changing the interface language:
- 🇫🇷 French
- 🇬🇧 English
- 🇪🇸 Spanish

### 📖 HELP Tab

Displays this README directly in the interface.

---

## 📖 Mode Details

| Mode | Name | Action | Est. Duration |
|------|------|--------|---------------|
| **1** | **ALL** | Extraction + raw565 Conversion + Cache + _defaults | Long (5-30 min) |
| **2** | Download _defaults | Fetches system images from GitHub | Short (~1 min) |
| **3** | Extraction only | Reads gamelist.xml, copies images | Variable |
| **4** | raw565 conversion | PNG→raw565 + GIF→raw565pack/meta | Medium |
| **5** | 128x32 conversion | Resizes PNGs to 128x32 (image format) | Medium |
| **6** | Build Games Cache | Generates games_cache.bin (bigram index) | Fast |
| **7** | Generate systems_cache.dat | System index + slow/fast flags | Fast |
| **8** | **SD Copy** | Copies working folder to SD card | Fast |

---

## ✅ Recommended Workflow

```
1. 🔄 Scrape your games in Recalbox (cut-out logo or marquee)
   ↓
2. 📂 Launch RecalBoxDMD_tool3.4.py → Main tab
   ↓
3. 🗂️ Choose your ROMs folder (e.g. D:\Recalbox\share\roms)
   ↓
4. 🟢 Click Start (Mode 1)
   ↓
5. ⏳ Let the script work. It will:
      - Extract images from gamelist.xml
      - Convert PNG → .raw565 (fast RGB565 format)
      - Convert GIF → .raw565pack + .meta
      - Download _defaults from GitHub
      - Build the bigram cache (game indexing)
      - Mark large systems (flag "L" for MAME, FBNeo)
   ↓
6. ✅ Done! The sd_card/ folder is ready in %TEMP%/RecalBoxDMD
   ↓
7. 💾 Insert your SD card, the script offers to copy
   ↓
8. 🎮 Insert the SD in the ESP32 and power on!
      - Near-instant display even with 30,000 MAME games
```

---

## 🎨 Recalbox Scraping Tutorial

For the LED panel to display the right games, you must first **scrape** your ROMs in Recalbox:

1. From Recalbox, go to **Settings → Scraper**
2. Choose image type **CUT-OUT LOGO** or **MARQUEE**
3. Scrape all your systems
4. Images will be stored in `/recalbox/share/roms/...` with `gamelist.xml`

```
📁 /recalbox/share/roms/
   ├── 📁 mame/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 images/
   │   │   ├── 🖼️ kof98.png       ← Scraped image
   │   │   ├── 🖼️ sf2.png
   │   │   └── ...
   │   └── ...
   ├── 📁 snes/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 images/
   │   │   ├── 🖼️ supermetroid.png
   │   │   └── ...
   └── ...
```

> **💡 Tip**: Use Screenscraper.fr or TheGamesDB for best results!

---

## ⚡ ESP32 Firmware — Raw565 Edition

### 💡 Why raw565?

The **raw565** format is the key element of this version. Here's why:

```
┌────────────────────────────────────────────────────────────────┐
│                       SD CARD                                   │
│                                                                │
│  PNG (50 KB)  →  PC Conversion →  .raw565 (8,192 bytes)       │
│  GIF (200 KB) →  PC Conversion →  .raw565pack + .meta          │
│                                                                │
│  The ESP32 only needs to:                                      │
│    1. Open the .raw565 file (8 KB)                             │
│    2. Read 8192 bytes into RAM                                 │
│    3. drawRGBBitmap() → direct display                         │
│                                                                │
│  🚀 Total time: 5-15 ms (vs 500ms-3s for PNG/GIF)             │
└────────────────────────────────────────────────────────────────┘
```

### 🔧 Arduino IDE Compilation

1. Open `RecalBox_DMD.ino` in Arduino IDE
2. Install the following libraries:

| Library | Link | Purpose |
|---------|------|---------|
| ESP32-HUB75-MatrixPanel-I2S-DMA | [GitHub](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) | DMA LED panel control |
| AnimatedGIF | [GitHub](https://github.com/bitbank2/AnimatedGIF) | GIF decoding (fallback) |
| pngle | [GitHub](https://github.com/kikuchan/pngle) | PNG decoding (fallback) |
| WiFiManager | [GitHub](https://github.com/tzapu/WiFiManager) | WiFi configuration |
| Adafruit GFX Library | [GitHub](https://github.com/adafruit/Adafruit-GFX-Library) | Text and shape display |
| ArduinoJson | [GitHub](https://github.com/bblanchon/ArduinoJson) | Config and web |

3. Select **ESP32 Dev Module** from **Tools → Board**
4. Connect the ESP32, select the correct COM port
5. Click **Upload** (⚡)

### ⚙️ Configuration (config.ini)

Place this file at the root of the SD card:

```ini
# Info
info=0                     # 0 = no info on startup

# Playlist
playlist=TODO.txt          # Playlist to read in /playlist
random=1                   # 0 = sequential, 1 = random

# WiFi
wifi_enabled=1
wifi_ssid=mywifi
wifi_password=mypassword
wifi_static_enabled=1
wifi_static_ip=192.168.1.240
wifi_gateway=192.168.1.1
wifi_subnet=255.255.255.0

# MQTT
recalbox_ip=192.168.1.104   # Fixed IP of your Recalbox
image_folder=logo_detoure   # or "marquee"
```

### 🌐 Web Installer

> [👉 Install from the WebInstaller page](https://jamyz.github.io/RetroBoxLED/)

1. Use Chrome or Edge
2. Connect the ESP32 via USB
3. Click **Install** and select the COM port
4. **Important**: Check **« Erase device »** to completely wipe memory!

### 📡 MQTT — The system's brain

MQTT allows Recalbox to communicate with the ESP32 in real time:

```
Recalbox → marquee[rungame,...].sh → MQTT → ESP32 → LED Panel

1. You launch "King of Fighters '98"
2. The bash script detects the event → sends "mame/kof98" via MQTT
3. ESP32 receives → looks in order:
   a. /systems/mame/kof98.raw565      ← instant (5 ms)
   b. games_cache.bin → bigram        ← accelerated indexing
   c. /systems/_defaults/mame.raw565  ← system fallback
   d. /systems/_defaults/default.raw565 ← global fallback
4. Displayed in under 15 ms!
```

Place `marquee[rungame,endgame,systembrowsing,...].sh` in:
```
/recalbox/share/userscripts/
```

### 🔌 Telnet

The firmware includes a Telnet terminal for testing the ESP32. Connect with:
```
telnet 192.168.1.240
```
Type `help` for the command list.

---

## 🗂️ The raw565 Format in Detail

### 📄 .raw565 (still image from PNG)

```
Size: 128 × 32 × 2 = 8,192 bytes exactly
Format: Raw RGB565 (16 bits per pixel)
         Bit R: bits 15-11 (5 bits)
         Bit G: bits 10-5  (6 bits)
         Bit B: bits 4-0   (5 bits)

ESP32 reading:
  f.read(buffer, 8192);
  drawRGBBitmap(0, 0, buffer, 128, 32);
  // 1 SD operation + 1 draw → 5 ms
```

### 🎞️ .raw565pack + .meta (animated GIF)

The **`.raw565pack`** file concatenates all GIF frames:

```
[Filename].raw565pack
├── Frame 0  →  8,192 bytes (raw565)
├── Frame 1  →  8,192 bytes
├── Frame 2  →  8,192 bytes
├── ...
└── Frame N  →  8,192 bytes

[Filename].meta
├── delay_0  →  2 bytes (uint16, milliseconds)
├── delay_1  →  2 bytes
├── delay_2  →  2 bytes
└── ...
```

**Advantages**:
- **1 SD open** to read a frame (seek + bulk read)
- **No GIF decoding** on ESP32
- Accelerated reading via RAM-delay cache (`.meta` loaded once)
- Built-in speed control (`GIF_RAW_PACK_SPEED_PERCENT`)

### ⚡ Bigram Cache — accelerated indexing

To avoid listing SD folders (very slow with 30,000 files), the script builds a **bigram index**:

```
games_cache.bin
├── [Header]  → number of indexed systems
├── [System 0] → name + bigram table offset
├── [System 1] → name + offset
└── ...

Bigram table (703 entries = 2812 bytes per system)
├── Index 0   → '#'  (digits, symbols)
├── Index 1   → 'A'  (games starting with A)
├── Index 2   → 'AA' (games starting with AA)
├── Index 3   → 'AB'
├── ...
└── Index 702 → 'ZZ'

Each entry = offset in the cache file pointing to the game list
```

When the ESP32 searches for `kof98`:
1. Calculates bigram index: `K` → index `11*27+1=298` (prefix `KO`)
2. Loads the corresponding slice into RAM (bulk read)
3. Scans game names in this slice
4. Finds `kof98` and its type (`p` for raw565) → **instant**

### 🎭 Mask system

For very large systems (MAME, FBNeo, etc.), the mask mechanism prevents black screens:

```
┌─────────────────────────────────────────────────────────────┐
│  Step 1: Python script analyzes the system                 │
│  → Counts individual files                                 │
│  → >800 files? Flag "L" in systems_cache.dat               │
│                                                             │
│  Step 2: ESP32 receives "mame/kof98"                       │
│  → System MAME flagged "L"?                                │
│    YES → Displays IMMEDIATELY the default raw565            │
│           (preloaded in RAM, 0 ms wait)                    │
│                                                                   │
│  Step 3: In parallel, async task:                         │
│  → Looks for the specific .raw565                          │
│  → Not found? Decodes the PNG in background task          │
│  → When ready: replaces mask with the real image           │
│                                                             │
│  Result: User never sees a black screen!                   │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 SD Card Structure

```
📁 SD CARD (FAT32)
   │
   ├── 📄 config.ini              ← ESP32 Configuration
   │
   ├── 📁 systems/
   │   ├── 📁 mame/
   │   │   ├── 🖼️ kof98.raw565          ← Converted PNG (8 KB)
   │   │   ├── 🖼️ sf2.raw565
   │   │   ├── 🖼️ intro.raw565pack      ← Converted GIF (frames)
   │   │   ├── 🖼️ intro.meta            ← GIF timings
   │   │   └── ...
   │   ├── 📁 snes/
   │   │   ├── 🖼️ supermetroid.raw565
   │   │   └── ...
   │   └── 📁 _defaults/
   │       ├── 🖼️ default.raw565        ← Global fallback image
   │       ├── 🖼️ mame.raw565           ← MAME system fallback
   │       ├── 🖼️ snes.raw565
   │       └── ...
   │
   ├── 📁 gifs/                   ← Animation playlists
   │   ├── 📁 Arcade/
   │   ├── 📁 BEST_OF_TOP_30/
   │   └── 📁 Pixel_Art/
   │
   ├── 📁 playlists/
   │   ├── 📄 Arcade.txt
   │   └── 📄 TODO.txt
   │
   ├── 📄 games_cache.bin         ← Bigram cache (game indexing)
   └── 📄 systems_cache.dat       ← System index + slow/fast flags
```

---

## 🔧 Troubleshooting

### ❌ Problem: "Pillow is not installed"
- The script tries to install it automatically
- If it fails, run manually: `pip install Pillow`

### ❌ Problem: "GitHub API unreachable"
- Check your internet connection
- The _defaults modes require a connection

### ❌ Problem: "No removable drive detected"
- Insert your SD card
- Check it's detected by Windows (Explorer → This PC)
- Try again

### ❌ Problem: ESP32 displays nothing
- Check power supply (5V 4A minimum)
- Check config.ini is at the SD card root
- Check HUB75 wiring (ESP32 pinout)
- Try Telnet: type `help` to test

### ❌ Problem: "ESP32 not detected" (COM port missing)
Install USB drivers:

| USB Chip | Drivers |
|----------|---------|
| CP2102 | [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| CH340/CH341 | [SparkFun](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all) |

### ❌ Problem: Slow display or black screen between games
- Make sure you are using **Mode 1** (full raw565 conversion)
- Large systems (MAME, FBNeo) should be flagged **"L"** — check in `systems_cache.dat`
- If the flag is `N` (fast) but the system is slow, increase `SPRITE_LIMIT` in the .ino
- Check that `.raw565` files exist in `systems/<sys>/`

### ❌ Problem: Image not displaying on panel
- Make sure the image is in 128x32 format
- PNGs must be in RGB (not RGBA) for conversion
- raw565 files take priority over raw565pack

---

## 📁 Files Generated by the Script

| File | Format | Purpose |
|------|--------|---------|
| `sd_card/systems/.../*.raw565` | 8192 bytes RGB565 | Converted PNG (5ms display) |
| `sd_card/systems/.../*.raw565pack` | Concatenated frames | Converted animated GIF |
| `sd_card/systems/.../*.meta` | uint16[] × nb_frames | GIF frame timings |
| `sd_card/systems/_defaults/*.raw565` | 8192 bytes | Per-system fallback images |
| `sd_card/games_cache.bin` | Bigram index 703 entries | Game cache (accelerated lookup) |
| `sd_card/systems_cache.dat` | Text (val + name + flag) | System index + L/N flags |
| `missing_images.log` | Text | List of missing images |

---

## 🤝 Credits

- **Original RetroBoxLED project**: [Jamyz](https://github.com/Jamyz/RetroBoxLED) — the ESP32 firmware and base idea
- **Raw565 Edition**: Shan_ayA — raw565 format optimization, bigram cache, mask system, GUI
- **Inspiration**: [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) by fjgordillo86
- **Community**: [Recalbox](https://www.recalbox.com/)
- **Hardware**: [Mortaca - DMDos Board](https://www.mortaca.com/)

---

## ☕ Support

If this project helps you:
👉 [Donate via PayPal](https://www.paypal.com/paypalme/felysaya)

---

> **RecalBoxDMD Raw565 Edition** = Recalbox + DMD LED Panel + fast display even with 30,000 MAME games! 🎮⚡
