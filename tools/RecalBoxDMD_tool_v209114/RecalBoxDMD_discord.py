# -*- coding: utf-8 -*-
# RecalBoxDMD_discord.py -- envoi de rapports du toolkit vers le serveur Discord (detection des versions pour les tests, demande d'aide).
#
# v1 - 2026-11-24 - safe-modify - Creation (dev/toolkit-discord). Logique SANS interface (testable sans Tk) ; l'interface est dans le
#   bloc « Discord » de l'onglet Diagnostic (RecalBoxDMD_diag_gui.py). Une panne de ce module ne doit jamais empecher l'ouverture du toolkit.
#
# Principe : le toolkit POSTE un message dans un salon PRIVE du serveur via un webhook ; le bot Discord le lit, le supprime et
# range les versions du membre (appairage par un code personnel donne par la commande /dmdlink). Rien ne part sans clic
# sur « Envoyer » apres un apercu. Aucune adresse IP / MAC / SSID n'est envoyee. Les URL des webhooks ne sont PAS dans l'executable :
# elles sont lues dans un petit fichier publie sur les pages du projet (revocable sans nouvelle version du toolkit).
#
# Format du message (le meme que celui lu par le bot, voir alphareport.py du bot) :
#     DMD-REPORT v1 kind=test|support code=XXXXXXXX
#     channel=alpha|stable      toolkit=200112
#     esp32_fw=4.36             esp32_channel=alpha        (ou, sur un Raspberry Pi : pi_daemon / pi_channel)
#     rb_scripts=11             recalbox=10.1.1            os=Windows 11
#     [--- message ---  texte libre de l'utilisateur (demande d'aide)]
from __future__ import annotations

import io
import json
import os
import re
import time
import urllib.error
import urllib.request
import uuid
import zipfile

INTAKE_URL = os.environ.get("DMD_DISCORD_INTAKE") or "https://shan-aya.github.io/RecalBoxDMD/discord-intake.json"
WEBHOOK_RE = re.compile(r"^https://(?:discord|discordapp)\.com/api/webhooks/\d+/[A-Za-z0-9_\-]+$")
CODE_RE = re.compile(r"^[A-Z0-9]{6,12}$")
KINDS = {"test": "alpha_reports", "support": "support"}      # genre de rapport -> cle de l'URL dans le fichier d'entree
MIN_INTERVAL_S = 30            # delai minimal entre deux envois (en plus des limites du bot)
MAX_ZIP_BYTES = 7 * 1024 * 1024
MAX_MESSAGE_CHARS = 1900       # limite Discord : 2000
MAX_USER_TEXT = 900
_INTAKE_CACHE: tuple[float, dict] = (0.0, {})

RE_IP = re.compile(r"\b\d{1,3}(?:\.\d{1,3}){3}\b")
RE_MAC = re.compile(r"\b(?:[0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}\b")
RE_SSID_LINE = re.compile(r"(?im)^(\s*(?:wifi_ssid|ssid)\s*[=:]\s*).+$")


class DiscordError(Exception):
    """Erreur d'envoi avec un code court (cle de traduction dans l'interface) et un detail technique."""

    def __init__(self, code: str, detail: str = ""):
        super().__init__(code + (": " + detail if detail else ""))
        self.code, self.detail = code, detail


