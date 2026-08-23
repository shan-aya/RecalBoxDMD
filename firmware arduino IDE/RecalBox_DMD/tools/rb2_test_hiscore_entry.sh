#!/bin/sh
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Generalise le test
#   manuel valide ce soir sur batlzone (bind-mount temporaire par-dessus
#   /usr/share/libretro-mame/mame0278/plugins/hiscore/hiscore.dat --
#   necessaire car ce chemin est en lecture seule, overlay ro, voir
#   DECISIONS.md "Récolte multi-RB") : superpose un hiscore.dat corrige
#   SANS JAMAIS ecrire sur l'original, relance le jeu une fois via la
#   methode standard (launch/dwell/screenshot/quit), verifie si un .hi
#   apparait, puis DEMONTE systematiquement (meme en cas d'erreur) --
#   l'original redevient visible immediatement, garanti intact.
#
# Usage (a executer SUR RB2, via ssh) :
#   rb2_test_hiscore_entry.sh <rom> <chemin_dat_corrige>
#
# Le fichier corrige doit etre une copie COMPLETE du hiscore.dat original
# avec juste l'entree du jeu visee modifiee (voir find_value_in_ramdump.py
# pour obtenir la ligne candidate, et coller/remplacer a la main dans une
# copie de /usr/share/libretro-mame/mame0278/plugins/hiscore/hiscore.dat).
set -u
export DISPLAY=:0
export XDG_RUNTIME_DIR=/run/user/0

ROM="$1"
CORRECTED="$2"
DAT=/usr/share/libretro-mame/mame0278/plugins/hiscore/hiscore.dat
SAVE=/recalbox/share/saves/mame/mame0278/hiscore/${ROM}.hi

if [ -z "$ROM" ] || [ -z "$CORRECTED" ]; then
  echo "usage: rb2_test_hiscore_entry.sh <rom> <chemin_dat_corrige>"
  exit 1
fi
if [ ! -f "$CORRECTED" ]; then
  echo "!! fichier corrige introuvable: $CORRECTED"
  exit 1
fi

# nettoyage de securite : demonte tout bind-mount hiscore.dat residuel
# d'un test precedent qui aurait mal termine
if mount | grep -q " $DAT "; then
  echo "-- demontage residuel detecte, nettoyage --"
  umount "$DAT" 2>&1
fi

echo "--- .hi AVANT (etat de reference) ---"
ls -la "$SAVE" 2>&1

echo "--- bind-mount du dat corrige (reversible) ---"
mount --bind "$CORRECTED" "$DAT"
if [ $? -ne 0 ]; then
  echo "!! echec du bind-mount, abandon (original non touche)"
  exit 1
fi

cleanup() {
  umount "$DAT" 2>&1
  echo "--- demontage effectue, original restaure ---"
}
trap cleanup EXIT

echo "--- lancement direct ${ROM} (dat corrige actif) ---"
python3 /usr/bin/emulatorlauncher.pyc \
  -p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name "Steam Deck" \
  -p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath /dev/input/event14 \
  -p1physicalpath "pci-0000:04:00.4-usb-0:3:1.2" \
  -system mame -rom /recalbox/share/roms/mame/mame0278/${ROM}.zip -emulator libretro -core mame0278 \
  -ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade \
  > /tmp/test_hiscore_entry_${ROM}.log 2>&1 &
LPID=$!
sleep 1
PID=""
i=0
while [ $i -lt 20 ]; do
  PID=$(ps -o pid,args -ww | grep -- "/${ROM}.zip" | grep retroarch | grep -v grep | awk '{print $1}')
  if [ -n "$PID" ]; then break; fi
  sleep 1
  i=$((i + 1))
done
echo "retroarch pid=$PID"

if [ -n "$PID" ]; then
  sleep 15
  python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.sendto(b'QUIT\n', ('127.0.0.1', 55355))
s.close()
"
  i=0
  while [ $i -lt 15 ]; do
    kill -0 $PID 2>/dev/null || break
    sleep 1
    i=$((i + 1))
  done
  kill -0 $PID 2>/dev/null && kill -9 $PID
else
  echo "!! retroarch jamais apparu"
  kill $LPID 2>/dev/null
fi

sleep 2
echo "--- .hi APRES (dat corrige) ---"
ls -la "$SAVE" 2>&1
if [ -f "$SAVE" ]; then
  echo "SUCCES -- .hi genere :"
  od -An -tx1 "$SAVE" | head -5
else
  echo "ECHEC -- toujours pas de .hi avec cette correction"
fi
