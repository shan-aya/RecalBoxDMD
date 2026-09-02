#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v2
#
# v2 - 2026-08-31 - safe-modify - Retour utilisateur direct apres
#   verification sur ecran du 1er run : le pattern d'inputs "generique"
#   unique (v1) ne faisait PAS avancer le score sur un beat-em-up
#   (dynagear, boss combattu mais score reste a 0 -- attaque trop rare).
#   Remplace par fbneo_game_profiles.py (nouveau module, reprend un
#   principe deja amorce cote Codex/PLAY_PROFILES mais avec des
#   categories affinees et une lecture directe du genre gamelist.xml)
#   -- un pattern d'inputs different par famille de jeu (beat-em-up,
#   plateforme-shooter, plateforme pure, shmup horizontal, run-and-gun,
#   puzzle-viser-lancer, course, labyrinthe). Duree de jeu par defaut
#   portee a 120s (retour utilisateur : "augmente ta duree de jeu vu
#   que tu es autonome, ca n'a pas d'importance si tu dois jouer 2min
#   au lieu de 30s").
#
# v1 - 2026-08-31 - safe-modify - Creation initiale. Pipeline complet
#   d'auto-pilotage fbneo SANS AUCUNE intervention humaine, rendu
#   possible par la decouverte du dip switch Free Play exploitable via
#   set_freeplay.py (voir ce fichier pour le detail complet -- resout
#   definitivement le probleme d'automatisation credit qui avait fait
#   echouer 3 tentatives cette meme nuit, voir DECISIONS.md "Suite
#   immediate 20" puis "22"). Contrairement a rb2_fbneo_recipe_capture.py
#   (Codex, jamais committe dans tools/, utilisait PLAYER1_SELECT --
#   credit auto, structurellement non fonctionnel sur ce core), ce
#   script n'insere JAMAIS de credit -- il rend le credit inutile en
#   amont (Free Play), puis un SEUL appui START suffit.
#
#   VALIDE EN REEL sur dynagear (2026-08-31) : vraie partie demarree,
#   gameplay reel confirme par capture ecran. Chaine complete testee :
#   set_freeplay -> lancement -> probe fenetre RAM -> START -> inputs
#   generiques -> capture screenshot+RAM a intervalle regulier.
"""Pilote un jeu fbneo de bout en bout SANS intervention humaine :
active Free Play (set_freeplay.py), lance la rom, sonde la vraie
fenetre RAM, appuie sur START (credit inutile en Free Play), joue avec
des inputs generiques, capture screenshot+RAM a intervalle regulier.

Usage : rb2_fbneo_autopilot.py <rom> [--play-seconds 90] [--interval 5]
"""
import argparse
import glob
import json
import os
import socket
import subprocess
import sys
import time

os.environ.setdefault("DISPLAY", ":0")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/0")

OUT_ROOT = "/recalbox/share/system/rb_challenge_probe"
SCREENSHOT_DIR = "/recalbox/share/screenshots"
ROM_TEMPLATE = "/recalbox/share/roms/fbneo/arcade/{rom}.zip"
UDP_ADDR = ("127.0.0.1", 55355)
PROBE_POINTS = (0x10000, 0x18000, 0x20000, 0x30000, 0x40000, 0x60000,
                0x80000, 0x100000, 0x200000, 0x400000)
CHUNK = 16384

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rb2_input_device import detect_steam_deck_device  # noqa: E402
from fbneo_game_profiles import profile_for_rom  # noqa: E402


def log(msg):
    print("{} {}".format(time.strftime("%H:%M:%S"), msg), flush=True)


def retroarch_pid(rom):
    try:
        out = subprocess.check_output(
            "ps -o pid,args -ww | grep -- '/{}.zip' | grep retroarch | grep -v grep".format(rom),
            shell=True, text=True)
    except subprocess.CalledProcessError:
        return None
    line = out.strip().splitlines()[0] if out.strip() else ""
    return int(line.split()[0]) if line else None


def send(cmd):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.sendto((cmd + "\n").encode(), UDP_ADDR)
    finally:
        s.close()


def read_ram(addr, size):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(1.5)
    try:
        s.sendto("READ_CORE_RAM {:x} {}\n".format(addr, size).encode(), UDP_ADDR)
        reply, _ = s.recvfrom(200000)
    except (OSError, socket.timeout):
        return None
    finally:
        s.close()
    parts = reply.decode(errors="replace").strip().split()
    if len(parts) < 3 or parts[0] != "READ_CORE_RAM":
        return None
    try:
        return bytes(int(v, 16) for v in parts[2:])
    except ValueError:
        return None


def probe_ram_ceiling():
    ok_size = 0x10000
    for size in PROBE_POINTS:
        if read_ram(size - 16, 16) is None:
            break
        ok_size = size
    return ok_size


def dump_ram(path, base, size):
    data = bytearray()
    addr = base
    remaining = size
    while remaining > 0:
        n = min(CHUNK, remaining)
        block = read_ram(addr, n)
        if block is None or len(block) == 0:
            break
        data.extend(block)
        addr += len(block)
        remaining -= len(block)
        if len(block) < n:
            break
    with open(path, "wb") as f:
        f.write(bytes(data))
    return len(data)


def latest_screenshot(after):
    candidates = [p for p in glob.glob(os.path.join(SCREENSHOT_DIR, "*.png"))
                  if os.path.getmtime(p) >= after]
    return max(candidates, key=os.path.getmtime) if candidates else None


