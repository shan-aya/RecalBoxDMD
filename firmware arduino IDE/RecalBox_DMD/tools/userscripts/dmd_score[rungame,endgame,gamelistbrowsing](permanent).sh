#!/bin/ash
# v35 -- verrou anti-relance deplace ICI, tout en haut du fichier, AVANT le
# changelog -- meme motif/meme investigation que marquee.sh v27 (voir son
# commentaire complet) : ES relance ce script a chaque evenement
# gamelistbrowsing (present dans son propre nom de hooks), verrou deja en
# place mais verifie ~550 lignes plus loin -- reduit au minimum le travail
# fait par chaque relance dupliquee pendant une rafale de navigation.
# v41 -- extrait vers dmd_helpers/singleton_lock.sh, voir marquee.sh v43
# pour le detail complet (meme bloc duplique a l'identique dans les 3
# scripts, nettoyage differe puis repris le 2026-09-04).
. /recalbox/share/userscripts/dmd_helpers/singleton_lock.sh dmd_score 2>/dev/null || exit 1
# Pont MQTT hiscore FBNeo + infos/description jeu -> DMD (marquee/cmd/score,
# canal UNIQUE, architecture "DMD bete" v110 -- voir RecalBox_DMD.ino)
#
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v46
#
# v46 - 2026-09-05 - safe-modify - Placeholder niveau 3 (v43) : nom rang 1
#   corrige en "ShaN" (S/N majuscules) -- valeur exacte demandee par
#   l'utilisateur, remplace le "shan" tout minuscule pose par erreur en
#   v45 (malentendu sur la casse voulue, corrige apres plusieurs allers-
#   retours). Score rang 1 confirme "2026" (inchange).
#
# v45 - 2026-09-05 - safe-modify - Placeholder niveau 3 (v43) : nom rang 1
#   raccourci "shan_aya" -> 4 caracteres -- retour utilisateur sur materiel
#   reel, testee sur `badlands` (voir DECISIONS.md pour les 3 jeux de test
#   des 3 niveaux) : depassait legerement la largeur du DMD au rang 1
#   (police agrandie a ce rang, voir RecalBox_DMD.ino v111 -- seul le
#   SCORE y est agrandi, mais le NOM y reste a la taille normale deja
#   etroite des conventions hi-score classiques, ex. 3 lettres "MAA"/"CAP"
#   -- 8 caracteres ne rentrait pas). Rang 2 ("RecalBox 2026") inchange,
#   reste a taille normale sans agrandissement, pas de probleme de largeur
#   signale a ce rang.
#
# v44 - 2026-09-04 - safe-modify - build_score_payload() : niveau 2
#   intercale entre le .hi reel et le placeholder v43 -- table VERIFIEE
#   MANUELLEMENT (dmd_helpers/verified_default_scores.json, lookup via
#   dmd_helpers/dmd_hiscore_verified.py) pour un jeu ou meme une vraie
#   partie credit+jouee jusqu'au game over (percee credit MAME de ce
#   soir, voir commit "insertion credit reelle via impulsion courte
#   uinput") ne produit aucun .hi -- aucune adresse hiscore.dat ne
#   s'arme pour ce jeu specifiquement. L'ecran hi-score interne du jeu
#   est alors lu directement sur une capture d'ecran reelle (methode
#   "verite d'abord", jamais invente) et enregistre via
#   add_verified_score.py. Cascade finale : (1) .hi reel, toujours
#   verifie en premier -- (2) table verifiee manuellement -- (3)
#   placeholder generique shan_aya/RecalBox. Chaque niveau cede
#   automatiquement sa place au precedent des qu'il devient disponible,
#   sans rien a nettoyer (dmd_hiscore_generic.py est toujours appele en
#   premier, $topn n'est plus vide des qu'un vrai .hi existe). Demande
#   utilisateur explicite ("il faudrait qu'il cede sa place si un .hi
#   existe vraiment"), scope volontairement restreint a `fbneo|mame*`
#   (meme raison que v43 : aucun equivalent hiscore.dat/.hi cote
#   consoles).
#
# v43 - 2026-09-04 - safe-modify - build_score_payload() : repli
#   PLACEHOLDER pour les jeux arcade (fbneo/mame*) sans .hi reel encore
#   disponible -- demande utilisateur explicite ("on cree un tableau
#   factice... affiche si il n'y a pas de .hi a afficher"), en reponse
#   directe a l'idee ecartee juste avant (creer un faux .hi SUR DISQUE) :
#   teste en direct ce soir sur `asteroid` (mame0278, deja peuple) --
#   remplacer son .hi par un contenu bidon PUIS relancer le jeu (meme
#   cycle chargement+fast-forward+quit que la campagne de recolte) NE
#   L'ECRASE PAS (mtime/contenu inchanges). Le plugin hiscore MAME
#   n'a donc pas de logique "si le fichier existe deja, le rafraichir au
#   prochain chargement" -- un faux .hi ecrit sur disque bloquerait
#   durablement un vrai .hi futur derriere lui, sans certitude qu'une
#   VRAIE partie jouee le debloquerait non plus (non teste, aurait
#   demande un mecanisme de pilotage MAME pas encore construit).
#   Solution retenue, bien plus simple et sans risque : repli PUREMENT
#   D'AFFICHAGE, ne touche JAMAIS le fichier .hi -- quand
#   dmd_hiscore_generic.py ne renvoie rien (jeu jamais peuple), affiche
#   une table factice "1 shan_aya 2026|2 RecalBox 2026" (demande
#   utilisateur explicite) au lieu de sauter silencieusement le panneau
#   hi-score. Des qu'un vrai .hi
#   est peuple par une campagne de recolte, ce placeholder disparait
#   tout seul au prochain appel (aucun etat a nettoyer, purement
#   stateless). Scope volontairement restreint a `fbneo|mame*` -- les
#   consoles n'ont pas d'equivalent hiscore.dat/.hi, un placeholder y
#   serait affiche indefiniment sans jamais se resoudre.
#
# v42 - 2026-09-04 - safe-modify - Cache mono-emplacement pour
#   dmd_game_info.py (voir get_game_info(), commentaire complet juste avant
#   extract_field()) : description et info l'invoquaient chacun separement
#   avec les MEMES arguments (sys/gpath) pour la MEME donnee statique --
#   double invocation python3 par rotation complete round-robin, evitable.
#   Piste remontee par la revue pre-merge master du 2026-09-03 (voir
#   DECISIONS.md, "Explicitement PAS fait"), reprise ici. Comportement de
#   sortie inchange (extract_field() recoit exactement le meme payload
#   qu'avant, cache ou pas) -- verifie via `sh -n`.
#
# v41 - 2026-09-04 - safe-modify - Verrou anti-relance extrait vers
#   dmd_helpers/singleton_lock.sh, voir marquee.sh v43 pour le detail
#   complet (meme bloc duplique a l'identique dans les 3 scripts, nettoyage
#   differe puis repris ce soir). Comportement au runtime inchange (meme
#   LOCKDIR "dmd_score_singleton", meme logique mkdir/pid/kill -0).
#
# v40 - 2026-09-02 - safe-modify - Veille CIBLEE (round-robin hiscore/
#   description/info pendant rundemo) DESACTIVEE, retour utilisateur
#   explicite : "la fonction veille ciblee n'est que cosmetique et ne pese
#   rien face au besoin de stabilite". rundemo REJOINT startgameclip dans
#   son bloc no-op (round-robin jamais demarre) au lieu de partager le
#   case gamelistbrowsing) -- meme demarche que v39 (fusion des topics) :
#   reduire l'exposition au blocage TX MQTT post-CONNACK (mur de plateforme
#   atteint, voir DECISIONS.md/memoire projet). gamelistbrowsing) ne traite
#   plus que la vraie navigation humaine, comportement inchange pour elle.
#   marquee.sh v41 applique le meme choix de son cote (rundemo/startgameclip
#   -> playlist simple) -- les 2 scripts restent coherents entre eux.
#
# v39 - 2026-09-02 - safe-modify - Topic marquee/cmd/score -> marquee/cmd
#   unique (voir DECISIONS.md + RecalBox_DMD.ino v148, meme motif que
#   marquee.sh v40/dmd_achievement.sh v4) : 12 topics fusionnes en 1 seul
#   cote DMD pour reduire l'exposition au blocage TX MQTT post-CONNACK
#   (jusqu'a 24 SUBSCRIBE en rafale par reconnexion avant ce fix, au-dessus
#   du plafond d'environ 16 segments TCP simultanement non-accuses trouve
#   dans le sdkconfig du core ESP32). send_score() (seul point d'envoi de
#   ce script) publie desormais "CMD=score ARG=@<duree>|<contenu>" au lieu
#   du contenu brut sur marquee/cmd/score. DEPLOIEMENT NON RETROCOMPATIBLE :
#   ce script ET RecalBox_DMD.ino doivent etre a jour EN MEME TEMPS.
#
# v38 - 2026-09-02 - safe-modify - round_robin() : "hiscore" retire de la
#   rotation (une seule verification par appel, pas a chaque tour) quand le
#   rom n'a pas de hi-score disponible -- evite de lui reserver un tour
#   complet (~14s) pour rien, retour utilisateur explicite : "si hi-score
#   n'existe pas il ne faut pas le compter dans les temps de rotation et
#   passer au panneau suivant". Trouve en creusant pourquoi certaines demos
#   ES n'affichaient jamais leur panneau description/info (delai cumule
#   mesure ~60-70s avant le 1er contenu reel, proche/superieur a la duree
#   reelle de nombreuses demos ~54-100s) -- voir DECISIONS.md et memoire
#   projet pour le detail des mesures (punisherh/tbowlp/zerowing1). Aucun
#   changement pour un rom qui A un hi-score disponible (chemin inchange).
#
# v37 - 2026-09-01 - safe-modify - DIAGNOSTIC (pas de changement de
#   comportement fonctionnel) pour l'investigation "desync overlay/marquee
#   entre marquee.sh et dmd_score.sh" (DECISIONS.md, piste de depart deja
#   actee : "instrumenter les 2 scripts pour logger leur reference de jeu...
#   et comparer precisement les timestamps"). Ajouts : precise_ts()
#   (centieme de seconde via /proc/uptime, "date" ash/busybox n'exposant pas
#   %N sur ce materiel -- verifie) sur les points de publication
#   (send_score(), desormais avec sa reference de jeu $exp deja disponible
#   chez tous ses appelants -- state_still_valid() valide deja cette meme
#   valeur juste avant) et sur les points de detection de changement de
#   position (BROWSE/DWELL settled/DWELL abandoned). Objectif : au prochain
#   episode de desync observe, pouvoir correler precisement (meme horloge
#   /proc/uptime que marquee.sh, meme machine, aucun decalage possible) le
#   moment ou CE script publie un panneau pour un jeu donne avec le moment
#   ou marquee.sh publie le marquee de fond pour un jeu different -- sans
#   quoi la piste actee ("state_file PARTAGE entre les 2 scripts" ou autre
#   fix structurel) resterait une hypothese non confirmee.
#
# v36 - 2026-09-01 - safe-modify - BUG REEL MAJEUR confirme sur materiel
#   (retour utilisateur : navigation turbo -> CPU sature 100% tous coeurs,
#   65C+, ralentissement RB1 generalise proportionnel a la duree de
#   navigation -- voir DECISIONS.md pour l'investigation complete). Cause :
#   dmd_hiscore_generic.py/dmd_game_info.py/dmd_challenge.py vivaient dans
#   /recalbox/share/userscripts/ EN MEME TEMPS que ce script -- Emulation
#   Station invoque NATIVEMENT tout fichier de ce dossier portant ces noms
#   precis (mecanisme distinct du notre, convention d'arguments differente),
#   a CHAQUE evenement gamelistbrowsing, SANS AUCUNE limite de frequence --
#   totalement independant de ce script. Confirme par test decisif : retrait
#   PHYSIQUE des 3 fichiers de userscripts/ (sans toucher a ce script) fait
#   disparaitre TOTALEMENT la surcharge, 99% CPU/65C -> 0%/45C. Fix : les 3
#   fichiers deplaces dans un sous-dossier dedie (PYHELP_DIR=
#   "${SCRIPT_DIR}/dmd_helpers", jamais scanne par le mecanisme natif ES) --
#   seul ce script continue de les invoquer (SCRIPT_DIR -> PYHELP_DIR sur
#   les 3 points d'appel python3). Voir aussi dmd_hiscore_generic.py v4,
#   dmd_game_info.py v10, dmd_challenge.py v2 (garde defensive ajoutee en
#   parallele, insuffisante seule mais gardee en defense en profondeur).
#
# v35 - 2026-08-31 - safe-modify - verrou anti-relance deplace tout en haut
#   du fichier (voir commentaire complet ci-dessus) -- reduit au minimum le
#   cout de chaque invocation dupliquee par EmulationStation pendant une
#   rafale de navigation. Meme investigation que marquee.sh v27.
#
# v34 - 2026-08-31 - safe-modify - v33 INSUFFISANT (retour utilisateur,
#   reproduit "parfois" -- pas systematique -- meme apres v33) : le hi-score/
#   desc/info du jeu PRECEDENT peut encore s'afficher 1 fois apres le
#   marquee du jeu suivant. v33 avait bien ferme la fenetre entre
#   round_robin() et l'ENTREE de publish_one_panel(), mais pas la fenetre la
#   PLUS LARGE -- celle qui s'ouvre PENDANT l'appel Python
#   (dmd_hiscore_generic.py/dmd_game_info.py) lui-meme, potentiellement le
#   point le plus lent du flux. Au retour de Python, le code enchainait
#   directement sur le tout PREMIER send_score() de chaque fonction de
#   pagination SANS revalider l'etat -- seules les pages SUIVANTES (apres un
#   sleep) etaient gardees par state_still_valid(), jamais la 1ere. Meme
#   trou dans la branche galaga/gyruss (valeur unique) de publish_hiscore(),
#   qui n'appelait meme pas state_still_valid() du tout. Fix : garde ajoutee
#   juste avant CHAQUE 1ere publication -- send_hiscore_paginated() (page 1),
#   send_paginated_lines() (1ere page), send_paginated() (1ere page), et
#   publish_hiscore() (branche valeur unique) -- ferme enfin la fenetre a
#   l'endroit ou elle est reellement la plus large.
#
# v33 - 2026-08-25 - safe-modify - BUG REEL confirme par retour utilisateur
#   direct ("on a 1 ecran de desc ou info de l'ancien jeu qui vient
#   s'afficher 1 fois apres le marquee du nouveau") -- race condition dans
#   round_robin()/publish_one_panel() : round_robin() verifie bien
#   state_still_valid() juste apres son sleep (avant de choisir le type de
#   panneau), mais publish_one_panel() enchaine ensuite directement sur
#   python3 dmd_game_info.py (peut prendre un instant, surtout sous charge
#   systeme -- voir chantier "fuite de zombies ES" du meme jour) PUIS
#   l'envoi, SANS revalider l'etat juste avant. Si le contexte change
#   pendant cet appel Python (nouveau jeu/navigation), l'ancienne boucle
#   round_robin() -- qui avait pourtant valide l'etat un instant plus tot --
#   envoie quand meme son contenu perime une fois, juste apres que le
#   nouveau marquee (publie par marquee.sh, immediat) soit deja affiche.
#   Fix : state_still_valid() ajoutee en tout premier dans
#   publish_one_panel(), avant l'appel Python et tout envoi -- ferme la
#   fenetre au point le plus lent/sensible du flux, reutilise la fonction
#   deja existante (v13), aucun nouveau mecanisme.
#
# v32 - 2026-08-24 - safe-modify - v31 CORRIGE (retour utilisateur, meme
#   session : "pour le mode veille demo de jeu on reste bcp plus longtemps
#   sur un jeu... le probleme est different, on peut laisser l'affichage
#   overlay sur ce mode specifique") -- rundemo RETIRE du groupe "marquee
#   seul", rejoint gamelistbrowsing (round-robin overlay complet, comme
#   avant v31). SEUL startgameclip reste en marquee-only desormais (duree
#   fixe ~30s, trop courte) -- rundemo (duree variable, generalement bien
#   plus longue : lancement + presentation avant affichage reel) garde le
#   comportement complet. Voir le commentaire pres du case startgameclip)
#   pour le raisonnement complet.
#
# v31 - 2026-08-24 - safe-modify - rundemo/startgameclip separes du case
#   gamelistbrowsing : ne declenchent plus le round-robin overlay (hiscore/
#   infos/description), marquee seul pendant la veille demo/gameclip. Voir
#   le commentaire complet pres du nouveau case rundemo|startgameclip)
#   plus bas -- raisonnement complet (fragilite MQTT keepalive + temps
#   d'affichage trop court pour une sequence complete) documente la.
#
# v30 - 2026-08-23 - safe-modify - BUG REEL RECURRENT corrige (retour
#      utilisateur explicite : "souci rencontre de multiples fois... la
#      methode est a revoir pour le transfert des reglages") :
#      features_watcher() ecrasait INCONDITIONNELLEMENT FEATURES_FILE avec
#      tout message recu sur marquee/status/features, meme tronque -- un
#      seul message corrompu suffisait a casser TOUS les panneaux d'info/
#      description/RA sur TOUS les jeux simultanement (feat_enabled()/
#      feat_value() ne trouvent alors plus aucune cle valide). Confirme en
#      direct : la valeur RETENUE sur le broker elle-meme etait deja
#      tronquee ("2;dwell_seconds=3" au lieu des 11 champs complets), donc
#      pas une corruption locale a ce script -- voir aussi RecalBox_DMD.ino
#      v126 (garde de sanite cote firmware AVANT publish, complementaire).
#      Fix : features_line_complete() valide la presence des 11 cles
#      attendues avant d'accepter un message -- tout message incomplet est
#      REJETE (logue, cache existant conserve tel quel) plutot qu'accepte
#      aveuglement.
#
#
# v29 - 2026-08-23 - safe-modify - "startgameclip" ajoute au case (retour
#   utilisateur : "il existe 1 mode demo : lance des jeux et un mode demo
#   video : lance des clips video de jeu" -- v28 n'avait cable que le
#   premier). Verifie en direct sur RB1 (screensaver.type=gameclip) :
#   evenement REEL "startgameclip", memes champs es_state.inf que rundemo.
#   Voir marquee.sh v19 pour le detail complet cote cablage/decouverte.
#
# v28 - 2026-08-23 - safe-modify - "rundemo" ajoute au case gamelistbrowsing
#   (retour utilisateur : "en mode clip & demo afficher marquee + panneaux
#   d'info equivalent au survol de liste du jeu concerne, au lieu de la
#   playlist"). Decouverte cote marquee.sh v18 (voir son changelog complet)
#   : le mode demo/clip de cette version d'ES publie l'evenement REEL
#   "rundemo" (SystemId/GamePath peuples dans es_state.inf exactement comme
#   pendant un survol de liste), pas "startgameclip" comme suppose avant.
#   "gamelistbrowsing|rundemo)" reutilise tel quel tout le mecanisme
#   dwell+round_robin("browse") existant -- aucune nouvelle logique.
#
# v27 - 2026-08-23 - safe-modify - BUG REEL corrige (retour utilisateur,
#   "je suis etonne car des la creation .hi ca semblait fonctionner" -- oui,
#   mais UNIQUEMENT pour fbneo) : le hi-score generique v26 etait en realite
#   TRIPLEMENT gate sur fbneo, jamais atteignable pour MAME malgre la
#   recolte mame0278 de cette nuit (~700+ .hi frais) : (1) publish_one_panel()
#   n'appelait publish_hiscore() QUE si $sys="fbneo", (2) build_score_payload()
#   ne recevait meme pas $sys et utilisait un chemin .hi fbneo EN DUR pour
#   TOUS les jeux (y compris le fallback generique), (3) le handler endgame
#   avait le meme gate fbneo-only. Les 3 corriges : $sys chaine desormais de
#   bout en bout (publish_one_panel -> publish_hiscore -> build_score_payload
#   -> dmd_hiscore_generic.py), les 3 decodeurs geres en dur (galaga/gyruss/
#   1941) restent strictement reserves a `$sys=fbneo` (format .hi
#   specifique), tout le reste (fbneo non special-case + MAME) passe par le
#   decodeur generique. Voir aussi dmd_hiscore_generic.py v2 (chemins MAME
#   corriges -- necessaire en complement, sans ca ce fix ne trouvait quand
#   meme aucun .hi MAME).
#
# v26 - 2026-08-23 - safe-modify - Phase 1 hi-score generique BRANCHEE
#   (voir memoire projet -- chantier destine a la communaute Recalbox,
#   pas un usage perso). build_score_payload() : le cas par defaut ("jeu
#   non supporte") tente desormais dmd_hiscore_generic.py (manifeste
#   ~2758 jeux arcade convertis depuis hi2txt-xml) AVANT d'abandonner --
#   les 3 cas geres en dur (1941/galaga/gyruss) restent INCHANGES, chemin
#   deja prouve, aucun risque de regression. Etend la couverture hi-score
#   a tout jeu FBNeo present dans le manifeste, sans toucher au code
#   existant.
#
# v25 - 2026-08-23 - safe-modify - 2 bugs reels corriges (retours
#   utilisateur successifs sur le meme test) :
#   (1) $state (BROWSE_STATE_FILE/expected) ne dependait que de system/rom,
#   jamais de LAST_SYSTEMBROWSING_ID -- revoir le MEME jeu (blazing star)
#   via la collection "challenges" PUIS via son vrai systeme (fbneo)
#   produisait la MEME chaine d'etat, donc l'ancien round_robin() (fige sur
#   LAST_SYSTEMBROWSING_ID="challenges" au moment de son fork -- un sous-
#   shell ne voit jamais les mises a jour ulterieures d'une variable du
#   parent) ne se terminait jamais et publiait le tableau challenge EN
#   PARALLELE de la nouvelle boucle normale. Fix : LAST_SYSTEMBROWSING_ID
#   injecte dans $state.
#   (2) BUG SEPARE, PLUS ANCIEN (present depuis v22, jamais exerce par la
#   sequence de test d'origine) : le handler "systembrowsing)" ne faisait
#   QUE mettre a jour LAST_SYSTEMBROWSING_ID, sans jamais invalider un
#   round_robin("browse") deja en vol depuis le DERNIER jeu survole avant
#   de remonter au niveau systeme -- retour utilisateur : "survol
#   challenge = description qui s'affiche... alors qu'on demande aucun
#   affichage, c'est un systeme". Fix : meme nettoyage BROWSE_STATE_FILE
#   que les autres handlers (rungame/sleep), applique ici aussi.
#   (3) 3e BUG REEL corrige (retour utilisateur : "jeu via challenge : pas
#   de RB CHALLENGE affiche, uniquement marquee") : challenge_session_active()
#   cherchait "--challenge" (DEUX tirets) alors que le vrai flag passe par
#   emulatorlauncher.pyc est "-challenge <manifeste>" (UN SEUL tiret) --
#   verifie en DIRECT sur le materiel pendant une vraie session Challenge.
#   Cette fonction n'a donc probablement JAMAIS detecte une vraie session
#   avant ce fix (le "test reel" cite en v18/v24 etait masque par le bug
#   d'auto-match de l'epoque). Voir commentaire complet pres de la fonction.
#
# v24 - 2026-08-22 - safe-modify - BUG REEL trouve (retour utilisateur :
#   "les highscore ne s'affichent pas sur 1941 en mode jeu") :
#   challenge_session_active() s'AUTO-MATCHAIT via son propre grep -- meme
#   piege que le pkill -f documente ailleurs (DECISIONS.md), mais version
#   "ps | grep" : `ps -o args -ww | grep -q -- '--challenge '` voit AUSSI,
#   dans la sortie de ps, la ligne de commande du grep lui-meme (qui
#   contient litteralement la chaine cherchee "--challenge " dans ses
#   propres arguments), donc la fonction retournait TOUJOURS vrai, meme
#   sans aucune session de challenge reelle en cours -- confirme en direct
#   sur le materiel (MATCH_TRUE constate meme processus/jeu tourne). Consequence :
#   round_robin() restait scotche sur types="challenge " en PERMANENCE des
#   qu'un jeu tournait, quel qu'il soit -- dmd_challenge.py echoue
#   silencieusement pour tout jeu qui n'est pas le challenge du mois (aucun
#   match dans current.json), donc AUCUN panneau ne s'affichait jamais en
#   jeu (ni hi-score, ni infos, ni description) en dehors du marquee.
#   Explique aussi pourquoi ca n'avait pas ete detecte avant : le
#   commentaire d'origine (v18) notait deja "PAS ENCORE VALIDE SUR
#   MATERIEL avec une vraie session de challenge active" -- le seul test
#   reel fait depuis (blazing star via le vrai menu Challenges) avait donc
#   coincidentellement un match a la fois "auto" ET "reel", masquant le
#   bug. Fix : astuce classique anti-auto-match (`[-]-challenge ` au lieu
#   de `--challenge ` -- meme chaine recherchee cote cible, mais absente
#   litteralement des arguments du grep lui-meme, donc plus d'auto-match
#   possible). Verifie en direct : MATCH_FALSE desormais hors session
#   challenge reelle.
#
# v23 - 2026-08-22 - safe-modify - CORRECTION de v22 ci-dessous, avant
#   deploiement final -- v22 supprimait TOUT affichage (marquee seul)
#   pendant la navigation dans le systeme virtuel "challenges" -- retour
#   utilisateur explicite avec la matrice complete attendue :
#     - navigation NIVEAU SYSTEME "challenges" (avant d'entrer) -> juste
#       le logo/marquee, comme n'importe quel systeme (deja le
#       comportement naturel, rien a coder : systembrowsing ne demarre
#       jamais de round-robin).
#     - navigation NIVEAU JEU DANS "challenges" (Blazing Star atteint via
#       la collection) -> afficher le TABLEAU CHALLENGE, pas rien.
#     - MEME jeu joue/navigue SOUS SON SYSTEME REEL (fbneo), PAS via
#       "challenges" -> comportement normal (hiscore/infos/description
#       selon la config), et SURTOUT PAS le tableau challenge.
#   round_robin() choisit desormais "challenge" comme type EXCLUSIF pour
#   le contexte "browse" quand LAST_SYSTEMBROWSING_ID == "challenges" (au
#   lieu de court-circuiter round_robin() entierement comme le faisait
#   v22) -- meme mecanisme que le cas "ingame" (challenge_session_active())
#   mais base sur le contexte de navigation plutot que sur un processus
#   actif. La garde challenge_session_active() dans publish_one_panel()
#   (cas "challenge") est RETIREE -- elle bloquait a tort ce nouveau cas
#   navigation (aucun processus de jeu ne tourne pendant une simple
#   navigation) -- round_robin() est desormais la SEULE source de verite
#   pour decider quand "challenge" est legitime dans la rotation.
#
# v22 - 2026-08-22 - safe-modify - CORRECTION de v21 ci-dessous, avant meme
#   deploiement reel -- l'approche v21 (is_challenge_game(), comparaison
#   system/rom contre current.json) etait FAUSSE : retour utilisateur
#   explicite "tu as pas complique les choses ?... si je suis dans fbneo
#   ou dans neo-geo ou dans favoris je veux que l'affichage du jeu soit
#   present quand je le survole" -- "RB CHALLENGE" est un SYSTEME VIRTUEL
#   distinct dans ES (collection "Challenges"), la demande initiale ne
#   visait QUE la navigation A L'INTERIEUR de ce systeme virtuel precis,
#   PAS le meme jeu rencontre via son systeme reel (fbneo) ou via
#   favoris. Verifie en DIRECT sur materiel (capture de /tmp/es_state.inf
#   pendant la navigation) : au niveau systeme, l'evenement systembrowsing
#   rapporte bien SystemId=challenges -- MAIS au niveau JEU (une fois DANS
#   la collection), l'evenement gamelistbrowsing rapporte le SystemId REEL
#   (fbneo), IDENTIQUE a une navigation normale -- aucun champ ne
#   distingue "atteint via Challenges" a ce niveau. is_challenge_game()
#   (comparaison system/rom brute) etait donc TROP LARGE : elle aurait
#   masque les panneaux du jeu du challenge meme via fbneo/favoris.
#   Fix : nouvelle variable LAST_SYSTEMBROWSING_ID, mise a jour par un
#   NOUVEAU cas explicite "systembrowsing)" (jamais gere avant, tombait
#   dans le cas par defaut "*)") -- retient le SystemId du DERNIER
#   evenement systembrowsing vu. gamelistbrowsing verifie desormais CETTE
#   variable (contexte de navigation memorise) au lieu de comparer le jeu
#   lui-meme -- suppression du round-robin browse UNIQUEMENT si on
#   navigue actuellement A L'INTERIEUR du systeme virtuel "challenges",
#   peu importe le jeu affiche.
#
# v20 - 2026-08-22 - safe-modify - Titre de l'ecran classement renomme
#   "CHALLENGE" -> "RB CHALLENGE" (demande utilisateur explicite) --
#   simple changement de libelle, aucun impact fonctionnel.
#
# v19 - 2026-08-22 - safe-modify - Retour utilisateur explicite APRES
#   confirmation que v18 fonctionne sur materiel ("parfait ca fonctionne")
#   : "en mode challenge je veux uniquement le tableau challenge + marquee
#   rien d'autre" -- round_robin() REMPLACE desormais (au lieu d'ajouter
#   a) la liste des types en jeu par "challenge" seul des qu'une VRAIE
#   session de challenge est active (challenge_session_active()) --
#   hi-score/infos/description restent actifs normalement des que ce
#   n'est plus le cas.
#
# v18 - 2026-08-22 - safe-modify - BUG REEL corrige AVANT que ca cause un
#   probleme reel (retour utilisateur explicite, juste apres deploiement
#   de v17 : "attention il ne faut pas que pour chaque lancement de
#   blazing star le challenge remplace le hiscore ! uniquement si on lance
#   blazing star en passant par le RB challenge") : v17 n'affichait le
#   classement que si system/rom correspondaient au challenge du mois
#   (dmd_challenge.py), mais ca aurait afficher le classement pour
#   N'IMPORTE QUEL lancement du jeu concerne (gamelist normale), pas
#   seulement une VRAIE session de challenge. Nouvelle fonction
#   challenge_session_active() (verifie qu'un processus en cours porte
#   l'argument "--challenge <manifeste>", signal trouve en lisant le code
#   source RB configgen/emulatorlauncher.py -- CET argument n'est present
#   QUE lors d'un lancement via le menu "Challenges" de ES) -- ajoutee
#   comme garde supplementaire dans publish_one_panel(), cas "challenge".
#   PAS ENCORE VALIDE SUR MATERIEL avec une vraie session de challenge
#   active (aucune en cours au moment d'ecrire ceci) -- a confirmer au
#   prochain test reel.
#
# v17 - 2026-08-22 - safe-modify - Prise en charge du "Challenge" Recalbox
#   du mois (demande utilisateur explicite : "gerer comme on gere les RA"
#   -- clarifie ensuite : afficher le classement COMMUNAUTAIRE EN LIGNE,
#   pas le score local, en round-robin avec le marquee EN JEU uniquement,
#   MAJ suivant le fichier local que RB rafraichit deja tout seul). Voir
#   memoire projet pour l'exploration complete du systeme officiel RB
#   (configgen/challenge/*, ScoreWatch.py, HiscoreTable.py) qui a mene a ce
#   design : le fichier local /recalbox/share/system/challenges/
#   current.json contient DEJA le classement en JSON pret a l'emploi --
#   AUCUN decodage de fichier de sauvegarde necessaire pour ce circuit
#   (contrairement au chantier hi-score FBNeo/MAME general, en attente
#   d'une reponse de l'equipe RB, MIS DE COTE en parallele -- voir
#   dmd_challenge.py pour le detail du format source). Nouveau fichier
#   `dmd_challenge.py` (meme famille que dmd_game_info.py) : lit
#   current.json, verifie que system/rom correspondent au challenge actif
#   (evite d'afficher un classement perime d'un mois precedent sur un
#   AUTRE jeu), formate les 9 premieres entrees en lignes "rang nom score"
#   -- meme convention que le rendu hi-score GENERIQUE deja gere par le
#   firmware (nom/score separes par le DERNIER espace, AUCUN changement
#   firmware necessaire). Nouvelle fonction publish_challenge() (reutilise
#   send_paginated_lines(), meme mecanisme que INFOS). "challenge" ajoute
#   a la rotation UNIQUEMENT en contexte "ingame" (round_robin(), pas dans
#   enabled_panel_types() qui reste inchangee -- evite de complexifier son
#   contrat pour les 2 contextes alors que "challenge" n'a de sens qu'en
#   jeu). TOUJOURS ACTIF, pas de reglage web dedie pour ce v1 (decision
#   utilisateur explicite "toujours actif" -- ~1 seul jeu/mois concerne,
#   overhead negligeable les autres jours puisque dmd_challenge.py ne
#   renvoie rien silencieusement si system/rom ne correspondent pas, meme
#   comportement que INFOS/DESCRIPTION quand ils sont vides).
#
# v16 - 2026-08-20 - safe-modify - send_paginated() REVUE suite a un
#   malentendu identifie par l'utilisateur sur v15 ci-dessous : v15
#   RECULAIT vers la derniere ponctuation DANS la fenetre de
#   LINES_PER_PAGE lignes (pouvait couper une page court, ex. 1 seule
#   ligne, si la ponctuation tombait tot). Retour utilisateur explicite :
#   "je prefererais aller chercher la ponctuation SUIVANTE [...] mais
#   bloquer en repli a 5 pages" -- design REFAIT : pagination par PHRASES
#   ENTIERES (decoupees via sed sur "./!/?" + espace, chaque phrase
#   enveloppee individuellement puis empilee sur la page en cours tant que
#   ca tient, page cloturee et une nouvelle demarree sinon) -- une phrase
#   n'est JAMAIS coupee sauf le seul cas ou elle est a elle seule trop
#   longue pour tenir sur une page entiere (inevitable, etalee sur
#   plusieurs pages). MAX_PAGES_DESCRIPTION=5 inchange (meme plafond de
#   securite). Voir commentaire complet sur send_paginated() pour le
#   detail de l'algorithme.
#
# v15 - 2026-08-20 - safe-modify - send_paginated() (DESCRIPTION) rendue
#   sensible aux FINS DE PHRASE, suite a une question explicite de
#   l'utilisateur ("capable de detecter des fin de paragraphe... pour pas
#   couper au milieu d'un paragraphe ou d'une phrase au minimum") + sa
#   confirmation ("oui avec une securite a 5 pages max"). Reponse apportee
#   AVANT implementation (voir echange) : les paragraphes sont deja
#   detruits en amont par dmd_game_info.py (tous les retours a la ligne du
#   XML normalises en simple espace) -- seule la fin de PHRASE (./!/?)
#   reste detectable a ce stade. Nouvel algorithme 2-phases (calcule TOUS
#   les points de coupure AVANT tout envoi, prefere couper sur la derniere
#   ligne d'une fenetre de LINES_PER_PAGE lignes se terminant par ./!/?,
#   repli sur coupe fixe sinon) + nouveau plafond DEDIE
#   MAX_PAGES_DESCRIPTION=5 (separe de MAX_PAGES=3 qui reste inchange pour
#   INFOS/hi-score -- ce decoupage variable est moins previsible que
#   l'ancien decoupage a taille fixe, ce plafond en borne le pire cas).
#   Voir commentaire complet sur send_paginated().
#
# v14 - 2026-08-20 - safe-modify - WRAP_WIDTH 24 -> 25, suite au retour
#   d'Org_01 vers TomThumb cote firmware (RecalBox_DMD.ino v118 -- retour
#   utilisateur explicite "l'espacement entre les mots est important et ca
#   rompt avec le style general" sur Org_01). TomThumb + espacement
#   inter-caractere manuel +1px (gfxCharAdvance(), voir RecalBox_DMD.ino)
#   = ~5px/caractere MAJUSCULE en moyenne, contre ~5px/caractere estime
#   (proportionnel) pour Org_01 -- valeur proche mais recalculee sur une
#   base fixe cette fois (128-1)/5=25.4. Valeur de DEPART, a ajuster apres
#   validation visuelle materiel comme les autres constantes ci-dessous.
#
# v13 - 2026-08-20 - safe-modify - BUG REEL corrige (retours utilisateur
#   explicites : "lors du passage en mode jeu, la description a poursuivi
#   sa page 2 et 3 avec le marquee intercale apres la page 1" + confirme
#   symetrique "pareil quand on quitte le jeu hiscore continue a s'afficher
#   1 fois lui aussi") : round_robin() ne verifiait l'etat de reference
#   (state_file/expected, deja utilise pour s'arreter ENTRE 2 tours de
#   types de contenu differents) qu'a CE niveau-la -- jamais ENTRE LES
#   PAGES d'un seul contenu multi-pages en cours d'envoi, puisque
#   send_paginated()/send_paginated_lines()/send_hiscore_paginated() ont
#   chacune leurs propres sleep() internes invisibles pour round_robin()
#   tant qu'elles n'ont pas fini de rendre la main. Une sequence
#   multi-pages deja lancee (ex. description 3 pages) continuait donc
#   jusqu'a sa fin meme apres un changement de contexte survenu
#   entre-temps (jeu lance/quitte, deplacement en navigation), le nouveau
#   round-robin demarrant en parallele -> melange des 2 sur le meme canal.
#   Fix : nouvelle fonction state_still_valid() (voir plus bas), le
#   parametre state_file/expected est desormais transmis en cascade
#   round_robin() -> publish_one_panel()/publish_hiscore() -> les 3
#   fonctions de pagination, verifie APRES chaque sleep() interne, AVANT
#   d'envoyer la page suivante -- une sequence en vol s'interrompt donc
#   desormais au plus tard 1 page (pas 1 sequence complete) apres le
#   changement de contexte. N'interrompt pas une page DEJA en cours
#   d'affichage (un sleep deja demarre ne peut pas etre annule de
#   l'exterieur en shell POSIX) mais l'empeche de continuer au-dela.
#   Les appels HORS round-robin (endgame -> publish_hiscore() sans
#   state_file) omettent le parametre -- state_still_valid() renvoie alors
#   toujours vrai, comportement ponctuel inchange.
#
# v12 - 2026-08-20 - safe-modify - 2 corrections suite retours utilisateur :
#   (1) Page 2 hi-score envoie desormais "HI-SCORE" comme titre (au lieu de
#   vide) -- le titre doit persister sur toutes les pages, voir
#   RecalBox_DMD.ino v116 pour le changement cote firmware associe
#   (distinction page1/page2 basee sur le contenu, plus sur le titre).
#   (2) SCORE_TIMER_MARGIN_MS (800ms) ajoute a la duree envoyee au
#   firmware (PAS au rythme d'envoi du script) -- corrige un flash bref du
#   marquee entre 2 pages d'un meme contenu, cause par le minuteur
#   firmware qui pouvait expirer legerement avant l'arrivee de la page
#   suivante (latence reseau + traitement du script).
#
# v11 - 2026-08-20 - safe-modify - WRAP_WIDTH 21 -> 30 pour DESCRIPTION,
#   suite au passage a la police compacte TomThumb cote firmware
#   (RecalBox_DMD.ino v114, demande utilisateur explicite "reduire la
#   taille des caracteres... pour afficher plus de texte par ligne").
#
# v10 - 2026-08-20 - safe-modify - BUG REEL corrige (retour utilisateur
#   explicite : "l'affichage description est reste plusieurs secondes
#   affiche alors que le jeu etait lance" + "melange info page1-description
#   page2-hiscore-info page2-marquee") : rungame/gamelistbrowsing
#   n'effacaient QUE leur propre fichier d'etat (GAME_SESSION_FILE /
#   BROWSE_STATE_FILE respectivement), jamais l'AUTRE -- un round-robin de
#   l'ANCIEN contexte (ex. "browse" encore en plein envoi multi-pages)
#   continuait donc de publier EN PARALLELE du nouveau round-robin
#   ("ingame" au lancement du jeu), les 2 processus independants melant
#   leurs contenus sur le meme canal marquee/cmd/score. Fix : chaque
#   evenement efface desormais AUSSI le fichier d'etat de l'AUTRE contexte
#   -- le round-robin perime le detecte a sa prochaine verification
#   (entre 2 pages/sleeps, ne peut pas interrompre un sleep deja en cours)
#   et s'arrete de lui-meme au lieu de continuer indefiniment.
#
# v9 - 2026-08-20 - safe-modify - Pagination complete RETABLIE (MAX_PAGES
#   1 -> 3, page 2 hi-score rangs 3/4/5 restauree) -- la reduction v8 etait
#   un pansement temporaire le temps de trouver la vraie cause du crash
#   materiel (TASK_WDT) constate le meme soir. Cause REELLE trouvee et
#   corrigee cote FIRMWARE (RecalBox_DMD.ino v112) : "currentMode =
#   MODE_SCORE" avait disparu par erreur lors d'un refactor precedent
#   (v111), le mode ne passait donc jamais reellement en MODE_SCORE --
#   loop() redessinait le marquee par-dessus le texte des l'iteration
#   suivante (explique aussi le "flash d'une frame" rapporte par
#   l'utilisateur) et le volume de messages/redessins en boucle rapide qui
#   en resultait a tres probablement contribue a la pression heap ayant
#   mene au crash. Cause racine reglee -> plus besoin de brider le volume
#   de messages.
#
# v8 - 2026-08-20 - safe-modify - REDUCTION D'AGRESSIVITE suite a un vrai
#   crash materiel en test reel (TASK_WDT dans processPendingMqttCommand()/
#   String::String(), DMD reste BLOQUE -- pas de reboot auto cette fois,
#   necessite un debranchement physique -- reproduit en pleine lecture SD
#   du rawpack marquee). Le volume/frequence de messages MQTT du
#   round-robin+pagination (v6/v7) est fortement suspecte d'avoir fait
#   basculer un heap deja tres tendu toute la soiree dans un cas limite
#   existant -- PAS une correction de la cause racine (a investiguer une
#   prochaine session), juste une reduction de risque immediate :
#   MAX_PAGES 3 -> 1 (send_paginated()/send_paginated_lines()) et
#   send_hiscore_paginated() n'envoie plus qu'1 page (rangs 1/2, plus de
#   page 2 rangs 3/4/5) -- au plus 1 SEUL message MQTT par type de contenu
#   par tour de round-robin desormais, au lieu de jusqu'a 2-3.
#
# v7 - 2026-08-20 - safe-modify - 2 corrections suite au premier test reel
#   du round-robin/pagination (v6) :
#   (1) BUG REEL corrige (materiel : INFOS n'affichait que "Developpeur:
#   Capcom", les 4 autres champs disparaissaient silencieusement malgre un
#   gamelist.xml complet) -- extract_field() s'arretait au premier "|"
#   rencontre, hypothese invalidee par le fix du separateur INFOS
#   (dmd_game_info.py v8, "\n" -> "|", meme jour) : la valeur INFOS
#   contient elle-meme des "|" desormais. Voir commentaire complet sur
#   extract_field() plus bas.
#   (2) Durees par page augmentees (2.5s/5s jugees "beaucoup trop brefs
#   pour voir quelque chose" en test reel) -- 4s hi-score/infos, 6s
#   description. Toujours des valeurs de DEPART, pas definitives.
#
# v6 - 2026-08-20 - safe-modify - REFONTE COMPLETE du mecanisme de
#   repetition/dwell suite a un malentendu identifie par l'utilisateur sur
#   la notion "intervalle" (l'implementation v4/v5 rejouait tout le paquet
#   hi-score+infos+description groupe toutes les N "cycles" ; ce n'etait
#   PAS le comportement voulu). Nouveau modele, specifie explicitement par
#   l'utilisateur, PRESENTE ET VALIDE AVANT implementation (voir
#   DECISIONS.md/memoire projet) :
#
#   1. ROUND-ROBIN au lieu du paquet groupe, boucle INFINIE -- round_robin()
#      alterne UN SEUL type de contenu a la fois (hi-score, PUIS
#      description, PUIS infos, en sautant les types decoches), espace de
#      "ratio" intervalles de ~SLIDESHOW_GAP_S secondes chacun (approxime
#      la duree d'un affichage marquee -- aucune notion firmware de
#      "marquee affiche N fois" n'existe, pure approximation temporelle,
#      comme deja fait pour ingame_cycle_seconds() en v4/v5, retire).
#      Exemple ratio=3 : marquee(~3x7s)-hiscore-marquee(~3x7s)-
#      description-marquee(~3x7s)-infos-marquee(~3x7s)-hiscore-...
#      Continue tant que l'etat de reference (GAME_SESSION_FILE ou
#      BROWSE_STATE_FILE) ne change pas -- s'arrete des qu'un nouvel
#      evenement change cet etat (nouveau jeu, fin de partie, veille,
#      deplacement en navigation). Remplace repeater()/
#      ingame_cycle_seconds()/publish_slideshow() (bundle) -- retires.
#
#   2. DEUX ratios SEPARES, reglables independamment depuis la page web :
#      feat_repeat_cycles (en jeu, deja existant, reinterprete comme un
#      ratio au lieu d'un multiplicateur de cycle complet) et
#      feat_repeat_browse_cycles (navigation, NOUVEAU reglage cote
#      firmware/page web -- voir RecalBox_DMD.ino/web_config.h, meme date).
#
#   3. Delai d'immobilite (dwell) rendu REGLABLE depuis la page web
#      (etait fixe a 5s en dur depuis v3) -- feat_dwell_seconds (NOUVEAU
#      reglage). Plancher de securite applique A DEUX ENDROITS (defense en
#      profondeur) : cote firmware (constrain() sur la sauvegarde, voir
#      RecalBox_DMD.ino) ET ici cote script (DWELL_MIN_SECONDS) -- demande
#      utilisateur explicite : "un minimum securitaire doit etre impose
#      pour ne pas qu'il se declenche pendant une navigation normale".
#
#   4. "Scroll bete pilote par la RB" (en reponse a "hiscore est tronque a
#      3 valeurs sur 5, description inutilisable (pas de retour a la
#      ligne)") : AUCUN changement cote rendu de base firmware -- reste des
#      CMD_SCORE independants, statiques, sans etat anime -- mais 2 ajouts
#      firmware CONCERTES (v111, meme date, voir RecalBox_DMD.ino) :
#      (a) prefixe "@<ms>|" pour une duree d'affichage PAR MESSAGE (au lieu
#      du fixe 6s) -- necessaire pour que la DERNIERE page d'une sequence
#      respecte aussi la duree courte (sinon plancher a 6s, invalide le
#      test de vitesse de lecture demande). (b) rendu special hi-score
#      "rang 1 en gros + couleur dediee" quand le titre du payload vaut
#      exactement "HI-SCORE".
#      - send_hiscore_paginated() : 1941 (5 rangs) -> EXACTEMENT 2 pages,
#        specifie par l'utilisateur : page 1 = titre "HI-SCORE" + rang 1 +
#        rang 2 (rang 1 rendu en gros cote firmware) ; page 2 = titre VIDE
#        (tombe dans le rendu generique firmware, pas de traitement
#        special) + rangs 3/4/5.
#      - send_paginated_lines() : INFOS (dev/editeur/annee/joueurs/note,
#        deja des lignes courtes distinctes) -- pagine par groupes de
#        LINES_PER_PAGE, SANS reformater.
#      - send_paginated() : DESCRIPTION (texte libre) -- replie mot par mot
#        a WRAP_WIDTH caracteres/ligne PUIS pagine, plafonne a MAX_PAGES.
#      - Durees differenciees par type de contenu (valeurs de DEPART,
#        demande utilisateur explicite "on teste ma vitesse de lecture et
#        on reajuste" -- a AJUSTER ICI au prochain retour, pas une
#        constante gravee dans le marbre) : 2.5s/page pour hi-score/infos,
#        5s/page pour description (paragraphes plus denses).
#      NB : INFOS utilise send_paginated_lines(), pas send_paginated() --
#      bug latent corrige au passage dans dmd_game_info.py (meme date) : ce
#      champ etait deja pre-decoupe en lignes courtes mais separees par
#      "\n" (convention de l'ANCIEN systeme d'overlay, RecalBox_DMD.ino
#      v92, retire depuis) -- jamais mis a jour vers "|" (convention
#      CMD_SCORE actuelle), invisible tant qu'aucune pagination n'etait
#      tentee sur ce champ precisement.
#
# v5 - 2026-08-20 - safe-modify - Verrou anti-relance rendu ATOMIQUE (mkdir
#   au lieu d'un fichier PID check-then-write) -- voir le meme fix/le meme
#   raisonnement complet dans marquee[...].sh v12.
#
# v4 - 2026-08-20 - safe-modify - Repetition periodique du slideshow EN JEU
#   -- RETIRE en v6 ci-dessus (mauvaise interpretation du reglage
#   "intervalle") -- conserve ici pour tracer l'historique, voir _backups/
#   pour le detail complet de cette version.
#
# v3 - 2026-08-20 - safe-modify - Dwell (>5s) sur gamelistbrowsing avant de
#   declencher le slideshow navigation. Mecanisme de detection
#   (BROWSE_STATE_FILE, guetteur en arriere-plan qui verifie l'absence de
#   mouvement avant de publier) inchange en v6, seule la suite (round_robin()
#   au lieu d'un simple publish_slideshow() ponctuel) change.
#
# v2 - 2026-08-20 - safe-modify - Migration vers l'architecture "DMD bete"
#   (firmware v110) : 1 seul topic (marquee/cmd/score), PLUS DE RETAIN,
#   gating par les 8 reglages hi-score/info/description/RA x en-jeu/
#   navigation (page web DMD, diffuses en MQTT retenu sur
#   marquee/status/features) -- voir feat_enabled()/features_watcher().
#
# v1 - 2026-08-17 - safe-modify - PORT depuis dev/mame-score-mqtt-bridge.
#   Voir _backups/ pour le detail complet de cette version.
# ============================================
#
# Table de decodage hi-score (inchangee depuis v1) : portee depuis les
# definitions reelles de /recalbox/share/bios/fbneo/hiscore.dat, verifiees
# avec l'outil de reference hi2txt (GreatStoneEx/hi2txt-xml) sur des
# echantillons .hi reels, puis re-verifiees en executant cette meme logique
# en ash directement sur la Recalbox :
#   - galaga (et clones partageant la meme structure : galaga84, galagab2,
#     galagads, galagamf, galagamk, galagamw, galagao, gallag) : derniers 6
#     octets du .hi = "TOP SCORE", profil BCD-le + trim 0x24 (tuile
#     "blanc") -> galaga topscore = 20000 valide.
#   - gyruss : derniers 3 octets du .hi = "TOP SCORE", entier little-endian
#     affiche en hexadecimal -> gyruss topscore = 10000 valide.
#   - 1941 : bloc de 10 rangs [SCORE(4) NOM(3) GRADE(1)] a partir de
#     l'offset 40 -- rang N -> offset 40+N*8. Top 5 en scroll vertical.
# Seuls ces jeux sont geres pour l'instant (portage limite et volontaire,
# voir memoire projet "Faisabilite highscore/level DMD"). Un jeu FBNeo
# inconnu de cette table est simplement ignore (aucune publication).

