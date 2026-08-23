#!/bin/ash
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v18
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

echo "$(date '+%H:%M:%S.%N') TRACE start pid=$$ ppid=$PPID arg0=$0" >> /tmp/marquee_trace.log

# v12 -- verrou anti-relance rendu ATOMIQUE (mkdir au lieu d'un fichier PID
# check-then-write) -- BUG REEL reconfirme sur materiel (2026-08-20) : 4
# instances simultanees de dmd_achievement.sh (meme verrou fichier PID que
# celui-ci) trouvees vivantes en meme temps malgre le "fix" v5/v1.1 --
# EmulationStation peut lancer plusieurs invocations dans la MEME seconde
# (ex. rafale d'evenements au boot), et la sequence "verifier si le fichier
# existe" PUIS "ecrire son propre PID" n'est PAS une operation atomique :
# plusieurs instances peuvent toutes lire "pas de verrou" avant qu'aucune
# n'ait eu le temps d'ecrire le sien. mkdir EST atomique sur ce systeme de
# fichiers (tmpfs) -- un seul appel concurrent peut reussir, garanti par le
# noyau, fermant la fenetre de course entierement (contrairement au fichier
# PID simple). Verrou perime (proprietaire mort) detecte et reclame via
# rmdir+nouveau mkdir -- si la reclamation perd elle-meme la course contre
# une autre instance, sortie propre (comportement identique a avant).
LOCKDIR="/tmp/marquee_singleton.lock"
if ! mkdir "$LOCKDIR" 2>/dev/null; then
    oldpid=$(cat "$LOCKDIR/pid" 2>/dev/null)
    if [ -n "$oldpid" ] && kill -0 "$oldpid" 2>/dev/null; then
        echo "$(date '+%H:%M:%S.%N') TRACE exit pid=$$ (oldpid=$oldpid alive)" >> /tmp/marquee_trace.log
        exit 0
    fi
    rmdir "$LOCKDIR" 2>/dev/null
    if ! mkdir "$LOCKDIR" 2>/dev/null; then
        echo "$(date '+%H:%M:%S.%N') TRACE exit pid=$$ (course perdue a la reclamation)" >> /tmp/marquee_trace.log
        exit 0
    fi
fi
echo $$ > "$LOCKDIR/pid"
echo "$(date '+%H:%M:%S.%N') TRACE proceeding pid=$$" >> /tmp/marquee_trace.log

LOG="/recalbox/share/system/logs/marquee_mqtt.log"

