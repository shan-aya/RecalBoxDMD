#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-09-08 - safe-modify - Creation. Piste UDP full (voir
#   TRANSPORT_PLAN_UDP.md, RecalBox_DMD.ino v161 MQTT_ENABLED=false,
#   marquee.sh v45/dmd_score.sh v48) : sans retain MQTT, un DMD qui
#   reboote/perd le WiFi en cours de session reste fige sur son dernier
#   etat jusqu'au prochain evenement REEL de navigation -- ce script comble
#   ce trou. Ecoute en permanence un "hello" UDP envoye par le DMD a chaque
#   connexion/reconnexion WiFi (voir sendUdpHello() cote firmware) ; a
#   chaque hello recu, relit /tmp/es_state.inf A NEUF (pas un etat mis en
#   cache) et renvoie l'etat REEL courant en UDP -- garantit que le DMD
#   rattrape la bonne position meme s'il a manque plusieurs changements
#   pendant sa coupure. Duplique volontairement (pas factorise) la logique
#   de decision Action=rungame/gamelistbrowsing deja presente en shell dans
#   marquee.sh (case start), meme motif que send_udp() duplique plutot que
#   factorise entre marquee.sh/dmd_score.sh : la logique shell est un
#   chemin eprouve depuis des mois, ne pas la restructurer pour ce
#   mecanisme pas encore valide en charge. PAS ENCORE TESTE SUR MATERIEL.
#
# ============================================
#
# Lance en arriere-plan par marquee.sh a son demarrage (processus persistant
# associe au singleton marquee -- meme cycle de vie que features_watcher(),
# voir marquee.sh v45). N'ecoute PAS les evenements ES : purement reactif au
# "hello" UDP du DMD, rien d'autre. Etat mis a jour ("resync") = meme
# format de payload que MQTT/marquee.sh ("CMD=<nom> ARG=<valeur>"), envoye
# directement au DMD par socket UDP brut (memes port/IP que send_udp() cote
# shell).

import os
import re
import socket
import sys
import time

UDP_HELLO_PORT = 5006  # doit rester identique a UDP_HELLO_PORT, RecalBox_DMD.ino
DMD_UDP_IP = "192.168.0.51"  # EN DUR pour l'instant, voir marquee.sh DMD_UDP_IP (meme limite)
DMD_UDP_PORT = 5005  # doit rester identique a UDP_CMD_PORT, RecalBox_DMD.ino
ES_STATE_PATH = "/tmp/es_state.inf"
LOG_PATH = "/recalbox/share/system/logs/dmd_udp_resync.log"


def log(msg):
    ts = time.strftime("%H:%M:%S")
    line = f"{ts} {msg}"
    try:
        with open(LOG_PATH, "a") as f:
            f.write(line + "\n")
    except Exception:
        pass


def extract_field(content, key):
    # meme motif que extract_field() en shell (marquee.sh/dmd_score.sh) :
    # une ligne "KEY=valeur", valeur prend tout le reste de la ligne.
    m = re.search(rf"^{re.escape(key)}=(.*)$", content, re.MULTILINE)
    return m.group(1).strip() if m else ""


def normalize_rom(game_path):
    # meme motif que basename+sed dans marquee.sh (retire l'extension,
    # retire les espaces) -- pas de garantie de correspondance parfaite
    # avec normalize_system()/le detail des conventions ES, suffisant pour
    # ce mecanisme de secours (resync post-coupure, pas le chemin principal).
    base = os.path.basename(game_path)
    base = re.sub(r"\.[^.]*$", "", base)
    base = base.replace(" ", "")
    return base


def compute_current_state():
    """Relit es_state.inf A NEUF et retourne une liste de (cmd, arg) a
    envoyer -- meme decision que le case start) de marquee.sh (v17/v32/v33,
    voir son commentaire complet)."""
    try:
        with open(ES_STATE_PATH, "r", errors="replace") as f:
            content = f.read()
    except Exception as e:
        log(f"ERREUR lecture {ES_STATE_PATH}: {e}")
        return [("default", "1")]

    action = extract_field(content, "Action")
    game_path = extract_field(content, "GamePath")
    system_id = extract_field(content, "SystemId")

    if action == "rungame" and game_path:
        rom = normalize_rom(game_path)
        if system_id and rom:
            return [("game", f"{system_id}/{rom}"), ("ingame", "1")]
        return [("default", "1")]

    if action in ("systembrowsing", "gamelistbrowsing") and game_path and not os.path.isdir(game_path):
        rom = normalize_rom(game_path)
        if system_id and rom:
            return [("game", f"{system_id}/{rom}")]
        return [("default", "1")]

    return [("default", "1")]


def send_udp(sock, cmd, arg):
    payload = f"CMD={cmd} ARG={arg}".encode("utf-8", "replace")
    sock.sendto(payload, (DMD_UDP_IP, DMD_UDP_PORT))


def main():
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listen_sock.bind(("0.0.0.0", UDP_HELLO_PORT))
    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    log(f"demarre, ecoute UDP {UDP_HELLO_PORT}, resync vers {DMD_UDP_IP}:{DMD_UDP_PORT}")

    while True:
        try:
            data, addr = listen_sock.recvfrom(256)
        except Exception as e:
            log(f"ERREUR recvfrom: {e}")
            time.sleep(1)
            continue

        # v1 -- tout paquet recu sur ce port est traite comme un "hello" --
        # le contenu exact n'est pas verifie (seul le DMD envoie ici en
        # pratique, port dedie non expose ailleurs).
        cmds = compute_current_state()
        log(f"hello de {addr[0]} -> resync {cmds}")
        for cmd, arg in cmds:
            send_udp(send_sock, cmd, arg)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
