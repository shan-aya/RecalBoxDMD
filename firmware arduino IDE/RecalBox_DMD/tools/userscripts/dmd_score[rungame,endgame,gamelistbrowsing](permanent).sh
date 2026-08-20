#!/bin/ash
# Pont MQTT hiscore FBNeo + infos/description jeu -> DMD (marquee/cmd/score,
# canal UNIQUE, architecture "DMD bete" v110 -- voir RecalBox_DMD.ino)
#
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v3
#
# v3 - 2026-08-20 - safe-modify - Dwell (>5s) sur gamelistbrowsing avant de
#   declencher le slideshow navigation -- demande utilisateur explicite
#   (question posee AVANT implementation : "ca va perturber un firmware
#   deja fragile ?", reponse donnee : mecanisme CMD_SCORE sans risque cote
#   DMD, seul le DEBIT de publications MQTT est a surveiller). Corrige au
#   passage un vrai probleme non anticipe hier soir : SANS dwell, v2
#   declenchait le slideshow a CHAQUE changement de rom pendant un
#   defilement rapide -- un scroll traversant 20 jeux fbneo aurait spawn
#   20 slideshows en arriere-plan (rafale de publications), exactement le
#   genre de trafic que le coupe-circuit anti-rafale de marquee[...].sh a
#   ete concu pour eviter cote navigation generale. Mecanisme : chaque
#   evenement gamelistbrowsing ecrit l'etat courant (sys|rom) dans
#   BROWSE_STATE_FILE ; un changement de rom spawn un "guetteur" en
#   arriere-plan qui dort DWELL_SECONDS puis verifie que l'etat n'a PAS
#   change entre-temps avant de publier -- si l'utilisateur a deja bouge
#   vers un autre jeu, le guetteur se termine silencieusement sans rien
#   publier. Plusieurs guetteurs perimes peuvent coexister brievement
#   (harmless, verifient puis sortent), seul celui qui correspond a l'etat
#   REELLEMENT stabilise finit par publier. Pas besoin de toucher
#   marquee[...].sh (chaque script garde son propre abonnement independant
#   a Recalbox/EmulationStation/Event, deja le cas depuis v1).
#
# v2 - 2026-08-20 - safe-modify - Migration vers l'architecture "DMD bete"
#   (firmware v110, demande utilisateur explicite -- voir memoire projet
#   project_core_reassignment_rb_script_mismatch.md) :
#   1. UN SEUL topic desormais (marquee/cmd/score) -- marquee/cmd/game_info
#      n'existe plus cote firmware (jamais reintroduit avec le reste du
#      sous-systeme overlay retire en v104). Le champ DESCRIPTION et le
#      champ INFOS (issus de dmd_game_info.py) sont extraits separement du
#      payload combine et publies l'un apres l'autre (voir slideshow()) sur
#      ce meme canal -- le DMD affiche chaque contenu recu 6s puis revient
#      seul au jeu (MODE_SCORE, RecalBox_DMD.ino v110), aucune notion de
#      "carte suivante" a gerer cote firmware.
#   2. PLUS DE RETAIN (-r retire de tous les mosquitto_pub) -- un message
#      retenu perime a cause EXACTEMENT le bug observe en direct le
#      2026-08-19/20 (un ancien score restait sur le broker et se
#      republiait tout seul a chaque reconnexion, provoquant des affichages
#      fallback/playlist parasites) : le nouveau firmware n'a plus BESOIN
#      du retain (il revient tout seul au jeu par timer local), et le
#      retain devient un risque pur sans plus aucun benefice ici.
#   3. clear_game_data()/HAVE_GAME_DATA entierement retires -- n'avaient de
#      sens que pour effacer un RETENU perime ; sans retain, plus rien a
#      effacer.
#   4. Gating par les 8 reglages hi-score/info/description/RA x en-jeu/
#      navigation (page web DMD, diffuses en MQTT retenu sur
#      marquee/status/features) -- voir feat_enabled()/features_watcher().
#      RA (RetroAchievements) reste gere par dmd_achievement[watch](permanent).sh,
#      script separe (inchange dans son declenchement, migre de la meme
#      facon cote topic/retain -- voir ce fichier).
#
# v1 - 2026-08-17 - safe-modify - PORT depuis dev/mame-score-mqtt-bridge
#   (chantier reassignation de coeur, chunk 3) -- version SIMPLIFIEE : porte
#   le decodage hi-score (galaga/gyruss/1941) et le declenchement game_info,
#   sans le heartbeat periodique de la branche source. Voir _backups/ pour
#   le detail complet de cette version.
# ============================================
#
# Table de decodage hi-score (inchangee depuis v1) : portee depuis les
# definitions reelles de /recalbox/share/bios/fbneo/hiscore.dat, verifiees
# avec l'outil de reference hi2txt (GreatStoneEx/hi2txt-xml) sur des
# echantillons .hi reels, puis re-verifiees en executant cette meme logique
# en ash directement sur la Recalbox :
#   - galaga (et clones partageant la meme structure : galaga84, galagab2,
#     galagads, galagamf, galagamk, galagamw, galagao, gallag) : derniers 6
#     octets du .hi = "TOP SCORE", profil BCD-le + trim 0x24 (tuile
#     "blanc") -> galaga topscore = 20000 valide.
#   - gyruss : derniers 3 octets du .hi = "TOP SCORE", entier little-endian
#     affiche en hexadecimal -> gyruss topscore = 10000 valide.
#   - 1941 : bloc de 10 rangs [SCORE(4) NOM(3) GRADE(1)] a partir de
#     l'offset 40 -- rang N -> offset 40+N*8. Top 5 en scroll vertical.
# Seuls ces jeux sont geres pour l'instant (portage limite et volontaire,
# voir memoire projet "Faisabilite highscore/level DMD"). Un jeu FBNeo
# inconnu de cette table est simplement ignore (aucune publication).

