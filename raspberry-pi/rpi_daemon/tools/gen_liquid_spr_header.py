# -*- coding: utf-8 -*-
# gen_liquid_spr_header.py -- genere ../src/ui/liquid_spr_data.h : les SPRITES des objets flottants du widget Liquide (sd_card/widgets/liquid/*.spr, generes par tools/gen_liquid_sprites.py)
# EMBARQUES dans le DEMON RASPBERRY PI (le firmware ESP32 les telecharge sur la SD, voir DECISIONS.md « v418 »). Le demon les ecrit sur sa « SD » seulement s'ils sont absents (lqEnsureSpr dans widgets.cpp,
# jamais d'ecrasement : un sprite MODIFIE doit changer de NOM). A relancer apres toute modification d'un sprite.
import os
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "sd_card", "widgets", "liquid")
OUT = os.path.join(HERE, "..", "src", "ui", "liquid_spr_data.h")
files = sorted(f for f in os.listdir(SRC) if f.endswith(".spr"))
o = ["// liquid_spr_data.h -- GENERE par tools/gen_liquid_spr_header.py (ne pas editer a la main) : sprites des objets flottants du widget Liquide, %d fichiers." % len(files),
     "#pragma once", "#include <stdint.h>", "struct DecoLiqAsset { const char *file; const uint8_t *data; uint32_t len; };"]
names = []
for i, f in enumerate(files):
    d = open(os.path.join(SRC, f), "rb").read()
    o.append("static const uint8_t DECO_LIQ_%d[] = {" % i)
    for k in range(0, len(d), 32): o.append(",".join(str(b) for b in d[k:k + 32]) + ",")
    o.append("};"); names.append((f, i, len(d)))
o.append("static const DecoLiqAsset DECO_LIQ_ASSETS[] = {")
for f, i, n in names: o.append('  {"%s", DECO_LIQ_%d, %d},' % (f, i, n))
o.append("};")
open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(o) + "\n")
print("ok", OUT, len(files), "fichiers,", sum(n for _, _, n in names), "octets")
