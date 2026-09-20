#!/bin/ash
# ============================================
# dmd_vpx_config -- verifie / retablit les reglages Visual Pinball (VPX) utiles
# au DMD dans VPinballX-configgen.ini, au demarrage d'EmulationStation et
# apres chaque fin de partie.
# ============================================
# Version actuelle : v1
#
# v1 - 2026-09-20 - safe-modify - Creation (demande utilisateur : "un script pour
#   verifier que ces reglages sont presents et les reecrire au demarrage de RB
#   s'ils manquent"). Reglages garantis :
#     [Plugin.DMDUtil]  Enable = 1
#                       ZeDMDWiFiEnabled = 1
#                       ZeDMDWiFiAddr = <IP du DMD>   (jamais ecrase s'il est deja renseigne)
#     [Plugin.AlphaDMD] Enable = 1                    (tables a afficheur a segments)
#   Pourquoi aussi a "endgame" : VPX REECRIT ce fichier a sa fermeture, depuis
#   sa memoire -- une modification faite pendant qu'une table tourne est perdue
#   (constate le 2026-09-20). Le script n'ecrit donc JAMAIS tant qu'un processus
#   VPinballX tourne : il attend sa sortie, puis controle.
#   IP du DMD : celle deja presente dans le fichier ; sinon /tmp/dmd_udp_ip
#   (ecrit par dmd_helpers/dmd_udp_resync.py des que le DMD se manifeste) ; a
#   defaut, ATTEND (max 3 min) plutot que d'inventer une adresse.
#   Ne modifie que ces cles, une copie .bak-dmd-autoconfig est faite une seule
#   fois. Desactivation : creer le fichier
#   /recalbox/share/userscripts/dmd_helpers/vpx_autoconfig.disabled
#   Journal : /recalbox/share/system/logs/dmd_vpx_config.log (une ligne
#   seulement quand une correction est appliquee ou qu'il faut attendre).

INI="${VPX_INI:-/recalbox/share/system/configs/vpinball/VPinballX-configgen.ini}"
IPFILE="${VPX_IPFILE:-/tmp/dmd_udp_ip}"
DISABLE="/recalbox/share/userscripts/dmd_helpers/vpx_autoconfig.disabled"
LOG="${VPX_LOG:-/recalbox/share/system/logs/dmd_vpx_config.log}"
LOCK="/tmp/dmd_vpx_config.lock"
EVENT="${1:-manual}"

log() { echo "$(date '+%Y-%m-%d %H:%M:%S') [$EVENT] $*" >> "$LOG" 2>/dev/null; }

# valeur d'une cle dans une section : get_key <fichier> <section> <cle>
get_key() {
    awk -v sec="[$2]" -v key="$3" '
        /^\[/ { insec = ($0 == sec); next }
        insec && $0 ~ ("^" key "[[:space:]]*=") {
            sub("^" key "[[:space:]]*=[[:space:]]*", ""); gsub(/[[:space:]\r]+$/, ""); print; exit
        }' "$1" 2>/dev/null
}

# fixe une cle (cree la cle, ou la section, si absente) : set_key <fichier> <section> <cle> <valeur>
set_key() {
    tmp="$1.tmp.$$"
    awk -v sec="[$2]" -v key="$3" -v val="$4" '
        BEGIN { insec = 0; done = 0; seen = 0 }
        /^\[/ {
            if (insec && !done) { print key " = " val; done = 1 }
            insec = ($0 == sec); if (insec) seen = 1
        }
        insec && !done && $0 ~ ("^" key "[[:space:]]*=") { print key " = " val; done = 1; next }
        { print }
        END {
            if (!done) {
                if (!seen) { print ""; print sec }
                print key " = " val
            }
        }' "$1" > "$tmp" && [ -s "$tmp" ] && cat "$tmp" > "$1"
    rm -f "$tmp"
}

is_ipv4() { echo "$1" | grep -Eq '^[0-9]{1,3}(\.[0-9]{1,3}){3}$'; }
vpx_running() { ps 2>/dev/null | grep -v grep | grep -q 'VPinballX'; }

# controle d'une cle : ensure <section> <cle> <valeur souhaitee> ; renvoie 0 si corrigee
ensure() {
    cur=$(get_key "$INI" "$1" "$2")
    [ "$cur" = "$3" ] && return 1
    set_key "$INI" "$1" "$2" "$3"
    log "corrige [$1] $2 : '${cur}' -> '$3'"
    return 0
}

run_check() {
    [ -f "$DISABLE" ] && return 0
    [ -f "$INI" ] || return 0            # cree par Recalbox au 1er lancement de VPX

    # ne jamais ecrire pendant qu'une table tourne (VPX reecrirait le fichier a sa fermeture)
    n=0
    while vpx_running; do
        n=$((n + 1)); [ $n -gt 90 ] && { log "VPX toujours en cours apres 3 min : controle abandonne"; return 0; }
        sleep 2
    done
    sleep 2

    [ -f "$INI.bak-dmd-autoconfig" ] || cp -p "$INI" "$INI.bak-dmd-autoconfig" 2>/dev/null

    # IP du DMD : celle deja saisie, sinon celle observee sur le reseau
    ip=$(get_key "$INI" "Plugin.DMDUtil" "ZeDMDWiFiAddr")
    if ! is_ipv4 "$ip"; then
        ip=$(tr -d '\r\n ' < "$IPFILE" 2>/dev/null)
        is_ipv4 "$ip" || ip=""
    fi

    ensure "Plugin.DMDUtil" "Enable" "1"
    ensure "Plugin.DMDUtil" "ZeDMDWiFiEnabled" "1"
    if [ -n "$ip" ]; then
        cur=$(get_key "$INI" "Plugin.DMDUtil" "ZeDMDWiFiAddr")
        is_ipv4 "$cur" || ensure "Plugin.DMDUtil" "ZeDMDWiFiAddr" "$ip"
    else
        return 2                          # IP inconnue pour l'instant
    fi
    ensure "Plugin.AlphaDMD" "Enable" "1"
    return 0
}

# --- point d'entree : tout le travail en arriere-plan, ES n'est jamais bloque ---
if [ "$DMD_VPX_CONFIG_CHILD" != "1" ]; then
    DMD_VPX_CONFIG_CHILD=1 setsid "$0" "$EVENT" >/dev/null 2>&1 &
    exit 0
fi

mkdir "$LOCK" 2>/dev/null || exit 0       # une seule instance a la fois
trap 'rmdir "$LOCK" 2>/dev/null' EXIT

tries=0
while true; do
    run_check
    rc=$?
    [ $rc -ne 2 ] && break
    tries=$((tries + 1))
    if [ $tries -eq 1 ]; then log "IP du DMD inconnue (ni dans le .ini ni dans $IPFILE) : j'attends que le DMD se manifeste"; fi
    [ $tries -ge 36 ] && { log "IP du DMD toujours inconnue apres 3 min : ZeDMDWiFiAddr non ecrit"; break; }
    sleep 5
done
exit 0
