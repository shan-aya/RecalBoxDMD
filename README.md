# RecalBox DMD — ESP32 Marquee Firmware

## Overview

**RecalBox_DMD** is a complete ESP32 firmware for driving a **HUB75 LED panel (128×32 px)** displaying game marquee images from a Recalbox system. The firmware supports PNG and GIF playback, system playlists, MQTT integration for real-time game updates, Telnet debugging, and Bluetooth control.

---

## Features

- **Image Support**: PNG and GIF with intelligent caching
- **Performance Optimization**: raw565 / raw565pack formats for fast rendering
- **Game Cache**: Bigram-indexed game image cache for rapid lookup
- **System Cache**: System type detection and slow-system fallback handling
- **Networking**:
  - WiFi (static or DHCP)
  - MQTT (real-time game/system display)
  - Telnet (debugging & manual control)
- **Audio**: Bluetooth Serial support
- **Configuration**: `config.ini` driven setup
- **Fallback Logic**: automatic fallback to default images if game images missing

---

## Hardware Requirements

### Microcontroller
- **ESP32** (tested on DEVKIT-C, 4MB flash recommended)

### Display Panel
- **HUB75 LED Panel** 64×32 pixels, **2 panels chained** (total 128×32 visible)

### Storage
- **SD Card** (microSD via SPI adapter, **FAT32 formatted**)

### Power Supply
- 5V for LED panels (~2A typical)
- 5V USB for ESP32 (can share with panel supply via separate regulator)

---

## Pin Configuration

### SPI (SD Card)
| Pin | GPIO |
|-----|------|
| CS  | 5    |
| MISO| 19   |
| MOSI| 23   |
| SCLK| 18   |

### HUB75 Matrix Pins
| Signal | GPIO |
|--------|------|
| CLK    | 16   |
| OE     | 15   |
| LAT    | 4    |
| A      | 33   |
| B      | 32   |
| C      | 22   |
| D      | 17   |
| E      | -1   |

### Color Pins (Half Channels)
| Top Half | GPIO | Bottom Half | GPIO |
|----------|------|-------------|------|
| R1       | 25   | R2          | 14   |
| G1       | 26   | G2          | 12   |
| B1       | 27   | B2          | 13   |

---

## SD Card Structure

```
(SD Root)
├── config.ini                          # Main configuration
├── systems_cache.dat                   # System type index (generated)
├── games_cache.bin                     # Game image cache (generated)
├── systems/
│   ├── _defaults/
│   │   ├── default.png                # Fallback image (128×32 PNG)
│   │   ├── default.raw565             # Fast fallback (128×32 raw565)
│   │   ├── <system>.png               # System logos
│   │   ├── <system>.gif
│   │   ├── <system>.raw565
│   │   ├── <system>.raw565pack        # Pre-encoded GIF as raw frames
│   │   └── <system>.meta              # Delay metadata for raw565pack
│   │
│   └── <system>/
│       ├── <game>.png                 # Game marquee images
│       ├── <game>.gif
│       ├── <game>.raw565              # Pre-converted fast format
│       └── <game>.raw565pack          # Raw frame pack
│
└── playlists/
    ├── <playlist_name>.txt            # GIF file list
    ├── <playlist_name>.cache          # Compiled cache
    ├── <playlist_name>.idx            # Offset index
    └── <playlist_name>.sig            # Hash signature
```

---

## Arduino Libraries Required

Install these libraries via **Sketch → Include Library → Manage Libraries**:

| Library | Author | Version |
|---------|--------|---------|
| `ESP32-HUB75-MatrixPanel-I2S-DMA` | mrfaptastic | ≥1.0.0 |
| `AnimatedGIF` | Larry Bank | ≥2.0.0 |
| `SD` | Arduino | (built-in) |
| `SPI` | Arduino | (built-in) |
| `WiFi` | Arduino | (built-in) |
| `PubSubClient` | Nick O'Leary | ≥2.8.0 |
| `BluetoothSerial` | Espressif | (ESP32 core) |

**Note**: `pngle.h` is included directly in the sketch for PNG decoding.

---

## Configuration (`config.ini`)

Example:

```ini
# Display playlist on startup
playlist=marquee.txt

# WiFi Settings
wifi_enabled=1
wifi_ssid=MySSID
wifi_password=password123
wifi_static_enabled=0
# Optional static IP:
# wifi_static_ip=192.168.1.100
# wifi_gateway=192.168.1.1
# wifi_subnet=255.255.255.0
# wifi_dns1=8.8.8.8
# wifi_dns2=8.8.4.4

# Recalbox MQTT Broker
recalbox_ip=192.168.1.50

# MQTT Event Topic (receives events from Recalbox)
mqtt_event_topic=marquee/event

# Bluetooth
bluetooth_enabled=0
bluetooth_name=ESP32-GIF

# Display startup splash screen
info=1

# Playlist playback mode
random=1
```

