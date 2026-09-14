# -*- mode: python ; coding: utf-8 -*-

import glob
import os
import sys
import tempfile
import zipfile

# Manifest Windows personnalise (dpi_aware.manifest, source unique partagee
# avec setup_msi.py/cx_Freeze -- meme fichier, ne pas dupliquer) -- bug
# utilisateur : le fix DPI-awareness runtime (SetProcessDpiAwareness() dans
# RecalBoxDMD_GUI.py v52) marche en lancant le .py mais pas le .exe
# compile. Root cause : le manifest par defaut de PyInstaller
# (PyInstaller.utils.win32.winmanifest, _DEFAULT_MANIFEST_XML) ne declare
# AUCUNE conscience DPI, contrairement au manifest deja embarque nativement
# dans python.exe (python.org) qui la declare -- d'ou l'appel runtime
# insuffisant/trop tard uniquement dans l'exe compile (le manifest est
# applique par le loader Windows AVANT le moindre code Python, alors qu'un
# appel runtime peut arriver trop tard selon ce qui s'est deja passe au
# demarrage du process). Confirme en verifiant en direct la vraie
# conscience DPI du process via GetProcessDpiAwareness() (shcore) : 0
# (Unaware) sans ce fix, 2 (PerMonitorAware) avec.
# Porte depuis master dans ce worktree (dev-core-reassignment) le 2026-09-03
# -- ce worktree avait divergÃ© avant l'ajout icone/DPI-manifest sur master ;
# amelioration d'outillage pure, pas un changement de contenu, applique ici
# pour que ce build suive le meme format que les precedents.
with open("dpi_aware.manifest", "r", encoding="utf-8") as _f:
    _DPI_AWARE_MANIFEST = _f.read()

# BUG REEL corrige (2026-09-14, retour utilisateur en direct : crash
# immediat du .exe portable v6300 telecharge sur GitHub --
# "FileNotFoundError: Tcl data directory ..._tcl_data not found").
# Reproduit en local avec le meme binaire, donc pas specifique a la machine
# de l'utilisateur -- touche TOUT LE MONDE qui telecharge ce build.
#
# Cause racine : Python 3.14 (installateur officiel python.org) embarque
# desormais Tcl/Tk 9.0 via un mecanisme zipimport -- `tkinter.Tcl().eval
# ("info library")` renvoie "//zipfs:/lib/tcl/tcl_library", un chemin
# VIRTUEL (a l'interieur d'un .zip mappe par l'import Python), pas un vrai
# dossier sur le disque. PyInstaller 6.20.0
# (PyInstaller.utils.hooks.tcl_tk.TclTkInfo._load_tcl_tk_info()) teste
# `os.path.isdir()` sur ce chemin pour decider quels fichiers Tcl/Tk
# embarquer -- echoue silencieusement (seulement un WARNING au moment du
# build, jamais remonte en erreur), et ne bundle ALORS AUCUN fichier
# Tcl/Tk : l'exe compile plante au tout premier lancement, systematiquement,
# chez tout le monde. Issue connue et deja fermee "not planned"/"not our
# bug" cote PyInstaller (#8856, github.com/pyinstaller/pyinstaller) --
# PAS un correctif a attendre d'une mise a jour de l'outil, meme la version
# la plus recente (6.22.3 au moment de ce fix, nous sommes en 6.20.0) ne
# le corrige pas.
#
# Fix : extraire nous-memes les scripts Tcl/Tk -- livres tels quels en
# .zip par l'installateur officiel Python a cote de l'installation
# (<python>/tcl/libtcl9.0.4.zip et libtk9.0.4.zip, verifie present sur ce
# poste) -- et les ajouter explicitement a `datas` sous les noms EXACTS
# attendus par le hook d'execution de PyInstaller au demarrage de l'exe
# (`_tcl_data`/`_tk_data`, cf. PyInstaller.utils.hooks.tcl_tk.TclTkInfo.
# TCL_ROOTNAME/TK_ROOTNAME et hooks/rthooks/pyi_rth__tkinter.py qui pose
# TCL_LIBRARY/TK_LIBRARY sur ces 2 dossiers). Le contenu du zip a la
# structure attendue directement (tcl_library/init.tcl, tk_library/...).
#
# Ce contournement ne se declenche QUE si la detection normale de
# PyInstaller a deja echoue (meme test qu'elle : os.path.isdir() sur
# "info library") -- si une future version de Python/PyInstaller corrige
# ca nativement, ce bloc devient un no-op silencieux plutot que de risquer
# un doublon/conflit.
_extra_tcltk_datas = []
try:
    import tkinter as _tkinter_probe

    _tcl_data_dir = _tkinter_probe.Tcl().eval("info library")
    if not os.path.isdir(_tcl_data_dir):
        print("AVERTISSEMENT build : Tcl/Tk de ce Python n'est pas un vrai "
              "dossier disque (%s) -- contournement zipimport active "
              "(voir commentaire dans ce .spec)." % _tcl_data_dir)
        _tcl_tk_root = os.path.join(sys.base_prefix, "tcl")
        _tcl_zip = glob.glob(os.path.join(_tcl_tk_root, "libtcl*.zip"))
        _tk_zip = glob.glob(os.path.join(_tcl_tk_root, "libtk*.zip"))
        if _tcl_zip and _tk_zip:
            _extract_dir = os.path.join(
                tempfile.gettempdir(), "recalboxdmd_tcltk_extract"
            )
            with zipfile.ZipFile(_tcl_zip[0]) as _zf:
                _zf.extractall(os.path.join(_extract_dir, "tcl"))
            with zipfile.ZipFile(_tk_zip[0]) as _zf:
                _zf.extractall(os.path.join(_extract_dir, "tk"))
            _extra_tcltk_datas = [
                (os.path.join(_extract_dir, "tcl", "tcl_library"), "_tcl_data"),
                (os.path.join(_extract_dir, "tk", "tk_library"), "_tk_data"),
            ]
            print("Contournement Tcl/Tk : %d dossiers ajoutes a datas."
                  % len(_extra_tcltk_datas))
        else:
            print("ERREUR build : contournement Tcl/Tk zipimport IMPOSSIBLE -- "
                  "libtcl*.zip/libtk*.zip introuvables sous " + _tcl_tk_root
                  + " -- l'exe genere plantera au lancement (Tcl data "
                    "directory not found).")
except Exception as _e:
    print("ERREUR build : sonde Tcl/Tk (contournement zipimport) echouee : "
          + str(_e) + " -- l'exe genere risque de planter au lancement.")

a = Analysis(
    ['run_gui.py'],
    pathex=[],
    binaries=[],
    datas=[('themes', 'themes'), ('assets', 'assets'), ('HELP.md', '.'), ('HELP.fr.md', '.'), ('HELP.es.md', '.')] + _extra_tcltk_datas,
    hiddenimports=['RecalBoxDMD_prefs', 'RecalBoxDMD_themes', 'RecalBoxDMD_md_renderer', 'RecalBoxDMD_tool'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='RecalBoxDMD_GUI',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=['assets\\recalboxdmd_icon.ico'],
    manifest=_DPI_AWARE_MANIFEST,
)
