#!/usr/bin/env python3
"""Applique le filigrane invisible DGC aux logos DEJA publies d'un paquet de themes, puis recalcule la `rev` de chaque theme dans le manifeste
(les toolkits annoncent alors « theme mis a jour » et reecrivent les logos : une seule vague).

Idempotent : un logo deja marque n'est pas modifie ; relancer ne change plus rien. Sans --apply : simulation (aucun fichier ecrit).
Usage : python mark_package.py <dossier _themes> [<dossier _themes_rb11> ...] [--apply]
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_theme_package as b   # noqa: E402
import dgc_mark                   # noqa: E402


def process(root, apply):
    mpath = os.path.join(root, "manifest.json")
    with open(mpath, encoding="utf-8") as f:
        manifest = json.load(f)
    print(f"== {root}")
    changed = 0
    for name, entry in manifest.get("themes", {}).items():
        d = os.path.join(root, name)
        if not os.path.isdir(d):
            print(f"  {name:34s} dossier absent : ignore")
            continue
        n = 0
        for dp, _dirs, files in os.walk(d):
            for fn in files:
                if fn.endswith(".raw565"):
                    p = os.path.join(dp, fn)
                    with open(p, "rb") as f:
                        data = f.read()
                    new = dgc_mark.mark_raw565(data)
                    if new != data:
                        n += 1
                        if apply:
                            with open(p, "wb") as f:
                                f.write(new)
        old_rev = entry.get("rev")
        new_rev = b.theme_rev(d) if apply else "(simulation)"
        if apply and new_rev != old_rev:
            entry["rev"] = new_rev
            changed += 1
        print(f"  {name:34s} {n:5d} logo(s) a marquer ; rev {old_rev} -> {new_rev}")
    if apply and changed:
        with open(mpath, "w", encoding="utf-8") as f:
            json.dump(manifest, f, ensure_ascii=False, indent=1)
        print(f"  manifeste reecrit ({changed} rev modifiee(s))")
    return changed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("roots", nargs="+")
    ap.add_argument("--apply", action="store_true")
    a = ap.parse_args()
    for r in a.roots:
        process(r, a.apply)
    if not a.apply:
        print("\nSimulation : rien n'a ete ecrit (--apply pour appliquer).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
