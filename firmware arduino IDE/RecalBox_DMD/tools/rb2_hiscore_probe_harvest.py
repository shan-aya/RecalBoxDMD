#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-25 - safe-modify - Creation initiale. Portage du script
#   shell scratchpad ad-hoc (batch_plugin_16.sh/batch_plugin_v2.sh,
#   session precedente) vers un outil versionne du depot, meme role que
#   rb2_lua_memmap_harvest.py mais pour le plugin MAME reel
#   `mame_plugin_hiscore_probe` (voir tools/mame_plugin_hiscore_probe/
#   et DECISIONS.md "Suite immediate -- vrai plugin MAME") au lieu du
#   dump one-shot autoboot_script.
#
#   Active/desactive le plugin via plugin.ini AUTOUR du lot uniquement
#   (jamais laisse actif par defaut -- il presse credit+start sur
#   CHAQUE lancement MAME tant qu'actif, interfererait avec d'autres
#   campagnes de recolte en cours) -- meme si le script est interrompu
#   en cours de route (Ctrl-C/exception), `finally` le redesactive.
#
#   Prerequis sur RB2 (mis en place manuellement, voir DECISIONS.md
#   pour la recette complete) : pluginspath de mame.ini etendu en liste
#   (chemin lecture seule d'origine + /recalbox/share/bios/mame/plugins,
#   ecrivable), plugin deploye sous
#   /recalbox/share/bios/mame/plugins/hiscore_probe/.
"""Lance le plugin MAME hiscore_probe (sequence credit->start->dwell,
snapshots RAM) sur une liste de roms mame0278.

Usage: rb2_hiscore_probe_harvest.py <rom_list_file> [--timeout-s 90]
"""
import os
import subprocess
import sys
import time

os.environ["DISPLAY"] = ":0"
os.environ["XDG_RUNTIME_DIR"] = "/run/user/0"

sys.path.insert(0, os.path.dirname(__file__))
from rb2_input_device import detect_steam_deck_device  # noqa: E402

LOG = "/tmp/rb2_hiscore_probe_log.txt"
OUT_DIR = "/tmp/hiscore_probe_harvest"
SNAPSHOT_SRC = "/tmp/mame_lua_snapshot.txt"
PLUGIN_INI = "/recalbox/share/bios/mame/ini/plugin.ini"
LOAD_WAIT_S = 8
QUIT_WAIT_S = 3

DEVICE_PATH = detect_steam_deck_device()
LAUNCH_TEMPLATE = (
    "python3 /usr/bin/emulatorlauncher.pyc "
    "-p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name \"Steam Deck\" "
    "-p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath {device} "
    "-p1physicalpath \"pci-0000:04:00.4-usb-0:3:1.2\" "
    "-system mame -rom /recalbox/share/roms/mame/mame0278/{{rom}}.zip -emulator libretro -core mame0278 "
    "-ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade"
).format(device=DEVICE_PATH)


def log(msg):
    line = "{} {}".format(time.strftime("%H:%M:%S"), msg)
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def set_plugin_enabled(enabled):
    with open(PLUGIN_INI, "w") as f:
        f.write("hiscore_probe     {}\n".format(1 if enabled else 0))


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
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.sendto(b"QUIT\n", ("127.0.0.1", 55355))
        s.close()
    except Exception as e:
        log("!! erreur envoi QUIT: {}".format(e))


def harvest_one(rom, timeout_s):
    out_path = os.path.join(OUT_DIR, "{}.snapshot.txt".format(rom))
    try:
        os.remove(SNAPSHOT_SRC)
    except OSError:
        pass

    cmd = LAUNCH_TEMPLATE.format(rom=rom)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(LOAD_WAIT_S)
    pid = find_retroarch_pid(rom)
    if not pid:
        log(">>> {} : retroarch jamais apparu -- abandon".format(rom))
        try:
            proc.terminate()
        except Exception:
            pass
        time.sleep(2)
        return
    log(">>> {} lance (pid={})".format(rom, pid))

    waited = 0
    done = False
    while waited < timeout_s:
        if os.path.exists(SNAPSHOT_SRC):
            with open(SNAPSHOT_SRC, encoding="utf-8", errors="replace") as f:
                if "#done" in f.read():
                    done = True
                    break
        time.sleep(1)
        waited += 1

    send_quit()
    time.sleep(QUIT_WAIT_S)
    try:
        os.kill(pid, 9)
    except OSError:
        pass
    time.sleep(2)
    try:
        proc.wait(timeout=5)
    except Exception:
        pass

    if os.path.exists(SNAPSHOT_SRC):
        with open(SNAPSHOT_SRC, encoding="utf-8", errors="replace") as f:
            content = f.read()
        zones_line = next((l for l in content.splitlines() if l.startswith("#zones")), "?")
        snap_count = content.count("\nSNAP\t") + (1 if content.startswith("SNAP\t") else 0)
        os.rename(SNAPSHOT_SRC, out_path)
        log("==> {} : {} ({} apres {}s, {} lignes SNAP)".format(
            rom, zones_line, "termine" if done else "TIMEOUT", waited, snap_count))
    else:
        log("==> {} : AUCUN snapshot produit".format(rom))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    rom_list_file = sys.argv[1]
    timeout_s = 90
    if "--timeout-s" in sys.argv:
        timeout_s = int(sys.argv[sys.argv.index("--timeout-s") + 1])

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(rom_list_file) as f:
        roms = [l.strip() for l in f if l.strip()]

    log("=== rb2_hiscore_probe_harvest demarre, {} roms, timeout={}s ===".format(len(roms), timeout_s))
    set_plugin_enabled(True)
    try:
        for rom in roms:
            try:
                harvest_one(rom, timeout_s)
            except Exception as e:
                log("!! exception sur {}: {}".format(rom, e))
            time.sleep(2)
    finally:
        set_plugin_enabled(False)
        log("=== plugin desactive, lot termine ===")


if __name__ == "__main__":
    main()
