# -*- coding: utf-8 -*-
# RecalBoxDMD_diag_report.py -- rapport de diagnostic du toolkit (onglet Diagnostic) : donnees structurees, ANALYSE automatique
# (points d'attention en tete) et deux rendus : texte aligne (collable dans un message) et HTML autonome (couleurs, courbes, sections
# repliables). Langue : fr / en (es -> en). Aucune dependance Tk.
#
# v1 - 2026-10-31 - safe-modify - Creation (refonte de la mise en forme de RecalBoxDMD_diag.build_report, qui delegue ici).
from __future__ import annotations

import html as _html
import re
from datetime import datetime

import RecalBoxDMD_diag as dg

LOGO_URL = "https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/main/medias/branding/shan_aya_logo.png"

R = {
    "fr": {
        "title": "Rapport de diagnostic RecalBoxDMD", "generated": "Généré le", "device": "Appareil", "firmware": "Firmware", "pidaemon": "Démon Pi",
        "toolkit": "Toolkit", "scripts": "Scripts Recalbox", "notrep": "non annoncés", "unknown": "inconnu", "nodev": "aucun appareil joint",
        "summary": "Résumé", "attn": "{n} point(s) d'attention", "allok": "Aucun problème détecté", "verdict": "Verdict",
        "sec_ver": "Versions", "sec_dev": "Appareil (par Wi-Fi)", "sec_wifi": "Wi-Fi et liaison Recalbox", "sec_reach": "Accessibilité depuis le PC",
        "sec_link": "Liaison PC → DMD", "sec_pc": "PC", "sec_usb": "Ports série USB", "sec_ser": "Journal série", "sec_cfg": "Configuration du DMD",
        "sec_ev": "Historique Wi-Fi (le plus récent en haut)", "sec_tail": "Dernières lignes série", "sec_wlog": "Journal des événements (suivi Wi-Fi)", "sec_tailw": "Derniers événements", "none": "(aucun)", "noans": "(pas de réponse)",
        "k_kind": "Matériel", "k_boot": "Dernier redémarrage", "k_up": "En marche depuis", "k_ota": "Dernière mise à jour", "k_rbip": "Recalbox",
        "k_ssid": "Réseau", "k_ap": "Point d'accès", "k_ch": "Canal", "k_rssi": "Signal", "k_q": "Qualité", "k_ip": "IP du DMD / passerelle",
        "k_loss": "Perte", "k_lat": "Latence min / moy / max", "k_jit": "Gigue", "k_age": "Dernier paquet", "k_gaps": "Coupures UDP",
        "k_disc": "Reconnexions Wi-Fi", "k_last": "Dernière déconnexion", "k_heap": "Mémoire libre (minimum)", "k_http": "Aller-retour HTTP",
        "k_ping": "Ping ICMP", "k_samples": "Échantillons", "k_os": "Système", "k_cpu": "Processeur", "k_ram": "Mémoire", "k_py": "Python / pyserial",
        "k_pcwifi": "Wi-Fi du PC", "k_lines": "Lignes", "k_heapser": "Mémoire (série)", "k_reboots": "Redémarrages vus", "k_events": "Événements Recalbox",
        "k_crash": "Marqueurs de plantage", "k_port": "Port", "answered": "{a}/{b} réponses", "ago": "il y a", "conn": "connecté au point d'accès",
        "disc_ev": "déconnecté", "dropped": "{n} ligne(s) « {pat} » écartée(s) (filtre anti-rafale)", "tail_note": "150 dernières lignes, sans les lignes périodiques [LOOPDIAG]",
        "cfg_note": "{n} clés non vides, mots de passe masqués", "series_lat": "Latence Recalbox (ms)", "series_rssi": "Signal vu par le DMD (dBm)",
        "series_http": "Aller-retour HTTP du PC (ms)",
        "f_boot_bad": "Le dernier redémarrage est anormal : {r}", "f_boot_ok": "Dernier redémarrage : {r}", "f_ota_bad": "La dernière mise à jour a échoué : {o}",
        "f_ota_ok": "Dernière mise à jour : {o}", "f_nowifi": "Le DMD n'est pas connecté au Wi-Fi", "f_rssi_bad": "Signal Wi-Fi {q} ({r} dBm)",
        "f_rssi_warn": "Signal Wi-Fi {q} ({r} dBm)", "f_rssi_ok": "Signal Wi-Fi {q} ({r} dBm)", "f_norb": "IP de la Recalbox inconnue côté DMD",
        "f_rb_none": "La Recalbox ne répond à aucun test ({n} essais)", "f_loss_bad": "Perte de {l} % vers la Recalbox", "f_loss_warn": "Perte de {l} % vers la Recalbox",
        "f_loss_ok": "Aucune perte vers la Recalbox ({g}/{n}, {a} ms en moyenne)", "f_lat_bad": "Latence élevée vers la Recalbox : {a} ms en moyenne",
        "f_lat_warn": "Latence moyenne vers la Recalbox : {a} ms", "f_jit_bad": "Gigue élevée : {j} ms", "f_jit_warn": "Gigue notable : {j} ms",
        "f_disc_bad": "{n} reconnexions Wi-Fi depuis le démarrage (dernière : {r})", "f_disc_warn": "{n} reconnexion(s) Wi-Fi depuis le démarrage (dernière : {r})",
        "f_disc_ok": "Aucune reconnexion Wi-Fi depuis le démarrage", "f_gaps": "{n} coupure(s) de réception UDP depuis le démarrage",
        "f_heap_bad": "Mémoire très basse : minimum {m} octets", "f_heap_warn": "Mémoire basse : minimum {m} octets",
        "f_reach_bad": "{name} ({ip}) ne répond pas au ping depuis le PC", "f_reach_warn": "{name} ({ip}) : {a}/{b} réponses au ping",
        "f_http": "Réponses HTTP du DMD lentes : {a} ms en moyenne", "f_scripts": "Les scripts Recalbox ne sont pas annoncés au DMD (marquee.sh absent ou arrêté ?)",
        "f_crash": "{n} marqueur(s) de plantage dans le journal série", "f_reboots": "{n} redémarrage(s) vu(s) dans le journal série",
        "f_nodev": "Aucun appareil n'a répondu par Wi-Fi : adresse du DMD à vérifier", "f_ser_ok": "Journal série sans marqueur de plantage",
    },
    "en": {
        "title": "RecalBoxDMD diagnostic report", "generated": "Generated", "device": "Device", "firmware": "Firmware", "pidaemon": "Pi daemon",
        "toolkit": "Toolkit", "scripts": "Recalbox scripts", "notrep": "not reported", "unknown": "unknown", "nodev": "no device reached",
        "summary": "Summary", "attn": "{n} point(s) need attention", "allok": "No problem detected", "verdict": "Verdict",
        "sec_ver": "Versions", "sec_dev": "Device (over Wi-Fi)", "sec_wifi": "Wi-Fi and Recalbox link", "sec_reach": "Reachability from the PC",
        "sec_link": "PC → DMD link", "sec_pc": "PC", "sec_usb": "USB serial ports", "sec_ser": "Serial log", "sec_cfg": "DMD configuration",
        "sec_ev": "Wi-Fi history (most recent first)", "sec_tail": "Last serial lines", "sec_wlog": "Event log (Wi-Fi following)", "sec_tailw": "Last events", "none": "(none)", "noans": "(no answer)",
        "k_kind": "Hardware", "k_boot": "Last reboot", "k_up": "Running for", "k_ota": "Last update", "k_rbip": "Recalbox",
        "k_ssid": "Network", "k_ap": "Access point", "k_ch": "Channel", "k_rssi": "Signal", "k_q": "Quality", "k_ip": "DMD IP / gateway",
        "k_loss": "Loss", "k_lat": "Latency min / avg / max", "k_jit": "Jitter", "k_age": "Last packet", "k_gaps": "UDP gaps",
        "k_disc": "Wi-Fi reconnections", "k_last": "Last disconnection", "k_heap": "Free memory (minimum)", "k_http": "HTTP round trip",
        "k_ping": "ICMP ping", "k_samples": "Samples", "k_os": "System", "k_cpu": "CPU", "k_ram": "Memory", "k_py": "Python / pyserial",
        "k_pcwifi": "PC Wi-Fi", "k_lines": "Lines", "k_heapser": "Memory (serial)", "k_reboots": "Reboots seen", "k_events": "Recalbox events",
        "k_crash": "Crash markers", "k_port": "Port", "answered": "{a}/{b} answered", "ago": "ago", "conn": "connected to the access point",
        "disc_ev": "disconnected", "dropped": "{n} line(s) matching \"{pat}\" dropped (flood filter)", "tail_note": "last 150 lines, periodic [LOOPDIAG] lines left out",
        "cfg_note": "{n} non-empty keys, passwords masked", "series_lat": "Recalbox latency (ms)", "series_rssi": "Signal seen by the DMD (dBm)",
        "series_http": "PC HTTP round trip (ms)",
        "f_boot_bad": "The last reboot is abnormal: {r}", "f_boot_ok": "Last reboot: {r}", "f_ota_bad": "The last update failed: {o}",
        "f_ota_ok": "Last update: {o}", "f_nowifi": "The DMD is not connected to Wi-Fi", "f_rssi_bad": "Wi-Fi signal {q} ({r} dBm)",
        "f_rssi_warn": "Wi-Fi signal {q} ({r} dBm)", "f_rssi_ok": "Wi-Fi signal {q} ({r} dBm)", "f_norb": "The DMD does not know the Recalbox IP",
        "f_rb_none": "The Recalbox answers none of the tests ({n} tries)", "f_loss_bad": "{l} % loss towards the Recalbox", "f_loss_warn": "{l} % loss towards the Recalbox",
        "f_loss_ok": "No loss towards the Recalbox ({g}/{n}, {a} ms average)", "f_lat_bad": "High latency towards the Recalbox: {a} ms average",
        "f_lat_warn": "Average latency towards the Recalbox: {a} ms", "f_jit_bad": "High jitter: {j} ms", "f_jit_warn": "Noticeable jitter: {j} ms",
        "f_disc_bad": "{n} Wi-Fi reconnections since boot (last: {r})", "f_disc_warn": "{n} Wi-Fi reconnection(s) since boot (last: {r})",
        "f_disc_ok": "No Wi-Fi reconnection since boot", "f_gaps": "{n} UDP reception gap(s) since boot",
        "f_heap_bad": "Very low memory: minimum {m} bytes", "f_heap_warn": "Low memory: minimum {m} bytes",
        "f_reach_bad": "{name} ({ip}) does not answer ping from the PC", "f_reach_warn": "{name} ({ip}): {a}/{b} ping answers",
        "f_http": "Slow HTTP answers from the DMD: {a} ms average", "f_scripts": "The Recalbox scripts are not announced to the DMD (marquee.sh missing or stopped?)",
        "f_crash": "{n} crash marker(s) in the serial log", "f_reboots": "{n} reboot(s) seen in the serial log",
        "f_nodev": "No device answered over Wi-Fi: check the DMD address", "f_ser_ok": "Serial log without crash marker",
    },
}
BAD_BOOT = re.compile(r"WDT|PANIC|BROWNOUT|ABORT", re.I)
LEVEL_TXT = {"bad": "[X]", "warn": "[!]", "ok": "[OK]", "info": "[i]"}
RANK = {"bad": 3, "warn": 2, "info": 1, "ok": 0}


