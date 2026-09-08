# Coût mémoire de MQTT (PubSubClient/mqttTask) — même à l'arrêt

Note écrite le 2026-09-08, à partir d'une mesure faite sur le worktree `dev/dmd-udp-transport`
(prototype UDP, voir `TRANSPORT_PLAN_UDP.md` sur cette branche) — remontée ici sur `master`
(où MQTT reste le canal actif) car l'observation est potentiellement pertinente pour
l'investigation du mur de plateforme MQTT et des crashs mémoire déjà documentés dans
`DECISIONS.md`.

## L'observation

Deux builds du même firmware (même matériel, même config, même playlist ~597 GIFs),
seule différence : `#define MQTT_ENABLED` (nouveau flag introduit sur la branche UDP,
n'existe pas encore sur `master` — voir "Comment reproduire" plus bas pour l'adapter).

| | MQTT actif (comportement actuel de `master`) | MQTT désactivé (`mqttTask()` jamais créée) |
|---|---|---|
| Heap libre juste après `setupWebConfig()` au boot | ~9852 octets | ~14540 octets |
| Heap libre en régime établi (playlist stabilisée) | ~9784-9804 octets | ~14360-14424 octets |
| `maxalloc` (plus gros bloc contigu allouable) | 4596 octets (identique des 2 côtés) | 4596 octets (identique des 2 côtés) |
| Taille du binaire compilé | 2 082 533 octets | 2 070 341 octets (~12 Ko de moins) |

**~4700-4800 octets de heap libre en moins en permanence quand MQTT est actif** — alors
même que dans les deux cas, le firmware testé passait le plus clair de son temps
**sans connexion MQTT établie** (le mur de plateforme empêche `connect()`/`subscribe()`
d'aboutir une bonne partie du temps). Donc ce n'est pas le trafic MQTT actif qui coûte
cette mémoire — c'est la simple existence de `mqttTask()` (la tâche FreeRTOS, le
`WiFiClient`/`PubSubClient` sous-jacents, ses buffers) qui grève durablement le tas,
que la connexion aboutisse ou non.

`maxalloc` (le plafond de fragmentation, 4596 octets des deux côtés) est identique —
donc MQTT ne change pas la fragmentation en régime établi, il déplace juste le
**total disponible** vers le bas d'environ 4,7 Ko.

## Pourquoi c'est potentiellement important

Le crash mémoire documenté dans `DECISIONS.md`/mémoire projet (`Guru Meditation Error:
LoadProhibited`, `EXCVADDR=0x00000000`, dans `openGifImpl()` → `path.indexOf(...)`,
provoqué par une `String` dont le buffer interne est nul) survient systématiquement
quand le tas descend vers `free≈5000` octets. Une marge de ~4,7 Ko en moins en
permanence rapproche mécaniquement le système de ce seuil critique à chaque
allocation un peu plus lourde que la moyenne (ouverture GIF, panneau hi-score,
page web config, etc.) — **sans qu'aucun trafic MQTT réel n'ait besoin d'être en
cours** au moment précis du crash.

Ce n'est **pas une preuve** que MQTT est LA cause du crash (une seule série
d'observations sur la branche UDP, pas encore reproduite/isolée sur `master` avec un
protocole dédié) — mais c'est un candidat concret et mesuré à ajouter à la liste des
pistes pour ce bug, en plus d'une éventuelle fragmentation liée au trafic MQTT lui-même
(connect/subscribe/retry créant/détruisant des buffers en boucle).

## Comment reproduire/creuser depuis `master`

`master` n'a pas le flag `MQTT_ENABLED` (c'est un ajout de la branche UDP). Pour
reproduire cette mesure ici :

1. Repérer la création de la tâche MQTT dans `RecalBox_DMD.ino` :
   ```
   if(wifiEnabled&&recalboxIP.length()>0)
       xTaskCreatePinnedToCore(mqttTask,"mqttTask",4096,NULL,1,&mqttTaskHandle,0);
   ```
   (dans `setup()`, chercher `xTaskCreatePinnedToCore(mqttTask`).
2. Commenter temporairement cet appel (ou l'entourer d'un `#if 0`/flag de test), sans
   toucher au reste — `mqttClient`/`onMqttMessage()`/`processPendingMqttCommand()`
   restent intacts, seule la tâche qui pilote `connect()`/`loop()` ne démarre plus.
3. Comparer `free=`/`maxalloc=` dans les lignes `[LOOPDIAG]` du log série (déjà
   présentes, aucune instrumentation à ajouter) entre les deux builds, sur la même
   playlist, sur une fenêtre de plusieurs minutes en régime établi.
4. Idéalement, laisser tourner plusieurs heures/une vraie session de jeu des deux côtés
   pour voir si l'écart se creuse encore (fragmentation cumulative) ou reste stable
   autour de ~4,7 Ko.

## Contexte : pourquoi cette mesure existe

Faite en marge de la bascule complète en UDP sur `dev/dmd-udp-transport` (MQTT
désactivé là-bas pour de bon, demande utilisateur explicite suite à des mois de
fragilité réseau MQTT documentée). Le firmware testé côté "MQTT désactivé" est donc
en avance de plusieurs correctifs UDP par rapport à `master` — seule la comparaison
heap MQTT-actif vs MQTT-inactif est directement transposable ici, pas le reste des
différences entre les deux builds.
