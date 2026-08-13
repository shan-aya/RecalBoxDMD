# Building RecalBoxDMD Toolkit for Windows

Three ways to get the toolkit running on a Windows PC, from simplest to most
"official installer":

## 1. `install_and_run.bat` (run from Python source, no packaging)

Double-click `install_and_run.bat`. It checks for Python 3 (installs it via
`winget` if missing), installs the two required libraries (`Pillow`,
`Markdown`) if missing, then launches `run_gui.py`. No admin rights needed
beyond what `winget`/`pip` require.

## 2. Standalone `.exe` (PyInstaller, windowless, no install)

```
pyinstaller --noconfirm --clean RecalBoxDMD_GUI.spec
```

Produces `dist\RecalBoxDMD_GUI.exe` — a single portable file bundling Python,
Tkinter, Pillow, Markdown, the `themes/` folder and `assets/` (logo, scrape
help screenshots, default image gallery). `console=False` in the `.spec`
means no terminal window ever appears.

**⚠️ Build with Python 3.11 or 3.12, not 3.13+.** Newer CPython builds ship
Tcl/Tk 9.0 with its library data zipped (`libtcl9.0.4.zip`) instead of the
classic loose `tcl8.6/` folder; as of PyInstaller 6.x the Tk runtime hook
doesn't locate that yet and the frozen exe crashes on launch with
`FileNotFoundError: Tcl data directory ... not found`. Building the exact
same `.spec` with a 3.12 interpreter (`py -3.12 -m PyInstaller ...`) works
with no changes.

Both `Pillow` and `Markdown` must be installed in the interpreter you build
with (`pip install Pillow Markdown`) — `Markdown` in particular is easy to
miss since it's only imported by `RecalBoxDMD_md_renderer.py` (renders the
HELP tab / README).

## 3. Windows installer — `.exe` setup (Inno Setup) or `.msi` (cx_Freeze)

**Setup.exe (recommended — Start Menu/Desktop shortcuts, uninstaller):**
build the standalone exe first (step 2), then:
```
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" RecalBoxDMD_Setup.iss
```
→ `dist_installer\RecalBoxDMD_Toolkit_Setup.exe`. Script: `RecalBoxDMD_Setup.iss`
(installs to Program Files, FR/EN/ES setup wizard, optional desktop icon,
proper uninstaller registered in Windows).

**.msi (cx_Freeze — for environments that require a real MSI, e.g. group
policy deployment):**
```
pip install cx_Freeze
python setup_msi.py bdist_msi --dist-dir dist_msi
```
→ `dist_msi\RecalBoxDMD Toolkit-1.0.0-win64.msi`. Builds its own frozen exe
independently of PyInstaller (script: `setup_msi.py`), same bundled
assets/themes. Also build this one with Python 3.11/3.12 for the same
Tcl/Tk reason as above.

Both installers were smoke-tested with a silent install → launch → silent
uninstall cycle (`/VERYSILENT` for the Inno setup, `msiexec /qn` for the
MSI) — clean in both directions, nothing left behind.

## App icon

`assets/recalboxdmd_icon.ico` — generated from `assets/recalbox_logo_menu.png`
(cropped to the controller+box mark). Regenerate from a different source
image if you want a different icon; both the `.spec` and `RecalBoxDMD_Setup.iss`
reference this exact path.
