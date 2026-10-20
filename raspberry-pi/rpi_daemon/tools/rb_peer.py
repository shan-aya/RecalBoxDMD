#!/usr/bin/env python3
"""rb_peer.py -- fausse Recalbox pour tester le multi-Recalbox du demon (loopback : 127.0.0.1 et 127.0.0.2).

  rb_peer.py --ip 127.0.0.2 [--hello-port 5006] [--daemon-port 5005] [--mute-after S] [--send "S:CMD=system ARG=nes" ...] [--duration S]
Ecoute <ip>:<hello-port>, journalise ce qu'elle recoit (HELLO / PING / FEATURES), repond PONG a chaque PING (jusqu'a --mute-after
secondes), et envoie des commandes au demon depuis <ip> aux instants demandes."""
import argparse, socket, sys, time, threading

ap = argparse.ArgumentParser()
ap.add_argument('--ip', required=True)
ap.add_argument('--hello-port', type=int, default=5006)
ap.add_argument('--daemon-port', type=int, default=5005)
ap.add_argument('--mute-after', type=float, default=1e9)
ap.add_argument('--send', action='append', default=[])
ap.add_argument('--duration', type=float, default=30)
a = ap.parse_args()
t0 = time.time()
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((a.ip, a.hello_port))
s.settimeout(0.2)
tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
tx.bind((a.ip, 0))

def log(msg):
    print('[peer %s t=%5.1f] %s' % (a.ip, time.time() - t0, msg), flush=True)

def sender():
    for spec in sorted(a.send, key=lambda x: float(x.split(':', 1)[0])):
        at, cmd = spec.split(':', 1)
        time.sleep(max(0, float(at) - (time.time() - t0)))
        tx.sendto(cmd.encode(), ('127.0.0.1', a.daemon_port))
        log('envoie : ' + cmd)

threading.Thread(target=sender, daemon=True).start()
while time.time() - t0 < a.duration:
    try:
        data, addr = s.recvfrom(2048)
    except socket.timeout:
        continue
    m = data.decode(errors='replace')
    log('recu : ' + m[:50])
    if m == 'PING':
        if time.time() - t0 < a.mute_after:
            s.sendto(b'PONG', addr)
        else:
            log('(muette : pas de PONG)')