# v1.1 - 2026-08-19 - safe-modify - Verrou anti-relance (fichier PID) --
#   ES relance ce script a chaque evenement, sans protection chaque relance
#   dupliquerait la boucle mosquitto_sub principale.
PIDFILE="/tmp/dmd_score_singleton.pid"
if [ -f "$PIDFILE" ]; then
    oldpid=$(cat "$PIDFILE" 2>/dev/null)
    if [ -n "$oldpid" ] && kill -0 "$oldpid" 2>/dev/null; then
        exit 0
    fi
fi
echo $$ > "$PIDFILE"

LOG="/recalbox/share/system/logs/dmd_score.log"
SCRIPT_DIR=$(dirname "$0")
HI_DIR="/recalbox/share/saves/fbneo/fbneo"
FEATURES_FILE="/tmp/dmd_features_cache"
# Delai entre 2 elements d'un slideshow (hi-score puis description puis
# infos) -- doit etre >= SCORE_DISPLAY_DURATION_MS cote firmware (6000ms,
# voir RecalBox_DMD.ino) pour laisser chaque contenu pleinement visible
# avant le suivant. Marge de 500ms au-dessus pour ne jamais chevaucher.
SLIDESHOW_GAP_S=7
# v3 -- dwell navigation (voir entete changelog) : delai d'immobilite sur
# un rom avant de declencher le slideshow "browse". BROWSE_STATE_FILE
# partage l'etat courant (sys|rom) entre la boucle principale et les
# guetteurs en arriere-plan (necessaire : un "&" fork ne voit jamais les
# mises a jour ulterieures d'une variable shell du parent).
DWELL_SECONDS=5
BROWSE_STATE_FILE="/tmp/dmd_browse_state"

read_state() {
    grep "^${1}=" "/tmp/es_state.inf" 2>/dev/null | cut -d= -f2- | tr -d '\r\n '
}

