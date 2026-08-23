#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. 2e passe de la
#   methode "verite d'abord" (voir build_entry_from_truth.py, DECISIONS.md
#   "Récolte multi-RB") -- pour les jeux qui n'ont PAS produit de .hi (que
#   ce soit par absence dans hiscore.dat, ou entree presente mais fausse --
#   ex. reel trouve ce soir : batlzone rattache par erreur au bloc mayday),
#   le plugin hiscore natif de MAME ne peut evidemment rien nous donner.
#   Ce script capture donc, pour chaque rom SANS .hi : la meme capture
#   d'ecran que la recolte normale (verite terrain visuelle) + un DUMP RAM
#   LIVE (commande reseau RetroArch READ_CORE_RAM, prouvee fonctionnelle
#   sur le core mame0278, ~16 Ko/appel en ~20ms) -- sans dependre du
#   plugin hiscore ni de hiscore.dat. Le dump sert ensuite de donnee
#   source a find_value_in_ramdump.py pour localiser exactement le score
#   vu sur la capture, EXACTEMENT comme build_entry_from_truth.py le fait
#   deja sur un vrai fichier .hi -- juste applique a une zone RAM live au
#   lieu d'un fichier de sauvegarde deja ecrit par le plugin.
#
#   Zone dumpee par defaut : 0x0000-0x10000 (64 Ko, 4 blocs de 16 Ko) --
#   couvre la quasi-totalite du hardware arcade classique (Z80/6502/6809)
#   qui compose la majorite de la liste hiscore.dat. Pour un hardware a
#   adressage plus large (ex. 68000, RAM haute), utiliser --extra-addr
#   pour dumper une fenetre supplementaire ciblee.
#
#   Ne touche JAMAIS ES/Xorg (meme regle que direct_harvest_mame0278_rb2.py
#   -- machine X11, pas d'occupation exclusive d'ecran).
"""Capture screenshot + dump RAM live pour une liste de roms mame0278 sans
.hi, en vue d'une correction manuelle de hiscore.dat via la methode
"verite d'abord".

Usage: rb2_ram_truth_capture.py <rom_list_file> [--extra-addr 0xADDR --extra-size 0xSIZE]
"""
import subprocess, time, socket, os, sys

os.environ["DISPLAY"] = ":0"
os.environ["XDG_RUNTIME_DIR"] = "/run/user/0"

LOG = "/tmp/rb2_ram_truth_log.txt"
DUMP_DIR = "/tmp/ramtruth"
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
CHUNK = 16384
DEFAULT_DUMP_BASE = 0x0000
DEFAULT_DUMP_SIZE = 0x10000  # 64 Ko


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


def udp_client():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", 0))
    s.settimeout(3)
    return s


def send_udp(s, cmd):
    try:
        s.sendto(cmd, ("127.0.0.1", 55355))
    except Exception as e:
        log("!! erreur envoi {}: {}".format(cmd, e))


def send_recv(s, cmd):
    try:
        s.sendto(cmd.encode() + b"\n", ("127.0.0.1", 55355))
        data, _ = s.recvfrom(1 << 20)
        return data
    except (socket.timeout, OSError):
        return None


def read_core_ram_chunk(s, addr, size):
    """Un appel READ_CORE_RAM -- renvoie les octets lus (bytes) ou None."""
    resp = send_recv(s, "READ_CORE_RAM {:x} {}".format(addr, size))
    if resp is None:
        return None
    parts = resp.decode(errors="replace").strip().split()
    # format attendu : "READ_CORE_RAM <addr_hex> <b1> <b2> ..."
    if len(parts) < 3 or parts[0] != "READ_CORE_RAM":
        return None
    try:
        return bytes(int(x, 16) for x in parts[2:])
    except ValueError:
        return None


def dump_region(s, base, size, out_path):
    """Dump base..base+size par blocs de CHUNK, ecrit dans out_path.
    Retourne le nombre d'octets effectivement obtenus (peut etre < size
    si le core refuse de repondre au-dela de son espace d'adressage reel
    -- comportement attendu et sans gravite, on garde ce qu'on a)."""
    got = 0
    with open(out_path, "wb") as f:
        addr = base
        remaining = size
        while remaining > 0:
            n = min(CHUNK, remaining)
            data = read_core_ram_chunk(s, addr, n)
            if data is None or len(data) == 0:
                break
            f.write(data)
            got += len(data)
            addr += len(data)
            remaining -= len(data)
            if len(data) < n:
                break
    return got


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


def capture_one(rom, extra_addr=None, extra_size=0x4000):
    dump_path = os.path.join(DUMP_DIR, "{}.bin".format(rom))
    extra_path = os.path.join(DUMP_DIR, "{}.extra.bin".format(rom))
    if os.path.exists(dump_path):
        log("(skip) {} -- dump deja present".format(rom))
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

    # socket dediee au fire-and-forget (SCREENSHOT/QUIT) -- NE PAS la
    # reutiliser pour READ_CORE_RAM : RetroArch semble repondre a
    # SCREENSHOT sur le meme port, et cette reponse residuelle se faisait
    # sinon avaler par le premier recv() du dump (parsing rate -> 0 octet
    # obtenu, bug reel trouve lors du premier test de cet outil).
    s_fire = udp_client()
    send_udp(s_fire, b"SCREENSHOT\n")
    time.sleep(1)  # laisse RetroArch ecrire le PNG avant le dump/QUIT
    s_fire.close()

    s = udp_client()
    log("    dump RAM 0x{:x}-0x{:x}...".format(DEFAULT_DUMP_BASE, DEFAULT_DUMP_BASE + DEFAULT_DUMP_SIZE))
    got = dump_region(s, DEFAULT_DUMP_BASE, DEFAULT_DUMP_SIZE, dump_path)
    log("    dump principal : {} octets obtenus".format(got))

    if extra_addr is not None:
        log("    dump supplementaire 0x{:x}-0x{:x}...".format(extra_addr, extra_addr + extra_size))
        got2 = dump_region(s, extra_addr, extra_size, extra_path)
        log("    dump supplementaire : {} octets obtenus".format(got2))

    send_udp(s, b"QUIT\n")
    s.close()

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

    log("==> {} : capture terminee (dump {} octets)".format(rom, got))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    rom_list_file = sys.argv[1]
    extra_addr = None
    extra_size = 0x4000
    if "--extra-addr" in sys.argv:
        i = sys.argv.index("--extra-addr")
        extra_addr = int(sys.argv[i + 1], 0)
    if "--extra-size" in sys.argv:
        i = sys.argv.index("--extra-size")
        extra_size = int(sys.argv[i + 1], 0)

    os.makedirs(DUMP_DIR, exist_ok=True)
    with open(rom_list_file) as f:
        roms = [l.strip() for l in f if l.strip()]
    log("=== rb2_ram_truth_capture demarre, {} roms en file ===".format(len(roms)))
    for rom in roms:
        try:
            capture_one(rom, extra_addr, extra_size)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
        time.sleep(2)
    log("=== rb2_ram_truth_capture termine ===")


if __name__ == "__main__":
    main()
