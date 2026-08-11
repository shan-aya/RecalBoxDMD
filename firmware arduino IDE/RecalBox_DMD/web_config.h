// ============================================
// web_config.h — Interface web de configuration
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v58
//
// v58 — 2026-08-11 — safe-modify — Demande utilisateur, 2 ajouts sur
//   l'apercu en direct des themes horloge (v57) + luminosite (v55) :
//   (1) Page Horloge (loadConfig()) : des l'ouverture de la page, le theme
//   deja preselectionne (config sauvegardee ou brouillon localStorage)
//   declenche desormais l'apercu automatiquement, sans avoir a re-cliquer
//   dessus -- SAUF si "Aleatoire" (-1) est preselectionne (rien de
//   pertinent a previsualiser pour un tirage non encore effectue).
//   (2) Petit texte ajoute sous le selecteur de theme (page Horloge) et
//   sous le curseur de luminosite (page Affichage) pour indiquer que
//   l'ajustement s'applique en direct sur le DMD physique -- ces 2
//   mecanismes (v55 luminosite, v57 horloge) etaient jusqu'ici muets sur ce
//   comportement, seulement documentes en commentaire code.
//
// v57 — 2026-08-11 — safe-modify — Apercu en direct des themes horloge
//   (voir RecalBox_DMD.ino v72 pour le detail complet du mecanisme cote
//   firmware). Nouvel endpoint POST /clock-preview (handleWebConfigClockPreview(),
//   arg "theme" = "-1".."9" ou "stop") -- pont vers requestClockPreview()
//   defini dans RecalBox_DMD.ino (ce fichier est inclus AVANT le type
//   MqttCommand/pendingCmd, d'ou le pont plutot qu'un acces direct). Cote
//   JS (page Horloge, WEB_CONFIG_CLOCK_HTML) : le <select id="clock_theme">
//   envoie desormais la selection immediatement sur "onchange" (plus besoin
//   de "Sauvegarder"), et un navigator.sendBeacon() sur pagehide/beforeunload
//   envoie "theme=stop" pour arreter l'apercu quand on quitte la page
//   (Basic/Network/Media etant des pages separees, pas des onglets JS).
//   Pas encore teste sur materiel reel.
//
// v56 — 2026-08-11 — safe-modify — Luminosite DMD appliquee en direct (sans
//   reboot), demande explicite utilisateur (worktree dev/live-brightness,
//   voir aussi RecalBox_DMD.ino v71 pour la commande MQTT associee).
//   handleWebConfigSave() appelle desormais display->setBrightness8() juste
//   apres avoir mis a jour screenBrightness (effet immediat au clic
//   "Sauvegarder", au lieu d'attendre le prochain reboot). Nouvel endpoint
//   POST /set-brightness (handleWebConfigSetBrightness()) : met a jour
//   screenBrightness en RAM et appelle setBrightness8() SANS toucher a
//   /config.ini (ca reste le role de "Sauvegarder") -- utilise pour
//   l'apercu live pendant le drag du curseur de luminosite. Cote JS, le
//   slider #brightness envoie desormais un fetch throttle (>=120ms entre
//   deux envois pendant le drag) vers /set-brightness sur "input", plus un
//   envoi garanti sur "change" (relachement) pour ne pas perdre la valeur
//   finale. Pas encore teste sur materiel reel.
//
// v55 — 2026-08-10 — safe-modify — Page Medias (WEB_CONFIG_MEDIA_HTML),
//   texte d'accompagnement de la section "Envoi GIF" (desc_upload, FR/EN/ES) :
//   la phrase pour un transfert consequent renvoyait vers un ajout manuel
//   sur la carte SD ("retirez la carte SD et copiez-la depuis un PC"),
//   incoherent avec la page Playlist (desc_gen_playlist) qui renvoie deja
//   vers l'utilitaire RecalboxDMD_tool sur PC pour le meme type de cas
//   (volume important). Alignee sur la meme formulation/le meme outil.
//
// v54 — 2026-08-10 — safe-modify — Retour de la generation de playlist a
//   une machine a etats dans loop() (playlistGenStep()), retire de sa tache
//   FreeRTOS dediee (playlistGenTask(), introduite v31/2026-07-29). Cause :
//   bissection sur materiel reel (voir memoire projet, plusieurs heures de
//   diagnostic) a identifie cette architecture (playlistGenTask() ET
//   sdAccessMutex, pris a CHAQUE frame GIF affichee meme hors generation)
//   comme le point de bascule d'un deadlock mqttTask/LWIP touchant le
//   fonctionnement NORMAL du DMD (MQTT + affichage GIF continu, pas
//   seulement pendant une generation) -- mecanisme exact non elucide malgre
//   investigation poussee (rien dans ce code ne s'executait pourtant
//   pendant les scenarios qui plantaient), mais la disparition du crash a
//   la revert de cette architecture est reproductible sur plusieurs tests
//   intensifs. Priorite utilisateur explicite : fiabilite MQTT/affichage
//   (coeur du projet) avant confort de generation de playlist (bonus).
//   Toute la logique metier (generation hybride cache+scan, marqueur FULL,
//   embarquement automatique dans le fichier maitre, garde-fous heap,
//   detection de perte d'ecriture) preservee a l'identique -- seul le
//   mecanisme d'execution change (etapes bornees a
//   PLGEN_MAX_FILES_PER_STEP=20 fichiers par appel de loop(), au lieu d'une
//   tache separee). playlistGenTask()/scanFoldersToPlaylistFile()/
//   PlaylistGenRequest retires, remplaces par playlistGenStep()/
//   PlGenScanState (nouvel etat persistant g_plGenScan). sdAccessMutex ET
//   plGenStatusMutex retires entierement (plus aucun acces concurrent
//   possible, tout tourne desormais dans loop()) -- voir aussi
//   RecalBox_DMD.ino (gifPlayFrameCompat()/openNextGif(), revenus a leur
//   forme d'origine sans mutex). Compromis assume : l'affichage GIF/le
//   serveur web peuvent etre legerement moins fluides pendant une
//   generation ACTIVE (action rare, declenchee manuellement) qu'avec la
//   tache dediee -- comportement acceptable et explicitement valide par
//   l'utilisateur. Pas encore teste sur materiel reel.
//
// v53 — 2026-08-07 — safe-modify — Fix bug signale par l'utilisateur suite
//   au test reel du v52 : l'overlay "Chargement en cours..." ne s'affichait
//   que sur la page Playlist, et meme la, juste avant la fin du chargement
//   au lieu de juste apres le clic sur l'onglet -- laissant plusieurs
//   secondes de blanc/vide faisant penser a un crash. Cause : ce sont 6
//   pages HTML separees avec navigation classique (<a href="/config/...">),
//   pas une SPA -- "premier element du <body>" ne s'affiche que quand le
//   navigateur commence a peindre le NOUVEAU document, ce qui depend du
//   temps de transfert reseau de cette page, jamais du moment du clic.
//   Fix : nouvelle fonction showPageLoadingOverlay() + onclick sur tous
//   les liens de navigation interne des 6 pages (menu + basic/network/
//   clock/media) -- affichage instantane sur la page SOURCE (deja
//   chargee), independant du reseau, avant que la navigation ne parte.
//   Le mecanisme existant (masquage via hidePageLoadingOverlay() en fin
//   de bootstrap sur la page de destination) reste inchange.
//   PAS ENCORE teste sur mobile reel.
//
// v52 — 2026-08-05 — safe-modify — Overlay "Chargement en cours..." sur les
//   6 pages (demande utilisateur : ~3s d'attente sur mobile avant
//   affichage, impression de plantage/envie de F5). <div
//   id="pageLoadingOverlay"> pose en tout premier enfant de <body>, avant
//   tout script -- visible des que le navigateur commence a peindre la
//   page, meme fond #1a1a2e que le body reel (pas de flash de couleur).
//   Masque via hidePageLoadingOverlay(), accrochee en .finally() sur la
//   chaine de bootstrap existante (fetch('/lang')...) de chacune des 6
//   pages, succes ET echec. Couvre "avant la premiere page" (deja present
//   dans le tout premier octet de <body>) et "entre chaque page" (chaque
//   page recharge son propre overlay). Compilation via compile.ps1 : OK.
//   JS revalide (node --check) sur les 6 pages. PAS ENCORE teste sur
//   mobile reel.
//
//
// v51 — 2026-08-05 — safe-modify — Fusion dev/tous-txt-filter -> master.
//   Voir plus bas pour le detail complet (chaine v51...v32 ci-dessous,
//   apportee par dev/tous-txt-filter) et l'entree "v31 (branche master,
//   2026-08-01)" plus bas pour le detail du travail propre a master
//   fusionne ici (nettoyage playlists apres suppression de dossier).
//
// v51 — 2026-08-05 — safe-modify — Volet web de la refonte premier
//   demarrage/AP/mode config (voir RecalBox_DMD.ino v51 pour le volet
//   firmware/ecran physique). Resume :
//   1. first_boot n'est plus efface par le simple affichage d'une page
//      (clearFirstBoot() retiree de triggerWebConfigModeSoft() et du
//      duplicata upload) -- ne passe a 0 que dans handleWebConfigSave() si
//      playlist ET IP Recalbox sont non vides au moment de la sauvegarde.
//   2. Modale d'accueil (checklist + aide detaillee par fonctionnalite) +
//      lien "Aide" permanent, dupliques sur les 5 pages (MENU/BASIC/
//      NETWORK/CLOCK/MEDIA), exposee via first_boot dans /lang. Gate
//      sessionStorage (dmd_help_seen) pour n'afficher qu'une fois par
//      onglet -- bug corrige : elle reapparaissait a chaque navigation.
//   3. Bouton MEDIA "Enreg. & Redemarrer" (relabelage, comportement AJAX
//      deja immediat inchange) -- manquait par rapport aux 3 autres pages.
//   4. checkEssentialFields() (WiFi/playlist/IP Recalbox) avant
//      doReboot()/dmdResume()/saveAndReboot() -- confirm() listant les
//      champs vides, jamais bloquant (annulable).
//   5. Bug corrige : language= disparaissait de config.ini a chaque
//      sauvegarde BASIC/NETWORK/CLOCK/MEDIA (handleWebConfigSave() ne la
//      reemettait jamais, contrairement a first_boot=) -- meme valeur RAM
//      (uiLanguage) desormais toujours reecrite.
//   6. Brouillon localStorage multi-pages (dmd_draft_basic/_network/_clock)
//      -- 1er essai (window.onbeforeunload sur _formDirty) EXPLICITEMENT
//      REJETE par l'utilisateur ("pas d'alerte bloquante, un vrai
//      correctif"). Remplace par une vraie persistance cote navigateur :
//      chaque champ modifie est ecrit dans localStorage, relu et applique
//      PAR-DESSUS les valeurs serveur au chargement de chaque page -- plus
//      aucun champ perdu en naviguant entre pages, sans le moindre
//      avertissement. La sauvegarde reelle (config.ini) continue de
//      n'avoir lieu que sur clic explicite Enregistrer/Enreg.&Redemarrer,
//      qui efface alors le brouillon.
//   Compilation via compile.ps1 : OK (64% flash, 28% RAM). JS revalide
//   (node --check) sur les 6 pages a chaque etape. Test materiel reel EN
//   COURS (2026-08-05) : messages d'alerte web + DMD + modale d'aide
//   confirmes OK ; reste (brouillon localStorage, alerte champs essentiels,
//   fix language=, indicateurs rouge/orange) PAS ENCORE teste.
//
// v50 — 2026-08-03 — safe-modify — setTimeout(3000) (v49) confirme SANS
//   EFFET par lecture du code source NetworkClient.cpp : write() envoie via
//   send(..., MSG_DONTWAIT), qui ignore purement SO_SNDTIMEO -- le vrai
//   blocage (10-25s, escalade mesuree 10021/18244/24894ms) vient d'une
//   boucle de retry codee en dur dans la bibliotheque (10 x select() 1s,
//   RESET a chaque octet transmis), non configurable depuis ce sketch.
//   sendGzipHtml() reecrit : envoi manuel par blocs de 1024 octets (en-tete
//   HTTP construit a la main + webServer->client().write() en boucle) au
//   lieu d'un seul appel send_P() sur toute la page -- un bloc qui echoue
//   COMPLETEMENT (write() renvoie moins que demande) est detecte des ce
//   premier bloc perdu, connexion coupee immediatement au lieu de laisser
//   la lib s'acharner sur le reste. Plafonne le pire cas a ~10s au lieu de
//   18-25s. Demande explicite utilisateur (piste "grossir les paquets pour
//   mobile") verifiee et infirmee au passage : send_P() envoie deja TOUTE
//   la page en un seul write(), aucune "taille de paquet" a agrandir de ce
//   cote -- c'est la segmentation TCP/MSS, hors de portee du sketch.
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v49 — 2026-08-03 — safe-modify — Cause racine CONFIRMEE par
//   l'instrumentation v48 (test reel iOS) : send_P() bloque 10 a 25
//   SECONDES en escalade (10021ms -> 18244ms -> 24894ms) sur des envois de
//   quelques Ko, heap libre qui degringole par paliers pendant que maxalloc
//   reste fige -- signature d'une connexion abandonnee par le client (iOS
//   retente, le serveur mono-thread reste coince a ecrire sur l'ancienne
//   connexion). Fix : webServer->client().setTimeout(3000) avant send_P()
//   dans sendGzipHtml() -- aucun timeout d'ecriture n'etait pose sur ce
//   chemin (contrairement a l'upload). Compilation via compile.ps1 : OK.
//   PAS ENCORE teste sur materiel reel.
//
// v48 — 2026-08-03 — safe-modify — Test reel iOS Safari (setNoDelay v47 pas
//   suffisant) : nouvelle donnee cle -- meme les pages qui REUSSISSENT
//   mettent 10s+ a s'afficher (pas seulement celles qui echouent), motif
//   degressif (MENU/BASIC ok mais lents, puis tout echoue en boucle a partir
//   de NETWORK/CLOCK/MEDIA). Heap toujours sain sur les requetes mesurables
//   (maxalloc=8692). Piste non-firmware ajoutee (Private Relay iCloud+ /
//   permission "Reseau local" iOS, connue pour introduire des delais
//   massifs ou des echecs sur du trafic IP locale) -- a verifier cote
//   utilisateur. Instrumentation ajoutee dans sendGzipHtml() : chrono
//   dedie autour de send_P() (ecriture TCP bloquante) + log heap juste
//   avant, sur CHAQUE envoi de page (pas seulement le repli memoire faible
//   existant) -- objectif : distinguer un envoi reseau reellement lent
//   (send_P() long) d'un ralentissement situe ailleurs (acceptation de
//   connexion, cote client). Compilation via compile.ps1 : OK. PAS ENCORE
//   teste sur materiel reel.
//
// v47 — 2026-08-03 — safe-modify — Test reel iOS Safari (question
//   utilisateur) : page blanche a repetition, "Safari ne peut pas ouvrir la
//   page... connexion reseau perdue" -- meme sur MENU (page la plus legere,
//   6.7 Ko gzip), pas specifique a MEDIA. Log serie confirme la requete
//   atteignant bien le serveur (triggerWebConfigModeSoft() s'execute a
//   chaque tentative) et le garde heap de sendGzipHtml() (maxalloc<4096) ne
//   se declenchant pas -- la coupure n'est donc pas expliquee par ce
//   garde-fou existant. Hypothese testee : Nagle + mode economie d'energie
//   WiFi mobile (voir commentaire dans sendGzipHtml()). Fix applique :
//   webServer->client().setNoDelay(true) avant l'envoi de chaque page.
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel --
//   necessite un vrai test iOS pour confirmer/infirmer cette hypothese.
//
// v46 — 2026-08-03 — safe-modify — Analyse .har + log serie reels (question
//   utilisateur : la generation de playlist simple genere des erreurs web
//   pendant le scan de gros dossiers hors cache -- le reboot cible
//   aiderait-il ?). Diagnostic : NON, pas de la fragmentation heap -- le
//   .har montre la quasi-totalite des requetes /generate-playlist-status
//   echouant a EXACTEMENT ~9000-9016ms (timeout client, pas une erreur
//   serveur) sur un dossier de 1400+ fichiers (Arcade), pendant que le log
//   serie confirme le scan progressant normalement en parallele (aucun heap
//   critique). Le commentaire existant affirmant que playlistGenTask()
//   (tache FreeRTOS dediee, 2026-07-28) avait elimine ce risque est donc
//   FAUX en pratique sur un tres gros dossier -- cause exacte non
//   identifiee (le code cede la main via vTaskDelay(1) et ne garde aucun
//   mutex longtemps, ca semble suffisant en lecture statique). Correctif
//   applique (option choisie par l'utilisateur -- rapide, faible risque) :
//   AbortController du polling /generate-playlist-status remonte de 9000ms
//   a 25000ms, meme marge que le polling d'upload. N'accelere pas le scan
//   (toujours limite par la degradation FAT32 deja documentee sur ce
//   projet), reduit seulement les faux "echecs" affiches pendant qu'il
//   tourne encore. Compilation via compile.ps1 : OK. PAS ENCORE teste sur
//   materiel reel.
//
// v45 — 2026-08-03 — safe-modify — Bug reel confirme (test materiel : "DMD
//   bloque, aucun affichage web/DMD/serial" apres un reboot cible declenche
//   par /prepare-upload) : le JS de uploadGif() faisait location.reload()
//   une fois le serveur revenu -- une navigation/rechargement de page
//   detruit les objets File du navigateur correspondant aux fichiers
//   selectionnes par l'utilisateur, abandonnant silencieusement l'upload en
//   cours sans aucune indication (d'ou "rien ne s'affiche" : la page MEDIA
//   fraichement rechargee est juste... vide de toute activite, en attente
//   d'un nouveau clic sur Uploader que rien ne suggerait de refaire).
//   Corrige : plus de reload, on attend juste (poll sur /lang, endpoint
//   leger qui ne re-arme pas webDmdPause()) que le serveur reponde de
//   nouveau puis on POURSUIT la meme fonction JS avec les memes fichiers
//   deja en memoire -- upload repris automatiquement, aucune perte de
//   selection, aucun reclic requis.
//   Precision demandee par l'utilisateur : masquer la liste de dossiers/
//   suppression pendant la copie (idee initiale evoquee) ne liberait AUCUNE
//   RAM cote ESP32 (uniquement cosmetique navigateur, cf. discussion) --
//   ABANDONNEE sur cette base, aucun changement d'affichage fait ici.
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v44 — 2026-08-02 — safe-modify — Demande explicite utilisateur : le reboot
//   cible mode config (v43) ne doit se declencher qu'au clic sur "Uploader"
//   (page MEDIA), pas a l'ouverture de N'IMPORTE QUELLE page de config --
//   ouvrir MEDIA pour juste supprimer un dossier, ou BASIC/NETWORK/CLOCK
//   pour un reglage, ne justifie pas un reboot. Aucune preuve par ailleurs
//   que l'ecriture de playlists souffre du meme plafond heap que l'ecriture
//   GIF volumineuse de l'upload (confirme par l'utilisateur : /add-to-
//   playlists-batch, execute en fin de CHAQUE upload, n'a jamais echoue
//   dans les tests recents, y compris en fin de gros lot). `triggerWebConfigMode()`
//   (bool, avec reboot) et `sendRebootingPage()` (page HTML de patience)
//   supprimes -- remplaces par `triggerWebConfigModeSoft()` (jamais de
//   reboot, utilisee par TOUTES les pages + handleDmdOpen + UPLOAD_FILE_START)
//   et une nouvelle route `POST /prepare-upload` (`handleWebConfigPrepareUpload()`)
//   appelee par le JS de la page MEDIA juste avant le premier fichier d'un
//   upload (clic sur "Uploader") : reponse JSON `{"reboot":bool}` au lieu
//   d'une page HTML complete (cet appel part d'une page deja chargee, pas
//   d'une navigation) -- si true, le JS affiche un message d'attente et
//   attend (poll) le retour du serveur avant de recharger toute la page.
//   Compilation via compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v43 — 2026-08-02 — safe-modify — Test reel du garde heap<6000 (v42) :
//   bloquait ~99% des uploads MEDIA des que la playlist avait tourne un
//   moment (maxalloc reste bloque a 4596, jamais de recuperation meme apres
//   le delai(10) de v91) -- ce n'est pas un creux transitoire mais un
//   plancher STABLE, deja documente en memoire projet : chaque SD.open()
//   d'un GIF alloue un buffer setvbuf(4096) jamais recycle proprement,
//   plafonnant durablement le heap. Ce plancher avait deja motive un reboot
//   cible mode config (commit "v88", 2026-07-27), retire ensuite (commit
//   "v93") sur la foi d'une comparaison qui portait en realite sur un autre
//   symptome (nombre de requetes HTTP par upload) -- la cause racine du
//   plafond heap n'a donc jamais ete corrigee. Decision utilisateur
//   (2026-08-02) : reintroduire ce reboot cible sur CE firmware, et traiter
//   la vraie correction de fond (fopen()/setvbuf() statique pour la lecture
//   GIF) separement sur une branche dev dediee. `triggerWebConfigMode()`
//   repasse en `bool` (`false` = reboot deja declenche, reponse deja
//   envoyee via la nouvelle `sendRebootingPage()` -- l'appelant doit
//   s'arreter sans repondre) ; les 6 points d'appel (handleDmdOpen + les 5
//   handlers de page) verifient desormais la valeur de retour. Le point
//   d'appel dans `UPLOAD_FILE_START` (v41) reste volontairement en pause
//   inline SANS passer par `triggerWebConfigMode()` -- un reboot depuis ce
//   callback enverrait sa reponse HTTP en plein milieu du corps multipart
//   entrant, cassant la requete en cours (meme classe de bug que le
//   `send()` premature deja corrige sur ce chemin). Cote .ino : restauration
//   a l'identique de `g_skipPlaylistForConfig`/`force_config_boot`,
//   `g_playlistStartedThisBoot`, et du bloc de boot dedie qui saute
//   entierement la playlist/l'ouverture de GIF quand le flag est pose.
//   `requestReboot` (variable + check dans loop()) etait reste orphelin
//   depuis le retrait de juillet -- reutilise tel quel. Compilation via
//   compile.ps1 : OK. PAS ENCORE teste sur materiel reel.
//
// v42 — 2026-08-02 — safe-modify — Test reel 151 fichiers (v41) : crash
//   DIFFERENT des precedents malgre le try/catch (v40) -- abort() dans
//   lock_init_generic() (newlib), appele depuis fopen()->__sfp() lors de
//   l'ouverture du fichier cible (SD.open() dans UPLOAD_FILE_START),
//   backtrace decodee via addr2line. Cet abort() est un appel direct (assert
//   interne newlib sur l'allocation du mutex de flux FILE*), PAS une
//   exception C++ -- le try/catch de handleWebConfig() ne peut structurel-
//   lement pas l'intercepter. Cause racine identifiee : le garde heap
//   critique (ESP.getMaxAllocHeap()<6000, avec retry-apres-delai(10ms), voir
//   v91 ancien historique) qui refusait proprement l'upload AVANT le
//   SD.open() a ete perdu lors de la refonte multi-fichiers (v34-v40) --
//   plus rien n'empechait d'atteindre SD.open() avec un heap deja au plus
//   bas. Reintroduit a l'identique en tete de UPLOAD_FILE_START, avant toute
//   operation SD (mkdir/exists/open), pour retablir le refus propre a la
//   place du crash. Compilation via compile.ps1 : OK. PAS ENCORE teste sur
//   materiel reel.
//
// v41 — 2026-08-02 — safe-modify — Retour utilisateur : nettement plus
//   d'echecs d'upload qu'avant cette session (151 fichiers sans aucun
//   echec auparavant). Deux corrections : (1) AbortController remonte de
//   12s a 25s -- 12s coupait prematurement des transferts LENTS MAIS QUI
//   AURAIENT REUSSI (jusqu'a ~11s observes sous heap tendu, tres proche de
//   l'ancienne limite), plus court que le timeout serveur lui-meme (15s) ;
//   25s laisse une marge confortable au-dessus des deux. (2)
//   HTTP_UPLOAD_BUFLEN RETIRE completement (retour au defaut bibliotheque,
//   1436) -- sa reduction ralentissait les transferts (plus d'appels
//   SD.write() par fichier) sans benefice net demontre, et le try/catch de
//   handleWebConfig() (v40) rend cette customisation inutile : le crash
//   est desormais sans consequence quelle que soit la taille du buffer.
//   Ajout d'une passe de re-tentative finale (2026-08-02, retour test reel
//   sur un lot de 34 fichiers -- 13 echecs en fin de lot) : apres le lot
//   principal, les fichiers en echec beneficient d'une seconde chance
//   (pause 1.5s puis jusqu'a 3 nouvelles tentatives chacun) une fois le
//   reste du lot termine, le tas ayant eu le temps de se stabiliser un
//   peu. Compilation via compile.ps1 : OK (0 erreur, 63% flash, 28% RAM).
//   PAS ENCORE teste sur materiel reel.
//
// v40 — 2026-08-02 — safe-modify — Retour test reel : descendre
//   HTTP_UPLOAD_BUFLEN a 256 n'ameliore pas clairement les choses (effet
//   contraire possible non anticipe -- bien plus d'appels SD.write() par
//   gros fichier, potentiellement plus de fragmentation cumulee cote
//   bibliotheque SD/FatFs) et ralentit nettement les transferts -- remis a
//   512 (meilleur compromis observe). Nouvelle ligne de defense
//   PRINCIPALE : handleWebConfig() enveloppe desormais
//   webServer->handleClient() dans un try/catch -- les exceptions C++ sont
//   bien compilees dans ce build (confirme : __cxa_throw present dans
//   toutes les traces de crash decodees, jamais genere avec
//   -fno-exceptions), donc l'exception std::bad_alloc levee par
//   WebServer::_parseForm() (operator new() de HTTPUpload echoue sous heap
//   fragmente) devrait desormais etre rattrapee AVANT std::terminate()/
//   abort() -- convertit un crash+reboot complet en simple echec de LA
//   requete en cours, loop() continue normalement. Risque assume, non
//   verifie : la bibliotheque n'est pas concue pour etre interrompue par
//   une exception en cours de route, son etat interne pourrait rester
//   incoherent pour l'appel suivant -- a tester en priorite. Egalement :
//   handleWebConfigUploadFile() reengage desormais le mode config
//   (triggerWebConfigMode()) des le premier octet d'un upload si pas deja
//   actif (g_sdOpInProgress) -- couvre le cas ou le navigateur relance
//   l'upload tout seul apres un crash+reboot SANS recharger de page
//   d'abord, qui laissait sinon MQTT/lecture GIF actifs pendant l'upload.
//   Compilation via compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS
//   ENCORE teste sur materiel reel.
//
// v39 — 2026-08-02 — safe-modify — HTTP_UPLOAD_BUFLEN descendu de 512 a 256
//   (retour test reel : 512 reduisait la frequence du crash sans l'eliminer
//   -- 0 crash sur un run de 15 fichiers, 2 crashs sur le suivant). Ajout
//   AbortController (12s) sur le fetch d'upload JS -- detection d'echec
//   reseau plus rapide que le timeout TCP systeme (15-70s+ mesures dans le
//   .har navigateur), relance une tentative plus tot. Compilation via
//   compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v38 — 2026-08-02 — safe-modify — Retour a l'upload fichier par fichier
//   (abandon du regroupement par paquets de v36, confirme en test reel ne
//   pas reduire la frequence du crash -- celui-ci se produit par fichier
//   traite dans le corps multipart, pas par connexion) -- simplification
//   demandee par l'utilisateur pour isoler proprement l'effet du seul
//   correctif HTTP_UPLOAD_BUFLEN=512 (v37). Ajout d'un log diagnostique
//   temporaire au demarrage (sizeof(HTTPUpload)/HTTP_UPLOAD_BUFLEN) pour
//   confirmer que la redefinition est bien prise en compte par la
//   bibliotheque. Compilation via compile.ps1 : OK (0 erreur, 63% flash,
//   28% RAM). PAS ENCORE teste sur materiel reel.
//
// v37 — 2026-08-02 — safe-modify — Cause racine reelle du crash upload
//   trouvee (le regroupement par lot de v36 ne la corrigeait pas, confirme
//   en test reel -- 2 crashs sur 15 fichiers, frequence inchangee) : lu le
//   source de la bibliotheque WebServer (Parsing.cpp ligne ~496),
//   _currentUpload.reset(new HTTPUpload()) alloue une structure qui
//   EMBARQUE un buffer uint8_t[HTTP_UPLOAD_BUFLEN] -- 1436 octets par
//   defaut, EN UN SEUL BLOC CONTIGU, a CHAQUE fichier rencontre dans le
//   corps multipart (independant du nombre de connexions). #define
//   HTTP_UPLOAD_BUFLEN 512 AVANT le premier #include <WebServer.h> du
//   projet (personnalisation legitime prevue par la bibliotheque via son
//   garde #ifndef, pas un patch de son code source) -- reduit d'environ
//   2.8x la taille de l'allocation critique. Compilation via compile.ps1 :
//   OK (0 erreur, 63% flash, 28% RAM -- inchange, HTTPUpload est allouee
//   dynamiquement, pas globale). PAS ENCORE teste sur materiel reel.
//
// v36 — 2026-08-02 — safe-modify — Fiabilisation de l'upload MEDIA (hors
//   plan cache_master_gifs, suite a analyse .har navigateur pendant la
//   session de test) : cause racine identifiee -- WebServer force
//   "Connection: close" sur CHAQUE reponse (WebServer.cpp de la
//   bibliotheque, non modifiable), donc chaque requete HTTP = une nouvelle
//   poignee de main TCP, individuellement exposee a une perte de paquet
//   SYN/ACK WiFi (net::ERR_CONNECTION_RESET/ABORTED confirmes dans le .har,
//   15-70s de blocage sur l'etablissement de connexion, jamais un
//   ralentissement de traitement serveur). handleWebConfigUploadFile()
//   generalisee pour accepter PLUSIEURS fichiers dans une seule requete
//   multipart (uploadCurName/uploadBatchResults/uploadBatchOkCount,
//   reponse JSON {ok,files:[{name,ok,err}]} au lieu d'un texte simple) ;
//   uploadGif() (JS) regroupe desormais les fichiers par paquets de 4 --
//   moins de poignees de main TCP necessaires pour un meme lot, sans
//   requete unique demesuree (limite la pression heap). Repli automatique
//   sur upload fichier-par-fichier (methode individuelle deja fiable) si un
//   paquet echoue au niveau reseau apres 3 tentatives -- reuploader un
//   fichier deja reussi est sans consequence (idempotent). Compilation via
//   compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS ENCORE teste sur
//   materiel reel.
//
// v35 — 2026-08-02 — safe-modify — Retours test reel sur Partie A :
//   (1) le redemarrage apres suppression de dossier(s) lie(s) a des
//   playlists n'est plus automatique -- popup confirm() oui/non
//   (msg_confirm_reboot_playlists, remplace msg_folders_deleted_reboot)
//   laisse l'utilisateur choisir le moment ; bloquant par nature, empeche
//   aussi toute autre action pendant que la decision est en attente.
//   (2) Cache sessionStorage partage entre les pages Affichage et MEDIA
//   (cle 'dmd_gifdirs_cache', readDirsCache()/writeDirsCache()) pour la
//   liste des dossiers /gifs -- demande utilisateur : le va-et-vient
//   frequent entre les deux pages redemandait /lsgifdirs a chaque fois,
//   avec le risque d'echec reseau deja documente cette session. Affichage
//   immediat depuis le cache si present, rafraichissement en arriere-plan
//   qui remet le cache a jour ensuite (jamais bloquant sur le reseau).
//   Compilation via compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS
//   ENCORE reteste sur materiel reel.
//
// v34 — 2026-08-02 — safe-modify — Portage de la Partie A du plan
//   "cache_master_gifs" (jusque-la seulement sur master) dans ce worktree
//   dev/tous-txt-filter, pour permettre de tester A+B+C ensemble sur le
//   meme firmware pendant la session de test materiel en cours : nouvelle
//   fonction stripDeletedFoldersFromPlaylist() (reutilise le
//   writeBufChecked() deja present ici pour Partie B) branchee dans
//   handleWebConfigDeleteFolders() -- chaque suppression de dossier retire
//   desormais les lignes mortes des playlists concernees (cache_master_gifs.dat
//   exclu de ce nettoyage, jamais lu playlist par playlist a la lecture DMD)
//   et supprime leurs compagnons .cache/.sig/.idx. Cote JS (page MEDIA,
//   deleteSelected()) : message explicite puis redemarrage automatique
//   (doReboot(true)) si des playlists ont ete mises a jour. Nouvelles cles
//   i18n FR/EN/ES : msg_folders_deleted_reboot. Compilation via
//   compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS ENCORE teste sur
//   materiel reel (portage identique au code deja teste sur master, mais
//   jamais verifie sur CE worktree precis).
//
// v33 — 2026-08-02 — safe-modify — Retrait du compte de fichiers par
//   dossier (page Affichage), a titre de test suite a un crash reel
//   out-of-memory (abort() dans WebServer::_parseForm(), heap epuise
//   pendant un upload) observe en session de test materiel -- tentative
//   d'isoler si le scan de TOUS_MASTER_PATH dans handleWebConfigListGifDirs()
//   (+ le tableau static String dirNames[128]) et le nouvel endpoint
//   /lsgifdircount contribuaient a la pression heap ambiante. Retour a la
//   version simple de /lsgifdirs (liste de noms uniquement) ;
//   handleWebConfigGifCountFolder()/route /lsgifdircount retires ;
//   loadGenDirs() (JS) revient a un affichage sans compte, tri alphabetique
//   CONSERVE (pur JS, aucun cout heap firmware). Reste du plan (Partie
//   B hybride/marqueur FULL, Partie C etiquette SD) inchange. Compilation
//   via compile.ps1 : OK (0 erreur, 63% flash, 28% RAM -- variables
//   globales legerement reduites, 94444 vs 96532 octets, coherent avec le
//   retrait du tableau static). PAS ENCORE reteste sur materiel reel.
//
// v32 — 2026-08-01 — safe-modify — Partie B du plan "cache_master_gifs"
//   (simplification radicale, apres une longue serie de bugs reels trouves
//   en test materiel sur tousSyncTask()) : RETRAIT COMPLET de
//   tousSyncTask()/resync incrementale (struct ChangedFolderInfo,
//   fnv1aString(), defines TOUS_SYNC_MAX_*, handleWebConfigResyncTous(),
//   route /resync-tous, bouton "Resynchroniser l'index GIFs" + i18n
//   FR/EN/ES associes, champs isResync/foldersChanged/linesAdded/
//   linesRemoved de PlaylistGenStatus) -- elimine du meme coup la limite
//   des 1024 fichiers/dossier (n'existait que dans le code retire).
//   REMPLACE par une generation de playlist HYBRIDE :
//   handleWebConfigGeneratePlaylist() verifie desormais PAR DOSSIER (pas
//   globalement) la presence dans cache_master_gifs.dat (corrige au passage
//   un bug ou cocher un dossier neuf a cote de dossiers en cache produisait
//   une playlist silencieusement incomplete), filtre la portion deja en
//   cache (filterMasterIntoFile(), quasi instantane) et ne scanne que les
//   dossiers neufs (scanFoldersToPlaylistFile(), inchangee) -- qui sont
//   ensuite EMBARQUES AUTOMATIQUEMENT dans le fichier maitre
//   (appendMatchingLines(), plus besoin de rescanner /gifs/). Marqueur
//   "# FULL:dossier1,dossier2" ecrit en tete de chaque playlist generee par
//   le DMD (toujours des dossiers entiers) : handleWebConfigAddToPlaylists
//   Batch() le lit desormais pour decider si un nouveau fichier uploade
//   doit y etre ajoute (plus precis que l'ancien fileContainsNeedle() seul,
//   qui aurait pu polluer une playlist hybride cree cote PC -- retrocompat
//   totale pour les playlists sans marqueur). cache_master_gifs.dat est
//   desormais une cible d'ajout INCONDITIONNELLE lors d'un upload (avant :
//   seulement s'il referencait deja le dossier). /lsgifdirs renvoie
//   {name,count} (compte depuis le cache, "?" si dossier jamais vu) +
//   nouvel endpoint /lsgifdircount?dir= (compte exact d'UN SEUL dossier, a
//   la demande -- jamais /lsgiffiles, retiree v92, jamais reintroduite) ;
//   page Affichage : tri alphabetique + affichage/rafraichissement du
//   compte a la coche. Compilation via compile.ps1 : OK (0 erreur, 63%
//   flash, 29% RAM). PAS ENCORE teste sur materiel reel -- chantier volumineux,
//   tester en priorite : generation cache-seul, generation hybride
//   (cache+scan), premiere generation jamais lancee (bootstrap), upload
//   vers dossier neuf, playlist hybride creee cote PC (marqueur # FULL:
//   absent cote outil PC pour l'instant, session separee a prevoir).
//
// v31 — 2026-07-29 — safe-modify — BRANCHE DEV (dev/freertos-playlist-scan) :
//   playlistGenStep() (machine a etats appelee depuis loop()) remplacee par
//   playlistGenTask(), tache FreeRTOS dediee (cf. RecalBox_DMD.ino v37) --
//   un scan de dossier lent (Arcade/Consoles/Halloween/Vertical_DMD, confirme
//   plusieurs secondes/fichier par moments) ne bloque plus la page web ni le
//   bouton Arreter. Tout acces SD encadre par sdAccessMutex, non-bloquant
//   cote loop()/lecture GIF, bloquant cote tache. Bugs materiels reels
//   trouves et corriges pendant cette session : creation de tache jamais
//   verifiee (echec silencieux si heap insuffisant), pile 8192 trop grande
//   ramenee a 4096, un acces SD (forceDeleteFile sur arret demande) hors
//   mutex -- seul crash reel observe, corrige. Ajout d'un garde-fou heap
//   critique (ESP.getMaxAllocHeap() < 4096 -> arret propre au lieu d'un
//   abort()) suite a un second crash identique en scan normal (fragmentation
//   heap sur un tres long scan), avec message distinct cote utilisateur
//   ("memoire insuffisante" vs "annulee"). Bouton Arreter (JS) rendu robuste
//   (retry 3x) apres un cas reel de requete perdue laissant le bouton
//   desactive sans effet. Cache par dossier avec peremption par mtime
//   ESSAYE puis ABANDONNE le meme jour : 4 bugs reels trouves d'affilee
//   (descripteurs simultanes -> abort() fopen()/lock_init_generic(), dossier
//   modifie pendant son enumeration -> 0 fichier trouve, flush incrementaux
//   perdant des fichiers, comptages erratifs persistant meme apres passage
//   en RAM-only) -- le dernier test reel a confirme que le probleme venait
//   de ce code de cache lui-meme (pas de la creation/destruction repetee de
//   tache, hypothese testee et infirmee via une tache persistante puis
//   revertee). Retire entierement : playlistGenTask() revenue a un scan
//   direct simple, sans aucun cache par dossier. A remplacer eventuellement
//   par une approche filtrage-de-texte sur un TOUS.txt tenu a jour (evite
//   toute re-enumeration de /gifs/<dossier>), pas encore concue en detail.
//
// v31 (branche master, fusionnee 2026-08-05) — 2026-08-01 — safe-modify —
//   Partie A du plan "cache_master_gifs" (nettoyage des playlists apres
//   suppression de dossier, plan valide plusieurs sessions plus tot, jamais
//   implemente jusqu'ici) : nouvelle fonction
//   stripDeletedFoldersFromPlaylist() (+ portage de writeBufChecked(),
//   retries sur ecriture SD partielle) branchee dans
//   handleWebConfigDeleteFolders() -- chaque suppression de dossier
//   reellement effectuee retire desormais les lignes mortes de TOUTES les
//   playlists qui le referencaient, supprime leurs compagnons
//   .cache/.sig/.idx, et la reponse HTTP l'indique. Cote JS (page MEDIA,
//   deleteSelected()) : si la reponse signale des playlists mises a jour,
//   affiche un message explicite puis enchaine automatiquement sur un
//   redemarrage (doReboot(true), skip confirm) -- necessaire car la session
//   de lecture en cours a deja son cache playlist charge en RAM et n'est
//   jamais corrigee a chaud (limitation assumee). Nouvelles cles i18n
//   FR/EN/ES : msg_folders_deleted_reboot. Compilation via compile.ps1 :
//   OK (0 erreur, 62% flash, 28% RAM). PAS ENCORE teste sur materiel reel
//   au moment de cette entree -- voir plus haut (v51) pour l'etat de test
//   materiel le plus recent.
//
// v30 — 2026-07-23 — safe-modify — Bug confirme (retour utilisateur :
//   "recalbox_ip disparu du config.ini") : handleWebConfigSaveAP() faisait
//   encore SD.remove("/config.ini") puis ne reecrivait que 6 cles WiFi --
//   EXACTEMENT le meme bug deja corrige le 2026-07-21 (voir memoire
//   projet), reintroduit par la refonte multi-pages qui repartait d'un
//   instantane anterieur a ce fix. Chaque passage par la page AP (premier
//   boot ou mode secours WiFi) effacait donc recalbox_ip/playlist/
//   brightness/clock_*/language -- expliquant que les scripts Recalbox
//   (MQTT, installes et fonctionnels cote firmware/GitHub) semblaient ne
//   "rien faire" : le DMD n'avait plus l'IP pour se connecter au broker
//   MQTT de la Recalbox. Fix : re-applique le patch cle-par-cle
//   (writeConfigFlag()) au lieu du remove+rewrite complet -- preserve
//   desormais toutes les autres cles. Compilation verifiee OK (62%
//   flash). PAS ENCORE reteste sur materiel reel.
//
// v29 — 2026-07-23 — safe-modify — Parite fonctionnelle page fractionnee
//   vs ancienne page unique (retour test reel utilisateur), sur les 4
//   pages BASIC/NETWORK/CLOCK/MEDIA (backend deja intact, tous les
//   handlers /save /lsgifdirs /generate-playlist /delete-playlist /upload
//   etc. existaient toujours -- uniquement le FRONTEND avait regresse) :
//   (1) Bug SSID non recupere du config.ini corrige : scanWiFi() faisait
//   sel.innerHTML='' puis reconstruisait la liste depuis /scan-wifi SANS
//   jamais re-selectionner le SSID sauvegarde (charge juste avant par
//   loadConfig() puis efface par le scan) -- nouvelle variable savedSsid
//   memorisee avant le scan, re-selectionnee dans la liste scannee (ou
//   ajoutee en option separee si absente du scan).
//   (2) Barre de navigation permanente (topnav, 4 liens) ajoutee en haut
//   des 4 pages -- demande utilisateur, remplace le simple lien "Retour
//   au menu".
//   (3) Actions de bas de page manquantes restaurees sur les 4 pages :
//   Enregistrer / Enregistrer & Redemarrer / Redemarrer (/reboot) /
//   Reprendre DMD (/dmd-resume) -- absentes de la refonte, seul un simple
//   bouton "Enregistrer" existait.
//   (4) CLOCK : clock_theme et clock_tz etaient des <input> texte brut
//   (l'utilisateur devait connaitre les numeros de theme/codes POSIX) --
//   remplaces par les <select> complets de l'ancienne page (10 themes
//   nommes, liste pays/UTC).
//   (5) BASIC : ajout suppression de playlist (existait dans l'ancienne
//   page, absente de la refonte). MEDIA : ajout boutons "Tout/Rien
//   selectionner" pour les dossiers de la playlist generee.
//   (6) Esthetique alignee sur l'ancienne page (sections cartes, h2 avec
//   bordure, boutons colores par fonction) au lieu du style plat minimal
//   de la refonte -- retour utilisateur "esthetiquement moins beau".
//   Tailles apres regeneration gzip : BASIC 2345, NETWORK 2436, CLOCK
//   2581, MEDIA 2567 octets (AP inchangee, 3102) -- toutes tres en-dessous
//   du seuil ~12.5 Ko a risque (voir v28). Compilation verifiee OK (62%
//   flash, +4 Ko negligeable). PAS ENCORE teste sur materiel reel.
//
// v28 — 2026-07-23 — safe-modify — Reintegration multilingue (page AP
//   uniquement, decision utilisateur) suite au constat v27 : le passage a
//   l'architecture multi-pages (session anterieure) avait ete fait a
//   partir d'une base predatant TOUT travail i18n (meme l'ancien systeme
//   navigateur-only), pas seulement mon integration backend -- les 6
//   nouvelles pages etaient 100% francais en dur, y compris WEB_CONFIG_AP_HTML.
//   Reconstruit entierement sur cette page (dict AP_I18N fr/en/es inline,
//   data-i18n/data-i18n-placeholder, tr()/applyLang()/setLang(),
//   selecteur #langSelect) -- PAS sur les 5 autres pages (MENU/BASIC/
//   NETWORK/CLOCK/MEDIA), volontairement laissees en francais pour
//   l'instant (portee reduite, decision utilisateur : ce chantier sur les
//   6 pages aurait ete disproportionne). Priorite de langue : localStorage
//   > config.ini (nouvelle route GET /lang) > navigator.language > 'fr'.
//   setLang() persiste immediatement via POST /save-language (nouvelle
//   route, valide fr/en/es, writeConfigFlag("language", lang) + met a jour
//   uiLanguage en RAM). Nouveau extern String uiLanguage (variable definie
//   dans RecalBox_DMD.ino, forward-declaree ici comme les autres globals
//   partages). Page AP : 4969->8712 octets HTML, gzip 1995->3102 octets --
//   reste tres en-dessous du seuil ~12.5KB souponne d'etre a l'origine des
//   coupures reseau ayant motive le fractionnement (voir note v27).
//   Compilation verifiee OK (62% flash, +2KB negligeable).
//
// v27 — 2026-07-23 — safe-modify — Fix erreur de compilation reelle :
//   triggerWebConfigMode(const String&) est appelee par handleDmdOpen()
//   avant sa definition (celle-ci n'apparait que plus bas, juste avant
//   handleWebConfigRoot() qui l'utilise aussi) -- "not declared in this
//   scope". Fix : forward declaration ajoutee avant handleDmdPause()/
//   handleDmdOpen(). NOTE : le passage a l'architecture multi-pages
//   (WEB_CONFIG_MENU/BASIC/NETWORK/CLOCK/MEDIA_HTML, routes /config/*) est
//   deja present dans ce fichier au moment de ce fix mais n'a jamais ete
//   documente dans ce changelog (toujours v26) -- vraisemblablement une
//   session anterieure. Voir memoire projet pour la remise en etat en
//   cours (travail multilingue backend -- uiLanguage/route /lang/
//   /save-language -- absent de cette architecture, a reintegrer).
//
// v26 — 2026-07-14 — Integration horloge: 10eme theme "Level 1-1" dans le dropdown (+ i18n
//   FR/EN/ES). Section couleur Neon refaite: un seul champ couleur "clock_neon_color" + case
//   "Personnalisee" (clock_neon_color_enabled) remplacent les 2 selecteurs clock_neon_color1/2 -
//   cle config.ini CLOCK_COLOR remplace CLOCK_NEON_COLOR1/CLOCK_NEON_COLOR2.
// v25 — 2026-07-13 — Fuseau horaire: select pays/UTC-5..+5 au lieu du champ texte POSIX
//   (valeur = code POSIX injecte directement, aucun changement backend). Nouvelle case
//   "Demarrage silencieux" (section Affichage) <-> variable showInfo (inversee: cochee = info=0).
// v24 — 2026-07-02 — lsgifdirs retourne name+count (rapide, pas de string building). Tooltip liste fichiers chargé au survol via /lsgiffiles?dir=
// v20 — 2026-07-02 — showMsg push sur DMD (couleur succes/échec), msg_welcome "WEB DMD CONFIG"+IP, défilement DMD
// v19 — 2026-07-02 — Fix: deleteFolderRecursive chemin relatif (f.name() sans path), forceDeleteFile/rmdir avec fullPath
// v18 — 2026-07-02 — Page web ouverte = DMD en mode attente avec msg permanent, plus de resume auto, shutdown clock
// v17 — 2026-07-02 — SUPPRIME /gifcount + async count (causait freeze SD SPI au refresh page), test msg chargement supprime
// v16 — 2026-07-02 — Fix: delay(1) dans toutes les boucles SD longues — WDT timeout bloquait le resume DMD
// v15 — 2026-07-02 — Fix: refreshPlaylistSelect error visible, delay(1) dans lsplaylists/lsgifdirs/gifcount, async gifcount
// v14 — 2026-07-02 — Fix: lsgifdirs revert noms simples (WDT), /gifcount asynchrone, bouton reboot + i18n
// v13 — 2026-07-02 — Fix: msg flottant (position:fixed), uploadFile reset + uploadSuccess flag, add-to-playlists await, rename trick pour RO FAT32, chemin sous-dossier GIF fixe, delays DMD, rmdir rename fallback
// v12 — 2026-07-02 — Fix: mkdir workaround (RO), forceDeleteFile, msg DMD persistant MODE_BLACK, msg web scrollIntoView, compteur GIFs + tooltip, test msg chargement
// v11 — 2026-07-02 — Fix: ${name}->${0}, nettoyage filename, logs, rmdir fallback, showMsg dans dmdPause
// v10 — 2026-07-02 — Pause DMD pdt operations SD (upload/delete/gen), resume auto
// v9 — 2026-07-02 — Multi-upload, deletion dossiers, dropdown upload, regen playlists auto
// v8 — 2026-07-02 — i18n FR/EN/ES, auto-detect langue navigateur
// v7 — 2026-07-02 — Tooltips sur tous les champs
// ============================================

#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

// Fiabilisation upload (2026-08-02) -- HTTP_UPLOAD_BUFLEN (WebServer.h,
// taille du buffer interne alloue en un seul bloc a chaque fichier par
// WebServer::_parseForm(), cause du crash out-of-memory documente ici
// pendant cette session -- voir memoire projet) a ete redefini a 512 puis
// 256 pour tenter de reduire la frequence du crash. RETIRE (retour test
// reel, retour utilisateur : nettement plus d'echecs qu'avant cette
// session sur de gros lots -- 151 fichiers sans aucun echec auparavant) --
// un buffer plus petit multiplie le nombre d'appels UPLOAD_FILE_WRITE
// (donc de SD.write()) par fichier (~1500 vs ~270 pour 390 Ko a 256 vs
// 1436 octets), ralentissant nettement les transferts sans benefice net
// demontre. Le vrai filet de securite est desormais le try/catch dans
// handleWebConfig() (voir plus bas) : rend cette customisation inutile,
// le crash est maintenant sans consequence (requete en echec, pas de
// reboot) quelle que soit la taille du buffer -- autant garder le defaut
// de la bibliotheque (1436) pour la vitesse.

#include <WiFi.h>
#include <WebServer.h>
#include "web_config_html_gz.h"

extern int    screenBrightness;
extern void   requestClockPreview(const String &arg); // v72, RecalBox_DMD.ino
extern bool   wifiEnabled;
extern String wifiSSID;
extern String wifiPassword;
extern bool   wifiStaticEnabled;
extern String wifiStaticIP;
extern String wifiGateway;
extern String wifiSubnet;
extern String wifiDNS1;
extern String wifiDNS2;
extern bool   bluetoothEnabled;
extern String bluetoothName;
extern bool   showInfo;
extern String playlistName;
extern bool   playlistRandom;
extern String recalboxIP;
extern bool   clockEnabled;
extern int    clockTheme;
extern int    clockIntervalGifs;
extern int    clockIntervalMin;
extern int    clockDuration;
extern String clockTimeZone;
extern bool    clockNeonCustomColor;
extern uint8_t clockNeonR, clockNeonG, clockNeonB;
extern bool   requestReboot;
extern bool   g_playlistStartedThisBoot;
extern bool   g_firstBoot;
extern String uiLanguage;
extern void webDmdPause(const String &msg, uint16_t color = 0xFFFF);
extern void webDmdResume();
extern void webDmdSetMainMsg(const String &msg);
extern void clearFirstBoot();
extern String g_sdOpSubMsg;
extern bool   g_sdOpInProgress;

static WebServer *webServer = nullptr;
static File uploadFile;
static String uploadDir;
static unsigned long uploadStartMs;
static int uploadTotalBytes;
static bool uploadSuccess = false;
static String uploadErrorMsg;
// Fiabilisation upload par lot (2026-08-02, retour test reel + analyse HAR
// navigateur) -- WebServer::send() force TOUJOURS "Connection: close"
// (WebServer.cpp, code de la bibliotheque, non modifiable depuis ce
// projet) : chaque requete HTTP a besoin de sa propre poignee de main TCP.
// Uploader N fichiers = N connexions separees, chacune individuellement
// exposee a une perte de paquet SYN/ACK WiFi (confirme par analyse des
// .har navigateur : net::ERR_CONNECTION_RESET/ABORTED apres 15-70s de
// blocage sur l'etablissement de connexion, PAS un ralentissement cote
// traitement serveur, deja mesure a 2-3ms). Regrouper plusieurs fichiers
// dans UNE SEULE requete multipart (voir handleWebConfigUploadFile()) migre
// autant de cycles START/WRITE/END sur la MEME connexion, reduisant le
// nombre de poignees de main necessaires proportionnellement a la taille du
// lot cote JS (uploadGif()). uploadCurName : nom du fichier de LA PART en
// cours de traitement (utile car un seul UPLOAD_FILE_* callback partage
// pour toutes les parts d'une meme requete). uploadBatchResults : resultat
// JSON accumule au fil des UPLOAD_FILE_END successifs de la requete
// courante, lu et remis a zero par handleWebConfigUpload() (appelee une
// seule fois, apres la derniere part).
static String uploadCurName;
static String uploadBatchResults;
static int uploadBatchOkCount = 0;

// v92 -- bloc WEB_CONFIG_HTML (ancienne page monolithique pre-fractionnement,
// jamais servie par aucun handler depuis le passage aux 6 pages minces)
// retire integralement lors de la reconstruction depuis cette base -- code
// mort, cf. plan de reconstruction.
static const char WEB_CONFIG_MENU_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
.logo-wrap{text-align:center;margin:4px 0 10px}
.logo-wrap img{max-width:100%;height:auto;border-radius:8px}
.tagline{text-align:center;color:#aaa;font-size:13px;margin:0 0 14px}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
.continue{display:none;text-align:center;padding:12px;border-radius:8px;background:#0f766e;color:#ecfeff;font-weight:700;text-decoration:none;margin-bottom:12px}
.menu{display:grid;gap:10px}
.btn{display:block;padding:12px 14px;border-radius:8px;background:#0f3460;color:#eee;font-weight:600;text-decoration:none;text-align:center}
.btn:hover{background:#16478a}
.small{font-size:12px;color:#9ca3af;margin-top:10px;text-align:center}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
#helpLink{position:absolute;top:14px;right:75px;font-size:13px;color:#8ab4f8;text-decoration:underline;cursor:pointer}
.help-backdrop{display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);z-index:1000;align-items:center;justify-content:center;padding:16px}
.help-backdrop.show{display:flex}
.help-box{background:#16213e;border-radius:8px;padding:20px;max-width:520px;max-height:85vh;overflow-y:auto;position:relative;text-align:left}
.help-box h2{color:#ffd146;font-size:16px;margin:0 22px 10px 0}
.help-box h3{color:#8ab4f8;font-size:14px;margin:14px 0 6px}
.help-box p{font-size:13px;line-height:1.5;margin:0 0 8px}
.help-box ul{margin:0 0 8px 18px;font-size:13px;line-height:1.5}
.help-close{position:absolute;top:10px;right:14px;background:none;border:none;color:#aaa;font-size:22px;cursor:pointer;line-height:1}
#pageLoadingOverlay{position:fixed;inset:0;background:#1a1a2e;color:#eee;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:16px;font-weight:600;z-index:99999;text-align:center;padding:20px;gap:14px}
#pageLoadingOverlay .pgspin{width:34px;height:34px;border:4px solid #333;border-top-color:#8ab4f8;border-radius:50%;animation:pgspin .8s linear infinite}
@keyframes pgspin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div id="pageLoadingOverlay"><div class="pgspin"></div><div data-i18n="loading_text">Chargement en cours...</div></div>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<span id="helpLink" onclick="showHelpModal()" data-i18n="help_link">Aide</span>
<div id="helpBackdrop" class="help-backdrop" onclick="if(event.target===this)closeHelpModal()">
<div class="help-box">
<button class="help-close" onclick="closeHelpModal()">&times;</button>
<h2 data-i18n="help_title">Bienvenue</h2>
<p data-i18n="help_intro"></p>
<h3 data-i18n="help_checklist_title"></h3>
<ul>
<li data-i18n="help_check_ip"></li>
<li data-i18n="help_check_playlist"></li>
<li data-i18n="help_check_wifi"></li>
</ul>
<p id="helpUrlReminder"></p>
<h3 data-i18n="help_features_title"></h3>
<ul>
<li data-i18n="help_feat_playlists"></li>
<li data-i18n="help_feat_clock"></li>
<li data-i18n="help_feat_display"></li>
<li data-i18n="help_feat_network"></li>
</ul>
</div>
</div>
<div class="logo-wrap"><img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQQAAAC4CAMAAAAyqWKCAAAAYFBMVEX///8U//n+/v77+/vr6+tw6Nz7lJ/9ixTNqaFwkZSUTlJXOy9EOjk6KSXpEBDmDQ17Dg45Hx45Dg4RUlQPKiodFhYZDxAKEREVBgULBgYFBAQCAwMBAQE4AAAAAAEAAAAgLdlgAAAQH0lEQVR42u2di3aiOhSGU2A0QGhF5RZief+3PPuSQECwdk5nIdW91rRgtWvy9d+X3IjoXtaJF4IXhBeEF4QXhBeEF4QXhBeEH4fw+YIAZvqL9mkhtLNAngmCMaf4zE03XZVtn8K3IbRta6r47cRiMCaWqe7aJ3SH89vb2RgWQhzLuNo4hW9C+DSn07k6gRA6pNB2WYwUim1TEN/MCdXb21sM3nCuOD4qGSugcN50XPiuEtrTKX5DA0WgGFKZdmdQQ7ZlCt+PCS14wxuSOEG721iqtgAIEBfM80CAv/75LT5XZ1QCxkUMCjKOm6fKDq2BuNjakiljAunZPFeKNB0mBywXEAIRaJ+uYkQIZ5cS24w00JrnK5v1+K41T6gEjI7jdhvwjudSgrlcasuia5JE5WXzdEo4fIAdLh1VzcmOLAEWz1MsGUMMwJBCDwEt227/4bt9hwOq4IIkaqybkl1S5ipNEoRgnkQJNTDA7xf8bjoNnkCv57td8SQQDDYeFaBREXBb7nYKUqRu1W7XPBOExoUG8AYQQO5iw7NkB4JwoSoBlaA7EECBSjAQG56nF6khImoXE3SX7nZcPSKEtnui7ADpgbODRi+AEiHNwC3Sp4FgKdg6YVQmqKeB0EIiIBkcagoBOqMSAa1onwJCP+FW12Y04dIUucrpDea3Q0AEhVKqwFCoTS8NvDVFBj+YTs/9OggggzKWkRAijGLsLMHIEhq0Wqs4CuEHkUw2Ogsj7h5ESJCACPCLkEOfsVX8Ov0gUpscZBJ3dh51DBqglgZBGCCGoqgarZWEV+wPQtBDvEUK4j5f0FKELAKyIES3QBPD60ACXt8iBXFfPAAGYmRBMP5OF47Cb4TQdrFjAJIfWo3mcQkthe0V0OIeBir0feHKkEwQJgXEhwBDxubGmMSdAYFbK6IEksQECN/G8NYqAmmEQrYbCwviDiEkzhlCobrBNXoGURxHItNQNhGt7TmE+Do7NhG7PoYA1bYqCMeRQMJSBRUq6FR2pJIgiBpjfhUE7YRALQclJH5OYHU0oIEo65qY8yVKQf8mCFAYR4HVP5QFAEFRdeB5gzJNA04SSRctwkC2v0UJLRoModk6MVJVVcEfWMM3FfVaAHfAnlUEGpAqi+2bwTno41uH4P6WHAhHRdCoeILGJzFWjpF27w4pV2xnnadYZqAkWsYih6Z6M46lnycD6kFGIjbYl8BbUEeGn43VNiiI5QqJ44AKub2UCSUsX4X1anJcK4ShLDody661URTey33LcBuDbmI5MULjoHmqr5i97DCulygEdDoCH8g4WEC0lPhpITeRK8WiEALSdTzURlAdh8q0UEOPi2goEguulKS0snGfC8JNLPMUS9UBNz5Sfv8xYCWE076DSChBBL1EAIJCl9lIxbDkDhj/gwAW5o1DoCwKOe07YPpMMmx0X0ti3oTSKcDvm1VC2xUhVwB60l8aFUrTfrRfSxviKKItLPIUC96gsAKGnsIEQiBEMNuZFpOhBQnVEv+ODUhhSQn0VyxhTiGaZoKFMYVpzxKUUERUNultQjDYdvbnJhK3xlOWhllEpB1J/fhZUixVSgFH9v8BgTPMFsaZxGKC5GGyawhYLlzZ1WBT1HCtsQl/EIsJUkQwx/T3SsDVO9o51QYh2AQZ44yjvgqMYTxjYRBcQ2B/iB5/RZdYGkyi5AbLkSYQQpHM/ZrJwCNAaBGCwmzy+ElSLCdILnPG0y5UAPB4iWduiHX0NtP1SXKDEGA/S+/Lk7knRDM3ctYaau4IQv9pSBRmcxC8BImTT0HoM1hIeLD9w6dgcwJnmeDh/UEs9SA5v1P97DFYbA9OU/lDboqVkIVbSJJz7mBsgqTrqh9svsWAcEXDJI3E9Ioz+ptIkuJGghx8I6B/4Av61gQFDkfSW4XzgD5JthuDAMtT/d4fTkmzxL/aEg0LeiR3pcLhwxRe1IP7g1hMkH3+g6FVuJcwqHSVGyeZsqO3wrjk8M5yC/4gFhJkNHqxyO7e4VPQKrbB6Jc9+Ap4MZsgodCLVeZZkd1rRVHQF7YMC+qHLxrF0hDrzf6RZ+Ft86qG7UDACVjBU8s/YkE/4rghCJQgg68FcK/Z7tRjJ0kxGxJ+2B49KIi/CAnftkcPCi8ILwgvCC8ILwgvCC8I/wQC9h1EeHsB+EsJvx8CrmFM4jBOEnmr2P7dEGjVZgHdo5mVTNuHMPQCg5nL/gpap3UWwTL/OAhn3k2/6PfHBFJC1rnR2N+kBN7phhb0V+Hci4GDkMy9B2Ze8OtGIUT9ICGsbS/KsiqLKpZwhS+VKsqqEgwupYVgGvqJlKX9nMJfIXHJ3wZWLs1DwNX7Ma3vDmmHgyzxHl6U/Aenr3TvlKD620ri6LwKYUE4jTzLx5+JW4AAG+Al+zMs5IVt4gTF4II+8nHIibB1HPZ7ySEmRBpvDc5PK2NgeTiuBu8KudmYEMFzEiTGe5g9ajU/dDGSsA6BIUS46ymO8U1x20J2aFuEwLcFfgbWMIoYJyCKeLMQpFudQauZae+CJL3zgzixMFAoAusOBTGxt22MrVchXKoEd89FG3WHCMQueflqDJc0qyIjd6XwCi7hC8zOKdgJ526ldFcx/wq4z6In7zuEW68Yp5MNc2Wk2yw/uuV32F/xFBXjqxf5gvCC8ILwgvAkEIKfhxBsUAk/bhtTAu7UED9uwbbWJ+BG8fjHTW1s9dq/+e+aza1y1z9u7euAvO51SuALwgvCC8ILwvdOSyB7KWFlRCtDgJqkyHOY6GvWFMP6SpD07Guc6Lt+CGLbVviYL5jbMp9fqWWrEIzuHwC+gwffNjMN4dP46By6pZOIPu8429fcdKs1IcCDCXaeyfGBKQZO4judmMFbrJeakO73CVre/v3TKlaEgA828yHskpEWWjyg1LerMymNrhud7NkOt0RwPIAdwepHg6DLZDc22e+q/vw0upfBm3OJ8TZtOGhkP1iSpulx6QmIh3drx8eCAFvqd7jawTZfoijksJMOHlrwNrXxmZSmqdP92A7XfXargvePdziL4P390DTaPBQEWvHBENyFOzQFTuGL//z587bsEKY77qeWpMfruPDx7hsey3CFQawXFQte3IQK4JUvIwjxG7xkKfyBy6kWdJHury2ZRo3DATUw5mAeRwmmgKUfPoTIgwCbk7HlVgt/eh5VD6nc7/EjrvVAkCDk5VQHHxMG7x+HQz0WzIru0OFKB246rmfgK/tE09bEDAbbPlyNIKAPScegv/ZPIQIdQPKEZg++wEQuj6EEOFENlz3ZpmeZg9C/QcVJbJsuSTGRD6F3JkkCkD6EwZqPDwmLzHYWwscO9JLgDeTLR4AA29J3KruCIJOcD5OpzvxThIBrXsIxBGNK60PY9J1dPyknEDSsuY56CEQklKyFg59H1oOQ7OgvLEfugKcI4QljcEytUokHwSrh3FcCn4o/RA3HdVTuund3eLKNFDbaYMM59Ej2j4eBsBulSOlOXGQIsQ+BhGLrZ/7f56niN1DDfSB+7OXQi39+aHjklttiungcCF6t6Gj0ECgecnagBXMuXbbca9rvFyDsa+8pB5kVkcQkkcQ2uDCE7uEg+GdvOgh/RnWCK5x6CGk6D2GfuGnP40G5H2BaSO3NpiB45hWPPQSqE2yKhJ0nA4S9lYL5eE8XIEBg1JuD4JmNCbY4cHXCLhyu97araA7vh0UI7+5Qs01CqAYIvuGa2wkE6DtCVuSEQIf/ggMRBCgVPqhiMusXS/8DAnx6v2wDBKiPuDSgUvGDNmVQhnRHAf92CIaU8I6jTtxoCwFfpNvHgSCTf6mEj7G5HsSjQUg+7WhzJv8NhL7jNDUYZTLrd6Uz6jVolfBoSrIblc3/C4Lp3WHZvKJx1ZElKXNY0JVj07UuFXlHMYLQX5y+AeG4GQg40IpTDaYtIXQVdvzdVf7VGScc4pODEPc0fhUE2lqlSjp4FGDgUFiewYmc/RgjjSm2RpMOzu0pnoWQp1uG0MBho6nKS93AWfWqgC5ygw8t+/RnXiqccDm5lhONqmVGqaVQmz4SHDcHwZQ59m5UDufPAoSsaOCFNM0at8brDKbZM+AC4gRpwRtvRgqHY6PTg+0xNPdAGFKF14cSqwUEajxvmMLvOd/nZUujxNUZldCazxYuHI1qeEpua1JuLjxamhretPqYcmdqgGBHFS/1DI8HgAAnEKM7sNnvMIeEksADaonB6QzN5gt4vqPGi8r7DXWNE/o8q5tQWvxkHfRnwh8O7gxsywM6DMbxWB9CW3oQsPEDBJQCuMCJKfD3U0+jcu5ijmme1/aI57rGgAqhNT0kpTecfsEGX2CY5XK85rE6BE0QmEKaWgr4FSBoFIJtvDOPBkfGushx8hFkY+oMYgnjgA3ctfbqsfpA7aYRFqwcL/CY7frCPA6rQ2gYQs8gVVYNAKEx1xA8Gtp6E34qP9ZNeczpou004kj92RfTi8SQbzCP+mMy3LwOhJYg9BRQEil7BULQ+haECmNGfSQI0Pjj0eGoS74qhwn+epiOZx48kwsztB/H1UeWtA+hN4ZQNHAE3y0I+EztHgJYanHkecZXpXah80IQCMqUR/O5MgSYVYeS+ZoCZkxYxVXdhgArUtAHbOOdZY4BigIzBZAiBsfjpdY1X18whpiGLtdWQts05RAURhDywkI4L0KAYmEGwmAAQXtCgPZeao+HfflSDzP0YpVuA7rDvBLugqDr2xDKhuVmW45N7i96HsdhguJxIZwXIeivIVDc8CAcr3GgFlaGAE9pwWdBz4QEOMOeIZwXITQEIZtnkAEEbN4CBA9HL4V1YgJDyOchoBKWpIAvVxVDyG9AgAB4B4TaPASEbEYIX0DAn0JyyZekkBMECJ4A4fIFBL0uBAwKUwrZdyAAhSxbgFBA8/Q9EGxZtUpM0JpyJEHIroTQQzgveEOFT3m6CQGkQBC+codGrxoYyR/GFDIfQvG3EDILARj0EPIlCPWKEIyDMFCgJ9bkxRjCeYYBQeh1NA8B+pJsRIBsAQL7w2oxgdphKVjzGFh/OF8z+A6EsnQMLAX/em0IfVAoKMhbBMSgh1BcURgYoD8sQMgcBHQHCp+9Hb1LW0zXvMh3PQgVU+j/mxaBZeCkcJ4wsBAqmx6yGQa4kaamoGAhZPmVOQj1ihBanwKRsN9hHxDYiMIw0jYwsGH1mkLGcfEGhGyQAiqB8oNYa+sTRYWBgmNAEJoxBN+Ygc2wVxSsV5XUvGbiDvhD9jsrhZUhUJK8olA6BssUKh8CSyGbMBhDKHwGSCFnChYCFFVmNQgUFZrKp1A6BrjNvpqnwC9raqDTeo8h6xmQN1jGEwheUChXhmCjAlHwzTFYgGAZWBUVA4U+yeZcbjGEuvSkQK6QTyHwHhCx4o7IZorBMsADKKsZDJWDoEct9AiMhODczY8JV0pYFQJKQVv/L71gAPKkUzh1NcVQDQwmFDxjt7IM7FuKfDZFUkXV6FWVAG21EEamae+fp4WpsVC4hVcUisIXgpPC8fgFhP8A7kK1Ey30vv0AAAAASUVORK5CYII=" alt="RecalBox"></div>
<div class="tagline" data-i18n="tagline">Configuration DMD</div>
<div class="section">
<a id="continueLink" class="continue" href="#"></a>
<div class="menu">
<a class="btn" href="/config/basic" onclick="showPageLoadingOverlay()" data-i18n="menu_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a class="btn" href="/config/network" onclick="showPageLoadingOverlay()" data-i18n="menu_network">&#x1F4F6; Wi-Fi &amp; Bluetooth</a>
<a class="btn" href="/config/clock" onclick="showPageLoadingOverlay()" data-i18n="menu_clock">&#x23F0; Horloge</a>
<a class="btn" href="/config/media" onclick="showPageLoadingOverlay()" data-i18n="menu_media">&#x1F4BF; M&eacute;dias</a>
</div>
<div class="small" data-i18n="small_hint">Page fractionn&eacute;e pour un chargement rapide et fiable sur ESP32.</div>
</div>
<script>
const HELP_I18N={
fr:{help_link:'Aide',help_title:'Bienvenue sur la configuration du DMD',help_intro:'Voici ce qu\'il reste à vérifier avant de sauvegarder, et un résumé de ce que permet cette interface.',help_checklist_title:'À vérifier avant de sauvegarder',help_check_ip:'IP Recalbox renseignée (page Wi-Fi & Bluetooth)',help_check_playlist:'Playlist par défaut renseignée (page Affichage & Playlists)',help_check_wifi:'Le Wi-Fi est déjà validé à ce stade — inutile d\'y retoucher, sauf si vous voulez le changer',help_url_reminder:'Cette page reste accessible à tout moment en tapant l\'IP du DMD dans un navigateur — actuellement {ip}',help_features_title:'Ce que permet cette interface',help_feat_playlists:'GIFs (page Médias) : ajouter des GIFs sur la carte SD (upload direct depuis le navigateur, création de dossiers) — les playlists qui référencent un dossier modifié sont mises à jour automatiquement',help_feat_clock:'Horloge (page Horloge) : thème, couleur néon, intervalle et durée d\'affichage, fuseau horaire',help_feat_display:'Affichage, luminosité et playlists (page Affichage & Playlists) : luminosité de l\'écran, choix entre démarrage silencieux (titre seul) ou normal (IP détectée, synchronisation de l\'heure, etc.), sélection de la playlist par défaut, et création/suppression de playlists à partir des dossiers de GIFs',help_feat_network:'Réseau (page Wi-Fi & Bluetooth) : IP Recalbox (connexion MQTT), Wi-Fi (réseau, mot de passe, IP statique)'},
en:{help_link:'Help',help_title:'Welcome to the DMD configuration',help_intro:'Here is what\'s left to check before saving, and a summary of what this interface lets you do.',help_checklist_title:'To check before saving',help_check_ip:'Recalbox IP filled in (Wi-Fi & Bluetooth page)',help_check_playlist:'Default playlist filled in (Display & Playlists page)',help_check_wifi:'Wi-Fi is already validated at this stage — no need to touch it again, unless you want to change it',help_url_reminder:'This page stays accessible at any time by typing the DMD\'s IP in a browser — currently {ip}',help_features_title:'What this interface lets you do',help_feat_playlists:'GIFs (Media page): add GIFs to the SD card (direct upload from the browser, folder creation) — playlists referencing a modified folder are updated automatically',help_feat_clock:'Clock (Clock page): theme, custom neon color, display interval and duration, time zone',help_feat_display:'Display, brightness and playlists (Display & Playlists page): screen brightness, choice between silent startup (title only) or normal (detected IP, time sync, etc.), default playlist selection, and creating/deleting playlists from GIF folders',help_feat_network:'Network (Wi-Fi & Bluetooth page): Recalbox IP (MQTT connection), Wi-Fi (network, password, static IP)'},
es:{help_link:'Ayuda',help_title:'Bienvenido a la configuración del DMD',help_intro:'Esto es lo que falta comprobar antes de guardar, y un resumen de lo que permite esta interfaz.',help_checklist_title:'A comprobar antes de guardar',help_check_ip:'IP de Recalbox indicada (página Wi-Fi y Bluetooth)',help_check_playlist:'Playlist por defecto indicada (página Pantalla y listas)',help_check_wifi:'El Wi-Fi ya está validado en esta etapa — no hace falta tocarlo, salvo que quiera cambiarlo',help_url_reminder:'Esta página sigue accesible en cualquier momento escribiendo la IP del DMD en un navegador — actualmente {ip}',help_features_title:'Qué permite esta interfaz',help_feat_playlists:'GIFs (página Medios): añadir GIFs a la tarjeta SD (subida directa desde el navegador, creación de carpetas) — las playlists que referencian una carpeta modificada se actualizan automáticamente',help_feat_clock:'Reloj (página Reloj): tema, color neón personalizado, intervalo y duración de visualización, zona horaria',help_feat_display:'Pantalla, brillo y listas (página Pantalla y listas): brillo de la pantalla, elección entre inicio silencioso (solo título) o normal (IP detectada, sincronización horaria, etc.), selección de la playlist por defecto, y creación/eliminación de playlists a partir de las carpetas de GIFs',help_feat_network:'Red (página Wi-Fi y Bluetooth): IP de Recalbox (conexión MQTT), Wi-Fi (red, contraseña, IP estática)'}
};
function showHelpModal(){
  document.getElementById('helpBackdrop').classList.add('show');
  const p=document.getElementById('helpUrlReminder');
  if(p) p.textContent=((HELP_I18N[currentLang]&&HELP_I18N[currentLang].help_url_reminder)||HELP_I18N.fr.help_url_reminder).replace('{ip}',window.location.host);
}
function closeHelpModal(){document.getElementById('helpBackdrop').classList.remove('show');}
const MENU_I18N={
fr:{title:'RecalBox DMD',tagline:'Configuration DMD',menu_basic:'&#x1F4A1; Affichage &amp; Playlists',menu_network:'&#x1F4F6; Wi-Fi &amp; Bluetooth',menu_clock:'&#x23F0; Horloge',menu_media:'&#x1F4BF; Médias',small_hint:'Page fractionnée pour un chargement rapide et fiable sur ESP32.',cont_basic:'&#x1F4A1; Continuer : Affichage & Playlists',cont_network:'&#x1F4F6; Continuer : Wi-Fi & Bluetooth',cont_clock:'&#x23F0; Continuer : Horloge',cont_media:'&#x1F4BF; Continuer : Médias',essential_wifi:'Wi-Fi',essential_playlist:'Playlist par défaut',essential_ip:'IP Recalbox',msg_essential_missing:'Attention : champ(s) essentiel(s) vide(s) : {fields}. Le DMD risque de ne pas fonctionner correctement. Continuer quand même ?',loading_text:'Chargement en cours...',...HELP_I18N.fr},
en:{title:'RecalBox DMD',tagline:'DMD Configuration',menu_basic:'&#x1F4A1; Display &amp; Playlists',menu_network:'&#x1F4F6; Wi-Fi &amp; Bluetooth',menu_clock:'&#x23F0; Clock',menu_media:'&#x1F4BF; Media',small_hint:'Split page for fast, reliable loading on ESP32.',cont_basic:'&#x1F4A1; Resume: Display & Playlists',cont_network:'&#x1F4F6; Resume: Wi-Fi & Bluetooth',cont_clock:'&#x23F0; Resume: Clock',cont_media:'&#x1F4BF; Resume: Media',essential_wifi:'Wi-Fi',essential_playlist:'Default playlist',essential_ip:'Recalbox IP',msg_essential_missing:'Warning: missing essential field(s): {fields}. The DMD may not work correctly. Continue anyway?',loading_text:'Loading...',...HELP_I18N.en},
es:{title:'RecalBox DMD',tagline:'Configuración DMD',menu_basic:'&#x1F4A1; Pantalla y listas',menu_network:'&#x1F4F6; Wi-Fi y Bluetooth',menu_clock:'&#x23F0; Reloj',menu_media:'&#x1F4BF; Medios',small_hint:'Página dividida para una carga rápida y fiable en ESP32.',cont_basic:'&#x1F4A1; Continuar: Pantalla y listas',cont_network:'&#x1F4F6; Continuar: Wi-Fi y Bluetooth',cont_clock:'&#x23F0; Continuar: Reloj',cont_media:'&#x1F4BF; Continuar: Medios',essential_wifi:'Wi-Fi',essential_playlist:'Playlist por defecto',essential_ip:'IP de Recalbox',msg_essential_missing:'Atención: falta(n) campo(s) esencial(es): {fields}. Es posible que el DMD no funcione correctamente. ¿Continuar de todos modos?',loading_text:'Cargando...',...HELP_I18N.es}
};
let currentLang='fr';
// Overlay "Chargement en cours..." (2026-08-05, demande utilisateur :
// ~3s d'attente sur mobile avant affichage, impression de plantage/envie
// de F5). Present des le tout premier octet du <body> (avant tout script)
// donc visible des que le navigateur commence a peindre la page, meme si
// le reste du transfert (page + donnees /lang, /load) prend encore du
// temps -- masque via hidePageLoadingOverlay(), appelee en toute fin de
// la chaine de bootstrap (succes ET echec, voir .finally() plus bas).
function showPageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='flex';}
function hidePageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='none';}
function tr(k){return (MENU_I18N[currentLang]&&MENU_I18N[currentLang][k])||MENU_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&MENU_I18N[stored]){currentLang=stored;}
  else if(backendLang&&MENU_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=MENU_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.getElementById('langSelect').value=currentLang;
  updateContinueLink();
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
const SECTIONS={basic:{url:'/config/basic',key:'cont_basic'},network:{url:'/config/network',key:'cont_network'},clock:{url:'/config/clock',key:'cont_clock'},media:{url:'/config/media',key:'cont_media'}};
function updateContinueLink(){
  const last=localStorage.getItem('dmd_last_section');
  const a=document.getElementById('continueLink');
  if(last&&SECTIONS[last]){a.href=SECTIONS[last].url;a.onclick=showPageLoadingOverlay;a.innerHTML=tr(SECTIONS[last].key);a.style.display='block';}
  else{a.style.display='none';}
}
fetch('/lang').then(function(r){return r.json();}).then(function(d){applyLang(d.language);if(d.first_boot==='1'&&!sessionStorage.getItem('dmd_help_seen')){sessionStorage.setItem('dmd_help_seen','1');showHelpModal();}}).catch(function(){applyLang();}).finally(hidePageLoadingOverlay);
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) :
// aucun moyen fiable de distinguer une vraie fermeture d'un simple
// rafraichissement de page (habitude trop ancree pour l'utilisateur, faux
// positifs trop frequents).
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_BASIC_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Affichage</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
body.gen-busy .topnav a{pointer-events:none;opacity:.4}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
h2{color:#8ab4f8;font-size:15px;margin:0 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.btn-row{display:flex;gap:10px;justify-content:center;margin:18px 0;flex-wrap:wrap}
.btn{padding:12px 20px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#2d6a4f;color:#fff}
.btn-del{background:#555;color:#fff}
.btn-gen{background:#2d6a4f;color:#fff}
.desc{font-size:12px;color:#aaa;margin-bottom:8px}
.dirs{margin:8px 0;max-height:220px;overflow-y:auto}
.dirs label{display:flex;align-items:center;gap:8px;font-size:14px;padding:3px 0}
.dirs label span.name{flex:1}
.mini-row{display:flex;gap:8px;margin-bottom:8px}
.mini-btn{padding:4px 10px;border:none;border-radius:4px;background:#1a6b9e;color:#fff;font-size:11px;cursor:pointer}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
#helpLink{position:absolute;top:14px;right:75px;font-size:13px;color:#8ab4f8;text-decoration:underline;cursor:pointer}
.help-backdrop{display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);z-index:1000;align-items:center;justify-content:center;padding:16px}
.help-backdrop.show{display:flex}
.help-box{background:#16213e;border-radius:8px;padding:20px;max-width:520px;max-height:85vh;overflow-y:auto;position:relative;text-align:left}
.help-box h2{color:#ffd146;font-size:16px;margin:0 22px 10px 0}
.help-box h3{color:#8ab4f8;font-size:14px;margin:14px 0 6px}
.help-box p{font-size:13px;line-height:1.5;margin:0 0 8px}
.help-box ul{margin:0 0 8px 18px;font-size:13px;line-height:1.5}
.help-close{position:absolute;top:10px;right:14px;background:none;border:none;color:#aaa;font-size:22px;cursor:pointer;line-height:1}
#pageLoadingOverlay{position:fixed;inset:0;background:#1a1a2e;color:#eee;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:16px;font-weight:600;z-index:99999;text-align:center;padding:20px;gap:14px}
#pageLoadingOverlay .pgspin{width:34px;height:34px;border:4px solid #333;border-top-color:#8ab4f8;border-radius:50%;animation:pgspin .8s linear infinite}
@keyframes pgspin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div id="pageLoadingOverlay"><div class="pgspin"></div><div data-i18n="loading_text">Chargement en cours...</div></div>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<span id="helpLink" onclick="showHelpModal()" data-i18n="help_link">Aide</span>
<div id="helpBackdrop" class="help-backdrop" onclick="if(event.target===this)closeHelpModal()">
<div class="help-box">
<button class="help-close" onclick="closeHelpModal()">&times;</button>
<h2 data-i18n="help_title">Bienvenue</h2>
<p data-i18n="help_intro"></p>
<h3 data-i18n="help_checklist_title"></h3>
<ul>
<li data-i18n="help_check_ip"></li>
<li data-i18n="help_check_playlist"></li>
<li data-i18n="help_check_wifi"></li>
</ul>
<p id="helpUrlReminder"></p>
<h3 data-i18n="help_features_title"></h3>
<ul>
<li data-i18n="help_feat_playlists"></li>
<li data-i18n="help_feat_clock"></li>
<li data-i18n="help_feat_display"></li>
<li data-i18n="help_feat_network"></li>
</ul>
</div>
</div>
<div class="topnav">
<a href="/config/basic" onclick="showPageLoadingOverlay()" class="active" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" onclick="showPageLoadingOverlay()" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" onclick="showPageLoadingOverlay()" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" onclick="showPageLoadingOverlay()" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Affichage &amp; Playlists</h1>
<form id="basicForm" onsubmit="saveConfig(event)">
<div class="section">
<h2 data-i18n="sec_display">&#x1F4A1; Affichage</h2>
<div class="row"><label for="brightness" data-i18n="lbl_brightness">Luminosit&eacute; (%)</label><input id="brightness" type="range" min="0" max="100" value="50" oninput="onBrightnessInput(this)" onchange="sendBrightness(this.value,true)"><span id="bval" style="margin-left:8px;color:#ffd146;min-width:24px">50</span></div>
<div class="desc" data-i18n="desc_brightness_live">&#x1F4A1; Aper&ccedil;u appliqu&eacute; en direct sur l'&eacute;cran DMD.</div>
<div class="row"><label data-i18n="lbl_silent_boot">D&eacute;marrage silencieux</label><input id="silent_boot" type="checkbox"></div>
</div>
<div class="section">
<h2 data-i18n="sec_playlist">&#x1F4BF; Playlist</h2>
<div class="row"><label for="playlist" data-i18n="lbl_playlist_file">Playlist par d&eacute;faut</label><select id="playlist"></select></div>
<div class="row"><label data-i18n="lbl_random">Lecture al&eacute;atoire</label><input id="random" type="checkbox"></div>
</div>
<div class="section">
<h2 data-i18n="sec_manage_playlists">&#x2699; Gestion des playlists</h2>
<div class="desc" data-i18n="desc_gen_playlist">Cochez des dossiers pour g&eacute;n&eacute;rer une nouvelle playlist.</div>
<div class="mini-row">
<button type="button" class="mini-btn" onclick="selectAllGenDirs(true)" data-i18n="btn_select_all">Tout s&eacute;lectionner</button>
<button type="button" class="mini-btn" onclick="selectAllGenDirs(false)" data-i18n="btn_select_none">Rien s&eacute;lectionner</button>
</div>
<div id="genDirList" class="dirs"></div>
<div class="row"><label for="loadPlaylistSelect" data-i18n="lbl_load_playlist">Modifier une playlist existante</label><select id="loadPlaylistSelect" onchange="loadPlaylistForEdit()"><option value="">---</option></select></div>
<div class="row"><label for="playlistName" data-i18n="lbl_playlist_name">Nom playlist</label><input id="playlistName" data-i18n-placeholder="placeholder_playlist_name" placeholder="ex: MaPlaylist"></div>
<div class="btn-row"><button type="button" class="btn btn-gen" onclick="generatePlaylist()" data-i18n="btn_gen_playlist">&#x2699; G&eacute;n&eacute;rer playlist</button><button type="button" class="btn btn-del" id="genStopBtn" style="display:none" onclick="stopGeneratePlaylist()" data-i18n="btn_stop_gen">&#x23F9; Arr&ecirc;ter</button></div>
<div class="row"><label for="deletePlaylistSelect" data-i18n="lbl_delete_playlist">Supprimer</label><select id="deletePlaylistSelect"></select></div>
<div class="btn-row"><button type="button" class="btn btn-del" onclick="deletePlaylist()" data-i18n="btn_delete_playlist">&#x1F5D1; Supprimer playlist</button></div>
</div>
<div class="btn-row">
<button type="submit" class="btn btn-save" data-i18n="btn_save">&#x1F4BE; Enregistrer</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-del" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
</form>
<div id="msg" class="msg"></div>
<script>
const HELP_I18N={
fr:{help_link:'Aide',help_title:'Bienvenue sur la configuration du DMD',help_intro:'Voici ce qu\'il reste à vérifier avant de sauvegarder, et un résumé de ce que permet cette interface.',help_checklist_title:'À vérifier avant de sauvegarder',help_check_ip:'IP Recalbox renseignée (page Wi-Fi & Bluetooth)',help_check_playlist:'Playlist par défaut renseignée (page Affichage & Playlists)',help_check_wifi:'Le Wi-Fi est déjà validé à ce stade — inutile d\'y retoucher, sauf si vous voulez le changer',help_url_reminder:'Cette page reste accessible à tout moment en tapant l\'IP du DMD dans un navigateur — actuellement {ip}',help_features_title:'Ce que permet cette interface',help_feat_playlists:'GIFs (page Médias) : ajouter des GIFs sur la carte SD (upload direct depuis le navigateur, création de dossiers) — les playlists qui référencent un dossier modifié sont mises à jour automatiquement',help_feat_clock:'Horloge (page Horloge) : thème, couleur néon, intervalle et durée d\'affichage, fuseau horaire',help_feat_display:'Affichage, luminosité et playlists (page Affichage & Playlists) : luminosité de l\'écran, choix entre démarrage silencieux (titre seul) ou normal (IP détectée, synchronisation de l\'heure, etc.), sélection de la playlist par défaut, et création/suppression de playlists à partir des dossiers de GIFs',help_feat_network:'Réseau (page Wi-Fi & Bluetooth) : IP Recalbox (connexion MQTT), Wi-Fi (réseau, mot de passe, IP statique)'},
en:{help_link:'Help',help_title:'Welcome to the DMD configuration',help_intro:'Here is what\'s left to check before saving, and a summary of what this interface lets you do.',help_checklist_title:'To check before saving',help_check_ip:'Recalbox IP filled in (Wi-Fi & Bluetooth page)',help_check_playlist:'Default playlist filled in (Display & Playlists page)',help_check_wifi:'Wi-Fi is already validated at this stage — no need to touch it again, unless you want to change it',help_url_reminder:'This page stays accessible at any time by typing the DMD\'s IP in a browser — currently {ip}',help_features_title:'What this interface lets you do',help_feat_playlists:'GIFs (Media page): add GIFs to the SD card (direct upload from the browser, folder creation) — playlists referencing a modified folder are updated automatically',help_feat_clock:'Clock (Clock page): theme, custom neon color, display interval and duration, time zone',help_feat_display:'Display, brightness and playlists (Display & Playlists page): screen brightness, choice between silent startup (title only) or normal (detected IP, time sync, etc.), default playlist selection, and creating/deleting playlists from GIF folders',help_feat_network:'Network (Wi-Fi & Bluetooth page): Recalbox IP (MQTT connection), Wi-Fi (network, password, static IP)'},
es:{help_link:'Ayuda',help_title:'Bienvenido a la configuración del DMD',help_intro:'Esto es lo que falta comprobar antes de guardar, y un resumen de lo que permite esta interfaz.',help_checklist_title:'A comprobar antes de guardar',help_check_ip:'IP de Recalbox indicada (página Wi-Fi y Bluetooth)',help_check_playlist:'Playlist por defecto indicada (página Pantalla y listas)',help_check_wifi:'El Wi-Fi ya está validado en esta etapa — no hace falta tocarlo, salvo que quiera cambiarlo',help_url_reminder:'Esta página sigue accesible en cualquier momento escribiendo la IP del DMD en un navegador — actualmente {ip}',help_features_title:'Qué permite esta interfaz',help_feat_playlists:'GIFs (página Medios): añadir GIFs a la tarjeta SD (subida directa desde el navegador, creación de carpetas) — las playlists que referencian una carpeta modificada se actualizan automáticamente',help_feat_clock:'Reloj (página Reloj): tema, color neón personalizado, intervalo y duración de visualización, zona horaria',help_feat_display:'Pantalla, brillo y listas (página Pantalla y listas): brillo de la pantalla, elección entre inicio silencioso (solo título) o normal (IP detectada, sincronización horaria, etc.), selección de la playlist por defecto, y creación/eliminación de playlists a partir de las carpetas de GIFs',help_feat_network:'Red (página Wi-Fi y Bluetooth): IP de Recalbox (conexión MQTT), Wi-Fi (red, contraseña, IP estática)'}
};
function showHelpModal(){
  document.getElementById('helpBackdrop').classList.add('show');
  const p=document.getElementById('helpUrlReminder');
  if(p) p.textContent=((HELP_I18N[currentLang]&&HELP_I18N[currentLang].help_url_reminder)||HELP_I18N.fr.help_url_reminder).replace('{ip}',window.location.host);
}
function closeHelpModal(){document.getElementById('helpBackdrop').classList.remove('show');}
const PAGE_I18N={
fr:{title:'RecalBox DMD - Affichage',h1:'Affichage &amp; Playlists',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_display:'&#x1F4A1; Affichage',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Luminosité (%)',desc_brightness_live:'&#x1F4A1; Aperçu appliqué en direct sur l\'écran DMD.',lbl_silent_boot:'Démarrage silencieux',lbl_playlist_file:'Playlist par défaut',lbl_random:'Lecture aléatoire',lbl_delete_playlist:'Supprimer',btn_delete_playlist:'&#x1F5D1; Supprimer playlist',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_select_playlist:'Sélectionnez une playlist à supprimer',msg_confirm_delete:'Supprimer ${0} ?',msg_confirm_delete_default:'ATTENTION : ${0} est actuellement la playlist par defaut ! La supprimer peut empecher le DMD de demarrer normalement. Continuer ?',msg_deleting:'Suppression...',msg_load_error:'Impossible de charger la config',sec_manage_playlists:'&#x2699; Gestion des playlists',desc_gen_playlist:'Cochez des dossiers pour générer une nouvelle playlist. &#x26A0;&#xFE0F; La création n\'est performante que sur des dossiers avec un nombre limité de fichiers. Pour des playlists contenant des dossiers conséquents, passez par l\'utilitaire RecalboxDMD_tool sur PC.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',lbl_playlist_name:'Nom playlist',placeholder_playlist_name:'ex: MaPlaylist',btn_gen_playlist:'&#x2699; Générer playlist',msg_no_playlist_name:'Donnez un nom à la playlist',msg_select_folder:'Choisissez au moins un dossier',msg_generating:'Generation...',lbl_load_playlist:'Modifier une playlist existante',msg_scanning:'Analyse',msg_gen_busy:'Generation deja en cours ailleurs',msg_gen_start_error:'Impossible de demarrer la generation',msg_gen_leave_warning:'Une generation de playlist est en cours. Quitter la page ?',btn_stop_gen:'&#x23F9; Arreter',msg_confirm_stop_gen:'Arreter la generation ? La playlist en cours de creation sera supprimee.',msg_stopping_gen:'Arret playlist en cours, veuillez patienter...',msg_stop_gen_failed:'Echec de la demande d\'arret (reseau) -- reessayez',essential_wifi:'Wi-Fi',essential_playlist:'Playlist par défaut',essential_ip:'IP Recalbox',msg_essential_missing:'Attention : champ(s) essentiel(s) vide(s) : {fields}. Le DMD risque de ne pas fonctionner correctement. Continuer quand même ?',loading_text:'Chargement en cours...',...HELP_I18N.fr},
en:{title:'RecalBox DMD - Display',h1:'Display &amp; Playlists',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_display:'&#x1F4A1; Display',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Brightness (%)',desc_brightness_live:'&#x1F4A1; Live preview applied directly on the DMD screen.',lbl_silent_boot:'Silent boot',lbl_playlist_file:'Default playlist',lbl_random:'Random playback',lbl_delete_playlist:'Delete',btn_delete_playlist:'&#x1F5D1; Delete playlist',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_select_playlist:'Select a playlist to delete',msg_confirm_delete:'Delete ${0}?',msg_confirm_delete_default:'WARNING: ${0} is currently the default playlist! Deleting it may prevent the DMD from starting normally. Continue?',msg_deleting:'Deleting...',msg_load_error:'Unable to load config',sec_manage_playlists:'&#x2699; Playlist management',desc_gen_playlist:'Check folders to generate a new playlist. &#x26A0;&#xFE0F; Generation is only fast on folders with a limited number of files. For playlists covering large folders, use the RecalboxDMD_tool utility on PC instead.',btn_select_all:'Select all',btn_select_none:'Select none',lbl_playlist_name:'Playlist name',placeholder_playlist_name:'e.g. MyPlaylist',btn_gen_playlist:'&#x2699; Generate playlist',msg_no_playlist_name:'Please name the playlist',msg_select_folder:'Select at least one folder',msg_generating:'Generating...',lbl_load_playlist:'Edit an existing playlist',msg_scanning:'Scanning',msg_gen_busy:'A generation is already running',msg_gen_start_error:'Could not start generation',msg_gen_leave_warning:'A playlist generation is in progress. Leave the page?',btn_stop_gen:'&#x23F9; Stop',msg_confirm_stop_gen:'Stop generation? The playlist being created will be deleted.',msg_stopping_gen:'Stopping playlist generation, please wait...',msg_stop_gen_failed:'Stop request failed (network) -- please retry',essential_wifi:'Wi-Fi',essential_playlist:'Default playlist',essential_ip:'Recalbox IP',msg_essential_missing:'Warning: missing essential field(s): {fields}. The DMD may not work correctly. Continue anyway?',loading_text:'Loading...',...HELP_I18N.en},
es:{title:'RecalBox DMD - Pantalla',h1:'Pantalla y listas',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_display:'&#x1F4A1; Pantalla',sec_playlist:'&#x1F4BF; Lista',lbl_brightness:'Brillo (%)',desc_brightness_live:'&#x1F4A1; Vista previa aplicada en directo en la pantalla DMD.',lbl_silent_boot:'Arranque silencioso',lbl_playlist_file:'Lista predeterminada',lbl_random:'Reproducción aleatoria',lbl_delete_playlist:'Eliminar',btn_delete_playlist:'&#x1F5D1; Eliminar lista',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_select_playlist:'Selecciona una lista para eliminar',msg_confirm_delete:'¿Eliminar ${0}?',msg_confirm_delete_default:'ATENCIÓN: ¡${0} es actualmente la lista predeterminada! Eliminarla puede impedir que el DMD arranque normalmente. ¿Continuar?',msg_deleting:'Eliminando...',msg_load_error:'No se pudo cargar la configuración',sec_manage_playlists:'&#x2699; Gestión de listas',desc_gen_playlist:'Marque las carpetas para generar una nueva lista. &#x26A0;&#xFE0F; La creación solo es rápida en carpetas con un número limitado de archivos. Para listas con carpetas voluminosas, use la utilidad RecalboxDMD_tool en el PC.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',lbl_playlist_name:'Nombre de la lista',placeholder_playlist_name:'ej: MiLista',btn_gen_playlist:'&#x2699; Generar lista',msg_no_playlist_name:'Póngale un nombre a la lista',msg_select_folder:'Elija al menos una carpeta',msg_generating:'Generando...',lbl_load_playlist:'Editar una lista existente',msg_scanning:'Analizando',msg_gen_busy:'Ya hay una generación en curso',msg_gen_start_error:'No se pudo iniciar la generación',msg_gen_leave_warning:'Hay una generación de lista en curso. ¿Salir de la página?',btn_stop_gen:'&#x23F9; Detener',msg_confirm_stop_gen:'¿Detener la generación? La lista en creación se eliminará.',msg_stopping_gen:'Deteniendo la generación de la lista, espere...',essential_wifi:'Wi-Fi',essential_playlist:'Playlist por defecto',essential_ip:'IP de Recalbox',msg_essential_missing:'Atención: falta(n) campo(s) esencial(es): {fields}. Es posible que el DMD no funcione correctamente. ¿Continuar de todos modos?',loading_text:'Cargando...',...HELP_I18N.es}
};
let currentLang='fr';
// Overlay "Chargement en cours..." (2026-08-05, demande utilisateur :
// ~3s d'attente sur mobile avant affichage, impression de plantage/envie
// de F5). Present des le tout premier octet du <body> (avant tout script)
// donc visible des que le navigateur commence a peindre la page, meme si
// le reste du transfert (page + donnees /lang, /load) prend encore du
// temps -- masque via hidePageLoadingOverlay(), appelee en toute fin de
// la chaine de bootstrap (succes ET echec, voir .finally() plus bas).
function showPageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='flex';}
function hidePageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='none';}
let _plNameAutoFilled=false; // suivi de la suggestion auto de nom (voir updatePlaylistNameSuggestion())
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function trTpl(k,v){return tr(k).replace('${0}',v);}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.querySelectorAll('[data-i18n-placeholder]').forEach(function(el){el.placeholder=tr(el.dataset.i18nPlaceholder);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
let _formDirty=false;
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
// showMsg() sans miroir DMD : necessaire pour la confirmation de reprise
// (dmdResume()) -- /dmd-pause remet justement le DMD en mode pause/config,
// ce qui annulait la reprise a peine effectuee (ecran fige juste apres
// "DMD repris", confirme en test reel).
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function serialize(){return new URLSearchParams({brightness:document.getElementById('brightness').value,info:document.getElementById('silent_boot').checked?'0':'1',playlist:document.getElementById('playlist').value,random:document.getElementById('random').checked?'1':'0'});}
// v55 -- apercu live de la luminosite pendant le drag du curseur : envoi
// throttle vers /set-brightness (RAM uniquement sur le firmware, pas
// d'ecriture SD) pour ne pas spammer l'ESP32 a chaque pixel de drag, plus
// un envoi garanti au relachement ("change") pour ne jamais perdre la
// valeur finale. Independant de "Sauvegarder" (qui reste le seul a ecrire
// /config.ini).
let _brightnessLastSentMs = 0;
function sendBrightness(val, force){
  const now = Date.now();
  if (!force && (now - _brightnessLastSentMs) < 120) return;
  _brightnessLastSentMs = now;
  fetch('/set-brightness',{method:'POST',body:new URLSearchParams({value:val}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
function onBrightnessInput(el){
  document.getElementById('bval').textContent = el.value;
  sendBrightness(el.value, false);
}
// Brouillon localStorage (2026-08-05, correctif "reglages perdus si on
// change de page", demande explicite : pas d'alerte bloquante, un vrai
// correctif qui empeche la perte). Chaque frappe sur cette page ecrit
// l'etat courant du formulaire dans localStorage (cote navigateur, survit
// a une navigation complete entre pages -- contrairement a une simple
// variable JS) ; loadConfig() le relit au chargement et l'applique
// PAR-DESSUS les valeurs serveur (le brouillon represente ce qu'on est en
// train de saisir, donc plus recent que la derniere sauvegarde reelle).
// clearDraft() n'est appele qu'apres un /save reussi -- la config.ini
// elle-meme continue de n'etre ecrite que sur un clic explicite sur
// "Enregistrer"/"Enregistrer & Redemarrer", inchange.
const DRAFT_KEY='dmd_draft_basic';
const DRAFT_FIELDS=['brightness','silent_boot','playlist','random'];
function loadDraft(){try{const raw=localStorage.getItem(DRAFT_KEY);return raw?JSON.parse(raw):null;}catch(e){return null;}}
function saveDraft(){const o={};DRAFT_FIELDS.forEach(id=>{const el=document.getElementById(id);if(!el)return;o[id]=(el.type==='checkbox')?el.checked:el.value;});localStorage.setItem(DRAFT_KEY,JSON.stringify(o));}
function clearDraft(){localStorage.removeItem(DRAFT_KEY);}
function checkEssentialFields(){return fetch('/load').then(r=>r.json()).then(d=>{const missing=[];if(!d.wifi_ssid)missing.push(tr('essential_wifi'));if(!d.playlist)missing.push(tr('essential_playlist'));if(!d.recalbox_ip)missing.push(tr('essential_ip'));if(!missing.length)return true;return confirm(tr('msg_essential_missing').replace('{fields}',missing.join(', ')));}).catch(()=>true);}
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK')){_formDirty=false;clearDraft();}}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(skipConfirm){checkEssentialFields().then(ok=>{if(!ok)return;if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});});}
// skipConfirm=true (2026-07-29) : "Enreg. & Redemarrer" a deja un intitule
// explicite -- redemander confirmation juste apres la sauvegarde est
// redondant, contrairement au bouton "Redemarrer" seul.
function saveAndReboot(){saveConfig().then(()=>setTimeout(()=>doReboot(true),400));}
function dmdResume(){checkEssentialFields().then(ok=>{if(!ok)return;if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));});}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) : aucun
// moyen fiable de distinguer une vraie fermeture d'onglet/navigateur d'un
// simple rafraichissement de page (habitude trop ancree pour l'utilisateur,
// faux positifs trop frequents -- ni le JS ni le serveur ne peuvent
// distinguer les deux cas, une connexion qui se ferme se ressemble dans
// tous les cas).
// B (plan cache_master_gifs, retour test reel 2026-08-01) -- retry (5
// tentatives, 500ms d'ecart) SEULEMENT sur echec reel (fetch/parse), jamais
// sur une reponse vide reussie (contrairement a loadGenDirs()/loadDirs() :
// une liste de playlists vide est un etat legitime, pas forcement une
// anomalie transitoire). Meme cause que loadDirs() : un simple
// fetch().catch(()=>{}) laissait les 3 listes vides en silence des qu'une
// seule requete /lsplaylists echouait au chargement de la page.
async function fillPlaylists(selVal){
  for(let attempt=0;attempt<5;attempt++){
    try{
      const pl=await(await fetch('/lsplaylists')).json();
      const sel=document.getElementById('playlist');const del=document.getElementById('deletePlaylistSelect');const load=document.getElementById('loadPlaylistSelect');
      sel.innerHTML='';del.innerHTML='';load.innerHTML='';
      const opt=document.createElement('option');opt.value='';opt.textContent='---';sel.appendChild(opt);
      const opt2=document.createElement('option');opt2.value='';opt2.textContent='---';del.appendChild(opt2);
      const opt3=document.createElement('option');opt3.value='';opt3.textContent='---';load.appendChild(opt3);
      pl.forEach(p=>{const o=document.createElement('option');o.value=p;o.textContent=p;if(p===selVal)o.selected=true;sel.appendChild(o);const o2=document.createElement('option');o2.value=p;o2.textContent=p;del.appendChild(o2);const o3=document.createElement('option');o3.value=p;o3.textContent=p;load.appendChild(o3);});
      return;
    }catch(e){ if(attempt<4) await new Promise(r=>setTimeout(r,500)); }
  }
}
function deletePlaylist(){const name=document.getElementById('deletePlaylistSelect').value;if(!name){showMsg(tr('msg_select_playlist'),false);return;}
  // Playlist par defaut (demande utilisateur, 2026-07-30) : popup de
  // confirmation distincte et plus explicite si la playlist qu'on s'apprete
  // a supprimer est aussi celle configuree par defaut -- 'playlist' (le
  // select "Playlist par defaut") est deja pre-selectionne sur cette valeur
  // par fillPlaylists(), simple comparaison, aucun nouvel appel serveur.
  const isDefault=document.getElementById('playlist').value===name;
  if(isDefault){if(!confirm(trTpl('msg_confirm_delete_default',name)))return;}
  else if(!confirm(trTpl('msg_confirm_delete',name)))return;
  // showMsgLocal (pas showMsg) : meme raison que dans generatePlaylist()
  // ci-dessous -- eviter le fetch('/dmd-pause') interne de showMsg() en
  // concurrence avec le fetch('/delete-playlist') juste apres.
  showMsgLocal(tr('msg_deleting'),true);
  fetch('/delete-playlist',{method:'POST',body:new URLSearchParams({name:name}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));fillPlaylists('');}).catch(()=>showMsg(tr('msg_net_error'),false));}
// Generation de playlist (deplacee depuis MEDIA -- demande utilisateur :
// la gestion des playlists va dans Affichage, la gestion physique des
// fichiers/dossiers reste dans MEDIA). Liste des dossiers ici pour
// COCHER uniquement -- pas d'icone d'ouverture/consultation du contenu,
// reservee a la page MEDIA.
// Retry (5 tentatives, 500ms d'ecart) : un simple fetch().catch(()=>{})
// laissait la liste vide en silence, sans aucun message ni nouvelle
// tentative, si cette requete echouait une seule fois au chargement de la
// page (observe en test reel 2026-07-29 -- l'endpoint repond pourtant bien
// quand on le teste isolement juste apres). Retente aussi si la reponse est
// VIDE (pas juste en echec reseau) : sur ce projet, /lsgifdirs ne renvoie
// [] que si plGenIsActive() etait actif pile a ce moment (transitoire) --
// il existe toujours des dossiers reels, donc une liste vide est ici
// toujours anormale/transitoire, jamais un etat legitime a accepter tel
// quel.
// Cache sessionStorage PARTAGE avec la page MEDIA (meme cle,
// 'dmd_gifdirs_cache') -- demande utilisateur 2026-08-02 : le va-et-vient
// frequent entre Affichage et MEDIA redemandait /lsgifdirs a chaque fois,
// avec le risque d'echec reseau observe en test reel. Affiche IMMEDIATEMENT
// le contenu en cache si present (aucune attente reseau), PUIS rafraichit
// en arriere-plan et met a jour le cache.
function readDirsCache(){
  try{
    const raw=sessionStorage.getItem('dmd_gifdirs_cache');
    return raw?JSON.parse(raw):null;
  }catch(e){return null;}
}
function writeDirsCache(dirs){
  try{sessionStorage.setItem('dmd_gifdirs_cache',JSON.stringify(dirs));}catch(e){}
}
function renderGenDirs(dirs){
  const list=document.getElementById('genDirList');
  // B (plan cache_master_gifs) -- tri alphabetique cote JS, fonctionne quel
  // que soit l'etat/l'origine de cache_master_gifs.dat.
  const sorted=dirs.slice().sort((a,b)=>{
    const na=(a&&typeof a==='object')?a.name:a;
    const nb=(b&&typeof b==='object')?b.name:b;
    return na.localeCompare(nb);
  });
  list.innerHTML='';
  sorted.forEach(d=>{
    const name=(d&&typeof d==='object')?d.name:d;
    const row=document.createElement('label');
    row.innerHTML='<input type="checkbox" value="'+name+'"><span class="name">&#x1F4C1; '+name+'</span>';
    list.appendChild(row);
  });
}
async function loadGenDirs(){
  const cached=readDirsCache();
  if(cached&&cached.length)renderGenDirs(cached);
  for(let attempt=0;attempt<5;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{
      const dirs=await(await fetch('/lsgifdirs')).json();
      if(!dirs.length&&attempt<4)continue;
      renderGenDirs(dirs);
      writeDirsCache(dirs);
      return;
    }catch(e){}
  }
}
function selectAllGenDirs(v){document.querySelectorAll('#genDirList input').forEach(i=>i.checked=v);updatePlaylistNameSuggestion();}
// Suggestion de nom (demande utilisateur) : si exactement un dossier est
// coche, pre-remplit "Nom playlist" avec son nom -- efface a la 1ere prise
// de controle du champ par l'utilisateur (focus), jamais ecrase apres. Ne
// s'applique QUE pour une NOUVELLE playlist (loadPlaylistSelect vide) --
// ne touche jamais au nom d'une playlist existante en cours d'edition.
function updatePlaylistNameSuggestion(){
  if(document.getElementById('loadPlaylistSelect').value)return;
  const checked=[].slice.call(document.querySelectorAll('#genDirList input:checked'));
  const nameEl=document.getElementById('playlistName');
  if(checked.length===1){
    if(_plNameAutoFilled||nameEl.value==='') { nameEl.value=checked[0].value; _plNameAutoFilled=true; }
  } else if(_plNameAutoFilled){
    nameEl.value=''; _plNameAutoFilled=false;
  }
}
document.getElementById('genDirList').addEventListener('change',function(e){if(e.target&&e.target.type==='checkbox')updatePlaylistNameSuggestion();});
document.getElementById('playlistName').addEventListener('focus',function(){if(_plNameAutoFilled){this.value='';_plNameAutoFilled=false;}});
// Modifier une playlist existante (demande utilisateur) : precoche les
// dossiers qu'elle referme deja au lieu de forcer une re-selection complete
// avant de regenerer (la regeneration ecrase le fichier a l'identique --
// meme nom).
function loadPlaylistForEdit(){
  const raw=document.getElementById('loadPlaylistSelect').value;
  if(!raw){
    // Retour a "---" (demande utilisateur) : decoche tout plutot que de
    // laisser les cases d'une precedente edition/selection.
    selectAllGenDirs(false);
    document.getElementById('playlistName').value='';
    _plNameAutoFilled=false;
    return;
  }
  // raw vient de /lsplaylists, QUI INCLUT ".txt" -- retire l'extension avant
  // de l'utiliser : sinon /playlist-dirs cherche "name.txt.txt" (introuvable,
  // dossiers jamais precoches) et une regeneration ecrirait un fichier
  // "name.txt.txt" au lieu d'ecraser l'original.
  const name=raw.replace(/\.txt$/i,'');
  _plNameAutoFilled=false; // le nom vient d'une playlist existante, jamais ecrase par la suggestion auto
  document.getElementById('playlistName').value=name;
  fetch('/playlist-dirs?name='+encodeURIComponent(name)).then(r=>r.json()).then(dirs=>{
    document.querySelectorAll('#genDirList input[type=checkbox]').forEach(c=>{c.checked=dirs.indexOf(c.value)>=0;});
  }).catch(()=>showMsg(tr('msg_net_error'),false));
}
// Verrouille/deverrouille toute la page pendant la generation -- empeche de
// lancer une autre action (upload, suppression...) pendant qu'un scan est en
// cours, en plus du garde cote serveur (g_plGenStatus.active, RecalBox_DMD.ino).
function setPageBusy(busy){document.querySelectorAll('button,input,select').forEach(e=>{if(e.id!=='genStopBtn')e.disabled=busy;});document.body.classList.toggle('gen-busy',busy);
  document.getElementById('genStopBtn').style.display=busy?'inline-block':'none';
  if(busy)document.getElementById('genStopBtn').disabled=false; // etat frais a chaque nouvelle generation (peut avoir ete desactive par un arret precedent)
  // beforeunload : la generation continue cote serveur meme si l'utilisateur
  // quitte la page (machine a etats independante du navigateur), mais le
  // polling JS s'arreterait -- avertir plutot que laisser croire a un blocage
  // silencieux si jamais le verrou CSS est contourne (ex. navigation clavier).
  _pageBusy=busy; refreshBeforeUnload();
}
// Garde unifiee (2026-08-05, bug signale par l'utilisateur : "reglages
// perdus si on change de page") -- les liens de la barre de navigation
// (topnav, <a href> classiques) et le bouton retour du navigateur ne
// passaient par AUCUNE verification : seuls doReboot()/dmdResume()
// avertissaient (_formDirty) avant de partir. Un champ modifie puis
// jamais envoye a /save (aucun clic sur "Enregistrer") disparaissait donc
// silencieusement des qu'on changeait de page -- comportement HTML normal
// pour un simple <a>, mais sans le moindre avertissement contrairement aux
// autres actions de cette meme page. window.onbeforeunload est le seul
// mecanisme couvrant TOUS les cas de depart (topnav, precedent/suivant,
// fermeture d'onglet, actualisation) en un seul point.
let _pageBusy=false;
// _formDirty ne declenche plus onbeforeunload (2026-08-05, demande
// utilisateur explicite : pas d'alerte bloquante) -- remplace par la
// persistance de brouillon localStorage (loadDraft()/saveDraft()/
// clearDraft() ci-dessous), qui elimine le probleme a la racine : les
// champs modifies sur cette page survivent desormais a une navigation vers
// une autre page ou une fermeture d'onglet, sans le moindre avertissement,
// et sont restaures automatiquement au retour -- rien n'est plus "perdu"
// silencieusement, donc plus besoin de prevenir. _pageBusy reste protege
// par onbeforeunload : cas different, une generation de playlist active
// cote serveur (pas une histoire de champs de formulaire).
function refreshBeforeUnload(){
  window.onbeforeunload = _pageBusy ? function(){return tr('msg_gen_leave_warning');} : null;
}
// _stopRequestPending (2026-07-30, demande utilisateur : "Arreter" echoue
// presque a chaque fois) : le serveur ESP32 est mono-thread (une seule
// requete HTTP traitee a la fois) et la boucle de polling
// (generatePlaylist(), toutes les 700ms) tourne EN
// PERMANENCE pendant qu'un scan est actif -- la fenetre de collision avec
// la requete d'arret (qui doit pourtant reussir vite) est donc quasi
// garantie, le retry existant (3x/500ms) retombant lui-meme regulierement
// sur le sondage suivant. Les boucles de polling verifient ce drapeau et
// SAUTENT leur propre requete pendant qu'un arret est en cours, laissant le
// champ libre au serveur mono-thread plutot que de continuer a le
// solliciter en parallele.
let _stopRequestPending=false;
async function stopGeneratePlaylist(){
  if(!confirm(tr('msg_confirm_stop_gen')))return;
  // Message persistant immediat (pas de setTimeout d'auto-masquage) : le
  // temps reel d'arret depend de la lenteur SD en cours (jusqu'a ~1 min
  // observe en test reel) -- sans ca, rien n'indique que le clic a bien ete
  // pris en compte pendant cette attente. Reste affiche jusqu'a ce que la
  // boucle de polling deja en cours dans generatePlaylist() detecte la fin
  // reelle (!active) et affiche le resultat definitif.
  const msgEl=document.getElementById('msg');
  if(window._msgTimer)clearTimeout(window._msgTimer);
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_stopping_gen');
  document.getElementById('genStopBtn').disabled=true; // evite un double-clic pendant l'attente
  _stopRequestPending=true;
  // Retry (3 tentatives, 500ms d'ecart) : un simple fetch().catch(()=>{})
  // avalait silencieusement tout echec -- si cette requete tombe pile au
  // meme moment qu'un sondage de statut en cours (serveur ESP32 mono-thread,
  // une seule requete traitee a la fois), elle peut echouer sans laisser de
  // trace, bloquant l'utilisateur sur "Arret en cours..." indefiniment sans
  // que rien ne soit jamais retente (observe en test reel 2026-07-29).
  let ok=false;
  for(let attempt=0;attempt<3&&!ok;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{const r=await fetch('/generate-playlist-stop',{method:'POST'});ok=r.ok;}catch(e){ok=false;}
  }
  _stopRequestPending=false;
  if(!ok){
    msgEl.className='msg err';
    msgEl.textContent=tr('msg_stop_gen_failed');
    document.getElementById('genStopBtn').disabled=false;
  }
}
async function generatePlaylist(){
  const name=document.getElementById('playlistName').value.trim();
  const dirs=[].slice.call(document.querySelectorAll('#genDirList input:checked')).map(i=>i.value).join(',');
  if(!name){showMsg(tr('msg_no_playlist_name'),false);return;}
  if(!dirs){showMsg(tr('msg_select_folder'),false);return;}
  setPageBusy(true);
  const msgEl=document.getElementById('msg');
  if(window._msgTimer)clearTimeout(window._msgTimer);
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_generating');
  // finishGen() : affichage final partage entre la fin du polling (generation
  // classique, asynchrone) et une reponse DEJA terminee recue directement au
  // POST initial (filterPlaylistFromMaster() -- filtrage synchrone depuis
  // le fichier maitre interne, aucune tache creee cote serveur puisque aucun
  // scan de /gifs/ n'est necessaire). Avant ce correctif, une reussite synchrone tombait
  // dans la meme branche que "generation deja en cours"/erreur reseau (ci-
  // dessous) : jamais de minuteur d'auto-masquage (message fige en rouge en
  // permanence) ni de rafraichissement de la liste des playlists (nouvelle
  // playlist invisible sans F5) -- constate en test reel 2026-07-30.
  function finishGen(resultText,ok){
    msgEl.textContent=resultText||tr('msg_gen_start_error');
    msgEl.className='msg '+(ok?'ok':'err');
    if(window._msgTimer)clearTimeout(window._msgTimer);
    window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
    fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(resultText||''),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});
    document.getElementById('playlistName').value='';
    fillPlaylists('');
  }
  let started=false;
  try{
    const r=await fetch('/generate-playlist',{method:'POST',body:new URLSearchParams({name:name,dirs:dirs}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});
    const t=await r.text();
    started=t.includes('STARTED');
    if(!started){
      if(r.ok&&t.startsWith('OK')){
        // Filtrage synchrone depuis le fichier maitre interne : deja termine, pas de tache a suivre.
        finishGen(t,true);
        setPageBusy(false);
        return;
      }
      msgEl.textContent=(r.status===409)?tr('msg_gen_busy'):t;msgEl.className='msg err';
      if(window._msgTimer)clearTimeout(window._msgTimer);
      window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
    }
  }catch(e){
    // La reponse ("STARTED") peut echouer a arriver jusqu'au navigateur
    // (heap degrade apres plusieurs generations enchainees dans la meme
    // session) alors que la tache a deja bien demarre cote serveur --
    // observe en test reel (2026-07-29) : generation qui continue tres
    // normalement (DMD/logs), mais page qui affiche "erreur reseau" et
    // abandonne tout suivi. Avant d'abandonner, verifier le statut reel
    // plutot que de perdre le suivi d'une generation pourtant en cours.
    try{
      const st=await(await fetch('/generate-playlist-status')).json();
      started=!!st.active;
    }catch(e2){started=false;}
    if(!started){
      msgEl.textContent=tr('msg_net_error');msgEl.className='msg err';
      if(window._msgTimer)clearTimeout(window._msgTimer);
      window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
    }
  }
  if(!started){setPageBusy(false);return;}
  // Polling de progression (le WebServer ESP32 est mono-thread : impossible
  // de pousser une mise a jour depuis le serveur pendant que le scan tourne,
  // la page doit donc interroger periodiquement /generate-playlist-status).
  while(true){
    await new Promise(res=>setTimeout(res,700));
    if(_stopRequestPending)continue; // laisse la requete d'arret passer seule (serveur mono-thread)
    let st;
    try{
      // AbortController : sans ca, une seule requete de statut qui reste
      // bloquee (observe en test reel 2026-07-28 -- page figee a 20/165
      // pendant que le DMD, lui, continuait a avancer normalement) fige le
      // polling pour de bon, la boucle n'atteignant jamais l'iteration
      // suivante puisqu'elle reste indefiniment en attente du fetch().
      // 9000ms REMONTE A 25000ms (2026-08-03, analyse .har + log serie reels) :
      // le commentaire ci-dessous (desormais corrige) affirmait que le passage
      // a playlistGenTask() (tache FreeRTOS dediee) avait elimine ce risque --
      // INFIRME par un test reel sur un dossier de 1400+ fichiers (Arcade) :
      // le .har montre la quasi-totalite des requetes /generate-playlist-
      // status echouant a EXACTEMENT ~9000-9016ms (timeout client, pas une
      // erreur serveur), alors que le log serie confirme le scan progressant
      // normalement en parallele (aucun heap critique, aucun arret) -- le
      // serveur met donc parfois plus de 9s a repondre meme depuis la tache
      // dediee, cause exacte non identifiee (le code de playlistGenTask() cede
      // la main via vTaskDelay(1) et ne garde aucun mutex longtemps, en
      // lecture statique ca semble suffisant -- a investiguer plus a fond si
      // 25s s'avere un jour insuffisant). 25000ms : marge large au-dessus du
      // pire cas observe (borne reelle inconnue, le client abandonnait
      // toujours avant que le serveur ne reponde).
      const ctrl=new AbortController();
      const abortTimer=setTimeout(()=>ctrl.abort(),25000);
      st=await(await fetch('/generate-playlist-status',{signal:ctrl.signal})).json();
      clearTimeout(abortTimer);
    }catch(e){continue;}
    if(!st.active){
      finishGen(st.result,st.done);
      break;
    }
    // Compteur numerique reintroduit (2026-07-29) : retire le 2026-07-28 car
    // playlistGenStep() tournait alors dans loop(), donc un blocage SD figeait
    // aussi le serveur web -- le compteur affiche restait fige en meme temps
    // que tout le reste, donnant une fausse impression de gel. Depuis le
    // passage a playlistGenTask() (tache FreeRTOS dediee), /generate-
    // playlist-status repond generalement rapidement (plGenStatusMutex jamais
    // tenu pendant un acces SD) -- MAIS pas garanti au-dela de 9s sur un tres
    // gros dossier (infirme par test reel 2026-08-03, voir commentaire de
    // l'AbortController ci-dessus) : quand une requete de statut aboutit, sa
    // valeur reste fiable (pas de fausse info figee), seul le DELAI pour
    // l'obtenir peut varier.
    msgEl.textContent=tr('msg_scanning')+': '+st.dir+' ('+st.dirIdx+'/'+st.totalDirs+') - '+st.curDirGifs+' GIFs ('+st.gifs+' total)';
  }
  setPageBusy(false);
}
function loadConfig(){const draft=loadDraft();return fetch('/load').then(r=>r.json()).then(d=>{
  const g=(k,dv)=>(draft&&draft[k]!==undefined)?draft[k]:dv;
  document.getElementById('brightness').value=Math.max(0,Math.min(100,parseInt(g('brightness',d.brightness||50),10)));
  document.getElementById('bval').textContent=document.getElementById('brightness').value;
  document.getElementById('silent_boot').checked=g('silent_boot',d.info==='0');
  fillPlaylists(g('playlist',d.playlist||''));
  document.getElementById('random').checked=g('random',d.random==='1');
  if(draft)_formDirty=true; // reboot/reprise doivent quand meme avertir : la config.ini reelle n'a pas ce brouillon
}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','basic');
// loadGenDirs() enchainee APRES /lang+/load (jamais en parallele) : le
// WebServer ESP32 ne traite qu'une requete a la fois -- des fetch()
// concurrents corrompent silencieusement l'une des reponses (bug deja
// documente et corrige sur MEDIA via queuedFetch(), reintroduit ici par
// inattention lors de l'ajout de la generation de playlist, v79).
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);if(d.first_boot==='1'&&!sessionStorage.getItem('dmd_help_seen')){sessionStorage.setItem('dmd_help_seen','1');showHelpModal();}return loadConfig();}).catch(()=>{applyLang();return loadConfig();}).then(loadGenDirs).finally(hidePageLoadingOverlay);
document.getElementById('basicForm').addEventListener('input',()=>{_formDirty=true;saveDraft();});
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_NETWORK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Wi-Fi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
h2{color:#8ab4f8;font-size:15px;margin:0 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.btn-row{display:flex;gap:10px;justify-content:center;margin:18px 0;flex-wrap:wrap}
.btn{padding:12px 20px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#2d6a4f;color:#fff}
.btn-del{background:#555;color:#fff}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
#helpLink{position:absolute;top:14px;right:75px;font-size:13px;color:#8ab4f8;text-decoration:underline;cursor:pointer}
.help-backdrop{display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);z-index:1000;align-items:center;justify-content:center;padding:16px}
.help-backdrop.show{display:flex}
.help-box{background:#16213e;border-radius:8px;padding:20px;max-width:520px;max-height:85vh;overflow-y:auto;position:relative;text-align:left}
.help-box h2{color:#ffd146;font-size:16px;margin:0 22px 10px 0}
.help-box h3{color:#8ab4f8;font-size:14px;margin:14px 0 6px}
.help-box p{font-size:13px;line-height:1.5;margin:0 0 8px}
.help-box ul{margin:0 0 8px 18px;font-size:13px;line-height:1.5}
.help-close{position:absolute;top:10px;right:14px;background:none;border:none;color:#aaa;font-size:22px;cursor:pointer;line-height:1}
#pageLoadingOverlay{position:fixed;inset:0;background:#1a1a2e;color:#eee;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:16px;font-weight:600;z-index:99999;text-align:center;padding:20px;gap:14px}
#pageLoadingOverlay .pgspin{width:34px;height:34px;border:4px solid #333;border-top-color:#8ab4f8;border-radius:50%;animation:pgspin .8s linear infinite}
@keyframes pgspin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div id="pageLoadingOverlay"><div class="pgspin"></div><div data-i18n="loading_text">Chargement en cours...</div></div>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<span id="helpLink" onclick="showHelpModal()" data-i18n="help_link">Aide</span>
<div id="helpBackdrop" class="help-backdrop" onclick="if(event.target===this)closeHelpModal()">
<div class="help-box">
<button class="help-close" onclick="closeHelpModal()">&times;</button>
<h2 data-i18n="help_title">Bienvenue</h2>
<p data-i18n="help_intro"></p>
<h3 data-i18n="help_checklist_title"></h3>
<ul>
<li data-i18n="help_check_ip"></li>
<li data-i18n="help_check_playlist"></li>
<li data-i18n="help_check_wifi"></li>
</ul>
<p id="helpUrlReminder"></p>
<h3 data-i18n="help_features_title"></h3>
<ul>
<li data-i18n="help_feat_playlists"></li>
<li data-i18n="help_feat_clock"></li>
<li data-i18n="help_feat_display"></li>
<li data-i18n="help_feat_network"></li>
</ul>
</div>
</div>
<div class="topnav">
<a href="/config/basic" onclick="showPageLoadingOverlay()" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" onclick="showPageLoadingOverlay()" class="active" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" onclick="showPageLoadingOverlay()" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" onclick="showPageLoadingOverlay()" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Wi-Fi &amp; Bluetooth</h1>
<form id="networkForm" onsubmit="saveConfig(event)">
<div class="section">
<h2 data-i18n="sec_wifi">&#x1F4F6; Wi-Fi</h2>
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;</label><input id="wifi_enabled" type="checkbox"></div>
<div class="row"><label for="wifi_ssid" data-i18n="lbl_network">R&eacute;seau</label><select id="wifi_ssid"><option value="" data-i18n="opt_scanning">-- Scan en cours... --</option></select></div>
<div class="row"><label for="wifi_password" data-i18n="lbl_password">Mot de passe</label><input id="wifi_password" type="password"></div>
<div class="row"><label data-i18n="lbl_static_ip">IP statique</label><input id="wifi_static_enabled" type="checkbox"></div>
<div class="row"><label for="wifi_static_ip" data-i18n="lbl_fixed_ip">IP fixe</label><input id="wifi_static_ip"></div>
<div class="row"><label for="wifi_gateway" data-i18n="lbl_gateway">Passerelle</label><input id="wifi_gateway"></div>
<div class="row"><label for="wifi_subnet" data-i18n="lbl_subnet">Masque</label><input id="wifi_subnet"></div>
<div class="row"><label for="wifi_dns1" data-i18n="lbl_dns1">DNS 1</label><input id="wifi_dns1"></div>
<div class="row"><label for="wifi_dns2" data-i18n="lbl_dns2">DNS 2</label><input id="wifi_dns2"></div>
</div>
<div class="section">
<h2 data-i18n="sec_bt">&#x1F4F1; Bluetooth</h2>
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;</label><input id="bluetooth_enabled" type="checkbox"></div>
<div class="row"><label for="bluetooth_name" data-i18n="lbl_bt_name">Nom</label><input id="bluetooth_name"></div>
</div>
<div class="section">
<h2 data-i18n="sec_mqtt">&#x1F310; MQTT</h2>
<div class="row"><label for="recalbox_ip" data-i18n="lbl_mqtt_ip">IP Recalbox</label><input id="recalbox_ip"></div>
</div>
<div class="btn-row">
<button type="submit" class="btn btn-save" data-i18n="btn_save">&#x1F4BE; Enregistrer</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-del" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
</form>
<div id="msg" class="msg"></div>
<script>
const HELP_I18N={
fr:{help_link:'Aide',help_title:'Bienvenue sur la configuration du DMD',help_intro:'Voici ce qu\'il reste à vérifier avant de sauvegarder, et un résumé de ce que permet cette interface.',help_checklist_title:'À vérifier avant de sauvegarder',help_check_ip:'IP Recalbox renseignée (page Wi-Fi & Bluetooth)',help_check_playlist:'Playlist par défaut renseignée (page Affichage & Playlists)',help_check_wifi:'Le Wi-Fi est déjà validé à ce stade — inutile d\'y retoucher, sauf si vous voulez le changer',help_url_reminder:'Cette page reste accessible à tout moment en tapant l\'IP du DMD dans un navigateur — actuellement {ip}',help_features_title:'Ce que permet cette interface',help_feat_playlists:'GIFs (page Médias) : ajouter des GIFs sur la carte SD (upload direct depuis le navigateur, création de dossiers) — les playlists qui référencent un dossier modifié sont mises à jour automatiquement',help_feat_clock:'Horloge (page Horloge) : thème, couleur néon, intervalle et durée d\'affichage, fuseau horaire',help_feat_display:'Affichage, luminosité et playlists (page Affichage & Playlists) : luminosité de l\'écran, choix entre démarrage silencieux (titre seul) ou normal (IP détectée, synchronisation de l\'heure, etc.), sélection de la playlist par défaut, et création/suppression de playlists à partir des dossiers de GIFs',help_feat_network:'Réseau (page Wi-Fi & Bluetooth) : IP Recalbox (connexion MQTT), Wi-Fi (réseau, mot de passe, IP statique)'},
en:{help_link:'Help',help_title:'Welcome to the DMD configuration',help_intro:'Here is what\'s left to check before saving, and a summary of what this interface lets you do.',help_checklist_title:'To check before saving',help_check_ip:'Recalbox IP filled in (Wi-Fi & Bluetooth page)',help_check_playlist:'Default playlist filled in (Display & Playlists page)',help_check_wifi:'Wi-Fi is already validated at this stage — no need to touch it again, unless you want to change it',help_url_reminder:'This page stays accessible at any time by typing the DMD\'s IP in a browser — currently {ip}',help_features_title:'What this interface lets you do',help_feat_playlists:'GIFs (Media page): add GIFs to the SD card (direct upload from the browser, folder creation) — playlists referencing a modified folder are updated automatically',help_feat_clock:'Clock (Clock page): theme, custom neon color, display interval and duration, time zone',help_feat_display:'Display, brightness and playlists (Display & Playlists page): screen brightness, choice between silent startup (title only) or normal (detected IP, time sync, etc.), default playlist selection, and creating/deleting playlists from GIF folders',help_feat_network:'Network (Wi-Fi & Bluetooth page): Recalbox IP (MQTT connection), Wi-Fi (network, password, static IP)'},
es:{help_link:'Ayuda',help_title:'Bienvenido a la configuración del DMD',help_intro:'Esto es lo que falta comprobar antes de guardar, y un resumen de lo que permite esta interfaz.',help_checklist_title:'A comprobar antes de guardar',help_check_ip:'IP de Recalbox indicada (página Wi-Fi y Bluetooth)',help_check_playlist:'Playlist por defecto indicada (página Pantalla y listas)',help_check_wifi:'El Wi-Fi ya está validado en esta etapa — no hace falta tocarlo, salvo que quiera cambiarlo',help_url_reminder:'Esta página sigue accesible en cualquier momento escribiendo la IP del DMD en un navegador — actualmente {ip}',help_features_title:'Qué permite esta interfaz',help_feat_playlists:'GIFs (página Medios): añadir GIFs a la tarjeta SD (subida directa desde el navegador, creación de carpetas) — las playlists que referencian una carpeta modificada se actualizan automáticamente',help_feat_clock:'Reloj (página Reloj): tema, color neón personalizado, intervalo y duración de visualización, zona horaria',help_feat_display:'Pantalla, brillo y listas (página Pantalla y listas): brillo de la pantalla, elección entre inicio silencioso (solo título) o normal (IP detectada, sincronización horaria, etc.), selección de la playlist por defecto, y creación/eliminación de playlists a partir de las carpetas de GIFs',help_feat_network:'Red (página Wi-Fi y Bluetooth): IP de Recalbox (conexión MQTT), Wi-Fi (red, contraseña, IP estática)'}
};
function showHelpModal(){
  document.getElementById('helpBackdrop').classList.add('show');
  const p=document.getElementById('helpUrlReminder');
  if(p) p.textContent=((HELP_I18N[currentLang]&&HELP_I18N[currentLang].help_url_reminder)||HELP_I18N.fr.help_url_reminder).replace('{ip}',window.location.host);
}
function closeHelpModal(){document.getElementById('helpBackdrop').classList.remove('show');}
const PAGE_I18N={
fr:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi &amp; Bluetooth',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Activé',lbl_network:'Réseau',lbl_password:'Mot de passe',lbl_static_ip:'IP statique',lbl_fixed_ip:'IP fixe',lbl_gateway:'Passerelle',lbl_subnet:'Masque',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Nom',lbl_mqtt_ip:'IP Recalbox',opt_scanning:'-- Scan en cours... --',opt_select:'-- Sélectionnez --',opt_scan_error:'Erreur scan',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config',essential_wifi:'Wi-Fi',essential_playlist:'Playlist par défaut',essential_ip:'IP Recalbox',msg_essential_missing:'Attention : champ(s) essentiel(s) vide(s) : {fields}. Le DMD risque de ne pas fonctionner correctement. Continuer quand même ?',loading_text:'Chargement en cours...',...HELP_I18N.fr},
en:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi &amp; Bluetooth',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Enabled',lbl_network:'Network',lbl_password:'Password',lbl_static_ip:'Static IP',lbl_fixed_ip:'Fixed IP',lbl_gateway:'Gateway',lbl_subnet:'Subnet mask',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Name',lbl_mqtt_ip:'Recalbox IP',opt_scanning:'-- Scanning... --',opt_select:'-- Select --',opt_scan_error:'Scan error',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config',essential_wifi:'Wi-Fi',essential_playlist:'Default playlist',essential_ip:'Recalbox IP',msg_essential_missing:'Warning: missing essential field(s): {fields}. The DMD may not work correctly. Continue anyway?',loading_text:'Loading...',...HELP_I18N.en},
es:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi y Bluetooth',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Activado',lbl_network:'Red',lbl_password:'Contraseña',lbl_static_ip:'IP estática',lbl_fixed_ip:'IP fija',lbl_gateway:'Puerta de enlace',lbl_subnet:'Máscara de subred',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Nombre',lbl_mqtt_ip:'IP de Recalbox',opt_scanning:'-- Escaneando... --',opt_select:'-- Seleccione --',opt_scan_error:'Error de escaneo',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración',essential_wifi:'Wi-Fi',essential_playlist:'Playlist por defecto',essential_ip:'IP de Recalbox',msg_essential_missing:'Atención: falta(n) campo(s) esencial(es): {fields}. Es posible que el DMD no funcione correctamente. ¿Continuar de todos modos?',loading_text:'Cargando...',...HELP_I18N.es}
};
let currentLang='fr';
// Overlay "Chargement en cours..." (2026-08-05, demande utilisateur :
// ~3s d'attente sur mobile avant affichage, impression de plantage/envie
// de F5). Present des le tout premier octet du <body> (avant tout script)
// donc visible des que le navigateur commence a peindre la page, meme si
// le reste du transfert (page + donnees /lang, /load) prend encore du
// temps -- masque via hidePageLoadingOverlay(), appelee en toute fin de
// la chaine de bootstrap (succes ET echec, voir .finally() plus bas).
function showPageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='flex';}
function hidePageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='none';}
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
let savedSsid='';
let _formDirty=false;
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function serialize(){return new URLSearchParams({wifi_enabled:document.getElementById('wifi_enabled').checked?'1':'0',wifi_ssid:document.getElementById('wifi_ssid').value,wifi_password:document.getElementById('wifi_password').value,wifi_static_enabled:document.getElementById('wifi_static_enabled').checked?'1':'0',wifi_static_ip:document.getElementById('wifi_static_ip').value,wifi_gateway:document.getElementById('wifi_gateway').value,wifi_subnet:document.getElementById('wifi_subnet').value,wifi_dns1:document.getElementById('wifi_dns1').value,wifi_dns2:document.getElementById('wifi_dns2').value,bluetooth_enabled:document.getElementById('bluetooth_enabled').checked?'1':'0',bluetooth_name:document.getElementById('bluetooth_name').value,recalbox_ip:document.getElementById('recalbox_ip').value});}
// Brouillon localStorage -- voir le commentaire complet sur la page BASIC
// (correctif "reglages perdus si on change de page", 2026-08-05).
const DRAFT_KEY='dmd_draft_network';
const DRAFT_FIELDS=['wifi_enabled','wifi_ssid','wifi_password','wifi_static_enabled','wifi_static_ip','wifi_gateway','wifi_subnet','wifi_dns1','wifi_dns2','bluetooth_enabled','bluetooth_name','recalbox_ip'];
function loadDraft(){try{const raw=localStorage.getItem(DRAFT_KEY);return raw?JSON.parse(raw):null;}catch(e){return null;}}
function saveDraft(){const o={};DRAFT_FIELDS.forEach(id=>{const el=document.getElementById(id);if(!el)return;o[id]=(el.type==='checkbox')?el.checked:el.value;});localStorage.setItem(DRAFT_KEY,JSON.stringify(o));}
function clearDraft(){localStorage.removeItem(DRAFT_KEY);}
function checkEssentialFields(){return fetch('/load').then(r=>r.json()).then(d=>{const missing=[];if(!d.wifi_ssid)missing.push(tr('essential_wifi'));if(!d.playlist)missing.push(tr('essential_playlist'));if(!d.recalbox_ip)missing.push(tr('essential_ip'));if(!missing.length)return true;return confirm(tr('msg_essential_missing').replace('{fields}',missing.join(', ')));}).catch(()=>true);}
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK')){_formDirty=false;clearDraft();}}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(skipConfirm){checkEssentialFields().then(ok=>{if(!ok)return;if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});});}
// skipConfirm=true (2026-07-29) : "Enreg. & Redemarrer" a deja un intitule
// explicite -- redemander confirmation juste apres la sauvegarde est
// redondant, contrairement au bouton "Redemarrer" seul.
function saveAndReboot(){saveConfig().then(()=>setTimeout(()=>doReboot(true),400));}
function dmdResume(){checkEssentialFields().then(ok=>{if(!ok)return;if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));});}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) : aucun
// moyen fiable de distinguer une vraie fermeture d'onglet/navigateur d'un
// simple rafraichissement de page (habitude trop ancree pour l'utilisateur,
// faux positifs trop frequents -- ni le JS ni le serveur ne peuvent
// distinguer les deux cas, une connexion qui se ferme se ressemble dans
// tous les cas).
function scanWiFi(){
  const sel=document.getElementById('wifi_ssid');
  fetch('/scan-wifi').then(r=>r.json()).then(nets=>{
    sel.innerHTML='';
    const opt=document.createElement('option');opt.value='';opt.textContent=tr('opt_select');sel.appendChild(opt);
    let found=false;
    nets.forEach(n=>{const o=document.createElement('option');o.value=n;o.textContent=n;if(n===savedSsid){o.selected=true;found=true;}sel.appendChild(o);});
    if(savedSsid&&!found){const o=new Option(savedSsid,savedSsid,true,true);sel.add(o);}
  }).catch(()=>{sel.innerHTML='';const opt=document.createElement('option');opt.value=savedSsid;opt.textContent=savedSsid||tr('opt_scan_error');sel.appendChild(opt);});
}
function loadConfig(){const draft=loadDraft();fetch('/load').then(r=>r.json()).then(d=>{
  const g=(k,dv)=>(draft&&draft[k]!==undefined)?draft[k]:dv;
  document.getElementById('wifi_enabled').checked=g('wifi_enabled',d.wifi_enabled==='1');
  savedSsid=g('wifi_ssid',d.wifi_ssid||''); // scanWiFi() se charge de le (re)selectionner, meme si absent du scan (fallback deja en place)
  document.getElementById('wifi_password').value=g('wifi_password',d.wifi_password||'');
  document.getElementById('wifi_static_enabled').checked=g('wifi_static_enabled',d.wifi_static_enabled==='1');
  document.getElementById('wifi_static_ip').value=g('wifi_static_ip',d.wifi_static_ip||'');
  document.getElementById('wifi_gateway').value=g('wifi_gateway',d.wifi_gateway||'');
  document.getElementById('wifi_subnet').value=g('wifi_subnet',d.wifi_subnet||'');
  document.getElementById('wifi_dns1').value=g('wifi_dns1',d.wifi_dns1||'');
  document.getElementById('wifi_dns2').value=g('wifi_dns2',d.wifi_dns2||'');
  document.getElementById('bluetooth_enabled').checked=g('bluetooth_enabled',d.bluetooth_enabled==='1');
  document.getElementById('bluetooth_name').value=g('bluetooth_name',d.bluetooth_name||'');
  document.getElementById('recalbox_ip').value=g('recalbox_ip',d.recalbox_ip||'');
  scanWiFi();
  if(draft)_formDirty=true; // reboot/reprise doivent quand meme avertir : la config.ini reelle n'a pas ce brouillon
}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','network');
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);if(d.first_boot==='1'&&!sessionStorage.getItem('dmd_help_seen')){sessionStorage.setItem('dmd_help_seen','1');showHelpModal();}loadConfig();}).catch(()=>{applyLang();loadConfig();}).finally(hidePageLoadingOverlay);
document.getElementById('networkForm').addEventListener('input',()=>{_formDirty=true;saveDraft();});
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_CLOCK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Horloge</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.row input[type=color]{flex:0 0 60px;padding:2px}
.hint{flex:0 0 100%;font-size:11px;color:#666;margin-top:2px;margin-left:150px}
.btn-row{display:flex;gap:10px;justify-content:center;margin:18px 0;flex-wrap:wrap}
.btn{padding:12px 20px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#2d6a4f;color:#fff}
.btn-del{background:#555;color:#fff}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
#helpLink{position:absolute;top:14px;right:75px;font-size:13px;color:#8ab4f8;text-decoration:underline;cursor:pointer}
.help-backdrop{display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);z-index:1000;align-items:center;justify-content:center;padding:16px}
.help-backdrop.show{display:flex}
.help-box{background:#16213e;border-radius:8px;padding:20px;max-width:520px;max-height:85vh;overflow-y:auto;position:relative;text-align:left}
.help-box h2{color:#ffd146;font-size:16px;margin:0 22px 10px 0}
.help-box h3{color:#8ab4f8;font-size:14px;margin:14px 0 6px}
.help-box p{font-size:13px;line-height:1.5;margin:0 0 8px}
.help-box ul{margin:0 0 8px 18px;font-size:13px;line-height:1.5}
.help-close{position:absolute;top:10px;right:14px;background:none;border:none;color:#aaa;font-size:22px;cursor:pointer;line-height:1}
#pageLoadingOverlay{position:fixed;inset:0;background:#1a1a2e;color:#eee;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:16px;font-weight:600;z-index:99999;text-align:center;padding:20px;gap:14px}
#pageLoadingOverlay .pgspin{width:34px;height:34px;border:4px solid #333;border-top-color:#8ab4f8;border-radius:50%;animation:pgspin .8s linear infinite}
@keyframes pgspin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div id="pageLoadingOverlay"><div class="pgspin"></div><div data-i18n="loading_text">Chargement en cours...</div></div>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<span id="helpLink" onclick="showHelpModal()" data-i18n="help_link">Aide</span>
<div id="helpBackdrop" class="help-backdrop" onclick="if(event.target===this)closeHelpModal()">
<div class="help-box">
<button class="help-close" onclick="closeHelpModal()">&times;</button>
<h2 data-i18n="help_title">Bienvenue</h2>
<p data-i18n="help_intro"></p>
<h3 data-i18n="help_checklist_title"></h3>
<ul>
<li data-i18n="help_check_ip"></li>
<li data-i18n="help_check_playlist"></li>
<li data-i18n="help_check_wifi"></li>
</ul>
<p id="helpUrlReminder"></p>
<h3 data-i18n="help_features_title"></h3>
<ul>
<li data-i18n="help_feat_playlists"></li>
<li data-i18n="help_feat_clock"></li>
<li data-i18n="help_feat_display"></li>
<li data-i18n="help_feat_network"></li>
</ul>
</div>
</div>
<div class="topnav">
<a href="/config/basic" onclick="showPageLoadingOverlay()" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" onclick="showPageLoadingOverlay()" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" onclick="showPageLoadingOverlay()" class="active" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" onclick="showPageLoadingOverlay()" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Horloge</h1>
<form id="clockForm" onsubmit="saveConfig(event)">
<div class="section">
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;e</label><input id="clock_enabled" type="checkbox"></div>
<div class="row"><label for="clock_theme" data-i18n="lbl_theme">Th&egrave;me</label>
<select id="clock_theme" onchange="onClockThemeChanged(this.value)">
<option value="-1" data-i18n="opt_random">Al&eacute;atoire</option>
<option value="0" data-i18n="opt_mario">Mario</option><option value="1" data-i18n="opt_tetris">Tetris</option>
<option value="2" data-i18n="opt_pacman">Pac-Man</option><option value="3" data-i18n="opt_spaceinv">Space Invaders</option>
<option value="4" data-i18n="opt_pong">Pong</option><option value="5" data-i18n="opt_neon">Neon</option>
<option value="6" data-i18n="opt_matrix">Matrix</option><option value="7" data-i18n="opt_fire">Fire</option>
<option value="8" data-i18n="opt_rainbow">Rainbow</option><option value="9" data-i18n="opt_level11">Level 1-1</option>
</select>
<div class="hint" data-i18n="hint_theme_live">&#x1F4A1; Aper&ccedil;u affich&eacute; en direct sur l'&eacute;cran DMD tant que cette page est ouverte.</div>
</div>
<div class="row"><label for="clock_neon_color" data-i18n="lbl_neon_color">Couleur Neon</label><input id="clock_neon_color" type="color" value="#ff2878">
<label style="flex:0 0 auto;font-size:13px;display:inline-flex;align-items:center;gap:4px;margin-left:10px"><input id="clock_neon_color_enabled" type="checkbox" style="flex:0 0 16px;width:16px;height:16px"> <span data-i18n="lbl_custom">Personnalis&eacute;e</span></label>
<div class="hint" data-i18n="hint_neon">Th&egrave;me Neon uniquement</div>
</div>
<div class="row"><label for="clock_interval" data-i18n="lbl_interval_gifs">Intervalle (GIFs)</label><input id="clock_interval" type="number" min="1" max="999"></div>
<div class="row"><label for="clock_interval_min" data-i18n="lbl_interval_min">Intervalle (min)</label><input id="clock_interval_min" type="number" min="0" max="999"><div class="hint" data-i18n="hint_interval_min">0 = d&eacute;sactiv&eacute;</div></div>
<div class="row"><label for="clock_duration" data-i18n="lbl_duration">Dur&eacute;e (sec)</label><input id="clock_duration" type="number" min="1" max="120"></div>
<div class="row"><label for="clock_tz" data-i18n="lbl_tz">Fuseau horaire</label>
<select id="clock_tz">
<option value="CET-1CEST,M3.5.0,M10.5.0/3" data-i18n="opt_tz_ce">France / Espagne / Allemagne / Italie</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0" data-i18n="opt_tz_uk">Angleterre (UK) / Portugal</option>
<option value="EST5EDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_e">USA - Est (New York)</option>
<option value="CST6CDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_c">USA - Centre (Chicago)</option>
<option value="MST7MDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_m">USA - Montagnes (Denver)</option>
<option value="PST8PDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_p">USA - Pacifique (Los Angeles)</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4" data-i18n="opt_tz_ee">Gr&egrave;ce / Roumanie / Finlande</option>
<option value="UTC5">UTC-5</option><option value="UTC4">UTC-4</option><option value="UTC3">UTC-3</option>
<option value="UTC2">UTC-2</option><option value="UTC1">UTC-1</option><option value="UTC0">UTC+0</option>
<option value="UTC-1">UTC+1</option><option value="UTC-2">UTC+2</option><option value="UTC-3">UTC+3</option>
<option value="UTC-4">UTC+4</option><option value="UTC-5">UTC+5</option>
</select>
</div>
</div>
<div class="btn-row">
<button type="submit" class="btn btn-save" data-i18n="btn_save">&#x1F4BE; Enregistrer</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-del" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
</form>
<div id="msg" class="msg"></div>
<script>
const HELP_I18N={
fr:{help_link:'Aide',help_title:'Bienvenue sur la configuration du DMD',help_intro:'Voici ce qu\'il reste à vérifier avant de sauvegarder, et un résumé de ce que permet cette interface.',help_checklist_title:'À vérifier avant de sauvegarder',help_check_ip:'IP Recalbox renseignée (page Wi-Fi & Bluetooth)',help_check_playlist:'Playlist par défaut renseignée (page Affichage & Playlists)',help_check_wifi:'Le Wi-Fi est déjà validé à ce stade — inutile d\'y retoucher, sauf si vous voulez le changer',help_url_reminder:'Cette page reste accessible à tout moment en tapant l\'IP du DMD dans un navigateur — actuellement {ip}',help_features_title:'Ce que permet cette interface',help_feat_playlists:'GIFs (page Médias) : ajouter des GIFs sur la carte SD (upload direct depuis le navigateur, création de dossiers) — les playlists qui référencent un dossier modifié sont mises à jour automatiquement',help_feat_clock:'Horloge (page Horloge) : thème, couleur néon, intervalle et durée d\'affichage, fuseau horaire',help_feat_display:'Affichage, luminosité et playlists (page Affichage & Playlists) : luminosité de l\'écran, choix entre démarrage silencieux (titre seul) ou normal (IP détectée, synchronisation de l\'heure, etc.), sélection de la playlist par défaut, et création/suppression de playlists à partir des dossiers de GIFs',help_feat_network:'Réseau (page Wi-Fi & Bluetooth) : IP Recalbox (connexion MQTT), Wi-Fi (réseau, mot de passe, IP statique)'},
en:{help_link:'Help',help_title:'Welcome to the DMD configuration',help_intro:'Here is what\'s left to check before saving, and a summary of what this interface lets you do.',help_checklist_title:'To check before saving',help_check_ip:'Recalbox IP filled in (Wi-Fi & Bluetooth page)',help_check_playlist:'Default playlist filled in (Display & Playlists page)',help_check_wifi:'Wi-Fi is already validated at this stage — no need to touch it again, unless you want to change it',help_url_reminder:'This page stays accessible at any time by typing the DMD\'s IP in a browser — currently {ip}',help_features_title:'What this interface lets you do',help_feat_playlists:'GIFs (Media page): add GIFs to the SD card (direct upload from the browser, folder creation) — playlists referencing a modified folder are updated automatically',help_feat_clock:'Clock (Clock page): theme, custom neon color, display interval and duration, time zone',help_feat_display:'Display, brightness and playlists (Display & Playlists page): screen brightness, choice between silent startup (title only) or normal (detected IP, time sync, etc.), default playlist selection, and creating/deleting playlists from GIF folders',help_feat_network:'Network (Wi-Fi & Bluetooth page): Recalbox IP (MQTT connection), Wi-Fi (network, password, static IP)'},
es:{help_link:'Ayuda',help_title:'Bienvenido a la configuración del DMD',help_intro:'Esto es lo que falta comprobar antes de guardar, y un resumen de lo que permite esta interfaz.',help_checklist_title:'A comprobar antes de guardar',help_check_ip:'IP de Recalbox indicada (página Wi-Fi y Bluetooth)',help_check_playlist:'Playlist por defecto indicada (página Pantalla y listas)',help_check_wifi:'El Wi-Fi ya está validado en esta etapa — no hace falta tocarlo, salvo que quiera cambiarlo',help_url_reminder:'Esta página sigue accesible en cualquier momento escribiendo la IP del DMD en un navegador — actualmente {ip}',help_features_title:'Qué permite esta interfaz',help_feat_playlists:'GIFs (página Medios): añadir GIFs a la tarjeta SD (subida directa desde el navegador, creación de carpetas) — las playlists que referencian una carpeta modificada se actualizan automáticamente',help_feat_clock:'Reloj (página Reloj): tema, color neón personalizado, intervalo y duración de visualización, zona horaria',help_feat_display:'Pantalla, brillo y listas (página Pantalla y listas): brillo de la pantalla, elección entre inicio silencioso (solo título) o normal (IP detectada, sincronización horaria, etc.), selección de la playlist por defecto, y creación/eliminación de playlists a partir de las carpetas de GIFs',help_feat_network:'Red (página Wi-Fi y Bluetooth): IP de Recalbox (conexión MQTT), Wi-Fi (red, contraseña, IP estática)'}
};
function showHelpModal(){
  document.getElementById('helpBackdrop').classList.add('show');
  const p=document.getElementById('helpUrlReminder');
  if(p) p.textContent=((HELP_I18N[currentLang]&&HELP_I18N[currentLang].help_url_reminder)||HELP_I18N.fr.help_url_reminder).replace('{ip}',window.location.host);
}
function closeHelpModal(){document.getElementById('helpBackdrop').classList.remove('show');}
const PAGE_I18N={
fr:{title:'RecalBox DMD - Horloge',h1:'Horloge',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',lbl_enabled:'Activée',lbl_theme:'Thème',hint_theme_live:'&#x1F4A1; Aperçu affiché en direct sur l\'écran DMD tant que cette page est ouverte.',lbl_neon_color:'Couleur Neon',lbl_custom:'Personnalisée',hint_neon:'Thème Neon uniquement',lbl_interval_gifs:'Intervalle (GIFs)',lbl_interval_min:'Intervalle (min)',hint_interval_min:'0 = désactivé',lbl_duration:'Durée (sec)',lbl_tz:'Fuseau horaire',opt_random:'Aléatoire',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'France / Espagne / Allemagne / Italie',opt_tz_uk:'Angleterre (UK) / Portugal',opt_tz_usa_e:'USA - Est (New York)',opt_tz_usa_c:'USA - Centre (Chicago)',opt_tz_usa_m:'USA - Montagnes (Denver)',opt_tz_usa_p:'USA - Pacifique (Los Angeles)',opt_tz_ee:'Grèce / Roumanie / Finlande',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config',essential_wifi:'Wi-Fi',essential_playlist:'Playlist par défaut',essential_ip:'IP Recalbox',msg_essential_missing:'Attention : champ(s) essentiel(s) vide(s) : {fields}. Le DMD risque de ne pas fonctionner correctement. Continuer quand même ?',loading_text:'Chargement en cours...',...HELP_I18N.fr},
en:{title:'RecalBox DMD - Clock',h1:'Clock',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',lbl_enabled:'Enabled',lbl_theme:'Theme',hint_theme_live:'&#x1F4A1; Live preview shown directly on the DMD screen while this page is open.',lbl_neon_color:'Neon color',lbl_custom:'Custom',hint_neon:'Neon theme only',lbl_interval_gifs:'Interval (GIFs)',lbl_interval_min:'Interval (min)',hint_interval_min:'0 = disabled',lbl_duration:'Duration (sec)',lbl_tz:'Timezone',opt_random:'Random',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'France / Spain / Germany / Italy',opt_tz_uk:'England (UK) / Portugal',opt_tz_usa_e:'USA - East (New York)',opt_tz_usa_c:'USA - Central (Chicago)',opt_tz_usa_m:'USA - Mountain (Denver)',opt_tz_usa_p:'USA - Pacific (Los Angeles)',opt_tz_ee:'Greece / Romania / Finland',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config',essential_wifi:'Wi-Fi',essential_playlist:'Default playlist',essential_ip:'Recalbox IP',msg_essential_missing:'Warning: missing essential field(s): {fields}. The DMD may not work correctly. Continue anyway?',loading_text:'Loading...',...HELP_I18N.en},
es:{title:'RecalBox DMD - Reloj',h1:'Reloj',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',lbl_enabled:'Activado',lbl_theme:'Tema',hint_theme_live:'&#x1F4A1; Vista previa mostrada en directo en la pantalla DMD mientras esta página esté abierta.',lbl_neon_color:'Color Neon',lbl_custom:'Personalizado',hint_neon:'Solo tema Neon',lbl_interval_gifs:'Intervalo (GIFs)',lbl_interval_min:'Intervalo (min)',hint_interval_min:'0 = desactivado',lbl_duration:'Duración (seg)',lbl_tz:'Zona horaria',opt_random:'Aleatorio',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'Francia / España / Alemania / Italia',opt_tz_uk:'Inglaterra (UK) / Portugal',opt_tz_usa_e:'EE.UU. - Este (Nueva York)',opt_tz_usa_c:'EE.UU. - Centro (Chicago)',opt_tz_usa_m:'EE.UU. - Montañas (Denver)',opt_tz_usa_p:'EE.UU. - Pacífico (Los Ángeles)',opt_tz_ee:'Grecia / Rumanía / Finlandia',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración',essential_wifi:'Wi-Fi',essential_playlist:'Playlist por defecto',essential_ip:'IP de Recalbox',msg_essential_missing:'Atención: falta(n) campo(s) esencial(es): {fields}. Es posible que el DMD no funcione correctamente. ¿Continuar de todos modos?',loading_text:'Cargando...',...HELP_I18N.es}
};
let currentLang='fr';
// Overlay "Chargement en cours..." (2026-08-05, demande utilisateur :
// ~3s d'attente sur mobile avant affichage, impression de plantage/envie
// de F5). Present des le tout premier octet du <body> (avant tout script)
// donc visible des que le navigateur commence a peindre la page, meme si
// le reste du transfert (page + donnees /lang, /load) prend encore du
// temps -- masque via hidePageLoadingOverlay(), appelee en toute fin de
// la chaine de bootstrap (succes ET echec, voir .finally() plus bas).
function showPageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='flex';}
function hidePageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='none';}
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  const savedTheme=document.getElementById('clock_theme').value;
  const savedTz=document.getElementById('clock_tz').value;
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.getElementById('clock_theme').value=savedTheme;
  document.getElementById('clock_tz').value=savedTz;
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
let _formDirty=false;
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function serialize(){return new URLSearchParams({clock_enabled:document.getElementById('clock_enabled').checked?'1':'0',clock_theme:document.getElementById('clock_theme').value,clock_interval:document.getElementById('clock_interval').value,clock_interval_min:document.getElementById('clock_interval_min').value,clock_duration:document.getElementById('clock_duration').value,clock_tz:document.getElementById('clock_tz').value,clock_neon_color:document.getElementById('clock_neon_color').value,clock_neon_color_enabled:document.getElementById('clock_neon_color_enabled').checked?'1':'0'});}
// Brouillon localStorage -- voir le commentaire complet sur la page BASIC
// (correctif "reglages perdus si on change de page", 2026-08-05).
const DRAFT_KEY='dmd_draft_clock';
const DRAFT_FIELDS=['clock_enabled','clock_theme','clock_interval','clock_interval_min','clock_duration','clock_tz','clock_neon_color','clock_neon_color_enabled'];
function loadDraft(){try{const raw=localStorage.getItem(DRAFT_KEY);return raw?JSON.parse(raw):null;}catch(e){return null;}}
function saveDraft(){const o={};DRAFT_FIELDS.forEach(id=>{const el=document.getElementById(id);if(!el)return;o[id]=(el.type==='checkbox')?el.checked:el.value;});localStorage.setItem(DRAFT_KEY,JSON.stringify(o));}
// v72 -- apercu en direct du theme horloge sur le DMD physique, immediat a
// la selection (pas de bouton dedie). Independant du brouillon localStorage
// ci-dessus et du bouton "Sauvegarder" -- affichage uniquement, /config.ini
// n'est jamais touche par cet appel.
function onClockThemeChanged(theme){
  fetch('/clock-preview',{method:'POST',body:'theme='+encodeURIComponent(theme),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});
}
// Arret de l'apercu en quittant la page (Basic/Network/Media sont des pages
// separees, pas des onglets JS -- un vrai dechargement de page a donc lieu
// a chaque navigation). navigator.sendBeacon() (pas fetch) est le seul
// moyen fiable d'envoyer une requete PENDANT le dechargement ; le Blob type
// "application/x-www-form-urlencoded" est necessaire pour que
// webServer->hasArg("theme") le reconnaisse cote firmware (un sendBeacon
// avec une simple string finit en text/plain, non reconnu comme formulaire
// par WebServer). Pas de filet de securite additionnel (choix utilisateur) :
// si sendBeacon echoue (fermeture brutale du navigateur), l'apercu reste
// affiche jusqu'au prochain evenement MQTT ou reboot.
function stopClockPreview(){
  try{navigator.sendBeacon('/clock-preview',new Blob(['theme=stop'],{type:'application/x-www-form-urlencoded'}));}catch(e){}
}
window.addEventListener('pagehide',stopClockPreview);
window.addEventListener('beforeunload',stopClockPreview);
function clearDraft(){localStorage.removeItem(DRAFT_KEY);}
function checkEssentialFields(){return fetch('/load').then(r=>r.json()).then(d=>{const missing=[];if(!d.wifi_ssid)missing.push(tr('essential_wifi'));if(!d.playlist)missing.push(tr('essential_playlist'));if(!d.recalbox_ip)missing.push(tr('essential_ip'));if(!missing.length)return true;return confirm(tr('msg_essential_missing').replace('{fields}',missing.join(', ')));}).catch(()=>true);}
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK')){_formDirty=false;clearDraft();}}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(skipConfirm){checkEssentialFields().then(ok=>{if(!ok)return;if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});});}
// skipConfirm=true (2026-07-29) : "Enreg. & Redemarrer" a deja un intitule
// explicite -- redemander confirmation juste apres la sauvegarde est
// redondant, contrairement au bouton "Redemarrer" seul.
function saveAndReboot(){saveConfig().then(()=>setTimeout(()=>doReboot(true),400));}
function dmdResume(){checkEssentialFields().then(ok=>{if(!ok)return;if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));});}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) : aucun
// moyen fiable de distinguer une vraie fermeture d'onglet/navigateur d'un
// simple rafraichissement de page (habitude trop ancree pour l'utilisateur,
// faux positifs trop frequents -- ni le JS ni le serveur ne peuvent
// distinguer les deux cas, une connexion qui se ferme se ressemble dans
// tous les cas).
function loadConfig(){const draft=loadDraft();fetch('/load').then(r=>r.json()).then(d=>{
  const g=(k,dv)=>(draft&&draft[k]!==undefined)?draft[k]:dv;
  document.getElementById('clock_enabled').checked=g('clock_enabled',d.clock_enabled==='1');
  document.getElementById('clock_theme').value=g('clock_theme',d.clock_theme||'0');
  document.getElementById('clock_interval').value=g('clock_interval',d.clock_interval||'0');
  document.getElementById('clock_interval_min').value=g('clock_interval_min',d.clock_interval_min||'0');
  document.getElementById('clock_duration').value=g('clock_duration',d.clock_duration||'0');
  document.getElementById('clock_tz').value=g('clock_tz',d.clock_tz||'UTC0');
  document.getElementById('clock_neon_color').value=g('clock_neon_color',d.clock_neon_color||'#ff2878');
  document.getElementById('clock_neon_color_enabled').checked=g('clock_neon_color_enabled',d.clock_neon_color_enabled==='1');
  if(draft)_formDirty=true; // reboot/reprise doivent quand meme avertir : la config.ini reelle n'a pas ce brouillon
  // v58 -- demande utilisateur : preview immediat du theme deja
  // preselectionne (config sauvegardee ou brouillon ci-dessus) a
  // l'ouverture de la page, sans avoir a re-cliquer dessus -- SAUF si
  // "Aleatoire" (-1) est preselectionne (rien de pertinent a previsualiser
  // pour un tirage non encore effectue).
  const initialTheme=document.getElementById('clock_theme').value;
  if(initialTheme!=='-1') onClockThemeChanged(initialTheme);
}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','clock');
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);if(d.first_boot==='1'&&!sessionStorage.getItem('dmd_help_seen')){sessionStorage.setItem('dmd_help_seen','1');showHelpModal();}loadConfig();}).catch(()=>{applyLang();loadConfig();}).finally(hidePageLoadingOverlay);
document.getElementById('clockForm').addEventListener('input',()=>{_formDirty=true;saveDraft();});
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_MEDIA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Médias</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
h2{color:#8ab4f8;font-size:15px;margin:0 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.desc{font-size:12px;color:#aaa;margin-bottom:8px}
.dirs{margin:8px 0;max-height:220px;overflow-y:auto}
.dirs label{display:flex;align-items:center;gap:8px;font-size:14px;padding:3px 0}
.dirs label span.name{flex:1}
.mini-row{display:flex;gap:8px;margin-bottom:8px}
.mini-btn{padding:4px 10px;border:none;border-radius:4px;background:#1a6b9e;color:#fff;font-size:11px;cursor:pointer}
.upload-row{display:flex;gap:6px;flex-wrap:wrap;margin:10px 0}
.upload-row select,.upload-row input{flex:1;min-width:110px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.btn-row{display:flex;gap:10px;justify-content:center;margin:14px 0;flex-wrap:wrap}
.btn{padding:10px 18px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-upload{background:#1a6b9e;color:#fff}
.btn-del{background:#555;color:#fff}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#0f766e;color:#fff}
.btn-stop{background:#c0392b;color:#fff}
.progress{height:4px;background:#333;border-radius:2px;margin:8px 0;display:none}
.progress-bar{height:4px;background:#52b788;border-radius:2px;width:0%}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
#helpLink{position:absolute;top:14px;right:75px;font-size:13px;color:#8ab4f8;text-decoration:underline;cursor:pointer}
.help-backdrop{display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);z-index:1000;align-items:center;justify-content:center;padding:16px}
.help-backdrop.show{display:flex}
.help-box{background:#16213e;border-radius:8px;padding:20px;max-width:520px;max-height:85vh;overflow-y:auto;position:relative;text-align:left}
.help-box h2{color:#ffd146;font-size:16px;margin:0 22px 10px 0}
.help-box h3{color:#8ab4f8;font-size:14px;margin:14px 0 6px}
.help-box p{font-size:13px;line-height:1.5;margin:0 0 8px}
.help-box ul{margin:0 0 8px 18px;font-size:13px;line-height:1.5}
.help-close{position:absolute;top:10px;right:14px;background:none;border:none;color:#aaa;font-size:22px;cursor:pointer;line-height:1}
#pageLoadingOverlay{position:fixed;inset:0;background:#1a1a2e;color:#eee;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:16px;font-weight:600;z-index:99999;text-align:center;padding:20px;gap:14px}
#pageLoadingOverlay .pgspin{width:34px;height:34px;border:4px solid #333;border-top-color:#8ab4f8;border-radius:50%;animation:pgspin .8s linear infinite}
@keyframes pgspin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div id="pageLoadingOverlay"><div class="pgspin"></div><div data-i18n="loading_text">Chargement en cours...</div></div>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<span id="helpLink" onclick="showHelpModal()" data-i18n="help_link">Aide</span>
<div id="helpBackdrop" class="help-backdrop" onclick="if(event.target===this)closeHelpModal()">
<div class="help-box">
<button class="help-close" onclick="closeHelpModal()">&times;</button>
<h2 data-i18n="help_title">Bienvenue</h2>
<p data-i18n="help_intro"></p>
<h3 data-i18n="help_checklist_title"></h3>
<ul>
<li data-i18n="help_check_ip"></li>
<li data-i18n="help_check_playlist"></li>
<li data-i18n="help_check_wifi"></li>
</ul>
<p id="helpUrlReminder"></p>
<h3 data-i18n="help_features_title"></h3>
<ul>
<li data-i18n="help_feat_playlists"></li>
<li data-i18n="help_feat_clock"></li>
<li data-i18n="help_feat_display"></li>
<li data-i18n="help_feat_network"></li>
</ul>
</div>
</div>
<div class="topnav">
<a href="/config/basic" onclick="showPageLoadingOverlay()" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" onclick="showPageLoadingOverlay()" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" onclick="showPageLoadingOverlay()" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" onclick="showPageLoadingOverlay()" class="active" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">M&eacute;dias</h1>
<div class="section">
<h2 data-i18n="sec_dirs">&#x1F4C1; Dossiers (/gifs/)</h2>
<div class="desc" data-i18n="desc_dirs">Cochez des dossiers pour les supprimer.</div>
<div class="mini-row">
<button type="button" class="mini-btn" onclick="selectAllDirs(true)" data-i18n="btn_select_all">Tout s&eacute;lectionner</button>
<button type="button" class="mini-btn" onclick="selectAllDirs(false)" data-i18n="btn_select_none">Rien s&eacute;lectionner</button>
</div>
<div id="dirList" class="dirs"></div>
<div class="btn-row">
<button type="button" class="btn btn-del" onclick="deleteSelected()" data-i18n="btn_delete_sel">&#x1F5D1; Supprimer la s&eacute;lection</button>
</div>
</div>
<div class="section">
<h2 data-i18n="sec_upload">&#x1F4E4; Envoi GIF</h2>
<div class="desc" data-i18n="desc_upload">Ajoutez un fichier .gif directement depuis votre navigateur dans un dossier de /gifs/. Choisissez un dossier existant OU tapez un nouveau nom (cr&eacute;&eacute; automatiquement). &#x26A0;&#xFE0F; Pas fait pour transferer de nombreux fichiers (debit lent, risque d'erreur d'ecriture) -- reserve a l'ajout ponctuel de quelques fichiers. Pour un transfert consequent, passez par l'utilitaire RecalboxDMD_tool sur PC.</div>
<div class="upload-row">
<select id="uploadDir"></select>
<input id="uploadDirCustom" data-i18n-placeholder="placeholder_upload_dir" placeholder="ou nouveau dossier...">
</div>
<div class="row"><label for="uploadFile" data-i18n="lbl_upload_file">Fichiers .gif</label><input id="uploadFile" type="file" accept=".gif" multiple></div>
<div class="progress" id="uploadProgress"><div class="progress-bar" id="uploadProgressBar"></div></div>
<div id="uploadFileList" style="margin:4px 0;font-size:12px;color:#aaa"></div>
<div class="btn-row">
<button type="button" class="btn btn-upload" onclick="uploadGif()" data-i18n="btn_upload">&#x1F4E4; Uploader</button>
<button type="button" class="btn btn-stop" id="uploadStopBtn" onclick="stopUpload()" style="display:none" data-i18n="btn_stop">&#x23F9; Arr&ecirc;ter</button>
</div>
</div>
<div class="btn-row">
<button type="button" class="btn btn-reboot" onclick="doReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
<div id="msg" class="msg"></div>
<script>
const HELP_I18N={
fr:{help_link:'Aide',help_title:'Bienvenue sur la configuration du DMD',help_intro:'Voici ce qu\'il reste à vérifier avant de sauvegarder, et un résumé de ce que permet cette interface.',help_checklist_title:'À vérifier avant de sauvegarder',help_check_ip:'IP Recalbox renseignée (page Wi-Fi & Bluetooth)',help_check_playlist:'Playlist par défaut renseignée (page Affichage & Playlists)',help_check_wifi:'Le Wi-Fi est déjà validé à ce stade — inutile d\'y retoucher, sauf si vous voulez le changer',help_url_reminder:'Cette page reste accessible à tout moment en tapant l\'IP du DMD dans un navigateur — actuellement {ip}',help_features_title:'Ce que permet cette interface',help_feat_playlists:'GIFs (page Médias) : ajouter des GIFs sur la carte SD (upload direct depuis le navigateur, création de dossiers) — les playlists qui référencent un dossier modifié sont mises à jour automatiquement',help_feat_clock:'Horloge (page Horloge) : thème, couleur néon, intervalle et durée d\'affichage, fuseau horaire',help_feat_display:'Affichage, luminosité et playlists (page Affichage & Playlists) : luminosité de l\'écran, choix entre démarrage silencieux (titre seul) ou normal (IP détectée, synchronisation de l\'heure, etc.), sélection de la playlist par défaut, et création/suppression de playlists à partir des dossiers de GIFs',help_feat_network:'Réseau (page Wi-Fi & Bluetooth) : IP Recalbox (connexion MQTT), Wi-Fi (réseau, mot de passe, IP statique)'},
en:{help_link:'Help',help_title:'Welcome to the DMD configuration',help_intro:'Here is what\'s left to check before saving, and a summary of what this interface lets you do.',help_checklist_title:'To check before saving',help_check_ip:'Recalbox IP filled in (Wi-Fi & Bluetooth page)',help_check_playlist:'Default playlist filled in (Display & Playlists page)',help_check_wifi:'Wi-Fi is already validated at this stage — no need to touch it again, unless you want to change it',help_url_reminder:'This page stays accessible at any time by typing the DMD\'s IP in a browser — currently {ip}',help_features_title:'What this interface lets you do',help_feat_playlists:'GIFs (Media page): add GIFs to the SD card (direct upload from the browser, folder creation) — playlists referencing a modified folder are updated automatically',help_feat_clock:'Clock (Clock page): theme, custom neon color, display interval and duration, time zone',help_feat_display:'Display, brightness and playlists (Display & Playlists page): screen brightness, choice between silent startup (title only) or normal (detected IP, time sync, etc.), default playlist selection, and creating/deleting playlists from GIF folders',help_feat_network:'Network (Wi-Fi & Bluetooth page): Recalbox IP (MQTT connection), Wi-Fi (network, password, static IP)'},
es:{help_link:'Ayuda',help_title:'Bienvenido a la configuración del DMD',help_intro:'Esto es lo que falta comprobar antes de guardar, y un resumen de lo que permite esta interfaz.',help_checklist_title:'A comprobar antes de guardar',help_check_ip:'IP de Recalbox indicada (página Wi-Fi y Bluetooth)',help_check_playlist:'Playlist por defecto indicada (página Pantalla y listas)',help_check_wifi:'El Wi-Fi ya está validado en esta etapa — no hace falta tocarlo, salvo que quiera cambiarlo',help_url_reminder:'Esta página sigue accesible en cualquier momento escribiendo la IP del DMD en un navegador — actualmente {ip}',help_features_title:'Qué permite esta interfaz',help_feat_playlists:'GIFs (página Medios): añadir GIFs a la tarjeta SD (subida directa desde el navegador, creación de carpetas) — las playlists que referencian una carpeta modificada se actualizan automáticamente',help_feat_clock:'Reloj (página Reloj): tema, color neón personalizado, intervalo y duración de visualización, zona horaria',help_feat_display:'Pantalla, brillo y listas (página Pantalla y listas): brillo de la pantalla, elección entre inicio silencioso (solo título) o normal (IP detectada, sincronización horaria, etc.), selección de la playlist por defecto, y creación/eliminación de playlists a partir de las carpetas de GIFs',help_feat_network:'Red (página Wi-Fi y Bluetooth): IP de Recalbox (conexión MQTT), Wi-Fi (red, contraseña, IP estática)'}
};
function showHelpModal(){
  document.getElementById('helpBackdrop').classList.add('show');
  const p=document.getElementById('helpUrlReminder');
  if(p) p.textContent=((HELP_I18N[currentLang]&&HELP_I18N[currentLang].help_url_reminder)||HELP_I18N.fr.help_url_reminder).replace('{ip}',window.location.host);
}
function closeHelpModal(){document.getElementById('helpBackdrop').classList.remove('show');}
const PAGE_I18N={
fr:{title:'RecalBox DMD - Médias',h1:'Médias',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',
sec_dirs:'&#x1F4C1; Dossiers (/gifs/)',desc_dirs:'Cochez des dossiers pour les supprimer.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',btn_delete_sel:'&#x1F5D1; Supprimer la sélection',
sec_upload:'&#x1F4E4; Envoi GIF',desc_upload:'Ajoutez un fichier .gif directement depuis votre navigateur dans un dossier de /gifs/. Choisissez un dossier existant OU tapez un nouveau nom (créé automatiquement). &#x26A0;&#xFE0F; Pas fait pour transférer de nombreux fichiers (débit lent, risque d\'erreur d\'écriture) -- réservé à l\'ajout ponctuel de quelques fichiers. Pour un transfert consequent, passez par l\'utilitaire RecalboxDMD_tool sur PC.',placeholder_upload_dir:'ou nouveau dossier...',lbl_upload_file:'Fichiers .gif',btn_upload:'&#x1F4E4; Uploader',btn_stop:'&#x23F9; Arrêter',
btn_reboot:'&#x1F504; Redémarrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',
net_error:'Erreur réseau',msg_deleting:'Suppression...',msg_select_folder:'Choisissez au moins un dossier',msg_confirm_delete_folders:'Supprimer ${0} ?',msg_specify_dir:'Précisez un dossier cible',msg_select_gif:'Choisissez un fichier GIF',msg_select_gif_files:'Choisissez des fichiers .gif',msg_preparing_folder:'Preparation du dossier...',msg_cannot_create_folder:'Impossible de creer le dossier: ${0}',msg_net_error_folder:'Erreur reseau (creation dossier)',msg_uploading:'Upload...',msg_attempt:'tentative ${0}/${1}',msg_stopped_by_user:'Arrete par l\'utilisateur (${0}/${1})',msg_upload_fail:'ECHEC',msg_failures:'Echecs: ${0}',msg_upload_result:'${0}/${1} fichier(s) uploade(s)',msg_upload_result_fail:' -- echecs: ${0}',msg_confirm_reboot:'Redemarrer l\'ESP32 ?',msg_rebooting:'Redemarrage...',msg_dmd_resumed:'DMD repris',msg_updating_playlists:'Mise a jour des playlists...',msg_confirm_reboot_playlists:'Dossiers supprimes, ${0} playlist(s) mise(s) a jour. La suppression d\'un dossier lie a des playlists necessite un redemarrage du DMD pour etre prise en compte. Redemarrer maintenant ?',msg_retrying_failed:'Nouvelle tentative pour ${0} fichier(s) en echec...',msg_final_attempt:'tentative finale ${0}/${1}',msg_preparing_upload:'Preparation de l\'upload...',msg_rebooting_upload:'Redemarrage du DMD pour preparer la copie de fichiers, veuillez patienter...',essential_wifi:'Wi-Fi',essential_playlist:'Playlist par défaut',essential_ip:'IP Recalbox',msg_essential_missing:'Attention : champ(s) essentiel(s) vide(s) : {fields}. Le DMD risque de ne pas fonctionner correctement. Continuer quand même ?',loading_text:'Chargement en cours...',...HELP_I18N.fr},
en:{title:'RecalBox DMD - Media',h1:'Media',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',
sec_dirs:'&#x1F4C1; Folders (/gifs/)',desc_dirs:'Check folders to delete them.',btn_select_all:'Select all',btn_select_none:'Select none',btn_delete_sel:'&#x1F5D1; Delete selection',
sec_upload:'&#x1F4E4; GIF Upload',desc_upload:'Add a .gif file directly from your browser into a folder in /gifs/. Choose an existing folder OR type a new name (created automatically). &#x26A0;&#xFE0F; Not designed for transferring many files (slow throughput, risk of write errors) -- meant for occasionally adding a few files. For a large transfer, use the RecalboxDMD_tool utility on PC instead.',placeholder_upload_dir:'or new folder...',lbl_upload_file:'.gif files',btn_upload:'&#x1F4E4; Upload',btn_stop:'&#x23F9; Stop',
btn_reboot:'&#x1F504; Reboot',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_resume:'&#x25B6; Resume DMD',
net_error:'Network error',msg_deleting:'Deleting...',msg_select_folder:'Select at least one folder',msg_confirm_delete_folders:'Delete ${0}?',msg_specify_dir:'Please specify a target folder',msg_select_gif:'Select a GIF file',msg_select_gif_files:'Select .gif files',msg_preparing_folder:'Preparing folder...',msg_cannot_create_folder:'Unable to create folder: ${0}',msg_net_error_folder:'Network error (folder creation)',msg_uploading:'Uploading...',msg_attempt:'attempt ${0}/${1}',msg_stopped_by_user:'Stopped by user (${0}/${1})',msg_upload_fail:'FAILED',msg_failures:'Failures: ${0}',msg_upload_result:'${0}/${1} file(s) uploaded',msg_upload_result_fail:' -- failures: ${0}',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_updating_playlists:'Updating playlists...',msg_confirm_reboot_playlists:'Folders deleted, ${0} playlist(s) updated. Deleting a folder linked to playlists requires a DMD reboot to take effect. Reboot now?',msg_retrying_failed:'Retrying ${0} failed file(s)...',msg_final_attempt:'final attempt ${0}/${1}',msg_preparing_upload:'Preparing upload...',msg_rebooting_upload:'Rebooting the DMD to prepare the file copy, please wait...',essential_wifi:'Wi-Fi',essential_playlist:'Default playlist',essential_ip:'Recalbox IP',msg_essential_missing:'Warning: missing essential field(s): {fields}. The DMD may not work correctly. Continue anyway?',loading_text:'Loading...',...HELP_I18N.en},
es:{title:'RecalBox DMD - Medios',h1:'Medios',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',
sec_dirs:'&#x1F4C1; Carpetas (/gifs/)',desc_dirs:'Marque las carpetas para eliminarlas.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',btn_delete_sel:'&#x1F5D1; Eliminar selección',
sec_upload:'&#x1F4E4; Subir GIF',desc_upload:'Añada un archivo .gif desde su navegador a una carpeta en /gifs/. Elija una carpeta existente O escriba un nombre nuevo (se crea automáticamente). &#x26A0;&#xFE0F; No pensado para transferir muchos archivos (velocidad lenta, riesgo de error de escritura) -- reservado para añadir algunos archivos puntualmente. Para una transferencia importante, use la utilidad RecalboxDMD_tool en el PC.',placeholder_upload_dir:'o nueva carpeta...',lbl_upload_file:'Archivos .gif',btn_upload:'&#x1F4E4; Subir',btn_stop:'&#x23F9; Detener',
btn_reboot:'&#x1F504; Reiniciar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_resume:'&#x25B6; Reanudar DMD',
net_error:'Error de red',msg_deleting:'Eliminando...',msg_select_folder:'Elija al menos una carpeta',msg_confirm_delete_folders:'¿Eliminar ${0}?',msg_specify_dir:'Especifique una carpeta destino',msg_select_gif:'Seleccione un archivo GIF',msg_select_gif_files:'Seleccione archivos .gif',msg_preparing_folder:'Preparando carpeta...',msg_cannot_create_folder:'No se pudo crear la carpeta: ${0}',msg_net_error_folder:'Error de red (creación de carpeta)',msg_uploading:'Subiendo...',msg_attempt:'intento ${0}/${1}',msg_stopped_by_user:'Detenido por el usuario (${0}/${1})',msg_upload_fail:'ERROR',msg_failures:'Errores: ${0}',msg_upload_result:'${0}/${1} archivo(s) subido(s)',msg_upload_result_fail:' -- errores: ${0}',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_updating_playlists:'Actualizando listas...',msg_confirm_reboot_playlists:'Carpetas eliminadas, ${0} lista(s) de reproduccion actualizada(s). Eliminar una carpeta vinculada a listas requiere reiniciar el DMD para aplicarse. ¿Reiniciar ahora?',msg_retrying_failed:'Reintentando ${0} archivo(s) fallido(s)...',msg_final_attempt:'intento final ${0}/${1}',msg_preparing_upload:'Preparando la subida...',msg_rebooting_upload:'Reiniciando el DMD para preparar la copia de archivos, por favor espere...',essential_wifi:'Wi-Fi',essential_playlist:'Playlist por defecto',essential_ip:'IP de Recalbox',msg_essential_missing:'Atención: falta(n) campo(s) esencial(es): {fields}. Es posible que el DMD no funcione correctamente. ¿Continuar de todos modos?',loading_text:'Cargando...',...HELP_I18N.es}
};
let currentLang='fr';
// Overlay "Chargement en cours..." (2026-08-05, demande utilisateur :
// ~3s d'attente sur mobile avant affichage, impression de plantage/envie
// de F5). Present des le tout premier octet du <body> (avant tout script)
// donc visible des que le navigateur commence a peindre la page, meme si
// le reste du transfert (page + donnees /lang, /load) prend encore du
// temps -- masque via hidePageLoadingOverlay(), appelee en toute fin de
// la chaine de bootstrap (succes ET echec, voir .finally() plus bas).
function showPageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='flex';}
function hidePageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='none';}
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function trTpl(k){const args=[].slice.call(arguments,1);let s=tr(k);args.forEach((v,i)=>{s=s.split('${'+i+'}').join(v);});return s;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.querySelectorAll('[data-i18n-placeholder]').forEach(function(el){el.placeholder=tr(el.dataset.i18nPlaceholder);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
// File d'attente globale : l'ESP32 (WebServer mono-requete) ne traite
// qu'une connexion a la fois. Plusieurs fetch() partis en parallele (ex.
// les appels de chargement initial + un clic utilisateur pendant ce temps)
// se faisaient concurrence sur la meme connexion -- confirme en test reel
// comme net::ERR_INVALID_CHUNKED_ENCODING, meme sur des reponses courtes
// servies depuis le cache. TOUS les fetch() de cette page passent
// desormais par queuedFetch(), qui les serialise strictement.
let _reqQueue=Promise.resolve();
function queuedFetch(url,opts){
  const p=_reqQueue.then(()=>fetch(url,opts));
  _reqQueue=p.catch(()=>{});
  return p;
}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function checkEssentialFields(){return queuedFetch('/load').then(r=>r.json()).then(d=>{const missing=[];if(!d.wifi_ssid)missing.push(tr('essential_wifi'));if(!d.playlist)missing.push(tr('essential_playlist'));if(!d.recalbox_ip)missing.push(tr('essential_ip'));if(!missing.length)return true;return confirm(tr('msg_essential_missing').replace('{fields}',missing.join(', ')));}).catch(()=>true);}
function doReboot(skipConfirm){checkEssentialFields().then(ok=>{if(!ok)return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);queuedFetch('/reboot').catch(()=>{});});}
function dmdResume(){checkEssentialFields().then(ok=>{if(!ok)return;queuedFetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('net_error'),false));});}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29, voir
// page Affichage pour le detail) : aucun moyen fiable de distinguer une
// vraie fermeture d'un simple rafraichissement de page.
function selectAllDirs(v){document.querySelectorAll('#dirList input').forEach(i=>i.checked=v);}
// v85 : plus de navigation dans un dossier (contenu individuel des GIF) ni
// de statut cached/excluded -- decision utilisateur de retirer cette
// fonctionnalite (voir changelog web_config.h). renderDirs() redevient une
// simple liste de dossiers a cocher (creation/suppression/upload
// uniquement).
function renderDirs(dirs){
  const list=document.getElementById('dirList');list.innerHTML='';
  const sel=document.getElementById('uploadDir');sel.innerHTML='';
  const opt=document.createElement('option');opt.value='';opt.textContent='---';sel.appendChild(opt);
  dirs.forEach(d=>{
    const name=(d&&typeof d==='object')?d.name:d;
    const row=document.createElement('label');
    row.innerHTML='<input type="checkbox" value="'+name+'"><span class="name">&#x1F4C1; '+name+'</span>';
    list.appendChild(row);
    const o=document.createElement('option');o.value=name;o.textContent=name;sel.appendChild(o);
  });
}
// loadDirs() et loadUploadDirs() appelaient chacun /lsgifdirs
// independamment (2 scans SD + 2 parsings JSON pour la MEME donnee a
// chaque chargement de page/rafraichissement) -- fusionnes en un seul
// fetch partage pour reduire la pression heap qui contribuait au crash
// abort() observe en test reel apres plusieurs operations consecutives.
// B (plan cache_master_gifs, retour test reel 2026-08-01) -- retry (5
// tentatives, 500ms d'ecart), meme raison et meme pattern que loadGenDirs()
// (page Affichage, deja corrige le 2026-07-29 pour EXACTEMENT ce probleme) :
// un simple fetch().catch(()=>{}) laissait la liste MEDIA vide en silence,
// sans retenter, des qu'une seule requete /lsgifdirs echouait au chargement
// de la page -- necessitait un F5 manuel pour reessayer. Cette fonction
// n'avait jamais recu le meme correctif que loadGenDirs() a l'epoque.
// Cache sessionStorage PARTAGE avec la page Affichage (meme cle,
// 'dmd_gifdirs_cache') -- demande utilisateur 2026-08-02 : le va-et-vient
// frequent entre Affichage et MEDIA redemandait /lsgifdirs a chaque fois,
// avec le risque d'echec reseau observe en test reel. Affiche IMMEDIATEMENT
// le contenu en cache si present (aucune attente reseau), PUIS rafraichit
// en arriere-plan et met a jour le cache -- la liste se corrige donc
// silencieusement si elle avait change entre-temps (creation/suppression de
// dossier), sans jamais bloquer l'affichage initial sur le reseau.
function readDirsCache(){
  try{
    const raw=sessionStorage.getItem('dmd_gifdirs_cache');
    return raw?JSON.parse(raw):null;
  }catch(e){return null;}
}
function writeDirsCache(dirs){
  try{sessionStorage.setItem('dmd_gifdirs_cache',JSON.stringify(dirs));}catch(e){}
}
async function loadDirs(){
  const cached=readDirsCache();
  if(cached&&cached.length)renderDirs(cached);
  for(let attempt=0;attempt<5;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{
      const dirs=await(await queuedFetch('/lsgifdirs')).json();
      if(!dirs.length&&attempt<4)continue;
      renderDirs(dirs);
      writeDirsCache(dirs);
      return;
    }catch(e){}
  }
}
function loadUploadDirs(){return Promise.resolve();} // conserve pour compatibilite des appels existants -- loadDirs() peuple desormais aussi #uploadDir
function deleteSelected(){
  const dirs=[].slice.call(document.querySelectorAll('#dirList input:checked')).map(i=>i.value);
  if(!dirs.length){showMsg(tr('msg_select_folder'),false);return;}
  if(!confirm(trTpl('msg_confirm_delete_folders',dirs.join(', '))))return;
  showMsg(tr('msg_deleting'),true);
  queuedFetch('/delete-folders',{method:'POST',body:new URLSearchParams({dirs:dirs.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{
      // A.3 (plan cache_master_gifs) -- si la reponse indique qu'au moins une
      // playlist a ete mise a jour (lignes mortes retirees), un redemarrage
      // est necessaire pour que la session de lecture EN COURS (deja son
      // cache playlist .idx charge en RAM) soit corrigee -- mais laisse a
      // l'utilisateur le choix du moment (demande utilisateur 2026-08-02,
      // popup oui/non plutot qu'un redemarrage automatique impose). confirm()
      // est bloquant : empeche aussi toute autre action pendant que cette
      // decision est en attente (demande utilisateur : "info web... pour
      // eviter une action utilisateur inappropriee").
      const m=t.match(/(\d+) playlist/);
      if(m){
        if(confirm(trTpl('msg_confirm_reboot_playlists',m[1]))){
          doReboot(true);
        } else {
          showMsg(t,true);loadDirs();loadUploadDirs();
        }
      } else {
        showMsg(t,t.includes('OK'));loadDirs();loadUploadDirs();
      }
    })
    .catch(()=>showMsg(tr('net_error'),false));
}
async function uploadGif(){
  const sel=document.getElementById('uploadDir');
  const custom=document.getElementById('uploadDirCustom').value.trim();
  const dir=custom||sel.value;
  if(!dir){showMsg(tr('msg_specify_dir'),false);return;}
  const fileInput=document.getElementById('uploadFile');
  if(!fileInput.files.length){showMsg(tr('msg_select_gif'),false);return;}
  const files=Array.from(fileInput.files).filter(f=>f.name.toLowerCase().endsWith('.gif'));
  if(!files.length){showMsg(tr('msg_select_gif_files'),false);return;}
  const msgEl=document.getElementById('msg');
  // Pre-vol reboot cible (v44, demande explicite utilisateur) : declenche
  // UNIQUEMENT au clic sur Uploader (pas a l'ouverture de la page MEDIA) --
  // si la playlist tourne depuis un moment, le heap est plafonne par la
  // fragmentation setvbuf(4096) accumulee au fil des GIFs ouverts (cf.
  // memoire projet) et un reboot cible (playlist sautee au prochain boot)
  // redonne le maximum de heap disponible AVANT le premier octet d'upload,
  // plutot que d'echouer en cours de route. Reponse JSON {"reboot":bool} :
  // si true, la reponse HTTP a deja ete envoyee cote serveur et le reboot
  // reel survient dans l'instant qui suit (requestReboot, RecalBox_DMD.ino).
  // IMPORTANT (bug corrige 2026-08-03, retour test reel "DMD bloque, aucun
  // affichage") : ne JAMAIS faire location.reload() ici -- les fichiers
  // selectionnes par l'utilisateur (variable `files` ci-dessus, objets File
  // du navigateur) ne survivent PAS a une navigation/rechargement de page,
  // l'upload etait donc silencieusement abandonne sans aucune indication.
  // On attend juste (poll sur /lang, endpoint leger qui ne re-arme pas le
  // mode config) que le serveur reponde de nouveau, PUIS on continue cette
  // meme fonction avec les memes fichiers deja en memoire -- aucune perte de
  // selection, aucun reclic necessaire.
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_preparing_upload');
  try{
    const pr=await queuedFetch('/prepare-upload',{method:'POST'});
    const pd=await pr.json();
    if(pd.reboot){
      msgEl.textContent=tr('msg_rebooting_upload');
      await new Promise(resolve=>{
        function poll(){
          fetch('/lang',{cache:'no-store'}).then(function(r){
            if(r.ok) resolve(); else setTimeout(poll,1500);
          }).catch(function(){setTimeout(poll,1500);});
        }
        setTimeout(poll,1500);
      });
      msgEl.textContent=tr('msg_preparing_upload');
    }
  }catch(e){/* pas de reponse ou probleme reseau ponctuel -- poursuivre normalement, le garde heap d'UPLOAD_FILE_START reste la derniere protection */}
  _uploadStopRequested=false;
  const stopBtn=document.getElementById('uploadStopBtn');stopBtn.style.display='inline-block';
  const bar=document.getElementById('uploadProgress');bar.style.display='block';
  const barInner=document.getElementById('uploadProgressBar');
  const fileList=document.getElementById('uploadFileList');
  msgEl.textContent=tr('msg_preparing_folder');
  try{
    const cr=await queuedFetch('/create-folder',{method:'POST',body:new URLSearchParams({dir:dir}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});
    const ct=await cr.text();
    if(!ct.includes('OK')){stopBtn.style.display='none';showMsg(trTpl('msg_cannot_create_folder',ct),false);return;}
    // v92 -- ne rafraichir la liste des dossiers que si /create-folder en a
    // reellement cree un NOUVEAU ("OK: cree") : pour un dossier deja
    // existant ("OK: existant", cas le plus courant), la liste affichee
    // depuis le chargement de la page est deja a jour -- ce fetch /lsgifdirs
    // supplementaire n'apportait rien et ajoutait un cout heap par requete
    // mesure en test reel (diagnostic v90).
    if(ct.indexOf('cree')>=0){await loadDirs();await loadUploadDirs();}
  }catch(e){stopBtn.style.display='none';showMsg(tr('msg_net_error_folder'),false);return;}
  msgEl.textContent=tr('msg_uploading');
  let okCount=0;const failed=[];const uploaded=[];
  // B (plan fiabilisation upload, 2026-08-02) -- regroupement par paquets de
  // 4 fichiers ESSAYE puis ABANDONNE (retour test reel) : ne reduisait pas
  // la frequence du crash out-of-memory dans WebServer::_parseForm() (celui-
  // ci se produit a CHAQUE fichier rencontre dans le corps multipart,
  // regroupes ou non -- voir HTTP_UPLOAD_BUFLEN en tete de fichier pour la
  // cause racine reelle et le correctif applique), pour une complexite/risque
  // de regression superieurs (paquet plus gros = pression heap potentiellement
  // accrue). Retour a l'envoi simple fichier par fichier -- reponse JSON du
  // serveur ({ok,files:[{name,ok,err}]}) deja compatible avec un seul fichier
  // par requete, aucun changement cote handleWebConfigUploadFile() necessaire.
  // Detection d'echec plus rapide (2026-08-02, analyse .har navigateur) --
  // sans ceci, un hoquet reseau/heap au milieu d'un transfert laisse le
  // navigateur attendre le timeout TCP par defaut du systeme (15-70s+
  // mesures dans le .har) avant meme de lancer une nouvelle tentative.
  // AbortController a 12s ESSAYE PUIS REMONTE A 25s (retour test reel :
  // 12s coupait des transferts LENTS MAIS QUI AURAIENT REUSSI -- observe
  // jusqu'a ~11s pour un succes reel sous heap tendu, tres proche de
  // l'ancienne limite -- plus court que le propre timeout serveur
  // (webServer->client().setTimeout(15000) pendant l'upload), cette
  // coupure prematuree cote client augmentait le nombre d'echecs par
  // rapport au comportement d'origine (aucun timeout client du tout).
  // 25s : marge confortable au-dessus des 15s serveur et des transferts
  // lents deja observes, tout en restant nettement plus rapide que le
  // pire cas mesure (72s) pour detecter un VRAI blocage. Factorisee
  // (2026-08-02) pour etre reutilisee aussi par la passe de re-tentative
  // finale ci-dessous.
  async function uploadOneFile(file,label){
    let ok=false;
    for(let attempt=0;attempt<3&&!ok;attempt++){
      if(attempt>0){
        const attemptTxt=label+' - '+trTpl('msg_attempt',attempt+1,3);
        fileList.textContent=attemptTxt;
        try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:attemptTxt,color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
        await new Promise(r=>setTimeout(r,500));
      }
      const form=new FormData();form.append('dir',dir);form.append('file',file);
      const ctrl=new AbortController();
      const abortTimer=setTimeout(()=>ctrl.abort(),25000);
      try{
        const r=await queuedFetch('/upload',{method:'POST',body:form,signal:ctrl.signal});
        const j=await r.json();
        if(j&&j.files&&j.files[0]&&j.files[0].ok)ok=true;
      }catch(e){}
      clearTimeout(abortTimer);
    }
    return ok;
  }
  for(let i=0;i<files.length;i++){
    if(_uploadStopRequested){fileList.textContent=trTpl('msg_stopped_by_user',i,files.length);break;}
    const file=files[i];
    const label=file.name+' ('+(i+1)+'/'+files.length+')';
    barInner.style.width=Math.max(Math.round(((i+1)/files.length)*100),5)+'%';
    fileList.textContent=label;
    msgEl.textContent=tr('msg_uploading')+' '+file.name;
    try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:label,color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
    const ok=await uploadOneFile(file,label);
    if(ok){okCount++;uploaded.push(file.name);fileList.textContent=file.name+' OK ('+okCount+'/'+files.length+')';}
    else {failed.push(file.name);fileList.textContent=file.name+' '+tr('msg_upload_fail');}
  }
  // Passe de re-tentative finale (demande utilisateur 2026-08-02, retour
  // test reel sur un lot de 34 fichiers -- 13 echecs, concentres plutot en
  // milieu/fin de lot, coherent avec une degradation progressive du tas au
  // fil d'un long upload). Une pause de 1.5s puis une derniere serie de
  // tentatives APRES la fin du lot principal laisse une chance au tas de se
  // stabiliser un peu (plus de contention SD simultanee avec le reste du
  // lot) avant de retenter uniquement les fichiers deja identifies en
  // echec -- jamais un nouveau scan de /gifs/.
  if (!_uploadStopRequested && failed.length) {
    const retryList = failed.splice(0, failed.length);
    msgEl.textContent = trTpl('msg_retrying_failed', retryList.length);
    await new Promise(r=>setTimeout(r,1500));
    for (let i = 0; i < retryList.length; i++) {
      if (_uploadStopRequested) { failed.push(...retryList.slice(i)); break; }
      const name = retryList[i];
      const file = files.find(f=>f.name===name);
      if (!file) { failed.push(name); continue; }
      const label = name + ' (' + trTpl('msg_final_attempt', i+1, retryList.length) + ')';
      fileList.textContent = label;
      try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:label,color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
      const ok = await uploadOneFile(file, label);
      if (ok) { okCount++; uploaded.push(file.name); } else { failed.push(name); }
    }
  }
  stopBtn.style.display='none';
  if(uploaded.length){
    msgEl.textContent=tr('msg_updating_playlists');
    try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(tr('msg_updating_playlists')),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
    try{await queuedFetch('/add-to-playlists-batch',{method:'POST',body:new URLSearchParams({dir:dir,files:uploaded.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
  }
  fileList.textContent=failed.length?trTpl('msg_failures',failed.join(', ')):'';
  document.getElementById('uploadDirCustom').value='';
  // v92 -- retire le rafraichissement /lsgifdirs de fin d'upload : uploader
  // des FICHIERS dans un dossier ne change jamais l'ENSEMBLE des noms de
  // dossiers affiches (#dirList/#uploadDir) -- un dossier nouvellement cree
  // est deja reflete par le rafraichissement conditionnel juste apres
  // /create-folder ci-dessus. Ce fetch etait systematiquement inutile et
  // ajoutait un cout heap par requete (diagnostic v90) -- c'est lui qui
  // echouait "heap critique, liste vide" apres plusieurs tentatives d'upload
  // ratees en test reel, faisant croire a une disparition de la liste.
  const result=trTpl('msg_upload_result',okCount,files.length)+(failed.length?trTpl('msg_upload_result_fail',failed.join(', ')):'');
  showMsg(result,failed.length===0);
}
function stopUpload(){_uploadStopRequested=true;}
let _uploadStopRequested=false;
localStorage.setItem('dmd_last_section','media');
// Sequence de chargement initial serialisee via queuedFetch() -- avant
// v45, /lang + loadDirs() + loadPlaylists() + loadUploadDirs() partaient
// TOUS en parallele au chargement de la page (4 requetes concurrentes sur
// un serveur qui n'en traite qu'une a la fois), confirme en test reel
// comme la cause de net::ERR_INVALID_CHUNKED_ENCODING des le premier clic
// sur un dossier si l'utilisateur cliquait pendant que ce lot initial
// etait encore en cours.
queuedFetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);if(d.first_boot==='1'&&!sessionStorage.getItem('dmd_help_seen')){sessionStorage.setItem('dmd_help_seen','1');showHelpModal();}}).catch(()=>{applyLang();}).finally(hidePageLoadingOverlay);
loadDirs();loadUploadDirs();
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_AP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:500px;margin:auto;display:flex;flex-direction:column;min-height:100vh;justify-content:center;position:relative}
h1{color:#ffd146;text-align:center;margin:16px 0;font-size:22px}
.section{background:#16213e;border-radius:8px;padding:24px;margin:12px 0}
.row{display:flex;flex-direction:column;margin:12px 0}
.row label{font-size:14px;color:#aaa;margin-bottom:4px}
.row input,.row select{padding:10px 12px;border:1px solid #555;border-radius:6px;background:#0f3460;color:#eee;font-size:16px}
.row input:focus,.row select:focus{outline:2px solid #ffd146}
.row select{width:100%}
.pwd-row{display:flex;gap:8px;align-items:center}
.pwd-row input{flex:1}
.pwd-toggle{background:none;border:none;color:#888;font-size:20px;cursor:pointer;padding:4px 8px}
.pwd-toggle:hover{color:#ffd146}
.btn-row{text-align:center;margin:20px 0}
.btn{padding:14px 40px;border:none;border-radius:6px;font-size:18px;font-weight:bold;cursor:pointer;background:#ffd146;color:#1a1a2e;width:100%}
.btn:hover{background:#ffe070}
.btn-scan{background:#1a6b9e;color:#fff;font-size:14px;padding:8px 16px;border:none;border-radius:4px;cursor:pointer;margin-top:4px;width:100%}
.btn-scan:hover{background:#2880b8}
.hint{font-size:13px;color:#888;text-align:center;margin:8px 0 4px;line-height:1.5}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:14px 24px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:16px;box-shadow:0 4px 16px rgba(0,0,0,.6);max-width:90%}
.msg-ok{background:#2d6a4f;color:#d8f3dc;border:2px solid #52b788}
.msg-err{background:#6b0f0f;color:#ffcccc;border:2px solid #e63946}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px}
#pageLoadingOverlay{position:fixed;inset:0;background:#1a1a2e;color:#eee;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:16px;font-weight:600;z-index:99999;text-align:center;padding:20px;gap:14px}
#pageLoadingOverlay .pgspin{width:34px;height:34px;border:4px solid #333;border-top-color:#8ab4f8;border-radius:50%;animation:pgspin .8s linear infinite}
@keyframes pgspin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div id="pageLoadingOverlay"><div class="pgspin"></div><div data-i18n="loading_text">Chargement en cours...</div></div>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<h1 data-i18n="h1">&#x1F4E1; Configuration WiFi</h1>
<div class="hint" data-i18n="hint">Connectez-vous au r&eacute;seau <b>RecalBox-DMD-Config</b> puis s&eacute;lectionnez votre WiFi.</div>
<div id="msg" class="msg"></div>
<div class="section">
<div class="row"><label data-i18n="lbl_wifi">R&eacute;seau WiFi</label>
<select id="wifi_ssid" style="width:100%"><option value="" data-i18n="scan_wait">-- Scan en cours... --</option></select>
<button class="btn-scan" onclick="scanWiFi()" data-i18n="btn_scan">&#x1F50D; Scanner les r&eacute;seaux</button>
<input type="text" id="wifi_ssid_text" data-i18n-placeholder="ph_manual" placeholder="Ou saisir le nom manuellement" style="width:100%;margin-top:4px;padding:8px 10px;border:1px solid #555;border-radius:4px;background:#0f3460;color:#eee;font-size:14px">
</div>
<div class="row"><label data-i18n="lbl_pwd">Mot de passe</label>
<div class="pwd-row"><input type="password" id="wifi_password" data-i18n-placeholder="ph_pwd" placeholder="Mot de passe WiFi"><button class="pwd-toggle" id="pwdToggle" onclick="togglePwd()">&#x1F441;</button></div>
</div>
<div class="row"><label data-i18n="lbl_static_ip">IP statique (optionnel)</label><input type="text" id="wifi_static_ip" data-i18n-placeholder="ph_static_ip" placeholder="Laisser vide pour DHCP"></div>
<div class="btn-row"><button class="btn" onclick="saveWiFi()" data-i18n="btn_save">&#x1F504; Sauvegarder &amp; Red&eacute;marrer</button></div>
</div>
<script>
// ============= I18N (page AP -- premier boot / mode secours WiFi) =============
const AP_I18N={
fr:{title:'RecalBox DMD',h1:'&#x1F4E1; Configuration WiFi',hint:'Connectez-vous au réseau <b>RecalBox-DMD-Config</b> puis sélectionnez votre WiFi.',lbl_wifi:'Réseau WiFi',scan_wait:'-- Scan en cours... --',btn_scan:'&#x1F50D; Scanner les réseaux',ph_manual:'Ou saisir le nom manuellement',lbl_pwd:'Mot de passe',ph_pwd:'Mot de passe WiFi',lbl_static_ip:'IP statique (optionnel)',ph_static_ip:'Laisser vide pour DHCP',btn_save:'&#x1F504; Sauvegarder &amp; Redémarrer',sel_placeholder:'-- Sélectionnez --',no_networks:'Aucun réseau trouvé',scan_error:'Erreur scan',need_ssid:'Veuillez sélectionner ou saisir un réseau WiFi',saving:'Enregistrement...',restarting:'Redémarrage...',net_error:'Erreur réseau'},
en:{title:'RecalBox DMD',h1:'&#x1F4E1; WiFi Setup',hint:'Connect to the <b>RecalBox-DMD-Config</b> network then select your WiFi.',lbl_wifi:'WiFi network',scan_wait:'-- Scanning... --',btn_scan:'&#x1F50D; Scan networks',ph_manual:'Or type the name manually',lbl_pwd:'Password',ph_pwd:'WiFi password',lbl_static_ip:'Static IP (optional)',ph_static_ip:'Leave empty for DHCP',btn_save:'&#x1F504; Save &amp; Restart',sel_placeholder:'-- Select --',no_networks:'No network found',scan_error:'Scan error',need_ssid:'Please select or type a WiFi network',saving:'Saving...',restarting:'Restarting...',net_error:'Network error'},
es:{title:'RecalBox DMD',h1:'&#x1F4E1; Configuración WiFi',hint:'Conéctese a la red <b>RecalBox-DMD-Config</b> y luego seleccione su WiFi.',lbl_wifi:'Red WiFi',scan_wait:'-- Escaneando... --',btn_scan:'&#x1F50D; Escanear redes',ph_manual:'O escriba el nombre manualmente',lbl_pwd:'Contraseña',ph_pwd:'Contraseña WiFi',lbl_static_ip:'IP estática (opcional)',ph_static_ip:'Dejar vacío para DHCP',btn_save:'&#x1F504; Guardar y reiniciar',sel_placeholder:'-- Seleccione --',no_networks:'No se encontraron redes',scan_error:'Error de escaneo',need_ssid:'Seleccione o escriba una red WiFi',saving:'Guardando...',restarting:'Reiniciando...',net_error:'Error de red'}
};
let currentLang='fr';
// Overlay "Chargement en cours..." (2026-08-05, demande utilisateur :
// ~3s d'attente sur mobile avant affichage, impression de plantage/envie
// de F5). Present des le tout premier octet du <body> (avant tout script)
// donc visible des que le navigateur commence a peindre la page, meme si
// le reste du transfert (page + donnees /lang, /load) prend encore du
// temps -- masque via hidePageLoadingOverlay(), appelee en toute fin de
// la chaine de bootstrap (succes ET echec, voir .finally() plus bas).
function showPageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='flex';}
function hidePageLoadingOverlay(){var el=document.getElementById('pageLoadingOverlay');if(el)el.style.display='none';}
function tr(k){return (AP_I18N[currentLang]&&AP_I18N[currentLang][k])||AP_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&AP_I18N[stored]){currentLang=stored;}
  else if(backendLang&&AP_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=AP_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.querySelectorAll('[data-i18n-placeholder]').forEach(function(el){el.placeholder=tr(el.dataset.i18nPlaceholder);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
// ============= END I18N =============
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(t,ok){var e=document.getElementById('msg');e.textContent=t;e.className='msg '+(ok?'msg-ok':'msg-err');e.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(function(){e.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(t),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});}
function togglePwd(){var p=document.getElementById('wifi_password');p.type=(p.type=='password'?'text':'password');}
function scanWiFi(){
  var sel=document.getElementById('wifi_ssid');sel.innerHTML='<option value="">'+tr('scan_wait')+'</option>';
  fetch('/scan-wifi').then(function(r){return r.json();}).then(function(nets){
    sel.innerHTML='<option value="">'+tr('sel_placeholder')+'</option>';
    if(nets&&nets.length) nets.forEach(function(n){sel.innerHTML+='<option value="'+n+'">'+n+'</option>';});
    else sel.innerHTML='<option value="">'+tr('no_networks')+'</option>';
  }).catch(function(){sel.innerHTML='<option value="">'+tr('scan_error')+'</option>';});
}
function saveWiFi(){
  var sel=document.getElementById('wifi_ssid');
  var txt=document.getElementById('wifi_ssid_text');
  var ssid=sel.value||txt.value.trim();
  if(!ssid){showMsg(tr('need_ssid'),false);return;}
  var pwd=document.getElementById('wifi_password').value.trim();
  var ip=document.getElementById('wifi_static_ip').value.trim();
  var body='wifi_enabled=1&wifi_ssid='+encodeURIComponent(ssid)+'&wifi_password='+encodeURIComponent(pwd);
  body+='&wifi_static_enabled='+(ip?1:0)+'&wifi_static_ip='+encodeURIComponent(ip);
  showMsg(tr('saving'),true);
  fetch('/save-ap',{method:'POST',body:body,headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(function(r){return r.text();})
    .then(function(t){if(t.includes('OK'))showMsg(tr('restarting'),true);else showMsg(t,false);})
    .catch(function(){showMsg(tr('net_error'),false);});
}
fetch('/lang').then(function(r){return r.json();}).then(function(d){applyLang(d.language);scanWiFi();}).catch(function(){applyLang();scanWiFi();}).finally(hidePageLoadingOverlay);
</script>
</body>
</html>
)rawliteral";

// ================================================
// Handler helpers
// ================================================
static String jsonEscape(const String &s)
{
  String out;
  out.reserve(s.length() + 4);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\r') out += "\\r";
    else if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

static void handleWebConfigLoad()
{
  unsigned long t0 = millis(); // DIAGNOSTIC TEMPORAIRE (2026-07-30) -- lenteur page rapportee hors generation
  int b = (screenBrightness * 100 + 127) / 255;
  String json = "{";
  json += "\"brightness\":\"" + String(b) + "\"";
  json += ",\"info\":\"" + String(showInfo ? '1' : '0') + "\"";
  json += ",\"playlist\":\"" + jsonEscape(playlistName) + "\"";
  json += ",\"random\":\"" + String(playlistRandom ? '1' : '0') + "\"";
  json += ",\"wifi_enabled\":\"" + String(wifiEnabled ? '1' : '0') + "\"";
  json += ",\"wifi_ssid\":\"" + jsonEscape(wifiSSID) + "\"";
  json += ",\"wifi_password\":\"" + jsonEscape(wifiPassword) + "\"";
  json += ",\"wifi_static_enabled\":\"" + String(wifiStaticEnabled ? '1' : '0') + "\"";
  json += ",\"wifi_static_ip\":\"" + jsonEscape(wifiStaticIP) + "\"";
  json += ",\"wifi_gateway\":\"" + jsonEscape(wifiGateway) + "\"";
  json += ",\"wifi_subnet\":\"" + jsonEscape(wifiSubnet) + "\"";
  json += ",\"wifi_dns1\":\"" + jsonEscape(wifiDNS1) + "\"";
  json += ",\"wifi_dns2\":\"" + jsonEscape(wifiDNS2) + "\"";
  json += ",\"bluetooth_enabled\":\"" + String(bluetoothEnabled ? '1' : '0') + "\"";
  json += ",\"bluetooth_name\":\"" + jsonEscape(bluetoothName) + "\"";
  json += ",\"recalbox_ip\":\"" + jsonEscape(recalboxIP) + "\"";
  json += ",\"clock_enabled\":\"" + String(clockEnabled ? '1' : '0') + "\"";
  json += ",\"clock_theme\":\"" + String(clockTheme) + "\"";
  {
    char neonColorBuf[8];
    snprintf(neonColorBuf, sizeof(neonColorBuf), "#%02X%02X%02X", clockNeonR, clockNeonG, clockNeonB);
    json += ",\"clock_neon_color\":\"" + String(neonColorBuf) + "\"";
  }
  json += ",\"clock_neon_color_enabled\":\"" + String(clockNeonCustomColor ? '1' : '0') + "\"";
  json += ",\"clock_interval\":\"" + String(clockIntervalGifs) + "\"";
  json += ",\"clock_interval_min\":\"" + String(clockIntervalMin) + "\"";
  json += ",\"clock_duration\":\"" + String(clockDuration) + "\"";
  json += ",\"clock_tz\":\"" + jsonEscape(clockTimeZone) + "\"";
  json += "}";
  Serial.println("[WEB] load: " + String(millis() - t0) + "ms"); // DIAGNOSTIC TEMPORAIRE
  webServer->send(200, "application/json", json);
}

// Lecture de l'etat "generation de playlist active ?" -- utilisee pour
// garder les handlers listes ci-dessous en dehors de toute generation en
// cours, meme regle que les autres handlers SD deja gardes (upload/
// creation-suppression de dossier/suppression de playlist) : une
// ecriture/lecture SD concurrente avec playlistGenStep() (meme contexte
// d'execution que ces handlers, mais au milieu d'un lot de fichiers en
// cours) serait a risque.
static bool plGenIsActive()
{
  // plGenStatusMutex retire (2026-08-10) : plus d'acces concurrent
  // possible, tout tourne desormais dans loop() (voir playlistGenStep()).
  return g_plGenStatus.active;
}

static void handleWebConfigListPlaylists()
{
  unsigned long t0 = millis(); // DIAGNOSTIC TEMPORAIRE (2026-07-30) -- lenteur page rapportee hors generation, y compris hors upload
  // Rafraichissement silencieux (pas une action utilisateur explicite) --
  // renvoie une liste vide plutot qu'une erreur 409 pendant une generation.
  if (plGenIsActive()) {
    Serial.println("[WEB] lsplaylists: generation active, liste vide (" + String(millis() - t0) + "ms)"); // DIAGNOSTIC TEMPORAIRE
    webServer->send(200, "application/json", "[]");
    return;
  }
  String json = "[";
  int n = 0;
  File dir = SD.open("/playlists");
  if (dir && dir.isDirectory()) {
    bool first = true;
    File entry = dir.openNextFile();
    while (entry) {
      String name = String(entry.name());
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (!entry.isDirectory() && name.endsWith(".txt")) {
        if (!first) json += ",";
        json += "\"" + name + "\""; first = false; n++;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  Serial.println("[WEB] lsplaylists: " + String(n) + " playlist(s) en " + String(millis() - t0) + "ms, maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  webServer->send(200, "application/json", json);
}

// Chemin du fichier maitre interne (plan cache_master_gifs, simplification
// 2026-08-01 -- voir commentaire de filterMasterIntoFile() plus bas pour
// l'historique complet). Extension ".dat" (distincte de toute playlist
// ".txt") : jamais confondu avec une playlist nulle part, MAIS desormais
// tenu a jour AUTOMATIQUEMENT par deux mecanismes independants (plus de
// bouton "Resynchroniser" manuel, retire avec tousSyncTask()) : (1) tout
// upload web vers un dossier deja connu OU nouveau via
// handleWebConfigAddToPlaylistsBatch() (cache_master_gifs.dat est toujours
// une cible d'ajout inconditionnelle), (2) l'embarquement automatique d'un
// dossier a sa premiere apparition dans une generation de playlist (voir
// playlistGenTask() plus bas). Defini ici (avant sa premiere utilisation
// dans ce fichier, handleWebConfigListGifDirs() juste en dessous).
#define TOUS_MASTER_PATH "/playlists/cache_master_gifs.dat"

// B (plan cache_master_gifs) -- RETIRE (2026-08-02, retour test reel) : le
// compte de fichiers par dossier (scan de TOUS_MASTER_PATH ici +
// /lsgifdircount a la coche) a ete retire a titre de test pour isoler sa
// contribution eventuelle a la pression heap observee pendant cette session
// (crash reel out-of-memory dans WebServer::_parseForm() pendant un upload,
// voir changelog). Retour a la version simple (liste de noms uniquement),
// tri alphabetique cote JS conserve (pur JS, sans cout heap firmware).
static void handleWebConfigListGifDirs()
{
  unsigned long t0 = millis(); // DIAGNOSTIC TEMPORAIRE (2026-07-30)
  if (plGenIsActive()) {
    Serial.println("[WEB] lsgifdirs: generation active, liste vide (" + String(millis() - t0) + "ms)"); // DIAGNOSTIC TEMPORAIRE
    webServer->send(200, "application/json", "[]");
    return;
  }
  String json = "[";
  int n = 0;
  File dir = SD.open("/gifs");
  if (dir && dir.isDirectory()) {
    bool first = true;
    File entry = dir.openNextFile();
    while (entry) {
      if (entry.isDirectory()) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);
        if (!first) json += ",";
        json += "\"" + name + "\"";
        first = false; n++;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  Serial.println("[WEB] lsgifdirs: " + String(n) + " dossier(s) en " + String(millis() - t0) + "ms, maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  webServer->send(200, "application/json", json);
}

// v92 -- handleWebConfigListGifFiles()/handleWebConfigGifCount() (listing du
// CONTENU d'un dossier, compteur de fichiers) retires : aucune page reelle
// ne les appelle plus, la navigation/suppression fichier par fichier ayant
// ete abandonnee (decision produit anterieure) -- voir plan de
// reconstruction.

// Forward declaration -- definie plus bas avec le cache g_plRefCache*, mais
// invalidee ici (creation/suppression de playlist) et dans
// handleWebConfigDeletePlaylist().
static void invalidatePlaylistRefCache();

// Machine a etats non-bloquante de generation de playlist, appelee depuis
// loop() par petits pas bornes (PLGEN_MAX_FILES_PER_STEP=20, voir
// playlistGenStep()) -- ARCHITECTURE ACTUELLE depuis le 2026-08-10.
//
// Historique (pour comprendre pourquoi ce n'est PAS une tache FreeRTOS
// dediee, contrairement a ce qu'on pourrait attendre d'un scan potentiellement
// long) : entre le 2026-07-30 et le 2026-08-10, cette generation tournait sur
// sa propre tache FreeRTOS (playlistGenTask()), protegee par 2 mutex
// (sdAccessMutex partage avec gifPlayFrameCompat()/openNextGif(), pris a
// CHAQUE frame affichee ; plGenStatusMutex pour ce statut). Cette architecture
// a ete identifiee par bissection sur materiel reel (voir memoire projet)
// comme le point de bascule d'un deadlock mqttTask/LWIP touchant le
// fonctionnement NORMAL du DMD (MQTT + affichage GIF continu, pas seulement
// pendant une generation) -- mecanisme exact non elucide malgre une
// investigation poussee (aucune fonction de cette zone ne s'executait
// pourtant pendant les scenarios qui plantaient). Priorite utilisateur
// explicite : fiabilite MQTT/affichage (coeur du projet) avant confort de
// generation de playlist (bonus) -- retour a une machine a etats dans
// loop(), plus aucun mutex, plus aucune tache dediee.
//
// Compromis assume : pendant une generation ACTIVE (action rare, declenchee
// manuellement), l'affichage GIF/le serveur web peuvent etre legerement
// moins fluides qu'avec la tache dediee (chaque appel de playlistGenStep()
// traite jusqu'a PLGEN_MAX_FILES_PER_STEP fichiers avant de rendre la main a
// loop(), donc un cout borne mais non nul par iteration -- contrairement au
// fonctionnement normal hors generation, ou le cout est litteralement nul,
// un seul if).
//
// POST /generate-playlist initialise g_plGenScan/g_plGenStatus et repond
// immediatement ("STARTED") ; playlistGenStep() (appelee depuis loop() a
// chaque iteration) prend le relais des le prochain tour. GET
// /generate-playlist-status lit g_plGenStatus (struct definie dans
// RecalBox_DMD.ino avant #include "web_config.h", meme raison que
// MqttCommand : web_config.h l'utilise avant sa "vraie" position dans le
// fichier) -- lecture directe, aucun mutex necessaire (playlistGenStep()
// tourne dans le meme contexte d'execution, loop(), que ce handler HTTP).
// POST /generate-playlist-stop pose juste stopRequested, playlistGenStep()
// se termine proprement a son prochain appel.
//
// playlistGenStep() ne touche JAMAIS gif/display/currentMode directement
// (proprietes de loop()) -- seulement g_plGenStatus. C'est loop() qui,
// periodiquement, lit cet instantane et met a jour l'ecran DMD si le mode
// config est actif (voir webDmdOverlayLine2()/RecalBox_DMD.ino, loop()).

// Texte DMD compact -- fonction pure (pas de lecture de globals), utilisable
// a la fois depuis playlistGenTask() et depuis loop() (overlay progression).
// "Scan: <nom> (i/total) X/Y" pouvait depasser 30 caracteres, largement
// au-dessus de ce qu'un panneau 128px affiche sans defilement, et les mises
// a jour frequentes interrompaient le defilement avant un tour complet
// (illisible en pratique, retour utilisateur 2026-07-28) -- nom tronque a 10
// caracteres, pas de prefixe/index (deja visibles sur la page web). Comptage
// du total cible par dossier retire (meme date, demande explicite) : un 2e
// listing complet doublait l'exposition aux lenteurs SD deja documentees
// pour un gain d'affichage juge trop couteux.
String plGenDmdText(const String &dirName, int count)
{
  String n = dirName;
  if (n.length() > 10) n = n.substring(0, 10) + "..";
  return n + " " + String(count);
}

// Forward declaration -- definie plus bas avec deleteFolderRecursive() (meme
// fonction de suppression tolerante FAT32 lecture-seule), utilisee par
// playlistGenTask() pour supprimer la playlist partielle en cas d'arret
// demande par l'utilisateur.
static bool forceDeleteFile(const String &path);

// Ecrit buf dans f en verifiant le nombre reel d'octets ecrits (2026-07-30) :
// File::print() peut ecrire MOINS que demande sans lever d'erreur -- valeur
// de retour jamais verifiee jusqu'ici dans tout ce fichier, a chaque flush
// intermediaire de buffer (toutes les fonctions de scan/filtrage). Perte de
// donnees SILENCIEUSE confirmee en test reel (2026-07-30) : 15 fichiers
// consecutifs (meme prefixe, meme dossier) manquants dans _master_gifs.txt
// apres un scan complet reussi sans aucune erreur signalee -- un seul flush
// partiel explique exactement ce genre de trou contigu. Reessaie jusqu'a 3
// fois la partie non ecrite (delay(2) entre tentatives, laisse une chance a
// un hoquet SPI/SD transitoire de se resorber) avant d'abandonner avec un
// avertissement explicite (perte de donnees rarissime mais au moins visible
// au lieu de silencieuse).
// Retourne false si une partie du buffer n'a pas pu etre ecrite meme apres
// retries (2026-07-30) : permet a l'appelant de signaler le resultat comme
// suspect (voir hadWriteLoss dans playlistGenTask()/filterMasterIntoFile())
// plutot que de faire confiance a un fichier potentiellement troue en
// silence.
static bool writeBufChecked(File &f, const String &buf)
{
  size_t total = buf.length();
  size_t offset = 0;
  int attempts = 0;
  while (offset < total && attempts < 3) {
    // buf.substring(offset) SEULEMENT si necessaire (offset>0, cas de retry
    // rarissime) -- BUG CORRIGE (2026-07-30) : appeler substring(0) sur
    // CHAQUE tentative dupliquait inutilement tout le buffer (encore ~1000
    // octets a allouer) juste pour appeler print(), en plus de buf lui-meme
    // -- sous heap deja critique (confirme en test reel : maxalloc=8692 au
    // demarrage de la tache, degrade ensuite), cette allocation supplementaire
    // echouait silencieusement (String::substring() sur allocation ratee
    // renvoie une chaine VIDE, pas une erreur) -- print("") renvoie alors 0,
    // faussement interprete comme un echec d'ecriture SD alors que c'etait
    // uniquement ce correctif lui-meme qui aggravait la pression heap.
    size_t w = (offset == 0) ? f.print(buf) : f.print(buf.substring(offset));
    if (w == 0) { attempts++; delay(2); continue; }
    offset += w;
  }
  if (offset < total) {
    Serial.println("[WEB] writeBufChecked: PERTE DE DONNEES -- " + String(total - offset) + "/" + String(total) + " octets non ecrits apres retries");
    return false;
  }
  return true;
}

// Forward declarations -- definies plus bas dans ce fichier, mais utilisees
// par playlistGenTask()/handleWebConfigGeneratePlaylist() ci-dessous.
static bool fileContainsNeedle(File &f, const String &needle);
static bool filterMasterIntoFile(const String &dirsCsv, File &outFile, int &linesWrittenOut, bool &hadWriteLossOut, String &errOut);
static bool appendMatchingLines(const String &srcPath, const String &wantedCsv, const String &destPath, bool &hadWriteLossOut);

// ETAT PERSISTANT (2026-08-10, retour a l'architecture non-tache -- voir
// changelog v54) : porte la generation "hybride" (cache+scan) d'un appel de
// playlistGenStep() a l'autre puisqu'il n'y a plus de pile de tache dediee
// pour le faire. Remplace l'ancien PlaylistGenRequest (retire) + les
// variables locales de l'ancienne playlistGenTask()/scanFoldersToPlaylistFile().
// name/cachedDirsCsv/uncachedDirsCsv/fullMarker : meme sens qu'avant (voir
// handleWebConfigGeneratePlaylist() -- repartition decidee selon la
// presence de chaque dossier dans TOUS_MASTER_PATH ; fullMarker ecrit tel
// quel en tete du fichier de sortie, marqueur "# FULL:"). Un seul champ
// "phase" pilote la progression -- pas de mutex necessaire : playlistGenStep()
// tourne exclusivement dans loop() (meme contexte d'execution que
// gifPlayFrameCompat()/openNextGif()), donc plus aucun acces SD concurrent
// entre 2 threads (voir aussi sdAccessMutex, retire -- RecalBox_DMD.ino).
enum PlGenPhase { PLGEN_PHASE_IDLE, PLGEN_PHASE_CACHE, PLGEN_PHASE_SCAN };
struct PlGenScanState
{
  PlGenPhase phase = PLGEN_PHASE_IDLE;
  String name;
  String cachedDirsCsv;
  String uncachedDirsCsv;
  String fullMarker;
  File   outFile;
  bool   hadWriteLoss = false;
  int    totalGifsFromCache = 0;
  int    totalGifsScanned = 0;
  // Position courante dans la phase scan
  int    parseIdx = 0;
  int    dirIdx = 0;
  String curDirName;
  File   curDir;
  bool   dirOpen = false;
  String buf;
  int    curDirGifs = 0;
};
static PlGenScanState g_plGenScan;

// Meme ordre de grandeur que l'ancienne playlistGenStep() (v94, avant la
// tache dediee) -- borne le cout d'un appel pour ne jamais bloquer loop()
// (donc l'affichage GIF/MQTT) plus de quelques millisecondes d'affilee.
static const int PLGEN_MAX_FILES_PER_STEP = 20;

// Finalise une generation arretee (bouton Arreter ou heap critique) --
// meme comportement qu'avant (fichier partiel supprime si le heap le
// permet, sinon laisse sur la SD).
static void plGenFinalizeStopped(bool lowHeapAbort)
{
  if (g_plGenScan.dirOpen) { g_plGenScan.curDir.close(); g_plGenScan.dirOpen = false; }
  if (g_plGenScan.outFile) g_plGenScan.outFile.close();
  String outputPath = "/playlists/" + g_plGenScan.name + ".txt";
  if (ESP.getMaxAllocHeap() >= 4096) {
    forceDeleteFile(outputPath);
  } else {
    Serial.println("[WEB] playlistGenStep: heap trop bas pour nettoyer " + g_plGenScan.name + ".txt (fichier partiel laisse sur SD, maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
  }
  if (lowHeapAbort) {
    Serial.println("[WEB] playlistGenStep: heap insuffisant, " + g_plGenScan.name + ".txt annulee/supprimee");
  } else {
    Serial.println("[WEB] playlistGenStep: arret demande, " + g_plGenScan.name + ".txt annulee/supprimee");
  }
  g_plGenStatus.resultMsg = lowHeapAbort
    ? "Memoire insuffisante, playlist supprimee. Redemarrez le DMD puis reessayez"
    : "Generation annulee, playlist supprimee";
  g_plGenStatus.active = false;
  g_plGenStatus.done = true;
  g_plGenScan.phase = PLGEN_PHASE_IDLE;
}

// Finalise une generation reussie -- meme logique qu'avant (embarquement
// automatique dans le fichier maitre, message hybride/simple).
static void plGenFinalizeOk()
{
  // Flush final du reliquat (dernier bloc accumule sous le seuil de 1000
  // caracteres) -- meme comportement que l'ancien "flush final" en fin de
  // scanFoldersToPlaylistFile().
  if (g_plGenScan.buf.length() > 0) {
    if (!writeBufChecked(g_plGenScan.outFile, g_plGenScan.buf)) g_plGenScan.hadWriteLoss = true;
    g_plGenScan.buf = "";
  }
  if (g_plGenScan.outFile) g_plGenScan.outFile.close();
  // Pure RAM, pas de SD -- doit imperativement s'executer AVANT le flip
  // active=false ci-dessous : cet ordre garantit qu'un handler HTTP voyant
  // active=false ne peut lire ce cache qu'apres qu'il ait ete invalide.
  invalidatePlaylistRefCache();

  String outputPath = "/playlists/" + g_plGenScan.name + ".txt";
  int adoptedDirCount = 0;
  if (g_plGenScan.uncachedDirsCsv.length() > 1) {
    int cp = 1;
    while (cp < (int)g_plGenScan.uncachedDirsCsv.length()) { int cc = g_plGenScan.uncachedDirsCsv.indexOf(',', cp); if (cc < 0) break; adoptedDirCount++; cp = cc + 1; }
    bool adoptWriteLoss = false;
    bool adoptOk = appendMatchingLines(outputPath, g_plGenScan.uncachedDirsCsv, TOUS_MASTER_PATH, adoptWriteLoss);
    size_t masterSizeAfter = 0;
    File chk = SD.open(TOUS_MASTER_PATH, FILE_READ);
    if (chk) { masterSizeAfter = chk.size(); chk.close(); }
    Serial.println("[WEB] playlistGenStep: embarquement cache -- adoptOk=" + String(adoptOk ? "1" : "0") + " writeLoss=" + String(adoptWriteLoss ? "1" : "0") + " tailleCacheApres=" + String((unsigned long)masterSizeAfter) + " octets");
    if (adoptWriteLoss) g_plGenScan.hadWriteLoss = true;
  }

  int totalGifs = g_plGenScan.totalGifsFromCache + g_plGenScan.totalGifsScanned;
  bool hybrid = (g_plGenScan.cachedDirsCsv.length() > 1 && g_plGenScan.uncachedDirsCsv.length() > 1);
  String resultMsg;
  if (hybrid) {
    resultMsg = "OK: " + String(totalGifs) + " GIFs (" + String(g_plGenScan.totalGifsFromCache) + " depuis le cache + " + String(g_plGenScan.totalGifsScanned) + " nouvellement scannes";
    if (adoptedDirCount > 0) resultMsg += ", " + String(adoptedDirCount) + " dossier(s) ajoute(s) au cache";
    resultMsg += ") dans la playlist " + g_plGenScan.name + ".txt";
  } else {
    resultMsg = "OK: " + String(totalGifs) + " GIFs ajoutes dans la playlist " + g_plGenScan.name + ".txt";
    if (adoptedDirCount > 0) resultMsg += " (" + String(adoptedDirCount) + " dossier(s) ajoute(s) au cache)";
  }
  if (g_plGenScan.hadWriteLoss) resultMsg += " (ATTENTION: ecriture incomplete detectee, regenerez cette playlist pour verifier)";
  Serial.println("[WEB] " + resultMsg);
  g_plGenStatus.resultMsg = resultMsg;
  g_plGenStatus.active = false;
  g_plGenStatus.done = true;
  g_plGenScan.phase = PLGEN_PHASE_IDLE;
}

// Avance la generation d'un pas borne, appelee depuis loop() a CHAQUE
// iteration -- cout quasi nul quand aucune generation n'est active (un seul
// if). Remplace playlistGenTask() (tache FreeRTOS dediee, 2026-07-30 ->
// 2026-08-10) : la tache (et sdAccessMutex, le mutex qu'elle partageait avec
// gifPlayFrameCompat()/openNextGif()) a ete identifiee par bissection sur
// materiel reel comme le point de bascule d'un deadlock mqttTask/LWIP qui
// touchait le fonctionnement normal (MQTT + affichage GIF continu) --
// mecanisme exact non elucide malgre une investigation poussee, mais la
// disparition du crash a la revert de cette architecture est reproductible.
// Priorite utilisateur explicite : fiabilite MQTT/affichage > generation de
// playlist non-bloquante -- voir memoire projet. Toute la logique metier
// (generation hybride cache+scan, marqueur FULL, embarquement automatique,
// garde-fous heap, detection perte d'ecriture) est preservee a l'identique,
// seul le mecanisme d'execution change (etapes bornees depuis loop() au lieu
// d'une tache separee).
void playlistGenStep()
{
  if (!g_plGenStatus.active) return;

  if (g_plGenStatus.stopRequested)
  {
    plGenFinalizeStopped(false);
    return;
  }
  // Garde-fou heap critique (2026-07-29, crash reel : abort() par allocation
  // heap echouee) -- conserve a l'identique, verifie une fois par appel
  // (au lieu d'une fois par fichier dans l'ancienne version tache) puisque
  // chaque appel est deja borne a PLGEN_MAX_FILES_PER_STEP fichiers.
  if (ESP.getMaxAllocHeap() < 4096)
  {
    Serial.println("[WEB] playlistGenStep: heap critique (maxalloc=" + String(ESP.getMaxAllocHeap()) + "), arret propre du scan");
    plGenFinalizeStopped(true);
    return;
  }

  if (g_plGenScan.phase == PLGEN_PHASE_CACHE)
  {
    // Portion "deja en cache" (generation hybride, plan cache_master_gifs) --
    // quasi instantanee (filtrage texte, ne touche jamais /gifs/), traitee
    // en un seul appel comme avant.
    String marker = "# FULL:" + g_plGenScan.fullMarker + "\n";
    if (!writeBufChecked(g_plGenScan.outFile, marker)) g_plGenScan.hadWriteLoss = true;
    if (g_plGenScan.cachedDirsCsv.length() > 1) {
      int linesWritten = 0;
      bool cacheWriteLoss = false;
      String err;
      filterMasterIntoFile(g_plGenScan.cachedDirsCsv, g_plGenScan.outFile, linesWritten, cacheWriteLoss, err);
      if (cacheWriteLoss) g_plGenScan.hadWriteLoss = true;
      g_plGenScan.totalGifsFromCache = linesWritten;
    }
    if (g_plGenScan.uncachedDirsCsv.length() <= 1) { plGenFinalizeOk(); return; }
    g_plGenScan.phase = PLGEN_PHASE_SCAN;
    return; // laisse le scan commencer au prochain appel
  }

  // Phase scan -- SEULEMENT sur les dossiers pas encore couverts par le
  // fichier maitre, par blocs bornes a PLGEN_MAX_FILES_PER_STEP fichiers.
  int processed = 0;
  while (processed < PLGEN_MAX_FILES_PER_STEP)
  {
    if (!g_plGenScan.dirOpen)
    {
      if (g_plGenScan.parseIdx > (int)g_plGenScan.uncachedDirsCsv.length()) { plGenFinalizeOk(); return; }
      int comma = g_plGenScan.uncachedDirsCsv.indexOf(',', g_plGenScan.parseIdx);
      String dirName = (comma < 0) ? g_plGenScan.uncachedDirsCsv.substring(g_plGenScan.parseIdx) : g_plGenScan.uncachedDirsCsv.substring(g_plGenScan.parseIdx, comma);
      dirName.trim();
      g_plGenScan.parseIdx = (comma < 0) ? (int)(g_plGenScan.uncachedDirsCsv.length() + 1) : (comma + 1);
      if (dirName.length() == 0) continue; // segment vide (virgules successives)

      g_plGenScan.dirIdx++;
      g_plGenScan.curDirName = dirName;
      g_plGenScan.curDirGifs = 0;
      g_plGenStatus.curDirName = dirName;
      g_plGenStatus.dirIdx = g_plGenScan.dirIdx;
      g_plGenStatus.curDirGifs = 0;

      g_plGenScan.curDir = SD.open(("/gifs/" + dirName).c_str());
      g_plGenScan.dirOpen = g_plGenScan.curDir && g_plGenScan.curDir.isDirectory();
      if (!g_plGenScan.dirOpen && g_plGenScan.curDir) g_plGenScan.curDir.close();
      continue; // reprend la boucle -- ouvre le fichier ou passe au dossier suivant au prochain tour
    }

    File f = g_plGenScan.curDir.openNextFile();
    if (!f) { g_plGenScan.curDir.close(); g_plGenScan.dirOpen = false; continue; }
    if (!f.isDirectory()) {
      String fname = String(f.name());
      if (fname.endsWith(".gif")) {
        g_plGenScan.buf += "/gifs/" + g_plGenScan.curDirName + "/" + fname + "\n";
        g_plGenScan.totalGifsScanned++;
        g_plGenScan.curDirGifs++;
        if (g_plGenScan.buf.length() > 1000) {
          if (!writeBufChecked(g_plGenScan.outFile, g_plGenScan.buf)) g_plGenScan.hadWriteLoss = true;
          g_plGenScan.buf = "";
        }
      }
    }
    f.close();
    g_plGenStatus.curDirGifs = g_plGenScan.curDirGifs;
    g_plGenStatus.totalGifs = g_plGenScan.totalGifsFromCache + g_plGenScan.totalGifsScanned;
    processed++;
  }
  // Fin du lot borne -- laisse loop() continuer (MQTT/affichage GIF),
  // playlistGenStep() sera rappelee au prochain tour pour continuer
  // exactement ou elle s'est arretee (curDir reste ouvert entre 2 appels).
}

// Coeur du filtrage de TOUS_MASTER_PATH, ECRIT DIRECTEMENT dans un File
// deja ouvert (outFile) -- partage par filterPlaylistFromMaster()
// (playlist entierement en cache, chemin synchrone avec son propre
// temp+rename) et playlistGenTask() (portion "deja en cache" d'une
// generation hybride, ecrite directement dans le fichier de sortie deja
// proprietaire de la tache). Meme algorithme de lecture par blocs de 512
// octets que handleWebConfigPlaylistDirs() (pending += buf, decoupage sur
// '\n', report du reliquat, PLUS traitement de la derniere ligne sans '\n'
// final -- piege facile a oublier en adaptant ce motif). Ne touche JAMAIS
// /gifs/.
static bool filterMasterIntoFile(const String &dirsCsv, File &outFile, int &linesWrittenOut, bool &hadWriteLossOut, String &errOut)
{
  linesWrittenOut = 0;
  hadWriteLossOut = false;
  File src = SD.open(TOUS_MASTER_PATH, FILE_READ);
  if (!src) { errOut = "fichier maitre introuvable"; return false; }

  // ",dir1,dir2," -- meme convention que "seen" dans handleWebConfigPlaylistDirs().
  // reserve() : bug reel confirme en test materiel -- sans reservation
  // prealable, String::operator+=() peut echouer SILENCIEUSEMENT sous heap
  // critique (maxalloc=4596 observe) en pleine boucle de concatenation,
  // faisant purement et simplement disparaitre un ou plusieurs dossiers de
  // "wanted" SANS AUCUNE ERREUR VISIBLE -- 5 GIFs obtenus au lieu de ~11000
  // attendus (tous les dossiers coches) sur ce test precis. Une seule
  // grosse allocation en amont (au lieu de N petites reallocations
  // incrementales, chacune un point de defaillance silencieux distinct) et
  // une verification explicite de son succes transforment ce risque en
  // echec net et immediat plutot qu'un resultat faux et muet.
  String wanted = ",";
  if (!wanted.reserve(dirsCsv.length() + 4)) {
    src.close();
    errOut = "memoire insuffisante (liste de dossiers)";
    return false;
  }
  {
    int start = 0;
    while (true) {
      int comma = dirsCsv.indexOf(',', start);
      String d = (comma < 0) ? dirsCsv.substring(start) : dirsCsv.substring(start, comma);
      d.trim();
      if (d.length() > 0) wanted += d + ",";
      if (comma < 0) break;
      start = comma + 1;
    }
  }

  int written = 0;
  bool hadWriteLoss = false;
  String outBuf;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending;
  // reserve() (meme classe de bug que "wanted" plus haut) : pending ne
  // depasse jamais vraiment BUFSZ + une ligne (il est retaille a son
  // reliquat apres chaque bloc), donc une seule petite reservation en amont
  // evite les N reallocations incrementales repetees (une par bloc lu,
  // potentiellement des centaines sur un gros fichier maitre) qui sont
  // sinon autant de points de defaillance silencieuse individuels sous heap
  // critique -- une desynchronisation de pending corrompt le decoupage en
  // lignes pour TOUT le reste du fichier, pas seulement la ligne courante.
  pending.reserve(BUFSZ + 256);
  int chunkCount = 0;
  while (true) {
    int n = src.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      line.trim();
      // Segment dossier = meme extraction que handleWebConfigPlaylistDirs()
      // (indexOf('/', 6) sur le chemin entier) -- jamais un test de
      // sous-chaine naif : "Arcade" ne doit pas matcher dans "Arcade2".
      if (line.startsWith("/gifs/")) {
        int s2 = line.indexOf('/', 6);
        if (s2 > 6) {
          String dir = line.substring(6, s2);
          if (wanted.indexOf("," + dir + ",") >= 0) {
            outBuf += line + "\n";
            written++;
            if (outBuf.length() > 1000) { if (!writeBufChecked(outFile, outBuf)) hadWriteLoss = true; outBuf = ""; }
          }
        }
      }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart); // reliquat (ligne a cheval sur 2 blocs) pour le prochain tour
    if ((size_t)n < BUFSZ) break;
    if (++chunkCount % 20 == 0) yield(); // watchdog-safe sur un tres gros fichier maitre (aucun autre point de cession dans cette boucle)
  }
  pending.trim();
  if (pending.startsWith("/gifs/")) { // derniere ligne sans retour a la ligne final
    int s2 = pending.indexOf('/', 6);
    if (s2 > 6) {
      String dir = pending.substring(6, s2);
      if (wanted.indexOf("," + dir + ",") >= 0) { outBuf += pending + "\n"; written++; }
    }
  }
  if (outBuf.length() > 0) { if (!writeBufChecked(outFile, outBuf)) hadWriteLoss = true; }
  src.close();

  linesWrittenOut = written;
  hadWriteLossOut = hadWriteLoss;
  return true;
}

// Filtre le fichier maitre interne (TOUS_MASTER_PATH) vers outputPath (via
// filterMasterIntoFile() ci-dessus), en ecrivant d'abord le marqueur
// "# FULL:" (voir handleWebConfigAddToPlaylistsBatch()) -- toute selection
// DMD porte toujours sur des dossiers entiers. Chemin RAPIDE : dirsCsv est
// entierement couvert par le cache -- tourne directement dans le thread
// loop() (meme raison que handleWebConfigPlaylistDirs()/
// handleWebConfigAddToPlaylistsBatch() qui font deja ca sans tache ni mutex
// : aucune lenteur SD localisee possible sur un fichier texte). Ecrit
// d'abord dans outputPath+".flt" puis remplace atomiquement
// (forceDeleteFile + rename), jamais d'ecriture directe sur outputPath.
static bool filterPlaylistFromMaster(const String &dirsCsv, const String &outputPath, const String &fullMarker,
                                      int &linesWrittenOut, bool &hadWriteLossOut, String &errOut)
{
  String tmpPath = outputPath + ".flt";
  if (SD.exists(tmpPath.c_str())) SD.remove(tmpPath.c_str());
  File out = SD.open(tmpPath.c_str(), FILE_WRITE);
  if (!out) { errOut = "ecriture impossible"; return false; }

  bool hadWriteLoss = false;
  String marker = "# FULL:" + fullMarker + "\n";
  if (!writeBufChecked(out, marker)) hadWriteLoss = true;

  int written = 0;
  bool innerWriteLoss = false;
  bool ok = filterMasterIntoFile(dirsCsv, out, written, innerWriteLoss, errOut);
  if (innerWriteLoss) hadWriteLoss = true;
  out.close();
  if (!ok) { forceDeleteFile(tmpPath); return false; }

  if (SD.exists(outputPath.c_str())) forceDeleteFile(outputPath);
  SD.rename(tmpPath.c_str(), outputPath.c_str());

  // Nettoyage des compagnons perimes -- meme pattern que handleWebConfigDeletePlaylist().
  {
    String base = outputPath.substring(outputPath.lastIndexOf('/') + 1);
    int dot = base.lastIndexOf('.');
    if (dot > 0) base = base.substring(0, dot);
    const char *exts[] = {".cache", ".sig", ".idx"};
    for (int i = 0; i < 3; i++) {
      String p = "/playlists/" + base + exts[i];
      if (SD.exists(p.c_str())) SD.remove(p.c_str());
    }
  }
  invalidatePlaylistRefCache();

  linesWrittenOut = written;
  hadWriteLossOut = hadWriteLoss;
  return true;
}

// B.2.c (plan cache_master_gifs) -- transfere (par ajout, FILE_APPEND) les
// lignes de srcPath dont le dossier appartient a wantedCsv (",dir1,dir2,")
// vers destPath. Utilise pour "adopter" dans TOUS_MASTER_PATH les dossiers
// venant d'etre scannes pour la premiere fois (playlistGenTask()) -- evite
// un second scan de /gifs/, il suffit de relire la playlist qui vient
// elle-meme d'etre ecrite. Cree destPath s'il n'existe pas encore
// (bootstrap organique du fichier maitre, premiere generation de playlist
// jamais lancee sur cette carte). Ignore silencieusement toute ligne qui ne
// commence pas par "/gifs/" (dont le marqueur "# FULL:" en tete de
// srcPath).
static bool appendMatchingLines(const String &srcPath, const String &wantedCsv, const String &destPath, bool &hadWriteLossOut)
{
  hadWriteLossOut = false;
  File src = SD.open(srcPath.c_str(), FILE_READ);
  if (!src) return false;
  File dest = SD.open(destPath.c_str(), FILE_APPEND);
  if (!dest) { src.close(); return false; }

  bool hadWriteLoss = false;
  String outBuf; outBuf.reserve(1200);
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending; pending.reserve(BUFSZ + 256);
  int chunkCount = 0;
  while (true) {
    int n = src.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      line.trim();
      if (line.startsWith("/gifs/")) {
        int s2 = line.indexOf('/', 6);
        if (s2 > 6) {
          String dir = line.substring(6, s2);
          if (wantedCsv.indexOf("," + dir + ",") >= 0) { outBuf += line; outBuf += "\n"; }
        }
      }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart);
    if (outBuf.length() > 1000) { if (!writeBufChecked(dest, outBuf)) hadWriteLoss = true; outBuf = ""; }
    if ((size_t)n < BUFSZ) break;
    if (++chunkCount % 20 == 0) yield();
  }
  pending.trim();
  if (pending.startsWith("/gifs/")) {
    int s2 = pending.indexOf('/', 6);
    if (s2 > 6) {
      String dir = pending.substring(6, s2);
      if (wantedCsv.indexOf("," + dir + ",") >= 0) { outBuf += pending; outBuf += "\n"; }
    }
  }
  if (outBuf.length() > 0) { if (!writeBufChecked(dest, outBuf)) hadWriteLoss = true; }
  dest.close();
  src.close();
  hadWriteLossOut = hadWriteLoss;
  return true;
}

static void handleWebConfigGeneratePlaylist()
{
  // plGenStatusMutex retire (2026-08-10) : plus d'acces concurrent
  // possible, tout tourne desormais dans loop() (voir playlistGenStep()).
  bool alreadyActive = g_plGenStatus.active;
  if (alreadyActive) { webServer->send(409, "text/plain", "ERR: generation deja en cours"); return; }
  if (!webServer->hasArg("name") || !webServer->hasArg("dirs")) {
    webServer->send(400, "text/plain", "ERR: manque nom ou dirs"); return;
  }
  String name = webServer->arg("name");
  String dirsRaw = webServer->arg("dirs");
  String outputPath = "/playlists/" + name + ".txt";

  // B.1 (plan cache_master_gifs) -- verification PAR DOSSIER (pas globale)
  // de la presence dans le fichier maitre : cocher un dossier jamais mis en
  // cache aux cotes de dossiers deja en cache produisait auparavant une
  // playlist silencieusement incomplete (l'ancien test global -- "le
  // fichier maitre existe-t-il ?" -- prenait le chemin rapide pour TOUT des
  // qu'il existait, sans verifier que chaque dossier demande y etait
  // reellement represente). cachedDirsCsv/uncachedDirsCsv au format
  // ",dir1,dir2,". allDirsClean : tous les dossiers demandes, "dir1,dir2"
  // sans virgule d'encadrement -- marqueur "# FULL:" ecrit tel quel.
  String cachedDirsCsv = ",", uncachedDirsCsv = ",";
  String allDirsClean;
  {
    File master = SD.exists(TOUS_MASTER_PATH) ? SD.open(TOUS_MASTER_PATH, FILE_READ) : File();
    int start = 0;
    while (true) {
      int comma = dirsRaw.indexOf(',', start);
      String d = (comma < 0) ? dirsRaw.substring(start) : dirsRaw.substring(start, comma);
      d.trim();
      if (d.length() > 0) {
        if (allDirsClean.length() > 0) allDirsClean += ",";
        allDirsClean += d;
        bool inMaster = false;
        if (master) { master.seek(0); inMaster = fileContainsNeedle(master, "/gifs/" + d + "/"); }
        if (inMaster) cachedDirsCsv += d + ","; else uncachedDirsCsv += d + ",";
      }
      if (comma < 0) break;
      start = comma + 1;
    }
    if (master) master.close();
  }

  // Chemin RAPIDE : tous les dossiers demandes sont deja couverts par le
  // fichier maitre -- filtrage texte synchrone (tourne dans loop(), pas de
  // tache), ne touche jamais /gifs/.
  if (uncachedDirsCsv.length() <= 1) {
    int linesWritten = 0;
    bool hadWriteLoss = false;
    String err;
    bool ok = filterPlaylistFromMaster(cachedDirsCsv, outputPath, allDirsClean, linesWritten, hadWriteLoss, err);
    if (ok) {
      String msg = "OK: " + String(linesWritten) + " GIFs (generation rapide)";
      if (hadWriteLoss) msg += " (ATTENTION: ecriture incomplete detectee, regenerez cette playlist pour verifier)";
      Serial.println("[WEB] generate-playlist: " + msg + " -> " + name + ".txt");
      webServer->send(200, "text/plain", msg);
    } else {
      Serial.println("[WEB] generate-playlist: ECHEC filtrage (" + err + ")");
      webServer->send(500, "text/plain", "ERR: " + err);
    }
    return;
  }

  // Chemin hybride/complet (generation en tache de fond, avec progression) :
  // au moins un dossier demande n'est pas encore dans le fichier maitre
  // (soit il n'existe pas du tout -- bootstrap -- soit certains dossiers
  // sont neufs). playlistGenTask() ecrit d'abord la portion cachedDirsCsv
  // (quasi instantanee) puis scanne uniquement uncachedDirsCsv, avant
  // d'adopter automatiquement ces derniers dans le fichier maitre.
  if (!SD.exists("/playlists")) SD.mkdir("/playlists");
  if (SD.exists(outputPath.c_str())) SD.remove(outputPath.c_str());
  File outf = SD.open(outputPath.c_str(), FILE_WRITE);
  if (!outf) { webServer->send(500, "text/plain", "ERR: ecriture impossible"); return; }
  // Reste ouvert -- porte par g_plGenScan.outFile, playlistGenStep() ecrit
  // le marqueur "# FULL:" au tout premier pas (2026-08-10) : plus de
  // fermeture/reouverture, meme handle garde jusqu'a la fin de la
  // generation (ancien commentaire "playlistGenTask() rouvre le fichier"
  // obsolete depuis le retrait de la tache dediee).

  int totalDirsToScan = 0;
  { int cp = 1; while (cp < (int)uncachedDirsCsv.length()) { int cc = uncachedDirsCsv.indexOf(',', cp); if (cc < 0) break; totalDirsToScan++; cp = cc + 1; } }

  // Initialise l'etat persistant -- playlistGenStep() (appelee depuis
  // loop() a chaque iteration) prend le relais au prochain tour, plus de
  // tache FreeRTOS a creer ni de mutex a prendre (2026-08-10, voir
  // changelog v54 : la tache dediee et sdAccessMutex ont ete identifies par
  // bissection materielle comme le point de bascule d'un deadlock
  // mqttTask/LWIP touchant le fonctionnement normal MQTT/affichage GIF --
  // priorite utilisateur explicite : fiabilite avant confort de generation).
  g_plGenStatus = PlaylistGenStatus();
  g_plGenStatus.active = true;
  g_plGenStatus.totalDirs = totalDirsToScan;

  g_plGenScan = PlGenScanState();
  g_plGenScan.name = name;
  g_plGenScan.cachedDirsCsv = cachedDirsCsv;
  g_plGenScan.uncachedDirsCsv = uncachedDirsCsv;
  g_plGenScan.fullMarker = allDirsClean;
  g_plGenScan.outFile = outf;
  g_plGenScan.phase = PLGEN_PHASE_CACHE;

  Serial.println("[WEB] generate-playlist: demarrage " + name + ".txt, dirs=" + dirsRaw + ", heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
  webServer->send(200, "text/plain", "STARTED");
}

static void handleWebConfigGeneratePlaylistStatus()
{
  // plGenStatusMutex retire (2026-08-10) : plus d'acces concurrent possible,
  // playlistGenStep() tourne exclusivement dans loop(), meme contexte que
  // ce handler HTTP (lui-meme appele depuis loop() via handleWebConfig()).
  PlaylistGenStatus snap = g_plGenStatus;
  String json = "{\"active\":" + String(snap.active ? "true" : "false");
  json += ",\"done\":" + String(snap.done ? "true" : "false");
  json += ",\"dir\":\"" + jsonEscape(snap.curDirName) + "\"";
  json += ",\"dirIdx\":" + String(snap.dirIdx);
  json += ",\"totalDirs\":" + String(snap.totalDirs);
  json += ",\"gifs\":" + String(snap.totalGifs);
  json += ",\"curDirGifs\":" + String(snap.curDirGifs);
  json += ",\"result\":\"" + jsonEscape(snap.resultMsg) + "\"}";
  webServer->send(200, "application/json", json);
}

// Arret demande par l'utilisateur (bouton "Arreter") : pose juste le drapeau,
// ne touche plus AUCUN File -- playlistGenStep() (2026-08-10, plus de tache
// dediee) reste la SEULE proprietaire de g_plGenScan.outFile/du dossier en
// cours, meme contexte d'execution (loop()) que ce handler HTTP -- elimine
// par construction tout risque de double-fermeture/concurrence. Le nettoyage
// se fait au prochain appel de playlistGenStep() ; le polling web deja en
// place detecte la fin via son chemin normal (!active), sans changement JS
// necessaire.
static void handleWebConfigGeneratePlaylistStop()
{
  bool wasActive = g_plGenStatus.active;
  if (wasActive) g_plGenStatus.stopRequested = true;
  Serial.println(String("[WEB] generate-playlist-stop: ") + (wasActive ? "arret demande" : "rien a arreter"));
  webServer->send(200, "text/plain", wasActive ? "OK: arret demande" : "OK: rien a arreter");
}


// Renvoie la liste (JSON) des dossiers distincts references par une playlist
// existante -- utilise par la page web pour pre-cocher les cases du dossier
// correspondant quand l'utilisateur choisit de modifier une playlist deja
// generee, plutot que de devoir tout re-cocher a la main.
static void handleWebConfigPlaylistDirs()
{
  if (plGenIsActive()) { webServer->send(200, "application/json", "[]"); return; }
  if (!webServer->hasArg("name")) { webServer->send(400, "text/plain", "ERR: manque nom"); return; }
  String name = webServer->arg("name");
  int dotExt = name.lastIndexOf('.');
  if (dotExt > 0) name = name.substring(0, dotExt); // defensif : accepte "nom" ou "nom.txt"
  File f = SD.open(("/playlists/" + name + ".txt").c_str());
  if (!f) { webServer->send(200, "application/json", "[]"); return; }

  // Lecture par blocs fixes + extraction ligne par ligne -- jamais tout le
  // fichier en une seule String (meme raison que fileContainsNeedle : une
  // grosse playlist comme "tous", ~400 Ko, a montre en test reel des
  // blocages de plusieurs dizaines de secondes avec un simple readString()).
  String json = "[";
  String seen = ",";
  bool first = true;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending;
  // reserve() (2026-07-30) : bug reel confirme en test materiel -- sur une
  // grosse playlist (ALL2.txt, ~11000 lignes/18 dossiers), reouverte pour
  // modification, un SEUL dossier se retrouvait precoche au lieu de tous.
  // Meme cause que "wanted" dans filterPlaylistFromMaster() : pending +=
  // buf peut echouer silencieusement sous heap critique a l'un des
  // (potentiellement) centaines de blocs lus -- une seule desynchronisation
  // corrompt le decoupage en lignes pour TOUT le reste du fichier, faisant
  // disparaitre la quasi-totalite des dossiers reconnus d'un coup. pending
  // ne depasse jamais vraiment BUFSZ + une ligne (retaille a son reliquat
  // apres chaque bloc) -- une seule petite reservation en amont evite les
  // N reallocations incrementales, chacune un point de defaillance distinct.
  pending.reserve(BUFSZ + 256);
  while (true) {
    int n = f.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      line.trim();
      if (line.startsWith("/gifs/")) {
        int s2 = line.indexOf('/', 6);
        if (s2 > 6) {
          String dir = line.substring(6, s2);
          if (seen.indexOf("," + dir + ",") < 0) {
            seen += dir + ",";
            if (!first) json += ",";
            json += "\"" + jsonEscape(dir) + "\"";
            first = false;
          }
        }
      }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart); // garde le reste incomplet (ligne a cheval sur 2 blocs) pour le prochain tour
    if ((size_t)n < BUFSZ) break;
  }
  f.close();
  pending.trim();
  if (pending.startsWith("/gifs/")) { // derniere ligne sans retour a la ligne final
    int s2 = pending.indexOf('/', 6);
    if (s2 > 6) {
      String dir = pending.substring(6, s2);
      if (seen.indexOf("," + dir + ",") < 0) {
        if (!first) json += ",";
        json += "\"" + jsonEscape(dir) + "\"";
      }
    }
  }
  json += "]";
  webServer->send(200, "application/json", json);
}

static void handleWebConfigDeletePlaylist()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation en cours"); return; }
  if (!webServer->hasArg("name")) { webServer->send(400, "text/plain", "ERR: manque nom"); return; }
  String name = webServer->arg("name");
  String base = name;
  int dot = base.lastIndexOf('.');
  if (dot > 0) base = base.substring(0, dot);
  const char *exts[] = {".txt", ".cache", ".sig", ".idx"};
  int deleted = 0;
  for (int i = 0; i < 4; i++) {
    String path = "/playlists/" + base + exts[i];
    if (SD.exists(path.c_str())) { SD.remove(path.c_str()); deleted++; }
  }
  invalidatePlaylistRefCache();
  String msg = "OK: " + String(deleted) + " fichiers supprimes pour " + name;
  Serial.println("[WEB] " + msg);
  webServer->send(200, "text/plain", msg);
}

// v92 -- addFileToPlaylists() (mise a jour PAR FICHIER, relisait chaque
// playlist ligne par ligne a chaque appel) remplacee par le cache RAM
// g_plRefCache*/handleWebConfigAddToPlaylistsBatch() ci-dessous : c'est le
// fix exact du probleme "mise a jour playlist lente, sans buffer" -- un
// seul passage par LOT d'upload (pas par fichier), lecture bufferisee
// File::readString() au lieu de readStringUntil('\n') ligne par ligne.

// Cache RAM : pour g_plRefCacheFolder, liste (CSV) des playlists .txt qui
// referencent deja ce dossier. Invalide (chaine vide) a la creation ou
// suppression d'une playlist -- voir invalidatePlaylistRefCache().
static String g_plRefCacheFolder = "";
static String g_plRefCachePlaylists = "";

static void invalidatePlaylistRefCache() { g_plRefCacheFolder = ""; g_plRefCachePlaylists = ""; }

// Cherche needle dans f SANS charger tout le fichier en memoire (contrairement
// a f.readString(), qui a montre en test reel (2026-07-28) des blocages de
// 40-44s ET un resultat FAUX -- "found=0" pour une playlist "Tous"/~400 Ko
// qui referencait pourtant bien le dossier -- des qu'un fichier depasse
// quelques centaines de Ko avec un heap deja fragmente (maxalloc mesure a
// ~9-10 Ko a ce moment du boot) : la reallocation progressive d'une String
// Arduino jusqu'a des centaines de Ko dans un tas aussi fragmente est soit
// catastrophiquement lente, soit echoue silencieusement en cours de route.
// Lecture par blocs fixes (BUFSZ), avec chevauchement pour ne pas rater une
// correspondance a cheval sur 2 blocs -- cout memoire constant, quelle que
// soit la taille du fichier.
static bool fileContainsNeedle(File &f, const String &needle)
{
  const size_t BUFSZ = 512;
  size_t nlen = needle.length();
  if (nlen == 0 || nlen > 64) return false; // needle attendu court ("/gifs/<dossier>/")
  char buf[BUFSZ + 64];
  size_t carried = 0;
  while (true) {
    int n = f.read((uint8_t *)(buf + carried), BUFSZ);
    if (n <= 0) break;
    size_t total = carried + (size_t)n;
    if (total >= nlen) {
      for (size_t i = 0; i + nlen <= total; i++) {
        if (memcmp(buf + i, needle.c_str(), nlen) == 0) return true;
      }
    }
    size_t keep = (nlen > 1) ? (nlen - 1) : 0;
    if (keep > total) keep = total;
    if (keep > 0) memmove(buf, buf + (total - keep), keep);
    carried = keep;
    if ((size_t)n < BUFSZ) break; // fin de fichier
  }
  return false;
}

// Meme principe que fileContainsNeedle() ci-dessus, mais pour plusieurs
// chemins candidats en un seul passage streaming sur le fichier (utilise par
// la boucle d'ajout : verifie lesquels des fichiers d'un lot d'upload sont
// deja presents dans une playlist, sans jamais charger tout son contenu en
// memoire). candidates[]/found[] : memes indices, meme taille nCandidates.
static void fileFindExistingPaths(File &f, int nCandidates, const String candidates[], bool found[])
{
  for (int i = 0; i < nCandidates; i++) found[i] = false;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 300]; // marge pour l'overlap (chemins de fichiers potentiellement longs)
  buf[0] = '\n'; // emule le prefixe "\n"+contenu de l'ancienne version (detecte une correspondance des le tout debut du fichier)
  size_t carried = 1;
  while (true) {
    int n = f.read((uint8_t *)(buf + carried), BUFSZ);
    if (n <= 0) break;
    size_t total = carried + (size_t)n;
    size_t maxOverlap = 0;
    for (int i = 0; i < nCandidates; i++) {
      if (found[i]) continue;
      String withNl = candidates[i] + "\n";
      size_t nlen = withNl.length();
      if (nlen == 0 || nlen > 260) continue;
      if (nlen - 1 > maxOverlap) maxOverlap = nlen - 1;
      if (total >= nlen) {
        const char *needle = withNl.c_str();
        for (size_t p = 0; p + nlen <= total; p++) {
          if (memcmp(buf + p, needle, nlen) == 0) { found[i] = true; break; }
        }
      }
    }
    size_t keep = (maxOverlap > total) ? total : maxOverlap;
    if (keep > 0) memmove(buf, buf + (total - keep), keep);
    carried = keep;
    if ((size_t)n < BUFSZ) break;
  }
}

// Version "lot" : traite plusieurs fichiers du MEME dossier en un seul
// appel. Meme dossier => memes playlists concernees (via g_plRefCache*), et
// surtout chaque playlist candidate n'est lue qu'UNE FOIS (au lieu d'une
// fois par fichier du lot) pour verifier quels fichiers y sont deja
// presents, puis tous les fichiers manquants sont ajoutes en un seul
// SD.open(FILE_APPEND). Appelee par le JS (uploadGif()) une seule fois a la
// fin de tout un lot d'upload.
static void handleWebConfigAddToPlaylistsBatch()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  unsigned long tFn0 = millis(); // DIAGNOSTIC TEMPORAIRE (68s constates en test reel 2026-07-28) -- a retirer une fois la cause trouvee
  if (!webServer->hasArg("dir") || !webServer->hasArg("files")) { webServer->send(200, "text/plain", "OK:0"); return; }
  String folder = webServer->arg("dir"); folder.trim();
  String filesArg = webServer->arg("files");
  if (folder.length() == 0 || filesArg.length() == 0) { webServer->send(200, "text/plain", "OK:0"); return; }

  if (g_plRefCacheFolder != folder) {
    g_plRefCacheFolder = folder;
    g_plRefCachePlaylists = "";
    // Detection "quelle playlist reference ce dossier" : lecture bufferisee
    // complete (readString(), reutilisee plus bas dans cette meme fonction
    // pour la verification de doublons) + un seul indexOf(), au lieu d'une
    // relecture ligne par ligne (readStringUntil('\n') + delay(1) PAR
    // LIGNE) -- tres lent des qu'une playlist contient beaucoup d'entrees.
    unsigned long tScan0 = millis(); // DIAGNOSTIC TEMPORAIRE
    int plCount = 0;
    String needle = "/gifs/" + folder + "/";
    // Fichier maitre interne (cache_master_gifs.dat, TOUS_MASTER_PATH) --
    // reintroduit ici explicitement PAR NOM : son extension .dat (changee
    // volontairement pour ne plus jamais etre confondu avec une playlist
    // ailleurs, cf listing) le fait sortir du filtre ".txt" ci-dessous, qui
    // l'aurait sinon exclu de ce scan et donc de la mise a jour automatique
    // lors d'un upload.
    String masterBase = String(TOUS_MASTER_PATH);
    masterBase = masterBase.substring(masterBase.lastIndexOf('/') + 1);
    File plDir = SD.open("/playlists");
    if (plDir && plDir.isDirectory()) {
      File entry = plDir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;
        bool isMaster = (base == masterBase);
        if (!entry.isDirectory() && (base.endsWith(".txt") || isMaster)) {
          plCount++;
          unsigned long tEntry0 = millis(); // DIAGNOSTIC TEMPORAIRE
          bool found;
          if (isMaster) {
            // B (plan cache_master_gifs) -- cache_master_gifs.dat est cense
            // contenir TOUT /gifs/ par construction : il doit toujours etre
            // une cible d'ajout, meme pour un dossier flambant neuf qu'il ne
            // referencait pas encore (contrairement a une playlist
            // utilisateur, ou "n'ajouter que si elle reference deja ce
            // dossier" respecte une selection volontaire).
            found = true;
          } else {
            // Marqueur "# FULL:dossier1,dossier2" (plan cache_master_gifs) --
            // playlist "hybride" (dossiers entiers + selection personnalisee
            // de fichiers dans d'autres dossiers, voir outil PC) : sans ce
            // marqueur, fileContainsNeedle() (juste "cette playlist
            // reference-t-elle AU MOINS UNE ligne de ce dossier ?") ajouterait
            // a tort un nouveau fichier a une playlist qui n'a jamais demande
            // la totalite de ce dossier. Ne lit que la premiere ligne (peu
            // couteux) ; playlist "ancien style" sans marqueur -> comportement
            // inchange (fileContainsNeedle() sur tout le fichier).
            String firstLine;
            entry.seek(0);
            char peekBuf[513];
            int pn = entry.read((uint8_t *)peekBuf, sizeof(peekBuf) - 1);
            if (pn > 0) {
              peekBuf[pn] = 0;
              String chunk = String(peekBuf);
              int nl = chunk.indexOf('\n');
              firstLine = (nl >= 0) ? chunk.substring(0, nl) : chunk;
              firstLine.trim();
            }
            if (firstLine.startsWith("# FULL:")) {
              String listCsv = "," + firstLine.substring(7) + ",";
              found = listCsv.indexOf("," + folder + ",") >= 0;
            } else {
              entry.seek(0);
              // entry est deja un handle ouvert sur ce fichier precis (obtenu
              // par iteration via openNextFile(), pas par nom) -- pas besoin
              // de le rouvrir. fileContainsNeedle() lit par blocs fixes (voir
              // plus haut) : evite de charger tout le fichier en memoire,
              // cause reelle des blocages 40-44s mesures en test reel sur
              // "Tous"/"gaming" (l'ancienne hypothese "recherche par nom" a
              // ete infirmee par un test dedie).
              found = fileContainsNeedle(entry, needle);
            }
          }
          Serial.println("[WEB] plscan " + base + " " + String(millis() - tEntry0) + "ms found=" + String(found ? "1" : "0")); // DIAGNOSTIC TEMPORAIRE
          if (found) {
            if (g_plRefCachePlaylists.length() > 0) g_plRefCachePlaylists += ",";
            g_plRefCachePlaylists += base;
          }
        }
        entry.close(); entry = plDir.openNextFile();
        delay(1);
      }
      plDir.close();
    }
    Serial.println("[WEB] add-to-playlists-batch: scan " + String(plCount) + " playlist(s) en " + String(millis() - tScan0) + "ms, maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  }

  unsigned long tAppend0 = millis(); // DIAGNOSTIC TEMPORAIRE
  int totalAppended = 0;

  // Chemins candidats du lot, calcules une seule fois (identiques pour
  // toutes les playlists candidates ci-dessous) -- bornes a MAX_BATCH_FILES,
  // largement au-dessus des lots observes en usage reel (jusqu'a ~15).
  const int MAX_BATCH_FILES = 64;
  String candidates[MAX_BATCH_FILES];
  int nCand = 0;
  {
    int fstart = 0;
    while (fstart <= (int)filesArg.length() && nCand < MAX_BATCH_FILES) {
      int fcomma = filesArg.indexOf(',', fstart);
      String fname = (fcomma < 0) ? filesArg.substring(fstart) : filesArg.substring(fstart, fcomma);
      fname.trim();
      if (fname.length() > 0) candidates[nCand++] = "/gifs/" + folder + "/" + fname;
      if (fcomma < 0) break;
      fstart = fcomma + 1;
    }
  }

  int pstart = 0;
  while (pstart <= (int)g_plRefCachePlaylists.length()) {
    int pcomma = g_plRefCachePlaylists.indexOf(',', pstart);
    String base = (pcomma < 0) ? g_plRefCachePlaylists.substring(pstart) : g_plRefCachePlaylists.substring(pstart, pcomma);
    if (base.length() > 0) {
      String plPath = "/playlists/" + base;
      // fileFindExistingPaths() lit par blocs fixes (voir fileContainsNeedle
      // plus haut) -- evite de charger toute la playlist en memoire
      // (pl.readString() sur une grosse playlist a montre en test reel des
      // blocages de plusieurs dizaines de secondes, meme cause que le scan
      // de detection ci-dessus).
      bool already[MAX_BATCH_FILES];
      File pl = SD.open(plPath.c_str());
      if (pl) { fileFindExistingPaths(pl, nCand, candidates, already); pl.close(); }
      else { for (int i = 0; i < nCand; i++) already[i] = false; }
      String toAppend;
      for (int i = 0; i < nCand; i++) {
        if (!already[i]) { toAppend += candidates[i] + "\n"; totalAppended++; }
      }
      if (toAppend.length() > 0) {
        File plApp = SD.open(plPath.c_str(), FILE_APPEND);
        if (plApp) { plApp.print(toAppend); plApp.close(); }
      }
    }
    if (pcomma < 0) break;
    pstart = pcomma + 1;
  }
  Serial.println("[WEB] add-to-playlists-batch: dir=" + folder + " -> " + String(totalAppended) + " ajout(s), append=" + String(millis() - tAppend0) + "ms, total=" + String(millis() - tFn0) + "ms"); // DIAGNOSTIC TEMPORAIRE : timings ajoutes
  webServer->send(200, "text/plain", "OK:" + String(totalAppended));
}

// Cree /gifs/<dir> si absent, en route dediee (idempotente, appelee par le
// JS AVANT le premier fichier d'un upload). Decouple la creation de dossier
// du chemin critique de l'upload multipart : le workaround (mkdir + creer/
// supprimer un fichier temoin, necessaire pour eviter un attribut lecture
// seule sur certaines cartes SD) restait auparavant dans UPLOAD_FILE_START,
// ou meme un timeout client elargi ne suffisait pas toujours (requete
// concurrente du flux de donnees fichier en cours de reception). En le
// sortant du multipart, cette requete a son propre budget de temps et le
// dossier existe deja quand l'upload demarre vraiment.
static void handleWebConfigCreateFolder()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  if (!webServer->hasArg("dir")) { webServer->send(400, "text/plain", "ERR: dossier manquant"); return; }
  String dirName = webServer->arg("dir"); dirName.trim();
  if (dirName.length() == 0) { webServer->send(400, "text/plain", "ERR: dossier manquant"); return; }
  String dirPath = "/gifs/" + dirName;
  webServer->client().setTimeout(15000);
  if (SD.exists(dirPath.c_str())) {
    webServer->client().setTimeout(3000);
    webServer->send(200, "text/plain", "OK: existant");
    return;
  }
  unsigned long t0 = millis();
  bool mkOk = SD.mkdir(dirPath.c_str());
  Serial.println("[WEB] create-folder: mkdir " + dirPath + " -> " + (mkOk ? "OK" : "FAIL") + " (" + String(millis() - t0) + "ms)");
  unsigned long t1 = millis();
  File tmp = SD.open(dirPath + "/.tmp", FILE_WRITE);
  if (tmp) { tmp.close(); SD.remove(dirPath + "/.tmp"); }
  Serial.println("[WEB] create-folder: fichier temoin " + dirPath + " -> " + (tmp ? "OK" : "FAIL") + " (" + String(millis() - t1) + "ms)");
  webServer->client().setTimeout(3000);
  bool ok = SD.exists(dirPath.c_str());
  webServer->send(ok ? 200 : 500, "text/plain", ok ? "OK: cree" : "ERR: creation echouee");
}

static void handleWebConfigUpload()
{
  if (uploadFile) { uploadFile.close(); uploadFile = File(); } // filet de securite -- deja ferme normalement a UPLOAD_FILE_END
  String results = uploadBatchResults;
  int okCount = uploadBatchOkCount;
  // Remis a zero ICI (pas au prochain UPLOAD_FILE_START) : cette requete
  // /upload est entierement terminee, la PROCHAINE sera une requete HTTP
  // independante (nouvelle poignee de main TCP, cf. commentaire pres des
  // variables globales) qui doit repartir d'un accumulateur vide.
  uploadBatchResults = "";
  uploadBatchOkCount = 0;
  if (results.length() == 0) {
    // Ne JAMAIS appeler webServer->send() depuis handleWebConfigUploadFile()
    // (callback UPLOAD_FILE_*) : le client est encore en train d'envoyer le
    // corps multipart a ce moment-la, et une reponse prematuree casse la
    // connexion HTTP en cours (vu cote navigateur comme une erreur reseau).
    // Seul ce handler, appele une fois le corps entierement consomme, a le
    // droit d'envoyer une reponse. Ici : aucun fichier n'a meme atteint
    // UPLOAD_FILE_END (ex. heap critique des le tout debut de la requete).
    String msg = uploadErrorMsg.length() ? uploadErrorMsg : "ERR: aucun fichier recu";
    uploadErrorMsg = "";
    webServer->send(400, "text/plain", msg);
    return;
  }
  String json = "{\"ok\":" + String(okCount) + ",\"files\":[" + results + "]}";
  Serial.println("[WEB] upload batch: " + String(okCount) + " fichier(s) reussi(s) sur cette requete");
  webServer->send(200, "application/json", json);
}

static void handleWebConfigUploadFile()
{
  HTTPUpload &upload = webServer->upload();
  if (upload.status == UPLOAD_FILE_START) {
    // Reengage le mode config si necessaire (demande utilisateur 2026-08-02,
    // retour test reel) : apres un crash+reboot en pleine copie, le
    // navigateur relance l'upload tout seul (retry cote JS) SANS recharger
    // de page au prealable -- sur ce boot frais, g_sdOpInProgress est encore
    // false, donc la lecture GIF continue en fond ET MQTT tente de se
    // connecter (bloque uniquement par g_sdOpInProgress, voir mqttTask()
    // dans RecalBox_DMD.ino) pendant l'upload, aggravant la pression heap
    // deja critique. Le forcer ICI, avant meme le premier octet ecrit,
    // garantit le meme etat "config" qu'un upload demarre normalement
    // depuis la page MEDIA deja chargee.
    // Pause inline (PAS triggerWebConfigMode()) : ce dernier peut desormais
    // declencher un reboot cible (v42, voir plus bas) qui envoie sa propre
    // reponse HTTP -- inacceptable ici, le client est encore en train
    // d'envoyer le corps multipart (meme regle que le reste de ce handler,
    // cf. commentaire de handleWebConfigUpload() : jamais de send()
    // premature depuis un callback UPLOAD_FILE_*). En pratique le reboot
    // aurait de toute facon deja eu lieu au chargement de la page MEDIA
    // elle-meme si le heap etait plafonne -- ce chemin ne sert que le cas
    // de reprise auto post-crash sur un boot frais, ou le heap est encore
    // largement suffisant.
    if (!g_sdOpInProgress) {
      // Repli 0.0.0.0 + clearFirstBoot() retire : memes corrections que
      // triggerWebConfigModeSoft() (bug corrige 2026-08-05) -- duplicata
      // volontaire (voir commentaire au-dessus, PAS triggerWebConfigMode()
      // ici a cause du reboot cible qu'il peut declencher).
      String ip = WiFi.localIP().toString();
      if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
      if (ip == "0.0.0.0") ip = "192.168.4.1";
      String url = "http://" + ip;
      webDmdSetMainMsg("WEB DMD CONFIG");
      webDmdPause(url, 0xFFE0);
    }
    uploadCurName = upload.filename;
    { int p = uploadCurName.lastIndexOf('/'); if (p >= 0) uploadCurName = uploadCurName.substring(p + 1); }
    { int p = uploadCurName.lastIndexOf('\\'); if (p >= 0) uploadCurName = uploadCurName.substring(p + 1); }
    if (plGenIsActive()) { uploadErrorMsg = "ERR: generation de playlist en cours"; return; }
    // Timeout client elargi (defaut lib WebServer ~3s) le temps de l'upload :
    // les ecritures SD sous charge peuvent le depasser facilement -> la lib
    // coupe alors la connexion, vu cote navigateur comme ERR_CONNECTION_
    // RESET/TIMED_OUT. Remis a une valeur courte des la fin/l'abandon de
    // chaque fichier (plusieurs fichiers possibles dans la meme requete,
    // voir commentaire pres des variables globales).
    webServer->client().setTimeout(15000);
    uploadErrorMsg = "";
    uploadDir = webServer->arg("dir");
    uploadDir.trim();
    if (uploadDir.length() == 0) {
      uploadErrorMsg = "ERR: dossier cible manquant";
      return;
    }
    if (uploadCurName.length() == 0) { uploadErrorMsg = "ERR: nom fichier invalide"; return; }
    // Garde heap critique (reintroduite v42 -- perdue lors de la refonte
    // multi-fichiers, voir v91 historique + crash reel v41) : sans ce garde,
    // un heap deja au plus bas au moment du SD.open() plus bas peut faire
    // echouer l'allocation interne du mutex de flux FILE* (newlib) et
    // declencher un abort() direct -- PAS une exception C++, donc jamais
    // rattrapable par le try/catch de handleWebConfig(). Retry-apres-delai
    // pour ne pas refuser un creux transitoire (cf. v91).
    if (ESP.getMaxAllocHeap() < 6000) {
      unsigned long maBefore = ESP.getMaxAllocHeap();
      delay(10);
      unsigned long maAfter = ESP.getMaxAllocHeap();
      Serial.println("[WEB] Upload heap critique initial maxalloc=" + String(maBefore) + ", apres delay(10) maxalloc=" + String(maAfter));
      if (maAfter < 6000) {
        uploadErrorMsg = "ERR: heap critique, reessayez";
        Serial.println("[WEB] Upload refuse (heap critique, maxalloc=" + String(maAfter) + ")");
        return;
      }
      Serial.println("[WEB] Upload : creux transitoire resorbe, poursuite normale");
    }
    String path = "/gifs/" + uploadDir + "/" + uploadCurName;
    String dirPath = "/gifs/" + uploadDir;
    if (!SD.exists(dirPath.c_str())) {
      // Le JS appelle /create-folder avant le premier fichier -- ce cas ne
      // devrait normalement plus se produire. Filet de securite minimal
      // (pas de workaround fichier-temoin ici : trop lent pour le chemin
      // critique de l'upload, cf. handleWebConfigCreateFolder()).
      SD.mkdir(dirPath.c_str());
      Serial.println("[WEB] Upload: dossier absent au demarrage, mkdir de secours " + dirPath);
    }
    if (SD.exists(path.c_str())) SD.remove(path.c_str());
    uploadFile = SD.open(path.c_str(), FILE_WRITE);
    if (!uploadFile) {
      // Un dossier tout juste cree peut ne pas etre immediatement pret en
      // ecriture sur certaines cartes SD -- une nouvelle tentative apres un
      // court delai resout ce cas sans risquer de casser la connexion HTTP.
      delay(50);
      uploadFile = SD.open(path.c_str(), FILE_WRITE);
      Serial.println("[WEB] Upload: 2e tentative ouverture " + path + " -> " + (uploadFile ? "OK" : "FAIL"));
    }
    uploadStartMs = millis();
    uploadTotalBytes = 0;
    if (!uploadFile) {
      uploadErrorMsg = "ERR: ecriture SD impossible";
      Serial.println("[WEB] Upload start FAIL: " + path);
      return;
    }
    Serial.println("[WEB] Upload start: " + path);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      uploadTotalBytes += upload.currentSize;
      delay(1);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    webServer->client().setTimeout(3000);
    // Accumule le resultat de CETTE part (fichier) dans uploadBatchResults --
    // handleWebConfigUpload() (une seule fois, apres la DERNIERE part de la
    // requete) construit la reponse JSON finale a partir de cet accumulateur.
    if (uploadBatchResults.length() > 0) uploadBatchResults += ",";
    if (uploadFile) {
      uploadFile.close();
      uploadFile = File();
      unsigned long dt = millis() - uploadStartMs;
      Serial.println("[WEB] Upload done: " + uploadCurName + " " + String(uploadTotalBytes) + " bytes in " + String(dt) + "ms");
      uploadBatchResults += "{\"name\":\"" + jsonEscape(uploadCurName) + "\",\"ok\":true}";
      uploadBatchOkCount++;
    } else {
      String reason = uploadErrorMsg.length() ? uploadErrorMsg : "ERR: echec";
      Serial.println("[WEB] Upload FAIL: " + uploadCurName + " (" + reason + ")");
      uploadBatchResults += "{\"name\":\"" + jsonEscape(uploadCurName) + "\",\"ok\":false,\"err\":\"" + jsonEscape(reason) + "\"}";
    }
    uploadErrorMsg = "";
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    webServer->client().setTimeout(3000);
    if (uploadFile) { uploadFile.close(); uploadFile = File(); }
    Serial.println("[WEB] Upload aborted: " + uploadCurName);
  }
}

static void handleWebConfigSave()
{
  // "brightness" n'est plus obligatoire : chaque page (BASIC/NETWORK/CLOCK/
  // MEDIA) n'envoie que SES propres champs a /save -- l'exiger fait echouer
  // la sauvegarde depuis toutes les pages sauf BASIC.
  if (webServer->hasArg("brightness")) {
    int b = webServer->arg("brightness").toInt();
    if (b >= 0 && b <= 100) {
      screenBrightness = map(b, 0, 100, 0, 255);
      // v55 -- effet immediat sur le DMD des la sauvegarde, sans attendre
      // le prochain reboot (setBrightness8() est sans risque a tout
      // moment, voir commentaire d'en-tete de fichier).
      if (display) display->setBrightness8(screenBrightness);
    }
  }
  if (webServer->hasArg("playlist"))        playlistName = webServer->arg("playlist");
  if (webServer->hasArg("random"))          playlistRandom = webServer->arg("random") == "1";
  if (webServer->hasArg("info"))            showInfo = webServer->arg("info") == "1";
  if (webServer->hasArg("wifi_enabled"))    wifiEnabled = webServer->arg("wifi_enabled") == "1";
  if (webServer->hasArg("wifi_ssid"))       wifiSSID = webServer->arg("wifi_ssid");
  if (webServer->hasArg("wifi_password"))   wifiPassword = webServer->arg("wifi_password");
  if (webServer->hasArg("wifi_static_enabled")) wifiStaticEnabled = webServer->arg("wifi_static_enabled") == "1";
  if (webServer->hasArg("wifi_static_ip"))  wifiStaticIP = webServer->arg("wifi_static_ip");
  if (webServer->hasArg("wifi_gateway"))    wifiGateway = webServer->arg("wifi_gateway");
  if (webServer->hasArg("wifi_subnet"))     wifiSubnet = webServer->arg("wifi_subnet");
  if (webServer->hasArg("wifi_dns1"))       wifiDNS1 = webServer->arg("wifi_dns1");
  if (webServer->hasArg("wifi_dns2"))       wifiDNS2 = webServer->arg("wifi_dns2");
  if (webServer->hasArg("bluetooth_enabled")) bluetoothEnabled = webServer->arg("bluetooth_enabled") == "1";
  if (webServer->hasArg("bluetooth_name"))  bluetoothName = webServer->arg("bluetooth_name");
  if (webServer->hasArg("recalbox_ip"))     recalboxIP = webServer->arg("recalbox_ip");
  if (webServer->hasArg("clock_enabled"))   clockEnabled = webServer->arg("clock_enabled") == "1";
  if (webServer->hasArg("clock_theme"))     clockTheme = webServer->arg("clock_theme").toInt();
  if (webServer->hasArg("clock_neon_color_enabled")) clockNeonCustomColor = webServer->arg("clock_neon_color_enabled") == "1";
  if (webServer->hasArg("clock_neon_color")) {
    String v = webServer->arg("clock_neon_color");
    if (v.startsWith("#") && v.length() == 7) {
      unsigned long cv = strtoul(v.substring(1).c_str(), NULL, 16);
      clockNeonR = (cv >> 16) & 0xFF; clockNeonG = (cv >> 8) & 0xFF; clockNeonB = cv & 0xFF;
    }
  }
  if (webServer->hasArg("clock_interval"))  clockIntervalGifs = webServer->arg("clock_interval").toInt();
  if (webServer->hasArg("clock_interval_min")) clockIntervalMin = webServer->arg("clock_interval_min").toInt();
  if (webServer->hasArg("clock_duration"))  clockDuration = webServer->arg("clock_duration").toInt();
  if (webServer->hasArg("clock_tz"))        clockTimeZone = webServer->arg("clock_tz");

  int b = (screenBrightness * 100 + 127) / 255;

  File f = SD.open("/config.ini", FILE_WRITE);
  if (!f) { webServer->send(500, "text/plain", "ERR: SD write failed"); return; }
  f.println("# Info"); f.println("info=" + String(showInfo ? "1" : "0"));
  // language= re-ecrit ici avec la valeur RAM courante (uiLanguage, chargee
  // au boot depuis config.ini puis mise a jour immediatement par
  // handleWebConfigSaveLanguage() a chaque changement de langue) -- bug
  // corrige 2026-08-05, signale par l'utilisateur : cette cle n'etait
  // JAMAIS re-emise par cette reecriture complete de config.ini (utilisee
  // par les pages BASIC/NETWORK/CLOCK/MEDIA), donc silencieusement perdue
  // des la 1ere sauvegarde depuis l'une de ces pages, meme si elle avait ete
  // correctement ecrite par l'outil PC juste avant. handleWebConfigSaveAP()
  // (page AP) n'est PAS concernee : elle patche cle par cle via
  // writeConfigFlag(), qui preserve deja les cles non touchees.
  f.println("language=" + uiLanguage);
  f.println(); f.println("# Affichage"); f.println("brightness=" + String(b));
  f.println(); f.println("# Playlist"); f.println("playlist=" + playlistName); f.println("random=" + String(playlistRandom ? "1" : "0"));
  f.println(); f.println("# Wi-Fi & Bluetooth");
  f.println("wifi_enabled=" + String(wifiEnabled ? "1" : "0")); f.println("wifi_ssid=" + wifiSSID); f.println("wifi_password=" + wifiPassword);
  f.println("bluetooth_enabled=" + String(bluetoothEnabled ? "1" : "0")); f.println("bluetooth_name=" + bluetoothName);
  f.println(); f.println("wifi_static_enabled=" + String(wifiStaticEnabled ? "1" : "0")); f.println("wifi_static_ip=" + wifiStaticIP);
  f.println("wifi_gateway=" + wifiGateway); f.println("wifi_subnet=" + wifiSubnet);
  f.println("wifi_dns1=" + wifiDNS1); f.println("wifi_dns2=" + wifiDNS2);
  f.println(); f.println("# MQTT"); f.println("recalbox_ip=" + recalboxIP);
  f.println(); f.println("# Clock (horloge retro themes)");
  f.println("[CLOCK]"); f.println("CLOCK_ENABLED=" + String(clockEnabled ? "1" : "0"));
  f.println("CLOCK_THEME=" + String(clockTheme)); f.println("CLOCK_INTERVAL=" + String(clockIntervalGifs));
  f.println("CLOCK_INTERVAL_MIN=" + String(clockIntervalMin)); f.println("CLOCK_DURATION=" + String(clockDuration));
  if (clockNeonCustomColor) {
    char neonColorBuf[8];
    snprintf(neonColorBuf, sizeof(neonColorBuf), "#%02X%02X%02X", clockNeonR, clockNeonG, clockNeonB);
    f.println("CLOCK_COLOR=" + String(neonColorBuf));
  } else {
    f.println("CLOCK_COLOR=");
  }
  f.println("TZ=" + clockTimeZone);
  f.println();
  // first_boot ne passe a 0 QUE si la config est reellement complete
  // (playlist par defaut ET IP Recalbox renseignees) -- bug corrige
  // 2026-08-05, demande utilisateur (etape 3 de la logique cible) :
  // auparavant ecrit inconditionnellement ici, meme depuis une simple
  // sauvegarde de la page CLOCK sans jamais avoir renseigne playlist/IP.
  // Si la config reste incomplete, g_firstBoot garde sa valeur courante
  // (jamais remis a true ici : une fois la config complete atteinte au
  // moins une fois, elle ne "redevient" pas premier demarrage si un
  // champ est efface plus tard -- needWebConfigMode, setup(), continue
  // de toute facon a re-proposer le mode config tant que playlist/IP
  // sont vides, independamment de first_boot).
  if (playlistName.length() > 0 && recalboxIP.length() > 0) g_firstBoot = false;
  f.println("first_boot=" + String(g_firstBoot ? "1" : "0"));
  f.close();
  Serial.println("[WEB] config.ini saved (brightness=" + String(b) + "%)");
  webServer->send(200, "text/plain", "OK");
}

static bool forceDeleteFile(const String &path)
{
  if (SD.remove(path.c_str())) return true;
  // Lecture seule FAT32 : rename fonctionne (f_rename ignore AM_RDO),
  // puis on supprime le fichier renomme
  String tmpPath = path + ".del";
  int tries = 0;
  while (SD.exists(tmpPath.c_str()) && tries < 20) { tmpPath += "_"; tries++; }
  if (!SD.exists(tmpPath.c_str()) && SD.rename(path.c_str(), tmpPath.c_str())) {
    bool ok = SD.remove(tmpPath.c_str());
    if (ok) return true;
    SD.rename(tmpPath.c_str(), path.c_str()); // restaurer si echec
  }
  Serial.println("[WEB] forceDeleteFile FAIL: " + path);
  return false;
}

static bool deleteFolderRecursive(const String &path)
{
  File dir = SD.open(path.c_str());
  if (!dir) { Serial.println("[WEB] deleteFolder: impossible d'ouvrir " + path); return false; }
  if (!dir.isDirectory()) { dir.close(); bool ok = forceDeleteFile(path); Serial.println("[WEB] deleteFile: " + path + " -> " + (ok?"OK":"FAIL")); return ok; }
  bool allOk = true;
  File f = dir.openNextFile();
  while (f) {
    String fn = String(f.name());
    String fullPath = path + "/" + fn;
    if (f.isDirectory()) {
      f.close();
      if (fn == "." || fn == "..") { f = dir.openNextFile(); delay(1); continue; }
      if (!deleteFolderRecursive(fullPath)) allOk = false;
    } else {
      f.close();
      if (!forceDeleteFile(fullPath)) allOk = false;
    }
    f = dir.openNextFile();
    delay(1);
  }
  dir.close();
  bool ok = SD.rmdir(path.c_str());
  if (!ok) {
    // FAT32 lecture seule : rename le dossier puis rmdir le renomme
    String tmpPath = path + ".del";
    int tries = 0;
    while (SD.exists(tmpPath.c_str()) && tries < 20) { tmpPath += "_"; tries++; }
    if (!SD.exists(tmpPath.c_str()) && SD.rename(path.c_str(), tmpPath.c_str())) {
      if (SD.rmdir(tmpPath.c_str())) {
        ok = true;
      } else {
        SD.rename(tmpPath.c_str(), path.c_str()); // restaurer
      }
    }
    if (!ok) {
      Serial.println("[WEB] rmdir FAIL (readonly?) : " + path);
      allOk = false;
    }
  }
  return allOk;
}

// A.1 (plan cache_master_gifs, portee ici depuis master 2026-08-02 pour
// tester Parties A/B/C ensemble) -- Nettoie une playlist des lignes qui
// referencent un dossier venant d'etre supprime. Sans cela, rien ne met a
// jour les playlists existantes quand un dossier qu'elles referencent
// disparait : openNextGif() (RecalBox_DMD.ino) n'a aucune tolerance aux
// fichiers manquants -- ecran noir fige a cet index. deletedNamesCsv au
// format ",nom1,nom2," (test d'appartenance par indexOf("," + dir + ",")).
// Lecture par blocs fixes de 512 octets avec report de ligne incomplete
// (meme algorithme que handleWebConfigPlaylistDirs()) -- jamais
// f.readString() (blocage 40-44s mesure sur une grosse playlist en test
// reel). Fichier temporaire + echange atomique (forceDeleteFile() +
// SD.rename(), jamais de rename par-dessus un fichier existant). Retourne
// false si aucune ligne n'a ete retiree (rien a faire).
static bool stripDeletedFoldersFromPlaylist(const String &plBaseName, const String &deletedNamesCsv, int &linesRemovedOut)
{
  linesRemovedOut = 0;
  String path = "/playlists/" + plBaseName + ".txt";
  File f = SD.open(path.c_str());
  if (!f) return false;

  String tmpPath = path + ".new";
  int tries = 0;
  while (SD.exists(tmpPath.c_str()) && tries < 20) { tmpPath += "_"; tries++; }
  File out = SD.open(tmpPath.c_str(), FILE_WRITE);
  if (!out) {
    f.close();
    Serial.println("[WEB] stripDeletedFoldersFromPlaylist: impossible de creer " + tmpPath);
    return false;
  }

  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending; pending.reserve(BUFSZ + 300);
  String outBuf; outBuf.reserve(1200);
  int removed = 0;

  while (true) {
    int n = f.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      String trimmed = line; trimmed.trim();
      bool drop = false;
      if (trimmed.startsWith("/gifs/")) {
        int s2 = trimmed.indexOf('/', 6);
        if (s2 > 6) {
          String dirName = trimmed.substring(6, s2);
          if (deletedNamesCsv.indexOf("," + dirName + ",") >= 0) drop = true;
        }
      }
      if (drop) removed++;
      else { outBuf += line; outBuf += "\n"; }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart); // garde le reste incomplet pour le prochain tour
    if (outBuf.length() > 1000) { writeBufChecked(out, outBuf); outBuf = ""; }
    if ((size_t)n < BUFSZ) break;
  }
  pending.trim();
  if (pending.length() > 0) { // derniere ligne sans retour a la ligne final
    bool drop = false;
    if (pending.startsWith("/gifs/")) {
      int s2 = pending.indexOf('/', 6);
      if (s2 > 6) {
        String dirName = pending.substring(6, s2);
        if (deletedNamesCsv.indexOf("," + dirName + ",") >= 0) drop = true;
      }
    }
    if (drop) removed++;
    else { outBuf += pending; outBuf += "\n"; }
  }
  if (outBuf.length() > 0) writeBufChecked(out, outBuf);
  f.close();
  out.close();

  if (removed == 0) {
    forceDeleteFile(tmpPath); // rien a faire, jeter le brouillon
    return false;
  }

  if (!forceDeleteFile(path) || !SD.rename(tmpPath.c_str(), path.c_str())) {
    Serial.println("[WEB] stripDeletedFoldersFromPlaylist: echec remplacement " + path);
    forceDeleteFile(tmpPath);
    return false;
  }

  const char *companionExts[] = {".cache", ".sig", ".idx"};
  for (int i = 0; i < 3; i++) {
    String companion = "/playlists/" + plBaseName + companionExts[i];
    if (SD.exists(companion.c_str())) SD.remove(companion.c_str());
  }
  invalidatePlaylistRefCache();
  linesRemovedOut = removed;
  Serial.println("[WEB] stripDeletedFoldersFromPlaylist: " + plBaseName + ".txt -- " + String(removed) + " ligne(s) retiree(s)");
  return true;
}

static void handleWebConfigDeleteFolders()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  if (!webServer->hasArg("dirs")) { webServer->send(400, "text/plain", "ERR: missing dirs"); return; }
  String dirs = webServer->arg("dirs");
  int count = 0, fail = 0, start = 0;
  // A.2 (plan cache_master_gifs) -- accumule uniquement les dossiers
  // REELLEMENT supprimes (deleteFolderRecursive() == true), au format
  // ",nom1,nom2," attendu par stripDeletedFoldersFromPlaylist().
  String deletedNamesCsv = ",";
  while (true) {
    int comma = dirs.indexOf(',', start);
    String d = (comma < 0) ? dirs.substring(start) : dirs.substring(start, comma);
    d.trim();
    if (d.length() > 0) {
      String path = "/gifs/" + d;
      if (SD.exists(path.c_str())) {
        Serial.println("[WEB] deleteFolder start: " + path);
        if (deleteFolderRecursive(path)) { count++; deletedNamesCsv += d + ","; Serial.println("[WEB] deleteFolder OK: " + path); }
        else { fail++; Serial.println("[WEB] deleteFolder FAIL: " + path); }
      } else {
        Serial.println("[WEB] deleteFolder introuvable: " + path);
      }
    }
    if (comma < 0) break;
    start = comma + 1;
  }

  // A.2 -- nettoie toutes les playlists existantes des lignes qui
  // referencaient un des dossiers effectivement supprimes ci-dessus.
  // cache_master_gifs.dat (TOUS_MASTER_PATH) est intentionnellement exclu :
  // il n'est jamais lu playlist par playlist pendant la lecture DMD, son
  // eventuel contenu perime pour ce dossier sera simplement ignore/reecrit
  // a la prochaine generation qui le concerne.
  int plModified = 0, totalLinesRemoved = 0;
  if (deletedNamesCsv.length() > 1) {
    String masterBase = String(TOUS_MASTER_PATH);
    masterBase = masterBase.substring(masterBase.lastIndexOf('/') + 1);
    File plDir = SD.open("/playlists");
    if (plDir && plDir.isDirectory()) {
      File entry = plDir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        bool isDirEntry = entry.isDirectory();
        entry.close();
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;
        if (!isDirEntry && base.endsWith(".txt") && base != masterBase) {
          String plBaseName = base.substring(0, base.length() - 4);
          int linesRemoved = 0;
          if (stripDeletedFoldersFromPlaylist(plBaseName, deletedNamesCsv, linesRemoved)) {
            plModified++;
            totalLinesRemoved += linesRemoved;
            Serial.println("[WEB] playlist mise a jour: " + plBaseName + ".txt (" + String(linesRemoved) + " ligne(s) retiree(s))");
          }
        }
        entry = plDir.openNextFile();
        delay(1);
      }
      plDir.close();
    }
  }

  String msg = "OK: " + String(count) + " supprime(s)" + (fail>0?", " + String(fail) + " echec(s)":"");
  if (plModified > 0) msg += ", " + String(plModified) + " playlist(s) mise(s) a jour (" + String(totalLinesRemoved) + " ligne(s) retiree(s)), redemarrage necessaire";
  webServer->send(200, "text/plain", msg);
}

// v92 -- handleWebConfigDeleteFiles() (suppression de fichiers individuels
// dans un dossier) retiree : plus aucune page ne l'appelle -- voir plan de
// reconstruction.

// v92 -- handleWebConfigAddToPlaylists() (mise a jour PAR FICHIER, route
// /add-to-playlists singulier) retiree : remplacee par
// handleWebConfigAddToPlaylistsBatch()//add-to-playlists-batch (cache
// g_plRefCache* + lecture bufferisee, voir plus haut).

// Forward declaration : definie plus bas, mais appelee ici par
// handleDmdOpen() -- sans cette declaration, erreur de compilation "not
// declared in this scope".
static void triggerWebConfigModeSoft(const String &msg);

static void handleDmdPause()
{
  if (!webServer->hasArg("msg")) { webServer->send(400, "text/plain", "ERR: missing msg"); return; }
  String msg = webServer->arg("msg");
  String colorStr = webServer->arg("color");
  uint16_t color = 0xFFFF;
  if (colorStr == "1") color = 0x07E0;
  else if (colorStr == "2") color = 0xF800;
  webDmdPause(msg, color);
  webServer->send(200, "text/plain", "OK");
}

static void handleDmdResume()
{
  webServer->send(200, "text/plain", "OK REBOOT");
  delay(100);
  webDmdResume();
}

static void handleDmdOpen()
{
  if (!webServer->hasArg("msg")) { webServer->send(400, "text/plain", "ERR: missing msg"); return; }
  String msg = webServer->arg("msg");
  String full = msg + " " + WiFi.localIP().toString();
  triggerWebConfigModeSoft(msg);
  webServer->send(200, "text/plain", "OK " + full);
}

// v55 -- apercu live pendant le drag du curseur de luminosite (page
// BASIC). Volontairement distinct de /save : RAM uniquement, AUCUNE
// ecriture sur /config.ini ici (ca reste le role explicite du bouton
// "Sauvegarder") -- evite aussi de spammer la carte SD pendant un drag
// rapide. setBrightness8() est sans risque a tout moment (voir en-tete de
// fichier).
static void handleWebConfigSetBrightness()
{
  if (!webServer->hasArg("value")) { webServer->send(400, "text/plain", "ERR: missing value"); return; }
  int b = webServer->arg("value").toInt();
  if (b < 0 || b > 100) { webServer->send(400, "text/plain", "ERR: out of range"); return; }
  screenBrightness = map(b, 0, 100, 0, 255);
  if (display) display->setBrightness8(screenBrightness);
  webServer->send(200, "text/plain", "OK");
}

// v72 -- apercu en direct d'un theme horloge (page Horloge). "theme" =
// "-1".."9" (cf. <select id="clock_theme">) ou "stop" (arret, envoye via
// sendBeacon au moment de quitter la page). Se contente de poser la
// commande interne CMD_CLOCK_PREVIEW (voir requestClockPreview() dans
// RecalBox_DMD.ino) -- ne bloque jamais ce handler, l'affichage reel est
// gere par processPendingMqttCommand() au prochain tour de loop(). N'ecrit
// jamais /config.ini (independant du bouton "Sauvegarder").
static void handleWebConfigClockPreview()
{
  if (!webServer->hasArg("theme")) { webServer->send(400, "text/plain", "ERR: missing theme"); return; }
  requestClockPreview(webServer->arg("theme"));
  webServer->send(200, "text/plain", "OK");
}

static void handleWebConfigReboot() { webServer->send(200, "text/plain", "REBOOT"); delay(500); ESP.restart(); }

static void handleWebConfigScanWiFi()
{
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) { WiFi.scanNetworks(true); webServer->send(200, "application/json", "[]"); return; }
  if (n == WIFI_SCAN_RUNNING) { webServer->send(200, "application/json", "[]"); return; }
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\"", "\\\"");
    json += "\"" + ssid + "\"";
  }
  json += "]";
  WiFi.scanDelete();
  webServer->send(200, "application/json", json);
}

static void handleWebConfigLang()
{
  // "first_boot" ajoute (2026-08-05, demande utilisateur) : deja
  // l'endpoint appele au bootstrap de CHAQUE page (fetch('/lang'), meme
  // script sur les 5 pages) -- reutilise pour exposer cet etat au JS sans
  // aller-retour reseau supplementaire (declenche la modale d'accueil
  // premier demarrage, voir showHelpModal() cote JS de chaque page).
  webServer->send(200, "application/json", "{\"language\":\"" + uiLanguage + "\",\"first_boot\":\"" + String(g_firstBoot ? "1" : "0") + "\"}");
}

static void handleWebConfigSaveLanguage()
{
  if (!webServer->hasArg("language")) { webServer->send(400, "text/plain", "ERR: missing language"); return; }
  String lang = webServer->arg("language");
  if (lang != "fr" && lang != "en" && lang != "es") { webServer->send(400, "text/plain", "ERR: invalid language"); return; }
  writeConfigFlag("language", lang);
  uiLanguage = lang;
  webServer->send(200, "text/plain", "OK");
}

static void handleWebConfigSaveAP()
{
  if (!webServer->hasArg("wifi_ssid")) { webServer->send(400, "text/plain", "ERR: missing SSID"); return; }
  wifiEnabled = true;
  wifiSSID = webServer->arg("wifi_ssid"); wifiSSID.trim();
  wifiPassword = webServer->arg("wifi_password"); wifiPassword.trim();
  bool hasStatic = webServer->hasArg("wifi_static_ip");
  if (hasStatic) { String sip = webServer->arg("wifi_static_ip"); sip.trim(); hasStatic = (sip.length() > 0); }
  if (hasStatic) {
    wifiStaticEnabled = true;
    wifiStaticIP = webServer->arg("wifi_static_ip"); wifiStaticIP.trim();
  } else {
    wifiStaticEnabled = false;
    wifiStaticIP = "";
  }
  // Ecrire config.ini -- patch cle par cle (writeConfigFlag), JAMAIS un
  // SD.remove()+rewrite complet : ecraserait silencieusement toutes les
  // autres cles deja presentes (recalbox_ip, playlist, brightness, clock_*,
  // language...). Meme bug/fix que le 2026-07-21 (voir memoire projet),
  // reintroduit par la refonte multi-pages -- confirme responsable de la
  // disparition de recalbox_ip signalee par l'utilisateur.
  Serial.println("[WEB] AP save: ecriture config.ini (SSID=" + wifiSSID + ")");
  writeConfigFlag("wifi_enabled", "1");
  writeConfigFlag("wifi_ssid", wifiSSID);
  writeConfigFlag("wifi_password", wifiPassword);
  writeConfigFlag("wifi_static_enabled", wifiStaticEnabled ? "1" : "0");
  writeConfigFlag("wifi_static_ip", wifiStaticIP);
  // NE PAS ecrire first_boot ici (bug corrige 2026-08-05) : cette page
  // ne couvre que la 1ere des 2 phases du premier demarrage (WiFi).
  // first_boot ne passe a 0 QUE dans handleWebConfigSave() (page BASIC/
  // NETWORK/CLOCK/MEDIA), et seulement si la sauvegarde laisse playlist
  // ET recalbox_ip non vides -- jamais ici, jamais par le simple fait
  // d'ouvrir une page (voir triggerWebConfigModeSoft()). L'ecrire ici
  // desactivait a tort tout le parcours "premier demarrage" des la 1ere
  // phase -- au reboot suivant (WiFi maintenant connecte), l'ecran
  // d'invitation a terminer la config (voir needWebConfigMode dans
  // setup()) etait silencieusement saute.
  Serial.println("[WEB] AP save: fichier ecrit avec SSID=" + wifiSSID + " -> reboot");
  webServer->send(200, "text/plain", "OK");
  delay(1000);
  ESP.restart();
}

// Pause simple, SANS jamais rebooter -- utilisee par TOUTES les pages de
// config (Root/BASIC/NETWORK/CLOCK/MEDIA/handleDmdOpen) et par
// UPLOAD_FILE_START. Le reboot cible (v42/v43) a ete restreint (v44,
// demande explicite utilisateur) au seul point ou il est reellement prouve
// necessaire : le clic sur "Uploader" (voir handleWebConfigPrepareUpload()
// plus bas), pas l'ouverture de n'importe quelle page. Aucune preuve que
// l'ecriture de playlists (generation BASIC, ou l'ajout aux playlists en
// fin d'upload, /add-to-playlists-batch -- confirme sans souci meme en fin
// de lot d'upload par l'utilisateur) souffre du meme plafond heap que
// l'ecriture GIF volumineuse de l'upload -- perimetre volontairement
// restreint, a elargir seulement si un echec reel est constate ailleurs.
static void triggerWebConfigModeSoft(const String &msg)
{
  // "http://" explicite (2026-07-30, demande utilisateur) : certains
  // navigateurs (Firefox "HTTPS-First", Edge) tentent une connexion HTTPS
  // avant HTTP des qu'une adresse est tapee SANS schema, avec un delai
  // d'attente de plusieurs dizaines de secondes avant le repli sur HTTP (le
  // DMD n'ecoute qu'en HTTP, aucun moyen cote firmware d'empecher cette
  // tentative HTTPS qui se joue entierement avant l'envoi de la moindre
  // requete). En affichant l'URL complete avec schema, un utilisateur qui
  // COPIE/RETAPE exactement ce qui est affiche evite le declenchement de ce
  // mecanisme, sans reglage navigateur particulier.
  //
  // Repli 0.0.0.0 (bug corrige 2026-08-05) : WiFi.localIP() est vide en
  // mode AP pur -- cette fonction est appelee par TOUS les handlers de
  // page, y compris la racine "/" servie par la page AP elle-meme, donc
  // s'execute aussi a ce moment-la. Meme repli que partout ailleurs dans
  // ce fichier.
  String ip = WiFi.localIP().toString();
  if (ip == "0.0.0.0") ip = WiFi.softAPIP().toString();
  if (ip == "0.0.0.0") ip = "192.168.4.1";
  String url = "http://" + ip;
  // clearFirstBoot() retire d'ici (bug corrige 2026-08-05, demande
  // utilisateur -- etape 3 de la logique cible) : le simple AFFICHAGE
  // d'une page ne doit plus jamais effacer first_boot, seule une
  // sauvegarde reellement complete (handleWebConfigSave()) le fait
  // desormais.
  webDmdSetMainMsg(msg);
  webDmdPause(url, 0xFFE0);
}

// Pre-vol AJAX (v44) appele par le JS de la page MEDIA juste avant de
// demarrer la boucle d'upload (clic sur "Uploader", avant le 1er fichier).
// Remplace l'ancien reboot systematique a l'ouverture de la page MEDIA
// (v42/v43) -- demande explicite utilisateur : ouvrir MEDIA pour juste
// supprimer un dossier ne justifie pas un reboot, seul le fait de
// reellement lancer un upload le justifie (ecriture SD volumineuse, buffer
// setvbuf(4096) alloue par SD.open() jamais recycle proprement, cf. v42/v43
// pour le detail complet). Reponse JSON (pas de page HTML complete, cet
// appel part d'une page deja chargee) : {"reboot":true} si un reboot cible
// vient d'etre declenche (le JS doit alors afficher un message d'attente et
// recharger la page une fois l'ESP32 revenu, cf. uploadGif() page MEDIA),
// {"reboot":false} sinon (le JS peut demarrer l'upload immediatement).
static void handleWebConfigPrepareUpload()
{
  if (g_playlistStartedThisBoot) {
    Serial.println("[WEB] prepare-upload: playlist deja active -> reboot cible mode config");
    writeConfigFlag("force_config_boot", "1");
    webServer->send(200, "application/json", "{\"reboot\":true}");
    requestReboot = true;
    return;
  }
  webServer->send(200, "application/json", "{\"reboot\":false}");
}

static void sendGzipHtml(const uint8_t *content, size_t len)
{
  // TCP_NODELAY (2026-08-03, test reel iOS Safari : "connexion reseau
  // perdue" a repetition sur TOUTES les pages, meme la plus legere -- MENU,
  // 6.7 Ko gzip -- alors que le log serie confirme que la requete atteint
  // bien le serveur a chaque fois (triggerWebConfigModeSoft() s'execute) et
  // que le garde heap juste en dessous ne se declenche pas (maxalloc>4096)
  // -- la coupure n'est donc pas expliquee par le garde-fou existant).
  // Hypothese testee : l'algorithme de Nagle (actif par defaut sur les
  // sockets ESP32) retarde l'envoi de petits paquets en attendant soit un
  // ACK, soit assez de donnees a grouper -- combine au mode economie
  // d'energie WiFi des telephones (radio en veille entre paquets,
  // contrairement a un PC), l'attente peut depasser le delai de patience de
  // Safari mobile (plus impatient qu'un navigateur desktop), qui abandonne
  // la connexion en cours de transfert. setNoDelay(true) desactive Nagle --
  // fix standard, faible risque, deja largement documente pour cette classe
  // de probleme "ESP32 WebServer marche en desktop, coupe sur mobile".
  webServer->client().setNoDelay(true);
  // v50 (2026-08-03) -- webServer->client().setTimeout(3000) ESSAYE puis
  // ABANDONNE : verifie sans effet par lecture du code source de la lib
  // reseau (NetworkClient.cpp) -- write() envoie via send(..., MSG_DONTWAIT),
  // qui ignore purement et simplement SO_SNDTIMEO/notre setTimeout(). Le vrai
  // blocage observe (10-25s, escalade) vient d'une boucle de retry codee en
  // dur dans la bibliotheque (10 tentatives x select() 1s, compteur RESET a
  // 10 des qu'un seul octet passe) -- non configurable depuis ce sketch. Fix
  // retenu a la place, plus bas : decoupage manuel de l'envoi (voir
  // commentaire avant la boucle).
  // Garde-fou heap (2026-07-29, ERR_EMPTY_RESPONSE reel en test materiel) :
  // envoyer une page complete (plusieurs Ko gzip) peut echouer si le heap
  // est deja tres sollicite par un long scan playlistGenTask() en cours --
  // la connexion se fermait alors sans aucune donnee envoyee (vu cote
  // navigateur comme une page vide, ERR_EMPTY_RESPONSE, sur plusieurs
  // navigateurs differents -- pas un souci cote client). Repli sur une
  // reponse texte minimaliste (bien moins gourmande a envoyer, donc bien
  // plus susceptible de reussir meme sous pression heap) plutot que de
  // risquer le meme echec silencieux.
  // Seuil corrige de 8192 a 4096 (2026-07-29, meme erreur que pour le
  // garde-fou du scan) : maxalloc se situe couramment entre 4500 et 9000 en
  // fonctionnement tout a fait normal (meme sans generation active) --
  // 8192 declenchait ce message quasi en permanence, meme entre 2 pages ou
  // sur le simple menu. 4096 correspond a la valeur deja validee sans souci
  // par le garde-fou du scan lui-meme.
  if (ESP.getMaxAllocHeap() < 4096) {
    // Message volontairement generique (2026-07-29) : la cause reelle du
    // heap bas n'est pas forcement une generation de playlist en cours
    // (retour utilisateur : message trompeur affiche hors de tout scan) --
    // ne pas presumer d'une cause precise qui peut etre fausse.
    Serial.println("[WEB] sendGzipHtml: repli memoire faible, maxalloc=" + String(ESP.getMaxAllocHeap()) + " libre=" + String(ESP.getFreeHeap())); // DIAGNOSTIC TEMPORAIRE (2026-07-30) -- lenteur page rapportee hors generation
    webServer->send(200, "text/plain", "Memoire faible, reessayez dans quelques secondes");
    return;
  }
  // Envoi manuel par petits blocs avec abandon rapide (v50, 2026-08-03) --
  // cause racine confirmee par l'instrumentation v48/v49 (test reel iOS) +
  // lecture du code source de la lib reseau : send_P() remet toute la page
  // en UN SEUL appel write() a la bibliotheque -- si la connexion delivre au
  // compte-goutte, le compteur de retry interne (10 tentatives x 1s) se
  // RESET a chaque octet qui passe, pouvant etirer un seul appel a 18-25s+
  // (mesure en test reel) avant que Safari, bien moins patient, n'ait deja
  // abandonne de son cote. En decoupant nous-memes l'envoi et en verifiant
  // le retour de CHAQUE write(), un bloc qui echoue COMPLETEMENT (write()
  // renvoie moins que demande -- ses 10 tentatives internes deja epuisees
  // SANS le moindre progres sur ce bloc precis) est detecte des ce premier
  // bloc perdu : la connexion est alors coupee proprement plutot que de
  // laisser la bibliotheque s'acharner sur le reste de la page. Plafonne le
  // pire cas a ~10s (un seul bloc bloque) au lieu de 18-25s (tout le buffer).
  // En-tete HTTP construit a la main (sendHeader()/_prepareHeader() internes
  // a la lib ne sont pas accessibles hors de send()/send_P()) -- volontairement
  // minimal (Content-Type/Content-Encoding/Content-Length/Connection: close),
  // rien d'autre n'est utilise par ce firmware (pas de CORS, pas d'en-tete
  // additionnel a ce stade).
  unsigned long tSendStart = millis();
  Serial.println("[WEB] sendGzipHtml: envoi " + String(len) + " octets par blocs, maxalloc=" + String(ESP.getMaxAllocHeap()) + " libre=" + String(ESP.getFreeHeap()));
  {
    String header = webServer->version() + " 200 " + WebServer::responseCodeToString(200) + "\r\n";
    header += "Content-Type: text/html\r\n";
    header += "Content-Encoding: gzip\r\n";
    header += "Content-Length: " + String(len) + "\r\n";
    header += "Connection: close\r\n\r\n";
    webServer->sendContent(header);
  }
  const size_t CHUNK_SIZE = 1024;
  size_t sentTotal = 0;
  bool stalled = false;
  while (sentTotal < len)
  {
    size_t toSend = (len - sentTotal < CHUNK_SIZE) ? (len - sentTotal) : CHUNK_SIZE;
    size_t written = webServer->client().write(content + sentTotal, toSend);
    if (written < toSend)
    {
      Serial.println("[WEB] sendGzipHtml: bloc bloque a " + String(sentTotal) + "/" + String(len) + " octets -- connexion coupee");
      webServer->client().stop();
      stalled = true;
      break;
    }
    sentTotal += written;
  }
  Serial.println("[WEB] sendGzipHtml: " + String(stalled ? "abandon" : "termine") + " en " + String(millis() - tSendStart) + "ms (" + String(sentTotal) + "/" + String(len) + " octets)");
}

static void handleWebConfigRoot()
{
  triggerWebConfigModeSoft("WEB DMD CONFIG");
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    sendGzipHtml(WEB_CONFIG_AP_HTML_GZ, WEB_CONFIG_AP_HTML_GZ_LEN);
  } else {
    sendGzipHtml(WEB_CONFIG_MENU_HTML_GZ, WEB_CONFIG_MENU_HTML_GZ_LEN);
  }
}

static void handleWebConfigBasicPage()
{
  triggerWebConfigModeSoft("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_BASIC_HTML_GZ, WEB_CONFIG_BASIC_HTML_GZ_LEN);
}

static void handleWebConfigNetworkPage()
{
  triggerWebConfigModeSoft("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_NETWORK_HTML_GZ, WEB_CONFIG_NETWORK_HTML_GZ_LEN);
}

static void handleWebConfigClockPage()
{
  triggerWebConfigModeSoft("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_CLOCK_HTML_GZ, WEB_CONFIG_CLOCK_HTML_GZ_LEN);
}

static void handleWebConfigMediaPage()
{
  triggerWebConfigModeSoft("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_MEDIA_HTML_GZ, WEB_CONFIG_MEDIA_HTML_GZ_LEN);
}

void setupWebConfig()
{
  if (webServer) delete webServer;
  webServer = new WebServer(80);
  webServer->on("/", handleWebConfigRoot);
  webServer->on("/config/basic", handleWebConfigBasicPage);
  webServer->on("/config/network", handleWebConfigNetworkPage);
  webServer->on("/config/clock", handleWebConfigClockPage);
  webServer->on("/config/media", handleWebConfigMediaPage);
  webServer->on("/load", handleWebConfigLoad);
  webServer->on("/lsplaylists", handleWebConfigListPlaylists);
  webServer->on("/lsgifdirs", handleWebConfigListGifDirs);
  webServer->on("/generate-playlist", HTTP_POST, handleWebConfigGeneratePlaylist);
  webServer->on("/generate-playlist-status", handleWebConfigGeneratePlaylistStatus);
  webServer->on("/generate-playlist-stop", HTTP_POST, handleWebConfigGeneratePlaylistStop);
  webServer->on("/playlist-dirs", handleWebConfigPlaylistDirs);
  webServer->on("/delete-playlist", HTTP_POST, handleWebConfigDeletePlaylist);
  webServer->on("/prepare-upload", HTTP_POST, handleWebConfigPrepareUpload);
  webServer->on("/upload", HTTP_POST, handleWebConfigUpload, handleWebConfigUploadFile);
  webServer->on("/create-folder", HTTP_POST, handleWebConfigCreateFolder);
  webServer->on("/delete-folders", HTTP_POST, handleWebConfigDeleteFolders);
  webServer->on("/scan-wifi", handleWebConfigScanWiFi);
  webServer->on("/save-ap", HTTP_POST, handleWebConfigSaveAP);
  // Firefox/Edge demandent systematiquement /favicon.ico au chargement de
  // toute page (Chrome aussi, mais semble plus tolerant) -- sans route
  // dediee, cette requete tombe sur le 404 par defaut de la lib WebServer,
  // point d'incertitude ecarte ici a peu de frais (2026-07-30, lenteur page
  // rapportee, plus marquee sur Firefox/Edge que Chrome).
  webServer->on("/favicon.ico", []() { webServer->send(204); });
  webServer->on("/lang", handleWebConfigLang);
  webServer->on("/save-language", HTTP_POST, handleWebConfigSaveLanguage);
  webServer->on("/add-to-playlists-batch", HTTP_POST, handleWebConfigAddToPlaylistsBatch);
  webServer->on("/dmd-pause", HTTP_POST, handleDmdPause);
  webServer->on("/dmd-resume", HTTP_POST, handleDmdResume);
  webServer->on("/dmd-open", HTTP_POST, handleDmdOpen);
  webServer->on("/set-brightness", HTTP_POST, handleWebConfigSetBrightness);
  webServer->on("/clock-preview", HTTP_POST, handleWebConfigClockPreview);
  webServer->on("/save", HTTP_POST, handleWebConfigSave);
  webServer->on("/reboot", handleWebConfigReboot);
  webServer->begin();
  Serial.println("[WEB] Interface config sur http://" + WiFi.localIP().toString());
  // DIAGNOSTIC TEMPORAIRE (2026-08-02) -- verifie que la redefinition de
  // HTTP_UPLOAD_BUFLEN (voir tout en haut de web_config.h) est bien prise en
  // compte par la bibliotheque WebServer (sizeof(HTTPUpload) doit refleter
  // ~512+quelques octets de champs String/enum, pas ~1436+).
  Serial.println("[WEB] sizeof(HTTPUpload)=" + String(sizeof(HTTPUpload)) + " HTTP_UPLOAD_BUFLEN=" + String(HTTP_UPLOAD_BUFLEN));
}

// Fiabilisation upload (2026-08-02) -- tentative d'interception de
// l'exception std::bad_alloc levee par operator new() quand
// WebServer::_parseForm() echoue a allouer un HTTPUpload sous heap
// fragmente (voir HTTP_UPLOAD_BUFLEN en tete de fichier -- cause confirmee
// par plusieurs backtraces decodees, mais la reduction seule du buffer ne
// suffit pas a l'eliminer). Les exceptions C++ SONT compilees dans ce
// build (confirme : la trace de crash passe par __cxa_throw, jamais genere
// si -fno-exceptions) -- un try/catch ICI, autour de TOUT handleClient(),
// devrait rattraper l'exception avant qu'elle n'atteigne std::terminate()/
// abort() et ne redemarre tout l'appareil. Risque assume, non verifie
// avant ce commit : la bibliotheque WebServer n'est pas concue pour etre
// interrompue en cours de route par une exception -- son etat interne
// (_currentClient/_currentUpload prives) pourrait rester incoherent pour
// l'appel suivant. Mais le pire cas resterait probablement moins grave
// qu'un reboot complet (perte de la session de lecture en cours), donc
// teste malgre l'incertitude.
void handleWebConfig() {
  if (!webServer) return;
  try {
    webServer->handleClient();
  } catch (std::exception &e) {
    Serial.println(String("[WEB] EXCEPTION rattrapee dans handleClient() (probablement heap critique) : ") + e.what());
  } catch (...) {
    Serial.println("[WEB] EXCEPTION inconnue rattrapee dans handleClient()");
  }
}

#endif
