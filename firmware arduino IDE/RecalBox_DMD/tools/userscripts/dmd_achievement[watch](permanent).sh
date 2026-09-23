#!/bin/ash
# Pont succes RetroAchievements -> DMD, en UDP (commande "CMD=score
# ARG=SUCCES|<nom>" sur le port 5005 du DMD, architecture "DMD bete" v110 --
# voir RecalBox_DMD.ino)
#
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v8
#
# v8 - 2026-09-23 - safe-modify - Nettoyage MQTT (demande utilisateur) : MQTT
#   n'existe plus cote DMD depuis RecalBox_DMD.ino v209. Retires :
#   features_watcher() (mosquitto_sub sur marquee/status/features, plus
#   jamais publie -- le cache /tmp/dmd_features_cache est tenu par
#   dmd_udp_resync.py, message FEATURES: du DMD) et le mosquitto_pub vers
#   marquee/cmd (seul send_udp() atteint le DMD). Journal renomme
#   dmd_achievement_mqtt.log -> dmd_achievement.log. Fonctionnement inchange :
#   suivi de retroarch.log, "Awarding achievement", gating ra_ingame.
#   En-tete "Version actuelle" remis a jour (restait a v6 malgre la v7).
#
# v7 - 2026-09-19 - safe-modify - IP du DMD decouverte dynamiquement
#   (/tmp/dmd_udp_ip, ecrit par dmd_udp_resync.py v5), voir plus bas.
#
# v6 - 2026-09-08 - safe-modify - Piste UDP (demande utilisateur explicite,
#   apres revue : "qu'est-ce qu'on aurait pu oublier de mettre a jour ?").
#   Ce script n'avait jamais ete touche lors de la bascule full UDP
#   (marquee.sh v45/dmd_score.sh v48) -- ses notifications de succes
#   RetroAchievements ne partaient plus que via mosquitto_pub, mort depuis
#   RecalBox_DMD.ino v161 (MQTT_ENABLED=false). Ajoute send_udp() (meme
#   motif que les 2 autres scripts, duplique pas factorise), appele EN
#   PARALLELE du mosquitto_pub existant -- MQTT inchange, rien coupe.
#   PAS ENCORE TESTE SUR MATERIEL.
#
# v5 - 2026-09-04 - safe-modify - Verrou anti-relance extrait vers
#   dmd_helpers/singleton_lock.sh, voir marquee.sh v43 pour le detail
#   complet (meme bloc duplique a l'identique dans les 3 scripts, nettoyage
#   differe puis repris ce soir). Comportement au runtime inchange (meme
#   LOCKDIR "dmd_achievement_singleton", meme logique mkdir/pid/kill -0).
#
# v4 - 2026-09-02 - safe-modify - Topic marquee/cmd/score -> marquee/cmd
#   unique (voir DECISIONS.md + RecalBox_DMD.ino v148, meme motif que
#   marquee.sh v40/dmd_score.sh v39) : 12 topics fusionnes en 1 seul cote
#   DMD pour reduire l'exposition au blocage TX MQTT post-CONNACK. Payload
#   prefixe "CMD=score ARG=" au lieu d'etre publie brut sur marquee/cmd/score
#   -- le contenu "SUCCES|<nom>" lui-meme est inchange.
#
# v2 - 2026-08-20 - safe-modify - Migration vers l'architecture "DMD bete"
#   (voir dmd_score[...].sh pour le raisonnement complet, memoire projet
#   project_core_reassignment_rb_script_mismatch.md) :
#   1. Topic redirige vers marquee/cmd/score -- marquee/cmd/achievement
#      n'existe plus cote firmware (retire en v104 avec le reste du
#      sous-systeme overlay, jamais reintroduit). Payload prefixe
#      "SUCCES|<nom>" pour rester lisible sur l'ecran generique MODE_SCORE
#      (pas de mise en forme dediee cote firmware, contrairement a l'ancien
#      systeme -- coherent avec la philosophie "DMD bete").
#   2. Gating sur feat_ra_ingame (voir feat_enabled()/features_watcher(),
#      identique a dmd_score[...].sh) -- un succes debloque en dehors d'une
#      partie n'a de toute facon pas de sens (RetroArch ne tourne qu'en
#      jeu), le contexte "browse" n'est donc jamais verifie ici.
#   3. Verrou PID ajoute (absent jusqu'ici sur CE fichier precisement --
#      cause reelle d'une fuite de zombies massive documentee le 2026-08-19,
#      voir memoire projet : ce script tournait en plusieurs instances
#      simultanees, jamais protege malgre le meme risque deja corrige sur
#      marquee[...].sh v5).
#
# v1 - 2026-08-16 - safe-modify - Creation initiale. Voir _backups/ pour le
#   detail complet (mecanisme tail -F sur retroarch.log inchange ci-dessous).
# ============================================
#
# Script SEPARE (ne modifie aucun autre script) : n'ecoute PAS
# Recalbox/EmulationStation/Event, suit en DIRECT
# /recalbox/share/system/logs/retroarch.log via `tail -F`, publie chaque
# succes debloque des la ligne "Awarding achievement" ecrite par RCHEEVOS
# (confirme en reel : apparait PENDANT que RetroArch tourne encore, pas
# seulement a sa fermeture).
#
# PREREQUIS (config RetroArch, PAS ce script) :
# /recalbox/share/system/configs/retroarch/retroarchcustom.cfg :
#   log_to_file = true
#   log_dir = /recalbox/share/system/logs
#   log_verbosity = true
#   cheevos_verbose_enable = true
# Sans log_to_file=true, retroarch.log n'existe pas et ce script n'a rien a
# suivre (tail reste silencieux, pas d'erreur bruyante).
#
# tail -F (majuscule) gere nativement la troncature/recreation du fichier a
# CHAQUE nouveau lancement RetroArch (fichier REECRIT, pas complete, a
# chaque partie -- pas de risque de croissance indefinie sur la carte SD).
#
# Envoi ponctuel, jamais rejoue (inchange depuis v1) : un succes debloque est
# un evenement "vient de se produire", pas un etat permanent.

