# -*- coding: utf-8 -*-
# RecalBoxDMD_alpha_tests.py -- fenetre « Tests alpha » du toolkit ALPHA : le plan de test du Discord, a remplir depuis le toolkit.
#
# v1 - 2026-11-25 - safe-modify - Creation. Logique pure (sans Tk) : lecture du plan de tests publie par le projet (docs/alpha-tests.json, servi par les pages du
#   projet, genere depuis le bot Discord), texte d'un test dans la langue de l'utilisateur, et BLOC DE RESULTAT « ALPHA-RESULT test=T05 result=ok device=pi » place
#   en tete du message libre d'un rapport kind=test (voir RecalBoxDMD_discord.build_message). Le bot l'enregistre comme le formulaire du Discord, avec les versions
#   REELLEMENT detectees par le toolkit (le test compte alors pour « Pret stable »). La fenetre est dans RecalBoxDMD_diag_gui.py (bloc Discord, build alpha seulement).
import json
import os
import time
import urllib.request

URL = os.environ.get("DMD_ALPHA_TESTS") or "https://shan-aya.github.io/RecalBoxDMD/alpha-tests.json"
RESULTS = ("ok", "ko", "partial", "na")
DEVICES = ("", "esp32", "pi", "toolkit", "other")
_CACHE = {"ts": 0.0, "data": None}


class AlphaTestsError(Exception):
    pass


