#!/bin/sh
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-26 - safe-modify - Creation initiale (reconstruit depuis
#   une session Claude, voir rb2_fbneo_manual_capture.sh et DECISIONS.md
#   "Suite immediate (17)"). Arrete proprement une session lancee par
#   rb2_fbneo_manual_capture.sh -- a n'appeler QUE sur demande explicite
#   de l'utilisateur (il a fini de jouer), jamais automatiquement/sur
#   une duree fixe.
export DISPLAY=:0
export XDG_RUNTIME_DIR=/run/user/0
python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.sendto(b'QUIT\n', ('127.0.0.1', 55355))
"
sleep 3
if [ -f /tmp/fbneo_manual_pid.txt ]; then
  PID=$(cat /tmp/fbneo_manual_pid.txt)
  kill -9 "$PID" 2>/dev/null
  rm -f /tmp/fbneo_manual_pid.txt
fi
sleep 2
echo "--- log RAM ---"
cat /tmp/fbneo_manual_log.txt
echo "--- screenshots produits ---"
ls /recalbox/share/screenshots/*.png 2>/dev/null | wc -l
echo "--- etat processus ---"
ps aux | grep -E 'retroarch|emulatorlauncher' | grep -v grep
