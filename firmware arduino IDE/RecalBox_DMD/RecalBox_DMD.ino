// ============================================
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v160
//
// v160 - 2026-09-08 - safe-modify - RETRO_VERSION (splash boot, ecran
//   physique) renomme "Raw565 Ed. dev13" -> "Raw565 Ed. v13UDP" (demande
//   utilisateur) -- identifie visuellement ce build comme la piste UDP en
//   cours de validation, distinct du dev13 "de base" (branche dev/dmd-udp-
//   transport). Aucun changement fonctionnel.
//
// v159 - 2026-09-08 - safe-modify - Piste UDP : extension de
//   handleUdpCommand() (v158) a TOUT le jeu de commandes marquee/cmd
//   (stop/default/system/game/show_config/wifi_recovery/reboot/
//   brightness/brightness_up/brightness_down/score/ingame), meme dispatch
//   qu'onMqttMessage() (v148), duplique plutot que factorise -- ne pas
//   restructurer un chemin MQTT eprouve depuis des mois pour un prototype
//   pas encore valide en charge. CMD_SCORE seul avait ete valide sur
//   materiel reel juste avant cette extension (v158, voir DECISIONS.md) --
//   le reste des commandes PAS ENCORE teste. Prochaine etape : test reel
//   des autres commandes (stop/default/system/game au minimum) avant
//   d'envisager de reduire/couper MQTT.
//
// v158 - 2026-09-06 - safe-modify - Piste UDP (voir TRANSPORT_PLAN_UDP.md,
//   branche dev/dmd-udp-transport) : PROTOTYPE MINIMAL, CMD_SCORE
//   uniquement, PAS ENCORE TESTE SUR MATERIEL. Objectif : contourner le mur
//   de plateforme MQTT non resolu (connect()/subscribe() bloquants, voir
//   memoire projet/DECISIONS.md) en s'affranchissant de tout etat TCP a
//   faire "caler" -- UDP est fire-and-forget, sans connexion/handshake.
//   Ajouts : WiFiUDP dmdUdp (port UDP_CMD_PORT=5005), dmdUdp.begin() une
//   seule fois apres la 1ere connexion WiFi reussie (setupWiFiFromConfig(),
//   PAS rearme apres une reconnexion WiFi ulterieure -- a valider),
//   handleUdpCommand() (parsePacket() non bloquant, appelee a chaque
//   loop() juste apres processPendingMqttCommand()) : meme format de
//   payload que MQTT (v148, "CMD=<nom> ARG=<reste>"), meme extractField(),
//   mais dispatch limite a "score" (ecrit pendingCmd sous mqttCmdMutex,
//   comme onMqttMessage()) -- tout autre CMD est logue (Serial + mqttLog[])
//   mais sciemment ignore pour l'instant. MQTT INCHANGE, tourne toujours en
//   parallele (comparaison prevue, voir plan). Prochaine etape : test reel
//   (envoi UDP depuis RB1) avant d'etendre aux autres commandes.
//
// v157 - 2026-09-05 - safe-modify - drawScoreScreen() : score du rang 1
//   (ecran hi-score) desormais ALIGNE A DROITE, comme le rang 2 juste en
//   dessous -- retour utilisateur sur materiel reel (test du placeholder
//   niveau 3 de dmd_score.sh v45, sur `badlands`) : "le score est pas
//   aligne a droite". Bug reel confirme par lecture du code (present
//   depuis v113, jamais remarque avant faute d'un test dedie a ce rang
//   precis) : le rang 1 placait son score juste apres le nom (sx = fin du
//   nom + marge fixe), le rang 2 alignait deja le sien sur le bord droit
//   de l'ecran (v120) -- incoherence visuelle entre les 2 rangs d'un
//   meme ecran. Meme formule que le rang 2, adaptee a la police taille 2
//   du rang 1 (12px/caractere). Affecte TOUS les ecrans hi-score (rang 1
//   reel compris, pas seulement le placeholder qui a revele le bug).
//   Question de casse egalement soulevee par l'utilisateur sur ce meme
//   test (nom "shan" affiche pas clairement en minuscules) : AUCUNE
//   transformation de casse trouvee dans le code (police par defaut
//   Adafruit GFX, display->setFont(NULL) explicite, pas de
//   toUpperCase() sur ce chemin) -- probable effet de lisibilite de la
//   police pixel a petite taille plutot qu'un bug de casse reel, non
//   corrige faute de cause identifiee dans le code.
//
// v156 - 2026-09-04 - safe-modify - Cooldown maintainWiFi() rendu
//   progressif (5s base, +5s/cycle sans succes, plafond 30s, reset a la
//   base des qu'une connexion aboutit -- voir consecutiveWifiReconnectCycles
//   et le corps de maintainWiFi()) : corrige le vrai coupable identifie
//   pour l'episode v155 ci-dessous (le cooldown fixe de 5s interrompait
//   chaque tentative WiFi.begin() avant qu'elle ait pu vraiment aboutir en
//   conditions degradees -- raison=8, auto-initie cote DMD, PAS une
//   collision avec le mutex qui faisait deja correctement son travail).
//   Escalade WiFi (v144) REACTIVEE (WIFI_RESET_ESCALATION_ENABLED=true) --
//   synchronise desormais aussi lastWifiReconnectAttempt juste apres son
//   propre WiFi.begin(), pour que maintainWiFi() ne retente pas
//   immediatement par-dessus. PAS ENCORE VALIDE SUR MATERIEL REEL au
//   moment de ce commit (compile OK, flash en cours).
//
// v155 - 2026-09-04 - safe-modify - Deux retraits suite a l'episode
//   d'escalade WiFi reel de ce soir (13:33:16-13:36:55, ~3min39 de boucle
//   disconnect/reconnect malgre wifiResetMutex v144, voir DECISIONS.md) :
//   (1) escalade WiFi (v144, WIFI_RESET_ESCALATION_CYCLES) desactivee
//   temporairement via WIFI_RESET_ESCALATION_ENABLED=false -- code
//   conserve intact, cause exacte (suspicion : cooldown 5s de
//   maintainWiFi() trop court pour laisser WiFi.begin() vraiment aboutir
//   en conditions degradees, boucle de retry auto-entretenue une fois le
//   1er disconnect declenche) pas encore confirmee/corrigee. (2) endpoint
//   /log (v153/v154) retire completement : question utilisateur directe
//   ("le endpoint peut il degrader la connexion ?") -- reponse OUI,
//   confirme par le code : handleWebConfigMqttLog() utilisait
//   webServer->send() standard, PAS la voie protegee (sendGzipHtml()
//   fast-abandon, v48-v50) ; or le commentaire v62 pres de
//   webServer->handleClient() dans loop() documente deja qu'un handler
//   HTTP qui bloque y bloque TOUTE l'iteration loop(), donc
//   maintainWiFi() et processPendingMqttCommand() avec lui -- un client
//   qui stagne en pleine reponse (WiFi degrade, exactement le moment ou
//   on l'utiliserait) pouvait donc aggraver le probleme qu'il servait a
//   surveiller. mqttLog[]/mqttLogAdd()/MQTT_LOG_SIZE conserves (passifs,
//   aucun cout reseau, utiles independamment).
//
// v154 - 2026-09-04 - safe-modify - /log (v153) passe de text/plain a une
//   mini page HTML avec <meta http-equiv="refresh" content="3"> -- retour
//   utilisateur, la page ne se rafraichissait pas seule dans un navigateur
//   ouvert en continu. htmlEscapeSmall() ajoute par prudence sur les
//   parties dynamiques (topic/msg). DMD_Serial_Monitor.ps1 (tools/) mis a
//   jour en parallele pour extraire le contenu de <pre> proprement au lieu
//   d'afficher le HTML brut. Rappel : /log reste a retirer avant tout
//   passage en production/master, voir DECISIONS.md.
//
// v153 - 2026-09-03 - safe-modify - Endpoint HTTP GET /log (demande
//   utilisateur, DMD debranche de l'USB pendant une session de surveillance
//   nocturne -- besoin d'un mini suivi via WiFi, juste stabilite/pertes de
//   connexion, PAS un diagnostic complet, ca reste le role de l'USB).
//   Alternative volontairement plus legere qu'un serveur telnet complet
//   (retire en v18, cout RAM/CPU) -- reutilise le webServer deja actif, pas
//   de nouveau port/serveur. handleWebConfigMqttLog() (corps dans ce
//   fichier, declaration anticipee dans web_config.h -- inclus AVANT la
//   declaration de mqttLog[]/currentMode/etc., ordre oblige) : dump du
//   buffer mqttLog[] existant (10 derniers messages MQTT recus, deja
//   alimente par mqttLogAdd()) + une ligne d'etat courant (heap/RSSI/mode/
//   tentatives connect) equivalente a [LOOPDIAG]. Compilation verifiee OK
//   (66% flash/28% RAM). PAS ENCORE FLASHE (DMD debranche de l'USB au
//   moment de ce commit) -- a deployer au prochain branchement.
//
// v152 - 2026-09-03 - safe-modify - Patch EXTERNE (pas dans ce fichier) sur
//   PubSubClient.cpp, qui protege l'envoi du paquet CONNECT MQTT contre le
//   meme piege de blocage deja diagnostique et corrige pour les pages web
//   (web_config.h v48-v50 : write() de la bibliotheque reseau peut s'etirer
//   sur plusieurs secondes a cause d'une boucle de retry interne non
//   configurable, compteur qui se reset a chaque octet qui passe).
//   connect() envoyait le paquet CONNECT via ce chemin non protege,
//   expliquant une partie des ~19-20s observes avant un rc=-4
//   (MQTT_CONNECTION_TIMEOUT), en plus des ~10s deja expliques par l'attente
//   CONNACK elle-meme (deja bornee par socketTimeout). Fichier modifie :
//   D:\CROQUIS ARDUINO IDE\libraries\PubSubClient\src\PubSubClient.cpp
//   (HORS DEPOT GIT -- installation Arduino locale a cette machine, backup
//   manuel fait avant modification : PubSubClient.cpp.bak_2026-09-03_avant_fastwrite
//   dans ce meme dossier). Nouvelle fonction pubsubFastWrite() : meme
//   principe que mqttSubscribeFast() (RecalBox_DMD.ino) -- envoi par petits
//   blocs via le fd brut + select(), abandon rapide (budget 2s total) des
//   qu'un bloc ne progresse plus du tout. IMPORTANT : ce projet depend
//   desormais d'une bibliotheque PubSubClient patchee localement -- une
//   reinstallation/mise a jour de la bibliotheque sur une AUTRE machine (ou
//   apres reinstallation de celle-ci) perdrait ce patch silencieusement,
//   voir commentaire complet dans PubSubClient.cpp lui-meme pour le detail.
//   PAS ENCORE VALIDE SUR MATERIEL REEL au moment de ce commit.
//
// v151 - 2026-09-03 - safe-modify - Nouvelle escalade "recreation de
//   socket" pour connect() MQTT en echec soutenu (rc=-2/-4), discutee en
//   detail avec l'utilisateur avant implementation. Contexte : v133/v134
//   (2026-08-24) avaient deja tente un dernier recours pour ce cas precis
//   (WiFi.disconnect()/begin() force apres 90s) mais REVERTE -- collision
//   non synchronisee avec maintainWiFi() sur les 2 coeurs, WiFi reel
//   decrochant en boucle serree, situation pire qu'avant. Cette collision
//   precise est resolue depuis ce soir (wifiResetMutex, v144), mais
//   plutot que reintroduire un reset WiFi (cout reel : coupure totale du
//   canal web pendant la reassociation, utilise activement ce soir pour
//   diagnostiquer/rebasculer le DMD), piste alternative retenue : rester
//   ENTIEREMENT au niveau TCP/socket, jamais teste avant sur ce projet.
//   wifiClientMqtt (WiFiClient global, jamais recree depuis le tout
//   premier boot -- meme objet/etat interne reutilise sur potentiellement
//   des centaines de cycles connect/deconnect) est explicitement ferme
//   (stop()) et reconstruit a neuf apres SOCKET_RECREATE_ESCALATION_CYCLES
//   (5, ~170s vu le rythme reel observe cette nuit) echecs connect()
//   consecutifs -- mqttClient.setClient() rappele explicitement ensuite
//   pour garantir que PubSubClient resynchronise proprement son etat
//   interne (pas suppose implicite). AUCUN appel WiFi.* : ne touche pas a
//   l'association radio, pas de risque de collision avec maintainWiFi(),
//   pas besoin de wifiResetMutex (wifiClientMqtt n'est manipule que par
//   mqttTask()). Seuil delibere ni trop bas (perdrait la capacite a
//   distinguer un hoquet auto-resolu -- majorite des rc=-4 de cette nuit
//   se resolvent seuls a la tentative suivante) ni aussi haut que
//   l'escalade WiFi existante (code neuf jamais valide sur materiel,
//   prudence sur la frequence d'exposition tant que non confirme sain).
//   PAS ENCORE VALIDE SUR MATERIEL REEL au moment de ce commit.
//
// v150 - 2026-09-03 - safe-modify - Fix collision de retain MQTT
//   game/system/default/ingame (trouve par revue de code avant passage sur
//   master, jamais reproduit en direct). Ces 4 etats partageaient depuis
//   v148 le meme topic retenu marquee/cmd -- le retain MQTT etant PAR
//   TOPIC, 2 send_mqtt_retain() consecutifs (ex. rungame : "game" puis
//   "ingame") ecrasaient silencieusement le retain l'un de l'autre AU
//   NIVEAU DU BROKER, defaisant le fix de resynchronisation v123/
//   marquee.sh v17 (2026-08-23) des qu'une reconnexion survenait apres une
//   telle sequence. Fix : ces 4 etats recuperent chacun leur propre topic
//   retenu dedie (marquee/cmd/game|system|default|ingame), tout le reste
//   reste sur marquee/cmd (jamais retenu pour ces commandes, aucune
//   collision possible). Voir commentaires complets pres de
//   subscribeTopics[] et du dispatch dans onMqttMessage(). Cote script,
//   send_mqtt_retain() (marquee.sh) publie desormais vers
//   marquee/cmd/<nom> au lieu de marquee/cmd -- DEPLOIEMENT NON
//   RETROCOMPATIBLE, ce firmware ET marquee.sh doivent etre a jour EN MEME
//   TEMPS (meme contrainte que v148).
//   Nettoyage pre-merge master (meme revue de code) : le fix v149
//   (g_mqttConnectedScreenUntilMs=0 sur "vrai message recu") generalise a
//   CMD_SHOW_CONFIG/CMD_BRIGHTNESS/CMD_BRIGHTNESS_UP/CMD_BRIGHTNESS_DOWN
//   (ne le faisaient pas encore -- CMD_WIFI_RECOVERY/CMD_REBOOT non
//   concernes, ils redemarrent immediatement). g_mqttWaitingUntilMs/
//   MQTT_WAITING_GRACE_MS retires (code mort confirme -- inWaitingGrace,
//   leur seul lecteur, avait deja ete retire en v102). heartbeatTask()/
//   [DIAG] currentMode signales "obsoletes" par une revue automatisee mais
//   VERIFIES actifs (donnees hb= dans [SUBDIAG], transitions de mode dans
//   les logs de cette meme nuit) -- conserves, pas de suppression a
//   l'aveugle d'un outil de diagnostic pendant que le bug de fond MQTT
//   reste non resolu.
//
// v149 - 2026-09-03 - safe-modify - CMD_SCORE ne levait jamais l'ecran
//   d'attente "RecalBox connectee" (CMD_WAITING_MQTT/
//   g_mqttConnectedScreenUntilMs, v45 -- reste affiche indefiniment
//   jusqu'au "prochain vrai message MQTT"). CMD_GAME/CMD_SYSTEM/
//   CMD_DEFAULT levent tous les 3 ce drapeau (g_mqttConnectedScreenUntilMs
//   = 0), mais CMD_SCORE ne le touchait pas. Bug reel trouve en direct sur
//   materiel (retour utilisateur precis : "le message RecalBox connectee
//   avait remplace le marquee du jeu en cours dans la boucle d'affichage",
//   PENDANT une vraie partie) : si une reconnexion MQTT survient PENDANT
//   qu'un round-robin hiscore continue d'envoyer des CMD_SCORE (scenario
//   du soir), g_modeBeforeScore sauvegarde l'ecran "connectee" (mode actif
//   a ce moment) comme mode a restaurer -- le DMD boucle alors
//   indefiniment score/ecran-connectee sans jamais revenir au marquee
//   reel, jusqu'au prochain CMD_GAME/CMD_SYSTEM/CMD_DEFAULT (qui n'arrive
//   qu'au prochain changement de navigation/jeu, pas pendant une partie en
//   cours). Fix : g_mqttConnectedScreenUntilMs=0 ajoute dans CMD_SCORE,
//   cohabitant avec les 3 autres commandes qui le font deja -- un score
//   recu compte desormais comme un "vrai message" au meme titre,
//   coherent avec le commentaire de CMD_WAITING_MQTT lui-meme. Voir
//   DECISIONS.md pour le detail complet de l'episode (3 hypotheses
//   successives ecartees avant celle-ci, qui est verifiee par lecture de
//   code directe, pas une deduction).
//
// v148 - 2026-09-02 - safe-modify - FUSION des 12 topics marquee/cmd/*
//   (stop/default/system/game/show_config/wifi_recovery/reboot/
//   brightness/brightness_up/brightness_down/score/ingame) en UN SEUL
//   topic (marquee/cmd). Retour utilisateur explicite : plutot que de
//   continuer a chercher a corriger la cause racine du blocage TX post-
//   CONNACK (mur de plateforme atteint le meme soir, voir memoire projet
//   -- TCP_SND_BUF/SO_SNDBUF tous deux figes a la compilation), reduire
//   directement l'EXPOSITION -- 12 topics = jusqu'a 24 envois SUBSCRIBE
//   en rafale a chaque reconnexion, largement au-dessus du plafond
//   d'environ 16 segments TCP simultanement non-accuses trouve dans le
//   sdkconfig du core ce meme soir (v147). 1 seul topic = 1 seul
//   SUBSCRIBE par reconnexion (+ mqttEventTopic, le topic ES brut,
//   inchange car pas sous notre controle de publication).
//   Format du payload (pas de dependance JSON ajoutee, meme esprit que
//   extractField()/le topic evenement ES existant) : "CMD=<nom>
//   ARG=<reste du message>" -- CMD toujours en 1er (mot simple), ARG
//   toujours en dernier, prend tout le reste du message JUSQU'A LA FIN
//   (pas jusqu'au prochain espace) pour rester compatible avec des
//   arguments contenant espaces/pipes (score notamment). <nom> reprend
//   exactement les anciens suffixes de topic -- meme logique de
//   dispatch qu'avant, seule la SOURCE du nom de commande change (cmd
//   extrait du payload au lieu du topic MQTT lui-meme).
//   Cote scripts RB (marquee.sh/dmd_score.sh), tous les points de
//   publication doivent etre mis a jour en parallele pour publier vers
//   marquee/cmd avec ce nouveau format au lieu de marquee/cmd/<nom> --
//   voir leurs changelogs respectifs. PAS ENCORE TESTE SUR MATERIEL au
//   moment de cet ecrit (firmware ET scripts doivent etre deployes
//   ENSEMBLE, changement non retrocompatible dans un sens comme dans
//   l'autre).
//
// v147 - 2026-09-02 - safe-modify - TCP_SNDBUF releve a 16Ko sur le socket
//   MQTT (extension non-standard ESP-IDF), juste apres TCP_NODELAY (v146).
//   Chiffre trouve DIRECTEMENT dans le sdkconfig precompile du core
//   (esp32-libs/3.3.11/sdkconfig, pas une supposition) :
//   CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744, CONFIG_LWIP_TCP_MSS=1436 -- le
//   nombre de segments simultanement non-accuses autorises decoule de ce
//   ratio (formule lwIP standard ~4*SND_BUF/MSS, ~16 segments avec ces
//   valeurs), pas des octets bruts (nos ~12 topics a 23-30 octets ne
//   remplissent jamais 5744 octets en volume total, mais peuvent saturer
//   ce compteur de SEGMENTS si les accuses trainent -- deja confirme au
//   moins 2 fois cette soiree, CONNACK dont l'ACK client arrive assez
//   tard pour declencher une retransmission broker). TCP_SNDBUF releve
//   ce plafond PAR SOCKET sans reallocation immediate (lwIP alloue au
//   fil de l'eau) -- 16Ko choisi (pas les 28+Ko parfois cites en ligne)
//   vu le budget heap deja serre cette session (maxalloc ~4.6Ko en
//   continu). PAS ENCORE TESTE SUR MATERIEL au moment de cet ecrit.
//
// v146 - 2026-09-02 - safe-modify - RECHERCHE DE CAUSE RACINE (retour
//   utilisateur explicite : "on regle des consequences, pas la cause") --
//   TCP_NODELAY (desactive Nagle) applique sur le socket MQTT juste apres
//   connect() reussi, jamais fait nulle part dans ce fichier jusqu'ici.
//   Petits paquets SUBSCRIBE (23-30 octets, << MSS) + Nagle actif par
//   defaut = un nouveau segment reste retenu dans le buffer d'envoi local
//   tant que le precedent n'est pas accuse -- si un accuse est retarde
//   (deja confirme au moins 2 fois cette soiree : CONNACK dont l'ACK
//   client arrive assez tard pour declencher une retransmission broker),
//   les retries de mqttSubscribeFast() (2 par topic) s'empilent dans ce
//   MEME buffer jamais vide jusqu'a ce qu'il soit VRAIMENT plein --
//   correspond exactement au errno=EAGAIN observe partout (buffer plein,
//   pas juste "en attente"). Ne resout probablement pas pourquoi un
//   accuse peut etre retarde en premier lieu (cause encore inconnue,
//   lwIP/driver WiFi) mais retire un facteur d'amplification reel et
//   concret, sans aucun risque (reglage standard recommande pour les
//   protocoles MQTT). Complementaire aux delais v145 (pas un remplacement).
//   PAS ENCORE TESTE SUR MATERIEL au moment de cet ecrit.
//
// v145 - 2026-09-02 - safe-modify - Delai de stabilisation COURT (150ms,
//   vTaskDelay) insere juste apres mqttClient.connect() reussi, AVANT le
//   tout 1er subscribeChecked()/mqttSubscribeFast(). Motif : capture
//   tcpdump reelle (premiere fois disponible sur ce projet, RB1 -- voir
//   memoire projet pour le detail complet) a prouve au niveau paquet que
//   le SUBSCRIBE qui suit immediatement le CONNACK peut ne JAMAIS quitter
//   le DMD (0 octet sur le cable pendant 14s dans un cas observe), alors
//   que le CONNECT/CONNACK venait de s'echanger sans probleme sur la MEME
//   connexion quelques centaines de ms plus tot -- correle (2/2 cas
//   observes) avec un ACK client du CONNACK arrivant assez tard pour
//   declencher une retransmission broker. Hypothese : etat transitoire
//   cote lwIP/driver WiFi juste apres le CONNACK, pas un probleme reseau
//   externe (broker et Freebox tous deux ecartes avec preuve directe la
//   meme soiree). Volontairement PAS un reset socket/WiFi (casserait soit
//   la session MQTT en cours, soit deja teste et reverte, voir v134) --
//   juste laisser passer une courte fenetre avant de solliciter le TX.
//   PAS ENCORE VALIDE sur materiel au moment de cet ecrit.
//   SUITE (meme soir, tcpdump toujours actif) : 1er episode reel capture
//   apres flash -- le delai de 150ms fonctionne EXACTEMENT comme prevu
//   (CONNACK a .177904, 1er SUBSCRIBE parti a .352912, ~175ms plus tard,
//   confirme au paquet) et les 5 premiers topics (stop/default/system/
//   game/show_config) partent tous sans probleme. MAIS le 6e topic
//   (wifi_recovery) rebloque exactement comme avant -- le phenomene n'est
//   donc pas limite au tout premier envoi post-CONNACK, il peut survenir
//   apres plusieurs subscribe() reussis d'affilee aussi. Ajout : meme
//   delai court (30ms, plus petit que les 150ms initiaux car ici on
//   n'est plus juste apres le CONNACK/sa charge RX, juste entre 2 sends
//   qui s'enchainaient jusqu'ici sans aucune pause) apres CHAQUE
//   subscribe() reussi (les 2 branches OK 1er essai et OK retry), pas
//   seulement avant le tout premier. PAS ENCORE TESTE SUR MATERIEL au
//   moment de cet ajout.
//
// v144 - 2026-09-02 - safe-modify - ESCALADE WiFi reintroduite dans
//   mqttTask() (retiree en v134 faute de synchronisation, voir son
//   commentaire complet pres de SUBSCRIBE_BACKOFF_MAX_MS), cette fois
//   protegee par un vrai mutex (wifiResetMutex, voir sa declaration pres
//   de mqttCmdMutex) partage avec maintainWiFi() (loop(), coeur 1) --
//   plus aucun des 2 call sites WiFi.disconnect()/begin() ne peut
//   s'executer en meme temps que l'autre. Motif : investigation broker
//   (log mosquitto debug, session du 02/09 apres-midi) a confirme un
//   cycle connect(14s)/disconnect volontaire/recul(60s, plafond
//   SUBSCRIBE_BACKOFF_MAX_MS) qui se repete a l'identique pendant 10+
//   minutes sans jamais se resoudre tout seul -- le recul MQTT seul
//   (v104) n'a aucun mecanisme pour sortir de cette boucle quand la
//   cause depasse le niveau MQTT. Seuil d'escalade choisi expres
//   au-dela du plafond (15 cycles consecutifs, soit ~3 cycles pleinement
//   au plafond apres l'avoir atteint) pour laisser large priorite au
//   recul MQTT seul dans le cas courant. PAS ENCORE TESTE SUR MATERIEL
//   au moment de cet ecrit -- prochaine etape.
//
// v143 - 2026-09-02 - safe-modify - DIAGNOSTIC (pas de changement de
//   comportement fonctionnel) pour l'investigation rc=-4/blocage
//   subscribe() qui reste non resolue malgre l'elimination de RB1/de la
//   puce ESP32 individuelle comme causes (voir DECISIONS.md) -- delai
//   variable observe avant rechute (16min a 72min selon les sessions),
//   compatible avec une fuite de ressource qui s'accumule PAR CYCLE
//   connect()/subscribe() plutot qu'avec un facteur externe base sur le
//   temps. 3 ajouts : (1) `fd=` sur la ligne d'echec `mqttSubscribeFast`
//   -- si cette valeur ne fait QUE croitre au fil d'une session sans
//   jamais redescendre, preuve directe d'une fuite de socket (les fd
//   POSIX/lwIP sont normalement recycles au plus bas numero libre) ;
//   (2) `g_totalConnectAttempts` (nouveau compteur global PERSISTANT,
//   jamais remis a zero contrairement aux compteurs locaux existants de
//   mqttTask()) loggue sur chaque tentative connect() (reussie ou non) --
//   objectif : voir si la rechute correle avec un NOMBRE de cycles plutot
//   qu'avec le temps ecoule ; (3) `ESP.getMinFreeHeap()` (plancher
//   historique de heap libre, jamais suivi jusqu'ici) ajoute aux lignes
//   `connecting to`/`failed rc=`/`[LOOPDIAG]` -- complete `free`/
//   `maxalloc` deja suivis, qui ne disent rien du pire cas atteint au fil
//   de la session. PAS ENCORE TESTE SUR MATERIEL au moment de cet ecrit.
//
// v142 - 2026-09-01 - safe-modify - BUG REEL corrige par lecture de code
//   (DECISIONS.md, "angle mort du filet de securite retour playlist", trouve
//   31/08 soir, PAS ENCORE CORRIGE jusqu'ici) : le filet
//   MQTT_OFFLINE_FALLBACK_MS (60s, repli en playlist) mesurait le temps
//   depuis le dernier mqttClient.connect() REUSSI (lastMqttConnectedMs),
//   remis a zero a CHAQUE connect() reussi -- meme si tous les subscribe()
//   qui suivent echouent. Dans le pattern deja documente (connect() reussit
//   toutes les ~89s mais les subscribe() echouent en boucle, v98/v104), ce
//   chrono n'atteignait donc jamais son seuil : le DMD pouvait rester
//   bloque a boucler sur un vieux marquee indefiniment, sans jamais
//   retomber en playlist, alors qu'aucune commande utile n'etait recue
//   depuis potentiellement des heures. Fix : nouveau g_lastMqttUsefulMs
//   (voir sa declaration complete pres de mqttTaskHandle) mis a jour
//   uniquement sur une activite MQTT reellement UTILE -- un cycle de
//   subscribe() effectivement abouti (tous les topics souscrits, pas
//   seulement TCP connect()) OU un message MQTT reellement RECU
//   (onMqttMessage()) -- le filet verifie desormais CE timestamp, pas
//   lastMqttConnectedMs (laisse en place, inchange, juste plus consulte ici
//   -- risque nul a le garder). PAS ENCORE TESTE SUR MATERIEL en conditions
//   reelles de coupure reseau prolongee au moment de cet ecrit (le pattern
//   connect-OK/subscribe-KO en boucle n'est pas trivial a reproduire a la
//   demande) -- verifie seulement par relecture de code et compilation.
//
// v141 - 2026-09-01 - safe-modify - Suite immediate de v140 : 1er episode
//   reel capture au redemarrage (10:38:33, transitoire post-boot deja
//   documente) -- SO_ERROR=0 (connexion saine du point de vue noyau/lwIP,
//   aucune erreur pendante) MAIS l'echec est survenu bien plus vite que le
//   budget imparti (900ms max), suggerant que select() lui-meme peut
//   echouer/retourner tres vite (rc<0) plutot que d'attendre un vrai
//   timeout (rc=0 apres la duree complete) -- distinction invisible
//   jusqu'ici (les 2 cas remontaient identiquement "false" cote appelant).
//   socketWritableQuick() remonte desormais son rc/errno BRUT de select()
//   (parametres optionnels), logue en lastSelectRc/lastSelectErrno dans le
//   diagnostic ECHEC. PAS ENCORE TESTE SUR MATERIEL au moment de cet ecrit.
//
// v140 - 2026-09-01 - safe-modify - Suite de v139 (voir son commentaire
//   complet) : retour utilisateur, poursuite de l'investigation racine ("
//   pourquoi le socket ne devient jamais ecrit-pret"). mqttSubscribeFast()
//   interroge desormais SO_ERROR via getsockopt() quand l'envoi echoue
//   (jamais ecrit-pret dans le budget imparti) -- erreur socket PENDANTE
//   que ni select() ni un simple EAGAIN ne remontent autrement (getsockopt
//   la lit ET la remet a zero, seul moyen standard de la voir). Objectif :
//   distinguer si lwIP/lecoyau SAIT DEJA que la connexion est morte
//   (ECONNRESET/ETIMEDOUT/autre, SO_ERROR!=0) au moment du blocage, ou si
//   le socket se pretend sain (SO_ERROR=0) sans jamais progresser -- ces 2
//   cas orientent vers des causes tres differentes (etat socket deja connu
//   du noyau vs vrai blocage sans signal visible cote applicatif, plus
//   evocateur d'un probleme interne au driver WiFi/pilote radio). PAS
//   ENCORE TESTE SUR MATERIEL (attend le prochain episode reel).
//
// v139 - 2026-09-01 - safe-modify - RESULTAT DECISIF ronde 2 (voir v138 +
//   DECISIONS.md) : heartbeat CPU0 confirme hb reel~=hb attendu pendant un
//   subscribe() bloque -- CPU0 PAS affame, vrai blocage bas niveau. Sniffer
//   paquet maison (mini_sniffer2.py, tcpdump indisponible sur RB) localise
//   PRECISEMENT le blocage : CONNECT/CONNACK(rc=0) s'echangent parfaitement,
//   puis le DMD n'emet plus RIEN (pas meme un envoi partiel) pendant ~9-10s
//   -- le 1er SUBSCRIBE ne part jamais. Cause trouvee en lisant le code
//   source du coeur Arduino-ESP32 (NetworkClient.cpp, PubSubClient::write()
//   -> _client->write() -> NetworkClient::write()) : boucle de RETRY interne
//   au coeur, WIFI_CLIENT_MAX_WRITE_RETRY=10 x WIFI_CLIENT_SELECT_TIMEOUT_US
//   =1000000 (1s) = jusqu'a 10s d'attente via select() AVANT le moindre appel
//   send() si le socket ne devient jamais "ecrit-pret" -- correspond
//   EXACTEMENT aux essai1=8.7-10s/essai2=9.9-10s mesures. Ces constantes sont
//   des #define en dur dans un .cpp du coeur (pas overridable depuis ce
//   fichier). Fix : mqttSubscribeFast() -- construit et envoie le paquet
//   MQTT SUBSCRIBE (meme format que PubSubClient::subscribe(), QoS1) via un
//   send()/select() MAISON sur le fd brut de wifiClientMqtt, budget
//   BEAUCOUP plus court (3 essais x 300ms = 900ms max au lieu de 10s) --
//   remplace les 2 appels mqttClient.subscribe() dans subscribeChecked().
//   N'ecrit dans AUCUN etat interne de PubSubClient (compteur de paquet ID
//   independant) -- le broker traite un SUBSCRIBE bien forme identiquement
//   quel que soit qui l'a construit, aucune raison que la reception cote
//   broker s'en trouve affectee. Objectif : si le socket est reellement
//   bloque, echouer en <1s au lieu de 10s -- le cycle actuel de ~89s/echec
//   contient jusqu'a ~20-30s de blocage pur dans ces 2 appels, ce fix ne
//   resout pas la cause racine (toujours inconnue, probablement un etat
//   lwIP/socket interne au driver ESP-IDF, potentiellement declenche par le
//   CONNACK retransmis observe juste avant chaque episode) mais devrait
//   accelerer tres nettement la detection d'echec et donc la reconnexion.
//   PAS ENCORE TESTE SUR MATERIEL au moment de cet ecrit.
//
// v138 - 2026-08-26 - safe-modify - Heartbeat CPU0 (diagnostic bissection
//   MQTT round 2, voir DECISIONS.md) : v137 teste sur materiel (RB1 frais,
//   scripts head v25/v33) -- resultat NEGATIF, echec subscribe() reproduit
//   des t=~100s, heap HAUT (7016-9104 libres) pendant les echecs -- infirme
//   l'hypothese heap comme cause de ce mecanisme precis. Nouvelle tache
//   FreeRTOS independante (heartbeatTask(), coeur 0 = meme coeur que
//   loop(), tick toutes les 20ms) pour trancher : le CPU0 est-il vraiment
//   bloque en syscall reseau pendant les ~9-10s de subscribe(), ou juste
//   affame/preempte par autre chose (rendu GIF, lecture SD/SPI deja connue
//   bloquante) pendant ce temps ? Le SUBDIAG existant (v129-136) mesure le
//   temps ecoule AUTOUR de l'appel, pas le temps reellement passe dans le
//   blocage lui-meme -- ne peut pas distinguer les 2 hypotheses. Compteur
//   echantillonne avant/apres chaque tentative dans subscribeChecked(),
//   logue en "hb=<ticks reels>/<ticks attendus>" sur les 3 branches (OK 1er
//   essai, OK retry, ECHEC final). Note : une contention mqttTask-vs-loop()
//   sur le meme coeur avait deja ete testee une fois (v107, 18/08, deplace
//   mqttTask() entier vers le coeur 1) -- resultat REFUTE a l'epoque (meme
//   signature de tempete), mais ce test grossier ne mesurait pas
//   directement si CPU0 stallait au moment precis d'un subscribe() -- ce
//   heartbeat le mesure enfin, directement. NON ENCORE TESTE SUR MATERIEL
//   au moment de cet ecrit.
//
// v137 - 2026-08-26 - safe-modify - Reduction heap sysCachePerLetterVals
//   (piste trouvee en bissection MQTT round 2, voir DECISIONS.md) : cette
//   allocation (ajoutee en v124 "bucket", d7e5c34) coutait 8100 octets
//   permanents (BUCKET_COUNT=27 x SYS_CACHE_MAX=300), dimensionnee sur le
//   MEME plafond que sysCacheKeys/Vals/SlowVals alors que ces 3 tableaux
//   servent a TOUT systeme connu (jusqu'a 300, marge large et deja
//   necessaire) tandis que sysCachePerLetterVals ne sert QUE le detail
//   optionnel par bucket -- un systeme au-dela du nouveau plafond degrade
//   simplement vers le flag agrege par systeme (repli deja existant et
//   deja teste, aucune regression fonctionnelle). Nouveau plafond dedie
//   BUCKET_CACHE_MAX=96 (tres large pour le nombre reel de systemes
//   RecalBox, tout en coupant l'allocation a 2592 octets, -5508 vs avant).
//   TESTE SUR MATERIEL (RB1 frais, 26/08) : resultat NEGATIF -- echec
//   subscribe() reproduit des t=~100s de navigation reelle, cascade en
//   deconnexion forcee, HEAP HAUT (7016-9104 libres) pendant tous les
//   echecs -- infirme directement l'hypothese heap pour ce mecanisme
//   precis (voir DECISIONS.md pour le detail complet). Le patch reste une
//   amelioration heap legitime en soi (aucune regression fonctionnelle
//   constatee) mais n'est plus un candidat-fix pour le bug principal.
//
// v136 - 2026-08-24 - safe-modify - DIAGNOSTIC suite (retour utilisateur :
//   motif reproduit en boucle fiable ~30-90s sous v135 -- 10 cycles
//   consecutifs identiques observes 18:04-18:13, RSSI toujours sain (-22 a
//   -34 dBm) et AUCUNE ligne "[WIFI] STA disconnected" sur toute la
//   periode -- ecarte le WiFi radio/association comme cause directe.
//   Donnees [SUBDIAG] existantes (v129/v130) montrent en plus que le
//   blocage a lieu DANS l'appel bloquant mqttClient.subscribe() lui-meme
//   (write() qui n'obtient jamais d'accuse, ~9-10s plafond par tentative),
//   PAS une consequence d'un mqttTask() qui ne tournerait pas -- distinct
//   du mecanisme deja documente v83 (gifRawPackMode=1/CMD_GAME) : le GIF
//   affiche pendant cette reproduction passait par le chemin standard (voir
//   "[GIF] open OK standard", gifRawPackMode=false), pas par le rendu par
//   jeu. Ajoute une sonde independante du cache d'etat de PubSubClient :
//   wifiClientMqtt.connected() (le WiFiClient sous-jacent expose
//   directement, ligne 3380) interroge le socket brut via recv(MSG_PEEK)
//   cote driver, PAS le simple flag _state mis en cache par PubSubClient --
//   si les deux divergent au moment du blocage, ca situe precisement quelle
//   couche detecte la mort de la connexion en premier. WiFi.status()/RSSI()
//   et heap libre ajoutes aux memes points de controle (avant essai1, apres
//   echec essai1, a l'echec final) pour verifier l'etat radio EXACT pendant
//   le blocage (pas seulement au dernier connect(), plusieurs dizaines de
//   secondes plus tot). Diagnostic pur, aucun changement de comportement.
//
// v135 - 2026-08-24 - safe-modify - Diagnostic ajoute (proposition
//   utilisateur etudiee et validee point par point : "separer strictement
//   la recuperation MQTT de la recuperation WiFi... ajouter un diagnostic
//   minimal mais decisif : raison de deconnexion WiFi, RSSI, canal") --
//   jamais capture jusqu'ici cette session, tout ce qu'on avait etait
//   indirect. WiFi.onEvent() sur ARDUINO_EVENT_WIFI_STA_DISCONNECTED
//   (capture le code wifi_err_reason_t -- dira enfin SI et POURQUOI une
//   vraie deconnexion WiFi survient, distinct du cas "emission bloquee
//   mais status()=connecte" deja confirme cote broker). RSSI+canal ajoutes
//   aux 3 points de log MQTT existants (connecting/connected/failed) pour
//   correler l'etat radio exact avec chaque tentative. Diagnostic pur,
//   aucun changement de comportement.
//
// v134 - 2026-08-24 - safe-modify - URGENT : v132/v133 (force WiFi.
//   disconnect()/begin() depuis mqttTask()) REVERTES ENTIEREMENT (retour
//   utilisateur : "on a plein de wifi reconnect mais ca ne fonctionne pas"
//   -- observe en direct : APRES le declenchement du fix, le WiFi lui-meme
//   s'est mis a decrocher reellement en boucle serree, ~5s d'intervalle,
//   alertes "No wifi" recurrentes -- situation NETTEMENT PIRE qu'avant ce
//   fix). Cause tres probable : collision entre WiFi.disconnect()/begin()
//   appeles depuis mqttTask() (coeur 0) et le meme appel deja fait par
//   maintainWiFi() (tache loop(), coeur 1) sans aucune synchronisation
//   entre les 2 -- pas documente thread-safe. Retire completement (les 2
//   sites, plus les variables/constantes devenues inutilisees) -- seul le
//   mqttClient.disconnect() (niveau MQTT/socket, deja existant avant v132,
//   aucun risque de concurrence WiFi) subsiste. v131 (WiFi.setSleep(false)
//   reapplique dans maintainWiFi() ELLE-MEME, un seul point d'appel,
//   aucune concurrence) reste en place, non concerne par ce revert. Voir
//   DECISIONS.md pour le detail complet.
//
// v133 - 2026-08-24 - safe-modify - v132 ETENDU (retour utilisateur : "le
//   dmd est en rc -2" -- persiste plusieurs minutes AVANT MEME d'atteindre
//   l'etape subscribe, donc le filet v132 axe sur
//   consecutiveSubscribeFailCycles ne peut structurellement rien pour ce
//   cas precis, jamais atteint). Meme dernier recours (WiFi.disconnect()/
//   begin()/setSleep(false)) ajoute pour le cas ou mqttClient.connect()
//   lui-meme echoue de facon soutenue (rc=-2/-4) -- declenche par une DUREE
//   (WIFI_FORCE_RESET_CONNECT_FAIL_MS=90s, ~6 tentatives a MQTT_RETRY_MS=
//   15s) plutot qu'un nombre de cycles, puisque ce cas n'incremente aucun
//   compteur de cycles dedie existant.
//
// v132 - 2026-08-24 - safe-modify - BUG REEL confirme en direct (retour
//   utilisateur : "il se connecte bien au boot mais jamais a la RB" --
//   coince en boucle "3 subscribe() en echec" sans jamais atteindre une
//   connexion saine, meme apres un power-cycle physique complet, meme
//   signature exacte des blocages ~10s que celle chassee toute la soiree,
//   voir DECISIONS.md). Le cycle de recul existant (mqttClient.disconnect()
//   + backoff progressif, v98/v104) reste au niveau MQTT/socket seul -- ne
//   force jamais de reinitialisation WiFi complete. Ajoute : au-dela de
//   WIFI_FORCE_RESET_AFTER_SUBSCRIBE_FAIL_CYCLES cycles consecutifs, force
//   un vrai cycle WiFi.disconnect()/begin()/setSleep(false) (meme motif
//   que maintainWiFi()) en plus du disconnect MQTT existant -- dernier
//   recours quand le niveau MQTT seul ne suffit jamais a se debloquer.
//
// v131 - 2026-08-24 - safe-modify - Piste WiFi power-save reprise (retour
//   utilisateur : reprendre les recherches sur ce sujet) : WiFi.setSleep
//   (false) (fix documente Espressif pour exactement ce symptome -- TX
//   bloque pendant plusieurs secondes, radio endormie jusqu'au prochain
//   DTIM) etait deja pose une fois au tout premier boot, mais JAMAIS
//   reapplique apres un WiFi.disconnect()+WiFi.begin() -- ni dans
//   maintainWiFi() (reconnexion WiFi en cours d'uptime), ni dans les
//   retries de la boucle de connexion initiale. WiFi.disconnect() peut
//   reinitialiser cet etat cote driver (meme famille de bug deja identifie
//   et corrige pour l'IP fixe, qui elle etait deja reappliquee). Reapplique
//   aux 2 endroits. PAS CONFIRME comme LA cause exacte du probleme
//   keepalive/emission chasse toute la soiree (aucun WiFi.status()!=
//   WL_CONNECTED observe pendant les episodes, mais l'etat power-save reel
//   n'est pas observable depuis l'API Arduino) -- fix a cout nul, applique
//   par coherence/prudence. Voir DECISIONS.md pour le detail complet de
//   l'enquete.
//
// v130 - 2026-08-24 - safe-modify - DIAGNOSTIC suite : v129 mesure sur
//   materiel confirme que les 2 premiers subscribe() apres connect()
//   bloquent chacun ~9-10s SUR CHAQUE tentative (pas une lenteur broker qui
//   repond tard -- une ecriture qui attend un accuse jamais recu), le 3e
//   echouant instantanement (4/0ms). Ajoute ici : etat connected() juste
//   AVANT chaque tentative de subscribe, pour voir si la connexion est deja
//   tombee cote client avant meme d'ecrire.
//
// v129 - 2026-08-24 - safe-modify - DIAGNOSTIC (pas un fix) : le fix v128
//   (delay(1)) n'a EU AUCUN EFFET mesurable sur le cycle de deconnexion
//   "3 subscribe() en echec" -- verifie en direct, intervalle connect->echec
//   quasi identique (~38.8s) avant ET apres, sur des contenus differents --
//   invalide l'hypothese "contention SD/rendu rawpack" du v128. Timing fin
//   ajoute par tentative de subscribe() (voir subscribeChecked()) pour voir
//   EXACTEMENT ou passe le temps au lieu de continuer a deviner.
//
// v128 - 2026-08-24 - safe-modify - Deconnexions MQTT courtes pendant la
//   veille gameclip (retour utilisateur : "on a ce probleme depuis les
//   ajouts", confirme en direct -- subscribe() en echec observe 12min apres
//   un boot RB1, en pleine sequence gameclip 30s). Cause trouvee : les 2
//   boucles d'attente inter-frames (case MODE_PLAYLIST et case MODE_GIF)
//   utilisaient delay(0) (~taskYIELD(), quasi aucun temps rendu a
//   l'ordonnanceur) -- gifRawPackMode=1 (rendu marquee par jeu, sollicite en
//   continu par gameclip/demo depuis le cablage veille de cette session,
//   auparavant reserve a une vraie partie jouee) fait un SD.read() a CHAQUE
//   frame via drawGifRaw565Frame(), dont le propre commentaire (deja present
//   avant cette session) documente ce point precis comme "le point de
//   contention le plus frequent... deadlock mqttTask/LWIP". delay(0)->
//   delay(1) aux 2 sites (vrai yield d'au moins 1 tick) -- n'affecte pas la
//   duree totale d'attente ni la fluidite visible, laisse une fenetre reelle
//   au traitement WiFi/LWIP pendant les lectures SD repetees. Voir
//   DECISIONS.md pour le detail complet de l'enquete.
//
// v127 - 2026-08-24 - safe-modify - broadcastFeatureStatus() : methode
//   ENTIEREMENT REVUE (retour utilisateur explicite, meme demande que v126 :
//   "la methode est a revoir", pas juste un patch). Validation live du
//   garde v126 la nuit meme : 3 troncatures reelles observees sur le
//   broker (ex. "2;dwell_seconds=3"), toujours meme signature (prefixe de
//   longueur variable perdu, suffixe intact) -- compatible avec un echec
//   de reallocation en cours d'une longue chaine de ~20 concatenations
//   String. Remplace par un buffer FIXE sur la pile (snprintf) -- ZERO
//   allocation heap pour construire ce message, supprime le mecanisme
//   suspecte a la racine plutot que de continuer a le contourner apres
//   coup. Garde de sanite v126 conservee en filet de securite
//   complementaire (cout negligeable). Voir le commentaire complet pres de
//   la fonction pour le detail.
//
// v126 - 2026-08-23 - safe-modify - BUG REEL RECURRENT corrige (retour
//   utilisateur explicite : "souci rencontre de multiples fois... la
//   methode est a revoir pour le transfert des reglages") :
//   broadcastFeatureStatus() (marquee/status/features, topic retenu lu par
//   dmd_score.sh cote RB pour savoir quels panneaux hiscore/info/
//   description/RA afficher) pouvait publier une valeur TRONQUEE -- confirme
//   en direct sur materiel : la valeur RETENUE sur le broker elle-meme
//   etait deja tronquee ("2;dwell_seconds=3" au lieu des 11 champs
//   complets), pas seulement une corruption du cache local du script RB.
//   Consequence reelle observee : TOUS les panneaux info/description
//   disparaissaient d'un coup, sur TOUS les jeux, jusqu'a republication
//   manuelle de la valeur correcte. Cause racine exacte non confirmee avec
//   certitude (suspect : concatenation String sous pression heap, ce point
//   du code tourne juste apres un connect() MQTT reussi). Garde de sanite
//   ajoute AVANT le publish (startsWith/longueur minimale) : n'envoie
//   jamais une valeur visiblement cassee. Complement cote RB : dmd_score.sh
//   v30 rejette aussi tout message incomplet a la reception (cache existant
//   conserve plutot qu'ecrase par du contenu douteux) -- defense sur les 2
//   bouts de la chaine, la cause exacte de la troncature restant a
//   confirmer avec certitude si elle se reproduit malgre ces gardes.
//
// v125 - 2026-08-23 - safe-modify - RETRO_VERSION (splash boot, ecran
//   physique du DMD) passee de "Raw565 Ed. devCORE0" a "Raw565 Ed. dev13"
//   (retour utilisateur, etiquette de build informative uniquement, aucun
//   changement de comportement).
//
// v124 - 2026-08-23 - safe-modify - Chantier "bucket" : flag "lent" (L,
//   declenche l'ecran masque d'attente pendant le chargement d'un jeu)
//   calcule et stocke PAR SOUS-DOSSIER ALPHABETIQUE (bucket, A..Z/#) au
//   lieu de PAR SYSTEME ENTIER -- penalisait inutilement un sous-dossier
//   peu peuple des qu'un AUTRE sous-dossier du meme systeme faisait
//   depasser le seuil au total agrege. Porte depuis le worktree
//   dev-slow-flag-per-bucket (firmware v37, jamais merge ni teste sur
//   materiel reel -- compile seulement) vers dev-core-reassignment,
//   adapte a l'etat actuel du fichier (divergence significative depuis
//   le point de depart de cette branche dev, tout le travail hi-score/
//   resync/alertes de cette session n'existait pas encore cote bucket) :
//   - Nouvelles structures : BUCKET_COUNT=27, BUCKET_LETTERS[]=
//     "#ABCDEFGHIJKLMNOPQRSTUVWXYZ" (doit rester synchro avec LETTERS
//     cote RecalBoxDMD_tool.py), sysCachePerLetterVals[SYS_CACHE_MAX][27]
//     (heap, malloc dans setup() a cote des autres tableaux sysCache*).
//   - systems_cache.dat gagne un 4e champ optionnel (27 caracteres
//     'L'/'N', ordre BUCKET_LETTERS) : "<val> <sysName> <slowFlag>
//     <bucketFlags27>". Retrocompatibilite dans les 2 sens (comme le
//     plan d'origine) : un firmware NON MODIFIE qui lit un fichier a 4
//     champs continue de fonctionner (charAt(0) sur le 3e champ, ignore
//     silencieusement le 4e) ; un firmware MODIFIE qui lit un ANCIEN
//     fichier a 2/3 champs bascule automatiquement en repli sur le flag
//     systeme agrege pour tous les buckets. Validation par caractere (un
//     octet isole invalide/corrompu ne casse que ce bucket-la).
//   - Nouvelles fonctions : bucketLetterForFilename(fname), sysBucketSlowFlag
//     (sysName, bucketLetter) (lookup par bucket avec repli sur
//     sysCacheSlowVals[i]) -- celle-ci appelle desormais aussi
//     ensureSysDefaultCacheLoaded() (mecanisme de chargement paresseux
//     v43, posterieur au plan d'origine cote bucket -- absent de la
//     branche dev source, ajoute ici pour rester coherent avec
//     sysDefaultType()/sysDefaultSlowFlag()).
//   - 2 call sites mis a jour (les seuls existants, verifie identiques a
//     ceux de la branche source) : CMD_GAME et drawPng(). Logs Serial
//     enrichis d'un champ "bucket=".
//   - BUG REEL EVITE (trouve en portant, absent de la branche source) :
//     buildSysDefaultCache() (repli firmware, emprunte UNIQUEMENT si
//     systems_cache.dat est absent au boot) ne calcule aucune donnee par
//     bucket (decision actee, comme dans le plan d'origine) -- MAIS sans
//     initialiser sysCachePerLetterVals pour ces entrees, celui-ci
//     restait de la memoire heap NON INITIALISEE (malloc, pas calloc)
//     jusqu'au prochain reboot (loadSysDefaultCache() memset scorrectement
//     a '?' sur un fichier a 3 champs, mais ce chemin de repli n'appelle
//     jamais loadSysDefaultCache()) -- sysBucketSlowFlag() aurait pu lire
//     un octet parasite egal par hasard a 'L'/'N'. Fix : memset('?', ...)
//     explicite ajoute dans buildSysDefaultCache(), meme sentinel que le
//     parseur.
//   - Cote outil PC : voir changelog RecalBoxDMD_tool.py (meme plan,
//     seuil par defaut ramene de 5000 a 800 -- voir RecalBoxDMD_prefs.py
//     v8/RecalBoxDMD_GUI.py v50, retour utilisateur explicite : le 5000
//     n'avait de sens que pour l'ancien calcul par systeme entier).
//
// v123 - 2026-08-23 - safe-modify - Resynchro RB au demarrage/reconnexion
//   (retour utilisateur : verifier que le DMD ne repasse pas en playlist
//   locale alors qu'un jeu tourne reellement -- diagnostic complet dans
//   DECISIONS.md). marquee/cmd/ingame REABONNE (retire en v104 avec le
//   sous-systeme hi-score overlay, mais marquee.sh continuait de le publier
//   fidelement en retenu -- canal mort cote firmware jusqu'ici) : nouveau
//   booleen g_recalboxInGame, simple etat courant (pas un g_pendingX, pas de
//   redessin associe). Utilise en 2e ligne de defense dans CMD_DEFAULT : un
//   "default" recu alors que g_recalboxInGame est vrai est ignore au lieu de
//   faire basculer le marquee en playlist. Le vrai fix est cote script RB
//   (marquee.sh v17, handler start) : voir son changelog -- il forcait
//   "default" sans condition a chaque (re)demarrage du script (reboot RB OU
//   simple crash/relance ES en jeu), sans jamais verifier si une partie
//   etait reellement en cours (Action=rungame dans es_state.inf, jamais lu
//   jusqu'ici par ce script).
//
// v122 - 2026-08-23 - safe-modify - Alertes connectivite (retour
//   utilisateur) : (1) duree d'affichage des 3 alertes (verte "RecalBox
//   connectee", orange "hors ligne", rouge "pas de wifi") ramenee de 7s a
//   5s (NO_WIFI_ALERT_DISPLAY_MS et MQTT_WAITING_MIN_DISPLAY_MS, 7000->5000) ;
//   (2) texte "RecalBox connectee :)" passe de blanc a vert
//   (drawRecalboxConnectedOverlay(), 255,255,255 -> 0,255,0). Purement
//   cosmetique/timing, aucun changement de logique de declenchement.
//
// v121 - 2026-08-22 - safe-modify - (1) SHUFFLE_ENABLED remis a true : le
//   coupe-circuit anti-rafale de marquee.sh (v13, seuil=10/s) est reactive
//   cote RB et republie donc "!SHUFFLE" en rafale de navigation, or ce flag
//   etait reste a false (desactivation temporaire du 2026-08-19) -- l'echec
//   gracieux predit par le commentaire de v109 (repli sur default.png) ne
//   se produisait pas comme attendu : ecran NOIR signale par l'utilisateur
//   au lieu du GIF shuffle pendant la navigation rapide. (2) Lignes de
//   l'ecran DESCRIPTION CENTREES horizontalement au lieu d'alignees a
//   gauche (retour utilisateur explicite), largeur calculee via la somme
//   des avances TomThumb reelles de chaque ligne.
// v120 - 2026-08-22 - safe-modify - Score ALIGNE A DROITE dans le rendu
//   generique nom/score (retour utilisateur explicite : "aligner les
//   scores a droite de l'ecran car la ils se decalent en fonction de la
//   longueur des noms" -- visible surtout sur le classement RB CHALLENGE,
//   plusieurs rangs empiles avec des noms de longueurs tres variables).
//   Position du score desormais calculee depuis le BORD DROIT de l'ecran
//   (RAW565_W - longueur_score*6 - 1) au lieu de juste apres le nom --
//   applique aux 2 rendus qui partagent cette convention (rang generique
//   dans drawScoreScreen(), et rang 2 de l'ecran hi-score special). Repli
//   sur la fin du nom si le score deborderait a gauche (defense en
//   profondeur, ne devrait pas arriver avec les noms deja tronques cote
//   RB).
//
// v119 - 2026-08-20 - safe-modify - 2 ajustements suite au 1er retour
//   utilisateur sur v118 : (1) espacement inter-caractere DESCRIPTION
//   reduit ("reduie l'espace entre les lettre un petit peu") -- le +1px
//   manuel ajoute en v118 au-dela de l'avance native TomThumb est retire ;
//   (2) couleur DESCRIPTION FIXEE en gold ("on garde la couleur gold pour
//   le texte") -- remplace le test A/B blanc/or temporaire de v118. Voir
//   drawScoreScreen()/isDescriptionScreen.
//
// v118 - 2026-08-20 - safe-modify - Org_01 (v117) RETIRE suite au retour
//   utilisateur ("cette police semble avoir un espacement entre les mots
//   important et ca rompt avec le style general") -- retour a TomThumb
//   (v114) avec 2 ameliorations de lisibilite tentees ("essaye
//   d'ameliorer la police thumb") : espacement inter-caractere elargi
//   manuellement (dessin caractere par caractere, nouvelle fonction
//   tomThumbCharAdvance()) + rendu tout en MAJUSCULES. Test A/B temporaire
//   ajoute a la demande de l'utilisateur (alternance blanc/or par ligne
//   de contenu, "pour voir la difference de facilite de lecture") --
//   tranche en v119 (voir ci-dessus).
//
// v117 - 2026-08-20 - safe-modify - 2 ajustements suite au 1er retour
//   utilisateur sur v114-v116 : police DESCRIPTION Org_01 essayee a la
//   place de TomThumb ("un peu dure a lire") ; couleur titleColor rendue
//   plus vive ("elle se rapproche trop du blanc") -- bleu-violet sature
//   (90,90,255) au lieu du cyan pale (120,220,220).
//
// v116 - 2026-08-20 - safe-modify - BUG REEL corrige (retour utilisateur :
//   "la page 2 hi-score n'a pas son titre") -- le titre doit PERSISTER sur
//   toutes les pages d'un meme contenu (regle deja actee pour INFOS/
//   DESCRIPTION). Le titre "HI-SCORE" est desormais envoye sur les 2 pages
//   (dmd_score.sh v12) ; la distinction page1/page2 se fait maintenant sur
//   le CONTENU (rest commence par "1 " = rang 1 present = page 1) plutot
//   que sur le titre (etait auparavant vide sur la page 2 specifiquement
//   pour eviter de declencher le rendu special "rang 1 en gros").
//
// v115 - 2026-08-20 - safe-modify - Palette de couleurs UNIFIEE entre tous
//   les ecrans MODE_SCORE (retour utilisateur explicite : "unifie les
//   couleurs des differents elements entre les tableaux" + "le titre ne
//   doit pas avoir la meme couleur que les sous-titres/le texte --
//   actuellement les scores sont de la meme couleur que le titre") :
//   nouvelle couleur dediee titleColor pour TOUS les titres (etait 'gold',
//   partagee a tort avec les scores) ; INFOS bascule sur le meme motif
//   white(libelle)/gold(valeur) que hi-score(nom/score) au lieu de sa
//   propre palette dediee -- 3 roles coherents partout desormais
//   (titre/libelle/valeur), sauf le rang 1 (couleur d'emphase deliberee,
//   inchangee).
//
// v114 - 2026-08-20 - safe-modify - Police compacte TomThumb (3x5px,
//   Adafruit_GFX_Library) pour l'ecran DESCRIPTION -- demande utilisateur
//   explicite : "reduire la taille des caracteres du contenu de
//   descriptions pour afficher plus de texte par ligne" -- ~4px/caractere
//   au lieu de 6px, ~30 caracteres/ligne au lieu de ~21 (WRAP_WIDTH ajuste
//   en consequence cote dmd_score.sh v11). Positionnement adapte (ligne de
//   base, pas coin superieur-gauche). N'affecte QUE l'ecran DESCRIPTION --
//   hi-score/infos/titre restent sur la police classique.
//
// v113 - 2026-08-20 - safe-modify - Page hi-score corrigee suite au 1er
//   test reel de v112 : "le rang 1 doit etre affiche en taille 2 [ligne],
//   pas sur 2 lignes, le titre a disparu". Titre "HI-SCORE" restaure
//   (retire par erreur en v111 lors de la simplification "1 page avec
//   rang 1 plus gros") ; rang 1 desormais sur UNE SEULE ligne en taille 2
//   (nom+score ensemble) au lieu de nom taille 1 + score taille 2 sur 2
//   lignes distinctes.
//
// v112 - 2026-08-20 - safe-modify - 2 corrections de rendu suite au 1er
//   test reel du fix currentMode (v111) :
//   (1) BUG REEL corrige : currentMode = MODE_SCORE manquant (perdu
//   pendant la restructuration v111 pour le prefixe "@ms|") -- le texte se
//   dessinait bien un instant mais currentMode ne changeait jamais,
//   laissant loop() continuer a redessiner la frame suivante du marquee
//   par-dessus des l'iteration suivante ("flash" d'une seule frame
//   rapporte par l'utilisateur, confirme via un detecteur generique de
//   transition qui ne loguait JAMAIS de changement malgre un texte
//   visible). Root cause identifiee par l'utilisateur lui-meme ("le
//   rawpack continue a etre lu normalement, pas de redemarrage de
//   l'animation") avant meme la confirmation par instrumentation.
//   (2) Palette INFOS corrigee : "chaque ligne a une couleur c'est pas
//   bon, garde les sous-titre d'une couleur et le texte d'une autre" --
//   remplace la rotation 3 couleurs/ligne par un split libelle/valeur
//   coherent (2 couleurs fixes, coupe au premier ":").
//   (3) Ecran DESCRIPTION corrige : "ya des mots qui sont de la meme
//   couleur que le titre, enleve ca (dernier mot de chaque ligne)" --
//   tombait a tort dans le rendu generique hi-score (split au dernier
//   espace, dernier mot en or comme un score) -- rendu dedie ajoute,
//   1 seule couleur, aucun split.
//
// v111 - 2026-08-20 - safe-modify - 3 ajouts a drawScoreScreen()/CMD_SCORE,
//   demande utilisateur explicite en reponse au constat "hi-score tronque a
//   3/5 rangs, description/infos inutilisables (pas de retour a la ligne)" --
//   AUCUN etat/timing anime introduit (chaque appel reste un rendu statique
//   independant, coherent avec la philosophie "DMD bete" v110) :
//   (1) Duree d'affichage PAR MESSAGE optionnelle -- prefixe "@<ms>|" en
//   tete du payload CMD_SCORE (SCORE_DURATION_OVERRIDE_MIN/MAX_MS, bornes
//   500-15000ms), lit par dmd_score.sh v6 pour un "scroll bete" pagine
//   cote script (contenu trop long pour 4 lignes = plusieurs pages
//   statiques envoyees en remplacement l'une de l'autre, PAS de defilement
//   firmware). Sans prefixe, comportement inchange (SCORE_DISPLAY_DURATION_MS).
//   (2) Hi-score : rang 1 mis en valeur -- nom taille normale + SCORE en
//   gros (taille 2) et couleur dediee (orange, distincte de l'or standard),
//   rang 2 en dessous en taille normale (8+16+8=32px, tient exactement).
//   Plus de titre "HI-SCORE" affiche a part (consomme comme marqueur
//   uniquement -- gagne la place pour le rang 2). Declenche uniquement si
//   le titre du payload vaut exactement "HI-SCORE" ET qu'il y a du contenu
//   apres -- une page de continuation (titre VIDE, rangs 3/4/5) tombe
//   naturellement dans le rendu generique inchange.
//   (3) Palette dediee pour les ecrans INFOS (titre == "INFOS") : 3
//   couleurs en rotation par ligne (bleu clair/vert/ambre) au lieu du
//   split nom/score generique (n'avait pas de sens pour un format "Label:
//   valeur") -- nouvelle palette dessinee ici, l'ancien systeme
//   (startGameInfoOverlay(), retire en v104) etant introuvable dans
//   l'historique git malgre recherche approfondie.
//
// v110 - 2026-08-19 - safe-modify - CMD_SCORE reintroduit, version "DMD
//   bete" (demande utilisateur explicite, apres diagnostic du bug
//   CMD_STARTCLIP fantome sur l'autre worktree dev-mame-score-mqtt-bridge --
//   voir memoire projet project_core_reassignment_rb_script_mismatch) :
//   CMD_GAME/l'affichage du jeu restent 100% INCHANGES, geres par le DMD
//   exactement comme avant (aucune nouvelle dependance ajoutee a ce chemin).
//   Le score est ajoute en PUR PASSIF par-dessus : nouveau topic
//   marquee/cmd/score, nouveau MODE_SCORE d'affichage plein ecran (4 lignes,
//   parsees sur "|"), utilise le slot pendingCmd generique deja existant
//   (comme avant le retrait v104 -- perte occasionnelle sous rafale
//   deja acceptee, le score n'est jamais "ce qui doit etre affiche" a la
//   difference de default/system/game qui ont leurs slots dedies). AUCUNE
//   dependance au canal marquee/event/CMD_STARTCLIP/CMD_RESUMESYS (deja
//   presents avant ce commit, non touches ici) -- pas de garde
//   g_inGameMarquee/Event-topic re-introduit, cause structurelle exclue.
//   Garantie anti-blocage demandee explicitement par l'utilisateur : retour
//   AUTOMATIQUE au jeu apres SCORE_DISPLAY_DURATION_MS (timer 100% LOCAL au
//   DMD, ne depend d'AUCUN message RB ulterieur) -- si le script RB qui
//   publie le score meurt/retarde/echoue, le DMD ne reste JAMAIS bloque sur
//   un ecran de score perime, il revient de lui-meme au jeu.
//
// v104 - 2026-08-17 - safe-modify - BUG REEL confirme sur materiel EN DIRECT
//   (observation utilisateur : "reconnection validee ds le serial mais
//   fausse sur le dmd, pas de reaction au changement de jeu") : la
//   deconnexion forcee (v94/v98) n'avait AUCUN recul -- log confirme : TCP
//   se reconnecte vite (broker local, ~14ms, connexion reellement saine a
//   CE niveau), mais les subscribe() echouent A NOUVEAU sur cette connexion
//   fraiche ~19s plus tard (meme cause sous-jacente que le rc=-4/-2 deja
//   documente, non resolue), re-declenchant la meme deconnexion forcee --
//   boucle infinie toutes les ~39s observee, jamais d'etat stable ou les
//   souscriptions tiennent, DMD jamais reellement abonne a rien malgre
//   "connected" affiche en boucle. Fix : compteur de cycles d'echec
//   consecutifs (consecutiveSubscribeFailCycles), recul progressif
//   (5s x compteur, plafonne a 60s) avant de retenter apres une
//   deconnexion forcee -- reinitialise a 0 des qu'un cycle complet
//   reussit (souscriptions OK). PAS ENCORE VALIDE sur materiel au moment
//   d'ecrire cette entree.
//
// v103 - 2026-08-17 - safe-modify - RESOLUTION du mystere documente en v99/
//   v102 ("CMD_GAME/CMD_SYSTEM silencieux specifiquement post-reconnexion") :
//   inWaitingGrace (fenetre de grace 1.5s, g_mqttWaitingUntilMs, re-armee a
//   CHAQUE reconnexion via CMD_WAITING_MQTT, pas juste au 1er boot) filtrait
//   ENCORE system/game -- alors que "default" avait deja ete explicitement
//   retire de ce meme filtre en v45 pour EXACTEMENT la meme raison ("un
//   message RETENU reflete toujours le dernier etat REEL connu de RB,
//   jamais faux en soi"), argument qui s'applique identiquement a
//   system/game (juste oublies lors de ce fix v45, ou lors du refactor v96/
//   v97 qui a introduit les slots dedies). Preuve : reproduit plusieurs
//   fois (karatour, PUIS le MEME jeu "ctribe" observe echouant silencieux
//   la 1ere fois dans la rafale post-reconnexion, PUIS reussissant
//   parfaitement (log [DIAG] complet) quelques dizaines de secondes plus
//   tard hors rafale, sur le MEME boot). Fix : system/game retires du
//   filtre inWaitingGrace, meme traitement que default depuis v45.
//   PAS ENCORE VALIDE sur materiel au moment d'ecrire cette entree.
//
// v102 - 2026-08-17 - safe-modify - Test empirique approfondi (demande
//   utilisateur : "desactive plus profondement overlay", "pas que
//   l'affichage") : score/game_info/achievement ne sont plus stockes DU
//   TOUT a la reception (CMD_SCORE/CMD_GAME_INFO/CMD_ACHIEVEMENT) -- avant,
//   seul l'affichage etait bloque (scoreEnabled/gameInfoEnabled/
//   achievementEnabled=false) ou, pour game_info, seul le champ DESCRIPTION
//   etait filtre (v96-v99) -- desormais rejet complet, aucune allocation/
//   copie String, juste un log de reception (longueur seule). Objectif :
//   isoler si la RECEPTION seule (sans stockage ni affichage) contribue a
//   l'instabilite MQTT observee. A RETIRER (revenir au stockage normal +
//   conditions scoreEnabled/gameInfoEnabled/achievementEnabled) une fois le
//   test conclu. PAS ENCORE VALIDE sur materiel au moment d'ecrire cette
//   entree.
//
// v101 - 2026-08-17 - safe-modify - BUG REEL confirme sur materiel EN DIRECT
//   (observation live utilisateur + confirmation log) : connexion "zombie"
//   -- silence total >2min30, aucune tentative de reconnexion, ecran DMD
//   fige sur "RB connectee" -- mqttClient.connected() restait true (TCP a
//   moitie mort, pair disparu sans FIN/RST propre). Aucun garde-fou
//   existant (v94, verifie seulement au moment du connect()) ne peut
//   detecter un silence qui s'installe plus tard en cours de session.
//   Cause racine trouvee : mqttClient.setKeepAlive(60) (60s, sans
//   justification documentee dans le code, tres au-dessus des 15s par
//   defaut de la librairie) rendait le SEUL mecanisme capable de detecter
//   ce cas (keepalive PINGREQ/PINGRESP interne de PubSubClient) beaucoup
//   trop lent -- ~1.5-2x ce delai (90-120s+) avant meme de COMMENCER une
//   vraie reconnexion, coherent avec les silences de plusieurs minutes
//   observes plusieurs fois ce soir. Fix : setKeepAlive(15) (retour au
//   defaut librairie) -- cout reseau negligeable, detection ~4x plus
//   rapide.
//   EN PLUS (meme version, demande utilisateur) : test empirique -- overlay
//   COMPLETEMENT desactive a l'affichage (score+game_info force false a la
//   lecture de config.ini quelle que soit sa valeur reelle -- A RETIRER une
//   fois le test conclu -- achievement via son nouveau toggle, voir
//   ci-dessous). Objectif : verifier si la stabilite MQTT s'ameliore sans
//   AUCUNE activite overlay (heap/CPU).
//   Nouveau toggle web "achievementEnabled" (ACHIEVEMENT_ENABLED dans
//   config.ini, meme pattern que scoreEnabled/gameInfoEnabled v86) --
//   RetroAchievements n'avait jusqu'ici aucune option de desactivation.
//   Defaut false (comme les 2 autres a leur introduction) -- couvre aussi
//   de facto le test empirique ci-dessus pour ce 3e volet de l'overlay.
//   PAS ENCORE VALIDE sur materiel au moment d'ecrire cette entree.
//
// v100 - 2026-08-17 - safe-modify - 8e CRASH REEL confirme sur materiel (hash
//   ELF verifie + addr2line, reproduit 2x d'affilee) en testant v99 : crash
//   COMPLETEMENT DIFFERENT de tous les precedents cette session -- pas dans
//   notre code, mais DANS le driver WiFi d'ESP-IDF lui-meme (timer_task()
//   interne -> ieee80211_timer_process()/pp_timer_process() -> wifi_log()
//   tente d'ecrire un log diagnostique interne -> esp_log_write() -> ecriture
//   console/UART -> lock_init_generic() (1ere init du verrou recursif stdio)
//   -> echec -> abort()). Meme plateau heap bas que le reste de la session
//   (free=5196-5228 juste avant). Aucun try/catch possible (code C d'ESP-IDF,
//   pas d'exceptions). Fix : esp_log_level_set("wifi", ESP_LOG_NONE) tout au
//   debut de setup(), avant toute init WiFi -- si wifi_log() ne tente jamais
//   d'ecrire, ce chemin de code entier n'est plus emprunte. Pratique standard
//   en production ESP32, aucun risque fonctionnel (n'affecte que les logs
//   internes du driver, pas nos Serial.println()). PAS ENCORE VALIDE sur
//   materiel au moment d'ecrire cette entree.
//
// v99 - 2026-08-17 - safe-modify - Symptome recurrent observe sur materiel
//   MEME apres le fix v97 (slots dedies default/system/game/ingame) :
//   "RB connectee" + logo generique repete en boucle (confirme sur le log --
//   PNG-RAW redessine /systems/_defaults/default.raw565 toutes les ~17s) au
//   lieu du marquee du jeu reel, ET un overlay game_info vide/"0 car." pour
//   un payload dont l'utilisateur a confirme (log brut) qu'un champ INFOS
//   substantiel etait bien publie -- rentre dans l'ordre plus tard, "avec la
//   reconnexion suivante". Diagnostic de parsing ajoute (verification :
//   relecture manuelle du filtre DESCRIPTION ne revele aucun bug logique
//   evident) + hypothese plus probable retenue : game_info n'etait PAS
//   protege par le fix v97 (seuls default/system/game/ingame l'etaient),
//   donc toujours susceptible d'etre perdu par ecrasement de pendingCmd dans
//   la meme rafale post-reconnexion -- coherent avec "arrive avec la
//   reconnexion suivante" (une rafale ulterieure, moins chargee, a fini par
//   laisser passer un message game_info non ecrase). Fix : game_info recoit
//   desormais aussi son slot dedie (g_pendingGameInfo+Arg), meme mecanisme
//   que les 4 precedents. CMD_GAME_DEBUG_LOGS reactive en parallele pour
//   confirmer directement (au prochain cycle) si CMD_GAME lui-meme est bien
//   recu/traite pour un jeu qui reste bloque sur le fallback. PAS ENCORE
//   VALIDE sur materiel au moment d'ecrire cette entree.
//
// v98 - 2026-08-17 - safe-modify - 6e CRASH REEL confirme sur materiel (hash
//   ELF verifie + addr2line) en testant v97 : abort() MEME cause que les
//   crashes deja corriges en v91/v95 (double-echec d'allocation dans
//   getNextGif()->getNextGifRandom()->SD.open(), non rattrapable), mais un
//   3e site d'appel DIFFERENT, jamais repere avant -- dans la boucle de frame
//   MODE_GIF de loop() elle-meme (prefetch du GIF suivant PENDANT la lecture
//   du GIF courant, ligne ~7804), distinct des 2 deja proteges dans
//   openNextGif(). Fix : meme seuil dedie PREFETCH_NEXT_GIF_MIN_HEAP=8000
//   (v95), remonte en portee fichier pour etre partage entre les 2 sites.
//   Verification faite : plus aucun appel non protege a getNextGif() dans
//   tout le fichier (3 sites au total, tous couverts).
//   VALIDATION PARTIELLE en cours de test sur materiel : ce crash n'est plus
//   reproduit, mais a permis d'observer un autre defaut confirme du fix v94
//   (deconnexion forcee sur trop d'echecs subscribe()) : le check du seuil
//   ne s'executait qu'APRES les 15 tentatives sequentielles -- observe 8
//   echecs/15 avant declenchement, soit ~2min40 avant que la deconnexion
//   forcee agisse, perdant une bonne partie du gain de reactivite vise.
//   Fix additionnel (meme version) : boucle avec sortie anticipee des que le
//   seuil (3) est atteint, au lieu d'attendre la fin des 15.
//   7e CRASH REEL confirme sur materiel pendant ce meme test (hash ELF
//   verifie + addr2line) : 4e site distinct de la MEME famille (double-echec
//   d'allocation SD.open()->make_shared<VFSFileImpl>), cette fois DANS
//   openGif() lui-meme (aucun garde-fou avant), appele par CMD_GAME pour
//   charger le VRAI marquee du jeu (observe juste apres un reveil RB). Un
//   seuil heap (comme PREFETCH_NEXT_GIF_MIN_HEAP) a ete envisage PUIS
//   ECARTE : verification sur le log reel, maxalloc=4596 (plateau NORMAL)
//   juste avant CE crash, IDENTIQUE a des dizaines d'ouvertures REUSSIES la
//   meme session -- un seuil a 8000 aurait bloque le chargement du marquee
//   la plupart du temps (regression majeure), maxalloc n'etant pas un
//   predicteur fiable pour cette allocation precise. Fix retenu : openGif()
//   renomme en openGifImpl() (logique inchangee), nouveau wrapper openGif()
//   (meme signature/defauts) l'appelle dans un try/catch (meme technique
//   deja etablie pour getNextGifRandom(), v78) -- n'empeche pas le cas du
//   double-echec total (rare, non rattrapable par le C++ runtime), mais
//   capture le cas plus frequent d'un echec isole, sans aucun risque de
//   regression fonctionnelle. PAS ENCORE VALIDE sur materiel au moment
//   d'ecrire cette entree.
//
// v97 - 2026-08-17 - safe-modify - BUG REEL confirme sur materiel (diagnostic
//   via CMD_GAME_DEBUG_LOGS, cf. v96) : pendingCmd (slot unique partage par
//   TOUTES les commandes MQTT) perdait silencieusement "game" quand un AUTRE
//   type de commande (ex. "game_info", ~0.2s plus tard) l'ecrasait avant que
//   loop() ait eu l'occasion de le consommer -- confirme : aucun log [DIAG]
//   (pourtant inconditionnel avec CMD_GAME_DEBUG_LOGS actif) pour le message
//   "game" perdu, alors que game_info/ingame arrives juste apres etaient
//   bien traites. Explique le symptome observe (question utilisateur) : le
//   marquee du jeu reste bloque sur l'ecran "RB connectee" meme apres une
//   reconnexion MQTT reussie, alors que l'overlay (game_info) affiche du
//   contenu a jour par-dessus -- typiquement juste apres une reconnexion,
//   quand plusieurs messages retenus (default+system+game+game_info+ingame)
//   arrivent en rafale en moins d'une seconde.
//   Une queue/tableau generique a ete deliberement ECARTEE (demande explicite
//   utilisateur suite a une mise en garde retrouvee dans la memoire du
//   chantier source, project_marquee_heartbeat_rederivation_bug) : un
//   tableau de flags "commande en attente" indexe par type y avait ete
//   fortement suspecte de corruption memoire (ecriture hors-limites),
//   causant des commandes PERIMEES a se declencher en pleine partie active --
//   sur le MEME plateau heap bas que celui de ce chantier, jamais elucide.
//   Fix retenu a la place : les 4 commandes qui definissent "ce qui doit
//   etre affiche" (default/system/game/ingame) recoivent chacune sa PROPRE
//   variable dediee (g_pendingDefault/g_pendingSystem+Arg/g_pendingGame+Arg/
//   g_pendingIngame+Arg -- 4 paires nommees individuellement, PAS un tableau
//   indexe par type, meme pattern deja utilise sans probleme connu ailleurs
//   dans ce fichier pour g_noWifiRecalboxPending/g_achievementPendingShow) --
//   elles ne peuvent donc plus jamais etre ecrasees par un type de commande
//   different. processPendingMqttCommand() les consomme en priorite (1 par
//   appel, ordre default->system->game->ingame), le reste (score/game_info/
//   achievement/brightness/etc.) continue sur pendingCmd inchange (perte
//   occasionnelle deja acceptee, aucun bug confirme les concernant). Tous
//   les sites d'ecriture identifies et migres (dispatch principal MQTT,
//   fallback "injoignable", fallback evenement ES sans systeme connu).
//   CMD_GAME_DEBUG_LOGS repasse a false (diagnostic termine). PAS ENCORE
//   VALIDE sur materiel au moment d'ecrire cette entree.
//
// v96 - 2026-08-17 - safe-modify - Test empirique demande utilisateur :
//   champ "DESCRIPTION" du game_info (texte le plus long, cout heap/scroll
//   le plus eleve) desactive par filtrage de label dans
//   startGameInfoOverlay(), "INFOS" reste actif ainsi que le hi-score
//   (startScoreOverlay(), non affecte). Objectif : verifier si retirer le
//   champ le plus couteux en heap reduit la frequence des crashes/
//   instabilites observes en jeu. Aucun changement cote script RB. A
//   retirer (ou rendre configurable) une fois le test conclu. Titre splash
//   screen (RETRO_VERSION) egalement change de "v12" a "devCORE0" (demande
//   utilisateur, purement cosmetique).
//
// v95 - 2026-08-17 - safe-modify - 5e CRASH REEL confirme sur materiel (hash
//   ELF verifie + addr2line) en testant v94 : abort() a EXACTEMENT le meme
//   site que le crash deja "corrige" en v91 (prefetch nextGifPath=getNextGif()
//   dans openNextGif()) -- le garde-fou v91 (ESP.getMaxAllocHeap() >=
//   OPEN_NEXT_GIF_MIN_HEAP=4000) etait INSUFFISANT : au moment du crash,
//   maxalloc=4596, deja au-dessus du seuil, donc le garde a laisse passer.
//   4596 s'avere etre le plateau heap NORMAL de ce materiel (deja documente
//   ailleurs dans ce projet), pas une valeur anormale -- le seuil de 4000
//   (calibre pour un AUTRE point d'appel, le garde d'entree de openNextGif())
//   ne protegeait quasiment jamais en pratique pour cette allocation precise.
//   Fix : nouveau seuil DEDIE PREFETCH_NEXT_GIF_MIN_HEAP=8000, uniquement pour
//   ce prefetch (risque limite d'un seuil haut ici, contrairement au garde
//   d'entree : le pire cas est un prefetch simplement saute, le GIF en cours
//   reste affiche normalement). PAS ENCORE VALIDE sur materiel au moment
//   d'ecrire cette entree.
//
// v94 - 2026-08-17 - safe-modify - BUG REEL confirme sur materiel en testant
//   v93 : les 15 subscribe() de mqttTask() ont TOUS echoue (meme apres
//   retry v89), ~20s d'ecart chacun (2x le socket timeout 10s) -- TCP
//   accepte + CONNACK recu mais plus AUCUNE reponse ensuite (connexion
//   "zombie", mqttClient.connected() reste true mais sourde). Le fix v89
//   (log+retry par topic) suffit pour un echec isole mais n'avait aucun
//   recours ici : le code postait quand meme CMD_WAITING_MQTT, ecran
//   "RecalBox connectee" fige (observe en direct : "affichage fallback + rb
//   connectee en fixe"), sans plus rien recevoir. Observe SANS ce fix (v93
//   pur) : le keepalive interne de PubSubClient finit par detecter tout seul
//   la connexion morte et redeclenche une reconnexion -- mais seulement
//   apres ~5min30 de service coupe. Fix : compte les echecs de subscribe()
//   apres retry ; si >= 3 (seuil arbitraire, large marge au-dessus d'un
//   echec isole tolerable), la connexion est consideree morte --
//   deconnexion FORCEE (mqttClient.disconnect()) immediate, pas de publish
//   ip ni de CMD_WAITING_MQTT sur ce socket mort -- le prochain tour de
//   mqttTask() retente une vraie reconnexion (nouveau socket), au lieu
//   d'attendre le keepalive. PAS ENCORE VALIDE sur materiel au moment
//   d'ecrire cette entree.
//
// v93 - 2026-08-17 - safe-modify - 1 instrumentation + 1 BUG REEL confirme
//   sur materiel (tous deux en testant v92 en conditions reelles, partie sur
//   Alien Syndrome + reconnexions MQTT) :
//   INSTRUMENTATION : ajout du heap (free + maxalloc) sur chaque log
//   "[MQTT] connecting"/"failed rc=". Objectif : verifier avec des mesures
//   reelles l'hypothese utilisateur (observee empiriquement PLUSIEURS fois
//   aujourd'hui, independamment) selon laquelle la reconnexion MQTT
//   n'aboutit qu'apres un retour hors-jeu/playlist -- possible contention
//   sur le heap PARTAGE entre loop() (overlay/raw565pack en jeu, heap
//   maintenu bas ~4596 en continu observe ce jour) et mqttTask() (connect()
//   a peut-etre besoin d'un certain heap libre pour ses buffers TCP/MQTT).
//   A retirer une fois l'hypothese confirmee/infirmee.
//   BUG : observe en jeu (Alien Syndrome, PAS un overlay perime -- correction
//   d'un diagnostic errone de ma part en cours de session, l'utilisateur a
//   confirme etre reellement en partie) : ecran "RecalBox connectee" bloque
//   en boucle avec l'overlay game_info (courant, pas perime) par-dessus,
//   log "[PNG-RAW] MISSING raw565 path=/systems/_defaults/default.raw565
//   raw565=/systems/_defaults/default.raw565.raw565" -- pngToRaw565Path()
//   rajoutait ".raw565" a un chemin qui en avait DEJA un (le placeholder pose
//   par CMD_WAITING_MQTT), chemin double-extension forcement introuvable,
//   ~800ms perdus en chaine de repli avant rattrapage. Cause probable
//   (mecanisme precis non confirme a 100% -- connexion MQTT restee stable en
//   continu 7mn avant l'incident, donc PAS un reconnect en cours a ce
//   moment-la) : le repli "fast path totalement echoue" ajoute en v92 pour
//   CMD_GAME ne nettoyait pas currentPngPath, laissant ce placeholder perime
//   trainer pour un redessin ulterieur (endOverlay()) errone. Fix (2
//   parties) : (a) pngToRaw565Path() rendue idempotente (ne rajoute jamais
//   ".raw565" si deja present) -- ferme le symptome quelle que soit la cause
//   exacte ; (b) le repli v92 nettoie desormais aussi currentPngPath="".
//   ATTENTION : une premiere tentative de fix (g_inGameMarquee=false dans
//   CMD_WAITING_MQTT) a ete ECRITE PUIS REVERTEE dans cette meme session --
//   diagnostic initial errone (avait suppose l'overlay perime a tort),
//   l'utilisateur a corrige avant flash. Ne pas la reintroduire sans nouvelle
//   preuve.
//
// v92 - 2026-08-17 - safe-modify - 2 BUGS REELS lies, confirmes sur materiel
//   en testant v91 en conditions reelles (partie + reconnexion MQTT sous
//   stress) :
//   (1) CMD_GAME (systeme "rapide", isSlow=false, ex. fbneo) n'avait AUCUN
//   filet de secours si le jeu, son .gif ET default.png/_defaults echouaient
//   TOUS a s'ouvrir (observe juste apres une reconnexion MQTT stressee,
//   probable pression heap transitoire) -- le code continuait silencieusement
//   dans le bloc if(isSlow) juste en dessous, qui ne s'execute JAMAIS pour un
//   systeme rapide, laissant currentMode bloque a sa valeur PRECEDENTE
//   (observe : reste en MODE_PLAYLIST issu d'un resumePlaylist() anterieur,
//   le mecanisme "injoignable" qui resout les rc=-4 repetes) -- etat
//   incoherent qui bloquait aussi silencieusement le declencheur d'overlay
//   (exige MODE_PNG/MODE_GIF, jamais MODE_PLAYLIST, meme raisonnement que le
//   bug v85). Fix : etat MODE_BLACK explicite si le fast path echoue
//   entierement, log toujours visible (pas gate CMD_GAME_DEBUG_LOGS).
//   (2) case MODE_BLACK de loop() (observe separement : ecran noir fige
//   ~2min30 apres un simple echec d'ouverture GIF, nom de fichier tronque)
//   ne retentait JAMAIS rien -- contrairement au blocage heap
//   (requestNextGif=true, deja existant), un echec d'ouverture y laissait
//   l'ecran noir INDEFINIMENT jusqu'a un evenement MQTT externe fortuit. Fix :
//   retente periodique (meme requestNextGif, rate-limite 3s) -- couvre aussi
//   le nouveau MODE_BLACK du point (1) ci-dessus. PAS ENCORE VALIDE sur
//   materiel au moment d'ecrire cette entree.
//
// v91 - 2026-08-17 - safe-modify - 4e CRASH REEL confirme sur materiel (hash
//   ELF verifie + addr2line), decouvert en testant v90 : abort() ~2s apres
//   "[MQTT] connected" au boot, PAS lie au fix v90 (ingame/default differe --
//   ce scenario n'avait meme pas encore eu l'occasion de se produire).
//   Backtrace : loop() -> openNextGif() -> getNextGif() -> getNextGifRandom()
//   -> fs::FS::open() -> VFSImpl::open() -> operator new (shared_ptr
//   VFSFileImpl) -> __cxa_allocate_exception -> std::terminate() -> abort().
//   Cause : le prefetch "GIF suivant" (nextGifPath=getNextGif(), ligne ~3840,
//   execute a CHAQUE ouverture de GIF reussie) n'etait garde par AUCUN
//   controle heap propre -- seul le controle d'ENTREE de openNextGif()
//   (OPEN_NEXT_GIF_MIN_HEAP, v78) le precedait, mais openGif() qui s'execute
//   juste avant ce prefetch consomme lui-meme du heap, invalidant le controle
//   d'entree. Sous heap suffisamment bas, operator new echoue POUR la
//   construction de l'objet bad_alloc lui-meme (pas seulement pour
//   l'allocation demandee) -- double-echec qui force std::terminate() de
//   facon INCONDITIONNELLE, non rattrapable par le try/catch deja present
//   dans getNextGifRandom() (v78, qui ne protege que le cas d'un simple
//   echec d'allocation, pas ce double-echec). Fix : re-verification du seuil
//   heap juste avant ce 2e appel a getNextGif() ; si trop bas, prefetch
//   simplement saute (retente au prochain appel via le fallback existant).
//   PAS ENCORE VALIDE sur materiel au moment d'ecrire cette entree.
//
// v90 - 2026-08-17 - safe-modify - BUG REEL confirme sur materiel (observe
//   live juste apres validation de v89) : apres une reconnexion MQTT reussie
//   pendant une VRAIE partie (ingame=1, ex. relance de 1941), le DMD repart
//   en playlist ~6s plus tard ("[MQTT] default differe applique -> reprise
//   playlist") alors que le joueur est toujours en jeu. Cause : le mecanisme
//   "default differe" (v49, ecran de boot uniquement a l'origine) se re-arme
//   en realite a CHAQUE reconnexion MQTT (CMD_WAITING_MQTT emis a chaque
//   connect() reussi, pas seulement au 1er boot), et un "default" RETENU
//   (rejoue par le broker a la resouscription, cf. v89) peut arriver dans
//   cette fenetre de 5s en meme temps qu'une vraie partie demarre. CMD_GAME/
//   CMD_SYSTEM annulaient deja ce differe depuis v49 ("un vrai jeu prend le
//   pas sur un default differe"), mais CMD_INGAME (introduit en v79, apres ce
//   mecanisme) avait ete oublie -- oubli de synchronisation entre les 2
//   refactors. Fix : CMD_INGAME("1") annule desormais aussi le differe, meme
//   raisonnement, seulement sur la transition vers une vraie partie. PAS
//   ENCORE VALIDE sur materiel au moment d'ecrire cette entree.
//
// v89 - 2026-08-17 - safe-modify - BUG REEL confirme sur materiel : le DMD
//   restait affiche "[MQTT] connected" (connexion TCP saine, confirme cote
//   broker via netstat -- une seule connexion ESTABLISHED, pas de zombie)
//   mais ne recevait plus AUCUN message publie par la RB (confirme cote RB
//   : marquee.sh publiait bien, marquee_mqtt.log actif) -- observe apres
//   une reconnexion elle-meme precedee de plusieurs echecs (rc=-2/rc=-4).
//   Cause probable : 15 mqttClient.subscribe() consecutifs, SANS AUCUNE
//   verification du resultat ni delai entre eux, juste apres connect() --
//   si le reseau etait encore instable a cet instant precis, un ou
//   plusieurs subscribe() ont pu echouer silencieusement (retour bool
//   jamais teste). Fix : chaque subscribe() est desormais verifie,
//   reessaie une fois apres un court delai en cas d'echec, logue tout
//   echec definitif. VALIDE sur materiel apres flash+reboot : sequence
//   observee au serial juste apres "[MQTT] connected" -- marquee/cmd/system,
//   marquee/cmd/game, marquee/cmd/ingame recus normalement, aucune ligne
//   "subscribe ECHEC", playlist/GIFs enchaines normalement ensuite (heap
//   stable ~8.5k libre, maxalloc constant 4596). Reste a confirmer sur une
//   duree plus longue et lors d'une future reconnexion apres coupure reseau
//   reelle (le cas qui avait initialement revele le bug).
//
// v88 - 2026-08-17 - safe-modify - 3e CRASH REEL confirme sur materiel
//   (hash ELF verifie, decode via addr2line), MEME site que le tout 1er
//   crash de cette session (v80) : mqttTask() bloque dans
//   PubSubClient::connect() (PubSubClient.cpp:257), malgre le yield() deja
//   ajoute a cet endroit -- confirme cette fois pendant une PARTIE en
//   cours ([LOOPDIAG] montre loop() parfaitement sain, mode=MODE_OVERLAY,
//   jusqu'a 300ms avant le crash -- donc PAS loop() en cause, uniquement
//   mqttTask()). Cause du echec du 1er correctif : yield()/taskYIELD() sur
//   ESP32/FreeRTOS ne cede la main qu'aux taches de priorite EGALE OU
//   SUPERIEURE -- PAS a IDLE0 (priorite la plus basse, celle surveillee
//   par le watchdog materiel), donc pas une garantie fiable (explique
//   pourquoi ca a tenu plusieurs minutes de test avant de retomber). Fix :
//   yield() remplace par delay(1) (patch local PubSubClient.cpp, boucle de
//   connect() ET readByte() par prudence meme si non encore observee sur
//   ce 2e site) -- delay(1) force un vrai blocage d'au moins 1 tick,
//   laissant le planificateur choisir N'IMPORTE QUELLE tache prete, IDLE
//   inclue. PAS ENCORE VALIDE sur materiel au moment d'ecrire cette
//   entree.
//
// v87 - 2026-08-17 - safe-modify - INSTRUMENTATION DIAGNOSTIC TEMPORAIRE :
//   episode reel observe sur materiel -- rafale de std::bad_alloc
//   rattrapees (chunk 1, pre-chargement du GIF suivant en rotation
//   playlist normale, heap coince a maxalloc=4596) suivie de 65s de
//   silence total avant qu'une commande MQTT deja en attente ne soit
//   enfin traitee, alors que processPendingMqttCommand() est censee
//   tourner sans condition a chaque iteration de loop(). Hypothese a
//   confirmer : le cout du throw/catch C++ (chunk 1) sous heap critique
//   pourrait etre bien plus eleve que prevu sur ESP32. Heartbeat
//   [LOOPDIAG] inconditionnel (2s) ajoute en tout debut de loop() pour
//   localiser precisement ou loop() se trouve si le silence se
//   reproduit. PAS ENCORE VALIDE au moment d'ecrire cette entree.
//
// v86 - 2026-08-17 - safe-modify - PORT de l'UI web pour scoreEnabled/
//   gameInfoEnabled depuis dev/mame-score-mqtt-bridge (page /config/basic,
//   confirmee fonctionnelle par l'utilisateur sur cette branche source) :
//   2 cases a cocher ("Hi-score en jeu" / "Infos jeu en jeu") dans la
//   section Affichage, juste sous "Demarrage silencieux". Chaine complete
//   portee : HTML (web_config.h), i18n FR/EN/ES (PAGE_I18N), JS
//   (serialize()/DRAFT_FIELDS/chargement depuis /load), backend
//   (handleWebConfigSave() lit les args, JSON de statut les expose,
//   reecriture complete de config.ini les persiste avec
//   SCORE_INTERVAL_SEC/SCORE_DURATION/GAME_INFO_EVERY_N -- meme piege que
//   "language=" deja documente : une reecriture complete doit re-emettre
//   TOUTE cle existante). scoreEnabled/gameInfoEnabled repasses a false
//   par defaut (le forcage temporaire a true de v79 n'est plus necessaire,
//   l'activation se fait desormais depuis cette page).
//
// v85 - 2026-08-17 - safe-modify - CAUSE TROUVEE et corrigee pour le bug
//   "overlay hi-score/game_info ne se declenche jamais" (v84) -- PAS une
//   corruption memoire, un vrai bug de logique confirme par
//   l'instrumentation [TRIGDIAG] : currentMode reste bloque a
//   MODE_PLAYLIST alors que g_inGameMarquee reste vrai (etat incoherent,
//   confirme aussi par observation ecran directe -- "le DMD est repasse en
//   playlist en mode jeu"). Cause : resumePlaylist() est appelee depuis
//   plusieurs sites qui NE PASSENT PAS par le case CMD_DEFAULT de
//   processPendingMqttCommand() (ou j'avais ajoute le reset de
//   g_inGameMarquee en v79) -- notamment le mecanisme "default differe"
//   (v49, applique un CMD_DEFAULT recu tot pendant l'ecran de connexion,
//   mais potentiellement bien APRES qu'une vraie partie ait demarre entre-
//   temps) ainsi que l'auto-resolution des alertes WiFi/RecalBox et
//   "Reprendre DMD" (web). Le declencheur d'alternance exige
//   MODE_PNG/MODE_GIF (jamais MODE_PLAYLIST) -- une fois dans cet etat
//   incoherent, plus aucun overlay ne pouvait se declencher, silencieusement
//   et indefiniment, jusqu'au prochain vrai CMD_STOP/CMD_SYSTEM/CMD_INGAME.
//   Fix : g_inGameMarquee=false ajoute a chaque site individuel connu, ET
//   centralise DIRECTEMENT dans resumePlaylist() (qui signifie TOUJOURS
//   "on quitte pour la playlist hors-jeu") en dernier recours -- protege
//   aussi tout site futur non repere. PAS ENCORE VALIDE sur materiel au
//   moment d'ecrire cette entree.
//
// v84 - 2026-08-17 - safe-modify - INSTRUMENTATION DIAGNOSTIC TEMPORAIRE :
//   episode reel observe sur materiel (v83) ou l'overlay hi-score/game_info
//   ne s'est JAMAIS declenche pendant une partie de plusieurs minutes,
//   alors que g_inGameMarquee etait bien vrai (ingame=1 confirme). PISTE
//   INITIALE ECARTEE par retour utilisateur direct (2 preuves) : (1)
//   l'animation du logo/marquee jeu restait fonctionnelle tout du long
//   (loop() n'est donc PAS bloque -- un vrai gel figerait aussi le rendu) ;
//   (2) la carte SD elle-meme est confirmee saine (le carrousel playlist,
//   qui ouvre un nouveau GIF a chaque fois, fonctionne normalement des la
//   sortie de partie). Donc PAS un blocage bas niveau (SD/LWIP) -- un bug
//   de LOGIQUE dans la condition de declenchement de l'overlay elle-meme
//   (ou son "point de coupure propre" dans case MODE_GIF/MODE_PNG), qui a
//   simplement empeche g_overlayPendingStart de se poser ou de se
//   consommer, sans rien logger (le rendu GIF normal ne log rien par
//   frame, d'ou l'illusion d'un silence total). Logs [TRIGDIAG] ajoutes
//   (etat complet du bloc declencheur toutes les 5s pendant une partie
//   sans overlay actif, + suivi si pendingStart reste bloque en attente) --
//   a retirer une fois la cause confirmee.
//
// v83 - 2026-08-17 - safe-modify - CORRECTIF PLUS PROFOND de l'instabilite
//   MQTT residuelle (voir v82 -- l'hypothese "contention CPU overlay" de
//   v82 est INVALIDEE par retour utilisateur : le meme probleme existait
//   deja sur dev/mame-score-mqtt-bridge, SANS le changement de cœur, et
//   apparait meme en simple navigation de liste, sans overlay actif).
//   Investigation reseau reelle (netstat cote RB pendant un episode) :
//   socket zombie en etat CLOSING cote broker en plus de la connexion
//   active -- signature d'un ID client MQTT FIXE (MQTT_CLIENT=
//   "esp32-marquee") : quand le DMD retente connect() (a tort ou a raison),
//   le broker force la fermeture de l'ancienne connexion (norme MQTT,
//   meme ID client = kick de l'ancienne session), meme si elle etait encore
//   valide. Cause racine du "a tort" : PubSubClient.cpp:257 (boucle
//   d'attente du CONNACK) n'avait aucun yield() -- fixe DIRECTEMENT dans
//   PubSubClient.cpp (patch local, voir son commentaire) plutot que de
//   continuer a compenser cote firmware avec un timeout court. Ce timeout
//   court (v80, 3s) etait lui-meme devenu trop impatient : setSocketTimeout()
//   couvre TOUTES les lectures socket, pas seulement connect() -- une
//   lenteur passagere du broker (RB occupee a emuler un jeu, ex. observe
//   reellement pendant cette session) pendant mqttClient.loop() normal
//   pouvait aussi declencher une reconnexion prematuree, elle-meme
//   provoquant le kick d'ID client ci-dessus -- cascade auto-entretenue.
//   Remonte a 10s maintenant que le vrai risque watchdog est corrige a la
//   racine (voir setSocketTimeout(), section MQTT setup, pour le detail
//   complet). PAS ENCORE VALIDE sur materiel au moment d'ecrire cette
//   entree.
//
// v82 - 2026-08-17 - safe-modify - EXPERIENCE EN COURS (pas encore
//   confirmee) suite a v80/v81 : apres les 2 crashs corriges, une
//   instabilite MQTT residuelle (reconnexions rc=-4/rc=-2 en rafale,
//   retour playlist force par MQTT_OFFLINE_FALLBACK_MS) reste observee sur
//   materiel reel, correlee dans le temps avec un overlay hi-score/
//   game_info actif. Hypothese (pas encore prouvee) : sur ce worktree,
//   loop() et mqttTask() partagent desormais le MEME coeur (0, voir le
//   chantier de reassignation), contrairement a la branche source ou
//   OVERLAY_FRAME_INTERVAL_MS=45ms avait ete calibre pour l'AUTRE coeur
//   (EventsCore/WiFi) -- ce calibrage n'a plus le meme sens ici. Test A/B :
//   OVERLAY_FRAME_INTERVAL_MS 45ms->200ms (voir advanceOverlay()) pour
//   ceder nettement plus de temps CPU a mqttTask pendant un scroll actif.
//   PAS ENCORE VALIDE sur materiel au moment d'ecrire cette entree --
//   si confirme insuffisant, ne pas re-tenter la meme piste en boucle,
//   envisager plutot l'idee alternative deja discutee (utilisateur,
//   2026-08-17) : deporter le rythme de l'animation cote script Recalbox
//   (tick MQTT a frequence reduite) plutot que la garder cote firmware --
//   ecartee pour ce premier essai (ajoute du trafic MQTT, complexite de
//   coordination start/stop, alors que MQTT est justement la ressource
//   fragile) mais reste une option si le simple ralentissement local ne
//   suffit pas.
//
// v81 - 2026-08-17 - safe-modify - 2e CRASH REEL confirme sur materiel
//   (decode via addr2line, hash ELF verifie identique au binaire plante),
//   DIFFERENT du v80 (celui-la un abort() PANIC, pas un watchdog) --
//   preloadBigram()->loadBigramTable()->SD.open()->make_shared<VFSFileImpl>
//   ->operator new echoue a maxalloc=4596 (meme plateau heap deja documente
//   partout ailleurs dans ce fichier). Ce site (9 appelants, notamment
//   depuis le chemin CMD_GAME) n'avait AUCUN garde-fou, contrairement a
//   getNextGifRandom()/drawRaw565() -- trou preexistant dans master, pas
//   introduit par le chunk 3, mais le hi-score/game_info consomme plus de
//   heap pendant CMD_GAME, rendant ce plateau plus facile a atteindre. Fix :
//   meme double protection (seuil CMD_GAME_MIN_HEAP_FOR_FILE_OPEN avant
//   tentative + try/catch en filet de securite) que les guards existants,
//   centralisee dans loadBigramTable() (protege ses 9 appelants d'un coup).
//   CMD_GAME_MIN_HEAP_FOR_FILE_OPEN deplacee plus haut dans le fichier
//   (avant loadBigramTable(), qui en a besoin plus tot que drawRaw565()) --
//   simple relocalisation de declaration, aucun changement de valeur/
//   comportement pour drawRaw565() et ses autres appelants existants.
//
// v80 - 2026-08-17 - safe-modify - CRASH REEL confirme sur materiel
//   pendant le test du chunk 3 (v79, hi-score/game_info/achievement) --
//   reset TASK_WDT, decode via addr2line (hash ELF verifie identique au
//   binaire plante) : mqttTask() bloque dans PubSubClient::connect()
//   (boucle sans yield en attendant le CONNACK), setSocketTimeout(30)
//   laissait cette boucle affamer IDLE0 (cœur 0) assez longtemps pour
//   declencher le watchdog materiel. PAS un bug introduit par ce chantier
//   (mqttTask() etait deja seul sur le cœur 0 avant ET apres le changement
//   LoopCore -- comportement deja documente comme tel dans PubSubClient),
//   juste jamais declenche en test avant une vraie reconnexion qui traine.
//   Fix : timeout abaisse a 3s (voir setSocketTimeout(), section MQTT
//   setup) -- large marge pour un handshake local sain, borne desormais le
//   pire cas nettement sous le watchdog.
//
// v79 - 2026-08-17 - safe-modify - PORT depuis dev/mame-score-mqtt-bridge
//   (chunk 3/N -- hi-score FBNeo + game info + RetroAchievements + le
//   sous-systeme d'overlay de rendu non-bloquant qui les porte tous les
//   trois -- regroupes car NON SEPARABLES proprement dans la branche
//   source : g_overlayCycleIndex/g_overlayType/advanceOverlay() sont
//   partages entre les 3 fonctionnalites). Deja teste et fonctionnel sur
//   materiel reel dans la branche source -- portage fidele, pas de
//   re-conception. Nouvelles commandes MqttCommand::CMD_SCORE (marquee/cmd/
//   score), CMD_GAME_INFO (marquee/cmd/game_info), CMD_ACHIEVEMENT
//   (marquee/cmd/achievement) -- memorisent juste le payload recu,
//   l'affichage se fait en alternance avec le marquee du jeu EN COURS
//   (jamais avec la playlist hors-jeu), voir startScoreOverlay()/
//   startGameInfoOverlay()/startAchievementOverlay()/advanceOverlay() et
//   leur section dediee juste avant setup(). Nouvelles cles config.ini :
//   SCORE_ENABLED/SCORE_INTERVAL_SEC/SCORE_DURATION/GAME_INFO_ENABLED/
//   GAME_INFO_EVERY_N (opt-in, pas d'onglet web dedie a ce stade, meme
//   choix que la branche source).
//
//   DIVERGENCE DELIBEREE vs la branche source (retour utilisateur explicite
//   pendant cette session) : g_inGameMarquee N'EST PLUS pose par CMD_GAME
//   directement -- verifie dans marquee.sh (branche source ET master) que
//   marquee/cmd/game est publie A LA FOIS par un vrai lancement de partie
//   (rungame) ET par un simple survol de la liste des jeux
//   (gamelistbrowsing, meme topic, meme format "system/rom") -- le firmware
//   ne peut pas distinguer les 2 a partir de ce seul topic. Poser
//   g_inGameMarquee=true sur CHAQUE CMD_GAME (comme le fait la branche
//   source) aurait donc pu activer l'alternance hi-score/game_info (et son
//   cout heap/CPU associe -- allocations String dans start*Overlay(),
//   scroll 22fps) pendant un simple defilement RAPIDE de liste (plusieurs
//   CMD_GAME/seconde observes en test reel), exactement la zone deja la
//   plus marginale en heap de ce projet. Fix : nouveau topic dedie
//   marquee/cmd/ingame ("1"=rungame reel, "0"=sortie de jeu), publie par
//   marquee.sh UNIQUEMENT sur rungame/endgame (jamais sur
//   gamelistbrowsing) -- voir MqttCommand::CMD_INGAME. CMD_GAME continue de
//   reinitialiser le rythme d'alternance (dwell/cycle) a chaque nouvelle
//   entree (inoffensif hors-jeu, gate par g_inGameMarquee ailleurs), mais
//   ne touche plus g_inGameMarquee lui-meme.
//
//   Rappel utilisateur (contexte du chantier de reassignation de coeur en
//   cours) : la branche source fonctionnait globalement bien mais a connu
//   des corruptions memoire non elucidees (voir memoire
//   project_core_reassignment_rb_script_mismatch et
//   project_marquee_heartbeat_rederivation_bug, namespace
//   dev-mame-score-mqtt-bridge) EN PLUS du probleme de collision WiFi/coeur
//   que ce chantier de reassignation vise a corriger -- 2 problemes
//   distincts, ne pas les confondre si un symptome de corruption
//   reapparait apres ce portage (ne pas repartir sur des correctifs
//   MQTT/tristate, deja explores a fond cote branche source).
//
//   NON PORTE a ce stade (chunks suivants) : garde-fou heartbeat MQTT
//   anti-perte-de-message (marquee.sh v3.2, dmd_score.sh v9), dedup
//   CMD_GAME/CMD_SYSTEM (v109 source), blocage web-config pendant une
//   partie (v101/v108 source). v110 source (garde CMD_STARTCLIP/
//   CMD_RESUMESYS) explicitement EXCLU -- cause probable = corruption
//   memoire non elucidee, pas un vrai fix.
//
// v78 - 2026-08-17 - safe-modify - PORT depuis dev/mame-score-mqtt-bridge
//   (chantier "reassignation de coeur ESP32" -- port fonction par fonction
//   apres validation materielle du changement LoopCore=1->0, PAS une copie
//   de fichier en bloc). Chunk 1/N : garde-fous heap uniquement, deja bien
//   testes sur materiel reel dans la branche source (equivalent la-bas de
//   v99+v104+v105+v106 cumules) -- getNextGifRandom() protege par try/catch
//   (filet de securite EXCEPTION sur crash confirme addr2line, meme
//   technique que handleWebConfig()) + openNextGif() protege par un seuil
//   heuristique ESP.getMaxAllocHeap()<OPEN_NEXT_GIF_MIN_HEAP (4000, calibrage
//   deja arbitre sur materiel reel dans la branche source -- ne pas remonter
//   a 8000, bloquait le plateau heap stable normal en permanence). AUCUNE
//   des fonctionnalites hi-score/game_info/RA/heartbeat MQTT de la branche
//   source n'est incluse a ce stade (chunks suivants, testes separement sur
//   materiel avant d'enchainer -- voir memoire
//   project_core_reassignment_rb_script_mismatch). v110 de la branche source
//   (garde CMD_STARTCLIP/CMD_RESUMESYS) explicitement EXCLU du portage --
//   cause probable = corruption memoire non elucidee, pas un vrai fix.
//
// v77 - 2026-08-13 - safe-modify - Commandes MQTT relatives de luminosite
//   +10%/-10% (marquee/cmd/brightness_up, marquee/cmd/brightness_down,
//   payload vide), demande explicite utilisateur pour piloter la
//   luminosite depuis un script Recalbox (menu START > Parametres avances
//   > Scripts utilisateur) sans devoir connaitre/publier un pourcentage
//   absolu comme le fait CMD_BRIGHTNESS (v70/v71). Nouvelles commandes
//   MqttCommand::CMD_BRIGHTNESS_UP/CMD_BRIGHTNESS_DOWN, traitees dans le
//   meme case que CMD_BRIGHTNESS (processPendingMqttCommand()) : le
//   pourcentage courant est reconstruit depuis screenBrightness (0-255,
//   round(x*100/255)), le delta est applique et clampe [0,100], puis
//   applique en live via display->setBrightness8() comme les autres
//   commandes de luminosite. Difference volontaire avec CMD_BRIGHTNESS
//   (qui reste RAM-only) : ces 2 nouvelles commandes appellent aussi
//   writeConfigFlag("brightness", ...) pour persister la nouvelle valeur
//   dans /config.ini (choix utilisateur explicite -- un +10%/-10% declenche
//   depuis un script doit survivre a un reboot, sans repasser par le
//   bouton "Sauvegarder" de la page web). L'ecriture SD est conditionnelle
//   (seulement si le pourcentage clampe differe reellement du courant,
//   ex. deja a 100% et +10% demande) pour eviter des ecritures inutiles.
//   Pas encore teste sur materiel reel.
//
// v76 - 2026-08-11 - safe-modify - Log diagnostique esp_reset_reason() au
//   boot (demande utilisateur), suite a un crash a distance non explique :
//   log serie se terminant en texte UART corrompu ("[MQTT] faile rcLj�j"
//   etc.) juste avant un "rst:0x1 (POWERON_RESET)" generique du bootloader
//   ROM -- signature typique d'un brownout (chute de tension), mais le BOD
//   materiel de l'ESP32 est deliberement desactive au tout debut de setup()
//   ("evite reboot intempestifs", raison d'origine non documentee), donc
//   aucun message clair ne pouvait le confirmer jusqu'ici -- chaque
//   brownout reel se presentait comme un simple redemarrage generique.
//   Purement diagnostique, ne change AUCUN comportement : un seul
//   Serial.printf() supplementaire au tout debut de setup(), traduit
//   esp_reset_reason() (registre RTC distinct du BOD bas niveau desactive
//   plus haut) en texte lisible (POWERON/BROWNOUT/PANIC/TASK_WDT/...). Si
//   un futur crash reaffiche encore POWERON malgre ce log, ce sera la
//   confirmation que le probleme est hors de portee de tout diagnostic
//   logiciel (brownout trop severe/rapide pour etre vu par l'ESP32
//   lui-meme) -- pointerait vers l'alimentation (adaptateur/cable USB,
//   consommation cumulee ESP32+matrice HUB75+pics WiFi). Pas encore
//   reteste sur materiel apres ce fix (attend un futur crash pour
//   verifier son utilite).
//
// v75 - 2026-08-11 - safe-modify - Fix "Reprendre DMD" ignore pendant un
//   apercu horloge actif (3e bug trouve en test materiel sur l'apercu
//   horloge, apres v73/v74). Repro : ouvrir la page Horloge (apercu auto
//   v58), cliquer "Reprendre DMD" PENDANT que l'apercu tourne encore --
//   log serie confirme resumePlaylist() ouvre bien le GIF suivant
//   ("[GIF] open OK ...") mais le DMD reste visuellement bloque sur
//   l'animation du theme horloge. Cause : showClock() en mode preview ne
//   sait s'arreter que via hasPendingMqttCommand() (nouvelle selection de
//   theme ou "stop" poste par /clock-preview) -- /dmd-resume ne passe PAS
//   par ce mecanisme, webDmdResume()/resumePlaylist() modifient bien
//   currentMode mais showClock() n'en a aucune connaissance et continue de
//   dessiner par-dessus a chaque iteration de sa boucle bloquante.
//   Fix : nouveau drapeau dedie g_clockPreviewAbort (pas de reutilisation
//   du pendingCmd "stop" existant -- celui-ci repasserait currentMode en
//   MODE_CONFIG au tour suivant de loop(), ecrasant a tort le
//   MODE_PLAYLIST/GIF que resumePlaylist() vient de poser). Pose par
//   webDmdResume(), consomme par showClock() a ses 2 points de controle
//   (banniere de nom + boucle principale) : sortie immediate sans toucher
//   currentMode, deja a jour cote resumePlaylist(). Pas encore reteste sur
//   materiel apres ce fix.
//
// v74 - 2026-08-11 - safe-modify - Fix ecran noir/vide du preview horloge
//   (2e test materiel apres v73) : showClock() contient 4 gardes
//   `if (g_sdOpInProgress) return true;` (1 avant le choix du theme, 2 dans
//   les 2 bannieres de nom de 800ms, 1 dans la boucle principale) herites du
//   comportement normal (hors preview), ou ils evitent d'afficher l'horloge
//   par-dessus l'ecran de pause web -- mais en previewMode, g_sdOpInProgress
//   est PERMANENMMENT vrai (c'est justement la page Horloge, web ouverte,
//   qui demande le preview), donc le tout premier de ces 4 gardes coupait
//   systematiquement avant meme de choisir un theme : ecran noir garanti.
//   Log serie de reproduction : "[CLOCK] preview theme=X" (affiche cote
//   appelant, processPendingMqttCommand()) jamais suivi de
//   "[CLOCK] Start retro theme=" (plus bas dans showClock()) -- preuve du
//   retour immediat au tout premier garde. Les 4 occurrences corrigees en
//   `if (!previewMode && g_sdOpInProgress)` : en previewMode, seul
//   hasPendingMqttCommand() (nouvelle selection ou "stop") doit interrompre
//   l'affichage, comme deja documente en tete de cette fonction depuis v72
//   mais jamais reellement applique a ces 4 endroits. Pas encore reteste
//   sur materiel apres ce fix.
//
// v73 - 2026-08-11 - safe-modify - Fix test materiel reel du preview horloge
//   (v72) : 2 bugs bloquants trouves sur le premier test.
//   (1) La protection web (g_sdOpInProgress, ecran "WEB DMD CONFIG" pose par
//   triggerWebConfigModeSoft() a CHAQUE chargement de page config) etait
//   annulee des qu'on quittait l'onglet Horloge : le cas "stop" de
//   CMD_CLOCK_PREVIEW (déclenché par le sendBeacon pagehide/beforeunload de
//   la page Horloge, y compris en changeant simplement d'onglet vers
//   Basic/Network/Media) appelait resumePlaylist() sans condition, qui
//   repasse currentMode a MODE_PLAYLIST et relance les GIFs -- alors que
//   g_sdOpInProgress restait a true (jamais touche par resumePlaylist()).
//   Resultat observe en test reel : la protection "flotte", parait ne
//   s'activer qu'apres changement d'onglet. resumePlaylist() est reserve au
//   bouton explicite "Reprendre DMD" (/dmd-resume) ; le cas "stop" se
//   contente desormais de reafficher l'ecran de pause config existant
//   (MODE_CONFIG + webDmdForceRedraw(), g_sdOpMsg/g_sdOpSubMsg deja a jour
//   depuis le dernier triggerWebConfigModeSoft()).
//   (2) Le preview lui-meme ne pouvait jamais s'activer : le garde
//   `if (g_sdOpInProgress) { ... ignoree ... }` copie par erreur depuis les
//   handlers MQTT (ou g_sdOpInProgress=true protege le web contre une
//   interruption EXTERNE) est toujours vrai des que la page Horloge
//   elle-meme est ouverte (posee par son propre chargement de page juste
//   avant) -- bloquait donc 100% des tentatives reelles (confirme par le
//   log serie : "[CLOCK] preview ignoree (web open)" sur la seule
//   selection tentee). Garde supprimee : aucun conflit SD reel a proteger
//   ici (webServer->handleClient() est mono-thread, un upload bloquerait de
//   toute facon le traitement de toute autre requete concurrente).
//   Pas encore reteste sur materiel apres ce fix.
//
// v72 - 2026-08-11 - safe-modify - Apercu en direct des themes horloge
//   depuis la page web (onglet Horloge, worktree dev/clock-theme-preview) :
//   selectionner un theme dans la liste deroulante l'affiche IMMEDIATEMENT
//   sur le DMD physique, sans limite de duree -- il reste affiche jusqu'a
//   ce qu'un autre theme soit selectionne (bascule immediate) ou que la
//   page Horloge soit quittee (navigator.sendBeacon sur pagehide/
//   beforeunload, cote web_config.h). Pas de filet de securite additionnel
//   (decision utilisateur) : si le signal d'arret n'arrive jamais
//   (fermeture brutale du navigateur), l'apercu reste affiche jusqu'au
//   prochain evenement MQTT ou reboot -- comportement assume.
//   showClock() (inchangee pour son appel normal existant) gagne un
//   parametre optionnel forceTheme=-2 (sentinelle, ne collisionne pas avec
//   -1=aleatoire) : quand fourni (>=-1), ignore clockEnabled, impose
//   currentTheme, et sa boucle interne tourne SANS condition de duree
//   (fini par hasPendingMqttCommand() deja verifie a chaque iteration,
//   comme n'importe quelle interruption MQTT normale). Nouvelle commande
//   interne MqttCommand::CMD_CLOCK_PREVIEW (topic web uniquement, pas
//   expose en MQTT) : argument = theme ("-1".."9") ou "stop". Reutilise
//   l'infrastructure pendingCmd/processPendingMqttCommand() deja eprouvee
//   pour interrompre proprement ce qui est affiche (meme nettoyage que
//   CMD_STOP), sans risque de reentrance (showClock() elle-meme appelle
//   deja handleWebConfig() dans sa boucle, donc jamais d'appel direct
//   depuis un handler web). Pas encore teste sur materiel reel.
//
// v71 - 2026-08-11 - safe-modify - Luminosite DMD appliquee en direct (sans
//   reboot), demande explicite utilisateur (worktree dev/live-brightness).
//   Avant ce fix, display->setBrightness8() n'etait appele qu'une seule
//   fois au setup() -- tout changement de screenBrightness (page web ou,
//   desormais, MQTT) restait sans effet sur le hardware jusqu'au prochain
//   redemarrage. Fix : nouvelle commande MQTT dediee MqttCommand::
//   CMD_BRIGHTNESS (topic marquee/cmd/brightness, argument = pourcentage
//   0-100 en texte), traitee dans processPendingMqttCommand() -- map vers
//   0-255, ecrit screenBrightness (RAM uniquement, pas de commande MQTT
//   n'ecrit sur SD, coherent avec CMD_STOP/CMD_DEFAULT/etc.), puis appelle
//   display->setBrightness8() immediatement. Complement cote web_config.h
//   (v56) : handleWebConfigSave() applique aussi setBrightness8() des la
//   sauvegarde, + nouvel endpoint /set-brightness pour l'apercu live
//   pendant le drag du curseur. setBrightness8() ne fait que reecrire les
//   bits OE/PWM dans le buffer DMA deja actif (pas de begin()/
//   clearScreen()), donc sans risque a appeler en plein GIF/PNG affiche.
//   Pas encore teste sur materiel reel.
//
// v70 - 2026-08-11 - safe-modify - Fix incoherence de seuil flag "L" entre
//   le chemin normal (outil PC, build_systems_cache(), seuil reglable
//   depuis RecalBoxDMD_tool.py v33, defaut 5000) et le repli firmware
//   buildSysDefaultCache() (emprunte seulement si /systems_cache.dat est
//   absent de la SD au boot, voir loadSysDefaultCache()) : ce dernier
//   utilisait encore l'ANCIEN seuil 800 (jamais mis a jour lors des
//   revisions 800->15000->5000 cote PC). countPngGifOverRec("/systems/"+
//   sysName, 800, ...) -> seuil releve a 5000 pour aligner ce repli rare
//   sur la valeur par defaut actuelle de l'outil PC. Changement isole (une
//   constante), pas de reglage utilisateur cote firmware (reglage reserve
//   a l'outil PC, onglet Parametres -- voir RecalBoxDMD_GUI.py v45/
//   RecalBoxDMD_tool.py v33), pas encore teste sur materiel reel.
//
// v69 - 2026-08-10 - safe-modify - Chemin FAST (isSlow=false) de CMD_GAME :
//   ajout d'un pre-check du cache bigramme (findInGamesCache(), meme
//   mecanisme deja utilise par le chemin SLOW) AVANT toute tentative
//   d'ouverture SD reelle. Cause : mesure reelle sur mame (temporairement
//   teste en FAST sous un seuil de flag L trop haut, voir outil PC v31/v32)
//   -- un jeu absent de la SD force drawRaw565() a scanner l'INTEGRALITE du
//   dossier physique alphabetique avant de conclure "absent" (pire cas pour
//   un scan de repertoire sequentiel, pas de sortie anticipee), mesure
//   jusqu'a 3.3s sur mame/S (4641 entrees). Cette verification n'a jamais eu
//   de raison d'etre limitee au flag L : games_cache.bin est construit pour
//   TOUS les systemes sans distinction (RecalBoxDMD_tool.py::build_cache()),
//   le flag L ne determinait que QUEL chemin de code y avait acces. Fix :
//   si cached=='?' (jeu absent du cache), saut direct au repli
//   default.png/default.raw existant, sans tenter drawPng()/openGif() sur
//   le vrai chemin du jeu. Comportement du cas "jeu present" strictement
//   inchange (les 3 tentatives reelles restent identiques, juste sautees
//   dans le cas absent).
//
// v68 - 2026-08-10 - safe-modify - Suite de v67 : test reel confirme une
//   AMELIORATION MAJEURE (2 tests intensifs consecutifs sans incident sur
//   3do et amiga600) mais PAS une elimination totale -- un incident isole
//   de gel silencieux ~5min13s (avec auto-recuperation, sans reboot) sur
//   amiga600. Demande explicite utilisateur : continuer a eliminer le
//   DECLENCHEMENT du gel (le mecanisme bas niveau lui-meme restant hors de
//   portee), meme au detriment d'autres fonctionnalites. Piste identifiee
//   des le debut de la session (jamais pleinement testee) : WiFi.
//   setAutoReconnect(true) demarre une tache interne au driver WiFi,
//   opaque et hors controle applicatif, 2e candidat de collision LWIP avec
//   mqttTask en plus de playlistGenTask() (deja elimine en v67). Fix :
//   setAutoReconnect() passe a false (setupWiFiFromConfig()) --
//   maintainWiFi() (deja en place, appelee a chaque loop(), cooldown 5s,
//   reapplique l'IP fixe) devient la SEULE source de reconnexion WiFi,
//   entierement sous controle applicatif. Commentaire de
//   MQTT_WIFI_SETTLE_MS (mqttTask()) mis a jour pour refleter que la
//   mitigation vise desormais la fenetre de reconnexion de maintainWiFi(),
//   plus le driver. Compromis assume : reconnexion potentiellement un peu
//   moins reactive dans certains cas limites que le driver interne
//   n'aurait pu gerer. Pas encore teste sur materiel reel.
//
// v67 - 2026-08-10 - safe-modify - Correctif final de la session de
//   diagnostic mqttTask/LWIP : v65/v66 (ci-dessous) ciblaient sdAccessMutex/
//   plGenStatusMutex dans CE fichier (.ino) mais se sont averes
//   INSUFFISANTS en test reel -- bissection par FICHIER a ensuite montre
//   que la regression vit dans web_config.h, pas ici (voir memoire projet
//   pour le detail complet de la bissection). Root cause : playlistGenTask()
//   (tache FreeRTOS dediee pour la generation de playlist, introduite
//   2026-07-30) et sdAccessMutex (partage avec gifPlayFrameCompat()/
//   openNextGif() dans ce fichier). Fix retenu (approuve par l'utilisateur,
//   priorite explicite : fiabilite MQTT/affichage avant confort de
//   generation de playlist) : RETOUR de la generation de playlist a une
//   machine a etats dans loop() (playlistGenStep(), voir web_config.h v54)
//   au lieu d'une tache dediee. Consequence directe sur ce fichier :
//   - gifPlayFrameCompat()/openNextGif() : sdAccessMutex retire ENTIEREMENT
//     (plus juste "conditionnel" comme en v65) -- retour a leur forme
//     d'origine, plus aucune operation FreeRTOS bas niveau sur ce chemin
//     hors generation.
//   - loop() : appel a playlistGenStep() reintroduit (retire en meme temps
//     que playlistGenTask() avait ete ajoutee), meme position qu'a
//     l'origine (avant handleWebConfig()).
//   - sdAccessMutex ET plGenStatusMutex retires entierement (declaration,
//     creation dans setup(), tous les xSemaphoreTake/Give) : plus aucun
//     acces concurrent a proteger, playlistGenStep() tourne exclusivement
//     dans loop(), meme contexte d'execution que gifPlayFrameCompat()/
//     openNextGif()/les handlers HTTP.
//   - playlistGenTaskHandle retire (plus de tache a pointer).
//   Pas encore teste sur materiel reel -- verification prioritaire : la
//   meme rafale MQTT intensive (3do + amiga600/Zyconix 525f + Zool2 55f)
//   qui faisait planter v52/v64/v65/v66 de facon fiable.
//
// v66 - 2026-08-10 - safe-modify - v65 CONFIRME INSUFFISANT en test reel
//   (freeze 3do reproduit a l'identique malgre le fix gifPlayFrameCompat()/
//   openNextGif()). Candidat suivant du meme commit "v95/4c663fb" : dans
//   mqttTask(), un xSemaphoreTake(plGenStatusMutex,0)/Give tournait SANS
//   AUCUNE CONDITION a CHAQUE iteration de la boucle (~50 fois/seconde en
//   regime normal, meme sans playlistGenTask() actif) -- juste avant les
//   operations socket/LWIP de mqttTask lui-meme, candidat plus direct que
//   le precedent (c'est la MEME tache qui se bloque dans LWIP). Fix
//   identique : lecture non protegee de g_plGenStatus.active, mutex retire
//   entierement de ce point (plus jamais pris ici, meme si active=true --
//   le check g_sdOpInProgress||plGenActiveNow n'a de toute facon besoin
//   que d'un indice approximatif, pas d'une lecture strictement a jour).
//   Pas encore teste sur materiel reel.
//
// v65 - 2026-08-10 - safe-modify - Correctif cible suite a la regression
//   bissectee au commit git "v95/4c663fb" (30 juillet, introduction de
//   sdAccessMutex) : gifPlayFrameCompat() (tourne a CHAQUE frame affichee)
//   et openNextGif() (chaque transition entre 2 GIFs) prenaient
//   systematiquement sdAccessMutex (xSemaphoreTake/Give), meme quand
//   playlistGenTask() n'a jamais tourne (99% du temps reel) -- des
//   milliers d'operations FreeRTOS bas niveau par minute sur le chemin le
//   plus chaud du firmware, sur le meme coeur que mqttTask(), augmentant
//   la probabilite de collision avec son propre verrou LWIP interne
//   (voir memoire projet, deadlock mqttTask/LWIP deja documente et
//   reproduit systematiquement via rafale MQTT ciblee). Fix : lecture NON
//   PROTEGEE de g_plGenStatus.active en pre-check rapide (meme convention
//   que g_sdOpInProgress ailleurs dans ce fichier) -- sdAccessMutex n'est
//   desormais pris QUE si une generation semble reellement en cours.
//   Risque residuel accepte : une frame rare non protegee pendant la
//   fenetre de demarrage d'un scan, tres inferieur au cout systematique
//   actuel. Pas encore teste sur materiel reel.
//
// v64 - 2026-08-09 - safe-modify - Suite du diagnostic 3do : le crash
//   mqttTask/LWIP reproduit en rafale MQTT ciblee (BattleSport ->
//   CaptainQuazar -> Cyberia, tous 1 frame, flag N/FAST) a ete confirme
//   sur v63 (ELF SHA different du build precedent, lignes [DIAG]
//   presentes -- donc vrai test v63, pas un ancien binaire). Utilisateur a
//   refute l'hypothese "jamais teste aussi vite avant" (defilements
//   longs deja pratiques historiquement lors des sessions de resolution
//   de lenteur d'affichage) -- mes changements du jour (v60-v63) restent
//   donc suspects. Les 2 lignes [DIAG] (v63, Serial.println() inconditionnel
//   + concatenation String, dans le chemin chaud CMD_GAME) regatees
//   derriere CMD_GAME_DEBUG_LOGS (desactivees par defaut) pour ecarter cet
//   overhead comme confondeur -- garde-fou heap (v60-v62) et
//   reordonnancement loop() (v62) CONSERVES pour ce test (a re-tester sans
//   eux si le crash persiste malgre le retrait des logs). Pas encore
//   teste sur materiel reel.
//
// v63 - 2026-08-09 - safe-modify - DIAGNOSTIC TEMPORAIRE (demande
//   utilisateur : determiner si seuls les systemes flag L sont impactes
//   par les crashs/gels raw565pack, ou tout raw565pack quel que soit le
//   flag systeme). Ajout de 4 Serial.println("[DIAG] ...") TOUJOURS
//   VISIBLES (pas gates par CMD_GAME_DEBUG_LOGS) dans processPendingMqttCommand()
//   CMD_GAME : slowFlag/isSlow a l'entree, sysT sur le chemin FAST, cached
//   et sysT sur le chemin SLOW. Objectif : confirmer directement dans le
//   log serie, pour chaque jeu teste, son flag reel (L/N) et son type
//   (g/p/B) sans avoir a activer tout CMD_GAME_DEBUG_LOGS (trop verbeux).
//   A RETIRER (ou regater derriere CMD_GAME_DEBUG_LOGS) une fois
//   l'investigation terminee -- pas un correctif fonctionnel.
//
// v62 - 2026-08-09 - safe-modify - Suite de v61, deux corrections
//   distinctes issues de tests reels utilisateur le meme jour :
//   1) Blocage confirme et reproduit plusieurs fois (Zool2 55 frames, puis
//      ZakMcKracken 87 frames) : ecran DMD fige sur un raw565pack, PLUS
//      AUCUN changement de jeu traite/logue, alors que mqttTask() continue
//      des cycles connecting/connected toutes les ~89s+10s (log tres
//      regulier). Analyse : loop() (ligne 5410) appelait handleWebConfig()
//      (webServer->handleClient(), passe par la couche socket LWIP) AVANT
//      processPendingMqttCommand() -- si handleWebConfig() se bloque sur
//      un verrou LWIP bas niveau retenu ailleurs (meme famille que le
//      deadlock mqttTask/LWIP deja documente, backtrace decode
//      anterieurement : sys_mutex_lock/xQueueSemaphoreTake), TOUTE
//      l'iteration de loop() reste bloquee avec lui, y compris
//      processPendingMqttCommand() -- pendingCmd (un seul slot) se fait
//      alors ecraser silencieusement par chaque nouveau message MQTT recu
//      entre-temps, sans jamais etre traite ni logue. Fix : appel de
//      processPendingMqttCommand() deplace en TOUT PREMIER dans loop(),
//      avant handleWebConfig(). Ne resout pas la cause racine (verrou hors
//      de portee du code applicatif) mais garantit que la commande en
//      attente AU DEBUT de chaque iteration est bien consommee avant tout
//      risque de blocage sur la partie web.
//   2) Nouveau crash confirme (abort() std::terminate/make_shared<VFSFileImpl>,
//      backtrace decode) sur 3 changements de jeu tres rapproches (moins
//      de 700ms) -- PAS dans le chemin protege par le garde-fou v60/v61
//      (dispatch 'B'/'g'/'p') mais dans le dessin du MASK SYSTEME
//      (drawRaw565(maskBase+".raw565"), appele plus tot dans CMD_GAME,
//      avant meme la logique de type de jeu). Le garde-fou v60/v61 ne
//      couvrait qu'UN site d'appel parmi plusieurs. Fix : garde-fou
//      CENTRALISE directement dans drawRaw565() (verifie une seule fois,
//      protege TOUS les appelants -- mask systeme, repli 'B', repli 'g')
//      au lieu de dupliquer la verification a chaque site d'appel.
//   Pas encore teste sur materiel reel.
//
// v61 - 2026-08-09 - safe-modify - Bug confirme par test reel utilisateur
//   sur v60 : "il n'y a plus jamais de rawpack de lu" -- log serie montre
//   des dizaines de CMD_GAME sur amiga600 sans UNE SEULE ligne "[GIF] open
//   OK raw565pack", et rien d'autre non plus (ni raw565 ni defaut) --
//   silence total. Cause : le seuil v60 (8500) etait mal calibre. Le
//   plancher NORMAL de ESP.getMaxAllocHeap() en fonctionnement sain
//   tourne en continu autour de 4596-5876 (deja documente ailleurs dans
//   ce projet, du au setvbuf(4096) de SD.open() -- PAS une anomalie),
//   donc maxalloc<8500 etait vrai quasi en permanence : le garde-fou
//   interceptait SYSTEMATIQUEMENT avant meme d'essayer le raw565pack.
//   Fix : seuil CMD_GAME_MIN_HEAP_FOR_FILE_OPEN abaisse a 3000 (sous ce
//   plancher normal, pour ne plus intercepter le fonctionnement sain).
//   Egalement : les 2 lignes de log du declenchement du garde-fou (avant
//   caches derriere CMD_GAME_DEBUG_LOGS=false, donc silencieuses --
//   explique pourquoi le bug ci-dessus etait invisible dans les logs)
//   rendues TOUJOURS visibles (evenement rare/exceptionnel, pas du spam
//   par jeu) pour rester diagnosticable a l'avenir sans activer tous les
//   logs verbeux. Pas encore teste sur materiel reel.
//
// v60 - 2026-08-09 - safe-modify - Demande utilisateur suite a plusieurs
//   crashs reels confirmes (abort() dans lock_init_generic lors d'un
//   SD.open() en heap tres bas, sur amiga600/Zork* entre autres) :
//   garde-fou heap bas dans le chemin lent CMD_GAME, MAIS le repli ne se
//   declenche QUE si ESP.getMaxAllocHeap() < CMD_GAME_MIN_HEAP_FOR_FILE_OPEN
//   (8500 octets) -- jamais systematiquement sur un flag systeme 'B'.
//   Comportement normal (heap suffisant) inchange : 'B' suit toujours le
//   chemin 'g' (raw565pack via openGif() en premier). Si heap bas :
//   - flag 'B' -> tente drawRaw565(gameBase+".raw565") directement (une
//     seule lecture SD de 8192 octets, sans .meta ni cache de delais,
//     donc moins couteux que raw565pack) avant d'abandonner ;
//   - flag 'g'/'p' purs, ou 'B' sans .raw565 propre a ce jeu -> repli
//     direct sur drawDefaultRaw565Cached() (zero allocation, deja en RAM)
//     au lieu de tenter l'ouverture et risquer l'abort().
//   Pas encore teste sur materiel reel.
//
// v59 - 2026-08-09 - safe-modify - Demande utilisateur : rendre desactivables
//   les logs verbeux de CMD_GAME (19 Serial.println, la plupart avec
//   plusieurs concatenations de String Arduino, tournant a CHAQUE
//   changement de jeu sur les systemes lents) -- piste de test pour la
//   fragmentation heap observee (concatenation String = plusieurs
//   malloc/free de tailles variees par appel, cause classique documentee
//   de fragmentation sur ESP32/Arduino). Nouveau const bool
//   CMD_GAME_DEBUG_LOGS (desactive par defaut) enveloppe les 19 lignes.
//   Pas une correction confirmee -- un test pour isoler si la
//   fragmentation vient de la ou d'ailleurs. Repasser a true pour
//   retrouver le detail complet si besoin de redeboguer le flux
//   CMD_GAME.
//
// v58 - 2026-08-09 - safe-modify - Mitigation (pas un vrai fix -- cause
//   racine hors de portee du code applicatif) du deadlock mqttTask deja
//   documente (backtrace decode via addr2line a 2 reprises : blocage
//   dans PubSubClient::connect() -> appels LWIP internes ->
//   xQueueGenericSend/vPortExitCritical, jamais debloque avant le
//   watchdog -> abort()+reboot). Confirme une 3e fois par l'utilisateur,
//   cette fois sans watchdog : gel de ~101s de TOUT l'appareil (pas
//   seulement MQTT -- l'allocateur heap ESP32 utilise un verrou global
//   partage entre les 2 coeurs, un deadlock LWIP cote mqttTask peut donc
//   geler toute allocation memoire cote loop(), meme sur l'autre coeur),
//   suivi d'une recuperation automatique (alerte orange "RecalBox non
//   connectee" affichee une fois mqttClient.connect() enfin debloque en
//   echec, puis reprise normale).
//   mqttTask() n'attend desormais plus AU MOINS MQTT_WIFI_SETTLE_MS
//   (1.5s) apres une transition WiFi deconnecte->connecte avant de
//   tenter mqttClient.connect() -- reduit la fenetre de collision avec
//   la tache interne du driver WiFi (WiFi.setAutoReconnect(true)), qui
//   peut encore manipuler la pile socket juste apres une reconnexion.
//   Piste, pas une certitude : l'incident du log fourni par l'utilisateur
//   n'a PAS de reconnexion WiFi visible juste avant (WiFi deja stable
//   depuis longtemps) -- ce fix ne couvre donc pas forcement CE cas
//   precis, mais reduit un risque reel identifiable sans pretendre
//   corriger la cause profonde (verrou LWIP bas niveau).
//   PAS ENCORE teste sur materiel reel.
//
// v57 - 2026-08-09 - safe-modify - 2 bugs/retours sur le v56 (ecran "mode
//   secours AP") apres test reel :
//   1. Le SSID/IP ne ressortait PAS en blanc malgre le mecanisme deja en
//      place (g_sdOpSubMsgWhiteFrom) -- bug reel : le global n'etait
//      JAMAIS positionne aux 2 sites qui construisent g_sdOpSubMsg dans
//      maintainApRecovery()/setupWiFiFromConfig(), restait donc a sa
//      valeur par defaut -1 (comportement 1-couleur inchange). Fix :
//      calcule desormais la longueur du prefixe (trJoinWifi(ssid).length()
//      - ssid.length(), meme principe pour trOpenInBrowser()) et
//      positionne g_sdOpSubMsgWhiteFrom aux 2 sites.
//   2. Demande utilisateur : pause sur le SSID/IP une fois entierement
//      revele par le defilement (au lieu de reduire la vitesse partout,
//      qui aurait retarde tout le message y compris le prefixe). Nouveau
//      g_sdOpSubMsgPauseUntil : des que le defilement de la ligne 2
//      atteint la position ou la fin de la chaine (donc le SSID/IP en
//      blanc) est entierement visible a l'ecran, pause ~1.8s avant de
//      reprendre le defilement -- laisse le temps de lire sans repasser
//      par une vitesse plus lente sur tout le message.
//   3. Demande utilisateur : le SSID doit toujours s'afficher en premier
//      (avant l'IP). Bug reel trouve : le minuteur d'alternance
//      comparait millis() ABSOLU (temps ecoule depuis le tout premier
//      boot) a "lastToggle", initialise a 0 -- si le boot avant d'entrer
//      en mode secours prend deja plus de 6s (frequent), le tout 1er
//      basculement se declenchait quasi immediatement, montrant l'IP en
//      premier au lieu du SSID. Fix : minuteur desormais base sur
//      "elapsed" (temps ecoule DEPUIS l'entree en mode secours), garantit
//      une vraie fenetre de 6s de SSID avant le 1er basculement.
//
// v56 - 2026-08-09 - safe-modify - Suite immediate du v55 (ecran "mode
//   secours AP"), retour utilisateur apres relecture -- meme en corrigeant
//   le reset intempestif du defilement, la ligne 2 restait trop lente pour
//   parcourir tout le prefixe + SSID/IP dans la fenetre de 6s entre 2
//   bascules ("ca coupe la fin des messages avant qu'ils soient complets").
//   2 changements demandes :
//   1. Vitesse de defilement x4 (1px -> 4px par tick de 100ms, lignes 1 ET
//      2 de MODE_CONFIG) -- calcule pour que le message le plus long
//      ("Ouvrez dans un navigateur http://192.168.4.1") ait le temps de
//      reveler completement le SSID/IP en ~3.4s au lieu de ~13.6s, avec
//      marge dans la fenetre de 6s.
//   2. Le SSID/IP ressort desormais en BLANC (0xFFFF) plutot que la meme
//      couleur que le prefixe d'instruction -- nouveau global
//      g_sdOpSubMsgWhiteFrom (index a partir duquel basculer en blanc,
//      -1 = desactive/comportement inchange pour tous les autres ecrans
//      MODE_CONFIG) + nouvelle fonction partagee drawSdOpSubMsgAt(x),
//      utilisee par webDmdForceRedraw() ET le bloc de defilement de
//      loop() pour ne pas dupliquer la logique 2-couleurs. Seul
//      maintainApRecovery() positionne ce nouveau global -- jamais de
//      fuite vers les autres ecrans (la sortie du mode secours AP passe
//      toujours par un ESP.restart(), qui reinitialise ce global a -1).
//   PAS ENCORE teste sur materiel reel.
//
// v55 - 2026-08-08 - safe-modify - Fix bug signale par l'utilisateur sur
//   l'ecran "mode secours AP" (maintainApRecovery(), declenche par le
//   script Recalbox "WiFi Recovery DMD") : "sursaut" du defilement de la
//   ligne 2, SSID/IP jamais visibles, mots qui semblent se melanger.
//   Cause reelle (tracee par lecture du code, pas testee sur materiel) :
//   la mise a jour du compte a rebours (ligne 1, CHAQUE SECONDE) passait
//   par g_configDmdDirty=true -> webDmdForceRedraw(), qui remet aussi a
//   zero le defilement de la ligne 2 (g_sdOpScrollOffset) -- alors que
//   seule la ligne 1 avait change. Le SSID/l'IP, situes en fin de chaine
//   apres un long prefixe ("Rejoignez le wifi "/"Ouvrez dans un
//   navigateur "), n'avaient donc jamais le temps de defiler jusqu'a
//   l'ecran avant d'etre remis a zero la seconde suivante. Fix : la mise
//   a jour du countdown redessine desormais directement la ligne 1 seule
//   (meme rendu que webDmdForceRedraw() pour cette ligne), sans toucher
//   a g_configDmdDirty ni a l'etat de defilement de la ligne 2.
//   PAS ENCORE teste sur materiel reel.
//
// v54 - 2026-08-07 - safe-modify - Refonte demandee des 3 indicateurs DMD
//   (vert "RecalBox connectee", orange "RecalBox hors ligne", rouge
//   desormais traduit) suite au constat que meme le texte raccourci v53
//   restait contraignant sur 1 seule ligne :
//   - Passage sur 2 lignes centrees (nouvelle fonction partagee
//     drawTwoLineCenteredOverlay(), remplace la logique dupliquee dans
//     les 3 fonctions de dessin) avec police ADAPTATIVE : taille 2
//     (12px/caractere, plus lisible) si les 2 lignes tiennent dans
//     RAW565_W=128px, repli automatique sur taille 1 (6px/caractere)
//     sinon -- calcule independamment par alerte/langue.
//   - Symbole ASCII d'humeur ajoute en fin de 2e ligne (demande
//     utilisateur) : ":)" vert, ":/" orange (interrogatif/pas
//     convaincu), ":(" rouge -- pas de gras (double-dessin ombre noire
//     existant conserve tel quel, pas de passes supplementaires).
//   - Alerte rouge "No wifi, No Recalbox" : etait volontairement fixe/
//     non traduite depuis le 2026-08-05 (v52) -- demande utilisateur de
//     la traduire desormais que la place sur 2 lignes le permet.
//     Nouvelle trNoWifiNoRecalbox(). FR "Pas de wifi"/"Pas de Recalbox",
//     EN "No wifi"/"No Recalbox", ES "Sin wifi"/"Sin Recalbox".
//   - trRecalboxConnected()/trRecalboxDisconnected() changent de
//     signature (String&,String& en sortie au lieu d'un retour String
//     unique) pour porter les 2 lignes.
//   PAS ENCORE teste sur materiel reel (notamment le cas taille 2 sur
//   2 lignes qui remplit exactement les 32px de hauteur de l'ecran sans
//   marge pour l'ombre du bas -- devrait etre clippe silencieusement par
//   la lib d'affichage, a verifier visuellement).
//
// v53 - 2026-08-07 - safe-modify - Texte de l'alerte orange "RecalBox non
//   connectee"/"not connected"/"no conectada" (v52) depassait la largeur
//   de l'ecran DMD (RAW565_W=128px, budget 21 caracteres a taille de
//   police 1/6px-car) sur les 3 langues : FR 23 car., EN 23 car., ES 22
//   car. -- toutes en debordement, pas seulement le francais (bug
//   signale par l'utilisateur sur le FR, verifie ensuite sur les 3).
//   trRecalboxDisconnected() raccourci : "RecalBox deconnectee" (FR, 20
//   car.), "RecalBox offline" (EN, 17 car.), "RecalBox offline" (ES,
//   17 car. -- terme technique repris tel quel, "disconnected"/
//   "desconectada" restent trop longs meme seuls).
//
// v52 - 2026-08-05 - safe-modify - Fusion dev/tous-txt-filter -> master
//   (demande explicite utilisateur), tests materiel confirmes OK par
//   l'utilisateur pour ce lot. RETRO_VERSION (splash boot, ecran physique)
//   passee de "Raw565 Ed. dev12" a "Raw565 Ed. v12" -- retire le prefixe
//   "dev" devenu inexact une fois sur master (meme demande explicite).
//   Suite immediate (meme jour, meme v52) : demande explicite utilisateur
//   "limiter l'affichage des images d'alerte de connexion recalbox a 3
//   fois (initial, 60s, 120s)". Les indicateurs rouge "No wifi, No
//   Recalbox" et orange "RecalBox non connectee" (v51) se repetaient
//   indefiniment toutes les 60s tant que le probleme persistait --
//   plafonnes desormais a 3 occurrences par episode de coupure via 2
//   nouveaux compteurs locaux (wifiAlertCount/recalboxDisconnectedAlertCount,
//   mqttTask()), remis a 0 des que la connexion revient (une nouvelle
//   coupure ulterieure redeclenche donc bien 3 affichages a son tour).
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v51 - 2026-08-05 - safe-modify - Refonte complete premier demarrage/AP/
//   mode config (plan valide en mode Plan, voir memoire projet) + 2
//   indicateurs visuels DMD. Resume :
//   1. setupWiFiFromConfig() : repli AP sur echec WiFi ne se declenche plus
//      que si g_firstBoot est vrai (WiFi injoignable + config deja complete
//      => demarrage normal, plus de reboot force en AP).
//   2. setup() : needWebConfigMode = (playlist vide) || (IP Recalbox vide)
//      || g_firstBoot, remplace les usages isoles de g_firstBoot (recalboxIP
//      n'etait auparavant jamais teste comme declencheur du mode config).
//   3. Bug ecran "DMD WEB CONFIG"+"0.0.0.0" en mode AP pur corrige (repli
//      0.0.0.0->softAPIP()->192.168.4.1 ajoute a triggerWebConfigModeSoft()
//      et son duplicata inline upload).
//   4. Indicateur rouge clignotant "No wifi, No Recalbox" (WiFi injoignable,
//      1ere tentative puis toutes les 60s) et indicateur orange clignotant
//      "RecalBox non connectee" (WiFi OK mais mqttClient.state()==-2) --
//      tous deux : image de secours default.raw565, affichage temporise 5s
//      puis reprise automatique de la playlist, place entre 2 GIFs (jamais
//      en coupant une animation en cours).
//   Voir web_config.h v51 pour le volet interface web (first_boot n'est plus
//   efface par un simple affichage de page, modale d'aide, alerte champs
//   essentiels vides, brouillon localStorage multi-pages, fix language=).
//   Compilation via compile.ps1 : OK (64% flash, 28% RAM). Test materiel
//   reel EN COURS (2026-08-05) : messages d'alerte web + DMD + modale
//   d'aide confirmes OK par l'utilisateur ; reste du parcours (AP/premier
//   demarrage complet, coupure WiFi/MQTT reelle prolongee) PAS ENCORE
//   teste.
//
// v50 - 2026-08-03 - safe-modify - Bug reel confirme (retour utilisateur,
//   suite au fix v47) : "Reprendre DMD" alors que RB est en mode clip/demo
//   ne reprenait jamais la playlist -- RB annonce son passage en demo UNE
//   SEULE FOIS (CMD_DEFAULT/CMD_STARTCLIP), pas a chaque nouveau clip, donc
//   v47 (qui affiche l'ecran d'attente et attend un nouveau message MQTT)
//   restait bloque indefiniment dans ce cas precis. Fix : nouveau
//   g_lastMqttWasDefault, memorise le dernier contenu REELLEMENT affiche via
//   MQTT avant l'ouverture du mode config (true=playlist/demo via
//   CMD_DEFAULT/CMD_STARTCLIP, false=system/jeu precis via
//   CMD_SYSTEM/CMD_GAME/CMD_RESUMESYS). webDmdResume() reprend directement
//   la playlist si le dernier etat connu etait deja la playlist (encore
//   valide, RB ne va rien renvoyer de plus), affiche l'ecran d'attente
//   uniquement si c'etait un system/jeu precis (potentiellement perime).
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v49 - 2026-08-03 - safe-modify - Regression du fix v46 confirmee en test
//   reel (retour utilisateur : ca fonctionne mais le delai d'affichage de
//   5-10s de l'ecran "RecalBox connectee" n'est plus respecte, bascule
//   immediate sur playlist) -- v46 avait retire "default" du filtrage de la
//   fenetre de grace pour corriger le blocage indefini quand RB est deja en
//   demo a la connexion, mais du coup un "default" arrivant tres tot (RB
//   deja en demo) s'applique desormais instantanement, sans laisser voir
//   l'ecran de confirmation. Fix : nouveau delai minimum d'affichage
//   MQTT_WAITING_MIN_DISPLAY_MS (7000ms) distinct de la fenetre de grace
//   (1.5s, toujours utilisee pour system/game) -- si un CMD_DEFAULT arrive
//   pendant ce delai, l'action n'est PLUS ignoree (regression v46) ni
//   appliquee tout de suite (bug remonte) : elle est MEMORISEE
//   (g_mqttDefaultPendingAfterMinDisplay) et appliquee automatiquement des
//   que le delai est ecoule (nouveau bloc dans loop()), jamais perdue. Un
//   vrai system/game recu entre-temps annule cette action differee (plus
//   specifique qu'un simple retour a la playlist). Compilation via
//   compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v48 - 2026-08-03 - safe-modify - Incoherence corrigee par cohorte avec v46
//   (retour utilisateur : RB deja en mode demo/clip a la connexion, jamais
//   bascule sur playlist meme apres plusieurs clips lances) : CMD_STARTCLIP
//   et CMD_RESUMESYS ne remettaient pas g_mqttConnectedScreenUntilMs a 0
//   contrairement aux autres commandes qui peuvent quitter l'ecran d'attente
//   (CMD_STOP/CMD_DEFAULT/CMD_SYSTEM/CMD_GAME, v45/v46) -- ajoute par
//   coherence. Analyse du code n'a PAS trouve d'autre chemin expliquant le
//   symptome exact rapporte (CMD_STARTCLIP appelle deja resumePlaylist()
//   sans condition hors g_sdOpInProgress ; le repli "fallback default.raw565"
//   du mode CMD_GAME lent ne definit ni MODE_PNG ni currentPngPath=
//   DEFAULT_RAW565_PATH, donc ne peut pas a lui seul reactiver le
//   clignotement) -- log serie reel necessaire pour la suite. Compilation
//   via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v47 - 2026-08-03 - safe-modify - Demande explicite utilisateur : "Reprendre
//   DMD" (webDmdResume(), bouton web) forcait systematiquement resumePlaylist()
//   meme si la Recalbox etait deja connectee via MQTT -- coupant un contenu
//   RB legitime (partie en cours) au profit de la playlist locale, corrige
//   seulement au prochain evenement MQTT reel. Fix : si mqttClient.connected(),
//   laisse RB reprendre la main (meme traitement que CMD_WAITING_MQTT --
//   image de secours + texte en attendant le prochain vrai message) au lieu
//   de forcer la playlist. Playlist forcee uniquement si MQTT n'est PAS
//   connecte (aucune autre source de contenu). Compilation via compile.ps1 :
//   OK. PAS ENCORE teste sur materiel reel.
//
// v46 - 2026-08-03 - safe-modify - Bug reel confirme (retour utilisateur) :
//   la detection clip/demo ne fonctionnait pas si la Recalbox etait DEJA en
//   mode demo au moment ou le firmware se connecte a MQTT et affiche l'ecran
//   d'attente -- son message "marquee/cmd/default" arrive alors quasi
//   instantanement (comme un retenu), dans la fenetre de grace de 1.5s
//   (MQTT_WAITING_GRACE_MS, voir v15/v16), et etait ignore a tort. Sans la
//   reprise auto par delai (retiree en v45), l'ecran d'attente restait donc
//   bloque indefiniment dans ce cas precis. Fix : "default" retire du filtre
//   de la fenetre de grace (system/game restent filtres, seuls a risquer
//   d'afficher un jeu perime) -- voir commentaire complet dans
//   onMqttMessage(). Compilation via compile.ps1 : OK. PAS ENCORE teste sur
//   materiel reel.
//
// v45 - 2026-08-03 - safe-modify - 3 bugs confirmes en test reel sur l'ecran
//   "RecalBox connectee" (drawRecalboxConnectedOverlay(), CMD_WAITING_MQTT) :
//   (1) le clignotement noircissait un bandeau plein (fillRect(...,0)) au
//   lieu de laisser voir l'image de fond pendant la phase "invisible" --
//   corrige en redessinant la bande depuis le cache RAM defaultRaw565Buf
//   (deja charge a cet instant par CMD_WAITING_MQTT) au lieu de noircir.
//   (2) texte positionne pres du bas (y=24 fixe, hauteur panneau=32) au lieu
//   d'etre centre verticalement -- corrige (textY=(RAW565_H-8)/2).
//   (3) bascule vers la playlist auto au bout de MQTT_CONNECTED_SCREEN_MS
//   (10s) meme si la Recalbox reste connectee -- confirme par l'utilisateur
//   comme un vrai bug de conception, pas juste un delai trop court : la
//   logique voulue est d'attendre INDEFINIMENT un vrai message MQTT tant que
//   la Recalbox est allumee, c'est ELLE qui decide quand revenir a la
//   playlist (CMD_DEFAULT, pont marquee sur veille/lecture d'un clip),
//   jamais un delai arbitraire cote DMD. MQTT_CONNECTED_SCREEN_MS et le bloc
//   de reprise auto par delai retires entierement de loop() ;
//   g_mqttConnectedScreenUntilMs devient un simple drapeau "ecran d'attente
//   actif" (pose a CMD_WAITING_MQTT, remis a 0 par CMD_STOP/CMD_DEFAULT/
//   CMD_SYSTEM/CMD_GAME) utilise uniquement pour piloter le clignotement.
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v44 - 2026-08-03 - safe-modify - Demande explicite utilisateur : version
//   affichee au splash boot (RETRO_VERSION, ecran physique) passee de
//   "Raw565 Ed. dev_pl" a "Raw565 Ed. dev12" -- distinct du numero de
//   version safe-modify interne de ce fichier.
//
// v43 - 2026-08-03 - safe-modify - Suite de v42 (question utilisateur :
//   "Reprendre DMD" apres une copie relance la playlist ET MQTT, mais sans
//   les 3 caches sautes -- est-ce un probleme ?). Reponse : pas pour la
//   playlist (aucune dependance), mais sysDefaultType()/sysDefaultSlowFlag()/
//   findInGamesCache() renvoyaient silencieusement '?'/'N' en permanence
//   pour tout systeme/jeu si le cache n'avait jamais ete charge -- pas un
//   crash (replis existants sur PNG/GIF), mais des icones systeme/jeu plus
//   lentes/potentiellement mal choisies jusqu'au prochain reboot complet.
//   Choix retenu (vs forcer un reboot complet a "Reprendre DMD", qui aurait
//   annule tout l'interet du reboot cible rapide) : chargement PARESSEUX,
//   meme principe deja utilise par ensureDefaultRaw565Cached() -- nouveaux
//   sysCacheLoadAttempted/gamesCacheLoadAttempted, nouvelles
//   ensureSysDefaultCacheLoaded()/ensureGamesIndexLoaded() appelees en tete
//   de sysDefaultType()/sysDefaultSlowFlag()/findInGamesCache(), rechargent
//   le .dat/.bin existant (jamais buildSysDefaultCache(), scan recursif
//   trop long pour un chemin pouvant etre atteint en plein evenement MQTT)
//   une seule fois au premier vrai besoin si jamais charge au boot. Sur un
//   boot normal, les deux flags sont mis a true juste apres le chargement
//   eager habituel dans setup() -- aucun changement de comportement/cout
//   sur le chemin de boot normal. Compilation via compile.ps1 : OK. PAS
//   ENCORE teste sur materiel reel.
//
// v42 - 2026-08-02 - safe-modify - Demande explicite utilisateur : sur un
//   boot "reboot cible mode config" (g_skipPlaylistForConfig), sauter le
//   chargement des 3 caches lies a l'affichage GIF/MQTT (cache systemes
//   systems_cache.dat, image de secours default.raw565, cache jeux
//   games_cache.bin) -- aucun des trois n'est utilise pendant le mode
//   config (MQTT bloque par g_sdOpInProgress, aucun GIF ouvert), et ce boot
//   n'a qu'un seul but : liberer le heap au plus vite pour demarrer une
//   copie. force_config_boot desormais lu des le premier passage de lecture
//   de config.ini (avant ces 3 chargements), pas seulement dans
//   loadConfig() (appelee apres) qui le relit de toute facon sans effet de
//   bord (aucune ecriture entre les deux lectures). L'index playlist (.idx)
//   reste charge sur ce chemin (cout negligeable, ~7ms) pour que "Reprendre
//   DMD" continue de fonctionner pour la lecture playlist simple -- les 3
//   caches sautes ne servent qu'aux evenements MQTT systeme/jeu, qui ne
//   peuvent de toute facon pas survenir avant un reboot complet normal.
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v41 - 2026-08-02 - safe-modify - Reintroduction du reboot cible mode
//   config (g_skipPlaylistForConfig/force_config_boot/g_playlistStartedThisBoot),
//   retire en v37 (commit "v93") sur la foi d'une comparaison qui ne portait
//   pas sur ce symptome precis. Cause reelle (memoire projet, deja
//   documentee) : chaque SD.open() d'un GIF alloue en interne un buffer
//   setvbuf(4096) jamais recycle proprement -- apres quelques dizaines de
//   GIFs, ESP.getMaxAllocHeap() plafonne durablement vers 4500-5300 octets,
//   quel que soit le temps ecoule ensuite. Confirme en test reel 2026-08-02 :
//   avec le garde heap<6000 de l'upload (web_config.h v42) seul, ce plafond
//   bloquait ~99% des uploads des que la playlist avait tourne un moment.
//   `requestReboot` (variable + check dans loop()) etait deja reste en place,
//   orphelin, depuis le retrait v37 -- reutilise tel quel. Bloc de boot
//   dedie replace a l'identique de l'ancienne implementation (juste avant le
//   check playlistName.length()==0), `g_playlistStartedThisBoot=true` pose
//   au premier openNextGif() de boot. Le chantier de fond (remplacer
//   SD.open() par fopen()/setvbuf() statique pour la lecture GIF) est traite
//   separement sur une branche dev dediee -- ce reboot reste le contournement
//   en attendant. Compilation via compile.ps1 : OK. PAS ENCORE teste sur
//   materiel reel.
//
// v40 - 2026-08-01 - safe-modify - Partie C du plan "cache_master_gifs" :
//   renommage automatique de l'etiquette de volume SD au boot vers
//   "RecalBoxDMD" (11 caracteres, limite FAT classique) si elle ne
//   correspond pas deja -- f_getlabel()/f_setlabel() (API FatFs bas
//   niveau, deja compilees dans ce core), juste apres SD.begin() reussi.
//   Non bloquant en cas d'echec (carte protegee en ecriture, etc.), log
//   uniquement. #include "ff.h" ajoute. Compilation via compile.ps1 : OK
//   (0 erreur, 63% flash, 29% RAM). PAS ENCORE teste sur materiel reel.
//
// v39 - 2026-08-01 - safe-modify - Partie B du plan "cache_master_gifs" :
//   struct PlaylistGenStatus, retrait des champs isResync/foldersChanged/
//   linesAdded/linesRemoved (ajoutes pour tousSyncTask(), lui-meme retire
//   entierement cote web_config.h -- voir son changelog v32 pour le detail
//   complet du chantier). Compilation via compile.ps1 : OK (0 erreur, 63%
//   flash, 29% RAM). PAS ENCORE teste sur materiel reel.
//
// v38 - 2026-07-30 - safe-modify - Demande utilisateur : version affichee au
//   splash boot (RETRO_VERSION) passee de "Raw565 Ed. dev11" a "Raw565 Ed.
//   dev_pl" (branche dev/tous-txt-filter). "Raw565 Ed. dev_playlist" ne
//   rentrait pas (23 caracteres = 138px, ecran raw565 = 128px de large) --
//   abrege en gardant "Ed." (demande explicite) plutot que de le retirer.
//
// v37 - 2026-07-28 - safe-modify - Resynchronisation de cet historique,
//   reste fige sur v36 pendant plusieurs sessions alors que le code a
//   beaucoup change entre-temps (suivi fait via les commits git, pas ce
//   changelog -- v36 ci-dessous decrit un etat depuis longtemps obsolete,
//   source de confusion si lu sans le git log). Recap des changements reels
//   depuis v36, dans l'ordre :
//   - Mecanisme de reboot cible mode config (reintroduit en v36) RETIRE A
//     NOUVEAU et definitivement (commit git "v93") : comparaison avec
//     l'ancienne version fonctionnelle RecalBox_DMDv10_scriptsRB (flashee,
//     testee, fonctionne SANS ce reboot ni garde heap) a montre que le vrai
//     probleme etait ailleurs (voir points suivants) -- g_skipPlaylistForConfig/
//     g_playlistStartedThisBoot/sendRebootingPage/requestReboot supprimes.
//   - Cause racine reelle des blocages "generation de playlist" trouvee :
//     f.readString() chargeant tout le fichier en memoire (jusqu'a 40-44s de
//     blocage ET un resultat FAUX sur une grosse playlist, heap fragmente) --
//     remplace par une lecture en blocs fixes de 512 octets partout.
//   - Generation de playlist transformee en machine a etats non bloquante
//     avec vraie progression web (polling), page verrouillee pendant la
//     generation, bouton Arreter, edition d'une playlist existante par
//     pre-cochage des dossiers.
//   - Ecran "RecalBox connectee" (texte fr/en/es superpose a l'image de
//     secours default.raw565) + reprise automatique de la playlist apres 5s
//     au lieu d'attendre indefiniment le 1er message MQTT reel (commit git
//     "v94", fusionne sur master, valide sur materiel reel).
//   - BRANCHE DEV (ce fichier) : la machine a etats de generation de playlist
//     est deplacee sur sa propre tache FreeRTOS (playlistGenTask(), mirroir
//     de mqttTask()) -- une lenteur SD localisee (confirmee sur plusieurs
//     dossiers reels, simple listing sans lecture de contenu) ne bloque plus
//     loop() (donc le serveur web/le bouton Arreter/reboot) pendant le scan.
//     sdAccessMutex protege les acces SD partages avec la lecture GIF
//     (gifPlayFrameCompat()/openNextGif(), tentative NON bloquante + repli
//     gracieux cote loop() -- seule la tache de fond peut attendre bloquant).
//     Voir le commentaire complet pres de PlaylistGenStatus, juste avant
//     #include "web_config.h". PAS ENCORE teste sur materiel reel.
//
// v36 - 2026-07-27 - safe-modify - BRANCHE DEV : reintroduction du reboot
//   cible mode config (retire en v35), SYSTEMATIQUE cette fois (toutes les
//   pages de config, pas seulement MEDIA comme avant v77). Cause reelle
//   trouvee via logs Serial materiels reels (deux boots complets fournis par
//   l'utilisateur) : mettre en pause un GIF en cours (webDmdPause(), appele
//   par triggerWebConfigMode() dans web_config.h) provoque a lui seul un
//   effondrement de ESP.getMaxAllocHeap() (~4596 octets) par fragmentation
//   (le heap libre TOTAL augmente au meme instant -- ce n'est pas un manque
//   de memoire, c'est de la fragmentation), meme apres un seul GIF ouvert --
//   passe sous le seuil de garde "< 6000" deja utilise par
//   scanGifDirsRaw(), rendant /lsgifdirs definitivement vide pour le reste
//   du boot (BASIC comme MEDIA, puisque BASIC scanne aussi la SD depuis
//   v79 pour la generation de playlist). Restaure a l'identique de la
//   version pre-v35 : g_skipPlaylistForConfig/force_config_boot (config.ini),
//   g_playlistStartedThisBoot, requestReboot, le bloc dedie dans setup() qui
//   saute entierement la playlist/l'ouverture de GIF quand le flag est pose,
//   et le check requestReboot dans loop(). Cote web_config.h :
//   triggerWebConfigMode() redevient bool (voir son changelog v88) et decide
//   du reboot selon g_playlistStartedThisBoot. Le reste du retrait v35 (pas
//   de cache par dossier, pas de navigation/suppression fichier par fichier
//   dans MEDIA) reste inchange. PAS ENCORE teste sur materiel reel.
//
// v35 - 2026-07-27 - safe-modify - BRANCHE DEV, pivot majeur : decision
//   utilisateur de retirer completement la navigation/suppression de
//   fichiers INDIVIDUELS dans un dossier depuis MEDIA (le fait que
//   certains dossiers soient accessibles et d'autres non selon leur statut
//   de cache posait un probleme d'experience utilisateur) -- remplace a
//   terme par un ajout a l'outil PC pour composer des playlists
//   personnalisees (choix de GIF individuels, potentiellement avec
//   miniatures) -- a etudier separement, pas encore commence. Consequence
//   directe : plus besoin du reboot cible mode config (g_skipPlaylistFor
//   Config, force_config_boot, le bloc dedie dans setup()) ni de la machine
//   a etats de cache -- retires. g_playlistStartedThisBoot devenu mort
//   (uniquement lu par la decision de reboot, elle-meme supprimee cote
//   web_config.h) -- retire. loop() n'appelle plus cacheBuilderStep().
//   Verifie explicitement (question utilisateur) que ni le reboot MQTT
//   CMD_REBOOT (Recalbox, "marquee/cmd/reboot") ni le reboot de la page AP
//   (handleWebConfigSaveAP()) ne dependent de ce mecanisme -- deux chemins
//   ESP.restart() entierement separes, non touches. PAS ENCORE teste sur
//   materiel reel.
//
// v34 - 2026-07-27 - safe-modify - BRANCHE DEV : cablage de la nouvelle
//   machine a etats de cache (web_config.h v75). cacheBuilderStep() ajoutee
//   dans loop(), juste apres handleWebConfig() -- avance par petits pas a
//   chaque iteration, aucun cout quand rien a construire (CB_IDLE/CB_DONE).
//   cacheBuilderStart() appelee une seule fois au boot, a l'endroit exact
//   ou warmUpGifCaches() etait appelee avant (v30-v33) : seulement sur le
//   reboot cible mode config (g_skipPlaylistForConfig), jamais pendant une
//   lecture normale de playlist. Contrairement a warmUpGifCaches(),
//   cacheBuilderStart() ne bloque pas -- elle initialise juste l'etat, le
//   scan reel se fait au fil des iterations de loop() suivantes, pendant
//   que la page web reste deja utilisable. PAS ENCORE teste sur materiel
//   reel.
//
// v33 - 2026-07-27 - safe-modify - BRANCHE DEV (dev/cache-externalisation) :
//   suppression du prechauffage bloquant de tous les caches SD au boot
//   (warmUpGifCaches(), v30/v32) -- la fonction elle-meme et tout le
//   systeme de cache SD persistant qu'elle alimentait ont ete retires cote
//   web_config.h (v74, voir son changelog : le navigateur retient
//   desormais le resultat en sessionStorage, chaque dossier n'est plus
//   scanne qu'a la demande). Retire aussi l'affichage DMD associe
//   (trCachingMsg() sur la ligne 1) puisqu'il n'y a plus rien a
//   prechauffer. Consequence attendue : le boot en mode config n'est plus
//   jamais bloque plusieurs minutes par un scan de 18 dossiers avant meme
//   d'afficher la page web -- chaque dossier ne sera scanne (toujours
//   lentement sur les gros dossiers, cout FAT32 intact) qu'au moment ou
//   l'utilisateur clique dessus. PAS ENCORE reteste sur materiel reel.
//
// v32 - 2026-07-27 - safe-modify - Demande utilisateur : afficher un
//   message sur l'ecran DMD pendant le prechauffage des caches SD (v30) --
//   jusqu'ici totalement silencieux visuellement (ecran vierge pendant
//   plusieurs secondes sur une grosse collection, loop() n'ayant pas
//   encore demarre pour rafraichir l'affichage normal). Nouvelle
//   trCachingMsg() (fr/en/es, meme pattern que trConfigPageMsg() etc.) --
//   ligne 1 dessinee une seule fois juste avant l'appel a
//   warmUpGifCaches(). Nouvelle webDmdForceRedraw() (factorisee depuis le
//   bloc d'affichage MODE_CONFIG de loop(), reutilisee telle quelle) --
//   pas appelee directement ici, mais webDmdPause() dessinait deja sa
//   ligne 2 immediatement (mecanisme preexistant pour les progressions
//   d'upload), reutilise par warmUpGifCaches() (web_config.h, meme date)
//   pour afficher "nom_dossier (i/total)" en direct pendant le scan de
//   chaque dossier. Compilation verifiee OK. PAS ENCORE reteste.
//
// v31 - 2026-07-27 - safe-modify - Demande utilisateur : les messages de
//   statut transitoires mirrores sur l'ecran physique du DMD via
//   webDmdPause() (ex: "Mise en cache du contenu, patientez...", "OK",
//   erreurs -- appeles par web_config.h a chaque showMsg() cote page web)
//   restaient affiches indefiniment sur le DMD une fois le process
//   termine, contrairement au popup web qui s'auto-masque deja depuis la
//   v48 (2026-07-26). Nouveaux g_sdOpPersistentSubMsg/Color (message "de
//   fond", pose par triggerWebConfigMode() = IP du DMD) et
//   g_sdOpSubMsgSetAt (horodatage, mis a jour dans webDmdPause()) : en
//   MODE_CONFIG, si g_sdOpSubMsg n'a pas ete mis a jour depuis
//   SD_OP_SUBMSG_EXPIRE_MS (5000ms, meme duree que le cote web) ET differe
//   du message persistant, retour automatique a ce dernier (l'IP). Les
//   ecrans de boot/secours WiFi qui assignent g_sdOpSubMsg directement
//   (hors webDmdPause()) ne sont pas concernes -- g_sdOpSubMsgSetAt reste
//   a 0 dans leur cas, condition d'expiration jamais vraie. Compilation
//   verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v30 - 2026-07-27 - safe-modify - Demande utilisateur : appel de
//   warmUpGifCaches() (nouvelle, web_config.h meme date) juste apres le
//   reboot cible mode config, avant setupWebConfig() -- construit tous les
//   caches SD (dossiers /gifs + contenu de chaque sous-dossier) en une
//   seule fois pendant que le heap est proche de son maximum, au lieu de
//   payer ce cout plus tard sur des requetes web individuelles avec un
//   heap deja entame. Compilation verifiee OK. PAS ENCORE reteste.
//
// v29 - 2026-07-27 - safe-modify - Test reel du prechauffage (v28) :
//   ECHEC -- maxalloc identique avant/apres (18420->18420) et surtout
//   aucun "[WEB] DMD setMainMsg"/"DMD pause" logue pendant les 3s
//   d'attente, preuve que la requete WiFiClient vers nous-memes
//   (WiFi.localIP():80) n'a jamais ete traitee par le WebServer -- l'auto-
//   connexion via l'IP STA propre ne fonctionne visiblement pas de facon
//   fiable sur ce materiel/reseau (isolation client possible sur la box,
//   ou limite du bouclage lwIP/WiFi). Retire integralement (perdait 3s
//   pour rien). Le meme log a revele un fait plus important en comparant
//   les checkpoints : le vrai gros poste mal attribue precedemment
//   ("1ere page ~16 Ko") est en fait setupWebConfig() elle-meme
//   (creation WebServer + ~25 webServer->on() + begin()) : ~13 Ko a elle
//   seule (31732->18420, AVANT toute requete). La vraie 1ere page reelle
//   ne coute plus que ~3 Ko une fois ce poste isole. Nouveau checkpoint
//   ajoute juste apres setupWebConfig() pour confirmer ce chiffre
//   precisement au prochain test. Chaque webServer->on() alloue 2 objets
//   heap (FunctionRequestHandler + Uri clone(), verifie dans le code
//   source de la lib WebServer ESP32 3.3.11) mais ca ne semble pas
//   suffire a expliquer 13 Ko pour ~25 routes (quelques Ko tout au plus,
//   estimation) -- le reste vient probablement de _server.begin() (socket
//   TCP d'ecoute lwIP), cout d'infrastructure difficilement reductible
//   sans toucher a la configuration ESP-IDF sous-jacente (hors de portee
//   d'un sketch Arduino). Compilation verifiee OK. PAS ENCORE reteste.
//
// v28 - 2026-07-27 - safe-modify - Test reel confirme : meme apres
//   l'optimisation du tri (web_config.h v58), le scan lsgifdirs echoue
//   encore (maxalloc trop bas au moment du scan, ~10-14 Ko, et ne recupere
//   pas entre 2 tentatives dans le meme boot). Logs avec checkpoints (v27)
//   avaient confirme un cout FIXE et ponctuel de ~16 Ko sur le tout premier
//   envoi HTTP du WebServer (chaque page suivante ne coute presque plus
//   rien) -- implementation de la piste de "prechauffage" evoquee : sur le
//   chemin g_skipPlaylistForConfig, juste apres setupWebConfig(), le
//   firmware s'envoie une requete HTTP a lui-meme (WiFiClient vers
//   WiFi.localIP():80, GET /) pour declencher ce cout ponctuel PENDANT le
//   boot (quand il reste ~31 Ko disponibles, juste apres WiFi/NTP) plutot
//   qu'au moment ou l'utilisateur ouvre reellement une page et a besoin de
//   heap pour le scan. Bloquant (jusqu'a 3s max), mais deja attendu par
//   l'utilisateur pendant l'ecran "Redemarrage en cours". Compilation
//   verifiee OK. PAS ENCORE reteste sur materiel reel -- a confirmer :
//   maxalloc juste avant lsgifdirs devrait etre nettement plus haut
//   qu'avant (comparer avec "apres initNTP", ~31 Ko, cible).
//
// v27 - 2026-07-27 - safe-modify - Test reel du reboot cible (v25/v26) :
//   "Reprendre DMD" fonctionne desormais (confirme), mais la liste des
//   sous-dossiers a quand meme echoue cette fois (maxalloc=13812 juste
//   avant le scan, contre 18420 lors d'un essai precedent reussi). Fait
//   marquant : meme playlist sautee, maxalloc chute de 49140 (juste apres
//   boot) a 13812 (juste avant lsgifdirs) -- environ 35 Ko perdus SANS
//   jamais ouvrir de GIF, donc une bonne partie de la perte n'est pas liee
//   a la playlist du tout. Ajout de 2 points de mesure supplementaires
//   pour isoler la source : juste apres setupWiFiFromConfig() et juste
//   apres initNTP() -- permettra de savoir si le cout vient de la
//   connexion WiFi elle-meme ou des allers-retours de pages web (3
//   DMD pause/setMainMsg observes dans le log avant le scan, voir aussi
//   web_config.h meme date pour le point de mesure cote page web).
//   Compilation verifiee OK. PAS ENCORE reteste.
//
// v26 - 2026-07-27 - safe-modify - Bug remonte suite au reboot cible mode
//   config (v25) : "Reprendre DMD" affichait un ecran vide. Cause :
//   resumePlaylist() ne relance l'affichage que si gifCount>0, or gifCount
//   n'est jamais initialise sur le chemin g_skipPlaylistForConfig (aucun
//   chargement de playlist). Fix : ce chemin charge maintenant l'index
//   playlist existant (gifCount, via le fichier .idx deja construit -- pas
//   de rebuildPlaylistCache(), pas de showPlaylistInfoScreen(), pas
//   d'openNextGif()) si la signature du cache est encore valide -- cout
//   heap negligeable (~7ms mesures en conditions reelles pour cette seule
//   etape). "Reprendre DMD" doit desormais fonctionner normalement. Limite
//   acceptee : si le cache playlist est perime a ce moment precis (rare),
//   gifCount reste a 0 pour ce boot -- necessiterait un vrai reboot pour
//   se reconstruire, comme avant l'existence de ce chemin. Compilation
//   verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v25 - 2026-07-27 - safe-modify - Demande utilisateur : plutot que de
//   continuer a chercher la fuite heap par-GIF (v23/v24, pas encore
//   localisee dans la lib AnimatedGIF/SD), reboot cible en mode config des
//   que la playlist a deja tourne ce boot -- voir web_config.h v55 pour le
//   declenchement cote serveur web. Nouveaux globals : g_skipPlaylistForConfig
//   (lu depuis config.ini "force_config_boot", consomme/remis a "0" des sa
//   lecture dans setup() pour ne jamais boucler), g_playlistStartedThisBoot
//   (mis a true au tout premier openNextGif() du boot, expose a
//   web_config.h). Nouveau garde dans setup() (meme pattern que
//   g_forceApRecovery) : si g_skipPlaylistForConfig, saute directement en
//   MODE_CONFIG (message "page de configuration" + IP) sans jamais lancer
//   playlist/GIF -- le heap reste alors pres de son maximum post-boot
//   (~49 Ko au lieu de ~13 Ko mesures en conditions reelles apres
//   playlist+plusieurs GIFs). Compilation verifiee OK. PAS ENCORE reteste
//   sur materiel reel -- a valider : ouverture config web depuis une
//   session playlist normale doit maintenant rebooter puis afficher la
//   page demandee automatiquement une fois revenu ; un reboot manuel
//   ("Redemarrer") depuis la page config doit lui repartir en boot
//   playlist normal (pas de boucle sur ce nouveau chemin).
//
// v24 - 2026-07-27 - safe-modify - Suite de l'investigation v23 : log reel
//   fourni confirme que le total libre chute AUSSI (pas seulement maxalloc)
//   a chaque GIF ouvert (~4200 octets/ouverture, jamais recupere) --
//   signature d'une vraie fuite, pas juste de la fragmentation. Verifie
//   dans le code source de la lib AnimatedGIF (D:\...\libraries\
//   AnimatedGIF\src\) : pFrameBuffer (alloue par allocFrameBuf(), libere
//   par freeFrameBuf() -- mais close() n'appelle PAS freeFrameBuf()) n'est
//   utilise qu'en mode dessin GIF_DRAW_COOKED ; notre code reste en
//   GIF_DRAW_RAW (jamais setDrawType()/allocFrameBuf() appeles), donc ce
//   pointeur reste toujours NULL -- PAS notre fuite. gif.inl ne contient
//   aucun malloc/calloc (LZW decode sur buffers internes fixes). Nos
//   callbacks SD (GIFOpenFile/GIFReadFile/GIFSeekFile/GIFCloseFile) sont
//   legers, rien d'evident. Piste restante : classe File (SD/FS ESP32) ou
//   pipeline de dessin (GIFDraw()/HUB75), pas encore isole. Ajout d'un
//   nouveau point de mesure dans openGif() juste apres gif.open() reussi
//   (AVANT toute frame dessinee) pour savoir si la perte a deja eu lieu a
//   l'ouverture ou seulement pendant la lecture des frames qui suit.
//   Compilation verifiee OK. PAS ENCORE reteste (log a fournir au prochain
//   boot).
//
// v23 - 2026-07-26 - safe-modify - Investigation heap critique (voir
//   web_config.h v54) : log reel fourni par l'utilisateur montre
//   maxalloc=8692 des l'ouverture de la page web config, PUIS 4596 des le
//   debut du scan lsgifdirs -- MAIS ce log est un boot A FROID
//   (POWERON_RESET), pas une session longue comme suppose precedemment
//   (aucune activite prolongee avant, juste boot -> WiFi -> playlist ->
//   quelques rotations GIF -> ouverture web config). Le heap est donc deja
//   critique tres tot, pas seulement apres usage prolonge. Ajout de 3
//   points de mesure ESP.getMaxAllocHeap()/getFreeHeap() dans setup() pour
//   isoler QUELLE etape fait chuter maxalloc : juste apres le chargement
//   des caches (systemes/games), juste apres showPlaylistInfoScreen(), et
//   juste apres le tout premier openNextGif(). Objectif : comparer ces 3
//   valeurs avec le maxalloc=8692 deja observe a l'ouverture web config
//   pour localiser la source (chargement caches boot / lecture playlist /
//   decodage GIF) avant de tenter un correctif. Aucun changement de
//   comportement, uniquement des logs. Compilation verifiee OK. PAS ENCORE
//   reteste (log a fournir au prochain boot).
//
// v22 - 2026-07-26 - safe-modify - Investigation "MQTT connecting/failed
//   rc=-2 pendant une session web config" (log reel fourni par
//   l'utilisateur). Ajout mineur : msg.reserve(length+1) dans
//   onMqttMessage() avant la concatenation octet-par-octet (optimisation
//   generale, ce handler tournant pour chaque message MQTT recu) --
//   l'utilisateur a ensuite precise que dans le test en cause la Recalbox
//   etait eteinte, donc aucun message MQTT recu (seules des tentatives de
//   connexion echouees) : ce fix ne concerne pas cet evenement precis.
//   Vraie explication trouvee en relisant le log ligne a ligne : la tentative
//   "[MQTT] connecting" survient ~0.6s APRES un "[WEB] DMD resume" (donc
//   g_sdOpInProgress=false, comportement attendu du garde de mqttTask() --
//   pas un bug), puis ~1.4s plus tard un "[WEB] DMD pause: DMD repris"
//   remet g_sdOpInProgress a true : c'est le message de confirmation
//   "Reprendre DMD" qui se re-mirroitait lui-meme vers le DMD via
//   showMsg()/webDmdPause(), deja identifie et corrige cote web_config.h
//   (v47, fonction showMsgLocal() sans mirroir DMD) -- mais ce test reel
//   avait manifestement ete flashe AVANT ce correctif. mqttTask() n'a donc
//   pas de bug distinct : son garde g_sdOpInProgress fonctionne comme prevu,
//   c'est la fenetre reelle (mais tres breve, ~1.4s) entre un vrai resume et
//   son propre re-pause errone qui laissait passer une tentative de
//   connexion. Aucun changement de code necessaire ici au-dela du
//   reserve() ci-dessus -- reflasher avec le firmware incluant web_config.h
//   v47+ et reesayer le meme scenario pour confirmer. Compilation verifiee
//   OK. PAS ENCORE reteste sur materiel reel avec le firmware a jour.
//
// v21 - 2026-07-26 - safe-modify - Demande utilisateur : version affichee au
//   splash boot (RETRO_VERSION, ecran physique du DMD, info=1 uniquement)
//   passee de "Raw565 Ed. v10" a "Raw565 Ed. v11". N'a aucun rapport avec
//   le numero de version safe-modify de ce fichier (historique interne des
//   commits/patches, actuellement v21) -- deux compteurs distincts,
//   confirme volontairement inchange lors d'une precedente session.
//
// v20 - 2026-07-26 - safe-modify - Retire #include <esp_task_wdt.h> (v19) :
//   les esp_task_wdt_reset() ajoutes dans web_config.h echouaient en boucle
//   en test reel ("task not found" -- la tache HTTP n'est pas enregistree
//   aupres du Task Watchdog Timer), sans aucun benefice et avec un vrai
//   cout (log d'erreur repete). Tous retires cote web_config.h (v44),
//   include devenu inutile ici. Compilation verifiee OK.
//
// v19 - 2026-07-26 - safe-modify - Ajout #include <esp_task_wdt.h> : reboot
//   du DMD confirme en test reel a la premiere tentative de copie de
//   fichier (upload web vers un nouveau dossier /gifs). Piste la plus
//   probable : Task Watchdog de loopTask (~5s par defaut) declenche par un
//   premier mkdir()/ecriture SD anormalement lent. Voir web_config.h v40
//   pour les esp_task_wdt_reset() ajoutes dans handleWebConfigCreateFolder()
//   et le mkdir de secours d'UPLOAD_FILE_START. Compilation verifiee OK.
//   PAS ENCORE reteste sur materiel reel.
//
// v18 - 2026-07-26 - safe-modify - Demande utilisateur : serveur telnet de
//   debug (port 23) retire completement -- plus utilise, gagne RAM et CPU.
//   Supprime : bloc de fonctions telnetWrite/telnetWriteln/telnetPrompt/
//   stopTelnetClient/startTelnetServer/stopTelnetServer/printTelnetWifiInfo/
//   handleTelnetCommand/handleTelnetLineSubmit/handleTelnet (~250 lignes,
//   console de commandes debug : help/ip/wifi/wifiinfo/next/count/playlist/
//   random/reboot/heap/mode/mqttlog/syscache/rebuildcache/show/showsys/
//   showgame/exists/ls/default/black/resumesys) ; globals telnetServer/
//   telnetClient/telnetServerStarted/telnetClientActive/telnetLine/
//   telnetLastWasCR ; #define TELNET_PORT ; tous les appels handleTelnet()/
//   startTelnetServer()/stopTelnetServer() (loop(), maintainWiFi(),
//   setupWiFiFromConfig(), et les boucles d'attente MODE_GIF/MODE_PNG/
//   MODE_BLACK). Compilation verifiee OK (62% flash, -17 Ko de flash vs
//   v17). PAS ENCORE reteste sur materiel reel.
//
// v17 - 2026-07-23 - safe-modify - Bug confirme sur test reel du v16 :
//   ecran DMD noir/vide apres connexion MQTT ("page vide sur le DMD").
//   Cause racine trouvee : case MODE_PNG de loop() efface l'ecran et
//   repasse en MODE_BLACK des que currentPngPath est vide (comportement
//   preexistant, ~ligne 3970) -- mon CMD_WAITING_MQTT (v14) mettait
//   currentPngPath="" (copie du pattern openBestMedia(), qui a
//   probablement le meme defaut latent, non touche ici -- hors demande)
//   avant de definir currentMode=MODE_PNG, provoquant un auto-clear QUASI
//   INSTANTANE a la frame suivante : l'image de secours n'etait visible
//   qu'une fraction de frame, invisible en pratique. Fix : currentPngPath
//   mis a DEFAULT_RAW565_PATH (non-vide, jamais relu puisque pngDrawn=true
//   saute la logique de redessin) pour eviter ce garde. Ajout de logs de
//   diagnostic (ensureDefaultRaw565Cached() etait entierement silencieuse
//   sur ses 3 chemins d'echec ; nouveau print de resultat dans
//   CMD_WAITING_MQTT) pour eviter de futurs diagnostics a l'aveugle sur
//   ce chemin. Compilation verifiee OK (62% flash). PAS ENCORE reteste
//   sur materiel reel.
//
// v16 - 2026-07-23 - safe-modify - Bug confirme sur test reel du v15 (log
//   serie) : la fenetre de grace ne s'appliquait toujours pas -- cause
//   racine reelle trouvee : le garde currentMode!=MODE_PLAYLIST (copie de
//   la logique CMD_DEFAULT) empechait CMD_WAITING_MQTT/g_mqttWaitingUntilMs
//   d'etre poses des que la playlist avait deja demarre pendant les 12s de
//   MQTT_START_DELAY_MS -- ce qui est le cas NORMAL (le log montrait des
//   GIFs deja en lecture avant meme "[MQTT] connecting"). Fix : garde
//   retire pour CMD_WAITING_MQTT specifiquement (reste present pour
//   CMD_DEFAULT, comportement inchange). Compilation verifiee OK (62%
//   flash). PAS ENCORE reteste sur materiel reel.
//
// v15 - 2026-07-23 - safe-modify - Bug confirme sur test reel du v14 (log
//   serie) : CMD_WAITING_MQTT n'etait jamais visible -- "[MQTT] connected"
//   est immediatement suivi de "marquee/cmd/default -> 1" puis
//   "marquee/cmd/system -> lastplayed" dans le MEME cycle, car ce sont des
//   messages RETENUS (mosquitto -r) rejoues par le broker des la
//   souscription (pas de nouveaux evenements RB) -- ils ecrasaient
//   pendingCmd avant meme que l'image de secours soit rendue. Fix :
//   nouvelle fenetre de grace g_mqttWaitingUntilMs (MQTT_WAITING_GRACE_MS
//   = 1500ms, posee au moment ou CMD_WAITING_MQTT est emis) -- pendant
//   cette fenetre, onMqttMessage() ignore default/system/game (stop/
//   show_config/wifi_recovery/reboot restent des actions explicites,
//   jamais supprimees). Un vrai message d'evenement RB (le pont marquee
//   republie "system" ~5s apres son propre demarrage, largement apres
//   cette fenetre de 1.5s) n'est jamais bloque. Compilation verifiee OK
//   (62% flash). PAS ENCORE reteste sur materiel reel.
//
// v14 - 2026-07-23 - safe-modify - Demande utilisateur : au moment ou la
//   connexion MQTT a la Recalbox vient d'aboutir, afficher l'image de
//   secours statique (RAM default.raw565, drawDefaultRaw565Cached()) au
//   lieu de relancer directement la playlist en rotation -- le temps que
//   la Recalbox envoie un premier vrai message system/game. Nouveau
//   MqttCommand::CMD_WAITING_MQTT (distinct de CMD_DEFAULT, qui reste
//   utilise tel quel par le pont marquee sur stop/sleep -- relance bien la
//   playlist dans ce cas, comportement inchange). mqttTask() : au succes
//   de mqttClient.connect(), emet CMD_WAITING_MQTT au lieu de CMD_DEFAULT
//   (condition gifCount>0 retiree : l'image de secours ne depend pas du
//   contenu de la playlist). Compilation verifiee OK (62% flash). PAS
//   ENCORE teste sur materiel reel.
//
// v13 - 2026-07-21 - DMD multilingue (demande utilisateur) : nouveau global
//   uiLanguage (fr/en/es) + cle config.ini "language=" lue dans loadConfig().
//   7 helpers de traduction (trOpenBrowserAt/trWifiRecoveryCountdown/
//   trConnectWifiMsg/trOpenUrl/trConfigPageMsg/trJoinWifi/trOpenInBrowser)
//   couvrant les bannieres informatives ecran (CMD_SHOW_CONFIG, decompte +
//   ligne 2 SSID/IP du mode secours WiFi, ecrans "connectez-vous au WiFi"/
//   "page de configuration") -- les libelles techniques courts (WIFI OK,
//   NTP, BT ON/OFF, brightness%, splash boot) restent volontairement non
//   traduits (deja compacts/universels). Alternance SSID/IP du mode secours
//   passee de 2s a 6s (chaines plus longues avec le prefixe demande,
//   laisser le defilement horizontal avancer). Compile OK (arduino-cli via
//   compile.ps1, 1976449 octets/62% flash) -- PAS ENCORE flashe/teste sur
//   le DMD reel au moment de ce commentaire.
//
// v12 - 2026-07-21 - [Session parallele, suite du v11 -- les 3 scripts
//   utilisateur (Config Web DMD, WiFi Recovery DMD, Reboot DMD) sont
//   confirmes fonctionnels sur materiel reel par l'utilisateur apres flash]
//   1) Auto-detection mDNS de l'IP Recalbox : nouvelle fonction
//   autoDetectRecalboxIP() (#include <ESPmDNS.h>, requete MDNS.queryHost
//   ("recalbox")), appelee juste apres une connexion WiFi STA reussie dans
//   setupWiFiFromConfig() -- couvre a la fois le boot normal ET le retour
//   STA apres un mode secours WiFi, sans code specifique dans la page AP
//   (qui n'a pas acces au reseau domestique pendant qu'elle tourne). Ecrit
//   via writeConfigFlag() si recalbox_ip est vide ; n'ecrase jamais une
//   valeur deja renseignee manuellement. Objectif : MQTT disponible sans
//   ressaisie manuelle apres un passage par le mode secours/AP.
//   2) Refonte affichage ecran secours WiFi : ligne 1 = decompte
//   ("Secours WiFi XXXs", mise a jour chaque seconde) au lieu de la ligne 2
//   comme avant ; ligne 2 = alterne SSID ("RecalBox-DMD-Config") et IP
//   ("http://<ip AP>") toutes les 2s -- les deux tiennent seuls sous 128px
//   (pas de defilement horizontal lent a attendre pour lire l'info
//   complete). apRecoveryIP capture via WiFi.softAPIP() a l'entree en mode
//   secours (avec repli "192.168.4.1" si 0.0.0.0).
//   3) Fix cosmetique : "[WEB] Interface config sur http://0.0.0.0" dans
//   web_config.h (startWebServer) -- utilisait WiFi.localIP() (STA uniquement,
//   retourne 0.0.0.0 hors STA) au lieu du meme repli localIP->softAPIP->
//   "192.168.4.1" deja utilise ailleurs dans setup() pour ce cas.
//   Compile OK (arduino-cli, 1971569 octets/62% flash, 102440 octets/31%
//   RAM) -- PAS ENCORE teste sur le DMD reel au moment de ce commentaire.
//
// v11 - 2026-07-21 - [Session parallele -- au-dessus des correctifs "Reprendre
//   DMD sans reboot"/gzip notes dans le bloc v10 ci-dessous, ecrits par une
//   autre session Claude Code le meme jour] Portage cible (pas de copie de
//   fichier complet) depuis _wip_clock_themes/RecalBox_DMD_dev/, pour les 2
//   scripts utilisateur Recalbox installes via l'outil Windows (Mode 9,
//   RecalBoxDMD_tool.py) : ils publiaient/s'abonnaient sur des topics MQTT
//   (marquee/cmd/show_config, marquee/cmd/wifi_recovery, marquee/status/ip)
//   qui n'existaient QUE dans la copie dev, jamais fusionnes -- confirme sur
//   materiel reel (script "Config Web DMD" : mosquitto_pub reussit mais le
//   DMD ignore la commande, aucun effet). Ajouts : enum MqttCommand::
//   CMD_SHOW_CONFIG/CMD_WIFI_RECOVERY, abonnements aux 2 nouveaux topics,
//   publish retenu (retain=true) de marquee/status/ip a chaque connexion MQTT
//   (le script Recalbox le lit via mosquitto_sub -C 1 pour recuperer l'IP
//   sans etre connecte au moment exact du publish), writeConfigFlag()
//   (nouveau helper generique cle=valeur dans config.ini, meme pattern que
//   clearFirstBoot()), flag config.ini force_ap_recovery + g_forceApRecovery,
//   et le sous-systeme complet de secours WiFi (maintainApRecovery(), appele
//   depuis loop()) : CMD_WIFI_RECOVERY redemarre en WIFI_AP PUR (jamais
//   WIFI_AP_STA -- rejete precedemment sur ce materiel, crash heap + debit
//   radio casse, voir memoire projet) avec un compte a rebours de 3 min
//   affiche sur l'ecran DMD avant retour automatique en STA normal.
//   CMD_SHOW_CONFIG reutilise webDmdSetMainMsg()/webDmdPause() (deja
//   existants, section web config) pour afficher l'IP du DMD sur l'ecran LED
//   sans toucher au WiFi ; message ligne 2 elargi a "Ouvrez un navigateur sur
//   http://<IP>" (defile automatiquement, mecanisme deja existant) suite a
//   un retour utilisateur en test reel (l'IP seule manquait de contexte).
//   TESTE SUR MATERIEL REEL (2026-07-21) : CMD_SHOW_CONFIG fonctionnel des
//   le 1er flash. CMD_WIFI_RECOVERY avait un bug reel (absent de la copie
//   dev aussi, jamais teste avant) : le reboot en AP secours fonctionnait
//   (SSID RecalBox-DMD-Config visible, page config accessible/fonctionnelle,
//   confirme par log serie "[WIFI] force_ap_recovery actif -> AP secours
//   pur"), MAIS setup() continuait tout droit dans le chargement/affichage
//   de playlist (showPlaylistInfoScreen()/openNextGif(), qui remettait
//   currentMode=MODE_PLAYLIST) au lieu de sauter cette etape -- l'ecran DMD
//   restait donc en mode playlist normal au lieu d'afficher "Mode secours
//   WiFi"/le compte a rebours. Fix : nouveau garde-fou
//   "if(g_forceApRecovery){goto start_mqtt_task;}" dans setup(), au meme
//   endroit que les gardes existants pour g_firstBoot/playlist vide. Meme
//   fix applique dans _wip_clock_themes/RecalBox_DMD_dev/ (bug identique,
//   jamais teste non plus la-bas). Compile OK (arduino-cli, 1937697
//   octets/61% flash, 99968 octets/30% RAM) -- ce fix precis PAS ENCORE
//   reteste sur le DMD reel au moment de ce commentaire.
//   Ajout demande par l'utilisateur : 3e script "Reboot DMD"
//   (scripts/manual/Reboot DMD.sh, route userscripts/manual) -> nouvelle
//   commande MQTT marquee/cmd/reboot / MqttCommand::CMD_REBOOT, redemarrage
//   simple SANS aucune condition de garde (contrairement aux autres
//   commandes qui s'auto-ignorent si g_sdOpInProgress) -- bouton de secours
//   manuel en cas d'affichage fige. Limite connue et volontairement non
//   contournee : ne fonctionne que si la tache MQTT/loop() du DMD repond
//   encore (un blocage complet -- boucle infinie, tache plantee -- empeche
//   par definition de recevoir/traiter cette commande ; seul un
//   debranchement physique ou le watchdog materiel peuvent recuperer ce
//   cas-la). Compile OK (arduino-cli, 1937893 octets/61% flash, 99968
//   octets/30% RAM) -- PAS ENCORE teste sur le DMD reel.
//
// === Compilation / Flash ===
// Compilation (arduino-cli) :
//   arduino-cli compile --clean --fqbn esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=40,FlashMode=qio,FlashSize=4M,PartitionScheme=huge_app,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none --output-dir "compiled" "RecalBox_DMD.ino"
//
// Fusion merged.bin (esptool 5.3.0) :
//   esptool.exe --chip esp32 merge_bin --output "compiled/RecalBox_DMD.ino.merged.bin" --flash-mode keep --flash-freq keep --flash-size 4MB 0x1000 "compiled/RecalBox_DMD.ino.bootloader.bin" 0x8000 "compiled/RecalBox_DMD.ino.partitions.bin" 0xe000 "<ESP32_PATH>/tools/partitions/boot_app0.bin" 0x10000 "compiled/RecalBox_DMD.ino.bin"
//
// Flash merged.bin (carte vierge / premier flash) :
//   esptool.exe --chip esp32 --port COM4 --baud 921600 --before default-reset --after hard-reset write_flash -z --flash-mode keep --flash-freq keep --flash-size keep 0x0 "compiled/RecalBox_DMD.ino.merged.bin"
//
// Flash app seul (dev iteratif, preserve NVS/WiFi) :
//   esptool.exe --chip esp32 --port COM4 --baud 921600 --before default-reset --after hard-reset write_flash -z --flash-mode keep --flash-freq keep --flash-size keep 0x10000 "compiled/RecalBox_DMD.ino.bin"
//
//
// v10 - 2026-07-14 - Integration horloge 10 themes (etait 9): nouveau theme "Level 1-1" (scrolling
//   complet du niveau 1-1 sans personnage ni parallaxe, jusqu'au drapeau du chateau, boucle
//   seamless). Super Mario repris de zero (personnage de profil, decor parallaxe 3 vitesses,
//   vrais tuyaux, Goomba au sol). Pac-Man: dots geres par etat individuel par pastille
//   (dotEaten[60]) au lieu d'un span a 2 index - elimine le bloc de dots qui apparaissait
//   d'un coup au retour du mode panic; ordre des fantomes clampe (jamais devant en poursuite,
//   jamais derriere en panic). Space Invaders repris (reference Deluxe Space Invaders 1979).
//   Tetris: horloge recentree dans son cadre. Theme Neon: couleur personnalisable unique
//   (CLOCK_COLOR, remplace les 2 couleurs fixes CLOCK_NEON_COLOR1/CLOCK_NEON_COLOR2) ->
//   clockNeonColor1/2 (uint32_t) remplaces par clockNeonCustomColor (bool) + clockNeonR/G/B
//   (uint8_t). drawRetroClockTheme() recoit desormais millis()-themeStartMs (temps depuis le
//   debut d'affichage DU theme, pas l'uptime global) pour permettre une sequence d'ouverture
//   jouee une seule fois par affichage. Travail prepare et verifie dans _wip_clock_themes/
//   avant fusion (voir clock_themes.h et web_config.h, memes dates).
//   Note: cette fusion (basee sur un snapshot du fichier anterieur a la session v9-boot-
//   silencieux) a ecrase 2 fixes: RETRO_VERSION reste a "v9" (corrige -> v10 ici), et le
//   showSplashScreen()/bloc setup() remettaient un clearScreen() qui effacait le titre avant
//   le sablier (re-corrige: titre persistant, sablier par-dessus, cf bootHourglassTick()).
//   Note (2026-07-21) : webDmdResume() ne fait plus ESP.restart() -- "Reprendre DMD" quitte
//   desormais le mode config sans reboot (g_sdOpInProgress=false + resumePlaylist(), meme
//   mecanisme que la commande MQTT CMD_DEFAULT). Cote page web (web_config.h, meme date) :
//   confirmation ajoutee sur "Reprendre DMD" et "Redemarrer" quand des reglages ont ete
//   modifies sans etre sauvegardes. Fusionne depuis _wip_clock_themes/RecalBox_DMD_dev/ et
//   valide en conditions reelles (sauvegarde + reprise d'activite MQTT confirmees par
//   l'utilisateur). Applique en patch cible (pas de copie de fichier complet) pour ne pas
//   ecraser le travail en cours non fusionne sur ce fichier (WiFi 3-tentatives/IP statique/
//   theme horloge) -- cf diff non commite au moment de cette fusion.
// v9 - 2026-07-13 - Boot silencieux (info=0) revu: masque brightness/wifi/ntp (initNTP() gate
//   showInfo), ne montre que le splash (titre) + sablier anime coin haut-droit
//   (bootHourglassTick(), tick pendant l'attente WiFi et NTP). Web config: dropdown
//   fuseau horaire (pays + UTC-5..+5) remplace le champ texte POSIX, nouvelle case
//   "Demarrage silencieux" (inverse de info=) dans la section Affichage.
// v8 - 2026-07-11 - Fix crash/reboot loop apres flash du merged.bin: ajout nvs_flash_init()
//   (+ erase de secours) en tout debut de setup(). Cause: le merge esptool comble le trou
//   NVS (0x9000-0xE000, entre partitions.bin et boot_app0.bin) avec du 0xFF brut ecrase a
//   chaque flash. Sans init explicite, esp_wifi echoue a se connecter puis abort() dans
//   lock_init_generic (heap bas) lors du fallback WiFi.mode(WIFI_AP). FlashFreq 80->40
//   (fixait un 1er crash au boot, invalid segment length). --clean ajoute a arduino-cli.
//   Suite: 2e crash decouvert apres le fix NVS (ESP_ERR_NO_MEM dans esp_timer_create,
//   pendant wifi_sta_connect_internal/ppTask) -> setupBluetoothFromConfig() (qui libere
//   ~60 Ko via esp_bt_mem_release si BT desactive) appelee AVANT setupWiFiFromConfig()
//   au lieu d'apres, pour liberer ce heap avant que le WiFi en ait besoin.
// v7 - 2026-07-01 - Integration retro_clock: 9 themes pixel-art (Mario, Tetris, Pac-Man, Invaders, Pong, Neon, Matrix, Fire, Rainbow). CLOCK_STYLE -> CLOCK_THEME. Suppression anciens drawDigit*.
// v6 - 2026-06-29 - Ajout horloge multi-style + brightness configurable
// v5 - 2026-06-29 - Correction freeze playlist: skipRawPack dans openGif()
// v4 — 2026-06-26 — Correction flag B: verifie currentPngPath AVANT d'appeler openDG(), sinon a chaque loop() le firmware tente d'ouvrir le pack manquant et clignote. Flag 'g' aussi corrige (meme probleme).
// v3 — 2026-06-24 — Ajout sous-dossiers alphabetiques A..Z/# pour resoudre ralentissement FAT32 sur 800+ fichiers (flag L). alphaSubdirPath() insere un sous-dossier dans le chemin. drawRaw565() et openGif() tentent le sous-dossier en priorite avec fallback plat.
// v2 — 2026-06-11 — safe-modify — Optimisations affichage raw565/raw565pack
// v1 — 2026-06-10 — Creation initiale
// ============================================



#ifndef BitOrder
typedef uint8_t BitOrder; // Workaround: Adafruit_BusIO attend BitOrder (AVR) mais ESP32 le n'a pas
#endif

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// v114 -- police compacte TomThumb (3x5px, fournie par Adafruit_GFX_Library,
// deja presente dans ce projet) -- demande utilisateur explicite : "reduire
// la taille des caracteres du contenu de descriptions pour afficher plus de
// texte par ligne". Utilisee UNIQUEMENT pour l'ecran DESCRIPTION (voir
// drawScoreScreen()) -- ~4px/caractere au lieu de 6px avec la police
// classique, soit ~30 caracteres/ligne au lieu de ~21.
// v117 -- Org_01 essayee a la place (retour utilisateur "TomThumb un peu
// dure a lire") -- v118 -- Org_01 RETIRE (retour utilisateur : son
// caractere espace est trop large, rompt avec le style general des autres
// ecrans) -- retour DEFINITIF a TomThumb, avec espacement inter-caractere
// manuel + rendu MAJUSCULES pour ameliorer sa lisibilite (voir
// gfxCharAdvance()/drawScoreScreen() plus bas).
#include <Fonts/TomThumb.h>
#include <AnimatedGIF.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h> // v1 (piste UDP, voir TRANSPORT_PLAN_UDP.md) -- prototype minimal, cmd=score uniquement
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "esp_heap_caps.h"
#include "pngle.h"
#include <time.h>
#include "hal/brownout_ll.h"
#include "esp_system.h" // v76 -- esp_reset_reason(), diagnostic crash/brownout au boot
#include "esp_log.h" // v99 -- esp_log_level_set(), voir setup() pour le detail complet
#include "nvs_flash.h"
#include "ff.h" // Partie C (plan cache_master_gifs) -- f_getlabel()/f_setlabel(), renommage etiquette volume SD au boot
#include "clock_themes.h"
#include <lwip/sockets.h> // v139 -- select()/send() bas niveau pour mqttSubscribeFast()
#include <errno.h> // v139 -- EAGAIN/EWOULDBLOCK

// Declarations anticipees: web_config.h (inclus juste apres) utilise ces
// symboles avant leur definition/textuelle plus bas dans ce .ino -- l'auto-
// prototypage Arduino ne couvre pas les macros, et pas de facon fiable les
// fonctions referencees depuis un header inclus avant leur definition.
#define MQTT_PORT 1883
bool parseIP(const String &s, IPAddress &ip);
bool applyStaticIP();
void writeConfigFlag(const String &key, const String &value);

// Generation de playlist -- machine a etats a pas bornes (playlistGenStep(),
// definie dans web_config.h), appelee depuis loop() a chaque iteration
// (2026-08-10, RETOUR a cette architecture -- voir changelog v67 complet en
// tete de fichier). ANCIENNEMENT une tache FreeRTOS dediee (playlistGenTask(),
// introduite le 2026-07-30, "v95" de l'historique projet) protegee par 2
// mutex (sdAccessMutex partage avec gifPlayFrameCompat()/openNextGif(),
// plGenStatusMutex pour ce statut) -- cette tache (et sdAccessMutex, pris a
// CHAQUE frame affichee meme hors generation) a ete identifiee par
// bissection sur materiel reel comme le point de bascule d'un deadlock
// mqttTask/LWIP touchant le fonctionnement NORMAL (MQTT + affichage GIF
// continu, mecanisme exact non elucide malgre investigation poussee -- voir
// memoire projet). playlistGenStep() tourne desormais exclusivement dans
// loop(), meme contexte d'execution que gifPlayFrameCompat()/openNextGif()/
// les handlers HTTP -- plus aucun acces SD concurrent entre 2 threads,
// aucun mutex necessaire. Priorite utilisateur explicite : fiabilite
// MQTT/affichage (coeur du projet) avant confort de generation de playlist
// (bonus, potentiellement un peu moins fluide pendant une generation active
// -- compromis assume).
struct PlaylistGenStatus
{
  bool   active = false;
  bool   done = false;
  String curDirName;
  int    dirIdx = 0;
  int    totalDirs = 0;
  int    totalGifs = 0;
  int    curDirGifs = 0;
  String resultMsg;
  bool   stopRequested = false;
};
PlaylistGenStatus g_plGenStatus;

#include "web_config.h"

// --------------------------------------------------
// Cache des _defaults par systeme
// --------------------------------------------------
#define SYS_CACHE_MAX 300
static char (*sysCacheKeys)[32] = nullptr; // SYS_CACHE_MAX x 32 (heap)
static char *sysCacheVals = nullptr;       // SYS_CACHE_MAX (heap)
static char *sysCacheSlowVals = nullptr;   // SYS_CACHE_MAX (heap)
static int  sysCacheCount = 0;
// v43 -- chargement paresseux (demande explicite utilisateur, 2026-08-03) :
// sur un boot "reboot cible mode config" (g_skipPlaylistForConfig), ce
// cache est deliberement saute au demarrage (voir setup()) car inutile
// pendant la copie -- mais si l'utilisateur clique "Reprendre DMD" ensuite
// et que MQTT se reconnecte reellement, sysDefaultType()/sysDefaultSlowFlag()
// ont quand meme besoin d'un cache valide pour les icones systeme/jeu.
// sysCacheLoadAttempted distingue "jamais tente" (charger a la demande, une
// seule fois) de "deja tente, cache vide car fichier absent" (ne pas
// retenter a chaque appel -- couteux, appele tres frequemment). Sur un boot
// normal, mis a true juste apres le chargement eager habituel dans setup().
static bool sysCacheLoadAttempted = false;

static void ensureSysDefaultCacheLoaded()
{
  if (sysCacheLoadAttempted) return;
  sysCacheLoadAttempted = true;
  // Uniquement loadSysDefaultCache() (lecture rapide du .dat existant) --
  // JAMAIS buildSysDefaultCache() ici (scan recursif complet, potentiellement
  // long) : ce chemin peut etre atteint en plein traitement d'un evenement
  // MQTT temps reel, un scan long y serait inapproprie. Le .dat existe deja
  // forcement si ce boot fait suite a un boot normal anterieur (seul cas
  // realiste pour atteindre ce chemin).
  if (loadSysDefaultCache())
    Serial.println("[CACHE] charge a la demande (post-copie): " + String(sysCacheCount) + " systemes");
  else
    Serial.println("[CACHE] charge a la demande: /systems_cache.dat absent");
}

char sysDefaultType(const String &sysName)
{
  ensureSysDefaultCacheLoaded();
  for (int i = 0; i < sysCacheCount; i++)
    if (sysName == sysCacheKeys[i]) return sysCacheVals[i];
  return '?';
}

char sysDefaultSlowFlag(const String &sysName)
{
  ensureSysDefaultCacheLoaded();
  for (int i = 0; i < sysCacheCount; i++)
    if (sysName == sysCacheKeys[i]) return sysCacheSlowVals[i];
  return 'N';
}

// Flag "lent" (L) par sous-dossier alphabetique (bucket), voir plan
// "flag L par bucket alphabetique" (chantier "bucket", portage depuis le
// worktree dev-slow-flag-per-bucket, v37/v29) -- complement de
// sysCacheSlowVals (flag agrege par systeme, inchange, conserve comme
// repli). Chaque caractere vaut 'L'/'N' (donnee reelle) ou '?' (pas de
// donnee pour ce bucket -- ancien systems_cache.dat a 3 champs, ou 4e
// champ absent/invalide pour cette ligne -- repli automatique sur
// sysCacheSlowVals[i] dans sysBucketSlowFlag()). Ordre des colonnes =
// BUCKET_LETTERS.
#define BUCKET_COUNT 27
// v137 -- plafond DEDIE, plus petit que SYS_CACHE_MAX (voir changelog
// v137 ci-dessus) : sysCachePerLetterVals ne sert que le detail optionnel
// par bucket, avec repli deja existant sur le flag agrege par systeme
// pour tout indice i >= BUCKET_CACHE_MAX. 96 reste tres large pour le
// nombre reel de systemes RecalBox.
#define BUCKET_CACHE_MAX 96
static const char BUCKET_LETTERS[] = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // doit rester synchro avec LETTERS (RecalBoxDMD_tool.py)
static char (*sysCachePerLetterVals)[BUCKET_COUNT] = nullptr; // BUCKET_CACHE_MAX x 27 (heap)

// 1ere lettre du nom de fichier (avec ou sans extension, seul le 1er
// caractere compte), majuscule, '#' si non-alpha/vide -- factorise la
// regle deja presente dans alphaSubdirPath() (voir plus bas), reutilisee
// ici pour deriver le bucket d'un jeu/fichier au moment de decider
// isSlow. Meme regle que _bucket_letter_for_stem() cote outil PC.
static char bucketLetterForFilename(const String &fname)
{
  if (fname.length() == 0) return '#';
  char first = (char)toupper((unsigned char)fname.charAt(0));
  return isAlpha(first) ? first : '#';
}

// Flag "lent" par bucket, avec repli sur le flag systeme agrege
// (sysCacheSlowVals) si la donnee par bucket est absente (ancien cache,
// 4e champ manquant/invalide) ou si le systeme est inconnu au niveau
// bucket. sysDefaultSlowFlag() reste utilisee telle quelle ailleurs
// (choix du visuel du masque, qui reste par systeme).
char sysBucketSlowFlag(const String &sysName, char bucketLetter)
{
  ensureSysDefaultCacheLoaded();
  bucketLetter = (char)toupper((unsigned char)bucketLetter);
  for (int i = 0; i < sysCacheCount; i++)
  {
    if (sysName == sysCacheKeys[i])
    {
      // v137 -- borne sur BUCKET_CACHE_MAX (< SYS_CACHE_MAX desormais) :
      // un systeme dont l'indice depasse ce plafond dedie n'a jamais eu de
      // donnee par bucket ecrite pour lui (voir loadSysDefaultCache()) --
      // repli normal et attendu sur le flag agrege, pas une erreur.
      if (sysCachePerLetterVals && i < BUCKET_CACHE_MAX)
      {
        const char *p = strchr(BUCKET_LETTERS, bucketLetter);
        if (p)
        {
          int idx = (int)(p - BUCKET_LETTERS);
          char c = sysCachePerLetterVals[i][idx];
          if (c == 'L' || c == 'N') return c; // donnee par bucket disponible
        }
      }
      return sysCacheSlowVals[i]; // repli: flag systeme agrege
    }
  }
  return 'N'; // systeme totalement inconnu
}

#define SYS_CACHE_FILE "/systems_cache.dat"

bool loadSysDefaultCache()
{
  File f = SD.open(SYS_CACHE_FILE, FILE_READ);
  if (!f) return false;
  sysCacheCount = 0;
  while (f.available() && sysCacheCount < SYS_CACHE_MAX)
  {
    String line = f.readStringUntil('\n'); line.trim();
    if (line.length() < 3) continue;
    char val = line.charAt(0);
    if (val != 'g' && val != 'p' && val != 'B') continue;

    // Format attendu:
    //   <val> <sysName> <slowFlag> <bucketFlags27>
    // Avec compatibilitÃ© (chantier "bucket", 4e champ optionnel) :
    //   <val> <sysName> <slowFlag>   (pas de donnee par bucket -- ancien
    //                                 fichier ou firmware/outil PC non a jour)
    //   <val> <sysName>              (slowFlag implicitement 'N')
    String rest = line.substring(2);
    rest.trim();

    int sp2 = rest.indexOf(' ');
    String sysName = (sp2 >= 0) ? rest.substring(0, sp2) : rest;

    char slow = 'N';
    String bucketStr = ""; // vide => pas de donnee par bucket (ancien format 3 champs)
    if (sp2 >= 0)
    {
      String flag = rest.substring(sp2 + 1);
      flag.trim();
      if (flag.length() > 0)
      {
        slow = flag.charAt(0);
        int sp3 = flag.indexOf(' ');
        if (sp3 >= 0)
        {
          bucketStr = flag.substring(sp3 + 1);
          bucketStr.trim();
        }
      }
    }

    strncpy(sysCacheKeys[sysCacheCount], sysName.c_str(), 31);
    sysCacheKeys[sysCacheCount][31] = '\0';
    sysCacheVals[sysCacheCount] = val;
    sysCacheSlowVals[sysCacheCount] = slow;

    // Sentinel '?' par defaut : "pas de donnee pour ce bucket" -> repli
    // sur sysCacheSlowVals[i] dans sysBucketSlowFlag(). Validation
    // STRICTE de la longueur (27) avant utilisation, ET par caractere
    // (un octet isole invalide/corrompu ne casse que ce bucket-la, pas
    // toute la ligne) -- robuste a un 4e champ absent (ancien firmware/
    // ancien fichier), tronque ou corrompu.
    // v137 -- meme borne BUCKET_CACHE_MAX qu'a la lecture (sysBucketSlowFlag).
    if (sysCachePerLetterVals && sysCacheCount < BUCKET_CACHE_MAX)
    {
      memset(sysCachePerLetterVals[sysCacheCount], '?', BUCKET_COUNT);
      if (bucketStr.length() == BUCKET_COUNT)
      {
        for (int k = 0; k < BUCKET_COUNT; k++)
        {
          char c = bucketStr.charAt(k);
          if (c == 'l') c = 'L';
          if (c == 'n') c = 'N';
          if (c == 'L' || c == 'N') sysCachePerLetterVals[sysCacheCount][k] = c;
        }
      }
    }

    sysCacheCount++;
  }
  f.close();
  Serial.println("[CACHE] charge: " + String(sysCacheCount) + " systemes");
  return sysCacheCount > 0;
}

void saveSysDefaultCache()
{
  SD.remove(SYS_CACHE_FILE);
  File f = SD.open(SYS_CACHE_FILE, FILE_WRITE);
  if (!f) return;
  for (int i = 0; i < sysCacheCount; i++)
  {
    f.print(sysCacheVals[i]); f.print(' ');
    f.print(sysCacheKeys[i]); f.print(' ');
    f.println(sysCacheSlowVals[i]);
  }
  f.close();
  Serial.println("[CACHE] sauvegarde: " + String(sysCacheCount) + " systemes");
}

static void countPngGifOverRec(const String &dirPath, int limit,
                                int &pngCount, int &gifCount,
                                bool &pngOver, bool &gifOver)
{
  if (pngOver && gifOver) return;

  File dir = SD.open(dirPath.c_str());
  if (!dir) return;
  if (!dir.isDirectory()) { dir.close(); return; }

  File entry = dir.openNextFile();
  while (entry)
  {
    if (pngOver && gifOver) { entry.close(); break; }

    String entryName = String(entry.name());
    if (entry.isDirectory())
    {
      String subPath = dirPath + "/" + entryName;
      entry.close();
      countPngGifOverRec(subPath, limit, pngCount, gifCount, pngOver, gifOver);
    }
    else
    {
      if (entryName.endsWith(".raw565"))
      {
        pngCount++;
        if (pngCount > limit) pngOver = true;
      }
      else if (entryName.endsWith(".raw565pack"))
      {
        gifCount++;
        if (gifCount > limit) gifOver = true;
      }
      entry.close();
    }

    entry = dir.openNextFile();
  }
  dir.close();
}

void buildSysDefaultCache()
{
  sysCacheCount = 0;
  File root = SD.open("/systems");
  if (!root) return;
  File entry = root.openNextFile();
  while (entry && sysCacheCount < SYS_CACHE_MAX)
  {
    if (entry.isDirectory())
    {
      String fullName = String(entry.name());
      int slash = fullName.lastIndexOf('/');
      String sysName = (slash >= 0) ? fullName.substring(slash + 1) : fullName;
      if (sysName == "_defaults") { entry.close(); entry = root.openNextFile(); continue; }
      String base = "/systems/_defaults/" + sysName;
      char val = '?';

      bool hasPack = SD.exists((base + ".raw565pack").c_str()) && SD.exists((base + ".meta").c_str());
      bool hasRaw  = SD.exists((base + ".raw565").c_str());

      // RÃ¨gle:
      // - uniquement .raw565 => 'p'
      // - uniquement .raw565pack + .meta => 'g'
      // - les deux => 'B'
      if (hasPack && hasRaw) val = 'B';
      else if (hasPack)      val = 'g';
      else if (hasRaw)       val = 'p';
      strncpy(sysCacheKeys[sysCacheCount], sysName.c_str(), 31);
      sysCacheKeys[sysCacheCount][31] = '\0';
      sysCacheVals[sysCacheCount] = val;

      int pngCount = 0;
      int gifCount = 0;
      bool pngOver = false;
      bool gifOver = false;

      // Seuil 5000 (v70) -- aligne sur build_systems_cache() cote outil PC
      // (RecalBoxDMD_tool.py v33), qui fait foi en usage normal. Ce repli
      // n'est emprunte que si /systems_cache.dat est absent de la SD.
      countPngGifOverRec("/systems/" + sysName, 5000, pngCount, gifCount, pngOver, gifOver);
      sysCacheSlowVals[sysCacheCount] = (pngOver || gifOver) ? 'L' : 'N';
      // Chantier "bucket" : ce repli (scan recursif, pas de decoupage par
      // bucket -- decision actee, voir sysBucketSlowFlag()) ne calcule
      // aucune donnee par bucket. BUG EVITE (trouve en portant le chantier
      // bucket) : sans ce memset, sysCachePerLetterVals[sysCacheCount]
      // resterait de la memoire heap NON INITIALISEE (malloc, pas calloc)
      // pour cette entree -- sysBucketSlowFlag() pourrait alors lire un
      // octet parasite egal par hasard a 'L'/'N' et retourner une valeur
      // bucket bidon au lieu de retomber correctement sur le flag systeme
      // agrege ci-dessus. Sentinel '?' explicite = meme comportement que
      // loadSysDefaultCache() sur une ligne a 3 champs (repli garanti).
      // v137 -- meme borne BUCKET_CACHE_MAX que loadSysDefaultCache()/sysBucketSlowFlag().
      if (sysCachePerLetterVals && sysCacheCount < BUCKET_CACHE_MAX) memset(sysCachePerLetterVals[sysCacheCount], '?', BUCKET_COUNT);
      sysCacheCount++;
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  Serial.println("[CACHE] " + String(sysCacheCount) + " systemes indexes");
  saveSysDefaultCache();
}

// --------------------------------------------------
// Cache des jeux â€” index bigramme 703 entrees
//
// Index 0       = '#'  (chiffres, tirets, etc.)
// Index 1       = 'A'  (jeux "a" + car. non-lettre)
// Index 2..27   = 'AA'..'AZ'
// Index 28      = 'B'
// ...
// Index 676     = 'Z'
// Index 677..702= 'ZA'..'ZZ'
// Total = 703 entrees (0..702)
//
// bigramTable est alloue dynamiquement en heap
// et libere avant drawPng pour liberer la RAM a pngle
// --------------------------------------------------
#define GAMES_IDX_MAX 300
#define NB_IDX        703   // 1 + 26*27

struct GamesSysIdx { char sysName[32]; uint32_t offset; };
static GamesSysIdx *gamesIdx = nullptr; // GAMES_IDX_MAX en heap
static int         gamesIdxCount  = 0;
static String      gamesCacheFile = "/games_cache.bin";

// Table bigramme â€” allouee dynamiquement, liberee avant affichage
static uint32_t *bigramTable      = nullptr; // NB_IDX x 4 bytes en heap
static String    bigramTableSys   = "";
static bool      bigramTableLoaded = false;

// Buffer tranche bigramme courante
static uint8_t  *bigramBuf          = nullptr;
static size_t    bigramBufSize      = 0;
static String    bigramBufKey       = "";
static uint32_t  bigramBufAbsOffset = 0;

void freeBigramBuffer()
{
  if (bigramBuf) { free(bigramBuf); bigramBuf = nullptr; }
  bigramBufSize = 0; bigramBufKey = ""; bigramBufAbsOffset = 0;
}

void freeBigramAll()
{
  freeBigramBuffer();
  if (bigramTable) { free(bigramTable); bigramTable = nullptr; }
  bigramTableSys    = "";
  bigramTableLoaded = false;
}

// Calcule l'index bigramme (0..702)
static int bigramIndex(const String &name)
{
  if (name.length() == 0) return 0;
  char c1 = (char)toupper((unsigned char)name.charAt(0));
  if (!isAlpha(c1)) return 0;
  int i1   = c1 - 'A';
  int base = 1 + i1 * 27;
  if (name.length() < 2) return base;
  char c2 = (char)toupper((unsigned char)name.charAt(1));
  if (!isAlpha(c2)) return base;
  return base + (c2 - 'A') + 1;
}

// Label lisible (ex: 343 -> "MR", 1 -> "A", 0 -> "#")
static String bigramLabel(int bi)
{
  if (bi == 0) return "#";
  int idx = bi - 1;
  int i1  = idx / 27;
  int i2  = idx % 27;
  char c1 = 'A' + i1;
  if (i2 == 0) return String(c1);
  return String(c1) + String((char)('A' + i2 - 1));
}

bool loadGamesIndex()
{
  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) return false;
  uint32_t nb = 0;
  f.read((uint8_t*)&nb, 4);
  if (nb == 0 || nb > (uint32_t)GAMES_IDX_MAX) { f.close(); return false; }
  gamesIdxCount = 0;
  for (uint32_t i = 0; i < nb && gamesIdxCount < GAMES_IDX_MAX; i++)
  {
    f.read((uint8_t*)gamesIdx[gamesIdxCount].sysName, 32);
    f.read((uint8_t*)&gamesIdx[gamesIdxCount].offset, 4);
    gamesIdxCount++;
  }
  f.close();
  Serial.println("[GCACHE] " + String(gamesIdxCount) + " systemes ("
                 + gamesCacheFile + ")");
  return gamesIdxCount > 0;
}

// v43 -- chargement paresseux (meme principe et meme justification que
// ensureSysDefaultCacheLoaded() ci-dessus) : sur un boot "reboot cible mode
// config", ce cache est saute au demarrage -- rechargement automatique, une
// seule fois, au premier vrai besoin (findInGamesCache(), typiquement un
// evenement MQTT CMD_GAME apres "Reprendre DMD"). Sur un boot normal, mis a
// true juste apres le chargement eager habituel dans setup().
static bool gamesCacheLoadAttempted = false;

static void ensureGamesIndexLoaded()
{
  if (gamesCacheLoadAttempted) return;
  gamesCacheLoadAttempted = true;
  if (!loadGamesIndex())
    Serial.println("[GCACHE] charge a la demande: " + gamesCacheFile + " absent");
  else
    Serial.println("[GCACHE] charge a la demande (post-copie): " + String(gamesIdxCount) + " systemes");
}

// Garde-fou heap bas avant ouverture fichier dans le chemin lent CMD_GAME
// (raw565pack/.gif/.png/.raw565 sur /systems/..., + loadBigramTable() ci-
// dessous depuis v81) -- 2026-08-09, voir v60/v61/v62. v60 utilisait 8500
// (marge vs buffer de frame raw565(pack), 8192 octets) -- MAUVAIS calibrage
// confirme en test reel (log utilisateur) : le plancher NORMAL de
// ESP.getMaxAllocHeap() en fonctionnement sain tourne en continu autour de
// 4596-5876 (du au setvbuf(4096) de SD.open()), donc maxalloc<8500 etait
// vrai quasi en permanence -- le garde-fou interceptait SYSTEMATIQUEMENT,
// empechant tout raw565pack de se charger. Seuil abaisse a 3000 (sous ce
// plancher normal) en v61. Centralisee ici (avant drawRaw565() en v62, puis
// avant loadBigramTable() en v81 -- deplacee plus haut dans le fichier pour
// rester utilisable par les deux) pour que chaque site puisse l'utiliser
// directement sans dupliquer la verification.
const unsigned long CMD_GAME_MIN_HEAP_FOR_FILE_OPEN = 3000;

// Charge la table bigramme du systeme en heap (une seule lecture SD)
// v81 (2026-08-17) -- CRASH REEL confirme sur materiel (abort() decode via
// addr2line, hash ELF verifie identique au binaire plante, pendant le test
// du chunk 3) : SD.open()->fs::FS::open()->make_shared<VFSFileImpl>->
// operator new echoue quand le heap contigu est trop bas (observe a
// maxalloc=4596, le meme plateau deja documente ailleurs dans ce fichier
// pour getNextGifRandom()/drawRaw565()) -- ce site precis (appele 9 fois,
// notamment depuis preloadBigram() dans le chemin CMD_GAME) n'avait AUCUN
// garde-fou, contrairement a ces 2 autres. Fix : meme double protection
// deja eprouvee ailleurs -- seuil heuristique CMD_GAME_MIN_HEAP_FOR_FILE_OPEN
// (deja utilise pour un SD.open() comparable dans le meme chemin CMD_GAME,
// reutilise ici plutot que d'inventer une nouvelle constante) AVANT la
// tentative, + try/catch en filet de securite (un seuil fixe seul ne
// suffit pas toujours, voir le meme constat deja fait pour
// getNextGifRandom()). Centralise ICI (choix deliberе, meme raisonnement
// que pour drawRaw565() en v62) : protege ses 9 appelants d'un coup au lieu
// de dupliquer la verification a chaque site.
bool loadBigramTable(const String &sysName)
{
  if (bigramTableLoaded && bigramTableSys == sysName && bigramTable != nullptr)
    return true;

  if (ESP.getMaxAllocHeap() < CMD_GAME_MIN_HEAP_FOR_FILE_OPEN) {
    Serial.println("[GCACHE] loadBigramTable heap trop bas (maxalloc=" + String(ESP.getMaxAllocHeap())
                   + ") -> abandon avant open");
    return false;
  }

  uint32_t sysOffset = 0; bool found = false;
  for (int i = 0; i < gamesIdxCount; i++)
    if (sysName == gamesIdx[i].sysName) { sysOffset = gamesIdx[i].offset; found = true; break; }
  if (!found) return false;

  // Allouer si besoin
  if (!bigramTable)
  {
    bigramTable = (uint32_t*)malloc(NB_IDX * 4);
    if (!bigramTable) return false;
  }

  try
  {
    File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
    if (!f) { free(bigramTable); bigramTable = nullptr; return false; }

    f.seek(sysOffset);
    size_t read = f.read((uint8_t*)bigramTable, NB_IDX * 4);
    f.close();

    if (read < (size_t)(NB_IDX * 4))
    {
      free(bigramTable); bigramTable = nullptr; return false;
    }
  }
  catch (std::exception &e)
  {
    Serial.println(String("[GCACHE] EXCEPTION rattrapee dans loadBigramTable() (heap critique, maxalloc=")
                   + String(ESP.getMaxAllocHeap()) + ") : " + e.what());
    free(bigramTable); bigramTable = nullptr;
    return false;
  }
  catch (...)
  {
    Serial.println("[GCACHE] EXCEPTION inconnue rattrapee dans loadBigramTable() (maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
    free(bigramTable); bigramTable = nullptr;
    return false;
  }

  bigramTableSys    = sysName;
  bigramTableLoaded = true;
  Serial.println("[GCACHE] table " + sysName + " (" + String(NB_IDX*4) + " bytes)");
  return true;
}

// Charge la tranche du bigramme en heap
// La table doit etre chargee (loadBigramTable)
bool preloadBigram(const String &sysName, const String &gameName)
{
  if (!loadBigramTable(sysName)) return false;

  int    bi  = bigramIndex(gameName);
  String key = sysName + "/" + bigramLabel(bi);
  if (bigramBufKey == key && bigramBuf != nullptr) return true;

  uint32_t bigramOffset = bigramTable[bi];
  if (bigramOffset == 0) return false;

  // Trouver l'offset suivant non nul dans la table (RAM)
  uint32_t nextOffset = 0;
  for (int nbi = bi + 1; nbi < NB_IDX; nbi++)
    if (bigramTable[nbi] != 0) { nextOffset = bigramTable[nbi]; break; }

  if (nextOffset == 0 || nextOffset <= bigramOffset)
  {
    for (int i = 0; i < gamesIdxCount - 1; i++)
      if (sysName == gamesIdx[i].sysName) { nextOffset = gamesIdx[i+1].offset; break; }
    if (nextOffset == 0 || nextOffset <= bigramOffset)
    {
      File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
      if (f) { nextOffset = f.size(); f.close(); }
    }
  }

  size_t sliceSize = (nextOffset > bigramOffset) ? nextOffset - bigramOffset : 0;
  if (sliceSize == 0) return false;

  size_t maxAlloc = ESP.getMaxAllocHeap();
  if (sliceSize > maxAlloc / 2)
  {
    Serial.println("[GCACHE] tranche " + key + " trop grande ("
                   + String(sliceSize) + ") -> SD directe");
    return false;
  }

  freeBigramBuffer();
  bigramBuf = (uint8_t*)malloc(sliceSize);
  if (!bigramBuf) return false;

  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) { freeBigramBuffer(); return false; }
  f.seek(bigramOffset);
  f.read(bigramBuf, sliceSize);
  f.close();

  bigramBufSize      = sliceSize;
  bigramBufKey       = key;
  bigramBufAbsOffset = bigramOffset;

  Serial.println("[GCACHE] preload " + key + " (" + String(sliceSize)
                 + " bytes) free=" + String(ESP.getFreeHeap()));
  return true;
}

// Si le systÃ¨me est flag 'L' (lent), le cache bigram est inutile :
// les fichiers individuels existent mais on les cherche directement
// via SD.open() dans drawRaw565() / openGif().
// Retourner 'B' force la tentative des deux types.
static inline bool sysIsSlow(const String &sysName)
{
  char f = sysDefaultSlowFlag(sysName);
  return (f == 'L' || f == 'l');
}

char findInGamesCache(const String &sysName, const String &gameName)
{
  ensureGamesIndexLoaded();
  if (gamesIdxCount == 0) return '?';

  int    bi          = bigramIndex(gameName);
  String key         = sysName + "/" + bigramLabel(bi);
  String gameNameLow = gameName; gameNameLow.toLowerCase();

  // Recherche RAM
  if (bigramBufKey == key && bigramBuf != nullptr && bigramBufSize > 0)
  {
    uint8_t *ptr = bigramBuf;
    uint8_t *end = bigramBuf + bigramBufSize;
    char bestType = '?'; int bestLen = 0;

    while (ptr < end)
    {
      if (ptr + 1 >= end) break;
      char type = (char)*ptr; ptr++;
      uint8_t *nameStart = ptr;
      while (ptr < end && *ptr != 0) ptr++;
      if (ptr >= end) break;
      int nameLen = ptr - nameStart; ptr++;

      if (nameLen == (int)gameNameLow.length())
      {
        bool match = true;
        for (int i = 0; i < nameLen && match; i++)
          if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
            match = false;
        if (match) return type;
      }
      if (nameLen < (int)gameNameLow.length() && nameLen > bestLen)
      {
        bool pfx = true;
        for (int i = 0; i < nameLen && pfx; i++)
          if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
            pfx = false;
        if (pfx) { bestLen = nameLen; bestType = type; }
      }
    }
    return bestType;
  }

  // Fallback SD â€” lit une tranche en bloc puis parse en RAM
  if (!bigramTableLoaded || bigramTableSys != sysName)
    if (!loadBigramTable(sysName)) return '?';

  if (!bigramTable) return '?';
  uint32_t bigramOffset = bigramTable[bi];
  if (bigramOffset == 0) return '?';

  // Trouver la taille de la tranche (jusqu'au prochain bigramme non nul)
  uint32_t nextOffset = 0;
  for (int nbi = bi + 1; nbi < NB_IDX; nbi++)
    if (bigramTable[nbi] != 0) { nextOffset = bigramTable[nbi]; break; }
  if (nextOffset == 0 || nextOffset <= bigramOffset)
  {
    if (gamesIdxCount > 0) { nextOffset = gamesIdx[gamesIdxCount-1].offset; }
    else { nextOffset = bigramOffset + 2048; } // fallback 2KB
  }

  uint32_t sliceSize = (nextOffset > bigramOffset) ? nextOffset - bigramOffset : 2048;
  if (sliceSize > 8192) sliceSize = 8192; // sÃ©curitÃ© heap

  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) return '?';
  f.seek(bigramOffset);

  // Lecture en bloc (Ã©vite des centaines de petits reads SPI)
  uint8_t *buf = (uint8_t*)malloc(sliceSize);
  if (!buf) { f.close(); return '?'; }
  size_t got = f.read(buf, sliceSize);
  f.close();

  if (got == 0) { free(buf); return '?'; }

  // Parse en RAM
  uint8_t *ptr = buf;
  uint8_t *end = buf + got;
  char bestType = '?'; int bestLen = 0;

  while (ptr < end)
  {
    if (ptr + 1 >= end) break;
    char type = (char)*ptr; ptr++;
    uint8_t *nameStart = ptr;
    while (ptr < end && *ptr != 0) ptr++;
    if (ptr >= end) break;
    int nameLen = ptr - nameStart; ptr++;

    if (nameLen == (int)gameNameLow.length())
    {
      bool match = true;
      for (int i = 0; i < nameLen && match; i++)
        if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
          match = false;
      if (match) { free(buf); return type; }
    }
    if (nameLen < (int)gameNameLow.length() && nameLen > bestLen)
    {
      bool pfx = true;
      for (int i = 0; i < nameLen && pfx; i++)
        if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
          pfx = false;
      if (pfx) { bestLen = nameLen; bestType = type; }
    }
  }

  free(buf);
  return bestType;
}

// --------------------------------------------------
// Pins & dimensions
// --------------------------------------------------
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

#define CLK_PIN 16
#define OE_PIN  15
#define LAT_PIN  4
#define A_PIN   33
#define B_PIN   32
#define C_PIN   22
#define D_PIN   17
#define E_PIN   -1

#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13

#define SD_CS_PIN  5
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCLK 18

#define MQTT_PORT         1883
#define MQTT_CLIENT  "esp32-marquee"
#define MQTT_RETRY_MS    15000
#define MQTT_START_DELAY_MS 12000
// v1 -- piste UDP (voir TRANSPORT_PLAN_UDP.md) : port dedie, fire-and-forget,
// prototype limite a CMD=score pour valider la fiabilite avant d'etendre.
#define UDP_CMD_PORT      5005

// --------------------------------------------------
// Globaux
// --------------------------------------------------
MatrixPanel_I2S_DMA *display = nullptr;
AnimatedGIF gif;
SPIClass spiSD(VSPI);
File gifFile;
File nextGifFile;
BluetoothSerial SerialBT;

// v110 -- MODE_SCORE ajoute : ecran plein temporaire (score hi-score), ne
// touche JAMAIS gifOpened/currentPngPath/pngDrawn -- le jeu reste ouvert et
// intact "en dessous" pendant l'affichage du score, voir case MODE_SCORE de
// loop() pour le retour automatique.
enum DisplayMode { MODE_PLAYLIST, MODE_GIF, MODE_PNG, MODE_CONFIG, MODE_BLACK, MODE_SCORE };
volatile DisplayMode currentMode = MODE_PLAYLIST;

bool g_sdOpInProgress = false;

// v104 -- variables "precoces" du hi-score/game_info/achievement (scoreEnabled,
// g_inGameMarquee, g_overlayPendingStart, lastAchievementText,
// gameInfoEnabled, achievementEnabled, etc.) RETIREES entierement -- meme
// raison que le sous-systeme overlay plus bas dans ce fichier, voir son
// commentaire.

// v75 -- drapeau dedie pour interrompre un apercu horloge (showClock() en
// mode preview, boucle bloquante -- voir CMD_CLOCK_PREVIEW) depuis
// webDmdResume() ("Reprendre DMD"). NECESSAIRE en plus de pendingCmd/
// hasPendingMqttCommand() (deja utilise par le "stop" normal et le
// changement de theme) : reutiliser pendingCmd="stop" depuis webDmdResume()
// ferait retraiter ce "stop" par processPendingMqttCommand() a l'iteration
// SUIVANTE de loop(), qui repasse currentMode=MODE_CONFIG (ecran de pause)
// -- ecrasant le MODE_PLAYLIST/GIF que resumePlaylist() vient tout juste de
// poser dans ce meme appel. Un drapeau simple, distinct, evite ce
// chevauchement : showClock() le consomme lui-meme (return immediat, sans
// toucher currentMode -- deja fixe par resumePlaylist() entre-temps).
volatile bool g_clockPreviewAbort = false;

String   g_sdOpMsg       = "";
String   g_sdOpSubMsg    = "";
uint16_t g_sdOpSubMsgColor = 0xFFFF;
// Index (nombre de caracteres) a partir duquel g_sdOpSubMsg doit etre
// dessine en blanc plutot que g_sdOpSubMsgColor -- -1 = desactive (toute
// la ligne dans g_sdOpSubMsgColor, comportement historique). Utilise
// uniquement par maintainApRecovery() (2026-08-09, demande utilisateur :
// faire ressortir le SSID/l'IP du prefixe d'instruction) -- jamais
// touche par les autres sites qui assignent g_sdOpSubMsg (webDmdPause(),
// CMD_SHOW_CONFIG, etc.), sans risque de fuite d'etat entre l'ecran AP
// secours et les autres : la sortie de ce mode passe toujours par un
// ESP.restart() (jamais de retour normal a MODE_CONFIG "classique" sans
// reboot complet, qui reinitialise ce global a -1).
int      g_sdOpSubMsgWhiteFrom = -1;
// Horodatage (millis()) jusqu'auquel le defilement de la ligne 2 doit
// rester en pause -- 0 = pas de pause en cours. Positionne des que le
// defilement revele entierement la fin de la chaine (le SSID/IP, voir
// g_sdOpSubMsgWhiteFrom) pour laisser le temps de la lire (2026-08-09,
// demande utilisateur). Reinitialise a 0 par webDmdForceRedraw() a
// chaque nouveau message.
unsigned long g_sdOpSubMsgPauseUntil = 0;
// Message "de fond" (ex: IP du DMD, pose par triggerWebConfigMode()) a
// reafficher automatiquement quand un message de statut transitoire
// (ex: "Mise en cache...", "OK", erreurs -- via webDmdPause()) reste
// affiche sans mise a jour depuis SD_OP_SUBMSG_EXPIRE_MS : evite qu'un
// message ponctuel ne reste affiche indefiniment sur l'ecran physique une
// fois le process termine (demande utilisateur). g_sdOpSubMsgSetAt reste a
// 0 tant que webDmdPause() n'a jamais ete appelee (ecrans de boot/secours
// WiFi qui assignent g_sdOpSubMsg directement, hors webDmdPause() -- non
// concernes par cette expiration).
String        g_sdOpPersistentSubMsg = "";
uint16_t      g_sdOpPersistentSubMsgColor = 0xFFE0;
unsigned long g_sdOpSubMsgSetAt = 0;
static const unsigned long SD_OP_SUBMSG_EXPIRE_MS = 5000;
// Scroll tracking pour le sous-message
int      g_sdOpScrollOffset = 0;
unsigned long g_sdOpLastScroll = 0;
int      g_sdOpScrollOffset1 = 0;
unsigned long g_sdOpLastScroll1 = 0;
bool     g_configDmdDirty = false;
bool     g_firstBoot = true;
bool     g_forceApRecovery = false; // force_ap_recovery: demande via marquee/cmd/wifi_recovery
// v41 -- REINTRODUITS (retires en v37/commit "v93") : le plancher heap
// ~4596 octets du au buffer setvbuf(4096) alloue par SD.open() (voir memoire
// projet, "fuite ~4200 octets/GIF") est reapparu en test reel (upload MEDIA
// bloque presque a 100%, 2026-08-02) -- retire a l'epoque par comparaison
// avec RecalBox_DMDv10_scriptsRB qui n'en a jamais eu besoin, mais cette
// comparaison ne concernait pas ce symptome precis (elle portait sur le
// nombre de requetes HTTP par upload). La cause racine (setvbuf non statique
// dans la lib FS) n'a jamais ete corrigee -- ce reboot cible reste le seul
// contournement valide sur ce firmware en attendant le futur chantier
// fopen()/setvbuf statique (branche dev separee).
bool     g_skipPlaylistForConfig = false; // force_config_boot (config.ini) : ce boot doit sauter
  // directement en mode config sans jamais lancer la playlist/ouvrir de GIF --
  // consomme (remis a "0" dans config.ini) des lecture dans loadConfig().
bool     g_playlistStartedThisBoot = false; // true des que la playlist/le 1er GIF a reellement
  // demarre ce boot -- sert a triggerWebConfigMode() (web_config.h) pour savoir si un reboot
  // "propre" (sans playlist) apporterait un vrai gain de heap avant d'entrer en mode config.
String   uiLanguage = "fr"; // language: fr/en/es -- transmis par l'outil Windows via config.ini,
                             // pilote les bannieres informatives DMD + pages web (voir trOpenBrowserAt() etc.)

bool   gifOpened      = false;
bool   pngDrawn       = false;
String currentPngPath = "";

// ------------------------------
// PNG async (pour systemes "L")
// ------------------------------
static uint16_t *pngAsyncFb = nullptr; // 16-bit RGB565 plein Ã©cran (largeur = PANEL_RES_X*PANEL_CHAIN, hauteur = PANEL_RES_Y)
static size_t    pngAsyncFbPixels = 0;

static TaskHandle_t asyncPngTaskHandle = nullptr;
static volatile bool asyncPngInProgress = false;
static volatile bool asyncPngReady = false;
static volatile bool asyncPngCancel = false;
static uint32_t asyncPngRequestId = 0;
static uint32_t asyncPngActiveRequestId = 0;
static String asyncPngPath = "";
static volatile bool currentPngAsyncWanted = false;
unsigned long asyncPngStartMs = 0;
String playlistName       = "";
String playlistSourcePath = "";
String playlistCachePath  = "";
String playlistSigPath    = "";
String playlistIdxPath    = "";
bool   playlistRandom     = true;
String imageFolder        = "";  // Vide : images directement dans systems/<sys>/

int gifCount        = 0;
int playIndex       = 0;
int lastRandomIndex = -1;

File seqPlaylistFile;
File idxFileHandle;

bool   requestNextGif = false;
bool   requestReboot  = false;
String nextGifPath    = "";

bool   wifiEnabled               = true;
String wifiSSID                  = "";
String wifiPassword              = "";
bool   wifiStaticEnabled         = false;
String wifiStaticIP              = "";
String wifiGateway               = "";
String wifiSubnet                = "";
String wifiDNS1                  = "";
String wifiDNS2                  = "";
unsigned long lastWifiReconnectAttempt = 0;
int consecutiveWifiReconnectCycles = 0; // v155 -- recul progressif du cooldown maintainWiFi(), voir sa declaration

bool   bluetoothEnabled = false;
String bluetoothName    = "ESP32-GIF";
bool   showInfo         = true;
int    screenBrightness = 120;  // 0..255 (map depuis 0-100% dans config.ini: brightness=)

// v110 -- reglages hi-score/info/description/RA (4 fonctionnalites x 2
// contextes = 8 booleens), demande utilisateur explicite (2026-08-19) :
// reintroduits sur la page web (retiree en v104 avec tout le sous-systeme
// overlay), mais restent PUREMENT INFORMATIFS cote firmware -- personne ici
// ne decide QUAND afficher quoi (voir philosophie "DMD bete", memoire
// projet). C'est la RB (marquee.sh/dmd_score.sh) qui lit ces valeurs (via
// broadcastFeatureStatus(), topic retenu marquee/status/features) pour
// decider elle-meme d'envoyer -- ou non -- marquee/cmd/score. Valeurs par
// defaut (2026-08-20, demande utilisateur explicite) : en jeu -> hi-score +
// RA ; en liste de jeux -> infos + description.
bool featHiscoreIngame     = true;
bool featHiscoreBrowse     = false;
bool featInfoIngame        = false;
bool featInfoBrowse        = true;
bool featDescriptionIngame = false;
bool featDescriptionBrowse = true;
bool featRaIngame          = true;
bool featRaBrowse          = false;
// v111 -- espacement de repetition du slideshow hi-score/infos EN JEU,
// exprime en NOMBRE DE CYCLES (pas en secondes) -- demande utilisateur
// explicite (2026-08-20) : "exprime le slider en cycle d'affichage marquee
// plutot qu'en duree, pour eviter le probleme du chevauchement". 1 cycle =
// la duree naturelle d'un passage complet du slideshow (calculee par
// dmd_score.sh selon les cartes ingame actuellement actives -- hi-score
// seul, ou + infos/description). Repeter "toutes les N cycles" garantit
// PAR CONSTRUCTION que l'intervalle reel (N x duree du slideshow) ne peut
// jamais etre plus court qu'un slideshow complet, quel que soit le nombre
// de cartes activees -- plus besoin de choisir une duree "sure" a la
// main. 0 = desactive (comportement d'origine v110, une seule fois par
// partie). PUREMENT INFORMATIF ici aussi (voir featHiscoreIngame etc.) :
// c'est dmd_score.sh qui lit cette valeur et gere le minuteur lui-meme --
// le DMD ne voit AUCUNE difference entre un declenchement "une fois" et
// "repete". Ne s'applique qu'au contexte "ingame" -- le contexte
// navigation se re-declenche deja naturellement a chaque nouveau dwell.
int featRepeatCycles = 3;
// v111 -- meme principe que featRepeatCycles ci-dessus, mais pour le
// contexte NAVIGATION (round-robin apres dwell, dmd_score.sh v6) -- ratio
// separe, reglable independamment (demande utilisateur explicite : "fait 2
// valeurs separees pour chaque situation"). PUREMENT INFORMATIF ici aussi.
int featRepeatBrowseCycles = 3;
// v111 -- delai d'immobilite (secondes) avant de declencher le round-robin
// navigation -- etait fixe a 5s en dur cote script (DWELL_SECONDS,
// dmd_score.sh v3), rendu reglable ici (demande utilisateur explicite).
// Plancher de securite impose a la SAUVEGARDE (voir loadConfig()) : "un
// minimum securitaire doit etre impose pour ne pas qu'il se declenche
// pendant une navigation normale" -- meme plancher reapplique cote script
// (defense en profondeur, DWELL_MIN_SECONDS).
int featDwellSeconds = 5;

// --------------------------------------------------
// Horloge (Clock) - variables
// --------------------------------------------------
bool   clockEnabled       = false;   // CLOCK_ENABLED dans [CLOCK]
int    clockTheme         = -1;      // -1=random, 0..RETRO_THEME_COUNT-1=theme retro
int    clockIntervalGifs  = 10;      // CLOCK_INTERVAL - nb GIFs entre chaque horloge
int    clockIntervalMin   = 0;       // CLOCK_INTERVAL_MIN (0=desactive, utilise GIFs)
int    clockDuration      = 8;       // CLOCK_DURATION - secondes d'affichage
unsigned long lastClockMs   = 0;      // millis() de la derniere apparition
int    clockGifCounter     = 0;      // compteur de GIFs depuis derniere horloge
bool   clockVisible        = false;  // true pendant l'affichage de l'horloge
unsigned long clockStartMs = 0;      // millis() du debut de l'horloge actuelle
int    currentTheme        = 0;      // theme retro actif
int    lastThemePick       = -1;     // anti-repetition random
unsigned long themeStartMs  = 0;     // millis() du dernier changement de theme
bool   clockNtpSynced      = false;  // true si NTP a deja synchronise
unsigned long clockNtpLastTry = 0;   // millis() du dernier essai NTP
String clockTimeZone       = "CET-1CEST,M3.5.0,M10.5.0/3"; // timezone
bool   clockNeonCustomColor = false; // CLOCK_COLOR set? (Neon theme only)
uint8_t clockNeonR = 255, clockNeonG = 40, clockNeonB = 120; // defaults match Neon's built-in pink
String recalboxIP     = "";
String mqttEventTopic = "marquee/event";
const unsigned long MQTT_OFFLINE_FALLBACK_MS = 60000;
// v150 -- nettoyage pre-merge master (revue de code) : MQTT_WAITING_GRACE_MS/
// g_mqttWaitingUntilMs retires. Servaient a inWaitingGrace (fenetre de grace
// filtrant system/game trop tot apres connexion) -- ce filtre lui-meme a ete
// retire en v102 (system/game traites comme default depuis v45, plus jamais
// filtres), laissant ces 2 declarations en ecriture seule (2 sites
// d'ecriture, zero lecture) depuis. Voir DECISIONS.md pour le detail.

// Drapeau "ecran d'attente RecalBox connectee actif" (CMD_WAITING_MQTT) --
// pose (non-zero) a l'affichage de l'image de secours + texte, remis a 0 des
// qu'un vrai contenu MQTT prend la main (CMD_DEFAULT/CMD_SYSTEM/CMD_GAME/
// CMD_STOP). Sert uniquement a piloter le clignotement du texte (voir
// loop()). PLUS d'expiration par delai fixe (retiree v45, 2026-08-03,
// demande explicite utilisateur) : la logique voulue est d'attendre
// INDEFINIMENT tant que la Recalbox reste connectee -- c'est elle seule qui
// decide quand revenir a la playlist (CMD_DEFAULT, pont marquee sur
// veille/lecture d'un clip), jamais un delai arbitraire cote DMD.
unsigned long g_mqttConnectedScreenUntilMs = 0;

// Indicateur "No wifi, No Recalbox" (2026-08-05, demande utilisateur) --
// affiche brievement l'image de secours + texte rouge clignotant quand le
// WiFi lui-meme reste injoignable alors qu'un SSID est configure (voir
// mqttTask()/setupWiFiFromConfig() : sur un appareil deja entierement
// configure, first_boot=0, le repli AP a ete retire pour ce cas -- cet
// indicateur compense l'absence totale de feedback visuel qui en
// resultait). Contrairement a g_mqttConnectedScreenUntilMs (attente
// INDEFINIE d'un vrai message MQTT), ceci est un ecran TEMPORISE et
// auto-resolutif : aucun message externe ne viendra jamais tant que le
// WiFi est down, donc pas de sens a attendre indefiniment.
// g_noWifiRecalboxPending : demande posee par mqttTask() (tache de fond),
// consommee par loop() au prochain point sur qui ne coupe pas une
// animation en cours (entre deux GIFs, voir case MODE_PLAYLIST).
bool g_noWifiRecalboxPending = false;
// g_noWifiRecalboxScreenActive : ecran actuellement affiche, pilote le
// clignotement (voir loop()) -- remis a false soit par l'expiration du
// delai (voir g_noWifiRecalboxUntilMs), soit si un vrai contenu MQTT
// reprend la main entre-temps (memes points de reset que
// g_mqttConnectedScreenUntilMs=0).
bool g_noWifiRecalboxScreenActive = false;
unsigned long g_noWifiRecalboxUntilMs = 0;
// Duree d'affichage fixe avant retour automatique a la playlist -- valeur
// reprise de MQTT_WAITING_MIN_DISPLAY_MS (5000ms, voir plus bas) mais
// mecanisme different (auto-resolutif, pas juste un delai minimum avant
// interruption) : declaree separement plutot que de reutiliser cette
// constante existante, qui garde sa propre semantique.
const unsigned long NO_WIFI_ALERT_DISPLAY_MS = 5000;

// Indicateur "RecalBox non connectee" (2026-08-05, demande utilisateur --
// meme principe que l'indicateur "No wifi, No Recalbox" ci-dessus, en
// parallele) : WiFi OK mais la connexion MQTT elle-meme echoue avec
// mqttClient.state()==-2 (MQTT_CONNECT_FAILED, PubSubClient -- echec de
// connexion TCP au broker, ex. Recalbox eteinte/injoignable alors que le
// WiFi fonctionne). Texte orange clignotant, TRADUIT (contrairement a
// "No wifi, No Recalbox" -- celui-ci reprend le meme registre que
// trRecalboxConnected(), deja traduit). Meme duree d'affichage
// (NO_WIFI_ALERT_DISPLAY_MS, 5s) et memes points de reset que
// l'indicateur WiFi. Frequence de reaffichage suivie par horodatage
// (lastRecalboxDisconnectedAlertMs, dans mqttTask()) plutot que par
// comptage d'iterations : la boucle d'echec MQTT tourne a un rythme
// different (MQTT_RETRY_MS=15s, pas 1s) de la boucle WiFi-down, un simple
// modulo sur le nombre de tentatives ne donnerait pas 60s reels ici.
bool g_recalboxDisconnectedPending = false;
bool g_recalboxDisconnectedScreenActive = false;
unsigned long g_recalboxDisconnectedUntilMs = 0;

// Dernier etat MQTT reellement affiche (v50, 2026-08-03, bug reel confirme :
// apres "Reprendre DMD" alors que RB est en mode clip, la playlist ne
// reprenait jamais) -- RB annonce son passage en demo/clip UNE FOIS
// (CMD_DEFAULT/CMD_STARTCLIP), pas a chaque nouveau clip -- le fix v47
// (webDmdResume() attend un nouveau message MQTT au lieu de forcer la
// playlist) restait donc bloque indefiniment sur l'ecran d'attente dans ce
// cas precis, RB n'ayant plus rien de neuf a annoncer. true = le dernier
// contenu REEL affiche via MQTT etait la playlist/l'ecran d'attente
// (CMD_DEFAULT/CMD_STARTCLIP) ; false = un system/jeu precis
// (CMD_SYSTEM/CMD_GAME/CMD_RESUMESYS). webDmdResume() s'en sert : si true,
// reprend directement la playlist (etat encore valide, pas besoin d'attendre
// RB) ; si false, affiche l'ecran d'attente comme avant (un jeu/systeme
// precis pourrait etre perime, mieux vaut attendre une confirmation fraiche).
bool g_lastMqttWasDefault = true;

// Delai minimum d'affichage de l'ecran "RecalBox connectee" (v49,
// 2026-08-03, demande explicite utilisateur) : un "default" arrivant tres
// tot (RB deja en mode demo/clip a la connexion, cf. v46 -- desormais honore
// au lieu d'etre ignore) faisait basculer sur la playlist QUASI INSTANTANEMENT,
// sans laisser le temps de voir l'ecran de confirmation. Contrairement a
// MQTT_WAITING_GRACE_MS (1.5s, anti-retenu-perime pour system/game -- un
// "default" trop tot n'est PLUS ignore mais DIFFERE) : si un CMD_DEFAULT
// arrive avant ce delai, l'action (resumePlaylist()) est memorisee et
// appliquee automatiquement des que le delai est ecoule (voir loop()),
// jamais perdue -- contrairement a l'ancien filtrage qui pouvait bloquer
// indefiniment si aucun autre message ne suivait.
const unsigned long MQTT_WAITING_MIN_DISPLAY_MS = 5000;
unsigned long g_mqttWaitingMinDisplayUntilMs = 0;
bool          g_mqttDefaultPendingAfterMinDisplay = false;

WiFiClient   wifiClientMqtt;
PubSubClient mqttClient(wifiClientMqtt);
String       lastSysName = "";
String       displayedMaskSysName = "";

// v1 -- piste UDP (voir TRANSPORT_PLAN_UDP.md). dmdUdp.begin(UDP_CMD_PORT)
// est appele une seule fois, juste apres la 1ere connexion WiFi reussie
// (setupWiFiFromConfig()) -- PAS REARME apres une reconnexion WiFi
// ulterieure (maintainWiFi()) pour l'instant : a valider en conditions
// reelles si necessaire (le socket UDP local, bind() sur INADDR_ANY,
// devrait en principe survivre a un cycle disconnect/reconnect qui ne
// recree pas l'interface elle-meme -- pas encore confirme sur materiel).
WiFiUDP      dmdUdp;

// v139 -- voir changelog v139 en entete pour le contexte complet. Contourne
// NetworkClient::write() (coeur Arduino-ESP32, NetworkClient.cpp) qui peut
// bloquer jusqu'a WIFI_CLIENT_MAX_WRITE_RETRY(10) x
// WIFI_CLIENT_SELECT_TIMEOUT_US(1s) = 10s via sa propre boucle select()
// interne AVANT le moindre send() si le socket ne devient jamais
// "ecrit-pret" -- constantes en dur dans un .cpp du coeur, non overridables
// depuis ce fichier. Reconstruit le MEME paquet MQTT SUBSCRIBE que
// PubSubClient::subscribe() (fixe header type=8/QoS1 + ID paquet 2 octets +
// longueur topic 2 octets + topic + octet QoS), envoye via send()/select()
// MAISON avec un budget BEAUCOUP plus court. Compteur d'ID de paquet
// INDEPENDANT de celui de PubSubClient (this->nextMsgId est prive,
// inaccessible) -- sans consequence, le broker ne fait aucun lien entre nos
// souscriptions et les eventuels PUBLISH/PUBACK QoS>0 de PubSubClient sur ce
// firmware (aucun publish QoS>0 fait par ce DMD).
static uint16_t g_fastSubMsgId = 1;

// Sonde ecrit-pret avec budget COURT (ms), remplace l'attente 1s x 10
// essais du coeur par une seule fenetre select() configurable.
// v140 -- $outRc/$outErrno optionnels (nullable) : remontent le code retour
// BRUT de select() (et errno si <0) a l'appelant, pour distinguer "timeout
// franc" (rc=0, a attendu tout $timeoutMs pour rien) de "select() a
// lui-meme echoue" (rc<0, retourne probablement quasi instantanement,
// errno donne la vraie raison -- EBADF si fd invalide/deja ferme, etc.).
// Precedemment invisible : les 2 cas remontaient identiquement "false".
static bool socketWritableQuick(int fd, uint32_t timeoutMs, int *outRc = nullptr, int *outErrno = nullptr)
{
  if (fd < 0) { if (outRc) *outRc = -1000; if (outErrno) *outErrno = 0; return false; }
  fd_set set;
  struct timeval tv;
  FD_ZERO(&set);
  FD_SET(fd, &set);
  tv.tv_sec = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;
  int rc = select(fd + 1, NULL, &set, NULL, &tv);
  if (outRc) *outRc = rc;
  if (outErrno) *outErrno = (rc < 0) ? errno : 0;
  return (rc > 0 && FD_ISSET(fd, &set));
}

// Retourne true si le paquet SUBSCRIBE complet a ete envoye (send() a
// accepte tous les octets) -- ne garantit PAS la reception du SUBACK (comme
// PubSubClient::subscribe() lui-meme, qui ne l'attend pas non plus). $qos
// attendu 0 ou 1 (meme limite que PubSubClient).
bool mqttSubscribeFast(WiFiClient &client, const char *topic, uint8_t qos, uint32_t perAttemptMs, uint8_t maxAttempts)
{
  int fd = client.fd();
  if (fd < 0 || topic == nullptr) return false;
  size_t topicLen = strlen(topic);
  if (topicLen == 0 || topicLen > 250) return false; // marge large, largeur remaining-length 1 octet suffisante

  // Variable header (ID paquet, QoS1 impose par le protocole pour un
  // SUBSCRIBE meme si $qos demande=0, meme convention que PubSubClient::
  // subscribe() -- MQTTSUBSCRIBE|MQTTQOS1 code en dur cote appelant) +
  // payload (longueur topic 2 octets + topic + 1 octet qos).
  uint16_t msgId = g_fastSubMsgId++;
  if (g_fastSubMsgId == 0) g_fastSubMsgId = 1;

  uint8_t remLen = (uint8_t)(2 + 2 + topicLen + 1); // < 128, pas besoin du codage multi-octets
  uint8_t buf[8 + 256];
  size_t pos = 0;
  buf[pos++] = 0x82; // type=8 (SUBSCRIBE), flags=0010 (QoS1 obligatoire)
  buf[pos++] = remLen;
  buf[pos++] = (uint8_t)(msgId >> 8);
  buf[pos++] = (uint8_t)(msgId & 0xFF);
  buf[pos++] = (uint8_t)(topicLen >> 8);
  buf[pos++] = (uint8_t)(topicLen & 0xFF);
  memcpy(buf + pos, topic, topicLen);
  pos += topicLen;
  buf[pos++] = qos;

  size_t sent = 0;
  int lastErrno = 0;
  int lastSelectRc = 0;
  int lastSelectErrno = 0;
  for (uint8_t attempt = 0; attempt < maxAttempts && sent < pos; attempt++)
  {
    if (!socketWritableQuick(fd, perAttemptMs, &lastSelectRc, &lastSelectErrno)) continue; // pas ecrit-pret cette fenetre, on retente (budget court)
    int res = send(fd, buf + sent, pos - sent, MSG_DONTWAIT);
    if (res > 0) sent += (size_t)res;
    else if (res < 0)
    {
      lastErrno = errno;
      if (errno != EAGAIN && errno != EWOULDBLOCK) break; // socket vraiment casse, inutile d'insister
    }
  }
  // v140 -- diagnostic root-cause (voir changelog v140) : si l'envoi echoue
  // (jamais ecrit-pret dans le budget imparti), interroge SO_ERROR --
  // erreur socket PENDANTE que ni select() ni un send() EAGAIN ne
  // remontent autrement (getsockopt() la lit ET la remet a zero). Objectif
  // : voir si le noyau/lwIP sait DEJA que la connexion est morte (ECONNRESET,
  // ETIMEDOUT...) au moment ou select() refuse "ecrit-pret", ou si le socket
  // se pretend sain (SO_ERROR=0) alors qu'il ne progresse jamais -- ces 2 cas
  // pointent vers des causes tres differentes (etat socket connu vs
  // vraiment bloque sans raison visible du cote applicatif).
  if (sent != pos)
  {
    int soErr = -1;
    socklen_t soErrLen = sizeof(soErr);
    int gsoRc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &soErr, &soErrLen);
    // v143 -- fd= ajoute (voir entete changelog) : les fd POSIX/lwIP sont
    // normalement recycles au plus bas numero libre -- si cette valeur ne
    // fait QUE croitre au fil d'une session sans jamais redescendre, c'est
    // la preuve directe d'une fuite de socket (jamais ferme correctement
    // quelque part dans le cycle connect/subscribe/disconnect).
    Serial.println("[MQTT] mqttSubscribeFast ECHEC -> " + String(topic)
                   + " sent=" + String((unsigned)sent) + "/" + String((unsigned)pos)
                   + " SO_ERROR=" + String(soErr) + "(gso_rc=" + String(gsoRc) + ")"
                   + " lastErrno=" + String(lastErrno)
                   + " lastSelectRc=" + String(lastSelectRc) + " lastSelectErrno=" + String(lastSelectErrno)
                   + " fd=" + String(fd));
  }
  return sent == pos;
}


struct MqttCommand
{
  enum Type { CMD_NONE, CMD_STOP, CMD_DEFAULT, CMD_SYSTEM, CMD_GAME,
              CMD_STARTCLIP, CMD_RESUMESYS, CMD_SHOW_CONFIG, CMD_WIFI_RECOVERY,
              CMD_REBOOT, CMD_WAITING_MQTT, CMD_BRIGHTNESS, CMD_CLOCK_PREVIEW,
              CMD_BRIGHTNESS_UP, CMD_BRIGHTNESS_DOWN,
              CMD_SCORE /* v110 -- reintroduit, voir entete changelog */ };
  Type   type;
  String arg;
  MqttCommand() : type(CMD_NONE), arg("") {}
  MqttCommand(Type t, const String &a) : type(t), arg(a) {}
};

SemaphoreHandle_t mqttCmdMutex   = nullptr;
MqttCommand       pendingCmd;
TaskHandle_t      mqttTaskHandle = nullptr;

// v144 -- wifiResetMutex : protege TOUTE sequence WiFi.disconnect()/
// WiFi.begin() contre un acces concurrent entre maintainWiFi() (tache
// loop(), coeur 1, appelee a chaque tour) et l'escalade WiFi ajoutee dans
// mqttTask() (coeur 0, voir WIFI_RESET_ESCALATION_THRESHOLD). Motif deja
// identifie et documente (v132/v133 revertes en v134, voir son
// commentaire complet pres de SUBSCRIBE_BACKOFF_MAX_MS) : ces 2 appels
// n'etaient pas thread-safe l'un envers l'autre, une collision pouvait
// corrompre l'etat interne du driver WiFi (observe : WiFi lui-meme
// decrochant en boucle serree, pire que le probleme d'origine). Cette
// fois, les 2 call sites prennent ce mutex (timeout court, non bloquant
// pour l'appelant qui abandonne simplement ce tour si l'autre l'a deja)
// avant de toucher WiFi.disconnect()/begin()/setSleep(), au lieu de
// retirer l'escalade comme l'avait fait v134.
SemaphoreHandle_t wifiResetMutex = nullptr;

// v143 -- compteur PERSISTANT (jamais remis a zero, contrairement a
// consecutiveSubscribeFailCycles/wifiDownStreak qui sont locaux a
// mqttTask() et se remettent a zero des qu'un cycle reussit) du nombre
// total de tentatives connect() depuis le boot -- objectif : distinguer
// si la rechute du blocage subscribe()/connect() (delai variable observe,
// 2026-09-02 : 16min a 72min selon les sessions) correle avec un NOMBRE de
// cycles ecoules (evocateur d'une fuite de ressource qui s'accumule a
// chaque cycle, ex. socket/fd jamais ferme) plutot qu'avec le temps reel
// ecoule (evocateur d'un facteur externe base sur une horloge -- bail
// DHCP, timeout AP, etc.). Loggue sur chaque connect() (reussi ou non) et
// sur chaque echec definitif de cycle subscribe(), voir mqttTask().
uint32_t g_totalConnectAttempts = 0;

// v142 -- BUG REEL confirme par lecture de code (DECISIONS.md, "angle mort
// du filet de securite retour playlist", 31/08 soir) : le filet
// MQTT_OFFLINE_FALLBACK_MS mesurait le temps depuis le dernier connect()
// REUSSI (lastMqttConnectedMs, local a mqttTask()), remis a zero a CHAQUE
// connect() reussi meme si les subscribe() qui suivent echouent tous en
// boucle -- dans le pattern documente (connect() reussit toutes les ~89s
// mais aucun subscribe() n'aboutit), ce chrono n'atteignait donc jamais son
// seuil, le DMD pouvant rester bloque a boucler sur un vieux marquee
// indefiniment sans jamais retomber en playlist. Fix : nouveau timestamp
// dedie a l'activite MQTT reellement UTILE (pas juste "connect() a reussi")
// -- mis a jour (a) a chaque cycle de subscribe() effectivement reussi (voir
// mqttTask(), consecutiveSubscribeFailCycles=0) et (b) a chaque message MQTT
// RECU (onMqttMessage(), preuve la plus directe que le lien fonctionne dans
// les 2 sens). Le filet (voir mqttTask()) verifie desormais ce timestamp-ci
// plutot que lastMqttConnectedMs. Global (pas local a mqttTask()) car
// onMqttMessage() doit pouvoir l'ecrire -- ecrit uniquement depuis le
// contexte de mqttTask() (mqttClient.loop() appelle onMqttMessage() de
// facon synchrone dans la meme tache, voir son appel dans mqttTask()), donc
// aucune concurrence inter-tache reelle malgre les 2 sites d'ecriture.
unsigned long g_lastMqttUsefulMs = 0;

// v96 -- BUG REEL confirme sur materiel (CMD_GAME_DEBUG_LOGS active, hash ELF
// verifie) : pendingCmd (slot unique partage par TOUTES les commandes)
// perdait silencieusement un message si un AUTRE type de commande arrivait
// avant que loop() ait consomme le precedent -- confirme en conditions
// reelles : juste apres une reconnexion MQTT, une rafale de messages retenus
// (default+system+game+game_info+ingame en <1s) faisait ecraser "game"
// (jamais consomme, aucun [DIAG] log meme avec CMD_GAME_DEBUG_LOGS actif) par
// "game_info" arrive ~0.2s plus tard -- le marquee du jeu restait donc
// bloque sur l'ecran "RB connectee" meme apres une reconnexion reussie,
// alors que les messages suivants (game_info/ingame) etaient traites
// normalement (d'ou l'overlay a jour sur un fond perime).
// Remplacer pendingCmd par une queue/tableau generique a ete deliberement
// ECARTE (voir memoire projet dev-mame-score-mqtt-bridge,
// project_marquee_heartbeat_rederivation_bug) : un tableau de flags
// "commande en attente" indexe par type (g_cmdPending[CMD_STARTCLIP]) y a
// ete fortement suspecte de corruption memoire (ecriture hors-limites
// ailleurs dans le firmware atterrissant sur cette zone adjacente), causant
// des commandes PERIMEES a se declencher en pleine partie active -- sur le
// MEME plateau heap bas (maxalloc 4596-12000) que celui observe ici, jamais
// elucide, chantier explicitement abandonne sur cette piste (v110 source
// exclu pour cette raison). Fix retenu a la place, pattern deja eprouve
// ailleurs dans ce fichier (g_noWifiRecalboxPending, g_achievementPendingShow,
// etc., de simples booleens nommes, jamais un tableau) : uniquement les 4
// commandes qui definissent "ce qui doit etre affiche" recoivent chacune sa
// PROPRE variable dediee (pas un tableau indexe par type -- 4 paires
// bool+String nommees individuellement) -- elles ne peuvent donc plus jamais
// etre ecrasees par un type de commande DIFFERENT. score/game_info/
// achievement/brightness/etc. restent sur pendingCmd (perte occasionnelle
// deja acceptee, aucun bug confirme les concernant a ce jour).
bool   g_pendingDefault   = false;
bool   g_pendingSystem    = false;  String g_pendingSystemArg = "";
bool   g_pendingGame      = false;  String g_pendingGameArg   = "";
// v104 -- g_pendingIngame/g_pendingGameInfo retires (CMD_INGAME/CMD_GAME_INFO
// n'existent plus, voir suppression du sous-systeme hi-score/overlay).
//
// v122 -- marquee/cmd/ingame REABONNE (retour utilisateur : verifier la
// resynchro DMD au demarrage/reconnexion, cf. DECISIONS.md) -- MAIS sans
// raccrocher l'ancien sous-systeme overlay hi-score retire en v104 : ce
// simple booleen ne sert qu'a savoir si RB affirme etre reellement EN JEU
// au moment present (retenu, publie par marquee.sh a chaque rungame/
// endgame), pour gater CMD_DEFAULT (voir son case) et eviter de retomber en
// playlist locale sur un "default" perime/errone recu pendant qu'une vraie
// partie est en cours (bug reel identifie : marquee.sh v16 et anterieurs
// forcaient "default" sans condition dans son handler start), qui tourne a
// chaque redemarrage d'ES, y compris pendant une partie en cours -- corrige
// cote script en v17, ce flag cote DMD est une 2e ligne de defense). Pas de
// slot dedie type g_pendingGame : simple etat courant (pas une "commande a
// executer"), ecrit directement depuis onMqttMessage().
bool   g_recalboxInGame   = false;

// Pont pour web_config.h (v72) : #include "web_config.h" a lieu AVANT la
// definition du type MqttCommand/pendingCmd ci-dessus (ligne 1241) -- cette
// fonction permet au handler web /clock-preview de poser une commande
// CMD_CLOCK_PREVIEW sans exposer le type MqttCommand a web_config.h (juste
// son prototype, voir extern en tete de web_config.h).
void requestClockPreview(const String &arg)
{
  pendingCmd = MqttCommand(MqttCommand::CMD_CLOCK_PREVIEW, arg);
}

// Declaration anticipee (v72) : showClock() est definie plus bas (pres de
// loop(), son seul appelant jusqu'ici) mais processPendingMqttCommand()
// (CMD_CLOCK_PREVIEW, avant la definition dans l'ordre du fichier) doit
// desormais l'appeler aussi -- la generation automatique de prototype
// d'Arduino ne gere pas correctement l'argument par defaut ajoute a cette
// signature, d'ou cette declaration manuelle.
static bool showClock(int forceTheme = -2);

#define MQTT_LOG_SIZE 10
struct MqttLogEntry { String topic; String msg; unsigned long ts; };
MqttLogEntry mqttLog[MQTT_LOG_SIZE];
int mqttLogHead  = 0;
int mqttLogCount = 0;

void mqttLogAdd(const String &topic, const String &msg)
{
  mqttLog[mqttLogHead] = { topic, msg, millis() };
  mqttLogHead = (mqttLogHead + 1) % MQTT_LOG_SIZE;
  if (mqttLogCount < MQTT_LOG_SIZE) mqttLogCount++;
}

// v155 -- endpoint HTTP /log (v153/v154, surveillance WiFi temporaire)
// RETIRE : webServer->send() ici emprunte le meme chemin d'ecriture reseau
// (NetworkClient::write(), retry interne non configurable) deja identifie
// comme source de blocage pour les pages web_config (v48-v50) ET pour le
// paquet CONNECT MQTT (PubSubClient, v152) -- voir commentaire v62 pres de
// webServer->handleClient() dans loop() : un handler HTTP qui bloque y
// bloque TOUTE l'iteration loop(), donc maintainWiFi() et
// processPendingMqttCommand() avec lui. Un client qui stagne en cours de
// reponse (WiFi degrade -- exactement le moment ou on surveillerait via cet
// endpoint) pouvait donc aggraver activement le probleme qu'il servait a
// observer. Retire le 2026-09-04 sur decision utilisateur apres cette
// analyse (deja marque a retirer avant production dans DECISIONS.md,
// avance ici). mqttLog[]/mqttLogAdd()/MQTT_LOG_SIZE conserves (utiles
// independamment, purement passifs -- aucun cout reseau).

// --------------------------------------------------
// Helpers
// --------------------------------------------------
String getPlaylistLabel()
{
  String label = playlistName;
  int slash = label.lastIndexOf('/'); if (slash >= 0) label = label.substring(slash + 1);
  int dot   = label.lastIndexOf('.'); if (dot > 0)   label = label.substring(0, dot);
  label.trim();
  if (label.length() == 0) label = "UNKNOWN";
  return label;
}

String fitLabel(String s, int maxChars)
{
  s.trim();
  if ((int)s.length() <= maxChars) return s;
  if (maxChars <= 3) return s.substring(0, maxChars);
  return s.substring(0, maxChars - 3) + "...";
}

String extractField(const String &msg, const String &key)
{
  int idx = msg.indexOf(key + "="); if (idx < 0) return "";
  int start = idx + key.length() + 1;
  int end   = msg.indexOf(' ', start); if (end < 0) end = msg.length();
  return msg.substring(start, end);
}

// --------------------------------------------------
// Affichage
// --------------------------------------------------
void showMessage(const String &line1, const String &line2, uint16_t color = 0xFFE0)
{
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  display->setTextColor(color);
  display->setCursor(1, 6);  display->print(line1);
  display->setCursor(1, 18); display->print(line2);
}

void showPlaylistInfoScreen()
{
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  display->setTextColor(display->color565(235, 235, 235));
  display->setCursor(1, 5);  display->print(fitLabel(getPlaylistLabel(), 18));
  display->setTextColor(display->color565(255, 210, 70));
  display->setCursor(1, 18); display->print(String(gifCount) + " GIFS");
}

void drawWifiIconSmall(int x, int y, uint16_t color)
{
  display->drawPixel(x+4,y+8,color); display->drawLine(x+2,y+6,x+6,y+6,color);
  display->drawPixel(x+1,y+4,color); display->drawPixel(x+7,y+4,color);
  display->drawLine(x+2,y+3,x+6,y+3,color);
  display->drawPixel(x+0,y+1,color); display->drawPixel(x+8,y+1,color);
  display->drawLine(x+1,y+0,x+7,y+0,color);
}

void drawBluetoothIconSmall(int x, int y, uint16_t color)
{
  display->drawLine(x+4,y+0,x+4,y+8,color);
  display->drawLine(x+4,y+4,x+7,y+1,color); display->drawLine(x+4,y+0,x+7,y+3,color);
  display->drawLine(x+4,y+4,x+7,y+7,color); display->drawLine(x+4,y+8,x+7,y+5,color);
  display->drawLine(x+1,y+2,x+4,y+4,color); display->drawLine(x+1,y+6,x+4,y+4,color);
}

void showWifiStatusScreen(const String &line1, const String &line2, uint16_t color)
{
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  drawWifiIconSmall(3, 10, color); display->setTextColor(color);
  display->setCursor(18, 6);  display->print(line1);
  display->setCursor(18, 18); display->print(line2);
}

void showBluetoothStatusScreen(bool enabled)
{
  uint16_t color = enabled ? display->color565(80,170,255) : display->color565(255,0,0);
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  drawBluetoothIconSmall(3,10,color); display->setTextColor(color);
  display->setCursor(18, 6);  display->print("BT");
  display->setCursor(18, 18); display->print(enabled ? "ON" : "OFF");
}

void drawHourglassTallFancy(int x, int y, int w, int h, uint8_t phase)
{
  uint16_t borderOuter=display->color565(60,90,130);
  uint16_t borderInner=display->color565(170,220,255);
  uint16_t capColor   =display->color565(110,150,200);
  uint16_t sandColor  =display->color565(255,210,70);
  uint16_t sandGlow   =display->color565(255,235,140);
  uint16_t shadowColor=display->color565(25,30,40);

  int cx=x+w/2, topY=y, botY=y+h-1, neckY=y+h/2;
  display->drawRect(x,y,w,h,borderOuter);
  display->drawRect(x+1,y+1,w-2,h-2,shadowColor);
  display->drawLine(x+3,topY+3,x+w-4,topY+3,capColor);
  display->drawLine(x+3,botY-3,x+w-4,botY-3,capColor);
  display->drawLine(x+4,topY+4,cx,neckY-1,borderInner);
  display->drawLine(x+w-5,topY+4,cx,neckY-1,borderInner);
  display->drawLine(cx,neckY+1,x+4,botY-4,borderInner);
  display->drawLine(cx,neckY+1,x+w-5,botY-4,borderInner);

  int topFill,bottomFill;
  switch(phase&7){
    case 0:topFill=10;bottomFill=2;break; case 1:topFill=9;bottomFill=3;break;
    case 2:topFill=8;bottomFill=4;break;  case 3:topFill=7;bottomFill=5;break;
    case 4:topFill=6;bottomFill=6;break;  case 5:topFill=5;bottomFill=7;break;
    case 6:topFill=4;bottomFill=8;break;  default:topFill=3;bottomFill=9;break;
  }
  int topBaseY=max(neckY-topFill,topY+5);
  display->fillTriangle(x+6,topBaseY,x+w-7,topBaseY,cx,neckY-2,sandColor);
  display->drawLine(x+7,topBaseY+1,x+w-8,topBaseY+1,sandGlow);
  int bottomApexY=min(neckY+bottomFill,botY-5);
  display->fillTriangle(x+6,botY-5,x+w-7,botY-5,cx,bottomApexY,sandColor);
  display->drawLine(x+7,botY-6,x+w-8,botY-6,sandGlow);

  uint8_t sm=phase&7;
  if(sm==0||sm==4){display->drawPixel(cx,neckY-1,sandGlow);display->drawPixel(cx,neckY,sandColor);display->drawPixel(cx,neckY+1,sandColor);display->drawPixel(cx,neckY+2,sandGlow);}
  else if(sm==1||sm==5){display->drawPixel(cx,neckY-1,sandGlow);display->drawPixel(cx,neckY,sandColor);display->drawPixel(cx,neckY+1,sandGlow);}
  else if(sm==2||sm==6){display->drawPixel(cx,neckY,sandColor);display->drawPixel(cx,neckY+1,sandGlow);}
  else{display->drawPixel(cx,neckY-1,sandColor);display->drawPixel(cx,neckY,sandGlow);display->drawPixel(cx,neckY+1,sandColor);}

  if((phase&1)==0) display->drawPixel(cx-1,bottomApexY+1,sandGlow);
  else             display->drawPixel(cx+1,bottomApexY+1,sandGlow);
}

// Petit sablier anime, coin superieur droit -- utilise pendant le boot quand
// info=0 (masque les ecrans de statut habituels: brightness, wifi, ntp...).
static void bootHourglassTick()
{
  static uint8_t frame = 0;
  drawHourglassTallFancy(108, 1, 16, 16, frame++);
}

void showLoadingHourglass(int count)
{
  static uint8_t frame=0; frame++;
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  display->setTextColor(display->color565(235,235,235));
  display->setCursor(2,3);  display->print("GIFS");
  display->setTextColor(display->color565(255,210,70));
  display->setCursor(2,13); display->print(count);
  display->setTextColor(display->color565(150,200,255));
  display->setCursor(2,24); display->print(fitLabel(getPlaylistLabel(),7));
  drawHourglassTallFancy(44,1,18,30,frame);
}

// --------------------------------------------------
// Bluetooth
// --------------------------------------------------
void setupBluetoothFromConfig()
{
  if (showInfo) showBluetoothStatusScreen(bluetoothEnabled);
  delay(1200);
  if (!bluetoothEnabled) { btStop(); esp_bt_mem_release(ESP_BT_MODE_BTDM); return; }
  SerialBT.begin(bluetoothName);
}

// --------------------------------------------------
// PNG â€” libere le cache bigramme avant de decoder
// pour donner la RAM a pngle
// --------------------------------------------------
void pngleDrawCallback(pngle_t *pngle, uint32_t x, uint32_t y,
                       uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  (void)pngle; (void)w; (void)h;
  if ((int)x >= (PANEL_RES_X * PANEL_CHAIN) || (int)y >= PANEL_RES_Y) return;
  display->drawPixel((int)x, (int)y, display->color565(rgba[0], rgba[1], rgba[2]));
}

static void logHeapCaps(const char *where)
{
  // largest free block in bytes (avoids confusion with total freeHeap)
  size_t largest8bit   = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  size_t largestInt    = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  size_t largestSpiRam = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

  Serial.println(String("[HEAP] ") + where +
                 " freeHeap=" + String(ESP.getFreeHeap()) +
                 " maxAlloc=" + String(ESP.getMaxAllocHeap()) +
                 " largest8bit=" + String((uint32_t)largest8bit) +
                 " largestInternal=" + String((uint32_t)largestInt) +
                 " psramFound=" + String(psramFound() ? "1" : "0") +
                 " freePsram=" + String(ESP.getFreePsram()) +
                 " largestSpiram=" + String((uint32_t)largestSpiRam));
}

static const int RAW565_W = PANEL_RES_X * PANEL_CHAIN; // 128
static const int RAW565_H = PANEL_RES_Y;               // 32

// ============================================
// safe-modify â€” Historique des modifications
// ============================================
// Version actuelle : v4
//
// v4 - 2026-07-28 - BRANCHE DEV : gifPlayFrameCompat()/openNextGif() (lecture
//   de frame/transition entre GIFs) protegees par une tentative NON bloquante
//   de sdAccessMutex -- une generation de playlist tourne desormais sur sa
//   propre tache FreeRTOS et peut tenir ce mutex plusieurs secondes sur un
//   dossier a lenteur SD localisee ; loop() ne doit jamais l'attendre de
//   facon bloquante (degrade gracieusement : frame maintenue a l'identique /
//   nouvelle tentative au tour suivant). Voir web_config.h (playlistGenTask()).
//
// v3 - 2026-06-29 - Correction freeze playlist: skipRawPack dans openGif()
// v2 â€” 2026-06-24 â€” Ajout sous-dossiers alphabÃ©tiques pour rÃ©soudre le ralentissement FAT32 sur 800+ fichiers (flag L). alphaSubdirPath() insÃ¨re un sous-dossier A..Z/# dans le chemin. drawRaw565() et openGif() tentent le sous-dossier en prioritÃ©.
// v1 â€” 2026-06-10 â€” CrÃ©ation initiale
// ============================================

static String pngToRaw565Path(const String &pngPath)
{
  if (pngPath.length() >= 4 && pngPath.endsWith(".png"))
    return pngPath.substring(0, pngPath.length() - 4) + ".raw565";
  // v93 -- BUG REEL confirme sur materiel : cette fonction est censee
  // recevoir un chemin .png, mais currentPngPath peut parfois deja contenir
  // un chemin .raw565 (ex. DEFAULT_RAW565_PATH, pose par CMD_WAITING_MQTT
  // comme placeholder "informatif" puis relu a tort par un redessin force,
  // cf. endOverlay()) -- l'ancien comportement (retourner pngPath+".raw565"
  // sans condition) produisait alors un chemin double-extension du genre
  // "default.raw565.raw565", forcement introuvable (log [PNG-RAW] MISSING),
  // qui declenchait toute une chaine de repli (~800ms perdus, tentative de
  // decodage PNG sur un fichier qui n'en est pas un) avant de se rattraper.
  // Fix : idempotent, ne rajoute jamais ".raw565" si deja present.
  if (pngPath.endsWith(".raw565")) return pngPath;
  return pngPath + ".raw565";
}

// --------------------------------------------------
// Sous-dossiers alphabÃ©tiques A..Z et #
// InsÃ¨re un sous-dossier dans le chemin pour diviser
// les gros rÃ©pertoires (800+ fichiers) en 27 petits.
//
// Exemples:
//   "/systems/nes/zeld.raw565"  -> "/systems/nes/Z/zeld.raw565"
//   "/systems/nes/alex.raw565"  -> "/systems/nes/A/alex.raw565"
//   "/systems/nes/123.raw565"   -> "/systems/nes/#/123.raw565"
//   "/systems/_defaults/nes.raw565" -> inchangÃ© (pas de sous-dossier pour _defaults)
//
// La fonction garde le chemin plat si le sous-dossier
// n'existe pas (compatibilitÃ© ascendante).
// --------------------------------------------------
static String alphaSubdirPath(const String &path)
{
  // Ne pas toucher Ã  _defaults/
  if (path.indexOf("/_defaults/") >= 0) return path;

  int lastSlash = path.lastIndexOf('/');
  if (lastSlash < 0) return path;

  String dir   = path.substring(0, lastSlash);
  String fname = path.substring(lastSlash + 1);
  if (fname.length() == 0) return path;

  char first = (char)toupper((unsigned char)fname.charAt(0));
  String subdir;
  if (isAlpha(first)) {
    subdir = String(first);
  } else {
    subdir = "#";
  }

  return dir + "/" + subdir + "/" + fname;
}

static uint16_t *raw565FullBuf = nullptr;

// CMD_GAME_MIN_HEAP_FOR_FILE_OPEN deplacee plus haut dans le fichier en v81
// (2026-08-17) -- voir sa declaration pres de loadBigramTable(), qui en a
// desormais besoin AVANT ce point du fichier (variable globale, pas de
// forward-declaration possible contrairement aux fonctions).

// Cache RAM du fallback /systems/_defaults/default.raw565 (8KB)
static uint16_t *defaultRaw565Buf = nullptr;
static bool defaultRaw565Cached = false;
static const char *DEFAULT_RAW565_PATH = "/systems/_defaults/default.raw565";
// v107 -- animation "coupe-circuit anti-rafale" (voir memoire projet) :
// raw565pack dedie, fixe, pre-fabrique (statique/interference CRT), affiche
// en boucle pendant une rafale de navigation cote RB (marquee.sh) au lieu de
// laisser le dernier marquee reel fige. Chemin fixe sous _defaults (pas de
// sous-dossier alphabetique, meme convention que default.raw565) -- fichier
// a copier manuellement sur la carte SD (voir memoire projet pour le detail
// de generation de l'asset).
// v108 -- BUG REEL corrige AVANT tout test materiel (relecture ouGifImpl()
// suite a une question utilisateur) : ce chemin doit se terminer en ".gif",
// meme si aucun .gif reel n'existe. openGif()/openGifImpl() derive TOUJOURS
// les noms reels via gifToRaw565PackPath()/gifToRaw565MetaPath(), qui font
// une simple substitution de suffixe ".gif"->".raw565pack"/".meta" ; sans
// suffixe ".gif" en entree elles concatenent au lieu de substituer, ce qui
// cherchait a tort "_shuffle.raw565pack.raw565pack" / ".raw565pack.meta"
// (fichiers inexistants) au lieu de "_shuffle.raw565pack" / "_shuffle.meta"
// (fichiers reels). L'ouverture raw565pack echouait alors systematiquement,
// et comme _defaults est "raw-only strict" (refus sans fallback .gif, voir
// openGifImpl()), le tout aurait echoue silencieusement. Meme convention
// que partout ailleurs dans ce fichier (ex: openGif(gameGif, ...)) : le
// nom en .gif est une simple cle, jamais lu directement hors mode playlist.
static const char *SHUFFLE_GIF_PATH = "/systems/_defaults/_shuffle.gif";
// v109 -- desactivation demandee (2026-08-19) : test en cours pour isoler
// si le rc=-4 (chasse depuis plusieurs sessions) revient a vitesse de
// defilement max SANS aucune protection anti-rafale (ni cote RB -- voir
// marquee.sh v11, BURST_THRESHOLD releve a 50 -- ni cote firmware), main-
// tenant que l'overclock RPi5 suspecte comme cause principale a ete retire
// (voir memoire projet). Code garde intact (meme motif que
// CMD_GAME_DEBUG_LOGS) -- juste desactive, pas retire, reactivable en un
// mot (repasser a true) si le test tourne mal.
// v121 -- REACTIVATION (2026-08-22) : marquee.sh v13 remet le coupe-circuit
// anti-rafale en service cote RB (BURST_THRESHOLD=10, rc=-4 confirme sans
// lien avec ce mecanisme, voir memoire projet) et publie donc de nouveau
// "!SHUFFLE" en rafale. Laisser ce flag a false pendant que le script
// publiait "!SHUFFLE" reproduisait exactement l'echec predit par le
// commentaire ci-dessous (sysName=romName="!SHUFFLE" -> systeme introuvable
// -> echec silencieux) : ecran noir signale par l'utilisateur au lieu du
// GIF shuffle attendu.
const bool SHUFFLE_ENABLED = true;

static bool ensureDefaultRaw565Cached()
{
  if (defaultRaw565Cached) return true;

  // Diagnostic ajoute (bug remonte : ecran DMD noir/vide apres
  // CMD_WAITING_MQTT) -- cette fonction etait entierement silencieuse sur
  // ses 3 chemins d'echec, impossible de savoir depuis le log lequel se
  // produisait.
  const size_t totalBytes = (size_t)RAW565_W * (size_t)RAW565_H * sizeof(uint16_t);
  if (!defaultRaw565Buf)
  {
    defaultRaw565Buf = (uint16_t*)malloc(totalBytes);
    if (!defaultRaw565Buf) { Serial.println("[CACHE] default.raw565 malloc FAIL"); return false; }
  }

  File f = SD.open(DEFAULT_RAW565_PATH, FILE_READ);
  if (!f) { Serial.println("[CACHE] default.raw565 open FAIL " + String(DEFAULT_RAW565_PATH)); return false; }

  size_t gotAll = f.read((uint8_t*)defaultRaw565Buf, totalBytes);
  f.close();

  if (gotAll != totalBytes) { Serial.println("[CACHE] default.raw565 read incomplete " + String(gotAll) + "/" + String(totalBytes)); return false; }

  defaultRaw565Cached = true;
  return true;
}

static bool drawDefaultRaw565Cached()
{
  if (!ensureDefaultRaw565Cached()) return false;

  for (int y = 0; y < RAW565_H; y++)
    display->drawRGBBitmap(0, y, defaultRaw565Buf + (size_t)y * RAW565_W, RAW565_W, 1);

  return true;
}

static bool drawRaw565(const String &rawPath)
{
  // Garde-fou heap bas (2026-08-09, v62) -- CENTRALISE ici plutot qu'au
  // niveau de chaque appelant : un crash reel confirme (abort() dans
  // make_shared<VFSFileImpl>, RecalBox_DMD.ino:2197 avant ce fix) est
  // survenu via l'appel PAR LE MASK SYSTEME (CMD_GAME, "maskRaw565" avant
  // meme la logique 'B'/'g'/'p' plus bas dans la meme fonction) -- un site
  // d'appel non couvert par le garde-fou v60/v61 (qui ne protegeait que le
  // dispatch 'B'/'g'/'p'). drawRaw565() a plusieurs appelants (mask
  // systeme, repli 'B', repli 'g' apres echec raw565pack) -- verifier ici,
  // une seule fois, protege TOUS les appelants au lieu de dupliquer la
  // verification a chaque site (et d'en oublier). Tous les appelants
  // existants geraient deja un retour false gracieusement (voir leurs
  // branches "else" respectives), donc aucun changement de comportement
  // cote appelant necessaire.
  if (ESP.getMaxAllocHeap() < CMD_GAME_MIN_HEAP_FOR_FILE_OPEN) {
    Serial.println("[PNG-RAW] drawRaw565 heap trop bas (maxalloc=" + String(ESP.getMaxAllocHeap())
                   + ") -> abandon avant open t=" + String(millis()));
    return false;
  }
  // Essayer d'abord le chemin avec sous-dossier alphabÃ©tique
  String subPath = alphaSubdirPath(rawPath);
  File f = SD.open(subPath.c_str(), FILE_READ);

  // Si le sous-dossier n'existe pas, essayer le chemin plat (compatibilitÃ© ascendante)
  if (!f) {
    f = SD.open(rawPath.c_str(), FILE_READ);
  }
  if (!f) return false;

  const size_t rowBytes   = (size_t)RAW565_W * sizeof(uint16_t);
  const size_t totalBytes = rowBytes * (size_t)RAW565_H;

  // 1 seul gros read au lieu de 32 reads: Ã©vite le jitter SD.
  if (!raw565FullBuf)
  {
    raw565FullBuf = (uint16_t*)malloc(totalBytes);
    if (!raw565FullBuf)
    {
      // fallback: ancien comportement (lecture ligne par ligne)
      uint16_t row[RAW565_W];
      for (int y = 0; y < RAW565_H; y++)
      {
        size_t got = f.read((uint8_t*)row, rowBytes);
        if (got != rowBytes)
        {
          f.close();
          return false;
        }
        display->drawRGBBitmap(0, y, row, RAW565_W, 1);
      }
      f.close();
      return true;
    }
  }

  size_t gotAll = f.read((uint8_t*)raw565FullBuf, totalBytes);
  if (gotAll != totalBytes)
  {
    f.close();
    return false;
  }

  for (int y = 0; y < RAW565_H; y++)
    display->drawRGBBitmap(0, y, raw565FullBuf + (size_t)y * RAW565_W, RAW565_W, 1);

  f.close();
  return true;
}

bool drawPng(const String &path)
{
  if (nextGifFile)   { nextGifFile.close();   nextGifFile   = File(); }
  if (idxFileHandle) { idxFileHandle.close();  idxFileHandle = File(); }

  // 1) Tentative rapide: afficher *.raw565 si prÃ©sent
  String raw565Path = pngToRaw565Path(path);
  Serial.println("[PNG-RAW] drawRaw565 try raw565=" + raw565Path + " t=" + String(millis()));
  bool rawDrawn = drawRaw565(raw565Path);
  Serial.println(String("[PNG-RAW] drawRaw565 done raw565=") + raw565Path + " ok=" + (rawDrawn ? "1" : "0") + " t=" + String(millis()));

  if (rawDrawn)
  {
    Serial.println("[PNG-RAW] OK path=" + path + " raw565=" + raw565Path + " t=" + String(millis()));
    return true;
  }

  Serial.println("[PNG-RAW] MISSING raw565 path=" + path + " raw565=" + raw565Path);

  // 2) DÃ©terminer si le systÃ¨me est "L" (lenteur) ou "N" (rapide)
  auto extractSysNameFromSystemsPath=[&](const String &p)->String{
    if(!p.startsWith("/systems/")) return "";
    if(p.startsWith("/systems/_defaults/")) return "";
    int s0 = String("/systems/").length();
    int slash = p.indexOf('/', s0);
    if(slash < 0) return "";
    return p.substring(s0, slash);
  };

  String sysName = extractSysNameFromSystemsPath(path);

  // Bucket derive du nom de fichier (dernier segment de path, avec ou
  // sans extension -- seule la 1ere lettre compte) : flag lent par
  // sous-dossier alphabetique au lieu de par systeme entier (chantier
  // "bucket").
  int lastSlashForBucket = path.lastIndexOf('/');
  String fnameForBucket = (lastSlashForBucket >= 0) ? path.substring(lastSlashForBucket + 1) : path;
  char bucketLetter = bucketLetterForFilename(fnameForBucket);
  char slowFlag = sysBucketSlowFlag(sysName, bucketLetter);
  bool isSlow = (slowFlag == 'L' || slowFlag == 'l');

  Serial.println("[PNG-RAW] missing raw -> sysName=" + sysName + " bucket=" + String(bucketLetter) + " slowFlag=" + String(slowFlag) + " isSlow=" + String(isSlow));

  // 3) fallback "toujours rÃ©actif" si systÃ¨me lent: on n'essaie pas de dÃ©coder PNG
  String defPng = "/systems/_defaults/default.png";
  String defRaw565Path = pngToRaw565Path(defPng);

  if (isSlow)
  {
    Serial.println("[PNG-RAW] slow system -> fallback default raw drawRaw565 start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (slow skip png decode) for missing raw565 path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  // 4) SystÃ¨me rapide: on tente de dÃ©coder le PNG (si SD.open Ã©choue -> fallback default raw)
  freeBigramAll();

  if (nextGifFile)   { nextGifFile.close();   nextGifFile   = File(); }
  if (idxFileHandle) { idxFileHandle.close();  idxFileHandle = File(); }

  Serial.println("[PNG-RAW] fast system -> try png decode start path=" + path + " t=" + String(millis()));
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f)
  {
    Serial.println("[PNG-RAW] fast system -> SD.open png failed, fallback default raw start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (SD.open png failed) for path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    f.close();
    Serial.println("[PNG-RAW] fast system -> pngle_new failed, fallback default raw start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (pngle_new failed) for path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  pngle_set_draw_callback(pngle, pngleDrawCallback);

  uint8_t buf[256];
  bool ok = true;
  while (f.available())
  {
    int len = f.read(buf, sizeof(buf));
    if (len <= 0) break;
    if (pngle_feed(pngle, buf, len) < 0) { ok = false; break; }
  }

  pngle_destroy(pngle);
  f.close();

  if (!ok)
  {
    Serial.println("[PNG-RAW] fast system -> png decode failed, fallback default raw start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (png decode failed) for path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  Serial.println("[PNG-RAW] fast system -> png decode OK path=" + path + " t=" + String(millis()));
  return true;
}

// --------------------------------------------------
// PNG async (systemes "L") -> decode en tÃ¢che vers buffer RGB565
// puis blit depuis loop()
// --------------------------------------------------
static const int PNG_ASYNC_FB_W = 64 * 2; // PANEL_RES_X * PANEL_CHAIN = 128
static const int PNG_ASYNC_FB_H = 32;     // PANEL_RES_Y = 32
static const int PNG_ASYNC_FB_PIXELS = PNG_ASYNC_FB_W * PNG_ASYNC_FB_H;

static inline void pngleAsyncDrawCallback(pngle_t *p, uint32_t x, uint32_t y,
                                           uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  (void)p; (void)w; (void)h;
  if (!pngAsyncFb) return;
  if (x >= (uint32_t)PNG_ASYNC_FB_W || y >= (uint32_t)PNG_ASYNC_FB_H) return;
  // RGB565
  uint8_t r = rgba[0], g = rgba[1], b = rgba[2];
  uint16_t rgb565 = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  pngAsyncFb[y * PNG_ASYNC_FB_W + x] = rgb565;
}

static void blitPngAsyncFbToDisplay()
{
  if (!pngAsyncFb) return;
  // blit ligne par ligne (Ã©vite grosse allocation temporaire)
  for (int y = 0; y < PNG_ASYNC_FB_H; y++)
  {
    display->drawRGBBitmap(0, y, pngAsyncFb + (size_t)y * PNG_ASYNC_FB_W, PNG_ASYNC_FB_W, 1);
  }
}

static void pngAsyncDecodeTask(void *param)
{
  (void)param;

  uint32_t reqId = asyncPngActiveRequestId;
  String pathLocal = asyncPngPath;

  // reset
  asyncPngReady = false;
  asyncPngInProgress = true;

  Serial.println("[PNG-ASYNC] start reqId=" + String(reqId) + " path=" + pathLocal);
  Serial.println("[PNG-ASYNC] stackHighWater=" + String(uxTaskGetStackHighWaterMark(nullptr))
                 + " freeHeap=" + String(ESP.getFreeHeap()));

  // init buffer
  if (!pngAsyncFb)
  {
    pngAsyncFbPixels = PNG_ASYNC_FB_PIXELS;
    pngAsyncFb = (uint16_t*)malloc(pngAsyncFbPixels * sizeof(uint16_t));
  }

  if (!pngAsyncFb)
  {
    Serial.println("[PNG-ASYNC] malloc pngAsyncFb failed reqId=" + String(reqId)
                   + " freeHeap=" + String(ESP.getFreeHeap()));
    asyncPngInProgress = false;
    asyncPngReady = false;
    vTaskDelete(nullptr);
    return;
  }

  // DÃ©codage
  File f = SD.open(pathLocal);
  if (!f)
  {
    Serial.println("[PNG-ASYNC] SD.open failed reqId=" + String(reqId) + " path=" + pathLocal);
    asyncPngInProgress = false;
    asyncPngReady = false;
    vTaskDelete(nullptr);
    return;
  }

  // IMPORTANT: libÃ©rer la RAM AVANT de crÃ©er pngle (comme drawPng)
  // drawPng ferme aussi les fichiers pour maximiser le heap.
  freeBigramAll();

  if (nextGifFile)   { nextGifFile.close();   nextGifFile = File(); }
  if (idxFileHandle) { idxFileHandle.close();  idxFileHandle = File(); }

  Serial.println("[PNG-ASYNC] before pngle_new freeHeap=" + String(ESP.getFreeHeap()));
  Serial.println("[PNG-ASYYNC] largest8bit=" + String((uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    Serial.println("[PNG-ASYNC] pngle_new failed reqId=" + String(reqId)
                   + " freeHeap=" + String(ESP.getFreeHeap())
                   + " largest8bit=" + String((uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
    f.close();
    asyncPngInProgress = false;
    asyncPngReady = false;
    vTaskDelete(nullptr);
    return;
  }
  pngle_set_draw_callback(pngle, pngleAsyncDrawCallback);

  uint8_t buf[256];
  while (f.available())
  {
    if (asyncPngCancel || asyncPngActiveRequestId != reqId) break;
    int len = f.read(buf, sizeof(buf));
    if (len <= 0) break;
    if (pngle_feed(pngle, buf, len) < 0) break;
  }

  pngle_destroy(pngle);
  f.close();

  if (!asyncPngCancel && asyncPngActiveRequestId == reqId)
  {
    asyncPngReady = true;
    Serial.println("[PNG-ASYNC] ready reqId=" + String(reqId));
  }
  else
  {
    Serial.println("[PNG-ASYNC] not ready (cancel=" + String(asyncPngCancel ? "1" : "0")
                   + ", activeId=" + String(asyncPngActiveRequestId) + " reqId=" + String(reqId) + ")");
  }

  asyncPngInProgress = false;
  vTaskDelete(nullptr);
}

static void startAsyncPngDecodeIfNeeded(const String &path)
{
  // si on veut un PNG asynchrone mais dÃ©jÃ  lancÃ© pour le mÃªme path, on ne relance pas
  // (on utilise requestId pour simple tracking)
  if (asyncPngInProgress && asyncPngPath == path && asyncPngReady == false) return;

  // Annule lâ€™Ã©ventuelle tÃ¢che prÃ©cÃ©dente
  asyncPngCancel = true;
  delay(1);
  asyncPngCancel = false;

  asyncPngRequestId++;
  asyncPngActiveRequestId = asyncPngRequestId;
  asyncPngPath = path;
  asyncPngStartMs = millis();

  // IMPORTANT: Ã©viter la fenÃªtre oÃ¹ loop() relance des tÃ¢ches avant que la FreeRTOS task
  // ne passe Ã  son premier instruction. On marque "in progress" dÃ¨s maintenant.
  asyncPngReady = false;
  asyncPngInProgress = true;

  Serial.println("[PNG-ASYNC] scheduled reqId=" + String(asyncPngActiveRequestId)
                 + " inProgress=1 path=" + path);

  if (asyncPngTaskHandle) { asyncPngTaskHandle = nullptr; } // tÃ¢che gÃ©rÃ©e par vTaskDelete

  xTaskCreatePinnedToCore(pngAsyncDecodeTask, "pngAsyncDecode", 16384, nullptr, 1, &asyncPngTaskHandle, 1);
}

// --------------------------------------------------
// GIF callbacks
// --------------------------------------------------
void GIFDraw(GIFDRAW *pDraw)
{
  if (!display) return;
  uint8_t *s = pDraw->pPixels;
  int iWidth = pDraw->iWidth;
  if (iWidth > (PANEL_RES_X * PANEL_CHAIN)) iWidth = PANEL_RES_X * PANEL_CHAIN;
  int yOffset = (PANEL_RES_Y - pDraw->iHeight) / 2;
  int y = pDraw->iY + pDraw->y + yOffset;
  if (y < 0 || y >= PANEL_RES_Y) return;
  int xOffset = ((PANEL_RES_X * PANEL_CHAIN) - pDraw->iWidth) / 2;
  if (xOffset < 0) xOffset = 0;
  uint16_t usTemp[PANEL_RES_X * PANEL_CHAIN];
  for (int x = 0; x < iWidth; x++)
  {
    uint8_t idx = s[x];
    usTemp[x] = (idx == pDraw->ucTransparent && pDraw->ucHasTransparency)
                ? 0 : pDraw->pPalette[idx];
  }
  display->drawRGBBitmap(xOffset, y, usTemp, iWidth, 1);
}

void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  if (nextGifFile && String(fname) == nextGifPath)
  { nextGifFile.seek(0); gifFile = nextGifFile; nextGifFile = File(); }
  else gifFile = SD.open(fname);
  if (!gifFile) return nullptr;
  *pSize = gifFile.size();
  return (void *)&gifFile;
}

void GIFCloseFile(void *pHandle) { File *f=(File*)pHandle; if(f) f->close(); }

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t len)
{
  File *f=(File*)pFile->fHandle; if(!f) return 0;
  int32_t toRead=len;
  if((pFile->iSize-pFile->iPos)<len) toRead=pFile->iSize-pFile->iPos;
  if(toRead<=0) return 0;
  int32_t n=f->read(pBuf,toRead); pFile->iPos=f->position(); return n;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t position)
{
  File *f=(File*)pFile->fHandle; if(!f) return 0;
  f->seek(position); pFile->iPos=f->position(); return pFile->iPos;
}

static bool gifRawPackMode = false;
static uint32_t gifRawFrameIndex = 0;
static uint32_t gifRawFrameCount = 0;
static String gifRawPackPathCur = "";
static String gifRawMetaPathCur = "";

// Speed control for raw565pack playback:
// - 100 = normal speed
// - 50 = twice as fast
// - 25 = four times as fast
static const uint16_t GIF_RAW_PACK_SPEED_PERCENT = 50;
static const uint16_t GIF_RAW_PACK_MIN_DELAY_MS = 5;

static File gifRawPackFile;
static File gifRawMetaFile;
static const uint32_t RAW565_GIF_W = PANEL_RES_X * PANEL_CHAIN; // 128
static const uint32_t RAW565_GIF_H = PANEL_RES_Y;              // 32
static const uint32_t RAW565_GIF_FRAME_BYTES = RAW565_GIF_W * RAW565_GIF_H * 2;

static String gifToRaw565PackPath(const String &gifPath)
{
  if (gifPath.length() >= 4 && gifPath.endsWith(".gif"))
    return gifPath.substring(0, gifPath.length() - 4) + ".raw565pack";
  return gifPath + ".raw565pack";
}

static String gifToRaw565MetaPath(const String &gifPath)
{
  if (gifPath.length() >= 4 && gifPath.endsWith(".gif"))
    return gifPath.substring(0, gifPath.length() - 4) + ".meta";
  return gifPath + ".meta";
}

// Buffer cache pour tous les delays du meta file (lus en une fois a l'ouverture)
static uint16_t *gifRawDelayCache = nullptr;
static uint32_t  gifRawDelayCount = 0;

static void closeGifRawPackIfAny()
{
  if (gifRawPackFile) { gifRawPackFile.close(); }
  if (gifRawMetaFile) { gifRawMetaFile.close(); }
  if (gifRawDelayCache) { free(gifRawDelayCache); gifRawDelayCache = nullptr; }
  gifRawDelayCount = 0;
  gifRawPackFile = File();
  gifRawMetaFile = File();

  gifRawPackPathCur = "";
  gifRawMetaPathCur = "";
  gifRawFrameIndex = 0;
  gifRawFrameCount = 0;
  gifRawPackMode = false;
}

// Charge tous les delays du fichier meta en RAM a l'ouverture d'un raw565pack
static bool gifRawLoadMetaCache()
{
  if (gifRawDelayCache) free(gifRawDelayCache);
  gifRawDelayCache = nullptr;
  gifRawDelayCount = 0;
  if (!gifRawMetaFile) return false;
  size_t metaSize = gifRawMetaFile.size();
  if (metaSize < 2) return false;
  uint16_t count = metaSize / 2;
  gifRawDelayCache = (uint16_t*)malloc(metaSize);
  if (!gifRawDelayCache) return false;
  gifRawMetaFile.seek(0);
  size_t got = gifRawMetaFile.read((uint8_t*)gifRawDelayCache, metaSize);
  if (got < 2) { free(gifRawDelayCache); gifRawDelayCache = nullptr; return false; }
  gifRawDelayCount = count;
  return true;
}

static uint16_t gifRawReadDelayMs(uint32_t frameIndex)
{
  // Utiliser le cache RAM si disponible (plus de seek+read sur SD)
  if (gifRawDelayCache && frameIndex < gifRawDelayCount)
  {
    uint16_t ms = gifRawDelayCache[frameIndex];
    if (ms == 0) ms = GIF_RAW_PACK_MIN_DELAY_MS;
    ms = (uint16_t)((uint32_t)ms * GIF_RAW_PACK_SPEED_PERCENT / 100U);
    if (ms < GIF_RAW_PACK_MIN_DELAY_MS) ms = GIF_RAW_PACK_MIN_DELAY_MS;
    return ms;
  }
  // Fallback: lecture directe depuis le fichier meta
  if (!gifRawMetaFile) return 33;
  uint32_t metaPos = frameIndex * 2UL;
  gifRawMetaFile.seek(metaPos);
  uint16_t ms = 33;
  size_t got = gifRawMetaFile.read((uint8_t*)&ms, 2);
  if (got != 2) ms = 33;
  if (ms == 0) ms = GIF_RAW_PACK_MIN_DELAY_MS;
  ms = (uint16_t)((uint32_t)ms * GIF_RAW_PACK_SPEED_PERCENT / 100U);
  if (ms < GIF_RAW_PACK_MIN_DELAY_MS) ms = GIF_RAW_PACK_MIN_DELAY_MS;
  return ms;
}
// Buffer reusable pour la lecture bulk d'une frame raw565pack (8192 bytes)
// PartagÃ© avec drawRaw565() via raw565FullBuf
static uint16_t *gifRawFrameBuf = nullptr;

static void drawGifRaw565Frame(uint32_t frameIndex)
{
  if (!gifRawPackFile) return;

  const size_t frameBytes = RAW565_GIF_FRAME_BYTES; // 128*32*2 = 8192

  uint32_t baseOff = frameIndex * frameBytes;
  gifRawPackFile.seek(baseOff);

  // Utiliser raw565FullBuf s'il est dÃ©jÃ  allouÃ© (drawRaw565 l'alloue si besoin)
  if (!gifRawFrameBuf)
  {
    // Essayer raw565FullBuf d'abord (partagÃ© avec drawRaw565)
    if (raw565FullBuf)
    {
      gifRawFrameBuf = raw565FullBuf;
    }
    else
    {
      gifRawFrameBuf = (uint16_t*)malloc(frameBytes);
      if (!gifRawFrameBuf)
      {
        // Fallback: ancien comportement (lecture ligne par ligne)
        uint16_t row[RAW565_GIF_W];
        for (uint32_t y = 0; y < RAW565_GIF_H; y++)
        {
          size_t need = RAW565_GIF_W * 2UL;
          size_t got = gifRawPackFile.read((uint8_t*)row, need);
          if (got != need) break;
          display->drawRGBBitmap(0, (int)y, row, RAW565_GIF_W, 1);
        }
        return;
      }
    }
  }

  // 1 seul read bulk de toute la frame
  size_t gotAll = gifRawPackFile.read((uint8_t*)gifRawFrameBuf, frameBytes);
  if (gotAll != frameBytes) return;

  // Blit depuis RAM
  for (uint32_t y = 0; y < RAW565_GIF_H; y++)
    display->drawRGBBitmap(0, (int)y, gifRawFrameBuf + (size_t)y * RAW565_GIF_W, RAW565_GIF_W, 1);
}
// Lit/dessine une frame -- tourne sur loop() a CHAQUE frame affichee, donc
// c'est le point de contention le plus frequent avec playlistGenTask() (qui
// peut tenir sdAccessMutex plusieurs secondes sur un dossier a lenteur SD
// localisee). Tentative NON BLOQUANTE uniquement (voir le commentaire complet
// dans RecalBox_DMD.ino juste avant #include "web_config.h") : si le mutex
// est pris, on ne bloque jamais loop() pour l'attendre -- la frame courante
// reste affichee telle quelle quelques ms, puis loop() retente. Ne JAMAIS
// retourner false dans ce cas (serait interprete comme "GIF termine" par
// l'appelant et sauterait au suivant).
// sdAccessMutex retire entierement (2026-08-10, voir changelog v67 en tete
// de fichier) : playlistGenStep() tourne desormais dans loop(), meme
// contexte d'execution que cette fonction -- plus aucun acces SD concurrent
// entre 2 threads a proteger. Retour a la forme d'origine (avant le
// 2026-07-30, ancienne architecture "tache dediee" identifiee par
// bissection materielle comme cause d'un deadlock mqttTask/LWIP).
static bool gifPlayFrameCompat(bool first, int *pDelayMs)
{
  bool ok;
  if (gifRawPackMode)
  {
    if (first) gifRawFrameIndex = 0;
    if (gifRawFrameIndex >= gifRawFrameCount)
    {
      ok = false;
    }
    else
    {
      uint16_t ms = gifRawReadDelayMs(gifRawFrameIndex);
      *pDelayMs = (int)ms;
      drawGifRaw565Frame(gifRawFrameIndex);
      gifRawFrameIndex++;
      ok = true;
    }
  }
  else
  {
    ok = gif.playFrame(first, pDelayMs);
  }
  return ok;
}

static void gifResetCompat()
{
  if (gifRawPackMode)
  {
    gifRawFrameIndex = 0;
  }
  else
  {
    gif.reset();
  }
}

// v98 -- BUG REEL confirme sur materiel (4e site distinct de la MEME famille
// de crash deja corrigee 3x en v91/v95/v98 -- double-echec d'allocation dans
// SD.open()->make_shared<VFSFileImpl>, non rattrapable par try/catch) : cette
// fois via CMD_GAME (processPendingMqttCommand()) appelant directement
// openGif() pour charger le VRAI marquee du jeu (pas un prefetch), observe
// juste apres un reveil RB (heap sous pression transitoire). openGif()
// n'avait ICI AUCUN garde-fou. Un seuil heap type PREFETCH_NEXT_GIF_MIN_HEAP
// a ete ENVISAGE PUIS ECARTE : verification sur le log reel, maxalloc=4596
// (le plateau NORMAL de ce materiel) juste avant CE crash -- IDENTIQUE a
// des dizaines d'ouvertures REUSSIES la meme session. Un seuil a 8000
// bloquerait donc le chargement du marquee la plupart du temps (regression
// fonctionnelle majeure), pas seulement le cas rare de crash -- maxalloc
// n'est pas un predicteur fiable pour cette allocation precise. Fix retenu
// a la place : try/catch autour de la fonction (meme pattern deja etabli
// pour getNextGifRandom(), v78) -- n'empeche PAS le cas du double-echec
// total (rare, deja documente comme non rattrapable par le C++ runtime),
// mais capture le cas plus frequent d'un simple echec d'allocation isole
// (heap juste suffisant pour lever l'exception), sans aucun risque de faux
// positif/regression fonctionnelle contrairement a un seuil heuristique.
bool openGifImpl(const String &path, bool clearBefore, bool skipProbe, bool skipRawPack)
{
  if (!skipProbe)
  {
    // Essayer sous-dossier d'abord, puis plat
    String subPath = alphaSubdirPath(path);
    File p = SD.open(subPath.c_str(), FILE_READ);
    if (!p) {
      p = SD.open(path.c_str(), FILE_READ);
    }
    if (!p) return false;
    p.close();
  }

  // On ferme l'Ã©tat raw si on en avait un.
  closeGifRawPackIfAny();
  gifRawPackMode = false;
  gifOpened = false;

  // RULE:
  // - Si raw565pack + meta existent => on ouvre en raw565pack
  // - Sinon => fallback sur GIF standard (.gif)
  // - Cas spÃ©cifique masks _defaults: raw-only strict (pas de fallback GIF standard)
  bool isDefaults = (path.indexOf("/systems/_defaults/") >= 0);

  // -------- Raw565pack (si disponible) --------
  if (!skipRawPack)
  // skipRawPack=true : utilisÃ© par openNextGif() pour les GIFs de playlist (/gifs/...)
  // qui n'ont jamais de raw565pack. Ã‰vite 4 SD.open() Ã©chouant Ã  1-2s chacun -> 4-8s de freeze.
  if (!skipRawPack)
  // skipRawPack=true : utilisé par openNextGif() pour les GIFs de playlist (/gifs/...)
  // qui n'ont jamais de raw565pack. Évite 4 SD.open() échouant à 1-2s chacun -> 4-8s de freeze.
  if (!skipRawPack)
  // skipRawPack=true : utilisÃ© par openNextGif() pour les GIFs de playlist (/gifs/...)
  // qui n'ont jamais de raw565pack. Ã‰vite 4 SD.open() Ã©chouant Ã  1-2s chacun â†’ 4-8s de freeze.
  if (!skipRawPack)
  {
  String rawPack = gifToRaw565PackPath(path);
  String metaPath = gifToRaw565MetaPath(path);

  // Essayer d'abord le chemin avec sous-dossier alphabÃ©tique pour raw565pack+meta
  String subRawPack = alphaSubdirPath(rawPack);
  String subMetaPath = alphaSubdirPath(metaPath);

  {
    // Evite un pattern "probe" SD.exists()+SD.open() : on ouvre directement.
    if (clearBefore) display->clearScreen();

    // Tenter sous-dossier d'abord, puis plat (compatibilitÃ© ascendante)
    gifRawPackFile = SD.open(subRawPack.c_str(), FILE_READ);
    if (!gifRawPackFile) {
      gifRawPackFile = SD.open(rawPack.c_str(), FILE_READ);
    }

    gifRawMetaFile = SD.open(subMetaPath.c_str(), FILE_READ);
    if (!gifRawMetaFile) {
      gifRawMetaFile = SD.open(metaPath.c_str(), FILE_READ);
    }

      if (gifRawPackFile && gifRawMetaFile)
      {
        size_t packSize = gifRawPackFile.size();

        gif.close();
        gifOpened = true;
        gifRawPackMode = true;
        gifRawFrameIndex = 0;

        Serial.println("[GIF] open OK raw565pack req=" + path
                     + " rawPack=" + rawPack
                     + " meta=" + metaPath);

        gifRawPackPathCur = rawPack;
        gifRawMetaPathCur = metaPath;

        // Charger tous les delays en RAM pour eviter seek+read par frame
        gifRawLoadMetaCache();
        if (gifRawDelayCache) Serial.println("[GIF] meta delays en RAM: " + String(gifRawDelayCount) + " frames");

        gifRawFrameCount = (RAW565_GIF_FRAME_BYTES > 0) ? (packSize / RAW565_GIF_FRAME_BYTES) : 0;
        if (gifRawFrameCount == 0)
        {
          closeGifRawPackIfAny();
          gifOpened = false;
        }
        return gifOpened;
      }

      closeGifRawPackIfAny();
  }
  } // fin skipRawPack

  // raw-only strict: si ce sont des masks _defaults, on refuse sans fallback
  if (isDefaults)
  {
    gifRawPackMode = false;
    gif.close();
    gifOpened = false;
    return false;
  }

  // -------- Playlist => GIF standard --------
  if (clearBefore) display->clearScreen();
  gif.close();

  // AnimatedGIF::open signature:
  //   int open(const char *szFilename,
  //            GIF_OPEN_CALLBACK*,
  //            GIF_CLOSE_CALLBACK*,
  //            GIF_READ_CALLBACK*,
  //            GIF_SEEK_CALLBACK*,
  //            GIF_DRAW_CALLBACK*)
  // Ici on utilise nos callbacks SDFile via GIFOpenFile/GIFCloseFile/etc.
  gif.begin(LITTLE_ENDIAN_PIXELS);
  int rc = gif.open(path.c_str(), GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
  // AnimatedGIF::open / GIFInit() renvoie:
  //   1 = succÃ¨s (GIFInit OK)
  //   0 = Ã©chec
  if (rc == 1)
  {
    gifRawPackMode = false;
    gifOpened = true;
    Serial.println("[GIF] open OK standard path=" + path + " rc=" + String(rc));
    Serial.println("[GIF] apres open (avant 1ere frame), heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
    return true;
  }

  gifRawPackMode = false;
  gifOpened = false;
  Serial.println("[GIF] open FAIL standard path=" + path + " rc=" + String(rc));
  return false;
}

// v98 -- filet de securite EXCEPTION (voir commentaire complet pres de
// openGifImpl() ci-dessus) -- meme technique que getNextGifRandom() (v78).
bool openGif(const String &path, bool clearBefore=true, bool skipProbe=false, bool skipRawPack=false)
{
  try
  {
    return openGifImpl(path, clearBefore, skipProbe, skipRawPack);
  }
  catch (std::exception &e)
  {
    Serial.println(String("[GIF] EXCEPTION rattrapee dans openGif() (heap critique, maxalloc=")
                   + String(ESP.getMaxAllocHeap()) + ") : " + e.what());
    gifRawPackMode = false; gifOpened = false;
    return false;
  }
  catch (...)
  {
    Serial.println("[GIF] EXCEPTION inconnue rattrapee dans openGif() (maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
    gifRawPackMode = false; gifOpened = false;
    return false;
  }
}

// --------------------------------------------------
// openBestMedia
// --------------------------------------------------
DisplayMode openBestMedia(const String &basePath, const String &systemPath="")
{
  auto getSysName=[](const String &path)->String{
    if(!path.endsWith("/_default")) return "";
    int s2=path.lastIndexOf('/'); int s1=path.lastIndexOf('/',s2-1);
    if(s1<0) return ""; return path.substring(s1+1,s2);
  };
  auto getDP=[](const String &sysName)->String{return "/systems/_defaults/"+sysName;};
  auto openDP=[&](const String &sysName)->bool{
    String path=getDP(sysName)+".png";
    if(path==currentPngPath&&pngDrawn) return true;
    display->clearScreen();
    if(drawPng(path)){currentPngPath=path;pngDrawn=true;return true;}
    return false;
  };
  auto openDG=[&](const String &sysName)->bool{return openGif(getDP(sysName)+".gif", true, true);};

  bool isDefault=basePath.endsWith("/_default");
  if(!isDefault)
  {
    String path=basePath+".png";
    if(path==currentPngPath&&pngDrawn) return MODE_PNG;
    display->clearScreen();
    if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
    if(openGif(basePath+".gif", true, true)){pngDrawn=false;currentPngPath="";return MODE_GIF;}
  }
  else
  {
    String sysName=getSysName(basePath); char t=sysDefaultType(sysName);

    // Systemes _defaults : raw565 obligatoire, pas de raw565pack
    // drawPng() tente drawRaw565() puis fallback. Pas de openGif ici.
    if(drawPng(getDP(sysName)+".png")){currentPngPath=getDP(sysName)+".png";pngDrawn=true;return MODE_PNG;}
  }

  if(systemPath.length()>0)
  {
    String sysName=getSysName(systemPath); char t=sysDefaultType(sysName);

    // Ordre explicite selon le type cache:
    // p => PNG (raw565) d'abord
    // g => GIF (raw565pack) d'abord
    // B => GIF (raw565pack) d'abord, PNG (raw565) en repli si echec -- CORRIGE
    // (2026-07-28) : ce commentaire disait auparavant l'inverse ("PNG d'abord
    // puis GIF si echec"), ce qui ne correspondait pas au code juste en
    // dessous (repli sur B: on force raw565pack d'abord (openDG), meme si
    // raw565 existe) ni au meme choix applique pour B ailleurs dans ce
    // fichier (masque d'attente CMD_GAME lent, jeu lent) -- verifie par
    // lecture de code, aucun changement de comportement, uniquement le
    // commentaire qui etait faux.
    if(t=='g')
    {
      // D'abord vÃ©rifier si le PNG est dÃ©jÃ  affichÃ© et Ã  jour (Ã©vite openDG Ã  chaque loop)
      String path=getDP(sysName)+".png";
      if(path==currentPngPath&&pngDrawn) return MODE_PNG;

      if(openDG(sysName)){pngDrawn=false;currentPngPath="";return MODE_GIF;}
      display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
    }
    else if(t=='p')
    {
      String path=getDP(sysName)+".png";
      if(path==currentPngPath&&pngDrawn) return MODE_PNG;
      display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
      if(openDG(sysName)){pngDrawn=false;currentPngPath="";return MODE_GIF;}
    }
    else // B ou autre
    {
      // B : on force raw565pack d'abord (openDG), mÃªme si raw565 existe
      String path=getDP(sysName)+".png";
      if(path==currentPngPath&&pngDrawn) return MODE_PNG;  // Ã‰vite openDG() si le PNG est dÃ©jÃ  affichÃ©

      if(openDG(sysName)){pngDrawn=false;currentPngPath="";return MODE_GIF;}

      display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
    }
  }

  // Fallback final: forcer RAM default.raw565 (Ã©vite tout redÃ©codage PNG en loop())
  gif.close(); gifOpened = false;
  pngDrawn = true;
  currentPngPath = "";
  display->clearScreen();
  if(drawDefaultRaw565Cached()) return MODE_PNG;

  // Si (exceptionnel) la RAM default.raw565 n'est pas disponible, rebasculer sur l'ancien fallback
  char defType=sysDefaultType("default");
  if(defType!='p'&&openDG("default")){pngDrawn=false;currentPngPath="";return MODE_GIF;}
  if(defType!='g'){
    String path=getDP("default")+".png";
    if(path==currentPngPath&&pngDrawn) return MODE_PNG;
    display->clearScreen();
    if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
  }

  if(!pngDrawn&&!gifOpened){display->clearScreen();currentPngPath="";}
  return MODE_BLACK;
}

// --------------------------------------------------
// Playlist
// --------------------------------------------------
String getNextGifSequential()
{
  if(!seqPlaylistFile){seqPlaylistFile=SD.open(playlistCachePath,FILE_READ);if(!seqPlaylistFile)return "";}
  if(!seqPlaylistFile.available()){seqPlaylistFile.seek(0);playIndex=0;}
  while(seqPlaylistFile.available())
  {
    String line=seqPlaylistFile.readStringUntil('\n');line.trim();
    if(line.length()>0){playIndex++;return line;}
  }
  return "";
}

// v78 (2026-08-17) -- Port depuis dev/mame-score-mqtt-bridge (v99+v104+v105+
// v106, deja confirmes/ajustes plusieurs fois sur materiel reel dans cette
// branche source) : garde-fou heap pour le crash REEL confirme et decode via
// addr2line a plusieurs reprises (ELF verifie identique au binaire plante)
// -- getNextGifRandom() -> SD.open(playlistCachePath) -> fs::FS::open() ->
// operator new echoue si le heap contigu disponible est trop bas, exception
// non rattrapee -> abort()+reboot. Se produit typiquement juste apres boot
// (WiFi+MQTT+web fraichement inities, maxAllocHeap observe aussi bas que
// ~4.6 Ko a ce moment precis). Seuil OPEN_NEXT_GIF_MIN_HEAP=4000 : historique
// de calibrage complet (8000 essaye puis abandonne -- bloquait le plateau
// heap stable normal ~4596 en permanence, ecran noir indefini) deja dans la
// memoire projet source, pas rejoue ici. Le try/catch sur getNextGifRandom()
// est un filet de securite EXCEPTION en complement de ce seuil heuristique
// (pas garanti pour tous les fichiers -- crash reel observe a maxalloc=4596,
// DONC AU-DESSUS du seuil 4000) ; meme technique deja utilisee ailleurs dans
// ce fichier pour handleWebConfig(). Sur exception, retourne "" (meme
// comportement que les autres cas d'echec deja geres par les appelants).
const size_t OPEN_NEXT_GIF_MIN_HEAP = 4000;

// v95 -- seuil DEDIE, plus eleve, pour tout site de PREFETCH du GIF suivant
// (getNextGif() appele en dehors du chemin d'ouverture normal deja garde par
// OPEN_NEXT_GIF_MIN_HEAP ci-dessus) -- remonte en portee fichier en v97 car
// UN 2e site d'appel non protege a ete trouve (boucle de frame MODE_GIF dans
// loop(), distinct de celui dans openNextGif()) ; voir le commentaire complet
// pres du 1er site corrige (openNextGif(), v91/v95) pour le detail du crash.
const size_t PREFETCH_NEXT_GIF_MIN_HEAP = 8000;

String getNextGifRandom()
{
  if(gifCount<=0) return "";
  try
  {
    int idx=lastRandomIndex;
    if(gifCount>1){int t=0;while(idx==lastRandomIndex&&t<10){idx=random(0,gifCount);t++;}}
    else idx=0;
    lastRandomIndex=idx;
    if(!idxFileHandle){idxFileHandle=SD.open(playlistIdxPath,FILE_READ);if(!idxFileHandle)return getNextGifSequential();}
    idxFileHandle.seek((uint32_t)idx*4);
    uint32_t offset=0; idxFileHandle.read((uint8_t*)&offset,4);
    File cf=SD.open(playlistCachePath,FILE_READ); if(!cf) return "";
    cf.seek(offset); String line=cf.readStringUntil('\n'); cf.close(); line.trim();
    return line;
  }
  catch (std::exception &e)
  {
    Serial.println(String("[GIF] EXCEPTION rattrapee dans getNextGifRandom() (heap critique, maxalloc=")
                   + String(ESP.getMaxAllocHeap()) + ") : " + e.what());
    return "";
  }
  catch (...)
  {
    Serial.println("[GIF] EXCEPTION inconnue rattrapee dans getNextGifRandom() (maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
    return "";
  }
}

String getNextGif(){if(gifCount<=0)return "";return playlistRandom?getNextGifRandom():getNextGifSequential();}

// sdAccessMutex retire entierement (2026-08-10) : meme raison que
// gifPlayFrameCompat(), voir son commentaire complet.
void openNextGif()
{
  if (ESP.getMaxAllocHeap() < OPEN_NEXT_GIF_MIN_HEAP)
  {
    // v78 -- ce chemin etait totalement SILENCIEUX avant (port depuis
    // dev/mame-score-mqtt-bridge v103 diag) : suspect n°1 de l'ecran noir
    // persistant apres veille. Rate-limite (retente a CHAQUE loop() via
    // requestNextGif -- sans limite, spam total tant que le heap reste bas)
    // a 1 log/3s max.
    static unsigned long s_lastBlockedLogMs = 0;
    if (millis() - s_lastBlockedLogMs >= 3000)
    {
      s_lastBlockedLogMs = millis();
      Serial.println("[HEAP] openNextGif BLOQUE (maxalloc=" + String(ESP.getMaxAllocHeap())
                     + " < " + String(OPEN_NEXT_GIF_MIN_HEAP) + ") -- retente via requestNextGif t=" + String(millis()));
    }
    requestNextGif = true;
    return;
  }
  String next=(nextGifPath.length()>0)?nextGifPath:getNextGif(); nextGifPath="";
  bool ok = (next.length()>0) && openGif(next,false,true,true);
  // v91 -- BUG REEL confirme sur materiel (abort() decode via addr2line, ELF
  // verifie) : ce prefetch (nextGifPath=getNextGif(), execute a CHAQUE
  // ouverture de GIF reussie) n'etait garde par AUCUN controle heap propre --
  // seul le controle d'ENTREE de openNextGif() (ligne ~3821, OPEN_NEXT_GIF_MIN_HEAP)
  // le precedait, mais openGif() qui vient de s'executer juste au-dessus
  // consomme lui-meme du heap, donc le seuil verifie a l'entree n'est plus
  // garanti valide ici. Crash observe : SD.open() -> operator new echoue
  // (shared_ptr<VFSFileImpl>) ET l'allocation de l'exception bad_alloc
  // ELLE-MEME echoue aussi (heap trop bas meme pour ca) -> std::terminate()
  // direct, non rattrapable par le try/catch deja present dans
  // getNextGifRandom() (v78) -- ce filet ne protege QUE le cas ou il reste
  // juste assez de heap pour lever l'exception, pas le cas de double-echec.
  //
  // v94 -- CE MEME CRASH REPRODUIT sur materiel MALGRE le fix v91 ci-dessus
  // (hash ELF verifie, meme ligne exacte) : au moment du crash,
  // ESP.getMaxAllocHeap()==4596, DONC AU-DESSUS du seuil OPEN_NEXT_GIF_MIN_HEAP
  // (4000) -- le garde-fou v91 a laisse passer et le double-echec d'allocation
  // s'est quand meme produit. Cause : 4596 est le plateau heap NORMAL/HABITUEL
  // de ce materiel en fonctionnement courant (deja documente ailleurs dans ce
  // projet), pas une valeur anormalement basse -- OPEN_NEXT_GIF_MIN_HEAP=4000
  // (calibre a l'origine pour un AUTRE point d'appel, le garde d'ENTREE de
  // openNextGif()) ne protege quasiment JAMAIS en pratique pour CETTE
  // allocation precise (make_shared<VFSFileImpl>), qui a besoin de plus de
  // marge que 4596 pour reussir de facon fiable. Fix : seuil DEDIE, plus
  // eleve, uniquement pour ce prefetch -- contrairement au garde d'ENTREE
  // (dont un seuil trop haut avait cause un ecran noir indefini, historique
  // documente), un seuil haut ICI est peu risque : le pire cas est un
  // prefetch simplement saute (retente au prochain appel, le GIF EN COURS
  // reste affiche normalement pendant ce temps, pas d'ecran noir). Constante
  // remontee en portee fichier en v97 (voir sa declaration, un 2e site
  // d'appel non protege trouve ailleurs en a eu besoin aussi).
  if (ok && ESP.getMaxAllocHeap() >= PREFETCH_NEXT_GIF_MIN_HEAP) nextGifPath=getNextGif();
  if (!ok)
  {gifOpened=false;currentMode=MODE_BLACK;display->clearScreen();return;}
  currentMode=MODE_PLAYLIST;
}

void resumePlaylist()
{
  // v85 (2026-08-17) -- BUG REEL confirme sur materiel : plusieurs sites
  // d'appel de resumePlaylist() (deferred "default", auto-resolution des
  // alertes WiFi/RecalBox, "Reprendre DMD" web) ne passaient pas par le
  // case CMD_DEFAULT de processPendingMqttCommand() et donc ne
  // reinitialisaient jamais g_inGameMarquee -- si l'un d'eux s'appliquait
  // pendant qu'une vraie partie etait en cours (ex. "default" differe
  // depuis l'ecran de connexion, applique bien apres qu'un jeu ait
  // demarre), currentMode repassait a MODE_PLAYLIST mais g_inGameMarquee
  // restait bloque a true -- etat incoherent qui empechait silencieusement
  // et indefiniment toute alternance hi-score/game_info de se declencher
  // (elle exige MODE_PNG/MODE_GIF, jamais MODE_PLAYLIST). Corrige
  // individuellement a chaque site connu (voir leurs commentaires), ET ICI
  // en dernier recours centralise : resumePlaylist() signifie TOUJOURS
  // "on quitte ce qui tournait pour la playlist hors-jeu", donc c'est
  // TOUJOURS correct d'y desactiver l'alternance, quel que soit l'appelant
  // (present ou futur).
  // v104 -- g_inGameMarquee retire (hi-score port supprime).
  gif.close(); gifOpened=false; currentPngPath=""; pngDrawn=false;
  if(nextGifFile){nextGifFile.close();nextGifFile=File();}
  nextGifPath=""; freeBigramAll();
  displayedMaskSysName="";
  if(gifCount>0){currentMode=MODE_PLAYLIST;openNextGif();}
  else{currentMode=MODE_BLACK;display->clearScreen();}
}

// --------------------------------------------------
// Pause/Resume DMD depuis le serveur web (evite les conflits SD)
// --------------------------------------------------
void webDmdPause(const String &msg, uint16_t color)
{
  g_sdOpSubMsg = msg;
  g_sdOpSubMsgColor = color;
  g_sdOpSubMsgSetAt = millis();
  g_sdOpScrollOffset = 0;
  g_sdOpLastScroll = 0;

  // Fermer
  gif.close(); gifOpened = false; currentPngPath = ""; pngDrawn = false;
  if (nextGifFile) { nextGifFile.close(); nextGifFile = File(); }
  nextGifPath = ""; freeBigramAll(); displayedMaskSysName = "";
  g_configDmdDirty = true;

  g_sdOpInProgress = true;
  currentMode = MODE_CONFIG;

  // Dessiner directement la ligne 2 (permet les progressions depuis les handlers HTTP bloquants)
  webDmdOverlayLine2(msg, color);
}

// Dessine uniquement la ligne 2 (progression) -- SANS fermer gif/changer de
// mode, contrairement a webDmdPause() complet. Utilisee par loop() pour
// afficher la progression de playlistGenTask() (2026-07-28) : la tache ne
// touche jamais gif/display elle-meme (voir commentaire pres de
// PlaylistGenStatus, juste avant #include "web_config.h"), donc c'est loop()
// qui lit son instantane et appelle ceci -- UNIQUEMENT si currentMode vaut
// deja MODE_CONFIG, jamais pour l'y forcer. Corrige au passage un petit bug
// existant : l'ancien webDmdPause() periodique re-coupait une reprise DMD
// faite par l'utilisateur pendant un scan (il fermait gif/repassait en
// MODE_CONFIG a chaque rafraichissement) -- cette version ne touche plus rien
// d'autre que la ligne de texte.
void webDmdOverlayLine2(const String &msg, uint16_t color)
{
  display->fillRect(0, 24, 128, 8, 0);
  display->setTextColor(color);
  display->setCursor(1, 24);
  display->print(msg);
  Serial.println("[WEB] DMD pause: " + msg);
}

// Dessine g_sdOpSubMsg (ligne 2, MODE_CONFIG) avec le cursor X donne --
// factorise le rendu simple/2-couleurs pour eviter de le dupliquer entre
// webDmdForceRedraw() (redessin complet) et le bloc de defilement de
// loop() (2026-08-09, demande utilisateur : faire ressortir le SSID/IP
// du prefixe d'instruction sur l'ecran WiFi de secours -- voir
// g_sdOpSubMsgWhiteFrom).
void drawSdOpSubMsgAt(int x)
{
  if (g_sdOpSubMsgWhiteFrom >= 0 && g_sdOpSubMsgWhiteFrom < (int)g_sdOpSubMsg.length()) {
    String prefix = g_sdOpSubMsg.substring(0, g_sdOpSubMsgWhiteFrom);
    String value = g_sdOpSubMsg.substring(g_sdOpSubMsgWhiteFrom);
    display->setTextColor(g_sdOpSubMsgColor);
    display->setCursor(x, 24);
    display->print(prefix);
    display->setTextColor(0xFFFF); // blanc, fait ressortir le SSID/l'IP
    display->setCursor(x + (int)prefix.length() * 6, 24);
    display->print(value);
  } else {
    display->setTextColor(g_sdOpSubMsgColor);
    display->setCursor(x, 24);
    display->print(g_sdOpSubMsg);
  }
}

// Redessine immediatement l'ecran MODE_CONFIG (les 2 lignes) a partir de
// g_sdOpMsg/g_sdOpSubMsg -- factorise depuis loop() pour pouvoir aussi etre
// appelee depuis un contexte bloquant hors boucle normale si besoin.
void webDmdForceRedraw()
{
  g_configDmdDirty = false;
  g_sdOpScrollOffset = 0;
  g_sdOpScrollOffset1 = 0;
  g_sdOpSubMsgPauseUntil = 0;
  display->clearScreen();
  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(0xFFE0);
  display->setCursor(1, 4);
  display->print(g_sdOpMsg);
  display->fillRect(0, 24, 128, 8, 0);
  drawSdOpSubMsgAt(1);
}

void webDmdSetMainMsg(const String &msg)
{
  g_sdOpMsg = msg;
  g_sdOpScrollOffset1 = 0;
  g_sdOpLastScroll1 = 0;
  g_configDmdDirty = true;
  Serial.println("[WEB] DMD setMainMsg: " + msg);
}

void webDmdResume()
{
  // Ne redemarre plus l'ESP32 : on quitte simplement le mode config (les
  // ecrans/handlers HTTP restent actifs) et on reprend l'affichage normal.
  // g_sdOpInProgress doit etre remis a false explicitement ici -- avant, un
  // ESP.restart() le remettait a zero gratuitement au boot ; les handlers
  // MQTT (CMD_STOP/CMD_DEFAULT/CMD_SYSTEM/CMD_GAME) l'utilisent pour ignorer
  // toute commande tant que le mode config est actif, donc l'oublier ici
  // bloquerait ces commandes indefiniment apres un "Reprendre DMD".
  Serial.println("[WEB] DMD resume -> retour a l'affichage normal (sans reboot)");
  // v75 -- si un apercu de theme horloge (CMD_CLOCK_PREVIEW) tourne
  // actuellement dans sa boucle bloquante (showClock(), previewMode), le
  // signaler pour qu'elle s'arrete AU PROCHAIN TOUR (juste apres son appel
  // a handleWebConfig(), qui execute ce handler -- donc quasi immediat).
  // Sans ca, resumePlaylist() ci-dessous ouvre bien le GIF suivant mais
  // showClock() continue a dessiner le theme horloge par-dessus
  // indefiniment (elle ne connait que pendingCmd/hasPendingMqttCommand(),
  // jamais notifie par cet endpoint) -- bug reel constate en test materiel :
  // "Reprendre DMD" clique pendant un apercu actif, le DMD reste bloque sur
  // l'horloge alors que le GIF est bien ouvert en memoire.
  g_clockPreviewAbort = true;
  g_sdOpInProgress = false;
  // Demande explicite utilisateur (2026-08-03) : si la Recalbox est deja
  // connectee (MQTT actif), lui laisser reprendre la main plutot que de
  // forcer la playlist -- pendant tout le temps ou le mode config etait
  // actif, les vrais evenements MQTT (system/game) arrivaient bien mais
  // etaient ignores (voir "X ignored (web open)" dans les handlers).
  // v50 -- bug reel confirme (Reprendre DMD alors que RB est en mode
  // clip/demo : plus jamais de reprise playlist) : RB annonce son passage en
  // demo UNE SEULE FOIS (CMD_DEFAULT/CMD_STARTCLIP), pas a chaque nouveau
  // clip -- attendre un nouveau message ici bloquait donc indefiniment sur
  // l'ecran d'attente, RB n'ayant plus rien de neuf a annoncer. Fix : utilise
  // g_lastMqttWasDefault (dernier contenu REELLEMENT affiche avant l'ouverture
  // du mode config) pour decider. Si le dernier etat connu etait deja la
  // playlist/l'ecran d'attente (RB en demo), reprend directement la playlist
  // -- cet etat reste valide, pas besoin d'attendre RB. Si c'etait un
  // system/jeu precis, affiche l'ecran d'attente comme avant (pourrait etre
  // perime, mieux vaut attendre une confirmation fraiche -- partie en cours
  // par ex.). Si MQTT n'est PAS connecte (Recalbox injoignable), aucune
  // autre source de contenu -- comportement inchange, reprend la playlist.
  if (mqttClient.connected() && !g_lastMqttWasDefault)
  {
    if (mqttCmdMutex != nullptr && xSemaphoreTake(mqttCmdMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      pendingCmd = MqttCommand(MqttCommand::CMD_WAITING_MQTT, "");
      xSemaphoreGive(mqttCmdMutex);
    }
  }
  else
  {
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    resumePlaylist();
  }
}

// Marque first_boot=0 dans config.ini. PLUS APPELEE AUTOMATIQUEMENT depuis
// le 2026-08-05 (bug corrige, demande utilisateur -- etape 3 de la logique
// cible) : le simple affichage d'une page ne doit plus effacer first_boot,
// seule une sauvegarde reellement complete (playlist + IP Recalbox,
// handleWebConfigSave() dans web_config.h) le fait desormais. Conservee
// definie (plus aucun appelant actuel) au cas ou un declenchement manuel
// explicite serait utile plus tard -- cout nul.
void clearFirstBoot()
{
  if (!g_firstBoot) return;
  g_firstBoot = false;
  String all;
  File cfg = SD.open("/config.ini", FILE_READ);
  if (cfg) {
    while (cfg.available()) all += (char)cfg.read();
    cfg.close();
  }
  // Chercher et remplacer first_boot=1 par first_boot=0, ou ajouter si absent
  int pos = all.indexOf("first_boot=");
  if (pos >= 0) {
    int eol = all.indexOf('\n', pos);
    if (eol < 0) eol = all.length();
    String before = all.substring(0, pos);
    String after = all.substring(eol + 1);
    all = before + "first_boot=0\n" + after;
  } else {
    if (all.length() > 0 && all[all.length()-1] != '\n') all += "\n";
    all += "first_boot=0\n";
  }
  cfg = SD.open("/config.ini", FILE_WRITE);
  if (cfg) { cfg.print(all); cfg.close(); Serial.println("[BOOT] first_boot=0 written to config.ini"); }
}

// Ecrit une cle=valeur dans config.ini: remplace la ligne existante si presente,
// sinon l'ajoute a la fin. Meme pattern que clearFirstBoot() ci-dessus, generalise
// pour les flags ajoutes pour le mode secours WiFi (force_ap_recovery).
void writeConfigFlag(const String &key, const String &value)
{
  String all;
  File cfg = SD.open("/config.ini", FILE_READ);
  if (cfg) { while (cfg.available()) all += (char)cfg.read(); cfg.close(); }
  String needle = key + "=";
  int pos = all.indexOf(needle);
  if (pos >= 0) {
    int eol = all.indexOf('\n', pos);
    if (eol < 0) eol = all.length();
    all = all.substring(0, pos) + needle + value + "\n" + all.substring(eol + 1);
  } else {
    if (all.length() > 0 && all[all.length()-1] != '\n') all += "\n";
    all += needle + value + "\n";
  }
  cfg = SD.open("/config.ini", FILE_WRITE);
  if (cfg) { cfg.print(all); cfg.close(); }
}

// --------------------------------------------------
// Traductions des bannieres informatives DMD (fr/en/es, pilotees par
// uiLanguage/config.ini "language="). Les libelles techniques courts
// (WIFI OK, NTP, BT ON/OFF, brightness%, splash boot) restent volontairement
// non traduits -- deja compacts/quasi universels sur un ecran 128x32.
// Accents volontairement omis (police ecran/encodage source ASCII, meme
// convention que le reste des commentaires de ce fichier).
// --------------------------------------------------
String trOpenBrowserAt(const String &ip)
{
  if (uiLanguage == "en") return "Open a browser at http://" + ip;
  if (uiLanguage == "es") return "Abra un navegador en http://" + ip;
  return "Ouvrez un navigateur sur http://" + ip;
}

String trWifiRecoveryCountdown(unsigned long seconds)
{
  String base;
  if (uiLanguage == "en") base = "WiFi Recovery ";
  else if (uiLanguage == "es") base = "Recuperacion WiFi ";
  else base = "Secours WiFi ";
  return base + String(seconds) + "s";
}

String trConnectWifiMsg()
{
  if (uiLanguage == "en") return "Connect to WiFi RecalBox-DMD-Config";
  if (uiLanguage == "es") return "Conectese al WiFi RecalBox-DMD-Config";
  return "Connectez-vous au WiFi RecalBox-DMD-Config";
}

// Texte superpose a l'image de secours (default.raw565) affichee a la
// connexion MQTT (CMD_WAITING_MQTT) -- demande utilisateur (2026-07-28).
// Sur 2 lignes depuis v54 (l1/l2 en sortie) -- voir changelog.
void trRecalboxConnected(String &l1, String &l2)
{
  l1 = "RecalBox";
  if (uiLanguage == "en") { l2 = "connected :)"; return; }
  if (uiLanguage == "es") { l2 = "conectada :)"; return; }
  l2 = "connectee :)";
}

// Texte de l'indicateur "RecalBox non connectee" (2026-08-05, demande
// utilisateur) -- WiFi OK mais mqttClient.state()==-2, voir declaration
// de g_recalboxDisconnectedPending. Sur 2 lignes depuis v54 (l1/l2 en
// sortie, symbole ":/" -- voir changelog).
void trRecalboxDisconnected(String &l1, String &l2)
{
  l1 = "RecalBox";
  if (uiLanguage == "en") { l2 = "offline :/"; return; }
  if (uiLanguage == "es") { l2 = "offline :/"; return; }
  l2 = "hors ligne :/";
}

// Texte de l'alerte "No wifi, No Recalbox" -- desormais traduit depuis
// v54 (etait volontairement fixe non traduit depuis le 2026-08-05, voir
// changelog) : symbole ":(" en fin de 2e ligne.
void trNoWifiNoRecalbox(String &l1, String &l2)
{
  if (uiLanguage == "en") { l1 = "No wifi"; l2 = "No Recalbox :("; return; }
  if (uiLanguage == "es") { l1 = "Sin wifi"; l2 = "Sin Recalbox :("; return; }
  l1 = "Pas de wifi";
  l2 = "Pas de Recalbox :(";
}

// Dessine (visible=true) ou efface (visible=false) un texte sur 2 lignes
// centrees horizontalement/verticalement, avec police ADAPTATIVE (taille
// 2, plus lisible, si les 2 lignes tiennent dans RAW565_W ; repli taille
// 1 sinon) et la meme ombre noire que l'ancien rendu 1 ligne. Restaure le
// fond depuis le cache RAM de l'image de secours (PAS un bandeau noir --
// bug remonte en test reel 2026-08-03, voir ancien commentaire), sur la
// hauteur totale du bloc de 2 lignes desormais (au lieu d'une seule).
// Remplace la logique dupliquee des 3 fonctions de dessin (2026-08-07,
// demande utilisateur -- alertes de connexion sur 2 lignes + symboles).
void drawTwoLineCenteredOverlay(bool visible, const String &line1, const String &line2, uint16_t color)
{
  int size = 2;
  if ((int)line1.length() * 12 > RAW565_W || (int)line2.length() * 12 > RAW565_W) size = 1;
  const int charW = 6 * size;
  const int lineH = 8 * size;
  const int blockH = lineH * 2;
  const int textY0 = (RAW565_H - blockH) / 2;

  if (defaultRaw565Cached && defaultRaw565Buf) {
    for (int y = textY0; y < textY0 + blockH && y < RAW565_H; y++)
      display->drawRGBBitmap(0, y, defaultRaw565Buf + (size_t)y * RAW565_W, RAW565_W, 1);
  } else {
    display->fillRect(0, textY0, RAW565_W, blockH, 0);
  }
  if (!visible) return;

  display->setTextWrap(false);
  display->setTextSize(size);
  const String *lines[2] = {&line1, &line2};
  for (int i = 0; i < 2; i++) {
    const String &txt = *lines[i];
    int textW = (int)txt.length() * charW;
    int x = (RAW565_W - textW) / 2;
    if (x < 0) x = 0;
    int y = textY0 + i * lineH;
    display->setTextColor(display->color565(0, 0, 0));
    display->setCursor(x + 1, y + 1);
    display->print(txt);
    display->setTextColor(color);
    display->setCursor(x, y);
    display->print(txt);
  }
}

// Dessine (visible=true) ou efface (visible=false) le texte "RecalBox
// connectee", centre horizontalement ET verticalement, avec la meme ombre
// noir/blanc qu'avant -- clignotant pendant tout l'affichage (voir loop(),
// toggle periodique). Centre horizontal calcule dynamiquement (largeur
// variable selon la langue) plutot qu'une position fixe.
void drawRecalboxConnectedOverlay(bool visible)
{
  String l1, l2;
  trRecalboxConnected(l1, l2);
  drawTwoLineCenteredOverlay(visible, l1, l2, display->color565(0, 255, 0));
}

// Dessine (visible=true) ou efface (visible=false) le texte rouge
// clignotant "No wifi, No Recalbox" -- meme structure que
// drawRecalboxConnectedOverlay() ci-dessus (restauration du fond depuis
// le cache RAM de l'image de secours, centrage horizontal/vertical, ombre
// noire) mais texte fixe non traduit (2026-08-05, demande utilisateur --
// meme convention que les libellés techniques courts de ce fichier,
// jamais traduits : "WIFI OK", "NTP", etc.) et couleur rouge au lieu de
// blanc, pour signaler une situation anormale (WiFi injoignable) plutot
// qu'un etat normal d'attente.
void drawNoWifiNoRecalboxOverlay(bool visible)
{
  String l1, l2;
  trNoWifiNoRecalbox(l1, l2);
  drawTwoLineCenteredOverlay(visible, l1, l2, display->color565(255, 0, 0));
}

// Declenche l'affichage de l'alerte "No wifi, No Recalbox" (image de
// secours + texte rouge clignotant, 5s puis retour auto a la playlist --
// voir g_noWifiRecalboxScreenActive/g_noWifiRecalboxUntilMs et le bloc
// loop() qui pilote le clignotement + l'auto-resolution). Appelee depuis
// loop() a un point ou rien d'important n'est en train de jouer (entre
// deux GIFs en MODE_PLAYLIST, ou immediatement si aucune playlist n'est
// active) -- jamais depuis mqttTask() (tache de fond, pas de dessin direct
// hors thread principal, meme regle que le reste de ce fichier).
void showNoWifiRecalboxAlert()
{
  gif.close(); gifOpened=false; currentPngPath=String(DEFAULT_RAW565_PATH); pngDrawn=true;
  display->clearScreen();
  bool okDraw = drawDefaultRaw565Cached();
  currentMode = okDraw ? MODE_PNG : MODE_BLACK;
  if (okDraw) drawNoWifiNoRecalboxOverlay(true);
  g_noWifiRecalboxScreenActive = true;
  g_noWifiRecalboxUntilMs = millis() + NO_WIFI_ALERT_DISPLAY_MS;
  g_noWifiRecalboxPending = false;
  Serial.println("[WIFI] No wifi, No Recalbox -- alerte affichee");
}

// Dessine (visible=true) ou efface (visible=false) le texte orange
// clignotant "RecalBox non connectee" -- meme structure que
// drawRecalboxConnectedOverlay()/drawNoWifiNoRecalboxOverlay() (restauration
// du fond depuis le cache RAM de l'image de secours, centrage, ombre
// noire), texte TRADUIT (trRecalboxDisconnected()) et couleur orange.
void drawRecalboxDisconnectedOverlay(bool visible)
{
  String l1, l2;
  trRecalboxDisconnected(l1, l2);
  drawTwoLineCenteredOverlay(visible, l1, l2, display->color565(255, 140, 0));
}

// Declenche l'affichage de l'alerte "RecalBox non connectee" -- meme
// mecanisme que showNoWifiRecalboxAlert() (voir son commentaire), drapeau
// et duree d'affichage dedies.
void showRecalboxDisconnectedAlert()
{
  gif.close(); gifOpened=false; currentPngPath=String(DEFAULT_RAW565_PATH); pngDrawn=true;
  display->clearScreen();
  bool okDraw = drawDefaultRaw565Cached();
  currentMode = okDraw ? MODE_PNG : MODE_BLACK;
  if (okDraw) drawRecalboxDisconnectedOverlay(true);
  g_recalboxDisconnectedScreenActive = true;
  g_recalboxDisconnectedUntilMs = millis() + NO_WIFI_ALERT_DISPLAY_MS;
  g_recalboxDisconnectedPending = false;
  Serial.println("[MQTT] RecalBox non connectee -- alerte affichee");
}

String trOpenUrl(const String &ip)
{
  if (uiLanguage == "en") return "Open http://" + ip;
  if (uiLanguage == "es") return "Abrir http://" + ip;
  return "Ouvrir http://" + ip;
}

String trConfigPageMsg()
{
  if (uiLanguage == "en") return "Configuration page";
  if (uiLanguage == "es") return "Pagina de configuracion";
  return "Page de configuration";
}

// Ligne 2 de l'ecran secours WiFi : prefixe l'instruction avant le SSID/l'IP
// (demande utilisateur) -- toggle toutes les 6s (au lieu de 2s) dans
// maintainApRecovery() pour laisser le defilement horizontal le temps
// d'avancer sur ces chaines plus longues (depassent 128px).
String trJoinWifi(const String &ssid)
{
  if (uiLanguage == "en") return "Join the wifi " + ssid;
  if (uiLanguage == "es") return "Unase al wifi " + ssid;
  return "Rejoignez le wifi " + ssid;
}

String trOpenInBrowser(const String &url)
{
  if (uiLanguage == "en") return "Open in a browser " + url;
  if (uiLanguage == "es") return "Abra en un navegador " + url;
  return "Ouvrez dans un navigateur " + url;
}

// --------------------------------------------------
// MQTT command processing
// --------------------------------------------------
// Debug verbeux de CMD_GAME (2026-08-09, demande utilisateur) : desactive
// par defaut pour tester si cela reduit la fragmentation heap observee
// (plusieurs abort() dans lock_init_generic lors d'ouvertures de fichier
// -- piste : les nombreuses concatenations de String Arduino dans ces
// logs, qui tournent a CHAQUE changement de jeu, pourraient contribuer a
// la fragmentation sur une session avec beaucoup de changements rapides
// -- pas confirme, juste teste). Repasser a true pour retrouver le detail
// complet si besoin de deboguer a nouveau le flux CMD_GAME.
const bool CMD_GAME_DEBUG_LOGS = true; // v98 -- reactive : meme symptome recurrent (currentPngPath bloque sur le placeholder "RB connectee" en boucle) observe MEME apres le fix v97 (slot dedie pour game) -- besoin de confirmer si CMD_GAME est bien recu/traite cette fois, ou si un autre mecanisme est en cause

// v110 -- CMD_SCORE/MODE_SCORE (voir entete changelog). Duree fixe d'affichage
// avant retour automatique au jeu -- c'est LA garantie anti-blocage demandee
// par l'utilisateur, ne depend d'aucun message RB ulterieur. g_modeBeforeScore
// memorise le mode a restaurer (MODE_GIF ou MODE_PNG selon ce qui tournait
// avant le score) ; g_scoreShowUntilMs est l'echeance absolue (millis()).
const unsigned long SCORE_DISPLAY_DURATION_MS = 6000;
// v111 -- duree PAR MESSAGE optionnelle (demande utilisateur explicite --
// pagination "scroll bete" cote script RB, dmd_score.sh v6, qui a besoin
// de durees plus courtes/differenciees par type de page pour tester la
// vitesse de lecture). Prefixe optionnel "@<ms>|" en tete du payload
// CMD_SCORE (voir case MqttCommand::CMD_SCORE) -- SANS ce prefixe,
// comportement 100% inchange (SCORE_DISPLAY_DURATION_MS, ex.
// marquee/cmd/score venant de dmd_achievement.sh, jamais mis a jour pour
// ce prefixe). Bornes de securite : en-deca de MIN, un enchainement de
// pages deviendrait illisible/aveuglant ; au-dela de MAX, aucun interet
// (le round-robin cote RB a de toute facon son propre rythme).
const long SCORE_DURATION_OVERRIDE_MIN_MS = 500;
const long SCORE_DURATION_OVERRIDE_MAX_MS = 15000;
DisplayMode   g_modeBeforeScore  = MODE_BLACK;
unsigned long g_scoreShowUntilMs = 0;

// CMD_GAME_MIN_HEAP_FOR_FILE_OPEN deplacee plus haut dans le fichier en
// v62 (avant drawRaw565(), qui en depend desormais -- garde-fou
// centralise) -- voir sa declaration/changelog complet juste avant
// drawDefaultRaw565Cached().

// v110 -- ombre portee (1px, noir) derriere le texte principal -- reprend
// le style visuel de l'ancien drawOverlayTextShadowed() (branche
// dev-mame-score-mqtt-bridge, retire ici en v104 avec tout le reste du
// sous-systeme overlay) : ameliore juste la lisibilite sur fond de LEDs,
// AUCUNE logique d'etat/timing associee (pure fonction de dessin,
// appelee une seule fois par ecran -- pas de risque ajoute).
void drawScoreTextShadowed(int x, int y, const String &s, uint16_t mainColor)
{
  display->setTextColor(display->color565(0, 0, 0));
  display->setCursor(x + 1, y + 1);
  display->print(s);
  display->setTextColor(mainColor);
  display->setCursor(x, y);
  display->print(s);
}

// v118 -- avance reelle (xAdvance) du caractere c dans la police TomThumb
// -- necessaire pour l'espacement inter-caractere manuel de l'ecran
// DESCRIPTION (voir drawScoreScreen()/isDescriptionScreen). Lit
// directement TomThumbGlyphs[] (defini par Fonts/TomThumb.h, plage
// 0x20-0x7E) plutot que via display->gfxFont : ce dernier est `protected`
// dans Adafruit_GFX (inaccessible depuis ici, erreur de compilation
// constatee au 1er essai) -- sans interet de toute facon, cette fonction
// n'est appelee que pour CETTE police precise. Repli a 4px si le
// caractere est hors de la plage couverte (securite, ne devrait jamais
// arriver avec le texte MAJUSCULES pur ASCII envoye ici). pgm_read_byte
// -- meme convention qu'Adafruit_GFX.cpp en interne (TomThumbGlyphs[] est
// PROGMEM) -- no-op sur ESP32 (flash mappee en memoire) mais garde le
// code portable.
uint8_t tomThumbCharAdvance(char c)
{
  uint8_t uc = (uint8_t)c;
  if (uc < 0x20 || uc > 0x7E) return 4;
  return pgm_read_byte(&TomThumbGlyphs[uc - 0x20].xAdvance);
}

// v110 -- rendu MODE_SCORE : ecran plein, jusqu'a 4 lignes (128x32, taille de
// texte 1 = 8px/ligne -> tient exactement), payload decoupe sur "|" (format
// deja utilise par l'ancien systeme hi-score avant retrait v104, ex.
// "HI-SCORE|1 MAA 283200|2 CAP 30000|..."). Presentation reprise de l'ancien
// systeme (ombre portee, titre centre en or, rangs coupes sur le DERNIER
// espace pour colorer nom/score separement -- voir memoire projet) --
// demande utilisateur explicite (2026-08-19) : "ameliorer la presentation
// comme sur l'autre branche de dev". Volontairement SANS le defilement
// vertical/emphase rang-1-en-gros-caracteres de l'ancien systeme -- ca
// ajoutait un etat/timing anime (g_overlayScrollTop, pause sur le rang 1,
// etc.) coherent avec la philosophie "DMD bete" a eviter ici : si plus de 4
// champs sont envoyes, seuls les 4 premiers s'affichent (statique, aucune
// troncature dangereuse, juste moins d'info visible). Ne touche JAMAIS
// gifOpened/currentPngPath/pngDrawn -- le jeu en dessous reste intact.
void drawScoreScreen(const String &payload)
{
  display->clearScreen();
  display->setTextWrap(false);
  // v114 -- reset defensif de la police (voir TomThumb, ecran DESCRIPTION
  // plus bas) -- garantit que la police classique est toujours le point de
  // depart de CET appel, quel que soit l'etat laisse par un appel precedent.
  display->setFont(NULL);
  uint16_t gold  = display->color565(255, 200, 0); // convention "highscore" deja utilisee ailleurs (ex. showClock())
  uint16_t white = display->color565(235, 235, 235);
  // v115 -- palette UNIFIEE entre tous les ecrans (demande utilisateur
  // explicite : "unifie les couleurs des differents elements entre les
  // tableaux" + "le titre ne doit pas avoir la meme couleur que les
  // sous-titres ou le texte -- actuellement les scores sont de la meme
  // couleur que le titre") -- 3 roles COHERENTS partout : titleColor pour
  // TOUS les titres (HI-SCORE/INFOS/DESCRIPTION, plus utilise gold), white
  // pour le "libelle/nom" (nom de rang, libelle INFOS, texte DESCRIPTION),
  // gold pour la "valeur/donnee" (score de rang, valeur INFOS) -- remplace
  // les infoLabelColor/infoValueColor dediees (v112) par ce meme motif
  // white/gold deja utilise pour hi-score.
  // v117 -- couleur du titre plus vive (retour utilisateur : "elle se
  // rapproche trop du blanc") -- bleu-violet sature, sans ambiguite avec
  // le blanc (235,235,235) ni l'or des scores.
  uint16_t titleColor = display->color565(90, 90, 255);
  // v111 -- rang 1 mis en valeur (demande utilisateur explicite, "fais 1
  // page avec rang 1 plus gros ... et/ou avec couleur differente") :
  // couleur dediee, distincte de l'or utilise partout ailleurs -- exception
  // deliberee a la palette unifiee ci-dessus (emphase specifique du rang 1).
  uint16_t rank1Color = display->color565(255, 90, 40);

  // v111 -- titre (1ere ligne) extrait A PART, avant la boucle -- necessaire
  // pour decider du mode de rendu (hiscore rang-1-en-gros / infos couleurs /
  // generique) AVANT de traiter le reste du payload.
  int firstSep = payload.indexOf('|');
  String title = (firstSep == -1) ? payload : payload.substring(0, firstSep);
  String rest  = (firstSep == -1) ? String("") : payload.substring(firstSep + 1);

  // v113 -- page 1 hi-score (dmd_score.sh v6, pagination "scroll bete") --
  // CORRIGE suite au 1er test reel (retour utilisateur : "le rang 1 doit
  // etre affiche en taille 2 [ligne], pas sur 2 lignes, le titre a
  // disparu") : titre "HI-SCORE" RESTAURE (centre, or, taille 1, 8px) ;
  // rang 1 desormais sur UNE SEULE ligne en taille 2 (nom+score ensemble,
  // pas nom en taille 1 puis score en taille 2 sur une 2e ligne comme
  // avant) ; rang 2 en taille normale en dessous. 8+16+8=32px, tient
  // exactement. Le numero de rang ("1 ") est retire du nom affiche en gros
  // (implicite -- 1er rang juste sous le titre -- le garder ferait
  // deborder l'ecran pour un score a 6+ chiffres : 128px / 12px par
  // caractere en taille 2 = ~10-11 caracteres de budget).
  // v116 -- BUG REEL corrige (retour utilisateur : "la page 2 hi-score n'a
  // pas son titre") -- le titre doit PERSISTER sur toutes les pages (regle
  // deja actee, voir INFOS/DESCRIPTION qui le font deja correctement).
  // Auparavant, page 2 (rangs 3/4/5) envoyait un titre VIDE specifiquement
  // pour eviter de declencher le rendu special "rang 1 en gros" -- au prix
  // de perdre le titre. Fix : le titre "HI-SCORE" est desormais envoye sur
  // les 2 pages (voir dmd_score.sh v12) ; la distinction page1/page2 se
  // fait maintenant sur le CONTENU (rest commence par "1 " = le rang 1 est
  // present = page 1) plutot que sur le titre.
  if (title == "HI-SCORE" && rest.startsWith("1 ")) {
    int y = 0;
    display->setTextSize(1);
    { int tx = (RAW565_W - (int)title.length() * 6) / 2; if (tx < 0) tx = 0;
      drawScoreTextShadowed(tx, y, title, titleColor); }
    y += 8;
    int start = 0;
    int rankIdx = 0;
    while (start <= (int)rest.length() && rankIdx < 2) {
      int sep = rest.indexOf('|', start);
      String line = (sep == -1) ? rest.substring(start) : rest.substring(start, sep);
      int sp = line.lastIndexOf(' ');
      String name = (sp > 0) ? line.substring(0, sp) : line;
      String scoreVal = (sp > 0) ? line.substring(sp + 1) : "";
      if (rankIdx == 0) {
        String bigName = name;
        int firstSpace = name.indexOf(' ');
        if (firstSpace > 0) bigName = name.substring(firstSpace + 1); // retire le numero de rang
        display->setTextSize(2);
        drawScoreTextShadowed(1, y, bigName, white);
        // v157 -- score du rang 1 ALIGNE A DROITE (retour utilisateur sur
        // materiel reel : "le score est pas aligne a droite") -- corrige
        // une incoherence reelle avec le rang 2 juste en dessous, qui
        // aligne deja son score a droite (voir son commentaire v120,
        // "meme alignement a droite que le rendu generique"). Avant ce
        // fix, le score du rang 1 etait simplement colle juste apres le
        // nom (sx = fin du nom + marge), jamais aligne sur le bord droit
        // de l'ecran. Meme formule que le rang 2 (RAW565_W - largeur du
        // texte - marge), adaptee a la police taille 2 (12px/caractere
        // au lieu de 6px) -- garde-fou identique si le nom est trop long
        // pour laisser la place au score (nameEnd, meme principe que
        // rang 2).
        int sx = RAW565_W - (int)scoreVal.length() * 12 - 1;
        int nameEnd = 1 + (int)bigName.length() * 12 + 6;
        if (sx < nameEnd) sx = nameEnd;
        drawScoreTextShadowed(sx, y, scoreVal, rank1Color);
        y += 16;
      } else {
        display->setTextSize(1);
        drawScoreTextShadowed(1, y, name, white);
        // v120 -- meme alignement a droite que le rendu generique (voir
        // commentaire complet plus bas) -- coherence visuelle entre les 2
        // ecrans qui partagent la meme convention nom/score.
        if (scoreVal.length() > 0) {
          int sx = RAW565_W - (int)scoreVal.length() * 6 - 1;
          int nameEnd = 1 + (int)name.length() * 6 + 4;
          if (sx < nameEnd) sx = nameEnd;
          drawScoreTextShadowed(sx, y, scoreVal, gold);
        }
        y += 8;
      }
      rankIdx++;
      if (sep == -1) break;
      start = sep + 1;
    }
    return;
  }

  display->setTextSize(1);
  int y = 0;
  int start = 0;
  int lineIdx = 0;
  const int maxLines = 4;
  bool isInfoScreen = (title == "INFOS");
  // v112 -- ecran DESCRIPTION : texte libre, AUCUN split nom/score --
  // corrige (retour utilisateur : "ya des mots qui sont de la meme couleur
  // que le titre, enleve ca (dernier mot de chaque ligne)") -- ce champ
  // tombait avant dans le rendu GENERIQUE ci-dessous (concu pour les rangs
  // hi-score, nom/score separes par le DERNIER espace), colorant a tort le
  // dernier mot de chaque ligne en or comme s'il s'agissait d'un score.
  bool isDescriptionScreen = (title == "DESCRIPTION");
  while (start <= (int)payload.length() && lineIdx < maxLines) {
    int sep = payload.indexOf('|', start);
    String line = (sep == -1) ? payload.substring(start) : payload.substring(start, sep);
    if (lineIdx == 0) {
      // Titre (1ere ligne) : centre, couleur dediee (titleColor, v115).
      // Chaine vide (page de continuation hi-score/pagination generique) ->
      // ne dessine rien, consomme juste ce slot de ligne.
      if (line.length() > 0) {
        int tx = (RAW565_W - (int)line.length() * 6) / 2; if (tx < 0) tx = 0;
        drawScoreTextShadowed(tx, y, line, titleColor);
      }
    } else if (isInfoScreen) {
      // v115 -- ecran INFOS : split au PREMIER ":" -- libelle (avant, avec
      // le ":") en white, valeur (apres) en gold -- palette UNIFIEE avec
      // hi-score (nom=white, score=gold), voir commentaire plus haut.
      int cp = line.indexOf(':');
      if (cp > 0) {
        String label = line.substring(0, cp + 1); // inclut le ":"
        String value = line.substring(cp + 1);
        drawScoreTextShadowed(1, y, label, white);
        drawScoreTextShadowed(1 + (int)label.length() * 6 + 4, y, value, gold);
      } else {
        drawScoreTextShadowed(1, y, line, white);
      }
    } else if (isDescriptionScreen) {
      // v118 -- Org_01 (v117) RETIRE : retour utilisateur explicite
      // ("cette police semble avoir un espacement entre les mots important
      // et ca rompt avec le style general" -- le caractere espace d'Org_01
      // est large, cassait la coherence visuelle avec le reste de
      // l'ecran). Retour a TomThumb (1ere police compacte testee, rejetee
      // a l'epoque pour "un peu dure a lire") avec 2 ameliorations de
      // lisibilite tentees sur demande explicite ("essaye d'ameliorer la
      // police thumb") :
      //  (1) espacement inter-caractere manuel (dessin caractere par
      //      caractere via tomThumbCharAdvance()) -- v119 -- REDUIT (retour
      //      utilisateur : "reduie l'espace entre les lettre un petit peu")
      //      -- +1px supplementaire au-dela de l'avance native RETIRE,
      //      avance native de la police seule desormais (deja ~1px pour la
      //      plupart des glyphes).
      //  (2) rendu tout en MAJUSCULES -- meilleure lisibilite a hauteur
      //      <=5px (TomThumb n'a de toute facon pas de vraies formes
      //      minuscules distinctes des majuscules a cette taille).
      // Positionnement : GFXfont custom = ligne de BASE (pas coin
      // superieur-gauche) -- yOffset=-5 pour la quasi-totalite des glyphes
      // TomThumb -> cursor.y = haut voulu + 5.
      // v119 -- couleur FIXEE en gold (retour utilisateur explicite : "on
      // garde la couleur gold pour le texte") -- remplace le test A/B
      // blanc/or TEMPORAIRE de v118 (alternance par ligne), desormais
      // tranche.
      // v121 -- lignes CENTREES horizontalement (retour utilisateur : "modifie
      // description en centrant les lignes au lieu de aligner a gauche") --
      // largeur totale de la ligne calculee au prealable (somme des avances
      // TomThumb reelles, pas une largeur fixe par caractere) pour un
      // centrage exact independant du contenu de chaque page.
      display->setFont(&TomThumb);
      String upper = line;
      upper.toUpperCase();
      int lineWidth = 0;
      for (size_t ci = 0; ci < upper.length(); ci++) {
        lineWidth += tomThumbCharAdvance(upper.charAt(ci));
      }
      int cx = (RAW565_W - lineWidth) / 2;
      if (cx < 0) cx = 0;
      for (size_t ci = 0; ci < upper.length(); ci++) {
        char ch = upper.charAt(ci);
        drawScoreTextShadowed(cx, y + 5, String(ch), gold);
        cx += tomThumbCharAdvance(ch);
      }
      display->setFont(NULL);
    } else {
      // Rang : coupe sur le DERNIER espace -- nom (blanc) a gauche, score
      // (or). v120 -- score desormais ALIGNE A DROITE (retour utilisateur
      // explicite : "les scores... se decalent en fonction de la longueur
      // des noms" -- visible surtout sur le classement CHALLENGE, plusieurs
      // rangs empiles avec des noms de longueurs tres variables, ex. "SNK"
      // vs "RUFOTHEONE") -- position calculee depuis le BORD DROIT de
      // l'ecran (independante de la longueur du nom), au lieu de juste
      // apres le nom (glissait a droite avec les noms longs, jamais aligne
      // entre les rangs). Repli sur le nom seul si le score deborderait a
      // gauche du nom (nom trop long pour le budget restant -- ne devrait
      // pas arriver avec les noms deja tronques a 10 caracteres cote RB,
      // mais defense en profondeur).
      int sp = line.lastIndexOf(' ');
      if (sp > 0) {
        String name = line.substring(0, sp);
        String scoreVal = line.substring(sp + 1);
        drawScoreTextShadowed(1, y, name, white);
        int sx = RAW565_W - (int)scoreVal.length() * 6 - 1;
        int nameEnd = 1 + (int)name.length() * 6 + 4;
        if (sx < nameEnd) sx = nameEnd;
        drawScoreTextShadowed(sx, y, scoreVal, gold);
      } else {
        drawScoreTextShadowed(1, y, line, white);
      }
    }
    y += 8;
    lineIdx++;
    if (sep == -1) break;
    start = sep + 1;
  }
}

bool hasPendingMqttCommand()
{
  if(mqttCmdMutex==nullptr) return false;
  if(xSemaphoreTake(mqttCmdMutex,0)!=pdTRUE) return false;
  // v96/v104 -- inclut les 3 slots dedies (voir leur declaration).
  bool has = g_pendingDefault || g_pendingSystem || g_pendingGame
             || (pendingCmd.type!=MqttCommand::CMD_NONE);
  xSemaphoreGive(mqttCmdMutex); return has;
}

void processPendingMqttCommand()
{
  if(mqttCmdMutex==nullptr) return;
  if(xSemaphoreTake(mqttCmdMutex,0)!=pdTRUE) return;
  // v96 -- priorite aux 4 slots dedies (jamais ecrases par un type different,
  // voir leur declaration) avant le pendingCmd generique -- ordre naturel
  // d'arrivee cote RB (default/system/game deja dans cet ordre logique,
  // ingame en dernier car publie apres coup par marquee.sh). Un seul
  // consomme par appel, comme avant -- les autres suivront au(x) prochain(s)
  // appel(s) de loop() (quelques ms plus tard), jamais perdus entre-temps.
  MqttCommand cmd(MqttCommand::CMD_NONE,"");
  if      (g_pendingDefault) { g_pendingDefault=false; cmd=MqttCommand(MqttCommand::CMD_DEFAULT,""); }
  else if (g_pendingSystem)  { g_pendingSystem=false;  cmd=MqttCommand(MqttCommand::CMD_SYSTEM,g_pendingSystemArg); }
  else if (g_pendingGame)    { g_pendingGame=false;    cmd=MqttCommand(MqttCommand::CMD_GAME,g_pendingGameArg); }
  else                       { cmd=pendingCmd; pendingCmd=MqttCommand(MqttCommand::CMD_NONE,""); }
  xSemaphoreGive(mqttCmdMutex);
  if(cmd.type==MqttCommand::CMD_NONE) return;

  // v111 -- INSTRUMENTATION DIAGNOSTIC TEMPORAIRE (a retirer une fois la
  // cause confirmee) : utilisateur rapporte un affichage MODE_SCORE
  // "fugace" (1/10eme de seconde au lieu des 4-6s attendues) alors que le
  // log [MQTT] "score -> affiche X.Xs" ne prouve que l'INTENTION posee a
  // la reception, pas la duree REELLEMENT tenue -- ce log ne capture donc
  // PAS une interruption ulterieure. Log EXPLICITE ici, au point exact ou
  // n'importe quelle AUTRE commande (game/default/system/stop/etc.) est
  // sur le point d'ecraser un MODE_SCORE encore actif, avec le temps qu'il
  // restait -- confirme ou infirme en un seul test si une commande
  // concurrente coupe le score prematurement.
  if (currentMode == MODE_SCORE && cmd.type != MqttCommand::CMD_SCORE) {
    long remainingMs = (long)(g_scoreShowUntilMs - millis());
    Serial.println("[DIAG] MODE_SCORE interrompu par cmd=" + String((int)cmd.type)
                   + " apres seulement " + String(remainingMs) + "ms restants (sur la duree demandee)");
  }

  switch(cmd.type)
  {
  case MqttCommand::CMD_STOP:
    if(currentMode==MODE_PLAYLIST||g_sdOpInProgress){Serial.println("[MQTT] stop ignored");break;}
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    g_mqttDefaultPendingAfterMinDisplay = false;
    gif.close();gifOpened=false;currentPngPath="";pngDrawn=false;
    currentMode=MODE_BLACK;display->clearScreen();
    break;

  case MqttCommand::CMD_DEFAULT:
    if (g_sdOpInProgress) { Serial.println("[MQTT] default ignored (web open)"); break; }
    // v122 -- 2e ligne de defense (voir declaration de g_recalboxInGame) :
    // un "default" recu alors que RB affirme encore etre EN JEU (dernier
    // marquee/cmd/ingame retenu = "1") est traite comme perime/errone --
    // ignore plutot que de faire basculer le marquee en playlist locale
    // pendant qu'une vraie partie tourne. Le vrai fix est cote script
    // (marquee.sh v17, handler start), ceci couvre les cas non prevus par
    // ce fix (autre source de "default" non identifiee, etc.) sans risque :
    // le pire cas est de rester sur le dernier affichage connu un peu plus
    // longtemps, jamais un ecran errone.
    if (g_recalboxInGame) { Serial.println("[MQTT] default ignore (RB toujours en jeu selon ingame=1)"); break; }
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    g_lastMqttWasDefault = true; // v50 -- pose ici, avant meme le differe eventuel : RB a bien annonce "default"
    // Delai minimum d'affichage de l'ecran "RecalBox connectee" (v49) : si
    // ce default arrive PENDANT que cet ecran est encore affiche ET avant le
    // delai minimum, on ne bascule pas tout de suite -- on memorise l'action
    // pour l'appliquer automatiquement une fois le delai ecoule (voir
    // loop()), au lieu de l'ignorer (ancien bug) ou de basculer trop tot
    // (regression du fix v46).
    if (g_mqttConnectedScreenUntilMs != 0 && millis() < g_mqttWaitingMinDisplayUntilMs)
    {
      Serial.println("[MQTT] default recu pendant l'ecran de connexion -- differe jusqu'au delai minimum");
      g_mqttDefaultPendingAfterMinDisplay = true;
      break;
    }
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    g_mqttDefaultPendingAfterMinDisplay = false;
    resumePlaylist();
    break;

  // Emis uniquement au moment ou la connexion MQTT vient d'aboutir (voir
  // mqttTask()) -- affiche l'image de secours statique (RAM default.raw565)
  // au lieu de relancer directement la playlist en rotation, le temps que
  // la Recalbox envoie un premier vrai message (system/game). Distinct de
  // CMD_DEFAULT (qui reste utilise par le pont marquee sur stop/sleep et
  // doit continuer a relancer la playlist normalement).
  case MqttCommand::CMD_WAITING_MQTT:
    if (g_sdOpInProgress) { Serial.println("[MQTT] waiting ignored (web open)"); break; }
    // Bug trouve sur test reel (ecran DMD noir/vide) : le case MODE_PNG de
    // loop() efface l'ecran et repasse en MODE_BLACK des que
    // currentPngPath est vide (voir loop(), ~ligne 3970) -- currentPngPath="",
    // copie du pattern openBestMedia(), provoquait donc un auto-clear
    // QUASI INSTANTANE a la frame suivante. Fix : currentPngPath non-vide
    // (chemin informatif seulement, jamais relu puisque pngDrawn=true
    // saute la logique de redessin) pour eviter ce garde.
    gif.close(); gifOpened=false; currentPngPath=String(DEFAULT_RAW565_PATH); pngDrawn=true;
    display->clearScreen();
    {
      bool okDraw = drawDefaultRaw565Cached();
      currentMode = okDraw ? MODE_PNG : MODE_BLACK;
      Serial.println(String("[MQTT] waiting -> default.raw565 ") + (okDraw ? "OK" : "FAIL (ecran vide)"));
      if (okDraw)
      {
        // Texte superpose (demande utilisateur) -- pngDrawn=true fait sauter
        // le redessin dans loop(), donc ce texte reste affiche par-dessus
        // l'image tant que rien d'autre ne prend la main sur l'affichage.
        // Le clignotement (loop()) prend le relais juste apres.
        drawRecalboxConnectedOverlay(true);
      }
      // Drapeau "ecran d'attente actif" (pilote uniquement le clignotement,
      // voir loop()) -- plus d'expiration par delai, on attend indefiniment
      // le prochain vrai message MQTT (v45, voir commentaire pres de la
      // declaration de g_mqttConnectedScreenUntilMs).
      g_mqttConnectedScreenUntilMs = 1;
      // Delai minimum d'affichage (v49) : reinitialise a chaque nouvel
      // affichage de cet ecran -- voir declaration de
      // MQTT_WAITING_MIN_DISPLAY_MS pour le detail complet.
      g_mqttWaitingMinDisplayUntilMs = millis() + MQTT_WAITING_MIN_DISPLAY_MS;
      g_mqttDefaultPendingAfterMinDisplay = false;
    }
    break;

  case MqttCommand::CMD_SYSTEM:
    if (g_sdOpInProgress) { Serial.println("[MQTT] system ignored (web open)"); break; }
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    g_mqttDefaultPendingAfterMinDisplay = false; // un vrai system prend le pas sur un default differe (v49)
    g_lastMqttWasDefault = false; // v50
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";
    currentMode=MODE_BLACK;
    if(nextGifFile){nextGifFile.close();nextGifFile=File();nextGifPath="";}
    freeBigramAll();
    currentMode=openBestMedia("/systems/"+cmd.arg+"/_default");
    displayedMaskSysName=cmd.arg;
    break;

  case MqttCommand::CMD_GAME:
    if (g_sdOpInProgress) { Serial.println("[MQTT] game ignored (web open)"); break; }
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    g_mqttDefaultPendingAfterMinDisplay = false; // un vrai jeu prend le pas sur un default differe (v49)
    g_lastMqttWasDefault = false; // v50
    // v104 -- bloc de reinitialisation de l'alternance overlay score/game_info
    // retire (hi-score port supprime).
    // v107 -- coupe-circuit anti-rafale (voir memoire projet) : marquee.sh
    // envoie ce signal special sur ce MEME topic marquee/cmd/game (deja
    // souscrit, deja dispatche) au moment ou une rafale de navigation est
    // detectee cote RB, AU LIEU de publier chaque jeu survole. Le raw565pack
    // dedie (SHUFFLE_GIF_PATH) boucle automatiquement via le
    // mecanisme MODE_GIF/gifResetCompat() deja existant -- aucune nouvelle
    // logique d'affichage necessaire, on reutilise openGif() tel quel. La
    // vraie position (le jeu reellement choisi) suit dans un 2e message
    // normal (marquee/cmd/game = "sys/rom") des que la rafale se termine,
    // qui remplacera cet affichage comme n'importe quel autre CMD_GAME.
    // v108 -- SHUFFLE_RAW565PACK_PATH renomme SHUFFLE_GIF_PATH (voir son
    // commentaire de declaration) : openGif() a besoin d'une cle en ".gif"
    // pour deriver correctement les vrais noms .raw565pack/.meta.
    // v109 -- SHUFFLE_ENABLED (voir sa declaration) : desactive pour test,
    // code garde intact. Si desactive et que "!SHUFFLE" arrivait quand meme
    // (ne devrait plus jamais arriver, marquee.sh v11 ne l'envoie plus),
    // ca tomberait dans le parsing normal juste en dessous : pas de '/'
    // trouve -> sysName=romName="!SHUFFLE" -> systeme inconnu -> echec
    // gracieux (repli sur default.png existant), aucun crash, juste un
    // affichage de repli generique inoffensif.
    if (SHUFFLE_ENABLED && cmd.arg == "!SHUFFLE")
    {
      if (openGif(String(SHUFFLE_GIF_PATH), true, true))
      {
        pngDrawn = false;
        currentPngPath = "";
        currentMode = MODE_GIF;
        Serial.println("[MQTT] game -> shuffle (anti-rafale)");
      }
      break;
    }
  {
    int slash=cmd.arg.indexOf('/');
    String sysName=(slash>=0)?cmd.arg.substring(0,slash):cmd.arg;
    String romName=(slash>=0)?cmd.arg.substring(slash+1):cmd.arg;
    String sysBase="/systems/"+sysName+"/_default";
    String gameBase="/systems/"+cmd.arg;
    if(imageFolder.length()>0)
      gameBase="/systems/"+sysName+"/"+imageFolder+"/"+romName;

    // Bucket derive de romName (deja extrait ci-dessus, sans cout SD
    // supplementaire) : flag lent par sous-dossier alphabetique au lieu
    // de par systeme entier (chantier "bucket").
    char bucketLetter = bucketLetterForFilename(romName);
    char slowFlag=sysBucketSlowFlag(sysName, bucketLetter);
    bool isSlow=(slowFlag=='L'||slowFlag=='l');
    bool needDrawMask = false;
    bool maskDrawn = false;

    // [DIAG-TEMP 2026-08-09] Regate derriere CMD_GAME_DEBUG_LOGS (2026-08-09,
    // suite) -- suspect comme confondeur possible du test 3do (overhead de
    // concatenation String + UART ajoute dans le chemin chaud, hypothese a
    // ecarter). Protocole : retester IDENTIQUE avec ce garde actif (donc ces
    // lignes desactivees) pour isoler si mes changements du jour influencent
    // le declenchement du crash mqttTask/LWIP.
    if (CMD_GAME_DEBUG_LOGS) Serial.println("[DIAG] enter sys=" + sysName
                   + " rom=" + romName
                   + " bucket=" + String(bucketLetter)
                   + " slowFlag=" + String(slowFlag)
                   + " isSlow=" + String(isSlow)
                   + " gameBase=" + gameBase);

    // FAST path (isSlow=0) : tenter directement le jeu en raw (drawPng gÃ¨re *.raw via fallback)
    // Si le jeu n'existe pas (sur ta SD ps2 sans /systems/ps2), tomber sur default.png/default.raw des _defaults.
    if(!isSlow)
    {
      String gamePng = gameBase + ".png";
      String gameGif = gameBase + ".gif";
      char sysT = sysDefaultType(sysName);
      // [DIAG-TEMP]
      if (CMD_GAME_DEBUG_LOGS) Serial.println("[DIAG] FAST path sysT=" + String(sysT));

      // Pre-check cache bigramme (2026-08-10, v69) -- meme mecanisme que le
      // chemin SLOW (findInGamesCache()), etendu ici : evite un scan SD
      // couteux (jusqu'a 3.3s mesure sur mame/S, 4641 entrees) quand le jeu
      // est absent, en sautant directement au repli default.png/default.raw
      // ci-dessous. games_cache.bin couvre tous les systemes sans
      // distinction de flag, cette verification n'a jamais eu de raison
      // d'etre limitee au chemin SLOW.
      bool fastSkipToDefault = false;
      {
        preloadBigram(sysName, romName);
        char cachedFast = findInGamesCache(sysName, romName);
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] fast cache pre-check sys=" + sysName
                       + " rom=" + romName + " cached=" + String(cachedFast));
        if (cachedFast == '?') fastSkipToDefault = true;
      }

      display->clearScreen();

      if (!fastSkipToDefault)
      {
        // Pour B : raw565pack d'abord (openGif sur .gif => .raw565pack+.meta)
        if(sysT == 'B')
        {
          if(openGif(gameGif, false, true))
          {
            pngDrawn = false;
            currentPngPath = "";
            currentMode = MODE_GIF;
            break;
          }
        }

        // Ordre standard : PNG d'abord
        if(drawPng(gamePng))
        {
          pngDrawn = true;
          currentPngPath = gamePng;
          currentMode = MODE_PNG;
          break;
        }

        // Puis GIF
        if(openGif(gameGif, false, true))
        {
          pngDrawn = false;
          currentPngPath = "";
          currentMode = MODE_GIF;
          break;
        }
      }

      // fallback : default.png/default.raw
      String defPng = "/systems/_defaults/default.png";
      if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] fast fallback -> " + defPng);
      display->clearScreen();
      if(drawPng(defPng))
      {
        pngDrawn = true;
        currentPngPath = defPng;
        currentMode = MODE_PNG;
        break;
      }

      // v92 -- BUG REEL confirme sur materiel : si TOUT echoue ici (jeu, gif,
      // ET default.png -- observe apres une reconnexion MQTT sous stress,
      // heap probablement sous pression transitoire a ce moment precis), le
      // code continuait silencieusement dans le bloc if(isSlow) juste en
      // dessous, qui ne s'execute JAMAIS pour un systeme rapide (isSlow=false,
      // ex. fbneo) -- currentMode restait donc bloque a sa valeur PRECEDENTE
      // (ex. MODE_PLAYLIST issu d'un resumePlaylist() anterieur, cf. v90/
      // "injoignable"), etat incoherent qui bloque aussi silencieusement le
      // declencheur d'overlay (exige MODE_PNG/MODE_GIF, jamais MODE_PLAYLIST,
      // meme raisonnement que le bug v85). Log volontairement TOUJOURS visible
      // (pas gate CMD_GAME_DEBUG_LOGS) : evenement rare/exceptionnel, besoin
      // de rester diagnosticable sans activer tous les logs verbeux.
      if (!isSlow)
      {
        Serial.println("[CMD_GAME] fast path totalement echoue (jeu+gif+default.png), maxalloc="
                       + String(ESP.getMaxAllocHeap()));
        currentMode = MODE_BLACK;
        // v93 -- laisser un currentPngPath perime (ex. le placeholder .raw565
        // de CMD_WAITING_MQTT) ici a cause un redessin errone plus tard (voir
        // pngToRaw565Path() et son commentaire v93) si un overlay se
        // declenche puis se termine avant le prochain CMD_GAME reussi.
        currentPngPath = "";
        pngDrawn = false;
        display->clearScreen();
        break;
      }

      // si meme default ne passe pas, on retombe sur le mask
    }

    // Fermer l'animation precedente pour liberer l'etat, mais sans forcement clear l'ecran.
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";

    if(isSlow)
    {
      needDrawMask = (displayedMaskSysName != sysName);
      // 1) afficher le mask d'attente si necessaire (sinon on le garde deja a l'ecran)
      if(needDrawMask)
      {
        String maskBase="/systems/_defaults/"+sysName;
        char maskType=sysDefaultType(sysName);

        if(maskType=='p')
        {
          String maskPng=maskBase+".png";
          display->clearScreen();
          if(drawPng(maskPng))
          {
            currentPngPath=maskPng;
            pngDrawn=true;
            currentMode=MODE_PNG;
            maskDrawn=true;
          }
          else
          {
            // PNG indisponible -> fallback GIF (au moins 1Ã¨re frame)
            String maskGif=maskBase+".gif";
            int fd=0;
            if(openGif(maskGif,true,true))
            {
              gif.playFrame(true,&fd);
              currentMode=MODE_GIF;
              pngDrawn=false;
              currentPngPath="";
              maskDrawn=true;
            }
            else
            {
              currentMode=MODE_BLACK;
            }
          }
        }
        else
        {
          // Mask obligatoire en raw565 (jamais raw565pack)
          String maskRaw565 = maskBase + ".raw565";
          if(drawRaw565(maskRaw565))
          {
            currentMode=MODE_BLACK; // on garde juste le contenu affichÃ© du mask
            pngDrawn=false;
            currentPngPath="";
            maskDrawn=true;
          }
          else
          {
            currentMode=MODE_BLACK;
          }
        }

        if (maskDrawn)
        {
          displayedMaskSysName=sysName;
        }
        else if (needDrawMask)
        {
          displayedMaskSysName="";
        }
      }

      // 2) precharger le bigramme (table chargee une fois par systeme)
      preloadBigram(sysName, romName);
      char cached=findInGamesCache(sysName, romName);

      // DEBUG pour comprendre pourquoi le jeu n'est pas affichÃ© (vs mask) -- voir CMD_GAME_DEBUG_LOGS
      if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] debug sys=" + sysName
                   + " rom=" + romName
                   + " bucket=" + String(bucketLetter)
                   + " cached=" + String(cached)
                   + " isSlow=" + String(isSlow)
                   + " gameBase=" + gameBase
                   + " slowFlag=" + String(slowFlag));
      // [DIAG-TEMP]
      if (CMD_GAME_DEBUG_LOGS) Serial.println("[DIAG] SLOW path cached=" + String(cached));

      // NE PAS clearScreen ici: le mask doit rester visible pendant le chargement.
      // En mode lent, on force le type d'affichage selon le flag systÃ¨me:
      // - sysType 'g'/'B' => raw565pack via openGif(...) (pas drawPng/raw565)
      // - sysType 'p'     => drawPng/raw565
      {
        char sysT = sysDefaultType(sysName);
        // [DIAG-TEMP]
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[DIAG] SLOW path sysT=" + String(sysT));
        // Si le jeu n'est PAS dans le cache bigram (cached='?'), fallback RAM direct.
        // Ne PAS forcer 'g' (qui ferait openGif() lent sur dossier de 800+ fichiers).
        if(cached == '?')
        {
          if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached=? -> fallback default.raw565 RAM t=" + String(millis()));
          if(drawDefaultRaw565Cached())
          {
            // Garder displayedMaskSysName=sysName pour que loop() ne clearScreen pas
            // (voir MODE_PNG: if(displayedMaskSysName.length()==0) display->clearScreen())
            displayedMaskSysName = sysName;
            pngDrawn=true; currentPngPath=""; currentMode=MODE_BLACK;
            if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached=? fallback OK t=" + String(millis()));
            break;
          }
        }
        // cached present: force le type d'affichage selon le flag systeme
        char cachedBefore = cached;
        if(sysT=='g' || sysT=='B') cached = 'g';
        else if(sysT=='p') cached = 'p';
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached force sysType=" + String(sysT)
                       + " cachedBefore=" + String(cachedBefore)
                       + " cachedAfter=" + String(cached));

        // Garde-fou heap bas (2026-08-09, demande utilisateur : le repli
        // ne doit se declencher QUE si le heap est reellement trop bas,
        // jamais systematiquement sur un flag B). Comportement normal
        // (heap suffisant) inchange : 'B' suit exactement le chemin 'g'
        // ci-dessous (raw565pack via openGif() en premier). Plusieurs
        // crashes reels confirmes (abort() dans lock_init_generic puis
        // dans make_shared<VFSFileImpl>, les deux lors d'un SD.open())
        // quand le heap est trop fragmente pour la moindre allocation,
        // meme petite -- seuil choisi avec de la marge par rapport au
        // plus gros besoin ponctuel de ce chemin (buffer de frame
        // raw565(pack), 8192 octets).
        if (ESP.getMaxAllocHeap() < CMD_GAME_MIN_HEAP_FOR_FILE_OPEN) {
          if (sysT == 'B') {
            // B a une alternative moins couteuse que raw565pack : raw565
            // statique (un seul SD.open()+read() de 8192 octets, pas de
            // .meta ni de cache des delais). Tente ce repli AVANT
            // d'abandonner sur l'image de secours generique -- perd
            // l'animation mais garde le vrai visuel du jeu.
            String rawPathDirect = gameBase + ".raw565";
            // Log TOUJOURS visible (pas gate derriere CMD_GAME_DEBUG_LOGS) :
            // evenement rare/exceptionnel (heap critique), pas du spam par
            // jeu -- besoin de rester diagnosticable sans activer tous les
            // logs verbeux (voir bug v60 "plus jamais de rawpack lu",
            // silencieux car cache derriere le flag desactive par defaut).
            Serial.println("[CMD_GAME] heap trop bas (maxalloc=" + String(ESP.getMaxAllocHeap())
                           + ") sysType=B -> tentative raw565 direct t=" + String(millis()));
            if (drawRaw565(rawPathDirect)) {
              pngDrawn = true;
              currentPngPath = rawPathDirect;
              // Garder displayedMaskSysName inchange pour que loop() ne
              // clearScreen pas (meme raison que le repli raw565 de la
              // branche 'g' plus bas).
              currentMode = MODE_PNG;
              break;
            }
          }
          // g/p purs (pas d'alternative moins couteuse), ou B sans .raw565
          // propre a ce jeu: repli direct sur l'image de secours deja en
          // RAM (drawDefaultRaw565Cached(), zero allocation necessaire)
          // plutot que tenter l'ouverture et risquer un abort().
          Serial.println("[CMD_GAME] heap trop bas (maxalloc=" + String(ESP.getMaxAllocHeap())
                         + ") -> repli direct default.raw565 RAM t=" + String(millis()));
          if (drawDefaultRaw565Cached()) {
            displayedMaskSysName = sysName;
            pngDrawn = true; currentPngPath = ""; currentMode = MODE_BLACK;
            break;
          }
          // Meme le repli RAM echoue (defaultRaw565Cached jamais charge) :
          // continue vers le chemin normal, mieux qu'un ecran fige.
        }
      }
      if(cached=='p')
      {
        String path=gameBase+".png";
        // Async pngle_new() Ã©choue systÃ©matiquement en tÃ¢che => fallback synchrone
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow PNG fallback sync sys=" + sysName + " path=" + path);

        currentPngPath = path;
        currentPngAsyncWanted = false;
        asyncPngCancel = true;

        // Remplacer le mask par l'image PNG (garder le mask si affiche)
        // En LENT, on ne clear que si pas de mask actif.
        if (displayedMaskSysName.length() == 0)
        {
          if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow clearScreen (no mask) BEFORE t=" + String(millis()));
          display->clearScreen();
          if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow clearScreen AFTER t=" + String(millis()));
        }
        else
        {
          if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow keep mask on screen during load t=" + String(millis()));
        }

        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow before drawPng t=" + String(millis())
                       + " maskLen=" + String(displayedMaskSysName.length()));
        bool okDraw = drawPng(path);
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow after drawPng ok=" + String(okDraw ? "1" : "0")
                       + " t=" + String(millis()));

        if(okDraw)
        {
          pngDrawn = true;
          displayedMaskSysName = "";
          currentMode = MODE_PNG;
        }
        else
        {
          // PNG indisponible -> fallback GIF
          pngDrawn = false;
          String gameGif = gameBase + ".gif";

          int fd = 0;
          if(openGif(gameGif, false, true))
          {
            gif.playFrame(true, &fd);
            displayedMaskSysName = "";
            currentMode = MODE_GIF;
          }
          else
          {
            currentMode = MODE_BLACK;
            display->clearScreen();
            displayedMaskSysName = "";
          }

          currentPngPath = "";
        }

        break;
      }
      else if(cached=='g')
      {
        // Tentative raw565pack via openGif (ne pas clearScreen, le mask reste visible)
        String gifPath=gameBase+".gif";
        if(openGif(gifPath,false,true))
        {
          currentMode=MODE_GIF;
          displayedMaskSysName="";
          loadBigramTable(sysName);
          break;
        }
        loadBigramTable(sysName);

        // raw565pack Ã©chouÃ© â†’ tenter le raw565 spÃ©cifique du jeu (drawRaw565 direct)
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached=g raw565pack fail -> try game raw565 t=" + String(millis()));
        {
          String rawPath=gameBase+".raw565";
          if(drawRaw565(rawPath))
          {
            pngDrawn=true;
            currentPngPath=rawPath;
            // Garder displayedMaskSysName inchangÃ© pour que loop() ne clearScreen pas.
            // MODE_PNG avec pngDrawn=true â†’ loop() ne touche pas Ã  l'affichage.
            currentMode=MODE_PNG;
            if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached=g game raw565 OK t=" + String(millis()));
            break;
          }
        }
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached=g game raw565 fail -> fallback default.raw565 t=" + String(millis()));
        if(drawDefaultRaw565Cached())
        {
          pngDrawn=true;
          currentPngPath="";
          displayedMaskSysName="";
          currentMode=MODE_PNG;
          if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] slow cached=g fallback default.raw565 OK t=" + String(millis()));
          break;
        }
        // fallback mem epuise -> probe3 classique
      }

      // 3) fallback: tester existence PNG/GIF sans effacer l'ecran
      {
        String pngPath=gameBase+".png";

        unsigned long tProbeStart = millis();
        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] probe3 start t=" + String(tProbeStart) + " png=" + pngPath);

        if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] probe3 calling drawPng (skip SD.exists) t=" + String(millis()));

        if(drawPng(pngPath))
        {
          currentPngPath=pngPath; pngDrawn=true; currentMode=MODE_PNG;
          displayedMaskSysName="";
          loadBigramTable(sysName);
          break;
        }
        else
        {
          if (CMD_GAME_DEBUG_LOGS) Serial.println("[CMD_GAME] probe3 drawPng failed t=" + String(millis()));
        }
      }

      // Dernier recours (N classique) si tout echoue
      currentMode=openBestMedia(gameBase,sysBase);
      loadBigramTable(sysName);
      break;
    }

    // NORMAL: comportement actuel maintenu
    currentMode=MODE_BLACK;
    preloadBigram(sysName, romName);
    char cached=findInGamesCache(sysName, romName);

    if(cached=='p'){
      String path=gameBase+".png"; display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;currentMode=MODE_PNG;break;}
      loadBigramTable(sysName);
    } else if(cached=='g'){
      if(openGif(gameBase+".gif")){pngDrawn=false;currentPngPath="";currentMode=MODE_GIF;break;}
    }
    currentMode=openBestMedia(gameBase,sysBase);
    loadBigramTable(sysName);
    break;
  }

  case MqttCommand::CMD_STARTCLIP:
    if (g_sdOpInProgress) { Serial.println("[MQTT] startclip ignored"); break; }
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    g_mqttDefaultPendingAfterMinDisplay = false;
    g_lastMqttWasDefault = true; // v50
    Serial.println("[MQTT] startgameclip -> playlist");
    resumePlaylist();
    break;

  case MqttCommand::CMD_RESUMESYS:
    if (g_sdOpInProgress) { Serial.println("[MQTT] resumesys ignored"); break; }
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    g_mqttDefaultPendingAfterMinDisplay = false;
    g_lastMqttWasDefault = false; // v50
    Serial.println("[MQTT] resumesys -> "+cmd.arg);
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";
    currentMode=MODE_BLACK;
    if(nextGifFile){nextGifFile.close();nextGifFile=File();nextGifPath="";}
    freeBigramAll();
    currentMode=openBestMedia("/systems/"+cmd.arg+"/_default");
    displayedMaskSysName=cmd.arg;
    break;

  case MqttCommand::CMD_SHOW_CONFIG:
    // Declenche depuis Recalbox (script "Config Web DMD") pour retrouver/
    // afficher l'IP du DMD sans toucher au WiFi -- reutilise exactement
    // l'affichage deja declenche par handleDmdOpen() quand la page web est
    // ouverte normalement.
    if (g_sdOpInProgress) { Serial.println("[MQTT] show_config ignored (web deja ouvert)"); break; }
    // v150 -- un CMD_SHOW_CONFIG est un vrai message MQTT au meme titre que
    // CMD_GAME/CMD_SYSTEM/CMD_DEFAULT/CMD_SCORE (qui levent deja ce drapeau,
    // voir nettoyage pre-merge master) -- change de mode d'affichage de
    // toute facon juste apres, mais evite que le clignotement "en attente"
    // reprenne si jamais MODE_CONFIG est quitte sans autre message entre
    // temps.
    g_mqttConnectedScreenUntilMs = 0;
    clearFirstBoot();
    webDmdSetMainMsg("WEB DMD CONFIG");
    // Ligne 2 : message complet (defile automatiquement si >128px, cf boucle
    // de rendu MODE_CONFIG) plutot que la seule IP -- plus clair pour
    // l'utilisateur qui regarde l'ecran du DMD sans autre contexte.
    // Repli 0.0.0.0 (bug corrige 2026-08-05, seul site du fichier qui ne
    // l'avait pas) : WiFi.localIP() est vide en mode AP pur.
    {
      String ip = WiFi.localIP().toString();
      if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
      if (ip == "0.0.0.0") ip = "192.168.4.1";
      webDmdPause(trOpenBrowserAt(ip), 0xFFE0);
    }
    break;

  case MqttCommand::CMD_WIFI_RECOVERY:
    // Declenche depuis Recalbox (script "WiFi Recovery DMD") quand le web
    // config est injoignable via l'IP STA normale (ex. pare-feu inter-VLAN).
    // Redemarre en WIFI_AP pur (seul mode fiable mesure sur ce materiel, cf
    // AP_STA rejete precedemment) avec compte a rebours de 3 min avant retour
    // automatique.
    Serial.println("[MQTT] wifi_recovery -> reboot en AP secours");
    writeConfigFlag("force_ap_recovery", "1");
    delay(100);
    ESP.restart();
    break;

  case MqttCommand::CMD_REBOOT:
    // Declenche depuis Recalbox (script "Reboot DMD") : redemarrage simple,
    // sans condition (pas de garde g_sdOpInProgress) -- c'est le bouton de
    // secours en cas de DMD bloque/affichage fige, il ne doit jamais pouvoir
    // etre lui-meme ignore.
    Serial.println("[MQTT] reboot demande par l'utilisateur");
    delay(100);
    ESP.restart();
    break;

  // Luminosite en direct via MQTT (v70) : cmd.arg = pourcentage 0-100 en
  // texte (ex. publie par "mosquitto_pub -t marquee/cmd -m 'CMD=brightness
  // ARG=50'" depuis v148 -- topic unique, plus marquee/cmd/brightness).
  // RAM uniquement (comme les autres commandes MQTT) -- pas de reecriture
  // de /config.ini ici, ca reste le role explicite de "Sauvegarder" sur la
  // page web. setBrightness8() est sans risque a appeler a tout moment
  // (reecrit juste les bits OE/PWM du buffer DMA deja actif).
  case MqttCommand::CMD_BRIGHTNESS:
  {
    // v150 -- meme raisonnement que CMD_SCORE/CMD_SHOW_CONFIG (nettoyage
    // pre-merge master) : un vrai message MQTT recu, meme s'il ne change ni
    // currentMode ni currentPngPath (la luminosite seule n'affecte que le
    // PWM du buffer DMA deja affiche) -- sans ca, l'ecran "RecalBox
    // connectee" pouvait continuer a clignoter indefiniment alors que du
    // trafic MQTT reel passait bel et bien.
    g_mqttConnectedScreenUntilMs = 0;
    int pct = cmd.arg.toInt();
    if (pct >= 0 && pct <= 100) {
      screenBrightness = map(pct, 0, 100, 0, 255);
      if (display) display->setBrightness8(screenBrightness);
      Serial.println("[MQTT] brightness -> " + String(pct) + "%");
    } else {
      Serial.println("[MQTT] brightness ignoree (valeur hors 0-100: " + cmd.arg + ")");
    }
    break;
  }

  // Pas de luminosite relatif +10%/-10% via MQTT (v77) : declenche par un
  // script Recalbox ("mosquitto_pub -t marquee/cmd -m 'CMD=brightness_up'"
  // ou "CMD=brightness_down" depuis v148 -- topic unique), payload ignore.
  // Contrairement a CMD_BRIGHTNESS ci-dessus (RAM only), ces 2 commandes
  // PERSISTENT la nouvelle valeur dans /config.ini via writeConfigFlag() --
  // choix utilisateur explicite, un +10%/-10% declenche depuis un script
  // doit survivre a un reboot sans passer par le bouton "Sauvegarder" web.
  // Ecriture SD conditionnelle (seulement si la valeur clampee change
  // reellement, ex. deja a 100% et +10% redemande) pour ne pas ecrire sur
  // la SD inutilement.
  case MqttCommand::CMD_BRIGHTNESS_UP:
  case MqttCommand::CMD_BRIGHTNESS_DOWN:
  {
    // v150 -- meme motif que CMD_BRIGHTNESS juste au-dessus.
    g_mqttConnectedScreenUntilMs = 0;
    int curPct = (int)round(screenBrightness * 100.0 / 255.0);
    int delta  = (cmd.type == MqttCommand::CMD_BRIGHTNESS_UP) ? 10 : -10;
    int newPct = constrain(curPct + delta, 0, 100);
    if (newPct != curPct) {
      screenBrightness = map(newPct, 0, 100, 0, 255);
      if (display) display->setBrightness8(screenBrightness);
      writeConfigFlag("brightness", String(newPct));
      Serial.println("[MQTT] brightness " + String(delta > 0 ? "+" : "") + String(delta) +
                      "% -> " + String(newPct) + "% (sauvegarde config.ini)");
    } else {
      Serial.println("[MQTT] brightness deja au " + String(newPct == 0 ? "minimum" : "maximum") + " (" + String(newPct) + "%)");
    }
    break;
  }

  // v104 -- cases CMD_INGAME/CMD_GAME_INFO/CMD_ACHIEVEMENT restent retirees
  // (game_info/achievement port supprime, pas redemande). CMD_SCORE
  // reintroduit ci-dessous en v110, voir entete changelog complet.
  case MqttCommand::CMD_SCORE:
  {
    if (g_sdOpInProgress) { Serial.println("[MQTT] score ignore (web open)"); break; }
    // v149 -- un CMD_SCORE est un "vrai message MQTT" au meme titre que
    // CMD_GAME/CMD_SYSTEM/CMD_DEFAULT (qui levent deja ce drapeau) : sans
    // cette ligne, un score recu pendant l'ecran "RecalBox connectee"
    // (CMD_WAITING_MQTT) memorisait CET ECRAN comme mode a restaurer
    // (juste en dessous) -- le DMD restait alors coince en boucle
    // score/ecran-connectee indefiniment. Voir changelog v149 + DECISIONS.md.
    g_mqttConnectedScreenUntilMs = 0;
    // v110 -- ne memorise le mode a restaurer QUE si on n'est pas deja en
    // train d'afficher un score (sinon un 2e score arrivant pendant
    // l'affichage du 1er ecraserait g_modeBeforeScore avec MODE_SCORE
    // lui-meme -- le jeu redemarrerait alors en MODE_SCORE au lieu de
    // reprendre le jeu, cassant la garantie anti-blocage).
    if (currentMode != MODE_SCORE) g_modeBeforeScore = currentMode;
    // v111 -- prefixe optionnel "@<ms>|" (voir SCORE_DURATION_OVERRIDE_MIN/
    // MAX_MS) : duree d'affichage PAR MESSAGE au lieu du fixe
    // SCORE_DISPLAY_DURATION_MS -- retire du payload AVANT drawScoreScreen()
    // (jamais visible a l'affichage, purement un en-tete de controle).
    // Format invalide/hors bornes -> ignore silencieusement, comportement
    // par defaut inchange (aucun risque de plantage sur un payload malforme).
    {
      String payload = cmd.arg;
      unsigned long durMs = SCORE_DISPLAY_DURATION_MS;
      if (payload.length() > 1 && payload.charAt(0) == '@') {
        int sep = payload.indexOf('|');
        if (sep > 1) {
          long parsed = payload.substring(1, sep).toInt();
          if (parsed >= SCORE_DURATION_OVERRIDE_MIN_MS && parsed <= SCORE_DURATION_OVERRIDE_MAX_MS) {
            durMs = (unsigned long)parsed;
            payload = payload.substring(sep + 1);
          }
        }
      }
      drawScoreScreen(payload);
      // v111 -- BUG REEL introduit par erreur PENDANT cette meme session
      // (perdu lors de la restructuration pour le prefixe "@ms|" ci-dessus) :
      // cette ligne avait disparu, donc currentMode ne passait JAMAIS a
      // MODE_SCORE -- drawScoreScreen() dessinait bien le texte, mais des
      // l'iteration loop() suivante, le switch(currentMode) restait sur
      // l'ancien mode (MODE_GIF) et redessinait la frame suivante du
      // marquee PAR-DESSUS, expliquant le "flash" d'une seule frame
      // rapporte par l'utilisateur (rawpack jamais interrompu, exactement
      // son hypothese). Confirme sur materiel via un detecteur generique
      // de transition de currentMode : AUCUNE transition n'etait jamais
      // loguee malgre un texte visible un instant -- preuve que
      // currentMode ne changeait tout simplement pas.
      currentMode = MODE_SCORE;
      g_scoreShowUntilMs = millis() + durMs;
      Serial.println("[MQTT] score -> affiche " + String(durMs / 1000.0, 1) + "s puis retour auto au jeu");
    }
    break;
  }

  // Apercu de theme horloge depuis la page web (v72, onglet Horloge ;
  // fixes v73 ci-dessous) -- cmd.arg = "stop" (quitte la page ou aucun
  // thème selectionne) ou un theme ("-1".."9", cf. select #clock_theme).
  // RAM only / affichage uniquement, ne touche jamais /config.ini
  // (independant du bouton "Sauvegarder"). Jamais emise par la Recalbox
  // (topic web uniquement).
  case MqttCommand::CMD_CLOCK_PREVIEW:
  {
    if (cmd.arg == "stop") {
      // v73 : NE PLUS appeler resumePlaylist() ici -- ce "stop" arrive a
      // chaque fois qu'on quitte la page Horloge (pagehide/beforeunload),
      // y compris pour changer d'onglet vers Basic/Network/Media, PAS
      // seulement en quittant tout le web config. Or g_sdOpInProgress reste
      // vrai dans ce cas (repose de toute facon par le triggerWebConfigModeSoft()
      // de la page suivante) -- relancer la playlist ici cassait la
      // protection web (ecran "WEB DMD CONFIG" annule, GIFs qui repartent).
      // resumePlaylist() reste le rôle exclusif du bouton "Reprendre DMD"
      // (/dmd-resume, voir webDmdResume()). On se contente de reafficher
      // l'ecran de pause config deja pose (g_sdOpMsg/g_sdOpSubMsg encore a
      // jour depuis le dernier triggerWebConfigModeSoft()).
      Serial.println("[CLOCK] preview stop");
      currentMode = MODE_CONFIG;
      display->clearScreen();
      webDmdForceRedraw();
      break;
    }
    // v73 : garde g_sdOpInProgress supprimee -- elle bloquait 100% des
    // tentatives (voir en-tete de fichier v73) : cette commande vient
    // justement de la page Horloge, qui vient elle-meme de poser
    // g_sdOpInProgress=true en se chargeant. Aucun conflit SD reel a
    // proteger ici (webServer mono-thread).
    int previewTheme = cmd.arg.toInt();
    if (previewTheme < -1 || previewTheme >= RETRO_THEME_COUNT) {
      Serial.println("[CLOCK] preview ignoree (theme invalide: " + cmd.arg + ")");
      break;
    }
    // Meme nettoyage que CMD_STOP : interrompt proprement un GIF/PNG en
    // cours avant de basculer sur l'apercu.
    gif.close(); gifOpened=false; currentPngPath=""; pngDrawn=false;
    currentMode=MODE_BLACK; display->clearScreen();
    Serial.println("[CLOCK] preview theme=" + cmd.arg);
    showClock(previewTheme); // bloquant, sans limite de duree -- voir showClock()
    break;
  }

  default: break;
  }
}

// --------------------------------------------------
// MQTT callback
// --------------------------------------------------
void onMqttMessage(char *topic, byte *payload, unsigned int length)
{
  // v142 -- voir commentaire complet pres de sa declaration : un message
  // RECU (quel qu'il soit) est la preuve la plus directe que le lien MQTT
  // est reellement UTILE, pas seulement "connecte" au sens transport.
  g_lastMqttUsefulMs=millis();
  String t=String(topic); String msg="";
  // reserve() : sans lui, la concatenation octet-par-octet reallouait le
  // buffer de la String a chaque caractere dans le pire cas -- ce handler
  // s'execute pour CHAQUE message MQTT recu (seule l'action qui en decoule
  // est ignoree via g_sdOpInProgress plus bas, pas ce traitement). Reste une
  // optimisation valable en general, meme si non liee au cas heap-critique
  // reporte le 2026-07-26 (RB eteinte au moment du test -> aucun message
  // MQTT recu, seules des tentatives de connexion en echec -- ce handler
  // n'avait donc pas pu s'executer ce jour-la).
  msg.reserve(length + 1);
  for(unsigned int i=0;i<length;i++) msg+=(char)payload[i];
  msg.trim();
  Serial.println("[MQTT] "+t+" -> "+msg);
  mqttLogAdd(t,msg);

  if(mqttCmdMutex==nullptr) return;
  if(xSemaphoreTake(mqttCmdMutex,pdMS_TO_TICKS(10))!=pdTRUE) return;

  // Fenetre de grace CMD_WAITING_MQTT (voir mqttTask()) : un message RETENU
  // (mosquitto -r, rejoue par le broker des la souscription -- ex:
  // "system=lastplayed" publie par le pont marquee lors d'une session
  // precedente) arrive quasi instantanement a la connexion et ecraserait
  // sinon l'image de secours avant meme qu'elle soit visible. On ignore
  // uniquement system/game pendant cette fenetre tres courte (1.5s) --
  // stop/show_config/wifi_recovery/reboot restent des actions explicites,
  // jamais supprimees.
  // "default" RETIRE de ce filtre (v45, 2026-08-03, bug reel confirme :
  // detection clip/demo ne fonctionnait pas si RB etait DEJA en mode demo au
  // moment de la connexion MQTT -- son message "default" arrive alors lui
  // aussi quasi instantanement, dans cette meme fenetre, et etait ignore a
  // tort comme s'il s'agissait d'un retenu perime). Contrairement a
  // system/game (qui peuvent afficher un JEU perime/faux), "default" ne
  // presente aucun risque a etre honore immediatement, retenu ou frais : il
  // reflete toujours le DERNIER etat connu reel de RB (veille/demo), jamais
  // "faux" en soi -- et depuis le retrait de la reprise auto par delai (v45
  // egalement), c'est desormais le SEUL moyen de sortir de l'ecran d'attente
  // si RB est deja en demo a la connexion.
  // v102 -- inWaitingGrace (le filtre lui-meme) retire : voir le commentaire
  // complet pres du dispatch system/game plus bas, qui etait le DERNIER
  // usage de cette variable (system/game desormais traites comme default,
  // v45). g_mqttWaitingUntilMs/MQTT_WAITING_GRACE_MS retires a leur tour en
  // v150 (nettoyage pre-merge master) -- restaient ecrits sans plus jamais
  // etre lus depuis ce retrait, code mort confirme par revue.

  // v96 -- default/system/game passent desormais par leur slot dedie (voir
  // declaration de g_pendingGame et al.) au lieu du pendingCmd generique,
  // pour ne plus jamais etre ecrases par un type de commande different
  // (score/game_info/ingame...) arrivant juste apres dans la meme rafale.
  // v148 -- FUSION des 12 topics marquee/cmd/* en UN SEUL topic
  // (marquee/cmd), retour utilisateur explicite (session du 02/09 tard) :
  // reduire l'EXPOSITION au blocage TX post-CONNACK plutot que d'attendre
  // sa cause racine (mur de plateforme atteint, voir memoire projet) --
  // 12 topics = jusqu'a 24 envois SUBSCRIBE en rafale a chaque
  // reconnexion, largement au-dessus du plafond de ~16 segments TCP
  // simultanement non-accuses trouve dans le sdkconfig du core (v147).
  // 1 seul topic = 1 seul SUBSCRIBE par reconnexion (+ mqttEventTopic,
  // topic ES brut inchange car pas sous notre controle de publication).
  // Format du payload, meme esprit que extractField()/le topic evenement
  // ES existant (paires cle=valeur separees par des espaces, pas de
  // dependance JSON ajoutee) : "CMD=<nom> ARG=<reste du message>" --
  // CMD toujours en 1er (mot simple, sans espace, extractField() suffit),
  // ARG toujours en dernier et prend tout le reste du message JUSQU'A LA
  // FIN (pas jusqu'au prochain espace) pour rester compatible avec des
  // arguments contenant des espaces/pipes (ex. score = "@6800|DESCRIPTION|
  // texte avec espaces"). <nom> reprend exactement les anciens suffixes
  // de topic (stop/default/system/game/show_config/wifi_recovery/reboot/
  // brightness/brightness_up/brightness_down/score/ingame) -- meme
  // logique de dispatch qu'avant, seule la SOURCE du nom de commande
  // change (cmd extrait du payload au lieu de t).
  // v150 -- game/system/default/ingame arrivent desormais sur leur propre
  // topic retenu dedie (voir commentaire complet pres de subscribeTopics[]),
  // meme format de payload "CMD=<nom> ARG=<valeur>" qu'avant -- extraction
  // et dispatch identiques, seule la condition d'entree est elargie.
  if (t=="marquee/cmd" || t=="marquee/cmd/game" || t=="marquee/cmd/system"
      || t=="marquee/cmd/default" || t=="marquee/cmd/ingame")
  {
    String cmd = extractField(msg, "CMD");
    int argIdx = msg.indexOf("ARG=");
    String arg = (argIdx >= 0) ? msg.substring(argIdx + 4) : "";
    if     (cmd=="stop")    pendingCmd=MqttCommand(MqttCommand::CMD_STOP,"");
    else if(cmd=="default") g_pendingDefault = true;
    // v102 -- BUG REEL confirme sur materiel, reproduit plusieurs fois
    // (karatour, ctribe x2 -- la MEME commande "game" pour le MEME jeu
    // echoue silencieusement la 1ere fois dans la rafale post-reconnexion,
    // puis reussit normalement quelques dizaines de secondes plus tard
    // hors rafale) : inWaitingGrace (fenetre 1.5s, re-armee a CHAQUE
    // reconnexion via CMD_WAITING_MQTT, pas juste au 1er boot) filtrait
    // ENCORE system/game -- alors que "default" avait deja ete
    // explicitement retire de ce meme filtre en v45 pour EXACTEMENT la
    // meme raison ("un message RETENU reflete toujours le dernier etat
    // REEL connu de RB, jamais faux en soi"), argument qui s'applique
    // identiquement a system/game. system/game etaient donc
    // systematiquement ignores (sans aucun log, contrairement aux autres
    // skips explicites de ce fichier) des qu'ils faisaient partie de la
    // rafale de messages retenus livree juste apres resouscription --
    // expliquant precisement le symptome observe ("CMD_GAME/SYSTEM
    // silencieux post-reconnexion", documente comme mystere non resolu
    // avant cette decouverte). Fix : retire du filtre, meme traitement
    // que default depuis v45.
    else if(cmd=="system")  { lastSysName=arg; g_pendingSystemArg=arg; g_pendingSystem=true; }
    else if(cmd=="game")    { g_pendingGameArg=arg; g_pendingGame=true; }
    else if(cmd=="show_config") pendingCmd=MqttCommand(MqttCommand::CMD_SHOW_CONFIG,"");
    else if(cmd=="wifi_recovery") pendingCmd=MqttCommand(MqttCommand::CMD_WIFI_RECOVERY,"");
    else if(cmd=="reboot")        pendingCmd=MqttCommand(MqttCommand::CMD_REBOOT,"");
    else if(cmd=="brightness")    pendingCmd=MqttCommand(MqttCommand::CMD_BRIGHTNESS,arg);
    else if(cmd=="brightness_up")   pendingCmd=MqttCommand(MqttCommand::CMD_BRIGHTNESS_UP,"");
    else if(cmd=="brightness_down") pendingCmd=MqttCommand(MqttCommand::CMD_BRIGHTNESS_DOWN,"");
    // v110 -- score : payload vide ignore (rien a afficher). game_info/
    // achievement restent retires (v104), pas redemandes par l'utilisateur.
    else if(cmd=="score") { if(arg.length()>0) pendingCmd=MqttCommand(MqttCommand::CMD_SCORE,arg); }
    // v122 -- ingame (voir g_recalboxInGame) : simple etat courant, pas
    // une commande d'affichage -- ecrit directement ici, pas de passage
    // par pendingCmd/le switch de processPendingMqttCommand().
    else if(cmd=="ingame") g_recalboxInGame = (arg=="1");
  }
  else if(t==mqttEventTopic)
  {
    String ev=extractField(msg,"EVENT");
    String inGame=extractField(msg,"IN_GAME");
    String lastSys=extractField(msg,"LAST_SYS");
    Serial.println("[EVENT] ev="+ev+" in_game="+inGame+" sys="+lastSys);
    if(ev=="startgameclip"&&inGame=="0")
      pendingCmd=MqttCommand(MqttCommand::CMD_STARTCLIP,"");
    else if((ev=="stopgameclip"||ev=="wakeup"||ev=="systembrowsing")&&inGame=="0")
    {
      String sys=(lastSys.length()>0)?lastSys:lastSysName;
      if(sys.length()>0){lastSysName=sys;pendingCmd=MqttCommand(MqttCommand::CMD_RESUMESYS,sys);}
      else g_pendingDefault = true; // v96 -- slot dedie, voir commentaire pres de sa declaration
    }
  }
  xSemaphoreGive(mqttCmdMutex);
}

// --------------------------------------------------
// v1/v2 -- piste UDP (voir TRANSPORT_PLAN_UDP.md)
// --------------------------------------------------
// Fire-and-forget, sans connexion/handshake -- objectif : eliminer la classe
// de bug MQTT documentee (mur de plateforme, connect()/subscribe() bloquants,
// voir memoire projet) en s'affranchissant de tout etat TCP a faire "caler".
// v1 -- prototype limite a CMD_SCORE seul, VALIDE sur materiel reel le
// 2026-09-08 (paquet UDP -> [UDP] logue -> CMD_SCORE affiche -> retour
// normal, voir DECISIONS.md). v2 -- etendu a TOUT le jeu de commandes du
// bloc marquee/cmd de onMqttMessage() (stop/default/system/game/
// show_config/wifi_recovery/reboot/brightness*/score/ingame), meme
// dispatch, duplique plutot que factorise (voir commentaire pres du
// dispatch plus bas). PAS ENCORE teste sur materiel au-dela de score.
// Meme format de payload que MQTT (v148) : "CMD=<nom> ARG=<reste>", memes
// extractField()/dispatch que onMqttMessage() -- seule la SOURCE change.
// Non bloquant : parsePacket() renvoie 0 immediatement s'il n'y a rien a
// lire (pas d'attente), donc un appel a chaque loop() est sans cout notable
// quand aucun datagramme n'arrive.
// MQTT reste actif en parallele (comparaison prevue par le plan) --
// AUCUNE des 2 voies n'est encore coupee.
void handleUdpCommand()
{
  int packetSize = dmdUdp.parsePacket();
  if (packetSize <= 0) return;

  // v1 -- 255 = tres large marge (le plus long payload MQTT connu, le score
  // multi-rangs, fait ~90 octets, voir commentaire v79 pres de
  // mqttClient.setBufferSize()) ; un paquet UDP plus long que le buffer est
  // tronque par read(), jamais un debordement.
  char buf[256];
  int len = dmdUdp.read(buf, sizeof(buf) - 1);
  if (len <= 0) return;
  buf[len] = '\0';
  String msg = String(buf);
  msg.trim();
  Serial.println("[UDP] " + dmdUdp.remoteIP().toString() + " -> " + msg);
  mqttLogAdd("udp/cmd", msg);

  String cmd = extractField(msg, "CMD");
  int argIdx = msg.indexOf("ARG=");
  String arg = (argIdx >= 0) ? msg.substring(argIdx + 4) : "";

  if (mqttCmdMutex == nullptr) return;
  if (xSemaphoreTake(mqttCmdMutex, pdMS_TO_TICKS(10)) != pdTRUE) return;

  // v2 -- extension : meme jeu de commandes que le bloc marquee/cmd de
  // onMqttMessage() (voir son commentaire complet, v148/v150), DELIBEREMENT
  // duplique plutot que factorise -- onMqttMessage() est un chemin
  // eprouve en conditions reelles depuis des mois, ne pas le restructurer
  // pour ce prototype encore non valide en charge. game_info/achievement
  // restent hors scope (deja retires cote MQTT, v104). CMD_STARTCLIP/
  // CMD_RESUMESYS (topic evenement ES brut, pas marquee/cmd) restent hors
  // scope : rien cote RB1 ne publierait ca en UDP, ce topic n'est pas sous
  // notre controle de publication.
  if      (cmd=="stop")    pendingCmd=MqttCommand(MqttCommand::CMD_STOP,"");
  else if (cmd=="default") g_pendingDefault = true;
  else if (cmd=="system")  { lastSysName=arg; g_pendingSystemArg=arg; g_pendingSystem=true; }
  else if (cmd=="game")    { g_pendingGameArg=arg; g_pendingGame=true; }
  else if (cmd=="show_config")     pendingCmd=MqttCommand(MqttCommand::CMD_SHOW_CONFIG,"");
  else if (cmd=="wifi_recovery")   pendingCmd=MqttCommand(MqttCommand::CMD_WIFI_RECOVERY,"");
  else if (cmd=="reboot")          pendingCmd=MqttCommand(MqttCommand::CMD_REBOOT,"");
  else if (cmd=="brightness")      pendingCmd=MqttCommand(MqttCommand::CMD_BRIGHTNESS,arg);
  else if (cmd=="brightness_up")   pendingCmd=MqttCommand(MqttCommand::CMD_BRIGHTNESS_UP,"");
  else if (cmd=="brightness_down") pendingCmd=MqttCommand(MqttCommand::CMD_BRIGHTNESS_DOWN,"");
  else if (cmd=="score") { if(arg.length()>0) pendingCmd=MqttCommand(MqttCommand::CMD_SCORE,arg); }
  else if (cmd=="ingame") g_recalboxInGame = (arg=="1");

  xSemaphoreGive(mqttCmdMutex);
}

// --------------------------------------------------
// v137 -- Heartbeat CPU0 (diagnostic bissection round 2, voir DECISIONS.md)
// --------------------------------------------------
// Instrument pour trancher entre 2 hypotheses jamais distinguees jusqu'ici
// pour le blocage subscribe() ~9-10s (WiFi/RSSI/heap tous sains a chaque
// fois, cote broker confirme ne rien recevoir du DMD) :
//   (a) un vrai blocage bas niveau (syscall socket/lwIP) pendant l'appel
//   (b) la tache qui appelle subscribe() n'a simplement pas la main
//       pendant ~9-10s (CPU0 accapare par autre chose -- rendu GIF,
//       lecture SD/SPI deja documentee comme bloquante ailleurs) -- le
//       SUBDIAG existant mesure le temps ecoule AUTOUR de tout l'appel,
//       pas le temps reellement passe dans le syscall bloquant lui-meme,
//       donc ne peut pas distinguer (a) de (b).
// Tache independante, tres legere, epinglee sur le MEME coeur que loop()
// (LoopCore=0) : incremente un compteur toutes les ~20ms. Si ce compteur
// continue d'avancer normalement pendant un subscribe() bloque -> (a),
// vrai blocage reseau. S'il se fige lui aussi -> (b), famine CPU0, piste
// a rediriger vers le rendu/SD plutot que MQTT/lwIP. Note : une
// contention mqttTask-vs-loop() sur le meme coeur a deja ete testee une
// fois (v107, 18/08) en deplacant mqttTask() entier vers le coeur 1 --
// resultat REFUTE (meme signature de tempete observee), mais ce test
// grossier ne mesurait pas directement si CPU0 stallait reellement au
// moment precis d'un subscribe() -- ce heartbeat le mesure enfin.
static volatile uint32_t g_heartbeatCounter = 0;

static void heartbeatTask(void *param)
{
  (void)param;
  for (;;)
  {
    g_heartbeatCounter++;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// --------------------------------------------------
// MQTT task
// --------------------------------------------------
void mqttTask(void *param)
{
  (void)param;
  vTaskDelay(pdMS_TO_TICKS(MQTT_START_DELAY_MS));
  unsigned long lastMqttConnectedMs=millis();
  g_lastMqttUsefulMs=millis(); // v142 -- meme motif que lastMqttConnectedMs ci-dessus : evite un declenchement immediat du filet si le tout 1er connect()/subscribe() met du temps
  // Compteur de cycles consecutifs "WiFi non connecte" (2026-08-05, demande
  // utilisateur) -- pilote l'alerte "No wifi, No Recalbox" (voir
  // showNoWifiRecalboxAlert()). Uniquement le WiFi lui-meme : ne compte PAS
  // les echecs mqttClient.connect() quand le WiFi est OK (ce cas garde son
  // traitement existant, ecran "RecalBox connectee" une fois reellement
  // connecte -- pas d'alerte rouge si le WiFi fonctionne).
  unsigned long wifiDownStreak = 0;
  // Horodatage du dernier affichage de l'alerte "RecalBox non connectee"
  // (2026-08-05, demande utilisateur) -- WiFi OK mais mqttClient.state()==-2
  // (MQTT_CONNECT_FAILED). Base sur le temps ecoule (pas un compteur
  // d'iterations comme wifiDownStreak) car cette branche tourne au rythme
  // de MQTT_RETRY_MS (15s), different du 1s de la boucle WiFi-down.
  unsigned long lastRecalboxDisconnectedAlertMs = 0;
  // Plafond a 3 affichages par episode de coupure (2026-08-05, demande
  // utilisateur : "limiter l'affichage des images d'alerte de connexion
  // recalbox a 3 fois (initial, 60s, 120s)") -- auparavant repete
  // indefiniment toutes les 60s tant que le probleme persistait. Remis a 0
  // des que la connexion revient (memes points que les compteurs
  // ci-dessus), donc une NOUVELLE coupure ulterieure redeclenche bien 3
  // affichages a son tour -- seule la repetition SANS FIN au sein d'une
  // meme coupure prolongee est supprimee.
  int wifiAlertCount = 0;
  int recalboxDisconnectedAlertCount = 0;
  const int MAX_CONNECTION_ALERT_COUNT = 3;
  // Horodatage de la derniere transition WiFi deconnecte->connecte
  // (2026-08-09, mitigation deadlock mqttTask/LWIP -- voir changelog v58).
  // MISE A JOUR (2026-08-10, v68) : WiFi.setAutoReconnect() est desormais
  // FALSE (setupWiFiFromConfig()) -- la source de collision visee
  // initialement ici (sa tache interne au driver, opaque, independante de
  // mqttTask) n'existe plus. Ce delai reste utile pour la source de
  // reconnexion restante, maintainWiFi() (application-level, appelee
  // depuis loop()) : juste apres son WiFi.begin() qui reussit, la pile
  // socket peut encore etre en cours de stabilisation au moment ou
  // mqttTask tente mqttClient.connect(), meme risque de collision sur les
  // verrous LWIP internes. N'elimine pas la cause (verrou bas niveau, hors
  // de portee du code applicatif) mais reduit la fenetre de collision la
  // plus evidente.
  unsigned long wifiConnectedSinceMs = 0;
  bool wasWifiConnected = false;
  const unsigned long MQTT_WIFI_SETTLE_MS = 1500UL;
  // v104 -- BUG REEL confirme sur materiel EN DIRECT : la deconnexion forcee
  // (v94/v98, sur trop d'echecs subscribe()) n'avait AUCUN recul -- observe :
  // TCP se reconnecte vite (broker local, ~14ms, connexion reellement
  // saine a ce niveau), mais les subscribe() echouent A NOUVEAU sur cette
  // connexion fraiche ~19s plus tard, re-declenchant une nouvelle
  // deconnexion forcee -- boucle infinie toutes les ~39s, jamais d'etat
  // stable ou les souscriptions tiennent. Symptome utilisateur : "connected"
  // visible dans le serial en boucle, mais AUCUNE reaction reelle du DMD
  // (jamais reellement abonne a rien). Fix : compteur d'echecs consecutifs,
  // recul progressif avant de retenter apres plusieurs cycles d'affilee --
  // laisse au reseau/broker le temps de se stabiliser au lieu de marteler
  // en boucle serree.
  int consecutiveSubscribeFailCycles = 0;
  const unsigned long SUBSCRIBE_BACKOFF_STEP_MS = 5000UL;
  const unsigned long SUBSCRIBE_BACKOFF_MAX_MS = 60000UL;
  // v151 -- compteur jumeau pour le chemin connect() lui-meme en echec
  // (rc=-2/-4, AVANT meme d'atteindre subscribe -- consecutiveSubscribeFailCycles
  // ne peut structurellement rien pour ce cas, jamais atteint, meme
  // constat que celui qui avait motive v133/v134 en 2026-08-24). Discussion
  // explicite avec l'utilisateur (2026-09-03) sur le bon seuil : ni le 1er
  // echec (perdrait la capacite a distinguer un hoquet auto-resolu d'un
  // vrai episode soutenu -- la grande majorite des rc=-4 de cette nuit se
  // resolvent seuls des la tentative suivante), ni un seuil aussi haut que
  // WIFI_RESET_ESCALATION_CYCLES (chemin different, coute plus cher) --
  // seuil modere retenu, cf. SOCKET_RECREATE_ESCALATION_CYCLES plus bas.
  int consecutiveConnectFailCycles = 0;

  for(;;)
  {
    if(!wifiEnabled||recalboxIP.length()==0){vTaskDelay(pdMS_TO_TICKS(2000));continue;}
    if(WiFi.status()!=WL_CONNECTED){
      wasWifiConnected = false;
      wifiDownStreak++;
      // Cette branche boucle a ~1/s (vTaskDelay 1000ms ci-dessous) : la
      // 1ere fois (~1s apres la coupure) puis toutes les ~60 iterations
      // (~60s) tant que ca persiste. Pas de dessin direct depuis cette
      // tache de fond (voir showNoWifiRecalboxAlert(), appelee depuis
      // loop() uniquement) -- juste une demande best-effort.
      if (!g_sdOpInProgress && wifiAlertCount < MAX_CONNECTION_ALERT_COUNT
          && (wifiDownStreak==1 || wifiDownStreak % 60 == 0)) {
        g_noWifiRecalboxPending = true;
        wifiAlertCount++;
      }
      vTaskDelay(pdMS_TO_TICKS(1000));continue;
    }
    wifiDownStreak = 0;
    wifiAlertCount = 0;
    if (!wasWifiConnected) {
      wasWifiConnected = true;
      wifiConnectedSinceMs = millis();
    }

    // En mode config web : ne pas tenter de connexion MQTT (garde les sockets libres pour HTTP)
    // Idem pendant une generation de playlist (2026-07-29, test en cours) :
    // playlistGenTask() tourne sur le meme coeur que cette tache -- une
    // tentative de (re)connexion MQTT ici (allocations pour le TCP/DNS)
    // pourrait etre le facteur qui fait basculer le heap sous ce dont
    // openNextFile() a besoin au mauvais moment, cause suspectee du crash
    // reel observe sur ce meme materiel. Ne saute que la TENTATIVE de
    // connexion -- .loop() reste actif si deja connecte, donc une commande
    // (ex. reboot) recue avant le debut du scan continue d'etre traitee.
    // (2026-08-10, v65 suite) : v65 a deja retire ce cout de
    // gifPlayFrameCompat()/openNextGif() (chemin le plus chaud) sans
    // resoudre le probleme -- ce check-ci tournait pourtant SANS AUCUNE
    // CONDITION, a CHAQUE iteration de mqttTask (~50 fois/seconde en
    // regime normal), meme quand playlistGenTask() n'a jamais tourne.
    // Contrairement au cas precedent, c'est mqttTask() qui prend son
    // propre semaphore juste avant ses operations socket/LWIP -- candidat
    // plus direct. Meme fix : lecture non protegee de g_plGenStatus.active
    // en pre-check rapide, plGenStatusMutex seulement pris si necessaire.
    bool plGenActiveNow = g_plGenStatus.active;
    if(g_sdOpInProgress || plGenActiveNow) { if(mqttClient.connected()) mqttClient.loop(); vTaskDelay(pdMS_TO_TICKS(1000)); continue; }

    if(!mqttClient.connected())
    {
      // Delai de stabilisation post-reconnexion WiFi -- voir commentaire
      // pres de MQTT_WIFI_SETTLE_MS plus haut.
      if (millis() - wifiConnectedSinceMs < MQTT_WIFI_SETTLE_MS) {
        vTaskDelay(pdMS_TO_TICKS(200));
        continue;
      }
      // v92 -- instrumentation heap sur chaque tentative connect()/echec, pour
      // verifier l'hypothese utilisateur (observee empiriquement plusieurs
      // fois : la reconnexion n'aboutit qu'apres un retour hors-jeu/playlist)
      // d'une contention sur le heap PARTAGE entre loop() (overlay/raw565pack
      // en jeu, heap maintenu bas) et mqttTask() (connect() a peut-etre besoin
      // d'assez de heap libre pour ses buffers TCP/MQTT). A RETIRER une fois
      // l'hypothese confirmee ou infirmee par des mesures reelles.
      // v135 -- RSSI/canal ajoutes (voir commentaire complet pres de
      // WiFi.onEvent() dans connectWiFi()) -- correle l'etat radio EXACT au
      // moment de chaque tentative, jamais capture jusqu'ici.
      // v143 -- attempt= (g_totalConnectAttempts, voir sa declaration) +
      // minFreeHeap= (ESP.getMinFreeHeap(), plancher historique jamais
      // remis a zero -- complete free/maxalloc, deja suivis mais qui ne
      // disent rien du pire cas atteint au fil de la session).
      g_totalConnectAttempts++;
      Serial.println("[MQTT] connecting to "+recalboxIP+" (free="+String(ESP.getFreeHeap())
                     +" maxalloc="+String(ESP.getMaxAllocHeap())
                     +" minFreeHeap="+String(ESP.getMinFreeHeap())
                     +" rssi="+String(WiFi.RSSI())+" ch="+String(WiFi.channel())
                     +" attempt="+String(g_totalConnectAttempts)+")");
      if(mqttClient.connect(MQTT_CLIENT))
      {
        // v135 -- rssi ajoute ici aussi : reference "etat radio au moment
        // d'un succes" a comparer avec les echecs (voir les 2 autres sites).
        Serial.println("[MQTT] connected (rssi="+String(WiFi.RSSI())+")");
        lastMqttConnectedMs=millis();
        lastRecalboxDisconnectedAlertMs=0; // reautorise l'alerte immediate en cas de future deconnexion
        recalboxDisconnectedAlertCount=0;
        // v146 -- RECHERCHE DE CAUSE RACINE (pas juste une consequence de
        // plus a retarder, retour utilisateur explicite : "on regle des
        // consequences, pas la cause"). TCP_NODELAY n'etait active NULLE
        // PART dans tout ce fichier -- l'algorithme de Nagle est donc actif
        // par defaut sur ce socket. Nos paquets SUBSCRIBE (23-30 octets)
        // sont bien plus petits que le MSS -- avec Nagle actif, un nouveau
        // petit segment reste RETENU dans le buffer d'envoi local tant que
        // le segment precedent n'est pas accuse, au lieu d'etre transmis
        // immediatement. Le broker accuse quasiment toujours instantanement
        // (confirme au paquet), donc Nagle seul n'explique pas un blocage
        // multi-secondes DANS LA MAJORITE des cas -- MAIS on a aussi
        // confirme au moins 2 cycles ou l'ACK du CONNACK lui-meme arrivait
        // assez tard pour declencher une retransmission broker (v139/ce
        // soir) : si un accuse est retenu/retarde pour une autre raison
        // (etat lwIP/driver WiFi transitoire, cause encore inconnue), Nagle
        // empile alors les tentatives suivantes (mqttSubscribeFast() retente
        // 2 fois par topic) dans ce MEME buffer jamais vide -- jusqu'a ce
        // qu'il soit reellement plein, ce qui correspond exactement au
        // errno=EAGAIN observe (buffer plein, pas juste "en attente").
        // Desactiver Nagle ne resout probablement pas la cause profonde
        // (pourquoi un accuse peut etre retarde en premier lieu) mais
        // retire un facteur d'AMPLIFICATION reel et bien identifie -- sans
        // aucun risque (TCP_NODELAY est le reglage standard recommande pour
        // les protocoles a petits paquets frequents comme MQTT, la plupart
        // des clients MQTT serieux l'activent par defaut). Applique une
        // seule fois par connexion (fd change a chaque reconnexion).
        {
          int fd = wifiClientMqtt.fd();
          if (fd >= 0)
          {
            int one = 1;
            int rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            Serial.println("[MQTT] TCP_NODELAY set rc=" + String(rc) + " errno=" + String(errno) + " fd=" + String(fd));
            // v146 suite -- meme soir, chiffre trouve directement dans le
            // sdkconfig precompile du core (esp32-libs/3.3.11/sdkconfig,
            // pas une supposition) : CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744
            // (TCP_MSS=1436) -- le nombre de segments simultanement non-
            // accuses autorises decoule de ce ratio (formule lwIP standard
            // ~4*SND_BUF/MSS, ~16 ici) et non des octets bruts (nos ~12
            // topics a 23-30 octets ne remplissent jamais les 5744 octets
            // en volume, mais peuvent saturer ce compteur de SEGMENTS si
            // les accuses trainent). TCP_SNDBUF est une extension NON
            // standard d'ESP-IDF qui permet d'augmenter ce plafond PAR
            // SOCKET (pas de reallocation immediate de toute la taille --
            // lwIP alloue les pbuf au fil de l'eau, ce reglage releve
            // juste le plafond) -- valeur modeste choisie (16 Ko, pas les
            // 28+ Ko parfois cites en ligne) vu notre budget heap deja
            // serre cette session (maxalloc ~4.6 Ko en continu). Objectif
            // : voir si la meme boucle de blocage recidive malgre une
            // marge nettement plus large avant saturation.
            // v147 suite -- 1er essai (IPPROTO_TCP/TCP_SND_BUF) compile
            // mais ECHOUE A L'EXECUTION (rc=-1 errno=109 ENOPROTOOPT --
            // macro definie mais non geree par cette implementation
            // setsockopt() de lwIP/ESP-IDF, confirme en serial reel).
            // Bascule sur l'option POSIX standard (SOL_SOCKET/SO_SNDBUF),
            // plus generalement supportee par lwIP que l'extension
            // specifique tentee en 1er.
            int sndbufSize = 16384;
            int rcSndbuf = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbufSize, sizeof(sndbufSize));
            Serial.println("[MQTT] SO_SNDBUF set rc=" + String(rcSndbuf) + " errno=" + String(errno) + " target=" + String(sndbufSize));
          }
        }
        // v145 -- delai de stabilisation COURT avant le tout 1er subscribe()
        // (capture tcpdump reelle, session du 02/09 soir) : preuve paquet
        // directe que le SUBSCRIBE qui suit immediatement le CONNACK ne
        // quitte parfois jamais le DMD (0 octet sur le cable pendant
        // plusieurs secondes, alors que le CONNECT/CONNACK venait de
        // s'echanger sans probleme sur la MEME connexion quelques centaines
        // de ms plus tot) -- correle avec un CONNACK dont l'ACK client
        // arrive assez tard pour declencher une retransmission broker (2/2
        // cycles observes avec retransmission ont aussi echoue au 1er
        // subscribe). Hypothese : un etat transitoire cote lwIP/driver WiFi
        // juste apres le CONNACK (traitement RX qui prive temporairement le
        // chemin TX de ressource) plutot qu'un vrai probleme reseau externe
        // (broker/Freebox tous deux ecartes avec preuve directe le meme
        // soir). PAS un WiFi.disconnect()/begin() (deja tente et reverte,
        // voir v134) -- juste laisser passer une courte fenetre avant de
        // solliciter le TX. Ne casse rien au niveau protocole (contrairement
        // a fermer/rouvrir le socket, qui casserait la session MQTT en
        // cours). Valeur choisie a l'estime (~211ms observes pour le RTO de
        // retransmission broker mesure la meme session) -- PAS ENCORE
        // VALIDE sur l'episode reel au moment de cet ecrit, a confirmer via
        // tcpdump si le symptome persiste malgre ce delai.
        vTaskDelay(pdMS_TO_TICKS(150));
        // v89 (2026-08-17) -- BUG REEL confirme sur materiel : le DMD
        // restait "[MQTT] connected" mais ne recevait plus AUCUN message
        // publie par la RB (confirme cote RB : marquee.sh publiait bien),
        // observe apres une reconnexion elle-meme precedee de plusieurs
        // echecs (rc=-2/rc=-4) -- le reseau etait donc probablement encore
        // instable au moment exact de cette rafale de 15 subscribe()
        // consecutifs, SANS AUCUNE verification du resultat ni delai entre
        // eux. subscribe() peut echouer silencieusement (retour bool
        // jamais teste jusqu'ici) si l'ecriture socket echoue -- aucun
        // moyen de le savoir depuis les logs. Fix : verifie chaque
        // subscribe(), reessaie une fois apres un court delai en cas
        // d'echec, logue tout echec definitif (visibilite minimale, pas
        // de garantie absolue si le reseau reste degrade en continu, mais
        // couvre le cas d'une instabilite passagere juste apres connect()).
        int subscribeFailCount = 0;
        // v128 -- DIAGNOSTIC temporaire (retour utilisateur : cycle de
        // deconnexion tres regulier ~38.8s observe plusieurs fois, avant ET
        // apres le fix delay(1) -- donc pas lie au rendu comme suppose. Le
        // commentaire v98 ci-dessous chiffre deja "chacun ~20s (2x le socket
        // timeout 10s)" pour un topic en echec complet (1er essai + retry) --
        // 38.8s ~ 2 topics au plafond -- mais jamais mesure directement.
        // Timestamp avant/apres CHAQUE tentative pour voir EXACTEMENT ou
        // passe le temps, plutot que de continuer a deviner par arithmetique.
        // v129 suite -- DIAGNOSTIC : mesure sur materiel montre les 2
        // premiers subscribe() apres connect() bloquant chacun ~9-10s SUR
        // CHAQUE tentative (essai1 ET essai2), le 3e echouant instantanement
        // (4ms/0ms) -- signature d'une ecriture qui attend un accuse jamais
        // recu sur un socket deja mort, PAS une lenteur broker qui repond
        // tard. Ajoute ici : etat mqttClient.connected() juste AVANT chaque
        // tentative, pour voir si la connexion est deja tombee cote client
        // avant meme d'essayer d'ecrire (confirmerait/infirmerait un socket
        // deja rompu au moment du 1er subscribe(), pas seulement au 3e).
        auto subscribeChecked = [&subscribeFailCount](const char *topic) {
          bool preConnected = mqttClient.connected();
          // v136 -- sonde independante du cache d'etat PubSubClient (voir
          // entete changelog) : socket brut + etat radio au tout premier
          // point de controle, avant la moindre tentative d'ecriture.
          bool preRawSocket = wifiClientMqtt.connected();
          wl_status_t preWifiStatus = WiFi.status();
          int32_t preRssi = WiFi.RSSI();
          // v137 -- heartbeat CPU0 (voir commentaire pres de sa declaration) :
          // echantillonne avant/apres chaque tentative, log en tick attendus
          // vs reels pour reperer une famine de tache sans faire le calcul a
          // la main a chaque fois (20ms/tick).
          uint32_t hb0 = g_heartbeatCounter;
          unsigned long t0 = millis();
          // v139 -- mqttSubscribeFast() remplace mqttClient.subscribe()
          // (voir son commentaire complet + changelog v139) : meme paquet
          // MQTT envoye, mais budget d'attente 900ms max (3x300ms) au lieu
          // de jusqu'a 10s via la boucle select() interne du coeur ESP32.
          bool ok1 = mqttSubscribeFast(wifiClientMqtt, topic, 0, 300, 3);
          unsigned long t1 = millis();
          uint32_t hb1 = g_heartbeatCounter;
          if (ok1)
          {
            Serial.println("[SUBDIAG] " + String(topic) + " OK 1er essai (" + String(t1 - t0) + "ms) preConnected=" + String(preConnected)
                           + " hb=" + String(hb1 - hb0) + "/" + String((t1 - t0) / 20));
            // v145 suite -- meme motif que le delai avant le 1er subscribe
            // (voir son commentaire complet plus haut) : capture tcpdump a
            // montre que le blocage TX peut aussi survenir APRES plusieurs
            // subscribe() reussis d'affilee (6e topic bloque alors que les
            // 5 precedents venaient de partir sans probleme) -- pas
            // strictement limite au tout premier envoi post-CONNACK. Meme
            // delai court entre chaque subscribe reussi, pour la meme
            // raison (laisser souffler le TX plutot que l'enchainer sans
            // pause).
            vTaskDelay(pdMS_TO_TICKS(30));
            return;
          }
          // v136 -- meme sonde juste apres l'echec du 1er essai (bloquant
          // ~9-10s d'apres v129/v130) : la connexion a-t-elle deja bascule
          // pendant CETTE tentative precise, avant meme le delay(50)/retry ?
          bool postRawSocket1 = wifiClientMqtt.connected();
          wl_status_t postWifiStatus1 = WiFi.status();
          delay(50);
          bool preConnected2 = mqttClient.connected();
          uint32_t hb2 = g_heartbeatCounter;
          unsigned long t2 = millis();
          bool ok2 = mqttSubscribeFast(wifiClientMqtt, topic, 0, 300, 3); // v139, voir 1er essai
          unsigned long t3 = millis();
          uint32_t hb3 = g_heartbeatCounter;
          if (ok2)
          {
            Serial.println("[SUBDIAG] " + String(topic) + " OK retry (essai1=" + String(t1 - t0) + "ms, essai2=" + String(t3 - t2) + "ms) preConnected=" + String(preConnected) + "/" + String(preConnected2)
                           + " hb=" + String(hb1 - hb0) + "/" + String((t1 - t0) / 20) + "," + String(hb3 - hb2) + "/" + String((t3 - t2) / 20));
            vTaskDelay(pdMS_TO_TICKS(30)); // v145 suite -- voir commentaire complet au 1er site (OK 1er essai)
            return;
          }
          // v137 -- hb=<reel1>/<attendu1>,<reel2>/<attendu2> : si <reel> est
          // proche de <attendu> pendant les 2 blocages -> CPU0 tournait
          // normalement, vrai blocage bas niveau (a). Si <reel> s'effondre
          // (proche de 0) alors que <attendu> est ~450-500 (9-10s/20ms) ->
          // CPU0 affame par autre chose au meme moment (b), piste a rediriger
          // vers le rendu/SD plutot que MQTT/lwIP.
          Serial.println("[MQTT] subscribe ECHEC (apres 1 retry) -> " + String(topic)
                         + " [SUBDIAG essai1=" + String(t1 - t0) + "ms essai2=" + String(t3 - t2)
                         + "ms preConnected=" + String(preConnected) + "/" + String(preConnected2)
                         + " postConnected=" + String(mqttClient.connected())
                         + " hb=" + String(hb1 - hb0) + "/" + String((t1 - t0) / 20) + "," + String(hb3 - hb2) + "/" + String((t3 - t2) / 20)
                         // v136 -- socket brut (avant essai1 / apres essai1 / final), etat
                         // radio (avant essai1 / final), RSSI (avant essai1 / final), heap
                         // libre au moment de l'echec definitif.
                         + " rawSocket=" + String(preRawSocket) + "/" + String(postRawSocket1) + "/" + String(wifiClientMqtt.connected())
                         + " wifiStatus=" + String((int)preWifiStatus) + "/" + String((int)postWifiStatus1) + "/" + String((int)WiFi.status())
                         + " rssi=" + String(preRssi) + "/" + String(WiFi.RSSI())
                         + " heap=" + String(ESP.getFreeHeap()) + "]");
          subscribeFailCount++;
        };
        // v98 -- BUG REEL confirme sur materiel : le check du seuil (voir
        // SUBSCRIBE_FAIL_THRESHOLD plus bas) ne s'executait qu'APRES cette
        // sequence de 15 appels sequentiels -- observe : 8 echecs sur 15,
        // chacun ~20s (2x le socket timeout 10s), soit ~2min40 avant meme que
        // la deconnexion forcee (v94) puisse se declencher -- perd une bonne
        // partie du gain de reactivite vise par ce fix. Passe en boucle avec
        // sortie anticipee des que le seuil est atteint, au lieu d'attendre
        // les 15 tentatives.
        const int SUBSCRIBE_FAIL_THRESHOLD = 3;
        // v148 -- 12 topics fusionnes en 1 seul (marquee/cmd), voir le
        // commentaire complet pres du dispatch dans onMqttMessage().
        // v150 -- BUG REEL trouve par revue de code avant passage sur master
        // (jamais reproduit en direct, trouve par lecture) : game/system/
        // default/ingame partageaient TOUS le meme topic retenu marquee/cmd
        // depuis v148 -- le retain MQTT est PAR TOPIC (le broker ne garde
        // qu'UN SEUL message retenu par topic), donc 2 send_mqtt_retain()
        // consecutifs (ex. rungame : "game" puis "ingame", marquee.sh lignes
        // ~1602-1603) ecrasaient silencieusement le retain l'un de l'autre
        // AU NIVEAU DU BROKER -- pas juste cote firmware. Un DMD qui se
        // reconnecte apres une telle sequence ne recevait plus que le DERNIER
        // etat retenu, jamais les autres, defaisant silencieusement le fix
        // de resynchronisation v123/marquee.sh v17 (2026-08-23). Fix : ces 4
        // etats "collants" (game/system/default/ingame) recuperent chacun
        // leur propre topic retenu dedie -- tout le reste (score/achievement/
        // stop/show_config/wifi_recovery/reboot/brightness*) reste sur le
        // topic unique marquee/cmd (jamais retenu pour ces commandes-la, donc
        // aucune collision possible entre elles). Meme format de payload
        // (CMD=<nom> ARG=<valeur>), seul le topic change -- voir dispatch
        // dans onMqttMessage() pour la meme extraction appliquee aux 5
        // topics. 1(cmd)+4(etats)+1(evenement ES brut) = 6 topics au total,
        // toujours 54% de moins que les 12+1 d'origine.
        const char *subscribeTopics[] = {
          "marquee/cmd",
          "marquee/cmd/game",
          "marquee/cmd/system",
          "marquee/cmd/default",
          "marquee/cmd/ingame"
        };
        const int nSubscribeTopics = sizeof(subscribeTopics) / sizeof(subscribeTopics[0]);
        for (int si = 0; si < nSubscribeTopics && subscribeFailCount < SUBSCRIBE_FAIL_THRESHOLD; si++)
          subscribeChecked(subscribeTopics[si]);
        if (subscribeFailCount < SUBSCRIBE_FAIL_THRESHOLD)
          subscribeChecked(mqttEventTopic.c_str());

        // v94 -- BUG REEL confirme sur materiel : le fix v89 (subscribeChecked,
        // log+retry par topic) suffisait pour un echec ISOLE/transitoire, mais
        // ne gerait pas le cas observe en direct ici -- les 15 subscribe()
        // ont TOUS echoue (meme apres retry), ~20s d'ecart chacun (2x le
        // socket timeout 10s), TCP accepte + CONNACK recu mais plus AUCUNE
        // reponse ensuite -- connexion "zombie" (mqttClient.connected() reste
        // true, mais sourde). Avant ce fix, le code continuait quand meme :
        // publish("marquee/status/ip") sur un socket mort, et surtout
        // CMD_WAITING_MQTT poste -> ecran "RecalBox connectee" affiche et
        // fige plusieurs minutes (observe : "affichage fallback + rb
        // connectee en fixe" pendant ~5min30 -- PAS indefiniment : le
        // keepalive interne de PubSubClient finit par detecter la connexion
        // morte tout seul et redeclenche une vraie reconnexion, observe sur
        // materiel SANS ce fix). Fix : plutot que d'attendre ce keepalive
        // (~5-6 minutes de sortie de service ici), si un nombre significatif
        // de subscribe() ont echoue (seuil arbitraire mais large marge --
        // 1-2 echecs isoles
        // restent tolerables sans reagir, cf. v89), la connexion est
        // consideree morte : deconnexion FORCEE (mqttClient.disconnect()),
        // pas de publish ip ni de CMD_WAITING_MQTT sur ce socket -- le
        // prochain tour de boucle de mqttTask() (if(!mqttClient.connected()))
        // retentera une VRAIE reconnexion (nouveau socket TCP). Seuil declare
        // plus haut (v98, avec la boucle a sortie anticipee).
        if (subscribeFailCount >= SUBSCRIBE_FAIL_THRESHOLD)
        {
          consecutiveSubscribeFailCycles++;
          // v104 -- recul progressif : sans ca, un connect() rapide (broker
          // local) enchaine immediatement sur une nouvelle rafale de
          // subscribe() qui peut echouer pour la MEME raison sous-jacente
          // (probablement le meme reseau instable que le rc=-4/-2 deja
          // documente), redeclenchant cette meme deconnexion forcee en
          // boucle serree (~39s observes) sans jamais laisser le temps au
          // reseau/broker de se stabiliser.
          unsigned long backoffMs = min((unsigned long)consecutiveSubscribeFailCycles * SUBSCRIBE_BACKOFF_STEP_MS, SUBSCRIBE_BACKOFF_MAX_MS);
          Serial.println("[MQTT] " + String(subscribeFailCount)
                         + " subscribe() en echec (sortie anticipee, v98) -- connexion consideree morte, deconnexion forcee"
                         + " (cycle consecutif #" + String(consecutiveSubscribeFailCycles)
                         + ", recul " + String(backoffMs) + "ms avant nouvelle tentative)");
          mqttClient.disconnect();
          // v134 -- v132 (force WiFi.disconnect()/begin() depuis mqttTask()
          // apres plusieurs cycles subscribe-fail) REVERTE (retour
          // utilisateur : "on a plein de wifi reconnect mais ca ne
          // fonctionne pas" -- observe en direct : APRES le declenchement de
          // ce fix, le WiFi lui-meme s'est mis a decrocher reellement en
          // boucle serree, "[WIFI] reconnect" toutes les ~5s, alertes "No
          // wifi" qui reviennent -- situation NETTEMENT PIRE qu'avant).
          // Cause tres probable : WiFi.disconnect()/begin() appeles ICI
          // (tache mqttTask(), coeur 0) en concurrence avec maintainWiFi()
          // (tache loop(), coeur 1, son propre cycle WiFi.disconnect()/
          // begin() independant) -- ces appels ne sont pas documentes
          // thread-safe pour un usage concurrent depuis 2 taches sans
          // synchronisation, une collision peut corrompre l'etat interne du
          // driver bien plus qu'un reset MQTT seul. Retire entierement --
          // seul mqttClient.disconnect() (deja existant, niveau MQTT/socket
          // uniquement, pas de risque de concurrence WiFi) subsiste ici.
          //
          // v144 -- ESCALADE REINTRODUITE, cette fois SYNCHRONISEE
          // (wifiResetMutex, voir sa declaration) au lieu d'etre retiree.
          // Motif : au plafond de recul (60s), rejouer indefiniment le
          // meme mqttClient.disconnect() ne resout rien si la cause est
          // plus profonde que le seul niveau MQTT (pile socket/WiFi) --
          // observe en direct (retour utilisateur, session du 02/09) :
          // cycle connect(14s)/disconnect/recul(60s) reproduit a
          // l'identique pendant 10+ min sans jamais se resoudre tout
          // seul. Seuil choisi expres AU-DELA du plafond (12 cycles pour
          // l'atteindre + 3 de plus dessus) : laisse largement le temps
          // au recul MQTT seul de suffire dans le cas courant
          // (probleme transitoire), ne declenche l'action plus lourde
          // que si ca persiste vraiment. Prend le mutex avec un timeout
          // court (500ms) -- si maintainWiFi() le tient deja, on
          // abandonne cette tentative SANS bloquer mqttTask(), le seuil
          // restant depasse, on retentera au prochain cycle (~60s plus
          // tard) plutot que d'attendre indefiniment un verrou tenu par
          // l'autre tache.
          // v155 -- REACTIVEE apres correctif du vrai coupable identifie
          // (2026-09-04) : premier declenchement reel en conditions
          // reelles ce soir-la (13:33:16) avait ete suivi d'une boucle
          // serree WiFi.disconnect()/reconnect (raison=8, auto-initiee
          // cote DMD) pendant 3min39 avant retablissement seul --
          // reproduisant le symptome ayant fait retirer v132/v133 en
          // v134, malgre le mutex (v144) cense fermer cette fenetre.
          // Analyse ("creuse un peu autour du mutex") : le mutex
          // empechait bien la collision entre les 2 call sites, mais la
          // boucle observee n'en etait pas une -- raison=8 confirme que
          // c'est maintainWiFi() SEULE qui se redeclenchait toutes les
          // 5s (son cooldown fixe d'alors), trop vite pour laisser une
          // tentative WiFi.begin() precedente vraiment aboutir en
          // conditions degradees, interrompant chaque tentative avant
          // qu'elle ait pu reussir. Corrige : cooldown de maintainWiFi()
          // rendu progressif (voir consecutiveWifiReconnectCycles, sa
          // declaration, et le corps de maintainWiFi()). Cette escalade
          // synchronise maintenant aussi lastWifiReconnectAttempt juste
          // apres son propre WiFi.begin() : evite que maintainWiFi()
          // retente immediatement par-dessus des le prochain loop().
          const bool WIFI_RESET_ESCALATION_ENABLED = true;
          const int WIFI_RESET_ESCALATION_CYCLES = 15;
          if (WIFI_RESET_ESCALATION_ENABLED && consecutiveSubscribeFailCycles >= WIFI_RESET_ESCALATION_CYCLES)
          {
            if (wifiResetMutex!=nullptr && xSemaphoreTake(wifiResetMutex,pdMS_TO_TICKS(500))==pdTRUE)
            {
              Serial.println("[MQTT] escalade : " + String(consecutiveSubscribeFailCycles)
                             + " cycles consecutifs au plafond de recul -- reset WiFi complet (WiFi.disconnect()/begin())");
              delay(1500); WiFi.disconnect(); delay(50);
              applyStaticIP();
              WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());
              WiFi.setSleep(false);
              lastWifiReconnectAttempt = millis(); // v155 -- laisse cette tentative respirer avant que maintainWiFi() n'en retente une autre par-dessus
              xSemaphoreGive(wifiResetMutex);
              consecutiveSubscribeFailCycles = 0; // repart de zero apres l'action forte
            }
            else
            {
              Serial.println("[MQTT] escalade WiFi voulue (" + String(consecutiveSubscribeFailCycles)
                             + " cycles) mais mutex indisponible (maintainWiFi() l'utilise) -- retente au prochain cycle");
            }
          }
          vTaskDelay(pdMS_TO_TICKS(backoffMs));
        }
        else
        {
        consecutiveSubscribeFailCycles = 0; // v104 -- connexion saine (souscriptions OK), recul reinitialise
        g_lastMqttUsefulMs=millis(); // v142 -- souscriptions reellement abouties, voir declaration de g_lastMqttUsefulMs
        // Retenu (retain=true) : un abonne (script Recalbox) qui se connecte
        // plus tard recoit immediatement la derniere IP publiee, sans avoir
        // besoin d'etre a l'ecoute au moment exact de cette connexion.
        mqttClient.publish("marquee/status/ip", WiFi.localIP().toString().c_str(), true);
        broadcastFeatureStatus(); // v110 -- voir sa declaration
        if(mqttCmdMutex!=nullptr&&xSemaphoreTake(mqttCmdMutex,pdMS_TO_TICKS(100))==pdTRUE)
        {
          // Connexion MQTT tout juste effective : affiche l'image de
          // secours statique (pas la playlist en rotation) en attendant le
          // premier vrai message system/game de la Recalbox -- demande
          // utilisateur. Volontairement SANS le garde currentMode!=MODE_PLAYLIST
          // (contrairement a CMD_DEFAULT) : MQTT_START_DELAY_MS (12s) laisse
          // largement le temps a la playlist de demarrer AVANT que MQTT ne
          // se connecte -- currentMode==MODE_PLAYLIST est donc le cas normal
          // ici, pas une exception. Bug confirme sur test reel (log serie) :
          // avec ce garde, CMD_WAITING_MQTT (et g_mqttWaitingUntilMs) n'etait
          // JAMAIS pose, donc la fenetre de grace ne s'appliquait jamais et
          // "lastplayed" (retenu) passait directement sans filtrage.
          if(!g_sdOpInProgress)
          {
            pendingCmd=MqttCommand(MqttCommand::CMD_WAITING_MQTT,"");
          }
          xSemaphoreGive(mqttCmdMutex);
        }
        }
      }
      else
      {
        // v135 -- RSSI/canal + statut WiFi ajoutes ici aussi (voir
        // commentaire complet pres de WiFi.onEvent() dans connectWiFi()).
        Serial.println("[MQTT] failed rc="+String(mqttClient.state())+" (free="+String(ESP.getFreeHeap())
                       +" maxalloc="+String(ESP.getMaxAllocHeap())
                       +" minFreeHeap="+String(ESP.getMinFreeHeap())
                       +" rssi="+String(WiFi.RSSI())+" ch="+String(WiFi.channel())
                       +" wifiStatus="+String(WiFi.status())
                       +" attempt="+String(g_totalConnectAttempts)+")");
        unsigned long now=millis();
        // v134 -- v133 (force WiFi.disconnect()/begin() ici apres 90s de
        // connect() en echec soutenu) REVERTE, MEME RAISON que le retrait
        // du bloc jumeau v132 plus haut (voir son commentaire complet) :
        // suspecte de collision avec maintainWiFi() (tache loop(), coeur 1)
        // sur WiFi.disconnect()/begin() non synchronise entre 2 taches --
        // observe en direct : situation reseau nettement PIRE juste apres
        // le declenchement de ce fix (WiFi decrochant reellement en boucle
        // serree, alors qu'avant ce n'etait qu'un probleme MQTT/emission
        // avec WiFi.status() toujours a WL_CONNECTED). connectFailStreakSinceMs
        // reste declare (inoffensif, plus utilise) -- a retirer completement
        // dans un futur nettoyage si cette piste n'est pas reprise.
        // Alerte "RecalBox non connectee" (2026-08-05, demande utilisateur)
        // -- uniquement rc==-2 (MQTT_CONNECT_FAILED, echec de connexion TCP
        // au broker) : WiFi deja confirme OK a ce point (garde plus haut
        // dans la boucle), donc ce n'est PAS un probleme WiFi (pas
        // d'alerte "No wifi, No Recalbox" ici, voir wifiDownStreak). 1ere
        // fois, puis toutes les 60s tant que ca persiste.
        if (mqttClient.state() == -2 && !g_sdOpInProgress
            && recalboxDisconnectedAlertCount < MAX_CONNECTION_ALERT_COUNT
            && (lastRecalboxDisconnectedAlertMs == 0 || (now - lastRecalboxDisconnectedAlertMs) >= 60000UL)) {
          g_recalboxDisconnectedPending = true;
          lastRecalboxDisconnectedAlertMs = now;
          recalboxDisconnectedAlertCount++;
        }
        // v142 -- verifie desormais g_lastMqttUsefulMs (derniere activite
        // MQTT reellement UTILE : subscribe() abouti ou message recu), plus
        // lastMqttConnectedMs (juste "connect() a reussi", remis a zero a
        // chaque connect() meme si les subscribe() qui suivent echouent en
        // boucle -- voir commentaire complet pres de sa declaration/le bug
        // documente dans DECISIONS.md). lastMqttConnectedMs laisse en place
        // inchange (informatif uniquement desormais, plus consulte ici).
        if((now-g_lastMqttUsefulMs)>=MQTT_OFFLINE_FALLBACK_MS)
        {
          if(currentMode!=MODE_PLAYLIST&&gifCount>0&&!g_sdOpInProgress)
          {
            Serial.println("[MQTT] injoignable -> reprise playlist");
            // v96 -- slot dedie (voir commentaire pres de g_pendingGame).
            if(mqttCmdMutex!=nullptr&&xSemaphoreTake(mqttCmdMutex,pdMS_TO_TICKS(100))==pdTRUE)
            {g_pendingDefault=true;xSemaphoreGive(mqttCmdMutex);}
            g_lastMqttUsefulMs=now; // evite de re-declencher a chaque tour tant que ca reste injoignable
          }
        }
        // v151 -- escalade "recreation de socket" pour connect() en echec
        // soutenu (rc=-2/-4). Contrairement a l'escalade WiFi (v144,
        // consecutiveSubscribeFailCycles), celle-ci reste ENTIEREMENT au
        // niveau TCP/socket : ferme le fd sous-jacent et reconstruit
        // wifiClientMqtt de zero, sans toucher au WiFi (pas de
        // WiFi.disconnect()/begin(), donc aucun risque de collision avec
        // maintainWiFi() -- pas besoin de wifiResetMutex ici,
        // wifiClientMqtt n'est touche que par cette tache). Objectif :
        // vider un eventuel etat lwIP/buffer coince sur CET objet reutilise
        // depuis le tout premier boot (jamais recree avant ce soir), sans
        // payer le cout d'une reassociation WiFi complete. Seuil choisi
        // avec l'utilisateur : ni trop tot (perdrait la capacite a
        // distinguer un hoquet auto-resolu -- la majorite des rc=-4
        // observes cette nuit se resolvent seuls des la tentative
        // suivante), ni aussi haut que l'escalade WiFi (chemin de code
        // neuf, jamais teste sur ce materiel -- prudence).
        consecutiveConnectFailCycles++;
        const int SOCKET_RECREATE_ESCALATION_CYCLES = 5;
        if (consecutiveConnectFailCycles >= SOCKET_RECREATE_ESCALATION_CYCLES)
        {
          Serial.println("[MQTT] escalade socket : " + String(consecutiveConnectFailCycles)
                          + " echecs connect() consecutifs -- recreation de wifiClientMqtt (TCP seul, WiFi non touche)");
          wifiClientMqtt.stop();
          wifiClientMqtt = WiFiClient();
          mqttClient.setClient(wifiClientMqtt);
          consecutiveConnectFailCycles = 0; // repart de zero apres l'action
        }
        vTaskDelay(pdMS_TO_TICKS(MQTT_RETRY_MS)); continue;
      }
    }
    else { lastMqttConnectedMs=millis(); consecutiveConnectFailCycles = 0; }

    mqttClient.loop();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// --------------------------------------------------
// WiFi
// --------------------------------------------------
bool parseIP(const String &s, IPAddress &ip)
{
  int a,b,c,d;
  if(sscanf(s.c_str(),"%d.%d.%d.%d",&a,&b,&c,&d)!=4) return false;
  if(a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return false;
  ip=IPAddress(a,b,c,d); return true;
}

bool applyStaticIP()
{
  if(!wifiStaticEnabled) return true;
  IPAddress localIP,gateway,subnet,dns1,dns2;
  if(!parseIP(wifiStaticIP,localIP)||!parseIP(wifiGateway,gateway)||!parseIP(wifiSubnet,subnet)) return false;
  bool h1=parseIP(wifiDNS1,dns1),h2=parseIP(wifiDNS2,dns2);
  if(h1&&h2) return WiFi.config(localIP,gateway,subnet,dns1,dns2);
  if(h1)     return WiFi.config(localIP,gateway,subnet,dns1);
  return WiFi.config(localIP,gateway,subnet);
}

// Resout automatiquement l'IP de la Recalbox via mDNS (nom d'hote "recalbox",
// annonce par Avahi cote Recalbox) -- evite d'avoir a la ressaisir a la main
// dans la page de config (necessaire notamment apres un mode secours WiFi,
// pour que MQTT redevienne disponible sans intervention). N'ecrase jamais un
// recalboxIP deja renseigne (choix manuel de l'utilisateur, ex: IP fixe ou
// nom d'hote personnalise) -- ne fait rien si le champ n'est pas vide.
void autoDetectRecalboxIP()
{
  if (recalboxIP.length() > 0) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (!MDNS.begin("dmd-marquee")) { Serial.println("[MDNS] begin echoue"); return; }
  IPAddress ip = MDNS.queryHost("recalbox", 3000);
  MDNS.end();
  if (ip == IPAddress(0,0,0,0)) { Serial.println("[MDNS] recalbox.local introuvable"); return; }
  recalboxIP = ip.toString();
  writeConfigFlag("recalbox_ip", recalboxIP);
  Serial.println("[MDNS] Recalbox detectee: " + recalboxIP);
}

// Mode secours declenche via marquee/cmd/wifi_recovery (config.ini: force_ap_recovery).
// WIFI_AP pur (pas WIFI_AP_STA -- rejete precedemment, cf memoire projet) avec un
// compte a rebours de 3 min avant retour automatique en STA normal.
unsigned long apRecoveryStartMs = 0;
bool          apRecoveryActive  = false;
String        apRecoveryIP      = "";
const unsigned long AP_RECOVERY_DURATION_MS = 180000UL;
const char*   AP_RECOVERY_SSID  = "RecalBox-DMD-Config";

void maintainApRecovery()
{
  if (!apRecoveryActive) return;
  unsigned long ms = millis();
  if (ms < apRecoveryStartMs) apRecoveryStartMs = ms; // protection wrap millis()
  unsigned long elapsed = ms - apRecoveryStartMs;
  if (elapsed >= AP_RECOVERY_DURATION_MS) {
    apRecoveryActive = false;
    Serial.println("[WIFI] fin mode secours -> reboot STA normal");
    writeConfigFlag("force_ap_recovery", "0");
    delay(100);
    ESP.restart();
    return;
  }
  static unsigned long lastSecUpdate = 0;
  if (ms - lastSecUpdate >= 1000UL) {
    lastSecUpdate = ms;
    unsigned long remaining = (AP_RECOVERY_DURATION_MS - elapsed) / 1000UL;
    g_sdOpMsg = trWifiRecoveryCountdown(remaining);
    // v55 : redessine directement la ligne 1 SEULE (meme rendu que
    // webDmdForceRedraw() pour cette ligne), au lieu de passer par
    // g_configDmdDirty=true -- ce dernier declenche un reset complet des
    // 2 lignes (webDmdForceRedraw() remet aussi g_sdOpScrollOffset a 0),
    // alors que seule la ligne 1 (countdown) vient de changer ici. Sans
    // ce fix, le defilement de la ligne 2 (SSID/IP, alterne toutes les
    // 6s juste en dessous) etait remis a zero CHAQUE SECONDE -- jamais
    // assez de temps pour defiler jusqu'au SSID/IP, situes en fin de
    // chaine apres un long prefixe. Bug signale par l'utilisateur.
    display->setTextWrap(false);
    display->setTextSize(1);
    display->fillRect(0, 4, 128, 8, 0);
    display->setTextColor(0xFFE0);
    display->setCursor(1, 4);
    display->print(g_sdOpMsg);
    g_sdOpScrollOffset1 = 0;
    g_sdOpLastScroll1 = ms;
  }
  // Alterne SSID / IP (avec instruction prefixee) sur la ligne 2 toutes les
  // 6s -- ces chaines depassent 128px avec le prefixe, 6s (au lieu de 2s)
  // laisse le defilement horizontal existant le temps d'avancer avant de
  // reinitialiser le scroll sur la chaine suivante. Premier message =
  // SSID (demande utilisateur, ordre SSID puis IP) -- base sur "elapsed"
  // (temps ecoule DEPUIS l'entree en mode secours, pas millis() absolu)
  // pour garantir une vraie fenetre de 6s avant le 1er basculement : avec
  // l'ancien "lastToggle" compare a millis() absolu (temps ecoule depuis
  // le tout premier boot), le compte a rebours pouvait deja depasser 6s
  // au moment du tout premier appel (selon la duree du boot avant
  // d'atteindre ce point), faisant basculer sur IP quasi immediatement.
  static unsigned long lastToggleElapsed = 0;
  static bool showSSID = true;
  if (elapsed - lastToggleElapsed >= 6000UL) {
    lastToggleElapsed = elapsed;
    showSSID = !showSSID;
    if (showSSID) {
      String ssid = String(AP_RECOVERY_SSID);
      g_sdOpSubMsg = trJoinWifi(ssid);
      g_sdOpSubMsgWhiteFrom = (int)g_sdOpSubMsg.length() - (int)ssid.length();
    } else {
      String url = String("http://") + apRecoveryIP;
      g_sdOpSubMsg = trOpenInBrowser(url);
      g_sdOpSubMsgWhiteFrom = (int)g_sdOpSubMsg.length() - (int)url.length();
    }
    g_configDmdDirty = true;
  }
}

void setupWiFiFromConfig()
{
  if(!wifiEnabled){WiFi.disconnect(true);WiFi.mode(WIFI_OFF);return;}
  if(g_forceApRecovery){
    Serial.println("[WIFI] force_ap_recovery actif -> AP secours pur");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_RECOVERY_SSID);
    delay(500);
    apRecoveryStartMs = millis();
    apRecoveryActive = true;
    apRecoveryIP = WiFi.softAPIP().toString();
    if (apRecoveryIP == "0.0.0.0") apRecoveryIP = "192.168.4.1";
    g_sdOpMsg = trWifiRecoveryCountdown(AP_RECOVERY_DURATION_MS / 1000UL);
    {
      String ssid = String(AP_RECOVERY_SSID);
      g_sdOpSubMsg = trJoinWifi(ssid);
      g_sdOpSubMsgWhiteFrom = (int)g_sdOpSubMsg.length() - (int)ssid.length();
    }
    g_sdOpSubMsgColor = 0xFFE0;
    g_sdOpInProgress = true;
    currentMode = MODE_CONFIG;
    g_configDmdDirty = true;
    return;
  }
  if(wifiSSID.length()==0){
    Serial.println("[WIFI] No SSID -> AP mode");
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAP("RecalBox-DMD-Config");
    delay(500);
    String apIP = WiFi.softAPIP().toString();
    if (apIP == "0.0.0.0") apIP = "192.168.4.1";
    Serial.println("[WIFI] AP started: " + apIP);
    if(showInfo) showWifiStatusScreen("AP: RecalBox-DMD", apIP, display->color565(0,180,255));
    delay(500);
    return;
  }
  // setAutoReconnect desormais FALSE (2026-08-10, voir changelog v68) :
  // sa tache interne au driver, opaque et hors controle applicatif, est
  // un 2e candidat de collision LWIP avec mqttTask -- maintainWiFi()
  // (deja en place, appelee a chaque loop(), cooldown 5s, reapplique
  // l'IP fixe) reste desormais la SEULE source de reconnexion.
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.setAutoReconnect(false);
  // v135 -- diagnostic ajoute (proposition utilisateur, etudiee et validee :
  // "separer strictement la recuperation MQTT de la recuperation WiFi...
  // ajouter un diagnostic minimal mais decisif : raison de deconnexion WiFi
  // ESP32, RSSI, canal") -- jamais capture jusqu'ici cette session, tout ce
  // qu'on avait etait indirect (WiFi.status() reste a WL_CONNECTED, sans
  // savoir pourquoi les emissions ne passent plus). Enregistre UNE SEULE
  // FOIS ici (couvre aussi bien les deconnexions initiees par maintainWiFi()
  // que toute perte inattendue cote AP) -- callback leger, un seul
  // Serial.println, pas de travail lourd dans le contexte evenementiel.
  WiFi.onEvent(
    [](arduino_event_id_t event, arduino_event_info_t info) {
      Serial.println("[WIFI] STA disconnected, reason=" + String(info.wifi_sta_disconnected.reason)
                     + " (voir wifi_err_reason_t pour la signification exacte du code)");
    },
    ARDUINO_EVENT_WIFI_STA_DISCONNECTED
  );
  if(!applyStaticIP()){if(showInfo)showWifiStatusScreen("WIFI","IP CFG ERR",display->color565(255,0,0));delay(1200);}
  if(showInfo)showWifiStatusScreen("WIFI","CONNECT",display->color565(0,180,255));
  // Plusieurs tentatives avant d'abandonner et de basculer en AP: sur un
  // reseau multi-VLAN, l'association + le bail DHCP peuvent occasionnellement
  // depasser une seule fenetre de 12s (relais DHCP inter-VLAN, convergence
  // STP du port switch, attribution dynamique de VLAN par SSID/RADIUS) sans
  // que le SSID/mot de passe soit en cause -- un seul echec transitoire ne
  // doit pas condamner tout le boot a un fallback AP definitif (jusqu'au
  // reboot). Les identifiants reellement faux echouent quand meme aux 3
  // tentatives (retenter ne repare pas un mauvais mot de passe), donc le
  // comportement de fallback lui-meme est inchange, juste retarde de
  // quelques secondes.
  const int WIFI_CONNECT_ATTEMPTS = 3;
  const unsigned long WIFI_ATTEMPT_TIMEOUT_MS = 9000;
  for (int attempt = 0; attempt < WIFI_CONNECT_ATTEMPTS; attempt++) {
    // v131 -- meme raisonnement que maintainWiFi() (voir son commentaire
    // complet) : WiFi.disconnect() peut reinitialiser l'etat power-save
    // pose une seule fois plus haut (avant la 1ere tentative) -- reapplique
    // ici aussi pour qu'un simple retry au boot ne perde jamais ce reglage.
    if (attempt > 0) { WiFi.disconnect(); delay(100); WiFi.setSleep(false); }
    WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());
    unsigned long start=millis();
    while(WiFi.status()!=WL_CONNECTED&&(millis()-start)<WIFI_ATTEMPT_TIMEOUT_MS){
      if(!showInfo) bootHourglassTick();
      delay(200);
    }
    if (WiFi.status()==WL_CONNECTED) break;
    Serial.println("[WIFI] attempt " + String(attempt+1) + "/" + String(WIFI_CONNECT_ATTEMPTS) + " failed");
  }
  if(WiFi.status()==WL_CONNECTED)
  {
    String ip=WiFi.localIP().toString();
    if(showInfo) showWifiStatusScreen("WIFI OK",fitLabel(ip,14),display->color565(0,255,0));
    Serial.println("[WIFI] connected: "+ip);
    // v1 -- piste UDP (voir TRANSPORT_PLAN_UDP.md) : ne depend PAS de
    // recalboxIP (contrairement au bloc mqttClient juste apres) -- le DMD se
    // contente d'ecouter, n'importe quelle source peut lui envoyer un
    // datagramme. begin() une seule fois ici ; PAS rearme apres une
    // reconnexion WiFi ulterieure pour l'instant (voir commentaire pres de
    // la declaration de dmdUdp).
    dmdUdp.begin(UDP_CMD_PORT);
    Serial.println("[UDP] listening on port " + String(UDP_CMD_PORT));
    delay(1200);
    autoDetectRecalboxIP();
    if(recalboxIP.length()>0){
      mqttClient.setServer(recalboxIP.c_str(),MQTT_PORT);
      mqttClient.setCallback(onMqttMessage);
      // v79 -- port depuis dev/mame-score-mqtt-bridge : buffer PubSubClient
      // par defaut = 256 octets (MQTT_MAX_PACKET_SIZE), largement suffisant
      // pour tous les topics d'avant ce chunk (le plus long, le top 5
      // hiscore multi-rangs, fait ~90 octets) -- mais game_info peut faire
      // jusqu'a ~700 octets de texte (voir MAX_TOTAL_LEN dans
      // dmd_game_info.py), largement au-dela : sans setBufferSize(),
      // PubSubClient tronque/rejette silencieusement le message (pas
      // d'erreur visible). 1024 = marge confortable (payload + topic +
      // entetes MQTT).
      mqttClient.setBufferSize(1024);
      // v100 -- BUG REEL confirme sur materiel : connexion "zombie" observee
      // en DIRECT (silence total >2min30, aucune tentative de reconnexion,
      // ecran DMD fige sur "RB connectee") -- mqttClient.connected() restait
      // true (TCP "a moitie mort", pair disparu sans FIN/RST propre), et
      // AUCUN de nos garde-fous existants (v94, verifie seulement au moment
      // du connect()) ne peut detecter un silence qui s'installe PLUS TARD
      // en cours de session. Seul le keepalive interne de PubSubClient
      // (PINGREQ/PINGRESP) peut detecter ce cas -- mais setKeepAlive(60)
      // (60s, sans justification documentee, tres au-dessus des 15s par
      // defaut de la librairie) le rend beaucoup trop lent : il faut ~1.5-2x
      // ce delai avant que PubSubClient marque la connexion morte, soit
      // 90-120s+ avant meme de COMMENCER une vraie reconnexion -- cohere
      // avec les silences de plusieurs minutes observes ce soir. Fix :
      // retour a 15s (defaut librairie) -- cout reseau negligeable (quelques
      // octets toutes les 15s sur un WiFi local qui vehicule deja des
      // payloads bien plus gros), gain direct : detection ~4x plus rapide
      // d'une connexion zombie.
      mqttClient.setKeepAlive(15);
      // v80 (2026-08-17) -- CRASH REEL confirme sur materiel (reset
      // TASK_WDT, decode via addr2line, hash ELF verifie identique au
      // binaire plante) : mqttTask() bloque dans PubSubClient::connect()
      // (PubSubClient.cpp:257, boucle "while(!_client->available())" SANS
      // AUCUN yield/delay en attendant le CONNACK) -- avec un timeout de
      // 30s, cette boucle serree peut affamer IDLE0 (cœur 0) assez
      // longtemps pour declencher le watchdog materiel. Comportement DEJA
      // documente comme tel dans PubSubClient (pas un bug introduit par ce
      // chantier -- mqttTask() etait deja seul sur le cœur 0 avant ET apres
      // le changement LoopCore, ce risque existait deja, juste jamais
      // declenche en test avant maintenant : il faut une reconnexion qui
      // traine pour l'atteindre). Premier fix : timeout abaisse a 3s.
      // v83 (2026-08-17) -- CORRECTIF PLUS PROFOND suite a investigation
      // reseau sur materiel reel (reconnexions spontanees en rafale meme
      // au repos, sans rapport avec le rendu/l'overlay -- deja constate sur
      // dev/mame-score-mqtt-bridge, independant du changement de cœur).
      // Root cause de la boucle sans yield corrigee DIRECTEMENT dans
      // PubSubClient.cpp (patch local, voir son commentaire pres de la
      // ligne 257 : yield() ajoute dans la boucle d'attente du CONNACK,
      // meme protection que readByte() plus bas dans ce meme fichier, qui
      // l'avait deja) -- le watchdog ne peut plus se declencher quelle que
      // soit la duree d'attente. Peut donc remonter ce timeout a une valeur
      // plus tolerante SANS reintroduire le risque watchdog : 3s s'est
      // revele trop impatient en usage reel -- setSocketTimeout() couvre
      // TOUTES les lectures socket de PubSubClient (pas seulement
      // connect()), donc une lenteur passagere du broker (ex. RB occupee a
      // emuler un jeu) pendant mqttClient.loop() normal pouvait aussi
      // declencher une reconnexion prematuree -- qui elle-meme, via l'ID
      // client FIXE (MQTT_CLIENT="esp32-marquee"), force le broker a fermer
      // la connexion precedente (cause probable des sockets zombies
      // "CLOSING" observees cote broker), cascade auto-entretenue. 10s :
      // large tolerance pour ce cas, tout en restant borne (pas de retour
      // au risque watchdog d'avant v80, corrige a la racine par le patch).
      mqttClient.setSocketTimeout(10);
    }
  }
  else{
    // Repli AP uniquement si le parcours "premier demarrage" n'est pas
    // encore termine (bug corrige 2026-08-05, demande utilisateur --
    // logique cible en 3 etapes) : un appareil DEJA entierement
    // configure (first_boot=0) dont le WiFi devient temporairement
    // injoignable (routeur eteint, coupure passagere) ne doit PAS etre
    // renvoye en mode AP/config -- il continue de demarrer normalement
    // (playlist locale), maintainWiFi() se chargeant de reessayer la
    // reconnexion en tache de fond sans reboot ni ecran force. Le repli
    // AP reste le comportement voulu tant que g_firstBoot est vrai
    // (identifiants WiFi eventuellement faux saisis lors de la phase 1,
    // il faut pouvoir les ressaisir).
    if (g_firstBoot) {
      Serial.println("[WIFI] failed -> AP fallback");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("RecalBox-DMD-Config");
      delay(1000);
      String apIP = WiFi.softAPIP().toString();
      if (apIP == "0.0.0.0") apIP = "192.168.4.1";
      // Mode config avec message AP
      g_sdOpMsg = trConnectWifiMsg();
      g_sdOpSubMsg = trOpenUrl(apIP);
      g_sdOpSubMsgColor = 0xFFE0;
      g_sdOpInProgress = true;
      currentMode = MODE_CONFIG;
      g_configDmdDirty = true;
      Serial.println("[WIFI] AP fallback -> http://" + apIP);
    } else {
      Serial.println("[WIFI] failed, first_boot=0 -> pas de repli AP, demarrage normal (maintainWiFi() reessaiera)");
    }
  }
}

void maintainWiFi()
{
  if(!wifiEnabled||wifiSSID.length()==0) return;
  if(WiFi.status()==WL_CONNECTED) { consecutiveWifiReconnectCycles=0; return; } // v155 -- reconnexion reussie, recul reinitialise
  // Ne pas reconnecter en mode AP (fallback) ou si config web ouverte
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) return;
  if (g_sdOpInProgress) return;
  unsigned long now=millis();
  // v155 -- cooldown PROGRESSIF (etait fixe 5000ms). Suspecte d'etre trop
  // court pour laisser une tentative WiFi.begin() vraiment aboutir
  // (association + DHCP) en conditions degradees -- chaque nouvel appel
  // interrompait alors la tentative precedente avant qu'elle ait pu
  // reussir, via un nouveau WiFi.disconnect(), creant une boucle de retry
  // auto-entretenue (raison=8, deconnexion auto-initiee cote DMD).
  // Reproduit en direct le 2026-09-04 (13:33-13:36, 3min39, voir
  // DECISIONS.md) juste apres le tout premier declenchement reel de
  // l'escalade WiFi (v144) -- le mutex (wifiResetMutex) empechait bien
  // la collision entre les 2 call sites, mais ne protegeait pas
  // maintainWiFi() seule contre ses propres reprises trop rapprochees.
  // Meme principe que le recul deja en place cote MQTT
  // (SUBSCRIBE_BACKOFF_STEP_MS/MAX_MS) : cooldown croit par cycle
  // consecutif sans succes, plafonne, repart a la base des qu'une
  // connexion aboutit (reset ci-dessus).
  const unsigned long WIFI_RECONNECT_COOLDOWN_BASE_MS = 5000;
  const unsigned long WIFI_RECONNECT_COOLDOWN_STEP_MS = 5000;
  const unsigned long WIFI_RECONNECT_COOLDOWN_MAX_MS  = 30000;
  unsigned long cooldown = WIFI_RECONNECT_COOLDOWN_BASE_MS
                          + (unsigned long)consecutiveWifiReconnectCycles * WIFI_RECONNECT_COOLDOWN_STEP_MS;
  if (cooldown > WIFI_RECONNECT_COOLDOWN_MAX_MS) cooldown = WIFI_RECONNECT_COOLDOWN_MAX_MS;
  if(now-lastWifiReconnectAttempt<cooldown) return;
  // v144 -- mutex partage avec l'escalade WiFi de mqttTask() (voir
  // wifiResetMutex, sa declaration). Timeout court et non bloquant :
  // si l'autre cote tient deja le mutex (son propre reset WiFi en
  // cours), on abandonne simplement CE tour -- le cooldown ci-dessus
  // fait qu'on retentera naturellement au prochain loop().
  if(wifiResetMutex!=nullptr && xSemaphoreTake(wifiResetMutex,pdMS_TO_TICKS(200))!=pdTRUE) return;
  lastWifiReconnectAttempt=now;
  consecutiveWifiReconnectCycles++; // v155 -- ce cycle n'a pas encore prouve son succes, recul augmente pour le prochain
  Serial.println("[WIFI] reconnect (cycle " + String(consecutiveWifiReconnectCycles)
                 + ", cooldown etait " + String(cooldown) + "ms)");
  delay(1500);WiFi.disconnect();delay(50);
  // Reappliquer l'IP fixe: WiFi.disconnect() reinitialise la config IP de
  // l'interface, sans reappel ici toute reconnexion repassait silencieusement
  // en DHCP (bug identifie en session -- IP fixe perdue apres la moindre
  // coupure WiFi transitoire).
  applyStaticIP();
  WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());
  // v131 -- MEME PRINCIPE que le fix IP fixe juste au-dessus (retour
  // utilisateur : cycles courts de deconnexion MQTT non expliques,
  // enquete DECISIONS.md -- cause racine confirmee cote broker : le DMD
  // n'emet plus rien pendant 10-25s juste apres un connect() MQTT reussi,
  // symptome classique du mode power-save WiFi ESP32 par defaut
  // (WIFI_PS_MIN_MODEM, radio periodiquement endormie jusqu'au prochain
  // cycle DTIM) -- voir doc Espressif, wifi-performance-and-power-save).
  // WiFi.setSleep(false) EST deja pose une fois au tout premier boot
  // (setup(), avant WiFi.begin()) mais JAMAIS reapplique ici : exactement
  // comme l'IP fixe, WiFi.disconnect()+WiFi.begin() peut reinitialiser cet
  // etat cote driver -- toute reconnexion WiFi en cours d'uptime (RSSI,
  // hoquet AP, etc., independante des reconnexions MQTT elles-memes)
  // pouvait donc laisser le power-save se reactiver silencieusement pour
  // le reste de la session, sans jamais etre repose. Pas encore confirme
  // comme LA cause exacte (aucun WiFi.status()!=WL_CONNECTED observe
  // pendant les episodes chasses cette nuit -- mais l'etat power-save
  // reel du driver n'est pas observable depuis l'API Arduino, seule une
  // reconnexion anterieure non tracee peut l'avoir declenche) -- fix a
  // cout nul, applique par prudence/coherence avec le fix IP fixe deja
  // en place pour exactement la meme classe de probleme.
  WiFi.setSleep(false);
  if(wifiResetMutex!=nullptr) xSemaphoreGive(wifiResetMutex); // v144
}

// --------------------------------------------------
// Config
// --------------------------------------------------
void loadConfig()
{
  File cfg=SD.open("/config.ini"); if(!cfg) return;
  while(cfg.available())
  {
    String line=cfg.readStringUntil('\n');line.trim();
    if(!line.length()||line[0]=='#'||line[0]==';') continue;
    int eq=line.indexOf('=');if(eq<0) continue;
    String key=line.substring(0,eq),value=line.substring(eq+1);
    key.trim();value.trim();key.toLowerCase();
    int cp=value.indexOf('#');if(cp>=0)value=value.substring(0,cp);
    cp=value.indexOf(';');   if(cp>=0)value=value.substring(0,cp);
    value.trim();

    if     (key=="playlist"            &&value.length()) playlistName     =value;
    else if(key=="wifi_enabled")                         wifiEnabled      =(value=="1");
    else if(key=="wifi_ssid")                            wifiSSID         =value;
    else if(key=="wifi_password")                        wifiPassword     =value;
    else if(key=="wifi_static_enabled")                  wifiStaticEnabled=(value=="1");
    else if(key=="wifi_static_ip")                       wifiStaticIP     =value;
    else if(key=="wifi_gateway")                         wifiGateway      =value;
    else if(key=="wifi_subnet")                          wifiSubnet       =value;
    else if(key=="wifi_dns1")                            wifiDNS1         =value;
    else if(key=="wifi_dns2")                            wifiDNS2         =value;
    else if(key=="bluetooth_enabled")                    bluetoothEnabled =(value=="1");
    else if(key=="bluetooth_name"     &&value.length())  bluetoothName    =value;
    else if(key=="recalbox_ip"        &&value.length())  recalboxIP       =value;
    else if(key=="random")                               playlistRandom   =(value!="0");
    else if(key=="info")                                 showInfo         =(value!="0");
    // v110 -- reglages hi-score/info/description/RA (voir declaration).
    else if(key=="feat_hiscore_ingame")                  featHiscoreIngame    =(value!="0");
    else if(key=="feat_hiscore_browse")                  featHiscoreBrowse    =(value!="0");
    else if(key=="feat_info_ingame")                     featInfoIngame       =(value!="0");
    else if(key=="feat_info_browse")                      featInfoBrowse       =(value!="0");
    else if(key=="feat_description_ingame")              featDescriptionIngame=(value!="0");
    else if(key=="feat_description_browse")              featDescriptionBrowse=(value!="0");
    else if(key=="feat_ra_ingame")                       featRaIngame         =(value!="0");
    else if(key=="feat_ra_browse")                       featRaBrowse         =(value!="0");
    else if(key=="feat_repeat_cycles")                   featRepeatCycles     =constrain(value.toInt(),0,20);
    // v111 -- voir declaration (featRepeatBrowseCycles/featDwellSeconds).
    // Plancher de securite 3s IMPOSE ICI (pas seulement cote script) pour
    // featDwellSeconds -- demande utilisateur explicite.
    else if(key=="feat_repeat_browse_cycles")            featRepeatBrowseCycles=constrain(value.toInt(),0,20);
    else if(key=="feat_dwell_seconds")                   featDwellSeconds     =constrain(value.toInt(),3,30);
    else if(key=="brightness")                            screenBrightness =map(constrain(value.toInt(),0,100),0,100,0,255);
    else if(key=="mqtt_event_topic"   &&value.length())  mqttEventTopic   =value;
    else if(key=="first_boot")                           g_firstBoot      =(value!="0");
    else if(key=="force_ap_recovery")                    g_forceApRecovery=(value!="0");
    else if(key=="force_config_boot")                    g_skipPlaylistForConfig=(value!="0");
    else if(key=="language" && (value=="fr"||value=="en"||value=="es")) uiLanguage=value;
  }
  cfg.close();

  // Images directement dans systems/<sys>/, pas de sous-dossier
  gamesCacheFile = "/games_cache.bin";
  Serial.println("[CACHE] fichier jeux: " + gamesCacheFile);

  if(!playlistName.length()) return;
  playlistSourcePath="/playlists/"+playlistName;
  String base=playlistName; int dot=base.lastIndexOf('.');if(dot>0)base=base.substring(0,dot);
  playlistCachePath="/playlists/"+base+".cache";
  playlistSigPath  ="/playlists/"+base+".sig";
  playlistIdxPath  ="/playlists/"+base+".idx";
}

// v110 -- diffuse les 8 reglages hi-score/info/description/RA en un seul
// message MQTT RETENU (marquee/status/features) : appelee a chaque
// (re)connexion MQTT reussie (voir mqttTask(), juste apres la publication
// de marquee/status/ip) ET a chaque sauvegarde web (voir
// handleWebConfigSave()). Retenu = un script RB qui redemarre recoit
// IMMEDIATEMENT le dernier etat connu sans avoir a interroger le DMD --
// coherent avec la philosophie "DMD bete" (le DMD ANNONCE passivement son
// reglage, il ne pilote jamais l'affichage lui-meme). Format cle=valeur
// separe par ";", volontairement simple/parsable en shell POSIX (meme
// esprit que le reste du protocole marquee/cmd/*).
void broadcastFeatureStatus()
{
  if (mqttClient.state() != 0) return; // pas connecte, rien a publier
  // v127 -- METHODE ENTIEREMENT REVUE (retour utilisateur explicite : "la
  // methode est a revoir pour le transfert des reglages", pas juste un
  // patch par-dessus). Validation live du garde v126 CETTE MEME SOIREE :
  // 3 troncatures reelles observees sur le broker (ex. "2;dwell_seconds=3"),
  // A CHAQUE fois avec la MEME signature -- un PREFIXE de longueur variable
  // perdu, le SUFFIXE toujours intact -- compatible avec un echec de
  // reallocation survenant EN COURS d'une longue chaine de concatenations
  // String (l'ancienne construction enchainait ~20 operator+ successifs,
  // chacun creant un objet temporaire avec sa propre allocation/liberation
  // heap, juste apres un connect() MQTT reussi -- moment ou le heap est
  // deja scrute dans plusieurs autres bugs de ce fichier). Plutot que de
  // continuer a detecter puis rejeter la corruption apres coup (v126, garde
  // toujours en place ci-dessous en filet de securite complementaire),
  // supprime le mecanisme suspecte A LA RACINE : buffer FIXE sur la PILE
  // (snprintf) -- ZERO allocation heap pour construire ce message, donc
  // structurellement impossible qu'un echec de realloc EN COURS DE CHAINE
  // le corrompe. 256o : chaine complete ~178 caracteres au maximum
  // (repeat_cycles/repeat_browse_cycles a 2 chiffres, marge large).
  char buf[256];
  int n = snprintf(buf, sizeof(buf),
    "hiscore_ingame=%d;hiscore_browse=%d;info_ingame=%d;info_browse=%d;"
    "description_ingame=%d;description_browse=%d;ra_ingame=%d;ra_browse=%d;"
    "repeat_cycles=%d;repeat_browse_cycles=%d;dwell_seconds=%d",
    featHiscoreIngame ? 1 : 0, featHiscoreBrowse ? 1 : 0,
    featInfoIngame ? 1 : 0, featInfoBrowse ? 1 : 0,
    featDescriptionIngame ? 1 : 0, featDescriptionBrowse ? 1 : 0,
    featRaIngame ? 1 : 0, featRaBrowse ? 1 : 0,
    featRepeatCycles, featRepeatBrowseCycles, featDwellSeconds);
  // v126 -- garde de sanite CONSERVEE en filet de securite complementaire
  // (defense en profondeur, cout negligeable) : n<0 = erreur snprintf,
  // n>=sizeof(buf) = aurait ete tronque par la taille du buffer (jamais
  // attendu ici vu la marge, mais verifie explicitement plutot que de
  // supposer). 2e ligne de defense independante cote RB (dmd_score.sh v30) :
  // rejette aussi tout message incomplet a la reception, cache existant
  // conserve.
  if (n <= 0 || n >= (int)sizeof(buf) || strncmp(buf, "hiscore_ingame=", 15) != 0)
  {
    Serial.println("[MQTT] broadcastFeatureStatus ABANDON (snprintf n=" + String(n) + "): " + String(buf));
    return;
  }
  mqttClient.publish("marquee/status/features", buf, true);
  Serial.println(String("[MQTT] marquee/status/features -> ") + buf);
}

bool isValidPlaylistLine(String line){line.trim();return line.length()&&line[0]!='#'&&line[0]!=';'&&line[0]=='/';}

uint32_t computeFileHash(const String &path)
{
  File f = SD.open(path, FILE_READ);
  if (!f) return 0;

  uint32_t h = 2166136261u;

  uint8_t buf[512];
  while (true)
  {
    size_t n = f.read(buf, sizeof(buf));
    if (n == 0) break;

    for (size_t i = 0; i < n; i++)
    {
      h ^= buf[i];
      h *= 16777619u;
    }
  }

  f.close();
  return h;
}

uint32_t readSavedSignature()
{
  File f=SD.open(playlistSigPath,FILE_READ);if(!f)return 0;
  String s=f.readStringUntil('\n');f.close();s.trim();
  return s.length()?(uint32_t)strtoul(s.c_str(),NULL,10):0;
}

bool writeSignature(uint32_t sig)
{
  if(SD.exists(playlistSigPath))SD.remove(playlistSigPath);
  File f=SD.open(playlistSigPath,FILE_WRITE);if(!f)return false;
  f.println(String(sig));f.close();return true;
}

int rebuildPlaylistCache()
{
  File src=SD.open(playlistSourcePath,FILE_READ);if(!src)return 0;
  if(SD.exists(playlistCachePath))SD.remove(playlistCachePath);
  File cache=SD.open(playlistCachePath,FILE_WRITE);if(!cache){src.close();return 0;}
  int n=0;showLoadingHourglass(0);
  while(src.available()){
    String line=src.readStringUntil('\n');line.trim();
    if(isValidPlaylistLine(line)){cache.println(line);n++;if((n%6)==0)showLoadingHourglass(n);}
    delay(0);
  }
  src.close();cache.close();showLoadingHourglass(n);return n;
}

int buildOffsetIndex()
{
  File cache=SD.open(playlistCachePath,FILE_READ);if(!cache)return 0;
  if(SD.exists(playlistIdxPath))SD.remove(playlistIdxPath);
  File idx=SD.open(playlistIdxPath,FILE_WRITE);
  int n=0;
  while(cache.available()){
    uint32_t pos=cache.position();
    String line=cache.readStringUntil('\n');line.trim();
    if(line.length()){if(idx)idx.write((uint8_t*)&pos,4);n++;}
    delay(0);
  }
  cache.close();if(idx)idx.close();
  if(idxFileHandle)idxFileHandle.close();
  idxFileHandle=SD.open(playlistIdxPath,FILE_READ);
  if(!playlistRandom){if(seqPlaylistFile)seqPlaylistFile.close();seqPlaylistFile=SD.open(playlistCachePath,FILE_READ);playIndex=0;}
  return n;
}


// --------------------------------------------------
// Splash screen â€” version au dÃ©marrage (info=1 uniquement)
// --------------------------------------------------
#define RETRO_VERSION "Raw565 Ed. v13UDP"

void showSplashScreen()
{
  display->clearScreen();
  display->setTextWrap(false);
  display->setTextSize(1);

  uint16_t red   = display->color565(255, 40,  40);
  uint16_t blue  = display->color565( 60, 130, 255);
  uint16_t green = display->color565( 50, 220,  80);
  uint16_t white = display->color565(235, 235, 235);

  // Panel = 2 x 64px = 128px de large, 32px de haut
  // Taille 1 = 6px par caractere
  // "RetroBoxLED" = 11 x 6 = 66px  â†’ x = (128-66)/2 = 31
  // "v1.0.7"      =  6 x 6 = 36px  â†’ x = (128-36)/2 = 46 (confirme)

  // Ligne 1 : RetroBoxLED centrÃ©, un seul setCursor puis print enchaÃ®nÃ©s
  display->setCursor(31, 8);
  display->setTextColor(red);   display->print("Recal");
  display->setTextColor(blue);  display->print("Box");
  display->setTextColor(green); display->print("DMD");

  // Ligne 2 : version centrée ("Raw565 Ed. dev_pl" = 17 x 6 = 102px -> x = (128-102)/2 = 13)
  display->setCursor(13, 21);
  display->setTextColor(white);
  display->print(RETRO_VERSION);

  delay(2500);
  // En boot silencieux (info=0) le titre reste affiche, le sablier se dessine
  // par-dessus (cf bootHourglassTick()) jusqu'a la fin du boot WiFi/NTP.
  if (showInfo) display->clearScreen();
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
// --------------------------------------------------
// Horloge -- 3 styles de police proceduraux (fillRect segments epais)
// Dimensions: 128x32, chaque digit ~20x30px avec segments de 4px
// --------------------------------------------------
// Dimensions digit: 20 largeur x 30 hauteur, espacement 2px
// ":" = 8x30, espacement 2px
// Total: 4*20 + 8 + 3*2 = 94px -> baseX=(128-94)/2=17, baseY=(32-30)/2=1
// Chaque segment utilise fillRect pour effet epais

// Segments 7-segments pour un digit 20x30 (epaisseur 4px)
// Segment A: haut  (x+1..x+18, y..y+3)
// Segment B: droite-haut  (x+16..x+19, y+4..y+13)
// Segment C: droite-bas   (x+16..x+19, y+17..y+26)
// Segment D: bas   (x+1..x+18, y+27..y+30)
// Segment E: gauche-bas   (x+0..x+3, y+17..y+26)
// Segment F: gauche-haut  (x+0..x+3, y+4..y+13)
// Segment G: milieu (x+1..x+18, y+14..y+17)

// Segments sans gap: B/C/E/F etendus d'1px pour toucher G et D
// Ainsi les chiffres avec segment G (2,3,5,6,8,9,0) ont la meme
// hauteur visuelle que ceux sans (1,4,7)

// Convertit le fuseau POSIX (ex: "CET-1CEST,M3.5.0,M10.5.0/3") en libelle
// comprehensible pour l'utilisateur (ex: "UTC+1" ou "UTC+2 (DST)").
static String tzFriendlyLabel()
{
  time_t now;
  time(&now);
  struct tm local_tm, utc_tm;
  localtime_r(&now, &local_tm);
  gmtime_r(&now, &utc_tm);
  // tm_gmtoff n'est pas disponible sur ce toolchain ESP32/newlib: on calcule
  // l'offset UTC en reinterpretant l'heure UTC comme si elle etait locale.
  utc_tm.tm_isdst = local_tm.tm_isdst;
  time_t utc_as_local = mktime(&utc_tm);
  long off = (long)difftime(now, utc_as_local);
  char sign = (off < 0) ? '-' : '+';
  long absOff = labs(off);
  int oh = absOff / 3600;
  int om = (absOff % 3600) / 60;
  char buf[20];
  if (om == 0) snprintf(buf, sizeof(buf), "UTC%c%d", sign, oh);
  else         snprintf(buf, sizeof(buf), "UTC%c%d:%02d", sign, oh, om);
  if (local_tm.tm_isdst > 0) strncat(buf, " (DST)", sizeof(buf) - strlen(buf) - 1);
  return String(buf);
}

static void initNTP()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[CLOCK] NTP skip: WiFi not connected");
    return;
  }

  if (showInfo)
  {
    display->clearScreen();
    display->setTextWrap(false); display->setTextSize(1);
    display->setTextColor(display->color565(255, 200, 0));
    display->setCursor(1, 8);  display->print("SYNC NTP...");
  }

  configTzTime(clockTimeZone.c_str(), "pool.ntp.org", "time.google.com");
  if (showInfo) { display->setCursor(1, 20); display->print(tzFriendlyLabel()); }

  // Attente active synchrone jusqu'à 10 secondes
  unsigned long timeout = millis() + 10000UL;
  bool ok = false;
  while ((long)(millis() - timeout) < 0)
  {
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);
    if (ti.tm_year > 100)
    {
      ok = true;
      break;
    }
    if (!showInfo) bootHourglassTick();
    delay(200);
    yield();
  }

  clockNtpSynced = ok;

  if (showInfo)
  {
    display->clearScreen();
    if (ok)
    {
      display->setTextColor(display->color565(0, 255, 0));
      display->setCursor(1, 8);  display->print("NTP OK ");
      // Afficher l'heure synchronisée
      time_t now;
      struct tm ti;
      time(&now);
      localtime_r(&now, &ti);
      char buf[6];
      sprintf(buf, "%02d:%02d", ti.tm_hour, ti.tm_min);
      display->setCursor(1, 20); display->print(buf);
    }
    else
    {
      display->setTextColor(display->color565(255, 0, 0));
      display->setCursor(1, 12); display->print("NTP FAIL");
    }
    delay(1200);
    display->clearScreen();
  }
  Serial.println(ok ? "[CLOCK] NTP sync OK" : "[CLOCK] NTP sync FAILED (timeout 10s)");
}

static bool getClockTime(int &h, int &m, int &s)
{
  time_t now;
  struct tm ti;
  time(&now);
  localtime_r(&now, &ti);
  if (ti.tm_year < 100)
  {
    if (!clockNtpSynced && millis() - clockNtpLastTry > 10000UL)
    {
      Serial.println("[CLOCK] Waiting for NTP sync...");
      clockNtpLastTry = millis();
    }
    return false;
  }
  clockNtpSynced = true;
  h = ti.tm_hour;
  m = ti.tm_min;
  s = ti.tm_sec;
  return true;
}

// -- Show clock during clockDuration seconds, checks MQTT each frame --
// Affiche meme si NTP pas encore synchro (heure 1970 temporaire).
// -- Show clock using retro themes --
// Affiche les themes retro pixel-art pendant clockDuration.
// forceTheme (v72) : -2 (sentinelle, defaut) = comportement normal inchange
// (lit clockEnabled/clockTheme/clockDuration, appel existant dans loop()).
// -1..RETRO_THEME_COUNT-1 = mode apercu web (CMD_CLOCK_PREVIEW) : ignore
// clockEnabled, impose currentTheme (-1 tire un theme au hasard UNE fois,
// pas de rotation periodique -- voir plus bas), et la boucle d'affichage
// tourne SANS limite de duree (uniquement hasPendingMqttCommand(), deja
// verifie a chaque iteration, y met fin -- nouvelle selection ou "stop").
static bool showClock(int forceTheme)
{
  bool previewMode = (forceTheme != -2);
  if (!previewMode) {
    if (!clockEnabled) return true;
  } else {
    // v75 -- reset defensif : evite qu'un g_clockPreviewAbort pose par un
    // "Reprendre DMD" precedent (deja consomme ou arrive apres coup, sans
    // preview actif a interrompre a ce moment-la) ne fasse avorter CETTE
    // NOUVELLE preview des sa 1ere iteration.
    g_clockPreviewAbort = false;
  }
  // v73 fix (2e bug trouve au 1er test materiel post-v73) : ce garde
  // g_sdOpInProgress (herite du comportement normal hors preview, ou il
  // sert a ne pas afficher l'horloge par-dessus l'ecran de pause web) sortait
  // ICI avant meme de choisir un theme des que previewMode est actif -- or
  // g_sdOpInProgress est TOUJOURS vrai en mode preview (c'est justement la
  // page Horloge, web ouverte, qui vient de le demander) : ecran noir/vide
  // garanti a 100%, confirme par le log serie ("[CLOCK] preview theme=X"
  // affiche cote appelant mais jamais "[CLOCK] Start retro theme=" plus bas,
  // preuve du retour immediat ici). Les 3 autres occurrences de ce meme
  // garde plus bas dans cette fonction (banniere de nom x2 + boucle
  // principale) ont le meme probleme et sont corrigees pareil : en
  // previewMode, seul hasPendingMqttCommand() (nouvelle selection ou "stop")
  // doit interrompre l'affichage, comme deja documente en tete de fonction.
  if (!previewMode && g_sdOpInProgress) return true;

  // Choisir le theme
  int themeSource = previewMode ? forceTheme : clockTheme;
  currentTheme = (themeSource >= 0 && themeSource < RETRO_THEME_COUNT)
                  ? themeSource
                  : random(0, RETRO_THEME_COUNT);
  themeStartMs = millis();

  clockVisible = true;
  clockStartMs = millis();
  lastClockMs = clockStartMs;

  // Afficher brievement le nom du theme
  display->fillRect(0, 0, 128, 32, 0);
  display->setTextSize(1);
  display->setTextColor(display->color565(255,255,255));
  int tx = (128 - strlen(retroThemeNames[currentTheme]) * 6) / 2;
  if (tx < 0) tx = 0;
  display->setCursor(tx, 12);
  display->print(retroThemeNames[currentTheme]);
  {
    unsigned long nameEnd = millis() + 800UL;
    while (millis() < nameEnd) {
      handleWebConfig();
      yield();
      if (!previewMode && g_sdOpInProgress) { // v73 fix, voir plus haut
        clockVisible = false;
        return true;
      }
      if (previewMode && g_clockPreviewAbort) { // v75, voir webDmdResume()
        g_clockPreviewAbort = false;
        Serial.println("[CLOCK] preview interrupted by DMD resume");
        clockVisible = false;
        return true;
      }
      if (hasPendingMqttCommand()) {
        clockVisible = false;
        return true;
      }
      delay(1);
    }
  }
  display->clearScreen();

  Serial.println("[CLOCK] Start retro theme=" + String(retroThemeNames[currentTheme]) + " id=" + String(currentTheme));

  unsigned long endMs = clockStartMs + ((unsigned long)clockDuration * 1000UL);
  while (previewMode || millis() < endMs) {
    handleWebConfig();
    yield();

    // Interruption si page web ouverte (v73 : jamais en previewMode, ou
    // g_sdOpInProgress est en permanence vrai -- voir garde en tete de
    // fonction)
    if (!previewMode && g_sdOpInProgress) {
      Serial.println("[CLOCK] Interrupted by web page");
      clockVisible = false;
      return true;
    }

    // v75 -- "Reprendre DMD" clique pendant cet apercu (voir webDmdResume()
    // et le commentaire complet a la declaration de g_clockPreviewAbort).
    // resumePlaylist() a deja ouvert le GIF/mis a jour currentMode a ce
    // stade -- on se contente de sortir SANS y toucher.
    if (previewMode && g_clockPreviewAbort) {
      g_clockPreviewAbort = false;
      Serial.println("[CLOCK] preview interrupted by DMD resume");
      clockVisible = false;
      return true;
    }

    // MQTT interruption
    if (hasPendingMqttCommand()) {
      Serial.println("[CLOCK] Interrupted by MQTT");
      clockVisible = false;
      return true;
    }

    // Changement de theme si random. Le garde-fou sur endMs evite qu'une
    // rotation se declenche dans les derniers instants de la session: comme
    // themeStartMs et clockStartMs demarrent quasi en meme temps, sans lui
    // la bannière de nom (800ms) se declenchait juste avant la sortie du
    // clock, donnant l'impression a tort d'un nom affiche "a la sortie".
    if (!previewMode && clockTheme == -1 && (millis() - themeStartMs) >= ((unsigned long)clockDuration * 1000UL)
        && (long)(endMs - millis()) > 800) {
      int prevTheme = currentTheme;
      do { currentTheme = random(0, RETRO_THEME_COUNT); } while (currentTheme == prevTheme && RETRO_THEME_COUNT > 1);
      themeStartMs = millis();

      // Afficher le nouveau nom
      display->fillRect(0, 0, 128, 32, 0);
      display->setTextSize(1);
      display->setTextColor(display->color565(255,255,255));
      int tx2 = (128 - strlen(retroThemeNames[currentTheme]) * 6) / 2;
      if (tx2 < 0) tx2 = 0;
      display->setCursor(tx2, 12);
      display->print(retroThemeNames[currentTheme]);
      {
        unsigned long nameEnd = millis() + 800UL;
        while (millis() < nameEnd) {
          yield();
          if (!previewMode && g_sdOpInProgress) { // v73 fix, voir plus haut
            clockVisible = false;
            return true;
          }
          if (hasPendingMqttCommand()) {
            clockVisible = false;
            return true;
          }
          delay(1);
        }
      }
      display->clearScreen();
    }

    int h, m, s;
    getClockTime(h, m, s);

    // Elapsed time since THIS theme started being shown (not the device's
    // global uptime) — lets a theme play a one-time "opening" sequence
    // (e.g. Pac-Man: he appears alone, then the ghosts join the chase)
    // exactly once per display, then loop normally for the rest of it.
    drawRetroClockTheme(currentTheme, h, m, s, millis() - themeStartMs);

    delay(10);
    if (hasPendingMqttCommand()) break;
  }

  clockVisible = false;
  display->clearScreen();
  Serial.println("[CLOCK] End display");
  return true;
}

// v104 -- Sous-systeme overlay (score/game_info/achievement) RETIRE
// entierement (test empirique : isoler si le CODE du port hi-score, meme
// desactive/inutilise, contribue a l'instabilite MQTT observee cette
// session -- voir memoire projet). Portait ~500 lignes (enum OverlayType,
// endOverlay()/drawOverlayTextShadowed()/startScoreOverlay()/
// startAchievementOverlay()/startGameInfoOverlay()/advanceOverlay()).

void setup()
{
  brownout_ll_bod_enable(false);     // Desactive BOD (evite reboot intempestifs)
  brownout_ll_intr_enable(false);    // Desactive IRQ BOD
  brownout_ll_reset_config(false, 0, BROWNOUT_RESET_LEVEL_CHIP);
  Serial.begin(115200); delay(1000);

  // v137 -- heartbeat CPU0, demarre le plus tot possible (voir commentaire
  // pres de heartbeatTask()) pour couvrir toute la sequence de boot, y
  // compris les tout premiers subscribe() juste apres connect().
  xTaskCreatePinnedToCore(heartbeatTask, "heartbeat", 1536, nullptr, 1, nullptr, 0);

  // v99 -- BUG REEL confirme sur materiel (hash ELF verifie + addr2line,
  // reproduit 2x d'affilee) : crash COMPLETEMENT DIFFERENT de tous les autres
  // de cette session -- pas dans notre code du tout, mais DANS le driver
  // WiFi d'ESP-IDF lui-meme : timer_task() (tache interne esp_timer) ->
  // ieee80211_timer_process()/pp_timer_process() -> wifi_log() (le driver
  // WiFi tente d'ecrire un log DIAGNOSTIQUE INTERNE, ex. avertissement bas
  // niveau) -> esp_log_write() -> ecriture console/UART -> le verrou
  // recursif de stdio doit etre initialise a la premiere utilisation
  // (lock_init_generic()) -> echec (heap trop bas au meme plateau que le
  // reste de cette session, free=5196-5228 observe juste avant) -> abort().
  // Aucun try/catch possible ici (code C d'ESP-IDF, pas d'exceptions C++).
  // Fix : faire taire les logs internes du driver WiFi AVANT toute
  // initialisation WiFi -- si wifi_log() ne tente jamais d'ecrire (filtre
  // par niveau AVANT meme de formater le message), ce chemin de code entier
  // (donc ce crash) n'est plus jamais emprunte. Pratique standard/
  // recommandee en production ESP32 (verbosite driver reduite), aucun risque
  // fonctionnel -- desactive uniquement les logs internes du tag "wifi",
  // pas nos propres Serial.println().
  esp_log_level_set("wifi", ESP_LOG_NONE);

  // v76 -- log de la cause du dernier reset (demande utilisateur, suite a un
  // crash a distance non explique : log serie termine en texte UART
  // corrompu juste avant un "rst:0x1 (POWERON_RESET)" generique du
  // bootloader ROM -- signature typique d'un brownout, MAIS le BOD materiel
  // est desactive juste au-dessus (voir commentaire "evite reboot
  // intempestifs"), donc aucun message clair ne le confirmait). Purement
  // diagnostique -- ne change AUCUN comportement, juste un Serial.println()
  // suppelmentaire au boot. esp_reset_reason() lit un registre RTC distinct
  // du BOD materiel desactive ci-dessus : reste capable de rapporter
  // ESP_RST_PANIC/ESP_RST_TASK_WDT/ESP_RST_INT_WDT/ESP_RST_BROWNOUT dans les
  // cas ou le SDK les detecte par un autre chemin que le BOD bas niveau --
  // meme desactive, si un brownout est assez severe pour etre vu par un
  // autre capteur de tension interne, ce sera visible ici. Si le prochain
  // crash reaffiche encore ESP_RST_POWERON malgre ce log, ce sera la
  // confirmation que le brownout se produit "sous" tout ce que l'ESP32 peut
  // lui-meme observer (cas materiel pur, hors de portee logicielle).
  {
    esp_reset_reason_t rr = esp_reset_reason();
    const char *rrName = "UNKNOWN";
    switch (rr) {
      case ESP_RST_POWERON:   rrName = "POWERON (alimentation/reset externe)"; break;
      case ESP_RST_EXT:       rrName = "EXT (broche reset externe)"; break;
      case ESP_RST_SW:        rrName = "SW (ESP.restart())"; break;
      case ESP_RST_PANIC:     rrName = "PANIC (exception logicielle)"; break;
      case ESP_RST_INT_WDT:   rrName = "INT_WDT (watchdog interruption)"; break;
      case ESP_RST_TASK_WDT:  rrName = "TASK_WDT (watchdog tache -- boucle bloquee)"; break;
      case ESP_RST_WDT:       rrName = "WDT (autre watchdog)"; break;
      case ESP_RST_DEEPSLEEP: rrName = "DEEPSLEEP"; break;
      case ESP_RST_BROWNOUT:  rrName = "BROWNOUT (sous-tension detectee)"; break;
      case ESP_RST_SDIO:      rrName = "SDIO"; break;
      default: break;
    }
    Serial.printf("[BOOT] cause du dernier reset : %s (code=%d)\n", rrName, (int)rr);
  }

  // NVS doit etre explicitement (re)initialisee: apres un flash du merged.bin
  // (bootloader+partitions+app en un bloc), la zone NVS est ecrasee en 0xFF brut
  // par esptool merge_bin (comblement du trou entre partitions.bin et boot_app0.bin).
  // Sans ce nvs_flash_init() + erase de secours, esp_wifi peut echouer a se
  // connecter (NVS non formatee) puis planter (abort() dans lock_init_generic,
  // heap bas) lors du fallback WiFi.mode(WIFI_AP).
  {
    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      nvs_flash_erase();
      nvsErr = nvs_flash_init();
    }
    Serial.printf("[NVS] init: %s\n", esp_err_to_name(nvsErr));
  }

  // Vrai random seed: analogRead(A0) non connecte donne du bruit thermique
  randomSeed(analogRead(A0) * 12345L + micros());
  // Melanger le generateur
  for (int i = 0; i < 10; i++) random(100);
  

  // Heap allocations to reduce BSS (ESP32 DRAM linker limit on .dram0.bss)
  if (!sysCacheKeys)
  {
    sysCacheKeys = (char (*)[32])malloc(sizeof(char[32]) * SYS_CACHE_MAX);
    sysCacheVals = (char*)malloc(SYS_CACHE_MAX);
    sysCacheSlowVals = (char*)malloc(SYS_CACHE_MAX);
    // v137 -- BUCKET_CACHE_MAX (< SYS_CACHE_MAX) : 2592 octets au lieu de
    // 8100, voir changelog v137 en tete de fichier.
    sysCachePerLetterVals = (char (*)[BUCKET_COUNT])malloc(sizeof(char[BUCKET_COUNT]) * BUCKET_CACHE_MAX);
    gamesIdx = (GamesSysIdx*)malloc(sizeof(GamesSysIdx) * GAMES_IDX_MAX);
  }

  if (!sysCacheKeys || !sysCacheVals || !sysCacheSlowVals || !sysCachePerLetterVals || !gamesIdx)
  {
    Serial.println("[MEM] heap alloc failed - halting");
    while (1) { delay(100); yield(); }
  }

  HUB75_I2S_CFG::i2s_pins pins={R1_PIN,G1_PIN,B1_PIN,R2_PIN,G2_PIN,B2_PIN,A_PIN,B_PIN,C_PIN,D_PIN,E_PIN,LAT_PIN,OE_PIN,CLK_PIN};
  HUB75_I2S_CFG mxconfig(PANEL_RES_X,PANEL_RES_Y,PANEL_CHAIN,pins);
  mxconfig.latch_blanking=4; mxconfig.i2sspeed=HUB75_I2S_CFG::HZ_10M;
  mxconfig.min_refresh_rate=60; mxconfig.clkphase=false; mxconfig.double_buff=false;

  display=new MatrixPanel_I2S_DMA(mxconfig);
  display->begin(); display->setBrightness8(screenBrightness); display->clearScreen();

  spiSD.begin(VSPI_SCLK,VSPI_MISO,VSPI_MOSI,SD_CS_PIN);
  if(!SD.begin(SD_CS_PIN,spiSD)){
    showMessage("SD ERROR","NO CARD",display->color565(255,0,0));
    while(1){delay(100);yield();}
  }

  // Partie C (plan cache_master_gifs) -- renomme l'etiquette de volume FAT
  // en "RecalBoxDMD" si elle ne correspond pas deja (carte deplacee
  // frequemment entre le DMD et un PC pour inspection : une etiquette
  // reconnaissable facilite son identification parmi d'autres lecteurs
  // amovibles). f_getlabel()/f_setlabel() (API FatFs bas niveau, deja
  // compilees dans ce core ESP32 -- CONFIG_FATFS_USE_LABEL=y) operent sur
  // le chemin FatFs "0:", distinct du chemin VFS "/sdcard" utilise par
  // SD.begin() -- le volume est deja monte a ce point, aucun demontage/
  // remontage necessaire. Limite FAT classique : 11 caracteres exactement
  // ("RecalBox_DMD", 12, ne rentre pas -- "RecalBoxDMD" retenu, coherent
  // avec le nom de fichier de l'outil PC RecalBoxDMD_tool.py, lui non plus
  // sans underscore entre "Box" et "DMD"). Non bloquant : une erreur
  // quelconque (carte protegee en ecriture, etc.) est juste loguee, ne doit
  // jamais retarder/interrompre le boot.
  {
    char label[34];
    FRESULT flr = f_getlabel("0:", label, NULL);
    String current = (flr == FR_OK) ? String(label) : String("");
    current.trim();
    current.toUpperCase();
    if (current != "RECALBOXDMD") {
      FRESULT fsr = f_setlabel("0:RecalBoxDMD");
      if (fsr == FR_OK) {
        Serial.println("[SD] Etiquette renommee: " + current + " -> RecalBoxDMD");
      } else {
        Serial.println("[SD] Echec renommage etiquette (code FatFs " + String((int)fsr) + "), etiquette actuelle: " + current);
      }
    } else {
      Serial.println("[SD] Etiquette deja correcte (RecalBoxDMD)");
    }
  }

  gif.begin(LITTLE_ENDIAN_PIXELS);

  {
    File cfg=SD.open("/config.ini");
    if(cfg){
      while(cfg.available()){
        String line=cfg.readStringUntil('\n');line.trim();
        if(line.startsWith("info=")||line.startsWith("info =")){
          String val=line.substring(line.indexOf('=')+1);val.trim();
          showInfo=(val!="0");
        }
        else if(line.startsWith("brightness=")){
          int v=line.substring(line.indexOf('=')+1).toInt();
          if(v>=0&&v<=100)screenBrightness=map(v,0,100,0,255);
        }
        else if(line.startsWith("CLOCK_ENABLED="))clockEnabled=(line.substring(line.indexOf('=')+1).toInt()!=0);
        // v104 -- parsing SCORE_ENABLED/SCORE_INTERVAL_SEC/SCORE_DURATION/
        // GAME_INFO_ENABLED/ACHIEVEMENT_ENABLED/GAME_INFO_EVERY_N retire
        // (hi-score port supprime, test empirique rc=-4).
else if(line.startsWith("CLOCK_THEME=")){int s=line.substring(line.indexOf('=')+1).toInt();if(s>=-1&&s<RETRO_THEME_COUNT)clockTheme=s;}
        else if(line.startsWith("CLOCK_COLOR=")){
          String v=line.substring(line.indexOf('=')+1);v.trim();
          if(v.startsWith("#")&&v.length()==7){
            unsigned long cv=strtoul(v.substring(1).c_str(),NULL,16);
            clockNeonR=(cv>>16)&0xFF; clockNeonG=(cv>>8)&0xFF; clockNeonB=cv&0xFF;
            clockNeonCustomColor=true;
          } else {
            clockNeonCustomColor=false;
          }
        }
        else if(line.startsWith("CLOCK_INTERVAL="))clockIntervalGifs=line.substring(line.indexOf('=')+1).toInt();
        else if(line.startsWith("CLOCK_INTERVAL_MIN="))clockIntervalMin=line.substring(line.indexOf('=')+1).toInt();
        else if(line.startsWith("CLOCK_DURATION="))clockDuration=line.substring(line.indexOf('=')+1).toInt();
        else if(line.startsWith("TZ=")){clockTimeZone=line.substring(line.indexOf('=')+1);clockTimeZone.trim();}
        // Lu ICI (v45, demande explicite utilisateur), AVANT loadConfig() --
        // les 3 caches ci-dessous (systemes, default.raw565, jeux) ne servent
        // qu'a l'affichage GIF/MQTT (icones systeme/jeu, image de secours) --
        // JAMAIS utilises pendant le mode config (MQTT bloque par
        // g_sdOpInProgress, aucun GIF ouvert). Sur un boot "reboot cible" dont
        // le seul but est de liberer le heap au plus vite pour un upload, les
        // charger coute du temps ET de la RAM pour rien. loadConfig() (plus
        // bas) relit aussi cette cle -- lecture redondante mais harmless,
        // aucune ecriture entre les deux.
        else if(line.startsWith("force_config_boot="))g_skipPlaylistForConfig=(line.substring(line.indexOf('=')+1).toInt()!=0);
      }
      cfg.close();
    }
  }

  showSplashScreen();  // Toujours affichÃ©, indÃ©pendamment de info=
  

  // Charge le cache systÃ¨mes (systems_cache.dat). Si absent, on ne rescanner
  // que si l'utilisateur a info=1. Le script Python Ã©crit dÃ©jÃ  ce fichier.
  if (!g_skipPlaylistForConfig) {
  if(!loadSysDefaultCache()){
    if(showInfo)showMessage("MARQUEE","Indexation...",display->color565(100,100,255));
    buildSysDefaultCache();
  } else {
    Serial.println("[CACHE] utilise .dat existant, pas de scan recusrsif");
  }

  // Precharger le fallback default.raw565 en RAM des le setup
  // (pas au 1er appel de fallback). 8KB, heap dispo.
  ensureDefaultRaw565Cached();
  if (defaultRaw565Cached) Serial.println("[CACHE] default.raw565 en RAM");
  else                     Serial.println("[CACHE] default.raw565 absent");
  sysCacheLoadAttempted = true;
  } else {
    Serial.println("[CACHE] reboot cible mode config -- caches systemes/default.raw565 sautes");
  }

  loadConfig();

  // Pour les systÃ¨mes flags 'L' (lents), games_cache.bin est court-circuitÃ©
  // dans findInGamesCache(). Inutile de le charger pour ces systÃ¨mes.
  // On le charge quand mÃªme pour les systÃ¨mes 'N' qui en ont besoin.
  if (!g_skipPlaylistForConfig) {
  if(!loadGamesIndex())
    Serial.println("[GCACHE] "+gamesCacheFile+" absent");
  else
    Serial.println("[GCACHE] OK - "+String(gamesIdxCount)+" systemes");
  gamesCacheLoadAttempted = true;
  } else {
    Serial.println("[GCACHE] reboot cible mode config -- cache jeux saute");
  }
  Serial.println("[BOOT] apres chargement caches, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));

  // Boot silencieux (info=0): le titre (splash) reste affiche, le sablier coin
  // haut-droit se dessine par-dessus jusqu'a la fin du boot (WiFi/NTP).
  if(!showInfo) bootHourglassTick();

  
  // BT avant WiFi: si le Bluetooth est desactive, esp_bt_mem_release() libere
  // ~60 Ko de DRAM reserves au controleur BT. Fait apres le WiFi, ce heap
  // manquait pendant la connexion (ESP_ERR_NO_MEM dans esp_timer_create,
  // ppTask/wifi_sta_connect_internal) et provoquait un abort().
  setupBluetoothFromConfig();

  setupWiFiFromConfig();
  Serial.println("[BOOT] apres setupWiFiFromConfig, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
  // Attente connexion WiFi puis synchro NTP (ecran masque si info=0, cf initNTP())
  if (showInfo) {
    display->clearScreen();
    display->setTextWrap(false); display->setTextSize(1);
    display->setTextColor(display->color565(200, 200, 200));
    display->setCursor(1, 8); display->print("Brightness: ");
    display->setCursor(1, 20); display->print(String(screenBrightness * 100 / 255) + "%");
    delay(800);
  }
  
  initNTP();
  Serial.println("[BOOT] apres initNTP, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));

  // Entree en mode web config : condition unique (bug corrige 2026-08-05,
  // demande utilisateur -- logique cible en 3 etapes) combinant les 3
  // raisons reelles d'y entrer, la ou elles etaient auparavant eparpillees
  // et incompletes (g_firstBoot seul court-circuitait TOUT le reste via
  // goto, empechant le test playlistName de jamais s'executer tant qu'il
  // etait vrai ; recalboxIP n'etait lui jamais teste comme condition
  // d'entree nulle part dans ce fichier).
  bool needWebConfigMode = (playlistName.length()==0) || (recalboxIP.length()==0) || g_firstBoot;

  // Invite l'utilisateur a ouvrir la page web. Message choisi selon
  // l'etat REEL de connexion (bug corrige 2026-08-05) : ce bloc s'execute
  // apres CHAQUE appel a setupWiFiFromConfig() ci-dessus, donc aussi bien
  // juste apres un repli AP (WiFi.status()!=WL_CONNECTED, deja invite a
  // rejoindre RecalBox-DMD-Config par setupWiFiFromConfig() lui-meme --
  // meme message ici, coherent) QUE juste apres une VRAIE connexion STA
  // reussie (playlist/IP Recalbox manquantes, ou 2e phase du 1er
  // demarrage) -- dans ce dernier cas, "Connectez-vous au WiFi
  // RecalBox-DMD-Config" etait un contresens (ce reseau AP n'existe plus
  // a ce stade, le DMD est deja sur le reseau reel) : utilise le meme
  // message que les autres ecrans "page de configuration" du fichier.
  if (needWebConfigMode) {
    String ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
    if (ip == "0.0.0.0") ip = "192.168.4.1";
    g_sdOpMsg = (WiFi.status() == WL_CONNECTED) ? trConfigPageMsg() : trConnectWifiMsg();
    g_sdOpSubMsg = trOpenUrl(ip);
    g_sdOpSubMsgColor = 0x07E0;
    g_sdOpInProgress = true;
    currentMode = MODE_CONFIG;
    g_configDmdDirty = true;
    Serial.println("[BOOT] Mode web config - ouvrir http://" + ip);
  }

  mqttCmdMutex=xSemaphoreCreateMutex();
  pendingCmd=MqttCommand(MqttCommand::CMD_NONE,"");
  wifiResetMutex=xSemaphoreCreateMutex(); // v144 -- voir sa declaration
  // plGenStatusMutex/sdAccessMutex retires (2026-08-10) : playlistGenStep()
  // tourne exclusivement dans loop(), plus d'acces concurrent a proteger.
  if (needWebConfigMode) {
    goto start_mqtt_task;
  }

  if (g_forceApRecovery) {
    // Mode secours WiFi (marquee/cmd/wifi_recovery) : setupWiFiFromConfig()
    // a deja positionne l'ecran (g_sdOpMsg/currentMode=MODE_CONFIG) -- sauter
    // le chargement/affichage de la playlist qui l'ecraserait sinon (bug
    // trouve en test reel le 2026-07-21 : le boot continuait tout droit dans
    // showPlaylistInfoScreen()/openNextGif() apres le mode secours, qui
    // remettait currentMode=MODE_PLAYLIST avant meme que l'utilisateur ne
    // voie l'ecran "Mode secours WiFi").
    goto start_mqtt_task;
  }

  if (g_skipPlaylistForConfig) {
    // Reboot demande par triggerWebConfigMode() (web_config.h) pour repartir
    // en mode config avec un maximum de heap disponible -- ne JAMAIS lancer
    // la playlist/ouvrir de GIF sur ce boot precis (chaque GIF ouvert perd
    // durablement quelques Ko de heap via le buffer setvbuf(4096) alloue par
    // SD.open(), jamais recupere avant reboot -- et meme mettre en pause UN
    // SEUL GIF deja ouvert fragmente fortement le heap, confirme en test reel
    // 2026-07-27 puis reconfirme 2026-08-02). Flag consomme immediatement
    // (config.ini remis a "0") pour qu'un reboot normal ulterieur
    // ("Redemarrer") reparte bien en boot playlist standard, pas en boucle
    // sur ce chemin.
    writeConfigFlag("force_config_boot", "0");
    // Charge quand meme l'index playlist (gifCount), SANS jamais ouvrir de
    // GIF ni dessiner l'ecran playlist (showPlaylistInfoScreen()) -- lecture
    // seule d'un fichier .idx deja existant, cout heap negligeable (~7ms
    // mesures en conditions reelles). Sans ca, "Reprendre DMD" (resumePlaylist(),
    // qui ne fait rien si gifCount==0) laissait un ecran noir en sortie de
    // config -- gifCount ne serait sinon jamais initialise sur ce chemin.
    // Si le cache playlist est perime (signature differente), gifCount reste
    // a 0 pour ce boot precis (limite acceptee : cas rare, un vrai reboot
    // normal ulterieur reconstruira le cache comme d'habitude).
    if (playlistName.length() > 0) {
      uint32_t curSig = computeFileHash(playlistSourcePath);
      uint32_t savSig = readSavedSignature();
      if (curSig && curSig == savSig) {
        if (idxFileHandle) idxFileHandle.close();
        idxFileHandle = SD.open(playlistIdxPath, FILE_READ);
        if (idxFileHandle) {
          size_t idxSize = idxFileHandle.size();
          gifCount = (idxSize >= 4) ? (int)(idxSize / 4) : 0;
          if (!playlistRandom) {
            if (seqPlaylistFile) seqPlaylistFile.close();
            seqPlaylistFile = SD.open(playlistCachePath, FILE_READ);
            playIndex = 0;
          }
        }
      }
    }
    {
      String ip = WiFi.localIP().toString();
      g_sdOpMsg = trConfigPageMsg();
      g_sdOpSubMsg = trOpenUrl(ip);
      g_sdOpSubMsgColor = 0x07E0;
      g_sdOpInProgress = true;
      currentMode = MODE_CONFIG;
      g_configDmdDirty = true;
      Serial.println("[BOOT] Reboot cible mode config (heap max) -> http://" + ip + " gifCount=" + String(gifCount));
    }
    goto start_mqtt_task;
  }

  // Bloc "if(playlistName.length()==0)" retire (bug corrige 2026-08-05) :
  // entierement redondant avec needWebConfigMode ci-dessus, qui couvre
  // deja ce cas (et goto start_mqtt_task AVANT ce point si vrai) --
  // playlistName ne change jamais entre les deux, ce bloc ne pouvait
  // donc plus jamais s'executer.

  {
    Serial.println("[PLAYLIST] sig compute start t=" + String(millis()));
    uint32_t curSig=computeFileHash(playlistSourcePath);
    Serial.println("[PLAYLIST] sig compute done t=" + String(millis()) + " curSig=" + String(curSig));

    Serial.println("[PLAYLIST] saved signature start t=" + String(millis()));
    uint32_t savSig=readSavedSignature();
    Serial.println("[PLAYLIST] saved signature done t=" + String(millis()) + " savSig=" + String(savSig));

    bool haveCache = false;
    if (curSig && curSig==savSig)
    {
      File cacheF = SD.open(playlistCachePath, FILE_READ);
      File idxF   = SD.open(playlistIdxPath,   FILE_READ);
      haveCache   = (cacheF && idxF);

      if (cacheF) cacheF.close();
      if (idxF)   idxF.close();
    }
    Serial.println("[PLAYLIST] cacheCheck haveCache=" + String(haveCache) + " t=" + String(millis()));

    if(!haveCache){
      Serial.println("[PLAYLIST] rebuildPlaylistCache start t=" + String(millis()));
      int rebuildN = rebuildPlaylistCache();
      Serial.println("[PLAYLIST] rebuildPlaylistCache done n=" + String(rebuildN) + " t=" + String(millis()));
      Serial.println("[PLAYLIST] writeSignature start t=" + String(millis()));
      (void)writeSignature(curSig);
      Serial.println("[PLAYLIST] writeSignature done t=" + String(millis()));

      Serial.println("[PLAYLIST] buildOffsetIndex start t=" + String(millis()));
      gifCount=buildOffsetIndex();
      Serial.println("[PLAYLIST] buildOffsetIndex done gifCount=" + String(gifCount) + " t=" + String(millis()));
    }
    else
    {
      // Index deja present: on ne le reconstruit pas.
      Serial.println("[PLAYLIST] use cached idx start t=" + String(millis()));
      if(idxFileHandle) idxFileHandle.close();
      idxFileHandle = SD.open(playlistIdxPath, FILE_READ);
      if(!idxFileHandle){
        Serial.println("[PLAYLIST] cached idx open failed -> rebuild");
        Serial.println("[PLAYLIST] buildOffsetIndex start t=" + String(millis()));
        gifCount=buildOffsetIndex();
        Serial.println("[PLAYLIST] buildOffsetIndex done gifCount=" + String(gifCount) + " t=" + String(millis()));
      } else {
        size_t idxSize = idxFileHandle.size();
        gifCount = (idxSize >= 4) ? (int)(idxSize / 4) : 0;
        if(!playlistRandom){
          if(seqPlaylistFile) seqPlaylistFile.close();
          seqPlaylistFile = SD.open(playlistCachePath, FILE_READ);
          playIndex = 0;
        }
        Serial.println("[PLAYLIST] cached idx ok gifCount=" + String(gifCount) + " t=" + String(millis()));
      }
    }

    Serial.println("[PLAYLIST] showPlaylistInfoScreen start t=" + String(millis()));
    showPlaylistInfoScreen(); delay(1300);
    Serial.println("[PLAYLIST] showPlaylistInfoScreen done t=" + String(millis()));
    Serial.println("[BOOT] apres showPlaylistInfoScreen, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
    if(gifCount==0){
      String ip = WiFi.localIP().toString();
      if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
      if (ip == "0.0.0.0") ip = "192.168.4.1";
      g_sdOpMsg = trConfigPageMsg();
      g_sdOpSubMsg = trOpenUrl(ip);
      g_sdOpSubMsgColor = 0x07E0;
      g_sdOpInProgress = true;
      currentMode = MODE_CONFIG;
      g_configDmdDirty = true;
      Serial.println("[BOOT] Playlist empty -> config mode sur http://" + ip);
      goto start_mqtt_task;
    }
    g_playlistStartedThisBoot = true;
    playIndex=0;lastRandomIndex=-1;currentMode=MODE_PLAYLIST;openNextGif();
    Serial.println("[BOOT] apres 1er openNextGif, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
  }

start_mqtt_task:
  // v107 (2026-08-18) -- TEST EMPIRIQUE : mqttTask() deplace temporairement
  // du coeur 0 vers le coeur 1, pour tester l'hypothese d'une contention
  // avec loop() (LoopCore=0) comme cause du rc=-4/timeout observe toute la
  // journee (log broker correle, voir memoire projet -- CONNACK envoye
  // mais client timeout ~24-30s plus tard faute de PINGREQ a temps).
  // RESULTAT : hypothese REFUTEE sur materiel -- tempete demarree ~90s
  // apres boot avec mqttTask sur coeur 1, meme signature, aucune
  // amelioration (voir memoire projet pour le detail). Revert au coeur 0
  // (etat historique, identique a master -- voir memoire projet) faute de
  // benefice demontre.
  if(wifiEnabled&&recalboxIP.length()>0)
    xTaskCreatePinnedToCore(mqttTask,"mqttTask",4096,NULL,1,&mqttTaskHandle,0);

  // Interface web de configuration
  if (wifiEnabled) setupWebConfig();
  Serial.println("[BOOT] apres setupWebConfig, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
}

// --------------------------------------------------
// Loop
// --------------------------------------------------
void loop()
{
  // v111 -- INSTRUMENTATION DIAGNOSTIC TEMPORAIRE (a retirer une fois la
  // cause confirmee) : detecteur GENERIQUE de changement de currentMode --
  // plus de 40 sites d'ecriture differents dans ce fichier, impossible a
  // tous instrumenter individuellement. Capture ICI, au tout debut de
  // loop() (avant tout traitement de cette iteration), la valeur de
  // currentMode telle qu'elle etait a la FIN de l'iteration precedente --
  // n'importe quelle transition, peu importe le site qui l'a causee, est
  // ainsi loguee. Objectif : confirmer/infirmer qu'une sortie prematuree de
  // MODE_SCORE (bug "flash fugace" rapporte par l'utilisateur, ni le
  // diagnostic cmd= existant ni le log "score expire" ne l'ont captee)
  // passe bien par une ecriture directe de currentMode plutot que par un
  // mecanisme non encore identifie.
  {
    static DisplayMode s_lastLoggedMode = MODE_PLAYLIST;
    static bool s_first = true;
    if (s_first) { s_lastLoggedMode = currentMode; s_first = false; }
    else if (currentMode != s_lastLoggedMode) {
      Serial.println("[DIAG] currentMode " + String((int)s_lastLoggedMode) + " -> " + String((int)currentMode)
                     + " t=" + String(millis()));
      s_lastLoggedMode = currentMode;
    }
  }
  // v87 (2026-08-17) -- INSTRUMENTATION DIAGNOSTIC TEMPORAIRE : episode reel
  // observe sur materiel -- rafale de std::bad_alloc rattrapees (chunk 1,
  // pre-chargement du GIF suivant pendant une rotation playlist normale,
  // heap coince a maxalloc=4596) suivie de 65s de silence TOTAL avant
  // qu'une commande MQTT deja en attente (posee des la connexion reussie)
  // ne soit enfin traitee -- alors que processPendingMqttCommand() est
  // censee etre appelee sans aucune condition a CHAQUE iteration (voir
  // juste en dessous). Hypothese a confirmer/infirmer : le cout du
  // throw/catch C++ lui-meme (chunk 1, try/catch autour de SD.open()) sous
  // heap deja critique pourrait etre bien plus eleve que prevu sur ESP32.
  // Heartbeat INCONDITIONNEL (2s, ne depend d'aucun etat) pour savoir OU
  // loop() se trouve reellement si ce silence se reproduit -- si meme ce
  // heartbeat s'arrete, loop() est vraiment bloque (pas juste "coince en
  // MODE_BLACK sans rien logger"). A retirer une fois la cause confirmee.
  {
    static unsigned long s_loopDiagLastMs = 0;
    if (millis() - s_loopDiagLastMs >= 2000)
    {
      s_loopDiagLastMs = millis();
      // v109 -- instrumentation diagnostic (voir memoire projet worktree
      // dev-mame-score-mqtt-bridge : corruption memoire silencieuse
      // suspectee de longue date, jamais localisee -- hypothese testee ici :
      // depassement de PILE de loopTask, pas de heap. uxTaskGetStackHighWaterMark()
      // renvoie le MINIMUM historique d'octets de pile libres jamais atteint
      // depuis le demarrage de la tache (valeur qui ne fait QUE decroitre,
      // jamais remonter) -- inutile de capturer pile au bon moment, ce log
      // periodique (2s, deja existant) suffit a reveler le pire cas atteint
      // au fil d'une session, meme si le pic de pile lui-meme ne dure que
      // quelques ms (ex. pendant la chaine d'appels profonde drawRaw565()->
      // fs::FS::open()->VFS->FatFs->SPI vue dans le crash TASK_WDT du
      // 2026-08-18). stackMin exprime en OCTETS (uxTaskGetStackHighWaterMark
      // renvoie des words sur ESP32/FreeRTOS -- xPortGetFreeHeapSize non
      // utilise ici, conversion *4 le rend directement comparable a la
      // taille de pile allouee en octets, ex. 8192 par defaut pour loopTask).
      uint32_t stackMinWords = uxTaskGetStackHighWaterMark(nullptr);
      Serial.println("[LOOPDIAG] t=" + String(millis())
                     + " mode=" + String((int)currentMode)
                     + " gifOpened=" + String(gifOpened)
                     + " gifRawPackMode=" + String(gifRawPackMode)
                     + " requestNextGif=" + String(requestNextGif)
                     + " nextGifPathLen=" + String(nextGifPath.length())
                     + " free=" + String(ESP.getFreeHeap())
                     + " maxalloc=" + String(ESP.getMaxAllocHeap())
                     // v143 -- minFreeHeap= (ESP.getMinFreeHeap(), plancher
                     // historique jamais remis a zero) + connectAttempts=
                     // (g_totalConnectAttempts) : suivi continu (pas
                     // seulement au moment des tentatives MQTT) pour
                     // corroborer une derive lente vs un declenchement lie
                     // au nombre de cycles connect()/subscribe() ecoules.
                     + " minFreeHeap=" + String(ESP.getMinFreeHeap())
                     + " connectAttempts=" + String(g_totalConnectAttempts)
                     + " pendingType=" + String((int)pendingCmd.type)
                     + " stackMinBytes=" + String(stackMinWords * 4));
    }
  }
  // processPendingMqttCommand() APPELE EN PREMIER (2026-08-09, v62) --
  // AVANT handleWebConfig() -- voir changelog v62 : webServer->handleClient()
  // et mqttClient.loop() (mqttTask()) passent tous deux par la meme couche
  // socket LWIP bas niveau ; si handleWebConfig() se bloque sur un verrou
  // LWIP retenu ailleurs (meme famille que le deadlock mqttTask/LWIP deja
  // documente), TOUT le reste de cette iteration de loop() -- y compris
  // processPendingMqttCommand() -- restait bloque avec lui, laissant
  // pendingCmd (un seul slot) se faire ecraser silencieusement par chaque
  // nouveau message MQTT recu entre-temps (rien n'etait jamais traite ni
  // logue). Ne resout pas la cause racine (verrou hors de portee du code
  // applicatif) mais garantit que la derniere commande en attente AU DEBUT
  // de l'iteration est bien consommee avant tout risque de blocage sur
  // handleWebConfig().
  processPendingMqttCommand();
  // v1 -- piste UDP (voir TRANSPORT_PLAN_UDP.md) : parsePacket() non
  // bloquant, meme raisonnement de placement que processPendingMqttCommand()
  // juste au-dessus (consommer/poser pendingCmd tot dans l'iteration, avant
  // tout risque de blocage plus bas type handleWebConfig()).
  handleUdpCommand();
  // playlistGenStep() (2026-08-10, RETOUR de cette architecture -- voir
  // changelog v67) : avance la generation de playlist d'un pas borne, cout
  // quasi nul quand aucune generation n'est active (un seul if). Appelee
  // ici, avant handleWebConfig(), meme position qu'a l'origine (avant le
  // 2026-07-30).
  playlistGenStep();
  handleWebConfig(); maintainWiFi(); maintainApRecovery();
  // Alerte "No wifi, No Recalbox" (2026-08-05, demande utilisateur) --
  // repli ici pour le cas ou la demande (g_noWifiRecalboxPending, posee
  // par mqttTask()) survient alors qu'aucune playlist n'est en cours de
  // lecture (MODE_BLACK, aucun GIF charge, etc.) : rien a proteger d'une
  // coupure en plein milieu dans ce cas, applique immediatement. Si une
  // playlist tourne (MODE_PLAYLIST), c'est plutot le case MODE_PLAYLIST
  // ci-dessous (entre deux GIFs) qui consomme cette demande.
  if (g_noWifiRecalboxPending && currentMode != MODE_PLAYLIST) {
    showNoWifiRecalboxAlert();
  }
  // Idem pour "RecalBox non connectee" (2026-08-05) -- meme repli hors
  // MODE_PLAYLIST, voir commentaire ci-dessus.
  if (g_recalboxDisconnectedPending && currentMode != MODE_PLAYLIST) {
    showRecalboxDisconnectedAlert();
  }
  // Application differee d'un "default" recu trop tot (v49, 2026-08-03,
  // demande explicite utilisateur) : voir CMD_DEFAULT/declaration de
  // MQTT_WAITING_MIN_DISPLAY_MS pour le detail complet -- ici, on se
  // contente d'appliquer l'action memorisee des que le delai minimum
  // d'affichage de l'ecran "RecalBox connectee" est ecoule.
  if (g_mqttDefaultPendingAfterMinDisplay && millis() >= g_mqttWaitingMinDisplayUntilMs)
  {
    g_mqttDefaultPendingAfterMinDisplay = false;
    g_mqttConnectedScreenUntilMs = 0;
    // Idem pour l'alerte "No wifi, No Recalbox" (2026-08-05) : un vrai
    // contenu MQTT reprend la main, plus besoin d'attendre son
    // expiration ni de laisser une demande en attente perimee.
    g_noWifiRecalboxScreenActive = false;
    g_noWifiRecalboxPending = false;
    g_recalboxDisconnectedScreenActive = false;
    g_recalboxDisconnectedPending = false;
    // v85 (2026-08-17) -- BUG REEL confirme sur materiel (log + observation
    // ecran) : ce chemin appelle resumePlaylist() DIRECTEMENT, en
    // contournant le case CMD_DEFAULT de processPendingMqttCommand() (voir
    // ce case pour le raisonnement complet) -- donc s'il s'applique APRES
    // qu'une vraie partie ait demarre entre-temps (le "default" etait
    // differe depuis l'ecran de connexion, potentiellement plusieurs
    // dizaines de secondes plus tot), l'affichage repassait bien en
    // MODE_PLAYLIST mais g_inGameMarquee restait bloque a true -- etat
    // incoherent (currentMode==MODE_PLAYLIST mais "en jeu" du point de vue
    // de l'alternance hi-score/game_info, qui exige MODE_PNG/MODE_GIF pour
    // se declencher -- ne se declenche donc plus JAMAIS, silencieusement,
    // jusqu'au prochain vrai CMD_STOP/CMD_DEFAULT/CMD_SYSTEM/CMD_INGAME).
    // C'etait la cause du bug "overlay ne s'affiche jamais" (voir memoire
    // projet) -- PAS une corruption memoire.
    // v104 -- g_inGameMarquee retire (hi-score port supprime).
    Serial.println("[MQTT] default differe applique -> reprise playlist");
    resumePlaylist();
  }
  // Clignotement du texte "RecalBox connectee" pendant tout l'affichage
  // (demande utilisateur 2026-07-29) -- sans effet si un vrai media a deja
  // pris la main. Toggle simple ~2 fois/seconde, pas de garde-fou de cout
  // necessaire (juste un redessin de bande + eventuellement 2 print, deja
  // fait a chaque CMD_WAITING_MQTT).
  // v45 (2026-08-03) -- reprise automatique de la playlist par delai fixe
  // RETIREE (demande explicite utilisateur, comportement confirme errone en
  // test reel) : la logique voulue est d'attendre INDEFINIMENT un vrai
  // message MQTT tant que la Recalbox reste connectee -- c'est elle qui
  // decide quand revenir a la playlist (CMD_DEFAULT, pont marquee sur
  // veille/lecture d'un clip), jamais un delai arbitraire cote DMD.
  // g_mqttConnectedScreenUntilMs n'est plus un horodatage d'expiration mais
  // un simple drapeau "ecran d'attente actif" (pose a CMD_WAITING_MQTT,
  // remis a 0 des qu'un vrai contenu prend la main : CMD_DEFAULT/CMD_SYSTEM/
  // CMD_GAME/CMD_STOP) -- utilise uniquement pour piloter ce clignotement.
  if (g_mqttConnectedScreenUntilMs != 0 && currentMode == MODE_PNG && currentPngPath == String(DEFAULT_RAW565_PATH) && !g_sdOpInProgress)
  {
    static unsigned long lastBlinkMs = 0;
    static bool blinkVisible = true;
    if (millis() - lastBlinkMs > 400)
    {
      blinkVisible = !blinkVisible;
      drawRecalboxConnectedOverlay(blinkVisible);
      lastBlinkMs = millis();
    }
  }
  // Clignotement + auto-resolution de l'alerte "No wifi, No Recalbox"
  // (2026-08-05, demande utilisateur) -- bloc jumeau du precedent mais
  // drapeau/duree distincts (voir declaration de g_noWifiRecalboxScreenActive) :
  // ecran TEMPORISE (5s, NO_WIFI_ALERT_DISPLAY_MS), pas d'attente indefinie
  // d'un message externe qui ne viendra jamais tant que le WiFi est down.
  if (g_noWifiRecalboxScreenActive && currentMode == MODE_PNG && currentPngPath == String(DEFAULT_RAW565_PATH) && !g_sdOpInProgress)
  {
    static unsigned long lastNoWifiBlinkMs = 0;
    static bool noWifiBlinkVisible = true;
    if (millis() - lastNoWifiBlinkMs > 400)
    {
      noWifiBlinkVisible = !noWifiBlinkVisible;
      drawNoWifiNoRecalboxOverlay(noWifiBlinkVisible);
      lastNoWifiBlinkMs = millis();
    }
    if (millis() >= g_noWifiRecalboxUntilMs)
    {
      g_noWifiRecalboxScreenActive = false;
      // v104 -- g_inGameMarquee retire (hi-score port supprime).
      Serial.println("[WIFI] No wifi, No Recalbox -- delai ecoule, reprise playlist");
      resumePlaylist();
    }
  }
  // Clignotement + auto-resolution de l'alerte "RecalBox non connectee"
  // (2026-08-05, demande utilisateur) -- bloc jumeau du precedent (WiFi
  // OK mais mqttClient.state()==-2, drapeau/duree distincts).
  if (g_recalboxDisconnectedScreenActive && currentMode == MODE_PNG && currentPngPath == String(DEFAULT_RAW565_PATH) && !g_sdOpInProgress)
  {
    static unsigned long lastRecalboxDiscBlinkMs = 0;
    static bool recalboxDiscBlinkVisible = true;
    if (millis() - lastRecalboxDiscBlinkMs > 400)
    {
      recalboxDiscBlinkVisible = !recalboxDiscBlinkVisible;
      drawRecalboxDisconnectedOverlay(recalboxDiscBlinkVisible);
      lastRecalboxDiscBlinkMs = millis();
    }
    if (millis() >= g_recalboxDisconnectedUntilMs)
    {
      g_recalboxDisconnectedScreenActive = false;
      // v104 -- g_inGameMarquee retire (hi-score port supprime).
      Serial.println("[MQTT] RecalBox non connectee -- delai ecoule, reprise playlist");
      resumePlaylist();
    }
  }
  // Progression de playlistGenTask() affichee sur le DMD (voir
  // PlaylistGenStatus/webDmdOverlayLine2(), web_config.h) -- uniquement si le
  // mode config est DEJA actif (jamais pour l'imposer : la tache elle-meme
  // ne touche jamais gif/display/currentMode -- voir le commentaire complet
  // pres de PlaylistGenStatus, juste avant #include "web_config.h"). Si
  // l'utilisateur a repris le DMD pendant le scan (currentMode != MODE_CONFIG),
  // on ne touche a rien -- la lecture GIF continue sans interference.
  // Throttle 2s, large marge sous les 5000ms d'expiration du message DMD
  // (SD_OP_SUBMSG_EXPIRE_MS).
  {
    static unsigned long lastPlGenDmdMs = 0;
    if (millis() - lastPlGenDmdMs > 2000)
    {
      // plGenStatusMutex retire (2026-08-10) : plus d'acces concurrent
      // possible, tout tourne desormais dans loop().
      bool active = g_plGenStatus.active;
      String dirName = g_plGenStatus.curDirName;
      int gifs = g_plGenStatus.curDirGifs;
      if (active && currentMode == MODE_CONFIG)
      {
        webDmdOverlayLine2(plGenDmdText(dirName, gifs), 0x07E0);
      }
      lastPlGenDmdMs = millis();
    }
  }
  // v104 -- bloc declencheur d'alternance score/game_info (TRIGDIAG inclus)
  // retire entierement (hi-score port supprime, test empirique rc=-4).
  if(requestNextGif&&!g_sdOpInProgress){requestNextGif=false;openNextGif();}
  if(requestReboot) {delay(100);ESP.restart();}

  switch(currentMode)
  {
  case MODE_PLAYLIST:
    if(!gifOpened){display->clearScreen();currentMode=MODE_BLACK;break;}
    {
      int fd=0; bool frameOk=gifPlayFrameCompat(false,&fd);
      if(!frameOk){
        // Alerte "No wifi, No Recalbox" (2026-08-05, demande utilisateur) :
        // le GIF courant vient de se terminer naturellement (frameOk==false)
        // -- point d'insertion volontairement choisi ICI, AVANT openNextGif(),
        // pour ne jamais couper une animation en plein milieu. L'alerte
        // affichee prend la main pour 5s (voir showNoWifiRecalboxAlert()),
        // puis resumePlaylist() (appele automatiquement dans loop() a
        // l'expiration du delai) enchaine sur le GIF suivant normalement --
        // la rotation reprend sans perte de position.
        if (g_noWifiRecalboxPending) {
          showNoWifiRecalboxAlert();
          break;
        }
        // Idem pour "RecalBox non connectee" (2026-08-05) -- meme
        // placement entre deux GIFs.
        if (g_recalboxDisconnectedPending) {
          showRecalboxDisconnectedAlert();
          break;
        }
        if(clockEnabled) clockGifCounter++;
        openNextGif();
        if(clockEnabled && clockIntervalMin <= 0)
        {
          if(clockGifCounter >= clockIntervalGifs)
          {
            clockGifCounter = 0;
            if(!showClock()) { currentMode = MODE_BLACK; break; }
            if(g_sdOpInProgress) { break; }
            if(nextGifFile) { nextGifFile.close(); nextGifFile = File(); }
            break;
          }
        }
        break;
      }
      if(fd<=0)fd=10;
      // v97 -- BUG REEL confirme sur materiel (abort() decode via addr2line,
      // ELF verifie) : MEME cause que le crash deja corrige en v91/v95
      // (getNextGif()->getNextGifRandom()->SD.open() double-echec
      // d'allocation, non rattrapable par le try/catch existant), mais un
      // 3e site d'appel DIFFERENT, jamais protege -- celui-ci dans la boucle
      // de frame MODE_GIF elle-meme (prefetch du GIF suivant pendant la
      // lecture), distinct des 2 deja corriges dans openNextGif(). Meme
      // seuil dedie (PREFETCH_NEXT_GIF_MIN_HEAP=8000, v95) -- pire cas si
      // heap trop bas : prefetch simplement saute, retente a la frame
      // suivante (aucun impact visuel, le GIF en cours continue).
      if(nextGifPath.length()==0 && ESP.getMaxAllocHeap()>=PREFETCH_NEXT_GIF_MIN_HEAP)nextGifPath=getNextGif();
      unsigned long t=millis();
      // v128 -- delay(0)->delay(1), meme raisonnement/meme fix que la boucle
      // jumelle case MODE_GIF (rawpack) plus bas -- voir son commentaire
      // complet. Ce site touche SD moins souvent (prefetch, pas par frame)
      // mais c'est exactement le meme motif, applique par coherence.
      while((long)(millis()-t)<fd){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(1);}
      // Pre-chargement opportuniste (deja optionnel avant : ne fait rien si
      // nextGifFile est deja pris). sdAccessMutex retire (2026-08-10).
      if(nextGifPath.length()>0&&!nextGifFile){
        nextGifFile=SD.open(nextGifPath.c_str());
      }
    }
    break;

  case MODE_GIF:
    if(!gifOpened){display->clearScreen();currentMode=MODE_BLACK;break;}
    {
      int fd=0; bool frameOk=gifPlayFrameCompat(false,&fd);
      if(!frameOk){
        gifResetCompat();
        // v104 -- point de coupure overlay score/game_info/achievement retire
        // (hi-score port supprime, test empirique rc=-4).
        break;
      }
      if(fd<=0)fd=10;
      unsigned long t=millis();
      // v128 -- delay(0) -> delay(1) (retour utilisateur : deconnexions MQTT
      // courtes pendant la veille gameclip, correlees a un rendu MODE_GIF/
      // gifRawPackMode=1 prolonge -- voir DECISIONS.md). Cause trouvee :
      // drawGifRaw565Frame() (appelee par gifPlayFrameCompat() juste avant
      // cette boucle) fait un SD.read() a CHAQUE frame, en continu pendant
      // toute la duree de l'animation -- son propre commentaire, deja
      // present avant cette session, documente ce point precis comme "le
      // point de contention le plus frequent... deadlock mqttTask/LWIP".
      // delay(0) (~taskYIELD(), quasi aucun temps rendu a l'ordonnanceur)
      // remplace par delay(1) (vrai yield d'au moins 1 tick) -- laisse une
      // fenetre reelle au traitement WiFi/LWIP pendant les lectures SD
      // repetees. N'affecte pas la duree totale d'attente (la boucle
      // continue de cibler fd ms, juste avec des iterations moins
      // frequentes/plus efficaces) ni la fluidite visible de l'animation.
      while((long)(millis()-t)<fd){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(1);}
    }
    break;

  case MODE_PNG:
    if(currentPngPath.length()==0){
      // si un mask doit rester visible (LENT), on n'efface pas l'Ã©cran ici
      if(displayedMaskSysName.length()==0) display->clearScreen();
      currentMode=MODE_BLACK;break;
    }
    if(!pngDrawn)
    {
      if(currentPngAsyncWanted)
      {
        // Lancer le decode async si necessaire
        if(!asyncPngInProgress && !asyncPngReady)
        {
          // async PNG doit Ãªtre dÃ©clenchÃ© uniquement depuis CMD_GAME (slow PNG),
          // on Ã©vite donc de relancer ici pour ne pas spammer des tÃ¢ches.
        }
        // Fallback sÃ©curitÃ© si lâ€™async ne devient jamais prÃªt
        // Si lâ€™async sâ€™est terminÃ©e (Ã©chec ou abandon) sans devenir ready, fallback immÃ©diatement
        else if(!asyncPngInProgress && !asyncPngReady)
        {
          Serial.println("[PNG-ASYNC] async ended -> fallback drawPng reqId=" + String(asyncPngActiveRequestId)
                         + " path=" + currentPngPath);
          asyncPngCancel = true;
          drawPng(currentPngPath);
          pngDrawn = true;
          currentPngAsyncWanted = false;
          asyncPngReady = false;
        }
        else if(!asyncPngReady && asyncPngStartMs > 0 && (millis() - asyncPngStartMs) > 1500UL)
        {
          Serial.println("[PNG-ASYNC] timeout(1500ms) fallback drawPng reqId=" + String(asyncPngActiveRequestId) + " path=" + currentPngPath);
          asyncPngCancel = true;
          drawPng(currentPngPath);
          pngDrawn = true;
          currentPngAsyncWanted = false;
          asyncPngReady = false;
        }

        // Si pret, blit et on repasse en etat PNG affiche
        if(asyncPngReady)
        {
          // Ne pas effacer le mask tant qu'il doit rester affichÃ© : on recouvre directement en blit
          if(displayedMaskSysName.length()==0) display->clearScreen();
          blitPngAsyncFbToDisplay();
          asyncPngReady = false;
          currentPngAsyncWanted = false;
          pngDrawn = true;
        }
      }
      else
      {
        // Comportement normal (PNG "N")
        drawPng(currentPngPath);
        pngDrawn = true;
      }
    }
    // v104 -- consommation de l'overlay en attente retiree (hi-score port
    // supprime, test empirique rc=-4).
    {
      unsigned long t=millis();
      while((long)(millis()-t)<100){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(1);}
    }
    break;

  case MODE_CONFIG:
    // Message de statut transitoire (webDmdPause(), ex: "Mise en cache...")
    // expire apres SD_OP_SUBMSG_EXPIRE_MS sans mise a jour -- retour au
    // message persistant (IP du DMD, pose par triggerWebConfigMode()) plutot
    // que de rester affiche indefiniment une fois le process termine.
    // g_sdOpSubMsgSetAt reste a 0 (donc cette condition jamais vraie) tant
    // que webDmdPause() n'a jamais ete appelee -- les ecrans de boot/secours
    // WiFi (g_sdOpSubMsg assigne directement) ne sont pas concernes.
    if (g_sdOpSubMsgSetAt != 0 && g_sdOpSubMsg != g_sdOpPersistentSubMsg &&
        millis() - g_sdOpSubMsgSetAt > SD_OP_SUBMSG_EXPIRE_MS) {
      g_sdOpSubMsg = g_sdOpPersistentSubMsg;
      g_sdOpSubMsgColor = g_sdOpPersistentSubMsgColor;
      g_sdOpSubMsgSetAt = millis();
      g_configDmdDirty = true;
    }
    if (g_configDmdDirty) {
      webDmdForceRedraw();
    }
    // Defilement ligne 1 -- pas de 4px/tick (au lieu de 1px, 2026-08-09,
    // demande utilisateur : le message WiFi de secours coupait la fin du
    // texte avant d'avoir eu le temps de defiler jusqu'au SSID/IP dans la
    // fenetre de 6s entre 2 bascules, voir maintainApRecovery()).
    {
      bool scroll1 = (g_sdOpMsg.length() * 6) > 128;
      if (scroll1 && millis() - g_sdOpLastScroll1 > 100) {
        g_sdOpScrollOffset1 = (g_sdOpScrollOffset1 + 4) % (g_sdOpMsg.length() * 6 + 32);
        g_sdOpLastScroll1 = millis();
        display->fillRect(0, 4, 128, 8, 0);
        display->setTextColor(0xFFE0);
        display->setCursor(1 - g_sdOpScrollOffset1, 4);
        display->print(g_sdOpMsg);
      }
    }
    // Defilement ligne 2 -- meme acceleration + drawSdOpSubMsgAt() pour
    // le rendu 2-couleurs (voir g_sdOpSubMsgWhiteFrom). Pause ~1.8s des
    // que la fin de la chaine (le SSID/IP en blanc) devient entierement
    // visible a l'ecran (2026-08-09, demande utilisateur), au lieu de
    // continuer a defiler sans jamais s'arreter dessus.
    {
      int textW2 = (int)g_sdOpSubMsg.length() * 6;
      bool scroll2 = textW2 > 128;
      if (scroll2 && millis() - g_sdOpLastScroll > 100) {
        if (g_sdOpSubMsgPauseUntil != 0 && (long)(millis() - g_sdOpSubMsgPauseUntil) < 0) {
          // En pause : ne pas avancer le defilement pour l'instant.
        } else {
          g_sdOpSubMsgPauseUntil = 0;
          int revealOffset = textW2 - 128; // fin de chaine tout juste entierement visible
          int newOffset = g_sdOpScrollOffset + 4;
          bool justRevealed = (g_sdOpScrollOffset < revealOffset) && (newOffset >= revealOffset);
          g_sdOpScrollOffset = newOffset % (textW2 + 32);
          g_sdOpLastScroll = millis();
          display->fillRect(0, 24, 128, 8, 0);
          drawSdOpSubMsgAt(1 - g_sdOpScrollOffset);
          if (justRevealed) g_sdOpSubMsgPauseUntil = millis() + 1800UL;
        }
      }
    }
    delay(1);
    break;

  case MODE_SCORE:
    // v110 -- retour AUTOMATIQUE au jeu, timer 100% local (voir entete
    // changelog) : ne consomme aucun message MQTT pour revenir, garantie
    // anti-blocage explicitement demandee par l'utilisateur.
    if ((long)(millis() - g_scoreShowUntilMs) >= 0) {
      currentMode = g_modeBeforeScore;
      // MODE_PNG saute son dessin si pngDrawn==true (voir case MODE_PNG) --
      // jamais touche pendant l'affichage du score, donc encore a true ici :
      // sans ce reset, l'ecran resterait fige sur le score apres le retour
      // de mode. MODE_GIF/MODE_PLAYLIST n'ont pas besoin de ca :
      // gifPlayFrameCompat() redessine integralement a chaque appel.
      if (currentMode == MODE_PNG) pngDrawn = false;
      Serial.println("[MQTT] score expire -> retour au jeu (mode=" + String((int)currentMode) + ")");
    }
    delay(1);
    break;

  case MODE_BLACK:
  default:
    if (g_sdOpInProgress) {
      processPendingMqttCommand();
      delay(1);
    }
    // v92 -- BUG REEL confirme sur materiel (ecran noir fige ~2min30 observe
    // sur ce build meme, suite a un nom de fichier tronque faisant echouer
    // openNextGif()) : ce case n'avait JAMAIS retente quoi que ce soit --
    // contrairement au blocage heap (qui pose requestNextGif=true et retente,
    // voir openNextGif()), un simple echec d'ouverture (fichier tronque/
    // corrompu, ou desormais aussi le repli CMD_GAME totalement echoue,
    // v92 plus haut) laissait l'ecran noir INDEFINIMENT jusqu'a un evenement
    // MQTT externe fortuit. Fix : retente periodique (meme mecanisme
    // requestNextGif que le blocage heap, rate-limite a 3s pour ne pas
    // marteler la SD) -- getNextGifRandom()/Sequential() tirera tres
    // probablement un index DIFFERENT du fichier fautif, donc pas de boucle
    // infinie sur le meme fichier corrompu.
    //
    // v106 (2026-08-18) -- BUG REEL confirme sur materiel : ce retry se
    // declenchait AUSSI pendant un scroll rapide sur un systeme "lent"
    // (isSlow, ex. mame gros romset) -- le repli CMD_GAME sur ROM non
    // cachee (case 'cached=?', voir plus haut) pose deliberement
    // currentMode=MODE_BLACK comme simple astuce technique pour eviter un
    // clearScreen() (le mask/repli visuel reste affiche, ecran PAS
    // reellement noir/vide), mais laisse ce case le traiter comme un VRAI
    // echec bloque -- au bout de 3s sans nouvelle commande (facilement
    // atteint entre deux ROM du meme defilement rapide), la playlist
    // normale se relancait par-dessus, provoquant des flashs de GIFs
    // aleatoires visibles EN PLEIN MILIEU d'un defilement actif. Distinction
    // fiable trouvee : ce repli pose aussi pngDrawn=true (contenu reellement
    // affiche), alors que les VRAIS cas bloques vises par le fix v92
    // (fichier gif corrompu, repli CMD_GAME totalement echoue) posent tous
    // pngDrawn=false. Fix : ne retenter que si pngDrawn est faux.
    if (!g_sdOpInProgress && !pngDrawn)
    {
      static unsigned long s_lastBlackRetryMs = 0;
      if (millis() - s_lastBlackRetryMs >= 3000)
      {
        s_lastBlackRetryMs = millis();
        requestNextGif = true;
      }
    }
    break;
  }
}