def _t(lang):
    return R["fr"] if lang == "fr" else R["en"]


def _stat(vals):
    v = [x for x in vals if isinstance(x, (int, float))]
    if not v:
        return None
    avg = sum(v) / len(v)
    return {"n": len(v), "min": min(v), "avg": avg, "max": max(v), "jit": (sum((x - avg) ** 2 for x in v) / len(v)) ** 0.5}


def serial_facts(lines) -> dict:
    L = list(lines)
    heap = [tuple(int(x) for x in m.groups()) for m in (dg._HEAP.search(ln) for ln in L) if m]
    return {"lines": len(L), "heap": heap,
            "resets": [ln.strip()[:120] for ln in L if "rst:0x" in ln or ("[REBOOT]" in ln and ("détecté" in ln or "detecte" in ln or "detected" in ln))],   # serie (rst:0x) ou suivi Wi-Fi ([REBOOT])
            "events": [ln.strip()[:140] for ln in L if "[EVENT]" in ln], "crash": dg.scan_crash(L),
            "tail": [ln for ln in L if not ln.startswith(dg._NOISE)][-150:]}


def build_data(pc, ports, ip, wifi, serial_lines, port_name="", baud=0, hidden=0, hide=None, lang="fr") -> dict:
    t = _t(lang)
    load, d = (wifi.load if wifi else None), (wifi.last if wifi else None)
    reached = bool(load or d)
    kind = dg.device_kind(load, d) if reached else None
    fw = (load or {}).get("fw_version") or None
    sc = (load or {}).get("scripts_bundle") or None
    ser = serial_facts(serial_lines)
    st_http = _stat([s["http_ms"] for s in wifi.samples]) if wifi and wifi.samples else None
    pings = [s["ping_ms"] for s in wifi.samples] if wifi and wifi.samples else []
    F = []   # (niveau, texte)

    def add(level, key, **kw):
        F.append((level, t[key].format(**kw)))

    if not reached:
        add("warn", "f_nodev")
    if load:
        br = (load.get("boot_reason") or "").strip()
        if br:
            add("bad" if BAD_BOOT.search(br) else "info", "f_boot_bad" if BAD_BOOT.search(br) else "f_boot_ok", r=br)
        ota = (load.get("ota_last") or "").strip()
        if ota.startswith("echec"):
            add("bad", "f_ota_bad", o=ota)
        elif ota:
            add("ok", "f_ota_ok", o=ota)
        if kind == "ESP32" and not sc:
            add("warn", "f_scripts")
    if d:
        up = bool(d.get("up"))
        if not up:
            add("bad", "f_nowifi")
        else:
            lvl, qi = dg.quality(d.get("rssi", -127))
            q = dg.QUALITY[lang if lang in dg.QUALITY else "en"][qi]
            add({"good": "ok", "warn": "warn", "bad": "bad"}[lvl], {"good": "f_rssi_ok", "warn": "f_rssi_warn", "bad": "f_rssi_bad"}[lvl], q=q.lower(), r=d.get("rssi"))
        n, got = d.get("n", 0), d.get("got", 0)
        if not d.get("rb"):
            add("warn", "f_norb")
        elif n:
            if got == 0:
                add("bad", "f_rb_none", n=n)
            else:
                loss = d.get("loss", 0)
                if loss > 10:
                    add("bad", "f_loss_bad", l=loss)
                elif loss > 0:
                    add("warn", "f_loss_warn", l=loss)
                else:
                    add("ok", "f_loss_ok", g=got, n=n, a=d.get("avg"))
                if d.get("avg", 0) >= 200:
                    add("bad", "f_lat_bad", a=d.get("avg"))
                elif d.get("avg", 0) >= 80:
                    add("warn", "f_lat_warn", a=d.get("avg"))
                if d.get("jit", 0) > 40:
                    add("bad", "f_jit_bad", j=d.get("jit"))
                elif d.get("jit", 0) >= 10:
                    add("warn", "f_jit_warn", j=d.get("jit"))
        disc = d.get("disc", 0)
        rr = dg.reason_text(d.get("dr"), lang)
        if disc >= 3:
            add("bad", "f_disc_bad", n=disc, r=rr)
        elif disc > 0:
            add("warn", "f_disc_warn", n=disc, r=rr)
        elif up:
            add("ok", "f_disc_ok")
        if d.get("gaps", 0) > 0:
            add("warn", "f_gaps", n=d.get("gaps"))
        if kind == "ESP32" and d.get("minheap"):
            mh = d["minheap"]
            if mh < 10000:
                add("bad", "f_heap_bad", m=mh)
            elif mh < 25000:
                add("warn", "f_heap_warn", m=mh)
    if wifi and wifi.reach:
        for name, vals in wifi.reach.items():
            ok = [v for v in vals if v is not None]
            if not ok:
                add("bad", "f_reach_bad", name=name, ip=wifi.reach_ips.get(name, ""))
            elif len(ok) < len(vals):
                add("warn", "f_reach_warn", name=name, ip=wifi.reach_ips.get(name, ""), a=len(ok), b=len(vals))
    if st_http and st_http["avg"] > 150:
        add("warn", "f_http", a=round(st_http["avg"]))
    if ser["lines"]:
        if ser["crash"]:
            add("bad", "f_crash", n=len(ser["crash"]))
        else:
            add("ok", "f_ser_ok")
        if ser["resets"]:
            add("warn", "f_reboots", n=len(ser["resets"]))
    worst = max((RANK[l] for l, _ in F), default=0)
    attn = sum(1 for l, _ in F if l in ("bad", "warn"))
    return {"lang": lang, "t": t, "date": datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "pc": pc, "ports": ports, "ip": ip, "wifi": wifi,
            "load": load, "d": d, "kind": kind, "fw": fw, "scripts": sc, "ser": ser, "port": port_name, "baud": baud, "hidden": hidden, "hide": hide or [],
            "http": st_http, "pings": pings, "findings": F, "worst": worst, "attn": attn}


