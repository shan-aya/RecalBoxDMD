# FAQ & Dépannage

[🇬🇧 English](FAQ.md) · **🇫🇷 Français** · [🇪🇸 Español](FAQ.es.md)

## ⚠️ Alimentation — à lire si ton DMD gèle ou se corrompt

Le DMD tire **toute** son alimentation du port/câble USB — il n'y a pas d'entrée d'alimentation séparée pour le panneau LED. Une image claire allume beaucoup plus de LED à pleine intensité qu'une image sombre, ce qui fait que l'appel de courant du panneau grimpe nettement à chaque fois que l'affichage passe sur quelque chose de clair. Sur un port/câble USB qui ne suit pas ce pic, les symptômes peuvent ressembler à un bug du firmware : l'animation se fige sur une image, l'affichage se corrompt/montre des pixels aberrants, ou dans les cas plus sévères le DMD disparaît carrément du PC (le port série/COM tombe avec une erreur Windows) pendant un instant.

**Si tu observes des gels, de la corruption d'affichage ou des déconnexions aléatoires — surtout si ça semble arriver sur des GIF clairs/lumineux — essaie, dans cet ordre :**

1. **Utilise un câble USB plus court/de meilleure qualité.** Un câble fin ou long est la cause la plus fréquente de chute de tension USB.
2. **Alimente-le via un hub USB alimenté** plutôt qu'un port PC/portable direct, ou un adaptateur secteur USB 5V type chargeur de téléphone (pas besoin de data une fois que ça tourne, seulement pour le flashage/la config). Certains ports PC ne fournissent tout simplement pas assez de courant lors d'un pic soudain.
3. **Baisse la luminosité** depuis la page de configuration web — un réglage `brightness` plus bas réduit directement l'appel de courant par LED, ce qui réduit la taille du pic sur un contenu clair. Pas besoin d'aller jusqu'à 10% ; l'objectif est juste d'avoir de la marge par rapport à ce que ta source USB peut fournir de façon fiable.

Ce n'est pas un bug du firmware et reflasher ne réglera rien — c'est le budget de puissance physique de l'USB. Une fois que tu as trouvé une combinaison câble/source d'alimentation/luminosité stable pour ton installation, elle reste stable — c'est un réglage à faire une seule fois par DMD, pas quelque chose qui revient plus tard.

## ⚠️ Révision du chip ESP32 — certaines cartes peuvent être plus sensibles que d'autres

<!-- TODO (utilisateur) : completer avec les infos precises sur les revisions ESP32-D0WD-V3 concernees, les references produit ou marchands a eviter/preferer, et tout autre retour d'experience -->

Tous les modules ESP32 ne sont pas identiques sous le capot — Espressif a produit plusieurs **révisions de silicium** de la puce présente dans le module ESP32-D0WD-V3 courant (par ex. v3.0 vs v3.1) au fil des années, chacune avec son propre lot de défauts corrigés/introduits au niveau matériel. En pratique, certaines révisions semblent **plus sensibles aux conditions d'alimentation limites** que d'autres — les gels/corruptions décrits plus haut peuvent apparaître bien plus tôt (ou jamais) selon uniquement la révision présente sur ta carte précise, même avec exactement le même firmware et la même installation USB que quelqu'un qui n'a jamais rencontré le problème.

Tu peux vérifier quelle révision ta carte annonce pendant le flashage — la sortie du Web Installer / `esptool` affiche une ligne du type `Chip is ESP32-D0WD-V3 (revision vX.X)`. Il n'y a actuellement aucun moyen de changer ça après coup (c'est gravé dans la puce physique) — si ta carte s'avère être une révision plus sensible, les conseils d'alimentation ci-dessus deviennent d'autant plus importants pour toi qu'ils pourraient ne pas l'être pour le DMD de quelqu'un d'autre.

## La qualité de la carte microSD compte

