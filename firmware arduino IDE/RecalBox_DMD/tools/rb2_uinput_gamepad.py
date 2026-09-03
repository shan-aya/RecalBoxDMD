#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rb2_uinput_gamepad.py
======================
Manette virtuelle au niveau NOYAU (uinput) pour piloter RetroArch/fbneo sur
RB2 (x86 Linux, Steam Deck) depuis un script externe.

# Version actuelle : v2
#
# v2 - 2026-09-03 - safe-modify - Test live sur `mtwins` : creer le clone
#   APRES le lancement du jeu n'a AUCUN effet observable (compare a un
#   temoin zero-input strictement au meme instant : demo/attract-mode
#   identique au pixel pres des deux cotes -- TIME fige, FREE PLAY,
#   score 0). Hypothese retenue : emulatorlauncher.pyc lie RetroArch au
#   PATH du device REEL passe en -p1devicepath au lancement -- notre
#   clone, cree apres coup, n'est jamais bind au joueur 1. Ajout de
#   `event_path()` (cherche /dev/input/eventN assigne au clone via
#   /proc/bus/input/devices) pour pouvoir le passer explicitement en
#   -p1devicepath DES le lancement -- pas encore teste.
#
# v1 - 2026-09-03 (v1) : creation. Remplace la voie UDP:55355 (`PLAYER1_START` etc.)
#   confirmee comme un no-op silencieux cette nuit : ces commandes texte ne
#   font PAS partie de la Network Control Interface officielle de RetroArch
#   (verifie via https://docs.libretro.com/development/retroarch/network-control-interface/).
#   La seule voie reseau alternative documentee ("Remote RetroPad", port 55400+)
#   necessite de charger un core dedie A LA PLACE du jeu - inutilisable pour
#   piloter une session fbneo deja lancee (confirme via le projet tiers
#   dmang-dev/mcp-retroarch qui documente precisement cette limite).
#   uinput cree un vrai peripherique noyau que RetroArch lit exactement comme
#   la manette physique Steam Deck (confirmee fonctionnelle par l'utilisateur) -
#   le clone reprend le meme bustype/vendor/product/version (0003:28de:1205:0110)
#   et les memes bits EV_KEY/EV_ABS que le device reel (verifie: 22 boutons,
#   10 axes, memes codes - correspond exactement aux `-p1nbbuttons 22
#   -p1nbaxes 10` utilises par emulatorlauncher.pyc), donc RetroArch/SDL le
#   traite avec le meme GUID/autoconfig sans reglage manuel.
#   AUCUNE dependance externe (pas de python-uinput/evdev, absents sur RB2) -
#   ioctl bruts via ctypes/fcntl, format struct input_event standard 64-bit.

Usage (depuis un autre script) :
    from rb2_uinput_gamepad import VirtualGamepad, BTN_START, BTN_SOUTH, ...
    pad = VirtualGamepad()
    pad.create()
    pad.tap(BTN_START, hold=0.05)
    pad.destroy()

Codes boutons/axes reels du Steam Deck (sondes le 2026-09-03 via EVIOCGBIT) :
    EV_KEY : 289,290,294 (boutons additionnels/back grips non nommes standard),
             304=SOUTH(A) 305=EAST(B) 307=NORTH(X) 308=WEST(Y)
             310=TL 311=TR 314=SELECT 315=START 316=MODE 317=THUMBL 318=THUMBR
             544=DPAD_UP 545=DPAD_DOWN 546=DPAD_LEFT 547=DPAD_RIGHT
             704-707=TRIGGER_HAPPY1..4 (paddles arriere)
    EV_ABS : 0=X 1=Y 2=Z 3=RX 4=RY 5=RZ (stick gauche/droit + gachettes analogiques)
             16=HAT0X 17=HAT0Y 18=HAT1X 19=HAT1Y
