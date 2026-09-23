# Changelog

Historique de **RecalBoxDMD — RawEdition v2.0**, couvrant à la fois le **firmware ESP32** (y compris sa page de configuration web) et la **boîte à outils PC**, depuis le tout premier commit jusqu'à aujourd'hui. Les entrées sont groupées par date ; chaque puce est étiquetée avec la partie du projet qu'elle concerne.

[🇬🇧 English](CHANGELOG.md) · 🇫🇷 **Français** · [🇪🇸 Español](CHANGELOG.es.md)

Ceci est un résumé sélectionné de l'historique interne des versions du projet (185+ révisions firmware, 64+ révisions config web, 43+ révisions boîte à outils, 62+ révisions GUI) — regroupé par jalons réellement pertinents pour un utilisateur, pas un déversement brut de chaque micro-correctif.

---

## 2026-09-24 — Toolkit PC : cache des systèmes corrigé, dossiers par défaut des Modes 2/3/6/7/11

- **Toolkit PC** (build `8053`) : correction du **Mode 7** qui annonçait « 0 système trouvé » sur un dossier dont les images de jeux ne sont pas encore converties (par exemple juste après un Mode 3). Le type de chaque système (image fixe, animation ou les deux) est maintenant lu dans son image par défaut de `systems/_defaults/`, exactement comme le fait le firmware du DMD — il était jusqu'ici déduit à tort des images de jeux du dossier du système. Vaut aussi pour le Mode 1. Le cache des systèmes ne liste plus que les dossiers de systèmes réellement présents (plus `default`), jamais plus que ce que le DMD peut garder.
- **Toolkit PC** : le drapeau « lent » du cache des systèmes compte aussi les images pas encore converties (un `.png` compte comme son futur `.raw565`, un `.gif` comme son futur `.raw565pack` + `.meta`) : un gros dossier comme `mame/S` est bien marqué lent même avant la conversion.
- **Toolkit PC** : les Modes 2, 3 et 11 ouvrent le dossier de travail à la fin ; les Modes 6 et 7 pointent par défaut sur le dossier `systems` du dossier de travail s'il existe (un dossier choisi à la main est conservé).

## 2026-09-23 (2) — Toolkit PC : affichage du Mode 9 corrigé, nettoyage des scripts

