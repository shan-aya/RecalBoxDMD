# Piste UDP — remplacer MQTT par des datagrammes UDP fire-and-forget

Worktree créé le 2026-09-03 (nuit), branche à l'origine depuis `dev/core-reassignment` @
`77b67dd` (v151 — inclut Volet 1/2, fix retain v150, escalade socket v151). **Rebasée le
2026-09-06 sur `master` @ `690ca57`** (à la demande de l'utilisateur — `dev/core-reassignment`
n'est pas fusionnable en l'état, voir `HANDOFF_SESSION_2026-09-05_merge-divergence.md` ;
seul le commit de ce plan était propre à cette branche, tout le reste venait de
`core-reassignment`, donc rien perdu au rebase). Pas encore commencé, juste le plan
discuté avec l'utilisateur.

## Pourquoi

Le canal MQTT souffre d'un mur de plateforme non résolu (voir DECISIONS.md) :
`connect()`/`subscribe()` se bloquent parfois plusieurs minutes. Le mécanisme précis
en cause implique l'état de connexion TCP lui-même (handshake, retransmission,
`connect()`/`subscribe()` de PubSubClient) — l'UDP n'a structurellement AUCUN de ces
mécanismes : pas de connexion persistante, pas de handshake, pas d'état à faire
"caler". Élimine la classe de bug entière plutôt que de la contourner.

## Architecture proposée

RB1 envoie des datagrammes UDP fire-and-forget vers le DMD (port dédié, ex. 5005),
même format de payload texte que MQTT aujourd'hui (`CMD=<nom> ARG=<valeur>`).

- **DMD (firmware)** : `WiFiUDP` (déjà dans le core ESP32, aucune lib externe), un
  `udp.parsePacket()`/`udp.read()` dans `loop()` ou une tâche dédiée légère. Dispatch
  réutilise directement la logique existante `onMqttMessage()`/le switch `MqttCommand`
  (extraire `CMD=`/`ARG=` du payload UDP au lieu du payload MQTT — même parsing).
- **RB1 (scripts)** : `send_mqtt_retain()`/`send_score()` remplaceraient
  `mosquitto_pub` par un envoi UDP (ex. `python3 -c "import socket;
  socket.socket(socket.AF_INET,socket.SOCK_DGRAM).sendto(b'...', ('<ip_dmd>', 5005))"`
  ou un petit binaire `nc -u`/équivalent BusyBox si disponible).

## Points à trancher avant de coder

1. **Aucune garantie de livraison ni d'ordre** — un paquet perdu ne revient jamais tout
   seul (contrairement à MQTT `-r`/retain). Pour ce cas d'usage (affichage courant,
   pas un historique), c'est probablement acceptable : le prochain envoi écrase de
   toute façon le précédent. Mais ça renonce explicitement au mécanisme de
   resynchronisation automatique après reconnexion (fix v123 + v150 de ce soir) — il
   faudrait un ré-envoi périodique de l'état courant côté RB1 (heartbeat, ex. toutes
   les 5-10s) pour que le DMD finisse toujours par recevoir l'état réel même après un
   paquet perdu ou un reboot DMD.