# v2 -- lit le cache local des 8 reglages (voir features_watcher()) --
# renvoie vrai (0) si la cle demandee vaut 1, faux (1) sinon -- y compris
# si le cache n'existe pas encore (rien recu du DMD depuis le demarrage de
# ce script -- comportement prudent : ne publie rien tant que le reglage
# reel n'est pas connu, plutot que de supposer "active").
feat_enabled() {
    [ -f "$FEATURES_FILE" ] || return 1
    val=$(sed -n "s/.*${1}=\([01]\).*/\1/p" "$FEATURES_FILE" | head -n1)
    [ "$val" = "1" ]
}

# v2 -- sous-processus DEDIE (comme le heartbeat de l'ancienne v9, meme
# raisonnement : un fork ne voit jamais les mises a jour ulterieures d'une
# variable shell du processus principal -- necessite un FICHIER partage).
# marquee/status/features est RETENU cote DMD (voir broadcastFeatureStatus(),
# RecalBox_DMD.ino) : la toute premiere lecture arrive donc immediatement a
# la souscription, meme si ce script demarre apres le DMD. Connexion
# persistante (pas de reconnexion par evenement), threat model identique au
# pont marquee[...] existant.
features_watcher() {
    mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/status/features" 2>/dev/null | \
    while IFS= read -r line; do
        printf '%s\n' "$line" > "$FEATURES_FILE"
    done
}
features_watcher &

# v2 -- PLUS DE RETAIN (voir en-tete changelog). Envoi simple, best-effort
# (QoS 0, comme le reste de ce protocole) -- une perte occasionnelle n'est
# qu'un contenu manque, jamais un ecran fige (le firmware revient de toute
# facon tout seul au jeu apres 6s, avec ou sans nouveau message).
send_score() {
    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd/score" -m "$1" 2>/dev/null
    echo "$(date '+%H:%M:%S') SEND marquee/cmd/score = $1" >> "$LOG"
}

# v2 -- extrait un champ ("DESCRIPTION" ou "INFOS") du payload combine
# genere par dmd_game_info.py (format "LABEL|contenu|LABEL|contenu|...",
# voir build_payload() dans ce fichier) -- script PYTHON NON MODIFIE
# (deja bien teste/durci, voir son propre changelog), le decoupage se fait
# ici cote shell pour respecter les 2 toggles separes info/description
# sans toucher a du code python deja eprouve.
extract_field() {
    raw="$1"; want="$2"
    case "$raw" in
        *"${want}|"*)
            rest="${raw#*${want}|}"
            content="${rest%%|*}"
            [ -n "$content" ] && echo "${want}|${content}"
            ;;
    esac
}

decode_galaga_topscore() {
    size=$(wc -c < "$1" 2>/dev/null)
    off=$((size - 6))
    hex=$(od -An -tx1 -j "$off" -N 6 "$1" 2>/dev/null)
    set -- $hex
    rev="$6 $5 $4 $3 $2 $1"
    result=""
    started=0
    for b in $rev; do
        if [ "$started" -eq 0 ] && [ "$b" = "24" ]; then
            continue
        fi
        started=1
        result="${result}$(printf '%s' "$b" | cut -c2)"
    done
    [ -z "$result" ] && result="0"
    result=$(printf '%s' "$result" | sed 's/^0*//')
    [ -z "$result" ] && result="0"
    echo "$result"
}

decode_gyruss_topscore() {
    size=$(wc -c < "$1" 2>/dev/null)
    off=$((size - 3))
    hex=$(od -An -tx1 -j "$off" -N 3 "$1" 2>/dev/null)
    set -- $hex
    val=$(( 0x$1 + 0x$2 * 256 + 0x$3 * 65536 ))
    printf '%X' "$val"
}

decode_1941_rank_score() {
    hifile="$1"; idx="$2"
    off=$((40 + idx * 8))
    hex=$(od -An -tx1 -j "$off" -N 4 "$hifile" 2>/dev/null)
    set -- $hex
    result="$1$2$3$4"
    result=$(printf '%s' "$result" | sed 's/^0*//')
    [ -z "$result" ] && result="0"
    echo "$result"
}

decode_1941_rank_name() {
    hifile="$1"; idx="$2"
    off=$((40 + idx * 8 + 4))
    dd if="$hifile" bs=1 skip="$off" count=3 2>/dev/null | tr -cd 'A-Za-z0-9 .'
}