Le firmware lit la carte SD en permanence — chaque GIF, la playlist, et son propre fichier de configuration y vivent — donc une carte limite ou défaillante peut causer un éventail de symptômes surprenant, sans aucun rapport apparent avec le firmware lui-même : retour en boucle sur l'écran de configuration WiFi au lieu de se connecter, réglages qui ne se sauvegardent pas vraiment, affichage bloqué sur une image, et plus encore. Utilise une carte microSD authentique, raisonnablement récente, d'une marque connue — les cartes très bon marché/sans marque sont de loin la source la plus fréquente de ce genre de problèmes.

**Si tu suspectes une erreur de carte SD :**

- Éteins le DMD, retire la carte SD, et remets-la fermement en place. Ça suffit à résoudre la plupart des soucis de lecture transitoires.
- Redémarre le DMD ensuite si le problème persiste.

## La page de configuration web charge lentement, ou pas du tout

La pile WiFi/réseau du DMD peut être réellement occupée à un instant donné (elle parle à Recalbox, elle affiche, elle traite d'autres requêtes) — qu'une page de l'interface de configuration web charge lentement, ou qu'une sauvegarde/action mette du temps à répondre, c'est normal de temps en temps, pas forcément le signe d'un problème.

**Bons réflexes dans ce cas :**

- **Ne martèle pas le rafraîchissement/la nouvelle tentative.** Chaque tentative est une nouvelle requête qui vient en concurrence avec ce que le DMD est déjà en train de faire — spammer le rechargement a tendance à faire s'accumuler les demandes plutôt qu'à accélérer quoi que ce soit.
- **Attends l'erreur de ton propre navigateur** (un timeout, "impossible d'accéder à cette page", etc.) avant de retenter. Une seule nouvelle tentative après ce point suffit généralement.
- **Si ça ne charge toujours pas ensuite, redémarre le DMD.** Un redémarrage frais restaure généralement un accès immédiat à la page de configuration web.

## Coupure WiFi brève / le DMD semble désynchronisé de Recalbox

Une coupure WiFi courte et temporaire ne nécessite aucune action de ta part. Le firmware détecte la reconnexion tout seul, généralement en quelques secondes, et resynchronise automatiquement l'affichage du DMD avec ce que Recalbox est réellement en train de faire à ce moment-là (jeu en cours, navigation, mode démo...) — inutile de redémarrer quoi que ce soit, sur le DMD ou sur Recalbox, pour qu'une coupure de ce genre se résolve d'elle-même.

## Reboucle sur le point d'accès WiFi du DMD lui-même (mode AP)

Si, après avoir enregistré ton réseau WiFi sur la page de configuration captive du DMD lui-même, il reboucle sans arrêt sur son propre point d'accès (`RecalBox-DMD-Config`) au lieu de rejoindre ton réseau, la solution la plus simple est d'éviter complètement cet écran : utilise plutôt **l'étape WiFi intégrée au Mode 1 de la boîte à outils PC**. Elle scanne tes réseaux, vérifie le mot de passe en testant réellement la connexion depuis ton PC, et l'écrit directement dans `config.ini` sur la carte SD avant même que la carte n'aille dans le DMD — le DMD rejoint alors ton réseau dès son tout premier démarrage, et l'écran de portail captif n'a jamais besoin d'apparaître.

Si tu dois quand même utiliser la page de configuration du DMD lui-même et qu'elle reboucle : vérifie bien le mot de passe (une faute de frappe est la cause la plus fréquente), et regarde "la qualité de la carte microSD" plus haut — une carte défaillante peut empêcher silencieusement la sauvegarde des réglages WiFi, même quand la configuration elle-même semblait s'être bien déroulée.

## Donner une IP fixe à ton DMD — choisis UNE seule méthode, jamais les deux

Certaines intégrations — les scripts côté Recalbox qui parlent au DMD, par exemple — le joignent via une adresse IP fixe. Si l'adresse de ton DMD change au fil du temps (la plupart des routeurs/box distribuent des adresses dynamiquement via DHCP, et peuvent en attribuer une différente après un redémarrage ou une reconnexion), tout ce qui se base sur l'ancienne adresse s'arrête silencieusement de fonctionner.

Il existe deux façons de lui donner une adresse fixe :

- **Une réservation DHCP sur ton routeur/ta box**, qui associe une adresse fixe à l'adresse MAC WiFi du DMD — le DMD continue de demander une adresse normalement, ton routeur lui donne juste toujours la même.
- **Une IP statique configurée directement sur le DMD** (`wifi_static_enabled` / `wifi_static_ip` dans `config.ini`, ou les champs correspondants sur la page de configuration web) — le DMD s'attribue lui-même l'adresse, sans rien demander au routeur.

**Mets en place une seule des deux méthodes, jamais les deux en même temps.** Configurer une réservation DHCP sur le routeur *et* une IP statique sur le DMD peut les faire pointer vers des adresses différentes et en conflit, ou échouer d'une façon difficile à diagnostiquer. Choisis celle qui te convient le mieux — une réservation côté routeur est généralement plus simple et garde toute la configuration réseau au même endroit — et laisse l'autre méthode de côté.

## Les scripts Recalbox ne semblent plus rien faire après une mise à jour

Si le DMD arrête de réagir à ce qui se passe sur Recalbox après une mise à jour du firmware ou de la boîte à outils PC, les scripts côté Recalbox déjà installés sont peut-être obsolètes. Réinstalle-les avec le **Mode 9** (ou un nouveau **Mode 1**) depuis la boîte à outils PC — ça retire aussi automatiquement les anciennes versions de scripts. Consulte [UPGRADING.md](UPGRADING.fr.md) si tu viens d'une version plus ancienne : certaines mises à jour changent le protocole sous-jacent utilisé par les scripts et le DMD pour se parler, donc les scripts ont vraiment besoin d'être réinstallés, pas seulement le firmware reflashé.

## Dépannage général

| Symptôme | Cause probable | Essaie ceci |
|---|---|---|
| L'animation se fige sur une image, parfois avec des artefacts visuels | Pic de courant USB sur une image claire (voir plus haut) | Câble plus court, hub/adaptateur alimenté, luminosité plus basse |
| Le DMD disparaît du PC (erreurs de port COM, "périphérique ne fonctionne pas") pendant un test en USB | Comme ci-dessus — assez sévère pour perturber le lien USB lui-même, pas seulement l'affichage | Comme ci-dessus |
| L'écran affiche "RecalBox connectée" alors que Recalbox est éteinte | Bug d'affichage cosmétique, corrigé en firmware v210+ | Mettre à jour le firmware |
| La page de config web charge lentement ou expire | Pile WiFi/réseau du DMD occupée à ce moment-là | Attendre l'erreur du navigateur, retenter une fois ; redémarrer si ça persiste |
| Affichage brièvement désynchronisé après une coupure WiFi | Normal — le firmware resynchronise tout seul | Rien à faire, attendre quelques secondes |
| Reboucle sur la config WiFi, réglages qui ne se sauvegardent pas, affichage bloqué | Carte microSD limite/défaillante | Retirer et remettre la carte fermement, redémarrer si besoin ; essayer une autre carte |
| Reboucle sans arrêt sur le point d'accès WiFi du DMD | Faute de frappe sur le mot de passe, ou sauvegarde du portail captif qui n'a pas pris | Utiliser plutôt l'étape WiFi du Mode 1 ; vérifier le mot de passe ; voir la note sur la carte microSD plus haut |
| Les scripts Recalbox ont arrêté de fonctionner après une mise à jour | Anciennes versions de scripts encore sur Recalbox | Réinstaller via le Mode 9 (ou un nouveau Mode 1) |

<!-- TODO (utilisateur) : ajouter d'autres entrees FAQ au fur et a mesure des retours -->