2. **Pas d'accusé de réception** : RB1 ne sait jamais si le DMD a bien reçu quoi que ce
   soit (contrairement à MQTT où `subscribe`/l'état de connexion donnent un signal).
   Envisager un `GET /status` HTTP (garder le web server existant en parallèle comme
   canal de lecture d'état) si un retour est nécessaire un jour.
3. **Sécurité/robustesse minimale** : UDP n'a aucune authentification — n'importe quel
   appareil du LAN pourrait envoyer des commandes au DMD. Probablement acceptable sur
   un réseau domestique de confiance (déjà le cas avec MQTT non authentifié
   aujourd'hui), mais à noter.
4. Garder MQTT en parallèle un temps pour comparer en conditions réelles, ou couper
   directement ? Même question que la piste HTTP.

## État

**2026-09-08 (fin de soirée) — BASCULE FULL UDP décidée et faite, MQTT coupé.**
Décision utilisateur explicite (fragilité réseau MQTT documentée depuis des mois,
raison d'être de toute cette piste) — plus une question de comparaison, la piste UDP
est maintenant celle utilisée en pratique :
- Firmware v161 : `MQTT_ENABLED=false`, `mqttTask()` n'est plus jamais créée (aucun
  `connect()`/`subscribe()`, élimination complète du mur de plateforme). Code MQTT
  laissé intact mais inerte, un seul flag pour revenir en arrière.
- RB1 v45 (`marquee.sh`)/v48 (`dmd_score.sh`) : les 3 `mosquitto_pub` commentés (pas
  supprimés), seul `send_udp()` reste actif.
- **Effet de bord mesuré et documenté séparément** (`MQTT_HEAP_COST_INVESTIGATION.md`,
  remonté sur `master`) : ~4700-4800 octets de heap libre en PLUS en permanence sans
  `mqttTask()`, même quand MQTT n'était pas connecté — candidat concret (non prouvé)
  pour le crash mémoire déjà documenté dans `DECISIONS.md`.
- **Trou identifié et comblé le même soir** : sans retain MQTT, un DMD qui
  reboote/perd le WiFi en session restait figé sur son dernier état. Fix v162
  (firmware, `sendUdpHello()` au (re)connect WiFi) + `dmd_helpers/dmd_udp_resync.py`
  v1 (RB1, écoute le hello et relit `es_state.inf` à neuf) — **validé en conditions
  réelles** : reboot du DMD pendant une vraie partie en cours, resync correct
  (`CMD=game`), bascule sur l'écran du jeu au lieu de rester sur la playlist.

Tout ça déployé et actif sur RB1/DMD réels au moment de cette mise à jour.

**Prochaine étape possible** : observer en usage réel prolongé (pas juste quelques
minutes de test) — fiabilité UDP sous charge de navigation rapide (risque perf des 2
forks Python par événement déjà documenté dans les changelogs `marquee.sh`/
`dmd_score.sh`, jamais formellement mesuré en rafale), et robustesse du mécanisme de
resync sur plusieurs cycles reboot/reconnexion.

## Historique (prototype initial, avant la bascule full UDP)

**2026-09-08 (après-midi) — Prototype DMD codé et validé EN DIRECT sur matériel réel** (v158 puis
v159, voir changelog `RecalBox_DMD.ino`) :
- `WiFiUDP dmdUdp` sur le port `UDP_CMD_PORT=5005`, `handleUdpCommand()` appelée à
  chaque `loop()` (non bloquant), même parsing/dispatch que `onMqttMessage()`
  (dupliqué, pas factorisé — ne pas restructurer un chemin MQTT éprouvé pour un
  prototype pas encore validé en charge).
- v158 : `CMD_SCORE` seul, testé et validé (paquet UDP → écran score affiché).
- v159 : étendu à tout le jeu `stop/default/system/game/show_config/
  wifi_recovery/reboot/brightness/brightness_up/brightness_down/score/ingame`.
  Tous testés un par un via UDP direct vers `192.168.0.51:5005` (sauf
  `wifi_recovery`/`reboot`, disruptifs, non testés) — dispatch correct, y compris
  interaction propre avec le chemin MQTT concurrent (un vrai score MQTT arrivé en
  cours de test a été interrompu proprement par un `show_config` UDP, preuve que le
  partage de `pendingCmd`/`mqttCmdMutex` entre les 2 sources fonctionne).
- MQTT reste actif en parallèle, rien coupé. Côté RB1 (scripts `marquee.sh`/
  `dmd_score.sh`), **rien encore fait** — seul le DMD écoute, personne ne lui parle
  encore en UDP en conditions réelles de jeu.

**Prochaine étape** : côté RB1, remplacer (ou dupliquer en parallèle pour comparer)
les appels `mosquitto_pub` par un envoi UDP dans `marquee.sh`/`dmd_score.sh`, puis
observer en charge réelle (session de jeu complète) si le mur de plateforme MQTT
(déconnexions/subscribe bloquant) est bien évité côté UDP. Comparer avec la piste
HTTP (`dev/dmd-http-transport`) avant de choisir laquelle poursuivre — décision pas
encore prise à ce stade, aucune des deux voies n'est encore coupée.
