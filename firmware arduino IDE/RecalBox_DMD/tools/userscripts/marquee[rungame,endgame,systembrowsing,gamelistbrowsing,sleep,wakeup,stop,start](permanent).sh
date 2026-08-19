#!/bin/ash
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v11
#
# v11 - 2026-08-19 - safe-modify - Desactivation du coupe-circuit anti-rafale
#   (throttle + !SHUFFLE) SANS retirer le code -- BURST_THRESHOLD remonte de
#   5 a 50 survols/seconde, largement hors de portee d'une navigation humaine
#   meme tres rapide (les rafales mesurees cette session culminaient a 5-8/s).
#   Motivation : la cause principale du rc=-4/gels de navigation chassee
#   depuis plusieurs sessions semble in fine etre l'overclock RPi5 sous
#   canicule (voir memoire projet, decouverte du 2026-08-19), pas le trafic
#   MQTT local -- le coupe-circuit avait ete concu specifiquement pour
#   attenuer une correlation activite/rc=-4 qui n'a peut-etre jamais ete la
#   vraie cause. Test demande : navigation en direct sans throttle (comme
#   avant v6), pour voir si le systeme tient maintenant que l'hypothese
#   thermique/overclock est traitee. Mecanisme garde intact (juste rendu
#   inatteignable) -- seuil abaissable a nouveau instantanement si besoin.
#
# v10 - 2026-08-18 - safe-modify - Suppression du polling permanent (-W 1)
#   hors rafale. BUG REEL confirme sur materiel (log debug mosquitto, meme
#   jour) : le "-W 1" ajoute en v6 pour detecter la fin de rafale faisait
#   sortir la boucle mosquitto_sub TOUTES LES ~1s EN PERMANENCE, meme en
#   idle total sans aucune navigation -- pas seulement pendant une vraie
#   rafale comme prevu au design. Chaque sortie de boucle relance un NOUVEAU
#   mosquitto_sub, donc une NOUVELLE connexion locale (127.0.0.1) toutes les
#   secondes, en continu, 24h/24. Capture broker (log debug) : rafale de
#   connexions locales QUASI CONTINUE (~1/s, ~30 connexions/31s observees)
#   coincidant exactement avec "Client esp32-marquee has exceeded timeout,
#   disconnecting." -- ce trafic de fond genere par le script LUI-MEME est
#   un suspect direct pour le rc=-4 (le broker peine peut-etre a traiter la
#   session distante du DMD au milieu de ce bruit local constant). C'est
#   l'inverse du but recherche par le coupe-circuit anti-rafale (v6), qui
#   visait a REDUIRE le trafic MQTT local, pas a en ajouter en permanence.
#   Fix : le timeout "-W 1" n'est desormais utilise QUE pendant une rafale
#   effectivement en cours (throttled=1) -- seul moment ou une detection de
#   fin de rafale a un sens. Hors rafale (cas normal, largement majoritaire
#   en usage reel), retour a un mosquitto_sub -C 1 BLOQUANT SANS timeout,
#   comme avant v6 -- une connexion locale UNIQUEMENT quand un vrai
#   evenement ES arrive, zero trafic de fond en idle. Aucun changement de
#   comportement fonctionnel : le detecteur de rafale, le seuil, le
#   !SHUFFLE et la publication de fin de rafale restent identiques,
#   seulement actifs pendant la fenetre (courte, bornee) ou throttled=1.
#
# v9 - 2026-08-18 - safe-modify - Affichage transitoire pendant le throttle
#   ("coupe-circuit anti-rafale", v6). Au lieu de laisser le dernier marquee
#   reel fige a l'ecran pendant la fenetre de throttle, une publication
#   UNIQUE ("!SHUFFLE", NON retenue -- voir raison dans le code) declenche
#   au moment ou throttled passe a 1 une animation dediee (statique/
#   interference CRT, raw565pack pre-fabrique, genere via IA + convert_gif_
#   to_raw565pack_meta() du RecalBoxDMD_tool.py) qui boucle localement cote
#   firmware (RecalBox_DMD.ino v107, reutilise le mecanisme MODE_GIF/
#   gifResetCompat() deja existant -- aucun nouveau code de lecture/boucle).
#   La vraie position suit normalement au "BURST end". Toujours 2
#   publications max par rafale (debut + fin), pas plus qu'avant v9.
#
# v8 - 2026-08-18 - safe-modify - Lecture atomique SystemId/GamePath.
#   BUG REEL confirme sur materiel : SystemId et GamePath etaient lus par 2
#   appels SEPARES de read_state() (2 lectures separees de
#   /tmp/es_state.inf) -- pas atomique, EmulationStation pouvant reecrire ce
#   fichier ENTRE les deux lectures pendant un defilement rapide. Observe :
#   "raw=amiga600 game=.../roms/nes/xxx.zip" -- systeme et jeu incoherents,
#   publies tels quels -> image de repli cote DMD (chemin inexistant).
#   Symptome utilisateur : "images fallback a l'arret du defilement, corrige
#   par un aller-retour" (qui redeclenche une lecture, cette fois coherente
#   par hasard). Deja present avant le throttle (v6/v7) mais imperceptible
#   (corrige ~100ms plus tard par la publication suivante) -- devenu visible
#   car le throttle ne publie qu'UNE fois par rafale, sans rien pour
#   corriger une derniere lecture incoherente. Fix : lecture UNIQUE du
#   fichier (read_state_snapshot()), SystemId et GamePath extraits du MEME
#   instantane (extract_field()) -- plus de fenetre de race possible.
#
# v7 - 2026-08-18 - safe-modify - Ajustement seuil coupe-circuit anti-rafale.
#   Retour utilisateur apres test reel (v6, seuil 3/s) : "le 3/s un peu trop
#   restrictif" -- se declenchait pendant une navigation rapide mais encore
#   normale, pas seulement les vraies rafales extremes. BURST_THRESHOLD
#   remonte de 3 a 5 survols/seconde -- reste facilement atteint pendant un
#   VRAI defilement rapide continu (les logs v6 montraient des survols a
#   <100ms d'intervalle pendant les rafales), mais laisse plus de marge a
#   une navigation "rapide mais normale" avant de couper les publications.
#   Aucun autre changement de logique.
#
# v6 - 2026-08-18 - safe-modify - Coupe-circuit anti-rafale (burst throttle).
#   BUG REEL confirme sur materiel (session 2026-08-18, log broker mosquitto
#   correle avec le serial DMD, voir memoire projet) : une navigation rapide
#   dans une liste de jeux/systemes (usage NORMAL de l'interface RecalBox --
#   pas un cas limite) declenche une rafale de cycles connect/subscribe/
#   publish/disconnect locaux (ce script republie -- donc se reconnecte via
#   mosquitto_pub -- a CHAQUE jeu survole). Episode capture : 32 connexions
#   locales en ~30s pendant un scroll rapide, correle avec un decrochage
#   MQTT du DMD (rc=-4/timeout) au meme moment. Demande explicite
#   utilisateur : PAS de latence fixe ajoutee sur l'usage normal (le
#   firmware DMD est concu pour etre hyper-reactif, un delai systematique
#   de 1-2s "casserait" ce qui a ete construit) -- seulement couper
#   temporairement PENDANT une vraie rafale, et reprendre la reactivite
#   normale des que le defilement ralentit, sans attente artificielle
#   supplementaire au moment de la reprise.
#
#   Fonctionnement : compteur de survols (gamelistbrowsing/systembrowsing)
#   dans la MEME seconde horloge (precision seconde entiere -- ash/BusyBox
#   n'a pas d'horloge sub-seconde fiable partout, et ce n'est pas necessaire
#   ici : le seuil visé est "plusieurs survols dans la meme seconde", pas
#   un intervalle precis). BURST_THRESHOLD survols dans la meme seconde ->
#   bascule en mode "throttled" : les survols suivants ne publient PLUS
#   (LAST_SYSTEM/LAST_ROM restent a jour en interne, silencieusement) tant
#   que la rafale continue. mosquitto_sub est appele avec "-W 1" (timeout
#   1s) au lieu d'un blocage pur -- des qu'une iteration timeout (aucun
#   evenement recu pendant 1s complete), c'est le signal que la rafale
#   vient de s'arreter : publication IMMEDIATE de la position courante
#   (system/game tel qu'il a ete mis a jour silencieusement pendant le
#   throttle), retour en mode reactif normal. Navigation normale (un pas a
#   la fois, meme rapide) : le seuil (3 dans la meme seconde) n'est
#   quasiment jamais atteint, donc AUCUN changement de comportement --
#   publication immediate comme avant v6. rungame/endgame/stop (transitions
#   definitives, pas de simples survols) reinitialisent le compteur de
#   rafale -- une vraie action utilisateur ne doit jamais rester "en
#   attente" a cause d'un throttle en cours.
#
# v5 - 2026-08-18 - safe-modify - Verrou anti-relance (fichier PID). BUG REEL
#   confirme sur materiel (session 2026-08-17/18) : EmulationStation relance
#   ce script A CHAQUE evenement correspondant a un des noms entre crochets
#   du nom de fichier (gamelistbrowsing, systembrowsing, rungame, etc.) --
#   le suffixe "(permanent)" est une pure convention de nommage, RIEN cote
#   ES ne l'empeche de re-invoquer le script en plus de l'instance deja en
#   cours d'execution. Or ce script n'avait AUCUNE protection contre sa
#   propre re-invocation : chaque nouvel appel entrait dans sa propre boucle
#   "while true" infinie, sans jamais se terminer -- accumulation illimitee
#   de processus zombies au fil de la navigation (observe : plusieurs
#   instances simultanees, chacune avec son propre etat LAST_SYSTEM/
#   LAST_ROM desynchronise des autres, chacune consommant/publiant en
#   double sur les memes topics MQTT). Symptomes observes cote utilisateur :
#   navigation menu tres lente (fork+exec shell a chaque event), DMD affiche
#   un systeme perime alors que la RB est sur un autre, doublons "marquee.sh"
#   trouves a repetition toute la soiree precedente (deja documente v4).
#   Fix initial tente : flock -n sur un fd dedie -- ECARTE apres test reel,
#   un fd ouvert via "exec 9>" est HERITE par tout process fils issu d'un
#   fork() ulterieur (meme sans re-executer le flock), donc un fils qui
#   hérite du fd deja verrouille par son parent continue de tourner sans
#   jamais etre bloque -- observe sur materiel : 2 process actifs
#   simultanement partageant le meme fd 9 (verifie via /proc/PID/fd/9).
#   Fix retenu : verrou par FICHIER PID classique, insensible a l'heritage
#   de descripteur -- verifie via "kill -0" si le PID enregistre est un
#   process VIVANT (pas juste "le fichier existe", qui casserait apres un
#   crash/kill -9 sans nettoyage). Si vivant -> sortie immediate. Sinon
#   (fichier absent, ou PID mort/perime) -> on ecrit notre propre PID et on
#   continue normalement.
#
# v4 - 2026-08-17 - safe-modify - PORT depuis dev/mame-score-mqtt-bridge
#   (chantier reassignation de coeur, chunk 3 -- signal dedie "vraiment en
#   jeu"). Nouveau topic marquee/cmd/ingame ("1" sur rungame, "0" sur
#   endgame/stop) -- AJOUTE, ne remplace rien de l'existant. Necessaire cote
#   firmware (RecalBox_DMD.ino v79) : marquee/cmd/game est publie A LA FOIS
#   par un vrai lancement de partie (rungame) ET par un simple survol de la
#   liste des jeux (gamelistbrowsing, voir plus bas dans ce fichier -- meme
#   topic, meme format) -- le firmware ne peut pas distinguer les 2 a partir
#   de ce seul topic pour savoir s'il faut activer l'alternance hi-score/
#   game_info (cout heap/CPU non-nul, a eviter pendant un simple defilement
#   rapide de liste).