# ---------------------------------------------------------------- tables communes aux deux rendus
def _fmt_ms(s):
    return f"{s['min']:.0f} / {s['avg']:.0f} / {s['max']:.0f} ms (±{s['jit']:.0f})" if s else "-"


def _rows(data):
    """Sections -> liste de (titre, [(cle, valeur)]). Le rendu choisit la mise en forme."""
    t, d, load, lang = data["t"], data["d"], data["load"], data["lang"]
    out = []
    kind = data["kind"]
    dev_name = kind or t["nodev"]
    fwlabel = t["pidaemon"] if kind == "Raspberry Pi" else t["firmware"]
    out.append(("sec_ver", [(t["toolkit"], data["pc"].get("toolkit") or t["unknown"]), (fwlabel, f"{data['fw'] or t['unknown']}  ({dev_name})"),
                            (t["scripts"], data["scripts"] or t["notrep"])]))
    if load or d:
        rows = [(t["k_kind"], dev_name)]
        if load:
            if load.get("boot_reason"):
                rows.append((t["k_boot"], load["boot_reason"]))
            if load.get("uptime_s"):
                rows.append((t["k_up"], dg.fmt_ago(int(load["uptime_s"]))))
            if load.get("ota_last"):
                rows.append((t["k_ota"], load["ota_last"]))
            rows.append((t["k_rbip"], load.get("recalbox_active") or load.get("recalbox_ip") or "-"))
        out.append(("sec_dev", rows))
    if d:
        up = bool(d.get("up"))
        lvl, qi = dg.quality(d.get("rssi", -127)) if up else ("bad", 5)
        rows = [(t["k_ssid"], d.get("ssid") or "-"), (t["k_ap"], d.get("bssid") or "-"), (t["k_ch"], d.get("ch") if up else "-"),
                (t["k_rssi"], f"{d.get('rssi')} dBm" if up else "-"), (t["k_q"], dg.QUALITY[lang if lang in dg.QUALITY else "en"][qi]),
                (t["k_ip"], f"{d.get('ip')} / {d.get('gw')}" if up else "-")]
        if d.get("rb") and d.get("n"):
            rows += [(t["k_rbip"], d.get("rb")), (t["k_loss"], f"{d.get('loss')} %  ({d.get('got')}/{d.get('n')})"),
                     (t["k_lat"], f"{d.get('min')} / {d.get('avg')} / {d.get('max')} ms"), (t["k_jit"], f"{d.get('jit')} ms")]
        age = d.get("age", -1)
        rows += [(t["k_age"], "-" if age < 0 else f"{age / 1000:.1f} s"), (t["k_gaps"], d.get("gaps", 0)), (t["k_disc"], d.get("disc", 0))]
        if d.get("disc"):
            rows.append((t["k_last"], f"{dg.reason_text(d.get('dr'), lang)} ({t['ago']} {dg.fmt_ago(d.get('up_s', 0) - d.get('da', 0))})"))
        if data["kind"] != "Raspberry Pi":
            rows.append((t["k_heap"], f"{round(d.get('heap', 0) / 1024)} Ko ({round(d.get('minheap', 0) / 1024)} Ko)"))
        out.append(("sec_wifi", rows))
    w = data["wifi"]
    if w and w.reach:
        rows = []
        for name, vals in w.reach.items():
            ok = [v for v in vals if v is not None]
            rows.append((f"{name} {w.reach_ips.get(name, '')}".strip(), t["answered"].format(a=len(ok), b=len(vals)) + (f", {min(ok):.0f}-{max(ok):.0f} ms" if ok else "")))
        out.append(("sec_reach", rows))
    if w and w.samples:
        s = w.samples
        pings = data["pings"]
        lost = sum(1 for x in pings if x is None)
        out.append(("sec_link", [(t["k_samples"], f"{len(s)} / {s[-1]['t'] - s[0]['t']:.0f} s, {w.errors} err."), (t["k_http"], _fmt_ms(data["http"])),
                                 (t["k_ping"], f"{_fmt_ms(_stat(pings))}, {lost}/{len(pings)} ✗"), (t["k_rssi"], _fmt_ms(_stat([x['rssi'] for x in s])).replace(" ms", " dBm"))]))
    pc = data["pc"]
    out.append(("sec_pc", [(t["k_os"], f"{pc.get('os')} ({pc.get('machine')})"), (t["k_cpu"], pc.get("cpu") or "-"), (t["k_ram"], pc.get("ram")),
                           (t["k_py"], f"{pc.get('python')} / {pc.get('pyserial')}")] + [(t["k_pcwifi"], x) for x in pc.get("wifi", [])]))
    out.append(("sec_usb", [(p["device"], f"{p['description']} [{p['vid_pid']}] {p['chip']}".strip()) for p in data["ports"]] or [("", t["none"])]))
    return out