# v35 -- verrou qui vivait ICI deplace tout en haut du fichier (voir
# commentaire complet la-bas). Ne pas le reintroduire ici.

LOG="/recalbox/share/system/logs/dmd_score.log"
SCRIPT_DIR=$(dirname "$0")
# v36 -- BUG REEL MAJEUR confirme sur materiel (retour utilisateur,
# 2026-09-01 : navigation turbo -> CPU sature 100% tous coeurs, 65C+,
# ralentissement generalise RB1 proportionnel a la duree de navigation --
# voir DECISIONS.md pour l'investigation complete). Cause reelle : ce
# script vivait dans /recalbox/share/userscripts/ EN MEME TEMPS que
# dmd_hiscore_generic.py/dmd_game_info.py/dmd_challenge.py -- EmulationStation
# invoque NATIVEMENT tout fichier de ce dossier portant ces noms precis
# (mecanisme distinct du notre, convention d'arguments differente --
# "-action gamelistbrowsing -statefile ... -param ..." au lieu des
# positionnels system/rom utilises ci-dessous), a CHAQUE evenement
# gamelistbrowsing, SANS AUCUNE limite de frequence -- independamment de ce
# script (confirme par test decisif : retirer PHYSIQUEMENT ces 3 fichiers
# de userscripts/, sans meme toucher a ce script, fait disparaitre
# TOTALEMENT la surcharge CPU, 99% -> 0%, 65C -> 45C). Fix retenu : les 3
# fichiers deplaces dans un sous-dossier DEDIE (PYHELP_DIR, jamais scanne
# par le mecanisme natif ES qui ne regarde que le dossier userscripts/
# lui-meme, pas ses sous-dossiers) -- seul CE script (via PYHELP_DIR, chemin
# explicite) continue de les invoquer, avec la convention d'arguments
# positionnelle qui leur est propre. hiscore_manifest.json deplace avec
# dmd_hiscore_generic.py (meme dossier -- MANIFEST_PATH y reste relatif a
# __file__, aucun changement necessaire cote python). current.json
# (dmd_challenge.py) et gamelist.xml (dmd_game_info.py) restent a leurs
# emplacements RecalBox standards, inchanges (jamais dans userscripts/).
PYHELP_DIR="${SCRIPT_DIR}/dmd_helpers"
HI_DIR="/recalbox/share/saves/fbneo/fbneo"
FEATURES_FILE="/tmp/dmd_features_cache"
# Delai entre 2 publications successives de contenus DIFFERENTS en
# round-robin -- sert aussi d'unite d'approximation pour "un affichage
# marquee" (aucune notion firmware de "marquee affiche N fois" n'existe).
SLIDESHOW_GAP_S=7
# v6 -- durees d'affichage PAR PAGE, DIFFERENCIEES par type de contenu
# (voir en-tete changelog point 4) -- VALEURS DE DEPART a AJUSTER ICI selon
# retour utilisateur ("on teste ma vitesse de lecture et on reajuste"), pas
# une constante definitive. Forme ms (protocole firmware "@<ms>|") et forme
# secondes (argument de "sleep", accepte les decimales sous busybox/ash)
# tenues manuellement en synchronisation -- pas de calcul flottant en shell
# POSIX standard.
HISCORE_INFO_PAGE_DURATION_MS=4000
HISCORE_INFO_PAGE_DURATION_S="4"
DESC_PAGE_DURATION_MS=6000
DESC_PAGE_DURATION_S="6"
# v14 -- retour a TomThumb (Org_01 retire, v118 cote firmware -- retour
# utilisateur "espacement entre les mots important, rompt avec le style
# general") -- avance native TomThumb ~4px/caractere MAJUSCULE +1px de
# marge supplementaire ajoutee manuellement cote firmware (gfxCharAdvance()
# + espacement manuel, voir RecalBox_DMD.ino v118) = ~5px/caractere final.
# (128-1)/5 = 25.4, arrondi PAR DEFAUT (25) -- a ajuster apres validation
# visuelle sur materiel si trop/pas assez conservateur (meme demarche que
# les autres constantes de duree/pagination de ce fichier).
WRAP_WIDTH=25
# v6 -- lignes de contenu par page (send_paginated()/send_paginated_lines()) :
# maxLines=4 cote firmware moins 1 reservee au titre (toujours ligne 0) = 3.
LINES_PER_PAGE=3
# v9 -- MAX_PAGES=1 (v8) RETABLI a 3 -- la reduction d'agressivite v8 etait
# un pansement temporaire en attendant de trouver la vraie cause du crash
# materiel (TASK_WDT) constate le meme soir. Cause REELLE trouvee et
# corrigee cote FIRMWARE (RecalBox_DMD.ino v112) : "currentMode =
# MODE_SCORE" avait ete perdu par erreur lors d'un refactor precedent
# (v111) -- le mode ne passait donc jamais reellement en MODE_SCORE, loop()
# continuait a redessiner le marquee par-dessus le texte des l'iteration
# suivante ET (tres probablement) le volume de messages/redessins en
# boucle rapide qui en resultait a contribue a la pression heap ayant mene
# au crash. Cause racine reglee -> plus besoin de brider le volume de
# messages, la pagination complete est restauree.
MAX_PAGES=3
# v15 -- plafond DEDIE a DESCRIPTION (send_paginated(), pagination
# "sensible aux phrases" -- voir commentaire complet sur send_paginated())
# -- demande utilisateur explicite ("oui avec une securite a 5 pages max")
# suite a la question "peux-tu detecter les fins de phrase/paragraphe pour
# ne pas couper au milieu". SEPARE de MAX_PAGES (INFOS/hi-score, pagination
# a taille de page FIXE, inchangee) car la pagination par phrase produit un
# nombre de pages moins previsible -- ce plafond en borne le pire cas.
MAX_PAGES_DESCRIPTION=5
# v6 -- plancher de securite pour le delai d'immobilite navigation
# (feat_dwell_seconds, reglable depuis la page web) -- meme plancher
# applique cote firmware (constrain()) -- defense en profondeur.
DWELL_MIN_SECONDS=3
# v3 -- dwell navigation : delai d'immobilite sur un rom avant de
# declencher le round-robin "browse". BROWSE_STATE_FILE partage l'etat
# courant (sys|rom) entre la boucle principale et les guetteurs en
# arriere-plan (necessaire : un "&" fork ne voit jamais les mises a jour
# ulterieures d'une variable shell du parent).
BROWSE_STATE_FILE="/tmp/dmd_browse_state"
# v4 -- session de partie en cours (voir round_robin()).
GAME_SESSION_FILE="/tmp/dmd_game_session"

