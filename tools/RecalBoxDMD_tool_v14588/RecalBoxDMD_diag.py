# -*- coding: utf-8 -*-
# RecalBoxDMD_diag.py -- logique du Mode 21 « Diagnostic » (USB + Wi-Fi) : aucune dependance Tk, testable seule.
#
# v1 - 2026-10-31 - safe-modify - Creation. Reprend le mode diagnostic de la page web du DMD (/config/diag : POST /diag-start,
#   GET /diag-data toutes les 2 s, POST /diag-stop, POST /dmd-resume ; memes baremes de qualite / memes codes de deconnexion),
#   ajoute un moniteur SERIE (pyserial, DTR/RTS maintenus bas : ouvrir le port NE redemarre PAS le DMD), un ping ICMP du PC,
#   l'inventaire materiel du PC / des ports USB / du DMD (ESP32 ou Raspberry Pi) et l'export du tout dans un .zip.
#   Le serveur web du DMD est mono-connexion : toutes les requetes sont SEQUENTIELLES, une seule a la fois.
from __future__ import annotations

import csv
import io
import json
import os
import platform
import re
import subprocess
import threading
import time
import urllib.request
import zipfile
from collections import deque
from datetime import datetime

try:
    import serial                        # pyserial
    import serial.tools.list_ports as _lp
except Exception:                        # pyserial absent : le Wi-Fi reste utilisable
    serial = None
    _lp = None

# ---------------------------------------------------------------- textes (codes de deconnexion, qualite)
REASONS = {
    "fr": {1: "raison non précisée", 2: "authentification expirée", 3: "quitté par le point d'accès", 4: "association expirée",
           8: "déconnexion demandée ou perdue", 15: "échange de clés expiré (signal limite ou mot de passe)",
           200: "balises perdues : le point d'accès n'est plus entendu", 201: "point d'accès introuvable",
           202: "authentification refusée", 203: "association refusée", 205: "connexion échouée", "x": "code"},
    "en": {1: "unspecified reason", 2: "authentication expired", 3: "left by the access point", 4: "association expired",
           8: "disconnect requested or lost", 15: "4-way handshake timeout (weak signal or wrong password)",
           200: "beacon timeout: the access point is no longer heard", 201: "access point not found",
           202: "authentication failed", 203: "association failed", 205: "connection failed", "x": "code"},
    "es": {1: "motivo no especificado", 2: "autenticación caducada", 3: "expulsado por el punto de acceso", 4: "asociación caducada",
           8: "desconexión solicitada o perdida", 15: "intercambio de claves caducado (señal débil o contraseña)",
           200: "balizas perdidas: ya no se oye el punto de acceso", 201: "punto de acceso no encontrado",
           202: "autenticación rechazada", 203: "asociación rechazada", 205: "conexión fallida", "x": "código"},
}
QUALITY = {"fr": ["Excellent", "Bon", "Moyen", "Faible", "Très faible", "Pas de Wi-Fi"],
           "en": ["Excellent", "Good", "Fair", "Weak", "Very weak", "No Wi-Fi"],
           "es": ["Excelente", "Bueno", "Medio", "Débil", "Muy débil", "Sin Wi-Fi"]}


def reason_text(code, lang="fr") -> str:
    t = REASONS.get(lang, REASONS["fr"])
    try:
        c = int(code)
    except (TypeError, ValueError):
        c = -1
    return t.get(c) or f"{t['x']} {code}"


def quality(rssi: int):
    """(niveau 'good'|'warn'|'bad', index dans QUALITY) -- meme bareme que la page web du DMD."""
    if rssi >= -55:
        return "good", 0
    if rssi >= -67:
        return "good", 1
    if rssi >= -75:
        return "warn", 2
    if rssi >= -82:
        return "bad", 3
    return "bad", 4


def fmt_ago(s) -> str:
    s = max(0, int(s))
    return f"{s // 60} min {s % 60} s" if s >= 60 else f"{s} s"


