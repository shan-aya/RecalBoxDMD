#!/bin/ash
# v27 -- verrou anti-relance (voir son commentaire complet plus bas, "v12")
# deplace ICI, tout en haut du fichier, AVANT tout le reste -- le verrou
# doit passer avant la MOINDRE commande executee (voir v43 ci-dessous pour
# le detail de cette contrainte, toujours respectee par l'extraction).
# v43 -- extrait vers dmd_helpers/singleton_lock.sh (nettoyage differe lors
# de la revue pre-merge master du 2026-09-03, voir DECISIONS.md
# "Explicitement PAS fait" -- repris ici) : ce bloc etait duplique a
# l'identique dans marquee.sh/dmd_score.sh/dmd_achievement.sh, seul le nom
# du verrou differait. Chemin ABSOLU (pas de calcul de SCRIPT_DIR ici --
# couterait un fork/exec supplementaire AVANT le verrou, exactement ce que
# ce fix v27 cherchait a eviter) -- coherent avec les autres chemins deja
# codes en dur dans ce fichier (ex. LOG plus bas). "|| exit 1" = garde
# fail-closed si dmd_helpers/ est absent (deja arrive une fois sur ce
# projet, package incomplet) : le script s'arrete plutot que de tourner
# sans protection anti-relance.
. /recalbox/share/userscripts/dmd_helpers/singleton_lock.sh marquee 2>/dev/null || exit 1
echo "$(date '+%H:%M:%S.%N') TRACE proceeding pid=$$ ppid=$PPID arg0=$0" >> /tmp/marquee_trace.log
LOG="/recalbox/share/system/logs/marquee_mqtt.log"
# v35 -- BUG REEL confirme sur materiel (retour utilisateur, meme session,
# apres v34 : le sondage direct de es_state.inf elimine bien toute
# contention MQTT -- log verifie : une seule publication propre et rapide
# par rafale, plus de flottement -- MAIS le delai residuel persiste, mesure
# PROPORTIONNEL a la duree de la navigation rapide qui precede, correle a
# un ralentissement generalise de la lecture video cote RB1 lui-meme, tous
# jeux confondus). Raisonnement utilisateur explicite qui a mene au fix :
# "si on lit vraiment le frontend RecalBox et plus ES, on ne devrait plus
# avoir de contention MQTT -- donc le shuffle ne devrait plus rester
# affiche". Vrai -- et confirme par le log (plus de contention MQTT). Le
# reste ne peut donc venir que de l'ORDONNANCEMENT du PROCESS marquee.sh
# lui-meme sous contention systeme reelle (mesuree ce jour : chute memoire
# ~400Mo/13s, 24% io-wait pendant la navigation) : sa boucle de sondage
# (POLL_INTERVAL_S=0.15, voir poll_navigation_position()) peut tourner
# beaucoup PLUS LENTEMENT en temps REEL que prevu si l'ordonnanceur ne lui
# donne pas de temps CPU a temps, meme si tout reste coherent en interne
# une fois qu'elle s'execute enfin (chaque tick mesure le temps ECOULE
# depuis le precedent via date +%s, pas un delai fixe garanti) -- plus la
# contention post-navigation est forte/longue (proportionnel a l'intensite
# de la navigation qui vient d'avoir lieu), plus la boucle elle-meme rate
# son rythme cible, plus la detection de fin de rafale et la publication
# finale sont retardees d'autant, INDEPENDAMMENT du fait que la logique/le
# MQTT restent corrects. Fix : le process qui gagne le verrou (donc le
# daemon persistant, jamais les invocations dupliquees qui sortent
# immediatement ci-dessus) s'auto-donne une priorite d'ordonnancement CPU
# elevee des le demarrage (renice, root -> valeurs negatives autorisees) --
# reste reactif meme quand RB1 est charge par autre chose (rendu ES,
# decodage video de la liste, etc.), au lieu de rivaliser a egalite pour le
# temps CPU. -10 (pas -20, le maximum) : marge significative sans risquer
# d'affamer completement d'autres taches legitimes sur un systeme deja
# charge. echec silencieux tolere (2>/dev/null) -- renice peut ne pas etre
# disponible/autorise sur certaines configurations, ne doit jamais empecher
# le script de demarrer.
renice -n -10 -p $$ >/dev/null 2>&1
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v43
#
# v43 - 2026-09-04 - safe-modify - Verrou anti-relance extrait vers
#   dmd_helpers/singleton_lock.sh -- code identique retire d'ici, de
#   dmd_score.sh (v41) et dmd_achievement.sh (v5), seul le nom du verrou
#   passe en argument. Nettoyage explicitement differe lors de la revue
#   pre-merge master du 2026-09-03 (voir DECISIONS.md, "Explicitement PAS
#   fait"), repris ce soir. Chemin ABSOLU utilise (pas de SCRIPT_DIR calcule
#   ici) : eviter un fork/exec supplementaire avant meme le verrou, exactement
#   ce que le fix v27 ci-dessous cherchait a eviter. Comportement au runtime
#   INCHANGE (meme LOCKDIR, meme logique mkdir/pid/kill -0) -- verifie via
#   `sh -n` sur les 4 fichiers concernes (ce script + les 2 autres +
#   singleton_lock.sh) avant commit. DEPLOIEMENT : dmd_helpers/ doit etre a
#   jour EN MEME TEMPS que ce script (deja une dependance obligatoire pour
#   dmd_score.sh -- pas un nouveau risque de deploiement, juste elargi a
#   marquee.sh/dmd_achievement.sh) -- garde fail-closed ajoutee ("|| exit 1")
#   si jamais dmd_helpers/ manquait au deploiement.
#
# v42 - 2026-09-03 - safe-modify - Fix collision de retain MQTT trouve par
#   revue de code avant passage sur master (jamais reproduit en direct,
#   voir RecalBox_DMD.ino v150 pour le detail complet). game/system/
#   default/ingame partageaient depuis v40 le meme topic retenu marquee/cmd
#   -- le retain MQTT etant PAR TOPIC, 2 send_mqtt_retain() consecutifs
#   (ex. rungame : "game" puis "ingame") ecrasaient silencieusement le
#   retain l'un de l'autre AU NIVEAU DU BROKER, defaisant le fix de
#   resynchronisation v123/v17 des qu'une reconnexion DMD survenait apres
#   une telle sequence. Fix : send_mqtt_retain() publie desormais vers
#   marquee/cmd/<nom> (topic dedie par etat) au lieu de marquee/cmd --
#   DEPLOIEMENT NON RETROCOMPATIBLE, ce script ET RecalBox_DMD.ino (v150+)
#   doivent etre a jour EN MEME TEMPS. Signature de send_mqtt_retain()
#   inchangee ($1=suffixe, $2=valeur), aucun appelant a modifier -- seul le
#   topic construit a l'interieur change. send_score()/les 2 envois
#   directs !SHUFFLE restent sur marquee/cmd non retenu, inchanges (jamais
#   concernes par cette collision).
#
# v41 - 2026-09-02 - safe-modify - Veille CIBLEE (marquee + jeu demo/clip
#   pendant rundemo/startgameclip) DESACTIVEE au profit de la PLAYLIST
#   simple, retour utilisateur explicite : "la fonction veille ciblee n'est
#   que cosmetique et ne pese rien face au besoin de stabilite". Meme
#   demarche que v40 (fusion des topics) : reduire l'exposition au blocage
#   TX MQTT post-CONNACK (mur de plateforme atteint, voir DECISIONS.md/
#   memoire projet) plutot que de continuer a le corriger a la source. Le
#   case rundemo|startgameclip) publie desormais "default" (playlist) UNE
#   SEULE FOIS par entree en veille (garde demo_veille_playlist_sent,
#   remise a 0 au reveil) au lieu de suivre le jeu demo en cours -- tout le
#   mecanisme de tracking SystemId/GamePath/DEMO_SYSTEM/DEMO_ROM reste en
#   place mais n'est plus atteint (return anticipe), conserve tel quel au
#   cas ou ce choix serait revu. dmd_score.sh v40 applique le meme choix de
#   son cote (rundemo rejoint startgameclip dans son bloc no-op round-robin)
#   -- les 2 scripts restent coherents entre eux.
#
# v40 - 2026-09-02 - safe-modify - FUSION des 12 topics marquee/cmd/* en
#   UN SEUL topic (marquee/cmd), voir RecalBox_DMD.ino v148 et DECISIONS.md
#   pour le detail complet du motif (reduire l'exposition au blocage TX
#   MQTT post-CONNACK cote DMD -- 12 topics = jusqu'a 24 SUBSCRIBE en
#   rafale par reconnexion, au-dessus du plafond d'environ 16 segments TCP
#   simultanement non-accuses trouve dans le sdkconfig du core ESP32).
#   send_mqtt_retain() (seul point d'envoi central, signature inchangee)
#   publie desormais vers marquee/cmd avec le payload "CMD=<suffixe>
#   ARG=<valeur>" au lieu de marquee/cmd/<suffixe> avec la valeur brute.
#   Les 2 envois directs !SHUFFLE (hors send_mqtt_retain, non retenus)
#   mis a jour de la meme facon. DEPLOIEMENT NON RETROCOMPATIBLE : ce
#   script ET RecalBox_DMD.ino doivent etre a jour EN MEME TEMPS (un
#   ancien firmware ne comprendrait plus rien publie par ce script v40+,
#   et inversement un firmware v148+ n'entendrait plus rien d'un ancien
#   marquee.sh <v40).
#
# v39 - 2026-09-01 - safe-modify - DIAGNOSTIC (pas de changement de
#   comportement fonctionnel) pour l'investigation "desync overlay/marquee
#   entre marquee.sh et dmd_score.sh" (DECISIONS.md, piste de depart deja
#   actee). precise_ts() (centieme de seconde via /proc/uptime -- "date"
#   ash/busybox n'exposant pas %N sur ce materiel, verifie) ajoute a la ligne
#   SEND(R) existante (send_mqtt_retain(), deja loguee avec le systeme/jeu
#   publie) -- meme horloge commune que dmd_score.sh v37, meme machine,
#   correlation directe sans decalage possible. Objectif : capturer au
#   prochain episode de desync reel le delai precis entre la publication du
#   marquee de fond (ce script) et celle de l'overlay hi-score/desc/info
#   (dmd_score.sh) pour un jeu different, condition prealable a valider avant
#   d'implementer le fix structurel deja envisage (state_file PARTAGE ou
#   autre).
#
# v38 - 2026-09-01 - safe-modify - Retrait du diagnostic pub_time (v36) --
#   voir commentaire complet pres de send_mqtt_retain(). Demande utilisateur
#   apres avoir corrige la vraie cause de la saturation CPU (dmd_score.sh
#   v36, helpers python deplaces) : le diagnostic avait rempli son role,
#   coutait desormais un fork+fichier temporaire par publication sans
#   justification.
#
# v37 - 2026-09-01 - safe-modify - REVERT du sondage continu v34 (retour
#   utilisateur explicite, APRES decouverte de la vraie cause de la
#   saturation CPU en navigation turbo -- voir DECISIONS.md et
#   dmd_score.sh v36 : ES invoquait nativement dmd_hiscore_generic.py/
#   dmd_game_info.py/dmd_challenge.py sans limite, INDEPENDAMMENT de ce
#   script -- confirme decisif par retrait physique de ces 3 fichiers,
#   CPU 99%->0%, 65C->45C, sans toucher a marquee.sh). Le sondage continu
#   de es_state.inf (poll_navigation_position(), POLL_INTERVAL_S=0.15s)
#   mesurait un cout reel et PERMANENT de ~13% d'UN COEUR EN CONTINU, MEME
#   A L'ARRET TOTAL (mesure directe /proc/pid/stat, jiffies utime+stime+
#   cutime+cstime sur 10s d'idle reel) -- sans ameliorer le probleme qu'il
#   visait a corriger (confirme non concluant sur materiel apres
#   deploiement, AVANT la decouverte ci-dessus). Vrai cout permanent,
#   benefice non demontre : retour a la lecture EVENEMENTIELLE (v28) +
#   restauration du case gamelistbrowsing|systembrowsing) complet (logique
#   v29/v31 inchangee, seulement redeplacee depuis poll_navigation_
#   position(), desormais fonction MORTE laissee definie mais plus jamais
#   appelee -- risque nul a la garder). v30 (vidange non-bloquante) RESTE
#   active. v35 (renice -10) et v36 (diagnostic pub_time) restent actifs
#   egalement (utiles independamment du modele de lecture).
#
# v36 - 2026-09-01 - safe-modify - DIAGNOSTIC (retour utilisateur : "60%
#   CPU c'est pas une saturation, le reseau est peut-etre libre" -- mesure
#   directe demandee au lieu de supposer). send_mqtt_retain() chronometre
#   desormais (mot-cle shell "time", precision sous-seconde confirmee
#   fonctionner sur cet ash/busybox) le temps REEL d'execution de l'appel
#   mosquitto_pub lui-meme, loggue en clair sur la ligne SEND(R). But :
#   determiner si la lenteur observee (delai proportionnel a la duree de
#   navigation, correle a un ralentissement RB1 general) vient de l'envoi
#   local (temps mesure ici qui grimperait) ou d'ailleurs (reseau/broker/
#   DMD, temps mesure ici qui resterait petit meme pendant le
#   ralentissement observe).
#
# v35 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel APRES v34
#   (retour utilisateur, meme session : plus de contention MQTT confirmee
#   par log, mais delai residuel PROPORTIONNEL a la duree de navigation
#   rapide, correle a un ralentissement video RB1 generalise). Raisonnement
#   utilisateur explicite : "si on lit vraiment le frontend RecalBox et plus
#   ES, on ne devrait plus avoir de contention MQTT -- donc le shuffle ne
#   devrait plus rester affiche". Verifie vrai. Le residu ne peut donc venir
#   que de l'ORDONNANCEMENT du process marquee.sh lui-meme sous contention
#   systeme reelle (mesuree ce jour : chute memoire ~400Mo/13s, 24% io-wait
#   pendant la navigation) -- sa boucle de sondage peut tourner plus
#   lentement en temps REEL que prevu si l'ordonnanceur ne lui donne pas de
#   temps CPU a temps. Fix : le daemon s'auto-renice a -10 des le
#   demarrage (juste apres l'acquisition du verrou, avant tout fork
#   ulterieur -- herite par mosquitto_sub et le sous-shell de la boucle
#   principale). Voir detail complet pres de l'appel "renice", juste apres
#   le verrou en tete de fichier.
#
# v34 - 2026-09-01 - safe-modify - REFONTE ARCHITECTURALE (demande explicite
#   utilisateur, meme session, apres v29-v33 : "on affiche toujours la queue
#   dans l'ordre, en plus lentement que la navigation sur RB1 -- je veux
#   qu'on se base sur Recalbox (le frontend) et pas sur ES pour la
#   navigation et l'affichage du DMD, pour etre dans le meme timing").
#   Mesure decisive (sonde MQTT independante, meme session, voir
#   DECISIONS.md) : un BURST reel a dure exactement 10s (start->end), avec
#   un flux CONTINU d'evenements de rattrapage (DRAIN) pendant ces 10
#   secondes entieres -- preuve que le retard n'etait pas un probleme de
#   traitement cote script (deja optimise par v30) mais le rythme de
#   publication d'ES lui-meme, structurellement variable et parfois tres
#   lent. Tant que la position de navigation affichee restait pilotee par
#   les evenements ES, AUCUNE optimisation de traitement ne pouvait la
#   rendre plus rapide que ce rythme.
#   Fix radical : la position de navigation (gamelistbrowsing/
#   systembrowsing) n'est PLUS DU TOUT pilotee par les evenements ES.
#   Nouvelle fonction poll_navigation_position() (voir son emplacement et
#   changelog complet), appelee a CHAQUE tick de la boucle principale --
#   qu'un evenement ES soit disponible ou non -- relit DIRECTEMENT
#   es_state.inf (deja etabli comme reflet FIDELE et quasi instantane de ce
#   que RB1/ES affiche reellement a l'ecran, independamment du retard de
#   publication de ses propres evenements -- voir DECISIONS.md/memoire
#   projet, section "il faut se baser sur RB pas sur ES"). Le detecteur de
#   rafale/doublon (v6/v16/v26/v29/v31, logique de detection INCHANGEE)
#   tourne desormais sur des positions ECHANTILLONNEES DANS LE TEMPS
#   (POLL_INTERVAL_S=0.15, "sleep" fractionnaire confirme fonctionner sur
#   cet ash/busybox contrairement a "read -t"/"date +%N") plutot que sur des
#   evenements RECUS -- insensible par construction au rythme de
#   publication d'ES.
#   Mecanisme de lecture reecrit en consequence : "read -t 0" (peek non-
#   bloquant, confirme fonctionner correctement) remplace le "read -t 1"/
#   blocage pur d'avant -- reactif instantanement si un evenement est deja
#   present (rungame/endgame/stop/sleep/wakeup/demo, INCHANGES, toujours
#   evenement-pilotes -- es_state.inf ne les distingue pas fiablement d'une
#   simple navigation), sinon "sleep $POLL_INTERVAL_S" puis nouveau tick.
#   demo_throttled (mode demo/clip, v18) adapte au meme principe : finalise
#   sur le temps ECOULE depuis le dernier evenement vu (demo_last_event_ts,
#   nouveau) a chaque tick, au lieu du timeout de lecture qui n'existe plus
#   sous cette forme -- comportement fonctionnel inchange.
#   Le case gamelistbrowsing|systembrowsing) devient un NO-OP (conserve
#   uniquement pour eviter le "*)" par defaut) -- toute sa logique (detecteur
#   de rafale, gestion boot_sweep_suppress, publication) vit desormais dans
#   poll_navigation_position(), avec la MEME logique de detection (seuils,
#   doublons, sustain) qu'avant v34, seule la source du "tick" change.
#
# v33 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel
#   IMMEDIATEMENT apres v32 (retour utilisateur : toujours "playlist" alors
#   qu'il navigue). v32 corrigeait le case start), mais celui-ci ne se
#   declenche QUE sur un vrai evenement ES "start" recu via MQTT -- pas au
#   lancement du PROCESS marquee.sh lui-meme. Or une ligne
#   "send_mqtt_retain default 1" totalement INCONDITIONNELLE existait tout
#   en haut du fichier (avant meme d'entrer dans la boucle d'ecoute MQTT),
#   executee a CHAQUE lancement du process (y compris mes redemarrages
#   manuels pour deployer un correctif pendant que RB reste actif en
#   continu, sans jamais publier de "start" -- contrairement a un vrai
#   reboot ES). Confirme dans marquee_mqtt.log : "SEND(R) marquee/cmd/
#   default = 1" apparait juste AVANT la banniere de demarrage v32, donc
#   le fix v32 n'avait jamais eu l'occasion de s'executer a ce redemarrage.
#   Fix : meme logique que v32 (lecture atomique es_state.inf, Action=
#   rungame ou gamelistbrowsing+position reelle -> publie cette position et
#   initialise LAST_SYSTEM/LAST_ROM/IN_GAME en consequence) appliquee ICI, au
#   tout premier point de publication du process, avant l'initialisation des
#   variables d'etat (voir detail complet pres de son emplacement). "default"
#   reste le comportement uniquement pour un demarrage genuinement idle.
#
# v32 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel (retour
#   utilisateur : "il est en playlist alors qu'il ne devrait pas puisqu'il
#   est connecte et que je suis sur la liste", apres un redemarrage de ce
#   script pendant que l'utilisateur naviguait activement sur un jeu
#   precis). Cause : le handler start) ne traitait specifiquement que
#   Action=rungame (v17) -- Action=gamelistbrowsing avec une position REELLE
#   deja atteinte (GamePath sur un fichier, pas juste un dossier/menu)
#   tombait dans le meme "else" que le cas genuinement idle, forcant
#   "default" (playlist) avant meme de verifier s'il y avait une vraie
#   selection en cours (confirme sur es_state.inf au moment du bug :
#   Action=gamelistbrowsing, GamePath=.../pangpoms.zip -- navigation reelle,
#   pas idle). Expose par les redemarrages repetes de ce script pendant
#   cette session de test (RB reste actif entre 2 redemarrages, contrairement
#   a un reboot complet) mais tout aussi reel lors d'un simple crash/relance
#   d'ES en cours de navigation. Fix : nouvelle branche elif dediee,
#   symetrique a rungame) -- publie directement la position REELLE lue dans
#   es_state.inf des le demarrage, sans jamais passer par "default" ni par
#   la fenetre de grace boot_sweep_pending (qui n'a de sens que pour un vrai
#   sweep de menu, pas pour une position de jeu deja connue et stable). Voir
#   le detail complet pres du handler start).
#
# v31 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel APRES
#   deploiement de v30 (retour utilisateur : "exactement le meme
#   comportement") -- le fix v30 (vidange des evenements de rattrapage en
#   retard) fonctionnait bien (confirme par les lignes "DRAIN N
#   evenement(s)" dans marquee_mqtt.log) mais ne traitait qu'UNE des 2
#   causes reelles du flottement shuffle/marquee/shuffle -- l'autre etant le
#   bug v29 (seuil de doublon a 1s trop fragile face a l'arrondi seconde
#   entiere) que j'avais diagnostique correctement puis REVERTE a tort suite
#   a une premiere lecture erronee du retour utilisateur (voir DECISIONS.md
#   pour le detail de cette correction). Reproduit a l'identique apres v30,
#   confirmant que c'etait bien un bug reel independant : a 14:05:32, "nsmb"
#   finalise a tort une rafale (doublon ES + ecart apparent de 1s) alors que
#   la navigation continuait reellement (evenements distincts individuels
#   juste apres, nouvelle vraie rafale reconnue seulement 2s plus tard).
#   Fix : seuil de "doublon = position stable" remonte de 1s a
#   EARLY_STABLE_SECONDS=2 (nouvelle constante dediee, meme valeur/
#   philosophie que BURST_SUSTAIN_SECONDS) -- voir le detail complet pres de
#   son usage (case gamelistbrowsing|systembrowsing). v30 et v31 sont
#   complementaires, pas redondants : v30 evite de RE-TRAITER inutilement
#   un paquet d'evenements de rattrapage deja accumule ; v31 evite de
#   CONCLURE A TORT qu'une rafale est terminee sur la foi d'un unique
#   doublon coincidant avec un changement de seconde horloge.
#
# v30 - 2026-09-01 - safe-modify - BUG REEL confirme (retour utilisateur,
#   meme session, immediatement apres v29 : "1 phase de shuffle et a
#   l'arret du defilement, on passe en defilement de marquee... puis de
#   nouveau l'image du shuffle... jusqu'a l'affichage du bon marquee" --
#   flottement repete). v29 (seuil de doublon) etait une fausse piste
#   (retour utilisateur explicite : le shuffle s'arretait bien au bon
#   moment reel -- le probleme etait APRES, pas pendant). Vraie cause
#   trouvee par sonde MQTT independante (mosquitto_sub brut, ne passe pas
#   par ce script) en parallele d'un poll direct de es_state.inf, sur la
#   MEME fenetre : le flux ES/Recalbox lui-meme se fige jusqu'a 4s SANS
#   emettre un seul evenement pendant que l'ecran RB1 change reellement (38
#   fois en 4s dans le test), puis deverse tout le retard d'un coup (22
#   evenements dans la MEME seconde a la reprise). Chaque evenement en
#   rafale de rattrapage coute plusieurs fork/exec (cat es_state.inf,
#   grep+cut x2-4, date x2-3, mosquitto_pub) a traiter individuellement --
#   ce script mettait plus d'1s a absorber un tel paquet de 22, pendant
#   lesquelles le DMD recevait un vrai defilement de marquees individuels
#   (throttled deja retombe a 0) puis un retour de rafale (seuil 5/s
#   re-atteint) avant de se stabiliser -- alors que RB avait DEJA fini de
#   naviguer et affichait la bonne image depuis le debut du paquet (retour
#   utilisateur : se baser sur RB/es_state.inf, pas sur le rythme d'ES).
#   Fix : des qu'un evenement est lu, verifie (lecture non bloquante
#   "read -t 0", teste et confirmee fonctionner correctement sous cet ash/
#   busybox) si d'autres lignes sont DEJA disponibles dans le pipe -- si
#   oui, saute directement a la PLUS RECENTE sans faire le traitement
#   complet des lignes intermediaires (aucun cat/grep/cut/date/
#   mosquitto_pub) ; seule la derniere est traitee normalement. Sans risque
#   de perte d'information : ce script lit deja es_state.inf directement
#   (pas le contenu de l'evenement) -- sauter des evenements-declencheurs
#   ne fait que sauter des LECTURES redondantes du meme etat RB, qui est
#   deja a jour au moment ou on le lit. Sans effet sur la navigation
#   normale (rien d'autre n'attend jamais dans le pipe entre 2 pas isoles,
#   la boucle de vidange ne s'execute donc jamais dans ce cas).
#
# v29 - 2026-09-01 - safe-modify - BUG REEL confirme (retour utilisateur +
#   video RB1+DMD filmes simultanement pour lever toute ambiguite d'horloge,
#   puis correlation precise avec marquee_mqtt.log -- voir DECISIONS.md pour
#   le detail complet). Le "delai shuffle" restant (cloture a tort en v28
#   comme "100% cote EmulationStation, rien a corriger cote script/firmware")
#   avait en fait une cause reelle et corrigeable de CE cote-ci. Preuve
#   directe : sur la video, le relachement manette est visible au moment ou
#   RB1 affiche la selection finale stabilisee (art+description charges,
#   "COMMANDO (SEGA)") -- confirme par le log correlé au meme timestamp
#   (12:40:02, evenement gamelistbrowsing sur commsega). Mais le DMD ne
#   recoit la vraie commande que 11s plus tard (12:40:13, "BURST end"),
#   pendant lesquelles marquee_mqtt.log montre ES republier gamelistbrowsing
#   pour la MEME rom (commsega, position deja atteinte, inchangee) en boucle,
#   plusieurs dizaines de fois. Cause : le detecteur de fin de rafale ne
#   connaissait qu'un seul signal (silence TOTAL >=1s, aucun evenement recu,
#   quel qu'il soit) -- un doublon (meme rom republiee) est un evenement
#   recu comme un autre pour ce detecteur, qui repousse donc le silence
#   necessaire encore d'1s a chaque fois, indefiniment tant que les
#   doublons arrivent plus vite qu'1/s. Fix : la position candidate
#   (system/rom) est desormais lue et comparee a LAST_SYSTEM/LAST_ROM AVANT
#   le detecteur de rafale (deplace depuis son ancien emplacement post-
#   compteur, v8) -- un doublon (position deja atteinte) ne compte plus
#   comme un survol pour le detecteur (n'incremente/ne prolonge plus
#   burst_count/burst_qualifying_streak/!SHUFFLE), et si une rafale ou un
#   sweep boot etait deja en cours, declenche IMMEDIATEMENT la meme
#   finalisation que le timeout de silence normal des qu'au moins 1s s'est
#   ecoulee depuis le dernier VRAI changement de position (nouvel
#   horodatage dedie last_real_change_ts, jamais mis a jour par un doublon)
#   -- au lieu d'attendre un silence total qui peut ne jamais survenir.
#   Aucun changement pour la navigation normale (positions qui changent a
#   chaque evenement, cas ecrasamment majoritaire) : le detecteur se
#   comporte exactement comme avant v29 dans ce cas.
#
# v28 - 2026-09-01 - safe-modify - BUG REEL confirme sur materiel (retour
#   utilisateur, saut alphabetique rapide : "la vitesse de defilement du
#   DMD semble plafonnee, plus basse que la navigation possible") -- mesure
#   precise sur une rafale de 58s : seulement 128 evenements captures par ce
#   script contre ~440 changements reels de es_state.inf sur la MEME
#   fenetre (poll independant 10x/s) -- ~70% des evenements REELS perdus.
#   Cause : relancer un "mosquitto_sub -C1 [-W1]" tout neuf a CHAQUE
#   iteration (fork+connexion TCP au broker a chaque fois) laisse une
#   fenetre ou plus personne n'ecoute pendant le traitement synchrone de
#   l'evenement precedent (cat es_state.inf + mosquitto_pub, chacun un
#   sous-processus separe) -- tout evenement publie par ES pendant cette
#   fenetre est perdu DEFINITIVEMENT (chaque "mosquitto_sub -C1" est une
#   souscription fraiche sans file d'attente entre 2 invocations). Fix :
#   UNE SEULE connexion mosquitto_sub PERSISTANTE (meme pattern deja en
#   place et jamais defaillant dans dmd_score.sh) -- "read -t 1" remplace
#   "-W 1" pour le timeout de detection de silence pendant une rafale,
#   "read" bloquant simple sinon, tous deux sur le MEME pipe deja ouvert --
#   plus aucun trou d'ecoute entre 2 evenements. Garde ajoutee : une lecture
#   BLOQUANTE (sans -t) qui echoue = pipe reellement ferme (broker mort),
#   sort proprement au lieu de boucler a 100% CPU (le verrou singleton
#   laisse la prochaine relance par ES redemarrer le script a neuf).
#
# v27 - 2026-08-31 - safe-modify - verrou anti-relance deplace tout en haut
#   du fichier (voir commentaire complet ci-dessus) -- reduit au minimum le
#   cout de chaque invocation dupliquee par EmulationStation pendant une
#   rafale de navigation.
#
# v26 - 2026-08-31 - safe-modify - BUG REEL corrige (retour utilisateur :
#   navigation SOUS le seuil reel de 5/s, le shuffle se declenche quand
#   meme au redemarrage de la navigation apres une pause, des le tout
#   premier marquee -- impossible d'avoir depasse 5/s a ce stade). Cause :
#   burst_qualifying_streak n'etait reevalue QUE quand un nouvel evenement
#   arrivait -- une pause (aucun evenement pendant plusieurs secondes)
#   laissait la variable figee a sa valeur d'avant la pause, et le premier
#   evenement de reprise finalisait la seconde PRECEDANT la pause (pas une
#   vraie rafale actuelle). Fix : la streak n'est desormais finalisee que
#   si le gap entre la nouvelle seconde et burst_window_start est <=1s
#   (contigu) -- une vraie pause la reinitialise immediatement a 0 au lieu
#   de completer un tour perime.
#
# v25 - 2026-08-25 - safe-modify - DIAGNOSTIC ajoute (retour utilisateur :
#   ressenti de "congestion" episodique cote RB, symptome le plus parlant
#   = le shuffle !SHUFFLE ne se declenche pas cote DMD meme en naviguant
#   vite -- gels/sauts de marquee visibles). burst_count/burst_qualifying_
#   streak (compteurs INTERNES du detecteur de rafale, voir v6/v16) etaient
#   calcules mais jamais logues -- seul le declenchement final ("BURST
#   start") l'etait. Ajoutes a la ligne BROWSE existante : permet de voir
#   directement si le compte reel plafonne SOUS BURST_THRESHOLD=5/s meme
#   pendant une navigation ressentie comme rapide (marquee.sh/ES en retard
#   de traitement plutot qu'un vrai defaut de navigation utilisateur).
#   Diagnostic pur, aucun changement de comportement.
#
# v24 - 2026-08-24 - safe-modify - DIAGNOSTIC ajoute (bsp/bss/bslp/now sur la
#   ligne BROWSE), PUIS RESULTAT LU EN DIRECT : le mecanisme v22/v23 est en
#   fait CORRECT, fausse alerte du test precedent (lecture du log SANS les
#   valeurs reelles, avant ce diagnostic). Preuve directe (reboot RB1,
#   12:43:20-21) : evenement "3do" -> bss=0, publie normalement ; evenement
#   "lastplayed" (MEME seconde) -> bss=1, correctement SUPPRIME par
#   BOOT_SWEEP_MIN_PUBLISH_INTERVAL_S (juste LAST_SYSTEM mis a jour en
#   silence) ; 1s plus tard, vrai silence -> "BOOT SWEEP termine" ->
#   publish_settled_position() publie la position FINALE reellement
#   atteinte ("lastplayed") -- c'est le flush de fin de sequence qui
#   fonctionne comme prevu (meme mecanisme que le mode demo), PAS une
#   republication en violation de la limite de frequence comme suppose a
#   tort au tour precedent. Champs de diagnostic laisses en place (cout
#   negligeable, utiles si un futur souci similaire doit etre debogue).
#   Reserve honnete : seul un sweep a 2 evenements (3do->lastplayed) a pu
#   etre reproduit ce soir (cache gamelist ES probablement deja chaud apres
#   plusieurs redemarrages) -- jamais le sweep complet a ~70 systemes de
#   l'incident d'origine. Le mecanisme est verifie correct sur le principe
#   (limite de frequence + flush final), pas stress-teste a cette echelle.
#
# v23 - 2026-08-24 - safe-modify - v22 BUGUE, reproduit a l'identique en test
#   reel immediat (2e reboot RB1 consecutif, meme session) : "BOOT SWEEP
#   termine" toujours declenche 1s apres BOOT settle, avant le vrai debut du
#   sweep (12:31:58 vs sweep reel a 12:32:39). Cause : boot_sweep_seen_any
#   etait bien calcule/mis a 1 dans le case gamelistbrowsing|systembrowsing),
#   mais JAMAIS EXIGE dans la condition de desarmement du timeout (oubli
#   pur et simple -- le code ecrit ne correspondait pas a l'intention
#   documentee dans le changelog v22). Fix reel : condition corrigee en
#   "[ boot_sweep_pending -eq 1 ] && [ boot_sweep_seen_any -eq 1 ]".
# v21 - 2026-08-24 - safe-modify - v20 INSUFFISANT, confirme en test reel
#   immediat (retour utilisateur : "test rb1 avec reboot" puis "vu qu il est
#   en rc-4 il va rien recevoir ton dmd" -- observation en direct qui a
#   permis de voir le vrai probleme). Test : kill+relance ES (12:09-12:10),
#   EVENT=start observe a 12:10:41, mais le sweep systembrowsing ne commence
#   QUE 42s plus tard (12:11:23 -- ES fait d'autres taches internes avant
#   d'attaquer sa liste de systemes). La fenetre de grace glissante v20 est
#   ancree sur BOOT_TIME et ne glisse QUE si des evenements arrivent deja --
#   avec un ecart initial de 42s (largement > 10s) avant le tout premier
#   evenement du sweep, la fenetre est deja perimee AVANT MEME que le sweep
#   commence -- le tout premier systembrowsing passe donc tel quel, sans
#   filtrage, exactement comme avant v20. Mecanisme v20 entierement remplace
#   (pas un correctif dessus) : boot_sweep_pending (arme dans start), dans
#   la branche "pas de vraie partie en cours") reste actif DEPUIS start)
#   JUSQU'AU PREMIER VRAI SILENCE observe (meme detection -W1 que fin de
#   rafale/mode demo) -- ne depend plus d'AUCUN delai fixe depuis le boot,
#   couvre le sweep quelle que soit sa date de debut ou sa duree reelle.
#   Pendant boot_sweep_pending, meme mecanisme de limite de frequence que le
#   mode demo (v18, DEMO_MIN_PUBLISH_INTERVAL_S) plutot qu'un blocage total
#   (un sweep peut durer largement plus d'une minute -- un DMD completement
#   fige tout ce temps serait percu comme casse) : boot_sweep_suppress
#   calcule une fois par evenement, combine (ET logique, jamais en
#   remplacement) avec le "throttled" existant aux 3 points de publication
#   du case gamelistbrowsing|systembrowsing). Desarme aussi immediatement
#   sur toute transition definitive (rungame/endgame/stop), meme philosophie
#   que throttled/burst_count -- une vraie action utilisateur ne doit jamais
#   rester en attente a cause d'un sweep suppose en cours.
#
# v20 - 2026-08-24 - safe-modify - BUG REEL trouve en enquetant sur un crash
#   firmware (retour utilisateur : "on a quand meme une serie de modif qui
#   amene ce crash a se produire... il faut enqueter"). Correlation directe
#   serial DMD <-> marquee_mqtt.log sur la fenetre exacte du crash (23/08
#   20:49-20:52) : AUCUN trafic bucket/CMD_GAME cote DMD pendant toute cette
#   fenetre (bucket et le chantier "veille" hors de cause pour CET incident
#   precis) -- en revanche, marquee.sh venait de redemarrer (20:49:43, reboot
#   RB/relance ES) puis ES a balaye la TOTALITE de sa liste de systemes en
#   interne au demarrage : ~70 EVENT=systembrowsing a la suite, ~1/s,
#   PENDANT PLUS D'UNE MINUTE (20:50:25 a >20:51:37), chacun republie tel
#   quel en marquee/cmd/system (throttled=0 sur chaque ligne du log). CE
#   FLUX ECHAPPE COMPLETEMENT aux 2 garde-fous existants :
#   - la fenetre de grace boot (BOOT_TIME, ci-dessous) ne couvrait que 10s
#     fixes -- trop courte, le balayage ES dure largement plus longtemps
#     pour une collection de systemes consequente ;
#   - le detecteur anti-rafale (BURST_THRESHOLD/BURST_SUSTAIN_SECONDS) ne
#     compte que les evenements dans la MEME seconde d'horloge -- un debit
#     de ~1/s ne l'atteint JAMAIS, quelle que soit la duree.
#   EXACTEMENT le meme trou architectural deja identifie et corrige pour le
#   mode demo (v18, "BUG REEL #2" -- taux modere mais SOUTENU, invisible au
#   detecteur instantane) -- mais ce fix (DEMO_MIN_PUBLISH_INTERVAL_S)
#   n'avait ete branche QUE sur le case rundemo|startgameclip, jamais sur
#   gamelistbrowsing|systembrowsing ou le meme trou existait depuis toujours
#   (present bien avant cette session, aucun commit anterieur n'a jamais
#   touche a ce mecanisme -- pas une regression bucket/resync/veille, un
#   angle mort pre-existant simplement jamais autant expose qu'avec un
#   redemarrage RB1 pendant cette session de tests intensifs).
#   Ce flux soutenu correlait directement, cote DMD, avec les cycles
#   d'echec de reconnexion MQTT observes dans la meme fenetre (charge SD/
#   MQTT/heap concurrente bien plus elevee que l'idle normal) et precede de
#   pres le crash observe (abort() a 20:52:46, <700ms apres un [MQTT]
#   connected) -- explique plausiblement AUSSI les symptomes "difficulte de
#   connexion/lenteur" rapportes comme co-occurrents, pas juste le crash
#   isole.
#   Fix retenu : fenetre de grace boot rendue GLISSANTE (BOOT_TIME remis a
#   "now" a CHAQUE evenement ignore pendant la grace) au lieu d'un delai fixe
#   one-shot -- se prolonge automatiquement tant que les evenements
#   continuent d'arriver rapprocjes (<10s d'ecart), se termine des le premier
#   vrai silence >=10s (fin du balayage automatise OU navigation humaine
#   normale, deja assez espacee). Aucun impact sur la navigation humaine
#   normale hors de cette fenetre post-boot (le detecteur de rafale usuel,
#   inchange, reste seul juge ensuite) -- meme philosophie que le mecanisme
#   de fin de rafale/demo deja existant (detection par silence, pas par
#   delai fixe).
#
# v19 - 2026-08-23 - safe-modify - BUG REEL trouve par retour utilisateur
#   direct ("il existe 1 mode demo : lance des jeux et un mode demo video :
#   lance des clips video de jeu") -- v18 n'avait cable que le premier
#   (screensaver.type=demo -> "rundemo"/"enddemo"). Teste en direct sur RB1
#   avec le 2e mode force (screensaver.type=gameclip, bascule faite par
#   l'utilisateur pendant que ce script ecoutait le flux MQTT en direct) :
#   evenements REELS "startgameclip"/"stopgameclip" (cette fois bel et bien
#   vivants, contrairement au diagnostic v18 -- qui restait correct POUR LE
#   MODE demo specifiquement, juste incomplet). Meme structure es_state.inf
#   (SystemId/GamePath peuples pendant startgameclip, identique a rundemo),
#   cadence stable ~30s/clip (jamais de rafale observee, contrairement au
#   mode demo qui peut atteindre ~1/s soutenu -- voir BUG REEL #2 du
#   changelog v18 ci-dessous). Fix : "startgameclip"/"stopgameclip" fusionnes
#   dans les cases rundemo)/enddemo) existants (memes patterns partages,
#   meme DEMO_SYSTEM/DEMO_ROM, meme limite de frequence 3s -- jamais genante
#   ici, 30s >> 3s). L'ancien traitement "legacy" (differe a la 1ere
#   occurrence via PREV_EVENT, puis silencieux) est retire -- remplace par
#   le meme mecanisme robuste (deduplication par contenu reel, pas par
#   position dans la sequence).
#
# v18 - 2026-08-23 - safe-modify - BUG REEL trouve par test en conditions
#   reelles (retour utilisateur : "en mode clip & demo afficher le marquee
#   du jeu concerne, comme un survol de liste, au lieu de la playlist ;
#   garder la playlist pour bouncing/dim/black") -- verification complete du
#   flux d'evenements ES avec screensaver.type force successivement sur les
#   4 valeurs (dim/black/bouncing/demo), fenetre longue (idle timer ne
#   demarre qu'apres le reglage complet du menu au boot, ~90-100s apres un
#   redemarrage ES, pas juste apres le delai configure) :
#   - dim/black/bouncing : un seul evenement "sleep", jamais repete --
#     comportement INCHANGE (playlist), deja correct.
#   - demo : evenements REELS "rundemo"/"enddemo" -- PAS
#     "startgameclip"/"stopgameclip" comme suppose depuis l'origine de ce
#     script (jamais declenches sur ce materiel, code mort garde par
#     prudence). es_state.inf peuple SystemId/GamePath pendant rundemo
#     exactement comme pendant un survol de liste -- nouveau case rundemo)
#     publie desormais game=system/rom (comme gamelistbrowsing), au lieu de
#     rester bloque sur la playlist affichee par le "sleep" qui precede.
#     DEMO_SYSTEM/DEMO_ROM dedies (jamais LAST_SYSTEM/LAST_ROM) pour ne pas
#     corrompre la position reelle de navigation restauree par wakeup).
#
#   BUG REEL #2 trouve en testant CE fix sur materiel (pas en theorie) :
#   ES peut enchainer les jeux demo a ~1/s de facon SOUTENUE pendant
#   plusieurs MINUTES (pas juste un pic isole) -- publier sans garde a
#   bloque le DMD indefiniment sur l'ecran d'attente (confirme : zero
#   "[MQTT] marquee/cmd/xxx ->" recu cote DMD pendant 3+ minutes, alors que
#   les topics retenus etaient bien a jour cote broker -- meme famille de
#   symptome que le rc=-4 deja documente, mais profil different : taux
#   modere SOUTENU dans la duree, pas un pic instantane, donc invisible au
#   detecteur de rafale "instantanee" existant (>=BURST_THRESHOLD dans la
#   MEME seconde -- 1/s reste toujours EN-DESSOUS de ce seuil). Fix : limite
#   de frequence dediee basee sur le temps ECOULE (DEMO_MIN_PUBLISH_INTERVAL_S
#   = 3s, pas un compteur par seconde) -- state separee (demo_throttled/
#   demo_last_publish_ts), jamais throttled/burst_* (ceux-la pilotent
#   publish_settled_position(), la position REELLE de navigation).
#
# v17 - 2026-08-23 - safe-modify - BUG REEL confirme par relecture de code
#   (retour utilisateur : verifier la resynchro DMD au demarrage/reconnexion
#   -- "au lieu d'afficher une playlist alors qu'un jeu est en cours") : le
#   handler start) (tourne a chaque (re)demarrage de ce script -- reboot RB
#   OU simple crash/relance d'ES pendant qu'une VRAIE partie tourne deja)
#   forcait INCONDITIONNELLEMENT send_mqtt_retain "default" "1", sans jamais
#   verifier si un jeu etait reellement en cours a cet instant -- le DMD (si
#   deja connecte, donc hors de sa fenetre de grace 5s post-connexion)
#   repassait alors immediatement en playlist locale alors que le joueur
#   etait toujours en jeu. Fix : lecture atomique de /tmp/es_state.inf (meme
#   motif que v8) AVANT toute publication -- le champ Action= (ecrit par ES
#   lui-meme) vaut "rungame" si une partie est reellement en cours ; dans ce
#   cas, publie game+ingame=1 (comme le ferait un vrai evenement rungame),
#   PAS default. Voir DECISIONS.md pour le detail complet et le volet
#   firmware associe (marquee/cmd/ingame reabonne cote DMD, v104 l'avait
#   retire).
#
# v16 - 2026-08-22 - safe-modify - Declenchement !SHUFFLE base sur une
#   DUREE soutenue au lieu d'un seuil instantane. Retour utilisateur apres
#   test reel du seuil 5/s (v15) : "ca fonctionne mais... visuellement il
#   se declenche un peu trop tot" -- le debit reel plafonne a 5-7/s (pas de
#   pics plus hauts observes), donc n'importe quel seuil fixe autour de 5
#   se declenche des la 1ere seconde ou le debit instantane depasse le
#   seuil, meme pour un survol juste un peu plus rapide que la normale, pas
#   forcement une vraie rafale soutenue. Nouveau : BURST_SUSTAIN_SECONDS
#   (secondes CONSECUTIVES a >=BURST_THRESHOLD requises avant de basculer
#   throttled=1) + burst_qualifying_streak (compteur de secondes pleines
#   consecutives qualifiees, evalue au moment ou le bucket seconde tourne --
#   necessite d'attendre la fin complete d'une seconde pour connaitre son
#   tally final, d'ou un delai de detection de ~1 seconde supplementaire
#   par seconde de sustain exigee). Valeur de depart : 2s. Reinitialise a
#   chaque point ou burst_count/throttled etaient deja remis a zero (fin de
#   rafale par timeout, boot, transition system/game reelle, endgame,
#   stop).
#
#   TODO (non fait, demande utilisateur 2026-08-22, "a mettre dans un
#   coin et a me le rappeler") : BURST_THRESHOLD/BURST_SUSTAIN_SECONDS
#   sont actuellement des constantes EN DUR ci-dessous, calibrees sur LE
#   materiel de l'utilisateur a CE jour. Raison de la demande : aucun
#   moyen de tester a l'avance un scenario de SD plus lente (ou autre
#   contrainte materielle) qui abaisserait le debit max soutenable --
#   souhait explicite de pouvoir ABAISSER ce seuil (voire DESACTIVER le
#   coupe-circuit entierement, cf BURST_THRESHOLD=50 en v11/v12 =
#   desactivation de fait) sans reflasher/redeployer un script, si un
#   souci apparait plus tard. Piste envisageable : reglage expose sur la
#   page web de config firmware (comme les 8 toggles hi-score/infos/RA
#   deja existants) puis lu depuis config.ini cote script, memes
#   conventions que le reste du sous-systeme "DMD bete" (voir section
#   dediee DECISIONS.md).
#
# v15 - 2026-08-22 - safe-modify - BURST_THRESHOLD 6 -> 5 (retour utilisateur
#   apres verification croisee avec la memoire projet du 2026-08-18 :
#   l'episode "32 connexions" cite a l'epoque etait "32 en ~30s", PAS 32/s
#   -- mais ce meme episode notait aussi "rafales de 6 connexions/seconde"
#   comme niveau des episodes precedents, et le choix historique du seuil
#   (v7, 3->5) etait explicitement motive par "reste facilement atteint
#   pendant un VRAI defilement rapide continu", PAS par une valeur pile au
#   plafond mesure). 6 (v14) colle exactement au maximum re-mesure ce jour
#   (6/s) -- trop fragile compte tenu du bucketing par seconde d'horloge
#   ENTIERE (voir limite v14 ci-dessous, toujours non resolue) : une rafale
#   reelle a cheval sur une frontiere de seconde peut ne JAMAIS atteindre un
#   seuil pile au plafond. 5 restaure la marge de securite voulue a
#   l'origine.
#
# v14 - 2026-08-22 - safe-modify - BURST_THRESHOLD 10 -> 6 (retour terrain :
#   "shuffle ne s'est pas declenche" sur une session de navigation rapide
#   reelle sur fbneo). Verifie sur marquee_mqtt.log (agrege sur toute la
#   journee, grep+uniq -c par seconde d'horloge) : le debit maximum JAMAIS
#   observe pour gamelistbrowsing sur une meme seconde entiere est 6/s, y
#   compris pendant cette session de test -- confirme aussi par les
#   timestamps milliseconde du serial DMD (3 events sur la seconde :32, 6
#   sur la seconde :33 de la meme rafale). Seuil de 10 structurellement
#   inatteignable au debit reel de la navigation rapide RB sur ce materiel
#   -- pas un bug du detecteur, juste une valeur de depart trop haute.
#   Limite connue non traitee ici : bucketing sur la seconde d'HORLOGE
#   ENTIERE (date +%s, voir commentaire v6 pres de BURST_THRESHOLD) peut
#   scinder une rafale soutenue a cheval sur une frontiere de seconde (ex.
#   3+6 events sur 2 secondes consecutives = 9 events en ~1s reel, mais
#   n'atteint jamais le seuil dans AUCUN des 2 buckets) -- fenetre glissante
#   non implementee, a envisager seulement si 6 s'avere encore insuffisant.
#
# v13 - 2026-08-22 - safe-modify - REACTIVATION du coupe-circuit
#   anti-rafale (BURST_THRESHOLD 50 -> 10, voir commentaire complet pres
#   de la constante) -- demande utilisateur explicite pour preserver la
#   SD/stabilite pendant le mode de navigation RAPIDE de RB, la cause du
#   rc=-4 etant depuis confirmee comme l'overclock RPi5+canicule (pas le
#   trafic MQTT local que le coupe-circuit visait a l'origine). Valeur de
#   DEPART (10), a ajuster apres test reel comme convenu avec
#   l'utilisateur.
#
# v12 - 2026-08-20 - safe-modify - Verrou anti-relance rendu ATOMIQUE (voir
#   commentaire complet pres de LOCKDIR plus bas) -- 4 instances simultanees
#   de dmd_achievement.sh (meme mecanisme de verrou) retrouvees vivantes le
#   meme jour, preuve que le fichier PID check-then-write n'etait pas
#   suffisant contre une rafale d'invocations quasi simultanees par
#   EmulationStation. mkdir (atomique) remplace le fichier PID simple.
#
# v11 - 2026-08-19 - safe-modify - Desactivation du coupe-circuit anti-rafale
#   (throttle + !SHUFFLE) SANS retirer le code -- BURST_THRESHOLD remonte de
#   5 a 50 survols/seconde, largement hors de portee d'une navigation humaine
#   meme tres rapide (les rafales mesurees cette session culminaient a 5-8/s).
#   Motivation : la cause principale du rc=-4/gels de navigation chassee
#   depuis plusieurs sessions semble in fine etre l'overclock RPi5 sous
#   canicule (voir memoire projet, decouverte du 2026-08-19), pas le trafic
#   MQTT local -- le coupe-circuit avait ete concu specifiquement pour
#   attenuer une correlation activite/rc=-4 qui n'a peut-etre jamais ete la
#   vraie cause. Test demande : navigation en direct sans throttle (comme
#   avant v6), pour voir si le systeme tient maintenant que l'hypothese
#   thermique/overclock est traitee. Mecanisme garde intact (juste rendu
#   inatteignable) -- seuil abaissable a nouveau instantanement si besoin.
#
# v10 - 2026-08-18 - safe-modify - Suppression du polling permanent (-W 1)
#   hors rafale. BUG REEL confirme sur materiel (log debug mosquitto, meme
#   jour) : le "-W 1" ajoute en v6 pour detecter la fin de rafale faisait
#   sortir la boucle mosquitto_sub TOUTES LES ~1s EN PERMANENCE, meme en
#   idle total sans aucune navigation -- pas seulement pendant une vraie
#   rafale comme prevu au design. Chaque sortie de boucle relance un NOUVEAU
#   mosquitto_sub, donc une NOUVELLE connexion locale (127.0.0.1) toutes les
#   secondes, en continu, 24h/24. Capture broker (log debug) : rafale de
#   connexions locales QUASI CONTINUE (~1/s, ~30 connexions/31s observees)
#   coincidant exactement avec "Client esp32-marquee has exceeded timeout,
#   disconnecting." -- ce trafic de fond genere par le script LUI-MEME est
#   un suspect direct pour le rc=-4 (le broker peine peut-etre a traiter la
#   session distante du DMD au milieu de ce bruit local constant). C'est
#   l'inverse du but recherche par le coupe-circuit anti-rafale (v6), qui
#   visait a REDUIRE le trafic MQTT local, pas a en ajouter en permanence.
#   Fix : le timeout "-W 1" n'est desormais utilise QUE pendant une rafale
#   effectivement en cours (throttled=1) -- seul moment ou une detection de
#   fin de rafale a un sens. Hors rafale (cas normal, largement majoritaire
#   en usage reel), retour a un mosquitto_sub -C 1 BLOQUANT SANS timeout,
#   comme avant v6 -- une connexion locale UNIQUEMENT quand un vrai
#   evenement ES arrive, zero trafic de fond en idle. Aucun changement de
#   comportement fonctionnel : le detecteur de rafale, le seuil, le
#   !SHUFFLE et la publication de fin de rafale restent identiques,
#   seulement actifs pendant la fenetre (courte, bornee) ou throttled=1.
#
# v9 - 2026-08-18 - safe-modify - Affichage transitoire pendant le throttle
#   ("coupe-circuit anti-rafale", v6). Au lieu de laisser le dernier marquee
#   reel fige a l'ecran pendant la fenetre de throttle, une publication
#   UNIQUE ("!SHUFFLE", NON retenue -- voir raison dans le code) declenche
#   au moment ou throttled passe a 1 une animation dediee (statique/
#   interference CRT, raw565pack pre-fabrique, genere via IA + convert_gif_
#   to_raw565pack_meta() du RecalBoxDMD_tool.py) qui boucle localement cote
#   firmware (RecalBox_DMD.ino v107, reutilise le mecanisme MODE_GIF/
#   gifResetCompat() deja existant -- aucun nouveau code de lecture/boucle).
#   La vraie position suit normalement au "BURST end". Toujours 2
#   publications max par rafale (debut + fin), pas plus qu'avant v9.
#
# v8 - 2026-08-18 - safe-modify - Lecture atomique SystemId/GamePath.
#   BUG REEL confirme sur materiel : SystemId et GamePath etaient lus par 2
#   appels SEPARES de read_state() (2 lectures separees de
#   /tmp/es_state.inf) -- pas atomique, EmulationStation pouvant reecrire ce
#   fichier ENTRE les deux lectures pendant un defilement rapide. Observe :
#   "raw=amiga600 game=.../roms/nes/xxx.zip" -- systeme et jeu incoherents,
#   publies tels quels -> image de repli cote DMD (chemin inexistant).
#   Symptome utilisateur : "images fallback a l'arret du defilement, corrige
#   par un aller-retour" (qui redeclenche une lecture, cette fois coherente
#   par hasard). Deja present avant le throttle (v6/v7) mais imperceptible
#   (corrige ~100ms plus tard par la publication suivante) -- devenu visible
#   car le throttle ne publie qu'UNE fois par rafale, sans rien pour
#   corriger une derniere lecture incoherente. Fix : lecture UNIQUE du
#   fichier (read_state_snapshot()), SystemId et GamePath extraits du MEME
#   instantane (extract_field()) -- plus de fenetre de race possible.
#
# v7 - 2026-08-18 - safe-modify - Ajustement seuil coupe-circuit anti-rafale.
#   Retour utilisateur apres test reel (v6, seuil 3/s) : "le 3/s un peu trop
#   restrictif" -- se declenchait pendant une navigation rapide mais encore
#   normale, pas seulement les vraies rafales extremes. BURST_THRESHOLD
#   remonte de 3 a 5 survols/seconde -- reste facilement atteint pendant un
#   VRAI defilement rapide continu (les logs v6 montraient des survols a
#   <100ms d'intervalle pendant les rafales), mais laisse plus de marge a
#   une navigation "rapide mais normale" avant de couper les publications.
#   Aucun autre changement de logique.
#
# v6 - 2026-08-18 - safe-modify - Coupe-circuit anti-rafale (burst throttle).
#   BUG REEL confirme sur materiel (session 2026-08-18, log broker mosquitto
#   correle avec le serial DMD, voir memoire projet) : une navigation rapide
#   dans une liste de jeux/systemes (usage NORMAL de l'interface RecalBox --
#   pas un cas limite) declenche une rafale de cycles connect/subscribe/
#   publish/disconnect locaux (ce script republie -- donc se reconnecte via
#   mosquitto_pub -- a CHAQUE jeu survole). Episode capture : 32 connexions
#   locales en ~30s pendant un scroll rapide, correle avec un decrochage
#   MQTT du DMD (rc=-4/timeout) au meme moment. Demande explicite
#   utilisateur : PAS de latence fixe ajoutee sur l'usage normal (le
#   firmware DMD est concu pour etre hyper-reactif, un delai systematique
#   de 1-2s "casserait" ce qui a ete construit) -- seulement couper
#   temporairement PENDANT une vraie rafale, et reprendre la reactivite
#   normale des que le defilement ralentit, sans attente artificielle
#   supplementaire au moment de la reprise.
#
#   Fonctionnement : compteur de survols (gamelistbrowsing/systembrowsing)
#   dans la MEME seconde horloge (precision seconde entiere -- ash/BusyBox
#   n'a pas d'horloge sub-seconde fiable partout, et ce n'est pas necessaire
#   ici : le seuil visé est "plusieurs survols dans la meme seconde", pas
#   un intervalle precis). BURST_THRESHOLD survols dans la meme seconde ->
#   bascule en mode "throttled" : les survols suivants ne publient PLUS
#   (LAST_SYSTEM/LAST_ROM restent a jour en interne, silencieusement) tant
#   que la rafale continue. mosquitto_sub est appele avec "-W 1" (timeout
#   1s) au lieu d'un blocage pur -- des qu'une iteration timeout (aucun
#   evenement recu pendant 1s complete), c'est le signal que la rafale
#   vient de s'arreter : publication IMMEDIATE de la position courante
#   (system/game tel qu'il a ete mis a jour silencieusement pendant le
#   throttle), retour en mode reactif normal. Navigation normale (un pas a
#   la fois, meme rapide) : le seuil (3 dans la meme seconde) n'est
#   quasiment jamais atteint, donc AUCUN changement de comportement --
#   publication immediate comme avant v6. rungame/endgame/stop (transitions
#   definitives, pas de simples survols) reinitialisent le compteur de
#   rafale -- une vraie action utilisateur ne doit jamais rester "en
#   attente" a cause d'un throttle en cours.
#
# v5 - 2026-08-18 - safe-modify - Verrou anti-relance (fichier PID). BUG REEL
#   confirme sur materiel (session 2026-08-17/18) : EmulationStation relance
#   ce script A CHAQUE evenement correspondant a un des noms entre crochets
#   du nom de fichier (gamelistbrowsing, systembrowsing, rungame, etc.) --
#   le suffixe "(permanent)" est une pure convention de nommage, RIEN cote
#   ES ne l'empeche de re-invoquer le script en plus de l'instance deja en
#   cours d'execution. Or ce script n'avait AUCUNE protection contre sa
#   propre re-invocation : chaque nouvel appel entrait dans sa propre boucle
#   "while true" infinie, sans jamais se terminer -- accumulation illimitee
#   de processus zombies au fil de la navigation (observe : plusieurs
#   instances simultanees, chacune avec son propre etat LAST_SYSTEM/
#   LAST_ROM desynchronise des autres, chacune consommant/publiant en
#   double sur les memes topics MQTT). Symptomes observes cote utilisateur :
#   navigation menu tres lente (fork+exec shell a chaque event), DMD affiche
#   un systeme perime alors que la RB est sur un autre, doublons "marquee.sh"
#   trouves a repetition toute la soiree precedente (deja documente v4).
#   Fix initial tente : flock -n sur un fd dedie -- ECARTE apres test reel,
#   un fd ouvert via "exec 9>" est HERITE par tout process fils issu d'un
#   fork() ulterieur (meme sans re-executer le flock), donc un fils qui
#   hérite du fd deja verrouille par son parent continue de tourner sans
#   jamais etre bloque -- observe sur materiel : 2 process actifs
#   simultanement partageant le meme fd 9 (verifie via /proc/PID/fd/9).
#   Fix retenu : verrou par FICHIER PID classique, insensible a l'heritage
#   de descripteur -- verifie via "kill -0" si le PID enregistre est un
#   process VIVANT (pas juste "le fichier existe", qui casserait apres un
#   crash/kill -9 sans nettoyage). Si vivant -> sortie immediate. Sinon
#   (fichier absent, ou PID mort/perime) -> on ecrit notre propre PID et on
#   continue normalement.
#
# v4 - 2026-08-17 - safe-modify - PORT depuis dev/mame-score-mqtt-bridge
#   (chantier reassignation de coeur, chunk 3 -- signal dedie "vraiment en
#   jeu"). Nouveau topic marquee/cmd/ingame ("1" sur rungame, "0" sur
#   endgame/stop) -- AJOUTE, ne remplace rien de l'existant. Necessaire cote
#   firmware (RecalBox_DMD.ino v79) : marquee/cmd/game est publie A LA FOIS
#   par un vrai lancement de partie (rungame) ET par un simple survol de la
#   liste des jeux (gamelistbrowsing, voir plus bas dans ce fichier -- meme
#   topic, meme format) -- le firmware ne peut pas distinguer les 2 a partir
#   de ce seul topic pour savoir s'il faut activer l'alternance hi-score/
#   game_info (cout heap/CPU non-nul, a eviter pendant un simple defilement
#   rapide de liste).