def capture_one(rom_dir, index, dump_base, dump_size):
    phase = "obs{:02d}".format(index)
    started = time.time()
    send("SCREENSHOT")
    ram_path = os.path.join(rom_dir, "{}.ram.bin".format(phase))
    got = dump_ram(ram_path, dump_base, dump_size)
    time.sleep(1.0)
    source = latest_screenshot(started - 0.2)
    screenshot = None
    if source:
        screenshot = os.path.join(rom_dir, "{}.png".format(phase))
        try:
            os.rename(source, screenshot)
        except OSError:
            screenshot = source
    return {"phase": phase, "at": started, "screenshot": screenshot,
            "ram": ram_path, "ram_bytes": got}


def set_freeplay(rom):
    """Reutilise la logique de set_freeplay.py directement (evite un
    sous-process pour rester dans le meme script)."""
    path = "/recalbox/share/system/configs/retroarch/cores/retroarch-core-options.cfg"
    fields = ["Free_Play", "Coinage", "Coin_A", "Coin_B"]
    with open(path, encoding="utf-8") as f:
        lines = f.readlines()
    changed = []
    for field in fields:
        key = "fbneo-dipswitch-{}-{} = ".format(rom, field)
        for i, line in enumerate(lines):
            if line.startswith(key):
                value = "On" if field == "Free_Play" else "Free Play"
                lines[i] = '{}"{}"\n'.format(key, value)
                changed.append(field)
    if changed:
        with open(path, "w", encoding="utf-8") as f:
            f.writelines(lines)
    return changed


def launch(rom):
    device = detect_steam_deck_device()
    cmd = [
        "python3", "/usr/bin/emulatorlauncher.pyc",
        "-p1index", "0", "-p1guid", "0300f617de2800000512000010010000",
        "-p1name", "Steam Deck", "-p1nbaxes", "10", "-p1nbhats", "0",
        "-p1nbbuttons", "22", "-p1devicepath", device,
        "-p1physicalpath", "pci-0000:04:00.4-usb-0:3:1.2",
        "-system", "fbneo", "-rom", ROM_TEMPLATE.format(rom=rom),
        "-emulator", "libretro", "-core", "fbneo", "-ratio", "auto",
        "-videobackend", "default", "-rotation", "0", "-resolution", "1280x800",
        "-systemtype", "arcade",
    ]
    return subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)




def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("rom")
    parser.add_argument("--play-seconds", type=int, default=120,
                        help="duree totale de jeu (defaut 120s -- pas de cout en Free Play, "
                             "retour utilisateur : autant jouer plus longtemps pour des "
                             "screenshots plus surs)")
    parser.add_argument("--interval", type=float, default=10.0)
    parser.add_argument("--load-wait", type=int, default=20)
    parser.add_argument("--boot-wait", type=int, default=15)
    args = parser.parse_args()

    changed = set_freeplay(args.rom)
    if changed:
        log("Free Play active via : {}".format(", ".join(changed)))
    else:
        log("!! aucun dip switch de credit trouve pour '{}' -- le jeu n'a "
            "probablement jamais ete lance (retroarch-core-options.cfg vide "
            "pour ce jeu). Un lancement manuel prealable peut etre necessaire.".format(args.rom))

    if not os.path.exists(ROM_TEMPLATE.format(rom=args.rom)):
        log("!! rom introuvable : {}".format(ROM_TEMPLATE.format(rom=args.rom)))
        return

    process = launch(args.rom)
    pid = None
    for _ in range(args.load_wait):
        pid = retroarch_pid(args.rom)
        if pid:
            break
        time.sleep(1)
    if not pid:
        log("!! jamais lance apres {}s -- abandon".format(args.load_wait))
        return
    log(">>> lance (pid={}), attente boot {}s".format(pid, args.boot_wait))
    time.sleep(2)
    dump_size = probe_ram_ceiling()
    log(">>> fenetre RAM sondee : 0x{:x}".format(dump_size))
    time.sleep(args.boot_wait)

    session_dir = os.path.join(OUT_ROOT, args.rom, time.strftime("%Y%m%d-%H%M%S"))
    os.makedirs(session_dir, exist_ok=True)

    category, play_fn = profile_for_rom(args.rom)
    log(">>> profil de jeu : {} (d'apres gamelist.xml)".format(category))

    events = [capture_one(session_dir, 0, 0, dump_size)]
    log("    obs00 (avant START) : ram={} octets".format(events[-1]["ram_bytes"]))

    log(">>> UN SEUL appui START (Free Play -- pas de credit necessaire)")
    send("PLAYER1_START")
    time.sleep(3)

    index = 1
    deadline = time.time() + args.play_seconds
    while time.time() < deadline:
        if not retroarch_pid(args.rom):
            log("jeu quitte de lui-meme -- arret")
            break
        play_fn(send, min(args.interval, deadline - time.time()))
        event = capture_one(session_dir, index, 0, dump_size)
        events.append(event)
        log("    obs{:02d} : ram={} octets".format(index, event["ram_bytes"]))
        index += 1

    with open(os.path.join(session_dir, "run.json"), "w", encoding="utf-8") as f:
        json.dump({"rom": args.rom, "dump_size": dump_size, "events": events}, f, indent=2)

    send("QUIT")
    time.sleep(3)
    pid2 = retroarch_pid(args.rom)
    if pid2:
        try:
            os.kill(pid2, 9)
        except OSError:
            pass
    log("=== termine : {} observations dans {} ===".format(len(events), session_dir))


if __name__ == "__main__":
    main()