echo "$(date '+%H:%M:%S.%N') TRACE start pid=$$ ppid=$PPID arg0=$0" >> /tmp/marquee_trace.log

# v5 -- verrou anti-relance : DOIT etre la toute premiere action du script
# (avant meme LOG=...), pour sortir le plus vite possible si une instance
# tourne deja -- evite tout travail inutile (et surtout, evite d'entrer
# dans la boucle "while true" qui ne se terminerait jamais). Fichier PID
# plutot que flock (voir changelog v5 ci-dessus pour la raison).
PIDFILE="/tmp/marquee_singleton.pid"
if [ -f "$PIDFILE" ]; then
    oldpid=$(cat "$PIDFILE" 2>/dev/null)
    if [ -n "$oldpid" ] && kill -0 "$oldpid" 2>/dev/null; then
        echo "$(date '+%H:%M:%S.%N') TRACE exit pid=$$ (oldpid=$oldpid alive)" >> /tmp/marquee_trace.log
        exit 0
    fi
fi
echo $$ > "$PIDFILE"
echo "$(date '+%H:%M:%S.%N') TRACE proceeding pid=$$" >> /tmp/marquee_trace.log

LOG="/recalbox/share/system/logs/marquee_mqtt.log"

read_state() {
    grep "^${1}=" "/tmp/es_state.inf" 2>/dev/null | cut -d= -f2- | tr -d '\r\n '
}