read_state() {
    grep "^${1}=" "/tmp/es_state.inf" 2>/dev/null | cut -d= -f2- | tr -d '\r\n '
}

# v37 -- DIAGNOSTIC (retour utilisateur, desync overlay/marquee entre ce
# script et marquee.sh, voir DECISIONS.md "BUG TROUVE, PAS ENCORE CORRIGE --
# desync overlay/marquee" -- piste de depart actee : "instrumenter les 2
# scripts pour logger leur reference de jeu... et comparer precisement les
# timestamps de desync"). "date" ash/busybox n'expose pas %N (verifie sur ce
# materiel -- retourne le "%N" litteral, non substitue -- coherent avec le
# commentaire historique v6 de marquee.sh sur l'absence d'horloge sub-seconde
# fiable). /proc/uptime expose 2 decimales (centieme de seconde), lu via
# "read" (builtin ash, aucun fork) -- horloge commune aux 2 scripts (meme
# machine), donc correlation directe et precise sans souci de decalage
# d'horloge entre eux. Cout : une lecture de fichier, pas de sous-processus.
precise_ts() {
    read _pts_up _pts_rest < /proc/uptime 2>/dev/null
    echo "$_pts_up"
}

# v2 -- lit le cache local des reglages (voir features_watcher()) --
# renvoie vrai (0) si la cle demandee vaut 1, faux (1) sinon -- y compris
# si le cache n'existe pas encore (comportement prudent : ne publie rien
# tant que le reglage reel n'est pas connu).
feat_enabled() {
    [ -f "$FEATURES_FILE" ] || return 1
    val=$(sed -n "s/.*${1}=\([01]\).*/\1/p" "$FEATURES_FILE" | head -n1)
    [ "$val" = "1" ]
}

