# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Outil DE DEVELOPPEMENT
#   (pas execute au runtime sur le DMD/RB -- sert uniquement a REGENERER
#   hiscore_manifest.json, deja versionne dans le depot, si hi2txt-xml
#   evolue un jour ou si le convertisseur est ameliore). Ne s'execute pas
#   tel quel : necessite un clone local de hi2txt-xml a cote de ce script
#   (git clone --depth 1 https://github.com/GreatStoneEx/hi2txt-xml.git),
#   PAS inclus dans ce depot (depot tiers, ~8449 fichiers). Resultat de la
#   1ere generation (2026-08-23) : 673 definitions reelles + 2085 alias
#   resolus = ~2758 jeux/2/3102 fichiers XML source (~89%), voir memoire
#   projet pour le detail complet de la demarche et des validations.
"""Convertit les definitions hi2txt-xml (structure file=".hi" uniquement --
perimetre arcade MAME/FBNeo) en un manifeste JSON exploitable directement,
sans reimplementer le moteur hi2txt complet.

Gere : loop/elt sequentiels (avec loops imbriques a un seul niveau), types
int (base=16 ou decoding-profile="bcd-le") et text (charset optionnel,
ascii-offset, byte-trim), <sameas> (alias resolu vers la definition reelle).

Ignore/signale (pas assez de volume pour justifier l'effort, ou risque de
mal-decodage silencieux) : bitmask, table-index, multiples <structure>
ambigues sans <check><size> exploitable, boucles imbriquees >1 niveau,
formats de sortie <format> non triviaux (garde le id brut).
"""
import glob
import json
import os
import re
import sys
from xml.etree import ElementTree as ET

DB_DIR = os.path.join(os.path.dirname(__file__), "hi2txt-xml", "src", "main", "db")
OUT_PATH = os.path.join(os.path.dirname(__file__), "hiscore_manifest.json")
REPORT_PATH = os.path.join(os.path.dirname(__file__), "hiscore_manifest_report.txt")


CUSTOM_ENTITY_RE = re.compile(r"&(?!amp;|lt;|gt;|quot;|apos;)[a-zA-Z][a-zA-Z0-9_-]*;")


def strip_doctype(text):
    # ElementTree tente de resoudre le DOCTYPE externe (hi2txt.dtd, absent
    # localement) -- on le retire, il n'apporte aucune info utile ici.
    text = re.sub(r"<!DOCTYPE[^>]*>", "", text)
    # ~180 entites personnalisees (glyphes decoratifs : coeurs/etoiles/
    # zodiaque/kana japonais...) utilisees comme valeurs dst="" dans les
    # tables <charset> -- non resolvables sans le hi2txt.dtd d'origine, et
    # de toute facon non affichables par la police DMD (5x7 ASCII de base).
    # Remplacees par un caractere de repli generique : ca laisse le XML
    # parsable sans fausser la structure (juste une valeur d'attribut
    # simplifiee), au lieu de perdre le fichier entier pour une entite non
    # critique pour notre usage.
    text = CUSTOM_ENTITY_RE.sub("?", text)
    return text


def parse_int_attr(v, default=0):
    if v is None:
        return default
    return int(v)


def load_charset(root):
    """<charset id="X"><char src="0xNN" dst="c"/>...</charset> -- retourne
    {charset_id: {byte_value: dst_string}}."""
    out = {}
    for cs in root.findall("charset"):
        cid = cs.get("id")
        if not cid:
            continue
        table = {}
        for ch in cs.findall("char"):
            src = ch.get("src")
            dst = ch.get("dst", "")
            if src is None:
                continue
            try:
                val = int(src, 16) if src.lower().startswith("0x") else int(src)
            except ValueError:
                continue
            table[val] = dst
        out[cid] = table
    return out


def elt_to_field(elt):
    """Un <elt> -> dict decrivant comment le decoder, ou None si type non
    gere (raw pur, souvent juste un padding/pointeur ignore par le hi2txt
    d'origine lui-meme via display='debug')."""
    typ = elt.get("type")
    size = parse_int_attr(elt.get("size"), 1)
    fid = elt.get("id", "")
    if typ == "int":
        base = elt.get("base")
        profile = elt.get("decoding-profile")
        endian = elt.get("endianness", "big_endian")
        byte_trim = elt.get("byte-trim")
        field = {
            "kind": "int",
            "id": fid,
            "size": size,
            "endian": "little" if "little" in endian else "big",
        }
        if profile == "bcd-le":
            field["format"] = "bcd"
            field["endian"] = "little"
        elif base == "16":
            field["format"] = "bcd"  # meme decodage que nos scripts prod : nibbles = chiffres
        else:
            field["format"] = "binary"
        if byte_trim is not None:
            try:
                field["byte_trim"] = int(byte_trim, 16) if byte_trim.lower().startswith("0x") else int(byte_trim)
            except ValueError:
                pass
        return field
    if typ == "text":
        field = {
            "kind": "text",
            "id": fid,
            "size": size,
        }
        cs = elt.get("charset")
        if cs:
            field["charset"] = cs
        ao = elt.get("ascii-offset")
        if ao is not None:
            try:
                field["ascii_offset"] = int(ao)
            except ValueError:
                pass
        bs = elt.get("byte-skip")
        if bs:
            field["byte_skip"] = bs
        return field
    # raw / autre : occupe de la place dans le flux mais pas de donnee utile
    return {"kind": "raw", "id": fid, "size": size}


