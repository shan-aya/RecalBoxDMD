# Piste UDP — remplacer MQTT par des datagrammes UDP fire-and-forget

Worktree créé le 2026-09-03 (nuit), branche depuis `dev/core-reassignment` @ `77b67dd`
(v151 — inclut Volet 1/2, fix retain v150, escalade socket v151). Pas encore commencé,
juste le plan discuté avec l'utilisateur.

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

Rien codé. Prochaine étape suggérée : prototype minimal (DMD écoute UDP + republie un
seul type de commande côté RB1, ex. `CMD=score`) pour valider la fiabilité en charge
réelle avant de migrer le reste. Comparer avec la piste HTTP (`dev/dmd-http-transport`)
sur le même protocole de test avant de choisir laquelle poursuivre.
