#!/usr/bin/env python3
"""zedmd_send.py -- client ZeDMD-WiFi de test (UDP 3333), bibliotheque standard seulement.

Reproduit ce que font libzedmd / libdmdutil : datagrammes « FRAME » + messages « ZeDMD » <cmd> <taille BE16> <flag zlib> <charge>,
zones de 8x4 (index 0..127), messages coupes entre deux datagrammes (suite SANS prefixe).

  zedmd_send.py [--host 127.0.0.1] [--port 3333] [--src-port N] ACTION...
Actions :
  full565[:graine]   image complete RGB565 (bruit pseudo-aleatoire de la graine) compressee, coupee en datagrammes de 1400 octets
  full888[:graine]   idem en RGB888, SANS compression, un message par datagramme
  zones888:i,j,k     zones RGB888 (couleur unie differente par zone), non compressees
  clearzone:i,j      efface les zones (index + 128)
  clear              cmd 10 (efface l'ecran)
  render             cmd 6
  keepalive          cmd 11
  bright:N           cmd 22, N = 0..15
  lost565            message coupe dont la suite est remplacee par un datagramme « FRAME » (le message doit etre abandonne)
  sleep:S            pause (secondes)
Les motifs sont reproductibles : voir pixel_565()/pixel_888() (repris par zedmd_check.py).
"""
import random
import socket
import struct
import sys
import time
import zlib

W, H, ZW, ZH, ZPR = 128, 32, 8, 4, 16


def zone_rgb888(seed, zone):
    r = random.Random(seed * 1000 + zone)
    return bytes(r.randrange(256) for _ in range(ZW * ZH * 3))


def rgb565_from_888(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def zone_rgb565(seed, zone):
    d = zone_rgb888(seed, zone)
    out = bytearray()
    for i in range(0, len(d), 3):
        out += struct.pack('<H', rgb565_from_888(d[i], d[i + 1], d[i + 2]))
    return bytes(out)


def msg(cmd, payload=b'', compress=False):
    if compress:
        payload = zlib.compress(payload)
    return b'ZeDMD' + bytes([cmd]) + struct.pack('>H', len(payload)) + bytes([1 if compress else 0]) + payload


class Sender:
    def __init__(self, host, port, src_port):
        self.addr = (host, port)
        self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if src_port:
            self.s.bind(('0.0.0.0', src_port))

    def send(self, data):
        self.s.sendto(data, self.addr)

    def send_split(self, message, chunk=1400):
        """Comme libzedmd : « FRAME » + debut du message, puis la suite brute dans les datagrammes suivants."""
        first = b'FRAME' + message[:chunk - 5]
        self.send(first)
        rest = message[chunk - 5:]
        for i in range(0, len(rest), chunk):
            self.send(rest[i:i + chunk])
            time.sleep(0.002)


def main():
    host, port, src = '127.0.0.1', 3333, 0
    args = sys.argv[1:]
    acts = []
    i = 0
    while i < len(args):
        if args[i] == '--host':
            host = args[i + 1]; i += 2
        elif args[i] == '--port':
            port = int(args[i + 1]); i += 2
        elif args[i] == '--src-port':
            src = int(args[i + 1]); i += 2
        else:
            acts.append(args[i]); i += 1
    sd = Sender(host, port, src)
    for a in acts:
        name, _, val = a.partition(':')
        if name == 'full565':
            seed = int(val or 1)
            payload = b''.join(bytes([z]) + zone_rgb565(seed, z) for z in range(128))
            sd.send_split(msg(5, payload, compress=True))
            sd.send(b'FRAME' + msg(6))
        elif name == 'full888':
            seed = int(val or 1)
            for z0 in range(0, 128, 8):  # 8 zones = 776 octets par message
                payload = b''.join(bytes([z]) + zone_rgb888(seed, z) for z in range(z0, z0 + 8))
                sd.send(b'FRAME' + msg(4, payload))
            sd.send(b'FRAME' + msg(6))
        elif name == 'zones888':
            ids = [int(x) for x in val.split(',')]
            payload = b''.join(bytes([z]) + bytes([(z * 37) % 256, (z * 91) % 256, 200]) * (ZW * ZH) for z in ids)
            sd.send(b'FRAME' + msg(4, payload))
            sd.send(b'FRAME' + msg(6))
        elif name == 'clearzone':
            ids = [int(x) for x in val.split(',')]
            sd.send(b'FRAME' + msg(5, bytes(128 + z for z in ids)))
            sd.send(b'FRAME' + msg(6))
        elif name == 'clear':
            sd.send(b'FRAME' + msg(10))
        elif name == 'render':
            sd.send(b'FRAME' + msg(6))
        elif name == 'keepalive':
            sd.send(b'FRAME' + msg(11))
        elif name == 'bright':
            sd.send(b'FRAME' + msg(22, bytes([int(val)])))
        elif name == 'lost565':
            payload = b''.join(bytes([z]) + zone_rgb565(9, z) for z in range(128))
            m = msg(5, payload, compress=True)
            sd.send(b'FRAME' + m[:1000])
            time.sleep(0.05)
            sd.send(b'FRAME' + msg(11))  # la suite attendue est remplacee par un nouveau datagramme FRAME
        elif name == 'sleep':
            time.sleep(float(val))
        else:
            print('action inconnue :', a)
            return 2
        time.sleep(0.03)
    return 0


if __name__ == '__main__':
    sys.exit(main())
