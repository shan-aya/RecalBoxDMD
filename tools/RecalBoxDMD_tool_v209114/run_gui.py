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
    # v2 (2026-11-23) : identite du build, verifiee par build_release.ps1 (piste stable / alpha)
    lines.append("channel=%s" % toolkit.ACTIVE_CHANNEL)
    lines.append("toolkit_build=%s" % RecalBoxDMD_GUI.TOOLKIT_RELEASE_VERSION)
    lines.append("custom_logos_tab_visible=%s" % toolkit.feature_visible("custom_logos_tab"))
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
        # filigrane DGC : ce que ecrit la boite a outils doit etre reconnu par dgc_mark.detect
        import dgc_mark
        marked = dgc_mark.detect(data, "x.raw565")
        lines.append("filigrane DGC sur le rendu SVG : %s" % (marked,))
        ok = ok and bool(marked)
        # prefer_keys / detect_variants (fonctions utilisees par la fenetre) : simple verification d'appel
        lines.append("prefer_keys(fr_FR, eu) = %s" % (tl.prefer_keys("fr_FR", "eu"),))
        # onglet Personnalisation (v144) : les 3 modules doivent etre dans l'exe (hiddenimports du .spec) et pi_logos / custom_logos doivent fonctionner
        import custom_logos as cl
        import pi_logos as pil
        import RecalBoxDMD_custom_logos_gui as clg   # noqa: F401
        import game_gifs, game_gifs_store, gifmatch, RecalBoxDMD_game_gifs_gui   # noqa: F401  (Mode 21 GIFs du jeu : modules embarques ; regle de correspondance verifiee)
        _gi = gifmatch.gif_info('ARCADE_AfterBurner.gif')
        assert gifmatch.rank(gifmatch.norm('After Burner'), _gi[0], _gi[1]) == 1
        raw = cl.rgb_to_raw565(PIL.Image.new("RGB", (128, 32), (255, 0, 0)))
        lines.append("onglet Personnalisation : custom_logos + pi_logos + fenetre importes, fnv32(vide)=%s, raw565=%d octets" % (pil.fnv32(b""), len(raw)))
        ok = ok and pil.fnv32(b"") == "811c9dc5" and len(raw) == 8192
        import pi_playlist as pipl   # onglet Playlist par Wi-Fi (demon 3.92) : module embarque
        _sup = (pipl.supported({"is_pi": True, "version": "3.92"}), pipl.supported({"is_pi": True, "version": "3.91"}), pipl.supported({"is_pi": False, "version": "3.99"}))
        lines.append("onglet Playlist (Pi Wi-Fi) : pi_playlist importe, PL_MIN=%d, supported(3.92 / 3.91 / non-Pi)=%s" % (pipl.PL_MIN, _sup))
        ok = ok and pipl.PL_MIN == 392 and _sup == (True, False, False)
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
import RecalBoxDMD_pi
import RecalBoxDMD_pi_gui   # noqa: F401  (Modes 16 / 17 : importe ici pour que PyInstaller l'embarque)
import RecalBoxDMD_diag_gui   # noqa: F401  (onglet Diagnostic : importe ici pour que PyInstaller l'embarque)
import RecalBoxDMD_diag_report   # noqa: F401  (rapport texte / HTML de l'onglet Diagnostic)
import fat32_format   # noqa: F401  (formatage FAT32 de la carte SD : importe ici pour que PyInstaller l'embarque)
import dgc_mark   # noqa: F401  (filigrane invisible DGC : importe ici pour que PyInstaller l'embarque)

import os

if os.environ.get("RECALBOXDMD_SELFTEST"):
    sys.exit(_selftest(os.environ["RECALBOXDMD_SELFTEST"]))

# Modes 16 / 17 : le processus ELEVE relance par la fenetre du Mode 16 ecrit la carte SD du Raspberry Pi, sans ouvrir de fenetre.
if '--pi-write-job' in sys.argv:
    sys.exit(RecalBoxDMD_pi.helper_main(sys.argv[sys.argv.index('--pi-write-job') + 1]))

# Popup « Carte SD » du Mode 1 : le processus ELEVE relance par le bouton « Formater en FAT32… » formate la carte, sans ouvrir de fenetre.
if '--fat32-format-job' in sys.argv:
    sys.exit(fat32_format.helper_main(sys.argv[sys.argv.index('--fat32-format-job') + 1]))

sd = toolkit.get_sd_card_dir(_tools_dir)
RecalBoxDMD_GUI.RetroBoxLEDGui(toolkit, sd).run()
