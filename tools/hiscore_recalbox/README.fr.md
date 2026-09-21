# Données hi-score pour Recalbox — fichiers `.hi` et tables JSON

[🇬🇧 English](README.md) · 🇫🇷 **Français**

Instantané de tout ce que le projet [RecalBoxDMD](../../README.fr.md) a récolté sur les **hi-scores d'arcade** (FBNeo et MAME 0.278), mis en forme pour être transmis à l'équipe Recalbox ou déposé sur n'importe quelle Recalbox.

- **3161 fichiers `.hi`** — les fichiers de hi-score bruts écrits par les émulateurs : 2491 pour FBNeo, 670 pour MAME 0.278.
- **2 fichiers JSON** — une table de scores par défaut vérifiés à la main, et le manifeste qui explique comment décoder les vrais `.hi`.
- Date de l'instantané : **22/09/2026**.

## Contenu

```text
hi/
  fbneo/fbneo/<rom>.hi                 2491 fichiers  → /recalbox/share/saves/fbneo/fbneo/
  mame/mame0278/hiscore/<rom>.hi        670 fichiers  → /recalbox/share/saves/mame/mame0278/hiscore/
json/
  verified_default_scores.json         3941 entrées (2277 mame0278 + 1664 fbneo)
  hiscore_manifest.json                3089 entrées (une par ROM)
hiscore_hi_pack.zip                    les mêmes .hi dans une seule archive (utilisée par le Toolkit PC)
MANIFEST.csv                           chaque .hi : chemin, taille, SHA-256, indicateur all_zero
```

Les dossiers sous `hi/` reproduisent les vrais chemins sous `/recalbox/share/saves/` : copier `hi/*` sur `/recalbox/share/saves/` place chaque fichier là où l'émulateur le cherche.

## À lire avant d'utiliser les `.hi`

- **Ne jamais écraser.** Un `.hi` déjà présent sur une Recalbox peut venir d'une vraie partie de son propriétaire. Copier avec une règle « ignorer l'existant » (`rsync --ignore-existing`, `cp -n`, …). Le Toolkit PC fait exactement cela (voir plus bas).
- **Ce ne sont pas tous de vrais records de joueur.** Les fichiers sont l'union de ce que deux machines Recalbox ont produit. Le premier lot (929 fichiers, de la machine la plus ancienne) vient de parties réelles et de récoltes antérieures ; une large part du reste a été produite par **jeu automatisé** (un script qui insère un crédit et envoie des commandes simulées) : le score de ces fichiers est ce que le robot a atteint, pas un record humain. L'origine de chaque fichier n'a pas été suivie individuellement.
- **132 fichiers ne contiennent que des zéros** (jeux où aucun score n'a jamais été écrit). Ils sont marqués `all_zero=1` dans `MANIFEST.csv` et **absents de `hiscore_hi_pack.zip`** : le Toolkit PC ne les installe donc jamais.
- `hi/mame/mame0278/hiscore/` ne contient volontairement **pas** le `plugin.cfg` présent à côté des vrais fichiers sur une Recalbox — c'est un fichier de configuration, pas un hi-score.

## Les fichiers JSON

**`json/verified_default_scores.json`** — tables de scores par défaut lues à l'œil sur des captures d'écran (jeu « Scores » de progetto-SNAPS, MAME 0.288) pour les jeux dont le vrai `.hi` n'a pas pu être obtenu. Clé = `<système>_<rom>` (ex. `mame0278_1943`, `fbneo_19xx`) ; valeur :

```json
{
  "lines":  ["1 ABG 96500", "2 TAC 20000", "3 YAM 15000", "4 POO 10000", "5 MR. 7000"],
  "source": "comment la valeur a été obtenue",
  "date":   "2026-09-05"
}
```

C'est seulement un repli : un vrai `.hi` est toujours prioritaire. Les clés racine `_readme` et `_format_entree` documentent le format.

**`json/hiscore_manifest.json`** — pour chaque ROM, la structure de son vrai fichier `.hi` (taille attendue, début des entrées de score, nombre, pas, taille/endianness/format des champs comme le BCD, jeux de caractères), pour pouvoir décoder la table des scores. Exemple :

```json
"10yard": { "expected_size": 11,
            "entries": [{ "mode": "score_only", "start_offset": 1, "count": 5, "stride": 2,
                          "fields": [{ "rel_offset": 0, "kind": "int", "id": "SCORE",
                                       "size": 2, "endian": "little", "format": "bcd" }] }],
            "top_score": null, "charsets": {} }
```

## Comment le Toolkit PC utilise ce dossier

- Les **Modes 1 et 9** copient les `.hi` de `hiscore_hi_pack.zip` vers la Recalbox (partage SMB d'abord, SSH en repli) en **sautant tout fichier déjà présent**. Le journal se termine par une ligne du type `N .hi copiés, M déjà présents (conservés tels quels)`.
- Les deux JSON sont installés par les mêmes modes avec les scripts Recalbox, depuis `tools/recalbox_scripts/dmd_helpers/` — garder cette copie synchronisée avec `json/` ici à chaque mise à jour des tables.
