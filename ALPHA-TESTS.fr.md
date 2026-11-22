# 🧪 Plan de test alpha

Signalez chaque résultat avec **`/alphatest`** sur Discord (voir le [README](README.fr.md)). ⚠️ = précaution.

## T01 · Ordre des couleurs de la dalle (ESP32)  —  *ESP32*

**Étapes**

1. Page web du DMD → **Réglages** → *Ordre des couleurs*.
2. Choisir un autre ordre (ex. GRB) → *Appliquer et redémarrer*.
3. Regarder le titre au démarrage / un logo : les couleurs suivent-elles l'ordre choisi ?
4. Revenir à l'ordre qui donne les bonnes couleurs ; redémarrer encore une fois.

**Attendu** : Le DMD redémarre, les couleurs changent avec l'ordre, l'ordre choisi est conservé après un nouveau redémarrage.

## T02 · Ordre des couleurs de la dalle (Raspberry Pi)  —  *Raspberry Pi*

**Étapes**

1. Page web du Pi → **Réglages** → *Ordre des couleurs*.
2. Choisir un autre ordre → appliquer (le service redémarre).
3. Vérifier les couleurs sur la dalle, puis remettre le bon ordre.

**Attendu** : La dalle revient avec le nouvel ordre ; le réglage survit à un redémarrage du Pi.

## T03 · Mise en page de la page Réglages  —  *ESP32 / Pi / Toolkit*

**Étapes**

1. Ouvrir les **Réglages** sur l'ESP32 et sur le Pi (téléphone et PC).
2. Vérifier les sections : *Canal du firmware*, *Affichage* (ordre des couleurs au-dessus de la luminosité), *Démarrage et modes*.
3. Vérifier que les trois boutons (changer de canal / appliquer et redémarrer / configurer) ont la même taille et sont alignés.
4. Changer la langue de la page FR / EN / ES.

**Attendu** : Même aspect sur les deux appareils, aucun chevauchement, textes traduits, commandes utilisables sur téléphone.

## T04 · Changement de canal alpha ↔ stable (ESP32)  —  *ESP32*

**Attendre au moins 60 secondes de fonctionnement normal entre deux changements.**

**Étapes**

1. **Réglages** → *Passer au canal STABLE…* → confirmer.
2. Attendre environ 1,5 minute (téléchargement, redémarrages).
3. Vérifier le titre sur la dalle (`STABLE`) et la version sur la page.
4. Attendre 60 s, puis repasser en ALPHA.

**Attendu** : Les deux changements réussissent, les pages web reviennent toutes seules, le Wi-Fi et les réglages sont conservés.

## T05 · Changement de canal alpha ↔ stable (Raspberry Pi)  —  *Raspberry Pi*

**Étapes**

1. **Réglages** du Pi → *Passer au canal STABLE…* → confirmer.
2. Attendre le redémarrage du service ; vérifier le titre `Pi STABLE` et la version.
3. Repasser en ALPHA.

**Attendu** : Le démon du canal choisi est installé à chaque fois ; le Pi ne change jamais de canal tout seul.

## T06 · Les avis de mise à jour restent dans le canal alpha  —  *ESP32 / Pi / Toolkit*

**Étapes**

1. Ouvrir les pages web (ESP32, Pi) et la boîte à outils.
2. Lire les bandeaux de mise à jour (firmware, Pi, toolkit).
3. Suivre les liens.

**Attendu** : Les bandeaux ne citent que des versions alpha (pré-releases) ; jamais un numéro ou un téléchargement stable.

## T07 · Les pages web reviennent après une mise à jour du firmware  —  *ESP32*

**Étapes**

1. Mettre à jour le firmware par Wi-Fi (page web ou Mode 14 du toolkit).
2. Attendre 1 à 2 minutes sans rien toucher.
3. Recharger la page web.

**Attendu** : Les pages reviennent toutes seules. Si la page de secours s'affiche, signalez la ligne de cause (capture d'écran).

## T08 · Les réglages survivent aux redémarrages  —  *ESP32*

**Étapes**

1. Modifier quelques réglages (luminosité, langue, un widget) et enregistrer.
2. Redémarrer le DMD trois fois (bouton / alimentation).
3. Vérifier les réglages et qu'il rejoint bien votre Wi-Fi.

**Attendu** : Tout est conservé ; le DMD ne retombe jamais sur son propre point d'accès (mode AP).

## T09 · Changer de réseau Wi-Fi  —  *ESP32*

**Étapes**

