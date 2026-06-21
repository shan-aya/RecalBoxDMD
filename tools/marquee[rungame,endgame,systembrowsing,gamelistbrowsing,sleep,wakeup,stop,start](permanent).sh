#!/bin/ash
LOG="/recalbox/share/system/logs/marquee_mqtt.log"

read_state() {
    grep "^${1}=" "/tmp/es_state.inf" 2>/dev/null | cut -d= -f2- | tr -d '\r\n '
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

echo "$(date) - Marquee bridge started" >> "$LOG"

send_mqtt_retain "default" "1"

LAST_SYSTEM=""
LAST_ROM=""
IN_GAME=0
BOOT_TIME=0

while true; do
    event=$(mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 \
        -t "Recalbox/EmulationStation/Event" -C 1 2>/dev/null | tr -d '\r')

    echo "$(date '+%H:%M:%S') EVENT=$event IN_GAME=$IN_GAME LAST_SYS=$LAST_SYSTEM LAST_ROM=$LAST_ROM" >> "$LOG"

    case "$event" in

        start)
            IN_GAME=0
            LAST_ROM=""
            LAST_SYSTEM=""
            BOOT_TIME=$(date +%s)
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

            system_raw=$(read_state "SystemId")
            system=$(normalize_system "$system_raw")
            game_path=$(read_state "GamePath")

            echo "$(date '+%H:%M:%S') BROWSE raw=$system_raw norm=$system game=$game_path in_game=$IN_GAME" >> "$LOG"

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
                        send_mqtt_retain "system" "$system"
                    fi
                else
                    rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                    if [ -n "$system" ] && [ -n "$rom" ]; then
                        if [ "$rom" != "$LAST_ROM" ] || [ "$system" != "$LAST_SYSTEM" ]; then
                            LAST_SYSTEM="$system"
                            LAST_ROM="$rom"
                            send_mqtt_retain "game" "${system}/${rom}"
                        else
                            echo "$(date '+%H:%M:%S') BROWSE skipped (same game)" >> "$LOG"
                        fi
                    fi
                fi
            elif [ -n "$system" ]; then
                if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
                    LAST_SYSTEM="$system"
                    LAST_ROM=""
                    send_mqtt_retain "system" "$system"
                else
                    echo "$(date '+%H:%M:%S') BROWSE skipped (same system)" >> "$LOG"
                fi
            else
                echo "$(date '+%H:%M:%S') BROWSE skipped (empty)" >> "$LOG"
            fi
            ;;

        rungame)
            IN_GAME=1
            system_raw=$(read_state "SystemId")
            game_path=$(read_state "GamePath")
            rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
            system=$(normalize_system "$system_raw")

            echo "$(date '+%H:%M:%S') GAME sys=$system rom=$rom" >> "$LOG"

            if [ -n "$system" ] && [ -n "$rom" ]; then
                LAST_SYSTEM="$system"
                LAST_ROM="$rom"
                send_mqtt_retain "game" "${system}/${rom}"
            fi
            ;;

        endgame)
            IN_GAME=0
            LAST_ROM=""
            system_raw=$(read_state "SystemId")
            system=$(normalize_system "$system_raw")

            echo "$(date '+%H:%M:%S') ENDGAME sys=$system last=$LAST_SYSTEM" >> "$LOG"

            if [ -n "$system" ]; then
                LAST_SYSTEM="$system"
                send_mqtt_retain "system" "$system"
            fi
            ;;

        stop)
            echo "$(date '+%H:%M:%S') STOP -> playlist" >> "$LOG"
            IN_GAME=0
            LAST_ROM=""
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

        *)
            ;;
    esac
done
