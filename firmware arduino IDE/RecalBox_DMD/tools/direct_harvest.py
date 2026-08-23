#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# direct_harvest.py v1 -- orchestrateur de recolte Phase 2 sans passer par
# le mode demo d'ES (voir discussion 2026-08-23 : mode demo pas concu pour
# ca, timer/input capricieux confirme en reel ce soir sur battlex). Lance
# chaque rom manquante directement via emulatorlauncher.pyc (memes
# arguments que ceux captures depuis un vrai lancement demo ES, juste sans
# -demo/-demoduration/-demoinfoduration), attend le chargement, envoie un
# QUIT propre via l'interface reseau RetroArch (UDP 127.0.0.1:55355), verifie
# le .hi, passe au suivant. ES DOIT etre arrete avant de lancer ce script
# (conflit KMS sinon) -- voir /etc/init.d/S31emulationstation stop/start.
#
# Cible : FBNeo, matériel type Raspberry Pi (framebuffer/KMS direct, ex.
# RB1). Pour une machine x86/X11 (ex. Steam Deck), voir
# direct_harvest_mame0278_rb2.py -- DISPLAY/XDG_RUNTIME_DIR y sont requis
# (voir DECISIONS.md "Récolte multi-RB").
import subprocess, time, socket, os, sys

ROM_LIST_FILE = sys.argv[1] if len(sys.argv) > 1 else "/tmp/priority_roms.txt"
LOG = "/tmp/direct_harvest_log.txt"
LAUNCH_TEMPLATE = (
    "python3 /usr/bin/emulatorlauncher.pyc "
    "-p1index 0 -p1guid 1800819562720000010a000000010000 -p1name JammaControllerP1 "
    "-p1nbaxes 4 -p1nbhats 0 -p1nbbuttons 13 -p1devicepath /dev/input/event1 -p1physicalpath \"\" "
    "-p2index 1 -p2guid 1800c19462720000010a000000010000 -p2name JammaControllerP2 "
    "-p2nbaxes 4 -p2nbhats 0 -p2nbbuttons 13 -p2devicepath /dev/input/event2 -p2physicalpath \"\" "
    "-system fbneo -rom /recalbox/share/roms/fbneo/arcade/{rom}.zip -emulator libretro -core fbneo "
    "-ratio auto -videobackend default -crtadaptor recalboxrgbjamma -crtsuperrez 1 "
    "-crtresolutiontype auto -crtscreentype kHz15 -crtsignaltype RGB -crtscanlines none "
    "-crtvideostandard auto -crtregion auto -rotation 0 -resolution 640x480 "
    "-jammalayoutp1 line -jammalayoutp2 line -systemtype arcade"
)
LOAD_WAIT_S = 20      # temps max d'attente d'apparition du process retroarch
DWELL_S = 15          # temps laisse au core une fois charge (init hiscore.dat)
QUIT_WAIT_S = 15      # temps laisse apres QUIT avant de verifier/forcer
SAVE_GLOB = "/recalbox/share/saves/fbneo/fbneo/{rom}.hi"


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

    # laisse le launcher/wrapper python se nettoyer
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
    log("=== direct_harvest demarre, {} roms en file ===".format(len(roms)))
    for rom in roms:
        try:
            harvest_one(rom)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
        time.sleep(2)
    log("=== direct_harvest termine ===")
    log("=== redemarrage EmulationStation ===")
    subprocess.call("/etc/init.d/S31emulationstation start", shell=True)


if __name__ == "__main__":
    main()
