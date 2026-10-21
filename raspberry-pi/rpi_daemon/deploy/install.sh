#!/bin/bash
# install.sh -- installe le demon RecalBox DMD sur un Raspberry Pi (Raspberry Pi OS Lite, Bookworm, 32 bits).
#
# Usage (en root, depuis le dossier qui contient ce script) :
#     sudo ./install.sh                  installe le binaire fourni (bin/dmd_daemon) et le service
#     sudo ./install.sh --build          compile d'abord le demon depuis les sources (hzeller + cmake, ~8 min sur un Pi 3B)
#     sudo ./install.sh --rtc            ajoute l'horloge RTC DS1307 de la carte d'interface (dtoverlay=i2c-rtc,ds1307)
#     sudo ./install.sh --sd /chemin     dossier du contenu (defaut : /opt/recalboxdmd/sd)
#     sudo ./install.sh --no-start       n'active pas le service tout de suite (le premier demarrage le fera)
#     sudo ./install.sh --offline        SANS reseau (premier demarrage d'une carte preparee par le toolkit) : pas d'apt, pas de demarrage du service
#     sudo ./install.sh --media ETIQUETTE  le contenu est sur la partition FAT32 d'etiquette ETIQUETTE, montee dans /media/dmd (fstab)
#
# Idempotent : on peut le relancer pour mettre le demon a jour ; config.ini et le contenu existants ne sont JAMAIS ecrases.
# Un redemarrage est necessaire la premiere fois (isolcpus, son coupe) ; le script le dit en fin d'execution.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX=/opt/recalboxdmd
SD="$PREFIX/sd"
BUILD=0; RTC=0; START=1; OFFLINE=0; MEDIA=""
HZELLER_REPO=https://github.com/hzeller/rpi-rgb-led-matrix.git
HZELLER_COMMIT=14464b3ad8974665f75a3c82355cf872311fec21   # version validee au jalon 0

while [ $# -gt 0 ]; do
  case "$1" in
    --build) BUILD=1 ;;
    --rtc) RTC=1 ;;
    --no-start) START=0 ;;
    --offline) OFFLINE=1 ;;
    --media) MEDIA="$2"; shift ;;
    --sd) SD="$2"; shift ;;
    -h|--help) sed -n '2,13p' "$0"; exit 0 ;;
    *) echo "option inconnue : $1" >&2; exit 2 ;;
  esac
  shift
done

[ "$(id -u)" = 0 ] || { echo "a lancer en root : sudo $0" >&2; exit 1; }
say() { printf '\n== %s\n' "$*"; }

BOOT=/boot/firmware; [ -d "$BOOT" ] || BOOT=/boot
NEED_REBOOT=0
[ -n "$MEDIA" ] && SD=/media/dmd

if [ "$OFFLINE" = 0 ]; then
  say "Paquets"
  export DEBIAN_FRONTEND=noninteractive
  apt-get update -qq
  # network-manager / dnsmasq-base / iproute2 sont deja dans Raspberry Pi OS ; les bibliotheques giflib et httplib du demon sont livrees dans lib/
  apt-get install -y -qq network-manager dnsmasq-base iproute2 >/dev/null
  if [ "$BUILD" = 1 ]; then
    apt-get install -y -qq build-essential cmake git libgif-dev libpng-dev zlib1g-dev libcpp-httplib-dev >/dev/null
  fi
fi

say "Binaire du demon"
if [ "$BUILD" = 1 ]; then
  SRC="$HERE/.."; [ -f "$HERE/CMakeLists.txt" ] && SRC="$HERE"
  [ -f "$SRC/CMakeLists.txt" ] || { echo "sources introuvables (CMakeLists.txt)" >&2; exit 1; }
  RGB=/opt/rpi-rgb-led-matrix
  if [ ! -d "$RGB/.git" ]; then git clone -q "$HZELLER_REPO" "$RGB"; fi
  git -C "$RGB" checkout -q "$HZELLER_COMMIT" || echo "(commit hzeller $HZELLER_COMMIT indisponible : version courante conservee)"
  make -C "$RGB/lib" -j"$(nproc)" >/dev/null
  mkdir -p "$SRC/build"
  ( cd "$SRC/build" && cmake .. -DRGBMATRIX_DIR="$RGB" >/dev/null && make -j"$(nproc)" dmd_daemon )
  BIN="$SRC/build/dmd_daemon"; WEB="$SRC/web"
else
  BIN="$HERE/bin/dmd_daemon"; WEB="$HERE/web"
fi
[ -f "$BIN" ] || { echo "binaire absent : $BIN (utilisez --build pour le compiler)" >&2; exit 1; }
[ -d "$WEB" ] || { echo "pages web absentes : $WEB" >&2; exit 1; }

