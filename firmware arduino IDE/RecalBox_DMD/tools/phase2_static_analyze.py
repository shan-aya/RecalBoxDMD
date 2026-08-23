# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v4
#
# v4 - 2026-08-23 - safe-modify - BUG REEL corrige (trouve en fusionnant le
#   lot de 613 .hi frais recoltes par direct_harvest.py cette nuit) :
#   `is_valid_bcd4()` valide n'importe quel octet dont les 2 nibbles sont
#   <=9 -- or les octets ASCII imprimables courants (espace 0x20, chiffres
#   '0'-'9' = 0x30-0x39) ont TOUJOURS des nibbles valides par coincidence
#   (0x30='0' -> nibbles 3,0 ; 0x20=espace -> nibbles 2,0), donc un champ
#   texte/remplissage adjacent au vrai score (souvent des espaces ou des
#   "0" de padding) peut passer la validation BCD et etre pris pour un
#   score valide. Symptome observe en nombre sur ce lot : des jeux avec un
#   "score" affichant des motifs clairement issus de texte ASCII brut
#   (ex. 30303030, 20202020) au lieu d'un vrai nombre BCD. Sur 613 fichiers
#   analyses, 126 marques FORTE par l'heuristique d'origine -- verification
#   manuelle poussee (3 passes : rang1=0, filtre "?"/echelle/noms 1-lettre,
#   motif ASCII pur) a fait tomber ce nombre a 80 reellement fiables. Fix
#   : `is_valid_bcd4()` rejette desormais un champ score si TOUS ses octets
#   sont dans la plage remplissage/texte ASCII (0x20 ou 0x30-0x39) --
#   n'exclut quasiment aucun vrai score (un score reel a 4 octets qui
#   tomberait ENTIEREMENT par coincidence dans cette plage etroite est
#   extremement improbable), mais elimine la source la plus frequente de
#   faux-positifs constatee ce soir. Les 3 autres categories de faux-
#   positifs rencontrees (score au rang 1 = 0 -- deja gere par le filtre
#   FORTE existant si combine a une verification manuelle ; noms "?" partout
#   -- charset/decalage faux ; echelle incoherente entre rangs) restent
#   NON automatisees -- nécessitent encore une relecture des resultats
#   avant fusion definitive dans hiscore_manifest.json, ne pas fusionner le
#   fichier de sortie a l'aveugle meme apres ce fix.
#
# v3 - 2026-08-23 - safe-modify - 2 bugs corriges apres validation reelle
#   par l'utilisateur (willow verifie a l'ecran attract, sans avoir besoin
#   de jouer -- "willow 55000 cap") :
#   (1) nom systematiquement tronque quand plusieurs largeurs de nom
#   restaient plausibles (willow : "CA" garde au lieu de "CAP" correct) --
#   name_size ajoute comme critere de choix, prefere le nom le plus long
#   A QUALITE EGALE (pas en priorite absolue).
#   (2) variante "score(4) + 1 octet d'ecart + nom" ajoutee (deja vue sur
#   afighter.xml/gyruss.xml, hi2txt-xml) -- manquait, causait exactement
#   la troncature de willow (l'octet d'ecart etait confondu avec le 1er
#   caractere du nom).
#   Limite CONNUE, PAS corrigee (decouverte sur 64street, exclu du
#   fusionnage pour cette raison ET pour un probleme de score plus grave,
#   voir memoire projet) : "preferer le nom le plus long" peut aussi
#   REGRESSER un jeu dont le vrai nom est plus court si un octet suivant
#   decode par coincidence en lettre ASCII plausible -- pas de solution
#   generale trouvee ce soir, know issue.
#   8 jeux valides et fusionnes dans hiscore_manifest.json (bouldash,
#   gtmr, sidepckt, ssf2t, ssriders, superbon, supmodel, willow) --
#   64street explicitement exclu (score errone, cause non elucidee : le
#   fichier .hi n'utilise pas le meme encodage que la RAM live pour ce
#   jeu specifiquement, voir memoire projet).
"""Phase 2 -- analyse statique autonome (aucun lancement de jeu, aucune
lecture d'ecran humaine) des jeux presents dans hiscore.dat mais absents
du manifeste Phase 1 (hi2txt-xml), a partir de vrais fichiers .hi DEJA
existants sur la carte de l'utilisateur.

Reutilise/etend l'heuristique BCD+ASCII deja construite plus tot cette
session (detect_score_name.py/batch_analyze.py) -- teste plusieurs
decoupages (stride 6-14, score-puis-nom ou nom-puis-score, tailles de nom
2 ou 3), garde le plus plausible, assigne une confiance FORTE/moyenne/
FAIBLE. Sortie : entrees pretes a fusionner dans hiscore_manifest.json
pour les FORTE uniquement -- le reste est signale pour verification
humaine (lecture d'ecran/photo), jamais fusionne a l'aveugle.
"""
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from parse_hiscore_dat import parse_hiscore_dat, compute_offsets  # noqa: E402

