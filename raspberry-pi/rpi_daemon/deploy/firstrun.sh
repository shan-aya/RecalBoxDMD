#!/bin/bash
# firstrun.sh -- premier demarrage d'une carte preparee par le toolkit (Mode 16) : installe le DMD HORS LIGNE.
# Lance par systemd via cmdline.txt (systemd.run=...) apres le premier redemarrage de Raspberry Pi OS (custom.toml). Aucun reseau a ce stade :
# tout ce dont le demon a besoin est dans /boot/firmware/recalboxdmd (binaire, pages web, bibliotheques lib/). Le journal reste dans firstrun.log.
BOOT=/boot/firmware
SRC="$BOOT/recalboxdmd"
mountpoint -q "$BOOT" || mount "$BOOT"
mount -o remount,rw "$BOOT"
mount -o remount,rw /
# JAMAIS de boucle : le crochet est retire de cmdline.txt AVANT tout le reste (si l'installation echoue, le Pi redemarre normalement et le Mode 17 reprendra)
sed -i -E 's/ systemd\.(run|run_success_action|run_failure_action|unit)=[^ ]+//g' "$BOOT/cmdline.txt"
sync
exec >> "$SRC/firstrun.log" 2>&1
date; echo "== firstrun : installation hors ligne du DMD"
bash "$SRC/install.sh" --offline --media RBPIDMD
rc=$?
echo "== install.sh rc=$rc"
sync
exit $rc
