#!/bin/ash
# ============================================
# dmd_vpx_config -- verifie / retablit les reglages Visual Pinball (VPX) utiles
# au DMD dans VPinballX-configgen.ini, au demarrage d'EmulationStation et
# apres chaque fin de partie.
# ============================================
# Version actuelle : v4
#
# v4 (suite) - fichier renomme dmd_vpx_config[start,endgame,rungame].sh : a CHAQUE lancement de jeu, pose pinball.externaldmd=zedmd dans recalbox.conf (voir prelaunch()), sans quoi Recalbox efface DMDUtil Enable.
#
# v4 - 2026-10-24 - safe-modify - BUG REEL (Pi, RB2) : ZeDMDWiFiAddr perimee (.52, ancien ESP32) jamais corrigee -> VPinball n'affichait rien sur le Pi (.26). (1) mode « option Pinball du DMD » : l'adresse du DMD
#   qui repond (/tmp/dmd_udp_ip) REMPLACE desormais une adresse deja saisie si elle differe ; (2) mode natif : dmd.zedmd.wifi (option ZeDMD de Recalbox) fait foi meme sans pinball.externaldmd=zedmd.
#
# v3 - 2026-10-11 - safe-modify - MODE RECALBOX NATIF / « external DMD : zedmd » : la configuration Visual Pinball de Recalbox 11 (vpinballGenerator.py, lue sur RB2 en
#   11.0-patron-1.1) n'ecrit que Enable=1 et ZeDMD=1 pour « zedmd » (ZeDMD USB, auto-detecte) : elle NE TRANSMET PAS l'adresse WiFi (dmd.zedmd.wifi n'est lu que par
#   EmulationStation pour la marquee) et ne nettoie jamais ZeDMDWiFiAddr (une adresse perimee reste : constate le 2026-10-11, .142 au lieu de .52). Tant que Recalbox ne le
#   fait pas, ce script le fait a sa place : si recalbox.conf contient pinball.externaldmd=zedmd ET dmd.zedmd.wifi=<IPv4>, il force [Plugin.DMDUtil] ZeDMDWiFiEnabled=1 et
#   ZeDMDWiFiAddr=<dmd.zedmd.wifi> (le reglage de Recalbox fait foi : l'utilisateur ne saisit l'adresse qu'a UN endroit). Ce mode est INDEPENDANT du firmware du DMD (pas
#   besoin de vpinball_dmd=1). Sinon, comportement v2 inchange.
#
# v2 - 2026-09-20 - safe-modify - Le script n'est ACTIF que si l'option "Mode
#   Pinball (VPX)" est cochee sur le DMD (demande utilisateur : cocher l'option
#   = accepter que Recalbox soit modifiee, sinon aucune ecriture). Le firmware
#   (v230) ajoute vpinball_dmd=0|1 au message FEATURES, ecrit par
#   dmd_udp_resync.py dans /tmp/dmd_features_cache. Cle absente (firmware plus
#   ancien) ou a 0 -> script inerte ; fichier pas encore recu -> attente (3 min).
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
FEATURES="${VPX_FEATURES:-/tmp/dmd_features_cache}"
DISABLE="/recalbox/share/userscripts/dmd_helpers/vpx_autoconfig.disabled"
LOG="${VPX_LOG:-/recalbox/share/system/logs/dmd_vpx_config.log}"
LOCK="/tmp/dmd_vpx_config.lock"
# ES appelle les scripts avec "-action <evenement> ..." (constate sur RB2) ; appel manuel : $1 = evenement
if [ "$1" = "-action" ]; then EVENT="${2:-?}"; else EVENT="${1:-manual}"; fi

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

# etat de l'option "Mode Pinball (VPX)" du DMD, relaye par le firmware (v230+) dans
# FEATURES -> /tmp/dmd_features_cache (cle vpinball_dmd=0|1). 1 = active, 0 = inactive
# (ou firmware plus ancien sans cette cle), 2 = pas encore recu.
feat_state() {
    [ -f "$FEATURES" ] || return 2
    v=$(sed -n 's/.*vpinball_dmd=\([01]\).*/\1/p' "$FEATURES" 2>/dev/null | head -n1)
    [ "$v" = "1" ] && return 1
    return 0
}