# v4 -- variante numerique de feat_enabled() (repeat_cycles/
# repeat_browse_cycles/dwell_seconds) -- renvoie 0 si le cache n'existe pas
# encore ou si la cle est absente (comportement prudent).
feat_value() {
    [ -f "$FEATURES_FILE" ] || { echo 0; return; }
    val=$(sed -n "s/.*${1}=\([0-9]*\).*/\1/p" "$FEATURES_FILE" | head -n1)
    [ -n "$val" ] && echo "$val" || echo 0
}

# v6 -- liste (separee par des espaces) des types de contenu ACTIVES pour
# ce contexte ("ingame" ou "browse"), dans l'ordre fixe hiscore/description/
# infos -- recalculee a CHAQUE appel de round_robin() pour suivre tout
# changement de reglage en direct.
enabled_panel_types() {
    ctx="$1"
    types=""
    feat_enabled "hiscore_${ctx}" && types="${types}hiscore "
    feat_enabled "description_${ctx}" && types="${types}description "
    feat_enabled "info_${ctx}" && types="${types}info "
    echo "$types"
}

# v17 -- BUG REEL corrige AVANT deploiement (retour utilisateur explicite :
# "attention il ne faut pas que pour chaque lancement de blazing star le
# challenge remplace le hiscore ! uniquement si on lance blazing star en
# passant par le RB challenge") -- la 1ere version comparait seulement
# system/rom contre current.json (voir dmd_challenge.py), ce qui aurait
# affiche le classement pour N'IMPORTE QUEL lancement du jeu concerne (ex.
# depuis la gamelist normale), pas seulement une VRAIE session de
# challenge. Signal fiable trouve en lisant le code source RB
# (configgen/emulatorlauncher.py, ligne ~242) : le suivi ScoreWatch/
# challenge n'est instancie QUE si l'argument de ligne de commande
# "--challenge <manifeste>" a ete passe au lancement -- ce qui n'arrive
# QUE via le menu "Challenges" de ES, jamais pour un lancement normal
# depuis la gamelist. Cette fonction verifie qu'un processus EN COURS
# porte bien cet argument (le processus emulatorlauncher.py reste actif
# pendant toute la duree de la partie, cf. code source : il attend la fin
# de l'emulateur avant d'appeler scoreWatch.finish()).
# ATTENTION : PAS ENCORE VALIDE SUR MATERIEL avec une vraie session de
# challenge active (aucune en cours au moment d'ecrire ceci) -- a
# confirmer au prochain test reel.
# v24 -- pattern "[-]-challenge " (au lieu de "--challenge ") : evite que ce
# grep ne se matche LUI-MEME dans la sortie de ps (voir changelog v24).
# v25 -- 2e BUG REEL corrige (retour utilisateur : "jeu via challenge : pas
# de RB CHALLENGE affiche, uniquement marquee") : le vrai flag passe par
# emulatorlauncher.pyc est "-challenge <manifeste>" avec UN SEUL TIRET,
# pas "--challenge" -- verifie en DIRECT sur le materiel via `ps` pendant
# une vraie session Challenge active :
#   sh -c -- python .../emulatorlauncher.pyc ... -challenge
#   /recalbox/share/system/challenges/current.json
# Le commentaire d'origine (v17) et le fix v24 supposaient tous les deux
# DEUX tirets par erreur -- cette fonction n'a donc probablement JAMAIS
# detecte une vraie session Challenge correctement, y compris lors du
# "seul test reel" cite en v24 (masque par le bug d'auto-match de l'epoque,
# qui renvoyait toujours vrai independamment du vrai flag). Pattern corrige
# a UN tiret, meme astuce anti-auto-match (bracket autour du tiret unique).
# Verifie en direct : MATCH_TRUE avec une vraie session Challenge active.
challenge_session_active() {
    ps -o args -ww 2>/dev/null | grep -q -- '[-]challenge '
}

