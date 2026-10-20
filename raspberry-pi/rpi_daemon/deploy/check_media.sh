#!/bin/bash
# check_media.sh -- controle au demarrage : la partition MEDIA du Pi doit s'appeler RBPIDMD (le toolkit la reconnait a ce nom, la carte DMD de
# l'ESP32 s'appelle RECALBOXDMD) ; si l'etiquette a change (renommee sous Windows, reformatee...), elle est restauree.
# Lance par le service du demon (ExecStartPre), apres le montage de /media/dmd. Ne bloque jamais le demarrage (toujours exit 0).
WANT=RBPIDMD
MNT=/media/dmd
DEV="$(findmnt -no SOURCE "$MNT" 2>/dev/null)" || exit 0
[ -n "$DEV" ] || exit 0
CUR="$(blkid -s LABEL -o value "$DEV" 2>/dev/null)"
[ "$CUR" = "$WANT" ] && exit 0
TYPE="$(blkid -s TYPE -o value "$DEV" 2>/dev/null)"
echo "[check_media] etiquette de $DEV : '$CUR' -> '$WANT' ($TYPE)"
case "$TYPE" in
  vfat) fatlabel "$DEV" "$WANT" || echo "[check_media] fatlabel a echoue" ;;
  exfat) if command -v exfatlabel >/dev/null 2>&1; then exfatlabel "$DEV" "$WANT" || echo "[check_media] exfatlabel a echoue"; else echo "[check_media] exfatlabel absent : etiquette non restauree"; fi ;;
  *) echo "[check_media] systeme de fichiers inattendu : $TYPE" ;;
esac
exit 0
