#!/bin/sh
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-30 - safe-modify - Creation initiale. Lance une rom fbneo
#   SANS AUCUNE simulation d'input -- juste le boot, l'utilisateur gere
#   credit/start lui-meme a la manette physique. Remplace definitivement
#   toute tentative d'automatisation du credit (3 motifs testes et
#   abandonnes la meme nuit -- voir DECISIONS.md "Suite immediate 20" :
#   pulse unique, maintien 60Hz, motif mame0278 2xSELECT+START -- aucun
#   ne fonctionne sur fbneo, confirme structurellement absent meme de
#   l'outillage officiel RecalBox NetCmd.py). A utiliser avec
#   rb2_fbneo_manual_wide_capture.py (capture passive qui detecte le
#   process lance par ce script).
#
#   Usage : sh launch_only.sh <rom>
#   Ex.   : sh launch_only.sh mtwins
export DISPLAY=:0
export XDG_RUNTIME_DIR=/run/user/0
ROM="$1"
DEVICE=$(python3 -c "
import sys
sys.path.insert(0, '/recalbox/share/system/rb_challenge_probe')
from rb2_input_device import detect_steam_deck_device
print(detect_steam_deck_device())
")
nohup python3 /usr/bin/emulatorlauncher.pyc \
  -p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name "Steam Deck" \
  -p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath "$DEVICE" \
  -p1physicalpath "pci-0000:04:00.4-usb-0:3:1.2" \
  -system fbneo -rom "/recalbox/share/roms/fbneo/arcade/${ROM}.zip" \
  -emulator libretro -core fbneo \
  -ratio auto -videobackend default -rotation 0 -resolution 1280x800 \
  -systemtype arcade > "/tmp/${ROM}_launch.log" 2>&1 &
echo "launched pid=$!"