# v27 -- verrou + TRACE log qui vivaient ICI ont ete deplaces tout en haut
# du fichier (juste apres le shebang) -- voir le commentaire complet la-bas.
# Ne pas les reintroduire ici : LOCKDIR/LOG sont deja definis a ce stade.

read_state() {
    grep "^${1}=" "/tmp/es_state.inf" 2>/dev/null | cut -d= -f2- | tr -d '\r\n '
}

# v39 -- DIAGNOSTIC (retour utilisateur, desync overlay/marquee entre ce
# script et dmd_score.sh, voir DECISIONS.md "BUG TROUVE, PAS ENCORE CORRIGE
# -- desync overlay/marquee" -- meme ajout cote dmd_score.sh v37, voir son
# commentaire complet pour le detail). "date" ash/busybox n'expose pas %N
# (verifie sur ce materiel) -- /proc/uptime (centieme de seconde, "read"
# builtin, aucun fork) sert d'horloge commune aux 2 scripts pour correler
# precisement leurs publications respectives.
precise_ts() {
    read _pts_up _pts_rest < /proc/uptime 2>/dev/null
    echo "$_pts_up"
}

# v8 -- BUG REEL confirme sur materiel (session 2026-08-18) : SystemId et
# GamePath etaient lus par 2 appels SEPARES de read_state() (donc 2 lectures
# separees de /tmp/es_state.inf) -- pas atomique : EmulationStation peut
# reecrire ce fichier ENTRE les deux lectures pendant un defilement rapide,
# donnant un SystemId et un GamePath qui ne correspondent pas au meme
# instant (observe : "raw=amiga600 game=.../roms/nes/xxx.zip" -- systeme et
# jeu totalement incoherents). Consequence : LAST_SYSTEM/LAST_ROM se
# retrouvent desynchronises, publies tels quels -> chemin inexistant cote
# DMD -> image de repli affichee au lieu du vrai marquee. Ce bug existait
# deja avant le throttle (v6/v7) mais restait imperceptible : une lecture
# incoherente etait corrigee ~100ms plus tard par la publication suivante.
# Avec le throttle, si c'est la DERNIERE lecture avant la pause, rien ne la
# corrige -- d'ou le symptome "fallback a l'arret du defilement, corrige
# par un aller-retour" (qui redeclenche une lecture, cette fois coherente).
# Fix : lire le fichier UNE SEULE FOIS (un seul cat), extraire SystemId ET
# GamePath depuis ce MEME instantane -- plus aucune fenetre de race
# possible entre les deux champs.
read_state_snapshot() {
    cat "/tmp/es_state.inf" 2>/dev/null
}

