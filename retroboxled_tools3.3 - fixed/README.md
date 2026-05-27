# 🎮 RetroBoxLED Toolkit GUI (Recalbox) — Help 

---

## ✅ Goal

Prepare the SD card for the ESP32 **marquee HUB75 / DMD 128x32** by generating:

- `systems/<system>/...` (images per game/system)
- `systems/_defaults/` (replacement files)
- `games_cache*.bin` (game images index)
- `systems_cache.dat` (ESP32 systems index)
- (then copy to the SD card)

---

## 📁 Where does the script work?

On Windows, the script creates a temporary working folder (like “sd_card”) in:

**%LOCALAPPDATA%/Temp/RetroBoxLED**

At the end, the “copy” mode synchronizes this content to the **root of your SD card**.

---

## 🎛️ Modes (choice 1 to 8)

### Mode 1 — Extraction + Conversion + Build cache (ALL)
- reads `gamelist.xml` from your ROMs folder
- extracts images (PNG/GIF)
- converts **PNG → 128x32** (GIFs are kept as-is)
- builds `games_cache*.bin` + `systems_cache.dat`
- also downloads `systems/_defaults` (GitHub)

### Mode 2 — Download `systems/_defaults` via GitHub only
- **downloads only** `systems/_defaults/`
- does **not** extract, convert, or generate any caches

> Ideal if you only want to update the `_defaults` fallback files.

### Mode 3 — Extract gamelist only
- extracts images from `gamelist.xml`
- does not generate caches

### Mode 4 — Convert only
- converts PNG images to **128x32**
- keeps GIFs
- does not generate caches

### Mode 5 — Build `games_cache.bin` only
- generates the games image cache from already present images

### Mode 6 — Generate `systems_cache.dat` only
- builds the ESP32 systems index
- relies on `systems/_defaults` and/or already present systems

### Mode 7 — Copy to SD card (robocopy)
- copies the working folder (`sd_card/`) **to the root** of the SD card
- uses `robocopy` (fast)

---

## 🧠 `_defaults` replacement (fallback)

The toolkit uses:

- `systems/_defaults/`

If a system/game image is missing, the ESP32 will fall back to the file inside `_defaults`.

---

## 🖼️ 128x32 Conversion (PNG → 128x32)

Conversion requires **Pillow**.
The script tries to install it automatically if needed.
If Pillow is unavailable, conversion is disabled (but GIFs still work).

---

## 🧪 Prerequisites (SD card & ROMs)

- SD card: **FAT32**
- ROMs: contain subfolders with a `gamelist.xml` (standard Recalbox layout)

---

## ▶️ How to use

### Via the Tkinter interface
1. Run `RetroBoxLED_gui.py`
2. Select the ROMs folder
3. Select a **mode**
4. Click **Start**
5. (Mode 7) Pick the SD card and start the copy

### Via the script in console (optional)
Follow the text menu: choice `2` corresponds to **downloading `_defaults` only**.

---

## ℹ️ Useful notes
- The script can download from GitHub for the `_defaults` workflow (internet required).
- Mode 2 is intentionally “light”: **only** `systems/_defaults`.

---

## 🧾 What to check after copying to SD
On the SD card **root**, you should see:

- `systems/` (including `_defaults/`)
- `systems_cache.dat`
- `games_cache*.bin` (or `games_cache.bin` depending on the images/folders)