# v3 - 2026-08-20 - safe-modify - Verrou anti-relance rendu ATOMIQUE (mkdir
#   au lieu d'un fichier PID check-then-write) -- BUG REEL reconfirme sur
#   materiel : 4 instances simultanees de CE script retrouvees vivantes
#   malgre le verrou fichier PID v2 -- EmulationStation peut lancer
#   plusieurs invocations dans la MEME seconde (rafale d'evenements au
#   boot), et "verifier si le fichier existe" PUIS "ecrire son propre PID"
#   n'est pas atomique. mkdir EST atomique sur ce systeme de fichiers
#   (tmpfs), fermant la fenetre de course entierement.
# v5 - 2026-09-04 - safe-modify - Extrait vers dmd_helpers/singleton_lock.sh,
#   voir marquee.sh v43 pour le detail complet (meme bloc duplique a
#   l'identique dans les 3 scripts, nettoyage differe puis repris ce jour).
. /recalbox/share/userscripts/dmd_helpers/singleton_lock.sh dmd_achievement 2>/dev/null || exit 1

LOG="/recalbox/share/system/logs/dmd_achievement.log"
RA_LOG="/recalbox/share/system/logs/retroarch.log"
FEATURES_FILE="/tmp/dmd_features_cache"
# v6 -- BUG REEL trouve en revue (pas en usage reel -- retour utilisateur
# explicite : "qu'est-ce qu'on aurait pu oublier de mettre a jour ?").
# Ce script n'avait JAMAIS ete touche lors de la bascule full UDP
# (marquee.sh v45, dmd_score.sh v48) -- ses notifications de succes
# RetroAchievements ne partaient plus QUE via mosquitto_pub, mort depuis
# RecalBox_DMD.ino v161 (MQTT_ENABLED=false). Meme send_udp() que les 2
# autres scripts (duplique, pas factorise -- meme motif qu'eux).
# v7 - 2026-09-19 - safe-modify - BUG REEL corrige, meme fix que
# marquee.sh v52/dmd_score.sh v52 (voir marquee.sh pour le detail complet) :
# DMD_UDP_IP restait l'IP fixe du DMD de developpement, cassant tout envoi
# vers un DMD reel sur un autre reseau. Decouverte dynamique via
# dmd_udp_resync.py (v5) desormais, meme fichier cache que les 2 autres.
DMD_UDP_IP_FALLBACK="192.168.0.51"
DMD_UDP_IP_CACHE="/tmp/dmd_udp_ip"
DMD_UDP_PORT=5005
send_udp() {
    DMD_UDP_IP=$(cat "$DMD_UDP_IP_CACHE" 2>/dev/null)
    [ -n "$DMD_UDP_IP" ] || DMD_UDP_IP="$DMD_UDP_IP_FALLBACK"
    python3 -c "import socket,sys; socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(sys.argv[1].encode('utf-8','replace'), (sys.argv[2], int(sys.argv[3])))" "$1" "$DMD_UDP_IP" "$DMD_UDP_PORT" 2>/dev/null
}

# Fonctions actives du DMD : /tmp/dmd_features_cache, ecrit par
# dmd_udp_resync.py a chaque message FEATURES: du DMD (v8 -- l'ancien
# sous-processus mosquitto_sub est retire, MQTT n'existe plus cote DMD).
# Fichier absent (aucun DMD n'a parle depuis le demarrage) = desactive.
feat_enabled() {
    [ -f "$FEATURES_FILE" ] || return 1
    val=$(sed -n "s/.*${1}=\([01]\).*/\1/p" "$FEATURES_FILE" | head -n1)
    [ "$val" = "1" ]
}

echo "$(date) - DMD achievement bridge started (v8, UDP seul)" >> "$LOG"

# -n 0 : ne rejoue pas le contenu deja present au demarrage du script. -F
# suit meme si le fichier est recree entre-temps.
tail -n 0 -F "$RA_LOG" 2>/dev/null | \
while IFS= read -r line; do
    case "$line" in
        *"Awarding achievement "*)
            # Format confirme en reel : [INFO] [RCHEEVOS] Awarding
            # achievement 535052: Ribs to Go -- extrait tout ce qui suit le
            # PREMIER ": " apres l'ID (un nom de succes contenant lui-meme
            # ":" reste donc intact).
            name="${line#*Awarding achievement }"
            name="${name#*: }"
            name=$(printf '%s' "$name" | tr -d '\r')
            [ -z "$name" ] && continue
            if feat_enabled "ra_ingame"; then
                echo "$(date '+%H:%M:%S') ACHIEVEMENT $name" >> "$LOG"
                send_udp "CMD=score ARG=SUCCES|${name}"
            else
                echo "$(date '+%H:%M:%S') ACHIEVEMENT $name (ignore, ra_ingame desactive)" >> "$LOG"
            fi
            ;;
    esac
done
