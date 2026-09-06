#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v4
#
# v4 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel (retour
#   utilisateur : navigation turbo -> CPU sature 100% sur tous les coeurs,
#   temperature grimpant a 65C+, ralentissement generalise RB1 (video,
#   MQTT...) proportionnel a la duree de navigation). Cause trouvee par
#   monitoring `top` repete pendant une rafale reelle : ce script est
#   invoque directement par EmulationStation lui-meme (PPID = PID d'ES,
#   confirme par `ps`), a CHAQUE evenement gamelistbrowsing, avec la
#   convention d'arguments `-action gamelistbrowsing -statefile <path>
#   -param <path>` -- JAMAIS concue pour ce fichier (main() attend des
#   positionnels system/rom, convention de dmd_score.sh qui l'appelle
#   correctement en parallele). Consequence : sys.argv[1]="-action",
#   sys.argv[2]="gamelistbrowsing" -- resolve() echoue forcement (pas un
#   vrai rom), MAIS load_manifest() (parse de hiscore_manifest.json, 432Ko)
#   s'execute quand meme AVANT cet echec -- cout complet paye pour rien, a
#   CHAQUE survol de liste, sans la moindre limite de frequence. Pendant
#   une rafale de navigation rapide (dizaines d'evenements/s), ca produit
#   un essaim de dizaines de processus Python concurrents (confirme par
#   `top` : chaque invocation demarre un interprete + parse le manifeste),
#   cause probable dominante de la saturation CPU observee -- independante
#   de marquee.sh/dmd_score.sh (confirme par test d'isolation : meme
#   comportement avec les 2 completement arretes). Fix chirurgical : sortie
#   immediate en tete de main() si le 1er argument commence par "-"
#   (signature exclusive de l'appel natif ES, jamais utilisee par
#   dmd_score.sh) -- AVANT tout parsing de manifeste. Rend les appels ES
#   natifs quasi gratuits (juste le demarrage Python) sans toucher au
#   chemin legitime dmd_score.sh. Origine exacte de cet appel natif ES
#   (hook RecalBox officiel ?) non elucidee -- ce fix protege sans en
#   dependre.
# v3 - 2026-08-23 - safe-modify - decode_int() applique desormais un
#   facteur d'echelle optionnel (`field["scale"]`, defaut 1) apres decodage
#   BCD/binaire. Necessaire pour les jeux qui stockent le score sous forme
#   REDUITE (ex. en milliers, 1 seul octet BCD = 2 chiffres, x1000 pour
#   obtenir le score reel affiche) -- decouvert en verifiant `ikari` contre
#   une vraie capture d'ecran (RetroArch, commande reseau SCREENSHOT) :
#   l'ancienne heuristique statistique (Phase 2) ne testait jamais de champ
#   score aussi etroit (1-2 octets), se rabattait sur un champ 3-4 octets
#   mal aligne qui mordait sur le premier octet du NOM adjacent (lu comme
#   un chiffre BCD parasite) -- explique le motif "suffixe parasite apres
#   un score par ailleurs correct" observe sur plusieurs jeux cette nuit.
#   Nouvel outil `build_entry_from_truth.py` (scratchpad) : construit une
#   entree DIRECTEMENT a partir d'un score+nom verifies a l'ecran (recherche
#   exhaustive score/echelle + localisation du nom + stride par repetition
#   du nom), au lieu de deviner statistiquement puis corriger a posteriori.
#
# v2 - 2026-08-23 - safe-modify - Chemins MAME corriges (voir commentaire
#   HI_SEARCH_PATHS plus bas) + mame0278/mame0274/mame0258/mame2000 ajoutes
#   -- la version precedente ne trouvait AUCUN .hi MAME (chemins jamais
#   verifies en direct, structure reelle decouverte cette nuit en batissant
#   direct_harvest_mame0278.py). Branche par dmd_score.sh v27 (le gate
#   `[ "$sys" = "fbneo" ]` qui bloquait tout jeu MAME avant meme d'atteindre
#   ce script est retire).
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Phase 1 du chantier
#   hi-score generique MAME/FBNeo (demande utilisateur explicite : projet
#   destine a etre distribue a toute la communaute Recalbox, pas un usage
#   perso -- besoin d'une methode fiable a l'echelle plutot que du RAM
#   live jeu par jeu). Genere a partir de hiscore_manifest.json, lui-meme
#   produit par hi2txt_convert.py (voir ce fichier) a partir du depot
#   communautaire hi2txt-xml (GreatStoneEx/hi2txt-xml, ~3100 jeux
#   documentes, fige depuis fevrier 2022 mais reutilisable tel quel).
#   Valide contre 6 vrais fichiers .hi de jeux FBNeo reellement presents
#   sur la RB de l'utilisateur (afighter/aliens/avsp/bermudat/mslug2/
#   sonicwi2) -- tous decodent des scores/noms plausibles. PAS ENCORE
#   branche dans dmd_score.sh (round_robin()/publish_one_panel()) -- reste
#   un decodeur autonome pour l'instant, appelable en ligne de commande
#   comme dmd_challenge.py.
"""dmd_hiscore_generic.py <system> <rom>

Decode le hi-score d'un jeu via le manifeste hi2txt-xml converti
(hiscore_manifest.json, ~2758 jeux) + le fichier .hi sauvegarde localement
par l'emulateur -- AUCUNE lecture RAM live, uniquement le fichier ecrit a
la fin de partie (voir memoire projet 2026-08-23 : approche retenue pour
la Phase 1 "hi-score generique", plus fiable a l'echelle communautaire que
la RAM live qui demande une recette par jeu construite a la main).

Meme convention de sortie que les autres decodeurs (dmd_challenge.py,
decode_1941_topN() en dur dans dmd_score.sh) : une ligne "rang nom score"
par rang, jointes par "|", silencieux (rien affiche) si le jeu n'est pas
dans le manifeste, si le fichier .hi est absent, ou si la taille ne
correspond pas a la definition attendue (prudence : mieux vaut ne rien
afficher qu'afficher un decodage errone).
"""
import json
import os
import re
import sys
import unicodedata

