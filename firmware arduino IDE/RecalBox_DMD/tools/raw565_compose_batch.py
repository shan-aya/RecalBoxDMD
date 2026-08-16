# -*- coding: utf-8 -*-
"""
raw565_compose_batch.py -- pour chaque PNG depose dans incoming_fr/ et
incoming_es/, recompose : icone+fond = fichier .raw565 ORIGINAL (jamais
modifie), texte = le PNG genere par l'IA (crop seul ou image complete,
detection auto). Ecrit le resultat dans sd_card/systems/_defaults/fr/ et /es/.

Usage : python raw565_compose_batch.py
"""
import re
from pathlib import Path

from localization_compose import compose_final

ROOT = Path(__file__).parent
KIT_DIR = ROOT / "assets" / "systems_defaults_localization_kit"
DEFAULTS = ROOT.parent / "sd_card" / "systems" / "_defaults"


def expected_stems():
    fr_stems, es_stems = set(), set()
    idx = KIT_DIR / "PROMPTS_INDEX.md"
    if idx.exists():
        fr_stems = set(re.findall(r"incoming_fr/([\w-]+)\.png", idx.read_text(encoding="utf-8")))
    es_idx = KIT_DIR / "PROMPTS_ES_INDEX.md"
    if es_idx.exists():
        es_stems = set(re.findall(r"incoming_es/([\w-]+)\.png", es_idx.read_text(encoding="utf-8")))
    return fr_stems, es_stems


# Pseudo-systemes deja traduits en FR "a la main" des le depart (favoris,
# derniers jeux, etc.) -- leur reference ES a ete generee par l'IA a partir
# du CROP FR (fr_reference/), pas du crop EN. La recomposition ES doit donc
# repositionner l'icone/texte sur la base FR (memes proportions que la
# reference utilisee), sinon le texte EN d'origine (plus court/different)
# laisse un residu visible autour du texte ES colle par-dessus.
ALREADY_FR_STEMS = {
    "genre-adventure", "genre-adventurerealtime3d",
    "allgames", "favorites", "lastplayed", "ports",
}


def base_source_for(stem: str, lang: str) -> Path:
    """Fichier ORIGINAL utilise comme reference icone+fond.
    lang="es" sur un pseudo-systeme deja traduit en FR : la reference ES a
    ete generee depuis le crop FR, donc on recompose sur la base FR (memes
    proportions), pas EN. Sinon (FR, ou ES sur un genre normal) : FR reel
    existant si disponible, sinon EN de base."""
    fr_real = DEFAULTS / "fr" / f"{stem}.raw565"
    en_real = DEFAULTS / f"{stem}.raw565"
    if lang == "es" and stem in ALREADY_FR_STEMS and fr_real.exists():
        return fr_real
    if fr_real.exists() and not en_real.exists():
        return fr_real
    if en_real.exists():
        return en_real
    return fr_real


def compose_folder(src_dir: Path, dst_dir: Path, expected: set, lang: str):
    dst_dir.mkdir(parents=True, exist_ok=True)
    found = {p.stem for p in src_dir.glob("*.png")} if src_dir.exists() else set()
    done = []
    for stem in sorted(found):
        base = base_source_for(stem, lang)
        if not base.exists():
            print(f"  ! pas de fichier original pour {stem}, ignore")
            continue
        compose_final(base, src_dir / f"{stem}.png", dst_dir / f"{stem}.raw565")
        done.append(stem)
    missing = sorted(expected - found)
    extra = sorted(found - expected)
    return done, missing, extra


def main():
    fr_expected, es_expected = expected_stems()

    fr_done, fr_missing, fr_extra = compose_folder(KIT_DIR / "incoming_fr", DEFAULTS / "fr", fr_expected, "fr")
    es_done, es_missing, es_extra = compose_folder(KIT_DIR / "incoming_es", DEFAULTS / "es", es_expected, "es")

    print(f"FR : {len(fr_done)}/{len(fr_expected)} composes -> {DEFAULTS / 'fr'}")
    if fr_missing:
        print(f"  manquants ({len(fr_missing)}): {', '.join(fr_missing)}")
    if fr_extra:
        print(f"  inattendus (pas dans PROMPTS.md): {', '.join(fr_extra)}")

    print(f"ES : {len(es_done)}/{len(es_expected)} composes -> {DEFAULTS / 'es'}")
    if es_missing:
        print(f"  manquants ({len(es_missing)}): {', '.join(es_missing)}")
    if es_extra:
        print(f"  inattendus (pas dans PROMPTS.md): {', '.join(es_extra)}")


if __name__ == "__main__":
    main()