decode_1941_topN() {
    hifile="$1"; n="$2"
    i=0
    out=""
    while [ "$i" -lt "$n" ]; do
        s=$(decode_1941_rank_score "$hifile" "$i")
        nm=$(decode_1941_rank_name "$hifile" "$i")
        rank=$((i + 1))
        line="${rank} ${nm} ${s}"
        if [ -z "$out" ]; then out="$line"; else out="${out}|${line}"; fi
        i=$((i + 1))
    done
    echo "$out"
}

# v2 -- ne publie plus directement : renvoie le payload sur stdout (ou rien
# si non applicable), le decoupage slideshow/gating reste au niveau appelant.
build_score_payload() {
    rom="$1"
    hifile="${HI_DIR}/${rom}.hi"
    [ -f "$hifile" ] || { echo "$(date '+%H:%M:%S') SCORE skip $rom (pas de .hi)" >> "$LOG"; return; }
    size=$(wc -c < "$hifile" 2>/dev/null)
    glabel=""

    case "$rom" in
        galaga|galaga84|galagab2|galagads|galagamf|galagamk|galagamw|galagao|gallag)
            if [ "$size" != "51" ]; then
                echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 51)" >> "$LOG"
                return
            fi
            score=$(decode_galaga_topscore "$hifile")
            glabel="GALAGA"
            echo "HI-SCORE ${score}"
            ;;
        gyruss)
            if [ "$size" != "43" ]; then
                echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 43)" >> "$LOG"
                return
            fi
            score=$(decode_gyruss_topscore "$hifile")
            glabel="GYRUSS"
            echo "HI-SCORE ${score}"
            ;;
        1941)
            if [ "$size" != "124" ]; then
                echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 124)" >> "$LOG"
                return
            fi
            topn=$(decode_1941_topN "$hifile" 5)
            glabel="1941"
            score="${topn}"
            echo "HI-SCORE|${topn}"
            ;;
        *)
            echo "$(date '+%H:%M:%S') SCORE skip $rom (jeu non supporte)" >> "$LOG"
            return
            ;;
    esac
    echo "$(date '+%H:%M:%S') SCORE $rom (${glabel}) -> ${score}" >> "$LOG" 1>&2
}

# v2 -- slideshow BACKGROUNDE (appele avec "&" par l'appelant) : publie
# jusqu'a 3 contenus (hi-score, description, infos) espaces de
# SLIDESHOW_GAP_S -- ne bloque JAMAIS la boucle d'evenements principale
# (voir appels rungame/gamelistbrowsing plus bas). $1=system $2=game_path
# $3=rom $4=context ("ingame" ou "browse", determine quels toggles verifier).
publish_slideshow() {
    sys="$1"; gpath="$2"; rom="$3"; ctx="$4"
    first=1

    if [ "$sys" = "fbneo" ] && [ -n "$rom" ] && feat_enabled "hiscore_${ctx}"; then
        payload=$(build_score_payload "$rom")
        if [ -n "$payload" ]; then
            [ "$first" -eq 0 ] && sleep "$SLIDESHOW_GAP_S"
            send_score "$payload"
            first=0
        fi
    fi

    want_desc_or_info=1
    feat_enabled "description_${ctx}" || feat_enabled "info_${ctx}" || want_desc_or_info=0
    if [ "$want_desc_or_info" -eq 1 ] && [ -n "$sys" ] && [ -n "$gpath" ]; then
        raw=$(python3 "${SCRIPT_DIR}/dmd_game_info.py" "$sys" "$gpath" 2>>"$LOG")
        if [ -n "$raw" ]; then
            if feat_enabled "description_${ctx}"; then
                desc=$(extract_field "$raw" "DESCRIPTION")
                if [ -n "$desc" ]; then
                    [ "$first" -eq 0 ] && sleep "$SLIDESHOW_GAP_S"
                    send_score "$desc"
                    first=0
                fi
            fi
            if feat_enabled "info_${ctx}"; then
                info=$(extract_field "$raw" "INFOS")
                if [ -n "$info" ]; then
                    [ "$first" -eq 0 ] && sleep "$SLIDESHOW_GAP_S"
                    send_score "$info"
                    first=0
                fi
            fi
        else
            echo "$(date '+%H:%M:%S') INFO/DESC skip $sys/$gpath (pas de champ exploitable gamelist.xml)" >> "$LOG"
        fi
    fi
}

