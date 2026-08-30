#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v3
#
# v3 - 2026-08-30 - safe-modify - Retour utilisateur ("il faut que tu
#   testes la RAM avant de programmer tes screenshots pour caler la
#   taille de cette derniere") : la fenetre par defaut (0x20000) etait
#   devinee a l'aveugle -- plusieurs jeux ce soir avaient une fenetre
#   READ_CORE_RAM reelle bien plus grande (jusqu'a 0x100000, voir
#   DECISIONS.md "Suite immediate 19"), jamais capturee car jamais
#   demandee. Ajoute probe_ram_ceiling() : sonde la vraie limite juste
#   apres detection du jeu, AVANT de commencer la boucle de capture --
#   --dump-size devient un PLAFOND optionnel (sonde quand meme, mais ne
#   depasse jamais la valeur donnee) plutot qu'une taille fixe imposee.
#
# v2 - 2026-08-29
#
# v2 - 2026-08-29 - safe-modify - 1er test reel (joemacr, 17 observations,
#   3 scores lus a l'oeil 300/2000/6100) : 0 candidat trouve par
#   rb2_fbneo_recipe_from_truth.py. Cause probable = MEME piege de
#   latence deja documente sur mame0278 (DECISIONS.md "Suite immediate
#   6") : l'ordre precedent (SCREENSHOT -> sleep 1s -> localise le PNG
#   -> SEULEMENT ENSUITE dump RAM) laisse le jeu continuer ~1s+ avant le
#   dump, largement suffisant pour rater un score qui grimpe vite
#   (beat-em-up). Fix : dump RAM immediatement apres l'envoi de
#   SCREENSHOT (quelques ms), attente du fichier PNG APRES coup -- le
#   contenu du PNG est fige au moment de l'envoi de la commande, pas au
#   moment ou le fichier apparait sur disque, donc rapprocher le dump RAM
#   de l'ENVOI reduit le decalage reel.
#
# v1 - 2026-08-29 - safe-modify - Creation initiale. Pivot decide par
#   l'utilisateur (deadline recipes RB Challenge = ce WE) : jouer les
#   parties SOI-MEME plutot que de fiabiliser l'insertion credit
#   automatique fbneo (session du jour : credit auto probablement en
#   echec meme sur nemo malgre un HUD d'attract-mode trompeur -- voir
#   DECISIONS.md "Outillage FBNeo/RB2 apporte par Codex").
#
#   Design deliberement PASSIF : ce script ne lance PAS le jeu et
#   n'envoie AUCUN input. L'utilisateur lance/joue via l'interface
#   normale de RB2 (ES + manette physique -- credit+start reels, zero
#   ambiguite attract-mode/vraie partie). Le script se contente de
#   detecter le processus retroarch de la rom ciblee puis d'archiver
#   SCREENSHOT + dump RAM large a intervalle regulier, TANT QUE le
#   processus vit -- s'arrete de lui-meme quand l'utilisateur quitte le
#   jeu normalement. Aucune coupure automatique sur une duree fixe
#   (lecon actee depuis la session du 2026-08-26, "tu ne me laisses
#   jamais le temps de jouer" -- voir DECISIONS.md section 17).
#
#   Sortie compatible avec rb2_fbneo_recipe_from_truth.py (meme
#   convention de nommage <phase>-<horodatage>.ram.bin/.png sous
#   /recalbox/share/system/rb_challenge_probe/<rom>/<session>/) : une
#   fois les scores releves a l'oeil sur les screenshots, le meme outil
#   de recherche d'adresse peut etre reutilise tel quel.
#
#   Fenetre RAM par defaut 0x0-0x20000 (128 Ko, chunks 16 Ko) : couvre
#   large (l'adresse 0x11000 evoquee par le createur RB pour inthunt,
#   invalidee mais illustrative de l'ordre de grandeur, y est incluse).
#   Ajustable via --dump-base/--dump-size si un jeu suggere une zone
#   differente.
"""Capture passive (screenshot + dump RAM) pendant une partie fbneo
jouee manuellement sur RB2 -- ne lance rien, n'envoie aucun input.

Usage : lancer le jeu normalement sur RB2 (ES + manette), PUIS :
  python3 rb2_fbneo_manual_wide_capture.py inthunt
  python3 rb2_fbneo_manual_wide_capture.py inthunt --interval 5 --dump-size 0x30000

S'arrete tout seul quand le jeu est quitte normalement. Pour arreter la
capture SANS quitter le jeu (rare) : `touch /tmp/fbneo_wide_stop.<rom>`.
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
UDP_ADDR = ("127.0.0.1", 55355)
CHUNK = 16384


def now():
    return time.strftime("%Y%m%d-%H%M%S")


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
        reply, _ = s.recvfrom(65535)
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


PROBE_POINTS = (0x10000, 0x18000, 0x20000, 0x30000, 0x40000, 0x60000,
                0x80000, 0x100000, 0x200000, 0x400000)


def probe_ram_ceiling(cap=None):
    """Sonde des tailles croissantes, retourne la plus grande qui repond
    reellement -- jamais > cap si fourni, jamais > 0x400000."""
    ok_size = 0x10000
    for size in PROBE_POINTS:
        if cap is not None and size > cap:
            break
        data = read_ram(size - 16, 16)
        if data is None or len(data) == 0:
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
    # v2 -- dump RAM tout de suite (quelques ms), PAS apres l'attente du
    # PNG : le contenu du PNG est fige au moment de cet envoi, donc c'est
    # ICI qu'il faut lire la RAM pour minimiser le decalage reel avec ce
    # qui sera visible sur la capture.
    ram_path = os.path.join(rom_dir, "{}-{}.ram.bin".format(phase, now()))
    got = dump_ram(ram_path, dump_base, dump_size)
    time.sleep(1.0)
    source = latest_screenshot(started - 0.2)
    screenshot = None
    if source:
        screenshot = os.path.join(rom_dir, "{}-{}.png".format(phase, now()))
        try:
            os.rename(source, screenshot)
        except OSError:
            screenshot = source
    return {"phase": phase, "at": started, "screenshot": screenshot,
            "ram": ram_path, "ram_bytes": got}


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("rom")
    parser.add_argument("--interval", type=float, default=8.0,
                         help="secondes entre 2 captures (defaut 8)")
    parser.add_argument("--dump-base", default="0x0")
    parser.add_argument("--dump-size", default=None,
                         help="plafond optionnel (hex) -- sans cette option, la fenetre est "
                              "sondee automatiquement juste apres le lancement du jeu")
    parser.add_argument("--wait-launch", type=int, default=120,
                         help="secondes a attendre que l'utilisateur lance le jeu (defaut 120)")
    args = parser.parse_args()
    dump_base = int(args.dump_base, 0)
    dump_size_cap = int(args.dump_size, 0) if args.dump_size else None

    session_dir = os.path.join(OUT_ROOT, args.rom, now())
    os.makedirs(session_dir, exist_ok=True)
    stop_flag = "/tmp/fbneo_wide_stop.{}".format(args.rom)
    if os.path.exists(stop_flag):
        os.remove(stop_flag)

    log("en attente du lancement de '{}' (jusqu'a {}s) -- lance le jeu normalement sur RB2".format(
        args.rom, args.wait_launch))
    waited = 0
    pid = None
    while waited < args.wait_launch:
        pid = retroarch_pid(args.rom)
        if pid:
            break
        time.sleep(1)
        waited += 1
    if not pid:
        log(">>> '{}' jamais detecte apres {}s -- abandon (relance le script quand tu es pret)".format(
            args.rom, args.wait_launch))
        return

    log(">>> '{}' detecte (pid={}) -- sondage de la vraie fenetre RAM...".format(args.rom, pid))
    time.sleep(2)  # laisse le core finir son init avant de sonder
    dump_size = probe_ram_ceiling(dump_size_cap)
    log(">>> capture demarree dans {}".format(session_dir))
    log("    dump RAM 0x{:x}-0x{:x} (fenetre sondee) toutes les {}s -- AUCUNE coupure automatique,".format(
        dump_base, dump_base + dump_size, args.interval))
    log("    s'arrete quand tu quittes le jeu, ou `touch {}`".format(stop_flag))

    events = []
    index = 0
    while True:
        if not retroarch_pid(args.rom):
            log("jeu quitte (processus disparu) -- capture terminee normalement")
            break
        if os.path.exists(stop_flag):
            log("arret demande explicitement (fichier stop present) -- jeu laisse tel quel")
            os.remove(stop_flag)
            break
        index += 1
        event = capture_one(session_dir, index, dump_base, dump_size)
        events.append(event)
        log("    {} : ram={} octets, screenshot={}".format(
            event["phase"], event["ram_bytes"],
            os.path.basename(event["screenshot"]) if event["screenshot"] else "ABSENT"))
        with open(os.path.join(session_dir, "run.json"), "w", encoding="utf-8") as f:
            json.dump({"rom": args.rom, "system": "fbneo", "mode": "manual_passive",
                       "dump_base": dump_base, "dump_size": dump_size,
                       "interval": args.interval, "events": events}, f, indent=2)
        time.sleep(args.interval)

    log("=== termine : {} observations dans {} ===".format(len(events), session_dir))


if __name__ == "__main__":
    main()
