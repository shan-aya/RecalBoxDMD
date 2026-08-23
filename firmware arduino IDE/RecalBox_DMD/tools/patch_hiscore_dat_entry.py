#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v1
#
# v1 - 2026-08-23 - safe-modify - Creation initiale. Genere une COPIE
#   corrigee d'un hiscore.dat (ne touche jamais le fichier source) : si le
#   rom existe deja quelque part dans le fichier (seul ou dans un groupe
#   partage avec d'autres alias -- ex. reel trouve ce soir : batlzone
#   mele au groupe mayday), retire uniquement sa ligne d'alias de ce
#   groupe (les autres alias du groupe ne sont pas affectes) puis ajoute
#   un nouveau bloc dedie en fin de fichier avec l'adresse/longueur/valeurs
#   de declenchement fournies (typiquement obtenues via
#   find_value_in_ramdump.py). Si le rom n'existe pas du tout, ajoute
#   simplement le nouveau bloc.
#
#   A utiliser avec rb2_test_hiscore_entry.sh pour valider AVANT toute
#   application definitive.
"""Produit une copie corrigee de hiscore.dat pour un seul jeu.

Usage: patch_hiscore_dat_entry.py <dat_source> <dat_sortie> <rom> \
    <address_hex> <length_hex> <wait_first_hex> <wait_last_hex>

Exemple (candidat trouve par find_value_in_ramdump.py) :
  patch_hiscore_dat_entry.py hiscore.dat.orig_backup /tmp/hiscore_batlzone_fix.dat \
      batlzone 1a40 4 12 34
"""
import re
import sys


def remove_rom_line(content, rom):
    """Retire la ligne '<rom>:' exacte (regex multiligne), peu importe le
    groupe ou elle se trouve. Ne touche a rien d'autre dans ce groupe."""
    pattern = re.compile(r"^{}:\n".format(re.escape(rom)), re.MULTILINE)
    new_content, n = pattern.subn("", content, count=1)
    return new_content, n > 0


def main():
    if len(sys.argv) != 8:
        print(__doc__)
        return
    dat_source, dat_out, rom, address, length, wait_first, wait_last = sys.argv[1:8]

    with open(dat_source, "r") as f:
        content = f.read()

    content, removed = remove_rom_line(content, rom)
    if removed:
        print("-- ancienne ligne '{}:' retiree de son groupe d'origine".format(rom))
    else:
        print("-- '{}' absent du fichier source, ajout d'une entree neuve".format(rom))

    new_block = "\n\n;--- correctif verite d'abord ({}) ---\n{}:\n@:maincpu,program,{},{},{},{}\n".format(
        rom, rom, address, length, wait_first, wait_last)
    content = content.rstrip("\n") + new_block

    with open(dat_out, "w") as f:
        f.write(content)

    print("-- ecrit: {}".format(dat_out))
    print("-- nouvelle entree: @:maincpu,program,{},{},{},{}".format(
        address, length, wait_first, wait_last))


if __name__ == "__main__":
    main()