def walk_structure(structure_elt):
    """Parcourt sequentiellement structure/(loop|elt), calcule les offsets
    cumules. Retourne (fields_flat, loops_info) ou fields_flat est une liste
    de (offset, field_dict) et loops_info decrit les boucles top-level
    trouvees : [{start_offset, count, elt_size, fields:[(rel_offset,field)]}]."""
    offset = 0
    flat = []
    loops = []
    for child in structure_elt:
        tag = child.tag
        if tag == "check":
            continue
        if tag == "loop":
            count = parse_int_attr(child.get("count"), 1)
            # elts DIRECTS de cette boucle (pas de sous-loop geree -- rare/absent
            # dans l'echantillon observe pour nos jeux cibles)
            inner_fields = []
            inner_offset = 0
            skip_inner = False
            for sub in child:
                if sub.tag != "elt":
                    skip_inner = True  # sous-loop imbriquee : trop rare, on renonce sur CE loop
                    break
                f = elt_to_field(sub)
                inner_fields.append((inner_offset, f))
                inner_offset += f["size"]
            if skip_inner:
                loops.append({"unsupported": "nested-loop", "start_offset": offset})
                # on ne peut plus calculer l'offset total en confiance -> abandon du fichier
                return None, None
            loop_total = inner_offset * count
            loops.append({
                "start_offset": offset,
                "count": count,
                "elt_size": inner_offset,
                "fields": inner_fields,
            })
            offset += loop_total
        elif tag == "elt":
            f = elt_to_field(child)
            flat.append((offset, f))
            offset += f["size"]
        # autres tags (rien de connu) ignores
    return flat, loops


def pick_structure(root):
    """Choisit la <structure file=".hi"> -- s'il y en a plusieurs (variantes
    selon check/size), prend la 1ere (le hi2txt reel choisirait via
    <check><size>/<definition> compare au fichier reel -- on n'a pas cette
    logique ici, 1ere definition = comportement le plus courant observe)."""
    structures = [s for s in root.findall("structure") if s.get("file") == ".hi"]
    return structures[0] if structures else None


