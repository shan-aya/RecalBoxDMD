#!/usr/bin/env python3
"""Point d'entrée pour la compilation PyInstaller."""

import sys
from pathlib import Path

# Ajouter le répertoire tools/ au path
_tools_dir = Path(__file__).resolve().parent
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

import RecalBoxDMD_tool as toolkit
import RecalBoxDMD_GUI
import RecalBoxDMD_themes
import RecalBoxDMD_md_renderer
import RecalBoxDMD_prefs

sd = toolkit.get_sd_card_dir(_tools_dir)
RecalBoxDMD_GUI.RetroBoxLEDGui(toolkit, sd).run()