MANIFEST_PATH = os.path.join(os.path.dirname(__file__), "hiscore_manifest.json")

# Chemins de sauvegarde .hi connus par core -- a completer au fil de l'eau
# si d'autres cores/emulateurs sont utilises.
# v2 (2026-08-23) -- BUG REEL corrige : les chemins MAME d'origine
# ("mame2003-plus/mame2003-plus/...") etaient FAUX (jamais verifies en
# direct) -- vrais chemins confirmes sur materiel cette nuit en construisant
# direct_harvest_mame0278.py : TOUS les cores MAME vivent sous
# "/recalbox/share/saves/mame/<core>/..." (pas "<core>/<core>/"), avec un
# sous-dossier "hiscore" pour les cores recents a plugin Lua (mame0278,
# confirme -- mame0274/mame0258 memes generations, memes plugins, motif
# identique par extrapolation) et "hi" pour les cores plus anciens
# (mame2003-plus confirme via find reel, 31 vrais .hi presents ; 2003/2010/
# 2015/2000 memes generation que 2003-plus, motif identique par
# extrapolation, pas individuellement reverifies). fbneo reste confirme et
# inchange.
HI_SEARCH_PATHS = [
    "/recalbox/share/saves/fbneo/fbneo/{rom}.hi",
    "/recalbox/share/saves/mame/mame0278/hiscore/{rom}.hi",
    "/recalbox/share/saves/mame/mame0274/hiscore/{rom}.hi",
    "/recalbox/share/saves/mame/mame0258/hiscore/{rom}.hi",
    "/recalbox/share/saves/mame/mame2003-plus/hi/{rom}.hi",
    "/recalbox/share/saves/mame/mame2010/hi/{rom}.hi",
    "/recalbox/share/saves/mame/mame2015/hi/{rom}.hi",
    "/recalbox/share/saves/mame/mame2003/hi/{rom}.hi",
    "/recalbox/share/saves/mame/mame2000/hi/{rom}.hi",
]

MAX_ENTRIES = 9  # aligne avec dmd_challenge.py (3 pages x 3 lignes)


def find_hi_file(rom):
    for pattern in HI_SEARCH_PATHS:
        path = pattern.format(rom=rom)
        if os.path.isfile(path):
            return path
    return None


def load_manifest():
    with open(MANIFEST_PATH, encoding="utf-8") as f:
        return json.load(f)


def resolve(manifest, rom, depth=0):
    """Suit une chaine <sameas> (max 5 sauts, largement suffisant --
    aucune chaine >1 saut observee dans l'echantillon converti)."""
    if depth > 5 or rom not in manifest:
        return None
    entry = manifest[rom]
    if "alias_of" in entry:
        return resolve(manifest, entry["alias_of"], depth + 1)
    return entry


def decode_int(data, off, field):
    size = field["size"]
    raw = data[off:off + size]
    if len(raw) < size:
        return None
    if field.get("endian") == "little":
        raw = raw[::-1]
    trim = field.get("byte_trim")
    if trim is not None:
        # v1 -- octet de "case vide" (ex. tuile blanc 0x24 sur galaga)
        # traite comme un chiffre "0" pour les positions non utilisees --
        # meme principe que decode_galaga_topscore() deja en prod dans
        # dmd_score.sh. Pas verifie sur tous les jeux utilisant byte-trim,
        # a affiner si un decodage errone est constate.
        raw = bytes(0 if b == trim else b for b in raw)
    scale = field.get("scale", 1)
    if field.get("format") == "bcd":
        digits = ""
        for b in raw:
            hi, lo = b >> 4, b & 0xF
            if hi > 9 or lo > 9:
                return None
            digits += str(hi) + str(lo)
        v = int(digits) if digits else 0
        return v * scale
    v = 0
    for b in raw:
        v = v * 256 + b
    return v * scale


