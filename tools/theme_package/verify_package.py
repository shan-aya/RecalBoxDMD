#!/usr/bin/env python3
"""Verifie la REPRODUCTIBILITE du paquet de logos de themes : reconstruit tout le paquet dans un dossier temporaire et compare la
revision (rev) de chaque theme avec le manifeste publie. A lancer (manuellement) avant d'activer la mise a jour planifiee : si le
meme theme (meme version du hub) donne une autre rev sur Linux, chaque PR automatique contiendrait des changements parasites.

Usage : python verify_package.py <dossier _themes publie> --known <dossier _defaults>
Code de sortie : 0 = reproductible, 1 = ecart sur un theme dont la version du hub n'a pas change.
"""
import argparse
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_theme_package as b   # noqa: E402
import dgc_mark   # noqa: E402


def unmarked_files(published):
    """Logos .raw565 du paquet PUBLIE qui devraient porter le filigrane DGC et ne l'ont pas (mark_raw565 les modifierait encore)."""
    out = []
    for root, _d, files in os.walk(published):
        for name in sorted(files):
            if name.endswith(".raw565"):
                with open(os.path.join(root, name), "rb") as f:
                    data = f.read()
                if dgc_mark.mark_raw565(data) != data:
                    out.append(os.path.relpath(os.path.join(root, name), published))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("published")
    ap.add_argument("--known", required=True)
    a = ap.parse_args()
    with open(os.path.join(a.published, "manifest.json"), encoding="utf-8") as f:
        pub = json.load(f)
    with tempfile.TemporaryDirectory() as tmp:
        m = b.build(tmp, a.known, log=lambda s: None)
    bad = 0
    print(f"{'theme':36s} {'version publiee':16s} {'version hub':12s} rev")
    for name, e in m["themes"].items():
        p = pub.get("themes", {}).get(name)
        if not p:
            print(f"{name:36s} {'-':16s} {str(e.get('version')):12s} nouveau theme (absent du paquet publie)")
            continue
        same_version = str(p.get("version")) == str(e.get("version"))
        same_rev = p.get("rev") == e.get("rev")
        verdict = "identique" if same_rev else ("DIFFERENT (version du hub changee : normal)" if not same_version else "DIFFERENT A VERSION EGALE : NON REPRODUCTIBLE")
        if not same_rev and same_version:
            bad += 1
        print(f"{name:36s} {str(p.get('version')):16s} {str(e.get('version')):12s} {verdict}")
    for name in pub.get("themes", {}):
        if name not in m["themes"]:
            print(f"{name:36s} retire du hub (serait supprime)")
    um = unmarked_files(a.published)
    if um:
        print(f"\nFILIGRANE DGC ABSENT sur {len(um)} logo(s) du paquet publie (ex. {um[0]}) : lancer mark_package.py --apply")
    print("\nRESULTAT :", "reproductible" if not bad else f"{bad} theme(s) non reproductible(s)", "; filigrane DGC " + ("incomplet" if um else "present sur tous les logos"))
    return 1 if (bad or um) else 0


if __name__ == "__main__":
    sys.exit(main())
