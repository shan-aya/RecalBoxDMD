#!/bin/sh
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-26 - safe-modify - Creation initiale (reconstruit depuis
#   une session Claude, fichier original perdu suite a un redemarrage
#   RB2 qui vide /tmp -- voir DECISIONS.md "Suite immediate (17)").
#
#   Lance un jeu sur le core fbneo et enregistre en continu (RAM a
#   l'adresse ADDR via READ_CORE_RAM + captures ecran) PENDANT que
#   l'utilisateur joue REELLEMENT (insertion credit+start manuelle --
#   aucun mecanisme automatique fiable trouve pour fbneo cette session :
#   PLAYER1_SELECT/START, rafales, L2/R2/L/R tous testes sans effet
#   malgre un core qui expose bien "P1 Coin"/"P1 Start" comme entrees
#   distinctes, log verbose a l'appui -- cause exacte non identifiee).
#
#   IMPORTANT (retour utilisateur direct, a ne JAMAIS reproduire) : NE
#   JAMAIS couper la partie de force sur une duree fixe -- l'ancienne
#   version (v0, non versionnee) tuait le processus des qu'une boucle de
#   N iterations s'terminait, MEME SI l'utilisateur etait encore en
#   train de chercher le bon bouton ou de jouer ("tu ne me laisses
#   jamais le temps de jouer"). Ce script tourne longtemps (10 min max)
#   et NE COUPE JAMAIS de lui-meme -- utiliser rb2_fbneo_stop.sh
#   separement, uniquement quand l'utilisateur signale explicitement
#   avoir fini.
#
#   Usage : sh rb2_fbneo_manual_capture.sh <rom> <adresse_hex_READ_CORE_RAM>
#   Ex.   : sh rb2_fbneo_manual_capture.sh inthunt 11000
export DISPLAY=:0
export XDG_RUNTIME_DIR=/run/user/0
ROM="${1:-inthunt}"
ADDR="${2:-11000}"
rm -f /recalbox/share/screenshots/${ROM}-*.png
rm -f /tmp/fbneo_manual_log.txt
DEVICE=$(python3 -c "
import sys
sys.path.insert(0, '/tmp')
from rb2_input_device import detect_steam_deck_device
print(detect_steam_deck_device())
")

python3 /usr/bin/emulatorlauncher.pyc \
  -p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name "Steam Deck" \
  -p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath "$DEVICE" \
  -p1physicalpath "pci-0000:04:00.4-usb-0:3:1.2" \
  -system fbneo -rom /recalbox/share/roms/fbneo/arcade/$ROM.zip -emulator libretro -core fbneo \
  -ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade > /dev/null 2>&1 &
sleep 8
PID=$(ps -o pid,args -ww | grep -- "/$ROM.zip" | grep retroarch | grep -v grep | awk '{print $1}' | head -n1)
echo "$PID" > /tmp/fbneo_manual_pid.txt
echo "pid=$PID -- PRET, prends tout ton temps, aucune coupure automatique"

netcmd() {
  python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.sendto(b'$1\n', ('127.0.0.1', 55355))
"
}

readmem() {
  python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(1.0)
s.sendto(b'$1 $2 $3\n', ('127.0.0.1', 55355))
try:
    data, _ = s.recvfrom(4096)
    print(data.decode(errors='replace').strip())
except socket.timeout:
    print('$1 $2 TIMEOUT')
"
}

# 10 minutes max (200 x 3s), PAS de QUIT/kill ici -- voir rb2_fbneo_stop.sh
i=0
while [ $i -lt 200 ]; do
  echo "=== t=$((i * 3))s ===" >> /tmp/fbneo_manual_log.txt
  readmem READ_CORE_RAM $ADDR 8 >> /tmp/fbneo_manual_log.txt
  netcmd SCREENSHOT
  sleep 3
  if [ ! -f /tmp/fbneo_manual_pid.txt ]; then
    echo "arret demande (fichier pid supprime)" >> /tmp/fbneo_manual_log.txt
    break
  fi
  i=$((i+1))
done
echo "boucle d'enregistrement terminee (jeu toujours actif, PAS coupe)"