- **Toolkit PC** (build `7951`) : les boutons Pause / Reprise / Passe / Stop du cadre Progression ne disparaissent plus en bas de la fenêtre pendant le Mode 9 (ils étaient repoussés quand le texte de résultat apparaissait). Le cadre Progression garde toujours sa place, le résultat du Mode 9 a une hauteur fixe et la description du Mode 9 est plus courte (elle mentionne maintenant la copie des `.hi`).
- **Toolkit PC** : après l'installation des scripts, le message demande de **redémarrer la Recalbox** — redémarrer seulement EmulationStation laisse tourner les anciens scripts.
- **Scripts Recalbox** (`dmd_score` v54, `dmd_achievement` v8) : restes de code MQTT retirés (le DMD n'utilise plus MQTT) ; le journal du pont RetroAchievements s'appelle maintenant `dmd_achievement.log`. Aucun changement de comportement. Réinstallez avec le Mode 9, puis redémarrez la Recalbox.

## 2026-09-23 — Hi-scores indépendants de la version de MAME, tables vérifiées MAME corrigées

- **Scripts Recalbox** (`dmd_hiscore_verified.py` v2) : correction des tables de hi-score vérifiées à la main **qui ne s'affichaient jamais pour les jeux MAME** — la recherche utilisait le nom du système (`mame`) alors que les clés de la table portent la version de MAME (`mame0278_…`). Environ 2270 jeux MAME affichent maintenant leur table quand aucun vrai `.hi` n'existe. Réinstallez les scripts avec le Mode 9.
- **Scripts Recalbox** (`dmd_hiscore_generic.py` v6) : le dossier de hi-score MAME n'est plus écrit en dur — le script utilise le cœur MAME réellement utilisé par Recalbox (`mame.core` dans `recalbox.conf`, sinon le dossier où MAME a écrit en dernier). Un futur cœur MAME ne demande aucune mise à jour du script.
- **Toolkit PC** (build `7651`) : les `.hi` MAME sont copiés dans le dossier du cœur MAME utilisé sur la Recalbox (même règle), au lieu d'un dossier `mame0278` fixe. Un `.hi` existant n'est toujours jamais écrasé.
- **Données** : dans [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/), les fichiers MAME passent dans `hi/mame/hiscore/` (sans version) et un nouveau `hiscore_hi_pack_v2.zip` est utilisé par le Toolkit ; l'ancien zip reste pour le build 7550.

## 2026-09-22 — Le Toolkit PC copie les fichiers de hi-score, nouveau dossier de données hi-score

- **Toolkit PC** (build `7550`) : **le Mode 1 et le Mode 9 copient maintenant aussi les fichiers de hi-score (`.hi`)** — environ 3000 fichiers pour FBNeo et MAME 0.278 — dans `/recalbox/share/saves` sur la Recalbox, pour que le DMD ait des scores à afficher pour des jeux jamais joués. **Un `.hi` déjà présent sur la Recalbox n'est jamais écrasé** ; seuls les manquants sont ajoutés. Par le partage réseau, avec SSH en repli. Les tables de hi-score (JSON) étaient déjà installées avec les scripts.
- **Toolkit PC** : l'onglet Aide mentionne maintenant la copie des `.hi` (3 langues).
- **Docs / données** : nouveau dossier [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/) — 3161 fichiers `.hi` (FBNeo 2491, MAME 0.278 670), les deux fichiers JSON de hi-score, un manifeste de sommes de contrôle et un README, prêts à être transmis à l'équipe Recalbox. La table de hi-score `verified_default_scores.json` est mise à jour (3941 entrées : 2277 mame0278 + 1664 fbneo) ; réinstallez les scripts avec le Mode 9 pour l'obtenir.

## 2026-09-20 (2) — Mode Visual Pinball (VPX) sans redémarrage, nouvelle page d'accueil web config, options de veille Recalbox

- **Firmware** : nouveau **Mode Pinball (VPX)** — une table Visual Pinball lancée depuis Recalbox s'affiche en direct sur le DMD (protocole ZeDMD-WiFi, plugin DMDUtil côté Recalbox). **Aucun redémarrage du DMD** : il bascule sur place au lancement de la table et revient à la normale à la fin de la partie (ou environ 5 s après la dernière image). Pendant la table, la playlist et les écrans Recalbox (« RecalBox connectée »…) sont suspendus pour ne rien dessiner par-dessus. Désactivé par défaut ; la luminosité est restaurée à la sortie. Testé pour l'instant uniquement sur des tables 128×32.
- **Firmware** : pour lui faire de la place, le budget mémoire a été retravaillé — un décompresseur zlib minimal intégré (uniquement en pile) remplace le précédent, plus lourd, les tampons ne sont alloués que si le mode est activé, et l'index système/jeux en mémoire est redimensionné (300 → 160 entrées ; une ligne de log le signale si une très grosse collection atteint la limite). La mémoire libre gagne environ 12 Ko.
- **Config web** : le menu principal (page d'accueil) s'ouvre désormais sur le cadre **Affichage** — luminosité, démarrage silencieux et interrupteur **Mode Pinball (VPX)**. Son bouton Enregistrer affiche une confirmation sur la ligne 2 du DMD. La page Affichage conserve les options des écrans superposés en jeu.
- **Firmware** : le titre de démarrage du DMD affiche désormais la **vraie version du firmware** (ex. `RawEdition v2.31`) au lieu d'un `v2.0` figé ; le badge du README et le Web Installer affichent le même numéro.
- **Firmware + script Recalbox** : l'option Mode Pinball **autorise** désormais un nouveau script Recalbox, `dmd_vpx_config`, à vérifier et écrire les réglages Visual Pinball dont le DMD a besoin (DMDUtil / ZeDMD WiFi, AlphaDMD) dans `VPinballX-configgen.ini` à chaque démarrage de la Recalbox et après chaque partie — jamais pendant qu'une table tourne, et sans jamais écraser une IP que vous avez saisie. Décochée, le script ne fait rien. Un avertissement dans la bulle d'aide de l'option (config web) le précise. Nécessite de réinstaller les scripts avec le Mode 9.
- **Firmware** : corrige le mode Pinball sur les tables colorisées (Serum) — les grosses mises à jour de couleur étaient en partie perdues, d'où des fonds dessinés les uns sur les autres et des textes absents. L'émetteur coupe un message sur deux paquets UDP quand il dépasse 1400 octets ; le DMD recolle désormais ces messages au lieu de les jeter. Les tables colorisées (Diner…) s'affichent correctement.
- **Firmware** : corrige l'absence des overlays hi-score / infos jeu / description quand le DMD est allumé *avant* la Recalbox (seul un redémarrage du DMD y remédiait) — le DMD renvoie désormais ses réglages à la Recalbox à la première réponse après son démarrage ou après une coupure.
- **Docs** : les tables anciennes à afficheur à segments (alphanumérique) demandent d'activer le plugin VPX **AlphaDMD** côté Recalbox — voir le README (section Pinball). Testé sur 8 tables : 6 fonctionnent ; deux (Black Hole, Farfalla) ont une disposition d'afficheur que ce plugin ne gère pas.
- **Firmware** : corrige l'écran *RecalBox connectée* qui s'affichait alors que la Recalbox était éteinte (puis remplacé par *RecalBox hors ligne*) après « Reprendre DMD », le bouton Enregistrer de la page d'accueil ou la sortie du mode Pinball — il n'apparaît désormais qu'une fois la Recalbox réellement joignable.
- **Config web** : un bouton **Accueil** figure désormais en premier dans le bandeau de menu de toutes les pages de configuration (Affichage, Playlist, Wi-Fi & BT, Horloge, Médias).
- **Config web** : nouvelle section **Veille Recalbox** — pendant les économiseurs d'écran « démos de jeux » / « clips vidéo de jeux », choisissez entre suivre le logo du jeu (comme avant, par défaut) ou la playlist simple, comme les autres veilles.
- **Scripts Recalbox** (`marquee` v54, `dmd_score` v53) : les options de veille ci-dessus, et une sortie immédiate du Mode Pinball à la fin d'une partie. Réinstallez les scripts avec le Mode 9 pour en bénéficier.
- **Docs** : le README (3 langues) documente le Mode Pinball, la page d'accueil et les nouvelles clés `config.ini` (`feat_vpinball_dmd`, `feat_demo_follow`, `feat_clip_follow`).
- **Toolkit PC** (build `7449`) : l'onglet Aide est maintenant un vrai manuel d'utilisation (premier lancement, Mode 1 pas a pas, chaque mode de l'onglet Avance, onglet Playlist, scripts Recalbox dont `dmd_vpx_config`, depannage) au lieu d'une copie du README ; plus de mentions de MQTT.

## 2026-09-20 — IP du DMD découverte automatiquement, Toolkit PC adapté au DPI Windows, image shuffle installée, renommage d'étiquette SD rétabli

- **Scripts Recalbox** : corrige le DMD qui restait bloqué sur sa playlist de veille pendant une partie — les scripts avaient l'adresse IP du DMD codée en dur (`192.168.0.51`) et parlaient donc dans le vide sur tout réseau où le DMD avait une autre adresse. Ils utilisent désormais l'adresse depuis laquelle le DMD s'annonce réellement (repli sur l'ancienne valeur tant qu'aucune n'a été vue). Fait suite à un retour réel d'un testeur.
- **Firmware** : le renommage d'étiquette de la carte SD est de nouveau actif — il était resté désactivé depuis une session de diagnostic.
- **Toolkit PC** : toute l'interface suit maintenant la mise à l'échelle d'affichage de Windows (125 %, 150 %...) et pas seulement les polices — à 125 % sur un écran 4K, le bas de la fenêtre (la section Progression) était coupé. Pris en compte à la prochaine ouverture de session si l'échelle vient d'être changée.
- **Toolkit PC** : le numéro de build est maintenant affiché dans le bandeau de la fenêtre (actuellement `7349`), pour savoir facilement quelle version quelqu'un utilise.
- **Toolkit PC** : le Mode 9 rappelle désormais de redémarrer EmulationStation après l'installation des scripts utilisateur — le menu *Scripts utilisateur* reste grisé tant qu'EmulationStation n'a pas relu ses scripts au démarrage.
- **Toolkit PC** : la création de carte SD installe aussi l'image de brouillage CRT du mode shuffle (`_shuffle.raw565pack` + `_shuffle.meta`) ; auparavant ces deux fichiers étaient ignorés et le DMD affichait un écran vide en mode shuffle. Si ta SD a été créée avec un build antérieur, copie ces deux fichiers depuis `carte SD/systems/_defaults/` vers `systems/_defaults/` sur la carte.
## 2026-09-14 — MQTT entièrement retiré, crash watchdog corrigé, IP fixe optionnelle, page FAQ

- **Firmware** : le sous-système MQTT (code de connexion/tâche, ~800 lignes) est désormais entièrement retiré des sources — pas seulement désactivé par défaut comme la veille. Si tu avais branché quelque chose sur les anciens topics MQTT du DMD, voir [UPGRADING.md](UPGRADING.fr.md) pour ce que ça implique.
- **Firmware** : corrige un écran « RecalBox connectée » affiché à tort même quand la Recalbox est éteinte — il se déclenchait auparavant au simple ENVOI d'un message UDP, sans confirmation qu'il avait bien été reçu ; attend désormais une vraie réponse avant de s'afficher.
- **Firmware** : corrige un vrai crash — une simple visite normale de la page de configuration web pouvait occasionnellement déclencher le watchdog matériel de l'ESP32 et forcer un redémarrage (`WebServer::_parseRequest()` pouvait attendre activement jusqu'à 5 secondes sans jamais rendre la main, tout près de la limite des 5 secondes du watchdog de la plateforme elle-même). Corrigé et confirmé sur matériel réel : 300 requêtes rapprochées reproduisant exactement le déclencheur d'origine, aucun échec.
- **PC Toolkit** : l'étape de configuration WiFi du Mode 1 peut désormais aussi définir une IP fixe sur le DMD (optionnel, décoché par défaut) — inclut un avertissement explicite contre la configuration simultanée d'une réservation DHCP côté routeur ET d'une IP statique côté DMD (peut causer de vrais problèmes de connexion), et pré-remplit les champs réseau à partir de la configuration de ce PC pour réduire le risque de faute de frappe.
- **PC Toolkit** : corrige une fausse erreur « impossible de joindre la Recalbox » dans le Mode 1 malgré une IP correcte — le test de joignabilité ne faisait qu'une connexion TCP brute au port SMB, que certaines règles pare-feu/antivirus bloquent pour un processus non reconnu, alors que l'Explorateur Windows accède normalement au même partage via son propre client SMB ; l'outil retente désormais un vrai accès au partage (le même mécanisme que l'Explorateur) avant d'abandonner.
- **PC Toolkit** : tentative de correctif pour la disparition du bas de l'interface (la section Progression) sur les écrans haute résolution (signalé par un testeur sur un écran 4K à 125 % de mise à l'échelle) — des retours ultérieurs ont montré que ça ne couvrait pas tous les cas ; un correctif complémentaire est en cours.
- **Docs** : nouvelle page dédiée [FAQ et dépannage](FAQ.fr.md) — sensibilité à l'alimentation USB (luminosité vs appel de courant), remarques sur la révision du chip ESP32, qualité de la carte microSD, boucles de configuration WiFi, conseils pour l'IP fixe, et plus — reliée depuis la section Dépannage du README.

