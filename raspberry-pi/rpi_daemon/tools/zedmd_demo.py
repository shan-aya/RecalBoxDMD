#!/usr/bin/env python3
"""zedmd_demo.py -- animation ZeDMD de demonstration (comme un client VPX/ES) : barres de couleur qui defilent, compteur d'images.
  zedmd_demo.py [--host 127.0.0.1] [--port 3333] [--fps 30] [--seconds 20]"""
import socket, struct, sys, time, zlib

W, H, ZW, ZH, ZPR = 128, 32, 8, 4, 16
host, port, fps, secs = '127.0.0.1', 3333, 30, 20
a = sys.argv[1:]
for i in range(0, len(a), 2):
    if a[i] == '--host': host = a[i + 1]
    elif a[i] == '--port': port = int(a[i + 1])
    elif a[i] == '--fps': fps = int(a[i + 1])
    elif a[i] == '--seconds': secs = int(a[i + 1])

def pix(x, y, t):
    band = ((x + t * 2) // 16) % 4
    r, g, b = [(255, 40, 40), (40, 255, 40), (60, 90, 255), (255, 220, 0)][band]
    if y < 2 or y >= H - 2:
        r, g, b = 255, 255, 255
    return r, g, b

def frame(t):
    payload = bytearray()
    for z in range(128):
        payload.append(z)
        x0, y0 = (z % ZPR) * ZW, (z // ZPR) * ZH
        for y in range(ZH):
            for x in range(ZW):
                r, g, b = pix(x0 + x, y0 + y, t)
                payload += struct.pack('<H', ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
    body = zlib.compress(bytes(payload))
    return b'ZeDMD' + bytes([5]) + struct.pack('>H', len(body)) + bytes([1]) + body

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
t0 = time.time(); n = 0
while time.time() - t0 < secs:
    m = frame(n)
    first = b'FRAME' + m[:1395]
    s.sendto(first, (host, port))
    for i in range(1395, len(m), 1400):
        s.sendto(m[i:i + 1400], (host, port))
    s.sendto(b'FRAME' + b'ZeDMD' + bytes([6, 0, 0, 0]), (host, port))
    n += 1
    nxt = t0 + n / fps
    time.sleep(max(0, nxt - time.time()))
print('images envoyees :', n)