# v13 -- BUG REEL corrige (retour utilisateur : "la description a
# poursuivi sa page 2 et 3... apres le marquee du lancement de jeu" / puis
# confirme symetrique : "quand on quitte le jeu, hiscore continue a
# s'afficher 1 fois lui aussi") -- round_robin() ne verifiait l'etat de
# reference (state_file/expected) qu'ENTRE 2 TOURS (2 types de contenu
# differents), jamais ENTRE LES PAGES d'un MEME contenu en cours d'envoi
# (send_paginated()/send_paginated_lines()/send_hiscore_paginated() ont
# chacune leurs propres sleep() internes, invisibles pour round_robin()
# tant que la fonction n'est pas revenue). Une sequence multi-pages deja
# lancee continuait donc jusqu'a sa fin meme apres un changement de
# contexte (nouveau jeu, fin de partie, deplacement en navigation). Fix :
# ce garde est desormais aussi verifie ICI, entre CHAQUE page, via les 2
# parametres optionnels state_file/expected passes en cascade depuis
# round_robin() -> publish_one_panel()/publish_hiscore() -> les fonctions
# de pagination. $1=state_file (vide = pas de verification, comportement
# inchange pour les appels HORS round-robin, ex. endgame) $2=expected.
state_still_valid() {
    [ -z "$1" ] && return 0
    current=$(cat "$1" 2>/dev/null)
    [ "$current" = "$2" ]
}

# v6 -- replie $1 sur des lignes d'au plus $2 caracteres, mot par mot
# (coupe brutalement seulement si un mot depasse la largeur a lui seul --
# rare, evite une ligne infinie). Sortie : une ligne par ligne.
wrap_text() {
    text="$1"; width="$2"
    line=""
    for word in $text; do
        if [ -z "$line" ]; then cand="$word"; else cand="$line $word"; fi
        if [ "${#cand}" -gt "$width" ]; then
            [ -n "$line" ] && printf '%s\n' "$line"
            line="$word"
            while [ "${#line}" -gt "$width" ]; do
                printf '%s\n' "$(printf '%s' "$line" | cut -c1-"$width")"
                line=$(printf '%s' "$line" | cut -c$((width + 1))-)
            done
        else
            line="$cand"
        fi
    done
    [ -n "$line" ] && printf '%s\n' "$line"
}

# v13 -- pagine des lignes DEJA DECOUPEES (INFOS) -- groupe par
# LINES_PER_PAGE SANS reformater, envoie chaque page en remplacement de la
# precedente. Verifie state_still_valid() APRES chaque sleep, AVANT
# d'envoyer la page suivante -- interrompt immediatement si le contexte a
# change entre-temps (voir commentaire complet pres de state_still_valid()).
# $1=titre $2="ligne1|ligne2|..." $3=duree_ms $4=duree_s $5=state_file
# (optionnel) $6=expected (optionnel).
send_paginated_lines() {
    title="$1"; content="$2"; dur_ms="$3"; dur_s="$4"; sf="$5"; exp="$6"
    [ -z "$content" ] && return 1
    old_ifs="$IFS"
    IFS='|'
    set -- $content
    IFS="$old_ifs"
    page=""; n=0; page_count=0; first_page=1
    for ln in "$@"; do
        [ "$page_count" -ge "$MAX_PAGES" ] && break
        if [ -z "$page" ]; then page="$ln"; else page="${page}|${ln}"; fi
        n=$((n + 1))
        if [ "$n" -eq "$LINES_PER_PAGE" ]; then
            if [ "$first_page" -eq 0 ]; then
                sleep "$dur_s"
            fi
            # v34 -- revalide aussi avant la 1ere page (pas seulement les
            # suivantes) : le python3 qui a construit $content a pu prendre
            # un instant, voir changelog v34 en entete.
            state_still_valid "$sf" "$exp" || return 1
            send_score "${title}|${page}" "$dur_ms" "$exp"
            first_page=0; page_count=$((page_count + 1))
            page=""; n=0
        fi
    done
    if [ -n "$page" ] && [ "$page_count" -lt "$MAX_PAGES" ]; then
        if [ "$first_page" -eq 0 ]; then
            sleep "$dur_s"
        fi
        state_still_valid "$sf" "$exp" || return 1
        send_score "${title}|${page}" "$dur_ms"
    fi
    return 0
}

# v6 -- replie $2 (texte libre) a WRAP_WIDTH caracteres/ligne PUIS pagine
# par LINES_PER_PAGE, plafonne a MAX_PAGES. Ellipse "..." ajoutee a la
# DERNIERE ligne gardee si le contenu a ete tronque par ce plafond -- la
# liste de lignes est tronquee EN AMONT (avant la boucle de pagination),
# PAS pendant, pour eviter un bug reel trouve en test local (WSL/dash) :
# verifier le plafond AU DEBUT de chaque iteration laissait $page toujours
# VIDE au moment de la troncature quand elle tombait pile sur une limite
# de page (cas le PLUS frequent, pas un cas rare) -- l'ellipse ne
# s'affichait alors jamais, aucun signal visuel qu'il manquait du texte.
# v16 -- pagination par PHRASES ENTIERES (v15 remonte reculait vers la
# derniere ponctuation DANS la fenetre de LINES_PER_PAGE lignes -- retour
# utilisateur explicite : "je prefererais aller chercher la ponctuation
# SUIVANTE [...] mais bloquer en repli a 5 pages" -- i.e. ne JAMAIS couper
# une phrase court, avancer jusqu'a sa fin quitte a depasser
# LINES_PER_PAGE, le plafond de pages restant le seul filet de securite).
# (Rappel : les PARAGRAPHES, eux, sont deja detruits en amont --
# dmd_game_info.py normalise tous les retours a la ligne XML en simple
# espace -- seule la fin de PHRASE, . ! ?, est encore detectable ici.)
#
# Algorithme : (1) decoupe $text en PHRASES via sed (coupe apres
# ".", "!" ou "?" suivi d'un/des espace(s)) ; (2) empile les phrases une a
# une dans la page en cours, chaque phrase enveloppee (wrap_text)
# INDIVIDUELLEMENT -- si elle ne tient pas dans le reste de la page en
# cours, clot cette page et demarre une nouvelle page avec cette phrase ;
# (3) SEULE exception ou une coupure "au milieu" reste possible : une
# phrase UNIQUE trop longue pour tenir sur une page entiere (impossible a
# eviter, etalee sur plusieurs pages consecutives LINES_PER_PAGE lignes a
# la fois) ; (4) plafonne a MAX_PAGES_DESCRIPTION pages, "..." ajoute a la
# derniere ligne de la DERNIERE page SEULEMENT si troncature reelle.
#
# Construction (Phase A, $pages) puis envoi (Phase B) SEPARES -- meme
# raison que partout ailleurs dans ce fichier : decider une troncature
# APRES avoir deja commence a ENVOYER une page interdirait d'ajouter
# l'ellipse a temps (bug reel deja trouve en test local sur l'ancienne
# version de cette fonction, voir historique v6/v15 plus haut). $pages
# accumule les pages COMPLETES (chaque page = ses lignes deja jointes par
# "|") separees par un octet 0x01 (improbable dans du texte de jeu, evite
# tout conflit avec "|" deja utilise comme separateur de ligne/champ).
#
# $1=titre $2=texte brut $3=duree_ms $4=duree_s $5=state_file (optionnel)
# $6=expected (optionnel).
send_paginated() {
    title="$1"; text="$2"; dur_ms="$3"; dur_s="$4"; sf="$5"; exp="$6"
    [ -z "$text" ] && return 1

    sentences=$(printf '%s' "$text" | sed -e 's/\([.!?]\)[ ]\{1,\}/\1\n/g')
    sep=$(printf '\1')

    page=""; page_lines=0
    pages=""; page_count=0
    truncated=0

    while IFS= read -r sentence; do
        [ -z "$sentence" ] && continue
        if [ "$page_count" -ge "$MAX_PAGES_DESCRIPTION" ]; then truncated=1; break; fi

        swrapped=$(wrap_text "$sentence" "$WRAP_WIDTH")
        [ -z "$swrapped" ] && continue
        scount=$(printf '%s\n' "$swrapped" | wc -l)

        # La phrase ne tient pas dans le reste de la page en cours (et la
        # page en cours n'est pas vide) -- clot-la avant de commencer
        # cette phrase sur une page fraiche (ne coupe JAMAIS une phrase
        # pour la faire tenir de force -- c'est exactement ce que ce
        # design evite).
        if [ "$page_lines" -gt 0 ] && [ $((page_lines + scount)) -gt "$LINES_PER_PAGE" ]; then
            if [ -z "$pages" ]; then pages="$page"; else pages="${pages}${sep}${page}"; fi
            page_count=$((page_count + 1))
            page=""; page_lines=0
            if [ "$page_count" -ge "$MAX_PAGES_DESCRIPTION" ]; then truncated=1; break; fi
        fi

        while IFS= read -r ln; do
            if [ -z "$page" ]; then page="$ln"; else page="${page}|${ln}"; fi
            page_lines=$((page_lines + 1))
            # Phrase trop longue pour une page (scount > LINES_PER_PAGE,
            # seul cas inevitable de coupure "au milieu") -- clot des que
            # la page est pleine, meme SI la phrase n'est pas terminee.
            if [ "$page_lines" -eq "$LINES_PER_PAGE" ]; then
                if [ -z "$pages" ]; then pages="$page"; else pages="${pages}${sep}${page}"; fi
                page_count=$((page_count + 1))
                page=""; page_lines=0
                [ "$page_count" -ge "$MAX_PAGES_DESCRIPTION" ] && truncated=1
            fi
        done <<EOF
$swrapped
EOF
        [ "$truncated" -eq 1 ] && break
    done <<EOF
$sentences
EOF

    if [ -n "$page" ] && [ "$page_count" -lt "$MAX_PAGES_DESCRIPTION" ]; then
        if [ -z "$pages" ]; then pages="$page"; else pages="${pages}${sep}${page}"; fi
        page_count=$((page_count + 1))
    elif [ -n "$page" ]; then
        # Reste un residu de page au moment de la troncature (derniere
        # phrase traitee avant d'atteindre le plafond) -- garde comme
        # derniere page envoyee plutot que de le perdre silencieusement.
        if [ -z "$pages" ]; then pages="$page"; else pages="${pages}${sep}${page}"; fi
        page_count=$((page_count + 1))
        truncated=1
    fi

    [ -z "$pages" ] && return 1

    old_ifs="$IFS"
    IFS="$sep"
    set -- $pages
    IFS="$old_ifs"
    total_pages=$#
    n=0; first_page=1
    for pg in "$@"; do
        n=$((n + 1))
        if [ "$truncated" -eq 1 ] && [ "$n" -eq "$total_pages" ]; then
            pg="${pg}..."
        fi
        if [ "$first_page" -eq 0 ]; then
            sleep "$dur_s"
        fi
        # v34 -- revalide aussi avant la 1ere page, voir changelog v34 en
        # entete (meme motif que send_paginated_lines()/
        # send_hiscore_paginated()).
        state_still_valid "$sf" "$exp" || return 1
        send_score "${title}|${pg}" "$dur_ms" "$exp"
        first_page=0
    done
    return 0
}

# v12 -- pagination SPECIFIQUE hi-score 1941 (5 rangs), EXACTEMENT 2 pages
# specifiees par l'utilisateur : page 1 = titre "HI-SCORE" + rang 1 + rang
# 2 ; page 2 = titre "HI-SCORE" AUSSI (BUG REEL corrige, retour
# utilisateur : "la page 2 hi-score n'a pas son titre" -- le titre doit
# PERSISTER sur toutes les pages d'un meme contenu, regle deja actee et
# deja respectee par INFOS/DESCRIPTION) + rangs 3/4/5. Le rendu special
# "rang 1 en gros" cote firmware (RecalBox_DMD.ino v116) se declenche
# desormais sur le CONTENU (rang 1 present) plutot que sur le titre seul,
# ce qui permet aux 2 pages de partager le meme titre sans ambiguite.
# v13 -- state_still_valid() ajoutee (voir commentaire complet la-bas) --
# meme motif que send_paginated()/send_paginated_lines(). $2=state_file
# (optionnel) $3=expected (optionnel), decales car $1 reste topn.
# $1="1 nom score|2 nom score|..." $2=state_file $3=expected.
send_hiscore_paginated() {
    topn="$1"; sf="$2"; exp="$3"
    old_ifs="$IFS"
    IFS='|'
    set -- $topn
    IFS="$old_ifs"
    r1="$1"; r2="$2"; r3="$3"; r4="$4"; r5="$5"

    # v34 -- revalide avant la page 1 (jusqu'ici seule la page 2 l'etait,
    # apres son sleep) -- voir changelog v34 en entete.
    state_still_valid "$sf" "$exp" || return 1

    page1="HI-SCORE"
    [ -n "$r1" ] && page1="${page1}|${r1}"
    [ -n "$r2" ] && page1="${page1}|${r2}"
    send_score "$page1" "$HISCORE_INFO_PAGE_DURATION_MS" "$exp"

    page2=""
    for r in "$r3" "$r4" "$r5"; do
        [ -z "$r" ] && continue
        if [ -z "$page2" ]; then page2="$r"; else page2="${page2}|${r}"; fi
    done
    if [ -n "$page2" ]; then
        sleep "$HISCORE_INFO_PAGE_DURATION_S"
        state_still_valid "$sf" "$exp" || return 1
        send_score "HI-SCORE|${page2}" "$HISCORE_INFO_PAGE_DURATION_MS" "$exp"
    fi
}