# v8 -- BUG REEL confirme sur materiel (session 2026-08-18) : SystemId et
# GamePath etaient lus par 2 appels SEPARES de read_state() (donc 2 lectures
# separees de /tmp/es_state.inf) -- pas atomique : EmulationStation peut
# reecrire ce fichier ENTRE les deux lectures pendant un defilement rapide,
# donnant un SystemId et un GamePath qui ne correspondent pas au meme
# instant (observe : "raw=amiga600 game=.../roms/nes/xxx.zip" -- systeme et
# jeu totalement incoherents). Consequence : LAST_SYSTEM/LAST_ROM se
# retrouvent desynchronises, publies tels quels -> chemin inexistant cote
# DMD -> image de repli affichee au lieu du vrai marquee. Ce bug existait
# deja avant le throttle (v6/v7) mais restait imperceptible : une lecture
# incoherente etait corrigee ~100ms plus tard par la publication suivante.
# Avec le throttle, si c'est la DERNIERE lecture avant la pause, rien ne la
# corrige -- d'ou le symptome "fallback a l'arret du defilement, corrige
# par un aller-retour" (qui redeclenche une lecture, cette fois coherente).
# Fix : lire le fichier UNE SEULE FOIS (un seul cat), extraire SystemId ET
# GamePath depuis ce MEME instantane -- plus aucune fenetre de race
# possible entre les deux champs.
read_state_snapshot() {
    cat "/tmp/es_state.inf" 2>/dev/null
}