SCRATCH = os.path.dirname(__file__)
# v5 - 2026-08-23 - safe-modify - Chemins parametrables (--dat/--hidir/
#   --manifest/--out) au lieu de codes en dur sur FBNeo -- reutilise pour
#   MAME0278 (voir DECISIONS.md "Récolte MAME0278", process complet
#   tranche 1). Defauts inchanges (retro-compatibles avec l'usage FBNeo
#   d'origine sans argument).
DAT_PATH = os.path.join(SCRATCH, "hiscore_fbneo.dat")
HI_DIR = os.path.join(SCRATCH, "phase2_hi")
MANIFEST_PATH = os.path.join(SCRATCH, "userscripts", "hiscore_manifest.json")
OUT_PATH = os.path.join(SCRATCH, "phase2_new_entries.json")

CANDIDATE_STRIDES = [6, 7, 8, 9, 10, 11, 12, 13, 14]
NAME_SIZES = [2, 3]


def is_valid_bcd4(b4):
    for byte in b4:
        if (byte >> 4) > 9 or (byte & 0xF) > 9:
            return False
    # v4 -- rejette un champ dont TOUS les octets sont dans la plage
    # remplissage/texte ASCII (espace 0x20, chiffres '0'-'9' 0x30-0x39) --
    # ces octets ont des nibbles nibble-valides par pure coincidence (voir
    # changelog), la cause la plus frequente de faux score BCD trouvee en
    # pratique sur le lot du 2026-08-23.
    if all(b == 0x20 or 0x30 <= b <= 0x39 for b in b4):
        return False
    return True


def bcd_to_int(bn):
    s = ""
    for byte in bn:
        s += str((byte >> 4) & 0xF) + str(byte & 0xF)
    return int(s) if s else 0


def is_valid_ascii_name(bn):
    for byte in bn:
        c = chr(byte)
        if not (c.isalnum() or c in " .'-"):
            return False
    return True


def analyze_rank(chunk, name_size):
    """Teste score(4)+nom puis nom+score(4), aux 2 offsets possibles.
    Retourne (desc, score, name) ou None."""
    n = len(chunk)
    if n >= 4 + name_size:
        score4, namen = chunk[0:4], chunk[4:4 + name_size]
        if is_valid_bcd4(score4) and is_valid_ascii_name(namen):
            return ("score+nom", bcd_to_int(score4), bytes(namen).decode("ascii", "replace"))
    # v3 -- variante "score(4) + 1 octet d'ecart + nom" -- motif deja vu sur
    # afighter.xml/gyruss.xml (hi2txt-xml), manquait ici (willow tronquait
    # son nom "CAP" -> "CA" en confondant l'octet d'ecart avec le 1er
    # caractere du nom).
    if n >= 5 + name_size:
        score4, gap, namen = chunk[0:4], chunk[4:5], chunk[5:5 + name_size]
        if is_valid_bcd4(score4) and is_valid_ascii_name(namen):
            return ("score+gap+nom", bcd_to_int(score4), bytes(namen).decode("ascii", "replace"))
    if n >= name_size + 1 + 4:
        namen, score4 = chunk[0:name_size], chunk[name_size + 1:name_size + 5]
        if is_valid_ascii_name(namen) and is_valid_bcd4(score4):
            return ("nom+idx+score", bcd_to_int(score4), bytes(namen).decode("ascii", "replace"))
    if n >= name_size + 4:
        namen, score4 = chunk[0:name_size], chunk[name_size:name_size + 4]
        if is_valid_ascii_name(namen) and is_valid_bcd4(score4):
            return ("nom+score", bcd_to_int(score4), bytes(namen).decode("ascii", "replace"))
    return None


