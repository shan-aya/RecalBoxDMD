#!/usr/bin/env python3
"""Verifie, pour TOUS les themes du paquet, que le logo que le DMD choisira (logique du firmware v238 : variante de langue l_<lang>, variante de region
r_<region>, puis base) est le MEME que celui qu'EmulationStation choisirait pour la meme langue et la meme region.

Reference independante du moteur de conversion : on relit les elements <image name="logo"> du theme (attributs path / path.EU / path.fr..., dans l'ORDRE du XML) et on
applique la regle d'ES (ThemeData.cpp : un attribut localise ne s'applique que s'il correspond a la langue / langue_pays / region ET si le fichier existe ; un attribut
neutre est ignore une fois la propriete localisee). Le resultat attendu est rendu (SVG/PNG -> raw565) puis compare octet pour octet a ce que la SD donnerait.

Usage : python verify_package_semantics.py <dossier _themes du paquet> --known <dossier _defaults> [--only theme1,theme2] [--report fichier.txt]
Code de sortie : 0 = aucun ecart, 1 = au moins un ecart.

safe-modify -- v2 - 2026-10-04 - les elements <image name="logo" region="eu" ...> (un element par region, ex. bounitos-crt-color-package) sont pris en compte : l'element ne
s'applique que si sa region / langue correspond, et tous les elements sont fusionnes DANS L'ORDRE du XML (le dernier gagne), comme ES.
safe-modify -- v1 - 2026-10-04 - creation (demande utilisateur : « verifier tous les themes sans tester a la main »).
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import theme_logos as tl   # noqa: E402

LANGS = (("en", "US"), ("fr", "FR"), ("es", "ES"), ("de", "DE"), ("it", "IT"), ("pt", "PT"))
REGIONS = ("us", "eu", "jp")


def logo_elements(src):
    """Elements <image name="logo"> contenant $system : liste de listes [(suffixe|None, modele)] dans l'ordre du XML (attributs puis sous-noeuds)."""
    xml_files = [f for f in src.files() if f.lower().endswith(".xml")]
    xml_files.sort(key=lambda f: (f.lower() != "theme.xml", f.count("/") > 0, src.size(f), f))
    out, budget = [], tl.XML_BUDGET
    for f in xml_files[:tl.XML_MAX_FILES]:
        sz = src.size(f)
        if sz > budget:
            continue
        budget -= sz
        try:
            xml = src.read(f).decode("utf-8", errors="replace")
        except Exception:
            continue
        for m in re.finditer(r"<image\b(?P<attrs>[^>]*?)(?:/>|>(?P<body>.*?)</image>)", xml, flags=re.S | re.I):
            attrs = m.group("attrs")
            if not re.search(r"\bname=\"logo\"", attrs, re.I):
                continue
            paths = []
            pairs = re.findall(r"([\w.]+)=\"([^\"]*)\"", attrs)
            ekey = next((v.lower() for k, v in pairs if k.lower() in ("region", "lang", "language")), None)   # filtre au niveau de l'element
            for k, v in pairs:
                kl = k.lower()
                if kl == "path":
                    paths.append((None, v))
                elif kl.startswith("path."):
                    paths.append((kl[5:], v))
            for node in re.finditer(r"<(path(?:\.[A-Za-z_]+)?)>\s*([^<]*?)\s*</\1>", m.group("body") or "", flags=re.I):
                tag = node.group(1).lower()
                paths.append((None if tag == "path" else tag[5:], node.group(2)))
            paths = [(s, v) for s, v in paths if re.search(r"\$system(?!\w)", v)]
            if paths:
                out.append({"key": ekey, "paths": paths})
    return out


def es_pick(elems, sid, lang, country, region, exists):
    """Fichier retenu par ES pour (langue, pays, region) : elements fusionnes dans l'ordre du XML, attributs lus dans l'ordre, localise = prioritaire,
    un chemin n'est retenu que si le fichier existe."""
    chosen, localized = None, False
    for el in elems:
        if el["key"] is not None and el["key"] not in (lang, f"{lang}_{country}".lower(), region):
            continue          # <image region="eu"> : element ignore pour une autre region / langue
        for suffix, tpl in el["paths"]:
            if suffix is not None and suffix not in (lang, f"{lang}_{country}".lower(), region):
                continue
            if suffix is None and localized:
                continue
            rel = re.sub(r"^\$\{root\}/|^\./", "", tpl.strip()).replace("$system", sid)
            if "$" in rel:
                continue
            if exists(rel):
                chosen = rel
            if suffix is not None and chosen is not None:
                localized = True
    return chosen


