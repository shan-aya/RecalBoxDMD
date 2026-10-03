#!/usr/bin/env python3
"""Genere le PAQUET de logos de themes Recalbox a publier sur GitHub (outil du mainteneur, pas de l'utilisateur final).

Pour chaque theme (themes actifs du theme-hub + themes installes sur la Recalbox) :
  _themes/<theme>/<systeme>.raw565   logos de BASE (variante US / anglais), + _index.bin + _source.json
  _themes/<theme>/fr/<systeme>.raw565  SEULEMENT les logos qui different en fr_FR (a superposer a plat, comme _defaults/fr)
  _themes/<theme>/es/<systeme>.raw565  idem pour es_ES
  _themes/manifest.json              liste de tous les fichiers (une seule requete pour le toolkit, pas d'API GitHub)

Le toolkit (Mode 1) telecharge manifest.json puis les fichiers de la langue choisie pour les logos par defaut.
Les mises a jour / ajouts ulterieurs se font par la fenetre "Logos de theme Recalbox" (theme_logos.py).

Usage : python build_theme_package.py <dossier _themes> [--known <dossier _defaults>] [--only theme1,theme2]
                                      [--incremental] [--summary <fichier.md>]
  --incremental : le dossier de sortie contient deja le paquet publie ; seuls les themes dont la version du hub a change
                  (ou nouveaux) sont reconstruits, les themes retires du hub sont supprimes, un theme deja reconnu "sans
                  logo" n'est retente que si sa version change. Aucun changement => rien n'est ecrit (manifeste intact).
  --summary     : ecrit un compte rendu Markdown des changements (corps de la Pull Request du workflow GitHub).
Dependances : resvg-py, numpy, pillow (versions figees dans requirements.txt : le rendu doit rester reproductible).

safe-modify -- v6 - 2026-10-03 - mode --incremental + --summary (workflow GitHub Actions "Update theme logos"), skipped_versions.
v5 : manifest skipped (themes du hub sans logo de systeme). v4 : champ rev (revision du contenu de chaque theme, comparee par le
toolkit avec la SD). v3 : champ variants (langues/regions proposees par theme). v2 : fr/es utilisent la region eu (consoles
europeennes), base = us. v1 : creation. Sauvegardes : _backups/build_theme_package.py.*.bak
"""
import argparse
import hashlib
import json
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import theme_logos as tl   # noqa: E402

LANGS = {"en": "en_US", "fr": "fr_FR", "es": "es_ES"}
REGIONS = {"en": "us", "fr": "eu", "es": "eu"}   # region d origine des consoles associee a chaque langue (fr/es -> logos de consoles europeens)
PACKAGE_FORMAT = 1
NO_LOGO_REASON = "aucun logo de système exploitable"


def _open_source(folder, hub, rb_root):
    """ZIP du THEME-HUB en priorite (reference publique/stable : decision utilisateur 2026-10-03, les copies installees sur une
    Recalbox de test sont trop variables) ; copie de la Recalbox seulement si le theme n est pas dans le hub ET que rb_root est fourni."""
    h = hub.get(folder)
    if h and h.get("zips"):
        rf = tl.RangeFile(tl.hub_zip_urls(folder, h["zips"][0]))
        return tl.ZipSource(rf), "hub", rf
    rb_dir = os.path.join(rb_root, folder) if rb_root else ""
    if rb_dir and os.path.exists(os.path.join(rb_dir, "theme.xml")):
        return tl.DirSource(rb_dir), "rb", None
    return None, None, None


def _alias_name(sid, base_ids):
    """'auto-X' -> 'X' (SystemId envoye par ES) quand X n'est pas un vrai logo de base (meme regle que convert_theme)."""
    return sid[5:] if sid.startswith("auto-") and len(sid) > 5 and sid[5:] not in base_ids else None


def theme_rev(theme_dir):
    """Revision du contenu publie d'un theme (12 car.) : hash des logos de base + surcharges fr/es + _index.bin.
    Stockee dans le manifeste (entry["rev"]) ; le toolkit la recopie dans <theme>/_source.json sur la SD pour savoir si la SD est a jour."""
    h = hashlib.sha1()
    for root, dirs, files in os.walk(theme_dir):
        dirs.sort()
        for name in sorted(files):
            if name.endswith(".raw565") or name == "_index.bin":
                p = os.path.join(root, name)
                h.update(os.path.relpath(p, theme_dir).replace(os.sep, "/").encode("utf-8"))
                with open(p, "rb") as f:
                    h.update(hashlib.sha1(f.read()).digest())
    return h.hexdigest()[:12]