def _serial_rows(data):
    t, ser = data["t"], data["ser"]
    rows = [(t["k_port"], f"{data['port'] or '-'}" + (f" @ {data['baud']}" if data["baud"] else "")), (t["k_lines"], ser["lines"])]
    if ser["heap"]:
        free, mx, mn = zip(*ser["heap"])
        rows.append((t["k_heapser"], f"free {free[0]} → {free[-1]} (min {min(free)}), block max min {min(mx)}, minFreeHeap {min(mn)}"))
    rows.append((t["k_reboots"], len(ser["resets"])))
    rows.append((t["k_events"], len(ser["events"])))
    rows.append((t["k_crash"], len(ser["crash"])))
    return rows


def _events(data):
    d, t = data["d"], data["t"]
    if not d:
        return []
    out = []
    for e in d.get("ev", []):
        age = dg.fmt_ago(d.get("up_s", 0) - e[1])
        out.append(f"{t['ago']} {age} : " + (f"{t['disc_ev']} - {dg.reason_text(e[2], data['lang'])}" if e[0] == "D" else f"{t['conn']} ({e[2]})"))
    return out


# ---------------------------------------------------------------- rendu texte
def render_text(data) -> str:
    t = data["t"]
    L = []
    bar = "=" * 72
    L += [bar, f"  {t['title'].upper()}", bar, f"  {t['generated']} : {data['date']}", ""]
    verdict = t["allok"] if data["attn"] == 0 else t["attn"].format(n=data["attn"])
    L += [f"{t['summary'].upper()} - {verdict}", "-" * 72]
    for lvl, msg in sorted(data["findings"], key=lambda x: -RANK[x[0]]):
        L.append(f"  {LEVEL_TXT[lvl]:<5} {msg}")
    L.append("")

    def table(rows):
        w = min(26, max((len(str(k)) for k, _ in rows), default=0))
        return [f"  {str(k):<{w}}  {v}" for k, v in rows]
    for key, rows in _rows(data):
        L += [t[key].upper(), "-" * 72] + table(rows) + [""]
        if key == "sec_wifi":
            ev = _events(data)
            d = data["d"]
            if ev:
                L += [t["sec_ev"], *[f"  - {x}" for x in ev], ""]
            if d and d.get("lat"):
                L += [f"  {t['series_lat']} : " + ",".join(str(x) for x in d["lat"]), f"  {t['series_rssi']} : " + ",".join(str(x) for x in d["rs"]), ""]
    if data["ser"]["lines"] or data["port"]:
        wl = str(data["port"]).startswith("Wi-Fi")      # source = suivi Wi-Fi (pas de journal serie brut)
        L += [t["sec_wlog" if wl else "sec_ser"].upper(), "-" * 72] + table(_serial_rows(data))
        if data["hidden"]:
            L.append("  " + t["dropped"].format(n=data["hidden"], pat=", ".join(data["hide"])))
        for c in data["ser"]["crash"]:
            L.append(f"    ! {c}")
        L.append("")
        if data["ser"]["tail"]:
            L += [f"{t['sec_tailw' if wl else 'sec_tail']}" + ("" if wl else f" ({t['tail_note']})"), *[f"    {x[:200]}" for x in data["ser"]["tail"]], ""]
    cfg = dg.config_lines(data["load"])
    if cfg:
        L += [t["sec_cfg"].upper() + f" ({t['cfg_note'].format(n=len(cfg))})", "-" * 72] + [f"  {x}" for x in cfg]
    return "\n".join(L)