# ---------------------------------------------------------------- HTTP (sequentiel : le serveur du DMD est mono-connexion)
_HTTP_LOCK = threading.Lock()   # mesure Wi-Fi, suivi des evenements et rapport partagent le MEME serveur mono-connexion : une requete a la fois, jamais en parallele


def http_json(ip: str, path: str, method: str = "GET", timeout: float = 6.0):
    """(dict|None, rtt_ms|None, erreur|None). Jamais d'exception."""
    t0 = time.perf_counter()
    try:
        req = urllib.request.Request(f"http://{ip}{path}", data=(b"" if method == "POST" else None), method=method)
        with _HTTP_LOCK:
            t0 = time.perf_counter()   # le temps d'attente du verrou ne compte pas dans l'aller-retour mesure
            with urllib.request.urlopen(req, timeout=timeout) as r:
                raw = r.read()
        rtt = (time.perf_counter() - t0) * 1000.0
        if not raw.strip():
            return {}, rtt, None
        try:
            return json.loads(raw.decode("utf-8", "replace")), rtt, None
        except ValueError:
            return {}, rtt, None
    except Exception as e:
        return None, None, f"{type(e).__name__}: {e}"


_SECRET = re.compile(r"pass|pwd|psk|token|secret|key", re.I)


def redact(obj):
    """Copie de `obj` dont toute valeur dont la CLE ressemble a un secret (mot de passe Wi-Fi, jeton, cle) est masquee."""
    if isinstance(obj, dict):
        return {k: ("***" if _SECRET.search(str(k)) and v not in ("", None) else redact(v)) for k, v in obj.items()}
    if isinstance(obj, list):
        return [redact(v) for v in obj]
    return obj


LOAD_KEYS = ("fw_version", "platform", "boot_reason", "uptime_s", "ota_last", "scripts_bundle", "rb_age", "recalbox_ip",
             "recalbox_active", "recalbox_native_mode", "recalbox_target", "wifi_ssid", "wifi_cur_ip", "wifi_cur_gw",
             "wifi_cur_mask", "wifi_static_enabled", "language", "btn_enabled", "btn_pin", "bluetooth_enabled")


def device_kind(load: dict | None, diag: dict | None) -> str:
    if (load or {}).get("platform") == "rpi" or (diag or {}).get("pi"):
        return "Raspberry Pi"
    return "ESP32"


