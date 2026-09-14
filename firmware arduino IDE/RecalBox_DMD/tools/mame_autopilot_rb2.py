#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# mame_autopilot_rb2.py -- campagne niveau1 (vrai .hi via vraie partie
# jouee) sur les 195 candidats "hiscore.dat confirme, aucune couverture
# niveau2" (2026-09-13). Remplace direct_harvest_mame0278_rb2.py sur un
# point CRITIQUE : ce dernier utilisait PLAYER1_SELECT/PLAYER1_START via
# UDP:55355, confirme etre un no-op silencieux (voir DECISIONS.md
# "PLAYER1_X n'a JAMAIS ete une vraie commande RetroArch") -- le credit+
# start de la campagne phase1 (5732 roms, 296 .hi) n'a donc probablement
# JAMAIS fonctionne. Ce script utilise le mecanisme REEL valide sur fbneo
# (rb2_uinput_gamepad.py, clone noyau du controleur Steam Deck) pour :
#   1. inserer un vrai credit (BTN_SELECT) + start (BTN_START)
#   2. jouer activement (mouvement + tir, pattern par genre via
#      mame_game_profiles.py) au lieu d'un dwell passif
# MAME (contrairement a fbneo) n'expose pas de dip switch Free_Play par
# jeu dans retroarch-core-options.cfg (verifie : 0 entree mame0278 dans
# ce fichier) -- credit reel obligatoire, pas de raccourci Free Play.
#
# Resultat campagne 2026-09-13/14 (195 roms, 120s/rom) : 51/195 .hi
# produits (49 avec score reellement non-nul) -- ~25% de reussite,
# contre ~5% pour la methode UDP historique. Detail complet dans
# DECISIONS.md "Campagne niveau1 autopilote mame0278 sur les 195
# candidats".
import argparse
import os
import subprocess
import socket
import sys
import time

os.environ.setdefault("DISPLAY", ":0")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/0")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rb2_input_device import detect_steam_deck_device  # noqa: E402
from rb2_uinput_gamepad import VirtualGamepad, BTN_SELECT, BTN_START  # noqa: E402
from mame_game_profiles import profile_for_rom  # noqa: E402

# v2 - 2026-09-14 - safe-modify - LOG/ATTEMPTED_FILE deplaces de /tmp
# (tmpfs, vide a CHAQUE redemarrage RB2) vers /recalbox/share/system/
# (stockage persistant reel) -- necessaire pour une campagne longue
# (2275 roms, ~3-4 jours estimes) qui doit survivre a un redemarrage
# RB2 eventuel sans perdre la progression. ROM_LIST_FILE reste
# parametrable (peut aussi pointer vers /tmp pour un test ponctuel).
PERSIST_DIR = "/recalbox/share/system/mame_autopilot"
os.makedirs(PERSIST_DIR, exist_ok=True)
ROM_LIST_FILE = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("--") else os.path.join(PERSIST_DIR, "target_list.txt")
LOG = os.path.join(PERSIST_DIR, "mame_autopilot_log.txt")
ATTEMPTED_FILE = os.path.join(PERSIST_DIR, "mame_autopilot_attempted.txt")
ROM_TEMPLATE = "/recalbox/share/roms/mame/mame0278/{rom}.zip"
SAVE_GLOB = "/recalbox/share/saves/mame/mame0278/hiscore/{rom}.hi"
UDP_ADDR = ("127.0.0.1", 55355)
LOAD_WAIT_S = 20
BOOT_SETTLE_S = 4
QUIT_WAIT_S = 15


def log(msg):
    line = "{} {}".format(time.strftime("%H:%M:%S"), msg)
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def find_retroarch_pid(rom):
    try:
        out = subprocess.check_output(
            "ps -o pid,args -ww | grep -- '/{}.zip' | grep retroarch | grep -v grep".format(rom),
            shell=True, text=True)
    except subprocess.CalledProcessError:
        return None
    line = out.strip().splitlines()[0] if out.strip() else ""
    return int(line.split()[0]) if line else None


def send_udp(cmd):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.sendto(cmd, UDP_ADDR)
        s.close()
    except Exception as e:
        log("!! erreur envoi {}: {}".format(cmd, e))


def send_screenshot():
    send_udp(b"SCREENSHOT\n")


def send_quit():
    send_udp(b"QUIT\n")


def send_credit_and_start(pad):
    """2 credits (BTN_SELECT) + 1 start (BTN_START), via le clone uinput
    REEL -- remplace le PLAYER1_SELECT/PLAYER1_START UDP confirme no-op."""
    pad.tap(BTN_SELECT, hold=0.08)
    time.sleep(0.4)
    pad.tap(BTN_SELECT, hold=0.08)
    time.sleep(0.4)
    pad.tap(BTN_START, hold=0.08)