# ---------------------------------------------------------------- rendu HTML
_CSS = """
:root{--bg:#f4f6fa;--card:#fff;--fg:#1b2333;--mut:#667085;--line:#e3e8ef;--ok:#18794e;--okbg:#e6f6ee;--warn:#9a6700;--warnbg:#fff4d6;--bad:#b42318;--badbg:#fde8e6;--info:#175cd3;--infobg:#e8f0fe;--code:#0f1720}
@media (prefers-color-scheme:dark){:root{--bg:#10151f;--card:#18202e;--fg:#e6ebf3;--mut:#9aa6b8;--line:#2a3548;--ok:#5fd39b;--okbg:#12301f;--warn:#f5c451;--warnbg:#3a2f10;--bad:#ff8a80;--badbg:#3d1714;--info:#8ab4f8;--infobg:#14284a}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.5 'Segoe UI',Tahoma,sans-serif}
.wrap{max-width:900px;margin:0 auto;padding:20px 16px 60px}
header{display:flex;gap:16px;align-items:center;margin-bottom:14px}header img{width:84px;height:auto}
h1{font-size:22px;margin:0}.sub{color:var(--mut);font-size:13px}
.verdict{border-radius:12px;padding:14px 16px;margin:14px 0;font-weight:600;font-size:16px}
.v0{background:var(--okbg);color:var(--ok)}.v1,.v2{background:var(--warnbg);color:var(--warn)}.v3{background:var(--badbg);color:var(--bad)}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px 16px;margin:12px 0}
h2{font-size:15px;margin:0 0 8px;text-transform:uppercase;letter-spacing:.04em;color:var(--mut)}
table{border-collapse:collapse;width:100%}td{padding:4px 8px;border-top:1px solid var(--line);vertical-align:top}tr:first-child td{border-top:0}
td:first-child{color:var(--mut);width:34%;white-space:nowrap}
ul.f{list-style:none;margin:0;padding:0}ul.f li{padding:6px 10px;border-radius:8px;margin:5px 0;display:flex;gap:10px;align-items:baseline}
.l-bad{background:var(--badbg);color:var(--bad)}.l-warn{background:var(--warnbg);color:var(--warn)}.l-ok{background:var(--okbg);color:var(--ok)}.l-info{background:var(--infobg);color:var(--info)}
.tag{font-weight:700;min-width:44px}
.spark{margin:8px 0 2px}.spark svg{width:100%;height:56px;background:var(--bg);border-radius:8px}.cap{font-size:12px;color:var(--mut)}
details{margin:8px 0}summary{cursor:pointer;color:var(--info);font-weight:600}
pre{background:var(--code);color:#d6deeb;border-radius:8px;padding:10px;overflow:auto;font:12px/1.45 Consolas,monospace;max-height:420px}
.bad{color:var(--bad)}footer{color:var(--mut);font-size:12px;text-align:center;margin-top:24px}
"""