# v3 -- adresse du ZeDMD WiFi que RECALBOX utilise deja pour la marquee, quand « external DMD : zedmd » est choisi pour Visual Pinball ; vide sinon
RBCONF="${VPX_RBCONF:-/recalbox/share/system/recalbox.conf}"
rb_zedmd_addr() {
    # recalbox.conf ecrit « cle = valeur » (espaces autour du =, constate sur RB2) ou « cle=valeur » : les deux formes sont lues
    ext=$(sed -n 's/^pinball\.externaldmd[[:space:]]*=[[:space:]]*\(.*\)$/\1/p' "$RBCONF" 2>/dev/null | tail -n1 | tr -d '\r \t')
    a=$(sed -n 's/^dmd\.zedmd\.wifi[[:space:]]*=[[:space:]]*\(.*\)$/\1/p' "$RBCONF" 2>/dev/null | tail -n1 | tr -d '\r \t')
    is_ipv4 "$a" || return 1
    # v4 : l'adresse de l'option ZeDMD de Recalbox (dmd.zedmd.wifi) fait foi, avec ou sans « pinball.externaldmd=zedmd » (cle absente constatee sur RB2)
    echo "$a"
}

# attend la sortie de VPX (jamais d'ecriture pendant qu'une table tourne) ; renvoie 1 si VPX tourne encore apres 3 min
wait_vpx_exit() {
    n=0
    while vpx_running; do
        n=$((n + 1)); [ $n -gt 90 ] && return 1
        sleep 2
    done
    sleep 2
    return 0
}

run_check() {
    [ -f "$DISABLE" ] && return 0
    # v3 -- le reglage de Recalbox fait foi (voir l'en-tete) : actif meme sans l'option Pinball du DMD
    rbip=$(rb_zedmd_addr)
    if [ -n "$rbip" ]; then
        [ -f "$INI" ] || return 0
        wait_vpx_exit || { log "VPX toujours en cours apres 3 min : controle abandonne"; return 0; }
        [ -f "$INI.bak-dmd-autoconfig" ] || cp -p "$INI" "$INI.bak-dmd-autoconfig" 2>/dev/null
        ensure "Plugin.DMDUtil" "ZeDMDWiFiEnabled" "1"
        ensure "Plugin.DMDUtil" "ZeDMDWiFiAddr" "$rbip"
        return 0
    fi
    # SCRIPT INACTIF tant que l'option du DMD n'est pas cochee : aucune ecriture dans les fichiers de Recalbox
    feat_state; fs=$?
    [ $fs -eq 2 ] && return 3            # reglages du DMD pas encore recus : on attend
    [ $fs -eq 0 ] && return 0
    [ -f "$INI" ] || return 0            # cree par Recalbox au 1er lancement de VPX

    # ne jamais ecrire pendant qu'une table tourne (VPX reecrirait le fichier a sa fermeture)
    n=0
    while vpx_running; do
        n=$((n + 1)); [ $n -gt 90 ] && { log "VPX toujours en cours apres 3 min : controle abandonne"; return 0; }
        sleep 2
    done
    sleep 2

    [ -f "$INI.bak-dmd-autoconfig" ] || cp -p "$INI" "$INI.bak-dmd-autoconfig" 2>/dev/null

    # IP du DMD : v4 -- celle observee sur le reseau (le DMD qui repond a cette Recalbox) prime sur une adresse deja saisie, qui peut etre
    # perimee (constate sur RB2 : .52 = ancien ESP32 alors que le DMD est un Pi en .26 -> VPX envoyait ses images dans le vide)
    ip=$(tr -d '\r\n ' < "$IPFILE" 2>/dev/null)
    if ! is_ipv4 "$ip"; then
        ip=$(get_key "$INI" "Plugin.DMDUtil" "ZeDMDWiFiAddr")
        is_ipv4 "$ip" || ip=""
    fi

    ensure "Plugin.DMDUtil" "Enable" "1"
    ensure "Plugin.DMDUtil" "ZeDMDWiFiEnabled" "1"
    if [ -n "$ip" ]; then
        ensure "Plugin.DMDUtil" "ZeDMDWiFiAddr" "$ip"
    else
        return 2                          # IP inconnue pour l'instant
    fi
    ensure "Plugin.AlphaDMD" "Enable" "1"
    return 0
}

