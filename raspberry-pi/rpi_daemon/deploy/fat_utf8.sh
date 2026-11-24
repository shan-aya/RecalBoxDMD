#!/bin/bash
# fat_utf8.sh -- v3.93 : monte la partition MEDIA du Pi en iocharset=utf8 (noms de fichiers accentues / espace insecable U+00A0 des ROM vpinball), ou revient a l'ancien montage.
#   sudo fat_utf8.sh [--sd /media/dmd]          active iocharset=utf8
#   sudo fat_utf8.sh --ascii [--sd /media/dmd]  retire l'option (retour au montage d'origine, ascii par defaut du noyau)
#   fat_utf8.sh --status                         affiche utf8 / ascii / na (rien a faire : racine hors FAT, exFAT...)
# Sans effet (« ok:deja ») si le montage est deja bon. Sauvegarde /etc/fstab, arrete le service le temps de demonter/remonter (l'ecran s'eteint ~10 s), restaure et relance en cas d'echec.
# Resultat ecrit dans <prefix>/fat_utf8_last : en-cours / ok / ok:deja / echec:<raison>. Un remontage a chaud (mount -o remount) est ignore par le noyau pour iocharset : il faut demonter.
PREFIX=/opt/recalboxdmd
SD=""; [ -r /etc/default/recalboxdmd ] && SD="$(sed -n 's/^DMD_SD=//p' /etc/default/recalboxdmd | tail -n1)"
MODE=utf8
while [ $# -gt 0 ]; do
  case "$1" in
    --sd) SD="$2"; shift ;;
    --ascii) MODE=ascii ;;
    --status) MODE=status ;;
  esac
  shift
done
[ -n "$SD" ] || SD=/media/dmd
MP="$(findmnt -no TARGET -T "$SD" 2>/dev/null)"; [ -n "$MP" ] && SD="$MP"   # le dossier de contenu peut etre un lien : on travaille sur le POINT DE MONTAGE reel
LAST="$PREFIX/fat_utf8_last"
say() { echo "[fat_utf8] $*"; }
state() { findmnt -no FSTYPE,OPTIONS -T "$SD" 2>/dev/null | awk '{ if ($1!="vfat" && $1!="msdos") print "na"; else if ($2 ~ /(^|,)iocharset=utf8(,|$)/) print "utf8"; else print "ascii" }'; }
if [ "$MODE" = status ]; then state; exit 0; fi
fail() { say "ECHEC : $1"; echo "echec:$1" > "$LAST" 2>/dev/null; exit 1; }
[ "$(id -u)" = 0 ] || { echo "echec:root"; exit 1; }
mountpoint -q "$SD" || fail "non-monte"
ST="$(state)"
case "$ST" in na) say "partition hors FAT (exFAT, ext4...) : rien a faire"; echo "ok:deja" > "$LAST"; exit 0 ;; esac
WANT=$MODE
[ "$ST" = "$WANT" ] && { say "deja en $WANT"; echo "ok:deja" > "$LAST"; exit 0; }
grep -qE "^[^#]*[[:space:]]$SD[[:space:]]" /etc/fstab || fail "pas-de-ligne-fstab"
echo "en-cours" > "$LAST"
BAK="/etc/fstab.bak_fat_utf8_$(date +%Y%m%d_%H%M%S)"
cp -a /etc/fstab "$BAK" || fail "sauvegarde-fstab"
# options du champ 4 : retire iocharset=..., ajoute iocharset=utf8 (mode utf8)
awk -v sd="$SD" -v mode="$MODE" '
  /^[[:space:]]*#/ || NF < 4 || $2 != sd { print; next }
  { n = split($4, o, ","); r = ""
    for (i = 1; i <= n; i++) if (o[i] !~ /^iocharset=/ && o[i] != "") r = r (r == "" ? "" : ",") o[i]
    if (mode == "utf8") r = r (r == "" ? "" : ",") "iocharset=utf8"
    $4 = r; print }' /etc/fstab > /etc/fstab.new_fat_utf8 && cat /etc/fstab.new_fat_utf8 > /etc/fstab && rm -f /etc/fstab.new_fat_utf8 || { cp -a "$BAK" /etc/fstab; fail "ecriture-fstab"; }
WAS_ACTIVE=0; systemctl is-active --quiet recalboxdmd && WAS_ACTIVE=1
restore() { cp -a "$BAK" /etc/fstab; systemctl daemon-reload; mountpoint -q "$SD" || mount "$SD"; [ "$WAS_ACTIVE" = 1 ] && systemctl start recalboxdmd; }
[ "$WAS_ACTIVE" = 1 ] && systemctl stop recalboxdmd
sync
umount "$SD" 2>/dev/null || { sleep 2; umount "$SD" 2>/dev/null; } || { restore; fail "occupe"; }
systemctl daemon-reload
mount "$SD" || { restore; fail "montage"; }
[ "$(state)" = "$WANT" ] || { umount "$SD" 2>/dev/null; restore; fail "option-non-prise"; }
[ "$WAS_ACTIVE" = 1 ] && systemctl start recalboxdmd
say "partition $SD montee en $WANT (sauvegarde fstab : $BAK)"
echo "ok" > "$LAST"
exit 0