extract_field() {
    # $1 = instantane (contenu multi-lignes), $2 = nom du champ
    echo "$1" | grep "^${2}=" | cut -d= -f2- | tr -d '\r\n '
}

send_mqtt_retain() {
    # v38 -- diagnostic pub_time (v36) retire (retour utilisateur, une fois
    # la vraie cause de la saturation CPU trouvee et corrigee -- voir
    # dmd_score.sh v36/DECISIONS.md) : avait rempli son role (confirme que
    # l'envoi local mosquitto_pub n'etait jamais la source de la lenteur,
    # orientant l'investigation vers la vraie cause) -- ajoutait un cout non
    # nul et permanent (sous-shell "time" + fichier temporaire a CHAQUE
    # publication) desormais sans justification.
    # v40 -- topic marquee/cmd/${1} -> marquee/cmd unique (voir DECISIONS.md
    # + RecalBox_DMD.ino v148) : 12 topics fusionnes en 1 seul cote DMD pour
    # reduire l'exposition au blocage TX post-CONNACK (jusqu'a 24 SUBSCRIBE
    # en rafale par reconnexion avant ce fix, largement au-dessus du
    # plafond d'environ 16 segments TCP simultanement non-accuses trouve
    # dans le sdkconfig du core). Payload : "CMD=<ancien suffixe> ARG=<$2>"
    # -- CMD toujours en 1er (mot simple), ARG toujours en dernier (prend
    # tout le reste jusqu'a la fin cote DMD, compatible avec des valeurs a
    # espaces/pipes comme le score). Signature de cette fonction INCHANGEE
    # ($1=suffixe, $2=valeur) -- aucun appelant a modifier.
    # v42 -- topic dedie marquee/cmd/${1} (voir changelog v42 en tete de
    # fichier) : ${1} vaut toujours game/system/default/ingame ici (seuls
    # suffixes utilises avec send_mqtt_retain() dans tout ce script), donc
    # ce chemin ne cree jamais plus de 4 topics distincts.
    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -r -t "marquee/cmd/${1}" -m "CMD=${1} ARG=$2" 2>/dev/null
    # v39 -- precise_ts() ajoute (voir sa declaration complete) : diagnostic
    # desync overlay/marquee, correlation avec les logs dmd_score.sh v37.
    echo "$(date '+%H:%M:%S') [$(precise_ts)] SEND(R) marquee/cmd/${1} = $2" >> "$LOG"
}