"""
import fcntl
import struct
import subprocess
import sys
import time

sys.path.insert(0, "/recalbox/share/system/rb_challenge_probe")

EV_SYN, EV_KEY, EV_ABS = 0x00, 0x01, 0x03
SYN_REPORT = 0

# --- codes boutons (input-event-codes.h) ---
BTN_THUMB, BTN_THUMB2, BTN_BASE = 289, 290, 294
BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST = 304, 305, 307, 308
BTN_TL, BTN_TR = 310, 311
BTN_SELECT, BTN_START, BTN_MODE = 314, 315, 316
BTN_THUMBL, BTN_THUMBR = 317, 318
BTN_DPAD_UP, BTN_DPAD_DOWN, BTN_DPAD_LEFT, BTN_DPAD_RIGHT = 544, 545, 546, 547
BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2, BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4 = 704, 705, 706, 707

ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY, ABS_RZ = 0, 1, 2, 3, 4, 5
ABS_HAT0X, ABS_HAT0Y, ABS_HAT1X, ABS_HAT1Y = 16, 17, 18, 19

# --- ioctl noms/valeurs (x86_64 Linux, calcules une fois pour toutes) ---
UI_SET_EVBIT = 0x40045564
UI_SET_KEYBIT = 0x40045565
UI_SET_ABSBIT = 0x40045567
UI_DEV_CREATE = 0x5501
UI_DEV_DESTROY = 0x5502

UINPUT_MAX_NAME_SIZE = 80
ABS_CNT = 0x40


def _ioc(dir_, type_, nr, size):
    return (dir_ << 30) | (type_ << 8) | nr | (size << 16)


EVIOCGID = _ioc(2, ord('E'), 0x02, 8)


def _eviocgbit(ev, length):
    return _ioc(2, ord('E'), 0x20 + ev, length)


def _get_bits(fd, ev, nbytes):
    buf = bytearray(nbytes)
    fcntl.ioctl(fd, _eviocgbit(ev, nbytes), buf, True)
    bits = set()
    for byte_i, b in enumerate(buf):
        for bit_i in range(8):
            if b & (1 << bit_i):
                bits.add(byte_i * 8 + bit_i)
    return bits


def probe_real_device():
    """Retourne (path, bustype, vendor, product, version, key_bits, abs_bits)
    du vrai device Steam Deck physique. Lecture seule."""
    from rb2_input_device import detect_steam_deck_device
    path = detect_steam_deck_device()
    fd = open(path, "rb", buffering=0)
    try:
        idbuf = bytearray(8)
        fcntl.ioctl(fd, EVIOCGID, idbuf, True)
        bustype, vendor, product, version = struct.unpack("<HHHH", bytes(idbuf))
        key_bits = _get_bits(fd, EV_KEY, 96)
        abs_bits = _get_bits(fd, EV_ABS, 8)
        return path, bustype, vendor, product, version, key_bits, abs_bits
    finally:
        fd.close()


class VirtualGamepad:
    """Manette uinput clonee sur le vrai device Steam Deck (meme id, memes
    boutons/axes) - RetroArch la traite comme n'importe quelle manette
    physique, sans configuration supplementaire."""

    def __init__(self, name=b"Steam Deck (virtual)"):
        self.name = name
        self.fd = None
        self.key_bits = set()
        self.abs_bits = set()

    def create(self):
        (_, bustype, vendor, product, version,
         key_bits, abs_bits) = probe_real_device()
        self.key_bits, self.abs_bits = key_bits, abs_bits

        ui = open("/dev/uinput", "wb", buffering=0)
        fcntl.ioctl(ui, UI_SET_EVBIT, EV_KEY)
        for code in key_bits:
            fcntl.ioctl(ui, UI_SET_KEYBIT, code)
        fcntl.ioctl(ui, UI_SET_EVBIT, EV_ABS)
        for code in abs_bits:
            fcntl.ioctl(ui, UI_SET_ABSBIT, code)

        name = self.name.ljust(UINPUT_MAX_NAME_SIZE, b"\x00")[:UINPUT_MAX_NAME_SIZE]
        idpart = struct.pack("<HHHH", bustype, vendor, product, version)
        ff_effects_max = struct.pack("<I", 0)
        absmax = [0] * ABS_CNT
        absmin = [0] * ABS_CNT
        absfuzz = [0] * ABS_CNT
        absflat = [0] * ABS_CNT
        for code in abs_bits:
            if code < ABS_CNT:
                absmin[code] = -32768
                absmax[code] = 32767
        payload = (name + idpart + ff_effects_max
                   + struct.pack("<%di" % ABS_CNT, *absmax)
                   + struct.pack("<%di" % ABS_CNT, *absmin)
                   + struct.pack("<%di" % ABS_CNT, *absfuzz)
                   + struct.pack("<%di" % ABS_CNT, *absflat))
        ui.write(payload)
        fcntl.ioctl(ui, UI_DEV_CREATE)
        self.fd = ui
        time.sleep(0.3)  # laisse le temps au noyau/udev d'enregistrer le device
        return self

    def event_path(self, retries=20, delay=0.2):
        """Retrouve /dev/input/eventN assigne par le noyau a CE clone,
        en cherchant son nom dans /proc/bus/input/devices. Necessaire
        pour le passer explicitement en -p1devicepath au lancement
        (sinon RetroArch reste lie au device reel enumere au demarrage,
        notre clone cree apres coup n'etant jamais bind au joueur 1)."""
        target_name = self.name.decode()
        for _ in range(retries):
            with open("/proc/bus/input/devices", encoding="utf-8") as f:
                blocks = f.read().split("\n\n")
            for block in blocks:
                if 'N: Name="{}"'.format(target_name) in block:
                    for line in block.splitlines():
                        if line.startswith("H: Handlers="):
                            for tok in line.split("=", 1)[1].split():
                                if tok.startswith("event"):
                                    return "/dev/input/" + tok
            time.sleep(delay)
        return None

    def destroy(self):
        if self.fd is not None:
            try:
                fcntl.ioctl(self.fd, UI_DEV_DESTROY)
            finally:
                self.fd.close()
                self.fd = None

    def _write_event(self, ev_type, code, value):
        # struct input_event 64-bit : timeval(16) + type(u16) + code(u16) + value(s32) = 24 octets
        now = time.time()
        sec, usec = int(now), int((now % 1) * 1_000_000)
        self.fd.write(struct.pack("<qqHHi", sec, usec, ev_type, code, value))

    def _syn(self):
        self._write_event(EV_SYN, SYN_REPORT, 0)

    def key(self, code, pressed):
        if code not in self.key_bits:
            raise ValueError("code bouton %d absent des capacites clonees" % code)
        self._write_event(EV_KEY, code, 1 if pressed else 0)
        self._syn()

    def tap(self, code, hold=0.08):
        self.key(code, True)
        time.sleep(hold)
        self.key(code, False)

    def abs_move(self, axis, value):
        if axis not in self.abs_bits:
            raise ValueError("axe %d absent des capacites clonees" % axis)
        self._write_event(EV_ABS, axis, value)
        self._syn()

    def abs_center(self, axis):
        self.abs_move(axis, 0)

    def __enter__(self):
        return self.create()

    def __exit__(self, *exc):
        self.destroy()


if __name__ == "__main__":
    # auto-test infra : cree le clone, verifie /proc/bus/input/devices, detruit.
    with VirtualGamepad() as pad:
        print("clone cree (%d boutons, %d axes)" % (len(pad.key_bits), len(pad.abs_bits)))
        out = subprocess.check_output(
            ["grep", "-B2", "-A3", pad.name.decode(), "/proc/bus/input/devices"],
            text=True)
        print(out)
    print("clone detruit")