def dmd_pick(theme_dir, sid, lang, region):
    """Fichier .raw565 que le firmware v238 lirait : variante de langue, variante de region, base."""
    cands = []
    if lang != "en":
        cands.append(os.path.join(theme_dir, f"l_{lang}", sid + ".raw565"))
    if region != "us":
        cands.append(os.path.join(theme_dir, f"r_{region}", sid + ".raw565"))
    cands.append(os.path.join(theme_dir, sid + ".raw565"))
    for c in cands:
        if os.path.isfile(c):
            return c
    return None


def check_theme(name, src, theme_dir, log):
    names = set(src.files())
    exists = names.__contains__
    elems = logo_elements(src)
    sids = set()
    for el in elems:
        for _s, tpl in el["paths"]:
            rx = re.escape(re.sub(r"^\$\{root\}/|^\./", "", tpl.strip())).replace(re.escape("$system"), r"(?P<sid>[^/]+?)")
            try:
                cre = re.compile("^" + rx + "$", re.I)
            except re.error:
                continue
            for f in names:
                m = cre.match(f)
                if m:
                    sids.add(m.group("sid").lower())
    sids = {s for s in sids if not tl._NOISE.search(s)}
    cache = {}

    def rendered(rel):
        if rel not in cache:
            try:
                cache[rel] = tl.convert_bytes(src.read(rel), rel.lower().endswith(".svg"))
            except Exception:
                cache[rel] = None
        return cache[rel]

    checked = bad = ambiguous = 0
    problems = []
    for sid in sorted(sids):
        for lang, country in LANGS:
            for region in REGIONS:
                pick = es_pick(elems, sid, lang, country, region, exists)
                expected = {pick} if pick else set()
                if not expected:
                    continue
                got = dmd_pick(theme_dir, sid, lang, region)
                checked += 1
                if got is None:
                    bad += 1
                    problems.append((sid, lang, region, sorted(expected)[0].split("/")[-1], "AUCUN fichier sur la SD"))
                    continue
                gb = open(got, "rb").read()
                exp_bytes = [rendered(e) for e in expected]
                if len(expected) > 1:
                    ambiguous += 1
                if gb not in exp_bytes:
                    bad += 1
                    problems.append((sid, lang, region, sorted(expected)[0].split("/")[-1], os.path.relpath(got, theme_dir)))
    log(f"{name}: {len(sids)} systemes, {checked} combinaisons langue x region verifiees, {bad} ecart(s)" + (f", {ambiguous} cas ou plusieurs vues du theme divergent" if ambiguous else ""))
    return checked, bad, problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("published")
    ap.add_argument("--known", required=True)
    ap.add_argument("--only", default="")
    ap.add_argument("--report", default=None)
    a = ap.parse_args()
    only = {x for x in a.only.split(",") if x}
    lines = []

    def log(s):
        print(s, flush=True)
        lines.append(s)

    cat = {h["folder"]: h for h in tl.hub_catalog(lambda *x: None) if h.get("active", True)}
    total_c = total_b = 0
    allp = []
    for folder in sorted(os.listdir(a.published)):
        tdir = os.path.join(a.published, folder)
        if not os.path.isdir(tdir) or (only and folder not in only):
            continue
        h = cat.get(folder)
        if not h or not h.get("zips"):
            log(f"{folder}: absent du catalogue, ignore")
            continue
        src, _rf = tl.open_hub_source(h, cache_dir=tl.default_cache_dir())
        c, b, p = check_theme(folder, src, tdir, log)
        total_c += c
        total_b += b
        allp += [(folder,) + x for x in p]
    for p in allp[:80]:
        log("  ECART " + " | ".join(str(x) for x in p) + "   (theme | systeme | langue | region | attendu | fourni)")
    log(f"TOTAL : {total_c} combinaisons verifiees, {total_b} ecart(s)")
    if a.report:
        open(a.report, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    return 1 if total_b else 0


if __name__ == "__main__":
    sys.exit(main())