# v6 -- boucle round-robin INFINIE (voir en-tete changelog point 1) --
# alterne UN SEUL type de contenu a la fois parmi ceux actives pour ce
# contexte, espace de "ratio" intervalles de ~SLIDESHOW_GAP_S secondes.
# Continue INDEFINIMENT tant que $state_file contient toujours $expected.
# $1=ctx("ingame"/"browse") $2=sys $3=gpath $4=rom $5=state_file
# $6=expected $7=ratio_key(feat_value)
round_robin() {
    ctx="$1"; sys="$2"; gpath="$3"; rom="$4"; state_file="$5"; expected="$6"; ratio_key="$7"
    idx=0
    # v38 -- hiscore_checked/hiscore_ok : voir commentaire complet au point
    # d'usage plus bas (retrait de "hiscore" de $types quand indisponible
    # pour ce rom, verifie UNE SEULE FOIS par appel round_robin()).
    hiscore_checked=0
    hiscore_ok=1
    while true; do
        ratio=$(feat_value "$ratio_key")
        [ "$ratio" -le 0 ] && return
        sleep $((ratio * SLIDESHOW_GAP_S))
        current=$(cat "$state_file" 2>/dev/null)
        [ "$current" = "$expected" ] || return
        types=$(enabled_panel_types "$ctx")
        # v38 -- retour utilisateur explicite : "si hi-score n'existe pas il
        # ne faut pas le compter dans les temps de rotation et passer au
        # panneau suivant" -- jusqu'ici, un rom sans hi-score gardait quand
        # meme sa place dans la rotation : round_robin() lui consacrait un
        # tour complet (sleep ratio*SLIDESHOW_GAP_S, ~14s par defaut) pour
        # rien (publish_hiscore() retombe sur "SCORE skip ... jeu non
        # supporte", aucun envoi), retardant d'autant l'arrivee du panneau
        # suivant (description/info) -- mesure sur materiel (DECISIONS.md,
        # cas punisherh/tbowlp) : ~60-70s avant le 1er contenu reel, proche
        # voire superieur a la duree de nombreuses demos ES (~54-100s
        # observees), le panneau n'a alors JAMAIS le temps de s'afficher.
        # Fix : disponibilite verifiee UNE SEULE FOIS par appel round_robin
        # (pas a chaque tour -- build_score_payload() peut invoquer
        # dmd_hiscore_generic.py, cout python3 a ne pas repeter inutilement
        # toutes les ~14s) ; si indisponible, "hiscore" est retire de
        # $types pour tous les tours suivants -- le round-robin alterne
        # alors directement entre les types restants, sans jamais lui
        # reserver de tour.
        case "$types" in
            *hiscore*)
                if [ "$hiscore_checked" -eq 0 ]; then
                    hiscore_checked=1
                    hp_check=$(build_score_payload "$rom" "$sys")
                    [ -n "$hp_check" ] || hiscore_ok=0
                fi
                if [ "$hiscore_ok" -eq 0 ]; then
                    types=$(echo "$types" | sed 's/hiscore //')
                fi
                ;;
        esac
        # v19 -- "challenge" (classement communautaire Recalbox du mois),
        # uniquement en contexte "ingame". Retour utilisateur explicite :
        # "en mode challenge je veux uniquement le tableau challenge +
        # marquee rien d'autre" -- REMPLACE (pas ajoute a) le reste de la
        # rotation en jeu tant qu'une VRAIE session de challenge est
        # active (challenge_session_active(), voir son commentaire complet
        # plus haut) -- hi-score/infos/description restent actifs comme
        # avant des que ce n'est plus le cas (partie normale).
        # v23 -- CORRECTION de v22 : la navigation A L'INTERIEUR du
        # systeme virtuel "challenges" doit afficher le TABLEAU challenge
        # (pas rien -- retour utilisateur explicite : "jeu contenu dans
        # challenge... on affiche le tableau challenge"), tandis que le
        # MEME jeu rencontre via son systeme reel (fbneo/favoris) garde le
        # comportement normal SANS jamais montrer le tableau challenge.
        # Matrice complete (voir changelog v23) :
        #   - navigation systeme "challenges" -> marquee seul (rien de
        #     special a faire ICI, systembrowsing ne demarre jamais de
        #     round-robin -- comme n'importe quel autre systeme)
        #   - navigation JEU dans "challenges" -> tableau challenge SEUL
        #     (LAST_SYSTEMBROWSING_ID == "challenges")
        #   - navigation JEU via fbneo/favoris (meme jeu) -> config
        #     normale (hiscore/infos/description), JAMAIS le tableau
        #     challenge
        if [ "$ctx" = "ingame" ] && challenge_session_active; then
            types="challenge "
        elif [ "$ctx" = "browse" ] && [ "$LAST_SYSTEMBROWSING_ID" = "challenges" ]; then
            types="challenge "
        fi
        if [ -z "$types" ]; then
            continue
        fi
        set -- $types
        count=$#
        pos=$((idx % count))
        i=0; chosen=""
        for t in "$@"; do
            [ "$i" -eq "$pos" ] && chosen="$t"
            i=$((i + 1))
        done
        idx=$((idx + 1))
        # v17 -- "challenge" ajoute ICI (pas dans enabled_panel_types(),
        # qui reste inchangee/partagee entre ingame et browse) car ce
        # contenu n'a de sens qu'en jeu -- voir changelog v17. Overhead
        # negligeable si aucun challenge n'est actif (publish_one_panel()
        # renvoie silencieusement 1, round_robin() continue normalement).
        echo "$(date '+%H:%M:%S') ROUNDROBIN ctx=$ctx type=$chosen ratio=$ratio pos=${pos}/${count}" >> "$LOG"
        publish_one_panel "$sys" "$gpath" "$rom" "$chosen" "$state_file" "$expected"
        current=$(cat "$state_file" 2>/dev/null)
        [ "$current" = "$expected" ] || return
    done
}

# v30 -- BUG REEL RECURRENT (retour utilisateur explicite : "souci
# rencontre de multiples fois... la methode est a revoir pour le transfert
# des reglages") : marquee/status/features peut arriver TRONQUE (observe en
# direct : "2;dwell_seconds=3" au lieu de la chaine complete a 11 champs,
# aussi bien dans la valeur RETENUE sur le broker que dans ce cache -- donc
# pas une corruption locale a ce script, la troncature remonte au firmware,
# voir son changelog RecalBox_DMD.ino). L'ANCIEN code ecrasait
# INCONDITIONNELLEMENT $FEATURES_FILE avec CE QUI ARRIVE, meme tronque --
# UN SEUL message corrompu suffisait a casser tous les panneaux d'un coup
# (feat_enabled()/feat_value() ne trouvent alors plus aucune cle valide,
# TOUS les jeux perdent leurs panneaux simultanement, pas juste celui en
# cours). Fix (2e ligne de defense, complement du garde cote firmware) :
# validation de completude AVANT d'ecraser le cache -- un message qui ne
# contient pas les 11 cles attendues est REJETE (cache existant conserve
# tel quel, jamais efface par du contenu douteux) plutot qu'accepte
# aveuglement. Cause racine exacte de la troncature encore non confirmee
# avec certitude (suspect : concatenation String cote firmware sous
# pression heap) -- ce garde protege quelle que soit la cause, cote
# reception.
FEATURES_REQUIRED_KEYS="hiscore_ingame hiscore_browse info_ingame info_browse description_ingame description_browse ra_ingame ra_browse repeat_cycles repeat_browse_cycles dwell_seconds"

features_line_complete() {
    line="$1"
    for k in $FEATURES_REQUIRED_KEYS; do
        case "$line" in
            *"${k}="*) ;;
            *) return 1 ;;
        esac
    done
    return 0
}

# v2 -- sous-processus DEDIE (marquee/status/features RETENU cote DMD --
# 1ere lecture immediate a la souscription, meme si ce script demarre
# apres le DMD).
features_watcher() {
    mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/status/features" 2>/dev/null | \
    while IFS= read -r line; do
        if features_line_complete "$line"; then
            printf '%s\n' "$line" > "$FEATURES_FILE"
        else
            echo "$(date '+%H:%M:%S') FEATURES rejete (message incomplet/corrompu, cache conserve): $line" >> "$LOG"
        fi
    done
}
features_watcher &

# v6 -- prefixe desormais "@<duree_ms>|" (voir RecalBox_DMD.ino v111,
# CMD_SCORE) -- $2 optionnel, defaut 6000 (comportement identique a avant
# v6 si omis). PLUS DE RETAIN (depuis v2). Best-effort (QoS 0) -- une
# perte occasionnelle n'est qu'un contenu manque, jamais un ecran fige.
# v11 -- BUG REEL corrige (retour utilisateur : "avant la page 3 de
# description en navigation, on a 1 flash du marquee qui apparait") :
# la duree envoyee au firmware (@<ms>|) et le sleep() entre 2 pages (voir
# send_paginated()/send_hiscore_paginated()) utilisaient la MEME valeur --
# le minuteur firmware demarre a la RECEPTION du message (apres latence
# reseau), le sleep() cote script demarre a l'ENVOI -- si le traitement du
# script (repli mot-par-mot, construction de la page suivante) prend ne
# serait-ce que quelques dizaines de ms de plus que la latence reseau, le
# minuteur firmware de la page EN COURS peut expirer et revenir
# brievement au marquee AVANT que la page suivante n'arrive. Fix : marge
# de securite ajoutee UNIQUEMENT a la duree envoyee au firmware (le rythme
# d'envoi cote script, lui, reste inchange) -- le firmware tient donc
# toujours legerement plus longtemps que l'intervalle reel entre 2 envois.
SCORE_TIMER_MARGIN_MS=800
send_score() {
    # v37 -- $3=ref (optionnel, "sys_browsing_id|system|rom" -- deja
    # disponible chez tous les appelants via $exp, meme valeur que
    # state_still_valid() vient de valider juste avant chaque appel) +
    # precise_ts() : voir DECISIONS.md/commentaire complet pres de
    # precise_ts(), diagnostic desync overlay/marquee.
    payload="$1"; dur="$2"; ref="$3"
    [ -z "$dur" ] && dur=6000
    fw_dur=$((dur + SCORE_TIMER_MARGIN_MS))
    # v39 -- topic marquee/cmd/score -> marquee/cmd unique (voir DECISIONS.md
    # + RecalBox_DMD.ino v148, meme motif que marquee.sh v40) : 12 topics
    # fusionnes en 1 seul cote DMD pour reduire l'exposition au blocage TX
    # post-CONNACK. Payload prefixe "CMD=score ARG=" -- le reste (@duree|
    # contenu) est inchange, ARG prend tout jusqu'a la fin cote DMD donc
    # compatible avec les espaces/pipes deja presents dans ce payload.
    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd" -m "CMD=score ARG=@${fw_dur}|${payload}" 2>/dev/null
    echo "$(date '+%H:%M:%S') [$(precise_ts)] SEND marquee/cmd/score ref=${ref} = @${fw_dur}|${payload}" >> "$LOG"
}

# v42 -- cache mono-emplacement (sys+gpath) pour dmd_game_info.py : les cas
# "description" et "info" de publish_one_panel() l'invoquaient CHACUN
# separement avec les MEMES arguments alors qu'il renvoie deja les 2 champs
# combines en 1 seul appel (voir extract_field() juste apres) -- round_robin()
# les affiche a des tours SEPARES de la rotation, ce qui doublait
# l'invocation python3 (demarrage interpreteur + parse XML) par rotation
# complete pour EXACTEMENT la meme donnee statique. Piste remontee par la
# revue pre-merge master du 2026-09-03 (voir DECISIONS.md, "Explicitement
# PAS fait"), reprise ici. Cache invalide des que sys/gpath changent (nouveau
# jeu) -- ce sont deja les seules cles dont depend la sortie de
# dmd_game_info.py, donc suffisantes pour garantir un cache correct. Portee
# du cache : les variables globales survivent tant que dure LE round_robin()
# en cours (chaque appel round_robin ... & demarre un NOUVEAU processus
# forke, donc un cache neuf -- jamais de staleness entre 2 parties/sessions
# de navigation differentes).
_GI_CACHE_SYS=""
_GI_CACHE_GPATH=""
_GI_CACHE_RAW=""
get_game_info() {
    _gi_sys="$1"; _gi_gpath="$2"
    if [ "$_gi_sys" = "$_GI_CACHE_SYS" ] && [ "$_gi_gpath" = "$_GI_CACHE_GPATH" ] && [ -n "$_GI_CACHE_RAW" ]; then
        echo "$_GI_CACHE_RAW"
        return 0
    fi
    _gi_raw=$(python3 "${PYHELP_DIR}/dmd_game_info.py" "$_gi_sys" "$_gi_gpath" 2>>"$LOG")
    _GI_CACHE_SYS="$_gi_sys"; _GI_CACHE_GPATH="$_gi_gpath"; _GI_CACHE_RAW="$_gi_raw"
    echo "$_gi_raw"
}

# v2 -- extrait un champ ("DESCRIPTION" ou "INFOS") du payload combine
# genere par dmd_game_info.py (format "LABEL|contenu|LABEL|contenu|...").
# v6 -- BUG REEL corrige (trouve en test reel sur materiel : INFOS
# n'affichait que "Developpeur: Capcom", les 4 autres champs disparaissaient
# silencieusement alors que le gamelist.xml source etait complet). Cause :
# INFOS (dmd_game_info.py v8) joint desormais ses sous-champs avec "|" au
# lieu de "\n" (fix du bug de separateur v8, meme date) -- mais
# extract_field() s'arretait au tout PREMIER "|" rencontre (`${rest%%|*}`),
# hypothese valable seulement quand chaque valeur tenait sur une seule
# ligne, plus le cas pour INFOS. Fix : DESCRIPTION est delimite par le
# marqueur du champ SUIVANT connu ("|INFOS|", voir FIELD_ORDER dans
# dmd_game_info.py -- DESCRIPTION toujours avant INFOS) ; INFOS, TOUJOURS
# DERNIER champ du payload, s'etend jusqu'a la fin (moins le "|" de
# fermeture ajoute par build_payload()).
extract_field() {
    raw="$1"; want="$2"
    case "$raw" in
        *"${want}|"*)
            rest="${raw#*${want}|}"
            if [ "$want" = "DESCRIPTION" ]; then
                content="${rest%%|INFOS|*}"
            else
                content="$rest"
            fi
            content="${content%|}"
            [ -n "$content" ] && echo "${want}|${content}"
            ;;
    esac
}

