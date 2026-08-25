# Adresses hi-score validées — chantier `hiscore_probe` (MAME0278) — RB Challenge

Document préparé pour le RB Challenge — ce qui suit est **CERTAIN**
uniquement (rien de spéculatif). Objectif de ce document : que tu
vérifies qu'on a bien tous les éléments dont tu as besoin côté
adressage (vie, score, crédit, continue, etc.) pour le marquee DMD, et
que tu nous dises ce qui manque pour qu'on complète.

**Méthode utilisée** (pour référence, si tu veux reproduire/étendre) :
plugin MAME réel `tools/mame_plugin_hiscore_probe/` qui pilote
automatiquement credit→start→tir→mouvement pendant ~90s, prend des
snapshots RAM réguliers, et croise avec des captures d'écran réelles
("vérité d'abord") pour identifier quel octet correspond à ce qui est
affiché. Détail complet (bugs trouvés, décisions) dans `DECISIONS.md`
(sections "Suite immédiate (3) à (9)", 2026-08-25).

## Ce qui est CERTAIN à ce stade

### `inthunt` — In the Hunt (Irem, shmup, scroll horizontal)

- **Score 1P : adresse `0xea488`** — zone `:maincpu program`
  (`mainram`, `0xe0000-0xeffff`)
- **Encodage** : 1 octet, valeur = score affiché ÷ 100
- **Preuve 1 (démo attract-mode)** : `1200` et `2900` retrouvés
  EXACTEMENT sur 2 captures d'écran successives
- **Preuve 2 (vraie partie pilotée par le plugin)** : progression
  monotone complète `0→200→1000→1800→2900→3900→4300→4300(palier)
  →0(mort/reset)` — une capture intermédiaire lue "1P 00001700" tombe
  exactement entre les échantillons 1000 et 1800
- **C'est la SEULE adresse (vie/crédit/continue/score/autre) confirmée
  à ce jour sur l'ensemble du chantier.**

## Ce qui NOUS MANQUE (à toi de nous dire ce qui est prioritaire)

Pour l'instant on n'a QUE le score 1P d'un seul jeu. Question directe :
**de quels éléments as-tu besoin pour le marquee DMD, jeu par jeu ?**
Score seul suffit, ou il te faut aussi :

- vies restantes
- nombre de crédits insérés
- continues utilisés/restants
- score 2P (si 2 joueurs)
- niveau/stage/round en cours
- autre chose ?

Et : **`inthunt` est intéressant comme preuve de méthode, mais c'est
un seul jeu sur les ~3000 de MAME — quelle est la priorité pour la
suite** : couvrir d'abord tous les éléments d'un même jeu (vie/score/
crédit/etc. sur `inthunt`), ou élargir le score seul à plus de jeux
d'abord ?

## Comment vérifier l'adresse ci-dessus

Lancer `inthunt`, insérer un crédit, jouer jusqu'à obtenir un score
non nul, lire la valeur à `0xea488` (debugger MAME `-debug`, ou
`READ_CORE_RAM ea488 <n>` en commande réseau RetroArch — **ne
fonctionne pas sur tous les drivers, échoue par exemple sur ce même
jeu à d'autres adresses testées cette session, à vérifier au cas par
cas**), comparer à ce qui est affiché à l'écran (`x100` pour retrouver
le score affiché).

## Autres jeux du lot "RB Challenge" — statut credit/start uniquement (score non testé)

Ces jeux ont un crédit+start fonctionnel confirmé (le personnage
démarre bien une vraie partie) mais AUCUNE adresse de score n'a
encore été recherchée dessus : `joemacr`, `pzloop2`, `nemo`,
`gbusters`, `tbyahhoo`, `jjsquawk`, `dynagear`, `msgogo`, `mtwins`,
`whoopee`, `gogomile`, `progear`, `willow`, `kamenrid`, `osman`.