normalize_system() {
    local sys="$1"
    case "$sys" in
        *) echo "$sys" ;;
    esac
}

# v6 -- publie la position courante (system+rom silencieusement mis a jour
# pendant un throttle) -- factorise car appelee a la fois par la fin de
# rafale (timeout) et potentiellement reutilisable ailleurs.
publish_settled_position() {
    if [ "$IN_GAME" -eq 1 ]; then
        return
    fi
    if [ -n "$LAST_ROM" ]; then
        send_mqtt_retain "game" "${LAST_SYSTEM}/${LAST_ROM}"
    elif [ -n "$LAST_SYSTEM" ]; then
        send_mqtt_retain "system" "$LAST_SYSTEM"
    fi
}

# v34 -- BUG REEL confirme sur materiel a plusieurs reprises meme apres v30/
# v31 (retour utilisateur explicite, 2026-09-01 : "on affiche toujours la
# queue dans l'ordre, en plus lentement que la navigation sur RB1 -- je veux
# qu'on se base sur Recalbox (le frontend) et pas sur ES pour la navigation
# et l'affichage du DMD, pour etre dans le meme timing"). Tous les fix
# precedents (v29/v31 seuils, v30 vidange) reduisaient l'IMPACT du retard/
# de la rafale de rattrapage du flux d'evenements ES, mais restaient
# fondamentalement DEPENDANTS de son rythme de publication -- mesure directe
# (sonde MQTT independante, meme session) : un BURST reel a dure tout juste
# 10s (start->end), avec un flux CONTINU d'evenements de rattrapage (DRAIN)
# pendant ces 10 secondes entieres -- ES a genuinement mis 10 secondes REELLES
# a finir de publier, quelle que soit l'efficacite du traitement cote script.
# Cette fonction remplace entierement la dependance a l'evenement ES pour la
# position de navigation : appelee a CHAQUE tick de la boucle principale
# (evenement recu OU simple sondage, voir POLL_INTERVAL_S et son usage plus
# bas), elle relit es_state.inf DIRECTEMENT (deja etabli comme reflet FIDELE
# et quasi instantane de ce que RB1 affiche reellement, independamment du
# retard de publication des evenements -- voir DECISIONS.md/memoire projet)
# -- le detecteur de rafale/doublon (v6/v16/v26/v29/v31, logique inchangee)
# tourne desormais sur des positions echantillonnees dans le temps plutot que
# sur des evenements recus, ce qui le rend insensible par construction au
# rythme de publication d'ES : que la position ait ete atteinte il y a 10ms
# ou 10s ne change rien, seul l'etat ACTUEL du fichier compte a chaque tick.
poll_navigation_position() {
    if [ "$IN_GAME" -eq 1 ]; then
        return
    fi
    now=$(date +%s)

    # v21 -- inchange (voir changelog v21/v22 complet) : rate-limiting
    # pendant un sweep boot detecte -- reste pertinent sous le sondage (un
    # sweep interne d'ES rapide se traduit par des positions echantillonnees
    # qui changent a chaque tick, meme mecanisme de detection que pour une
    # vraie rafale de navigation).
    boot_sweep_suppress=0
    if [ "$boot_sweep_pending" -eq 1 ]; then
        if [ $((now - boot_sweep_last_publish_ts)) -lt "$BOOT_SWEEP_MIN_PUBLISH_INTERVAL_S" ]; then
            boot_sweep_suppress=1
        else
            boot_sweep_last_publish_ts=$now
        fi
    fi

    _snap=$(read_state_snapshot)
    system_raw=$(extract_field "$_snap" "SystemId")
    system=$(normalize_system "$system_raw")
    game_path=$(extract_field "$_snap" "GamePath")
    _cand_rom=""
    if [ -n "$game_path" ] && [ ! -d "$game_path" ]; then
        _cand_rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
    fi
    _position_unchanged=0
    if [ -n "$game_path" ] && [ ! -d "$game_path" ]; then
        [ "$_cand_rom" = "$LAST_ROM" ] && [ "$system" = "$LAST_SYSTEM" ] && _position_unchanged=1
    elif [ -n "$system" ]; then
        [ "$system" = "$LAST_SYSTEM" ] && [ -z "$LAST_ROM" ] && _position_unchanged=1
    else
        return
    fi

    # v34 -- boot_sweep_seen_any (voir changelog v22) desormais marque ICI,
    # des qu'un tick de sondage voit une position DIFFERENTE de la derniere
    # connue pendant boot_sweep_pending -- equivalent du "au moins un
    # evenement du sweep vu" d'avant, adapte au sondage.
    if [ "$boot_sweep_pending" -eq 1 ] && [ "$_position_unchanged" -eq 0 ]; then
        boot_sweep_seen_any=1
    fi

    if [ "$_position_unchanged" -eq 0 ]; then
        if [ "$now" = "$burst_window_start" ]; then
            burst_count=$((burst_count + 1))
        else
            if [ "$burst_window_start" -gt 0 ] && [ $((now - burst_window_start)) -le 1 ] && [ "$burst_count" -ge "$BURST_THRESHOLD" ]; then
                burst_qualifying_streak=$((burst_qualifying_streak + 1))
            else
                burst_qualifying_streak=0
            fi
            burst_window_start="$now"
            burst_count=1
        fi
        if [ "$burst_qualifying_streak" -ge "$BURST_SUSTAIN_SECONDS" ] && [ "$throttled" -eq 0 ]; then
            throttled=1
            echo "$(date '+%H:%M:%S') BURST start (seuil $BURST_THRESHOLD/s soutenu sur ${BURST_SUSTAIN_SECONDS}s) [poll]" >> "$LOG"
            mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd" -m "CMD=game ARG=!SHUFFLE" 2>/dev/null
            echo "$(date '+%H:%M:%S') SEND !SHUFFLE (non retenu)" >> "$LOG"
        fi
        last_real_change_ts="$now"
    else
        if [ "$throttled" -eq 1 ] && [ $((now - last_real_change_ts)) -ge "$EARLY_STABLE_SECONDS" ]; then
            throttled=0
            burst_count=0
            burst_qualifying_streak=0
            echo "$(date '+%H:%M:%S') BURST end (position stable ${EARLY_STABLE_SECONDS}s+) [poll]" >> "$LOG"
            publish_settled_position
        fi
        if [ "$boot_sweep_pending" -eq 1 ] && [ "$boot_sweep_seen_any" -eq 1 ] && [ $((now - last_real_change_ts)) -ge "$EARLY_STABLE_SECONDS" ]; then
            boot_sweep_pending=0
            echo "$(date '+%H:%M:%S') BOOT SWEEP termine (position stable) [poll]" >> "$LOG"
            publish_settled_position
        fi
        return
    fi

    echo "$(date '+%H:%M:%S') BROWSE raw=$system_raw norm=$system game=$game_path in_game=$IN_GAME throttled=$throttled bsp=$boot_sweep_pending bss=$boot_sweep_suppress bslp=$boot_sweep_last_publish_ts now=$now bc=$burst_count bqs=$burst_qualifying_streak [poll]" >> "$LOG"

    if [ -n "$game_path" ]; then
        if [ -d "$game_path" ]; then
            if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
                LAST_SYSTEM="$system"
                LAST_ROM=""
                if [ "$throttled" -eq 0 ] && [ "$boot_sweep_suppress" -eq 0 ]; then
                    send_mqtt_retain "system" "$system"
                fi
            fi
        else
            rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
            if [ -n "$system" ] && [ -n "$rom" ]; then
                LAST_SYSTEM="$system"
                LAST_ROM="$rom"
                if [ "$throttled" -eq 0 ] && [ "$boot_sweep_suppress" -eq 0 ]; then
                    send_mqtt_retain "game" "${system}/${rom}"
                fi
            fi
        fi
    elif [ -n "$system" ]; then
        if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
            LAST_SYSTEM="$system"
            LAST_ROM=""
            if [ "$throttled" -eq 0 ] && [ "$boot_sweep_suppress" -eq 0 ]; then
                send_mqtt_retain "system" "$system"
            fi
        fi
    fi
}

