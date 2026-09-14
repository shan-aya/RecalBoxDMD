"""
Build a Windows .msi installer for RecalBoxDMD Toolkit using cx_Freeze.
Usage (Python 3.12 recommended -- see build notes in RecalBoxDMD_GUI.spec):
    python setup_msi.py bdist_msi
Produces dist_msi\\RecalBoxDMD Toolkit-1.0.0-win64.msi

2026-09-14 -- base="Win32GUI" (ancienne convention de nommage cx_Freeze)
remplace par base="gui" : cx_Freeze 8.7.0 (installe pour ce build,
compatible Python 3.14) a renomme ses bases -- "Win32GUI" leve desormais
DistutilsOptionError ("no base named 'legacy/win32gui' -- Did you mean
'gui'?"). Compatible avec toutes les futures versions 8.x de cx_Freeze.
"""
from cx_Freeze import setup, Executable
import sys

build_exe_options = {
    "packages": ["tkinter", "PIL", "markdown"],
    "includes": [
        "RecalBoxDMD_prefs",
        "RecalBoxDMD_themes",
        "RecalBoxDMD_md_renderer",
        "RecalBoxDMD_tool",
    ],
    "include_files": [
        ("themes", "themes"),
        ("assets", "assets"),
        ("README.md", "README.md"),
        ("README.fr.md", "README.fr.md"),
        ("README.es.md", "README.es.md"),
    ],
    "excludes": ["unittest", "test", "distutils"],
    "build_exe": "build_msi_exe",
}

bdist_msi_options = {
    "upgrade_code": "{6E9F6E7A-7C7B-4F5E-9B0D-3A2C6E8F1D45}",
    "add_to_path": False,
    "initial_target_dir": r"[ProgramFilesFolder]\RecalBoxDMD Toolkit",
    "summary_data": {
        "author": "Shan_ayA",
        "comments": "RecalBoxDMD Toolkit - Raw565 Edition PC Toolkit",
    },
}

base = "gui" if sys.platform == "win32" else None

executables = [
    Executable(
        "run_gui.py",
        base=base,
        target_name="RecalBoxDMD_GUI.exe",
        icon="assets/recalboxdmd_icon.ico",
        shortcut_name="RecalBoxDMD Toolkit",
        shortcut_dir="DesktopFolder",
        # dpi_aware.manifest : source unique partagee avec
        # RecalBoxDMD_GUI.spec/PyInstaller -- meme bug/meme fix (voir
        # commentaire dans le .spec), cx_Freeze a le meme souci de manifest
        # par defaut sans conscience DPI.
        manifest="dpi_aware.manifest",
    )
]

setup(
    name="RecalBoxDMD Toolkit",
    version="1.0.0",
    description="RecalBoxDMD Toolkit - Raw565 Edition PC Toolkit",
    options={"build_exe": build_exe_options, "bdist_msi": bdist_msi_options},
    executables=executables,
)
