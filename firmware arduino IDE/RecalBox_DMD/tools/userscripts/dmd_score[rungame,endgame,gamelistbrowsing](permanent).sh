#!/bin/ash
# Pont MQTT hiscore FBNeo + infos/description jeu -> DMD (marquee/cmd/score,
# canal UNIQUE, architecture "DMD bete" v110 -- voir RecalBox_DMD.ino)
#
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v16
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

LOCKDIR="/tmp/dmd_score_singleton.lock"
if ! mkdir "$LOCKDIR" 2>/dev/null; then
    oldpid=$(cat "$LOCKDIR/pid" 2>/dev/null)
    if [ -n "$oldpid" ] && kill -0 "$oldpid" 2>/dev/null; then
        exit 0
    fi
    rmdir "$LOCKDIR" 2>/dev/null
    if ! mkdir "$LOCKDIR" 2>/dev/null; then
        exit 0
    fi
fi
echo $$ > "$LOCKDIR/pid"

LOG="/recalbox/share/system/logs/dmd_score.log"
SCRIPT_DIR=$(dirname "$0")
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
                state_still_valid "$sf" "$exp" || return 1
            fi
            send_score "${title}|${page}" "$dur_ms"
            first_page=0; page_count=$((page_count + 1))
            page=""; n=0
        fi
    done
    if [ -n "$page" ] && [ "$page_count" -lt "$MAX_PAGES" ]; then
        if [ "$first_page" -eq 0 ]; then
            sleep "$dur_s"
            state_still_valid "$sf" "$exp" || return 1
        fi
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
            state_still_valid "$sf" "$exp" || return 1
        fi
        send_score "${title}|${pg}" "$dur_ms"
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

    page1="HI-SCORE"
    [ -n "$r1" ] && page1="${page1}|${r1}"
    [ -n "$r2" ] && page1="${page1}|${r2}"
    send_score "$page1" "$HISCORE_INFO_PAGE_DURATION_MS"

    page2=""
    for r in "$r3" "$r4" "$r5"; do
        [ -z "$r" ] && continue
        if [ -z "$page2" ]; then page2="$r"; else page2="${page2}|${r}"; fi
    done
    if [ -n "$page2" ]; then
        sleep "$HISCORE_INFO_PAGE_DURATION_S"
        state_still_valid "$sf" "$exp" || return 1
        send_score "HI-SCORE|${page2}" "$HISCORE_INFO_PAGE_DURATION_MS"
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
    while true; do
        ratio=$(feat_value "$ratio_key")
        [ "$ratio" -le 0 ] && return
        sleep $((ratio * SLIDESHOW_GAP_S))
        current=$(cat "$state_file" 2>/dev/null)
        [ "$current" = "$expected" ] || return
        types=$(enabled_panel_types "$ctx")
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
        echo "$(date '+%H:%M:%S') ROUNDROBIN ctx=$ctx type=$chosen ratio=$ratio pos=${pos}/${count}" >> "$LOG"
        publish_one_panel "$sys" "$gpath" "$rom" "$chosen" "$state_file" "$expected"
        current=$(cat "$state_file" 2>/dev/null)
        [ "$current" = "$expected" ] || return
    done
}

# v2 -- sous-processus DEDIE (marquee/status/features RETENU cote DMD --
# 1ere lecture immediate a la souscription, meme si ce script demarre
# apres le DMD).
features_watcher() {
    mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/status/features" 2>/dev/null | \
    while IFS= read -r line; do
        printf '%s\n' "$line" > "$FEATURES_FILE"
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
    payload="$1"; dur="$2"
    [ -z "$dur" ] && dur=6000
    fw_dur=$((dur + SCORE_TIMER_MARGIN_MS))
    mosquitto_pub -h 127.0.0.1 -p 1883 -q 0 -t "marquee/cmd/score" -m "@${fw_dur}|${payload}" 2>/dev/null
    echo "$(date '+%H:%M:%S') SEND marquee/cmd/score = @${fw_dur}|${payload}" >> "$LOG"
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
    rom="$1"
    hifile="${HI_DIR}/${rom}.hi"
    [ -f "$hifile" ] || { echo "$(date '+%H:%M:%S') SCORE skip $rom (pas de .hi)" >> "$LOG"; return; }
    size=$(wc -c < "$hifile" 2>/dev/null)
    glabel=""

    case "$rom" in
        galaga|galaga84|galagab2|galagads|galagamf|galagamk|galagamw|galagao|gallag)
            if [ "$size" != "51" ]; then
                echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 51)" >> "$LOG"
                return
            fi
            score=$(decode_galaga_topscore "$hifile")
            glabel="GALAGA"
            echo "HI-SCORE ${score}"
            ;;
        gyruss)
            if [ "$size" != "43" ]; then
                echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 43)" >> "$LOG"
                return
            fi
            score=$(decode_gyruss_topscore "$hifile")
            glabel="GYRUSS"
            echo "HI-SCORE ${score}"
            ;;
        1941)
            if [ "$size" != "124" ]; then
                echo "$(date '+%H:%M:%S') SCORE skip $rom (taille $size != 124)" >> "$LOG"
                return
            fi
            topn=$(decode_1941_topN "$hifile" 5)
            glabel="1941"
            score="${topn}"
            echo "HI-SCORE|${topn}"
            ;;
        *)
            echo "$(date '+%H:%M:%S') SCORE skip $rom (jeu non supporte)" >> "$LOG"
            return
            ;;
    esac
    echo "$(date '+%H:%M:%S') SCORE $rom (${glabel}) -> ${score}" >> "$LOG" 1>&2
}