LAST_SYSTEM=""
LAST_ROM=""
IN_GAME=0
BOOT_TIME=0
PREV_EVENT=""

# v33 -- BUG REEL confirme sur materiel (retour utilisateur : "il est en
# playlist alors qu'il ne devrait pas puisqu'il est connecte et que je suis
# sur la liste", 2026-09-01) -- v32 corrigeait le case start) (declenche par
# un vrai evenement ES "start" recu sur le topic MQTT), mais CETTE ligne
# (avant meme d'entrer dans la boucle d'ecoute) s'executait
# INCONDITIONNELLEMENT a CHAQUE lancement du PROCESS lui-meme (le mien y
# compris, a chaque redemarrage manuel pour deployer un correctif) --
# confirme dans marquee_mqtt.log : "SEND(R) marquee/cmd/default = 1"
# apparaissait juste AVANT la banniere de demarrage v32, donc le case
# start) corrige par v32 n'avait jamais eu l'occasion de s'executer a ce
# redemarrage precis (aucun evenement MQTT "start" n'est necessairement
# publie par ES a chaque fois que CE SCRIPT redemarre -- ES, lui, tourne
# deja en continu). Fix : meme logique que v32 (lecture atomique de
# es_state.inf, meme motif read_state_snapshot()/extract_field() que v8/v17)
# appliquee ICI, au tout premier point de publication du process -- si
# Action=rungame ou Action=gamelistbrowsing avec une position REELLE deja
# atteinte (GamePath sur un fichier), publie directement cette position
# (et initialise LAST_SYSTEM/LAST_ROM/IN_GAME en consequence, pas juste la
# publication) au lieu de "default". "default" reste le comportement
# seulement pour un demarrage genuinement idle (menu, pas de selection).
_boot0_snap=$(read_state_snapshot)
_boot0_action=$(extract_field "$_boot0_snap" "Action")
_boot0_game_path=$(extract_field "$_boot0_snap" "GamePath")
if [ "$_boot0_action" = "rungame" ] && [ -n "$_boot0_game_path" ]; then
    _boot0_sys=$(normalize_system "$(extract_field "$_boot0_snap" "SystemId")")
    _boot0_rom=$(basename "$_boot0_game_path" | sed 's/\.[^.]*$//; s/ //g')
    if [ -n "$_boot0_sys" ] && [ -n "$_boot0_rom" ]; then
        IN_GAME=1
        LAST_SYSTEM="$_boot0_sys"
        LAST_ROM="$_boot0_rom"
        send_mqtt_retain "game" "${_boot0_sys}/${_boot0_rom}"
        send_mqtt_retain "ingame" "1"
    else
        send_mqtt_retain "default" "1"
    fi