def _load_manifest(out_root):
    try:
        with open(os.path.join(out_root, "manifest.json"), encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def _mark_skipped(manifest, folder, reason, version):
    manifest.setdefault("skipped", {})[folder] = reason
    manifest.setdefault("skipped_versions", {})[folder] = version


def _write_summary(path, changes, total_themes):
    lines = ["## Mise à jour automatique des logos de thèmes (hub Recalbox)", ""]
    titles = (("new", "Nouveaux thèmes"), ("updated", "Thèmes mis à jour"), ("removed", "Thèmes retirés du hub (supprimés du paquet)"),
              ("skipped", "Thèmes sans logo de système exploitable (ignorés)"), ("errors", "Avertissements"))
    for key, title in titles:
        if changes.get(key):
            lines += [f"### {title}", ""] + [f"- {x}" for x in changes[key]] + [""]
    lines += [f"Paquet : {total_themes} thème(s). Les `rev` du manifeste changent uniquement pour les thèmes listés ci-dessus.",
              "", "À vérifier avant de fusionner : planche de contrôle d'un thème modifié (toolkit > Logos de thème > Aperçu) si le rendu a pu changer."]
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def build(out_root, known_dir, only=None, rb_root=None, log=print, incremental=False, summary_path=None):
    known = tl.known_systems_from_dir(known_dir)
    hub = {h["folder"]: h for h in tl.hub_catalog() if h.get("active", True)}
    hub_names = {tl.theme_dir_name(f) for f in hub}
    rb_names = []
    if rb_root and os.path.isdir(rb_root):
        rb_names = [n for n in sorted(os.listdir(rb_root)) if os.path.exists(os.path.join(rb_root, n, "theme.xml"))]
    folders = [f for f in dict.fromkeys(list(hub) + rb_names) if not only or f in only]
    os.makedirs(out_root, exist_ok=True)

    old = _load_manifest(out_root) if incremental else None
    reuse = bool(old and old.get("format") == PACKAGE_FORMAT and old.get("pipeline") == tl.PIPELINE_VERSION)
    if incremental and old and not reuse:
        log("format/pipeline different du paquet existant : reconstruction complete")
    old_themes = old.get("themes", {}) if reuse else {}
    old_skipped = old.get("skipped", {}) if reuse else {}
    old_skipped_v = old.get("skipped_versions", {}) if reuse else {}

    manifest = {"format": PACKAGE_FORMAT, "pipeline": tl.PIPELINE_VERSION, "generated": time.strftime("%Y-%m-%d %H:%M"), "themes": {}}   # heure incluse
    changes = {"new": [], "updated": [], "removed": [], "skipped": [], "errors": []}
    for folder in folders:
        name = tl.theme_dir_name(folder)
        hv = hub[folder].get("version") if folder in hub else None
        # --- incremental : version du hub inchangee => rien a refaire
        if reuse and hv is not None:
            o = old_themes.get(name)
            if o and str(o.get("version")) == str(hv) and os.path.isdir(os.path.join(out_root, name)):
                manifest["themes"][name] = o
                log(f"[{folder}] v{hv} inchange")
                continue
            if folder in old_skipped and str(old_skipped_v.get(folder)) == str(hv):
                _mark_skipped(manifest, folder, old_skipped[folder], hv)
                log(f"[{folder}] v{hv} toujours sans logo exploitable")
                continue
        src, origin, rf = _open_source(folder, hub, rb_root)
        if not src:
            log(f"[{folder}] source introuvable, ignore")
            if name in old_themes:
                manifest["themes"][name] = old_themes[name]
                changes["errors"].append(f"{folder} : source introuvable, version précédente conservée")
            continue
        by_lang = {}
        for code, lang in LANGS.items():
            by_lang[code] = tl.find_logos(src, known, tl.prefer_keys(lang, REGIONS[code]))[0]
        base = by_lang["en"]
        if not base:
            log(f"[{folder}] aucun logo de systeme : ignore")
            if name in old_themes:      # avait des logos avant : ne pas les perdre sur un echec de detection, a examiner
                manifest["themes"][name] = old_themes[name]
                changes["errors"].append(f"{folder} v{hv} : plus aucun logo détecté, version précédente conservée (à examiner)")
            else:
                _mark_skipped(manifest, folder, NO_LOGO_REASON, hv)   # affiche par le toolkit (SANS LOGOS)
                if str(old_skipped_v.get(folder)) != str(hv):
                    changes["skipped"].append(f"{folder} v{hv}")
            continue
        out = os.path.join(out_root, name)
        if os.path.isdir(out):
            shutil.rmtree(out)          # reconstruction complete du theme : aucun fichier obsolete (logos supprimes, anciennes surcharges)
        hub_info = {"hub_version": hv} if folder in hub else {}
        hub_info["packaged_from"] = origin
        tl.convert_theme(src, out, known, log=lambda s, f=folder: log(f"[{f}] {s}"), hub_info=hub_info,
                         source_label="package", prefer=tl.prefer_keys(LANGS["en"]))
        base_files = sorted(f for f in os.listdir(out) if f.endswith(".raw565"))
        entry = {"version": hv, "packaged_from": origin, "variants": tl.detect_variants(src, known),
                 "files": base_files, "overlays": {}, "bytes": sum(os.path.getsize(os.path.join(out, f)) for f in base_files)}
        for code in ("fr", "es"):
            diff = {sid: f for sid, f in by_lang[code].items() if base.get(sid) != f}
            if not diff:
                continue
            odir = os.path.join(out, code)
            os.makedirs(odir, exist_ok=True)
            written = []
            for sid, rel in sorted(diff.items()):
                data = tl.convert_bytes(src.read(rel), rel.lower().endswith(".svg"))
                names = [sid]
                a = _alias_name(sid, set(base))
                if a:
                    names.append(a)
                for n in names:
                    with open(os.path.join(odir, n + ".raw565"), "wb") as f:
                        f.write(data)
                    written.append(n + ".raw565")
            entry["overlays"][code] = sorted(written)
            entry["bytes"] += sum(os.path.getsize(os.path.join(odir, w)) for w in written)
        if rf is not None:
            log(f"[{folder}] telecharge : {rf.fetched / 1e6:.1f} Mo")
        entry["rev"] = theme_rev(out)
        try:
            os.unlink(os.path.join(out, "_source.json"))    # ecrit par convert_theme (horodatage) : non publie, le toolkit le recree sur la SD
        except OSError:
            pass
        manifest["themes"][name] = entry
        old_entry = old_themes.get(name)
        if old_entry is None:
            changes["new"].append(f"{folder} v{hv}")
        elif old_entry.get("rev") != entry["rev"] or str(old_entry.get("version")) != str(hv):
            changes["updated"].append(f"{folder} v{old_entry.get('version')} → v{hv}" + ("" if old_entry.get("rev") != entry["rev"] else " (contenu identique)"))
        log(f"[{folder}] base {len(base_files)} fichiers ; surcharges : " + (", ".join(f"{k}={len(v)}" for k, v in entry["overlays"].items()) or "aucune"))

    # --- themes d'un ancien paquet non traites ici (filtre --only) : conserves ; retires du hub : supprimes
    if reuse:
        for name, o in old_themes.items():
            if name in manifest["themes"]:
                continue
            if name in hub_names or any(tl.theme_dir_name(f) == name for f in rb_names):
                manifest["themes"][name] = o
            else:
                shutil.rmtree(os.path.join(out_root, name), ignore_errors=True)
                changes["removed"].append(name)
        for f, why in old_skipped.items():
            if f not in manifest.get("skipped", {}) and f in hub and (only and f not in only):
                _mark_skipped(manifest, f, why, old_skipped_v.get(f))

    dirty = (not incremental) or any(changes[k] for k in ("new", "updated", "removed", "skipped")) \
        or manifest.get("skipped", {}) != old_skipped or manifest.get("skipped_versions", {}) != old_skipped_v
    if incremental and not dirty:
        log("aucun changement : manifeste inchange")
    else:
        with open(os.path.join(out_root, "manifest.json"), "w", encoding="utf-8") as f:
            json.dump(manifest, f, ensure_ascii=False, indent=1)
    total = sum(e["bytes"] for e in manifest["themes"].values())
    log(f"manifest : {len(manifest['themes'])} themes, {total / 1e6:.1f} Mo")
    if summary_path:
        _write_summary(summary_path, changes, len(manifest["themes"]))
    return manifest


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out")
    ap.add_argument("--known", default=os.path.join("sd_card", "systems", "_defaults"))
    ap.add_argument("--only", default="")
    ap.add_argument("--rb", default=None, help="themes installes sur une Recalbox (hors hub) a ajouter ; par defaut : hub uniquement")
    ap.add_argument("--incremental", action="store_true", help="ne reconstruit que les themes dont la version du hub a change")
    ap.add_argument("--summary", default=None, help="fichier Markdown de compte rendu des changements")
    a = ap.parse_args()
    only = {x for x in a.only.split(",") if x} or None
    build(a.out, a.known, only, a.rb, incremental=a.incremental, summary_path=a.summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
