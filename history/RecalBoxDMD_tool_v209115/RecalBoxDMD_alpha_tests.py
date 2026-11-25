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
    if result not in RESULTS:
        raise ValueError("resultat invalide")
    tid = "".join(c for c in str(test_id).upper() if c.isalnum())[:8]
    if not tid:
        raise ValueError("test invalide")
    dev = device if device in DEVICES else ""
    head = f"ALPHA-RESULT test={tid} result={result}" + (f" device={dev}" if dev else "")
    return head + ("\n" + comment.strip()[:900] if comment and comment.strip() else "")
