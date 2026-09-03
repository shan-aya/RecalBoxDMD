# -*- mode: python ; coding: utf-8 -*-

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
# -- ce worktree avait divergé avant l'ajout icone/DPI-manifest sur master ;
# amelioration d'outillage pure, pas un changement de contenu, applique ici
# pour que ce build suive le meme format que les precedents.
with open("dpi_aware.manifest", "r", encoding="utf-8") as _f:
    _DPI_AWARE_MANIFEST = _f.read()

a = Analysis(
    ['run_gui.py'],
    pathex=[],
    binaries=[],
    datas=[('themes', 'themes'), ('assets', 'assets'), ('README.md', '.'), ('README.fr.md', '.'), ('README.es.md', '.')],
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
