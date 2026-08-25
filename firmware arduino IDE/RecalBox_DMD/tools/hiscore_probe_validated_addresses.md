# Adresses hi-score validées — chantier `hiscore_probe` (MAME0278)

Suivi des adresses de score trouvées par le plugin MAME `hiscore_probe`
(`tools/mame_plugin_hiscore_probe/`) — méthode : pilotage automatique
(credit+start+tir+mouvement) ou démo attract-mode, snapshots RAM
réguliers, croisement avec des captures d'écran réelles ("vérité
d'abord"). Voir `DECISIONS.md` (sections "Suite immédiate (3) à (9)")
pour le détail complet de la méthode et des découvertes.

**Comment vérifier une adresse ci-dessous** : lancer le jeu sur RB2/MAME
0.278, insérer un crédit, jouer jusqu'à obtenir un score non nul, lire
la valeur à l'adresse indiquée (ex. via le debugger MAME `-debug`, ou
`READ_CORE_RAM <addr_hex> <n>` en commande réseau RetroArch SI le
driver le supporte — voir DECISIONS.md, non garanti pour tous les
drivers), comparer à ce qui est affiché à l'écran.

## Légende statut

- **CONFIRMÉ** : au moins 2 preuves indépendantes concordantes (ex.
  démo attract-mode + vraie partie pilotée), comportement de reset
  cohérent (retombe à 0 à la mort / au retour à l'écran titre).
- **CANDIDAT** : une seule preuve, progression cohérente observée mais
  pas encore recoupée une 2e fois — à vérifier avant de considérer
  comme fiable à 100%.

## Jeux validés

### `inthunt` — In the Hunt (Irem, shmup, scroll horizontal)

- **Adresse : `0xea488`** — zone `:maincpu program` (`mainram`,
  `0xe0000-0xeffff`)
- **Encodage** : 1 octet, valeur = score affiché ÷ 100 (donc plage
  représentable 0-25500 avant dépassement)
- **Statut : CONFIRMÉ**
- **Preuve 1 (démo attract-mode)** : `DEMO_5=12`(1200 exact),
  `DEMO_6=29`(2900 exact) — scores lus sur capture d'écran zoomée,
  correspondance BCD/binaire exacte
- **Preuve 2 (vraie partie pilotée)** : progression monotone complète
  `0→200→1000→1800→2900→3900→4300→4300(palier)→0(mort/reset)` — une
  capture intermédiaire lue "1P 00001700" tombe exactement entre les
  échantillons 1000 et 1800
- **Config pilotage utilisée** : `MOVE_DIRECTION=down` (défaut),
  `FORWARD_DIRECTION=right` (défaut) — balayage haut/bas, avance à
  droite
- **Non résolu** : sur un 2e segment de démo (scène de boss), la même
  adresse ne correspondait pas au score affiché — hypothèse non
  vérifiée (mécanique différente sur ce segment)

### `gbusters` — Gang Busters (Konami, action, scroll vertical)

- **Adresse candidate : `0x40a6`** — zone `:maincpu program`
  (`0x4000-0x5fff`, 8 Ko, une seule zone RAM sur ce driver)
- **Encodage** : u8 ou u16 BE (les deux interprétations remontées par
  la recherche automatique, à trancher) — valeur brute observée
  `[0, 5, 73, 103, 103, 103, ...]` (montée puis palier)
- **Statut : CANDIDAT (pas encore confirmé par recoupement)**
- **Preuve** : score réel "1UP 5100" obtenu par capture d'écran
  pendant la même session de pilotage — pas encore corrélé
  précisément à un échantillon RAM donné (pas de 2e recoupement fait,
  contrairement à `inthunt`)
- **Config pilotage utilisée** : `MOVE_DIRECTION=right`,
  `FORWARD_DIRECTION=up` (scroll vertical — balayage gauche/droite,
  avance vers le haut)
- **2 bugs réels corrigés pendant ce test** (voir DECISIONS.md pour
  le détail) : le champ crédit trouvé automatiquement était à tort un
  DIP switch (`"Coin A"` sur `:DSW1`) au lieu du vrai input `"Coin 1"`
  sur `:SYSTEM` ; la direction d'avance déduite automatiquement était
  fausse pour un scroll vertical (corrigée en variable d'environnement
  dédiée `FORWARD_DIRECTION`, indépendante de `MOVE_DIRECTION`)

## Jeux testés SANS succès (démo attract-mode sans score exploitable)

Ces jeux n'ont PAS de démo attract-mode montrant un score réel qui
progresse (confirmé par observation directe sur l'écran physique) —
nécessiteraient le pilotage automatique complet (comme `gbusters`
ci-dessus) plutôt que la méthode démo pour être validés :

- `willow` (Capcom, platform) — démo dominée par des cinématiques
  narratives longues, la portion jouée montrée ne semble pas générer
  de kills (score resté à 0 sur toute la fenêtre observée)
- `gbusters` — démo affiche un score STATIQUE ("HIGH 163500" ne varie
  quasiment pas, même en légère baisse d'un échantillon à l'autre) —
  d'où le passage au pilotage complet, qui lui a fonctionné (voir
  ci-dessus)

## Reste à tester (catégories du lot "RB Challenge")

| Romset | Titre | Catégorie | Statut |
|---|---|---|---|
| `joemacr` | Joe & Mac Returns | Bubble | credit+start OK, score non testé |
| `pzloop2` | Puzz Loop 2 | Puzzle | credit+start OK, score non testé |
| `nemo` | Nemo | Platform | credit+start OK, score non testé |
| `tbyahhoo` | TwinBee Yahho! | Shmup | credit+start OK, score non testé |
| `jjsquawk` | J.J. Squawkers | Platform | credit+start OK, score non testé |
| `dynagear` | Dyna Gears | Action | credit+start OK, score non testé |
| `msgogo` | Mouse Shooter GoGo | Puzzle | credit+start OK, score non testé |
| `mtwins` | Mega Twins | Platform | credit+start OK, score non testé |
| `whoopee` | Pipi & Bibi's/Whoopee!! | Action | credit+start OK, score non testé |
| `gogomile` | Susume! Mile Smile | Puzzle | credit+start OK, score non testé |
| `progear` | Progear | Shmup | credit+start OK (2 coins + sélection pilote requis), score non testé |
| `willow` | Willow | Platform | démo sans score exploitable, pilotage non tenté |
| `kamenrid` | Masked Riders Club Battle Race | Race | credit+start OK, score non testé |
| `osman` | Osman | Action | credit+start OK, score non testé |

Pour chacun, "credit+start OK" signifie confirmé fonctionnel lors du
lot de 16 jeux (`hiscore_probe` v0.0.3, voir DECISIONS.md section
"Suite immédiate (3)") — mais le pilotage complet (tir+mouvement) et
la recherche d'adresse de score n'ont pas encore été tentés sur ces
jeux avec les fixes les plus récents (v0.0.20/v0.0.21).