say "Installation dans $PREFIX"
install -d "$PREFIX/bin" "$PREFIX/web"
systemctl stop recalboxdmd 2>/dev/null || true
install -m 0755 "$BIN" "$PREFIX/bin/dmd_daemon"
cp -f "$WEB"/* "$PREFIX/web/"
# bibliotheques du demon absentes de Raspberry Pi OS (libgif.so.7, libcpp-httplib.so.0.11) : livrees avec le demon (LD_LIBRARY_PATH dans le service)
if [ -d "$HERE/lib" ]; then install -d "$PREFIX/lib"; cp -f "$HERE/lib"/* "$PREFIX/lib/"; fi
# toute bibliotheque manquante est une erreur FRANCHE ici, pas un service qui boucle en silence
if MISSING="$(LD_LIBRARY_PATH="$PREFIX/lib" ldd "$PREFIX/bin/dmd_daemon" 2>/dev/null | grep "not found")"; then
  echo "ERREUR : bibliotheques manquantes pour le demon :" >&2; echo "$MISSING" >&2; exit 1
fi
install -m 0644 "$HERE/recalboxdmd.service" /etc/systemd/system/recalboxdmd.service
[ -f "$HERE/README.md" ] && install -m 0644 "$HERE/README.md" "$PREFIX/README.md"
# mise a jour depuis la page web (bouton « Mettre a jour ») : script + numero de version installee (compare a la derniere release pi-v...)
[ -f "$HERE/dmd_update.py" ] && install -m 0755 "$HERE/dmd_update.py" "$PREFIX/bin/dmd_update.py"
PIV="$HERE/PI_VERSION"; [ -f "$PIV" ] || PIV="$HERE/../PI_VERSION"     # charge utile : a cote d'install.sh ; sources (--build) : a la racine de rpi_daemon
[ -f "$PIV" ] && install -m 0644 "$PIV" "$PREFIX/PI_VERSION"
[ -f /etc/default/recalboxdmd ] || cat > /etc/default/recalboxdmd <<EOF
# Reglages du service recalboxdmd (voir l'unite systemd)
DMD_SD=$SD
# options supplementaires du demon, par exemple : DMD_OPTS="--led-brightness=60"
DMD_OPTS=""
EOF

say "Contenu ($SD)"
install -d "$SD"
if [ -n "$MEDIA" ]; then   # partition media du Pi (preparee par le toolkit), montee dans $SD
  # identifiee par son PARTUUID, PAS par son nom : un renommage (Windows...) ne doit jamais casser le montage ; check_media.sh restaure l'etiquette a chaque demarrage
  DEVM="$(blkid -L "$MEDIA" 2>/dev/null || blkid -L RECALBOXDMD 2>/dev/null || true)"
  if [ -n "$DEVM" ]; then SRCM="PARTUUID=$(blkid -s PARTUUID -o value "$DEVM")"; else SRCM="LABEL=$MEDIA"; fi
  sed -i "\|[[:space:]]$SD[[:space:]]|d" /etc/fstab
  printf '%s  %s  auto  defaults,noatime,nofail,umask=000,x-systemd.device-timeout=10  0  0\n' "$SRCM" "$SD" >> /etc/fstab   # vfat (FAT32) ou exfat : detecte
  systemctl daemon-reload 2>/dev/null || true
  mountpoint -q "$SD" || mount "$SD" 2>/dev/null || echo "(partition media pas encore montable : montee au prochain demarrage)"
  install -d /etc/systemd/system/recalboxdmd.service.d
  printf '[Unit]\nRequiresMountsFor=%s\n[Service]\nExecStartPre=-/opt/recalboxdmd/bin/check_media.sh\n' "$SD" > /etc/systemd/system/recalboxdmd.service.d/media.conf
  [ -f "$HERE/check_media.sh" ] && install -m 0755 "$HERE/check_media.sh" "$PREFIX/bin/check_media.sh"
fi
if [ ! -f "$SD/config.ini" ]; then
  # premier demarrage : sans playlist ni widgets, le demon affiche l'ecran de configuration (voir DECISIONS.md, jalon 7)
  if [ -f "$HERE/config.ini.seed" ]; then cp "$HERE/config.ini.seed" "$SD/config.ini"; else printf 'first_boot=1\n' > "$SD/config.ini"; fi
  echo "config.ini cree (first_boot=1)"
else
  echo "config.ini existant conserve"
fi

# le dossier de contenu appartient a l'utilisateur qui lance sudo (le toolkit y copie les fichiers par SFTP, sans etre root)
# (PAS sur une partition FAT32 / exFAT : chown n'y a aucun effet, l'acces vient des options de montage, et le parcours recursif de dizaines de milliers de fichiers bloquait l'installation
# pendant des minutes sur une carte deja remplie -- constate le 2026-10-20 sur la partition media de 54 Go)
FSTYPE="$(findmnt -no FSTYPE -T "$SD" 2>/dev/null || true)"
case "$FSTYPE" in
  vfat|msdos|exfat|fuseblk|ntfs*) ;;
  *) if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ]; then chown -R "$SUDO_USER":"$(id -gn "$SUDO_USER")" "$SD" 2>/dev/null || true; fi ;;
esac

say "Reseau (NetworkManager)"
install -d /etc/NetworkManager/conf.d
cat > /etc/NetworkManager/conf.d/wifi-powersave-off.conf <<'EOF'
[connection]
wifi.powersave = 2
EOF
systemctl enable NetworkManager >/dev/null 2>&1 || true

say "Affichage (anti-scintillement hzeller)"
# 1) son de la carte coupe : le module snd_bcm2835 utilise le meme bloc materiel que le pilote des dalles
echo "blacklist snd_bcm2835" > /etc/modprobe.d/blacklist-rgb-matrix.conf
if ! grep -q '^dtparam=audio=off' "$BOOT/config.txt"; then
  sed -i 's/^dtparam=audio=on/dtparam=audio=off/' "$BOOT/config.txt"
  grep -q '^dtparam=audio=off' "$BOOT/config.txt" || sed -i '0,/^\[all\]/s//dtparam=audio=off\n[all]/' "$BOOT/config.txt"
  NEED_REBOOT=1
fi
# 1b) ecran de la Raspberry Pi inutile (boitier sans ecran) : le pilote graphique KMS consomme memoire et temps de cycle ; reglage valide au jalon 0
if grep -q '^dtoverlay=vc4-f\?kms-v3d' "$BOOT/config.txt"; then
  sed -i 's/^\(dtoverlay=vc4-f\?kms-v3d.*\)$/#\1  # desactive par RecalBox DMD/' "$BOOT/config.txt"
  NEED_REBOOT=1
fi
sed -i 's/^camera_auto_detect=1/camera_auto_detect=0/; s/^display_auto_detect=1/display_auto_detect=0/' "$BOOT/config.txt"
# 2) coeur 3 reserve au pilote des dalles
if ! grep -q 'isolcpus=' "$BOOT/cmdline.txt"; then
  sed -i '1s/$/ isolcpus=domain,managed_irq,3 nohz_full=3 rcu_nocbs=3 irqaffinity=0,1,2/' "$BOOT/cmdline.txt"
  NEED_REBOOT=1
fi
# 3) horloge RTC de la carte d'interface (heure conservee hors ligne)
if [ "$RTC" = 1 ] && ! grep -q 'i2c-rtc,ds1307' "$BOOT/config.txt"; then
  printf '\n# RecalBox DMD : horloge RTC DS1307 de la carte d interface\ndtparam=i2c_arm=on\ndtoverlay=i2c-rtc,ds1307\n' >> "$BOOT/config.txt"
  apt-get remove -y -qq fake-hwclock >/dev/null 2>&1 || true
  NEED_REBOOT=1
fi

say "Service"
systemctl daemon-reload
systemctl enable recalboxdmd >/dev/null
[ "$OFFLINE" = 1 ] && START=0
if [ "$START" = 1 ] && [ "$NEED_REBOOT" = 0 ]; then
  systemctl restart recalboxdmd
  # verification : le service doit rester actif quelques secondes (sinon l'ecran du DMD resterait vide sans explication)
  OK=0
  for i in 1 2 3 4 5 6 7 8; do
    sleep 1
    if systemctl is-active --quiet recalboxdmd && [ "$(systemctl show -p NRestarts --value recalboxdmd)" = 0 ]; then OK=$((OK+1)); else OK=0; fi
  done
  if [ "$OK" -lt 4 ]; then
    echo "ERREUR : le service recalboxdmd ne reste pas actif. Dernieres lignes du journal :" >&2
    journalctl -u recalboxdmd -n 15 --no-pager >&2 || true
    exit 1
  fi
  echo "service demarre : journalctl -u recalboxdmd -f"
fi
if [ "$NEED_REBOOT" = 1 ]; then
  echo
  echo "REDEMARRAGE NECESSAIRE (parametres de demarrage modifies) : sudo reboot"
  echo "Le service demarrera tout seul ensuite."
fi
echo
echo "Termine. Page de configuration : http://$(hostname).local/ (ou l'adresse affichee sur le DMD)."
