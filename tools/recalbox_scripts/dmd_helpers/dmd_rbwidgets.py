#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-10-13 - safe-modify - Creation. Alimente les 3 widgets « Recalbox » du DMD
#   (firmware v315 : Challenge en cours, Recompenses a gagner, Sante). Lit les fichiers
#   locaux de la Recalbox (aucune API HTTP exploitable : /api/* repond 404) :
#     - /recalbox/share/system/challenges/current.json (nom, fin, recompenses, classement)
#     - .../standing.json (rang et GG du joueur, 0 = non classe)
#     - .../moves.json (mouvement de chaque joueur au classement : 1 monte, -1 baisse, 0 stable)
#     - frontend.log, lignes « [Health] <test> is <etat> » (temperature, alimentation,
#       horloge, reseau, disque, memoire) + /sys/class/thermal et /proc/meminfo pour les chiffres
#   et envoie 3 datagrammes UDP « CMD=rbw ARG=C|... », « ...R|... », « ...H|... » au DMD
#   (IP decouverte par dmd_udp_resync.py, fichier /tmp/dmd_udp_ip). Tout texte est reduit a
#   A-Z 0-9 espace (police du DMD). Lance par dmd_rbwidgets[watch](permanent).sh.
import json
import os
import re
import socket
import subprocess
import sys
import time
import unicodedata
from datetime import datetime, timezone

CH_DIR = "/recalbox/share/system/challenges/"
FRONTEND_LOG = "/recalbox/share/system/logs/frontend.log"
IP_CACHE = "/tmp/dmd_udp_ip"
IP_FALLBACK = "192.168.0.51"
PORT = 5005


def clean(s, n):
    s = "".join(c for c in unicodedata.normalize("NFD", s or "") if unicodedata.category(c) != "Mn").upper()
    s = re.sub(r"[^A-Z0-9 ]", " ", s)
    s = re.sub(r" +", " ", s).strip()
    return s[:n]


def load(name):
    try:
        with open(CH_DIR + name, encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return None


def challenge_msg():
    cur = load("current.json")
    if not cur or not cur.get("name"):
        return "C|"
    try:
        end = datetime.strptime(cur["end"], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
        left = (end - datetime.now(timezone.utc)).total_seconds()
        if left <= 0:
            return "C|"
        days = int(left // 86400) + (1 if left % 86400 else 0)
    except Exception:
        days = -1
    st = load("standing.json") or {}
    mv = (load("moves.json") or {}).get("players", {})
    top = []
    for e in (cur.get("leaderboard") or [])[:3]:
        m = mv.get(str(e.get("user")), [0, 0])
        mvv = m[1] if isinstance(m, list) and len(m) > 1 else 0
        top.append("%s:%d:%d" % (clean(e.get("nickname") or e.get("name"), 10) or "?", int(e.get("score", 0)), mvv))
    return "C|%s|%d|%d|%d|%s" % (clean(cur.get("name"), 30), days, int(st.get("rank", 0) or 0), int(st.get("ggs", 0) or 0), ";".join(top))


def rewards_msg():
    cur = load("current.json") or {}
    out = []
    for r in (cur.get("rewards") or [])[:4]:
        out.append("%s:%s:%s" % (clean(r.get("kind"), 6), clean(r.get("name"), 20), clean(r.get("condition"), 44)))
    return "R|" + "|".join(out)


def health_msg():
    states = {}
    try:
        with open(FRONTEND_LOG, "rb") as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            f.seek(max(0, size - 200000))
            txt = f.read().decode("utf-8", "replace")
        for m in re.finditer(r"\[Health\] (\w+) is (\w+)", txt):
            states[m.group(1)] = m.group(2).lower()
    except Exception:
        pass

    def code(*keys):
        worst = 0
        for k in keys:
            s = states.get(k)
            if s is None or s == "fine":
                continue
            worst = max(worst, 1 if s.startswith("warn") else 2)
        return worst

    order = ("temperature", "power", "clock", "network")
    sts = [code(k) for k in order] + [code("disk", "overlay"), code("memory")]
    temp = -1
    try:
        with open("/sys/class/thermal/thermal_zone0/temp") as f:
            temp = int(int(f.read().strip()) / 1000)
    except Exception:
        pass
    mem = -1
    try:
        mi = {}
        with open("/proc/meminfo") as f:
            for line in f:
                p = line.split()
                mi[p[0].rstrip(":")] = int(p[1])
        mem = 100 - int(100 * mi["MemAvailable"] / mi["MemTotal"])
    except Exception:
        pass
    disk = -1
    try:
        o = subprocess.check_output(["df", "/recalbox/share"], timeout=5).decode().splitlines()[-1].split()
        disk = int(o[4].rstrip("%"))
    except Exception:
        pass
    return "H|%s|%d|%d|%d" % ("".join(str(s) for s in sts), temp, mem, disk)


def main():
    try:
        with open(IP_CACHE) as f:
            ip = f.read().strip() or IP_FALLBACK
    except Exception:
        ip = IP_FALLBACK
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for fn in (challenge_msg, rewards_msg, health_msg):
        try:
            msg = "CMD=rbw ARG=" + fn()
            if "--print" in sys.argv:
                print(msg)
            else:
                s.sendto(msg.encode("ascii", "replace"), (ip, PORT))
                time.sleep(0.15)
        except Exception as e:
            print("erreur", fn.__name__, e, file=sys.stderr)


if __name__ == "__main__":
    main()
