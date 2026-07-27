// ============================================
// web_config.h — Interface web de configuration
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v88
//
// v88 — 2026-07-27 — safe-modify — Reintroduction du reboot cible en mode
//   config, SYSTEMATIQUE (toutes les pages, pas seulement MEDIA). Cause
//   reelle trouvee via logs Serial materiels reels fournis par l'utilisateur
//   (deux boots complets jusqu'au blocage) : mettre en pause un GIF en cours
//   (`webDmdPause()`, appele par `triggerWebConfigMode()`) provoque a lui
//   seul un effondrement de `ESP.getMaxAllocHeap()` (~4596 octets), meme
//   apres l'ouverture d'un seul GIF -- c'est de la FRAGMENTATION (le heap
//   libre total AUGMENTE au meme instant), pas un manque de memoire brut, et
//   ca passe sous le seuil de garde `< 6000` deja utilise par
//   `scanGifDirsRaw()`. Symptomes observes : `/lsgifdirs` renvoie une liste
//   vide en permanence pour le reste du boot ("heap critique"),
//   NS_ERROR_NET_EMPTY_RESPONSE cote navigateur, DMD apparemment fige dans
//   certains cas. Le retrait du reboot cible en v85 (justifie a l'epoque par
//   "MEDIA ne fait plus que des operations dossier, pas besoin de la marge
//   heap") s'est avere insuffisant : BASIC fait AUSSI un vrai scan SD
//   (/lsgifdirs pour la generation de playlist, v79) et souffre de la meme
//   fragmentation. Demande explicite utilisateur : reboot systematique pour
//   TOUTES les pages de config, pas seulement MEDIA. Restaure : externs
//   `g_playlistStartedThisBoot`/`requestReboot`, `sendRebootingPage()`,
//   `triggerWebConfigMode()` repasse de `void` a `bool` (retourne `false` si
//   un reboot a deja ete declenche et la reponse deja envoyee -- l'appelant
//   doit alors s'arreter immediatement sans envoyer sa propre reponse). Les
//   6 points d'appel (handleDmdOpen + les 5 handlers de page) verifient
//   desormais la valeur de retour via `if (!triggerWebConfigMode(...))
//   return;`. Cote .ino : restauration a l'identique de
//   `g_skipPlaylistForConfig`/`force_config_boot` (config.ini),
//   `g_playlistStartedThisBoot`, `requestReboot`, et du bloc de boot dedie
//   qui saute entierement la playlist/l'ouverture de GIF quand le flag est
//   pose. Le reste du retrait v85 (pas de cache par fichier, pas de
//   navigation/suppression fichier par fichier) reste inchange. PAS ENCORE
//   teste sur materiel reel.
//
//
// v87 — 2026-07-27 — safe-modify — Demande utilisateur : la generation de
//   playlist (liste de dossiers a cocher, nom, bouton) doit etre separee
//   graphiquement de la selection de la playlist active (qui reste juste
//   sous la section Affichage). Section "Playlist" (sec_playlist) ne garde
//   plus que le choix de la playlist par defaut + lecture aleatoire.
//   Nouvelle section separee "Gestion des playlists" (sec_manage_playlists,
//   nouvelle cle i18n remplace sec_gen_playlist devenue inutilisee) :
//   generation ET suppression de playlist, regroupees ensemble (toutes
//   deux des actions de GESTION de fichiers playlist, distinctes du choix
//   de lecture). PAS ENCORE teste sur materiel reel.
//
//
// v86 — 2026-07-27 — safe-modify — Bug reel confirme par l'utilisateur :
//   aucun dossier affiche dans la section generation de playlist (BASIC).
//   Cause trouvee : `loadGenDirs()` (v79) etait appelee en PARALLELE de la
//   sequence `fetch('/lang')...` au chargement de la page, au lieu d'etre
//   enchainee apres -- exactement la classe de bug deja documentee et
//   corrigee sur MEDIA via `queuedFetch()` (v45, memoire projet) : le
//   WebServer ESP32 ne traite qu'une requete a la fois, des fetch()
//   concurrents corrompent silencieusement l'une des reponses. BASIC n'a
//   pas de `queuedFetch()` (page plus simple, jusqu'ici sans besoin) --
//   fix minimal : `loadConfig()` retourne desormais sa promesse, et
//   `loadGenDirs()` est chainee en dernier (`.then(loadGenDirs)`) apres
//   /lang PUIS /load, au lieu de partir en parallele. PAS ENCORE teste sur
//   materiel reel.
//
// v85 — 2026-07-27 — safe-modify — Pivot majeur, decision utilisateur :
//   retrait complet de la navigation/suppression de fichiers INDIVIDUELS
//   dans un dossier depuis MEDIA (le fait que certains dossiers soient
//   accessibles et d'autres non selon leur statut de cache posait un
//   probleme d'experience utilisateur -- remplacement envisage : outil PC
//   pour composer des playlists personnalisees, GIF par GIF, a etudier
//   separement, pas commence). Retires : tout le sous-systeme de cache
//   /gifs par dossier (v75-v84 en integralite -- GifCacheStatus,
//   gifFilesCacheStatus/Ready/Excluded, readGifFilesCache,
//   BufferedCacheWriter, toute la machine a etats cacheBuilder*, format
//   V5 horodate) ; handleWebConfigListGifFiles() + route /lsgiffiles ;
//   handleWebConfigDeleteFiles() + route /delete-files ; le mecanisme de
//   reboot cible mode config (triggerWebConfigMode() simplifie, plus de
//   parametre allowReboot, plus jamais de reboot -- g_skipPlaylistForConfig/
//   force_config_boot/sendRebootingPage()/requestReboot retires cote .ino).
//   Verifie explicitement (question utilisateur) que ni le reboot MQTT
//   CMD_REBOOT (Recalbox) ni celui de la page AP (handleWebConfigSaveAP())
//   n'en dependaient. Cote MEDIA : plus d'icone d'ouverture de dossier, la
//   liste redevient une simple selection de dossiers (creer/supprimer/
//   uploader uniquement) -- la suppression de DOSSIERS ENTIERS reste
//   disponible (confirme avec l'utilisateur, distincte de la suppression
//   de fichiers individuels retiree). handleWebConfigListGifDirs() garde
//   scanGifDirsRaw() (liste des noms de dossiers, jamais mise en cache,
//   n'a jamais souffert de la degradation FAT32 qui ne touchait que
//   l'enumeration du CONTENU d'un dossier) mais ne renvoie plus de statut
//   cached/excluded (simple tableau de noms). PAS ENCORE teste sur
//   materiel reel.
//
// v84 — 2026-07-27 — safe-modify — Question utilisateur : un cache genere
//   hors ESP32 (outil PC modifie) serait-il detecte comme perime, et de
//   combien de fichiers ? Verification (v83) : NON, aucune detection.
//   Recherche menee (agent Explore) sur le code source de la lib SD
//   ESP32 (arduino-esp32 core 3.3.11, FS.h/vfs_api.cpp) : File::
//   getLastWrite() delegue a un stat() POSIX brut, SANS distinction
//   fichier/dossier -- donc utilisable sur un dossier pour connaitre son
//   horodatage de derniere modification en cout CONSTANT (pas
//   d'enumeration). Format de cache V4 -> V5 : en-tete etendu avec un
//   horodatage dossier a largeur FIXE (GIF_CACHE_MTIME_WIDTH=10, permet
//   une reecriture en place via seek()+"r+" sans jamais toucher au reste
//   du fichier). gifFilesCacheStatus() compare desormais l'horodatage
//   stocke a l'horodatage REEL du dossier (File::getLastWrite()) : un
//   cache dont l'horodatage ne correspond plus (deplace/genere hors ESP32,
//   ou dossier modifie par un chemin qui aurait echappe a l'invalidation
//   explicite) est traite comme absent et reconstruit. cacheBuilderAdvance
//   ToNextDir() capture l'horodatage au demarrage d'un scan et l'ecrit
//   dans l'en-tete ; l'ajout incremental (v82) reecrit ce champ en place
//   apres coup (sinon le cache tout juste mis a jour se croirait perime
//   des la prochaine lecture). Ne repond PAS a la question du "delta"
//   (combien de fichiers en plus/moins) -- seulement perime oui/non ;
//   un delta precis necessiterait soit un rescan complet, soit que
//   l'outil externe fournisse lui-meme la liste complete (le cache externe
//   remplace alors integralement l'ancien, meme principe qu'un rebuild
//   ESP32). PAS ENCORE teste sur materiel reel -- fiabilite du timestamp
//   FAT sur repertoire confirmee par lecture de code, jamais verifiee en
//   conditions reelles sur cette carte SD/ce materiel.
//
// v83 — 2026-07-27 — safe-modify — Question utilisateur ("si l'utilisateur
//   quitte la page web on doit repartir de zero ?") : reponse verifiee --
//   NON, l'etat de la machine a etats est entierement cote ESP32 (variables
//   globales), independant de toute connexion navigateur ; fermer la page
//   n'affecte rien (et accelere meme la construction, puisque le garde-fou
//   "client web actif" de v78 ne s'applique plus). En revanche, question
//   a fait remarquer un vrai gap adjacent : cacheBuilderStep() n'avait
//   AUCUNE condition sur le mode courant du DMD -- si l'utilisateur
//   cliquait "Reprendre DMD" (retour lecture normale) pendant qu'un scan
//   etait en cours, la construction continuait en tache de fond CONCURREM
//   MENT a la lecture GIF active, exactement la contention SD/heap que la
//   restriction du prechauffage au mode config (RecalBox_DMD.ino) visait
//   deja a eviter. cacheBuilderStep() se met desormais en PAUSE complete
//   (aucun etat touche, juste return immediat) tant que g_sdOpInProgress
//   est faux -- reprend exactement ou elle en etait des que le DMD
//   repasse en mode config. PAS ENCORE teste sur materiel reel.
//
// v82 — 2026-07-27 — safe-modify — Question utilisateur : "j'ai scanne
//   Consoles et 3600 gifs, je rajoute 1 gif... rescan de 0 ou simple
//   ajout d'1 element ?" Reponse en verifiant le code : c'etait un rescan
//   complet -- handleWebConfigAddToPlaylistsBatch() supprimait
//   INCONDITIONNELLEMENT tout le .dmdcache existant puis relancait la
//   machine a etats, qui reconstruisait le dossier ENTIER depuis l'entree
//   0 (les 3600 fichiers deja connus reenumeres), meme pour un seul
//   fichier ajoute. Sur un dossier degrade FAT32 (des centaines de ms par
//   entree en profondeur, deja confirme), ca voulait dire plusieurs
//   minutes de scan reperdues pour chaque upload individuel.
//   Nouveau : si le cache du dossier est deja pret (V4, valide), les noms
//   nouvellement uploades (deja connus par cette fonction, recus du JS)
//   sont ajoutes directement a la fin du fichier existant (SD.open(...,
//   FILE_APPEND)) -- cout quasi nul, proportionnel au nombre de NOUVEAUX
//   fichiers seulement, jamais au contenu deja present. La suppression de
//   fichier reste une invalidation complete (cas moins frequent, retrouver
//   la position exacte d'un nom pour le retirer serait plus complexe).
//   PAS ENCORE teste sur materiel reel.
//
// v81 — 2026-07-27 — safe-modify — Question utilisateur : un cacheBuild
//   interrompu reprend ou est perdu ? Verification du code reel : les
//   dossiers DEJA termines (.dmdcache final ecrit) sont toujours preserves
//   (jamais reconstruits inutilement). Mais le dossier ACTIVEMENT en cours
//   de scan au moment d'une invalidation etait TOUJOURS perdu (redemarre a
//   zero), meme quand l'invalidation concernait un dossier totalement
//   different (upload/suppression/creation ailleurs) -- cacheBuilderStart()
//   abandonnait sans condition le scan en cours. Sur un gros dossier
//   (Consoles, Arcade...) deja en cours depuis plusieurs minutes, des
//   actions web frequentes sur d'autres dossiers pouvaient le faire
//   recommencer indefiniment sans jamais aboutir. cacheBuilderStart() ne
//   touche plus au dossier en cours de scan (CB_SCANNING) : rafraichit
//   seulement la liste des dossiers a traiter APRES lui. PAS ENCORE teste
//   sur materiel reel.
//
// v80 — 2026-07-27 — safe-modify — Question utilisateur : un dossier
//   ajoute manuellement (carte SD retiree/modifiee sur PC) est-il detecte
//   et mis en cache ? Reponse en verifiant le code reel : OUI si le
//   passage par MEDIA declenche le reboot cible (cas courant, playlist
//   active) -- cacheBuilderStart() est appelee au boot suivant. Mais faille
//   trouvee : si MEDIA est atteinte SANS ce reboot (playlist vide au boot,
//   tout premier demarrage, mode AP), la machine a etats ne demarrait
//   JAMAIS pour toute la session -- un dossier ajoute manuellement restait
//   bloque au sablier indefiniment. handleWebConfigMediaPage() appelle
//   desormais aussi cacheBuilderStart() directement (si IDLE/DONE
//   uniquement -- jamais en cours de scan, pour ne pas perdre la
//   progression d'un gros dossier deja en route). PAS ENCORE teste sur
//   materiel reel.
//
// v79 — 2026-07-27 — safe-modify — Demande utilisateur : separer la
//   gestion des PLAYLISTS (generer/supprimer) de la gestion PHYSIQUE des
//   fichiers/dossiers (ajout/suppression, reservee a MEDIA). BASIC avait
//   deja une section "Playlist" (playlist par defaut, lecture aleatoire,
//   suppression -- deletePlaylist()/fillPlaylists() deja presents) : y
//   ajoute la GENERATION (liste de dossiers a cocher -- noms uniquement,
//   PAS d'icone d'ouverture/consultation du contenu, reservee a MEDIA
//   selon precision utilisateur explicite -- + nom + bouton). Retire de
//   MEDIA : ligne "Nom playlist"+bouton "Generer playlist", et toute la
//   section "Supprimer une playlist" (redondante avec celle deja presente
//   sur BASIC) -- generatePlaylist()/deletePlaylist()/loadPlaylists()
//   supprimees de MEDIA, appel loadPlaylists() retire de la sequence
//   d'init. h1 MEDIA "Medias & Playlists" -> "Medias" (seul) ;
//   desc_dirs MEDIA mise a jour (ne mentionne plus generer une playlist).
//   nav_basic (barre de navigation, dupliquee sur les 4 pages) renomme
//   "Affichage" -> "Affichage & Playlists" (fr/en/es) pour refleter le
//   nouveau perimetre. Aucun handler C++ modifie -- /lsgifdirs,
//   /generate-playlist, /delete-playlist, /lsplaylists sont deja des
//   routes generiques reutilisables depuis n'importe quelle page :
//   changement HTML/JS pur. PAS ENCORE teste sur materiel reel.
//
// v78 — 2026-07-27 — safe-modify — Test reel v76/v77 (dossier Consoles) :
//   donnees chiffrees decisives -- cout par entree FAT32 monte de ~7,6ms
//   (debut) a ~438ms (entree 1480), TOUJOURS croissant (392s cumulees, pas
//   termine). Confirme : aucun decoupage (deja teste 15 puis 5
//   entrees/pas) ne peut plus compenser un cout PAR APPEL individuel
//   devenu si eleve -- un seul openNextFile() peut a lui seul bloquer
//   loop() plusieurs centaines de ms, non interruptible en cours de route.
//   Decision utilisateur : exclure automatiquement les dossiers trop
//   lents/gros du cache en tache de fond plutot que de continuer a
//   chercher un decoupage plus fin (deja demontre insuffisant).
//   Nouveau : CB_DIR_TIME_BUDGET_MS (60s) -- si la construction d'un
//   dossier depasse ce budget, cacheBuilderExcludeCurrentDir() abandonne
//   proprement (jette le .tmp partiel) et ecrit un marqueur "EXCLU" a la
//   place du cache normal (GIF_CACHE_EXCLUDED_MARKER) -- ne sera plus
//   jamais retente automatiquement (cacheBuilderAdvanceToNextDir() saute
//   desormais les dossiers READY ET EXCLUDED). gifFilesCacheStatus()
//   distingue les 3 etats (absent/pret/exclu) par lecture des seuls
//   premiers octets du fichier cache, sans jamais tout charger.
//   handleWebConfigListGifDirs() expose ce statut ("excluded":bool) ; JS
//   affiche une 3e icone dediee (avertissement, non cliquable) sur les
//   dossiers exclus, distincte du sablier (encore en attente) et du
//   dossier ouvrable (pret). Limite connue et acceptee : un dossier exclu
//   n'est plus navigable via cette page web (impossible de voir/gerer son
//   contenu depuis MEDIA) -- gestion de ce cas via retrait de la carte SD
//   sur PC, comme deja le cas pour d'autres operations lourdes sur ce
//   projet.
//   Ajoute aussi : priorite absolue a un client web connecte dans
//   cacheBuilderStep() (saute le pas de construction entierement si un
//   client est actif) -- ameliore la reactivite des AUTRES dossiers/pages
//   pendant qu'un gros dossier est en cours de construction/exclusion.
//   PAS ENCORE teste sur materiel reel.
//
// v77 — 2026-07-27 — safe-modify — Question utilisateur : le reboot cible
//   est-il encore necessaire ? Reponse en creusant le code reel : OUI pour
//   MEDIA (marge heap + exclusivite SD confirmees par les logs), mais le
//   code declenchait ce reboot sur TOUTES les pages de config (root/BASIC/
//   NETWORK/CLOCK/MEDIA), pas seulement MEDIA -- confirme problematique
//   par l'utilisateur : un simple chargement de la racine (voire une
//   autocompletion/prechargement du navigateur, qui peut emettre une
//   vraie requete HTTP sans navigation deliberee) suffisait a interrompre
//   une lecture en cours pour rebooter, alors que ces pages ne font aucun
//   scan SD et n'ont donc aucun besoin de cette marge.
//   triggerWebConfigMode() prend desormais un parametre allowReboot :
//   root/BASIC/NETWORK/CLOCK passent false (jamais de reboot, juste le
//   passage en mode config a l'ecran comme avant) ; seule MEDIA passe true
//   (seule page qui construit reellement le cache). handleDmdOpen()
//   (declenchement explicite, pas une simple navigation de page) garde
//   aussi true. PAS ENCORE teste sur materiel reel.
//
// v76 — 2026-07-27 — safe-modify — BRANCHE DEV : test reel de v75 --
//   "la page de config n'apparait pas ou avec bcp de difficulte". Le log
//   montre le meme message DMD "Consoles (4/18)" repete en boucle SANS
//   compteur d'entrees ni chronometrage (pas ajoutes dans cette nouvelle
//   machine a etats, contrairement a l'ancien scanGifFilesInRaw() qui les
//   avait) -- impossible de distinguer une vraie progression lente
//   (attendue : cout FAT32 deja confirme degradant fortement en
//   profondeur, jusqu'a ~244ms/entree en fin de dossier) d'un blocage
//   reel. Deux changements : (1) CB_MAX_ENTRIES_PER_STEP 15 -> 5, pour
//   reduire le pire cas de temps ou loop() ne peut pas rappeler
//   webServer->handleClient() (hypothese principale du symptome : un pas
//   de 15 entrees profondement degradees peut bloquer plusieurs secondes
//   d'affilee) ; (2) chronometrage reel par pas (compteur d'entrees +
//   duree du pas + duree cumulee depuis le debut du dossier) dans le log
//   Serial, meme principe que l'ancienne instrumentation. PAS ENCORE
//   reteste sur materiel reel.
//
// v75 — 2026-07-27 — safe-modify — BRANCHE DEV : le test reel de v74 a
//   montre un probleme non anticipe -- sans aucun cache SD, CHAQUE
//   navigation vers un dossier (meme un dossier deja visite, meme un
//   dossier different) repaye le cout RAM complet d'un scan live. Log reel
//   confirme : apres seulement 2-3 dossiers ouverts a la suite, le heap
//   s'effondre (maxalloc 12276 -> 5876 -> 5620) et les dossiers suivants
//   echouent en "heap critique" avec liste vide, meme sur des dossiers pas
//   particulierement gros. Nouvelle approche demandee par l'utilisateur :
//   reintroduire un cache PERSISTANT sur SD (comme avant v74) mais le
//   CONSTRUIRE en tache de fond, PAS dans le thread d'une requete HTTP --
//   discussion avec l'utilisateur, deux options : (a) vraie tache FreeRTOS
//   separee avec mutex partage pour proteger l'acces SD concurrent, (b)
//   machine a etats cooperative avancee depuis loop(), meme thread que le
//   serveur web, sans mutex. Choix retenu : (b), plus simple et sans
//   risque de race condition -- satisfait exactement le besoin "jamais
//   deux acces SD simultanes" par construction (un seul thread), sans
//   avoir besoin d'aucune synchronisation explicite.
//   Suppression de scanGifFilesInRaw() (devenue inutile, remplacee par la
//   machine a etats). Reintroduction de BufferedCacheWriter (retiree en
//   v74) et d'un format de cache SIMPLIFIE (GIF_CACHE_VERSION "V4") : plus
//   de comptage de controle pour verifier la fraicheur -- la validite est
//   desormais "le fichier .dmdcache existe", et la fraicheur est garantie
//   par une invalidation EXPLICITE (suppression du fichier + relance de la
//   machine a etats) a chaque upload/suppression/creation de dossier,
//   plutot que par une re-verification couteuse (comptage) a chaque
//   lecture. Nouveau : handleWebConfigListGifDirs() indique desormais un
//   statut "cached" par dossier (demande utilisateur : icone dossier
//   ouvrable seulement si deja en cache, sablier inactif sinon, cote JS).
//   handleWebConfigListGifFiles() ne fait PLUS JAMAIS de scan live : lit le
//   cache s'il existe, renvoie "pas encore pret" sinon (statut HTTP 202).
//   PAS ENCORE teste sur materiel reel.
//
// v74 — 2026-07-27 — safe-modify — BRANCHE DEV (dev/cache-externalisation),
//   experimentation demandee par l'utilisateur suite a la confirmation
//   chiffree (log reel v73) que FAT32 est O(n^2) sur les gros dossiers
//   plats de ce materiel (Arcade, 1441 fichiers : ~352s de comptage +
//   ~352s de scan, cout par entree passant de ~10ms a ~244ms au fil du
//   parcours) -- la cause est le materiel/systeme de fichiers, pas le code
//   d'ecriture (buffer, etc, deja optimises sans effet suffisant).
//   Idee retenue : externaliser le SOUVENIR du resultat plutot que le scan
//   lui-meme (impossible a deporter, c'est l'ESP32 seul qui a acces
//   physique a la carte SD). Suppression COMPLETE du cache persistant sur
//   SD (.dmdcache, fichier .tmp, renommage, BufferedCacheWriter,
//   quickCountGifSubdirs()/quickCountGifFilesIn(), readListCacheFile(),
//   invalidateGifDirsCache()/invalidateGifFilesCache(), warmUpGifCaches())
//   -- toute cette mecanique existait pour rendre un second acces rapide,
//   mais construisait/entretenait un etat sur la carte SD qui a ete la
//   source de la quasi-totalite des bugs et lenteurs des dernieres
//   iterations (v54 a v73). Remplacee par scanGifDirsRaw()/
//   scanGifFilesInRaw() : un scan direct, UNE SEULE fois par requete (plus
//   de double-passage comptage+scan), dont le resultat est envoye tel
//   quel et jamais persiste sur la carte SD. C'est desormais le
//   NAVIGATEUR qui retient (sessionStorage, cote JS) qu'il a deja recu la
//   liste d'un dossier donne pour ne plus la redemander pendant la
//   session -- memoire quasi illimitee cote PC/telephone, contrairement
//   au heap ESP32. Consequence directe : plus de prechauffage bloquant de
//   18 dossiers au boot (supprime de RecalBox_DMD.ino) -- chaque dossier
//   n'est plus scanne qu'a la demande, une seule fois par session
//   navigateur. Le cout du premier scan d'un GROS dossier (les fameuses
//   ~12 minutes sur Arcade) N'EST PAS RESOLU par ce changement (le scan
//   FAT32 lui-meme est intact) -- seul le double-scan et la complexite/
//   fragilite du cache SD disparaissent. sessionStorage explicitement
//   invalide cote JS apres toute operation qui modifie le contenu reel
//   (upload, suppression fichier/dossier, creation dossier). A tester en
//   conditions reelles : (1) confirmer que la navigation repetee dans un
//   MEME dossier pendant une session ne re-declenche plus de scan SD ;
//   (2) confirmer que upload/suppression rafraichissent bien la liste
//   affichee sans necessiter un F5 ; (3) mesurer si le temps du PREMIER
//   scan d'un gros dossier a diminue (attendu : environ moitie moins,
//   gain du double-scan supprime, PAS un fix complet).
//
// v73 — 2026-07-27 — safe-modify — Retour utilisateur sur v72 : toujours
//   aucun indicateur d'activite ni log de chronometrage visible -- "le log
//   semble bloque". Ca change le diagnostic : v72 n'instrumentait que la
//   boucle de scan PRINCIPALE (dans ensureGifFilesCache()), pas
//   quickCountGifFilesIn()/quickCountGifSubdirs() -- appelees AVANT, pour
//   connaitre le compte attendu (verification cache + en-tete), qui font
//   leur PROPRE enumeration complete du dossier, entierement silencieuse.
//   Si l'utilisateur ne voit jamais rien, il est probable que le blocage
//   ait lieu DANS cette phase de comptage, jamais atteinte par les logs
//   de v72. Ajout d'un message DMD immediat des l'entree dans
//   quickCountGifFilesIn() (avant meme le premier openNextFile()) + log de
//   progression/chronometrage toutes les 30 entrees, meme principe que
//   quickCountGifSubdirs(). Si le prochain test ne montre TOUJOURS aucun
//   message, meme pas celui-ci, ca voudra dire que le blocage a lieu
//   encore plus tot (avant l'appel a quickCountGifFilesIn() lui-meme,
//   possible probleme materiel/SPI plutot qu'un cout cumulatif par
//   fichier). Compilation verifiee OK. PAS ENCORE reteste.
//
// v72 — 2026-07-27 — safe-modify — Test reel du buffer d'ecriture (v70) :
//   toujours ECHEC -- 5 minutes sur le dossier Arcade, pas termine. Le
//   buffer d'ecriture n'etait donc pas (ou pas seulement) la cause.
//   Hypothese retenue, appuyee sur un fait deja documente sur ce projet
//   (RecalBox_DMD.ino v3, 2026-06-24) : FAT32 est intrinsequement lent a
//   enumerer au-dela de ~800 fichiers dans un meme dossier physique sur ce
//   materiel -- deja rencontre et contourne a l'epoque pour la LECTURE de
//   GIF (sous-dossiers alphabetiques A..Z/#), jamais applique a
//   l'enumeration faite ici pour le cache. Si "Arcade" est un dossier
//   plat avec des centaines de fichiers, l'enumeration elle-meme
//   (openNextFile()/close() par entree) est probablement le vrai goulot,
//   pas l'ecriture. En attendant de confirmer par log reel : (1) retire un
//   appel redondant a quickCountGifFilesIn()/quickCountGifSubdirs()
//   (enumeration complete du dossier appelee 2 fois avant meme le scan
//   principal -- verification cache + en-tete -- desormais calculee une
//   seule fois et reutilisee) ; (2) ajoute un indicateur d'activite sur le
//   DMD toutes les 30 entrees (webDmdPause() avec compteur croissant --
//   demande utilisateur, le DMD semblait fige/plante sans aucun retour
//   pendant un scan long) ; (3) ajoute un log de chronometrage reel
//   (millis() toutes les 30 entrees + debut/fin) pour confirmer ou
//   infirmer l'hypothese FAT32 avec des chiffres plutot qu'une nouvelle
//   supposition. Compilation verifiee OK. PAS ENCORE reteste -- si le log
//   confirme un cout lineaire par entree incompressible (FAT32), il
//   faudra reconsiderer l'organisation physique des fichiers (sous-
//   dossiers alphabetiques, comme deja fait pour la lecture) plutot que
//   d'optimiser encore le code de scan lui-meme.
//
// v71 — 2026-07-27 — safe-modify — Demande utilisateur : mise a jour des
//   playlists apres upload (handleWebConfigAddToPlaylistsBatch()) signalee
//   lente et silencieuse. Meme cause que le fix precedent (v70) mais cote
//   lecture cette fois : la phase "quelles playlists referencent ce
//   dossier" relisait CHAQUE playlist ligne par ligne
//   (readStringUntil('\n') + delay(1) PAR LIGNE) -- tres lent des qu'une
//   playlist contient beaucoup d'entrees. Remplace par une lecture
//   bufferisee complete (readString(), deja utilisee plus bas dans la
//   meme fonction pour la verification de doublons) + un seul indexOf().
//   Nettoyage en meme temps : addFileToPlaylists() et
//   handleWebConfigAddToPlaylists() (route /add-to-playlists singulier)
//   confirmees mortes (plus aucun appelant JS depuis le passage au
//   traitement par lot, v41) -- supprimees plutot que de corriger un bug
//   dans du code inutilise. Silence corrige aussi : nouveau
//   msg_updating_playlists (fr/en/es) affiche cote page ET mirrorte sur le
//   DMD (/dmd-pause) avant l'appel a /add-to-playlists-batch, qui pouvait
//   rester invisible pendant toute cette phase. Compilation verifiee OK.
//   PAS ENCORE reteste.
//
// v70 — 2026-07-27 — safe-modify — Test reel du streaming direct (v67) :
//   ECHEC -- fonctionnel mais beaucoup trop lent (12 minutes signalees sur
//   le dossier Arcade, 1/18). Cause : un out.print() separe par nom de
//   fichier (des centaines d'ecritures individuelles sur la carte SD pour
//   un gros dossier, chacune avec sa propre latence physique) est
//   dramatiquement plus lent qu'une seule grosse ecriture. Fix : nouvelle
//   BufferedCacheWriter (buffer FIXE de 256 octets, jamais plus, donc
//   toujours pas de cout RAM proportionnel au contenu -- l'acquis de v67
//   est preserve) qui accumule plusieurs noms avant de les ecrire en un
//   seul bloc sur la carte SD -- meilleur des deux mondes : heap constant
//   ET peu d'ecritures physiques. Utilisee par ensureGifDirsCache() ET
//   ensureGifFilesCache(). Compilation verifiee OK. PAS ENCORE reteste --
//   objectif : les gros dossiers doivent se construire en quelques
//   secondes, pas en minutes.
//
// v69 — 2026-07-27 — safe-modify — Demande utilisateur : message de la page
//   d'attente pendant le reboot cible (sendRebootingPage()) mentionne
//   desormais explicitement le demarrage du DMD ("Redemarrage en cours,
//   demarrage du DMD, veuillez patienter...") -- le prechauffage complet
//   des caches (v30) peut ajouter plusieurs secondes a l'attente, sans
//   cette precision le delai pouvait sembler anormalement long. fr/en/es
//   mis a jour. Compilation verifiee OK. PAS ENCORE reteste.
//
// v68 — 2026-07-27 — safe-modify — Demande utilisateur : warmUpGifCaches()
//   affiche desormais "nom_dossier (i/total)" sur la ligne 2 de l'ecran
//   DMD pendant le scan de chaque dossier, via webDmdPause() (mecanisme
//   deja existant de dessin direct, immediat, sans attendre loop() -- voir
//   RecalBox_DMD.ino meme date pour la ligne 1, dessinee une seule fois
//   avant l'appel). Compilation verifiee OK. PAS ENCORE reteste.
//
// v67 — 2026-07-27 — safe-modify — Demande explicite utilisateur ("soit on
//   parvient a gerer les gros dossiers, soit on abandonne -- 80% equivaut
//   a non fonctionnel") : test reel du prechauffage (v64) confirmait 13/18
//   dossiers en cache avec succes, mais 5 dossiers (probablement les plus
//   gros : Arcade, Consoles, Halloween, Other, Pinball_Short) echouaient
//   encore MEME au prechauffage (heap au maximum, ~22 Ko). Cause : le
//   scan accumulait toujours un buffer `built` proportionnel au nombre de
//   fichiers avant d'ecrire le cache -- pour un dossier de plusieurs
//   centaines de GIFs, ce buffer seul suffit a depasser le budget heap,
//   independamment de toutes les optimisations precedentes (tri retire,
//   reserve reduit, chunked evite). Fix radical : ensureGifDirsCache()/
//   ensureGifFilesCache() ecrivent desormais CHAQUE nom directement sur la
//   carte SD (fichier .tmp) au fil du scan, sans jamais accumuler la liste
//   en RAM -- le seul cout RAM par entree redevient une String temporaire
//   (nom de fichier), liberee a chaque iteration, quelle que soit la
//   taille du dossier. Le fichier .tmp n'est renomme vers le chemin final
//   (SD.rename(), deja utilise ailleurs sur ce projet pour le contournement
//   FAT32 lecture-seule) qu'en cas de succes complet -- jamais de cache
//   partiel en place si le scan est interrompu. writeListCacheFile()
//   devenue morte, supprimee. La lecture du cache (readListCacheFile(),
//   servant les requetes une fois le cache construit) est inchangee --
//   deja confirmee fonctionner meme sur de gros caches (lecture bufferisee
//   en un seul bloc, pas de cout par entree). Compilation verifiee OK.
//   PAS ENCORE reteste sur materiel reel -- objectif : les 5 dossiers en
//   echec doivent desormais reussir, meme au prechauffage.
//
// v66 — 2026-07-27 — safe-modify — Suite discussion utilisateur sur le
//   cout heap des multiples triggerWebConfigMode() par page (~1 Ko/appel
//   mesure en conditions reelles, cf changelog v57/v60/v61). Proposition
//   initiale (page d'accueil comme point d'entree strict, sous-pages sans
//   triggerWebConfigMode()) ecartee : casserait la navigation directe
//   entre BASIC/NETWORK/CLOCK/MEDIA (barre de nav existante, demande
//   explicite anterieure) et un F5/favori sur une sous-page -- un
//   WebServer ESP32 est fondamentalement sans etat, "interdire" l'acces
//   direct demanderait un vrai suivi de session, fragile. Alternative plus
//   simple retenue : triggerWebConfigMode() court-circuite desormais
//   webDmdPause()/webDmdSetMainMsg() si g_sdOpInProgress est deja vrai (le
//   DMD est deja dans l'etat vise -- meme IP, meme message -- reappliquer
//   les memes valeurs ne changerait rien a l'affichage). Meme economie
//   heap que la proposition initiale, sans toucher a l'architecture de
//   navigation : chaque page continue d'appeler triggerWebConfigMode()
//   normalement, donc navigation directe/rafraichissement restent
//   garantis fonctionnels. Compilation verifiee OK. PAS ENCORE reteste.
//
// v65 — 2026-07-27 — safe-modify — Demande utilisateur, voir RecalBox_DMD.ino
//   meme date : triggerWebConfigMode() pose desormais g_sdOpPersistentSubMsg
//   (= IP du DMD) en plus de l'appel webDmdPause() habituel -- c'est ce
//   message "de fond" que l'ecran physique reaffiche automatiquement apres
//   l'expiration d'un message de statut transitoire (5s sans mise a jour).
//   Compilation verifiee OK. PAS ENCORE reteste.
//
// v64 — 2026-07-27 — safe-modify — Demande utilisateur : plutot que de
//   subir le cout du scan/cache dilue sur plusieurs requetes web (heap
//   deja entame par WiFi/serveur/navigations), construire TOUS les caches
//   SD (dossiers + contenu de chaque dossier) en une seule fois, juste
//   apres le reboot cible mode config, quand le heap est proche de son
//   maximum. Logique de scan+ecriture cache factorisee depuis
//   handleWebConfigListGifDirs()/ListGifFiles() dans 2 nouvelles fonctions
//   partagees : ensureGifDirsCache()/ensureGifFilesCache(dirName) --
//   verifient si le cache existant est deja valide (comptage de controle)
//   et ne rescannent que si necessaire (absent ou perime). Nouvelle
//   warmUpGifCaches() (appelee une fois depuis RecalBox_DMD.ino, voir meme
//   date) : assure le cache des dossiers, puis celui de CHAQUE sous-
//   dossier trouve. Une fois tous les caches valides, les handlers HTTP
//   (desormais tres simplifies : ensureXxxCache() + lecture + envoi) ne
//   font plus qu'une lecture rapide tant que le contenu de la carte SD ne
//   change pas -- "construction" une seule fois au reboot, puis simple
//   actualisation a la demande si un dossier est modifie entre-temps.
//   Compilation verifiee OK. PAS ENCORE reteste sur materiel reel -- le
//   prechauffage complet peut prendre plusieurs secondes selon le nombre
//   de dossiers/fichiers a scanner la premiere fois (deja attendu par
//   l'utilisateur pendant l'ecran "Redemarrage en cours").
//
// v63 — 2026-07-27 — safe-modify — Suite du fix v62 (liste des dossiers
//   desormais instantanee, confirme par l'utilisateur) : "aucun fichier"
//   affiche pour TOUS les dossiers sauf un (deja re-scanne pendant les
//   tests recents). Diagnostic : cette carte SD a traverse de nombreux
//   tests cette session avec des versions du firmware anterieures au fix
//   v54 (qui interdit d'ecrire un cache sur un scan interrompu par manque
//   de heap) -- des .dmdcache invalides/vides ecrits par ces anciens
//   scans avortes restent probablement present sur la carte, et passent
//   la verification de comptage par coincidence, donc resservis tels
//   quels indefiniment. Fix : GIF_CACHE_VERSION "V2"->"V3", force le rejet
//   et la reconstruction automatique de tous les .dmdcache existants au
//   prochain acces (meme mecanisme deja utilise en v49 pour un probleme
//   similaire). Compilation verifiee OK. PAS ENCORE reteste.
//
// v62 — 2026-07-27 — safe-modify — Test A/B reel decisif (meme materiel,
//   meme carte SD) confirme l'encodage chunke comme cause principale du
//   plantage heap : l'ancienne version RecalBox_DMDv9_preclockv2 (instrumentee
//   avec les memes logs heap, jamais chunkee) liste les ~18 memes dossiers
//   sans AUCUNE chute notable (maxalloc parfaitement stable a 14836 sur 15
//   entrees, -512 sur les 3 dernieres, remonte avant l'envoi) -- alors que
//   la version actuelle perdait ~9 Ko sur le meme scan. Nouveau
//   sendJsonArrayFromCommaList() (helper partage par
//   handleWebConfigListGifDirs()/ListGifFiles()) : sous SIMPLE_SEND_MAX_LEN
//   (4096 octets), un seul webServer->send() avec le JSON complet deja
//   construit -- exactement la methode de l'ancienne version. Au-dessus,
//   repli sur l'encodage chunke deja en place (necessaire pour les tres
//   gros dossiers, cf. ERR_CONTENT_LENGTH_MISMATCH confirme le 2026-07-25 --
//   ne pas retirer entierement le chunke, juste eviter de le payer pour les
//   petites/moyennes listes qui sont le cas courant). Le webServer->send()
//   qui demarrait le mode chunke AVANT le scan (donc avant meme de savoir
//   quelle taille aura la reponse) est retire des 2 handlers -- l'envoi
//   n'a plus lieu qu'une fois cachedNames connu, permettant de choisir le
//   bon mode. Logs de diagnostic devenus obsoletes retires (apres
//   reserve(512), apres SD.open(dossier), apres send chunked init) --
//   remplaces par le nouveau mecanisme lui-meme. Compilation verifiee OK.
//   PAS ENCORE reteste sur materiel reel.
//
// v61 — 2026-07-27 — safe-modify — Log reel v60 : maxalloc stable sur 13
//   entrees consecutives (Logo_Rpi2dmd a RB_intros, 9204 constant) --
//   ecarte definitivement l'hypothese "cout par ouverture de File" (aurait
//   du decroitre a chaque entree). Les 2 vraies chutes sont concentrees a
//   la toute 1ere entree (13812->8692, juste apres webServer->send(200,
//   "application/json","") qui demarre le mode chunke) et apres la
//   derniere entree listee (9204->5108, avant l'abandon). Nouvelle
//   analyse comparative (2e agent, meme methode) confirme via le
//   changelog du fichier lui-meme (v56 : perte identique -7168 sur un
//   dossier quasi vide "ecriture_test" ET sur ~10 dossiers) que la perte
//   est un COUT FIXE PAR REQUETE, pas proportionnel au nombre d'entrees --
//   incompatible avec un cout "par fichier/dossier". L'ancienne version
//   (RecalBox_DMDv9_preclockv2) n'utilise JAMAIS l'encodage chunke
//   (setContentLength(CONTENT_LENGTH_UNKNOWN)+sendContent()) nulle part
//   dans tout web_config.h -- c'est le seul mecanisme present UNIQUEMENT
//   dans les 2 handlers qui plantent (lsgifdirs/lsgiffiles) et absent de
//   tous les autres. Forte correlation, mecanisme exact non prouve dans le
//   code source de la lib WebServer 3.3.11 (send()/sendContent() ne
//   semblent allouer que de petites String d'en-tete). Ajout d'un
//   checkpoint heap juste apres le webServer->send(200,...,"") qui
//   demarre le mode chunke (avant tout scan SD) dans les 2 handlers, pour
//   isoler precisement ce cout de celui du scan qui suit. Compilation
//   verifiee OK. PAS ENCORE reteste.
//
// v60 — 2026-07-27 — safe-modify — Test reel du retrait du tri (v59) :
//   INSUFFISANT -- lsgifdirs plante encore exactement pareil (maxalloc
//   13812->4596) alors que /gifs ne contient qu'une petite dizaine de
//   sous-dossiers. Le tri n'etait donc pas la seule cause pour CE scan
//   precis (contrairement au scan de fichiers qui peut avoir des centaines
//   d'entrees, le scan de dossiers en a tres peu). Nouvelle piste : le
//   cycle SD.open()/openNextFile()/close() lui-meme pourrait couter du
//   heap par ouverture (buffer interne de la classe File, jamais
//   totalement exclu lors de l'investigation de la fuite GIF plus tot
//   cette session). Ajout d'un log heap (maxalloc) a CHAQUE entree de
//   dossier trouvee dans la boucle de handleWebConfigListGifDirs() --
//   nombre d'entrees faible, pas de risque de spam -- pour voir si le cout
//   est reparti uniformement par entree (confirmerait le cout par File)
//   ou concentre sur une seule. Egalement : commentaire obsolete
//   mentionnant le tri (retire en v59) corrige. Compilation verifiee OK.
//   PAS ENCORE reteste.
//
// v59 — 2026-07-27 — safe-modify — Demande explicite utilisateur : retire
//   integralement le tri alphabetique cote serveur (sortBuiltEntries()/
//   compareBuiltEntries(), ajoutees v47/v58) pour retrouver un listing de
//   dossiers/fichiers FONCTIONNEL en priorite -- meme apres optimisation
//   du tri (v58, tableau d'int au lieu de String), le scan echouait
//   encore en conditions reelles (heap trop bas au moment du scan, voir
//   memoire projet pour le detail des investigations heap de cette
//   session). Comparaison avec une tres ancienne version du firmware
//   (RecalBox_DMDv9_preclockv2, listing instantane sur le meme materiel)
//   a confirme que cette ancienne version n'a JAMAIS eu de tri, ni serveur
//   ni client -- juste un scan+envoi simple. Les dossiers/fichiers
//   s'affichent donc de nouveau dans l'ordre FAT (ordre de creation SD),
//   pas alphabetique -- regression assumee temporairement, une piste de
//   tri cote client (JavaScript, RAM du PC/telephone plutot que heap
//   ESP32) sera explorees separement sur une branche de developpement
//   avant toute reimplementation. Le reste de la mecanique (cache SD
//   persistant, envoi chunke, garde-fou heap critique) est inchange.
//   Compilation verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v58 — 2026-07-27 — safe-modify — Demande utilisateur : reduire le cout
//   heap du tri alphabetique lui-meme (v47), plutot que de continuer a
//   contourner via le reboot cible. Nouvelles sortBuiltEntries()/
//   compareBuiltEntries() : remplacent le tri sur un tableau de String
//   (String *names = new String[realCount] -- chaque entree dupliquait le
//   nom ET l'objet String lui-meme, ~24-28 octets de surcout par entree
//   rien que pour le conteneur, sans compter le contenu) par un tri par
//   insertion sur un tableau d'int (positions dans `built`, 4 octets/entree,
//   AUCUNE String temporaire creee pendant les comparaisons -- pure
//   arithmetique de pointeurs). Applique a handleWebConfigListGifDirs() ET
//   handleWebConfigListGifFiles(). Egalement : built.reserve() reduit de
//   4096 a 512 dans les 2 fonctions -- un log reel (meme date) montrait un
//   cout FIXE identique (~7168 octets) sur un tres petit dossier de test
//   ET un gros dossier, suggerant que ce reserve() upfront etait lui-meme
//   une part significative du cout, independamment du contenu reel scanne.
//   Compilation verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v57 — 2026-07-27 — safe-modify — Suite v56 : test reel montre que c'est
//   cette fois lsgifdirs (liste des dossiers) qui echoue, pas lsgiffiles
//   (contenu d'un dossier) -- variance de session en session, le heap
//   disponible au moment du scan n'est visiblement pas garanti stable
//   meme avec le reboot cible (v25/v26 RecalBox_DMD.ino). Ajout d'un point
//   de mesure dans triggerWebConfigMode() (chemin normal, pas de reboot)
//   pour voir le cout heap de CHAQUE chargement de page (le log montre 3
//   paires DMD setMainMsg/pause avant le scan, probablement MENU->MEDIA
//   ou rechargements) -- a comparer avec les nouveaux points de mesure
//   RecalBox_DMD.ino (meme date, apres WiFi/apres NTP) pour savoir si la
//   perte de ~35 Ko (49140->13812) vient surtout du WiFi ou des
//   navigations de page. Compilation verifiee OK. PAS ENCORE reteste.
//
// v56 — 2026-07-27 — safe-modify — Test reel confirme : le reboot cible
//   mode config (v55) fonctionne (lsgifdirs part de maxalloc=18420, plus
//   d'abandon premature) -- mais l'ouverture d'un dossier pour voir son
//   contenu (lsgiffiles) echoue systematiquement, meme sur un petit
//   dossier de test ("ecriture_test"). Log reel troublant : la perte
//   maxalloc du scan dossiers (18420->11252, soit -7168) et celle du scan
//   fichiers sur ecriture_test (11764->4596, soit -7168 aussi) sont
//   EXACTEMENT identiques, alors qu'un petit dossier de test ne devrait
//   quasi rien couter si le cout etait proportionnel au nombre de
//   fichiers. Suspicion : cout FIXE (pas proportionnel au contenu),
//   candidat n°1 = built.reserve(4096) appele inconditionnellement avant
//   meme de savoir combien de fichiers existent. Ajout de 2 points de
//   mesure dans handleWebConfigListGifFiles() : juste apres
//   built.reserve(4096), et juste apres SD.open() du dossier -- pour
//   isoler si le cout vient de la reservation String ou de l'ouverture du
//   handle SD lui-meme (piste alternative : classe File/FS ESP32, deja
//   suspectee lors de l'investigation de la fuite GIF). Compilation
//   verifiee OK. PAS ENCORE reteste (log a fournir au prochain essai).
//
// v55 — 2026-07-27 — safe-modify — Demande utilisateur, suite investigation
//   heap critique (log reel : maxalloc passe de 49140 juste apres boot a
//   13300 juste avant l'ouverture de la config web, a cause de la playlist
//   + plusieurs GIFs ouverts avant meme que l'utilisateur n'accede a la
//   page -- chaque GIF perd durablement quelques Ko, jamais recupere avant
//   reboot, cf RecalBox_DMD.ino v24) : triggerWebConfigMode() retourne
//   desormais un bool. Si g_playlistStartedThisBoot (RecalBox_DMD.ino) est
//   deja vrai (playlist/GIF deja lances ce boot), au lieu d'entrer en mode
//   config avec un heap deja entame, on ecrit force_config_boot=1 dans
//   config.ini, on envoie une page "Redemarrage en cours, veuillez
//   patienter..." (JS poll fetch+catch, pas de <meta refresh> qui
//   tomberait sur une erreur navigateur pendant la fenetre de reboot), puis
//   requestReboot=true. Le prochain boot saute directement la playlist
//   (g_skipPlaylistForConfig, voir .ino) et repart avec le maximum de heap
//   disponible (~49 Ko au lieu de ~13 Ko). Si g_playlistStartedThisBoot est
//   deja faux (AP/premier boot/secours WiFi, ou ce reboot cible lui-meme),
//   comportement inchange (pas de reboot supplementaire, deja au maximum).
//   5 handlers de page (Root/Basic/Network/Clock/Media) + handleDmdOpen()
//   mis a jour pour ne pas envoyer leur page normale si un reboot vient
//   d'etre declenche (return si triggerWebConfigMode() renvoie false).
//   Compilation verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v54 — 2026-07-26 — safe-modify — Bug remonte : la liste des sous-dossiers
//   /gifs n'affichait plus qu'un seul dossier ("arcade"). Cause trouvee par
//   lecture du code (pas encore confirmee par log reel) : quand le scan SD
//   de handleWebConfigListGifDirs()/handleWebConfigListGifFiles() s'arrete
//   prematurement (garde heap critique < 6000, deja en place), le code
//   envoyait quand meme au client la liste PARTIELLE accumulee jusque-la
//   (`built`) comme si elle etait complete -- aucune verification sur
//   `realCount==-1`/`aborted` avant l'envoi. Le client affichait donc
//   uniquement les quelques dossiers vus avant l'abandon (le premier
//   scanne dans l'ordre FAT, pas alphabetique puisque le tri n'a jamais
//   lieu si le scan est interrompu), donnant l'impression que les autres
//   dossiers avaient disparu. Fix : sur abandon, `cachedNames` est mis a
//   vide (liste vide envoyee) plutot que la liste partielle -- le client
//   ne voit plus une fausse liste complete, quitte a devoir reessayer. Ne
//   corrige pas la cause du heap critique lui-meme (fragmentation
//   accumulee sur ce projet, deja documentee) -- seulement la consequence
//   trompeuse cote client. Compilation verifiee OK. PAS ENCORE reteste sur
//   materiel reel (a confirmer via le log Serial "heap critique, arret
//   premature" au moment du prochain repro).
//
// v53 — 2026-07-26 — safe-modify — Bug confirme en test reel des le premier
//   essai du v52 : un simple rafraichissement de page (F5) declenchait un
//   reboot du DMD. Cause : un F5 emet le MEME evenement `pagehide` qu'une
//   fermeture d'onglet reelle -- il n'existe pas de moyen fiable cote
//   navigateur de distinguer les deux (la nav interne entre pages de config
//   etait bien geree via le flag sessionStorage, mais pas ce cas). Le
//   mecanisme v52 (pagehide -> sendBeacon('/reboot')) est retire integralement
//   des 4 pages BASIC/NETWORK/CLOCK/MEDIA (doReboot()/dmdResume() et le
//   script de detection nav/pagehide) -- retour a l'etat v51. Alternative a
//   envisager si le besoin reste reel : timeout d'inactivite cote SERVEUR
//   (ESP32 suit lui-meme le dernier appel HTTP recu pendant g_sdOpInProgress,
//   et se resume/reboot tout seul apres N minutes sans AUCUNE requete -- un
//   F5 renvoie immediatement une nouvelle requete donc ne serait jamais
//   confondu avec un abandon reel) -- pas implementee ici, a valider avec
//   l'utilisateur avant de s'y lancer (compromis duree du timeout vs sessions
//   longues legitimes sans interaction deja signalees sur ce projet).
//   Compilation verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v52 — 2026-07-26 — safe-modify — Demande utilisateur : supprimer le besoin
//   du "reboot MQTT depuis la Recalbox" comme unique moyen de debloquer un
//   DMD reste bloque en mode config (g_sdOpInProgress) apres une fermeture
//   d'onglet/navigateur sans avoir clique "Reprendre DMD" ni "Redemarrer".
//   Sur BASIC/NETWORK/CLOCK/MEDIA (les 4 pages qui appellent webDmdPause()
//   a l'ouverture) : un evenement pagehide envoie desormais
//   navigator.sendBeacon('/reboot') -- SAUF si sessionStorage
//   'dmd_skip_abandon_reboot' vaut '1', flag pose (a) au clic sur un lien de
//   la barre .topnav (navigation normale vers une autre page de config, pas
//   un abandon) ou (b) dans doReboot()/dmdResume() une fois l'action confirmee
//   et effective (sortie explicite et propre, pas besoin d'un reboot forcé
//   en plus). Le flag est efface au chargement de chaque page (evite qu'un
//   clic de nav sur la page precedente ne desactive la protection pour de
//   bon sur toute la session onglet). Reutilise directement la route /reboot
//   existante (deja HTTP_ANY cote WebServer, pas de nouvelle route
//   necessaire) : sendBeacon() est toujours en POST, /reboot repondait deja
//   a n'importe quelle methode. mqttTask() n'est pas touche ici : le garde
//   g_sdOpInProgress qui saute les tentatives de connexion MQTT pendant le
//   mode config reste actif (cf. investigation v22 -- il fonctionnait deja
//   comme prevu, le vrai declencheur du log "MQTT connecting" observe etait
//   la fenetre entre un resume reel et son propre re-pause errone, deja
//   corrige en v47). Compilation verifiee OK. PAS ENCORE reteste sur
//   materiel reel -- a valider particulierement : fermeture d'onglet pendant
//   config (doit rebooter), navigation entre pages BASIC<->NETWORK<->CLOCK
//   <->MEDIA (ne doit PAS rebooter), et mise en arriere-plan de l'onglet sur
//   mobile (a surveiller : un pagehide peut aussi se declencher en cas de
//   mise en veille de l'app, pas seulement une fermeture reelle -- si ca
//   provoque des reboots intempestifs sur mobile, il faudra restreindre le
//   declencheur, ex. ignorer pagehide quand event.persisted est true).
//
// v51 — 2026-07-26 — safe-modify — Incoherence trouvee sur la page AP :
//   contrairement aux 4 autres pages (BASIC/NETWORK/CLOCK/MEDIA, fix v48),
//   son showMsg() avait deja un setTimeout(...,5000) mais SANS le
//   clearTimeout/window._msgTimer associe -- un message qui en ecrase un
//   autre avant la fin des 5s pouvait donc se faire masquer prematurement
//   par le timer du precedent. Aligne sur le meme pattern que les 4 autres
//   pages. Compilation verifiee OK (AP_HTML 9060->9128 octets brut,
//   3250->3271 gzip). PAS ENCORE reteste sur materiel reel.
//
// v50 — 2026-07-26 — safe-modify — Demande explicite : suppression du code
//   mort WEB_CONFIG_HTML (ancienne page monopage remplacee par le
//   fractionnement en 6 pages du 2026-07-23, ~635 lignes, aucune route ne
//   la servait plus depuis cette date). Compilation verifiee OK -- taille
//   flash strictement identique (le compilateur l'excluait deja du
//   binaire), donc ce nettoyage n'ameliore que la lisibilite du fichier
//   source, pas l'empreinte memoire. Les mentions de "WEB_CONFIG_HTML,
//   code mort" dans les entrees de changelog anterieures restent en l'etat
//   (historique, pas modifiees).
//
// v49 — 2026-07-26 — safe-modify — 2 corrections suite aux retours :
//   1) Le tri alphabetique des dossiers (v47) etait invisible en test reel
//      car un fichier /gifs/.dmdcache ecrit AVANT ce fix restait considere
//      valide (comptage inchange) et continuait a servir l'ancien ordre
//      non trie indefiniment. Ajoute un prefixe de version au format du
//      cache (GIF_CACHE_VERSION="V2") : tout cache existant ecrit avant ce
//      fix est desormais automatiquement rejete et reconstruit (trie) des
//      le premier acces, sans intervention manuelle sur la carte SD.
//   2) Demande explicite : tutoiement remplace par du vouvoiement dans les
//      textes francais et espagnols des 6 pages (dict i18n + texte HTML de
//      repli) -- ex. "Choisis"->"Choisissez", "Coche"->"Cochez",
//      "Clique"->"Cliquez", "ton navigateur"->"votre navigateur",
//      "Selecciona"->"Seleccione", "Marca"->"Marque", "tu WiFi"->"su
//      WiFi", etc. Portee : uniquement les 6 pages actives (MENU/BASIC/
//      NETWORK/CLOCK/MEDIA/AP) ; l'ancien WEB_CONFIG_HTML mort n'a pas ete
//      touche (jamais servi).
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v48 — 2026-07-26 — safe-modify — Retour utilisateur : "affichages web/dmd
//   qui persistent alors que le process est termine". Deux points :
//   1) BUG CONFIRME : sur BASIC/NETWORK/CLOCK/MEDIA, showMsg() (et
//      showMsgLocal(), v47) n'avait AUCUN timeout d'auto-masquage -- le
//      popup web restait affiche indefiniment jusqu'au message suivant
//      (contrairement a la page AP, qui a toujours eu ce timeout de 5s).
//      Puisque showMsg() miroite chaque message sur le DMD via
//      /dmd-pause, le popup web ET le message DMD restaient donc
//      affiches indefiniment ensemble. Ajoute le meme setTimeout(5000)
//      que la page AP sur les 4 pages.
//   2) Demande explicite : tri alphabetique (v47, jusqu'ici limite a la
//      liste des dossiers) etendu a la liste des fichiers a l'interieur
//      d'un dossier ouvert (handleWebConfigListGifFiles()) -- meme
//      principe (accumulation avant envoi, tri par insertion, cache de la
//      liste deja triee).
//   Question clarifiee avec l'utilisateur : le DMD physique reste en mode
//   pause/config jusqu'a un clic EXPLICITE sur "Reprendre DMD" (protection
//   anti-coupure) -- confirme comme comportement voulu, pas un bug, aucun
//   changement apporte de ce cote.
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v47 — 2026-07-26 — safe-modify — 3 retours utilisateur :
//   1) BUG CONFIRME : cliquer "Reprendre DMD" affichait "DMD repris" en
//      miroir sur l'ecran physique... via /dmd-pause, qui remet justement
//      le DMD en mode pause/config -- annulant la reprise a peine
//      effectuee (ecran fige juste apres, non bloquant : un 2e clic sur
//      "Reprendre DMD" recupere puisqu'il repasse par webDmdResume()).
//      Confirme par le log reel : "[WEB] DMD resume..." puis "[GIF] open
//      OK..." (reprise reussie) suivi immediatement de "[WEB] DMD pause:
//      DMD repris" (la confirmation elle-meme repausait tout). Fix :
//      nouvelle fonction showMsgLocal() (popup web identique, SANS l'appel
//      /dmd-pause) utilisee uniquement par dmdResume() sur les 4 pages
//      concernees (BASIC/NETWORK/CLOCK/MEDIA) pour ce message precis --
//      tous les autres messages continuent d'etre miroites sur le DMD
//      normalement.
//   2) Demande explicite : liste des dossiers triee par ordre alphabetique
//      (les dossiers crees manuellement ou copies via l'outil Windows
//      apparaissaient dans l'ordre FAT -- ordre de creation, pas
//      alphabetique). handleWebConfigListGifDirs() accumule desormais tous
//      les noms avant d'envoyer quoi que ce soit (au lieu d'envoyer au fur
//      et a mesure du scan), trie par insertion (nombre de dossiers
//      generalement modeste), puis envoie et met en cache la liste deja
//      triee -- un cache-hit ulterieur reste donc trie sans retri. Portee
//      limitee a la liste des DOSSIERS (pas les fichiers dans un dossier,
//      non demande).
//   3) Demande explicite : avertissement ajoute dans la description de la
//      fonction d'envoi GIF (page MEDIA, section "Envoi GIF") -- pas concue
//      pour transferer de nombreux fichiers (debit lent, risque d'erreur
//      d'ecriture), reservee a l'ajout ponctuel de quelques fichiers ;
//      recommande de retirer la carte SD pour un transfert consequent.
//      Traduit fr/en/es.
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v46 — 2026-07-26 — safe-modify — CRASH CONFIRME en test reel : abort()
//   + reboot pendant un upload en masse, apres une sequence suppression
//   dossier + creation dossier + plusieurs listings + plusieurs retries
//   d'upload. Backtrace abort() typique d'une allocation heap qui echoue
//   avec les exceptions C++ desactivees (Arduino ESP32) -- meme classe de
//   crash deja documentee sur ce projet (piste /sync-playlists-check
//   abandonnee en 2026-07). Deux actions :
//   1) Reduction de la pression heap a la source : loadDirs() et
//      loadUploadDirs() (page MEDIA) appelaient chacun /lsgifdirs
//      independamment -- 2 scans SD + 2 parsings JSON pour la MEME donnee
//      a chaque chargement de page ou rafraichissement post-action.
//      Fusionnes : loadDirs() peuple maintenant aussi #uploadDir,
//      loadUploadDirs() devient un no-op conserve pour compatibilite des
//      appels existants (aucun autre site a modifier).
//   2) Refus explicite et propre plutot qu'un crash silencieux : verifie
//      ESP.getMaxAllocHeap() en debut de handleWebConfigCreateFolder() et
//      d'UPLOAD_FILE_START (handleWebConfigUploadFile()) -- sous 6000
//      octets de plus gros bloc allouable, renvoie une erreur claire
//      ("ERR: heap critique, reessayez") au lieu de continuer vers une
//      allocation qui echouerait. Le JS retente deja automatiquement
//      (jusqu'a 3x, cf. v41) : au pire un fichier echoue proprement avec
//      un message clair au lieu de faire rebooter tout le DMD.
//   Reste un point d'attention : ces deux fixes reduisent la frequence et
//   la gravite du probleme mais ne l'eliminent pas structurellement --
//   l'usage intensif de String Arduino dans les chemins chauds (listing,
//   playlists) reste une source de fragmentation sur un ESP32 a heap
//   limite. Une resolution complete demanderait probablement de remplacer
//   ces String par des buffers de taille fixe, hors de portee d'un
//   correctif ponctuel.
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v45 — 2026-07-26 — safe-modify — Preuve decisive en test reel : le log
//   serie montrait le cache SD lu des le premier essai ("(cache SD)" sur
//   CHAQUE appel, y compris le tout premier a un dossier) et pourtant
//   net::ERR_INVALID_CHUNKED_ENCODING persistait cote navigateur -- ce
//   n'etait donc ni la lenteur du scan ni un probleme de cache. Cause
//   racine reelle : au chargement de la page MEDIA, /lang + loadDirs() +
//   loadPlaylists() + loadUploadDirs() partaient TOUS en parallele (aucun
//   await entre eux) -- 4 requetes concurrentes sur un WebServer ESP32 qui
//   n'en traite qu'une a la fois. Si l'utilisateur cliquait sur un dossier
//   PENDANT ce lot initial, sa requete /lsgiffiles entrait en collision
//   avec l'une d'elles, corrompant la reponse (chunk invalide) -- meme
//   symptome que le bug d'upload corrige en v41 (meme classe de bug,
//   endroit different). Fix definitif plutot qu'un nouveau correctif au
//   cas par cas : ajout d'une file d'attente globale (queuedFetch(), voir
//   MEDIA) qui serialise STRICTEMENT toutes les requetes de la page,
//   quelle que soit la fonction qui les declenche -- tous les fetch() de
//   la page MEDIA (seule page a declencher plusieurs requetes concurrentes
//   au chargement) passent desormais par queuedFetch() au lieu de fetch()
//   directement. Les 5 autres pages sequencent deja naturellement leurs
//   appels initiaux (chaque fetch() suivant est appele DANS le .then() du
//   precedent) et n'ont jamais presente ce risque.
//   Egalement : la popup "Mise en cache..." (v41) est desormais aussi
//   miroitee sur l'ecran DMD (demande explicite), en attendant proprement
//   la fin du POST /dmd-pause avant de lancer /lsgiffiles (meme principe
//   que uploadGif()). Seuil de securite heap critique des scans de listing
//   (v42) bascule de ESP.getFreeHeap() vers ESP.getMaxAllocHeap() (plus
//   grand bloc contigu allouable) : le log reel montrait un total libre
//   encore correct (~12 Ko) alors que le plus gros bloc disponible etait
//   deja tombe a ~4,5 Ko -- le total libre seul sous-estimait le risque
//   reel d'echec d'allocation sur un tas fragmente.
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v44 — 2026-07-26 — safe-modify — Log serie reel decisif : les
//   esp_task_wdt_reset() ajoutes en v40 echouaient EN BOUCLE avec
//   "task not found" (la tache qui traite les requetes HTTP n'est en fait
//   pas enregistree aupres du Task Watchdog Timer) -- chaque appel rate
//   coute un print d'erreur ESP-IDF, explique une bonne partie du
//   ralentissement observe. RETIRES INTEGRALEMENT (create-folder, mkdir de
//   secours upload, UPLOAD_FILE_WRITE, boucles de scan des listings +
//   quickCount*()) -- n'apportaient aucun benefice confirme et un cout
//   reel. #include <esp_task_wdt.h> retire du .ino (v20).
//   Root cause du "ne trouve plus qu'un seul fichier apres une erreur
//   reseau" : le heap ne recupere JAMAIS entre plusieurs scans de gros
//   dossiers (19-20 Ko libres au debut de la session, jamais revu ensuite,
//   descend en escalier jusqu'a ~7 Ko puis le filet de securite v42 coupe
//   le scan de plus en plus tot -- d'ou l'impression de "un seul fichier").
//   Deux sources de fragmentation reduites :
//   1) readListCacheFile() et la lecture de playlist dans handleWebConfig
//      AddToPlaylistsBatch() concatenaient le contenu d'un fichier
//      OCTET PAR OCTET (`content += (char)f.read()`) -- chaque += peut
//      reallouer tout le buffer de la String, un vrai generateur de
//      fragmentation sur un fichier de plusieurs Ko. Remplace par
//      f.readString() (lecture bufferisee).
//   2) Les accumulateurs `built` (listes construites pendant un scan)
//      reservent maintenant 4096 octets d'un coup (built.reserve(4096))
//      au lieu de grandir par petits a-coups au fil des noms de fichiers.
//   Logs de diagnostic enrichis : heap libre ET maxalloc (ESP.getMaxAlloc
//   Heap(), plus representatif de la fragmentation reelle que le total
//   libre) sur chaque debut/fin de scan ; distingue desormais explicitement
//   "pas de cache", "cache perime" et "cache SD" dans les logs (l'utilisateur
//   se demandait si le cache etait vraiment relu -- ces logs le confirmeront
//   sans ambiguite au prochain test).
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel -- si le heap continue de ne pas recuperer entre les
//   scans malgre ces deux fixes, il faudra investiguer plus loin (peut-etre
//   ailleurs dans le firmware, hors de ce fichier).
//
// v43 — 2026-07-26 — safe-modify — Remarque justifiee de l'utilisateur sur
//   le cache v40/v41 : un cache RAM mono-slot n'a quasiment aucun interet
//   des qu'on navigue entre plusieurs dossiers dans une meme session
//   (chaque changement de dossier evince le precedent -- confirme dans le
//   log du 2026-07-26, XXX_Mature repassait en scan SD apres consultation
//   de RB_intros entre-temps). Demande explicite : garder le cache de TOUS
//   les dossiers deja parcourus, et le conserver entre les sessions (reboot
//   du DMD). Remplace par un cache PERSISTANT sur la carte SD : un petit
//   fichier cache par dossier (/gifs/<dossier>/.dmdcache et /gifs/.dmdcache
//   pour la liste des dossiers), format "N|nom1,nom2,..." ou N est un
//   comptage de controle. A chaque requete, un COMPTAGE RAPIDE (sans
//   construire ni echapper les noms, donc bien plus leger que la liste
//   complete) verifie que le nombre reel correspond toujours au nombre
//   enregistre ; sinon le cache est ignore et reconstruit -- couvre a la
//   fois nos propres modifications (upload/suppression, qui suppriment le
//   fichier cache concerne) ET des modifications faites hors du firmware
//   (carte SD modifiee depuis un PC entre deux sessions, ex. via l'outil
//   Windows -- scenario courant sur ce projet). Le comptage rapide n'est
//   fait que si un fichier cache existe deja (sinon scan direct). Limite
//   acceptee : un remplacement de fichier a nombre de fichiers inchange ne
//   serait pas detecte (compromis face au cout d'une verification exacte
//   par hash/mtime). g_dirCache*/g_fileCache* (globals RAM) supprimes,
//   remplaces par quickCountGifSubdirs()/quickCountGifFilesIn()/
//   readListCacheFile()/writeListCacheFile()/invalidateGifDirsCache()/
//   invalidateGifFilesCache(). Le fichier .dmdcache d'un dossier supprime
//   disparait avec lui (deleteFolderRecursive() est recursif sans filtre
//   sur les fichiers caches, aucune action supplementaire necessaire).
//   Compilation verifiee OK (62% flash, RAM legerement reduite -- plus de
//   String globales de cache fixes). PAS ENCORE reteste sur materiel reel.
//
// v42 — 2026-07-26 — safe-modify — net::ERR_INVALID_CHUNKED_ENCODING encore
//   signale en test reel sur un GROS dossier (XXX_Mature) au retour dessus
//   (cache mono-slot deja evince par la consultation d'un autre dossier
//   entre-temps -- donc re-scan SD, pas un chemin "cache" bugue). Analyse
//   demandee de la methode de l'ancienne page monopage (WEB_CONFIG_HTML,
//   code mort) : son JS (refreshGifDirs()/openFolder(), lignes ~915-955)
//   utilise EXACTEMENT le meme endpoint /lsgiffiles et la meme methode
//   (un seul fetch, construit toutes les lignes d'un coup) -- aucune
//   difference de methode cote frontend, et le handler C++ est PARTAGE
//   entre les deux versions (une seule implementation dans tout le
//   fichier). Il n'existe donc pas d'"ancienne methode plus rapide" a
//   restaurer : soit ce dossier precis n'a jamais ete teste avec l'ancienne
//   page, soit le cout est intrinseque a un tres gros dossier peu importe
//   la version de page. Piste retenue : le scan d'un gros dossier (String
//   par fichier, jsonEscape() char-par-char) fragmente le heap ; sur un
//   retour au meme dossier avec un heap deja plus bas (accumulation de
//   fragmentation entre plusieurs scans), l'ecriture pourrait echouer en
//   cours de route sans jamais atteindre le chunk final -- vu cote
//   navigateur comme un flux chunke invalide. Ajoute pour handleWebConfig
//   ListGifDirs() ET handleWebConfigListGifFiles() : esp_task_wdt_reset()
//   dans la boucle de scan (meme cadence que le delay(1) existant), filet
//   de securite qui interrompt proprement le scan (chunk final quand meme
//   envoye, resultat partiel non mis en cache) si le heap libre descend
//   sous 8000 octets plutot que de risquer un crash en cours de reponse.
//   jsonEscape() reserve desormais sa capacite de sortie (moins de
//   reallocations). Compilation verifiee OK (62% flash). PAS ENCORE
//   reteste sur materiel reel.
//
// v41 — 2026-07-26 — safe-modify — Audit de la copie de fichiers demande
//   par l'utilisateur, log serie reel analyse (nombreux "Upload aborted"
//   avant reussite, jusqu'a 3 echecs definitifs sur 15 fichiers) :
//   1) CAUSE PRINCIPALE trouvee : dans uploadGif() (page MEDIA), le POST
//      /dmd-pause (message de progression) et l'appel loadDirs()/
//      loadUploadDirs() apres /create-folder etaient envoyes SANS attendre
//      leur fin (fire-and-forget), juste avant de lancer /upload. Le
//      WebServer de l'ESP32 ne traite qu'une requete a la fois : ces
//      requetes concurrentes se faisaient concurrence pour la meme
//      connexion, provoquant des UPLOAD_FILE_ABORTED cote firmware --
//      confirme par le log serie montrant des "Upload aborted" intermittents
//      correles a ce pattern. Fix : chaque fetch (/dmd-pause, loadDirs(),
//      loadUploadDirs()) est desormais attendu (await) avant l'appel
//      suivant.
//   2) Les tentatives de retry (2/3, 3/3) n'etaient pas visibles (ni page
//      web ni DMD) -- ajoute : le texte de progression et le message
//      /dmd-pause affichent maintenant "nom_fichier (i/n) - tentative X/3"
//      a partir de la 2e tentative.
//   3) Demande explicite : la mise a jour des playlists lors d'un upload en
//      masse verifiait TOUTES les playlists concernees pour CHAQUE fichier
//      individuellement (cache g_plRefCache* deja en place pour "quelles
//      playlists referencent ce dossier", mais la verification "ce fichier
//      est-il deja present" relisait la playlist entiere a chaque fichier).
//      Nouvelle route POST /add-to-playlists-batch (handleWebConfigAdd
//      ToPlaylistsBatch()) : traite tous les fichiers d'un meme lot en un
//      seul appel, chaque playlist candidate n'est lue qu'UNE FOIS pour
//      determiner les fichiers manquants, puis tous ajoutes en un seul
//      SD.open(FILE_APPEND). L'appel par fichier (addFileToPlaylists()
//      dans UPLOAD_FILE_END) est retire ; le JS appelle desormais le lot
//      une seule fois a la fin de tout l'upload.
//   4) esp_task_wdt_reset() ajoute aussi dans UPLOAD_FILE_WRITE (defensif,
//      meme precaution que la v40 sur /create-folder).
//   5) Demande explicite : popup "Mise en cache du contenu, patientez..."
//      affiche des l'ouverture d'un dossier (openFolder()), le temps que
//      /lsgiffiles reponde -- utile en particulier au premier scan d'un
//      dossier (avant mise en cache par le fix v40). N'utilise PAS le
//      miroir DMD habituel de showMsg() (pas de fetch /dmd-pause
//      supplementaire ici) pour eviter de reintroduire une requete
//      concurrente juste avant le fetch /lsgiffiles.
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v40 — 2026-07-26 — safe-modify — 2 retours utilisateur supplementaires :
//   1) "Reboot du DMD a la premiere tentative de copie de fichier" -- piste
//      la plus probable : le Task Watchdog de loopTask (~5s par defaut sur
//      ce coeur ESP32) se declenche si une carte SD est lente sur son tout
//      premier mkdir()/ecriture (creation d'un dossier neuf jamais touche
//      depuis le formatage) -- explique "seulement au premier essai".
//      Ajoute esp_task_wdt_reset() (#include <esp_task_wdt.h> cote .ino)
//      entre chaque etape SD bloquante de handleWebConfigCreateFolder()
//      (mkdir, creation/suppression du fichier temoin) et sur le mkdir de
//      secours dans UPLOAD_FILE_START, pour eviter un reset meme si une
//      etape individuelle est plus lente que d'habitude.
//   2) Listing /gifs signale lent : cache RAM ajoute pour /lsgifdirs
//      (g_dirCacheNames, une seule liste pour tout /gifs) et /lsgiffiles
//      (g_fileCacheFolder/g_fileCacheNames, un seul dossier a la fois --
//      meme principe que le cache add-to-playlists) -- evite de rescanner
//      la carte SD tant que rien n'a change. Invalide automatiquement :
//      creation de dossier reussie (/create-folder), upload de fichier
//      reussi (UPLOAD_FILE_END), suppression de fichiers (/delete-files,
//      dossier concerne uniquement) et suppression de dossiers
//      (/delete-folders, invalide tout par securite vu qu'une liste de
//      dossiers peut etre affectee).
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v39 — 2026-07-26 — safe-modify — Durcissement preventif de /lsgifdirs et
//   /lsgiffiles suite a un test reel montrant le firmware pre-v38 (meme
//   ERR_INVALID_CHUNKED_ENCODING) suivi d'un blocage complet du reseau
//   (timeout sur /dmd-pause et /delete-folders juste apres) -- DMD reste
//   bloque en mode web config, reseau injoignable. Ajoute par precaution
//   avant le retest du fix v38 : timeout client elargi a 5s (au lieu du
//   defaut ~3s) pendant l'enumeration + logs Serial (heap libre avant/
//   apres) sur les deux routes, pour pouvoir diagnostiquer via le moniteur
//   serie si le blocage se reproduit malgre le fix v38 (auquel cas ce
//   serait un probleme distinct, ex. epuisement heap -- deja rencontre sur
//   ce projet, cf. plus bas dans ce fichier). Compilation verifiee OK
//   (63% flash). PAS ENCORE reteste sur materiel reel.
//
// v38 — 2026-07-26 — safe-modify — Regression introduite par le fix v36 :
//   net::ERR_INVALID_CHUNKED_ENCODING sur /lsgiffiles (et potentiellement
//   /lsgifdirs), y compris sur des dossiers qui fonctionnaient au test
//   precedent -- le passage en envoi chunke (setContentLength(CONTENT_
//   LENGTH_UNKNOWN) + sendContent()) n'etait jamais termine par le chunk
//   final de taille 0 (sendContent("") apres le dernier "]"), obligatoire
//   pour un flux "Transfer-Encoding: chunked" valide cote HTTP -- sans lui
//   le navigateur rejette la reponse entiere, meme un tout petit dossier.
//   Ajoute sur handleWebConfigListGifDirs() et handleWebConfigListGifFiles().
//   Compilation verifiee OK. PAS ENCORE reteste sur materiel reel.
//
// v37 — 2026-07-25 — safe-modify — Demande explicite utilisateur : remettre
//   la gestion multilingue + tous les messages web/DMD accompagnant les
//   actions, comme dans l'ancienne version monopage (WEB_CONFIG_HTML,
//   desormais code mort). Deux volets :
//   1) i18n complet (dict fr/en/es, data-i18n/data-i18n-placeholder,
//      tr()/applyLang()/setLang(), selecteur #langSelect en haut a droite,
//      priorite localStorage > config.ini (/lang) > navigator.language >
//      fr, persistance via /save-language) porte sur les 5 pages qui ne
//      l'avaient pas (MENU/BASIC/NETWORK/CLOCK/MEDIA -- seule la page AP
//      l'avait, depuis le 2026-07-23). Chaque page garde son propre dict,
//      limite aux cles qu'elle utilise (meme discipline de poids que la
//      page AP), traductions reprises de l'ancien dict I18N complet
//      (WEB_CONFIG_HTML) quand une cle equivalente existait. Tailles gzip
//      apres ajout : MENU 6554, BASIC 3880, NETWORK 3938, CLOCK 4460,
//      MEDIA 6555 octets -- toutes tres en-dessous du seuil ~12.5 Ko a
//      risque.
//   2) Miroir DMD des messages web : showMsg() sur les 5 pages + AP envoie
//      desormais systematiquement le message (succes ou erreur) sur
//      l'ecran physique via POST /dmd-pause (stripAccents() cote JS,
//      l'ecran LED ne gere pas les caracteres accentues) -- exactement le
//      comportement de l'ancienne page unique, perdu par les 6 nouvelles
//      pages lors du fractionnement. Les messages DMD "en provenance du
//      DMD" (passage en mode AP, secours WiFi, page de config au boot)
//      etaient deja localises via uiLanguage/config.ini et les 7 helpers
//      trOpenBrowserAt/trWifiRecoveryCountdown/trConnectWifiMsg/trOpenUrl/
//      trConfigPageMsg/trJoinWifi/trOpenInBrowser (RecalBox_DMD.ino) --
//      rien a refaire de ce cote, deja en place. Le message de bascule en
//      mode "webconfig" ("WEB DMD CONFIG", triggerWebConfigMode()) reste
//      volontairement identique dans les 3 langues, comme dans l'ancien
//      dict (msg_welcome).
//   Compilation verifiee OK (62% flash, 28% RAM). PAS ENCORE reteste sur
//   materiel reel.
//
// v36 — 2026-07-25 — safe-modify — 3 retours utilisateur supplementaires en
//   test reel (console F12 + inspection SD directe) :
//   1) net::ERR_CONTENT_LENGTH_MISMATCH sur /lsgiffiles pour des dossiers
//      avec beaucoup de fichiers ("ca marche sur des dossiers avec peu de
//      fichiers sinon erreur reseau ou tres tres long") -- handleWebConfig
//      ListGifDirs()/ListGifFiles() construisaient tout le JSON dans un
//      seul String puis un seul send() ; au-dela d'une certaine taille, le
//      nombre d'octets reellement transmis par WebServer::send() peut etre
//      inferieur au Content-Length annonce (calcule sur le String complet)
//      -> rejet cote navigateur alors que le status HTTP est 200. Passes en
//      envoi chunke (setContentLength(CONTENT_LENGTH_UNKNOWN) + sendContent()
//      par entree), qui n'annonce aucune longueur a l'avance. delay(1) par
//      fichier egalement retire (un delay(1) toutes les 20 entrees suffit a
//      eviter le watchdog) -- explique aussi la lenteur signalee.
//   2) Cache RAM add-to-playlists (g_plRefCacheFolder/g_plRefCachePlaylists,
//      documente valide le 2026-07-20) totalement absent du code actuel --
//      9e regression confirmee du fractionnement du 23/07. Sans lui, un
//      upload vers un dossier NON reference par une playlist relit
//      integralement TOUTES les playlists a CHAQUE fichier (~15s/fichier
//      mesure a l'epoque). Restaure a l'identique (cache par dossier,
//      invalide a la creation/suppression d'une playlist).
//   3) ERR_CONNECTION_RESET persistant sur /upload vers un nouveau dossier
//      malgre le timeout 15s de la v34 : le mkdir + fichier-temoin (contre
//      l'attribut lecture seule) restait dans UPLOAD_FILE_START, sur le
//      chemin critique du multipart. Decouple dans une nouvelle route
//      dediee POST /create-folder (idempotente), appelee par le JS AVANT
//      le premier fichier -- le dossier existe deja quand l'upload
//      multipart demarre vraiment, et la creation a son propre budget de
//      temps sans concurrencer la reception du fichier. Corrige aussi "le
//      dossier cree n'apparait pas dans la liste" : loadDirs()/
//      loadUploadDirs() sont maintenant rafraichis juste apres la creation
//      reussie, pas seulement apres le premier fichier uploade.
//   Compilation verifiee OK (62% flash). PAS ENCORE reteste sur materiel
//   reel.
//
// v35 — 2026-07-25 — safe-modify — Verification exhaustive des 6 pages
//   suite a une demande explicite de l'utilisateur, apres un nouveau crash
//   console F12 sur BASIC ("deletePlaylist is not a function"). Trouves et
//   corriges :
//   1) BUG REEL confirme (BASIC) : <select id="deletePlaylist"> ET
//      function deletePlaylist(){} portaient le meme nom -- "DOM
//      clobbering" classique : le navigateur expose automatiquement tout
//      element avec un id comme propriete globale de window, ecrasant la
//      fonction du meme nom. L'ancienne page (WEB_CONFIG_HTML, code mort)
//      utilisait deja "deletePlaylistSelect" pour cette raison -- la
//      nouvelle page BASIC (fractionnement du 23/07) avait repris un nom
//      plus court sans ce garde-fou. Renomme l'id en "deletePlaylistSelect"
//      (BASIC uniquement -- verifie qu'aucune autre page ne collisionne :
//      MEDIA utilise deja "playlistSelect", distinct de sa fonction
//      deletePlaylist()).
//   2) Confirmation JS avant perte de modifications non sauvegardees sur
//      "Reprendre DMD"/"Redemarrer" (_formDirty) : documentee comme fusionnee
//      et validee le 2026-07-21, absente des 3 pages a formulaire (BASIC/
//      NETWORK/CLOCK) -- 7e regression confirmee du fractionnement du
//      23/07. Restauree : suit les evenements 'input' du formulaire,
//      remise a false apres un /save reussi, confirm() avant doReboot()/
//      dmdResume() si des modifications sont en attente.
//   3) handleWebConfigDeleteFiles() (suppression d'image individuelle dans
//      un dossier /gifs, page MEDIA) utilisait SD.remove() brut, sans le
//      repli forceDeleteFile() (rename puis remove) deja utilise par
//      deleteFolderRecursive() pour le bug FAT32 lecture-seule documente
//      sur ce projet -- pouvait echouer silencieusement sur certains
//      fichiers. Aligne sur le meme repli.
//   Verifie sans anomalie : toutes les routes fetch() des 6 pages
//   correspondent a des handlers webServer->on() enregistres ; tous les
//   champs serialize() de BASIC/NETWORK/CLOCK sont bien lus par
//   handleWebConfigSave() ET renvoyes par handleWebConfigLoad() (aucun
//   champ orphelin) ; aucune autre collision id/nom de fonction sur les
//   6 pages ; aucun onclick ne reference une fonction absente.
//   Compilation verifiee OK (62% flash). PAS ENCORE reteste sur materiel
//   reel.
//
// v34 — 2026-07-25 — safe-modify — Suite test reel (console F12) : le fix v33
//   (send() plus jamais appele depuis le callback upload) n'a PAS suffi --
//   ERR_CONNECTION_RESET x3 (les 3 tentatives) puis ERR_CONNECTION_TIMED_OUT
//   confirmes par le navigateur sur /upload lors de la creation d'un nouveau
//   dossier. Cause reelle trouvee : le timeout client HTTP elargi a 15s
//   pendant l'upload (deja documente et valide le 2026-07-21 -- les
//   ecritures/creations SD depassent le defaut ~3s de la lib WebServer,
//   qui coupe alors la connexion) etait ABSENT du code actuel : 5e
//   regression confirmee du fractionnement en pages du 2026-07-23 (apres
//   SSID, config.ini wipe, missing-params brightness, upload multi-fichier).
//   Restaure : webServer->client().setTimeout(15000) au tout debut de
//   UPLOAD_FILE_START (avant le mkdir/tentative d'ouverture), remis a 3000
//   des UPLOAD_FILE_END/UPLOAD_FILE_ABORTED. Compilation verifiee OK. PAS
//   ENCORE reteste sur materiel reel.
//
// v33 — 2026-07-25 — safe-modify — Retours utilisateur post-test reel (page
//   MEDIA + Reprendre DMD). Corriges :
//   1) "Reprendre DMD reste en mode web config" -- chaque page (MENU/BASIC/
//      NETWORK/CLOCK/MEDIA) faisait un fetch('/dmd-open') cote client des
//      son chargement, EN PLUS du triggerWebConfigMode() deja fait cote
//      serveur avant l'envoi du HTML (redondant, meme message "WEB DMD
//      CONFIG"). Cet appel asynchrone pouvait etre traite par le
//      WebServer APRES un clic rapide sur "Reprendre DMD", re-armant le
//      mode config juste apres la reprise. Supprime des 5 pages (garde sur
//      la page AP, qui envoie un message localise different).
//   2) Upload GIF "erreur reseau" a la creation d'un nouveau dossier --
//      handleWebConfigUploadFile() appelait webServer->send() DEPUIS le
//      callback UPLOAD_FILE_START en cas d'erreur (dossier manquant, nom
//      invalide, echec ouverture SD) alors que le client est encore en
//      train d'envoyer le corps multipart : une reponse prematuree casse
//      la connexion HTTP en cours, vu cote navigateur comme une erreur
//      reseau. Plus courant sur un dossier a creer (mkdir + tentative
//      d'ouverture juste apres, plus fragile). Fix : le callback upload ne
//      fait plus jamais send(), stocke l'erreur dans uploadErrorMsg ;
//      handleWebConfigUpload() (appele une fois le corps entierement
//      recu) est desormais seul a repondre. Ajout d'une 2e tentative
//      d'ouverture apres 50ms si la 1ere echoue juste apres un mkdir (SD
//      pas encore prete), et de logs Serial a chaque etape.
//   3) Upload GIF ne permettait plus qu'un seul fichier a la fois
//      (regression vs l'ancienne page unique) -- <input> repasse en
//      multiple, uploadGif() reecrit en boucle sequentielle asynchrone
//      (3 tentatives + 500ms de backoff par fichier, recap nomme des
//      echecs definitifs), bouton "Arreter" (n'interrompt qu'entre deux
//      fichiers) et progression /dmd-pause par fichier restaures --
//      parite avec le mecanisme documente le 2026-07-21 et perdu lors du
//      fractionnement en pages minces du 2026-07-23.
//   4) Page MENU (accueil) : menu du haut (topnav) retire -- faisait
//      double emploi avec la grille de liens juste en dessous. Reste sur
//      les memes couleurs que les sous-pages (section #16213e, accent
//      #8ab4f8), inchange sur BASIC/NETWORK/CLOCK/MEDIA (utile la, un seul
//      lien "actif" par page).
//   PAS CORRIGE (investigue, cause non trouvee par lecture statique) :
//   affichage du contenu d'un sous-dossier /gifs qui n'apparaitrait pas
//   apres clic sur l'icone dossier -- routes /lsgifdirs et /lsgiffiles
//   confirmees enregistrees, JS structurellement correct, format JSON
//   coherent des 2 cotes, pas de conflit CSS display ni de collision de
//   declaration JS trouves. A retester avec la console navigateur (F12)
//   ouverte pour voir si le fetch echoue silencieusement.
//   Compilation verifiee OK. PAS ENCORE teste sur materiel reel.
//
// v32 — 2026-07-23 — safe-modify — Logo Recalbox (fourni par l'utilisateur)
//   integre sur la page MENU en data-URI base64 (pas de nouvelle route/
//   PROGMEM binaire separe -- reste dans le pipeline gzip existant).
//   Recadre (source 1920x1080 -> zone logo+fantomes 1170x830) et
//   redimensionne a 260px de large, palette reduite a 32 couleurs (choix
//   utilisateur parmi 3 options testees : 220px/24c, 260px/32c, sans
//   image -- voir tools/assets pour le script de generation). Page MENU :
//   1252 -> 5791 octets gzip -- notable mais reste tres en-dessous du
//   seuil ~12.5 Ko a risque, et c'est la page d'accueil (chargee une
//   fois, pas en boucle). Compilation verifiee OK (62% flash). PAS
//   ENCORE reteste sur materiel reel.
//
// v31 — 2026-07-23 — safe-modify — Retours utilisateur post-test reel des
//   pages fractionnees (v29/v30) :
//   (1) Bug confirme "sauvegarder : missing parameter" : handleWebConfigSave()
//   exigeait "brightness" comme parametre obligatoire (herite de l'ancienne
//   page unique, ou tous les champs etaient dans le meme formulaire) --
//   les pages NETWORK/CLOCK/MEDIA ne l'envoient jamais, donc TOUTE
//   sauvegarde depuis ces pages echouait. Pire : la variable locale `b`
//   (brightness) etait ensuite ecrite telle quelle dans config.ini sans
//   repli sur la valeur persistee -- une simple suppression du garde
//   aurait ecrit brightness=0 (ecran noir) a chaque sauvegarde
//   NETWORK/CLOCK/MEDIA. Fix complet : "brightness" suit desormais le
//   meme pattern hasArg() que tous les autres champs, et `b` est
//   recalcule depuis screenBrightness (valeur persistee) juste avant
//   l'ecriture, meme formule que handleWebConfigLoad().
//   (2) Page MENU (accueil) reecrite pour s'harmoniser avec les 4 autres :
//   meme topnav permanente, meme palette/style de cartes, banniere titre.
//   Nouveau : lien "Continuer..." mis en avant si une section a deja ete
//   visitee (localStorage "dmd_last_section", ecrit par chaque page en
//   fin de script) -- sinon la grille des 4 sections reste affichee
//   normalement (pas de section "par defaut" arbitraire).
//   (3) Page MEDIA : parite complete avec l'ancienne page unique --
//   navigation dans un dossier (icone dossier ouvert) pour lister/
//   selectionner/supprimer des IMAGES individuelles (/delete-files), en
//   plus de la suppression de dossiers entiers deja presente
//   (/delete-folders, un seul bouton "Supprimer la selection"
//   desormais context-sensible comme sur l'ancienne page). Upload GIF :
//   champ texte ajoute a cote du menu deroulant pour taper un NOUVEAU nom
//   de dossier (le backend le cree deja automatiquement, seule l'UI ne le
//   permettait pas). Tailles apres regeneration : MENU 1252, MEDIA 3411
//   octets gzip (BASIC/NETWORK/CLOCK inchangees a la marge) -- toutes
//   tres en-dessous du seuil ~12.5 Ko a risque. Compilation verifiee OK
//   (62% flash). PAS ENCORE reteste sur materiel reel.
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

