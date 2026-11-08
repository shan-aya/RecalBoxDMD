# -*- coding: utf-8 -*-
# gen_widget_bg_header.py -- genere ../src/ui/widget_bg_data.h : les fonds d'image des widgets (sd_card/widgets/quotes/bg/*.raw565 + .col) EMBARQUES dans le DEMON RASPBERRY PI (le firmware ESP32 4.04+ ne les embarque plus : il les telecharge sur la SD, voir DECISIONS.md « ETAPE 1 »).
# Le firmware les ecrit sur la SD seulement s'ils sont absents (decoBgEnsure). A relancer apres toute modification d'un fond. Si la partition manquait de place,
# on pourrait retirer ce fichier et faire copier sd_card/widgets/ par le toolkit (voir DECISIONS.md).
import os
HERE = os.path.dirname(os.path.abspath(__file__)); BG = os.path.join(HERE, "..", "..", "sd_card", "widgets", "quotes", "bg"); OUT = os.path.join(HERE, "..", "src", "ui", "widget_bg_data.h")
files = sorted(f for f in os.listdir(BG) if f.endswith((".raw565", ".col")))
o = ["// widget_bg_data.h -- GENERE par tools/gen_widget_bg_header.py (ne pas editer a la main) : fonds d'image des widgets (128x32 raw565 + fichiers .col), %d fichiers." % len(files), "#pragma once", "#include <stdint.h>", "struct DecoBgAsset { const char *file; const uint8_t *data; uint16_t len; };"]
names = []
for i, f in enumerate(files):
    d = open(os.path.join(BG, f), "rb").read()
    o.append("static const uint8_t DECO_BG_%d[] PROGMEM = {" % i)
    for k in range(0, len(d), 24): o.append(",".join(str(b) for b in d[k:k + 24]) + ",")
    o.append("};"); names.append((f, i, len(d)))
o.append("static const DecoBgAsset DECO_BG_ASSETS[] = {")
for f, i, n in names: o.append('  {"%s", DECO_BG_%d, %d},' % (f, i, n))
o.append("};")
open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(o) + "\n"); print("ok", OUT, len(files), "fichiers")
