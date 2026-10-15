#!/usr/bin/env python3
"""Point d'entrée pour la compilation PyInstaller."""

import sys
from pathlib import Path

# Ajouter le répertoire tools/ au path
_tools_dir = Path(__file__).resolve().parent
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

import RecalBoxDMD_tool as toolkit


def _selftest(out_path):
    """Auto-test de l'exe gele (build 9860+) : lance avec RECALBOXDMD_SELFTEST=<fichier> il verifie les modules des themes
    (rendu SVG resvg_py, numpy, Pillow, conversion en raw565) puis quitte SANS ouvrir la fenetre. Ecrit le resultat dans <fichier>."""
    import io
    import traceback
    lines = ["frozen=%s" % bool(getattr(sys, "frozen", False))]
    ok = True
    try:
        import numpy
        import PIL
        import resvg_py
        import theme_logos as tl
        import RecalBoxDMD_theme_logos_gui as tlg   # noqa: F401  (module de la fenetre des themes)
        lines.append("imports ok : numpy %s, Pillow %s, resvg_py, theme_logos, fenetre des themes" % (numpy.__version__, PIL.__version__))
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="400" height="100" viewBox="0 0 400 100">'
               '<rect x="10" y="10" width="380" height="80" fill="#ffffff"/></svg>')
        data = tl.convert_bytes(svg.encode("utf-8"), True)
        lit = sum(1 for i in range(0, len(data), 2) if data[i] or data[i + 1])
        lines.append("convert_bytes(SVG) -> %d octets, %d pixels allumes" % (len(data), lit))
        ok = len(data) == 8192 and lit > 500
        # prefer_keys / detect_variants (fonctions utilisees par la fenetre) : simple verification d'appel
        lines.append("prefer_keys(fr_FR, eu) = %s" % (tl.prefer_keys("fr_FR", "eu"),))
    except Exception:
        ok = False
        lines.append(traceback.format_exc())
    lines.append("RESULTAT : " + ("OK" if ok else "ECHEC"))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    return 0 if ok else 1

import RecalBoxDMD_GUI
import RecalBoxDMD_themes
import RecalBoxDMD_md_renderer
import RecalBoxDMD_prefs

import os

if os.environ.get("RECALBOXDMD_SELFTEST"):
    sys.exit(_selftest(os.environ["RECALBOXDMD_SELFTEST"]))

sd = toolkit.get_sd_card_dir(_tools_dir)
RecalBoxDMD_GUI.RetroBoxLEDGui(toolkit, sd).run()
