#!/bin/ash
# Pont widgets « Recalbox » -> DMD (Challenge en cours, Recompenses, Sante), en UDP.
#
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-10-13 - safe-modify - Creation (firmware v315). Toutes les 2 minutes (et des le
#   demarrage) lance dmd_helpers/dmd_rbwidgets.py, qui lit les fichiers de la Recalbox et
#   envoie 3 datagrammes « CMD=rbw ARG=C|... / R|... / H|... » au DMD. Le DMD garde la
#   derniere valeur en RAM : un DMD redemarre attend au plus 2 minutes pour retrouver ses
#   widgets. Aucun envoi tant qu'aucun DMD ne s'est manifeste (/tmp/dmd_udp_ip absent).
. /recalbox/share/userscripts/dmd_helpers/singleton_lock.sh dmd_rbwidgets 2>/dev/null || exit 1

LOG="/recalbox/share/system/logs/dmd_rbwidgets.log"
echo "$(date) - DMD rbwidgets bridge started (v1)" >> "$LOG"

while true; do
    if [ -f /tmp/dmd_udp_ip ]; then
        python3 /recalbox/share/userscripts/dmd_helpers/dmd_rbwidgets.py 2>>"$LOG"
    fi
    sleep 120
done