decode_galaga_topscore() {
    size=$(wc -c < "$1" 2>/dev/null)
    off=$((size - 6))
    hex=$(od -An -tx1 -j "$off" -N 6 "$1" 2>/dev/null)
    set -- $hex
    rev="$6 $5 $4 $3 $2 $1"
    result=""
    started=0
    for b in $rev; do
        if [ "$started" -eq 0 ] && [ "$b" = "24" ]; then
            continue
        fi
        started=1
        result="${result}$(printf '%s' "$b" | cut -c2)"
    done
    [ -z "$result" ] && result="0"
    result=$(printf '%s' "$result" | sed 's/^0*//')
    [ -z "$result" ] && result="0"
    echo "$result"
}

decode_gyruss_topscore() {
    size=$(wc -c < "$1" 2>/dev/null)
    off=$((size - 3))
    hex=$(od -An -tx1 -j "$off" -N 3 "$1" 2>/dev/null)
    set -- $hex
    val=$(( 0x$1 + 0x$2 * 256 + 0x$3 * 65536 ))
    printf '%X' "$val"
}

decode_1941_rank_score() {
    hifile="$1"; idx="$2"
    off=$((40 + idx * 8))
    hex=$(od -An -tx1 -j "$off" -N 4 "$hifile" 2>/dev/null)
    set -- $hex
    result="$1$2$3$4"
    result=$(printf '%s' "$result" | sed 's/^0*//')
    [ -z "$result" ] && result="0"
    echo "$result"
}

decode_1941_rank_name() {
    hifile="$1"; idx="$2"
    off=$((40 + idx * 8 + 4))
    dd if="$hifile" bs=1 skip="$off" count=3 2>/dev/null | tr -cd 'A-Za-z0-9 .'
}

decode_1941_topN() {
    hifile="$1"; n="$2"
    i=0
    out=""
    while [ "$i" -lt "$n" ]; do
        s=$(decode_1941_rank_score "$hifile" "$i")
        nm=$(decode_1941_rank_name "$hifile" "$i")
        rank=$((i + 1))
        line="${rank} ${nm} ${s}"
        if [ -z "$out" ]; then out="$line"; else out="${out}|${line}"; fi
        i=$((i + 1))
    done
    echo "$out"
}

# v2 -- ne publie plus directement : renvoie le payload sur stdout (ou rien
# si non applicable), le decoupage/gating reste au niveau appelant
# (publish_hiscore(), v6).
build_score_payload() {
    rom="$1"; sys="$2"

    # v27 -- BUG REEL corrige (retour utilisateur, "je suis etonne car des
    # la creation .hi ca semblait fonctionner" -- si, mais UNIQUEMENT pour
    # fbneo) : cette fonction ne recevait pas $sys et gatait TOUT (y compris
    # le fallback generique v26) sur `hifile="${HI_DIR}/${rom}.hi"`, un
    # chemin fbneo EN DUR -- un jeu MAME (meme deja dans le manifeste
    # generique) ne pouvait donc jamais produire de payload, quel que soit
    # l'etat de son .hi reel (ailleurs sur le disque). Les 3 decodeurs geres
    # en dur (galaga/gyruss/1941) restent strictement reserves a fbneo
    # (roms fbneo, format .hi fbneo specifique, HI_DIR fbneo) -- un jeu
    # MAME homonyme (ex. un "1941" ou un clone galaga present aussi sous
    # mame) tombe desormais correctement dans le chemin generique, qui sait
    # retrouver le bon .hi quel que soit le core (voir HI_SEARCH_PATHS,
    # dmd_hiscore_generic.py v2).
    if [ "$sys" = "fbneo" ]; then
        case "$rom" in
            galaga|galaga84|galagab2|galagads|galagamf|galagamk|galagamw|galagao|gallag)
                hifile="${HI_DIR}/${rom}.hi"
                [ -f "$hifile" ] || { echo "$(date '+%H:%M:%S') SCORE skip $rom (pas de .hi)" >> "$LOG"; return; }
                size=$(wc -c < "$hifile" 2>/dev/null)
                if [ "$size" != "51" ]; then
                    echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 51)" >> "$LOG"
                    return
                fi
                score=$(decode_galaga_topscore "$hifile")
                echo "HI-SCORE ${score}"
                echo "$(date '+%H:%M:%S') SCORE $rom (GALAGA) -> ${score}" >> "$LOG" 1>&2
                return
                ;;
            gyruss)
                hifile="${HI_DIR}/${rom}.hi"
                [ -f "$hifile" ] || { echo "$(date '+%H:%M:%S') SCORE skip $rom (pas de .hi)" >> "$LOG"; return; }
                size=$(wc -c < "$hifile" 2>/dev/null)
                if [ "$size" != "43" ]; then
                    echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 43)" >> "$LOG"
                    return
                fi
                score=$(decode_gyruss_topscore "$hifile")
                echo "HI-SCORE ${score}"
                echo "$(date '+%H:%M:%S') SCORE $rom (GYRUSS) -> ${score}" >> "$LOG" 1>&2
                return
                ;;
            1941)
                hifile="${HI_DIR}/${rom}.hi"
                [ -f "$hifile" ] || { echo "$(date '+%H:%M:%S') SCORE skip $rom (pas de .hi)" >> "$LOG"; return; }
                size=$(wc -c < "$hifile" 2>/dev/null)
                if [ "$size" != "124" ]; then
                    echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 124)" >> "$LOG"
                    return
                fi
                topn=$(decode_1941_topN "$hifile" 5)
                echo "HI-SCORE|${topn}"
                echo "$(date '+%H:%M:%S') SCORE $rom (1941) -> ${topn}" >> "$LOG" 1>&2
                return
                ;;
        esac
    fi

    # v26 -- Phase 1 hi-score generique (voir memoire projet 2026-08-23) :
    # etend la couverture a tout jeu (fbneo ET mame desormais, v27) present
    # dans hiscore_manifest.json (~3100 jeux arcade via hi2txt-xml + Phase 2
    # statistique). dmd_hiscore_generic.py est silencieux (aucune sortie) si
    # le jeu n'est pas dans le manifeste, si le fichier ne fait pas la
    # taille attendue, ou en cas d'erreur -- meme prudence que les cas geres
    # en dur ci-dessus.
    topn=$(python3 "${PYHELP_DIR}/dmd_hiscore_generic.py" "$sys" "$rom" 2>>"$LOG")
    if [ -z "$topn" ]; then
        # v43 -- niveau 2 : table VERIFIEE MANUELLEMENT (demande utilisateur
        # explicite, 2026-09-04) -- pour un jeu ou meme une vraie partie
        # credit+jouee jusqu'au game over ne produit aucun .hi (aucune
        # adresse hiscore.dat ne s'arme pour ce jeu, ex. 1941/mame0278),
        # mais dont l'ecran hi-score interne du jeu a ete lu directement
        # sur une capture d'ecran reelle (methode "verite d'abord", jamais
        # invente) et enregistre via add_verified_score.py dans
        # verified_default_scores.json. TOUJOURS verifie APRES le .hi reel
        # ci-dessus (jamais prioritaire dessus) -- cede automatiquement sa
        # place des qu'un vrai .hi apparait, sans rien a nettoyer, car
        # dmd_hiscore_generic.py est appele EN PREMIER et $topn ne sera
        # alors plus vide.
        topn_verified=$(python3 "${PYHELP_DIR}/dmd_hiscore_verified.py" "$sys" "$rom" 2>>"$LOG")
        if [ -n "$topn_verified" ]; then
            echo "HI-SCORE|${topn_verified}"
            echo "$(date '+%H:%M:%S') SCORE $rom (VERIFIE-MANUEL) -> ${topn_verified}" >> "$LOG" 1>&2
            return
        fi
        # v41 -- repli PLACEHOLDER niveau 3 (demande utilisateur explicite,
        # 2026-09-04) : plutot que de ne rien afficher pour un jeu arcade
        # sans .hi reel ET sans table verifiee manuellement (niveau 2
        # ci-dessus), afficher une table factice "RE/CAL/BOX" (score 0)
        # pour que le panneau hi-score existe quand meme dans la rotation,
        # au lieu de sauter silencieusement ce jeu. PUREMENT un repli
        # d'AFFICHAGE -- ne touche JAMAIS le fichier .hi lui-meme
        # (contrairement a l'idee ecartee de creer un faux .hi sur disque :
        # teste ce soir, confirme qu'un .hi existant n'est PAS reecrase par
        # un simple chargement du jeu -- un vrai .hi finirait donc bloque
        # derriere un faux fichier si on l'ecrivait sur disque). Des qu'un
        # vrai .hi est peuple (campagnes de recolte en cours) OU qu'une
        # table verifiee manuellement est ajoutee (niveau 2), ce placeholder
        # disparait tout seul au prochain appel. Scope volontairement
        # restreint a l'arcade (fbneo/mame*) -- seuls ces systemes ont une
        # chance reelle d'etre un jour peuples par ces mecanismes (voir
        # memoire projet : le chantier hi-score generique est explicitement
        # limite a l'arcade, les consoles n'ont pas d'equivalent
        # hiscore.dat/.hi).
        case "$sys" in
            fbneo|mame*)
                echo "HI-SCORE|1 ShaN 2026|2 RecalBox 2026"
                echo "$(date '+%H:%M:%S') SCORE $rom (PLACEHOLDER, pas encore de .hi reel)" >> "$LOG" 1>&2
                return
                ;;
        esac
        echo "$(date '+%H:%M:%S') SCORE skip $rom (jeu non supporte)" >> "$LOG"
        return
    fi
    echo "HI-SCORE|${topn}"
    echo "$(date '+%H:%M:%S') SCORE $rom (GENERIQUE) -> ${topn}" >> "$LOG" 1>&2
}

# v6 -- point d'entree UNIQUE pour publier le hi-score d'un rom, utilise a
# la fois par round_robin() (type "hiscore") et par le handler endgame
# (refresh ponctuel en fin de partie) -- garantit le MEME rendu (rang 1 en
# gros, pagination 1941) dans les 2 cas.
# v13 -- $2=state_file/$3=expected optionnels, transmis a
# send_hiscore_paginated() -- endgame les omet (comportement inchange, un
# appel ponctuel hors round-robin n'a pas besoin d'etre interrompu).
# v27 -- $2=sys ajoute (necessaire a build_score_payload() pour distinguer
# fbneo des jeux MAME homonymes, voir son commentaire) -- state_file/
# expected decales en $3/$4.
publish_hiscore() {
    rom="$1"; sys="$2"; sf="$3"; exp="$4"
    payload=$(build_score_payload "$rom" "$sys")
    [ -n "$payload" ] || return 1
    title="${payload%%|*}"
    rest="${payload#*|}"
    if [ "$rest" = "$payload" ]; then
        # Pas de "|" du tout (jeu a valeur unique, galaga/gyruss).
        # v34 -- revalide l'etat ici : c'est le SEUL envoi de cette branche,
        # jamais garde jusqu'ici (voir changelog v34 en entete).
        state_still_valid "$sf" "$exp" || return 1
        send_score "$payload" "$HISCORE_INFO_PAGE_DURATION_MS" "$exp"
    else
        send_hiscore_paginated "$rest" "$sf" "$exp"
    fi
    return 0
}

# v6 -- publie UN SEUL type de panneau (appele par round_robin(), un a la
# fois -- remplace publish_slideshow() qui publiait le paquet complet).
# $1=sys $2=gpath $3=rom $4=type("hiscore"/"description"/"info")
# $5=state_file(optionnel) $6=expected(optionnel), transmis tels quels aux
# fonctions de pagination -- v13, voir state_still_valid(). Retourne
# 1 si rien publie -- round_robin() continue quand meme au tour suivant.
publish_one_panel() {
    sys="$1"; gpath="$2"; rom="$3"; type="$4"; sf="$5"; exp="$6"
    # v33 -- revalide l'etat ICI, avant tout appel Python/envoi (voir
    # changelog v33 en entete) -- ferme la fenetre de course ou l'appelant
    # (round_robin()) avait valide l'etat un instant plus tot, mais le
    # contexte a change PENDANT le python3 dmd_game_info.py qui suit.
    state_still_valid "$sf" "$exp" || return 1
    case "$type" in
        hiscore)
            # v27 -- BUG REEL corrige : restait gate sur fbneo uniquement
            # (voir commentaire complet dans build_score_payload()) -- tout
            # systeme arcade avec un rom connu peut desormais atteindre le
            # decodeur (generique ou special-case selon $sys).
            [ -n "$sys" ] && [ -n "$rom" ] || return 1
            publish_hiscore "$rom" "$sys" "$sf" "$exp"
            return $?
            ;;
        description)
            [ -n "$sys" ] && [ -n "$gpath" ] || return 1
            raw=$(get_game_info "$sys" "$gpath")
            [ -n "$raw" ] || return 1
            field=$(extract_field "$raw" "DESCRIPTION")
            [ -n "$field" ] || return 1
            title="${field%%|*}"; content="${field#*|}"
            send_paginated "$title" "$content" "$DESC_PAGE_DURATION_MS" "$DESC_PAGE_DURATION_S" "$sf" "$exp"
            return 0
            ;;
        info)
            [ -n "$sys" ] && [ -n "$gpath" ] || return 1
            raw=$(get_game_info "$sys" "$gpath")
            [ -n "$raw" ] || return 1
            field=$(extract_field "$raw" "INFOS")
            [ -n "$field" ] || return 1
            title="${field%%|*}"; content="${field#*|}"
            send_paginated_lines "$title" "$content" "$HISCORE_INFO_PAGE_DURATION_MS" "$HISCORE_INFO_PAGE_DURATION_S" "$sf" "$exp"
            return 0
            ;;
        challenge)
            # v17 -- classement communautaire du Challenge Recalbox du
            # mois -- voir changelog v17/v23 + dmd_challenge.py.
            # v23 -- garde challenge_session_active() RETIREE d'ici : elle
            # bloquait a tort le cas navigation (LAST_SYSTEMBROWSING_ID ==
            # "challenges", voir round_robin()) puisqu'aucun processus de
            # jeu ne tourne pendant une simple navigation. round_robin()
            # est desormais la SEULE source de verite pour decider QUAND
            # "challenge" fait partie de la rotation (session active EN
            # JEU via challenge_session_active(), OU navigation dans le
            # systeme virtuel "challenges" via LAST_SYSTEMBROWSING_ID) --
            # si on arrive ici, c'est deja legitime. dmd_challenge.py
            # verifie encore lui-meme sys/rom contre current.json (defense
            # en profondeur, silencieux si pas de correspondance).
            [ -n "$sys" ] && [ -n "$rom" ] || return 1
            lines=$(python3 "${PYHELP_DIR}/dmd_challenge.py" "$sys" "$rom" 2>>"$LOG")
            [ -n "$lines" ] || return 1
            send_paginated_lines "RB CHALLENGE" "$lines" "$HISCORE_INFO_PAGE_DURATION_MS" "$HISCORE_INFO_PAGE_DURATION_S" "$sf" "$exp"
            return 0
            ;;
    esac
    return 1
}

