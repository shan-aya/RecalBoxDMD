# RecalBoxDMD on Raspberry Pi

[🇬🇧 **English**](RASPBERRY-PI.md) · [🇫🇷 Français](RASPBERRY-PI.fr.md) · [🇪🇸 Español](RASPBERRY-PI.es.md)

The same DMD, with a **Raspberry Pi** instead of the ESP32: same Recalbox scripts, same SD card content, same web configuration pages, widgets and themes. The Pi drives two 64×32 P5 panels (128×32 in total) through an HUB75 interface board.

> ✅ **Validated: Raspberry Pi 3B** (Raspberry Pi OS Lite 32-bit "Bookworm").
> ⚠️ **Planned but untested: Pi 4 / 400 / CM4, Pi 2, Zero 2.** The software detects the model and sets `gpio_slowdown` accordingly (Pi 4: 4; Pi 2/3: 2; Pi 1/Zero: 1), but only the Pi 3B has been tried on real panels. If your Pi flickers, adjust `gpio_slowdown=1..4` in `config.ini`.

## Install (easiest: the PC toolkit)

1. Download the **PC toolkit** (*Releases* page) and run it. At the top of the window, pick the **Raspberry Pi** target.
2. **Mode 1**: the wizard chains everything, from a **blank SD card** to a card ready to put in the Pi — formatting, Raspberry Pi OS Lite, your Wi-Fi / user / SSH, the DMD software, and your converted logos in an `RBPIDMD` partition. Windows asks for administrator rights (the disk is written directly); the card is erased after confirmation.
3. Put the card in the Pi, power it on, wait 2–3 minutes. The DMD shows **Configuration page** and its address (or open `http://rbpidmd.local/`). Without valid Wi-Fi the Pi opens the **RecalBox-DMD-Config** network (address 192.168.4.1) so you can set it up from a phone.
4. Enter your Recalbox's IP address in the Wi-Fi settings, then run the toolkit's **Mode 9**: the Recalbox scripts are **the same** as for the ESP32.

The separate modes remain available: **Mode 16** (prepare a card, replaces Raspberry Pi Imager) and **Mode 17** (install or update the software on a Pi that is already running, and copy the DMD SD card content). The DMD software for the Pi is **downloaded from GitHub** (`pi-v…` release, SHA-256 checked); offline, the copy already on the PC is used.

## Hardware

- Raspberry Pi 3B (validated) and an HUB75 interface board (RPI2DMD-type boards: "regular" GPIO mapping, optional DS1307 RTC).
- Two 64×32 P5 panels chained (128×32), 1/16 scan: default settings `--led-cols=64 --led-rows=32 --led-chain=2 --led-pwm-bits=11 --led-slowdown-gpio=<per model>`.
- An optional physical button between GPIO 21 and GND.
- Pi audio is disabled and one core is reserved for the display (`isolcpus=3`): the installation does it.

## Update

- Toolkit **Mode 17** (easiest), or
- by hand: get `RecalBoxDMD_pi_payload.zip` from the latest `pi-v…` release and run `sudo bash install.sh` from the unpacked folder on the Pi.

`config.ini` and the card content are never overwritten.

## Build it yourself

The daemon is in [`raspberry-pi/rpi_daemon/`](raspberry-pi/rpi_daemon/) (C++17, CMake). It links against Henner Zeller's [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix):

```
git clone https://github.com/hzeller/rpi-rgb-led-matrix ~/rpi-rgb-led-matrix && make -C ~/rpi-rgb-led-matrix/lib
sudo apt install cmake g++ libpng-dev libgif-dev zlib1g-dev libcpp-httplib-dev
cd raspberry-pi/rpi_daemon && cmake -B build && cmake --build build -j3
```

Allow 10 to 25 minutes on a Pi 3B. `deploy/install.sh --build` does all of this and installs the service.

## License

The [`raspberry-pi/rpi_daemon/`](raspberry-pi/rpi_daemon/) folder is under **GPL-2.0-or-later** (the daemon is linked with rpi-rgb-led-matrix, GPL-2.0+): text in `raspberry-pi/rpi_daemon/LICENSE`, details in `LICENSE.md`. The distributed binary is built from this repository's sources. The rest of the repository (ESP32 firmware, toolkit) stays under the MIT license.

## Known limits

- A few defective panel LEDs stay visible whatever the setting (hardware).
- Recalbox native mode / Visual Pinball: validated with a simulator and by eye on the panels; no long test campaign.
- There is no automatic daemon OTA from GitHub yet: update with Mode 17 or by hand.
