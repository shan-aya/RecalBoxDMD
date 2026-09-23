# Hi-score data for Recalbox — `.hi` files and JSON tables

🇬🇧 **English** · [🇫🇷 Français](README.fr.md)

Snapshot of everything the [RecalBoxDMD](../../README.md) project has collected about **arcade hi-scores** (FBNeo and MAME 0.278), packaged so it can be handed over to the Recalbox team or dropped on any Recalbox.

- **3161 `.hi` files** — the raw hi-score files written by the emulators: 2491 for FBNeo, 670 for MAME 0.278.
- **2 JSON files** — a table of hand-verified default scores, and the manifest that explains how to decode the real `.hi` files.
- Snapshot date: **2026-09-22**.

## Contents

```text
hi/
  fbneo/fbneo/<rom>.hi                 2491 files  → /recalbox/share/saves/fbneo/fbneo/
  mame/hiscore/<rom>.hi                 670 files  → /recalbox/share/saves/mame/<MAME core>/hiscore/
json/
  verified_default_scores.json         3941 entries (2277 mame0278 + 1664 fbneo)
  hiscore_manifest.json                3089 entries (one per ROM)
hiscore_hi_pack_v2.zip                 the same .hi files in one archive (used by the PC Toolkit from build 7651)
hiscore_hi_pack.zip                    older archive with mame0278 paths, kept only for Toolkit build 7550
MANIFEST.csv                           every .hi: path, size, SHA-256, all_zero flag
```

`hi/fbneo/fbneo/` mirrors `/recalbox/share/saves/fbneo/fbneo/`. The MAME files carry **no MAME version** on purpose: on a Recalbox they go into `/recalbox/share/saves/mame/<core>/hiscore/`, where `<core>` is the MAME core that Recalbox runs (for example `mame0278`). The folder follows the **core**, not the romset: a Recalbox with `mame0288` roms running the `mame0278` core writes its scores in `mame0278/hiscore/`. These files were produced by the `mame0278` core.

## Read this before using the `.hi` files

- **Never overwrite.** A `.hi` already present on a Recalbox may come from a real game played by its owner. Copy with a "skip existing" rule (`rsync --ignore-existing`, `cp -n`, …). The PC Toolkit does exactly that (see below).
- **Not all of them are real player records.** The files are the union of what two Recalbox machines produced. The first batch (929 files, from the older machine) comes from real play and earlier harvesting; a large share of the rest was produced by **automated play** (a script inserting a credit and sending simulated inputs), so a score in those files is whatever the bot reached, not a human record. The origin of each individual file was not tracked.
- **132 files contain only zero bytes** (the game's score memory was still blank when the emulator saved it). They are flagged `all_zero=1` in `MANIFEST.csv` and are **left out of both zip archives**, so the PC Toolkit never installs them.
- `hi/mame/hiscore/` deliberately does **not** contain the `plugin.cfg` that sits next to the real files on a Recalbox — it is a configuration file, not a hi-score.

## The JSON files

**`json/verified_default_scores.json`** — default score tables read by eye from screenshots (progetto-SNAPS "Scores" set, MAME 0.288) for games whose real `.hi` could not be obtained. Key = `<system>_<rom>` (e.g. `mame0278_1943`, `fbneo_19xx`); value:

```json
{
  "lines":  ["1 ABG 96500", "2 TAC 20000", "3 YAM 15000", "4 POO 10000", "5 MR. 7000"],
  "source": "how the value was obtained",
  "date":   "2026-09-05"
}
```

It is only a fallback: a real `.hi` always wins. The top-level keys `_readme` and `_format_entree` document the format. The MAME keys keep the version they were read for (`mame0278_…`), but the lookup script (`dmd_hiscore_verified.py` v2) accepts any `mame*_<rom>` key, so the table keeps working with another MAME core.

**`json/hiscore_manifest.json`** — for each ROM, the layout of its real `.hi` file (expected size, where the score entries start, how many, stride, field sizes/endianness/format such as BCD, character sets), so the score table can be decoded. Example:

```json
"10yard": { "expected_size": 11,
            "entries": [{ "mode": "score_only", "start_offset": 1, "count": 5, "stride": 2,
                          "fields": [{ "rel_offset": 0, "kind": "int", "id": "SCORE",
                                       "size": 2, "endian": "little", "format": "bcd" }] }],
            "top_score": null, "charsets": {} }
```

## How the PC Toolkit uses this folder

- **Mode 1 and Mode 9** copy the `.hi` files of `hiscore_hi_pack_v2.zip` to the Recalbox (SMB share first, SSH as a fallback), **skipping every file that already exists**. The MAME target folder is chosen on the Recalbox itself: the `mame.core` setting of `recalbox.conf`, otherwise the `mame0NNN` folder where MAME most recently wrote a `.hi` — so a new MAME version needs no Toolkit update. The log ends with a line such as `N .hi copied, M already present (kept as they are)`.
- The two JSON files are installed by the same modes together with the Recalbox scripts, from `tools/recalbox_scripts/dmd_helpers/` — keep that copy in sync with `json/` here when the tables are updated.