echo "$(date) - DMD score bridge started (v40, veille ciblee (round-robin pendant rundemo) desactivee au profit de la playlist simple (priorite stabilite) + v39, topic marquee/cmd/score fusionne dans marquee/cmd (CMD=/ARG=), voir RecalBox_DMD.ino v148 + v38, round_robin() retire hiscore de la rotation si indisponible pour le rom (evite un tour ~14s perdu) + v37, precise_ts()/ref ajoutes a send_score()/BROWSE/DWELL -- diagnostic desync overlay/marquee, voir DECISIONS.md + v36, helpers python (hiscore_generic/game_info/challenge) deplaces hors de userscripts/ (PYHELP_DIR=dmd_helpers/) -- ES ne peut plus les invoquer nativement sans limite, cause reelle de la saturation CPU en navigation turbo + verrou anti-relance deplace tout en haut du fichier (cout minimal par relance dupliquee ES) + fix race condition 1ere page/envoi non gardee (hiscore/desc/info) + fix race condition publish_one_panel() v33 -- revalide l'etat avant python3/envoi, startgameclip = marquee seul mais rundemo garde l'overlay complet + features_watcher valide le message avant d'ecraser le cache + hi-score generique + round-robin infini + dwell/ratios reglables)" >> "$LOG"
# Efface une session/etat perime d'un lancement precedent.
: > "$GAME_SESSION_FILE"
: > "$BROWSE_STATE_FILE"

LAST_BROWSE_SYS=""
LAST_BROWSE_ROM=""
# v22 -- SystemId du DERNIER evenement "systembrowsing" vu (voir
# changelog v22) -- distingue une navigation A L'INTERIEUR du systeme
# virtuel "challenges" (ES, collection "Challenges") d'une navigation
# normale, meme quand le jeu affiche est identique (l'evenement
# gamelistbrowsing, lui, rapporte toujours le systeme REEL du jeu,
# jamais le systeme virtuel d'ou on l'a atteint).
LAST_SYSTEMBROWSING_ID=""

# Connexion MQTT PERSISTANTE (une seule souscription, lue en continu).
mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "Recalbox/EmulationStation/Event" 2>/dev/null | \
while IFS= read -r event; do
    event=$(printf '%s' "$event" | tr -d '\r')

    case "$event" in
        systembrowsing)
            # v22 -- voir commentaire complet sur LAST_SYSTEMBROWSING_ID
            # plus haut et changelog v22.
            LAST_SYSTEMBROWSING_ID=$(read_state "SystemId")
            # v25 -- BUG REEL corrige (retour utilisateur : "survol
            # challenge = description qui s'affiche... alors qu'on demande
            # aucun affichage, c'est un systeme") : ce handler ne faisait
            # QUE mettre a jour LAST_SYSTEMBROWSING_ID, sans jamais arreter
            # un round_robin("browse") deja en vol depuis le DERNIER jeu
            # survole avant de remonter au niveau systeme -- meme motif
            # exact que le fix rungame/sleep (voir leurs commentaires
            # complets) deja applique ailleurs mais oublie ICI. Resultat :
            # remonter au niveau systeme (ou l'affichage doit etre reduit
            # au marquee/logo, comme n'importe quel systeme) laissait
            # l'ancien contenu (description/infos/challenge) continuer de
            # s'afficher indefiniment. Fix : meme nettoyage que les autres
            # handlers -- invalide BROWSE_STATE_FILE, la boucle en vol le
            # detectera a sa prochaine verification et s'arretera d'elle-
            # meme.
            : > "$BROWSE_STATE_FILE"
            LAST_BROWSE_SYS=""
            LAST_BROWSE_ROM=""
            ;;
        rungame)
            # v10 -- BUG REEL corrige (retour utilisateur explicite : "info
            # page1 - description page2 - hiscore - info page2 - marquee",
            # melange incoherent de plusieurs types de contenu apres le
            # lancement d'un jeu) : rungame n'effacait QUE GAME_SESSION_FILE,
            # jamais BROWSE_STATE_FILE -- un round-robin "browse" encore en
            # vol (ex. en plein envoi multi-pages, avec ses propres sleep()
            # internes) continuait donc de publier meme apres le lancement
            # du jeu, EN PARALLELE du nouveau round-robin "ingame" qui
            # demarre juste en dessous -- les 2 processus independants
            # publiaient alors sur le MEME canal marquee/cmd/score, melant
            # leurs contenus de facon imprevisible. Fix : effacer aussi
            # BROWSE_STATE_FILE ici -- le round-robin browse en vol
            # detectera la non-correspondance a la prochaine verification
            # (entre 2 pages/sleeps) et s'arretera de lui-meme, au lieu de
            # continuer indefiniment. Ne stoppe pas la page EN COURS
            # d'envoi (deja lancee, un sleep en cours ne peut pas etre
            # interrompu de l'exterieur) mais l'empeche de continuer
            # au-dela.
            : > "$BROWSE_STATE_FILE"
            LAST_BROWSE_SYS=""
            LAST_BROWSE_ROM=""
            system=$(read_state "SystemId")
            game_path=$(read_state "GamePath")
            echo "$(date '+%H:%M:%S') RUNGAME sys=$system path=$game_path" >> "$LOG"
            rom=""
            if [ -n "$system" ] && [ -n "$game_path" ]; then
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                session="${system}|${rom}"
                printf '%s\n' "$session" > "$GAME_SESSION_FILE"
                round_robin "ingame" "$system" "$game_path" "$rom" "$GAME_SESSION_FILE" "$session" "repeat_cycles" &
            fi
            ;;
        endgame)
            # Republie le score final (publication UNIQUE, separee du
            # round-robin -- inchange depuis v1, seul le hi-score a un
            # interet a etre rafraichi en fin de partie). Efface aussi la
            # session -- arrete le round-robin ingame eventuellement en vol.
            : > "$GAME_SESSION_FILE"
            system=$(read_state "SystemId")
            # v27 -- BUG REEL corrige : restait gate sur fbneo uniquement
            # (voir commentaire complet dans build_score_payload()).
            if [ -n "$system" ] && feat_enabled "hiscore_ingame"; then
                game_path=$(read_state "GamePath")
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                echo "$(date '+%H:%M:%S') ENDGAME $system rom=$rom" >> "$LOG"
                [ -n "$rom" ] && publish_hiscore "$rom" "$system"
            fi
            ;;
        stop)
            # v4 -- defensif : ES peut envoyer "stop" sans "endgame"
            # prealable -- efface quand meme la session.
            : > "$GAME_SESSION_FILE"
            ;;
        sleep)
            # v6 -- NOUVEAU cas explicite : la mise en veille doit arreter
            # TOUT round-robin en vol, ingame ET navigation.
            : > "$GAME_SESSION_FILE"
            : > "$BROWSE_STATE_FILE"
            LAST_BROWSE_SYS=""
            LAST_BROWSE_ROM=""
            echo "$(date '+%H:%M:%S') SLEEP (round-robin ingame/browse arretes)" >> "$LOG"
            ;;
        startgameclip|rundemo)
            # v41 - 2026-09-02 - safe-modify - retour utilisateur explicite
            # (priorite stabilite > fonctionnalite cosmetique -- "la
            # fonction veille ciblee n'est que cosmetique et ne pese rien
            # face au besoin de stabilite") : rundemo REJOINT desormais
            # startgameclip dans ce bloc no-op (round-robin JAMAIS demarre),
            # inverse du choix v31 ci-dessous qui l'en excluait
            # deliberement -- meme demarche que le fix v40 marquee.sh
            # (fusion des topics MQTT)/v38 round_robin() (hiscore) : reduire
            # l'EXPOSITION au blocage TX MQTT post-CONNACK plutot que de
            # continuer a le corriger a la source (mur de plateforme
            # atteint, voir DECISIONS.md/memoire projet). marquee.sh v41
            # applique le meme choix de son cote (rundemo/startgameclip ->
            # playlist simple au lieu de marquee+jeu demo) -- les 2 scripts
            # restent coherents entre eux.
            #
            # v31 - 2026-08-24 - safe-modify - BUG REEL trouve en enquetant
            # sur des deconnexions MQTT courtes (rc=-4) survenant pendant la
            # veille gameclip, correlees cote firmware a un rendu CMD_GAME/
            # MODE_GIF (gifRawPackMode=1) tenant le DMD occupe pour une duree
            # anormalement longue (~27s observes en direct) -- mecanisme de
            # fragilite keepalive deja documente depuis v83 (17/08,
            # PubSubClient/mqttClient.loop() bloque sur une lecture socket
            # lente), mais jamais autant EXPOSE qu'avec ce mode veille non
            # surveille (avant, ce chemin de rendu n'etait sollicite que
            # pendant une vraie partie jouee, rare/attendue). Retour
            # utilisateur : gameclip change de jeu toutes les ~30s pile --
            # rarement assez de temps pour qu'une sequence round-robin
            # hiscore/infos/description arrive a son terme avant d'etre
            # coupee par le clip suivant, donc autant ne jamais la demarrer
            # ici. Le marquee lui-meme (publie independamment par
            # marquee.sh, ce script n'y participe pas) reste affiche
            # normalement -- seul le DECLENCHEMENT du round-robin overlay
            # est desactive.
            #
            # rundemo (vrais lancements de jeu en veille demo) VOLONTAIREMENT
            # PAS inclus ici (retour utilisateur explicite, meme session) :
            # contrairement a gameclip (duree fixe ~30s), un jeu demo reste
            # affiche bien plus longtemps en pratique (temps de lancement/
            # presentation avant que le jeu ne soit reellement visible) --
            # assez de temps pour qu'une sequence complete ait une chance
            # d'aboutir. rundemo rejoint donc gamelistbrowsing ci-dessous,
            # comportement inchange pour ce cas precis.
            : > "$GAME_SESSION_FILE"
            LAST_BROWSE_SYS=""
            LAST_BROWSE_ROM=""
            # BROWSE_STATE_FILE tout de meme vide : invalide proprement tout
            # round-robin browse deja en vol issu d'une navigation humaine
            # precedente -- ne doit jamais continuer a s'afficher par-dessus
            # du contenu clip.
            : > "$BROWSE_STATE_FILE"
            ;;

        gamelistbrowsing)
            # v28 -- "rundemo" avait ete ajoute ici (retour utilisateur :
            # "en mode clip & demo afficher marquee + panneaux d'info
            # equivalent au survol de liste du jeu concerne") -- v41 (voir
            # son changelog complet pres du case startgameclip|rundemo)
            # ci-dessus) : retire d'ici, rejoint desormais startgameclip
            # dans le bloc no-op (veille ciblee desactivee, priorite
            # stabilite). Ce case ne traite plus QUE la vraie navigation
            # humaine (gamelistbrowsing), comportement inchange pour elle.
            #
            # v10 -- symetrique du fix rungame ci-dessus : un round-robin
            # "ingame" encore en vol (retour rapide a la liste juste apres
            # avoir quitte un jeu, avant qu'endgame/stop n'ait ete traite)
            # ne doit pas continuer a publier en parallele du dwell/
            # round-robin browse qui va demarrer.
            : > "$GAME_SESSION_FILE"
            system=$(read_state "SystemId")
            game_path=$(read_state "GamePath")
            if [ -n "$system" ] && [ -n "$game_path" ] && [ ! -d "$game_path" ]; then
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                # v25 -- BUG REEL corrige (retour utilisateur : "j'ai tous les
                # panneaux d'infos qui s'affichent" en navigant RB CHALLENGE,
                # alors qu'on est cense voir UNIQUEMENT le tableau challenge) :
                # $state ne dependait QUE de system/rom, jamais de
                # LAST_SYSTEMBROWSING_ID -- revisiter le MEME jeu (ex.
                # blazing star) d'abord via la collection "challenges" PUIS
                # via son vrai systeme (fbneo) produit la MEME chaine d'etat
                # ("fbneo|blazstar" dans les 2 cas, le SystemId d'un
                # gamelistbrowsing revient toujours au systeme REEL au niveau
                # jeu, voir memoire projet). L'ANCIEN round_robin() (fige sur
                # LAST_SYSTEMBROWSING_ID="challenges" au moment de son fork --
                # un sous-shell ne voit jamais les mises a jour ulterieures
                # d'une variable du parent) ne detectait donc JAMAIS de
                # changement d'etat et continuait de publier le tableau
                # challenge indefiniment, EN PARALLELE de la nouvelle boucle
                # normale (info/description) -- d'ou le melange des 2
                # affichages observe. Fix : LAST_SYSTEMBROWSING_ID injecte
                # DANS $state -- un changement de contexte de navigation
                # (meme sur le meme jeu) invalide desormais correctement
                # toute boucle round_robin() en vol.
                state="${LAST_SYSTEMBROWSING_ID}|${system}|${rom}"
                printf '%s\n' "$state" > "$BROWSE_STATE_FILE"
                if [ "$system" != "$LAST_BROWSE_SYS" ] || [ "$rom" != "$LAST_BROWSE_ROM" ]; then
                    LAST_BROWSE_SYS="$system"
                    LAST_BROWSE_ROM="$rom"
                    dwell=$(feat_value "dwell_seconds")
                    if [ "$dwell" -lt "$DWELL_MIN_SECONDS" ]; then dwell="$DWELL_MIN_SECONDS"; fi
                    echo "$(date '+%H:%M:%S') [$(precise_ts)] BROWSE sys=$system rom=$rom (dwell ${dwell}s)" >> "$LOG"
                    (
                        sleep "$dwell"
                        current=$(cat "$BROWSE_STATE_FILE" 2>/dev/null)
                        if [ "$current" = "$state" ]; then
                            # v23 -- round_robin() demarre desormais dans
                            # TOUS les cas -- c'est SON propre choix de
                            # type (voir v23 plus haut) qui decide entre
                            # "challenge" seul (navigation dans le systeme
                            # virtuel "challenges") et la rotation normale
                            # hiscore/infos/description (meme jeu via
                            # fbneo/favoris). Avant v23, ce cas court-
                            # circuitait round_robin() entierement -- retour
                            # utilisateur explicite : "jeu contenu dans
                            # challenge... on affiche le tableau challenge",
                            # PAS rien.
                            echo "$(date '+%H:%M:%S') [$(precise_ts)] DWELL settled sys=$system rom=$rom (dernier systeme survole: $LAST_SYSTEMBROWSING_ID) -- demarrage round-robin browse" >> "$LOG"
                            round_robin "browse" "$system" "$game_path" "$rom" "$BROWSE_STATE_FILE" "$state" "repeat_browse_cycles"
                        else
                            echo "$(date '+%H:%M:%S') [$(precise_ts)] DWELL abandoned sys=$system rom=$rom (deplace entre-temps)" >> "$LOG"
                        fi
                    ) &
                fi
            else
                LAST_BROWSE_SYS=""
                LAST_BROWSE_ROM=""
                : > "$BROWSE_STATE_FILE"
            fi
            ;;
        *)
            LAST_BROWSE_SYS=""
            LAST_BROWSE_ROM=""
            ;;
    esac
done