echo "$(date) - DMD score bridge started (v3, architecture DMD bete + dwell navigation)" >> "$LOG"

# Dedoublonnage gamelistbrowsing : evite de relancer le slideshow a CHAQUE
# evenement si l'utilisateur reste sur le MEME rom, seulement au changement
# reel d'entree.
LAST_BROWSE_SYS=""
LAST_BROWSE_ROM=""

# Connexion MQTT PERSISTANTE (une seule souscription, lue en continu) --
# memes precautions que le pont marquee[...] existant (voir memoire projet).
mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "Recalbox/EmulationStation/Event" 2>/dev/null | \
while IFS= read -r event; do
    event=$(printf '%s' "$event" | tr -d '\r')

    case "$event" in
        rungame)
            system=$(read_state "SystemId")
            game_path=$(read_state "GamePath")
            echo "$(date '+%H:%M:%S') RUNGAME sys=$system path=$game_path" >> "$LOG"
            rom=""
            if [ -n "$system" ] && [ -n "$game_path" ]; then
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                publish_slideshow "$system" "$game_path" "$rom" "ingame" &
            fi
            ;;
        endgame)
            # Republie le score final (pas de description/infos ici --
            # inchange depuis v1, seul le hi-score a un interet a etre
            # rafraichi en fin de partie).
            system=$(read_state "SystemId")
            if [ "$system" = "fbneo" ] && feat_enabled "hiscore_ingame"; then
                game_path=$(read_state "GamePath")
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                echo "$(date '+%H:%M:%S') ENDGAME fbneo rom=$rom" >> "$LOG"
                if [ -n "$rom" ]; then
                    payload=$(build_score_payload "$rom")
                    [ -n "$payload" ] && send_score "$payload"
                fi
            fi
            ;;
        gamelistbrowsing)
            system=$(read_state "SystemId")
            game_path=$(read_state "GamePath")
            if [ -n "$system" ] && [ -n "$game_path" ] && [ ! -d "$game_path" ]; then
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                state="${system}|${rom}"
                # v3 -- ecrit a CHAQUE evenement (meme rom repete), pour que
                # les guetteurs deja en vol voient bien "rien n'a bouge"
                # meme si ES republie le meme evenement plusieurs fois.
                printf '%s\n' "$state" > "$BROWSE_STATE_FILE"
                if [ "$system" != "$LAST_BROWSE_SYS" ] || [ "$rom" != "$LAST_BROWSE_ROM" ]; then
                    LAST_BROWSE_SYS="$system"
                    LAST_BROWSE_ROM="$rom"
                    echo "$(date '+%H:%M:%S') BROWSE sys=$system rom=$rom (dwell ${DWELL_SECONDS}s)" >> "$LOG"
                    (
                        sleep "$DWELL_SECONDS"
                        current=$(cat "$BROWSE_STATE_FILE" 2>/dev/null)
                        if [ "$current" = "$state" ]; then
                            echo "$(date '+%H:%M:%S') DWELL settled sys=$system rom=$rom" >> "$LOG"
                            publish_slideshow "$system" "$game_path" "$rom" "browse"
                        else
                            echo "$(date '+%H:%M:%S') DWELL abandoned sys=$system rom=$rom (deplace entre-temps)" >> "$LOG"
                        fi
                    ) &
                fi
            else
                LAST_BROWSE_SYS=""
                LAST_BROWSE_ROM=""
                : > "$BROWSE_STATE_FILE"
            fi
            ;;
        *)
            LAST_BROWSE_SYS=""
            LAST_BROWSE_ROM=""
            ;;
    esac
done
