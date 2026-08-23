#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Dependance manquante
#   de phase2_static_analyze.py (import echoue -- fichier absent du depot,
#   jamais committe, probablement cree dans une session/worktree
#   differente et perdu). Reconstruit ici a partir du format documente en
#   tete du hiscore.dat natif du plugin Lua MAME/FBNeo (identique sur les
#   deux, confirme en lisant /usr/share/libretro-mame/mame0278/plugins/
#   hiscore/hiscore.dat sur RB2 -- voir DECISIONS.md "Récolte MAME0278").
"""Parseur du format hiscore.dat (plugin natif MAME/FBNeo).

Format (commentaires en tete du fichier lui-meme) :
  ; commentaire
  <gamename>:
  <alias2>:
  <alias3>:
  @<cputag>,<addressspace>,<address_hex>,<length_hex>,<wait_first_hex>,<wait_last_hex>[,<prefill>]
  @<cputag>,<addressspace>,<address_hex>,<length_hex>,...    (bloc supplementaire, meme groupe)

  (ligne vide = fin du groupe -- alias + blocs suivants remis a zero)

IMPORTANT : le fichier .hi ecrit par le plugin est un DUMP COMPACT et
CONTIGU de uniquement les blocs declares -- l'"adresse" d'un bloc est
l'adresse RAM LIVE (utile pour READ_CORE_RAM), PAS l'offset dans le .hi.
L'offset reel dans le .hi est la somme cumulee des LONGUEURS des blocs
precedents du meme jeu (voir compute_offsets()).
"""
import re

BLOCK_RE = re.compile(r"^@[^,]*,[^,]*,([0-9a-fA-F]+),([0-9a-fA-F]+),([0-9a-fA-F]+),([0-9a-fA-F]+)(?:,(\S+))?\s*$")
ALIAS_RE = re.compile(r"^(\S+):\s*$")


def parse_hiscore_dat(path):
    """Retourne {gamename_lower: [(address, length, wait_first, wait_last), ...]}."""
    entries = {}
    pending_names = []
    pending_blocks = []

    def flush():
        if pending_names and pending_blocks:
            for name in pending_names:
                entries[name.lower()] = list(pending_blocks)
        pending_names.clear()
        pending_blocks.clear()

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for raw_line in f:
            line = raw_line.rstrip("\n")
            stripped = line.strip()
            if not stripped:
                flush()
                continue
            if stripped.startswith(";"):
                continue
            m = BLOCK_RE.match(stripped)
            if m:
                address = int(m.group(1), 16)
                length = int(m.group(2), 16)
                wait_first = int(m.group(3), 16)
                wait_last = int(m.group(4), 16)
                pending_blocks.append((address, length, wait_first, wait_last))
                continue
            m = ALIAS_RE.match(stripped)
            if m:
                # une nouvelle ligne d'alias APRES des blocs deja accumules
                # (sans ligne vide entre) : cas non attendu en usage normal
                # -- on flush prudemment le groupe precedent d'abord.
                if pending_blocks:
                    flush()
                pending_names.append(m.group(1))
                continue
            # ligne non reconnue (variante @delay, syntaxe rare non geree) --
            # ignoree silencieusement, n'affecte pas les jeux geres par le
            # reste du fichier.
    flush()
    return entries


def compute_offsets(blocks):
    """[(address,length,wait_first,wait_last), ...] -> [(offset_dans_hi, length), ...]
    L'offset dans le .hi est la somme cumulee des longueurs precedentes --
    PAS l'adresse RAM (voir docstring module)."""
    offsets = []
    cursor = 0
    for (_address, length, _wf, _wl) in blocks:
        offsets.append((cursor, length))
        cursor += length
    return offsets


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("usage: parse_hiscore_dat.py <hiscore.dat> [gamename ...]")
        sys.exit(1)
    entries = parse_hiscore_dat(sys.argv[1])
    print("{} jeux parses".format(len(entries)))
    for name in sys.argv[2:]:
        blocks = entries.get(name.lower())
        print(name, "->", blocks, "offsets:", compute_offsets(blocks) if blocks else None)