read_state() {
    grep "^${1}=" "/tmp/es_state.inf" 2>/dev/null | cut -d= -f2- | tr -d '\r\n '
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
    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -r -t "marquee/cmd/${1}" -m "$2" 2>/dev/null
    echo "$(date '+%H:%M:%S') SEND(R) marquee/cmd/${1} = $2" >> "$LOG"
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

send_mqtt_retain "default" "1"

LAST_SYSTEM=""
LAST_ROM=""
IN_GAME=0
BOOT_TIME=0
PREV_EVENT=""
# v18 -- dedies au mode demo (rundemo/enddemo, voir leur case) : jamais
# LAST_SYSTEM/LAST_ROM directement, pour ne pas corrompre la position REELLE
# de navigation que wakeup) doit restaurer au reveil.
DEMO_SYSTEM=""
DEMO_ROM=""
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

# v13 -- echo de demarrage deplace ICI (apres l'assignation de
# BURST_THRESHOLD) : place plus haut dans le fichier (juste avant
# send_mqtt_retain "default"), la variable n'existait pas encore au moment
# de l'interpolation et le log affichait "seuil=/s" (vide) au lieu de
# "seuil=10/s" -- bug constate au demarrage reel, corrige en deplacant le
# log apres la declaration.
echo "$(date) - Marquee bridge started (v18, rundemo/enddemo -> marquee du jeu demo, coupe-circuit anti-rafale seuil=$BURST_THRESHOLD/s sur ${BURST_SUSTAIN_SECONDS}s consecutives, lock atomique acquis)" >> "$LOG"

while true; do
    PREV_EVENT="$event"
    # v10 -- "-W 1" reserve au cas throttled=1 (voir changelog v10) : hors
    # rafale, blocage pur (pas de timeout) -- zero connexion locale tant
    # qu'aucun vrai evenement ES n'arrive, comme avant v6. Pendant une
    # rafale, "-W 1" reste necessaire pour detecter sa fin (aucune iteration
    # ne se produirait sinon tant qu'aucun evenement n'arrive). $event vide
    # en sortie de boucle ne peut donc survenir QUE si throttled=1 (timeout)
    # ou (tres improbable) message vide recu -- traite pareil, sans
    # consequence (le case *) plus bas ignore deja les events vides).
    if [ "$throttled" -eq 1 ] || [ "$demo_throttled" -eq 1 ]; then
        event=$(mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 \
            -t "Recalbox/EmulationStation/Event" -C 1 -W 1 2>/dev/null | tr -d '\r')
    else
        event=$(mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 \
            -t "Recalbox/EmulationStation/Event" -C 1 2>/dev/null | tr -d '\r')
    fi

    if [ -z "$event" ]; then
        # v6 -- timeout : si une rafale etait en cours, elle vient de
        # s'arreter (aucun survol depuis >=1s) -- publier la position
        # courante MAINTENANT (pas d'attente supplementaire) et repasser en
        # mode reactif normal.
        if [ "$throttled" -eq 1 ]; then
            throttled=0
            burst_count=0
            burst_qualifying_streak=0
            echo "$(date '+%H:%M:%S') BURST end -- publication position stabilisee" >> "$LOG"
            publish_settled_position
        fi
        # v18 suite -- meme principe, limite de frequence demo (voir
        # demo_throttled/DEMO_MIN_PUBLISH_INTERVAL_S) : plus aucun rundemo
        # depuis >=1s (timeout -W1) -- la sequence rapide vient de finir
        # (ou le mode demo lui-meme s'est arrete), publie la DERNIERE
        # position demo connue, meme si elle est plus recente que la
        # derniere publication autorisee.
        if [ "$demo_throttled" -eq 1 ]; then
            demo_throttled=0
            if [ -n "$DEMO_SYSTEM" ] && [ -n "$DEMO_ROM" ]; then
                demo_last_publish_ts=$(date +%s)
                echo "$(date '+%H:%M:%S') DEMO sequence rapide terminee -- publication position stabilisee" >> "$LOG"
                send_mqtt_retain "game" "${DEMO_SYSTEM}/${DEMO_ROM}"
            fi
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

            # v17 -- voir changelog d'entete : ne force plus "default" sans
            # verifier d'abord si une vraie partie est en cours (Action=
            # dans es_state.inf, ecrit par ES lui-meme) -- lecture atomique
            # (meme motif que v8, voir read_state_snapshot()) pour eviter
            # toute race entre Action/SystemId/GamePath.
            _snap=$(read_state_snapshot)
            _action=$(extract_field "$_snap" "Action")
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
            else
                send_mqtt_retain "default" "1"

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
            # Ignorer la rafale pendant les 10s après le boot
            now=$(date +%s)
            if [ "$BOOT_TIME" -gt 0 ] && [ $((now - BOOT_TIME)) -lt 10 ]; then
                echo "$(date '+%H:%M:%S') BROWSE ignored (boot settle)" >> "$LOG"
                continue
            fi

            # v6 -- detecteur de rafale : compte les survols dans la MEME
            # seconde horloge entiere (precision volontairement grossiere,
            # voir changelog v6). BURST_THRESHOLD atteint -> bascule
            # throttled=1 (les publications s'arretent, voir plus bas).
            # v16 -- le seuil instantane seul ne suffit plus (voir
            # changelog v16) : au moment ou le bucket seconde TOURNE (donc
            # que le tally de la seconde qui vient de se terminer est
            # definitif), on evalue si CETTE seconde ecoulee a atteint le
            # seuil -- si oui, la "serie" de secondes consecutives
            # qualifiees s'allonge, sinon elle est remise a zero. !SHUFFLE
            # ne se declenche que quand cette serie atteint
            # BURST_SUSTAIN_SECONDS (debit soutenu), pas des la 1ere
            # seconde isolee au-dessus du seuil.
            if [ "$now" = "$burst_window_start" ]; then
                burst_count=$((burst_count + 1))
            else
                if [ "$burst_window_start" -gt 0 ]; then
                    if [ "$burst_count" -ge "$BURST_THRESHOLD" ]; then
                        burst_qualifying_streak=$((burst_qualifying_streak + 1))
                    else
                        burst_qualifying_streak=0
                    fi
                fi
                burst_window_start="$now"
                burst_count=1
            fi
            if [ "$burst_qualifying_streak" -ge "$BURST_SUSTAIN_SECONDS" ] && [ "$throttled" -eq 0 ]; then
                throttled=1
                echo "$(date '+%H:%M:%S') BURST start (seuil $BURST_THRESHOLD/s soutenu sur ${BURST_SUSTAIN_SECONDS}s)" >> "$LOG"
                # v9 -- coupe-circuit anti-rafale, affichage transitoire.
                # UNE SEULE publication (pas une par frame -- l'animation
                # boucle localement cote firmware, voir memoire projet et
                # RecalBox_DMD.ino v107) sur ce MEME topic marquee/cmd/game
                # deja utilise -- aucun nouveau topic, aucune nouvelle
                # souscription. mosquitto_pub direct (pas send_mqtt_retain)
                # : ce signal n'a pas vocation a etre retenu -- un abonne
                # qui se connecterait pendant une rafale ne doit jamais
                # recevoir "!SHUFFLE" comme dernier etat connu.
                mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd/game" -m "!SHUFFLE" 2>/dev/null
                echo "$(date '+%H:%M:%S') SEND !SHUFFLE (non retenu)" >> "$LOG"
            fi

            # v8 -- lecture atomique (voir changelog v8) : SystemId et
            # GamePath extraits du MEME instantane, plus de risque
            # d'incoherence entre les deux.
            _snap=$(read_state_snapshot)
            system_raw=$(extract_field "$_snap" "SystemId")
            system=$(normalize_system "$system_raw")
            game_path=$(extract_field "$_snap" "GamePath")

            echo "$(date '+%H:%M:%S') BROWSE raw=$system_raw norm=$system game=$game_path in_game=$IN_GAME throttled=$throttled" >> "$LOG"

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
                        # v6 -- pendant une rafale (throttled=1), on met a
                        # jour LAST_SYSTEM/LAST_ROM silencieusement SANS
                        # publier -- la publication effective se fera au
                        # "BURST end" (timeout) plus haut, avec la position
                        # la plus recente.
                        if [ "$throttled" -eq 0 ]; then
                            send_mqtt_retain "system" "$system"
                        fi
                    fi
                else
                    # 2026-08-09 : "s/ //g" ajoute -- l'outil PC (sanitize_filename(),
                    # RecalBoxDMD_tool.py) retire tous les espaces du nom de ROM en
                    # ecrivant les fichiers sur la carte SD DMD (ex: "Zynaps (Europe).zip"
                    # -> "Zynaps(Europe).raw565pack"), mais ce script envoyait le nom AVEC
                    # ses espaces d'origine -- flag "?" (fallback) sur le DMD pour tout jeu
                    # dont le nom de ROM contient un espace, meme si le fichier converti
                    # existe bel et bien sur la carte SD, juste sous un nom legerement
                    # different. Meme fix applique au bloc rungame plus bas.
                    rom=$(basename "$game_path" | sed 's/\.[^.]*$//; s/ //g')
                    if [ -n "$system" ] && [ -n "$rom" ]; then
                        if [ "$rom" != "$LAST_ROM" ] || [ "$system" != "$LAST_SYSTEM" ]; then
                            LAST_SYSTEM="$system"
                            LAST_ROM="$rom"
                            if [ "$throttled" -eq 0 ]; then
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
                    if [ "$throttled" -eq 0 ]; then
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
            burst_count=0
            burst_qualifying_streak=0
            throttled=0
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
        # de la playlist") : sur cette version d'ES, les evenements REELS de
        # ce mode sont "rundemo"/"enddemo" -- PAS "startgameclip"/
        # "stopgameclip" comme suppose depuis l'origine de ce script. Verifie
        # en conditions reelles (screensaver.type=demo force, capture
        # complete du flux Recalbox/EmulationStation/Event) : ces 2 anciens
        # noms ne se sont JAMAIS declenches sur ce materiel -- code mort
        # depuis le debut, tombant systematiquement dans le *) ci-dessous
        # (ignore). Gardes ci-dessous par prudence (cout nul) au cas ou une
        # autre version d'ES les utiliserait encore.
        #
        # /tmp/es_state.inf peuple SystemId/GamePath PENDANT rundemo,
        # EXACTEMENT comme pendant un survol de liste (verifie en direct :
        # Action=rundemo, SystemId=fbneo, GamePath=.../dacholer.zip) --
        # meme lecture atomique (v8) et meme publication que
        # gamelistbrowsing) reutilisees ici. DEMO_SYSTEM/DEMO_ROM dedies
        # (jamais LAST_SYSTEM/LAST_ROM) pour ne pas corrompre la position
        # REELLE de navigation que wakeup) doit restaurer au reveil -- un
        # jeu demo n'est PAS une vraie position utilisateur.
        rundemo)
            # v18 suite -- limite de frequence DEDIEE au mode demo (voir
            # DEMO_MIN_PUBLISH_INTERVAL_S/demo_throttled plus haut) : jamais
            # LAST_SYSTEM/LAST_ROM/throttled/burst_* (ceux-la pilotent
            # publish_settled_position(), qui publie la position REELLE de
            # navigation, jamais celle d'une demo).
            now=$(date +%s)
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
                        echo "$(date '+%H:%M:%S') DEMO -> ${system}/${rom}" >> "$LOG"
                        send_mqtt_retain "game" "${system}/${rom}"
                    else
                        demo_throttled=1
                    fi
                fi
            fi
            ;;

        enddemo)
            ;;

        # Mode demo/veille EmulationStation -- anciens noms d'evenement
        # (voir commentaire rundemo) ci-dessus), JAMAIS observes declenches
        # sur ce materiel mais gardes par prudence.
        startgameclip)
            if [ "$PREV_EVENT" != "startgameclip" ]; then
                echo "$(date '+%H:%M:%S') DEMO/VEILLE (legacy) -> playlist" >> "$LOG"
                send_mqtt_retain "default" "1"
            fi
            ;;

        stopgameclip)
            ;;

        *)
            # Event inconnu ou vide
            ;;
    esac
done
