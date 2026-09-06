# -*- coding: utf-8 -*-
"""
raw565_batch_encode.py -- convertit en masse les PNG localises deposes dans
tools/assets/systems_defaults_localization_kit/incoming_fr/ et incoming_es/
vers sd_card/systems/_defaults/fr/ et /es/ (.raw565, 128x32 RGB565).

Usage :
    python raw565_batch_encode.py

Rapporte aussi les fichiers manquants par rapport a la liste attendue dans
PROMPTS.md (mots-cles `incoming_fr/<stem>.png` / `incoming_es/<stem>.png`),
pour verifier que rien n'a ete oublie avant de continuer.
"""
import re
from pathlib import Path

from raw565_convert import png_to_raw565

ROOT = Path(__file__).parent
KIT_DIR = ROOT / "assets" / "systems_defaults_localization_kit"
DEFAULTS = ROOT.parent / "sd_card" / "systems" / "_defaults"


def expected_stems():
    text = (KIT_DIR / "PROMPTS.md").read_text(encoding="utf-8")
    fr_stems = set(re.findall(r"incoming_fr/([\w-]+)\.png", text))
    es_stems = set(re.findall(r"incoming_es/([\w-]+)\.png", text))
    return fr_stems, es_stems


def encode_folder(src_dir: Path, dst_dir: Path, expected: set):
    dst_dir.mkdir(parents=True, exist_ok=True)
    found = {p.stem for p in src_dir.glob("*.png")} if src_dir.exists() else set()
    for stem in sorted(found):
        png_to_raw565(src_dir / f"{stem}.png", dst_dir / f"{stem}.raw565")
    missing = sorted(expected - found)
    extra = sorted(found - expected)
    return found, missing, extra


def main():
    fr_expected, es_expected = expected_stems()

    fr_found, fr_missing, fr_extra = encode_folder(KIT_DIR / "incoming_fr", DEFAULTS / "fr", fr_expected)
    es_found, es_missing, es_extra = encode_folder(KIT_DIR / "incoming_es", DEFAULTS / "es", es_expected)

    print(f"FR : {len(fr_found)}/{len(fr_expected)} convertis -> {DEFAULTS / 'fr'}")
    if fr_missing:
        print(f"  manquants ({len(fr_missing)}): {', '.join(fr_missing)}")
    if fr_extra:
        print(f"  inattendus (pas dans PROMPTS.md): {', '.join(fr_extra)}")

    print(f"ES : {len(es_found)}/{len(es_expected)} convertis -> {DEFAULTS / 'es'}")
    if es_missing:
        print(f"  manquants ({len(es_missing)}): {', '.join(es_missing)}")
    if es_extra:
        print(f"  inattendus (pas dans PROMPTS.md): {', '.join(es_extra)}")


if __name__ == "__main__":
    main()