extract_field() {
    # $1 = instantane (contenu multi-lignes), $2 = nom du champ
    echo "$1" | grep "^${2}=" | cut -d= -f2- | tr -d '\r\n '
}

send_mqtt_retain() {
    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -r -t "marquee/cmd/${1}" -m "$2" 2>/dev/null
    echo "$(date '+%H:%M:%S') SEND(R) marquee/cmd/${1} = $2" >> "$LOG"
}

normalize_system() {
    local sys="$1"
    case "$sys" in
        *) echo "$sys" ;;
    esac
}

# v6 -- publie la position courante (system+rom silencieusement mis a jour
# pendant un throttle) -- factorise car appelee a la fois par la fin de
# rafale (timeout) et potentiellement reutilisable ailleurs.
publish_settled_position() {
    if [ "$IN_GAME" -eq 1 ]; then
        return
    fi
    if [ -n "$LAST_ROM" ]; then
        send_mqtt_retain "game" "${LAST_SYSTEM}/${LAST_ROM}"
    elif [ -n "$LAST_SYSTEM" ]; then
        send_mqtt_retain "system" "$LAST_SYSTEM"
    fi
}

echo "$(date) - Marquee bridge started (v11, lock acquired)" >> "$LOG"

send_mqtt_retain "default" "1"

