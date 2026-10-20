"""Extrait du firmware (RecalBox_DMD.ino) le code des WIDGETS du decor vers src/ui/widgets_fw.inc, a inclure tel quel dans
src/ui/widgets.cpp derriere la couche de compatibilite (compat/Arduino.h).

Widgets repris du firmware sans modification : date (carte calendrier + jours feries), ephemeride (soleil / lune), compte a
rebours, ambiances (plasma, pluie, etoiles, vie, liquide, sable, sable 2, lava lamp), meteo (Open-Meteo), le « ciel » anime qui leur
sert de fond, et les widgets Recalbox (challenge, recompenses, sante) quand ils sont presents dans le bloc.
Ce qui est ECRIT A LA MAIN dans widgets.cpp : la boucle d'animation du ciel (canevas hors ecran au lieu des bandes de l'ESP32), decoStop()
(interruption + entretien du reseau), le chargement des fonds d'image, la lecture des citations sur la SD.

Pourquoi un script : ce code vit dans le .ino (statique, melange a d'autres fonctions) ; le recopier a la main ferait deriver les deux
versions. Relancer apres chaque evolution des widgets dans le firmware, puis recompiler et re-tester.

Usage : python port_widgets.py [chemin/RecalBox_DMD.ino]   (defaut : ../RecalBox_DMD.ino)
"""
import re
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
ino = Path(sys.argv[1]) if len(sys.argv) > 1 else here.parents[1] / "RecalBox_DMD.ino"
out = here.parent / "src" / "ui" / "widgets_fw.inc"

lines = ino.read_text(encoding="utf-8", errors="replace").splitlines()
text = "\n".join(lines)
m = re.search(r"#define FW_VERSION_NUM (\d+)", text)
fw = m.group(1) if m else "?"


def find(prefix, start=0):
    for i in range(start, len(lines)):
        if lines[i].startswith(prefix):
            return i
    raise SystemExit(f"repere introuvable : {prefix!r}")


# (debut, fin EXCLUE) : repere = debut de ligne ; la recherche de « fin » commence apres « debut » (l'ordre est donc verifie)
BLOCKS = [
    ("struct RbwData {", "int32_t decoAmbCol[9][2]", "donnees des widgets Recalbox (CMD=rbw) et leur analyse"),
    ("static int decoIsoWeek", "// ============ v274 -- Widget COMPTE", "date + ephemeride"),
    ("static long decoJdn", "static long s_cdDays", "compte a rebours : calcul de l'evenement"),
    ("// -- Ambiances : plasma", "// ============ v276 -- Widget METEO", "ambiances"),
    ("struct DecoWx", "static void decoWxCloudG", "meteo : donnees, requete Open-Meteo, classes"),
    ("struct SkyKey", "class SkyStrip", "ciel anime : degrades, astres, nuages, pluie"),
    ("static int decoSkyClass", "// -- statistiques --", "avant-plans : meteo, date, compte a rebours, ephemeride"),
    ("// ---- v315 : widgets RECALBOX", "// Execute un widget. Retourne false", "avant-plans des widgets Recalbox : challenge, recompenses, sante"),
    ("// Themes d'origine embarques", "char g_quoteThemeName", "citations : themes d'origine embarques"),
    ("char g_quoteThemeName", "static void decoQuoteMkdirs", "citations : etat, table des themes"),
    ("// UTF-8 -> ASCII", "// Tire une citation au hasard", "citations : UTF-8 -> ASCII"),
    ("// Affichage : defilement vertical d'un texte", "// ============ v287 -- QCM", "citations : vitesse de defilement"),
    ("// ============ v287 -- QCM", "// v326 -- ecrit /widgets/quotes/bg", "QCM : decoupe de texte TomThumb"),
    ("static void decoQuoteBgLoadImpl();", "// Boucle commune d'un widget", "citations : fond, texte, QCM, defilement"),
]

parts = []
for start_p, end_p, label in BLOCKS:
    a = find(start_p)
    b = find(end_p, a + 1)
    parts.append((label, a, b, lines[a:b]))

header = f"""// ============================================================================
// GENERE par tools/port_widgets.py depuis RecalBox_DMD.ino (FW_VERSION_NUM {fw}).
// NE PAS EDITER A LA MAIN : relancer le script. Widgets du decor (voir l'en-tete du script).
// ============================================================================
"""
body = []
for label, a, b, chunk in parts:
    body.append(f"// ---- {label} (firmware, lignes {a + 1}-{b}) ----")
    body.extend(chunk)
    body.append("")
out.write_text(header + "\n".join(body) + "\n", encoding="utf-8")
print(f"{out} : " + ", ".join(f"{label} {b - a} lignes" for label, a, b, _ in parts) + f" (firmware {fw})")
