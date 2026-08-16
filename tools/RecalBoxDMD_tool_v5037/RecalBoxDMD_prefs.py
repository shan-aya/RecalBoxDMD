# ============================================
# safe-modify - Historique des modifications
# ============================================
# Version actuelle : v8
#
# v8 - 2026-08-15 - safe-modify - Ajout de la preference "systems_image_lang"
#      (langue des images systemes/genres telechargees dans _defaults/ depuis
#      GitHub -- "en"/"fr"/"es", distincte de "language" qui est la langue de
#      l'interface de l'outil). Choisie via un nouveau dialogue en Mode 1 et
#      dans le Mode 2 de l'onglet Avance, voir download_defaults(lang=...)
#      dans RecalBoxDMD_tool.py.
# v7 - 2026-08-11 - safe-modify - Ajout de la preference "slow_threshold"
#      (seuil de nombre de fichiers .raw565/.raw565pack/.meta au-dela duquel
#      un systeme recoit le flag "L" (lent) dans systems_cache.dat, voir
#      build_systems_cache() dans RecalBoxDMD_tool.py) -- rendu reglable
#      dans l'onglet Parametres de la GUI, la vitesse reelle d'une carte SD
#      variant d'un utilisateur a l'autre. Valeur par defaut "5000" alignee
#      sur le seuil code en dur jusqu'ici (v32 de RecalBoxDMD_tool.py).
# v6 - 2026-07-21 - safe-modify - Ajout de la preference "recalbox_ip" (nom
#      reseau ou IP de la Recalbox cible pour l'installation des scripts
#      userscripts, partagee entre le Mode 1 automatique et le Mode 9 dedie)
# v5 - 2026-07-13 - safe-modify - Valeur par defaut de "language" changee
#      "fr" -> "en" : au premier lancement (aucun RecalBoxDMD_prefs.json,
#      donc aucune preference sauvegardee), l'application demarre desormais
#      en anglais plutot qu'en francais.
# v4 - 2026-07-11 - safe-modify - Ajout de la preference
#      "default_fallback_image" (chemin vers l'image de secours
#      personnalisee choisie par l'utilisateur pour default.raw565 ; chaine
#      vide = garder le visuel par defaut du projet)
# v3 - 2026-07-11 - safe-modify - Valeur par defaut de "recalbox_profile"
#      renommee "10.1" -> "10.x" (coherent avec RECALBOX_PROFILES)
# v2 - 2026-07-10 - safe-modify - Ajout de la preference "recalbox_profile"
#      (version Recalbox choisie en Mode 1 : 10.x/9.x/legacy), persistee
#      comme le theme et la langue
# v1 - Version de base
# ============================================

#!/usr/bin/env python3
"""Préférences utilisateur — fichier JSON unique à côté de l'EXE.

Centralise la sauvegarde du thème, de la langue et toute future préférence.
"""

import sys
import json
from pathlib import Path

# Déterminer le dossier de base (à côté de l'EXE ou du module en dev)
if getattr(sys, "frozen", False):
    _BASE_DIR = Path(sys.executable).parent
else:
    _BASE_DIR = Path(__file__).parent

PREFS_FILE = _BASE_DIR / "RecalBoxDMD_prefs.json"

# Valeurs par défaut
_DEFAULTS = {
    "theme": "random",  # "random" ou nom d'un thème
    "language": "en",  # "fr", "en", "es" -- EN si aucune preference sauvegardee
    "recalbox_profile": "10.x",  # "10.x", "9.x", "legacy" (Mode 1)
    "default_fallback_image": "",  # chemin PNG source pour default.raw565 ("" = defaut projet)
    "recalbox_ip": "",  # nom reseau ou IP Recalbox pour l'installation des scripts (Mode 1 + Mode 9)
    "slow_threshold": "5000",  # seuil flag L (build_systems_cache) -- reglable, onglet Parametres
    "systems_image_lang": "en",  # "en"/"fr"/"es" -- langue des images systems/_defaults (Mode 1 + Mode 2)
}


def _load_raw() -> dict:
    """Charge le fichier JSON brut, ou retourne un dict vide."""
    try:
        if PREFS_FILE.exists():
            data = json.loads(PREFS_FILE.read_text(encoding="utf-8"))
            if isinstance(data, dict):
                return data
    except Exception:
        pass
    return {}


def _save_raw(data: dict) -> None:
    """Écrit le dictionnaire dans le fichier JSON."""
    try:
        PREFS_FILE.write_text(
            json.dumps(data, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )
    except Exception:
        pass


# ── API publique ──────────────────────────────────────────────────────────────


def get(key: str) -> str | None:
    """Retourne la valeur d'une préférence, ou la valeur par défaut, ou None."""
    data = _load_raw()
    return data.get(key, _DEFAULTS.get(key))


def set(key: str, value: str) -> None:
    """Définit une préférence et la sauvegarde immédiatement."""
    data = _load_raw()
    data[key] = value
    _save_raw(data)


def get_all() -> dict:
    """Retourne toutes les préférences (fusionnées avec les défauts)."""
    data = _load_raw()
    merged = dict(_DEFAULTS)
    merged.update(data)
    return merged