LAST_SYSTEM=""
LAST_ROM=""
IN_GAME=0
BOOT_TIME=0
PREV_EVENT=""

# v6 -- etat du detecteur de rafale (voir changelog v6 ci-dessus).
# v7 -- seuil remonte de 3 a 5 (retour utilisateur : 3 trop restrictif).
# v11 -- seuil remonte a 50 (desactivation de fait, voir changelog v11).
BURST_THRESHOLD=50
burst_window_start=0
burst_count=0
throttled=0

while true; do
    PREV_EVENT="$event"
    # v10 -- "-W 1" reserve au cas throttled=1 (voir changelog v10) : hors
    # rafale, blocage pur (pas de timeout) -- zero connexion locale tant
    # qu'aucun vrai evenement ES n'arrive, comme avant v6. Pendant une
    # rafale, "-W 1" reste necessaire pour detecter sa fin (aucune iteration
    # ne se produirait sinon tant qu'aucun evenement n'arrive). $event vide
    # en sortie de boucle ne peut donc survenir QUE si throttled=1 (timeout)
    # ou (tres improbable) message vide recu -- traite pareil, sans
    # consequence (le case *) plus bas ignore deja les events vides).
    if [ "$throttled" -eq 1 ]; then
        event=$(mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 \
            -t "Recalbox/EmulationStation/Event" -C 1 -W 1 2>/dev/null | tr -d '\r')
    else
        event=$(mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 \
            -t "Recalbox/EmulationStation/Event" -C 1 2>/dev/null | tr -d '\r')
    fi

    if [ -z "$event" ]; then
        # v6 -- timeout : si une rafale etait en cours, elle vient de
        # s'arreter (aucun survol depuis >=1s) -- publier la position
        # courante MAINTENANT (pas d'attente supplementaire) et repasser en
        # mode reactif normal.
        if [ "$throttled" -eq 1 ]; then
            throttled=0
            burst_count=0
            echo "$(date '+%H:%M:%S') BURST end -- publication position stabilisee" >> "$LOG"
            publish_settled_position
        fi
        continue
    fi

    echo "$(date '+%H:%M:%S') EVENT=$event IN_GAME=$IN_GAME LAST_SYS=$LAST_SYSTEM LAST_ROM=$LAST_ROM" >> "$LOG"

    case "$event" in

        start)
            IN_GAME=0
            LAST_ROM=""
            LAST_SYSTEM=""
            BOOT_TIME=$(date +%s)
            burst_count=0
            throttled=0
            send_mqtt_retain "default" "1"

            # Attendre la fin de la rafale automatique de boot
            sleep 5

            # Lire le vrai système affiché
            system_raw=$(read_state "SystemId")
            system=$(normalize_system "$system_raw")
            echo "$(date '+%H:%M:%S') BOOT settle -> sys=$system" >> "$LOG"
            if [ -n "$system" ]; then
                LAST_SYSTEM="$system"
                send_mqtt_retain "system" "$system"
            fi
            ;;

        gamelistbrowsing|systembrowsing)
            # Ignorer la rafale pendant les 10s après le boot
            now=$(date +%s)
            if [ "$BOOT_TIME" -gt 0 ] && [ $((now - BOOT_TIME)) -lt 10 ]; then
                echo "$(date '+%H:%M:%S') BROWSE ignored (boot settle)" >> "$LOG"
                continue
            fi

            # v6 -- detecteur de rafale : compte les survols dans la MEME
            # seconde horloge entiere (precision volontairement grossiere,
            # voir changelog v6). BURST_THRESHOLD atteint -> bascule
            # throttled=1 (les publications s'arretent, voir plus bas).
            if [ "$now" = "$burst_window_start" ]; then
                burst_count=$((burst_count + 1))
            else
                burst_window_start="$now"
                burst_count=1
            fi
            if [ "$burst_count" -ge "$BURST_THRESHOLD" ] && [ "$throttled" -eq 0 ]; then
                throttled=1
                echo "$(date '+%H:%M:%S') BURST start (seuil $BURST_THRESHOLD/s atteint)" >> "$LOG"
                # v9 -- coupe-circuit anti-rafale, affichage transitoire.
                # UNE SEULE publication (pas une par frame -- l'animation
                # boucle localement cote firmware, voir memoire projet et
                # RecalBox_DMD.ino v107) sur ce MEME topic marquee/cmd/game
                # deja utilise -- aucun nouveau topic, aucune nouvelle
                # souscription. mosquitto_pub direct (pas send_mqtt_retain)
                # : ce signal n'a pas vocation a etre retenu -- un abonne
                # qui se connecterait pendant une rafale ne doit jamais
                # recevoir "!SHUFFLE" comme dernier etat connu.
                mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd/game" -m "!SHUFFLE" 2>/dev/null
                echo "$(date '+%H:%M:%S') SEND !SHUFFLE (non retenu)" >> "$LOG"
            fi

            # v8 -- lecture atomique (voir changelog v8) : SystemId et
            # GamePath extraits du MEME instantane, plus de risque
            # d'incoherence entre les deux.
            _snap=$(read_state_snapshot)
            system_raw=$(extract_field "$_snap" "SystemId")
            system=$(normalize_system "$system_raw")
            game_path=$(extract_field "$_snap" "GamePath")

            echo "$(date '+%H:%M:%S') BROWSE raw=$system_raw norm=$system game=$game_path in_game=$IN_GAME throttled=$throttled" >> "$LOG"

            if [ "$IN_GAME" -eq 1 ]; then
                echo "$(date '+%H:%M:%S') BROWSE ignored (in game)" >> "$LOG"
                continue
            fi

            if [ -n "$game_path" ]; then
                if [ -d "$game_path" ]; then
                    echo "$(date '+%H:%M:%S') BROWSE subdir -> send system $system" >> "$LOG"
                    if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
                        LAST_SYSTEM="$system"
                        LAST_ROM=""
                        # v6 -- pendant une rafale (throttled=1), on met a
                        # jour LAST_SYSTEM/LAST_ROM silencieusement SANS
                        # publier -- la publication effective se fera au
                        # "BURST end" (timeout) plus haut, avec la position
                        # la plus recente.
                        if [ "$throttled" -eq 0 ]; then
                            send_mqtt_retain "system" "$system"
                        fi
                    fi
                else
                    # 2026-08-09 : "s/ //g" ajoute -- l'outil PC (sanitize_filename(),
                    # RecalBoxDMD_tool.py) retire tous les espaces du nom de ROM en
                    # ecrivant les fichiers sur la carte SD DMD (ex: "Zynaps (Europe).zip"
                    # -> "Zynaps(Europe).raw565pack"), mais ce script envoyait le nom AVEC
                    # ses espaces d'origine -- flag "?" (fallback) sur le DMD pour tout jeu
                    # dont le nom de ROM contient un espace, meme si le fichier converti
                    # existe bel et bien sur la carte SD, juste sous un nom legerement
                    # different. Meme fix applique au bloc rungame plus bas.
                    rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
                    if [ -n "$system" ] && [ -n "$rom" ]; then
                        if [ "$rom" != "$LAST_ROM" ] || [ "$system" != "$LAST_SYSTEM" ]; then
                            LAST_SYSTEM="$system"
                            LAST_ROM="$rom"
                            if [ "$throttled" -eq 0 ]; then
                                send_mqtt_retain "game" "${system}/${rom}"
                            fi
                        else
                            echo "$(date '+%H:%M:%S') BROWSE skipped (same game)" >> "$LOG"
                        fi
                    fi
                fi
            elif [ -n "$system" ]; then
                if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
                    LAST_SYSTEM="$system"
                    LAST_ROM=""
                    if [ "$throttled" -eq 0 ]; then
                        send_mqtt_retain "system" "$system"
                    fi
                else
                    echo "$(date '+%H:%M:%S') BROWSE skipped (same system)" >> "$LOG"
                fi
            else
                echo "$(date '+%H:%M:%S') BROWSE skipped (empty)" >> "$LOG"
            fi
            ;;

        rungame)
            # v6 -- transition definitive (vraie action utilisateur), pas un
            # simple survol -- reinitialise le detecteur de rafale pour ne
            # jamais laisser ce cas etre retarde par un throttle en cours.
            burst_count=0
            throttled=0
            IN_GAME=1
            # v8 -- lecture atomique (voir changelog v8).
            _snap=$(read_state_snapshot)
            system_raw=$(extract_field "$_snap" "SystemId")
            game_path=$(extract_field "$_snap" "GamePath")
            # "s/ //g" : voir commentaire du meme fix dans le bloc
            # gamelistbrowsing/systembrowsing plus haut (2026-08-09).
            rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
            system=$(normalize_system "$system_raw")

            echo "$(date '+%H:%M:%S') GAME sys=$system rom=$rom" >> "$LOG"

            if [ -n "$system" ] && [ -n "$rom" ]; then
                LAST_SYSTEM="$system"
                LAST_ROM="$rom"
                send_mqtt_retain "game" "${system}/${rom}"
                send_mqtt_retain "ingame" "1"
            fi
            ;;

        endgame)
            burst_count=0
            throttled=0
            IN_GAME=0
            LAST_ROM=""
            system_raw=$(read_state "SystemId")
            system=$(normalize_system "$system_raw")

            echo "$(date '+%H:%M:%S') ENDGAME sys=$system last=$LAST_SYSTEM" >> "$LOG"

            send_mqtt_retain "ingame" "0"
            if [ -n "$system" ]; then
                LAST_SYSTEM="$system"
                send_mqtt_retain "system" "$system"
            fi
            ;;

        stop)
            burst_count=0
            throttled=0
            echo "$(date '+%H:%M:%S') STOP -> playlist" >> "$LOG"
            IN_GAME=0
            LAST_ROM=""
            send_mqtt_retain "ingame" "0"
            send_mqtt_retain "default" "1"
            sleep 2
            ;;

        sleep)
            echo "$(date '+%H:%M:%S') SLEEP -> playlist" >> "$LOG"
            send_mqtt_retain "default" "1"
            ;;

        wakeup)
            echo "$(date '+%H:%M:%S') WAKEUP -> reaffiche last" >> "$LOG"
            if [ -n "$LAST_ROM" ] && [ -n "$LAST_SYSTEM" ]; then
                echo "$(date '+%H:%M:%S') WAKEUP -> jeu $LAST_SYSTEM/$LAST_ROM" >> "$LOG"
                send_mqtt_retain "game" "${LAST_SYSTEM}/${LAST_ROM}"
            elif [ -n "$LAST_SYSTEM" ]; then
                echo "$(date '+%H:%M:%S') WAKEUP -> systeme $LAST_SYSTEM" >> "$LOG"
                send_mqtt_retain "system" "$LAST_SYSTEM"
            else
                echo "$(date '+%H:%M:%S') WAKEUP -> playlist (rien de connu)" >> "$LOG"
                send_mqtt_retain "default" "1"
            fi
            ;;

        # Mode demo/veille EmulationStation (defilement automatique de clips
        # video) : ES ne publie ni "sleep" ni "wakeup" pour ce mode, juste
        # "startgameclip" en boucle toutes les ~30s -- sans ce cas, le DMD ne
        # repassait jamais en playlist pendant la demo (tombait dans le *)
        # ci-dessous, ignore). PREV_EVENT evite de renvoyer "default" a
        # chaque repetition (juste au moment ou on ENTRE en mode demo).
        startgameclip)
            if [ "$PREV_EVENT" != "startgameclip" ]; then
                echo "$(date '+%H:%M:%S') DEMO/VEILLE -> playlist" >> "$LOG"
                send_mqtt_retain "default" "1"
            fi
            ;;

        stopgameclip)
            ;;

        *)
            # Event inconnu ou vide
            ;;
    esac
done