1. Page **Réseau** → choisir ou ressaisir votre Wi-Fi → enregistrer.
2. Attendre la reconnexion.
3. Recharger la page Réglages.

**Attendu** : Le DMD se reconnecte et les pages se chargent ; rien d'autre n'est perdu.

## T10 · GIF du jeu pendant la partie  —  *ESP32 / Pi / Toolkit*

**Étapes**

1. Page web *Basic* → activer *GIF du jeu pendant la partie*.
2. Relancer le **Mode 9** (scripts Recalbox) du toolkit une fois.
3. Lancer des jeux dont un GIF porte le même nom (à placer dans `/gifs_jeux` sur la carte SD, ou laisser le mode auto).
4. Observer : 1 à 5 GIF, puis le logo ; laisser un jeu en mode démo.

**Attendu** : Seules les correspondances sûres sont jouées (jamais le mauvais jeu) ; 1 GIF par passage en démo ; un fichier d'exclusions exclut bien.

## T11 · Personnalisation des logos  —  *Toolkit*

**Étapes**

1. Toolkit → onglet *Personnalisation* → choisir un jeu.
2. Remplacer son logo par une image, puis par un GIF animé, puis par une carte texte.
3. Utiliser *Annuler* (version précédente, puis l'original), *Remettre en état*, et la sauvegarde / restauration zip.
4. Recommencer sur la carte SD et sur un Raspberry Pi en Wi-Fi si vous en avez un.

**Attendu** : Le logo change sur le DMD, chaque annulation rend ce qu'il faut, rien n'est perdu.

## T12 · Pastille de cible et Playlist en Wi-Fi  —  *Toolkit*

**Étapes**

1. Ouvrir le toolkit avec une carte insérée, puis avec un Pi sur le réseau.
2. Vérifier la pastille de cible (ESP32 / Pi) et les modes grisés.
3. Onglet *Playlist* : écrire sur la carte, puis en Wi-Fi vers un Pi sans carte (playlists saisonnières aussi).

**Attendu** : La bonne cible est présélectionnée, les modes inapplicables sont grisés, les playlists arrivent sur l'appareil.

## T13 · Onglet Installation (Mode 1)  —  *Toolkit*

**Étapes**

1. Toolkit → onglet *Installation* → page *Sans Recalbox*, puis *Avec une Recalbox*.
2. Parcourir Démarrer / Suivant / Continuer : dossier ROMs d'abord, puis version de la Recalbox.
3. Lire les avertissements sur la version de la Recalbox et les médias scrapés.

**Attendu** : Suivant / Continuer refusent un dossier ROMs invalide ; les avertissements sont clairs ; la copie se termine.

## T14 · Formater une carte SD en FAT32  —  *Toolkit*

**⚠️ Cela EFFACE la carte. Utilisez une carte de rechange, jamais une carte utile.**

**Étapes**

1. Insérer une carte de rechange (idéalement de plus de 32 Go).
2. Toolkit → Mode 1 → fenêtre Carte SD → *Formater en FAT32…*.
3. Accepter la demande administrateur et les deux confirmations qui nomment le lecteur.
4. Mettre la carte dans le DMD et vérifier qu'il démarre.

**Attendu** : Seules les cartes amovibles sont proposées ; la carte est formatée en FAT32 et fonctionne dans le DMD.

## T15 · Alerte d'écart de canal dans le toolkit  —  *Toolkit*

**Étapes**

1. Mettre un DMD en canal STABLE (T04).
2. Ouvrir le toolkit alpha avec ce DMD joignable.

**Attendu** : Le toolkit signale l'écart et propose la boîte à outils du canal du DMD.

## T16 · Le bouton de mise à jour du Pi reste en alpha  —  *Raspberry Pi*

**Étapes**

1. Sur la page web du Pi (alpha), utiliser le bouton de mise à jour quand un nouveau démon alpha est annoncé.
2. Vérifier la version et le titre ensuite.

**Attendu** : Le Pi reste en alpha (`Pi ALPHA`), n'installe jamais un démon stable.

## T17 · Thèmes Recalbox (Modes 12 / 13)  —  *Toolkit*

**Étapes**

1. Lancer le Mode 12 ou 13 sur une Recalbox alpha.
2. Vérifier que les logos du thème Recalbox par défaut ne sont pas écrasés par une ancienne conversion.
3. Retélécharger un thème : les fichiers périmés doivent disparaître.

**Attendu** : Le thème par défaut reste à jour avec la Recalbox alpha ; aucun reste périmé.
