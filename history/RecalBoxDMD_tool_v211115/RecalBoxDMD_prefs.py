# ============================================
# safe-modify - Historique des modifications
# ============================================
# Version actuelle : v11
#
# v11 - 2026-10-26 - safe-modify - Version INSTALLEE (Setup -> Program Files) :
#      le fichier de preferences etait cherche a cote de l'exe, dossier non
#      modifiable par un utilisateur normal ; _save_raw() avalait l'erreur,
#      donc AUCUNE preference (langue, theme, IP Recalbox, image de secours)
#      n'etait jamais enregistree. Maintenant : fichier a cote de l'exe s'il
#      existe et reste modifiable, ou si le dossier est modifiable (portable,
#      comportement inchange) ; sinon %APPDATA%\RecalBoxDMD\ (reprise du
#      contenu de l'ancien fichier en lecture seule). Un echec d'ecriture est
#      affiche (stderr) au lieu d'etre ignore.
#
# v10 - 2026-09-06 - safe-modify - Demande utilisateur : la langue par
#      defaut de l'appli doit etre celle du systeme Windows au tout
#      premier lancement, sauf si une preference a deja ete explicitement
#      validee dans l'onglet Parametres -- get() seul ne permet pas de
#      distinguer "jamais choisi" de "valeur de repli _DEFAULTS", d'ou la
#      nouvelle fonction has_saved(). Voir RecalBoxDMD_GUI.py v62 pour la
#      detection de langue systeme elle-meme (cote appelant).
#
# v9 - 2026-09-05 - safe-modify - Fusion master -> dev/core-reassignment :
#      combine deux ajouts independants faits en parallele sous le meme
#      label "v8" (voir entree v8 ci-dessous, 2026-08-15) -- (a) valeur par
#      defaut de "slow_threshold" ramenee de 5000 a 800 (retour utilisateur,
#      chantier "bucket" -- voir DECISIONS.md RecalBox_DMD) : le seuil avait
#      ete remonte a 5000 (v32 RecalBoxDMD_tool.py) uniquement pour eviter
#      que le comptage PAR SYSTEME ENTIER (avant le bucket alphabetique)
#      marque a tort de nombreux petits systemes comme "lents" a cause d'un
#      seul gros sous-dossier. Avec le flag L desormais calcule PAR BUCKET
#      alphabetique (A..Z/#), ce probleme n'existe plus par construction --
#      800 (seuil d'origine, pre-v31) redevient pertinent PAR BUCKET,
#      changement fait le 2026-08-23 sur dev/core-reassignment ; (b)
#      preference "systems_image_lang" (voir entree v8 ci-dessous), ajoutee
#      independamment sur master, absente jusqu'ici de ce worktree dev.
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

_PREFS_NAME = "RecalBoxDMD_prefs.json"


def _dir_writable(folder: Path) -> bool:
    """Vrai si on peut creer un fichier dans `folder` (Program Files : faux pour un utilisateur normal)."""
    import tempfile

    try:
        with tempfile.TemporaryFile(dir=str(folder)):
            pass
        return True
    except Exception:
        return False


def _resolve_prefs_file() -> Path:
    """Emplacement du fichier de preferences : a cote de l'exe si possible (portable), sinon %APPDATA%\\RecalBoxDMD."""
    import os

    local = _BASE_DIR / _PREFS_NAME
    appdata = Path(os.environ.get("APPDATA") or Path.home()) / "RecalBoxDMD" / _PREFS_NAME
    if local.exists() and os.access(str(local), os.W_OK):
        return local
    if not local.exists() and not appdata.exists() and _dir_writable(_BASE_DIR):
        return local
    # Dossier de l'exe non modifiable (version installee) : dossier utilisateur ; on reprend l'ancien fichier s'il existe
    try:
        appdata.parent.mkdir(parents=True, exist_ok=True)
        if local.exists() and not appdata.exists():
            appdata.write_bytes(local.read_bytes())
    except Exception as e:
        print(f"[prefs] dossier des preferences inutilisable ({appdata.parent}) : {e}", file=sys.stderr)
    return appdata


PREFS_FILE = _resolve_prefs_file()

# Valeurs par défaut
_DEFAULTS = {
    "theme": "random",  # "random" ou nom d'un thème
    "language": "en",  # "fr", "en", "es" -- EN si aucune preference sauvegardee
    "recalbox_profile": "11.x-10.x",  # "11.x-10.x", "9.x", "legacy" (Mode 1) ; ancienne valeur "10.x" migree par le GUI
    "default_fallback_image": "",  # chemin PNG source pour default.raw565 ("" = defaut projet)
    "recalbox_ip": "",  # nom reseau ou IP Recalbox pour l'installation des scripts (Mode 1 + Mode 9)
    "slow_threshold": "800",  # seuil flag L PAR BUCKET (build_systems_cache) -- reglable, onglet Parametres (v9: 5000->800, voir changelog)
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
    except Exception as e:
        print(f"[prefs] ecriture impossible ({PREFS_FILE}) : {e}", file=sys.stderr)


# ── API publique ──────────────────────────────────────────────────────────────


def get(key: str) -> str | None:
    """Retourne la valeur d'une préférence, ou la valeur par défaut, ou None."""
    data = _load_raw()
    return data.get(key, _DEFAULTS.get(key))


def has_saved(key: str) -> bool:
    """Vrai si `key` a ete EXPLICITEMENT enregistree dans le fichier JSON
    (par un appel a set()), par opposition a une valeur de repli tiree de
    _DEFAULTS -- get() ne permet pas de faire cette distinction, il
    renvoie toujours quelque chose des qu'une valeur par defaut existe.
    Utilise par RecalBoxDMD_GUI.py (v10) pour ne detecter la langue
    systeme Windows qu'au tout premier lancement (aucun choix jamais
    valide dans l'onglet Parametres) et ne plus jamais y toucher ensuite."""
    return key in _load_raw()


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