#include <WiFi.h>
#include <WebServer.h>
#include "web_config_html_gz.h"

extern int    screenBrightness;
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
extern bool   g_sdOpInProgress;
extern bool   g_playlistStartedThisBoot;
extern bool   requestReboot;
extern String uiLanguage;
extern void webDmdPause(const String &msg, uint16_t color = 0xFFFF);
extern void webDmdResume();
extern void webDmdSetMainMsg(const String &msg);
extern void clearFirstBoot();
extern String g_sdOpSubMsg;
extern String g_sdOpPersistentSubMsg;
extern uint16_t g_sdOpPersistentSubMsgColor;

static WebServer *webServer = nullptr;
static File uploadFile;
static String uploadDir;
static unsigned long uploadStartMs;
static int uploadTotalBytes;
static bool uploadSuccess = false;
static String uploadErrorMsg;

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
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="logo-wrap"><img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQQAAAC4CAMAAAAyqWKCAAAAYFBMVEX///8U//n+/v77+/vr6+tw6Nz7lJ/9ixTNqaFwkZSUTlJXOy9EOjk6KSXpEBDmDQ17Dg45Hx45Dg4RUlQPKiodFhYZDxAKEREVBgULBgYFBAQCAwMBAQE4AAAAAAEAAAAgLdlgAAAQH0lEQVR42u2di3aiOhSGU2A0QGhF5RZief+3PPuSQECwdk5nIdW91rRgtWvy9d+X3IjoXtaJF4IXhBeEF4QXhBeEF4QXhBeEH4fw+YIAZvqL9mkhtLNAngmCMaf4zE03XZVtn8K3IbRta6r47cRiMCaWqe7aJ3SH89vb2RgWQhzLuNo4hW9C+DSn07k6gRA6pNB2WYwUim1TEN/MCdXb21sM3nCuOD4qGSugcN50XPiuEtrTKX5DA0WgGFKZdmdQQ7ZlCt+PCS14wxuSOEG721iqtgAIEBfM80CAv/75LT5XZ1QCxkUMCjKOm6fKDq2BuNjakiljAunZPFeKNB0mBywXEAIRaJ+uYkQIZ5cS24w00JrnK5v1+K41T6gEjI7jdhvwjudSgrlcasuia5JE5WXzdEo4fIAdLh1VzcmOLAEWz1MsGUMMwJBCDwEt227/4bt9hwOq4IIkaqybkl1S5ipNEoRgnkQJNTDA7xf8bjoNnkCv57td8SQQDDYeFaBREXBb7nYKUqRu1W7XPBOExoUG8AYQQO5iw7NkB4JwoSoBlaA7EECBSjAQG56nF6khImoXE3SX7nZcPSKEtnui7ADpgbODRi+AEiHNwC3Sp4FgKdg6YVQmqKeB0EIiIBkcagoBOqMSAa1onwJCP+FW12Y04dIUucrpDea3Q0AEhVKqwFCoTS8NvDVFBj+YTs/9OggggzKWkRAijGLsLMHIEhq0Wqs4CuEHkUw2Ogsj7h5ESJCACPCLkEOfsVX8Ov0gUpscZBJ3dh51DBqglgZBGCCGoqgarZWEV+wPQtBDvEUK4j5f0FKELAKyIES3QBPD60ACXt8iBXFfPAAGYmRBMP5OF47Cb4TQdrFjAJIfWo3mcQkthe0V0OIeBir0feHKkEwQJgXEhwBDxubGmMSdAYFbK6IEksQECN/G8NYqAmmEQrYbCwviDiEkzhlCobrBNXoGURxHItNQNhGt7TmE+Do7NhG7PoYA1bYqCMeRQMJSBRUq6FR2pJIgiBpjfhUE7YRALQclJH5OYHU0oIEo65qY8yVKQf8mCFAYR4HVP5QFAEFRdeB5gzJNA04SSRctwkC2v0UJLRoModk6MVJVVcEfWMM3FfVaAHfAnlUEGpAqi+2bwTno41uH4P6WHAhHRdCoeILGJzFWjpF27w4pV2xnnadYZqAkWsYih6Z6M46lnycD6kFGIjbYl8BbUEeGn43VNiiI5QqJ44AKub2UCSUsX4X1anJcK4ShLDody661URTey33LcBuDbmI5MULjoHmqr5i97DCulygEdDoCH8g4WEC0lPhpITeRK8WiEALSdTzURlAdh8q0UEOPi2goEguulKS0snGfC8JNLPMUS9UBNz5Sfv8xYCWE076DSChBBL1EAIJCl9lIxbDkDhj/gwAW5o1DoCwKOe07YPpMMmx0X0ti3oTSKcDvm1VC2xUhVwB60l8aFUrTfrRfSxviKKItLPIUC96gsAKGnsIEQiBEMNuZFpOhBQnVEv+ODUhhSQn0VyxhTiGaZoKFMYVpzxKUUERUNultQjDYdvbnJhK3xlOWhllEpB1J/fhZUixVSgFH9v8BgTPMFsaZxGKC5GGyawhYLlzZ1WBT1HCtsQl/EIsJUkQwx/T3SsDVO9o51QYh2AQZ44yjvgqMYTxjYRBcQ2B/iB5/RZdYGkyi5AbLkSYQQpHM/ZrJwCNAaBGCwmzy+ElSLCdILnPG0y5UAPB4iWduiHX0NtP1SXKDEGA/S+/Lk7knRDM3ctYaau4IQv9pSBRmcxC8BImTT0HoM1hIeLD9w6dgcwJnmeDh/UEs9SA5v1P97DFYbA9OU/lDboqVkIVbSJJz7mBsgqTrqh9svsWAcEXDJI3E9Ioz+ptIkuJGghx8I6B/4Av61gQFDkfSW4XzgD5JthuDAMtT/d4fTkmzxL/aEg0LeiR3pcLhwxRe1IP7g1hMkH3+g6FVuJcwqHSVGyeZsqO3wrjk8M5yC/4gFhJkNHqxyO7e4VPQKrbB6Jc9+Ap4MZsgodCLVeZZkd1rRVHQF7YMC+qHLxrF0hDrzf6RZ+Ft86qG7UDACVjBU8s/YkE/4rghCJQgg68FcK/Z7tRjJ0kxGxJ+2B49KIi/CAnftkcPCi8ILwgvCC8ILwgvCC8I/wQC9h1EeHsB+EsJvx8CrmFM4jBOEnmr2P7dEGjVZgHdo5mVTNuHMPQCg5nL/gpap3UWwTL/OAhn3k2/6PfHBFJC1rnR2N+kBN7phhb0V+Hci4GDkMy9B2Ze8OtGIUT9ICGsbS/KsiqLKpZwhS+VKsqqEgwupYVgGvqJlKX9nMJfIXHJ3wZWLs1DwNX7Ma3vDmmHgyzxHl6U/Aenr3TvlKD620ri6LwKYUE4jTzLx5+JW4AAG+Al+zMs5IVt4gTF4II+8nHIibB1HPZ7ySEmRBpvDc5PK2NgeTiuBu8KudmYEMFzEiTGe5g9ajU/dDGSsA6BIUS46ymO8U1x20J2aFuEwLcFfgbWMIoYJyCKeLMQpFudQauZae+CJL3zgzixMFAoAusOBTGxt22MrVchXKoEd89FG3WHCMQueflqDJc0qyIjd6XwCi7hC8zOKdgJ526ldFcx/wq4z6In7zuEW68Yp5MNc2Wk2yw/uuV32F/xFBXjqxf5gvCC8ILwgvAkEIKfhxBsUAk/bhtTAu7UED9uwbbWJ+BG8fjHTW1s9dq/+e+aza1y1z9u7euAvO51SuALwgvCC8ILwvdOSyB7KWFlRCtDgJqkyHOY6GvWFMP6SpD07Guc6Lt+CGLbVviYL5jbMp9fqWWrEIzuHwC+gwffNjMN4dP46By6pZOIPu8429fcdKs1IcCDCXaeyfGBKQZO4judmMFbrJeakO73CVre/v3TKlaEgA828yHskpEWWjyg1LerMymNrhud7NkOt0RwPIAdwepHg6DLZDc22e+q/vw0upfBm3OJ8TZtOGhkP1iSpulx6QmIh3drx8eCAFvqd7jawTZfoijksJMOHlrwNrXxmZSmqdP92A7XfXargvePdziL4P390DTaPBQEWvHBENyFOzQFTuGL//z587bsEKY77qeWpMfruPDx7hsey3CFQawXFQte3IQK4JUvIwjxG7xkKfyBy6kWdJHury2ZRo3DATUw5mAeRwmmgKUfPoTIgwCbk7HlVgt/eh5VD6nc7/EjrvVAkCDk5VQHHxMG7x+HQz0WzIru0OFKB246rmfgK/tE09bEDAbbPlyNIKAPScegv/ZPIQIdQPKEZg++wEQuj6EEOFENlz3ZpmeZg9C/QcVJbJsuSTGRD6F3JkkCkD6EwZqPDwmLzHYWwscO9JLgDeTLR4AA29J3KruCIJOcD5OpzvxThIBrXsIxBGNK60PY9J1dPyknEDSsuY56CEQklKyFg59H1oOQ7OgvLEfugKcI4QljcEytUokHwSrh3FcCn4o/RA3HdVTuund3eLKNFDbaYMM59Ej2j4eBsBulSOlOXGQIsQ+BhGLrZ/7f56niN1DDfSB+7OXQi39+aHjklttiungcCF6t6Gj0ECgecnagBXMuXbbca9rvFyDsa+8pB5kVkcQkkcQ2uDCE7uEg+GdvOgh/RnWCK5x6CGk6D2GfuGnP40G5H2BaSO3NpiB45hWPPQSqE2yKhJ0nA4S9lYL5eE8XIEBg1JuD4JmNCbY4cHXCLhyu97araA7vh0UI7+5Qs01CqAYIvuGa2wkE6DtCVuSEQIf/ggMRBCgVPqhiMusXS/8DAnx6v2wDBKiPuDSgUvGDNmVQhnRHAf92CIaU8I6jTtxoCwFfpNvHgSCTf6mEj7G5HsSjQUg+7WhzJv8NhL7jNDUYZTLrd6Uz6jVolfBoSrIblc3/C4Lp3WHZvKJx1ZElKXNY0JVj07UuFXlHMYLQX5y+AeG4GQg40IpTDaYtIXQVdvzdVf7VGScc4pODEPc0fhUE2lqlSjp4FGDgUFiewYmc/RgjjSm2RpMOzu0pnoWQp1uG0MBho6nKS93AWfWqgC5ygw8t+/RnXiqccDm5lhONqmVGqaVQmz4SHDcHwZQ59m5UDufPAoSsaOCFNM0at8brDKbZM+AC4gRpwRtvRgqHY6PTg+0xNPdAGFKF14cSqwUEajxvmMLvOd/nZUujxNUZldCazxYuHI1qeEpua1JuLjxamhretPqYcmdqgGBHFS/1DI8HgAAnEKM7sNnvMIeEksADaonB6QzN5gt4vqPGi8r7DXWNE/o8q5tQWvxkHfRnwh8O7gxsywM6DMbxWB9CW3oQsPEDBJQCuMCJKfD3U0+jcu5ijmme1/aI57rGgAqhNT0kpTecfsEGX2CY5XK85rE6BE0QmEKaWgr4FSBoFIJtvDOPBkfGushx8hFkY+oMYgnjgA3ctfbqsfpA7aYRFqwcL/CY7frCPA6rQ2gYQs8gVVYNAKEx1xA8Gtp6E34qP9ZNeczpou004kj92RfTi8SQbzCP+mMy3LwOhJYg9BRQEil7BULQ+haECmNGfSQI0Pjj0eGoS74qhwn+epiOZx48kwsztB/H1UeWtA+hN4ZQNHAE3y0I+EztHgJYanHkecZXpXah80IQCMqUR/O5MgSYVYeS+ZoCZkxYxVXdhgArUtAHbOOdZY4BigIzBZAiBsfjpdY1X18whpiGLtdWQts05RAURhDywkI4L0KAYmEGwmAAQXtCgPZeao+HfflSDzP0YpVuA7rDvBLugqDr2xDKhuVmW45N7i96HsdhguJxIZwXIeivIVDc8CAcr3GgFlaGAE9pwWdBz4QEOMOeIZwXITQEIZtnkAEEbN4CBA9HL4V1YgJDyOchoBKWpIAvVxVDyG9AgAB4B4TaPASEbEYIX0DAn0JyyZekkBMECJ4A4fIFBL0uBAwKUwrZdyAAhSxbgFBA8/Q9EGxZtUpM0JpyJEHIroTQQzgveEOFT3m6CQGkQBC+codGrxoYyR/GFDIfQvG3EDILARj0EPIlCPWKEIyDMFCgJ9bkxRjCeYYBQeh1NA8B+pJsRIBsAQL7w2oxgdphKVjzGFh/OF8z+A6EsnQMLAX/em0IfVAoKMhbBMSgh1BcURgYoD8sQMgcBHQHCp+9Hb1LW0zXvMh3PQgVU+j/mxaBZeCkcJ4wsBAqmx6yGQa4kaamoGAhZPmVOQj1ihBanwKRsN9hHxDYiMIw0jYwsGH1mkLGcfEGhGyQAiqB8oNYa+sTRYWBgmNAEJoxBN+Ygc2wVxSsV5XUvGbiDvhD9jsrhZUhUJK8olA6BssUKh8CSyGbMBhDKHwGSCFnChYCFFVmNQgUFZrKp1A6BrjNvpqnwC9raqDTeo8h6xmQN1jGEwheUChXhmCjAlHwzTFYgGAZWBUVA4U+yeZcbjGEuvSkQK6QTyHwHhCx4o7IZorBMsADKKsZDJWDoEct9AiMhODczY8JV0pYFQJKQVv/L71gAPKkUzh1NcVQDQwmFDxjt7IM7FuKfDZFUkXV6FWVAG21EEamae+fp4WpsVC4hVcUisIXgpPC8fgFhP8A7kK1Ey30vv0AAAAASUVORK5CYII=" alt="RecalBox"></div>
<div class="tagline" data-i18n="tagline">Configuration DMD</div>
<div class="section">
<a id="continueLink" class="continue" href="#"></a>
<div class="menu">
<a class="btn" href="/config/basic" data-i18n="menu_basic">&#x1F4A1; Affichage &amp; Playlist</a>
<a class="btn" href="/config/network" data-i18n="menu_network">&#x1F4F6; Wi-Fi &amp; Bluetooth</a>
<a class="btn" href="/config/clock" data-i18n="menu_clock">&#x23F0; Horloge</a>
<a class="btn" href="/config/media" data-i18n="menu_media">&#x1F4BF; M&eacute;dias &amp; Playlists</a>
</div>
<div class="small" data-i18n="small_hint">Page fractionn&eacute;e pour un chargement rapide et fiable sur ESP32.</div>
</div>
<script>
const MENU_I18N={
fr:{title:'RecalBox DMD',tagline:'Configuration DMD',menu_basic:'&#x1F4A1; Affichage &amp; Playlist',menu_network:'&#x1F4F6; Wi-Fi &amp; Bluetooth',menu_clock:'&#x23F0; Horloge',menu_media:'&#x1F4BF; Médias &amp; Playlists',small_hint:'Page fractionnée pour un chargement rapide et fiable sur ESP32.',cont_basic:'&#x1F4A1; Continuer : Affichage & Playlist',cont_network:'&#x1F4F6; Continuer : Wi-Fi & Bluetooth',cont_clock:'&#x23F0; Continuer : Horloge',cont_media:'&#x1F4BF; Continuer : Médias & Playlists'},
en:{title:'RecalBox DMD',tagline:'DMD Configuration',menu_basic:'&#x1F4A1; Display &amp; Playlist',menu_network:'&#x1F4F6; Wi-Fi &amp; Bluetooth',menu_clock:'&#x23F0; Clock',menu_media:'&#x1F4BF; Media &amp; Playlists',small_hint:'Split page for fast, reliable loading on ESP32.',cont_basic:'&#x1F4A1; Resume: Display & Playlist',cont_network:'&#x1F4F6; Resume: Wi-Fi & Bluetooth',cont_clock:'&#x23F0; Resume: Clock',cont_media:'&#x1F4BF; Resume: Media & Playlists'},
es:{title:'RecalBox DMD',tagline:'Configuración DMD',menu_basic:'&#x1F4A1; Pantalla y lista',menu_network:'&#x1F4F6; Wi-Fi y Bluetooth',menu_clock:'&#x23F0; Reloj',menu_media:'&#x1F4BF; Medios y listas',small_hint:'Página dividida para una carga rápida y fiable en ESP32.',cont_basic:'&#x1F4A1; Continuar: Pantalla y lista',cont_network:'&#x1F4F6; Continuar: Wi-Fi y Bluetooth',cont_clock:'&#x23F0; Continuar: Reloj',cont_media:'&#x1F4BF; Continuar: Medios y listas'}
};
let currentLang='fr';
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
  if(last&&SECTIONS[last]){a.href=SECTIONS[last].url;a.innerHTML=tr(SECTIONS[last].key);a.style.display='block';}
  else{a.style.display='none';}
}
fetch('/lang').then(function(r){return r.json();}).then(function(d){applyLang(d.language);}).catch(function(){applyLang();});
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
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" class="active" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Affichage &amp; Playlists</h1>
<form id="basicForm" onsubmit="saveConfig(event)">
<div class="section">
<h2 data-i18n="sec_display">&#x1F4A1; Affichage</h2>
<div class="row"><label for="brightness" data-i18n="lbl_brightness">Luminosit&eacute; (%)</label><input id="brightness" type="range" min="0" max="100" value="50" oninput="document.getElementById('bval').textContent=this.value"><span id="bval" style="margin-left:8px;color:#ffd146;min-width:24px">50</span></div>
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
<div class="row"><label for="playlistName" data-i18n="lbl_playlist_name">Nom playlist</label><input id="playlistName" data-i18n-placeholder="placeholder_playlist_name" placeholder="ex: MaPlaylist"></div>
<div class="btn-row"><button type="button" class="btn btn-gen" onclick="generatePlaylist()" data-i18n="btn_gen_playlist">&#x2699; G&eacute;n&eacute;rer playlist</button></div>
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
const PAGE_I18N={
fr:{title:'RecalBox DMD - Affichage',h1:'Affichage &amp; Playlists',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_display:'&#x1F4A1; Affichage',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Luminosité (%)',lbl_silent_boot:'Démarrage silencieux',lbl_playlist_file:'Playlist par défaut',lbl_random:'Lecture aléatoire',lbl_delete_playlist:'Supprimer',btn_delete_playlist:'&#x1F5D1; Supprimer playlist',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_select_playlist:'Sélectionnez une playlist à supprimer',msg_confirm_delete:'Supprimer ${0} ?',msg_deleting:'Suppression...',msg_load_error:'Impossible de charger la config',sec_manage_playlists:'&#x2699; Gestion des playlists',desc_gen_playlist:'Cochez des dossiers pour générer une nouvelle playlist.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',lbl_playlist_name:'Nom playlist',placeholder_playlist_name:'ex: MaPlaylist',btn_gen_playlist:'&#x2699; Générer playlist',msg_no_playlist_name:'Donnez un nom à la playlist',msg_select_folder:'Choisissez au moins un dossier',msg_generating:'Generation...'},
en:{title:'RecalBox DMD - Display',h1:'Display &amp; Playlists',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_display:'&#x1F4A1; Display',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Brightness (%)',lbl_silent_boot:'Silent boot',lbl_playlist_file:'Default playlist',lbl_random:'Random playback',lbl_delete_playlist:'Delete',btn_delete_playlist:'&#x1F5D1; Delete playlist',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_select_playlist:'Select a playlist to delete',msg_confirm_delete:'Delete ${0}?',msg_deleting:'Deleting...',msg_load_error:'Unable to load config',sec_manage_playlists:'&#x2699; Playlist management',desc_gen_playlist:'Check folders to generate a new playlist.',btn_select_all:'Select all',btn_select_none:'Select none',lbl_playlist_name:'Playlist name',placeholder_playlist_name:'e.g. MyPlaylist',btn_gen_playlist:'&#x2699; Generate playlist',msg_no_playlist_name:'Please name the playlist',msg_select_folder:'Select at least one folder',msg_generating:'Generating...'},
es:{title:'RecalBox DMD - Pantalla',h1:'Pantalla y listas',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_display:'&#x1F4A1; Pantalla',sec_playlist:'&#x1F4BF; Lista',lbl_brightness:'Brillo (%)',lbl_silent_boot:'Arranque silencioso',lbl_playlist_file:'Lista predeterminada',lbl_random:'Reproducción aleatoria',lbl_delete_playlist:'Eliminar',btn_delete_playlist:'&#x1F5D1; Eliminar lista',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_select_playlist:'Selecciona una lista para eliminar',msg_confirm_delete:'¿Eliminar ${0}?',msg_deleting:'Eliminando...',msg_load_error:'No se pudo cargar la configuración',sec_manage_playlists:'&#x2699; Gestión de listas',desc_gen_playlist:'Marque las carpetas para generar una nueva lista.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',lbl_playlist_name:'Nombre de la lista',placeholder_playlist_name:'ej: MiLista',btn_gen_playlist:'&#x2699; Generar lista',msg_no_playlist_name:'Póngale un nombre a la lista',msg_select_folder:'Elija al menos una carpeta',msg_generating:'Generando...'}
};
let currentLang='fr';
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
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK'))_formDirty=false;}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});}
function saveAndReboot(){saveConfig().then(()=>setTimeout(doReboot,400));}
function dmdResume(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));}
function fillPlaylists(selVal){fetch('/lsplaylists').then(r=>r.json()).then(pl=>{const sel=document.getElementById('playlist');const del=document.getElementById('deletePlaylistSelect');sel.innerHTML='';del.innerHTML='';const opt=document.createElement('option');opt.value='';opt.textContent='---';sel.appendChild(opt);const opt2=document.createElement('option');opt2.value='';opt2.textContent='---';del.appendChild(opt2);pl.forEach(p=>{const o=document.createElement('option');o.value=p;o.textContent=p;if(p===selVal)o.selected=true;sel.appendChild(o);const o2=document.createElement('option');o2.value=p;o2.textContent=p;del.appendChild(o2);});}).catch(()=>{});}
function deletePlaylist(){const name=document.getElementById('deletePlaylistSelect').value;if(!name){showMsg(tr('msg_select_playlist'),false);return;}if(!confirm(trTpl('msg_confirm_delete',name)))return;showMsg(tr('msg_deleting'),true);fetch('/delete-playlist',{method:'POST',body:new URLSearchParams({name:name}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));fillPlaylists('');}).catch(()=>showMsg(tr('msg_net_error'),false));}
// Generation de playlist (deplacee depuis MEDIA -- demande utilisateur :
// la gestion des playlists va dans Affichage, la gestion physique des
// fichiers/dossiers reste dans MEDIA). Liste des dossiers ici pour
// COCHER uniquement -- pas d'icone d'ouverture/consultation du contenu,
// reservee a la page MEDIA.
function loadGenDirs(){fetch('/lsgifdirs').then(r=>r.json()).then(dirs=>{
  const list=document.getElementById('genDirList');list.innerHTML='';
  dirs.forEach(d=>{
    const name=(d&&typeof d==='object')?d.name:d;
    const row=document.createElement('label');
    row.innerHTML='<input type="checkbox" value="'+name+'"><span class="name">&#x1F4C1; '+name+'</span>';
    list.appendChild(row);
  });
}).catch(()=>{});}
function selectAllGenDirs(v){document.querySelectorAll('#genDirList input').forEach(i=>i.checked=v);}
function generatePlaylist(){
  const name=document.getElementById('playlistName').value.trim();
  const dirs=[].slice.call(document.querySelectorAll('#genDirList input:checked')).map(i=>i.value).join(',');
  if(!name){showMsg(tr('msg_no_playlist_name'),false);return;}
  if(!dirs){showMsg(tr('msg_select_folder'),false);return;}
  showMsg(tr('msg_generating'),true);
  fetch('/generate-playlist',{method:'POST',body:new URLSearchParams({name:name,dirs:dirs}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));if(t.includes('OK')){document.getElementById('playlistName').value='';fillPlaylists('');}}).catch(()=>showMsg(tr('msg_net_error'),false));
}
function loadConfig(){return fetch('/load').then(r=>r.json()).then(d=>{document.getElementById('brightness').value=Math.max(0,Math.min(100,parseInt(d.brightness||50,10)));document.getElementById('bval').textContent=document.getElementById('brightness').value;document.getElementById('silent_boot').checked=d.info==='0';fillPlaylists(d.playlist||'');document.getElementById('random').checked=d.random==='1';}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','basic');
// loadGenDirs() enchainee APRES /lang+/load (jamais en parallele) : le
// WebServer ESP32 ne traite qu'une requete a la fois -- des fetch()
// concurrents corrompent silencieusement l'une des reponses (bug deja
// documente et corrige sur MEDIA via queuedFetch(), reintroduit ici par
// inattention lors de l'ajout de la generation de playlist, v79).
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);return loadConfig();}).catch(()=>{applyLang();return loadConfig();}).then(loadGenDirs);
document.getElementById('basicForm').addEventListener('input',()=>{_formDirty=true;});
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
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" class="active" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
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
const PAGE_I18N={
fr:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi &amp; Bluetooth',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Activé',lbl_network:'Réseau',lbl_password:'Mot de passe',lbl_static_ip:'IP statique',lbl_fixed_ip:'IP fixe',lbl_gateway:'Passerelle',lbl_subnet:'Masque',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Nom',lbl_mqtt_ip:'IP Recalbox',opt_scanning:'-- Scan en cours... --',opt_select:'-- Sélectionnez --',opt_scan_error:'Erreur scan',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config'},
en:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi &amp; Bluetooth',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Enabled',lbl_network:'Network',lbl_password:'Password',lbl_static_ip:'Static IP',lbl_fixed_ip:'Fixed IP',lbl_gateway:'Gateway',lbl_subnet:'Subnet mask',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Name',lbl_mqtt_ip:'Recalbox IP',opt_scanning:'-- Scanning... --',opt_select:'-- Select --',opt_scan_error:'Scan error',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config'},
es:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi y Bluetooth',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Activado',lbl_network:'Red',lbl_password:'Contraseña',lbl_static_ip:'IP estática',lbl_fixed_ip:'IP fija',lbl_gateway:'Puerta de enlace',lbl_subnet:'Máscara de subred',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Nombre',lbl_mqtt_ip:'IP de Recalbox',opt_scanning:'-- Escaneando... --',opt_select:'-- Seleccione --',opt_scan_error:'Error de escaneo',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración'}
};
let currentLang='fr';
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
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK'))_formDirty=false;}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});}
function saveAndReboot(){saveConfig().then(()=>setTimeout(doReboot,400));}
function dmdResume(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));}
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
function loadConfig(){fetch('/load').then(r=>r.json()).then(d=>{document.getElementById('wifi_enabled').checked=d.wifi_enabled==='1';savedSsid=d.wifi_ssid||'';document.getElementById('wifi_password').value=d.wifi_password||'';document.getElementById('wifi_static_enabled').checked=d.wifi_static_enabled==='1';document.getElementById('wifi_static_ip').value=d.wifi_static_ip||'';document.getElementById('wifi_gateway').value=d.wifi_gateway||'';document.getElementById('wifi_subnet').value=d.wifi_subnet||'';document.getElementById('wifi_dns1').value=d.wifi_dns1||'';document.getElementById('wifi_dns2').value=d.wifi_dns2||'';document.getElementById('bluetooth_enabled').checked=d.bluetooth_enabled==='1';document.getElementById('bluetooth_name').value=d.bluetooth_name||'';document.getElementById('recalbox_ip').value=d.recalbox_ip||'';scanWiFi();}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','network');
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);loadConfig();}).catch(()=>{applyLang();loadConfig();});
document.getElementById('networkForm').addEventListener('input',()=>{_formDirty=true;});
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
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" class="active" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Horloge</h1>
<form id="clockForm" onsubmit="saveConfig(event)">
<div class="section">
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;e</label><input id="clock_enabled" type="checkbox"></div>
<div class="row"><label for="clock_theme" data-i18n="lbl_theme">Th&egrave;me</label>
<select id="clock_theme">
<option value="-1" data-i18n="opt_random">Al&eacute;atoire</option>
<option value="0" data-i18n="opt_mario">Mario</option><option value="1" data-i18n="opt_tetris">Tetris</option>
<option value="2" data-i18n="opt_pacman">Pac-Man</option><option value="3" data-i18n="opt_spaceinv">Space Invaders</option>
<option value="4" data-i18n="opt_pong">Pong</option><option value="5" data-i18n="opt_neon">Neon</option>
<option value="6" data-i18n="opt_matrix">Matrix</option><option value="7" data-i18n="opt_fire">Fire</option>
<option value="8" data-i18n="opt_rainbow">Rainbow</option><option value="9" data-i18n="opt_level11">Level 1-1</option>
</select>
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
const PAGE_I18N={
fr:{title:'RecalBox DMD - Horloge',h1:'Horloge',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',lbl_enabled:'Activée',lbl_theme:'Thème',lbl_neon_color:'Couleur Neon',lbl_custom:'Personnalisée',hint_neon:'Thème Neon uniquement',lbl_interval_gifs:'Intervalle (GIFs)',lbl_interval_min:'Intervalle (min)',hint_interval_min:'0 = désactivé',lbl_duration:'Durée (sec)',lbl_tz:'Fuseau horaire',opt_random:'Aléatoire',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'France / Espagne / Allemagne / Italie',opt_tz_uk:'Angleterre (UK) / Portugal',opt_tz_usa_e:'USA - Est (New York)',opt_tz_usa_c:'USA - Centre (Chicago)',opt_tz_usa_m:'USA - Montagnes (Denver)',opt_tz_usa_p:'USA - Pacifique (Los Angeles)',opt_tz_ee:'Grèce / Roumanie / Finlande',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config'},
en:{title:'RecalBox DMD - Clock',h1:'Clock',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',lbl_enabled:'Enabled',lbl_theme:'Theme',lbl_neon_color:'Neon color',lbl_custom:'Custom',hint_neon:'Neon theme only',lbl_interval_gifs:'Interval (GIFs)',lbl_interval_min:'Interval (min)',hint_interval_min:'0 = disabled',lbl_duration:'Duration (sec)',lbl_tz:'Timezone',opt_random:'Random',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'France / Spain / Germany / Italy',opt_tz_uk:'England (UK) / Portugal',opt_tz_usa_e:'USA - East (New York)',opt_tz_usa_c:'USA - Central (Chicago)',opt_tz_usa_m:'USA - Mountain (Denver)',opt_tz_usa_p:'USA - Pacific (Los Angeles)',opt_tz_ee:'Greece / Romania / Finland',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config'},
es:{title:'RecalBox DMD - Reloj',h1:'Reloj',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',lbl_enabled:'Activado',lbl_theme:'Tema',lbl_neon_color:'Color Neon',lbl_custom:'Personalizado',hint_neon:'Solo tema Neon',lbl_interval_gifs:'Intervalo (GIFs)',lbl_interval_min:'Intervalo (min)',hint_interval_min:'0 = desactivado',lbl_duration:'Duración (seg)',lbl_tz:'Zona horaria',opt_random:'Aleatorio',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'Francia / España / Alemania / Italia',opt_tz_uk:'Inglaterra (UK) / Portugal',opt_tz_usa_e:'EE.UU. - Este (Nueva York)',opt_tz_usa_c:'EE.UU. - Centro (Chicago)',opt_tz_usa_m:'EE.UU. - Montañas (Denver)',opt_tz_usa_p:'EE.UU. - Pacífico (Los Ángeles)',opt_tz_ee:'Grecia / Rumanía / Finlandia',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración'}
};
let currentLang='fr';
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
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK'))_formDirty=false;}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});}
function saveAndReboot(){saveConfig().then(()=>setTimeout(doReboot,400));}
function dmdResume(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));}
function loadConfig(){fetch('/load').then(r=>r.json()).then(d=>{document.getElementById('clock_enabled').checked=d.clock_enabled==='1';document.getElementById('clock_theme').value=d.clock_theme||'0';document.getElementById('clock_interval').value=d.clock_interval||'0';document.getElementById('clock_interval_min').value=d.clock_interval_min||'0';document.getElementById('clock_duration').value=d.clock_duration||'0';document.getElementById('clock_tz').value=d.clock_tz||'UTC0';document.getElementById('clock_neon_color').value=d.clock_neon_color||'#ff2878';document.getElementById('clock_neon_color_enabled').checked=d.clock_neon_color_enabled==='1';}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','clock');
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);loadConfig();}).catch(()=>{applyLang();loadConfig();});
document.getElementById('clockForm').addEventListener('input',()=>{_formDirty=true;});
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
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" class="active" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
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
<div class="desc" data-i18n="desc_upload">Ajoutez un fichier .gif directement depuis votre navigateur dans un dossier de /gifs/. Choisissez un dossier existant OU tapez un nouveau nom (cr&eacute;&eacute; automatiquement). &#x26A0;&#xFE0F; Pas fait pour transferer de nombreux fichiers (debit lent, risque d'erreur d'ecriture) -- reserve a l'ajout ponctuel de quelques fichiers. Pour un transfert consequent, retirez la carte SD et copiez-la depuis un PC.</div>
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
<button type="button" class="btn btn-reboot" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
<div id="msg" class="msg"></div>
<script>
const PAGE_I18N={
fr:{title:'RecalBox DMD - Médias',h1:'Médias',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',
sec_dirs:'&#x1F4C1; Dossiers (/gifs/)',desc_dirs:'Cochez des dossiers pour les supprimer.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',btn_delete_sel:'&#x1F5D1; Supprimer la sélection',
sec_upload:'&#x1F4E4; Envoi GIF',desc_upload:'Ajoutez un fichier .gif directement depuis votre navigateur dans un dossier de /gifs/. Choisissez un dossier existant OU tapez un nouveau nom (créé automatiquement). &#x26A0;&#xFE0F; Pas fait pour transférer de nombreux fichiers (débit lent, risque d\'erreur d\'écriture) -- réservé à l\'ajout ponctuel de quelques fichiers. Pour un transfert consequent, retirez la carte SD et copiez-la depuis un PC.',placeholder_upload_dir:'ou nouveau dossier...',lbl_upload_file:'Fichiers .gif',btn_upload:'&#x1F4E4; Uploader',btn_stop:'&#x23F9; Arrêter',
btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',
net_error:'Erreur réseau',msg_deleting:'Suppression...',msg_select_folder:'Choisissez au moins un dossier',msg_confirm_delete_folders:'Supprimer ${0} ?',msg_specify_dir:'Précisez un dossier cible',msg_select_gif:'Choisissez un fichier GIF',msg_select_gif_files:'Choisissez des fichiers .gif',msg_preparing_folder:'Preparation du dossier...',msg_cannot_create_folder:'Impossible de creer le dossier: ${0}',msg_net_error_folder:'Erreur reseau (creation dossier)',msg_uploading:'Upload...',msg_attempt:'tentative ${0}/${1}',msg_stopped_by_user:'Arrete par l\'utilisateur (${0}/${1})',msg_upload_fail:'ECHEC',msg_failures:'Echecs: ${0}',msg_upload_result:'${0}/${1} fichier(s) uploade(s)',msg_upload_result_fail:' -- echecs: ${0}',msg_confirm_reboot:'Redemarrer l\'ESP32 ?',msg_rebooting:'Redemarrage...',msg_dmd_resumed:'DMD repris',msg_updating_playlists:'Mise a jour des playlists...'},
en:{title:'RecalBox DMD - Media',h1:'Media',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',
sec_dirs:'&#x1F4C1; Folders (/gifs/)',desc_dirs:'Check folders to delete them.',btn_select_all:'Select all',btn_select_none:'Select none',btn_delete_sel:'&#x1F5D1; Delete selection',
sec_upload:'&#x1F4E4; GIF Upload',desc_upload:'Add a .gif file directly from your browser into a folder in /gifs/. Choose an existing folder OR type a new name (created automatically). &#x26A0;&#xFE0F; Not designed for transferring many files (slow throughput, risk of write errors) -- meant for occasionally adding a few files. For a large transfer, remove the SD card and copy from a PC instead.',placeholder_upload_dir:'or new folder...',lbl_upload_file:'.gif files',btn_upload:'&#x1F4E4; Upload',btn_stop:'&#x23F9; Stop',
btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',
net_error:'Network error',msg_deleting:'Deleting...',msg_select_folder:'Select at least one folder',msg_confirm_delete_folders:'Delete ${0}?',msg_specify_dir:'Please specify a target folder',msg_select_gif:'Select a GIF file',msg_select_gif_files:'Select .gif files',msg_preparing_folder:'Preparing folder...',msg_cannot_create_folder:'Unable to create folder: ${0}',msg_net_error_folder:'Network error (folder creation)',msg_uploading:'Uploading...',msg_attempt:'attempt ${0}/${1}',msg_stopped_by_user:'Stopped by user (${0}/${1})',msg_upload_fail:'FAILED',msg_failures:'Failures: ${0}',msg_upload_result:'${0}/${1} file(s) uploaded',msg_upload_result_fail:' -- failures: ${0}',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_updating_playlists:'Updating playlists...'},
es:{title:'RecalBox DMD - Medios',h1:'Medios',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',
sec_dirs:'&#x1F4C1; Carpetas (/gifs/)',desc_dirs:'Marque las carpetas para eliminarlas.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',btn_delete_sel:'&#x1F5D1; Eliminar selección',
sec_upload:'&#x1F4E4; Subir GIF',desc_upload:'Añada un archivo .gif desde su navegador a una carpeta en /gifs/. Elija una carpeta existente O escriba un nombre nuevo (se crea automáticamente). &#x26A0;&#xFE0F; No pensado para transferir muchos archivos (velocidad lenta, riesgo de error de escritura) -- reservado para añadir algunos archivos puntualmente. Para una transferencia importante, retire la tarjeta SD y cópiela desde un PC.',placeholder_upload_dir:'o nueva carpeta...',lbl_upload_file:'Archivos .gif',btn_upload:'&#x1F4E4; Subir',btn_stop:'&#x23F9; Detener',
btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',
net_error:'Error de red',msg_deleting:'Eliminando...',msg_select_folder:'Elija al menos una carpeta',msg_confirm_delete_folders:'¿Eliminar ${0}?',msg_specify_dir:'Especifique una carpeta destino',msg_select_gif:'Seleccione un archivo GIF',msg_select_gif_files:'Seleccione archivos .gif',msg_preparing_folder:'Preparando carpeta...',msg_cannot_create_folder:'No se pudo crear la carpeta: ${0}',msg_net_error_folder:'Error de red (creación de carpeta)',msg_uploading:'Subiendo...',msg_attempt:'intento ${0}/${1}',msg_stopped_by_user:'Detenido por el usuario (${0}/${1})',msg_upload_fail:'ERROR',msg_failures:'Errores: ${0}',msg_upload_result:'${0}/${1} archivo(s) subido(s)',msg_upload_result_fail:' -- errores: ${0}',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_updating_playlists:'Actualizando listas...'}
};
let currentLang='fr';
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
function doReboot(){if(!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);queuedFetch('/reboot').catch(()=>{});}
function dmdResume(){queuedFetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('net_error'),false));}
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
function loadDirs(){
  return queuedFetch('/lsgifdirs').then(r=>r.json()).then(renderDirs).catch(()=>{});
}
function loadUploadDirs(){return Promise.resolve();} // conserve pour compatibilite des appels existants -- loadDirs() peuple desormais aussi #uploadDir
function deleteSelected(){
  const dirs=[].slice.call(document.querySelectorAll('#dirList input:checked')).map(i=>i.value);
  if(!dirs.length){showMsg(tr('msg_select_folder'),false);return;}
  if(!confirm(trTpl('msg_confirm_delete_folders',dirs.join(', '))))return;
  showMsg(tr('msg_deleting'),true);
  queuedFetch('/delete-folders',{method:'POST',body:new URLSearchParams({dirs:dirs.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));loadDirs();loadUploadDirs();})
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
  _uploadStopRequested=false;
  const stopBtn=document.getElementById('uploadStopBtn');stopBtn.style.display='inline-block';
  const bar=document.getElementById('uploadProgress');bar.style.display='block';
  const barInner=document.getElementById('uploadProgressBar');
  const fileList=document.getElementById('uploadFileList');
  const msgEl=document.getElementById('msg');
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_preparing_folder');
  try{
    const cr=await queuedFetch('/create-folder',{method:'POST',body:new URLSearchParams({dir:dir}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});
    const ct=await cr.text();
    if(!ct.includes('OK')){stopBtn.style.display='none';showMsg(trTpl('msg_cannot_create_folder',ct),false);return;}
    await loadDirs();await loadUploadDirs();
  }catch(e){stopBtn.style.display='none';showMsg(tr('msg_net_error_folder'),false);return;}
  msgEl.textContent=tr('msg_uploading');
  let okCount=0;const failed=[];const uploaded=[];
  for(let i=0;i<files.length;i++){
    if(_uploadStopRequested){fileList.textContent=trTpl('msg_stopped_by_user',i,files.length);break;}
    const file=files[i];
    const pct=Math.round(((i+1)/files.length)*100);
    barInner.style.width=Math.max(pct,5)+'%';
    fileList.textContent=file.name+' ('+(i+1)+'/'+files.length+')';
    msgEl.textContent=tr('msg_uploading')+' '+file.name;
    try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:file.name+' ('+(i+1)+'/'+files.length+')',color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
    let ok=false,lastErr='';
    for(let attempt=0;attempt<3&&!ok;attempt++){
      if(attempt>0){
        const attemptTxt=file.name+' ('+(i+1)+'/'+files.length+') - '+trTpl('msg_attempt',attempt+1,3);
        fileList.textContent=attemptTxt;
        try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:attemptTxt,color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
        await new Promise(r=>setTimeout(r,500));
      }
      const form=new FormData();form.append('dir',dir);form.append('file',file);
      try{
        const r=await queuedFetch('/upload',{method:'POST',body:form});
        const t=await r.text();
        if(t.includes('OK')){ok=true;} else {lastErr=t;}
      }catch(e){lastErr=tr('net_error');}
    }
    if(ok){okCount++;uploaded.push(file.name);fileList.textContent=file.name+' OK ('+okCount+'/'+files.length+')';}
    else {failed.push(file.name);fileList.textContent=file.name+' '+tr('msg_upload_fail');}
  }
  stopBtn.style.display='none';
  if(uploaded.length){
    fileList.textContent=tr('msg_updating_playlists');
    try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(tr('msg_updating_playlists')),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
    try{await queuedFetch('/add-to-playlists-batch',{method:'POST',body:new URLSearchParams({dir:dir,files:uploaded.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
  }
  fileList.textContent=failed.length?trTpl('msg_failures',failed.join(', ')):'';
  document.getElementById('uploadDirCustom').value='';
  await loadDirs();await loadUploadDirs();
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
queuedFetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);}).catch(()=>{applyLang();});
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
</style>
</head>
<body>
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
fetch('/lang').then(function(r){return r.json();}).then(function(d){applyLang(d.language);scanWiFi();}).catch(function(){applyLang();scanWiFi();});
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
  out.reserve(s.length() + 4); // evite les reallocations repetees (chaque += peut recopier tout le buffer)
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
  webServer->send(200, "application/json", json);
}

static void handleWebConfigListPlaylists()
{
  String json = "[";
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
        json += "\"" + name + "\""; first = false;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  webServer->send(200, "application/json", json);
}

// Envoie un tableau JSON de strings a partir d'une liste "nom1,nom2,..."
// deja echappee (jsonEscape), qu'elle vienne d'un scan frais ou du cache SD.
//
// En dessous de SIMPLE_SEND_MAX_LEN : un seul webServer->send() avec le JSON
// complet deja construit -- exactement la methode de l'ancienne version du
// firmware (RecalBox_DMDv9_preclockv2), dont un test A/B reel sur le meme
// materiel/carte SD (2026-07-27) a confirme qu'elle ne montre AUCUN cout heap
// fixe mesurable (heap stable pendant tout le scan), contrairement au mode
// chunke ci-dessous qui coute a lui seul plusieurs Ko des son initialisation
// (setContentLength(CONTENT_LENGTH_UNKNOWN)+send(200,...,"") vide), quelle
// que soit la taille du contenu envoye ensuite -- confirme par plusieurs
// tests reels montrant une perte identique sur un dossier quasi vide ET sur
// des dossiers avec plusieurs dizaines d'entrees (donc un cout fixe par
// requete, pas proportionnel au contenu).
//
// Au-dessus de SIMPLE_SEND_MAX_LEN : repli sur l'encodage chunke
// (setContentLength(CONTENT_LENGTH_UNKNOWN) + sendContent() par petits
// morceaux) -- necessaire pour les tres gros dossiers, ou le JSON complet
// peut depasser ce que WebServer::send() sait transmettre en un seul envoi
// (le client recoit alors moins d'octets que le Content-Length annonce,
// d'ou net::ERR_CONTENT_LENGTH_MISMATCH cote navigateur, confirme en test
// reel le 2026-07-25 sur des dossiers avec de nombreux fichiers -- les
// petits dossiers ne declenchaient jamais ce depassement). Le cout fixe du
// mode chunke devient alors un compromis acceptable face au risque de
// depassement d'un envoi simple sur un contenu volumineux.
static const size_t SIMPLE_SEND_MAX_LEN = 4096;

static void sendJsonArrayFromCommaList(const String &names)
{
  if (names.length() < SIMPLE_SEND_MAX_LEN) {
    String json;
    json.reserve(names.length() + 16);
    json += "[";
    bool first = true;
    int start = 0;
    while (start <= (int)names.length()) {
      int comma = names.indexOf(',', start);
      String name = (comma < 0) ? names.substring(start) : names.substring(start, comma);
      if (name.length() > 0) { json += (first ? "\"" : ",\""); json += name; json += "\""; first = false; }
      if (comma < 0) break;
      start = comma + 1;
    }
    json += "]";
    webServer->send(200, "application/json", json);
    return;
  }
  webServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer->send(200, "application/json", "");
  webServer->sendContent("[");
  bool first = true;
  int start = 0;
  while (start <= (int)names.length()) {
    int comma = names.indexOf(',', start);
    String name = (comma < 0) ? names.substring(start) : names.substring(start, comma);
    if (name.length() > 0) { webServer->sendContent((first ? "\"" : ",\"") + name + "\""); first = false; }
    if (comma < 0) break;
    start = comma + 1;
  }
  webServer->sendContent("]");
  webServer->sendContent(""); // chunk final (taille 0) -- termine proprement l'encodage chunke
}

// Scan direct de /gifs (liste des dossiers), sans aucune persistance SD --
// voir changelog v74. Un seul passage FAT32, resultat accumule en RAM
// (String, proportionnelle au contenu -- limite connue, cf. filet de
// securite heap critique ci-dessous) puis envoye tel quel par l'appelant.
// Retourne false si le scan a du etre abandonne (heap critique) ; dans ce
// cas, l'appelant ne doit jamais presenter le contenu partiel comme complet
// (et le JS ne doit pas le mettre en sessionStorage).
static bool scanGifDirsRaw(String &outNames)
{
  outNames = "";
  outNames.reserve(512);
  bool first = true;
  bool aborted = false;
  unsigned long t0 = millis();
  File dir = SD.open("/gifs");
  if (dir && dir.isDirectory()) {
    int n = 0;
    File entry = dir.openNextFile();
    while (entry) {
      if (entry.isDirectory()) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);
        if (!first) outNames += ",";
        outNames += jsonEscape(name);
        first = false;
      }
      entry.close(); entry = dir.openNextFile();
      if ((++n % 20) == 0) delay(1);
      if ((n % 30) == 0) {
        webDmdPause("/gifs (" + String(n) + ")", 0x07E0);
        Serial.println("[WEB] scanGifDirsRaw : " + String(n) + " entrees vues, t=" + String(millis() - t0) + "ms");
      }
      // Filet de securite : abandonne proprement plutot que de risquer un
      // abort() par epuisement heap. getMaxAllocHeap() (plus grand bloc
      // contigu allouable) plutot que getFreeHeap() (total libre) : le
      // total libre seul sous-estime le risque d'echec d'allocation sur
      // un tas fragmente.
      if (ESP.getMaxAllocHeap() < 6000) {
        entry.close();
        aborted = true;
        break;
      }
    }
    dir.close();
  }
  Serial.println("[WEB] scanGifDirsRaw : termine, t=" + String(millis() - t0) + "ms" + (aborted ? " (ABANDON heap critique)" : ""));
  return !aborted;
}

// v85 -- decision utilisateur : retrait complet de la navigation/
// suppression de fichiers individuels dans un dossier depuis MEDIA. Tout
// le sous-systeme de cache /gifs par dossier (v75-v84 : machine a etats
// cooperative, format V5 horodate, exclusion sur budget de temps,
// detection de peremption) n'avait plus d'utilite -- il n'existait que
// pour rendre la LECTURE du contenu d'un dossier rapide/sure sur l'ESP32,
// fonctionnalite desormais retiree (remplacement envisage : composition
// de playlists personnalisees depuis l'outil PC, a etudier separement).
// handleWebConfigListGifDirs() (juste en dessous) reste base sur
// scanGifDirsRaw() seul (liste des NOMS de dossiers, toujours rapide,
// jamais mise en cache -- n'a jamais souffert de la degradation FAT32,
// qui ne touchait que l'enumeration du CONTENU d'un dossier).

static void handleWebConfigListGifDirs()
{
  String names;
  bool ok = scanGifDirsRaw(names);
  Serial.println("[WEB] lsgifdirs, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()) + (ok ? " (OK)" : " (heap critique, liste vide)"));
  webServer->client().setTimeout(5000);
  if (!ok) {
    webServer->send(503, "application/json", "[]");
    webServer->client().setTimeout(3000);
    return;
  }
  // v85 : simple tableau de noms (plus de statut cached/excluded -- le
  // cache par dossier qui portait cette notion a ete retire, voir plus
  // haut).
  sendJsonArrayFromCommaList(names);
  webServer->client().setTimeout(3000);
}

static void handleWebConfigGifCount()
{
  if (!webServer->hasArg("dir")) { webServer->send(400, "text/plain", "0"); return; }
  String dirName = webServer->arg("dir"); dirName.trim();
  int count = 0;
  File sub = SD.open(("/gifs/" + dirName).c_str());
  if (sub && sub.isDirectory()) {
    File f = sub.openNextFile();
    while (f) {
      if (!f.isDirectory() && String(f.name()).endsWith(".gif")) count++;
      f.close(); f = sub.openNextFile();
      delay(1);
    }
    sub.close();
  }
  webServer->send(200, "text/plain", String(count));
}

// Definie plus bas avec le cache g_plRefCache* ; declaree ici pour que
// handleWebConfigGeneratePlaylist()/handleWebConfigDeletePlaylist() puissent
// invalider le cache quand la liste des playlists change.
static void invalidatePlaylistRefCache();

static void handleWebConfigGeneratePlaylist()
{
  if (!webServer->hasArg("name") || !webServer->hasArg("dirs")) {
    webServer->send(400, "text/plain", "ERR: manque nom ou dirs"); return;
  }
  String name = webServer->arg("name");
  String dirs = webServer->arg("dirs");
  String outputPath = "/playlists/" + name + ".txt";
  if (!SD.exists("/playlists")) SD.mkdir("/playlists");
  if (SD.exists(outputPath.c_str())) SD.remove(outputPath.c_str());
  File outf = SD.open(outputPath.c_str(), FILE_WRITE);
  if (!outf) { webServer->send(500, "text/plain", "ERR: ecriture impossible"); return; }
  String buf;
  // Compter d'abord le nombre de dossiers pour la progression
  int totalDirs = 1, processedDirs = 0;
  for (int i = 0; i < dirs.length(); i++) if (dirs.charAt(i) == ',') totalDirs++;
  int totalGifs = 0, startIdx = 0;
  while (true) {
    int comma = dirs.indexOf(',', startIdx);
    String dirName = (comma < 0) ? dirs.substring(startIdx) : dirs.substring(startIdx, comma);
    dirName.trim();
    if (dirName.length() > 0) {
      processedDirs++;
      webDmdPause("Scan: " + dirName + " (" + String(processedDirs) + "/" + String(totalDirs) + ")", 0x07E0);
      File sysDir = SD.open(("/gifs/" + dirName).c_str());
      if (sysDir && sysDir.isDirectory()) {
        File f = sysDir.openNextFile();
        while (f) {
          String fname = String(f.name());
          if (!f.isDirectory() && fname.endsWith(".gif")) {
            buf += "/gifs/" + dirName + "/" + fname + "\n";
            totalGifs++;
            if (buf.length() > 4000) { outf.print(buf); buf = ""; }
          }
          f.close(); f = sysDir.openNextFile();
          delay(1);
        }
      }
      if (sysDir) sysDir.close();
    }
    if (comma < 0) break;
    startIdx = comma + 1;
    delay(1);
  }
  if (buf.length() > 0) outf.print(buf);
  outf.close();
  invalidatePlaylistRefCache();
  webDmdPause("Playlist creee: " + String(totalGifs) + " GIFs", 0x07E0);
  String msg = "OK: " + String(totalGifs) + " GIFs ajoutes dans la playlist " + name + ".txt";
  Serial.println("[WEB] " + msg);
  webServer->send(200, "text/plain", msg);
}

static void handleWebConfigDeletePlaylist()
{
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

// Cache RAM : pour g_plRefCacheFolder, liste (CSV) des playlists .txt qui
// referencent deja ce dossier. Invalide (chaine vide) a la creation ou
// suppression d'une playlist -- voir invalidatePlaylistRefCache().
static String g_plRefCacheFolder = "";
static String g_plRefCachePlaylists = "";

static void invalidatePlaylistRefCache() { g_plRefCacheFolder = ""; g_plRefCachePlaylists = ""; }

// Version "lot" : traite plusieurs fichiers du MEME
// dossier en un seul appel. Meme dossier => memes playlists concernees (via
// g_plRefCache*), et surtout chaque playlist candidate n'est lue qu'UNE
// FOIS (au lieu d'une fois par fichier du lot) pour verifier quels fichiers
// y sont deja presents, puis tous les fichiers manquants sont ajoutes en un
// seul SD.open(FILE_APPEND). Appelee par le JS (uploadGif()) une seule fois
// a la fin de tout un lot d'upload, plutot que addFileToPlaylists() a
// chaque fichier individuel (cout mesure : verification "deja present"
// relisait la playlist entiere pour chaque fichier meme avec le cache
// "quelles playlists referencent ce dossier").
static void handleWebConfigAddToPlaylistsBatch()
{
  if (!webServer->hasArg("dir") || !webServer->hasArg("files")) { webServer->send(200, "text/plain", "OK:0"); return; }
  String folder = webServer->arg("dir"); folder.trim();
  String filesArg = webServer->arg("files");
  if (folder.length() == 0 || filesArg.length() == 0) { webServer->send(200, "text/plain", "OK:0"); return; }

  if (g_plRefCacheFolder != folder) {
    g_plRefCacheFolder = folder;
    g_plRefCachePlaylists = "";
    // Detection "quelle playlist reference ce dossier" : lecture bufferisee
    // complete (readString(), deja utilisee plus bas dans cette meme
    // fonction pour la verification de doublons) + un seul indexOf(), au
    // lieu de la version precedente qui relisait CHAQUE playlist ligne par
    // ligne (readStringUntil('\n') + delay(1) PAR LIGNE) -- tres lent en
    // conditions reelles des qu'une playlist contient beaucoup d'entrees
    // (signale par l'utilisateur, silencieux en plus : aucun retour tant
    // que cette phase durait).
    String needle = "/gifs/" + folder + "/";
    File plDir = SD.open("/playlists");
    if (plDir && plDir.isDirectory()) {
      File entry = plDir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;
        if (!entry.isDirectory() && base.endsWith(".txt")) {
          String plPath = "/playlists/" + base;
          File pl = SD.open(plPath.c_str());
          bool found = false;
          if (pl) {
            String content = pl.readString();
            pl.close();
            found = content.indexOf(needle) >= 0;
          }
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
  }

  int totalAppended = 0;
  int pstart = 0;
  while (pstart <= (int)g_plRefCachePlaylists.length()) {
    int pcomma = g_plRefCachePlaylists.indexOf(',', pstart);
    String base = (pcomma < 0) ? g_plRefCachePlaylists.substring(pstart) : g_plRefCachePlaylists.substring(pstart, pcomma);
    if (base.length() > 0) {
      String plPath = "/playlists/" + base;
      String existing;
      File pl = SD.open(plPath.c_str());
      if (pl) { existing = pl.readString(); pl.close(); } // readString() bufferise, evite la fragmentation d'une concatenation octet-par-octet sur une grosse playlist
      String existingPadded = "\n" + existing;
      if (!existingPadded.endsWith("\n")) existingPadded += "\n";
      String toAppend;
      int fstart = 0;
      while (fstart <= (int)filesArg.length()) {
        int fcomma = filesArg.indexOf(',', fstart);
        String fname = (fcomma < 0) ? filesArg.substring(fstart) : filesArg.substring(fstart, fcomma);
        fname.trim();
        if (fname.length() > 0) {
          String gifPath = "/gifs/" + folder + "/" + fname;
          if (existingPadded.indexOf("\n" + gifPath + "\n") < 0) {
            toAppend += gifPath + "\n";
            totalAppended++;
          }
        }
        if (fcomma < 0) break;
        fstart = fcomma + 1;
      }
      if (toAppend.length() > 0) {
        File plApp = SD.open(plPath.c_str(), FILE_APPEND);
        if (plApp) { plApp.print(toAppend); plApp.close(); }
      }
    }
    if (pcomma < 0) break;
    pstart = pcomma + 1;
  }
  Serial.println("[WEB] add-to-playlists-batch: dir=" + folder + " -> " + String(totalAppended) + " ajout(s)");
  webServer->send(200, "text/plain", "OK:" + String(totalAppended));
}

// Cree /gifs/<dir> si absent, en route dediee (idempotente, appelee par le
// JS AVANT le premier fichier d'un upload). Decouple la creation de dossier
// du chemin critique de l'upload multipart : le workaround (mkdir + creer/
// supprimer un fichier temoin, necessaire pour eviter un attribut lecture
// seule sur certaines cartes SD) restait auparavant dans
// UPLOAD_FILE_START -- meme avec le timeout client elargi a 15s (v34), le
// navigateur continuait a signaler ERR_CONNECTION_RESET en test reel sur un
// nouveau dossier. En le sortant du multipart, cette requete a son propre
// budget de temps (elle n'est pas concurrente d'un flux de donnees fichier
// en cours de reception) et le dossier existe deja quand l'upload demarre
// vraiment.
static void handleWebConfigCreateFolder()
{
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
  // Meme filet de securite que UPLOAD_FILE_START : refuser proprement sur
  // un tas critique plutot que de risquer un abort().
  if (ESP.getMaxAllocHeap() < 6000) {
    webServer->client().setTimeout(3000);
    Serial.println("[WEB] create-folder refuse (heap critique, maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
    webServer->send(503, "text/plain", "ERR: heap critique, reessayez");
    return;
  }
  // NOTE : esp_task_wdt_reset() avait ete ajoute ici par precaution (theorie
  // du Task Watchdog sur mkdir lent) mais s'est revele actif erroner en test
  // reel ("task not found" en boucle, cf. changelog v44) -- la tache qui
  // traite les requetes web n'est en fait pas enregistree aupres du TWDT.
  // Retire : n'apportait aucun benefice et ajoutait un vrai cout (log
  // d'erreur repete).
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
  if (uploadFile) { uploadFile.close(); uploadFile = File(); }
  if (uploadSuccess) {
    uploadSuccess = false;
    String msg = "OK: fichier uploade dans /gifs/" + uploadDir;
    Serial.println("[WEB] " + msg);
    webServer->send(200, "text/plain", msg);
  } else {
    // Ne JAMAIS appeler webServer->send() depuis handleWebConfigUploadFile()
    // (callback UPLOAD_FILE_*) : le client est encore en train d'envoyer le
    // corps multipart a ce moment-la, et une reponse prematuree casse la
    // connexion HTTP en cours -- observe en test reel comme "erreur reseau"
    // cote navigateur, specifiquement lors de l'upload vers un dossier a
    // creer (chemin avec plus d'etapes SD synchrones avant l'ouverture du
    // fichier). Seul ce handler, appele une fois le corps entierement
    // consomme, a le droit d'envoyer une reponse.
    String msg = uploadErrorMsg.length() ? uploadErrorMsg : "ERR: aucun fichier recu";
    uploadErrorMsg = "";
    webServer->send(400, "text/plain", msg);
  }
}

static void handleWebConfigUploadFile()
{
  HTTPUpload &upload = webServer->upload();
  if (upload.status == UPLOAD_FILE_START) {
    // Timeout client elargi (defaut lib WebServer ~3s) le temps de l'upload :
    // les operations SD synchrones ci-dessous (mkdir + creation/suppression
    // d'un fichier temoin sur un dossier a creer, ecritures sous charge) le
    // depassent facilement -> la lib coupe alors la connexion, vu cote
    // navigateur comme ERR_CONNECTION_RESET/TIMED_OUT (confirme en test reel
    // le 2026-07-25, F12). Remis a une valeur courte des la fin/l'abandon de
    // l'upload. Meme correctif que celui documente le 2026-07-21, perdu lors
    // du fractionnement en pages du 2026-07-23.
    webServer->client().setTimeout(15000);
    uploadSuccess = false;
    uploadErrorMsg = "";
    // Refus propre plutot qu'un crash : sur un tas deja fragmente (upload
    // en masse avec plusieurs operations precedentes -- suppressions,
    // listings, retries), une allocation qui echoue plus loin dans ce
    // handler declenche abort() (exceptions C++ desactivees sur Arduino
    // ESP32) et fait rebooter le DMD -- confirme en test reel (backtrace
    // abort() apres plusieurs "Upload aborted" consecutifs). Mieux vaut
    // refuser explicitement ce fichier avec un message clair : le JS
    // retentera (jusqu'a 3x) et le prochain essai aura peut-etre plus de
    // marge si le tas s'est un peu detendu entre-temps.
    if (ESP.getMaxAllocHeap() < 6000) {
      uploadErrorMsg = "ERR: heap critique, reessayez";
      Serial.println("[WEB] Upload refuse (heap critique, maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
      return;
    }
    uploadDir = webServer->arg("dir");
    uploadDir.trim();
    if (uploadDir.length() == 0) {
      uploadErrorMsg = "ERR: dossier cible manquant";
      return;
    }
    String filename = upload.filename;
    { int p = filename.lastIndexOf('/'); if (p >= 0) filename = filename.substring(p + 1); }
    { int p = filename.lastIndexOf('\\'); if (p >= 0) filename = filename.substring(p + 1); }
    if (filename.length() == 0) { uploadErrorMsg = "ERR: nom fichier invalide"; return; }
    String path = "/gifs/" + uploadDir + "/" + filename;
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
    if (uploadFile) {
      uploadFile.close();
      uploadFile = File();
      uploadSuccess = true;
      unsigned long dt = millis() - uploadStartMs;
      Serial.println("[WEB] Upload done: " + String(uploadTotalBytes) + " bytes in " + String(dt) + "ms");
      // Mise a jour des playlists PLUS appelee ici par fichier -- le JS
      // (uploadGif()) appelle desormais /add-to-playlists-batch UNE SEULE
      // FOIS a la fin de tout le lot, avec la liste des fichiers uploades
      // avec succes. Meme dossier => memes playlists concernees : inutile
      // de rescanner toutes les playlists a chaque fichier individuel (le
      // cout mesure precedemment, cf. cache g_plRefCache*, restait par
      // fichier meme avec le cache "quelles playlists referencent ce
      // dossier" -- seule la verification "deja present" etait encore
      // faite par fichier). Meme principe desormais pour l'invalidation du
      // cache /gifs : faite UNE SEULE fois par lot, dans
      // handleWebConfigAddToPlaylistsBatch() (appelee a la fin du lot), pas
      // ici a chaque fichier individuel.
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    webServer->client().setTimeout(3000);
    if (uploadFile) { uploadFile.close(); uploadFile = File(); }
    Serial.println("[WEB] Upload aborted");
  }
}

static void handleWebConfigSave()
{
  // "brightness" n'est plus obligatoire : chaque page (BASIC/NETWORK/
  // CLOCK/MEDIA) n'envoie que SES propres champs a /save -- l'exiger
  // systematiquement (herite de l'ancienne page unique, ou tous les
  // champs etaient dans le meme formulaire) faisait echouer TOUTE
  // sauvegarde depuis NETWORK/CLOCK/MEDIA avec "missing params". Meme
  // pattern hasArg() que tous les autres champs ci-dessous.
  if (webServer->hasArg("brightness")) {
    int b = webServer->arg("brightness").toInt();
    if (b >= 0 && b <= 100) screenBrightness = map(b, 0, 100, 0, 255);
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

  // Recalcule depuis screenBrightness (valeur persistee, mise a jour ou
  // non ci-dessus selon que "brightness" etait present) -- meme formule
  // que handleWebConfigLoad(), jamais une variable locale non initialisee
  // qui aurait ecrit brightness=0 dans config.ini si la page appelante
  // n'envoyait pas ce champ (ecran DMD noir).
  int b = (screenBrightness * 100 + 127) / 255;

  File f = SD.open("/config.ini", FILE_WRITE);
  if (!f) { webServer->send(500, "text/plain", "ERR: SD write failed"); return; }
  f.println("# Info"); f.println("info=" + String(showInfo ? "1" : "0"));
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
  f.println(); f.println("first_boot=0");
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

static void handleWebConfigDeleteFolders()
{
  if (!webServer->hasArg("dirs")) { webServer->send(400, "text/plain", "ERR: missing dirs"); return; }
  String dirs = webServer->arg("dirs");
  int count = 0, fail = 0, start = 0;
  while (true) {
    int comma = dirs.indexOf(',', start);
    String d = (comma < 0) ? dirs.substring(start) : dirs.substring(start, comma);
    d.trim();
    if (d.length() > 0) {
      String path = "/gifs/" + d;
      if (SD.exists(path.c_str())) {
        Serial.println("[WEB] deleteFolder start: " + path);
        if (deleteFolderRecursive(path)) { count++; Serial.println("[WEB] deleteFolder OK: " + path); }
        else { fail++; Serial.println("[WEB] deleteFolder FAIL: " + path); }
      } else {
        Serial.println("[WEB] deleteFolder introuvable: " + path);
      }
    }
    if (comma < 0) break;
    start = comma + 1;
  }
  String msg = "OK: " + String(count) + " supprime(s)" + (fail>0?", " + String(fail) + " echec(s)":"");
  webServer->send(200, "text/plain", msg);
}

// v85 : handleWebConfigDeleteFiles() (suppression de fichiers INDIVIDUELS
// dans un dossier) et sa route /delete-files retirees -- decision
// utilisateur de supprimer la navigation/suppression de fichiers
// individuels depuis MEDIA (voir plus haut). La suppression de DOSSIERS
// ENTIERS (handleWebConfigDeleteFolders(), ci-dessus) reste disponible.

// Forward declaration : definie plus bas (juste avant handleWebConfigRoot,
// qui l'utilise aussi), mais appelee ici par handleDmdOpen() -- sans cette
// declaration, erreur de compilation "not declared in this scope".
static bool triggerWebConfigMode(const String &msg);

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
  if (!triggerWebConfigMode(msg)) return; // reboot cible deja declenche, reponse deja envoyee
  webServer->send(200, "text/plain", "OK " + full);
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
  webServer->send(200, "application/json", "{\"language\":\"" + uiLanguage + "\"}");
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
  writeConfigFlag("first_boot", "0");
  Serial.println("[WEB] AP save: fichier ecrit avec SSID=" + wifiSSID + " -> reboot");
  webServer->send(200, "text/plain", "OK");
  delay(1000);
  ESP.restart();
}

static void sendRebootingPage()
{
  // Page volontairement generee en C++ (pas de bloc PROGMEM/gzip) : tres
  // courte, contenu dynamique selon uiLanguage, inutile de passer par le
  // pipeline de generation gzip pour ca.
  // Poll JS (fetch + catch) plutot qu'un simple <meta refresh> : pendant la
  // fenetre ou l'ESP32 redemarre reellement, une navigation classique
  // (meta refresh) tomberait sur une erreur de connexion et le navigateur
  // afficherait sa page d'erreur native -- laquelle n'a plus notre balise
  // refresh, plus aucune nouvelle tentative automatique ensuite. Le fetch()
  // echoue silencieusement (catch) sans jamais quitter cette page tant que
  // le serveur ne repond pas, puis recharge des le premier succes reel.
  String msg = "Redemarrage du DMD en cours, veuillez patienter...";
  if (uiLanguage == "en") msg = "DMD rebooting, please wait...";
  else if (uiLanguage == "es") msg = "Reiniciando el DMD, por favor espere...";
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>RecalBox DMD</title>"
    "<style>body{font-family:sans-serif;background:#1a1a2e;color:#eee;display:flex;"
    "align-items:center;justify-content:center;height:100vh;margin:0;text-align:center}</style>"
    "</head><body><div>" + msg + "</div>"
    "<script>function poll(){fetch(location.href,{cache:'no-store'}).then(function(r){"
    "if(r.ok)location.reload();else setTimeout(poll,1500);"
    "}).catch(function(){setTimeout(poll,1500);});}"
    "setTimeout(poll,1500);</script>"
    "</body></html>";
  webServer->send(200, "text/html", html);
}

// v88 -- REINTRODUIT (retire en v85, ramene suite a un test reel) : le
// simple LISTING DES NOMS de dossiers (/lsgifdirs, utilise par BASIC ET
// MEDIA desormais, pas seulement l'ancien cache par fichier) echoue sans
// ce reboot -- mettre en pause une lecture GIF en cours (webDmdPause()
// juste en dessous) fragmente fortement le heap A ELLE SEULE (maxalloc
// mesure s'effondrant de ~13-18 Ko a ~4,6 Ko alors que le heap LIBRE total
// augmente au meme moment -- pure fragmentation, pas un manque de
// memoire), quelle que soit la duree de lecture avant l'ouverture de la
// page. Reboot desormais SYSTEMATIQUE (toutes les pages declenchent la
// meme logique, plus de parametre allowReboot -- BASIC a autant besoin de
// heap contigu pour son listing que MEDIA).
static bool triggerWebConfigMode(const String &msg)
{
  if (!g_playlistStartedThisBoot) {
    // Rien n'a ete lance depuis le boot (playlist deja sautee -- AP/premier
    // boot/secours WiFi, ou reboot precedent deja cible sur ce chemin) : le
    // heap est deja au maximum disponible, pas besoin de rebooter encore.
    if (g_sdOpInProgress) {
      // Le DMD est deja en mode config (page precedente de la meme
      // session, navigation directe entre sous-pages via la barre de nav).
      // Reappliquer webDmdPause()/webDmdSetMainMsg() avec les MEMES
      // valeurs (meme IP, meme message) ne changerait rien a l'affichage
      // mais reassignerait des String et redeclencherait un redraw pour
      // rien -- pur gaspillage de heap a un moment ou il est deja rare.
      clearFirstBoot();
      return true;
    }
    String ip = WiFi.localIP().toString();
    clearFirstBoot();
    webDmdSetMainMsg(msg);
    webDmdPause(ip, 0xFFE0);
    // Message "de fond" auquel revenir automatiquement apres un message de
    // statut transitoire (voir SD_OP_SUBMSG_EXPIRE_MS, RecalBox_DMD.ino).
    g_sdOpPersistentSubMsg = ip;
    g_sdOpPersistentSubMsgColor = 0xFFE0;
    Serial.println("[WEB] triggerWebConfigMode (pas de reboot), heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
    return true;
  }
  // La playlist/des GIFs ont deja tourne ce boot -- chaque GIF ouvert perd
  // durablement quelques Ko de heap (jamais recupere avant reboot), et
  // meme mettre en pause LE SEUL GIF deja ouvert fragmente fortement le
  // heap (confirme en test reel, v88). Plutot que d'entrer en mode config
  // avec un heap deja fragmente, rebooter directement et sauter la
  // playlist sur ce prochain boot (g_skipPlaylistForConfig, RecalBox_DMD.ino)
  // pour repartir avec le maximum de heap disponible.
  Serial.println("[WEB] triggerWebConfigMode: playlist deja active -> reboot cible mode config");
  writeConfigFlag("force_config_boot", "1");
  sendRebootingPage();
  requestReboot = true;
  return false; // reboot deja declenche, reponse deja envoyee -- l'appelant doit s'arreter la
}

static void sendGzipHtml(const uint8_t *content, size_t len)
{
  webServer->sendHeader("Content-Encoding", "gzip");
  webServer->send_P(200, "text/html", reinterpret_cast<PGM_P>(content), len);
}

static void handleWebConfigRoot()
{
  if (!triggerWebConfigMode("WEB DMD CONFIG")) return; // reboot cible deja declenche, reponse deja envoyee
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    sendGzipHtml(WEB_CONFIG_AP_HTML_GZ, WEB_CONFIG_AP_HTML_GZ_LEN);
  } else {
    sendGzipHtml(WEB_CONFIG_MENU_HTML_GZ, WEB_CONFIG_MENU_HTML_GZ_LEN);
  }
}

static void handleWebConfigBasicPage()
{
  if (!triggerWebConfigMode("WEB DMD CONFIG")) return; // reboot cible deja declenche, reponse deja envoyee
  sendGzipHtml(WEB_CONFIG_BASIC_HTML_GZ, WEB_CONFIG_BASIC_HTML_GZ_LEN);
}

static void handleWebConfigNetworkPage()
{
  if (!triggerWebConfigMode("WEB DMD CONFIG")) return; // reboot cible deja declenche, reponse deja envoyee
  sendGzipHtml(WEB_CONFIG_NETWORK_HTML_GZ, WEB_CONFIG_NETWORK_HTML_GZ_LEN);
}

static void handleWebConfigClockPage()
{
  if (!triggerWebConfigMode("WEB DMD CONFIG")) return; // reboot cible deja declenche, reponse deja envoyee
  sendGzipHtml(WEB_CONFIG_CLOCK_HTML_GZ, WEB_CONFIG_CLOCK_HTML_GZ_LEN);
}

static void handleWebConfigMediaPage()
{
  // v88 : reboot cible reintroduit de facon systematique (voir triggerWebConfigMode())
  // -- BASIC scanne aussi la SD (/lsgifdirs, generation de playlist) et souffre de la
  // meme fragmentation heap au moment de webDmdPause(), donc MEDIA garde la meme marge.
  if (!triggerWebConfigMode("WEB DMD CONFIG")) return; // reboot cible deja declenche, reponse deja envoyee
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
  webServer->on("/gifcount", handleWebConfigGifCount);
  webServer->on("/generate-playlist", HTTP_POST, handleWebConfigGeneratePlaylist);
  webServer->on("/delete-playlist", HTTP_POST, handleWebConfigDeletePlaylist);
  webServer->on("/create-folder", HTTP_POST, handleWebConfigCreateFolder);
  webServer->on("/upload", HTTP_POST, handleWebConfigUpload, handleWebConfigUploadFile);
  webServer->on("/delete-folders", HTTP_POST, handleWebConfigDeleteFolders);
  webServer->on("/scan-wifi", handleWebConfigScanWiFi);
  webServer->on("/save-ap", HTTP_POST, handleWebConfigSaveAP);
  webServer->on("/lang", handleWebConfigLang);
  webServer->on("/save-language", HTTP_POST, handleWebConfigSaveLanguage);
  webServer->on("/add-to-playlists-batch", HTTP_POST, handleWebConfigAddToPlaylistsBatch);
  webServer->on("/dmd-pause", HTTP_POST, handleDmdPause);
  webServer->on("/dmd-resume", HTTP_POST, handleDmdResume);
  webServer->on("/dmd-open", HTTP_POST, handleDmdOpen);
  webServer->on("/save", HTTP_POST, handleWebConfigSave);
  webServer->on("/reboot", handleWebConfigReboot);
  webServer->begin();
  Serial.println("[WEB] Interface config sur http://" + WiFi.localIP().toString());
}

void handleWebConfig() { if (webServer) webServer->handleClient(); }

#endif