# ---------------------------------------------------------------- code d'appairage
def normalize_code(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", s or "").upper()


def valid_code(s: str) -> bool:
    return bool(CODE_RE.match(normalize_code(s)))


# ---------------------------------------------------------------- fichier d'entree (URL des webhooks)
def load_intake(timeout: float = 6.0, force: bool = False) -> dict:
    """Lit le fichier d'entree publie par le projet : {"enabled": bool, "min_toolkit": int, "alpha_reports": url, "support": url}.
    Leve DiscordError('intake_unreachable' | 'intake_invalid' | 'intake_disabled'). Garde en cache 10 minutes."""
    global _INTAKE_CACHE
    ts, cached = _INTAKE_CACHE
    if cached and not force and time.time() - ts < 600:
        return cached
    try:
        req = urllib.request.Request(INTAKE_URL, headers={"User-Agent": "recalbox-toolkit"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            data = json.loads(r.read(20000).decode("utf-8"))
    except (urllib.error.URLError, OSError, ValueError) as e:
        raise DiscordError("intake_unreachable", f"{type(e).__name__}: {e}")
    if not isinstance(data, dict):
        raise DiscordError("intake_invalid")
    for key in ("alpha_reports", "support"):
        url = data.get(key)
        if url is not None and not WEBHOOK_RE.match(str(url)):
            raise DiscordError("intake_invalid", key)
    if data.get("enabled") is False:
        raise DiscordError("intake_disabled", str(data.get("message") or ""))
    _INTAKE_CACHE = (time.time(), data)
    return data


def webhook_for(kind: str, intake: dict, toolkit_build: int = 0) -> str:
    key = KINDS.get(kind)
    url = (intake or {}).get(key or "")
    if not url:
        raise DiscordError("kind_unavailable", kind)
    try:
        need = int(intake.get("min_toolkit") or 0)
    except (TypeError, ValueError):
        need = 0
    if toolkit_build and need and toolkit_build < need:
        raise DiscordError("toolkit_too_old", str(need))
    return url


# ---------------------------------------------------------------- detection
def detect(ip: str, toolkit_build, channel: str, os_name: str, probe, rb_version=None) -> tuple[dict, list]:
    """Versions REELLES du materiel joignable. -> (versions, notes).

    `probe(ip)` = RecalBoxDMD_tool._dmd_probe (dict ou None) ; `rb_version(host)` = read_recalbox_version (lecture seule, silencieuse).
    Un appareil injoignable donne une note, jamais une erreur : on envoie ce qu'on a pu detecter."""
    v = {"channel": (channel or "").lower(), "toolkit": str(toolkit_build or ""), "os": os_name or ""}
    notes = []
    d = probe(ip) if ip else None
    if not ip:
        notes.append("no_dmd_ip")
    elif not d:
        notes.append("dmd_unreachable")
    else:
        pfx = "pi" if d.get("is_pi") else "esp32"
        v["pi" if d.get("is_pi") else "esp32_fw"] = d.get("version") or ""
        v[pfx + "_channel"] = d.get("channel") or ""
        if d.get("scripts"):
            v["rb_scripts"] = d["scripts"]
        rb = d.get("rb_active") or (d.get("rb_ips") or [""])[0]
        if rb and rb_version:
            try:
                v["recalbox"] = rb_version(rb) or ""
            except Exception:   # noqa: BLE001
                v["recalbox"] = ""
            if not v["recalbox"]:
                notes.append("recalbox_unreadable")
        elif not rb:
            notes.append("no_recalbox")
    return {k: x for k, x in v.items() if x}, notes


# ---------------------------------------------------------------- confidentialite
def scrub(text: str, secrets=()) -> str:
    """Retire adresses IP, adresses MAC et SSID (et les valeurs `secrets` : SSID connu, etc.) d'un texte avant envoi."""
    text = RE_SSID_LINE.sub(r"\1<ssid>", text or "")
    for s in secrets:
        if s and len(s) >= 3:
            text = text.replace(s, "<ssid>")
    return RE_MAC.sub("xx:xx:xx:xx:xx:xx", RE_IP.sub("x.x.x.x", text))


# ---------------------------------------------------------------- message
def build_message(kind: str, code: str, versions: dict, user_text: str = "") -> str:
    code = normalize_code(code)
    if kind not in KINDS or not valid_code(code):
        raise DiscordError("bad_request")
    # cle interne (sortie de detect) -> nom dans le message ; le bot lit ces noms (alphareport.py)
    fields = (("channel", "channel"), ("toolkit", "toolkit"), ("esp32_fw", "esp32_fw"), ("esp32_channel", "esp32_channel"),
              ("pi", "pi_daemon"), ("pi_channel", "pi_channel"), ("rb_scripts", "rb_scripts"), ("recalbox", "recalbox"), ("os", "os"))
    lines = [f"DMD-REPORT v1 kind={kind} code={code}"]
    for key, name in fields:
        val = versions.get(key)
        if val:
            lines.append(f"{name}={scrub(str(val))[:60]}")
    if user_text.strip():
        lines += ["--- message ---", scrub(user_text.strip())[:MAX_USER_TEXT]]
    return "\n".join(lines)[:MAX_MESSAGE_CHARS]


def build_support_zip(report: str, serial_lines, secrets=()) -> bytes:
    """Archive envoyee avec une demande d'aide : rapport et journal serie, SCRUBBES (IP / MAC / SSID). Plafonnee a MAX_ZIP_BYTES."""
    lines = [scrub(x, secrets) for x in (serial_lines or [])]
    keep = 4000
    while True:
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as z:
            z.writestr("report.txt", scrub(report, secrets))
            if lines:
                z.writestr("serial_log.txt", "\n".join(lines[-keep:]))
        if buf.tell() <= MAX_ZIP_BYTES or keep <= 100:
            return buf.getvalue()
        keep //= 2


# ---------------------------------------------------------------- envoi
def _multipart(fields: dict, files: list) -> tuple[bytes, str]:
    b = "----dmd" + uuid.uuid4().hex
    out = io.BytesIO()
    for k, v in fields.items():
        out.write(f'--{b}\r\nContent-Disposition: form-data; name="{k}"\r\n\r\n{v}\r\n'.encode("utf-8"))
    for i, (name, data) in enumerate(files):
        out.write(f'--{b}\r\nContent-Disposition: form-data; name="files[{i}]"; filename="{name}"\r\n'
                  f"Content-Type: application/octet-stream\r\n\r\n".encode("utf-8"))
        out.write(data + b"\r\n")
    out.write(f"--{b}--\r\n".encode())
    return out.getvalue(), f"multipart/form-data; boundary={b}"


def post(url: str, content: str, files: list | None = None, timeout: float = 25.0) -> None:
    """POST au webhook (aucune mention possible : `allowed_mentions` vide). Leve DiscordError('http_<code>' | 'network' | 'rate_limited')."""
    if not WEBHOOK_RE.match(url or ""):
        raise DiscordError("intake_invalid")
    payload = {"content": content, "username": "DMD Toolkit", "allowed_mentions": {"parse": []}}
    if files:
        body, ctype = _multipart({"payload_json": json.dumps(payload, ensure_ascii=False)}, files)
    else:
        body, ctype = json.dumps(payload, ensure_ascii=False).encode("utf-8"), "application/json"
    req = urllib.request.Request(url, data=body, method="POST", headers={"Content-Type": ctype, "User-Agent": "recalbox-toolkit"})
    try:
        urllib.request.urlopen(req, timeout=timeout).read()
    except urllib.error.HTTPError as e:
        if e.code == 429:
            raise DiscordError("rate_limited")
        raise DiscordError(f"http_{e.code}")
    except (urllib.error.URLError, OSError) as e:
        raise DiscordError("network", f"{type(e).__name__}: {e}")


def too_soon(last_ts, now: float | None = None) -> int:
    """Secondes a attendre avant un nouvel envoi (0 = permis)."""
    try:
        wait = MIN_INTERVAL_S - ((now or time.time()) - float(last_ts))
    except (TypeError, ValueError):
        return 0
    return max(0, int(wait + 0.999)) if wait > 0 else 0