---

## Compilation & Flash

### 1. Prerequisites
- Arduino IDE ≥ 1.8.0
- ESP32 Board Package (≥2.0.0)
  - Add board URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

### 2. Open Sketch
- File → Open → `RecalBox_DMD.ino`

### 3. Configure Board
- Tools → Board: Select **ESP32 Dev Module** (or your ESP32 variant)
- Tools → Flash Size: Select **4MB** (or your board's size)
- Tools → Upload Speed: **921600** (or lower if unstable)
- Tools → Port: Select COM port

### 4. Install Dependencies
- Sketch → Include Library → Manage Libraries
- Search for and install each library from the table above

### 5. Compile
- Sketch → Verify (Ctrl+R)

### 6. Upload
- Sketch → Upload (Ctrl+U)
- Or use: **esptool.py** for command-line flashing:
  ```bash
  esptool.py --chip esp32 --port COM3 --baud 921600 write_flash -z 0x10000 RecalBox_DMD.ino.bin
  ```

---

## Usage

### Startup
1. Power on ESP32
2. If `info=1`, see splash screen (2.5 seconds)
3. If WiFi enabled, connect and display status
4. Load playlist (if configured) or show default
5. Ready for MQTT commands / Telnet input

### MQTT Topics (Input)
- `marquee/cmd/stop` — Stop playback
- `marquee/cmd/default` — Resume playlist
- `marquee/cmd/system <system_name>` — Display system logo
- `marquee/cmd/game <system>/<game>` — Display game image
- `marquee/event` — Subscribe to Recalbox events (handled automatically)

### Telnet Commands (Port 23)
```
help               — List all commands
ip                 — Show IP address
wifi               — Show WiFi status
wifiinfo           — Detailed WiFi info
next               — Play next GIF
count              — Show loaded GIF count
playlist           — Show current playlist
random on|off      — Toggle random playback
reboot             — Restart ESP32
show <path>        — Display file (e.g., /systems/ps2/game.png)
showsys <system>   — Display system logo
showgame <sys/rom> — Display game image
default            — Show default image
black              — Clear screen
mode               — Show current display mode
syscache           — List system cache
rebuildcache       — Rebuild system cache
heap               — Show free memory
```

---

## Data Processing Pipeline

### System Cache
1. On first boot (or `rebuildcache`), scan `/systems/` folder
2. Detect available image types per system (PNG/GIF/raw565pack)
3. Mark systems with >800 games as "slow" (flag 'L')
4. Save index to `systems_cache.dat`

### Game Cache
1. Python tools (`tools/`) pre-process game lists from `gamelist.xml`
2. Extract marquee images, convert PNG to 128×32
3. Build `games_cache.bin` with bigram-indexed lookup
4. Copy to SD card root

### Display Flow
- **Fast System (N)**: PNG decode + draw, fallback to raw565, fallback to default
- **Slow System (L)**: Check cache first, fallback to default raw565 (skip PNG decode)
- **MQTT Game Request**: Load system mask, then game image, with fallback chain

---

## Python Tools

Preparation scripts are available in the `tools/` folder:

- `RetroBoxLED_gui.py` — GUI for image extraction, conversion, and SD card preparation
- `RetroBoxLED_tool3.3.py` — Command-line batch processor

See `tools/README.md` for detailed instructions.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| **SD card not detected** | Check CS pin (5), ensure FAT32, test with `dir` command in Telnet |
| **Panel shows garbage** | Verify HUB75 pins match config; check panel polarity |
| **Slow image loading** | Mark system as slow (flag 'L') in config; pre-convert PNG to raw565 |
| **WiFi / MQTT not working** | Verify SSID, password, and broker IP in `config.ini`; check Telnet `wifiinfo` |
| **Out of memory** | Reduce bigram cache or convert large PNG to raw565pack format |

---

## Performance Notes

- **PNG Decoding**: ~200–500ms per image (depends on size and system speed)
- **GIF Playback**: 10–50 FPS (limited by panel refresh and SPI speed)
- **raw565 Blitting**: ~50ms per full-screen refresh (optimized with single bulk read)
- **MQTT Latency**: ~100–500ms from message to display update

---

## License

This firmware is provided as-is for personal and educational use.

---

## Support & Feedback

For issues, questions, or contributions, refer to the project repository or contact the maintainer.

---

**Version**: Raw565 Edition  
**Last Updated**: June 2026