def _svg(values, lo, hi, color="#2e7cf6", bad_marks=False, w=600, h=56):
    vals = list(values)
    if not vals:
        return ""
    n = len(vals)
    dx = w / max(19, n - 1)
    pts, segs, marks = [], [], []
    for i, v in enumerate(vals):
        if v is None:
            if len(pts) > 1:
                segs.append(pts)
            pts = []
            if bad_marks:
                marks.append(f'<rect x="{i * dx - 3:.1f}" y="{h - 12}" width="6" height="10" fill="#d92d20"/>')
            continue
        y = h - 6 - (min(max(v, lo), hi) - lo) / ((hi - lo) or 1) * (h - 12)
        pts.append((i * dx, y))
    if len(pts) > 1:
        segs.append(pts)
    paths = "".join(f'<polyline fill="none" stroke="{color}" stroke-width="2.4" stroke-linejoin="round" points="{" ".join(f"{x:.1f},{y:.1f}" for x, y in s)}"/>' for s in segs)
    dots = "".join(f'<circle cx="{s[0][0]:.1f}" cy="{s[0][1]:.1f}" r="2.4" fill="{color}"/>' for s in segs if len(s) == 1)
    grid = "".join(f'<line x1="0" x2="{w}" y1="{h * f:.0f}" y2="{h * f:.0f}" stroke="#8884" stroke-width="1"/>' for f in (0.25, 0.5, 0.75))
    return f'<svg viewBox="0 0 {w} {h}" preserveAspectRatio="none">{grid}{paths}{dots}{"".join(marks)}</svg>'