def wait_gone(pid, timeout_s):
    waited = 0
    while waited < timeout_s:
        try:
            os.kill(pid, 0)
        except OSError:
            return True
        time.sleep(1)
        waited += 1
    return False


def mark_attempted(rom):
    with open(ATTEMPTED_FILE, "a") as f:
        f.write(rom + "\n")


def launch(rom, devicepath):
    cmd = [
        "python3", "/usr/bin/emulatorlauncher.pyc",
        "-p1index", "0", "-p1guid", "0300f617de2800000512000010010000",
        "-p1name", "Steam Deck", "-p1nbaxes", "10", "-p1nbhats", "0",
        "-p1nbbuttons", "22", "-p1devicepath", devicepath,
        "-p1physicalpath", "pci-0000:04:00.4-usb-0:3:1.2",
        "-system", "mame", "-rom", ROM_TEMPLATE.format(rom=rom),
        "-emulator", "libretro", "-core", "mame0278", "-ratio", "auto",
        "-videobackend", "default", "-rotation", "0", "-resolution", "1280x800",
        "-systemtype", "arcade",
    ]
    return subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def harvest_one(rom, attempted, play_seconds):
    save_path = SAVE_GLOB.format(rom=rom)
    if os.path.exists(save_path):
        log("(skip) {} -- .hi deja present".format(rom))
        return
    if rom in attempted:
        log("(skip) {} -- deja tente cette campagne".format(rom))
        return
    if not os.path.exists(ROM_TEMPLATE.format(rom=rom)):
        log("(skip) {} -- rom introuvable sur RB2".format(rom))
        mark_attempted(rom)
        return

    pad = VirtualGamepad().create()
    vpath = pad.event_path()
    if not vpath:
        log("!! {} : event_path() introuvable pour le clone uinput -- abandon".format(rom))
        pad.destroy()
        mark_attempted(rom)
        return

    proc = launch(rom, vpath)
    pid = None
    waited = 0
    while waited < LOAD_WAIT_S:
        pid = find_retroarch_pid(rom)
        if pid:
            break
        time.sleep(1)
        waited += 1

    if not pid:
        log(">>> {} : retroarch jamais apparu ({}s) -- abandon".format(rom, LOAD_WAIT_S))
        try:
            proc.terminate()
        except Exception:
            pass
        pad.destroy()
        time.sleep(2)
        mark_attempted(rom)
        return

    category, play_fn = profile_for_rom(rom)
    log(">>> {} lance (pid={}), profil={}".format(rom, pid, category))

    time.sleep(BOOT_SETTLE_S)
    send_credit_and_start(pad)
    time.sleep(2)
    # 2e essai a mi-parcours du boot -- couvre le cas "jeu pas encore pret
    # au 1er essai" (meme logique que direct_harvest_mame0278_rb2.py v7).
    send_credit_and_start(pad)
    time.sleep(1)

    log("    jeu actif ({}s, pattern {})".format(play_seconds, category))
    play_fn(pad, play_seconds)

    send_screenshot()
    time.sleep(1)

    send_quit()
    ended_cleanly = wait_gone(pid, QUIT_WAIT_S)
    if not ended_cleanly:
        log("!! {} n'a pas quitte apres QUIT ({}s) -- kill forcé".format(rom, QUIT_WAIT_S))
        try:
            os.kill(pid, 9)
        except OSError:
            pass
        time.sleep(2)

    pad.destroy()
    try:
        proc.wait(timeout=5)
    except Exception:
        pass

    if os.path.exists(save_path):
        size = os.path.getsize(save_path)
        with open(save_path, "rb") as f:
            data = f.read()
        populated = any(b != 0 for b in data)
        log("==> {} : .hi present ({} octets, {})".format(
            rom, size, "PEUPLE" if populated else "zero"))
    else:
        log("==> {} : pas de .hi".format(rom))
    mark_attempted(rom)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("rom_list", nargs="?", default="/tmp/priority_mame0278.txt")
    parser.add_argument("--play-seconds", type=int, default=90)
    args = parser.parse_args()

    with open(args.rom_list) as f:
        roms = [l.strip() for l in f if l.strip()]
    attempted = set()
    if os.path.exists(ATTEMPTED_FILE):
        with open(ATTEMPTED_FILE) as f:
            attempted = set(l.strip() for l in f if l.strip())
    log("=== mame_autopilot_rb2 demarre, {} roms en file, {} deja tentees, play_seconds={} ===".format(
        len(roms), len(attempted), args.play_seconds))
    for rom in roms:
        try:
            harvest_one(rom, attempted, args.play_seconds)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
        time.sleep(2)
    log("=== mame_autopilot_rb2 termine ===")


if __name__ == "__main__":
    main()
