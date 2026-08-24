#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-24 - safe-modify - Creation initiale. Recolte en masse la
#   carte memoire COMPLETE de chaque rom via l'API Lua NATIVE de MAME
#   (tools/dump_full_memmap.lua, autoboot_script) -- independant de
#   READ_CORE_RAM (RetroArch), qui ne couvre que ~30-50% des drivers
#   (echoue -1 sur du hardware exotique type generateur vectoriel,
#   confirme sur batlzone/batman cette nuit). Le dump Lua, lui, a MARCHE
#   sur batlzone (carte memoire complete obtenue, dont un handler
#   "mayday_state::protection_r" qui confirme au passage que batlzone et
#   mayday partagent bien la meme classe de driver MAME).
#
#   PREREQUIS SUR RB2 (mis en place manuellement ce soir, PERSISTANT --
#   survit a un reboot car sous /recalbox/share, pas /tmp) :
#   - options core RetroArch (retroarch-core-options.cfg) :
#     mame_mame_paths_enable=enabled ET mame_read_config=enabled (les
#     DEUX ensemble -- un seul active mais l'autre desactive fait
#     planter le core, `mame_read_config` etait deja enabled par defaut
#     mais verifier si jamais modifie depuis).
#   - /recalbox/share/bios/mame/ini/mame.ini (rompath/pluginspath/
#     cfg_directory/nvram_directory/diff_directory/state_directory/
#     snapshot_directory pointant vers des chemins EXISTANTS et
#     inscriptibles -- un mame.ini minimal avec juste autoboot_script
#     fait planter le core immediatement, `Process exitcode: 1` en
#     moins d'1s, chasse en direct cette nuit) + autoboot_script pointant
#     vers /recalbox/share/bios/mame/ini/dump_full_memmap.lua (copie
#     persistante du script de ce depot, PAS /tmp).
#   Consequence collaterale ACCEPTEE : ce script Lua tourne desormais a
#   CHAQUE lancement mame0278 sur cette machine (y compris la recolte
#   .hi normale) -- inoffensif (ecrit juste un petit fichier de plus a
#   chaque partie), mais a savoir pour ne pas etre surpris de voir
#   /tmp/mame_lua_memmap.txt se mettre a jour pendant d'autres taches.
#
#   Format de sortie (voir dump_full_memmap.lua pour le detail complet
#   des champs) : un fichier <rom>.memmap.txt par jeu dans OUT_DIR, TSV
#   (cpu_tag, space, addr_start, addr_end, read_type, read_name,
#   write_type, write_name, share, region) -- exploitable directement en
#   filtrant read_type=="ram" ou write_type=="ram" pour ne garder que les
#   candidats RAM (score/vies/credits/continue potentiels), sans avoir a
#   relancer quoi que ce soit si le filtre doit changer plus tard.
import os
import subprocess
import sys
import time

os.environ["DISPLAY"] = ":0"
os.environ["XDG_RUNTIME_DIR"] = "/run/user/0"

sys.path.insert(0, os.path.dirname(__file__))
from rb2_input_device import detect_steam_deck_device  # noqa: E402

LOG = "/tmp/rb2_lua_memmap_log.txt"
OUT_DIR = "/tmp/lua_memmap"
MEMMAP_SRC = "/tmp/mame_lua_memmap.txt"
ATTEMPTED_FILE = "/tmp/rb2_lua_memmap_attempted.txt"

DEVICE_PATH = detect_steam_deck_device()
LAUNCH_TEMPLATE = (
    "python3 /usr/bin/emulatorlauncher.pyc "
    "-p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name \"Steam Deck\" "
    "-p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath {device} "
    "-p1physicalpath \"pci-0000:04:00.4-usb-0:3:1.2\" "
    "-system mame -rom /recalbox/share/roms/mame/mame0278/{{rom}}.zip -emulator libretro -core mame0278 "
    "-ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade"
).format(device=DEVICE_PATH)
LOAD_WAIT_S = 20
DWELL_S = 5  # le dump Lua tourne a l'autoboot, tres tot -- pas besoin
             # d'attendre longtemps, contrairement a la recolte .hi
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


def count_ram_entries(path):
    """Compte rapidement les entrees dont le handler lecture OU ecriture
    est 'ram' -- indicateur immediat de richesse du dump sans avoir a
    l'ouvrir manuellement."""
    count = 0
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                cols = line.rstrip("\n").split("\t")
                if len(cols) >= 7 and (cols[4] == "ram" or cols[6] == "ram"):
                    count += 1
    except Exception:
        pass
    return count


def harvest_one(rom, attempted):
    out_path = os.path.join(OUT_DIR, "{}.memmap.txt".format(rom))
    if os.path.exists(out_path):
        log("(skip) {} -- memmap deja present".format(rom))
        return
    if rom in attempted:
        log("(skip) {} -- deja tente cette campagne".format(rom))
        return

    try:
        os.remove(MEMMAP_SRC)
    except OSError:
        pass

    cmd = LAUNCH_TEMPLATE.format(rom=rom)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

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
        mark_attempted(rom)
        return

    log(">>> {} lance (pid={})".format(rom, pid))
    time.sleep(DWELL_S)

    send_quit()
    ended_cleanly = wait_gone(pid, QUIT_WAIT_S)
    if not ended_cleanly:
        log("!! {} n'a pas quitte apres QUIT ({}s) -- kill force".format(rom, QUIT_WAIT_S))
        try:
            os.kill(pid, 9)
        except OSError:
            pass
        time.sleep(2)

    try:
        proc.wait(timeout=5)
    except Exception:
        pass

    if os.path.exists(MEMMAP_SRC):
        os.rename(MEMMAP_SRC, out_path)
        ram_count = count_ram_entries(out_path)
        log("==> {} : memmap capture ({} entrees RAM)".format(rom, ram_count))
    else:
        log("==> {} : PAS de memmap produit (autoboot_script n'a pas tourne ou a plante)".format(rom))
    mark_attempted(rom)


def main():
    if len(sys.argv) < 2:
        print("usage: rb2_lua_memmap_harvest.py <rom_list_file>")
        return
    rom_list_file = sys.argv[1]
    os.makedirs(OUT_DIR, exist_ok=True)

    with open(rom_list_file) as f:
        roms = [l.strip() for l in f if l.strip()]
    attempted = set()
    if os.path.exists(ATTEMPTED_FILE):
        with open(ATTEMPTED_FILE) as f:
            attempted = set(l.strip() for l in f if l.strip())

    log("=== rb2_lua_memmap_harvest demarre, {} roms en file, {} deja tentees ===".format(
        len(roms), len(attempted)))
    for rom in roms:
        try:
            harvest_one(rom, attempted)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
        time.sleep(2)
    log("=== rb2_lua_memmap_harvest termine ===")


if __name__ == "__main__":
    main()