elif [ "$_boot0_action" = "gamelistbrowsing" ] && [ -n "$_boot0_game_path" ] && [ ! -d "$_boot0_game_path" ]; then
    _boot0_sys=$(normalize_system "$(extract_field "$_boot0_snap" "SystemId")")
    _boot0_rom=$(basename "$_boot0_game_path" | sed 's/\.[^.]*$//; s/ //g')
    if [ -n "$_boot0_sys" ] && [ -n "$_boot0_rom" ]; then
        LAST_SYSTEM="$_boot0_sys"
        LAST_ROM="$_boot0_rom"
        send_mqtt_retain "game" "${_boot0_sys}/${_boot0_rom}"
    else
        send_mqtt_retain "default" "1"
    fi
else
    send_mqtt_retain "default" "1"
fi
# v18 -- dedies au mode demo (rundemo/enddemo, voir leur case) : jamais
# LAST_SYSTEM/LAST_ROM directement, pour ne pas corrompre la position REELLE
# de navigation que wakeup) doit restaurer au reveil.
DEMO_SYSTEM=""
DEMO_ROM=""
# v41 -- garde pour n'envoyer "default" (playlist) qu'UNE SEULE FOIS par
# entree en veille demo/gameclip, pas a chaque changement de jeu demo (voir
# le case rundemo|startgameclip) plus bas, meme motif que demo_throttled
# ci-dessous mais pour un evenement different) -- remis a 0 uniquement au
# reveil (wakeup)), jamais a chaque enddemo/stopgameclip individuel (qui
# fire a CHAQUE jeu demo, pas juste a la fin de la session de veille).
demo_veille_playlist_sent=0
# v18 suite -- BUG REEL trouve en test reel (ES peut enchainer les jeux
# demo a ~1/s de facon SOUTENUE pendant plusieurs minutes -- pas juste un
# pic isole) : le detecteur de rafale "instantanee" (>=BURST_THRESHOLD
# evenements dans la MEME seconde d'horloge, celui de gamelistbrowsing) plus
# bas) ne se declenche JAMAIS ici -- 1 evenement/s reste sous ce seuil, peu
# importe la duree. Sans garde, chaque rundemo publiait immediatement
# (mosquitto_pub, UNE connexion locale par appel) -- confirme en direct :
# ~1/s soutenu pendant plus de 3 minutes a bloque le DMD indefiniment sur
# l'ecran d'attente (jamais un seul "[MQTT] marquee/cmd/xxx ->" recu apres
# la reconnexion, alors que les topics retenus etaient bien a jour cote
# broker) -- meme famille de symptome que le rc=-4 deja documente dans la
# memoire projet (rafale de connexions locales), mais sur un PROFIL DIFFERENT
# (taux modere mais SOUTENU dans la duree, pas un pic instantane). Fix :
# limite de frequence simple basee sur le temps ECOULE depuis la derniere
# publication (pas un compteur par seconde) -- publie immediatement si le
# jeu demo change ET qu'au moins DEMO_MIN_PUBLISH_INTERVAL_S se sont
# ecoules depuis la derniere publication, sinon met a jour DEMO_SYSTEM/
# DEMO_ROM silencieusement (aucun cout MQTT) et laisse le mecanisme de fin
# de rafale (timeout -W1, ci-dessous) publier la position enfin stabilisee.
DEMO_MIN_PUBLISH_INTERVAL_S=3
demo_last_publish_ts=0
demo_throttled=0
# v34 -- horodatage du dernier evenement rundemo/startgameclip vu (pas du
# dernier PUBLIE) -- permet de finaliser demo_throttled sur le temps ECOULE
# a chaque tick de boucle, au lieu du timeout de lecture "-t 1" retire en
# v34 (voir changelog v34 complet pres de poll_navigation_position()).
demo_last_event_ts=0

# v21 -- remplace l'ancienne fenetre de grace boot fixe/glissante (v20,
# voir changelog v21 complet en entete pour le detail de son insuffisance
# constatee en test reel : le sweep systeme d'ES peut ne commencer que 40+
# secondes apres l'evenement start, largement hors de portee d'une fenetre
# ancree sur un delai depuis le boot). boot_sweep_pending reste actif DEPUIS
# start) JUSQU'AU PREMIER vrai silence observe (meme detection -W1 que fin
# de rafale/demo) -- couvre le sweep quelle que soit sa date de debut/duree
# reelle. Pendant boot_sweep_pending, meme mecanisme de limite que le mode
# demo (v18) : publication au maximum toutes les
# BOOT_SWEEP_MIN_PUBLISH_INTERVAL_S secondes, position mise a jour
# silencieusement sinon -- jamais un silence TOTAL (contrairement a une
# rafale courte, un sweep peut durer plus d'une minute, un DMD fige tout ce
# temps serait percu comme casse).
BOOT_SWEEP_MIN_PUBLISH_INTERVAL_S=3
boot_sweep_pending=0
boot_sweep_last_publish_ts=0
# v22 -- BUG REEL trouve en test reel IMMEDIAT sur v21 (reboot RB1 complet,
# meme session) : "BOOT SWEEP termine (silence reel observe)" s'est
# declenche a 12:19:55, SEULEMENT 1s apres "BOOT settle", alors que le vrai
# sweep systembrowsing n'a demarre QUE 35s plus tard (12:20:30) -- le
# timeout -W1 (1s sans evenement) se declenchait sur le silence NORMAL
# precedant le sweep (ES encore en train de charger/initialiser autre
# chose), pas sur sa fin -- boot_sweep_pending se desarmait donc AVANT MEME
# que le sweep commence, qui passait ensuite entierement non filtre, meme
# symptome qu'avant v20 pour une raison differente. Fix : le timeout
# n'implique "sweep termine" que si AU MOINS UN evenement
# systembrowsing/gamelistbrowsing a deja ete vu depuis l'armement (voir
# boot_sweep_seen_any, mis a 1 dans le case correspondant) -- tant qu'aucun
# n'est encore arrive, un silence n'est que le delai normal AVANT le sweep,
# pas sa fin, et ne doit jamais desarmer.
boot_sweep_seen_any=0

# v6 -- etat du detecteur de rafale (voir changelog v6 ci-dessus).
# v7 -- seuil remonte de 3 a 5 (retour utilisateur : 3 trop restrictif).
# v11 -- seuil remonte a 50 (desactivation de fait, voir changelog v11 --
# a l'epoque, la cause du rc=-4 semblait pouvoir etre le trafic MQTT local
# genere par le coupe-circuit lui-meme, pas confirme).
# v13 -- REACTIVATION (demande utilisateur explicite, 2026-08-22) : la
# cause du rc=-4 est depuis confirmee comme l'overclock RPi5+canicule (voir
# memoire projet), pas le trafic MQTT local -- plus de raison de garder le
# coupe-circuit desactive. Objectif reaffirme : preserver la SD/stabilite
# specifiquement pendant le mode de navigation RAPIDE de RB (l'utilisateur
# a demande si RB "saute" par lettres au lieu de parcourir jeu par jeu en
# mode rapide -- verifie que ca ne change rien a l'approche : chaque
# position atteinte, meme par saut, publie un vrai evenement
# gamelistbrowsing, seul le DEBIT de ces evenements compte pour ce
# detecteur). Seuil remis a 10 (valeur de DEPART a ajuster sur test reel,
# demande explicite "5 est trop bas teste en reel essaye 10 et on
# modifiera" -- ni le 5 d'origine (juge trop bas cette fois) ni le 50
# (equivalent a desactive), point de depart intermediaire pour tester.
BURST_THRESHOLD=5
# v16 -- voir changelog v16 : nombre de secondes CONSECUTIVES a
# >=BURST_THRESHOLD requises avant de declencher !SHUFFLE (au lieu
# d'un declenchement instantane des la 1ere seconde qui depasse le seuil).
BURST_SUSTAIN_SECONDS=2
burst_window_start=0
burst_count=0
burst_qualifying_streak=0
throttled=0
# v29 -- horodatage du dernier VRAI changement de position (pas du dernier
# evenement recu, quel qu'il soit) -- voir changelog v29 et son usage dans
# le case gamelistbrowsing|systembrowsing plus bas.
last_real_change_ts=0
# v31 -- secondes ecoulees (bucketing entier, voir changelog v31) depuis
# last_real_change_ts exigees avant de considerer un doublon ES comme une
# preuve de position stabilisee. 2, pas 1 -- meme valeur/philosophie que
# BURST_SUSTAIN_SECONDS, marge contre l'arrondi a la seconde entiere.
EARLY_STABLE_SECONDS=2
# v34 -- cadence de sondage direct de es_state.inf quand aucun evenement
# n'est deja disponible dans le pipe (voir poll_navigation_position() et son
# changelog complet) -- "sleep" supporte les valeurs fractionnaires sur cet
# ash/busybox (teste et confirme, contrairement a "read -t" et "date +%N").
# 0.15 = compromis entre reactivite ("meme timing que RB1", demande
# explicite utilisateur) et cout (un cat/grep/cut par tick meme en idle,
# /tmp est en tmpfs -- pas de cout SD reel).
POLL_INTERVAL_S=0.15

# v13 -- echo de demarrage deplace ICI (apres l'assignation de
# BURST_THRESHOLD) : place plus haut dans le fichier (juste avant
# send_mqtt_retain "default"), la variable n'existait pas encore au moment
# de l'interpolation et le log affichait "seuil=/s" (vide) au lieu de
# "seuil=10/s" -- bug constate au demarrage reel, corrige en deplacant le
# log apres la declaration.
echo "$(date) - Marquee bridge started (v41, veille ciblee demo/gameclip desactivee au profit de la playlist simple (priorite stabilite) + v40, 12 topics marquee/cmd/* fusionnes en 1 seul marquee/cmd (CMD=/ARG=), voir RecalBox_DMD.ino v148 -- reduit l'exposition au blocage TX MQTT + v39, precise_ts()/proc-uptime ajoute a la ligne SEND(R) -- diagnostic desync overlay/marquee, voir DECISIONS.md + v38, diagnostic pub_time retire (role rempli, cout desormais injustifie) + REVERT du sondage continu v34 -- retour a la lecture EVENEMENTIELLE (zero cout CPU en idle ; vraie cause de la saturation CPU en navigation turbo trouvee ailleurs -- ES invoquait nativement les helpers python hi-score sans limite, voir dmd_score.sh v36) + doublons ES ignores par le detecteur de rafale/sweep boot (seuil ${EARLY_STABLE_SECONDS}s, marge anti-arrondi seconde entiere) + vidange non-bloquante du pipe (read -t 0) sur les evenements de rattrapage ES en retard + auto-renice -10 au demarrage (herite par mosquitto_sub/mosquitto_pub/sous-shell) + publication initiale du PROCESS et demarrage sur vrai evenement ES start) tiennent compte de es_state.inf (Action=rungame/gamelistbrowsing+position reelle) au lieu de forcer playlist inconditionnellement (v32/v33) + pipe mosquitto_sub PERSISTANT + verrou anti-relance en tete de fichier, coupe-circuit anti-rafale seuil=$BURST_THRESHOLD/s sur ${BURST_SUSTAIN_SECONDS}s consecutives, lock atomique acquis)" >> "$LOG"

