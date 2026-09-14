# Patch vendoré — crash TASK_WDT (watchdog CPU0) dans `WebServer::_parseRequest()`

Corrige le bug documenté dans `DECISIONS.md` ("BUG watchdog CPU0..." 2026-09-01,
reconfirmé 2026-09-12) : un simple accès normal à la page de config web du DMD
peut faire planter la carte (`E task_wdt: Task watchdog got triggered ... IDLE0
(CPU 0) ... Aborting`, reboot forcé).

## Cause racine (confirmée par lecture du core installé, 2026-09-14)

1. `WebServer::handleClient()` (`WebServer.cpp`) pose `_currentClient.setTimeout(HTTP_MAX_SEND_WAIT)`
   juste avant `_parseRequest()` — `HTTP_MAX_SEND_WAIT` = **5000 ms**
   (`WebServer.h`), utilisé ici comme délai d'attente de donnée en LECTURE
   (nom trompeur, hérité d'un usage côté envoi ailleurs dans la lib).
2. `Stream::timedRead()`/`timedPeek()` (`cores/esp32/Stream.cpp`) attendent
   une donnée en **busy-spin pur** (`do { c = read(); ... } while (millis()-_startMillis < _timeout)`),
   **sans le moindre `yield()`/`delay()`** dans la boucle.
3. Le core installé (`esp32-libs/3.3.11/sdkconfig`) a
   `CONFIG_ESP_TASK_WDT_TIMEOUT_S=5` — **exactement la même valeur que #1**.
   `compile.ps1` compile avec `LoopCore=0` : `loop()` (et donc
   `handleClient()`) tourne sur le CPU0, le même cœur dont la tâche IDLE0 est
   affamée par le busy-spin.

Un seul header HTTP qui tarde à arriver (client lent, WiFi marginal) suffit
donc à quasi garantir le crash — pas besoin d'accumulation ni de requêtes
scriptées rapprochées, ce qui explique la reconfirmation en usage normal du
2026-09-12 (simple ouverture de page).

## Tentative précédente (2026-08-20) — pourquoi elle avait été annulée

Un premier essai avait ajouté juste `delay(1)` dans la boucle de
`Stream::timedRead()`, SANS toucher `HTTP_MAX_SEND_WAIT`. Ça supprimait bien
le busy-spin (donc le crash), mais laissait un plafond par lecture toujours
égal à 5000 ms et aucun plafond global sur `_parseRequest()` — un client qui
distille ses lignes d'en-tête juste sous ce délai pouvait alors geler
`loop()` (donc tout l'affichage DMD, mono-tâche) pendant un temps non borné
en pratique, SANS le filet de sécurité (reboot watchdog) qui existait avant.
Décision actée à l'époque : ne pas garder ce patch seul.

## Fix retenu (2026-09-14)

Les deux changements ci-dessous, ENSEMBLE :

1. **`WebServer.cpp`** : `HTTP_MAX_SEND_WAIT` (5000 ms) → **800 ms** au seul
   point d'appel qui règle le timeout de lecture avant `_parseRequest()`.
   Cette constante n'est utilisée nulle part ailleurs dans la bibliothèque
   (vérifié par recherche dans tout `libraries/WebServer/`) — aucun autre
   comportement affecté. 800 ms reste une marge très large pour n'importe
   quel client réel sur un LAN (navigateur, PC Toolkit, script Recalbox) —
   largement sous le budget des watchdogs (5s tâche / 300ms IWDT), donc plus
   aucun risque qu'une lecture individuelle s'approche du seuil de crash.
2. **`Stream.cpp`** : `delay(1)` ajouté dans la boucle d'attente de
   `timedRead()`/`timedPeek()` — supprime le busy-spin lui-même (défense en
   profondeur, protège aussi tout AUTRE usage de `Stream::timedRead()` dans
   le firmware, pas seulement `WebServer`). Cette fois, le plafond par appel
   étant tombé à 800 ms (#1), l'objection d'origine ("retire la protection
   sans fournir de vraie sortie") ne s'applique plus : la sortie est déjà
   bornée par construction, `delay(1)` ne fait plus que la rendre non
   bloquante pour le reste du système pendant l'attente.

**Reste non traité, assumé** : pas de plafond global explicite sur
l'ensemble de `_parseRequest()` (somme de tous ses appels `readStringUntil`
successifs) — nécessiterait de patcher `Parsing.cpp` (plus gros, plus
risqué, logique de parsing multipart/chunked à ne pas casser). Avec le
plafond par appel à 800 ms, le pire cas théorique (un client qui aligne de
nombreuses lignes d'en-tête toutes juste sous 800 ms) reste borné et très
improbable avec les clients réels de ce projet — accepté comme risque
résiduel plutôt que d'élargir le patch. À reconsidérer seulement si un gel
(pas un crash) est un jour observé en usage réel.

## Pourquoi ce patch vit hors du dépôt Git

Les 2 fichiers modifiés font partie du core Arduino-ESP32 installé
(`esp32-libs`/`WebServer`), PAS du dépôt `RecalBox_DMD` — un `git diff` ne
les verra jamais, et une réinstallation du core (mise à jour, nouvelle
machine) les écrase silencieusement. `apply.ps1` (ce dossier) réapplique le
patch de façon idempotente et vérifiable — **à relancer après toute
(ré)installation du core `esp32` 3.3.11**, et à revalider si le projet
passe un jour à une autre version du core (le script refuse de patcher un
contenu qu'il ne reconnaît pas plutôt que de le faire à l'aveugle).

## Usage

```powershell
.\tools\vendored_patches\webserver_watchdog_fix\apply.ps1
```

Sans argument, cible l'installation par défaut
(`$env:LOCALAPPDATA\Arduino15`, core `esp32` `3.3.11`). Affiche l'état de
chaque fichier (déjà patché / patché avec succès / contenu inattendu) et
ressort en erreur (code non nul) si un fichier ne correspond à aucun des
deux cas attendus, plutôt que de modifier quoi que ce soit à l'aveugle.

Recompiler ensuite via `compile.ps1` normalement.
