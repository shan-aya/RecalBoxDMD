#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# direct_harvest_mame0278_rb2.py v1 -- variante de direct_harvest_mame0278.py
# (RB1, JAMMA/CRT) adaptee au materiel x86/X11 (ex. Steam Deck, pas de
# CRT/JAMMA, manette integree, controleur graphique nécessitant DISPLAY).
# Ligne de lancement capturee en reel le 2026-08-23 (jeu lance normalement
# via ES) -- uniquement -system/-rom/-core changent d'un jeu a l'autre.
#
# BUG REEL trouve/corrige (2026-08-23, voir DECISIONS.md "Récolte multi-RB")
# : sans DISPLAY/XDG_RUNTIME_DIR exportes, retroarch segfaultait
# systematiquement des le 2e lancement consecutif (jump a un pointeur de
# fonction NULL) -- confirme via /proc/<pid du process openbox lance par
# ES>/environ (DISPLAY=:0, XDG_RUNTIME_DIR=/run/user/0), absents d'une
# session SSH nue. Valide : 10/10 lancements consecutifs reussis avec le
# fix, 0/3 sans (peu importe le jeu, l'etat d'ES, ou le shader).
import subprocess, time, socket, os, sys

os.environ["DISPLAY"] = ":0"
os.environ["XDG_RUNTIME_DIR"] = "/run/user/0"

ROM_LIST_FILE = sys.argv[1] if len(sys.argv) > 1 else "/tmp/priority_mame0278.txt"
LOG = "/tmp/direct_harvest_mame0278_log.txt"
LAUNCH_TEMPLATE = (
    "python3 /usr/bin/emulatorlauncher.pyc "
    "-p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name \"Steam Deck\" "
    "-p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath /dev/input/event14 "
    "-p1physicalpath \"pci-0000:04:00.4-usb-0:3:1.2\" "
    "-system mame -rom /recalbox/share/roms/mame/mame0278/{rom}.zip -emulator libretro -core mame0278 "
    "-ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade"
)
LOAD_WAIT_S = 20
DWELL_S = 15
QUIT_WAIT_S = 15
SAVE_GLOB = "/recalbox/share/saves/mame/mame0278/hiscore/{rom}.hi"


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
    if not line:
        return None
    return int(line.split()[0])


def send_quit():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.sendto(b"QUIT\n", ("127.0.0.1", 55355))
        s.close()
    except Exception as e:
        log("!! erreur envoi QUIT: {}".format(e))


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


def harvest_one(rom):
    save_path = SAVE_GLOB.format(rom=rom)
    if os.path.exists(save_path):
        log("(skip) {} -- .hi deja present".format(rom))
        return

    cmd = LAUNCH_TEMPLATE.format(rom=rom)
    proc = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

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
        time.sleep(2)
        return

    log(">>> {} lance (pid={}), attente {}s".format(rom, pid, DWELL_S))
    time.sleep(DWELL_S)

    send_quit()
    ended_cleanly = wait_gone(pid, QUIT_WAIT_S)
    if not ended_cleanly:
        log("!! {} n'a pas quitte apres QUIT ({}s) -- kill forcé".format(rom, QUIT_WAIT_S))
        try:
            os.kill(pid, 9)
        except OSError:
            pass
        time.sleep(2)

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


def main():
    with open(ROM_LIST_FILE) as f:
        roms = [l.strip() for l in f if l.strip()]
    log("=== direct_harvest_mame0278_rb2 demarre, {} roms en file ===".format(len(roms)))
    for rom in roms:
        try:
            harvest_one(rom)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
        time.sleep(2)
    log("=== direct_harvest_mame0278_rb2 termine ===")
    log("=== redemarrage EmulationStation ===")
    subprocess.call("/etc/init.d/S31emulationstation start", shell=True)


if __name__ == "__main__":
    main()