def load_tests(timeout: float = 8.0, force: bool = False) -> dict:
    """Plan de tests {"targets": ..., "tests": [...]}. Garde en cache 10 minutes. Leve AlphaTestsError si injoignable ou invalide."""
    if not force and _CACHE["data"] is not None and time.time() - _CACHE["ts"] < 600:
        return _CACHE["data"]
    try:
        req = urllib.request.Request(URL, headers={"User-Agent": "recalbox-toolkit"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            data = json.loads(r.read().decode("utf-8-sig"))
    except Exception as e:   # noqa: BLE001
        raise AlphaTestsError(f"{type(e).__name__}: {e}")
    tests = data.get("tests") if isinstance(data, dict) else None
    if not isinstance(tests, list) or not tests or not all(isinstance(t, dict) and t.get("id") for t in tests):
        raise AlphaTestsError("plan de tests invalide")
    _CACHE.update(ts=time.time(), data=data)
    return data


def _tx(obj, lang: str) -> str:
    if isinstance(obj, dict):
        return obj.get(lang) or obj.get("en") or obj.get("fr") or ""
    return str(obj or "")


def title_of(t: dict, lang: str) -> str:
    return f"{t['id']} — {_tx(t.get('title'), lang)}"


def test_text(t: dict, lang: str) -> str:
    """Etapes et resultat attendu d'un test, en texte simple (sans gras Markdown)."""
    steps = (t.get("steps") or {})
    lst = steps.get(lang) or steps.get("en") or steps.get("fr") or []
    head = {"fr": ("Étapes", "Résultat attendu"), "es": ("Pasos", "Resultado esperado")}.get(lang, ("Steps", "Expected result"))
    warn = _tx(t.get("warn"), lang)
    out = ([f"⚠ {warn}", ""] if warn else []) + [head[0] + " :"] + [f"{i}. {s}" for i, s in enumerate(lst, 1)] + ["", f"{head[1]} : {_tx(t.get('expect'), lang)}"]
    return "\n".join(out).replace("**", "").replace("*", "")


def result_block(test_id: str, result: str, device: str = "", comment: str = "") -> str:
    """Bloc a mettre dans le message libre d'un rapport kind=test."""
    if result not in RESULTS + ("withdraw",):
        raise ValueError("resultat invalide")
    tid = "".join(c for c in str(test_id).upper() if c.isalnum())[:8]
    if not tid:
        raise ValueError("test invalide")
    dev = device if device in DEVICES else ""
    head = f"ALPHA-RESULT test={tid} result={result}" + (f" device={dev}" if dev else "")
    return head + ("\n" + comment.strip()[:900] if comment and comment.strip() else "")

AREA_ORDER = ("ESP32", "PI", "TOOL", "ALL")
AREA_LABELS = {"ESP32": {"fr": "ESP32", "en": "ESP32", "es": "ESP32"}, "PI": {"fr": "Raspberry Pi", "en": "Raspberry Pi", "es": "Raspberry Pi"},
               "TOOL": {"fr": "Toolkit", "en": "Toolkit", "es": "Toolkit"}, "ALL": {"fr": "Général", "en": "General", "es": "General"}}
SORTS = ("area", "id", "title")


def area_label(area: str, lang: str) -> str:
    return (AREA_LABELS.get(area) or {}).get(lang) or (AREA_LABELS.get(area) or {}).get("en") or str(area)


def sort_tests(tests: list, mode: str, lang: str) -> list:
    """Tri de la liste : « area » (zone puis numero), « id » (numero), « title » (ordre alphabetique du titre)."""
    def zone(t):
        a = t.get("area")
        return AREA_ORDER.index(a) if a in AREA_ORDER else len(AREA_ORDER)
    if mode == "title":
        return sorted(tests, key=lambda t: _tx(t.get("title"), lang).lower())
    if mode == "id":
        return sorted(tests, key=lambda t: t["id"])
    return sorted(tests, key=lambda t: (zone(t), t["id"]))


def groups(tests: list, lang: str) -> list:
    """[(libelle de la zone, [tests])] dans l'ordre des zones."""
    out = {}
    for t in sort_tests(tests, "area", lang):
        out.setdefault(t.get("area"), []).append(t)
    return [(area_label(a, lang), lst) for a, lst in out.items()]

# ---------------------------------------------------------------- memoire locale des envois (ce que l'utilisateur a deja fait / ce qu'il reste a faire)
RELEVANT = {"ESP32": ("esp32_fw",), "PI": ("pi",), "TOOL": ("toolkit",), "ALL": ("esp32_fw", "pi", "toolkit")}
ICONS = {"ok": "✅", "ko": "❌", "partial": "🟠", "na": "➖"}


def vkey(area: str, versions: dict | None) -> str:
    """Empreinte des versions qui comptent pour ce test (firmware ESP32, demon Pi, toolkit selon la zone) : un resultat n'est valable que pour elles."""
    if not versions:
        return ""
    return "|".join(f"{k}={versions.get(k, '')}" for k in RELEVANT.get(area, RELEVANT["ALL"]))


def status_of(entry: dict | None, area: str, versions: dict | None) -> tuple[str, str]:
    """(etat, icone) : « todo » (pas fait), « done » (fait sur les versions actuelles), « old » (fait sur d'anciennes versions : a refaire)."""
    if not entry:
        return "todo", "⚪"
    cur = vkey(area, versions)
    if cur and entry.get("vkey") != cur:
        return "old", "⚠"
    return "done", ICONS.get(entry.get("result"), "✅")


def remember(mem: dict, test_id: str, result: str, area: str, versions: dict | None, ts: float | None = None) -> dict:
    mem[test_id] = {"result": result, "vkey": vkey(area, versions), "ts": ts if ts is not None else time.time()}
    return mem


def forget(mem: dict, test_id: str) -> dict:
    mem.pop(test_id, None)
    return mem


def progress(tests: list, mem: dict, versions: dict | None) -> tuple[int, int]:
    """(tests faits sur les versions actuelles, total)."""
    return sum(1 for t in tests if status_of(mem.get(t["id"]), t.get("area"), versions)[0] == "done"), len(tests)


def load_memory(raw) -> dict:
    try:
        d = json.loads(raw) if raw else {}
        return d if isinstance(d, dict) else {}
    except (TypeError, ValueError):
        return {}


def dump_memory(mem: dict) -> str:
    return json.dumps(mem, ensure_ascii=False)
