// ============================================
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v37
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
#include <AnimatedGIF.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "esp_heap_caps.h"
#include "pngle.h"
#include <time.h>
#include "hal/brownout_ll.h"
#include "nvs_flash.h"
#include "clock_themes.h"

// Declarations anticipees: web_config.h (inclus juste apres) utilise ces
// symboles avant leur definition/textuelle plus bas dans ce .ino -- l'auto-
// prototypage Arduino ne couvre pas les macros, et pas de facon fiable les
// fonctions referencees depuis un header inclus avant leur definition.
#define MQTT_PORT 1883
bool parseIP(const String &s, IPAddress &ip);
bool applyStaticIP();
void writeConfigFlag(const String &key, const String &value);

// Generation de playlist -- tache FreeRTOS dediee (playlistGenTask(), definie
// dans web_config.h) + primitives de synchronisation avec loop()/les handlers
// HTTP. Meme principe que MqttCommand/mqttCmdMutex/pendingCmd (mqttTask())
// plus bas dans ce fichier : la tache ne touche JAMAIS gif/display/
// currentMode directement, seulement ce statut partage sous mutex.
//
// Deux mutex, deux strategies d'attente DIFFERENTES -- point critique de
// conception (2026-07-28) : le mutex interne de la lib SD/FS (esp32 core
// 3.3.11, vfs_api.cpp) NE protege PAS File::read/seek/close/openNextFile,
// utilisees a la fois par le scan ET par la lecture de chaque frame GIF
// (gifPlayFrameCompat(), tourne sur loop() a chaque frame). Un mutex
// classique bloquant des 2 cotes ne reglerait rien : si la tache de fond le
// tient plusieurs secondes (la lenteur SD localisee qu'on cherche justement
// a isoler) et que loop() attend ce meme mutex pour lire la frame GIF
// suivante, loop() -- donc le serveur web -- resterait bloque exactement
// comme avant, juste deplace. Regle stricte : sdAccessMutex ne doit JAMAIS
// etre attendu de facon bloquante depuis loop()/un handler HTTP (toujours
// xSemaphoreTake(sdAccessMutex, 0) + degradation gracieuse si indisponible) ;
// seule playlistGenTask() peut l'attendre bloquant. plGenStatusMutex ne
// protege que de simples champs (jamais de SD dans la section critique),
// hold time toujours negligeable des 2 cotes.
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
SemaphoreHandle_t plGenStatusMutex     = nullptr; // garde g_plGenStatus
PlaylistGenStatus g_plGenStatus;
SemaphoreHandle_t sdAccessMutex        = nullptr; // garde tout acces SD partage entre playlistGenTask() et loop()
TaskHandle_t      playlistGenTaskHandle = nullptr; // diagnostic uniquement -- ne jamais l'utiliser comme "scan actif ?" (voir g_plGenStatus.active)

// Tache PERSISTANTE (2026-07-29) essayee puis ABANDONNEE le meme jour :
// corrigeait bien un abort() reel (fopen()->lock_init_generic()) apparaissant
// apres une dizaine de generations separees dans la meme session, mais son
// cout heap permanent (~5 Ko, pile+TCB reserves des le boot au lieu de
// seulement pendant une generation) a cause en test reel un ralentissement/
// non-peuplement reproductible de la page de config (liste de dossiers vide
// alors que l'endpoint direct /lsgifdirs repondait correctement -- donc pas
// un blocage serveur, plutot une degradation generale de reactivite),
// persistant apres redemarrage complet du DMD. Retour a la creation par
// demande ci-dessous ; le crash rare qu'elle visait a corriger sera traite
// autrement par la future refonte TOUS.txt/diff (bien moins d'invocations de
// tache de fond attendues).

#include "web_config.h"

// --------------------------------------------------
// Cache des _defaults par systeme
// --------------------------------------------------
#define SYS_CACHE_MAX 300
static char (*sysCacheKeys)[32] = nullptr; // SYS_CACHE_MAX x 32 (heap)
static char *sysCacheVals = nullptr;       // SYS_CACHE_MAX (heap)
static char *sysCacheSlowVals = nullptr;   // SYS_CACHE_MAX (heap)
static int  sysCacheCount = 0;

char sysDefaultType(const String &sysName)
{
  for (int i = 0; i < sysCacheCount; i++)
    if (sysName == sysCacheKeys[i]) return sysCacheVals[i];
  return '?';
}