# v6 -- point d'entree UNIQUE pour publier le hi-score d'un rom, utilise a
# la fois par round_robin() (type "hiscore") et par le handler endgame
# (refresh ponctuel en fin de partie) -- garantit le MEME rendu (rang 1 en
# gros, pagination 1941) dans les 2 cas.
# v13 -- $2=state_file/$3=expected optionnels, transmis a
# send_hiscore_paginated() -- endgame les omet (comportement inchange, un
# appel ponctuel hors round-robin n'a pas besoin d'etre interrompu).
publish_hiscore() {
    rom="$1"; sf="$2"; exp="$3"
    payload=$(build_score_payload "$rom")
    [ -n "$payload" ] || return 1
    title="${payload%%|*}"
    rest="${payload#*|}"
    if [ "$rest" = "$payload" ]; then
        # Pas de "|" du tout (jeu a valeur unique, galaga/gyruss).
        send_score "$payload" "$HISCORE_INFO_PAGE_DURATION_MS"
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
    case "$type" in
        hiscore)
            [ "$sys" = "fbneo" ] && [ -n "$rom" ] || return 1
            publish_hiscore "$rom" "$sf" "$exp"
            return $?
            ;;
        description)
            [ -n "$sys" ] && [ -n "$gpath" ] || return 1
            raw=$(python3 "${SCRIPT_DIR}/dmd_game_info.py" "$sys" "$gpath" 2>>"$LOG")
            [ -n "$raw" ] || return 1
            field=$(extract_field "$raw" "DESCRIPTION")
            [ -n "$field" ] || return 1
            title="${field%%|*}"; content="${field#*|}"
            send_paginated "$title" "$content" "$DESC_PAGE_DURATION_MS" "$DESC_PAGE_DURATION_S" "$sf" "$exp"
            return 0
            ;;
        info)
            [ -n "$sys" ] && [ -n "$gpath" ] || return 1
            raw=$(python3 "${SCRIPT_DIR}/dmd_game_info.py" "$sys" "$gpath" 2>>"$LOG")
            [ -n "$raw" ] || return 1
            field=$(extract_field "$raw" "INFOS")
            [ -n "$field" ] || return 1
            title="${field%%|*}"; content="${field#*|}"
            send_paginated_lines "$title" "$content" "$HISCORE_INFO_PAGE_DURATION_MS" "$HISCORE_INFO_PAGE_DURATION_S" "$sf" "$exp"
            return 0
            ;;
    esac
    return 1
}

echo "$(date) - DMD score bridge started (v16, pagination DESCRIPTION par phrases entieres + TomThumb + round-robin infini + interruption inter-pages + titre hi-score page2 + marge anti-flash + dwell/ratios reglables)" >> "$LOG"
# Efface une session/etat perime d'un lancement precedent.
: > "$GAME_SESSION_FILE"
: > "$BROWSE_STATE_FILE"

LAST_BROWSE_SYS=""
LAST_BROWSE_ROM=""

# Connexion MQTT PERSISTANTE (une seule souscription, lue en continu).
mosquitto_sub -h 127.0.0.1 -p 1883 -q 0 -t "Recalbox/EmulationStation/Event" 2>/dev/null | \
while IFS= read -r event; do
    event=$(printf '%s' "$event" | tr -d '\r')

    case "$event" in
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
            if [ "$system" = "fbneo" ] && feat_enabled "hiscore_ingame"; then
                game_path=$(read_state "GamePath")
                rom=$(basename "$game_path" | sed 's/\.[^.]*$//')
                echo "$(date '+%H:%M:%S') ENDGAME fbneo rom=$rom" >> "$LOG"
                [ -n "$rom" ] && publish_hiscore "$rom"
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
        gamelistbrowsing)
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
                state="${system}|${rom}"
                printf '%s\n' "$state" > "$BROWSE_STATE_FILE"
                if [ "$system" != "$LAST_BROWSE_SYS" ] || [ "$rom" != "$LAST_BROWSE_ROM" ]; then
                    LAST_BROWSE_SYS="$system"
                    LAST_BROWSE_ROM="$rom"
                    dwell=$(feat_value "dwell_seconds")
                    if [ "$dwell" -lt "$DWELL_MIN_SECONDS" ]; then dwell="$DWELL_MIN_SECONDS"; fi
                    echo "$(date '+%H:%M:%S') BROWSE sys=$system rom=$rom (dwell ${dwell}s)" >> "$LOG"
                    (
                        sleep "$dwell"
                        current=$(cat "$BROWSE_STATE_FILE" 2>/dev/null)
                        if [ "$current" = "$state" ]; then
                            echo "$(date '+%H:%M:%S') DWELL settled sys=$system rom=$rom -- demarrage round-robin browse" >> "$LOG"
                            round_robin "browse" "$system" "$game_path" "$rom" "$BROWSE_STATE_FILE" "$state" "repeat_browse_cycles"
                        else
                            echo "$(date '+%H:%M:%S') DWELL abandoned sys=$system rom=$rom (deplace entre-temps)" >> "$LOG"
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
