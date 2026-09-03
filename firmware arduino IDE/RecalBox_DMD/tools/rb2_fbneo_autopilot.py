#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v5
#
# v5 - 2026-09-03 - safe-modify - `play_fn()` (fbneo_game_profiles.py v2)
#   recoit maintenant `pad` (uinput) au lieu de `send` (UDP, no-op) --
#   mouvement/attaque genre-specifiques desormais reellement actifs.
#   Cause du saut de core inexplique du v4 (test DPAD_RIGHT+BTN_SOUTH)
#   ELUCIDEE (retour utilisateur direct) : ce n'etait ni un bug
#   RetroArch ni le bouton SOUTH lui-meme -- le D-PAD (BTN_DPAD_*) est
#   lie a un systeme de raccourcis RecalBox de niveau SYSTEME (hors de
#   portee de toute config RetroArch, confirme par un fix retroarch.cfg/
#   overrides.cfg sans aucun effet). fbneo_game_profiles.py v2 utilise
#   maintenant le STICK ANALOGIQUE (ABS_X/ABS_Y) pour tout deplacement,
#   plus jamais le D-pad -- confirme sans danger sur `mtwins` (score/
#   TIME progressent normalement, aucun saut de contenu, sur plusieurs
#   tests dont un avec de vrais coups portes).
#
# v4 - 2026-09-03 - safe-modify - Remplace TOUT le mecanisme d'input
#   PLAYER1_X en UDP:55355 -- confirme cette nuit comme un pur no-op
#   silencieux (ces commandes n'existent pas dans la Network Control
#   Interface officielle de RetroArch, verifie sur la doc officielle).
#   TOUTES les conclusions "positives" precedentes de ce fichier basees
#   sur ce mecanisme (dynagear v1/v2, inthunt/mtwins v3) sont donc a
#   considerer INVALIDEES -- voir DECISIONS.md pour le detail complet
#   de l'enquete (y compris une fausse conclusion positive faite puis
#   corrigee la nuit meme via un test temoin zero-input).
#
#   Nouveau mecanisme (rb2_uinput_gamepad.py, CONFIRME 2/2 sur mtwins ET
#   dynagear avec temoin/comparaison propre) : cree un vrai peripherique
#   noyau (uinput) clone du device Steam Deck reel, AVANT le lancement
#   du jeu, et passe son /dev/input/eventN en -p1devicepath a
#   emulatorlauncher.pyc -- RetroArch lie alors reellement le joueur 1 a
#   ce clone (un clone cree APRES le lancement n'a AUCUN effet, meme
#   avec un GUID identique -- confirme par un temoin dedie).
#   `send()`/PLAYER1_X retires de la boucle START ; play_fn() (patterns
#   generiques par genre) reste sur `send()` pour l'instant -- PAS
#   ENCORE PORTE sur uinput (prudence : le tout premier test uinput
#   avec DPAD_RIGHT+BTN_SOUTH combines a provoque un saut de core vers
#   un autre jeu, mecanisme non identifie -- ne pas generaliser
#   aveuglement avant d'avoir isole quel bouton/combo en est la cause).
#
# v3 - 2026-09-02 - safe-modify - Retour utilisateur en observation
#   DIRECTE sur l'ecran physique (pas juste sur la RAM) : le test v2 sur
#   inthunt montrait une "vraie" progression de score en RAM
#   (400->2900->3100), a tort interprete comme preuve de partie reelle
#   -- l'utilisateur confirme que TOUTE la session est restee en
#   attract-mode (titre "FREE PLAY" clignotant, 2P actif) -- EXACTEMENT
#   le meme piege deja documente le 25/08 sur ce meme jeu (l'attract-
#   mode d'inthunt joue une demo scriptee avec un VRAI score qui
#   progresse). Lecon actee : une progression RAM seule n'est PAS une
#   preuve suffisante de partie reelle, il faut croiser avec le texte a
#   l'ecran. Fix tente (pas encore reverifie) : START renvoye avant
#   CHAQUE cycle de jeu au lieu d'une seule fois au debut -- la fenetre
#   d'acceptation semble limitee a un instant precis du cycle titre/
#   attract, ratee par un appui unique.
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
from rb2_uinput_gamepad import VirtualGamepad, BTN_START  # noqa: E402


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


def launch(rom, devicepath=None):
    # v4 -- devicepath permet de forcer -p1devicepath sur le clone
    # uinput (cree AVANT cet appel) au lieu du device physique reel --
    # c'est ce lien fait AU LANCEMENT qui determine quel device
    # RetroArch ecoute reellement pour le joueur 1 (confirme cette
    # nuit : un clone cree APRES le lancement n'a aucun effet).
    device = devicepath or detect_steam_deck_device()
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

    # v4 -- le clone uinput DOIT exister AVANT le lancement et son
    # /dev/input/eventN doit etre passe en -p1devicepath : c'est ce
    # lien fait AU LANCEMENT qui determine quel device RetroArch ecoute
    # reellement pour le joueur 1 (confirme 2/2 sur mtwins et dynagear
    # -- un clone cree apres coup n'a structurellement aucun effet,
    # meme avec un GUID identique au device reel).
    pad = VirtualGamepad().create()
    vpath = pad.event_path()
    if not vpath:
        log("!! event_path() introuvable pour le clone uinput -- abandon")
        pad.destroy()
        return
    log(">>> clone uinput cree, event_path={}".format(vpath))

    process = launch(args.rom, devicepath=vpath)
    pid = None
    for _ in range(args.load_wait):
        pid = retroarch_pid(args.rom)
        if pid:
            break
        time.sleep(1)
    if not pid:
        log("!! jamais lance apres {}s -- abandon".format(args.load_wait))
        pad.destroy()
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

    # v4 -- START envoye via le clone uinput (BTN_START, vrai device
    # noyau lie au joueur 1), plus via UDP (confirme no-op). Toujours
    # renvoye avant CHAQUE cycle (comportement v3 conserve par prudence,
    # meme si le nouveau mecanisme semble accepter START a tout moment
    # une fois le device correctement lie -- pas de fenetre de timing
    # etroite observee sur mtwins/dynagear).
    log(">>> START (uinput) envoye avant chaque cycle (Free Play -- pas de credit necessaire)")
    pad.tap(BTN_START, hold=0.1)
    time.sleep(3)

    index = 1
    deadline = time.time() + args.play_seconds
    while time.time() < deadline:
        if not retroarch_pid(args.rom):
            log("jeu quitte de lui-meme -- arret")
            break
        pad.tap(BTN_START, hold=0.1)
        time.sleep(0.3)
        play_fn(pad, min(args.interval, deadline - time.time()))
        event = capture_one(session_dir, index, 0, dump_size)
        events.append(event)
        log("    obs{:02d} : ram={} octets".format(index, event["ram_bytes"]))
        index += 1

    with open(os.path.join(session_dir, "run.json"), "w", encoding="utf-8") as f:
        json.dump({"rom": args.rom, "dump_size": dump_size, "events": events}, f, indent=2)

    pad.destroy()
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