char sysDefaultSlowFlag(const String &sysName)
{
  for (int i = 0; i < sysCacheCount; i++)
    if (sysName == sysCacheKeys[i]) return sysCacheSlowVals[i];
  return 'N';
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
    //   <val> <sysName> <slowFlag>
    // Avec compatibilitÃ©:
    //   <val> <sysName>              (slowFlag implicitement 'N')
    String rest = line.substring(2);
    rest.trim();

    int sp2 = rest.indexOf(' ');
    String sysName = (sp2 >= 0) ? rest.substring(0, sp2) : rest;

    char slow = 'N';
    if (sp2 >= 0)
    {
      String flag = rest.substring(sp2 + 1);
      flag.trim();
      if (flag.length() > 0) slow = flag.charAt(0);
    }

    strncpy(sysCacheKeys[sysCacheCount], sysName.c_str(), 31);
    sysCacheKeys[sysCacheCount][31] = '\0';
    sysCacheVals[sysCacheCount] = val;
    sysCacheSlowVals[sysCacheCount] = slow;
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

      countPngGifOverRec("/systems/" + sysName, 800, pngCount, gifCount, pngOver, gifOver);
      sysCacheSlowVals[sysCacheCount] = (pngOver || gifOver) ? 'L' : 'N';
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

// Charge la table bigramme du systeme en heap (une seule lecture SD)
bool loadBigramTable(const String &sysName)
{
  if (bigramTableLoaded && bigramTableSys == sysName && bigramTable != nullptr)
    return true;

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

  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) { free(bigramTable); bigramTable = nullptr; return false; }

  f.seek(sysOffset);
  size_t read = f.read((uint8_t*)bigramTable, NB_IDX * 4);
  f.close();

  if (read < (size_t)(NB_IDX * 4))
  {
    free(bigramTable); bigramTable = nullptr; return false;
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

// --------------------------------------------------
// Globaux
// --------------------------------------------------
MatrixPanel_I2S_DMA *display = nullptr;
AnimatedGIF gif;
SPIClass spiSD(VSPI);
File gifFile;
File nextGifFile;
BluetoothSerial SerialBT;

enum DisplayMode { MODE_PLAYLIST, MODE_GIF, MODE_PNG, MODE_CONFIG, MODE_BLACK };
volatile DisplayMode currentMode = MODE_PLAYLIST;

bool g_sdOpInProgress = false;

String   g_sdOpMsg       = "";
String   g_sdOpSubMsg    = "";
uint16_t g_sdOpSubMsgColor = 0xFFFF;
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

bool   bluetoothEnabled = false;
String bluetoothName    = "ESP32-GIF";
bool   showInfo         = true;
int    screenBrightness = 120;  // 0..255 (map depuis 0-100% dans config.ini: brightness=)

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
// Duree minimale d'affichage de l'image de secours (CMD_WAITING_MQTT) a la
// connexion MQTT -- sans ca, un message RETENU (mosquitto -r, publie par le
// pont marquee lors d'une session precedente : "system=lastplayed" par ex.)
// arrive quasi instantanement a la souscription et ecrase l'image de
// secours avant meme qu'elle soit visible (bug remonte : "il prend le
// premier mqtt lastplayed"). Un vrai message d'evenement RB (ex: le
// "system" envoye par le pont marquee ~5s apres son propre demarrage)
// arrive largement apres cette fenetre, donc n'est jamais bloque.
const unsigned long MQTT_WAITING_GRACE_MS = 1500;
unsigned long g_mqttWaitingUntilMs = 0;

// Duree d'affichage FIXE de l'ecran "RecalBox connectee" (CMD_WAITING_MQTT)
// avant reprise automatique de la playlist -- demande utilisateur
// (2026-07-28) : ne plus attendre indefiniment le premier message MQTT reel
// (system/game), reprendre la playlist au bout de ce delai si rien d'autre
// n'a pris la main sur l'affichage entre-temps.
const unsigned long MQTT_CONNECTED_SCREEN_MS = 10000; // 5000 -> 10000 (demande utilisateur 2026-07-29)
unsigned long g_mqttConnectedScreenUntilMs = 0;

WiFiClient   wifiClientMqtt;
PubSubClient mqttClient(wifiClientMqtt);
String       lastSysName = "";
String       displayedMaskSysName = "";

struct MqttCommand
{
  enum Type { CMD_NONE, CMD_STOP, CMD_DEFAULT, CMD_SYSTEM, CMD_GAME,
              CMD_STARTCLIP, CMD_RESUMESYS, CMD_SHOW_CONFIG, CMD_WIFI_RECOVERY,
              CMD_REBOOT, CMD_WAITING_MQTT };
  Type   type;
  String arg;
  MqttCommand() : type(CMD_NONE), arg("") {}
  MqttCommand(Type t, const String &a) : type(t), arg(a) {}
};

SemaphoreHandle_t mqttCmdMutex   = nullptr;
MqttCommand       pendingCmd;
TaskHandle_t      mqttTaskHandle = nullptr;

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

// Cache RAM du fallback /systems/_defaults/default.raw565 (8KB)
static uint16_t *defaultRaw565Buf = nullptr;
static bool defaultRaw565Cached = false;
static const char *DEFAULT_RAW565_PATH = "/systems/_defaults/default.raw565";

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
  char slowFlag = sysDefaultSlowFlag(sysName);
  bool isSlow = (slowFlag == 'L' || slowFlag == 'l');

  Serial.println("[PNG-RAW] missing raw -> sysName=" + sysName + " slowFlag=" + String(slowFlag) + " isSlow=" + String(isSlow));

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
static bool gifPlayFrameCompat(bool first, int *pDelayMs)
{
  if (xSemaphoreTake(sdAccessMutex, 0) != pdTRUE)
  {
    *pDelayMs = 5;
    return true;
  }
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
  xSemaphoreGive(sdAccessMutex);
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

bool openGif(const String &path, bool clearBefore=true, bool skipProbe=false, bool skipRawPack=false)
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

String getNextGifRandom()
{
  if(gifCount<=0) return "";
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

String getNextGif(){if(gifCount<=0)return "";return playlistRandom?getNextGifRandom():getNextGifSequential();}

void openNextGif()
{
  // Transition entre 2 GIFs (moins frequente qu'une frame, mais touche
  // encore la SD -- getNextGif()/openGif()). Meme regle de non-blocage que
  // gifPlayFrameCompat() : si playlistGenTask() tient sdAccessMutex, on ne
  // bascule pas en ecran noir pour rien -- on redemande ce meme GIF au
  // prochain tour de loop() via requestNextGif (deja verifie une fois par
  // iteration, voir loop()).
  if (xSemaphoreTake(sdAccessMutex, 0) != pdTRUE)
  {
    requestNextGif = true;
    return;
  }
  String next=(nextGifPath.length()>0)?nextGifPath:getNextGif(); nextGifPath="";
  bool ok = (next.length()>0) && openGif(next,false,true,true);
  if (ok) nextGifPath=getNextGif();
  xSemaphoreGive(sdAccessMutex);
  if (!ok)
  {gifOpened=false;currentMode=MODE_BLACK;display->clearScreen();return;}
  currentMode=MODE_PLAYLIST;
}

void resumePlaylist()
{
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

// Redessine immediatement l'ecran MODE_CONFIG (les 2 lignes) a partir de
// g_sdOpMsg/g_sdOpSubMsg -- factorise depuis loop() pour pouvoir aussi etre
// appelee depuis un contexte bloquant hors boucle normale si besoin.
void webDmdForceRedraw()
{
  g_configDmdDirty = false;
  g_sdOpScrollOffset = 0;
  g_sdOpScrollOffset1 = 0;
  display->clearScreen();
  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(0xFFE0);
  display->setCursor(1, 4);
  display->print(g_sdOpMsg);
  display->fillRect(0, 24, 128, 8, 0);
  display->setTextColor(g_sdOpSubMsgColor);
  display->setCursor(1, 24);
  display->print(g_sdOpSubMsg);
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
  // ecrans/handlers HTTP restent actifs) et on reprend l'affichage normal,
  // comme le fait deja resumePlaylist() pour les commandes MQTT CMD_DEFAULT.
  // g_sdOpInProgress doit etre remis a false explicitement ici -- avant, un
  // ESP.restart() le remettait a zero gratuitement au boot ; les handlers
  // MQTT (CMD_STOP/CMD_DEFAULT/CMD_SYSTEM/CMD_GAME) l'utilisent pour ignorer
  // toute commande tant que le mode config est actif, donc l'oublier ici
  // bloquerait ces commandes indefiniment apres un "Reprendre DMD".
  Serial.println("[WEB] DMD resume -> retour a l'affichage normal (sans reboot)");
  g_sdOpInProgress = false;
  resumePlaylist();
}

// Marque first_boot=0 dans config.ini (appele quand page web ouverte)
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
String trRecalboxConnected()
{
  if (uiLanguage == "en") return "RecalBox connected";
  if (uiLanguage == "es") return "RecalBox conectada";
  return "RecalBox connectee";
}

// Dessine (visible=true) ou efface (visible=false) le texte "RecalBox
// connectee", centre horizontalement, avec la meme ombre noir/blanc
// qu'avant -- clignotant pendant tout l'affichage (voir loop(), toggle
// periodique). Centre le calcule dynamiquement (largeur variable selon la
// langue) plutot qu'une position fixe.
void drawRecalboxConnectedOverlay(bool visible)
{
  display->fillRect(0, 24, RAW565_W, 8, 0);
  if (!visible) return;
  display->setTextWrap(false);
  display->setTextSize(1);
  String txt = trRecalboxConnected();
  int textW = txt.length() * 6; // taille 1 = 6px/caractere
  int x = (RAW565_W - textW) / 2;
  if (x < 0) x = 0;
  display->setTextColor(display->color565(0, 0, 0));
  display->setCursor(x + 1, 25);
  display->print(txt);
  display->setTextColor(display->color565(255, 255, 255));
  display->setCursor(x, 24);
  display->print(txt);
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
bool hasPendingMqttCommand()
{
  if(mqttCmdMutex==nullptr) return false;
  if(xSemaphoreTake(mqttCmdMutex,0)!=pdTRUE) return false;
  bool has=(pendingCmd.type!=MqttCommand::CMD_NONE);
  xSemaphoreGive(mqttCmdMutex); return has;
}

void processPendingMqttCommand()
{
  if(mqttCmdMutex==nullptr) return;
  if(xSemaphoreTake(mqttCmdMutex,0)!=pdTRUE) return;
  MqttCommand cmd=pendingCmd; pendingCmd=MqttCommand(MqttCommand::CMD_NONE,"");
  xSemaphoreGive(mqttCmdMutex);
  if(cmd.type==MqttCommand::CMD_NONE) return;

  switch(cmd.type)
  {
  case MqttCommand::CMD_STOP:
    if(currentMode==MODE_PLAYLIST||g_sdOpInProgress){Serial.println("[MQTT] stop ignored");break;}
    gif.close();gifOpened=false;currentPngPath="";pngDrawn=false;
    currentMode=MODE_BLACK;display->clearScreen();
    break;

  case MqttCommand::CMD_DEFAULT:
    if (g_sdOpInProgress) { Serial.println("[MQTT] default ignored (web open)"); break; }
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
      // Reprise automatique de la playlist apres un delai fixe (demande
      // utilisateur) -- ne plus attendre indefiniment le 1er message MQTT
      // reel (system/game). Verifie dans loop() (voir plus bas) ; sans effet
      // si un vrai media a deja pris la main sur l'affichage entre-temps.
      g_mqttConnectedScreenUntilMs = millis() + MQTT_CONNECTED_SCREEN_MS;
    }
    break;

  case MqttCommand::CMD_SYSTEM:
    if (g_sdOpInProgress) { Serial.println("[MQTT] system ignored (web open)"); break; }
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";
    currentMode=MODE_BLACK;
    if(nextGifFile){nextGifFile.close();nextGifFile=File();nextGifPath="";}
    freeBigramAll();
    currentMode=openBestMedia("/systems/"+cmd.arg+"/_default");
    displayedMaskSysName=cmd.arg;
    break;

  case MqttCommand::CMD_GAME:
    if (g_sdOpInProgress) { Serial.println("[MQTT] game ignored (web open)"); break; }
  {
    int slash=cmd.arg.indexOf('/');
    String sysName=(slash>=0)?cmd.arg.substring(0,slash):cmd.arg;
    String romName=(slash>=0)?cmd.arg.substring(slash+1):cmd.arg;
    String sysBase="/systems/"+sysName+"/_default";
    String gameBase="/systems/"+cmd.arg;
    if(imageFolder.length()>0)
      gameBase="/systems/"+sysName+"/"+imageFolder+"/"+romName;

    char slowFlag=sysDefaultSlowFlag(sysName);
    bool isSlow=(slowFlag=='L'||slowFlag=='l');
    bool needDrawMask = false;
    bool maskDrawn = false;

    // Debug gÃ©nÃ©ral (doit s'afficher pour PS2 aussi)
    Serial.println("[CMD_GAME] enter sys=" + sysName
                   + " rom=" + romName
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

      display->clearScreen();

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

      // fallback : default.png/default.raw
      String defPng = "/systems/_defaults/default.png";
      Serial.println("[CMD_GAME] fast fallback -> " + defPng);
      display->clearScreen();
      if(drawPng(defPng))
      {
        pngDrawn = true;
        currentPngPath = defPng;
        currentMode = MODE_PNG;
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

      // DEBUG pour comprendre pourquoi le jeu n'est pas affichÃ© (vs mask)
      Serial.println("[CMD_GAME] debug sys=" + sysName
                   + " rom=" + romName
                   + " cached=" + String(cached)
                   + " isSlow=" + String(isSlow)
                   + " gameBase=" + gameBase
                   + " slowFlag=" + String(slowFlag));

      // NE PAS clearScreen ici: le mask doit rester visible pendant le chargement.
      // En mode lent, on force le type d'affichage selon le flag systÃ¨me:
      // - sysType 'g'/'B' => raw565pack via openGif(...) (pas drawPng/raw565)
      // - sysType 'p'     => drawPng/raw565
      {
        char sysT = sysDefaultType(sysName);
        // Si le jeu n'est PAS dans le cache bigram (cached='?'), fallback RAM direct.
        // Ne PAS forcer 'g' (qui ferait openGif() lent sur dossier de 800+ fichiers).
        if(cached == '?')
        {
          Serial.println("[CMD_GAME] slow cached=? -> fallback default.raw565 RAM t=" + String(millis()));
          if(drawDefaultRaw565Cached())
          {
            // Garder displayedMaskSysName=sysName pour que loop() ne clearScreen pas
            // (voir MODE_PNG: if(displayedMaskSysName.length()==0) display->clearScreen())
            displayedMaskSysName = sysName;
            pngDrawn=true; currentPngPath=""; currentMode=MODE_BLACK;
            Serial.println("[CMD_GAME] slow cached=? fallback OK t=" + String(millis()));
            break;
          }
        }
        // cached present: force le type d'affichage selon le flag systeme
        char cachedBefore = cached;
        if(sysT=='g' || sysT=='B') cached = 'g';
        else if(sysT=='p') cached = 'p';
        Serial.println("[CMD_GAME] slow cached force sysType=" + String(sysT)
                       + " cachedBefore=" + String(cachedBefore)
                       + " cachedAfter=" + String(cached));
      }
      if(cached=='p')
      {
        String path=gameBase+".png";
        // Async pngle_new() Ã©choue systÃ©matiquement en tÃ¢che => fallback synchrone
        Serial.println("[CMD_GAME] slow PNG fallback sync sys=" + sysName + " path=" + path);

        currentPngPath = path;
        currentPngAsyncWanted = false;
        asyncPngCancel = true;

        // Remplacer le mask par l'image PNG (garder le mask si affiche)
        // En LENT, on ne clear que si pas de mask actif.
        if (displayedMaskSysName.length() == 0)
        {
          Serial.println("[CMD_GAME] slow clearScreen (no mask) BEFORE t=" + String(millis()));
          display->clearScreen();
          Serial.println("[CMD_GAME] slow clearScreen AFTER t=" + String(millis()));
        }
        else
        {
          Serial.println("[CMD_GAME] slow keep mask on screen during load t=" + String(millis()));
        }

        Serial.println("[CMD_GAME] slow before drawPng t=" + String(millis())
                       + " maskLen=" + String(displayedMaskSysName.length()));
        bool okDraw = drawPng(path);
        Serial.println("[CMD_GAME] slow after drawPng ok=" + String(okDraw ? "1" : "0")
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
        Serial.println("[CMD_GAME] slow cached=g raw565pack fail -> try game raw565 t=" + String(millis()));
        {
          String rawPath=gameBase+".raw565";
          if(drawRaw565(rawPath))
          {
            pngDrawn=true;
            currentPngPath=rawPath;
            // Garder displayedMaskSysName inchangÃ© pour que loop() ne clearScreen pas.
            // MODE_PNG avec pngDrawn=true â†’ loop() ne touche pas Ã  l'affichage.
            currentMode=MODE_PNG;
            Serial.println("[CMD_GAME] slow cached=g game raw565 OK t=" + String(millis()));
            break;
          }
        }
        Serial.println("[CMD_GAME] slow cached=g game raw565 fail -> fallback default.raw565 t=" + String(millis()));
        if(drawDefaultRaw565Cached())
        {
          pngDrawn=true;
          currentPngPath="";
          displayedMaskSysName="";
          currentMode=MODE_PNG;
          Serial.println("[CMD_GAME] slow cached=g fallback default.raw565 OK t=" + String(millis()));
          break;
        }
        // fallback mem epuise -> probe3 classique
      }

      // 3) fallback: tester existence PNG/GIF sans effacer l'ecran
      {
        String pngPath=gameBase+".png";

        unsigned long tProbeStart = millis();
        Serial.println("[CMD_GAME] probe3 start t=" + String(tProbeStart) + " png=" + pngPath);

        Serial.println("[CMD_GAME] probe3 calling drawPng (skip SD.exists) t=" + String(millis()));

        if(drawPng(pngPath))
        {
          currentPngPath=pngPath; pngDrawn=true; currentMode=MODE_PNG;
          displayedMaskSysName="";
          loadBigramTable(sysName);
          break;
        }
        else
        {
          Serial.println("[CMD_GAME] probe3 drawPng failed t=" + String(millis()));
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
    Serial.println("[MQTT] startgameclip -> playlist");
    resumePlaylist();
    break;

  case MqttCommand::CMD_RESUMESYS:
    if (g_sdOpInProgress) { Serial.println("[MQTT] resumesys ignored"); break; }
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
    clearFirstBoot();
    webDmdSetMainMsg("WEB DMD CONFIG");
    // Ligne 2 : message complet (defile automatiquement si >128px, cf boucle
    // de rendu MODE_CONFIG) plutot que la seule IP -- plus clair pour
    // l'utilisateur qui regarde l'ecran du DMD sans autre contexte.
    webDmdPause(trOpenBrowserAt(WiFi.localIP().toString()), 0xFFE0);
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

  default: break;
  }
}

// --------------------------------------------------
// MQTT callback
// --------------------------------------------------
void onMqttMessage(char *topic, byte *payload, unsigned int length)
{
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
  // uniquement default/system/game pendant cette fenetre tres courte (1.5s) --
  // stop/show_config/wifi_recovery/reboot restent des actions explicites,
  // jamais suppriemees.
  bool inWaitingGrace = (millis() < g_mqttWaitingUntilMs);

  if     (t=="marquee/cmd/stop")    pendingCmd=MqttCommand(MqttCommand::CMD_STOP,"");
  else if(t=="marquee/cmd/default") { if(!inWaitingGrace) pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,""); }
  else if(t=="marquee/cmd/system")  { if(!inWaitingGrace) {lastSysName=msg;pendingCmd=MqttCommand(MqttCommand::CMD_SYSTEM,msg);} }
  else if(t=="marquee/cmd/game")    { if(!inWaitingGrace) pendingCmd=MqttCommand(MqttCommand::CMD_GAME,msg); }
  else if(t=="marquee/cmd/show_config") pendingCmd=MqttCommand(MqttCommand::CMD_SHOW_CONFIG,"");
  else if(t=="marquee/cmd/wifi_recovery") pendingCmd=MqttCommand(MqttCommand::CMD_WIFI_RECOVERY,"");
  else if(t=="marquee/cmd/reboot")        pendingCmd=MqttCommand(MqttCommand::CMD_REBOOT,"");
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
      else pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,"");
    }
  }
  xSemaphoreGive(mqttCmdMutex);
}

// --------------------------------------------------
// MQTT task
// --------------------------------------------------
void mqttTask(void *param)
{
  (void)param;
  vTaskDelay(pdMS_TO_TICKS(MQTT_START_DELAY_MS));
  unsigned long lastMqttConnectedMs=millis();

  for(;;)
  {
    if(!wifiEnabled||recalboxIP.length()==0){vTaskDelay(pdMS_TO_TICKS(2000));continue;}
    if(WiFi.status()!=WL_CONNECTED){vTaskDelay(pdMS_TO_TICKS(1000));continue;}

    // En mode config web : ne pas tenter de connexion MQTT (garde les sockets libres pour HTTP)
    // Idem pendant une generation de playlist (2026-07-29, test en cours) :
    // playlistGenTask() tourne sur le meme coeur que cette tache -- une
    // tentative de (re)connexion MQTT ici (allocations pour le TCP/DNS)
    // pourrait etre le facteur qui fait basculer le heap sous ce dont
    // openNextFile() a besoin au mauvais moment, cause suspectee du crash
    // reel observe sur ce meme materiel. Ne saute que la TENTATIVE de
    // connexion -- .loop() reste actif si deja connecte, donc une commande
    // (ex. reboot) recue avant le debut du scan continue d'etre traitee.
    bool plGenActiveNow = false;
    if (xSemaphoreTake(plGenStatusMutex, 0) == pdTRUE) { plGenActiveNow = g_plGenStatus.active; xSemaphoreGive(plGenStatusMutex); }
    if(g_sdOpInProgress || plGenActiveNow) { if(mqttClient.connected()) mqttClient.loop(); vTaskDelay(pdMS_TO_TICKS(1000)); continue; }

    if(!mqttClient.connected())
    {
      Serial.println("[MQTT] connecting to "+recalboxIP);
      if(mqttClient.connect(MQTT_CLIENT))
      {
        Serial.println("[MQTT] connected");
        lastMqttConnectedMs=millis();
        mqttClient.subscribe("marquee/cmd/stop");
        mqttClient.subscribe("marquee/cmd/default");
        mqttClient.subscribe("marquee/cmd/system");
        mqttClient.subscribe("marquee/cmd/game");
        mqttClient.subscribe("marquee/cmd/show_config");
        mqttClient.subscribe("marquee/cmd/wifi_recovery");
        mqttClient.subscribe("marquee/cmd/reboot");
        mqttClient.subscribe(mqttEventTopic.c_str());
        // Retenu (retain=true) : un abonne (script Recalbox) qui se connecte
        // plus tard recoit immediatement la derniere IP publiee, sans avoir
        // besoin d'etre a l'ecoute au moment exact de cette connexion.
        mqttClient.publish("marquee/status/ip", WiFi.localIP().toString().c_str(), true);
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
            g_mqttWaitingUntilMs=millis()+MQTT_WAITING_GRACE_MS;
          }
          xSemaphoreGive(mqttCmdMutex);
        }
      }
      else
      {
        Serial.println("[MQTT] failed rc="+String(mqttClient.state()));
        unsigned long now=millis();
        if((now-lastMqttConnectedMs)>=MQTT_OFFLINE_FALLBACK_MS)
        {
          if(currentMode!=MODE_PLAYLIST&&gifCount>0&&!g_sdOpInProgress)
          {
            Serial.println("[MQTT] injoignable -> reprise playlist");
            if(mqttCmdMutex!=nullptr&&xSemaphoreTake(mqttCmdMutex,pdMS_TO_TICKS(100))==pdTRUE)
            {pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,"");xSemaphoreGive(mqttCmdMutex);}
            lastMqttConnectedMs=now;
          }
        }
        vTaskDelay(pdMS_TO_TICKS(MQTT_RETRY_MS)); continue;
      }
    }
    else lastMqttConnectedMs=millis();

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
    g_configDmdDirty = true;
  }
  // Alterne SSID / IP (avec instruction prefixee) sur la ligne 2 toutes les
  // 6s -- ces chaines depassent 128px avec le prefixe, 6s (au lieu de 2s)
  // laisse le defilement horizontal existant le temps d'avancer avant de
  // reinitialiser le scroll sur la chaine suivante.
  static unsigned long lastToggle = 0;
  static bool showSSID = true;
  if (ms - lastToggle >= 6000UL) {
    lastToggle = ms;
    showSSID = !showSSID;
    g_sdOpSubMsg = showSSID ? trJoinWifi(AP_RECOVERY_SSID) : trOpenInBrowser(String("http://") + apRecoveryIP);
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
    g_sdOpSubMsg = trJoinWifi(AP_RECOVERY_SSID);
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
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.setAutoReconnect(true);
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
    if (attempt > 0) { WiFi.disconnect(); delay(100); }
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
    delay(1200);
    autoDetectRecalboxIP();
    if(recalboxIP.length()>0){
      mqttClient.setServer(recalboxIP.c_str(),MQTT_PORT);
      mqttClient.setCallback(onMqttMessage);
      mqttClient.setKeepAlive(60);
      mqttClient.setSocketTimeout(30);
    }
  }
  else{
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
  }
}

void maintainWiFi()
{
  if(!wifiEnabled||wifiSSID.length()==0) return;
  if(WiFi.status()==WL_CONNECTED) return;
  // Ne pas reconnecter en mode AP (fallback) ou si config web ouverte
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) return;
  if (g_sdOpInProgress) return;
  unsigned long now=millis();
  if(now-lastWifiReconnectAttempt<5000) return;
  lastWifiReconnectAttempt=now;
  Serial.println("[WIFI] reconnect");
  delay(1500);WiFi.disconnect();delay(50);
  // Reappliquer l'IP fixe: WiFi.disconnect() reinitialise la config IP de
  // l'interface, sans reappel ici toute reconnexion repassait silencieusement
  // en DHCP (bug identifie en session -- IP fixe perdue apres la moindre
  // coupure WiFi transitoire).
  applyStaticIP();
  WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());
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
    else if(key=="brightness")                            screenBrightness =map(constrain(value.toInt(),0,100),0,100,0,255);
    else if(key=="mqtt_event_topic"   &&value.length())  mqttEventTopic   =value;
    else if(key=="first_boot")                           g_firstBoot      =(value!="0");
    else if(key=="force_ap_recovery")                    g_forceApRecovery=(value!="0");
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
#define RETRO_VERSION "Raw565 Ed. dev11"

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

  // Ligne 2 : version centrée ("Raw565 Ed. v10" = 14 x 6 = 84px -> x = (128-84)/2 = 22)
  display->setCursor(22, 21);
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
static bool showClock()
{
  if (!clockEnabled) return true;
  if (g_sdOpInProgress) return true;

  // Choisir le theme
  currentTheme = (clockTheme >= 0 && clockTheme < RETRO_THEME_COUNT)
                  ? clockTheme
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
      if (g_sdOpInProgress) {
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
  while (millis() < endMs) {
    handleWebConfig();
    yield();

    // Interruption si page web ouverte
    if (g_sdOpInProgress) {
      Serial.println("[CLOCK] Interrupted by web page");
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
    if (clockTheme == -1 && (millis() - themeStartMs) >= ((unsigned long)clockDuration * 1000UL)
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
          if (g_sdOpInProgress) {
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

void setup()
{
  brownout_ll_bod_enable(false);     // Desactive BOD (evite reboot intempestifs)
  brownout_ll_intr_enable(false);    // Desactive IRQ BOD
  brownout_ll_reset_config(false, 0, BROWNOUT_RESET_LEVEL_CHIP);
  Serial.begin(115200); delay(1000);

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
    gamesIdx = (GamesSysIdx*)malloc(sizeof(GamesSysIdx) * GAMES_IDX_MAX);
  }

  if (!sysCacheKeys || !sysCacheVals || !sysCacheSlowVals || !gamesIdx)
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
      }
      cfg.close();
    }
  }

  showSplashScreen();  // Toujours affichÃ©, indÃ©pendamment de info=
  

  // Charge le cache systÃ¨mes (systems_cache.dat). Si absent, on ne rescanner
  // que si l'utilisateur a info=1. Le script Python Ã©crit dÃ©jÃ  ce fichier.
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

  loadConfig();

  // Pour les systÃ¨mes flags 'L' (lents), games_cache.bin est court-circuitÃ©
  // dans findInGamesCache(). Inutile de le charger pour ces systÃ¨mes.
  // On le charge quand mÃªme pour les systÃ¨mes 'N' qui en ont besoin.
  if(!loadGamesIndex())
    Serial.println("[GCACHE] "+gamesCacheFile+" absent");
  else
    Serial.println("[GCACHE] OK - "+String(gamesIdxCount)+" systemes");
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

  // Premier demarrage : inviter l'utilisateur a ouvrir la page web
  if (g_firstBoot) {
    String ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
    if (ip == "0.0.0.0") ip = "192.168.4.1";
    g_sdOpMsg = trConnectWifiMsg();
    g_sdOpSubMsg = trOpenUrl(ip);
    g_sdOpSubMsgColor = 0x07E0;
    g_sdOpInProgress = true;
    currentMode = MODE_CONFIG;
    g_configDmdDirty = true;
    Serial.println("[BOOT] First boot - ouvrir http://" + ip);
  }

  mqttCmdMutex=xSemaphoreCreateMutex();
  pendingCmd=MqttCommand(MqttCommand::CMD_NONE,"");
  plGenStatusMutex=xSemaphoreCreateMutex();
  sdAccessMutex=xSemaphoreCreateMutex();
  if (g_firstBoot) {
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

  if(playlistName.length()==0){
    String ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
    if (ip == "0.0.0.0") ip = "192.168.4.1";
    g_sdOpMsg = trConfigPageMsg();
    g_sdOpSubMsg = trOpenUrl(ip);
    g_sdOpSubMsgColor = 0x07E0;
    g_sdOpInProgress = true;
    currentMode = MODE_CONFIG;
    g_configDmdDirty = true;
    Serial.println("[BOOT] No playlist -> config mode sur http://" + ip);
    goto start_mqtt_task;
  }

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
    playIndex=0;lastRandomIndex=-1;currentMode=MODE_PLAYLIST;openNextGif();
    Serial.println("[BOOT] apres 1er openNextGif, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
  }

start_mqtt_task:
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
  handleWebConfig(); maintainWiFi(); maintainApRecovery(); processPendingMqttCommand();
  // Reprise auto de la playlist apres l'ecran "RecalBox connectee" (voir
  // CMD_WAITING_MQTT) -- sans effet si un vrai media (system/game) a deja
  // pris la main sur l'affichage entre-temps (currentPngPath change).
  if (g_mqttConnectedScreenUntilMs != 0 && millis() >= g_mqttConnectedScreenUntilMs)
  {
    g_mqttConnectedScreenUntilMs = 0;
    if (currentMode == MODE_PNG && currentPngPath == String(DEFAULT_RAW565_PATH) && !g_sdOpInProgress)
    {
      Serial.println("[MQTT] fin ecran connexion -> reprise playlist");
      resumePlaylist();
    }
  }
  // Clignotement du texte "RecalBox connectee" pendant tout l'affichage
  // (demande utilisateur 2026-07-29) -- meme garde que la reprise auto
  // ci-dessus (sans effet si un vrai media a deja pris la main). Toggle
  // simple ~2 fois/seconde, pas de garde-fou de cout necessaire (juste un
  // fillRect + eventuellement 2 print, deja fait a chaque CMD_WAITING_MQTT).
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
      bool active = false; String dirName; int gifs = 0;
      if (xSemaphoreTake(plGenStatusMutex, 0) == pdTRUE) {
        active = g_plGenStatus.active;
        dirName = g_plGenStatus.curDirName;
        gifs = g_plGenStatus.curDirGifs;
        xSemaphoreGive(plGenStatusMutex);
      }
      if (active && currentMode == MODE_CONFIG)
      {
        webDmdOverlayLine2(plGenDmdText(dirName, gifs), 0x07E0);
      }
      lastPlGenDmdMs = millis();
    }
  }
  if(requestNextGif&&!g_sdOpInProgress){requestNextGif=false;openNextGif();}
  if(requestReboot) {delay(100);ESP.restart();}

  switch(currentMode)
  {
  case MODE_PLAYLIST:
    if(!gifOpened){display->clearScreen();currentMode=MODE_BLACK;break;}
    {
      int fd=0; bool frameOk=gifPlayFrameCompat(false,&fd);
      if(!frameOk){
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
      if(nextGifPath.length()==0)nextGifPath=getNextGif();
      unsigned long t=millis();
      while((long)(millis()-t)<fd){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(0);}
      // Pre-chargement opportuniste (deja optionnel avant : ne fait rien si
      // nextGifFile est deja pris). Non bloquant sur sdAccessMutex -- une
      // tentative ratee est sans consequence, retentee au prochain tour.
      if(nextGifPath.length()>0&&!nextGifFile&&xSemaphoreTake(sdAccessMutex,0)==pdTRUE){
        nextGifFile=SD.open(nextGifPath.c_str());
        xSemaphoreGive(sdAccessMutex);
      }
    }
    break;

  case MODE_GIF:
    if(!gifOpened){display->clearScreen();currentMode=MODE_BLACK;break;}
    {
      int fd=0; bool frameOk=gifPlayFrameCompat(false,&fd);
      if(!frameOk){gifResetCompat();break;}
      if(fd<=0)fd=10;
      unsigned long t=millis();
      while((long)(millis()-t)<fd){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(0);}
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
    // Defilement ligne 1
    {
      bool scroll1 = (g_sdOpMsg.length() * 6) > 128;
      if (scroll1 && millis() - g_sdOpLastScroll1 > 100) {
        g_sdOpScrollOffset1 = (g_sdOpScrollOffset1 + 1) % (g_sdOpMsg.length() * 6 + 32);
        g_sdOpLastScroll1 = millis();
        display->fillRect(0, 4, 128, 8, 0);
        display->setTextColor(0xFFE0);
        display->setCursor(1 - g_sdOpScrollOffset1, 4);
        display->print(g_sdOpMsg);
      }
    }
    // Defilement ligne 2
    {
      bool scroll2 = (g_sdOpSubMsg.length() * 6) > 128;
      if (scroll2 && millis() - g_sdOpLastScroll > 100) {
        g_sdOpScrollOffset = (g_sdOpScrollOffset + 1) % (g_sdOpSubMsg.length() * 6 + 32);
        g_sdOpLastScroll = millis();
        display->fillRect(0, 24, 128, 8, 0);
        display->setTextColor(g_sdOpSubMsgColor);
        display->setCursor(1 - g_sdOpScrollOffset, 24);
        display->print(g_sdOpSubMsg);
      }
    }
    delay(1);
    break;

  case MODE_BLACK:
  default:
    if (g_sdOpInProgress) {
      processPendingMqttCommand();
      delay(1);
    }
    break;
  }
}
