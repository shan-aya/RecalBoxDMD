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
    oldpid=$(cat "$LOCKDIR/pid" 2>/dev/null)
    if [ -n "$oldpid" ] && kill -0 "$oldpid" 2>/dev/null; then
        exit 0
    fi
    rm -rf "$LOCKDIR" 2>/dev/null
    if ! mkdir "$LOCKDIR" 2>/dev/null; then
        exit 0
    fi
fi
echo $$ > "$LOCKDIR/pid"