def render_html(data) -> str:
    t, e = data["t"], _html.escape
    verdict = t["allok"] if data["attn"] == 0 else t["attn"].format(n=data["attn"])
    P = [f'<!doctype html><html lang="{data["lang"]}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">',
         f'<title>{e(t["title"])}</title><style>{_CSS}</style></head><body><div class="wrap">',
         f'<header><img src="{LOGO_URL}" alt="Shan_ayA"><div><h1>{e(t["title"])}</h1><div class="sub">{e(t["generated"])} {e(data["date"])}</div></div></header>',
         f'<div class="verdict v{data["worst"]}">{e(verdict)}</div>', f'<div class="card"><h2>{e(t["summary"])}</h2><ul class="f">']
    for lvl, msg in sorted(data["findings"], key=lambda x: -RANK[x[0]]):
        P.append(f'<li class="l-{lvl}"><span class="tag">{LEVEL_TXT[lvl]}</span><span>{e(msg)}</span></li>')
    P.append("</ul></div>")
    d = data["d"]
    for key, rows in _rows(data):
        P.append(f'<div class="card"><h2>{e(t[key])}</h2><table>' + "".join(f"<tr><td>{e(str(k))}</td><td>{e(str(v))}</td></tr>" for k, v in rows) + "</table>")
        if key == "sec_wifi" and d:
            if d.get("rs"):
                P.append(f'<div class="spark">{_svg([None if v <= -127 else v for v in d["rs"]], -95, -30)}</div><div class="cap">{e(t["series_rssi"])}</div>')
            if d.get("lat"):
                P.append(f'<div class="spark">{_svg([None if v < 0 else v for v in d["lat"]], 0, max(100, d.get("max", 0)), "#12a37f", True)}</div><div class="cap">{e(t["series_lat"])}</div>')
            ev = _events(data)
            if ev:
                P.append(f'<h2 style="margin-top:12px">{e(t["sec_ev"])}</h2><ul style="margin:0;padding-left:18px">' + "".join(f"<li>{e(x)}</li>" for x in ev) + "</ul>")
        if key == "sec_link" and data["wifi"] and data["wifi"].samples:
            s = data["wifi"].samples
            P.append(f'<div class="spark">{_svg([x["http_ms"] for x in s], 0, max(50, max((x["http_ms"] or 0) for x in s)), "#7a5af8")}</div><div class="cap">{e(t["series_http"])}</div>')
        P.append("</div>")
    if data["ser"]["lines"] or data["port"]:
        wl = str(data["port"]).startswith("Wi-Fi")
        P.append(f'<div class="card"><h2>{e(t["sec_wlog" if wl else "sec_ser"])}</h2><table>' + "".join(f"<tr><td>{e(str(k))}</td><td>{e(str(v))}</td></tr>" for k, v in _serial_rows(data)) + "</table>")
        if data["hidden"]:
            P.append(f'<div class="cap">{e(t["dropped"].format(n=data["hidden"], pat=", ".join(data["hide"])))}</div>')
        if data["ser"]["crash"]:
            P.append('<details open><summary class="bad">' + e(t["k_crash"]) + f' ({len(data["ser"]["crash"])})</summary><pre>' + e("\n".join(data["ser"]["crash"])) + "</pre></details>")
        if data["ser"]["tail"]:
            P.append(f'<details{" open" if wl else ""}><summary>{e(t["sec_tailw" if wl else "sec_tail"])}</summary>' + ("" if wl else f'<div class="cap">{e(t["tail_note"])}</div>') + '<pre>' + e("\n".join(x[:200] for x in data["ser"]["tail"])) + "</pre></details>")
        P.append("</div>")
    cfg = dg.config_lines(data["load"])
    if cfg:
        P.append(f'<div class="card"><details><summary>{e(t["sec_cfg"])} ({e(t["cfg_note"].format(n=len(cfg)))})</summary><pre>' + e("\n".join(cfg)) + "</pre></details></div>")
    P.append('<footer>RecalBoxDMD - Shan_ayA</footer></div></body></html>')
    return "".join(P)