def decode_text(data, off, field, charsets):
    size = field["size"]
    raw = data[off:off + size]
    if len(raw) < size:
        return ""
    cs = charsets.get(field.get("charset"))
    ascii_offset = field.get("ascii_offset", 0)
    out = []
    for b in raw:
        if cs is not None and str(b) in cs:
            # v1 -- BUG REEL corrige avant tout deploiement (teste contre
            # un vrai echantillon afighter.hi) : la table <charset> de
            # hi2txt-xml ne couvre souvent que quelques octets SPECIAUX
            # (espace/./? -- tuiles decoratives), PAS l'alphabet complet --
            # les lettres normales du nom (deja de l'ASCII standard cote
            # jeu) ne sont JAMAIS listees. Les traiter comme "absentes du
            # charset -> vide" faisait disparaitre le nom entier (vu :
            # "?" au lieu de "DA"). Fix : la table ne s'applique QUE sur
            # les octets qu'elle liste explicitement, tout le reste passe
            # en ASCII litteral (meme branche que "pas de charset").
            out.append(cs[str(b)])
        else:
            v = b + ascii_offset
            out.append(chr(v) if 32 <= v < 127 else "")
    return "".join(out)


def clean_name(raw):
    name = unicodedata.normalize("NFKD", raw or "").encode("ascii", "ignore").decode("ascii")
    name = name.upper()
    name = re.sub(r"[^A-Z0-9 ]", "", name).strip()
    return name[:10] if name else "?"


def decode_entry_group(data, group, charsets):
    """mode combined/score_only : un seul groupe de champs par rang,
    SCORE et (facultativement) NAME co-localises."""
    results = []
    for i in range(group["count"]):
        base = group["start_offset"] + i * group["stride"]
        score = None
        name = ""
        for f in group["fields"]:
            off = base + f["rel_offset"]
            fid = f["id"].upper()
            if f["kind"] == "int" and "SCORE" in fid:
                v = decode_int(data, off, f)
                if v is not None:
                    score = v
            elif f["kind"] == "text" and "NAME" in fid:
                name = decode_text(data, off, f, charsets)
        if score is not None:
            results.append((score, name))
    return results


def decode_separate(data, entry, charsets):
    """mode separate : boucle SCORE et boucle NAME distinctes, meme
    nombre d'entrees (galaga : 2 <loop count="5"> successives)."""
    score_grp = entry["score"]
    name_grp = entry["name"]
    results = []
    for i in range(score_grp["count"]):
        sbase = score_grp["start_offset"] + i * score_grp["stride"]
        score = None
        for f in score_grp["fields"]:
            if f["kind"] == "int":
                v = decode_int(data, sbase + f["rel_offset"], f)
                if v is not None:
                    score = v
        nbase = name_grp["start_offset"] + i * name_grp["stride"]
        name = ""
        for f in name_grp["fields"]:
            if f["kind"] == "text":
                name = decode_text(data, nbase + f["rel_offset"], f, charsets)
        if score is not None:
            results.append((score, name))
    return results


def main():
    if len(sys.argv) < 3:
        return
    # v4 -- voir changelog v4 en tete de fichier : sortie immediate si
    # invoque avec la convention native ES ("-action ..."), jamais celle de
    # dmd_score.sh (positionnels system/rom, jamais prefixes par "-") --
    # evite le cout complet de load_manifest() (parse 432Ko) pour un appel
    # structurellement voue a l'echec.
    if sys.argv[1].startswith("-"):
        return
    system, rom = sys.argv[1], sys.argv[2]
    try:
        manifest = load_manifest()
    except (OSError, json.JSONDecodeError):
        return
    entry = resolve(manifest, rom)
    if entry is None:
        return

    hi_path = find_hi_file(rom)
    if hi_path is None:
        return
    try:
        with open(hi_path, "rb") as f:
            data = f.read()
    except OSError:
        return

    expected = entry.get("expected_size")
    if expected is not None and len(data) != expected:
        return

    charsets = entry.get("charsets", {})
    all_results = []
    for group in entry["entries"]:
        mode = group["mode"]
        if mode in ("combined", "score_only"):
            all_results.extend(decode_entry_group(data, group, charsets))
        elif mode == "separate":
            all_results.extend(decode_separate(data, group, charsets))

    if not all_results:
        return

    all_results.sort(key=lambda t: -t[0])
    lines = []
    for i, (score, name) in enumerate(all_results[:MAX_ENTRIES], start=1):
        lines.append(f"{i} {clean_name(name)} {score}")
    print("|".join(lines))


if __name__ == "__main__":
    main()