# v28 -- BUG REEL confirme sur materiel (retour utilisateur : "la vitesse
# de defilement du DMD semble plafonnee, plus basse que la navigation
# possible" -- mesure precise : 128 evenements captures par ce script
# contre ~440 changements reels de es_state.inf sur la MEME fenetre de 58s,
# soit ~70% des evenements reels PERDUS). Cause : relancer un
# "mosquitto_sub -C1 [-W1]" TOUT NEUF a CHAQUE iteration (fork+connexion
# TCP au broker a chaque fois) laisse une fenetre ou plus personne n'ecoute
# pendant le traitement synchrone de l'evenement precedent (cat es_state.inf
# + mosquitto_pub, chacun un sous-processus separe) -- tout evenement publie
# par ES pendant cette fenetre est perdu DEFINITIVEMENT (chaque
# "mosquitto_sub -C1" est une souscription fraiche, ne recoit que ce qui
# est publie APRES sa connexion, aucune file d'attente entre 2 invocations).
# Fix : UNE SEULE connexion mosquitto_sub PERSISTANTE (meme pattern deja
# utilise et jamais defaillant dans dmd_score.sh, voir son main loop) --
# "read -t 1" remplace "-W 1" pour le timeout de detection de silence
# pendant une rafale, "read" bloquant simple sinon, tous deux sur le MEME
# pipe deja ouvert -- plus aucun trou d'ecoute entre 2 evenements.
mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "Recalbox/EmulationStation/Event" 2>/dev/null | \
while true; do
    PREV_EVENT="$event"
    # v37 -- REVERT du sondage continu v34 (retour utilisateur, 2026-09-01,
    # apres decouverte de la VRAIE cause de la saturation CPU en navigation
    # turbo -- voir DECISIONS.md et dmd_score.sh v36 : ES invoquait
    # nativement dmd_hiscore_generic.py/dmd_game_info.py/dmd_challenge.py
    # sans limite, INDEPENDAMMENT de ce script -- confirme decisif par
    # retrait physique de ces 3 fichiers de userscripts/, seul fix reel,
    # CPU 99%->0%, 65C->45C). Le sondage continu de es_state.inf
    # (poll_navigation_position(), POLL_INTERVAL_S=0.15s) mesurait un cout
    # reel et permanent de ~13% d'UN COEUR EN CONTINU, MEME A L'ARRET TOTAL
    # (mesure directe via /proc/pid/stat, jiffies utime+stime+cutime+cstime
    # sur 10s d'idle reel) -- sans ameliorer le probleme qu'il visait a
    # corriger (confirme non concluant sur materiel APRES deploiement). Vrai
    # cout permanent, benefice non demontre : retour a la lecture
    # EVENEMENTIELLE (v28, "read -t 1" pendant une rafale/mode
    # demo/boot sweep, blocage pur sinon -- zero cout CPU en idle reel,
    # seulement des sous-process forkes en reaction a un vrai evenement
    # recu) + restauration du case gamelistbrowsing|systembrowsing) complet
    # (logique v29/v31 : detecteur de rafale/doublons, EARLY_STABLE_SECONDS)
    # -- INCHANGEE depuis avant v34, simplement redeplacee ici depuis le
    # corps de poll_navigation_position() (fonction desormais MORTE, laissee
    # definie mais plus jamais appelee -- risque nul a la garder, evite de
    # toucher a du code par ailleurs correct). v30 (vidange non-bloquante)
    # RESTE active -- toujours utile face a un paquet d'evenements de
    # rattrapage ES deja accumule, quel que soit le modele de lecture.
    if [ "$throttled" -eq 1 ] || [ "$demo_throttled" -eq 1 ] || [ "$boot_sweep_pending" -eq 1 ]; then
        IFS= read -r -t 1 event
        readRc=$?
    else
        IFS= read -r event
        readRc=$?
        # v28 -- lecture BLOQUANTE (sans -t) qui echoue = pipe reellement
        # ferme (mosquitto_sub mort/broker inaccessible), PAS un timeout
        # (impossible sans -t) -- continuer en boucle serree consommerait
        # 100% CPU pour rien. Sort proprement -- le verrou singleton laisse
        # la prochaine relance par ES (evenement quelconque) redemarrer
        # le script a neuf.
        if [ "$readRc" -ne 0 ]; then
            echo "$(date '+%H:%M:%S') mosquitto_sub pipe fermee (broker injoignable ?) -- sortie propre, relance attendue au prochain evenement ES" >> "$LOG"
            exit 1
        fi
    fi
    event=$(printf '%s' "$event" | tr -d '\r')

    # v30 -- BUG REEL confirme sur materiel (retour utilisateur + sonde MQTT
    # independante, 2026-09-01 -- voir DECISIONS.md/memoire projet). Le flux
    # ES/Recalbox lui-meme peut se figer plusieurs secondes SANS emettre le
    # moindre evenement pendant qu'un scroll rapide continue reellement a
    # l'ecran, puis deverser tout le retard d'un coup. Fix : des qu'un
    # evenement est lu, verifie (lecture non bloquante "read -t 0",
    # comportement confirme correct sous cet ash/busybox) si D'AUTRES
    # lignes sont DEJA disponibles dans le pipe -- si oui, saute directement
    # a la PLUS RECENTE sans faire le traitement complet des lignes
    # sautees ; seule la toute derniere est ensuite traitee normalement.
    # Plafond de securite (2000 iterations) purement defensif.
    _drain_n=0
    while read -t 0 -r _drain_peek 2>/dev/null; do
        IFS= read -r _drain_next
        [ $? -ne 0 ] && break
        event=$(printf '%s' "$_drain_next" | tr -d '\r')
        _drain_n=$((_drain_n + 1))
        [ "$_drain_n" -ge 2000 ] && break
    done
    if [ "$_drain_n" -gt 0 ]; then
        echo "$(date '+%H:%M:%S') DRAIN $_drain_n evenement(s) en retard sautes, traitement de la position finale uniquement" >> "$LOG"
    fi

    if [ -z "$event" ]; then
        # v6 -- timeout : si une rafale etait en cours, elle vient de
        # s'arreter (aucun survol depuis >=1s) -- publier la position
        # courante MAINTENANT et repasser en mode reactif normal.
        if [ "$throttled" -eq 1 ]; then
            throttled=0
            burst_count=0
            burst_qualifying_streak=0
            echo "$(date '+%H:%M:%S') BURST end -- publication position stabilisee" >> "$LOG"
            publish_settled_position
        fi
        # v18 suite -- meme principe, limite de frequence demo.
        if [ "$demo_throttled" -eq 1 ]; then
            demo_throttled=0
            if [ -n "$DEMO_SYSTEM" ] && [ -n "$DEMO_ROM" ]; then
                demo_last_publish_ts=$(date +%s)
                echo "$(date '+%H:%M:%S') DEMO sequence rapide terminee -- publication position stabilisee" >> "$LOG"
                send_mqtt_retain "game" "${DEMO_SYSTEM}/${DEMO_ROM}"
            fi
        fi
        # v21/v22 -- meme principe pendant boot_sweep_pending, seulement si
        # au moins un evenement du sweep a deja ete vu.
        if [ "$boot_sweep_pending" -eq 1 ] && [ "$boot_sweep_seen_any" -eq 1 ]; then
            boot_sweep_pending=0
            echo "$(date '+%H:%M:%S') BOOT SWEEP termine (silence reel observe) -- publication position stabilisee, reactivite normale retablie" >> "$LOG"
            publish_settled_position
        fi
        continue
    fi

    echo "$(date '+%H:%M:%S') EVENT=$event IN_GAME=$IN_GAME LAST_SYS=$LAST_SYSTEM LAST_ROM=$LAST_ROM" >> "$LOG"

    case "$event" in

        start)
            IN_GAME=0
            LAST_ROM=""
            LAST_SYSTEM=""
            BOOT_TIME=$(date +%s)
            burst_count=0
            burst_qualifying_streak=0
            throttled=0
            last_real_change_ts=$(date +%s)

            # v17 -- voir changelog d'entete : ne force plus "default" sans
            # verifier d'abord si une vraie partie est en cours (Action=
            # dans es_state.inf, ecrit par ES lui-meme) -- lecture atomique
            # (meme motif que v8, voir read_state_snapshot()) pour eviter
            # toute race entre Action/SystemId/GamePath.
            _snap=$(read_state_snapshot)
            _action=$(extract_field "$_snap" "Action")
            # v32 -- calcule ICI (une seule fois, reutilise par les 2
            # branches rungame/gamelistbrowsing ci-dessous) -- voir
            # changelog v32.
            _boot_game_path=$(extract_field "$_snap" "GamePath")
            if [ "$_action" = "rungame" ]; then
                system_raw=$(extract_field "$_snap" "SystemId")
                game_path=$(extract_field "$_snap" "GamePath")
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
                system=$(normalize_system "$system_raw")
                echo "$(date '+%H:%M:%S') BOOT (ES redemarre EN JEU) -> sys=$system rom=$rom" >> "$LOG"
                if [ -n "$system" ] && [ -n "$rom" ]; then
                    IN_GAME=1
                    LAST_SYSTEM="$system"
                    LAST_ROM="$rom"
                    send_mqtt_retain "game" "${system}/${rom}"
                    send_mqtt_retain "ingame" "1"
                else
                    send_mqtt_retain "default" "1"
                fi
            elif [ "$_action" = "gamelistbrowsing" ] && [ -n "$_boot_game_path" ] && [ ! -d "$_boot_game_path" ]; then
                # v32 -- BUG REEL confirme sur materiel (retour utilisateur :
                # DMD reste en playlist apres un (re)demarrage de ce script
                # alors qu'il est deja connecte ET que l'utilisateur navigue
                # activement sur un jeu precis -- "il est en playlist alors
                # qu'il ne devrait pas puisqu'il est connecte et que je suis
                # sur la liste"). Cause : seul Action=rungame etait traite
                # specifiquement (voir v17) -- Action=gamelistbrowsing avec
                # une position REELLE deja atteinte (GamePath pointant sur un
                # fichier, pas un dossier -- meme test que le case
                # gamelistbrowsing plus bas) tombait dans le meme "else" que
                # le cas genuinement idle/menu, forcant "default" (playlist)
                # avant meme de verifier s'il y avait une vraie selection en
                # cours. Expose par les redemarrages repetes de ce script
                # pendant cette session de test (RB reste actif entre 2
                # redemarrages, contrairement a un reboot complet) mais tout
                # aussi reel lors d'un simple crash/relance d'ES en cours de
                # navigation. Fix : meme traitement que rungame) -- publie
                # directement la position REELLE lue dans es_state.inf, sans
                # jamais passer par "default" ni par la fenetre de grace
                # boot_sweep_pending (qui n'a de sens que pour un vrai sweep
                # de menu, pas pour une position de jeu deja connue et
                # stable).
                system_raw=$(extract_field "$_snap" "SystemId")
                game_path="$_boot_game_path"
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
                system=$(normalize_system "$system_raw")
                echo "$(date '+%H:%M:%S') BOOT (ES redemarre EN NAVIGATION) -> sys=$system rom=$rom" >> "$LOG"
                if [ -n "$system" ] && [ -n "$rom" ]; then
                    LAST_SYSTEM="$system"
                    LAST_ROM="$rom"
                    send_mqtt_retain "game" "${system}/${rom}"
                else
                    send_mqtt_retain "default" "1"
                fi
            else
                send_mqtt_retain "default" "1"
                # v21 -- arme la detection du sweep systeme post-boot (voir
                # changelog v21 complet en entete) -- seulement dans cette
                # branche (pas de vraie partie en cours, pas de position de
                # navigation reelle deja connue -- voir v32) : un sweep ES
                # n'a de sens que si on redemarre dans le menu, pas en jeu ni
                # en pleine navigation deja positionnee.
                boot_sweep_pending=1
                boot_sweep_last_publish_ts=0
                boot_sweep_seen_any=0

                # Attendre la fin de la rafale automatique de boot
                sleep 5

                # Lire le vrai système affiché
                system_raw=$(read_state "SystemId")
                system=$(normalize_system "$system_raw")
                echo "$(date '+%H:%M:%S') BOOT settle -> sys=$system" >> "$LOG"
                if [ -n "$system" ]; then
                    LAST_SYSTEM="$system"
                    send_mqtt_retain "system" "$system"
                fi
            fi
            ;;

        gamelistbrowsing|systembrowsing)
            # v37 -- logique RESTAUREE ici (voir changelog v37 en tete de
            # boucle principale) -- inchangee depuis v31, seulement
            # redeplacee hors de poll_navigation_position() (fonction
            # desormais morte, v34 revert).
            now=$(date +%s)

            # v21 -- fenetre de grace boot glissante : pendant
            # boot_sweep_pending, publication limitee comme le mode demo (au
            # plus 1 toutes les BOOT_SWEEP_MIN_PUBLISH_INTERVAL_S) au lieu
            # d'etre bloquee ou totalement libre.
            boot_sweep_suppress=0
            if [ "$boot_sweep_pending" -eq 1 ]; then
                # v22 -- marque qu'un evenement du sweep a bien ete vu.
                boot_sweep_seen_any=1
                if [ $((now - boot_sweep_last_publish_ts)) -lt "$BOOT_SWEEP_MIN_PUBLISH_INTERVAL_S" ]; then
                    boot_sweep_suppress=1
                else
                    boot_sweep_last_publish_ts=$now
                fi
            fi

            # v29 -- position candidate lue ICI, AVANT le detecteur de
            # rafale -- necessaire pour distinguer un VRAI changement de
            # position d'une simple re-annonce ES de la position DEJA
            # ATTEINTE (ES peut republier la MEME rom en boucle).
            _snap=$(read_state_snapshot)
            system_raw=$(extract_field "$_snap" "SystemId")
            system=$(normalize_system "$system_raw")
            game_path=$(extract_field "$_snap" "GamePath")
            _cand_rom=""
            if [ -n "$game_path" ] && [ ! -d "$game_path" ]; then
                _cand_rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
            fi
            _position_unchanged=0
            if [ -n "$game_path" ] && [ ! -d "$game_path" ]; then
                [ "$_cand_rom" = "$LAST_ROM" ] && [ "$system" = "$LAST_SYSTEM" ] && _position_unchanged=1
            elif [ -n "$system" ]; then
                [ "$system" = "$LAST_SYSTEM" ] && [ -z "$LAST_ROM" ] && _position_unchanged=1
            fi

            # v6/v16 -- detecteur de rafale : compte les survols dans la
            # MEME seconde horloge entiere. BURST_THRESHOLD atteint sur
            # BURST_SUSTAIN_SECONDS secondes consecutives -> throttled=1.
            # v29 -- ignore pour un doublon (_position_unchanged=1).
            if [ "$_position_unchanged" -eq 0 ]; then
                if [ "$now" = "$burst_window_start" ]; then
                    burst_count=$((burst_count + 1))
                else
                    # v26 -- ne finalise la streak que si le gap est
                    # contigu (<=1s), sinon reset immediat.
                    if [ "$burst_window_start" -gt 0 ] && [ $((now - burst_window_start)) -le 1 ] && [ "$burst_count" -ge "$BURST_THRESHOLD" ]; then
                        burst_qualifying_streak=$((burst_qualifying_streak + 1))
                    else
                        burst_qualifying_streak=0
                    fi
                    burst_window_start="$now"
                    burst_count=1
                fi
                if [ "$burst_qualifying_streak" -ge "$BURST_SUSTAIN_SECONDS" ] && [ "$throttled" -eq 0 ]; then
                    throttled=1
                    echo "$(date '+%H:%M:%S') BURST start (seuil $BURST_THRESHOLD/s soutenu sur ${BURST_SUSTAIN_SECONDS}s)" >> "$LOG"
                    # v9 -- coupe-circuit anti-rafale, affichage transitoire
                    # (animation locale firmware, RecalBox_DMD.ino v107).
                    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd" -m "CMD=game ARG=!SHUFFLE" 2>/dev/null
                    echo "$(date '+%H:%M:%S') SEND !SHUFFLE (non retenu)" >> "$LOG"
                fi
                last_real_change_ts="$now"
            else
                # v29/v31 -- doublon recu pendant une rafale/un sweep boot
                # en cours ET au moins EARLY_STABLE_SECONDS ecoulees depuis
                # le dernier VRAI changement -- position stabilisee,
                # inutile d'attendre un silence total.
                if [ "$throttled" -eq 1 ] && [ $((now - last_real_change_ts)) -ge "$EARLY_STABLE_SECONDS" ]; then
                    throttled=0
                    burst_count=0
                    burst_qualifying_streak=0
                    echo "$(date '+%H:%M:%S') BURST end (doublon ES ignore par le detecteur, position deja stable ${EARLY_STABLE_SECONDS}s+)" >> "$LOG"
                    publish_settled_position
                fi
                if [ "$boot_sweep_pending" -eq 1 ] && [ "$boot_sweep_seen_any" -eq 1 ] && [ $((now - last_real_change_ts)) -ge "$EARLY_STABLE_SECONDS" ]; then
                    boot_sweep_pending=0
                    echo "$(date '+%H:%M:%S') BOOT SWEEP termine (doublon ES ignore par le detecteur, position deja stable ${EARLY_STABLE_SECONDS}s+)" >> "$LOG"
                    publish_settled_position
                fi
            fi

            echo "$(date '+%H:%M:%S') BROWSE raw=$system_raw norm=$system game=$game_path in_game=$IN_GAME throttled=$throttled bsp=$boot_sweep_pending bss=$boot_sweep_suppress bslp=$boot_sweep_last_publish_ts now=$now bc=$burst_count bqs=$burst_qualifying_streak dup=$_position_unchanged" >> "$LOG"

            if [ "$IN_GAME" -eq 1 ]; then
                echo "$(date '+%H:%M:%S') BROWSE ignored (in game)" >> "$LOG"
                continue
            fi

            if [ -n "$game_path" ]; then
                if [ -d "$game_path" ]; then
                    echo "$(date '+%H:%M:%S') BROWSE subdir -> send system $system" >> "$LOG"
                    if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
                        LAST_SYSTEM="$system"
                        LAST_ROM=""
                        if [ "$throttled" -eq 0 ] && [ "$boot_sweep_suppress" -eq 0 ]; then
                            send_mqtt_retain "system" "$system"
                        fi
                    fi
                else
                    # 2026-08-09 : "s/ //g" -- l'outil PC retire les espaces
                    # en ecrivant les fichiers sur la carte SD DMD.
                    rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
                    if [ -n "$system" ] && [ -n "$rom" ]; then
                        if [ "$rom" != "$LAST_ROM" ] || [ "$system" != "$LAST_SYSTEM" ]; then
                            LAST_SYSTEM="$system"
                            LAST_ROM="$rom"
                            if [ "$throttled" -eq 0 ] && [ "$boot_sweep_suppress" -eq 0 ]; then
                                send_mqtt_retain "game" "${system}/${rom}"
                            fi
                        else
                            echo "$(date '+%H:%M:%S') BROWSE skipped (same game)" >> "$LOG"
                        fi
                    fi
                fi
            elif [ -n "$system" ]; then
                if [ "$system" != "$LAST_SYSTEM" ] || [ -n "$LAST_ROM" ]; then
                    LAST_SYSTEM="$system"
                    LAST_ROM=""
                    if [ "$throttled" -eq 0 ] && [ "$boot_sweep_suppress" -eq 0 ]; then
                        send_mqtt_retain "system" "$system"
                    fi
                else
                    echo "$(date '+%H:%M:%S') BROWSE skipped (same system)" >> "$LOG"
                fi
            else
                echo "$(date '+%H:%M:%S') BROWSE skipped (empty)" >> "$LOG"
            fi
            ;;

        rungame)
            # v6 -- transition definitive (vraie action utilisateur), pas un
            # simple survol -- reinitialise le detecteur de rafale pour ne
            # jamais laisser ce cas etre retarde par un throttle en cours.
            # v21 -- boot_sweep_pending desarme ici aussi, meme logique : un
            # vrai lancement de jeu prouve qu'on n'est plus dans le sweep
            # automatise post-boot, inutile d'attendre un silence.
            burst_count=0
            burst_qualifying_streak=0
            throttled=0
            boot_sweep_pending=0
            last_real_change_ts=$(date +%s)
            IN_GAME=1
            # v8 -- lecture atomique (voir changelog v8).
            _snap=$(read_state_snapshot)
            system_raw=$(extract_field "$_snap" "SystemId")
            game_path=$(extract_field "$_snap" "GamePath")
            # "s/ //g" : voir commentaire du meme fix dans le bloc
            # gamelistbrowsing/systembrowsing plus haut (2026-08-09).
            rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
            system=$(normalize_system "$system_raw")

            echo "$(date '+%H:%M:%S') GAME sys=$system rom=$rom" >> "$LOG"

            if [ -n "$system" ] && [ -n "$rom" ]; then
                LAST_SYSTEM="$system"
                LAST_ROM="$rom"
                send_mqtt_retain "game" "${system}/${rom}"
                send_mqtt_retain "ingame" "1"
            fi
            ;;

        endgame)
            burst_count=0
            burst_qualifying_streak=0
            throttled=0
            boot_sweep_pending=0
            last_real_change_ts=$(date +%s)
            IN_GAME=0
            LAST_ROM=""
            system_raw=$(read_state "SystemId")
            system=$(normalize_system "$system_raw")

            echo "$(date '+%H:%M:%S') ENDGAME sys=$system last=$LAST_SYSTEM" >> "$LOG"

            send_mqtt_retain "ingame" "0"
            if [ -n "$system" ]; then
                LAST_SYSTEM="$system"
                send_mqtt_retain "system" "$system"
            fi
            ;;

        stop)
            burst_count=0
            burst_qualifying_streak=0
            throttled=0
            boot_sweep_pending=0
            last_real_change_ts=$(date +%s)
            echo "$(date '+%H:%M:%S') STOP -> playlist" >> "$LOG"
            IN_GAME=0
            LAST_ROM=""
            send_mqtt_retain "ingame" "0"
            send_mqtt_retain "default" "1"
            sleep 2
            ;;

        sleep)
            echo "$(date '+%H:%M:%S') SLEEP -> playlist" >> "$LOG"
            send_mqtt_retain "default" "1"
            ;;

        wakeup)
            demo_veille_playlist_sent=0 # v41 -- rearme pour la prochaine entree en veille demo/gameclip
            echo "$(date '+%H:%M:%S') WAKEUP -> reaffiche last" >> "$LOG"
            if [ -n "$LAST_ROM" ] && [ -n "$LAST_SYSTEM" ]; then
                echo "$(date '+%H:%M:%S') WAKEUP -> jeu $LAST_SYSTEM/$LAST_ROM" >> "$LOG"
                send_mqtt_retain "game" "${LAST_SYSTEM}/${LAST_ROM}"
            elif [ -n "$LAST_SYSTEM" ]; then
                echo "$(date '+%H:%M:%S') WAKEUP -> systeme $LAST_SYSTEM" >> "$LOG"
                send_mqtt_retain "system" "$LAST_SYSTEM"
            else
                echo "$(date '+%H:%M:%S') WAKEUP -> playlist (rien de connu)" >> "$LOG"
                send_mqtt_retain "default" "1"
            fi
            ;;

        # v18 -- BUG REEL trouve en verifiant en direct (retour utilisateur :
        # "en mode clip & demo afficher le marquee du jeu concerne au lieu
        # de la playlist") : RB a en realite 2 modes de veille "jeu" DISTINCTS
        # (retour utilisateur explicite, 2026-08-23 : "il existe 1 mode
        # demo : lance des jeux et un mode demo video : lance des clips
        # video de jeu"), chacun avec son propre screensaver.type et son
        # propre couple d'evenements ES, TOUS DEUX verifies en direct sur
        # ce materiel :
        #   - screensaver.type=demo   -> "rundemo"/"enddemo" (vrai lancement
        #     du jeu via l'emulateur, GamePath = vraie rom en cours
        #     d'execution).
        #   - screensaver.type=gameclip -> "startgameclip"/"stopgameclip"
        #     (lecture d'un clip .mp4 pre-enregistre, PAS de lancement
        #     emulateur -- GamePath pointe quand meme vers la rom
        #     CONCERNEE par le clip, memes champs es_state.inf exploitables).
        # Les 2 couples sont fusionnes ici (memes patterns partages) : les
        # 2 modes peuplent SystemId/GamePath de la MEME facon, EXACTEMENT
        # comme pendant un survol de liste -- meme lecture atomique (v8) et
        # meme publication que gamelistbrowsing) reutilisees pour les 2.
        # DEMO_SYSTEM/DEMO_ROM dedies (jamais LAST_SYSTEM/LAST_ROM) pour ne
        # pas corrompre la position REELLE de navigation que wakeup) doit
        # restaurer au reveil -- ni un jeu demo ni un clip video ne sont une
        # vraie position utilisateur.
        #
        # ATTENTION cadence tres differente entre les 2 : "demo" peut
        # enchainer les jeux a ~1/s de facon SOUTENUE (voir BUG REEL #2,
        # changelog v18 complet plus haut) alors que "gameclip" tourne a un
        # rythme stable ~30s/clip (verifie en direct, jamais de rafale
        # observee) -- la meme limite de frequence (DEMO_MIN_PUBLISH_
        # INTERVAL_S=3s) protege les 2 sans jamais gener gameclip (30s >> 3s).
        rundemo|startgameclip)
            # v41 -- retour utilisateur explicite (02/09 tard, priorite
            # stabilite > fonctionnalite cosmetique -- "la fonction veille
            # ciblee n'est que cosmetique et ne pese rien face au besoin de
            # stabilite") : la veille CIBLEE (marquee + panneaux hiscore/
            # description/info pendant demo/gameclip, tout le mecanisme
            # ci-dessous) est DESACTIVEE au profit de la PLAYLIST simple,
            # exactement comme dim/black/bouncing (voir sleep) plus haut) --
            # meme demarche que le fix v40 (fusion des 12 topics MQTT) :
            # reduire l'EXPOSITION au blocage TX MQTT post-CONNACK plutot
            # que de continuer a le corriger a la source (mur de plateforme
            # atteint, voir DECISIONS.md/memoire projet) -- la veille ciblee
            # generait un flux MQTT continu et soutenu (round-robin toutes
            # les ~14s + jusqu'a ~1/s en rafale de changement de jeu demo,
            # deja documente comme cas extreme, voir BUG REEL #2 plus haut)
            # pour un benefice purement cosmetique. return anticipe :
            # publie "default" (playlist) UNE SEULE FOIS par entree en
            # veille (garde demo_veille_playlist_sent, remise a 0 seulement
            # au reveil) -- tout le mecanisme de tracking SystemId/GamePath/
            # DEMO_SYSTEM/DEMO_ROM plus bas reste en place mais N'EST PLUS
            # ATTEINT, conserve tel quel au cas ou ce choix serait revu.
            if [ "$demo_veille_playlist_sent" != "1" ]; then
                echo "$(date '+%H:%M:%S') DEMO/CLIP -> playlist (veille ciblee desactivee, v41)" >> "$LOG"
                send_mqtt_retain "default" "1"
                demo_veille_playlist_sent=1
            fi
            continue
            now=$(date +%s)
            # v34 -- horodatage du dernier evenement vu ICI (pas seulement
            # au moment d'une publication effective) -- voir changelog v34
            # complet pres de poll_navigation_position() : remplace le
            # timeout de lecture "-t 1" qui pilotait auparavant
            # demo_throttled, desormais verifie a chaque tick de boucle sur
            # le temps ecoule depuis CET horodatage.
            demo_last_event_ts="$now"
            _snap=$(read_state_snapshot)
            system_raw=$(extract_field "$_snap" "SystemId")
            game_path=$(extract_field "$_snap" "GamePath")
            rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
            system=$(normalize_system "$system_raw")
            if [ -n "$system" ] && [ -n "$rom" ]; then
                if [ "$rom" != "$DEMO_ROM" ] || [ "$system" != "$DEMO_SYSTEM" ]; then
                    DEMO_SYSTEM="$system"
                    DEMO_ROM="$rom"
                    if [ $((now - demo_last_publish_ts)) -ge "$DEMO_MIN_PUBLISH_INTERVAL_S" ]; then
                        demo_throttled=0
                        demo_last_publish_ts="$now"
                        echo "$(date '+%H:%M:%S') DEMO/CLIP -> ${system}/${rom}" >> "$LOG"
                        send_mqtt_retain "game" "${system}/${rom}"
                    else
                        demo_throttled=1
                    fi
                fi
            fi
            ;;

        enddemo|stopgameclip)
            ;;

        *)
            # Event inconnu ou vide
            ;;
    esac
done