def ping_ms(ip: str, timeout_ms: int = 1000):
    """Ping ICMP du PC vers `ip` : ms (float) ou None. Sans fenetre console."""
    try:
        flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        if os.name == "nt":
            cmd = ["ping", "-n", "1", "-w", str(timeout_ms), ip]
        else:
            cmd = ["ping", "-c", "1", "-W", str(max(1, timeout_ms // 1000)), ip]
        out = subprocess.run(cmd, capture_output=True, timeout=timeout_ms / 1000.0 + 3, creationflags=flags).stdout
        txt = out.decode("mbcs" if os.name == "nt" else "utf-8", "replace")
        m = re.search(r"(?:temps|time|tiempo)\s*[=<]\s*([\d.,]+)\s*ms", txt, re.I)
        if m:
            return float(m.group(1).replace(",", "."))
        if re.search(r"(?:temps|time|tiempo)\s*<\s*1\s*ms", txt, re.I):
            return 0.5
    except Exception:
        pass
    return None


# ---------------------------------------------------------------- mesure Wi-Fi (miroir de /config/diag)
class WifiDiag:
    """Pilote le mode diagnostic du DMD : /diag-start puis /diag-data toutes les 2 s (le firmware quitte le mode apres 8 s sans lecture).
    Garde l'historique des echantillons pour le rapport. `on_sample(d)` est appele depuis le fil de mesure."""

    def __init__(self, ip: str, on_sample=None, on_error=None, with_ping: bool = True):
        self.ip, self.on_sample, self.on_error, self.with_ping = ip, on_sample, on_error, with_ping
        self.samples: list[dict] = []     # {t, rssi, ch, bssid, loss, avg, http_ms, ping_ms}
        self.last: dict | None = None
        self.load: dict | None = None
        self.errors = 0
        self.reach: dict = {}
        self.reach_ips: dict = {}
        self._stop = threading.Event()
        self._th: threading.Thread | None = None

    @property
    def running(self) -> bool:
        return bool(self._th and self._th.is_alive())

    def start(self) -> None:
        if self.running:
            return
        self._stop.clear()
        self._th = threading.Thread(target=self._run, daemon=True)
        self._th.start()

    def stop(self, resume: bool = False) -> None:
        """Arrete la mesure ; resume=True remet ensuite le DMD en fonctionnement normal (POST /dmd-resume)."""
        self._stop.set()
        if self._th and self._th is not threading.current_thread():
            self._th.join(timeout=8)
        http_json(self.ip, "/diag-stop", "POST", 4)
        if resume:
            time.sleep(0.7)
            http_json(self.ip, "/dmd-resume", "POST", 4)

    def collect_extra(self) -> None:
        """Releves ponctuels pour le rapport (a lancer HORS du fil d'interface) : /load et /diag-data si la mesure ne tourne pas
        (serveur mono-connexion : jamais en parallele de la mesure), puis ping du PC vers le DMD, la passerelle et la Recalbox."""
        if not self.running:
            d, _r, _e = http_json(self.ip, "/load", "GET", 8)
            if d:
                self.load = d
            d, _r, _e = http_json(self.ip, "/diag-data", "GET", 5)
            if d:
                self.last = d
        targets = {"DMD": self.ip}
        gw = (self.last or {}).get("gw") or (self.load or {}).get("wifi_cur_gw")
        if gw:
            targets["gateway"] = gw
        rb = (self.load or {}).get("recalbox_active") or (self.load or {}).get("recalbox_ip") or (self.last or {}).get("rb")
        if rb:
            targets["Recalbox"] = rb
        self.reach = {name: [ping_ms(ip) for _ in range(4)] for name, ip in targets.items()}
        self.reach_ips = targets

    def _run(self) -> None:
        d, _r, err = http_json(self.ip, "/load", "GET", 8)
        self.load = d
        if d is None and self.on_error:
            self.on_error(err or "")
        _d, _r, err = http_json(self.ip, "/diag-start", "POST", 6)
        if err and self.on_error:
            self.on_error(err)
        self._stop.wait(0.5)
        while not self._stop.is_set():
            d, rtt, err = http_json(self.ip, "/diag-data", "GET", 5)
            if d is None:
                self.errors += 1
                if self.on_error:
                    self.on_error(err or "")
                self._stop.wait(3.0)
                continue
            self.last = d
            pg = ping_ms(self.ip) if self.with_ping else None
            self.samples.append({"t": time.time(), "rssi": d.get("rssi"), "ch": d.get("ch"), "bssid": d.get("bssid", ""),
                                 "loss": d.get("loss"), "avg": d.get("avg"), "http_ms": rtt, "ping_ms": pg})
            if self.on_sample:
                self.on_sample(d)
            self._stop.wait(2.0)


# ---------------------------------------------------------------- suivi Wi-Fi (DMD sans USB)
_BAD_BOOT = re.compile(r"WDT|PANIC|BROWNOUT|ABORT", re.I)
WLOG_TXT = {
    "fr": {"start": "suivi de {ip} démarré : réseau {ssid}, AP {ap}, {rssi} dBm, en marche depuis {up}", "fw": "{kind} version {fw}, scripts Recalbox : {sc}, dernier démarrage : {br}, dernière mise à jour : {ota}",
           "down": "plus de réponse du DMD ({err})", "still": "toujours sans réponse depuis {s} s", "back": "DMD de nouveau joignable après {s} s",
           "reboot": "redémarrage détecté (le DMD tournait depuis {a} s, repart de {b} s)", "cause": "cause du redémarrage : {br}", "causeok": "cause du redémarrage : {br}",
           "disc": "déconnexion Wi-Fi n°{n} : {r}", "ap": "point d'accès changé : {a} -> {b} ({rssi} dBm)", "wifidown": "Wi-Fi perdu", "wifiup": "Wi-Fi rétabli ({rssi} dBm)",
           "weak": "signal faible : {rssi} dBm", "weakok": "signal redevenu correct : {rssi} dBm", "rbnone": "la Recalbox {rb} ne répond plus aux tests ({n} essais)",
           "rbloss": "pertes vers la Recalbox : {l} % ({g}/{n}), {a} ms en moyenne", "rbok": "liaison avec la Recalbox rétablie ({a} ms)", "gap": "coupure de réception UDP n°{n}",
           "mem": "mémoire minimale en baisse : {m} octets (libre {f})", "ok": "état : en marche depuis {up}, {rssi} dBm, perte {l} %, mémoire libre {f} octets"},
    "en": {"start": "following {ip} started: network {ssid}, AP {ap}, {rssi} dBm, up for {up}", "fw": "{kind} version {fw}, Recalbox scripts: {sc}, last reboot: {br}, last update: {ota}",
           "down": "the DMD stopped answering ({err})", "still": "still not answering after {s} s", "back": "DMD reachable again after {s} s",
           "reboot": "reboot detected (the DMD had been up {a} s, restarts from {b} s)", "cause": "reboot cause: {br}", "causeok": "reboot cause: {br}",
           "disc": "Wi-Fi disconnection #{n}: {r}", "ap": "access point changed: {a} -> {b} ({rssi} dBm)", "wifidown": "Wi-Fi lost", "wifiup": "Wi-Fi back ({rssi} dBm)",
           "weak": "weak signal: {rssi} dBm", "weakok": "signal OK again: {rssi} dBm", "rbnone": "the Recalbox {rb} no longer answers the tests ({n} tries)",
           "rbloss": "loss towards the Recalbox: {l} % ({g}/{n}), {a} ms average", "rbok": "Recalbox link restored ({a} ms)", "gap": "UDP reception gap #{n}",
           "mem": "minimum memory dropping: {m} bytes (free {f})", "ok": "status: up {up}, {rssi} dBm, loss {l} %, free memory {f} bytes"},
}


class WifiLogger:
    """Suivi d'un DMD SANS cable USB : /diag-data toutes les 3 s (leger), /load une seule fois au demarrage du suivi et apres chaque redemarrage
    detecte. Reconstitue un journal horodate d'EVENEMENTS (redemarrage et sa cause, perte de liaison, deconnexions Wi-Fi, changement de point
    d'acces, liaison Recalbox, memoire) -- ce n'est PAS le journal serie brut : ni l'ESP32 (3.98/4.00) ni le demon Pi n'exposent /log (404)."""

    def __init__(self, ip: str, on_line=None, interval: float = 3.0, lang: str = "fr", maxlines: int = 20000):
        self.ip, self.on_line, self.interval = ip, on_line, interval
        self.t = WLOG_TXT.get(lang, WLOG_TXT["en"])
        self.lang = lang if lang in WLOG_TXT else "en"
        self.lines: deque = deque(maxlen=maxlines)
        self.load: dict | None = None
        self.last: dict | None = None
        self._stop = threading.Event()
        self._th: threading.Thread | None = None

    @property
    def running(self) -> bool:
        return bool(self._th and self._th.is_alive())

    def start(self) -> None:
        if self.running:
            return
        self._stop.clear()
        self._th = threading.Thread(target=self._run, daemon=True)
        self._th.start()

    def stop(self) -> None:
        self._stop.set()
        if self._th and self._th is not threading.current_thread():
            self._th.join(timeout=8)

    def emit(self, tag: str, key: str, **kw) -> None:
        ln = f"{time.strftime('%H:%M:%S')} [{tag}] {self.t[key].format(**kw)}"
        self.lines.append(ln)
        if self.on_line:
            self.on_line(ln)

    def _fetch_load(self, announce: bool, reboot: bool) -> None:
        l, _r, _e = http_json(self.ip, "/load", "GET", 8)
        if not l:
            return
        self.load = l
        br = (l.get("boot_reason") or "-").strip()
        if reboot:
            bad = bool(_BAD_BOOT.search(br))
            self.emit("CRASH" if bad else "REBOOT", "cause", br=br)
        if announce:
            self.emit("INFO", "fw", kind=device_kind(l, self.last), fw=l.get("fw_version") or "?", sc=l.get("scripts_bundle") or "-", br=br, ota=l.get("ota_last") or "-")

    def _run(self) -> None:
        prev = None
        down_since = None
        last_still = 0.0
        last_status = time.time()
        load_at = None
        while not self._stop.is_set():
            d, _rtt, err = http_json(self.ip, "/diag-data", "GET", 5)
            now = time.time()
            if d is None or not d:
                if down_since is None:
                    down_since = now
                    last_still = now
                    self.emit("LINK", "down", err=(err or "").split(":")[0] or "timeout")
                elif now - last_still >= 30:
                    last_still = now
                    self.emit("LINK", "still", s=int(now - down_since))
                self._stop.wait(self.interval)
                continue
            self.last = d
            if down_since is not None:
                self.emit("LINK", "back", s=int(now - down_since))
                down_since = None
            up_s = int(d.get("up_s", 0))
            rssi = d.get("rssi", -127)
            if prev is None:
                self.emit("INFO", "start", ip=self.ip, ssid=d.get("ssid") or "-", ap=d.get("bssid") or "-", rssi=rssi, up=fmt_ago(up_s))
                load_at = now + 0.1
                self._fetch_load(True, False)
                load_at = None
            else:
                if up_s + 5 < int(prev.get("up_s", 0)):
                    self.emit("REBOOT", "reboot", a=int(prev.get("up_s", 0)), b=up_s)
                    load_at = now + 8.0     # laisser le DMD finir son demarrage avant d'interroger /load
                    prev = dict(prev, disc=0, gaps=0, minheap=d.get("minheap", 0))
                if bool(prev.get("up")) and not d.get("up"):
                    self.emit("WIFI", "wifidown")
                elif not prev.get("up") and d.get("up"):
                    self.emit("WIFI", "wifiup", rssi=rssi)
                if d.get("disc", 0) > prev.get("disc", 0):
                    self.emit("WIFI", "disc", n=d.get("disc"), r=reason_text(d.get("dr"), self.lang))
                if d.get("up") and prev.get("bssid") and d.get("bssid") and d.get("bssid") != prev.get("bssid"):
                    self.emit("WIFI", "ap", a=prev.get("bssid"), b=d.get("bssid"), rssi=rssi)
                if d.get("up") and rssi < -75 <= prev.get("rssi", -127) and prev.get("up"):
                    self.emit("WIFI", "weak", rssi=rssi)
                elif d.get("up") and rssi >= -75 > prev.get("rssi", 0) and prev.get("up"):
                    self.emit("WIFI", "weakok", rssi=rssi)
                if d.get("rb") and d.get("n"):
                    was_none = prev.get("n") and prev.get("got") == 0
                    if d.get("got") == 0 and not was_none:
                        self.emit("RB", "rbnone", rb=d.get("rb"), n=d.get("n"))
                    elif d.get("got") and was_none:
                        self.emit("RB", "rbok", a=d.get("avg"))
                    elif d.get("got") and d.get("loss", 0) > 0 and d.get("loss", 0) != prev.get("loss", 0) and (d.get("loss", 0) >= 10 or prev.get("loss", 0) == 0):
                        self.emit("RB", "rbloss", l=d.get("loss"), g=d.get("got"), n=d.get("n"), a=d.get("avg"))
                if d.get("gaps", 0) > prev.get("gaps", 0):
                    self.emit("UDP", "gap", n=d.get("gaps"))
                if not d.get("pi") and up_s > 180 and d.get("minheap") and prev.get("minheap") and d["minheap"] <= prev["minheap"] - 4096:   # pas pendant le demarrage (la memoire minimale y baisse normalement)
                    self.emit("MEM", "mem", m=d["minheap"], f=d.get("heap"))
            if load_at is not None and now >= load_at:
                load_at = None
                self._fetch_load(False, True)
            if now - last_status >= 60:
                last_status = now
                self.emit("OK", "ok", up=fmt_ago(up_s), rssi=rssi, l=d.get("loss", 0), f=d.get("heap"))
            prev = d
            self._stop.wait(self.interval)


# ---------------------------------------------------------------- USB / serie
USB_CHIPS = {(0x10C4, 0xEA60): "Silicon Labs CP210x (ESP32 DevKit)", (0x1A86, 0x7523): "WCH CH340 (ESP32 clone)",
             (0x1A86, 0x55D4): "WCH CH9102 (ESP32)", (0x0403, 0x6001): "FTDI FT232", (0x0403, 0x6015): "FTDI FT231X",
             (0x303A, 0x1001): "Espressif USB-JTAG (ESP32-S3/C3)", (0x2341, 0x0043): "Arduino Uno"}


def list_serial_ports() -> list[dict]:
    if _lp is None:
        return []
    out = []
    for p in _lp.comports():
        chip = USB_CHIPS.get((p.vid, p.pid), "") if p.vid else ""
        out.append({"device": p.device, "description": p.description or "", "manufacturer": p.manufacturer or "",
                    "vid_pid": (f"{p.vid:04X}:{p.pid:04X}" if p.vid else ""), "chip": chip, "serial": p.serial_number or ""})
    out.sort(key=lambda d: (not d["chip"], d["device"]))     # les ports reconnus (ESP32) en tete
    return out


CRASH_PATTERNS = ("Guru Meditation", "Backtrace:", "abort()", "assert failed", "CORRUPT HEAP", "Task watchdog", "Brownout",
                  "rst:0x", "Rebooting...", "ELF file SHA256", "Stack smashing", "LoadProhibited", "StoreProhibited", "panic", "[CRASH]")   # [CRASH] : ligne du suivi Wi-Fi (redemarrage a cause anormale)


_HEAP = re.compile(r"free=(\d+) maxalloc=(\d+) minFreeHeap=(\d+)")
_NOISE = ("[LOOPDIAG]", "[LOOPDIAG2]")


def serial_summary(lines) -> list[str]:
    """Resume du journal serie : memoire (free / maxalloc / minFreeHeap vus dans [LOOPDIAG]), nombre de redemarrages, evenements Recalbox."""
    L = list(lines)
    heap = [tuple(int(x) for x in m.groups()) for m in (_HEAP.search(ln) for ln in L) if m]
    out = []
    if heap:
        free, mx, mn = zip(*heap)
        out.append(f"Heap ({len(heap)} readings): free first {free[0]} / last {free[-1]} / lowest {min(free)} ; largest block lowest {min(mx)} ; minFreeHeap lowest {min(mn)}")
    resets = [ln.strip()[:120] for ln in L if "rst:0x" in ln]
    out.append(f"Reboots seen in this log: {len(resets)}" + ("".join(f"\n  {r}" for r in resets[:5])))
    ev = [ln.strip()[:140] for ln in L if "[EVENT]" in ln]
    if ev:
        out.append(f"Recalbox events seen: {len(ev)} (last 8):" + "".join(f"\n  {e}" for e in ev[-8:]))
    return out


def config_lines(load: dict | None, limit: int = 110) -> list[str]:
    """Configuration complete du DMD (/load) : cles non vides, secrets masques, valeurs tronquees."""
    if not load:
        return []
    r = redact(load)
    return [f"{k}={str(v)[:limit]}" for k, v in sorted(r.items()) if v not in ("", None)]


def scan_crash(lines) -> list[str]:
    """Lignes marquantes d'un plantage ESP32 (numero + texte) -- a joindre au rapport ; le SHA ELF identifie le binaire exact."""
    out = []
    for i, ln in enumerate(lines, 1):
        if any(p.lower() in ln.lower() for p in CRASH_PATTERNS):
            out.append(f"{i}: {ln.strip()[:200]}")
    return out[:80]


class SerialSession:
    """Moniteur serie. DTR et RTS sont mis a l'etat BAS avant l'ouverture : sur un ESP32 (DTR->GPIO0, RTS->EN) cela evite le
    redemarrage du DMD a la connexion (piege connu : ouvrir le port le redemarrait). reset_board() le redemarre a la demande."""

    def __init__(self, port: str, baud: int = 115200, on_line=None, maxlines: int = 20000):
        self.port, self.baud, self.on_line = port, baud, on_line
        self.lines: deque = deque(maxlen=maxlines)
        self.ser = None
        self._stop = threading.Event()
        self._th: threading.Thread | None = None
        self.error = ""
        self.hide: list[str] = []      # motifs de lignes a ne pas garder (ex. « WIFIPS » : inondation observee sur DMD1, 2500 lignes/s)
        self.hidden = 0                # nombre de lignes ecartees par `hide`
        self.garbage = 0               # lignes illisibles ecartees

    def _emit(self, ln: str) -> None:
        if any(h and h in ln for h in self.hide):
            self.hidden += 1
            return
        if ln and sum(1 for ch in ln if ch == "�" or (ord(ch) < 32 and ch != "\t")) > len(ln) * 0.3:
            self.garbage += 1             # bruit illisible (mauvais debit, ou sortie du bootloader ROM a 74880 bauds) : compte, jamais garde
            return
        self.lines.append(ln)
        if self.on_line:
            self.on_line(ln)

    def open(self) -> None:
        if serial is None:
            raise RuntimeError("pyserial absent")
        s = serial.Serial()
        s.port, s.baudrate, s.timeout = self.port, self.baud, 0.2
        s.dtr, s.rts = False, False
        s.open()
        self.ser = s
        self._stop.clear()
        self._th = threading.Thread(target=self._run, daemon=True)
        self._th.start()

    def _run(self) -> None:
        buf = b""
        while not self._stop.is_set():
            try:
                data = self.ser.read(self.ser.in_waiting or 1)
            except Exception as e:                     # cable debranche, port repris par un autre programme
                self.error = f"{type(e).__name__}: {e}"
                if self.on_line:
                    self.on_line(f"[toolkit] port serie perdu : {self.error}")
                return
            if not data:
                continue
            buf += data
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                self._emit(raw.decode("utf-8", "replace").rstrip("\r"))
            if len(buf) > 8192:                         # ligne sans fin (mauvais debit) : on la coupe
                self._emit(buf.decode("utf-8", "replace"))
                buf = b""

    def reset_board(self) -> None:
        """Impulsion sur EN (RTS) : redemarre l'ESP32 pour capturer son demarrage complet."""
        if self.ser:
            self.ser.dtr = False
            self.ser.rts = True
            time.sleep(0.15)
            self.ser.rts = False

    def close(self) -> None:
        self._stop.set()
        if self._th:
            self._th.join(timeout=2)
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass
        self.ser = None

    @property
    def is_open(self) -> bool:
        return bool(self.ser and self.ser.is_open and not self.error)


# ---------------------------------------------------------------- inventaire du PC
def _ram_gb() -> str:
    try:
        import ctypes

        class MS(ctypes.Structure):
            _fields_ = [("l", ctypes.c_ulong), ("load", ctypes.c_ulong), ("total", ctypes.c_ulonglong), ("avail", ctypes.c_ulonglong),
                        ("tp", ctypes.c_ulonglong), ("ap", ctypes.c_ulonglong), ("tv", ctypes.c_ulonglong), ("av", ctypes.c_ulonglong),
                        ("ae", ctypes.c_ulonglong)]
        m = MS()
        m.l = ctypes.sizeof(MS)
        ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
        return f"{m.total / 2**30:.1f} Go"
    except Exception:
        return "?"


def pc_wifi_lines() -> list[str]:
    """Etat de la liaison Wi-Fi du PC (SSID, BSSID, canal, signal, bande) -- utile pour savoir si le PC est en 5 GHz et le DMD en 2,4 GHz."""
    if os.name != "nt":
        return []
    try:
        out = subprocess.run(["netsh", "wlan", "show", "interfaces"], capture_output=True, timeout=8,
                             creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)).stdout.decode("mbcs", "replace")
    except Exception:
        return []
    keep = re.compile(r"^\s*(ssid|bssid|radio|canal|channel|signal|authentif|réception|reception|transmission|débit|rate)", re.I)
    return [ln.strip() for ln in out.splitlines() if keep.search(ln)]


def pc_info(tool_version: str = "") -> dict:
    return {"toolkit": tool_version, "os": platform.platform(), "machine": platform.machine(), "cpu": platform.processor(),
            "ram": _ram_gb(), "python": platform.python_version(), "pyserial": getattr(serial, "VERSION", "absent"),
            "wifi": pc_wifi_lines()}


# ---------------------------------------------------------------- rapport et export
def _stats(vals):
    v = [x for x in vals if isinstance(x, (int, float))]
    if not v:
        return "n/a"
    avg = sum(v) / len(v)
    jit = (sum((x - avg) ** 2 for x in v) / len(v)) ** 0.5
    return f"min {min(v):.0f} / avg {avg:.0f} / max {max(v):.0f} ms, jitter {jit:.0f} ms ({len(v)} ok)"


def build_report(pc: dict, ports: list[dict], ip: str, wifi: "WifiDiag | None", serial_lines, port_name: str = "", baud: int = 0,
                 hidden: int = 0, hide: "list[str] | None" = None, lang: str = "fr") -> str:
    """Rapport texte (resume des points d'attention + tableaux alignes). Voir RecalBoxDMD_diag_report.py."""
    import RecalBoxDMD_diag_report as rp
    return rp.render_text(rp.build_data(pc, ports, ip, wifi, serial_lines, port_name, baud, hidden, hide, lang))


def build_report_html(pc: dict, ports: list[dict], ip: str, wifi: "WifiDiag | None", serial_lines, port_name: str = "", baud: int = 0,
                      hidden: int = 0, hide: "list[str] | None" = None, lang: str = "fr") -> str:
    """Meme rapport en HTML autonome (couleurs, courbes, sections repliables)."""
    import RecalBoxDMD_diag_report as rp
    return rp.render_html(rp.build_data(pc, ports, ip, wifi, serial_lines, port_name, baud, hidden, hide, lang))


def export_zip(path: str, report: str, wifi: WifiDiag | None, serial_lines, html: "str | None" = None) -> str:
    """Ecrit le .zip de diagnostic (rapport, journal serie complet, echantillons Wi-Fi, /load masque) et renvoie `path`."""
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        if html:
            z.writestr("report.html", html)
        z.writestr("report.txt", report)
        if serial_lines:
            z.writestr("serial_log.txt", "\n".join(serial_lines))
        if wifi and wifi.samples:
            b = io.StringIO()
            w = csv.writer(b)
            w.writerow(["time", "rssi_dbm", "channel", "bssid", "loss_pct", "rb_avg_ms", "pc_http_ms", "pc_ping_ms"])
            for x in wifi.samples:
                w.writerow([datetime.fromtimestamp(x["t"]).strftime("%H:%M:%S"), x["rssi"], x["ch"], x["bssid"], x["loss"], x["avg"],
                            "" if x["http_ms"] is None else f"{x['http_ms']:.0f}", "" if x["ping_ms"] is None else f"{x['ping_ms']:.0f}"])
            z.writestr("wifi_samples.csv", b.getvalue())
        if wifi and wifi.load:
            z.writestr("device_load.json", json.dumps(redact(wifi.load), indent=1, ensure_ascii=False))
        if wifi and wifi.last:
            z.writestr("diag_data.json", json.dumps(wifi.last, indent=1, ensure_ascii=False))
    return path