## 2026-09-13 — MQTT → UDP : fusion de `dev/dmd-udp-transport` sur master

- **Firmware** : la liaison temps réel entre Recalbox et le DMD passe de **MQTT à UDP** — l'ancien transport se heurtait à un mur au niveau de la plateforme, dans la pile TCP/IP de l'ESP32 (un `TCP_SND_BUF` fixe d'environ 5,7 Ko, figé dans le core Arduino précompilé, sans levier applicatif possible) qui pouvait bloquer un `SUBSCRIBE` MQTT plusieurs secondes après une reconnexion ; l'UDP n'a pas cette poignée de main à bloquer. Le support MQTT était encore présent dans le firmware à ce moment-là, désactivé par défaut, comme filet de repli — entièrement retiré le lendemain, voir ci-dessus.
- **Firmware** : chasse de plusieurs mois à un gel de réception UDP sévère historiquement signalé (12 à 90s+) — jamais reproduit après une campagne d'instrumentation intensive (sonde active de gel, mesure du pire cas par appel, trafic réel soutenu), bien qu'une dizaine de bugs réels sans rapport aient été trouvés et corrigés au passage (ci-dessous). Conclusion actuelle : les signalements initiaux étaient très probablement un effet de bord bénin d'un seuil de détection trop agressif face à une perte de paquet UDP ordinaire, pas un véritable gel de réception — détaillé dans `DECISIONS.md`.
- **Firmware** : corrige une fausse alerte "Recalbox hors ligne" déclenchée par la perte d'un seul paquet UDP (normal, attendu en UDP) — exige désormais deux pertes consécutives avant d'alerter.
- **Firmware** : corrige la resynchronisation après reconnexion qui pouvait rester bloquée sur le mode d'affichage de repli en cache (`MODE_PNG`) au lieu de rattraper l'état réel de Recalbox.
- **Firmware** : corrige "Reprendre DMD" (page de config web) qui forçait systématiquement la playlist d'attente quoi que fasse Recalbox (en jeu, démo, gameclip...) — une vérification obsolète datant de l'ère MQTT rendait la bonne branche inatteignable depuis la bascule UDP ; resynchronise désormais l'état réel, comme le fait une reconnexion.
- **Firmware** : réduit le nombre d'ouvertures SD par changement de jeu (jusqu'à 6 auparavant, 1 seule désormais dans le meilleur des cas) en mémorisant laquelle des deux conventions de nommage de dossiers la carte SD utilise, au lieu de tester les deux à chaque fois — réduit aussi l'exposition à un rare crash d'allocation mémoire dans le pilote de système de fichiers SD ; une nouvelle tentative automatique a été ajoutée pour la variante rattrapable de ce même crash.
- **Scripts Recalbox** : corrige un processus zombie du round-robin hi-score qui pouvait survivre à un réveil de l'écran de veille et continuer à afficher un vieux score par-dessus le marquee indéfiniment.
- **Scripts Recalbox** : corrige le mode `gameclip`/démo qui affichait la playlist générique au lieu du marquee du jeu du clip (reliquat d'un contournement de l'ère MQTT qui l'avait trop largement désactivé).
- **Docs** : l'ancien master est archivé sous `archive/mqtt-pre-udp-transport-final` — dernier état du projet avant cette bascule de transport, conservé pour pouvoir revenir en arrière.
- **Image de marque** : l'édition firmware du projet est renommée **Raw565 Edition → RawEdition v2.0** (le format pixel `raw565` lui-même, et tout ce qui repose dessus, ne change pas — seuls le nom produit/le badge de version changent).

## 2026-09-06 — Fusion de `dev/core-reassignment` sur master : écrans superposés en jeu, Challenge RB, onglet Playlist

La plus grosse fusion de l'histoire du projet — plusieurs mois de travail sur une branche séparée, réconciliés avec tout ce qui était sorti sur master entretemps, puis testés point par point (chaque conflit résolu et retesté individuellement) avant d'atterrir ici. Tu utilises déjà une version antérieure ? Voir **[UPGRADING.fr.md](UPGRADING.fr.md)**.

- **Firmware** : nouveau **système d'écrans superposés en jeu** — pendant qu'un jeu tourne réellement, le panneau peut alterner automatiquement le marquee avec le vrai **Hi-Score MAME/FBNeo** (manifeste communautaire, ~2 758 jeux, décodé depuis le fichier de score sauvegardé par l'émulateur lui-même, aucune lecture RAM en direct), les **Infos jeu** (description/genre/développeur/année depuis `gamelist.xml`), les **RetroAchievements** débloqués, et le classement mensuel du **Challenge** officiel de Recalbox. Entièrement passif côté DMD — toute la logique de timing vit dans les scripts côté Recalbox, le firmware se contente d'afficher ce qu'on lui envoie et revient tout seul au marquee sur son propre minuteur local, pour qu'un script lent ou en échec ne puisse jamais bloquer le panneau. Voir la [section dédiée du README](README.fr.md#écrans-superposés-en-jeu--hi-score-infos-jeu-succès--challenge-rb).
- **Boîte à outils PC** : le **Mode 9** (et l'installation automatique intégrée au Mode 1) installe désormais aussi les scripts Hi-Score/Infos jeu/Succès/Challenge (`dmd_helpers/`) et nettoie les anciens noms de scripts d'une installation précédente — auparavant, seule l'étape de mise en scène séparée du Mode 1 le gérait, pas le Mode 9 lui-même.
- **Boîte à outils PC** : le flag « lent » du système de masque (**« L »**, déclenche l'écran d'attente sur les grosses collections) est désormais calculé **par sous-dossier alphabétique (« bucket »)** au lieu de par système entier — un système avec un gros sous-dossier et plusieurs petits ne pénalise plus inutilement les petits. Le seuil par défaut de l'onglet Paramètres suit (5 000 → 800, à nouveau pertinent maintenant qu'il s'applique par bucket).
- **Boîte à outils PC** : **onglet Playlist** — créez vos propres rotations en mode attente en combinant le pack de 600 GIFs et vos propres GIFs (glissez un dossier PC) ; possible désormais aussi **en plein Mode 1**, avant même la copie du dossier de travail sur la carte SD, et plus seulement après coup depuis une carte insérée.
- **Boîte à outils PC** : la langue de l'interface par défaut est désormais celle du **système Windows** au premier lancement (au lieu de toujours l'anglais) — un choix explicite dans l'onglet Paramètres reste toujours prioritaire ensuite.
- **Boîte à outils PC** : plusieurs correctifs trouvés en testant la fusion en direct — un cadre « copie SD » resté affiché pouvait pousser la barre de Progression hors de la fenêtre fixe sur certains modes de l'onglet Avancé, le tirage aléatoire de thème pouvait tomber sur le thème « default » tout nu, et fermer l'appli en pleine ajout de GIFs perso (étape playlist du Mode 1) reprend désormais le pipeline au lieu de quitter.
- **Firmware / MQTT** : les 12 topics séparés `marquee/cmd/*` ont été fusionnés en un seul topic `marquee/cmd` (payload compact `CMD=/ARG=`) — réduit le nombre de souscriptions MQTT par (re)connexion de 12 à 2, limitant l'exposition à une condition rare de la pile WiFi de l'ESP32 où une souscription pouvait silencieusement ne jamais quitter l'appareil.

## 2026-08-19 — Détection des lecteurs amovibles & positionnement des popups

- **Boîte à outils PC** : correction de la détection de carte SD qui échouait silencieusement sur les builds récentes de Windows 11 — la liste des lecteurs reposait entièrement sur `wmic.exe`, retiré par défaut par Microsoft sur les versions récentes de Windows 11 ; un utilisateur voyait sa carte SD dans l'Explorateur Windows mais elle n'apparaissait jamais dans l'outil (Mode 1/6/8), sans aucun message d'erreur. La détection passe désormais par `Get-CimInstance` (PowerShell), avec l'ancien appel `wmic` conservé uniquement en dernier recours pour les environnements inhabituels.
- **Boîte à outils PC** : correction de plusieurs popups (fin de copie, sélection du lecteur carte SD, confirmation de fermeture) qui apparaissaient hors écran ou hors de la fenêtre principale, surtout sur les configurations multi-écrans. Double cause : la fenêtre principale n'avait jamais de position explicite au lancement (désormais centrée explicitement sur l'écran primaire au démarrage), et les builds `.exe`/`.msi` compilées n'avaient pas de manifeste Windows déclarant la conscience DPI — présent nativement en lançant depuis les sources Python, mais absent par défaut des sorties PyInstaller/cx_Freeze, ce qui pouvait fausser les coordonnées de fenêtre rapportées par Windows. Le centrage des popups se confine désormais aussi au vrai moniteur que Windows rapporte pour la fenêtre principale, plutôt qu'à la taille d'écran primaire uniquement connue de Tk.

## 2026-08-16 — Images systèmes/genres multilingues (FR/ES)

- **Boîte à outils PC** : le pack de secours `systems/_defaults` (badges de genre, pseudo-systèmes comme Favoris/Derniers Jeux Joués/Portages/Tous Jeux) est désormais disponible en **français et espagnol**, 60/60 chacun — icône conservée pixel pour pixel (vectorisée, pas juste agrandie), seul le texte a été re-rendu et traduit. Les genres pas encore traduits basculent simplement en anglais, jamais d'absence.
- **Boîte à outils PC** : nouveau sélecteur de **langue des images système** (EN/FR/ES, avec aperçu comparatif côte à côte) dans le Mode 1 (pipeline auto) et le Mode 2 (onglet Avancé, téléchargement `_defaults` seul) — `download_defaults()` récupère toujours d'abord le jeu anglais de base (repli garanti), puis surcharge avec les fichiers traduits de la langue choisie.
- **Boîte à outils PC** : le Mode 2 propose désormais systématiquement la galerie d'image de secours (fermer sans choisir revient au visuel par défaut du projet) au lieu d'une question oui/non conditionnée à "pas déjà défini" ; les popups de confirmation après sélection ont aussi été retirées (le choix est déjà visible/appliqué immédiatement).
- **Boîte à outils PC** : correction d'un vrai bug de lenteur dans `_parallel_download_batch()` — `urlretrieve()` n'avait aucun timeout, une seule connexion figée dans le pool de 16 threads pouvait bloquer son slot indéfiniment ; un timeout socket borné est maintenant posé le temps du lot.
- **Assets firmware** : 15 logos systèmes/genres ajoutés à `_defaults` — 10 manquants par rapport au jeu de logos officiel Recalbox, plus 5 ajouts très récents de la branche alpha de Recalbox (Cassette Vision, EXL 100, ST-V, Vircon32, et le nouveau pseudo-système **Challenges**).

## 2026-08-13 — Préparation de la publication

- **Docs** : réécriture complète du README en anglais/français/espagnol — captures d'écran, vraies images de l'appareil, référence des modes, guide matériel.
- **Firmware** : [installateur Web](https://shan-aya.github.io/RecalBoxDMD/) — flashez l'ESP32 directement depuis Chrome/Edge, sans Arduino IDE.
- **Boîte à outils PC** : installateur Windows (`.exe` via Inno Setup) et `.msi` (via cx_Freeze), plus un `install_and_run.bat` en un clic pour lancer depuis les sources.

## 2026-08-11 — Aperçus en direct, pack de GIFs, et accordéon de l'onglet Avancé

- **Firmware / Config web** : choisir un thème horloge ou déplacer le curseur de luminosité sur la page de config web **s'affiche instantanément sur le panneau physique**, avant même d'enregistrer.
- **Firmware** : correction du bug « Reprendre DMD » ignoré pendant qu'un aperçu de thème horloge tournait encore ; journalisation diagnostique de la raison du dernier reset au démarrage.
- **Boîte à outils PC** : les 8 radios à plat de l'onglet Avancé réorganisées en **5 catégories repliables** (téléchargements GitHub / Gamelist / Images / Caches / Scripts) ; **Mode 10** (définir/générer l'image de secours globale) et **Mode 11** (téléchargement en un clic du pack ~600 GIFs) ajoutés ; le seuil « L » des systèmes lents devient une valeur réglable dans l'onglet Paramètres au lieu d'une constante codée en dur.

## 2026-08-09 – 2026-08-10 — Passe de stabilité sur matériel réel

- **Firmware** : plusieurs correctifs trouvés uniquement via des tests directs sur matériel, autour du masque des systèmes lents et de la recherche rapide de jeu.
- **Boîte à outils PC** : le travail sur le seuil du flag « L » a commencé ici (voir ci-dessus), motivé par des différences réelles de vitesse de carte SD signalées par les utilisateurs.

## 2026-08-06 – 2026-08-07 — Stabilité du tas mémoire (heap)

- **Firmware** : deux correctifs indépendants de fragmentation du tas (une étape dédiée de génération de playlist, désactivation de la reconnexion WiFi automatique) — aucun incident ensuite lors de tests intensifs réels, y compris une coupure/redémarrage du routeur en cours d'utilisation.

## 2026-08-05 — Fusion `dev/tous-txt-filter`

- **Boîte à outils PC** : outillage playlist et base du système de banque de GIFs GitHub fusionnés dans la branche principale.

## 2026-08-03 — Refonte du flux de premier démarrage

- **Firmware / Config web** : la page de configuration premier démarrage / point d'accès WiFi largement retravaillée suite à des tests réels de premier lancement.
- **Boîte à outils PC** : mises à jour correspondantes du sélecteur d'image de secours et des popups autour des messages de premier lancement/redémarrage.

## 2026-08-01 – 2026-08-02 — La refonte `cache_master_gifs`

- **Firmware + Config web + Boîte à outils PC** : refonte en trois parties du pipeline de playlists GIF autour de `cache_master_gifs.dat`, un index maître de tous les GIFs déjà présents sur la carte SD — accélère la navigation dans les dossiers de la page web Médias et la construction de playlists dans l'onglet Playlist de la boîte à outils, et a rendu les envois en gros volume depuis la page web bien plus fiables (ajustement de taille de buffer, sérialisation des envois pour éviter `ERR_INVALID_CHUNKED_ENCODING`).

## 2026-07-26 – 2026-07-29 — Passe de débogage sur matériel réel

- **Firmware** : investigations sur l'usage du tas mémoire et la connexion MQTT en conditions réelles ; plusieurs régressions trouvées et corrigées de cette façon.
- **Boîte à outils PC** : le Mode 9 (installer les scripts Recalbox) fiabilisé après le diagnostic d'un vrai cas d'échec SMB/connexion invité sur une Recalbox réelle.

## 2026-07-22 – 2026-07-23 — Pipeline du Mode 1 & détection réseau

- **Boîte à outils PC** : `detect_recalbox_share()` (détection NetBIOS automatique de `\\RECALBOX\share`) et `resolve_recalbox_ip()` ; le flux d'installation des scripts Recalbox entièrement retravaillé après des tests réels.

## 2026-07-20 – 2026-07-21 — Audit de traduction & installateur de scripts

- **Boîte à outils PC** : audit complet de traduction FR/EN/ES avec parité stricte des clés entre les trois langues ; **Mode 9** livré — installe les scripts utilisateur Recalbox (pont marquee, récupération WiFi, synchro config web) directement via le partage réseau de la Recalbox, remplaçant une approche FTP antérieure que la Recalbox cible ne supportait en réalité pas.

## 2026-07-14 — 10e thème horloge

- **Firmware** : « Level 1-1 » — une recréation défilante du premier niveau de Super Mario Bros — ajouté comme 10e thème horloge.

## 2026-07-13 — Interface trilingue

- **Firmware + Config web + Boîte à outils PC** : français/anglais/espagnol ajoutés partout — la page de config web du DMD et la boîte à outils Windows partagent la même langue, poussée automatiquement au DMD au tout début du Mode 1.

## 2026-07-11 — Images de secours & prise en compte de la version Recalbox

- **Boîte à outils PC** : sélecteur d'image de secours personnalisée (choisir ce qui s'affiche quand rien d'autre ne correspond) ; le **sélecteur « Version Recalbox »** (10.x / 9.x / legacy) est introduit, pour que l'outil lise la bonne balise `gamelist.xml` (`<logo>`/`<thumbnail>`/`<image>`) et le bon dossier média selon votre configuration.

## 2026-07-10 — L'interface graphique arrive

- **Boîte à outils PC** : `RecalBoxDMD_GUI.py` v1 — une interface Tkinter enveloppant l'outil console ; copie SD reprenable après interruption ; affinement constant de la mise en page/UX les jours suivants (onglet Avancé, panneau de progression, popup d'exploration de la carte SD).

## 2026-07-08 — La boîte à outils PC est née

- **Boîte à outils PC** : version de base de `RecalBoxDMD_tool.py` (console) — extraction `gamelist.xml`, conversion PNG→raw565/GIF→raw565pack, construction du cache. Le Mode 8 (vérification images manquantes) livré dès le premier jour.

## 2026-07-02 — La page de configuration web est née

- **Firmware / Config web** : première version de la page de config dans le navigateur — FR/EN/ES avec détection automatique de la langue du navigateur, infobulles sur chaque champ, upload/upload multiple/suppression de GIFs, régénération automatique des playlists, et le DMD qui se met en pause avec un message de statut pendant les opérations SD. Une série dense de correctifs de fiabilité le même jour a suivi : évitement des timeouts watchdog dans les longues boucles SD, contournements `mkdir`/`rmdir` pour les particularités FAT32 en lecture seule, message de statut flottant persistant.

## 2026-07-01 — Les thèmes horloge arrivent

- **Firmware** : intégration de `retro_clock` — 9 thèmes horloge pixel-art (Super Mario, Tetris, Pac-Man, Space Invaders, Pong, Neon, Matrix, Fire, Rainbow), remplaçant l'ancien rendu de chiffres brut.

## 2026-06-11 – 2026-06-29 — Premiers durcissements

- **Firmware** : optimisations du rendu raw565/raw565pack ; sous-dossiers alphabétiques `A..Z/#` ajoutés spécifiquement pour contourner les ralentissements FAT32 au-delà d'environ 800 fichiers par dossier ; première horloge multi-style avec luminosité configurable ; un bug de gel de playlist corrigé.

## 2026-06-10 — Naissance du projet : le fork Raw565

- **Firmware** : fork de [RetroBoxLED de Jamyz](https://github.com/Jamyz/RetroBoxLED). Le pipeline original de décodage PNG/GIF est remplacé par un format maison **raw565**/**raw565pack**, un **cache de jeux indexé par bigrammes** (`games_cache.bin`), et le **masque « L »** des systèmes lents — la fondation qui permet à un fullset MAME de 30 000 jeux de s'afficher en quelques millisecondes, sans écran noir entre deux jeux.

---

*Les dates proviennent des en-têtes de version conservés en haut de chaque fichier source (`RecalBox_DMD.ino`, `web_config.h`, `RecalBoxDMD_tool.py`, `RecalBoxDMD_GUI.py`) — la convention de changelog interne du projet, condensée ici pour la lisibilité.*
