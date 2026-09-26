# v1 - 2026-09-04 - safe-modify - Verrou anti-relance partage, extrait de la
# duplication a l'identique dans marquee.sh (v27)/dmd_score.sh (v35)/
# dmd_achievement.sh (v3) -- seule la valeur de LOCKDIR differait entre les
# 3 copies. Nettoyage differe explicitement lors de la revue pre-merge
# master du 2026-09-03 (voir DECISIONS.md, "Explicitement PAS fait") --
# repris le 2026-09-04.
#
# v2 - 2026-09-05 - safe-modify - BUG REEL trouve en deployant dmd_score.sh
#   v44 sur RB1/RB2 : `rmdir "$LOCKDIR"` echoue SILENCIEUSEMENT (2>/dev/null)
#   des que LOCKDIR contient encore le fichier "pid" -- `rmdir` exige un
#   dossier VIDE, il ne l'est jamais a ce stade (le fichier pid n'a jamais
#   ete supprime avant cet appel). Consequence observee : un ancien
#   processus tue de l'exterieur (kill -9 manuel pendant un deploiement,
#   mais aussi tout crash/OOM en usage normal) laisse un LOCKDIR non-vide
#   -- la PROCHAINE tentative de lancement (relance ES normale, ou notre
#   propre sequence de redeploiement) echoue alors a CHAQUE fois via ce
#   meme chemin (mkdir echoue -- oldpid mort donc pas d'exit precoce --
#   rmdir echoue silencieusement car non-vide -- mkdir echoue encore --
#   exit 0), le script ne demarre plus JAMAIS tant que personne ne
#   supprime le dossier a la main (`rm -rf`) -- un DEADLOCK PERMANENT,
#   pas juste un tour rate. Bug PREEXISTANT a l'extraction v1 (present a
#   l'identique dans le code duplique d'origine des 3 scripts depuis le
#   debut, jamais remarque avant faute d'avoir declenche ce cas precis).
#   Fix : `rm -rf` au lieu de `rmdir` -- supprime le dossier ET son
#   contenu en un seul appel, quel que soit son etat.
#
# v3 - 2026-09-12 - safe-modify - BUG REEL trouve en direct sur RB1 (chasse
#   au bug UDP) : marquee.sh ET dmd_score.sh retrouves morts (aucun process
#   actif), DMD fige sur le dernier marquee affiche depuis plusieurs
#   minutes -- toute tentative de relance (manuelle SSH, et donc aussi une
#   relance normale par ES) echouait silencieusement via ce fichier. Cause :
#   `kill -0 "$oldpid"` reussit aussi pour un ZOMBIE (process termine mais
#   jamais "reap" par son parent, `ps` le montre `<defunct>`, confirme sur
#   les 2 PID bloques ici) -- le verrou le traite comme "toujours vivant" et
#   bloque indefiniment tout nouveau demarrage, alors que le processus ne
#   fait plus RIEN depuis son exit. Meme famille que le DEADLOCK PERMANENT
#   deja documente en v2 (`rmdir` sur dossier non vide) mais un chemin
#   different : ici mkdir echoue, kill -0 "reussit" a tort, on ne descend
#   JAMAIS jusqu'au rm -rf/retry qui aurait resolu le cas. Fix : verifie
#   AUSSI l'etat du process via /proc/$oldpid/stat (champ etat juste apres
#   le dernier ')' du "comm" -- gere les noms de commande contenant des
#   espaces/parentheses) -- un etat "Z" (zombie) n'est plus traite comme un
#   process actif, meme si kill -0 reussit encore techniquement.
#
# v4 - 2026-09-26 - safe-modify - Course reelle trouvee en relisant le code
#   (piste ouverte par la comparaison avec ArcadeMatrix, verrou par socket) :
#   entre le `mkdir` reussi d'une instance A et son `echo $$ > pid`, une
#   instance B concurrente (rafale ES : le script est relance a CHAQUE
#   evenement) echoue sur mkdir, lit un pid VIDE, prend le verrou pour
#   orphelin, le supprime (rm -rf) et le reprend -> 2 demons en parallele.
#   La fenetre est de quelques instructions, mais s'elargit sous charge CPU,
#   precisement pendant les rafales. Fix : un pid vide = acquisition en
#   cours par une autre instance -> on sort. Filet anti-deadlock (lecon v2) :
#   un verrou reste VIDE plus d'1 minute (crash entre mkdir et echo) est
#   repris. En plus : /proc/$oldpid/stat lu par builtins (read + ${...##})
#   au lieu de `sed | cut` -- plus aucun sous-processus sur le chemin de
#   sortie d'une relance dupliquee (objectif deja vise par marquee.sh v27).
#   Existence de /proc/$oldpid/stat = process vivant (remplace kill -0,
#   meme resultat, l'etat Z restant exclu comme en v3). Pourquoi pas un
#   verrou par socket comme ArcadeMatrix : il faudrait lancer Python a chaque
#   relance (cout bien superieur au mkdir) ; et flock sur un descripteur
#   serait herite par les sous-processus du demon (mosquitto_sub...), qui
#   garderaient le verrou apres sa mort.
#
# A SOURCER (jamais executer directement), tout en haut du script appelant,
# AVANT tout le reste -- $1 = nom du verrou (ex. "marquee" -> LOCKDIR
# derive en /tmp/marquee_singleton.lock, compatible a l'identique avec les
# noms deja utilises par les 3 scripts). Etant SOURCE (". fichier", pas
# execute), un `exit` ici termine bien le SCRIPT APPELANT dans le meme
# processus shell -- pas un sous-shell perdu.
#
# mkdir EST atomique sur ce systeme de fichiers (tmpfs) -- fermant la
# fenetre de course entierement, contrairement a un fichier PID
# check-then-write (BUG REEL reconfirme sur materiel avec l'ancienne
# methode : 4 instances simultanees survivaient malgre le verrou, voir
# dmd_achievement.sh v3 pour le detail de cet episode).
#
# Chemin d'appel attendu (voir marquee.sh/dmd_score.sh/dmd_achievement.sh) :
#   . /recalbox/share/userscripts/dmd_helpers/singleton_lock.sh <nom> 2>/dev/null || exit 1
# Le "|| exit 1" est une garde FAIL-CLOSED delibere : si ce fichier est
# absent (ex. deploiement incomplet sans dmd_helpers/, deja arrive une fois
# sur ce projet), le script appelant s'arrete plutot que de tourner SANS
# protection anti-relance -- silencieusement laisser s'accumuler des
# instances est le risque exact que ce verrou existe pour eliminer.
LOCKDIR="/tmp/${1}_singleton.lock"
if ! mkdir "$LOCKDIR" 2>/dev/null; then
    oldpid=""
    # Test -r AVANT chaque read : sous dash (et potentiellement ash), une
    # redirection vers un fichier absent TERMINE le shell au lieu de
    # renvoyer une erreur (verifie au banc de test v4).
    [ -r "$LOCKDIR/pid" ] && read -r oldpid < "$LOCKDIR/pid"
    if [ -z "$oldpid" ]; then
        # v4 -- pid pas encore ecrit : une autre instance est en train
        # d'acquerir le verrou, ne pas le lui voler. Repris seulement s'il
        # est reste vide plus d'1 minute (voir changelog v4).
        if [ -n "$(find "$LOCKDIR" -maxdepth 0 -mmin -1 2>/dev/null)" ]; then
            exit 0
        fi
    else
        # v3/v4 -- vivant = /proc/<pid>/stat lisible ET etat different de
        # "Z" (zombie). Etat = 1er champ apres le dernier ") " du stat (le
        # nom de commande peut contenir espaces et parentheses).
        oldstat=""
        [ -r "/proc/$oldpid/stat" ] && read -r oldstat < "/proc/$oldpid/stat"
        oldstate=${oldstat##*") "}
        oldstate=${oldstate%% *}
        if [ -n "$oldstat" ] && [ "$oldstate" != "Z" ]; then
            exit 0
        fi
    fi
    rm -rf "$LOCKDIR" 2>/dev/null
    if ! mkdir "$LOCKDIR" 2>/dev/null; then
        exit 0
    fi
fi
echo $$ > "$LOCKDIR/pid"