# v4 -- AVANT CHAQUE LANCEMENT de jeu (evenement rungame, script non « permanent » = ES attend sa fin) : Recalbox genere VPinballX-configgen.ini a chaque lancement
# et remet [Plugin.DMDUtil] Enable a vide sauf si pinball.externaldmd=zedmd (vpinballGenerator.py) : sans cela VPX n'envoie RIEN au DMD. On pose donc cette cle dans
# recalbox.conf juste avant (ES peut la reecrire depuis sa memoire : d'ou le controle a CHAQUE lancement). Jamais si l'utilisateur a choisi autre chose (pixelcade, dmdserver...),
# ni sans DMD concerne (option « Mode Pinball » cochee OU adresse dmd.zedmd.wifi renseignee). Rapide et sans attente : ES est bloque pendant ce temps.
prelaunch() {
    [ -f "$DISABLE" ] && return 0
    [ -f "$RBCONF" ] || return 0
    cur=$(sed -n 's/^pinball\.externaldmd[[:space:]]*=[[:space:]]*\(.*\)$/\1/p' "$RBCONF" 2>/dev/null | tail -n1 | tr -d '\r \t')
    case "$cur" in ""|none) ;; *) return 0 ;; esac
    a=$(rb_zedmd_addr)
    feat_state; fs=$?
    [ -z "$a" ] && [ $fs -ne 1 ] && return 0
    [ -f "$RBCONF.bak-dmd-externaldmd" ] || cp -p "$RBCONF" "$RBCONF.bak-dmd-externaldmd" 2>/dev/null
    tmp="$RBCONF.tmp.$$"
    if grep -q '^pinball\.externaldmd' "$RBCONF"; then
        sed 's/^pinball\.externaldmd[[:space:]]*=.*/pinball.externaldmd=zedmd/' "$RBCONF" > "$tmp"
    else
        { cat "$RBCONF"; [ -n "$(tail -c1 "$RBCONF")" ] && echo; echo "pinball.externaldmd=zedmd"; } > "$tmp"
    fi
    [ -s "$tmp" ] && cat "$tmp" > "$RBCONF"
    rm -f "$tmp"
    log "avant lancement : pinball.externaldmd '${cur}' -> 'zedmd' (sans cela Recalbox vide [Plugin.DMDUtil] Enable)"
    return 0
}
if [ "$EVENT" = "rungame" ]; then prelaunch; exit 0; fi

# --- point d'entree : tout le travail en arriere-plan, ES n'est jamais bloque ---
if [ "$DMD_VPX_CONFIG_CHILD" != "1" ]; then
    DMD_VPX_CONFIG_CHILD=1 setsid sh "$0" "$@" >/dev/null 2>&1 &
    exit 0
fi

mkdir "$LOCK" 2>/dev/null || exit 0       # une seule instance a la fois
trap 'rmdir "$LOCK" 2>/dev/null' EXIT

tries=0
while true; do
    run_check
    rc=$?
    [ $rc -ne 2 ] && [ $rc -ne 3 ] && break
    tries=$((tries + 1))
    if [ $tries -eq 1 ]; then
        [ $rc -eq 3 ] && log "reglages du DMD pas encore recus ($FEATURES) : j'attends" \
                      || log "IP du DMD inconnue (ni dans le .ini ni dans $IPFILE) : j'attends que le DMD se manifeste"
    fi
    if [ $tries -ge 36 ]; then
        [ $rc -eq 3 ] && log "reglages du DMD toujours absents apres 3 min : rien ecrit" \
                      || log "IP du DMD toujours inconnue apres 3 min : ZeDMDWiFiAddr non ecrit"
        break
    fi
    sleep 5
done
exit 0