def best_candidate(data, block_start, block_len):
    best = None
    for stride in CANDIDATE_STRIDES:
        if stride > block_len:
            continue
        # v2 -- BUG REEL corrige AVANT tout usage (verifie sur 64street,
        # seul cas de verite terrain connue : le decoupage precedent, qui
        # supposait TOUJOURS la 1ere entree pile a block_start, donnait un
        # resultat FAUX). Certains jeux ont un octet (ou plus) d'en-tete/
        # remplissage AVANT la 1ere entree reelle -- decouvert manuellement
        # sur 64street (entree reelle a block_start+1, pas +0). On teste
        # desormais plusieurs decalages de depart, pas seulement 0.
        for shift in range(0, min(stride, 4)):
            start = block_start + shift
            for name_size in NAME_SIZES:
                idx = start
                scores, names = [], []
                order_votes = {}
                n_valid = n_total = 0
                while idx + 4 <= block_start + block_len:
                    chunk = data[idx:idx + stride]
                    if len(chunk) < 4:
                        break
                    n_total += 1
                    r = analyze_rank(chunk, name_size)
                    if r:
                        n_valid += 1
                        desc, score, name = r
                        scores.append(score)
                        names.append(name.strip())
                        order_votes[desc] = order_votes.get(desc, 0) + 1
                    idx += stride
                if n_total == 0 or n_valid == 0:
                    continue
                non_increasing = sum(1 for i in range(1, len(scores)) if scores[i] <= scores[i - 1])
                plausibility = non_increasing / max(1, len(scores) - 1) if len(scores) > 1 else (1 if scores else 0)
                ratio_valid = n_valid / n_total
                nonempty_names = sum(1 for n in names if n)
                name_ratio = nonempty_names / max(1, len(names))
                top_order = max(order_votes, key=order_votes.get) if order_votes else "?"
                # v3 -- BUG REEL corrige (retour utilisateur : willow donnait
                # score juste (55000) mais nom tronque "CA" au lieu de "CAP")
                # -- name_size=3 valide etait ecarte au profit de name_size=2
                # qui "collait" aussi mais moins bien. name_size ajoute comme
                # dernier critere de choix : a qualite egale (ratio_valid *
                # plausibilite * ratio_noms-non-vides), preferer le nom LE
                # PLUS LONG qui reste valide -- un nom de 3 caracteres propre
                # est une preuve plus fiable/informative qu'un nom de 2.
                score_key = (round(ratio_valid * plausibility * (0.5 + 0.5 * name_ratio), 3), name_size, ratio_valid, n_valid)
                if best is None or score_key > best[0]:
                    best = (score_key, stride, name_size, top_order, n_valid, n_total,
                            ratio_valid, plausibility, name_ratio, scores[:5], names[:5], shift)
    return best


def offset_for_order(order, name_size):
    if order == "score+nom":
        return 0, 4
    if order == "score+gap+nom":
        return 0, 5
    if order == "nom+idx+score":
        return name_size + 1, 0
    if order == "nom+score":
        return name_size, 0
    return None, None


def main():
    dat_path = DAT_PATH
    hi_dir = HI_DIR
    manifest_path = MANIFEST_PATH
    out_path = OUT_PATH
    if "--dat" in sys.argv:
        dat_path = sys.argv[sys.argv.index("--dat") + 1]
    if "--hidir" in sys.argv:
        hi_dir = sys.argv[sys.argv.index("--hidir") + 1]
    if "--manifest" in sys.argv:
        manifest_path = sys.argv[sys.argv.index("--manifest") + 1]
    if "--out" in sys.argv:
        out_path = sys.argv[sys.argv.index("--out") + 1]

    entries = parse_hiscore_dat(dat_path)
    hi_files = sorted(glob.glob(os.path.join(hi_dir, "*.hi")))
    with open(manifest_path, encoding="utf-8") as f:
        manifest = json.load(f)

    new_entries = {}
    print(f"{'JEU':12s} {'CONF':7s} {'STRIDE':6s} {'NOM':4s} {'ORDRE':14s} TOP3")
    print("=" * 100)
    for path in hi_files:
        name = os.path.splitext(os.path.basename(path))[0]
        gl = name.lower()
        if gl not in entries:
            print(f"{name:12s} -- absent de hiscore.dat (inattendu)")
            continue
        blocks = entries[gl]
        offsets = compute_offsets(blocks)
        if not offsets:
            continue
        block_start, block_len = offsets[0][0], offsets[0][1]
        with open(path, "rb") as f:
            data = f.read()
        if len(data) < block_start + 4:
            print(f"{name:12s} -- fichier trop court")
            continue
        result = best_candidate(data, block_start, block_len)
        if result is None:
            print(f"{name:12s} FAIBLE  -- aucun decoupage plausible trouve")
            continue
        (score_key, stride, name_size, order, n_valid, n_total,
         ratio_valid, plausibility, name_ratio, scores, names, shift) = result
        conf = "FORTE" if (ratio_valid > 0.7 and plausibility > 0.7 and name_ratio > 0.6 and n_valid >= 3) \
            else ("moyenne" if ratio_valid > 0.4 else "FAIBLE")
        top3 = list(zip(scores[:3], names[:3]))
        print(f"{name:12s} {conf:7s} stride={stride:<3d} nom={name_size:<2d} decalage={shift:<2d} {order:14s} {top3}")

        if conf == "FORTE":
            score_off, name_off = offset_for_order(order, name_size)
            new_entries[gl] = {
                "expected_size": None,
                "entries": [{
                    "mode": "combined",
                    "start_offset": block_start + shift,
                    "count": n_total,
                    "stride": stride,
                    "fields": [
                        {"rel_offset": score_off, "kind": "int", "id": "SCORE", "size": 4,
                         "endian": "big", "format": "bcd"},
                        {"rel_offset": name_off, "kind": "text", "id": "NAME", "size": name_size},
                    ],
                }],
                "top_score": None,
                "charsets": {},
                "source": "phase2-static-heuristic",
            }

    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(new_entries, f, indent=2)
    print(f"\n{len(new_entries)} jeu(x) FORTE prets a fusionner -> {out_path}")
    print(f"{len(hi_files) - len(new_entries)} jeu(x) non fusionnes (moyenne/FAIBLE) -- verification humaine necessaire")


if __name__ == "__main__":
    main()