def convert_one(path, charsets_global):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
    text = strip_doctype(text)
    try:
        root = ET.fromstring(text)
    except ET.ParseError as e:
        return None, f"xml-parse-error: {e}"

    sameas = root.find("sameas")
    if sameas is not None:
        return {"alias_of": sameas.get("id")}, None

    structure = pick_structure(root)
    if structure is None:
        return None, "no-.hi-structure"

    check = structure.find("check")
    expected_size = None
    if check is not None:
        size_el = check.find("size")
        if size_el is not None and size_el.text:
            try:
                expected_size = int(size_el.text.strip())
            except ValueError:
                pass

    local_charsets = load_charset(root)

    flat, loops = walk_structure(structure)
    if flat is None:
        return None, "unsupported-nested-loop"

    # Trouve la (les) boucle(s) contenant un id ressemblant a un score ET
    # une boucle (ou la meme) contenant un nom -- schema le plus courant :
    # soit UNE boucle avec SCORE+NAME dans les memes elts (galaga: 2 loops
    # separees mais meme count -- gyruss: 1 loop avec tout dedans).
    score_loops = []
    name_loops = []
    for lp in loops:
        ids = [f["id"].upper() for _, f in lp["fields"]]
        has_score = any("SCORE" in i for i in ids)
        has_name = any("NAME" in i for i in ids)
        if has_score:
            score_loops.append(lp)
        if has_name:
            name_loops.append(lp)

    if not score_loops:
        # v2 -- cas "score unique sans boucle" (ex. agress : un seul
        # <elt id="SCORE"> hors <loop>, pas de classement multi-rang, pas
        # de nom -- meme famille que le "TOP SCORE" deja gere ailleurs,
        # mais ici c'est le SEUL contenu du fichier). Traite comme un
        # classement a 1 seule entree, nom vide.
        top_score_flat = None
        for off, f in flat:
            if "SCORE" in f["id"].upper():
                top_score_flat = (off, f)
                break
        if top_score_flat is None:
            return None, "no-score-loop-found"
        off, f = top_score_flat
        manifest_charsets = {}
        return {
            "expected_size": expected_size,
            "entries": [{
                "mode": "score_only",
                "start_offset": off,
                "count": 1,
                "stride": f["size"],
                "fields": [{"rel_offset": 0, **f}],
            }],
            "top_score": None,
            "charsets": manifest_charsets,
        }, None

    # cas A : une seule boucle contient a la fois SCORE et NAME (gyruss-like)
    combined = None
    for lp in score_loops:
        ids = [f["id"].upper() for _, f in lp["fields"]]
        if any("NAME" in i for i in ids):
            combined = lp
            break

    entries = []
    if combined is not None:
        entries.append({
            "mode": "combined",
            "start_offset": combined["start_offset"],
            "count": combined["count"],
            "stride": combined["elt_size"],
            "fields": [{"rel_offset": ro, **f} for ro, f in combined["fields"]],
        })
    elif not name_loops:
        # v2 -- cas "classement sans aucun nom" (ex. atlantis : boucle de
        # 10 scores, jamais de champ NAME nulle part dans le fichier) --
        # classement valide quand meme, juste sans identite de joueur.
        score_lp = score_loops[0]
        entries.append({
            "mode": "score_only",
            "start_offset": score_lp["start_offset"],
            "count": score_lp["count"],
            "stride": score_lp["elt_size"],
            "fields": [{"rel_offset": ro, **f} for ro, f in score_lp["fields"]],
        })
    else:
        # cas B : boucle SCORE separee de la boucle NAME, meme count (galaga-like)
        score_lp = score_loops[0]
        name_lp = name_loops[0] if name_loops else None
        if name_lp is None or name_lp["count"] != score_lp["count"]:
            return None, "score-name-loop-mismatch"
        entries.append({
            "mode": "separate",
            "score": {
                "start_offset": score_lp["start_offset"],
                "count": score_lp["count"],
                "stride": score_lp["elt_size"],
                "fields": [{"rel_offset": ro, **f} for ro, f in score_lp["fields"]],
            },
            "name": {
                "start_offset": name_lp["start_offset"],
                "count": name_lp["count"],
                "stride": name_lp["elt_size"],
                "fields": [{"rel_offset": ro, **f} for ro, f in name_lp["fields"]],
            },
        })

    # champs top-score hors boucle (facultatif, juste pour info)
    top_score = None
    for off, f in flat:
        if "SCORE" in f["id"].upper():
            top_score = {"offset": off, **f}
            break

    if entries[0]["mode"] in ("combined", "score_only"):
        field_list = entries[0]["fields"]
    else:
        field_list = entries[0]["score"]["fields"] + entries[0]["name"]["fields"]
    manifest_charsets = {}
    for f in field_list:
        cs_id = f.get("charset")
        if cs_id and cs_id in local_charsets and cs_id not in manifest_charsets:
            manifest_charsets[cs_id] = {str(k): v for k, v in local_charsets[cs_id].items()}

    return {
        "expected_size": expected_size,
        "entries": entries,
        "top_score": top_score,
        "charsets": manifest_charsets,
    }, None


def main():
    files = sorted(glob.glob(os.path.join(DB_DIR, "*.xml")))
    manifest = {}
    stats = {"ok": 0, "alias": 0, "skipped": {}}
    skip_reasons = {}

    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        result, err = convert_one(path, {})
        if err:
            stats["skipped"][err] = stats["skipped"].get(err, 0) + 1
            skip_reasons.setdefault(err, []).append(name)
            continue
        if "alias_of" in result:
            manifest[name] = result
            stats["alias"] += 1
            continue
        manifest[name] = result
        stats["ok"] += 1

    # resout les alias vers la definition reelle (une seule indirection suffit
    # dans l'ecrasante majorite des cas observes -- pas de chaine profonde geree)
    resolved = 0
    unresolved = 0
    for name, entry in manifest.items():
        if "alias_of" in entry:
            target = entry["alias_of"]
            if target in manifest and "alias_of" not in manifest[target]:
                manifest[name] = {"alias_of": target}  # garde le pointeur, le lecteur suit
                resolved += 1
            else:
                unresolved += 1

    with open(OUT_PATH, "w", encoding="utf-8") as f:
        json.dump(manifest, f, separators=(",", ":"))

    with open(REPORT_PATH, "w", encoding="utf-8") as f:
        f.write(f"Total fichiers XML traites : {len(files)}\n")
        f.write(f"Definitions reelles converties (score+nom exploitable) : {stats['ok']}\n")
        f.write(f"Alias (<sameas>) : {stats['alias']} (dont {unresolved} pointant vers une cible non convertie)\n")
        f.write(f"Ignores : {sum(stats['skipped'].values())}\n")
        for reason, count in sorted(stats["skipped"].items(), key=lambda x: -x[1]):
            f.write(f"  - {reason} : {count}\n")
        f.write("\nExemples ignores par categorie (jusqu'a 10 chacun) :\n")
        for reason, names in skip_reasons.items():
            f.write(f"  {reason}: {', '.join(names[:10])}\n")

    print(f"OK={stats['ok']} alias={stats['alias']} skipped={sum(stats['skipped'].values())}")
    print(f"Manifeste : {OUT_PATH}")
    print(f"Rapport : {REPORT_PATH}")


if __name__ == "__main__":
    main()
