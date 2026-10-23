"""Extrait du firmware (RecalBox_DMD.ino) le code des 5 themes d'horloge ANIMES (plasma, vie, etoiles, pluie, lave) vers
src/ui/clock_ambient.inc, a inclure tel quel dans src/ui/clock.cpp derriere la couche de compatibilite Arduino.

Pourquoi un script : ce code vit dans le .ino (statique, melange a d'autres fonctions) ; le recopier a la main ferait deriver les deux
versions. Relancer ce script apres chaque evolution de ces themes dans le firmware, puis recompiler et re-tester.

Usage : python port_clock_ambient.py [chemin/RecalBox_DMD.ino]   (defaut : ../RecalBox_DMD.ino)
"""
import re
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
ino = Path(sys.argv[1]) if len(sys.argv) > 1 else here.parents[1] / "RecalBox_DMD.ino"
out = here.parent / "src" / "ui" / "clock_ambient.inc"

lines = ino.read_text(encoding="utf-8", errors="replace").splitlines()
text = "\n".join(lines)
m = re.search(r"#define FW_VERSION_NUM (\d+)", text)
fw = m.group(1) if m else "?"

def find(prefix, start=0):
    for i in range(start, len(lines)):
        if lines[i].startswith(prefix):
            return i
    raise SystemExit(f"repere introuvable : {prefix!r}")

# 1) structures de la lave (declarees plus haut dans le .ino)
s_lava = find("struct LavaBlob")
s_ball = find("struct LavaBall", s_lava)
struct_lines = lines[s_lava:s_ball + 1]   # LavaBlob, LavaSt (+ static_assert depuis v382), LavaBall
assert 3 <= len(struct_lines) <= 6 and struct_lines[-1].startswith("struct LavaBall"), "structures Lava* inattendues"

# 2) le bloc des themes d'horloge ambiants : de « struct AmbRain » jusqu'a juste avant clockThemeName()
a = find("struct AmbRain")
b = find("static const char *clockThemeName")
chunk = lines[a:b]

header = f"""// ============================================================================
// GENERE par tools/port_clock_ambient.py depuis RecalBox_DMD.ino (FW_VERSION_NUM {fw}, lignes {a + 1}-{b}).
// NE PAS EDITER A LA MAIN : relancer le script. Themes d'horloge animes : Plasma, Life, Stars, Rain, Lava.
// ============================================================================
"""
out.write_text(header + "\n".join(struct_lines) + "\n\n" + "\n".join(chunk) + "\n", encoding="utf-8")
print(f"{out} : {len(chunk)} lignes du firmware {fw} (lignes {a + 1}-{b})")
