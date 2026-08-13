# ============================================
# safe-modify - Historique des modifications
# ============================================
# Version actuelle : v35
#
# v35 - 2026-08-13 - BRANCHE DEV (test avant fusion master) - Portage du
#      flag "L" (lent) par sous-dossier alphabetique depuis le worktree
#      dev/slow-flag-per-bucket (v29, jamais fusionne) sur l'outil actuel
#      (v34). build_systems_cache() : remplace le comptage recursif par
#      systeme entier (count_ext_over) par un comptage par SOUS-DOSSIER
#      ALPHABETIQUE (count_ext_over_per_bucket, bucket A..Z/#), reutilise
#      le parametre slow_threshold existant (v33) au lieu du seuil 800
#      code en dur de la version d'origine du worktree. systems_cache.dat
#      gagne un 4e champ (27 caracteres L/N, ordre LETTERS) :
#      "<val> <sysName> <slowFlag> <bucketFlags27>". Nouvelle fonction
#      _bucket_letter_for_stem() (meme regle que _alpha_subdir()).
#      Contrepartie firmware : RecalBox_DMD.ino v77 (meme branche dev).
#      PAS ENCORE teste (ni recompilation du firmware contre cette
#      sortie, ni materiel reel) -- a faire avant toute fusion master.
#
# v34 - 2026-08-11 - safe-modify - "main_opt_quit" (bouton Quitter, 3
#      langues) en MAJUSCULES -- demande utilisateur (voir aussi
#      RecalBoxDMD_GUI.py v46 pour le bouton Demarrer).
#
# v33 - 2026-08-11 - safe-modify - Seuil flag "L" (build_systems_cache(),
#      jusqu'ici code en dur a 5000) rendu reglable : nouveau parametre
#      optionnel slow_threshold sur build_systems_cache(), resolu depuis
#      RecalBoxDMD_prefs.get("slow_threshold") quand non fourni explicitement
#      -- source unique, tous les appelants (GUI Mode 1/3/8, CLI) heritent
#      automatiquement du reglage utilisateur (onglet Parametres, GUI v45)
#      sans modification de leurs appels. Repli 5000 sur toute erreur de
#      lecture/conversion (coherent avec la valeur par defaut historique).
#
# v32 - 2026-08-10 - safe-modify - Repli du seuil flag "L" de 15000 (v31,
#      test) a 5000, suite au test reel v31 : mame/S (1547) et mame/M (1019)
#      confirmes trop lents SANS mask (2-3s par jeu, cas fallback/jeu absent
#      mesure a 3.3s -- cause identifiee : drawRaw565() tente d'abord le
#      chemin alphaSubdirPath, qui force un scan du dossier physique complet
#      quand le fichier est absent -- 4641 entrees dans mame/S). mame redevient
#      seul systeme en flag L (10420 > 5000, tous les autres restent sous
#      5000 : atarist 3926 est le plus proche). Voir
#      [[project-recalbox-dmd-slow-flag-per-bucket]].
#
# v31 - 2026-08-10 - safe-modify - Seuil du flag "L" (lent, ecran masque
#      d'attente) remonte de 800 a 15000 (build_systems_cache(), 3 appels
#      count_ext_over sur .raw565/.raw565pack/.meta) -- decision utilisateur
#      pour tester en reel si le mask est encore necessaire maintenant que
#      le cache bigramme (games_cache.bin) est fiable (fix v30). Objectif :
#      plus AUCUN systeme en flag L avec ce seuil (le plus gros, mame,
#      totalise 10420 fichiers par extension < 15000). Si le delai reel
#      s'avere trop lent sans mask, repli prevu a 5000 (mame resterait seul
#      en L, tous les autres systemes restent sous cette barre -- voir
#      [[project-recalbox-dmd-slow-flag-per-bucket]] pour le detail des
#      comptages par systeme/bucket).
#
# v30 - 2026-08-09 - safe-modify - Fix bug reel signale par l'utilisateur :
#      flags "?" (fallback) sur de nombreux logos de jeux sur les systemes
#      flag "L" (lents, ex: amiga600), meme quand le fichier converti
#      existe bel et bien sur la carte SD sous le bon nom. Root cause
#      trouvee par comparaison exhaustive (676 paires de lettres testees,
#      script dedie) : _calc_bigram_idx() (ici, cote construction du
#      cache games_cache.bin) et bigramIndex() (RecalBox_DMD.ino, cote
#      lecture au runtime) calculaient des index COMPLETEMENT differents
#      pour la meme paire de lettres -- 100% de desaccord sur les 676 cas
#      testes. Les deux formules ne s'accordaient QUE par coincidence
#      quand le 1er caractere n'est pas une lettre (les deux cotes
#      retournent 0 dans ce cas special) -- ce qui explique pourquoi
#      certains jeux (ex: "4D Sport Driving", commence par un chiffre)
#      s'affichaient correctement alors que d'autres sur le MEME systeme
#      flag L (ex: "Zynaps") echouaient systematiquement : le systeme
#      flag L n'a AUCUN repli sur un acces disque direct en cas d'echec du
#      cache (a la difference des systemes normaux, ou openBestMedia() est
#      tente ensuite), donc un index bigramme errone y donne TOUJOURS "?".
#      Fix : _calc_bigram_idx() reecrite pour reproduire EXACTEMENT
#      l'algorithme bigramIndex() du firmware (meme decoupage 1+i1*27,
#      meme gestion des cas non-alphabetiques) -- verifie par script de
#      comparaison directe sur les 676 paires, 0 desaccord restant apres
#      fix. Necessite de regenerer games_cache.bin (Mode 5/6) et de le
#      recopier sur la carte SD pour que le fix prenne effet -- aucun
#      changement firmware necessaire (bigramIndex() cote C++ reste la
#      reference, c'est le cote Python qui s'aligne dessus).
#
# v29 - 2026-08-03 - safe-modify - RECONSTRUCTION apres perte accidentelle
#      du worktree dev-cache-externalisation (git worktree remove --force
#      execute par une autre session sur un worktree contenant des
#      fichiers jamais suivis par git -- tools/*.py entier, aucun backup
#      exterieur disponible). Reconstruit depuis la memoire projet
#      detaillee (project_recalbox_dmd_playlist_tab.md) directement dans
#      ce worktree (dev-tous-txt-filter, "chantier actif"), a l'etat
#      FINAL documente (sans rejouer chaque etape intermediaire de
#      l'historique v29-v49 d'origine). Ajout `import filecmp`
#      (necessaire pour copy_external_gifs_to_sd). Backend Playlist
#      complet : PLAYLIST_GIFS_DIRNAME/PLAYLIST_DIR_NAME/
#      PLAYLIST_FULL_MARKER_PREFIX, list_playlist_gif_folders(),
#      list_gif_files_in_folder(), list_existing_playlists(),
#      _parse_full_folders_marker(), read_playlist()/
#      read_playlist_full_folders(), write_playlist() (parametre
#      full_folders -> marqueur "# FULL:dossier1,dossier2" en tete si
#      non vide, retrocompatible sinon), delete_playlist(),
#      build_playlist_entries_from_folders()/_from_files(),
#      copy_external_gifs_to_sd() (collision : identique->skip via
#      filecmp, different->renommage auto _2/_3), regenerate_playlist_gifs_cache()
#      (cache_master_gifs.dat, extension .dat hors convention .txt donc
#      invisible pour le firmware/list_existing_playlists), update_playlists_referencing_folder()
#      (ajout auto aux playlists FULL qui referencent le dossier + repli
#      retrocompatible pour les playlists sans marqueur qui referencaient
#      deja le dossier), remove_playlist_entries()/delete_gif_file()/
#      delete_gif_folder() (suppression physique + nettoyage playlists).
#      Verifie (script scratchpad, 8 scenarios bout-en-bout : listing,
#      ecriture/lecture avec et sans marqueur, suppression, 3 cas de
#      collision de copie, regeneration du cache maitre, mise a jour
#      selective FULL vs personnalise vs legacy, suppression en cascade)
#      -- **1 vrai bug trouve et corrige pendant la reconstruction**
#      (absent de la version d'origine perdue, ou alors deja corrige la-
#      bas sans que la memoire le mentionne) : remove_playlist_entries()
#      couplait a tort la mise a jour du marqueur FULL au changement des
#      entrees de ligne (un dossier entierement supprime dont aucune
#      entree de playlist ne subsistait deja ne retirait pas ce dossier
#      du marqueur) -- les 2 aspects sont desormais verifies
#      independamment. py_compile + smoke-test OK.
#
# v28 - 2026-07-26 - safe-modify - VRAIE cause du bug v25/v26/v27 (repli SSH
#      qui echouait toujours avec "Authentication failed", meme apres le fix
#      look_for_keys=False de v26, meme IP confirmee identique au nom qui
#      marche en SMB) : RECALBOX_SSH_PASSWORD etait "root", alors que le
#      vrai mot de passe root par defaut de Recalbox est "recalboxroot"
#      (confirme par l'utilisateur -- "root" est correct uniquement pour le
#      nom d'utilisateur). Toutes les tentatives precedentes (v25 pre-check
#      reachability, v26 look_for_keys/allow_agent, v27 diagnostic repr())
#      etaient donc de fausses pistes qui n'auraient jamais pu resoudre une
#      authentification avec un mot de passe simplement faux. Corrige :
#      RECALBOX_SSH_PASSWORD = "recalboxroot". py_compile verifie OK. PAS
#      ENCORE reteste en conditions reelles (mais tres haute confiance --
#      cause directement confirmee par l'utilisateur, pas une deduction).
#
# v27 - 2026-07-26 - safe-modify - Suite du bug v25/v26 : IP confirmee
#      identique au nom "recalbox" par l'utilisateur (meme machine), donc
#      ni une IP perimee ni le blocage invite SMB classique (le message
#      remonte etait mode9_share_unreachable, pas mode9_guest_blocked).
#      Piste retenue la plus probable : Windows traite differemment un nom
#      simple (NetBIOS, zone "intranet local") d'une IP brute pour le SMB
#      invite -- comportement du client SMB de Windows, hors de portee du
#      code Python. Ajout d'un log de diagnostic (repr(recalbox_host)) en
#      entree de download_recalbox_scripts() pour ecarter definitivement un
#      caractere invisible (espace largeur nulle etc.) issu d'un copier-
#      coller, que .strip() ne retire pas et qu'un print() normal ne
#      revelerait pas -- les 2 chemins de saisie (CLI input().strip(), GUI
#      Entry.get().strip()) sont deja corrects par ailleurs. py_compile
#      verifie OK. PAS ENCORE reteste en conditions reelles.
#
# v26 - 2026-07-26 - safe-modify - Suite du bug v25 : l'utilisateur confirme
#      que l'IP saisie est correcte (accessible en SSH via un outil tiers
#      avec les memes identifiants root/root) -- donc pas un probleme d'IP
#      perimee, mais un vrai echec d'authentification paramiko cote outil
#      ("Authentication failed"). Cause probable : _ssh_connect_recalbox()
#      appelait client.connect() SANS look_for_keys=False/allow_agent=False
#      -- par defaut paramiko essaie d'abord toute cle privee locale (~/.ssh)
#      puis un agent SSH (Pageant...) AVANT le mot de passe fourni ; si
#      l'utilisateur a des cles SSH configurees pour d'autres usages, ces
#      tentatives peuvent epuiser le nombre max de tentatives d'authen-
#      tification du serveur dropbear de la Recalbox avant meme que
#      root/root soit essaye, remontant un echec generique meme si le mot
#      de passe est correct. Fix : look_for_keys=False, allow_agent=False
#      ajoutes -- force un essai direct par mot de passe/keyboard-
#      interactive, jamais de cle. python -m py_compile verifie OK. PAS
#      ENCORE reteste en conditions reelles.
#
# v25 - 2026-07-26 - safe-modify - Bug remonte : le Mode 9 (installation des
#      scripts Recalbox seule) transferait bien en saisissant le nom
#      ("recalbox") mais pas en saisissant directement l'IP. Cause trouvee
#      par lecture du code : download_recalbox_scripts() (utilisee par le
#      Mode 9) tentait os.listdir() sur le chemin UNC SANS la
#      pre-verification rapide is_recalbox_reachable() (port 445, ~1.5s)
#      que install_recalbox_scripts() (Mode 1) fait deja avant d'appeler
#      son equivalent -- asymetrie trouvee entre les deux modes. Sans ce
#      garde, un hote saisi en IP peut faire attendre le redirecteur SMB
#      de Windows tres longtemps avant de lever une erreur qui declenche le
#      repli SSH (v24), donnant l'impression que l'IP "ne marche pas" alors
#      que SSH aurait fonctionne s'il avait ete atteint plus vite. Fix :
#      meme garde is_recalbox_reachable() ajoutee dans
#      download_recalbox_scripts(), repli SSH immediat si le port SMB n'est
#      pas rapidement joignable. python -m py_compile verifie OK. PAS
#      ENCORE reteste en conditions reelles (a confirmer specifiquement
#      avec une IP directe).
#
# v24 - 2026-07-23 - safe-modify - Demande utilisateur (cas reel : un
#      utilisateur sur reseau segmente par VLAN a SSH joignable mais SMB
#      bloque -- 445/139 filtres, 22 autorise) : repli SSH/SFTP automatique
#      pour Mode 1 ET Mode 9 quand le partage SMB est injoignable.
#      Nouveau : RECALBOX_SSH_USER/PASSWORD ("root"/"root", identifiants
#      par defaut documentes publiquement par Recalbox),
#      RECALBOX_SSH_USERSCRIPTS_PATH ("/recalbox/share/userscripts"),
#      _ensure_paramiko() (installe paramiko via pip a la demande, meme
#      mecanisme que Pillow/ensure_dependencies() mais seulement si le
#      repli SSH est reellement tente), is_recalbox_ssh_reachable() (test
#      TCP port 22), _ssh_connect_recalbox()/_sftp_ensure_userscripts_dirs()
#      (helpers partages), install_staged_scripts_via_ssh() (Mode 1 --
#      copie via SFTP les fichiers deja mis en scene localement),
#      install_recalbox_scripts() (Mode 1 -- tente SMB, repli SSH
#      automatique si injoignable, retourne aussi la methode utilisee),
#      _download_recalbox_scripts_via_ssh() (Mode 9 -- meme repli mais
#      telechargement direct GitHub->SFTP, sans mise en scene locale,
#      pour ne pas changer le comportement de download_recalbox_scripts()
#      au-dela de l'ajout du repli). download_recalbox_scripts() (Mode 9)
#      appelle desormais ce repli des que le partage SMB est injoignable
#      (share introuvable OU acces invite bloque winerror 1272) au lieu de
#      simplement abandonner. Smoke-teste : is_recalbox_ssh_reachable,
#      _ensure_paramiko (installation reelle via pip reussie), toutes les
#      nouvelles fonctions chargent sans erreur. PAS ENCORE teste avec une
#      vraie Recalbox joignable en SSH seul (cas reel du VLAN).
#
# v23 - 2026-07-22 - safe-modify - Nouvelle write_dmd_recalbox_ip(sd_dir,
#      ip) (demande utilisateur) : ecrit/patch "recalbox_ip=" dans
#      config.ini (meme logique lecture-modification-ecriture que
#      write_dmd_language()). Cette cle est deja lue par le firmware pour
#      le MQTT et pre-remplit le champ "IP Recalbox" de la page web config
#      -- normalement auto-detectee par mDNS au premier boot, mais
#      seulement si la Recalbox est joignable a ce moment precis. L'ecrire
#      directement avec l'IP deja validee par l'utilisateur en Mode 1 (RB
#      confirmee ET scripts installes avec succes) evite cette dependance,
#      meme Recalbox eteinte au moment du premier boot/de la config web.
#      Appelee depuis RecalBoxDMD_GUI.py::_pipeline_mode_1() (v38).
#      Smoke-teste (3 cas : absent/present-patch-cible/ip-vide-no-op) OK.
#
# v22 - 2026-07-22 - safe-modify - Bug trouve : le dossier
#      "recalbox_userscripts" (mise en scene locale Mode 1, v21) etait
#      copie tel quel vers la carte SD par _copy_to_drive() (utilisee par
#      le GUI, Mode 6) -- alors qu'il est destine a une copie MANUELLE vers
#      le partage reseau de la Recalbox, pas a la SD du DMD. Fix : nouveau
#      _walk_for_sd_copy(src) (os.walk avec elagage top-level via
#      _SD_COPY_EXCLUDED_DIRS) utilise par _copy_to_drive() pour le
#      decompte total ET la boucle de copie (les deux DOIVENT utiliser le
#      meme elagage, sinon total_files desynchronise de copied). _robocopy()
#      (utilisee par le CLI, chemin distinct) recoit le meme traitement via
#      son flag /XD existant. Smoke-teste (dossier temporaire avec
#      systems/ + recalbox_userscripts/manual/) : seul systems/ apparait
#      dans le parcours, total_files correct.
#
# v21 - 2026-07-22 - safe-modify - Refonte de l'installation des scripts
#      Recalbox en Mode 1 (demande utilisateur, nouveau flux complet) :
#      (1) Nouvelle _list_recalbox_script_files() : factorise l'appel API
#      GitHub (deja present dans download_recalbox_scripts(), extrait sans
#      changement de comportement). (2) Nouvelle
#      stage_recalbox_scripts_locally(dest_dir, progress_cb) : telecharge
#      les scripts vers un dossier LOCAL (dest_dir/manual/ + dest_dir/,
#      meme arborescence que le partage reseau) -- ne necessite aucun acces
#      a une Recalbox, juste Internet. Appelee INCONDITIONNELLEMENT en tete
#      de _pipeline_mode_1() (GUI), pour que l'utilisateur ait toujours une
#      copie prete a glisser-deposer manuellement, meme si l'install reseau
#      echoue/n'est pas tentee. (3) Nouvelle
#      install_staged_scripts_to_share(staged_dir, host, progress_cb) :
#      copie (pas de re-telechargement GitHub) les scripts DEJA mis en
#      scene localement vers \\<host>\share\userscripts\... -- remplace
#      download_recalbox_scripts() comme mecanisme d'install reseau pour le
#      Mode 1 (Mode 9 continue d'utiliser download_recalbox_scripts()
#      directement, inchange). Smoke-teste : resolve_recalbox_ip,
#      is_recalbox_reachable, stage_recalbox_scripts_locally (4/4 fichiers,
#      arborescence manual/ correcte) OK.
#
# v20 - 2026-07-22 - safe-modify - Nouvelle resolve_recalbox_ip(host) :
#      resout un nom NetBIOS ("RECALBOX") en IP numerique via
#      socket.gethostbyname(), pour affichage utilisateur dans le pre-vol
#      Mode 1 du GUI -- le nom "RECALBOX" est identique quelle que soit la
#      Recalbox physique allumee sur le reseau (detection auto par nom de
#      machine par defaut), donc inutile pour distinguer laquelle est visee
#      quand plusieurs sont allumees ; seule l'IP le permet.
#
# v19 - 2026-07-22 - safe-modify - (1) Nouvelle is_recalbox_reachable(host) :
#      connexion TCP courte (port 445/SMB, timeout 1.5s) pour tester la
#      joignabilite REELLE d'une cible Recalbox, utilisee par le GUI en
#      pre-vol Mode 1 (le simple "il y a une IP en cache dans les prefs" ne
#      prouvait pas que la Recalbox etait allumee maintenant -- bug remonte
#      par l'utilisateur : aucun prompt ni test de connexion visible).
#      (2) mode1_title harmonise fr/en/es (contenu complet identique,
#      prefixe "MODE 1 (AUTO) —"/"MODO 1 (AUTO) —" au lieu de "MODE AUTO"
#      en FR / description tronquee en EN/ES).
#
# v18 - 2026-07-21 - safe-modify - mode_full() (Mode 1) : installation des
#      scripts Recalbox deplacee tout en tete de pipeline (avant extraction/
#      téléchargement/conversion), juste apres write_dmd_language() -- le
#      script marquee est indispensable au bon fonctionnement de l'appareil
#      (pas juste optionnel), demande utilisateur pour tenter cette etape
#      des le debut, pendant que l'utilisateur est encore devant son ecran,
#      au lieu de l'enterrer en fin de pipeline non surveille.
#
# v17 - 2026-07-21 - safe-modify - DMD multilingue (demande utilisateur) :
#      nouveau CURRENT_LANG (code langue courant "fr"/"en"/"es", tenu a
#      jour partout ou T est reassigne -- select_language() ici,
#      RecalBoxDMD_GUI.py::_set_toolkit_language() cote GUI). Nouvelle
#      write_dmd_language(sd_dir, lang) : ecrit/patch la cle "language="
#      dans sd_dir/config.ini (lecture-modification-ecriture, cree le
#      fichier si absent, preserve toute autre cle deja presente). Appelee
#      automatiquement dans mode_full() (Mode 1 uniquement, le pipeline
#      "premier lancement") juste apres prepare_sd_card(). Meme langue
#      pour l'outil et le DMD (pas de preference separee) -- decision
#      utilisateur. Smoke-test des 3 cas (absent/present/langue invalide)
#      OK.
#
# v16 - 2026-07-21 - safe-modify - [Session parallele, suite du v15] Une fois
#      le blocage invite SMB leve, `download_recalbox_scripts()` echouait
#      encore avec "HTTP Error 404" x4 (2 sous-dossiers x 2 tentatives) :
#      GITHUB_SCRIPTS_API_BASE pointait vers "contents/scripts" avec des
#      sous-dossiers "scripts/manual"/"scripts/events" qui n'existent pas
#      sur github.com/shan-aya/RecalBoxDMD (verifie en direct contre l'API
#      -- la racine du depot contient binaries/, carte SD/, docs/, history/,
#      medias/, tools/ ; aucun dossier "scripts"). Les 3 scripts existent en
#      realite a plat sous tools/ (WiFi Recovery DMD.sh, Config Web
#      DMD(sync)(progress).sh, marquee[...].sh). Fix : GITHUB_SCRIPTS_API_
#      BASE/GITHUB_SCRIPTS_RAW_BASE pointent desormais vers "contents/tools"
#      (un seul listing, plus de sous-dossiers cote GitHub) ; le tri manuel
#      vs evenement se fait cote client par nom de fichier (nouvelle
#      fonction _is_manual_script(), marqueur "Config Web DMD" -> userscripts
#      /manual, sinon -> userscripts/). Verifie en direct contre l'API
#      GitHub reelle : listing + routage + telechargement raw des 3 fichiers
#      OK (plus de 404).
#
# v15 - 2026-07-21 - safe-modify - [Session parallele -- voir aussi v14
#      ci-dessous, ecrit par une autre session Claude Code sur le meme lot
#      Mode 9] Diagnostic reseau reel sur un cas concret (IP 192.168.0.35,
#      Recalbox allumee/joignable confirme, ping+port 445 OK) : le message
#      mode9_share_unreachable() etait affiche a tort -- cause reelle =
#      Windows bloque l'acces invite SMB non authentifie
#      (EnableInsecureGuestLogons=False, defaut Windows 10/11), identique
#      que la cible soit "RECALBOX" ou l'IP directe (confirme par
#      `net use` -> erreur systeme 1272). `download_recalbox_scripts()`
#      remplace le test `share_root.exists()` (qui avale silencieusement
#      l'erreur reelle) par un `os.listdir(share_root)` explicite : si
#      `OSError.winerror == 1272`, affiche desormais le message dedie
#      `mode9_guest_blocked` (diagnostic + commande PowerShell admin
#      Set-SmbClientConfiguration -EnableInsecureGuestLogons $true -Force)
#      au lieu du message generique "partage introuvable" qui aiguillait a
#      tort vers une verification reseau/IP. Nouvelle cle TRANSLATIONS
#      (parite stricte fr/en/es, 219/219/219 verifie).
#
# v14 - 2026-07-21 - safe-modify - Backup pris apres coup (voir
#      _backups/_index.md, checkpoint 2026-07-21_00-31-49) : nouveau mode 9
#      "Installer les scripts Recalbox", remplace le mecanisme FTP cote
#      firmware (abandonne : la Recalbox cible ne fait tourner aucun serveur
#      FTP). Le transfert passe desormais par ce meme outil Windows, via
#      simple copie de fichier vers le partage SMB de la Recalbox
#      (`\\<host>\share\...`, resolu nativement par Windows, aucune
#      bibliotheque FTP/SMB ajoutee). Nouveau : constantes
#      GITHUB_SCRIPTS_API_BASE/GITHUB_SCRIPTS_RAW_BASE (depot
#      shan-aya/RecalBoxDMD, dossier scripts/), detect_recalbox_share()
#      (tente `\\RECALBOX\share` -- resolution NetBIOS du nom par defaut,
#      zero dependance -- avant repli sur saisie manuelle),
#      download_recalbox_scripts() (telecharge scripts/manual -> userscripts
#      /manual et scripts/events -> userscripts, meme mecanisme Contents API
#      + urlretrieve que download_defaults()), mode_install_recalbox_scripts
#      _console() (wrapper console, entree "9" dans show_advanced_menu()/
#      run_mode()), et une nouvelle phase dans mode_full() (Mode 1) juste
#      apres download_defaults() : resout la cible via detect_recalbox_share
#      () puis prefs.get("recalbox_ip"), execute si trouvee, saute
#      silencieusement sinon (pas de saisie interactive forcee en plein
#      pipeline). Nouvelles cles TRANSLATIONS (parite stricte fr/en/es) :
#      mode9_title/mode9_short_title/mode9_desc/mode9_ip_prompt/
#      mode9_autodetect_ok/mode9_result_ok/mode9_result_fail/
#      mode9_share_unreachable/mode1_scripts_phase/mode1_scripts_skip.
#
# v13 - 2026-07-20 - safe-modify - Audit complet de la traduction FR/EN/ES :
#      ~45 nouvelles cles ajoutees (popup Tkinter de conversion, messages
#      GIF/build_cache/download_defaults/suppression PNG-GIF, copie SD
#      (_copy_to_drive/_copy_specific_files/_robocopy), console Mode 1
#      (check_missing_images_gamelist/check_final_media), labels de
#      progression GUI). Corrige 2 bugs reels trouves pendant l'audit :
#      (1) main_opt2_desc existait deja traduit mais n'etait jamais appele
#      (texte francais en dur affiche dans les 3 langues au menu avance) ;
#      (2) tr('dl_done') etait affiche sans etre appele avec son argument
#      dans mode_extract_download_defaults_only() (affichait la repr Python
#      de la lambda au lieu du texte) -- remplace par mode2_no_other_step/
#      mode2_copy_hint. Restaure aussi def _robocopy() qui avait disparu
#      (son "def" avait ete supprime, laissant le corps en code mort
#      inatteignable dans _copy_specific_files) : le Mode 6 "Copier sur la
#      carte SD" plantait avec NameError des qu'on l'utilisait. Les rapports
#      generes sur disque (mode8_report_*.txt, mode8_final_report_*.csv/.txt)
#      sont desormais en anglais fixe (choix utilisateur) au lieu du
#      francais fixe precedent, independamment de la langue d'interface.
#      Parite des cles verifiee (208/208/208 fr/en/es), compilation +
#      smoke-test des nouvelles cles (47 cles x 3 langues) OK.
#
# v12 - 2026-07-13 - safe-modify - download_defaults() : nouveau parametre
#      overwrite_existing_files=True. Quand un fichier de systems/_defaults/
#      existe deja et que ce parametre est False, le fichier est conserve
#      (message dl_file_skip) au lieu d'etre re-telecharge -- sauf
#      "default.raw565" qui est TOUJOURS re-telecharge/ecrase quel que soit
#      ce parametre (c'est l'image de secours geree par
#      set_default_fallback_image()/_apply_custom_default_fallback() cote
#      GUI, elle doit rester coherente avec le choix utilisateur courant).
#      Suppression du shutil.rmtree(defaults_dir) precedent (qui rendait
#      tout choix "conserver" impossible) : le dossier est desormais cree
#      avec mkdir(exist_ok=True) sans purge prealable. Nouvelle cle de
#      traduction dl_file_skip (fr/en/es).
#
# v11 - 2026-07-11 - safe-modify - Ajout de set_default_fallback_image() :
#      convertit une image (PNG ou autre) vers systems/_defaults/default.raw565,
#      reutilisant convert_png_to_raw565_only() telle quelle (deja compatible :
#      _alpha_subdir_if_needed() exclut "_defaults" du bucketing alphabetique,
#      donc le chemin de sortie n'est jamais modifie). Permet a l'utilisateur
#      de choisir l'image de secours du firmware (cote GUI : nouveau bouton
#      "Image de secours").
#
# v10 - 2026-07-11 - safe-modify - Renommage des identifiants de profil
#      RECALBOX_PROFILES : "10.1" -> "10.x", "9.2" -> "9.x" (plus generique,
#      couvre les sous-versions). "legacy" inchange. Repercute cote
#      RecalBoxDMD_GUI.py (combobox Mode 1/3/8, traductions, valeurs par
#      defaut) et RecalBoxDMD_prefs.py.
# v9 - 2026-07-10 - safe-modify - Mode 1 : gestion des versions Recalbox pour
#      le scrape marquee/logo. Profil "9.x" corrige (tag "thumbnail" au lieu
#      de "image" -- recommandation : la marquee doit etre scrapee dans le
#      champ "Selectionnez le type de vignette", pas "type d'image", pour
#      eviter le conflit avec un autre visuel). img_subdir corrige pour
#      "9.x"/"legacy" : "media/thumbnails"/"media/images" (prefixe "media/"
#      confirme par captures d'ecran reelles fournies par l'utilisateur,
#      coherent avec "media/wheels" du profil 10.x). Ajout
#      list_scrape_media_files() et clean_scrape_media_folders() :
#      nettoyage (avec apercu prealable) des dossiers media sur le partage
#      ROMs reel, systeme par systeme, avant un nouveau scrape Recalbox.
# v8 - 2026-07-10 - safe-modify - Reprise apres interruption de la copie SD
#      (_copy_to_drive) : ecriture atomique via fichier .part + os.replace()
#      (empeche un fichier corrompu/partiel d'etre pris pour "deja copie"
#      apres un crash/debranchement), skip par comparaison de taille au
#      lieu de la seule existence, arret precoce si la destination
#      disparait en cours de copie, manifest JSON persiste
#      (_copy_progress.json) pour detecter une copie interrompue au
#      prochain lancement. Ajout _copy_specific_files() pour un retry
#      cible sur les fichiers en echec (extrait _copy_one_file() en
#      fonction partagee)
# v7 - 2026-07-10 - safe-modify - check_final_media() : si le sous-dossier
#      d'un systeme (3do, amiga600...) est totalement absent d'un cote
#      (dossier temporaire OU SD), ce cote est ignore pour ce systeme
#      (verification basee sur l'autre cote uniquement) au lieu de
#      signaler chaque jeu comme "absent". Si absent des deux cotes,
#      compte a part dans "not_checked" (aucun motif affiche)
# v6 - 2026-07-10 - safe-modify - check_missing_images_gamelist() et
#      check_final_media() appellent desormais progress_cb() (kinds
#      "mode8_check"/"final_media") avec une progression globale reelle
#      (jeux traites/total), corrige la barre de progression figee pendant
#      le Mode 8
# v1 -
# v2 - 2026-07-08 - safe-modify - Ajout Mode 8 2026-07-08 - Version de base
# v3 - 2026-07-10 - safe-modify - check_missing_images_gamelist : utilise
#      resolve_image_path() (chemin exact du gamelist.xml) au lieu du
#      matching fuzzy par nom nettoye (hash/parentheses), source des
#      erreurs de reconnaissance en mode 8
# v4 - 2026-07-10 - safe-modify - _alpha_subdir_if_needed() idempotent :
#      corrige un bug ou convert_png_to_raw565_only()/
#      convert_gif_to_raw565pack_meta() re-imbriquaient un 2e sous-dossier
#      alphabetique (ex: systems/snes/K/K/nom.raw565) sur un fichier deja
#      bucketise, rendant le fichier invisible pour le firmware (qui ne
#      teste que systems/<sys>/<Lettre>/nom.raw565 puis le chemin plat)
# v5 - 2026-07-10 - safe-modify - Ajout check_final_media() +
#      generate_final_media_report() : compare le rapport Mode 8 avec le
#      dossier temporaire (et en option la SD physique) pour distinguer
#      images manquantes cote ROMs / non converties / non copiees sur SD
# ============================================

#!/usr/bin/env python3
"""
recalbox_toolkit.py — Unified tool for ESP32 Marquee
=====================================================
1. Gamelist extraction + 128x32 conversion + build cache
2. Gamelist extraction only
3. 128x32 conversion only
4. Build games_cache only
"""

# ─────────────────────────────────────────────────────────────────────────────
# CHANGELOG / MODIFS (GUI + docs) — RetroBoxLED_tool3.3
#
# Contexte : ces notes documentent les changements appliqués sur le toolkit
# (et l’interface GUI associée) dans le dossier retroboxled_tools3.3.
# ─────────────────────────────────────────────────────────────────────────────
# 1) Mode 2 (GUI) : comportement “download-only”
#    - Mode 2 ne doit plus déclencher extraction/conversion/sélection systèmes.
#    - La pipeline GUI passe désormais selected_systems = None pour Mode 2.
#    - L’autodétection/zone “systèmes” n’est plus visible en Mode 2.
#
# 2) Mode 2 (GUI) : titres / descriptifs harmonisés
#    - Mise à jour du titre Mode 2 (FR/EN/ES) pour refléter “systems/_defaults via GitHub uniquement”.
#    - Mise à jour des textes “Détail du mode” en langage courant.
#
# 3) Mode 1 (GUI) : correction sélection manuelle
#    - Bug corrigé : la sélection manuelle d’1 seul système n’influençait pas le traitement.
#    - Fix : le pipeline Mode 1 utilise désormais cfg.systems_selected (au lieu d’écraser par None).
#
# 4) Mode 7 / Copie SD (GUI) : barre de progression supportée
#    - Bug corrigé : la barre de progression ne bougeait pas pendant la copie robocopy.
#    - Fix : Mode 7 repasse la ProgressBar en mode indeterminate pendant robocopy, puis à 100% en fin.
#
# 5) Aide GUI (README)
#    - README.fr.md réécrit pour coller aux fonctionnalités du script tool3.3 (modes + fichiers générés).
#    - Traductions EN/ES ajoutées/sauvegardées (README.md, README.es.md) avec le même contenu adapté.
#
# 6) Cache unifié (v3.3.1)
#    - build_cache() remplace collect_games_for_folder : scan récursif rglob unique, produit un seul
#      games_cache.bin au lieu de multiples fichiers par lettre.
#    - _calc_bigram_idx() : calcul d'index bigramme compatible NB_IDX=703 (format ESP32).
#    - EXTENSIONS_CACHE étendu à .raw565 et .raw565pack pour que le cache trouve les fichiers
#      après conversion PNG→raw565 et GIF→raw565pack.
#
# 7) Pillow DeprecationWarning (v3.3.1)
#    - Trois appels à rgb_img.getdata() remplacés par rgb_img.tobytes() avec itération sur les
#      triplets RGB, supprimant le warning DeprecationWarning de Pillow.
#
# NOTE :
# - Les modifications ci-dessus impactent principalement RetroBoxLED_gui.py
#   (et les fichiers README *.md). Ce changelog résume l’historique pour RetroBoxLED_tool3.3.py.
# ─────────────────────────────────────────────────────────────────────────────

import os
import re
import sys
import shutil
import struct
import time
import threading
import tempfile
import json
import filecmp
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from typing import Optional
from urllib.parse import unquote

import RecalBoxDMD_prefs as prefs

# ─────────────────────────────────────────────────────────────────────────────
#  PAUSE CONTROLLER
# ─────────────────────────────────────────────────────────────────────────────


class PauseController:
    """
    Écoute la touche P en arrière-plan pendant un traitement.
    États : RUNNING / PAUSED / SKIP / STOP
    """

    RUNNING = "running"
    PAUSED = "paused"
    SKIP = "skip"
    STOP = "stop"

    def __init__(self):
        self.state = self.RUNNING
        self._lock = threading.Lock()
        self._thread = None
        self._active = False

    def start(self, listen_keyboard: bool = False):
        self.state = self.RUNNING
        self._active = True
        self._thread = None
        # Le thread clavier console a été retiré — la GUI pilote pause/stop.
        pass

    def stop(self):
        self._active = False

    def request_pause(self):
        with self._lock:
            if self.state == self.RUNNING:
                self.state = self.PAUSED

    def request_resume(self):
        with self._lock:
            if self.state in (self.PAUSED, self.SKIP):
                self.state = self.RUNNING

    def request_skip(self):
        with self._lock:
            if self.state in (self.RUNNING, self.PAUSED):
                self.state = self.SKIP

    def request_stop(self):
        with self._lock:
            if self.state in (self.RUNNING, self.PAUSED, self.SKIP):
                self.state = self.STOP

    def is_running(self):
        with self._lock:
            return self.state == self.RUNNING

    def should_skip(self):
        with self._lock:
            return self.state == self.SKIP

    def should_stop(self):
        with self._lock:
            return self.state == self.STOP

    def wait_if_paused(self):
        """Attend tant que l'état est PAUSED (bloque le thread principal)."""
        while True:
            with self._lock:
                if self.state != self.PAUSED:
                    break
            time.sleep(0.1)


# Global pause controller
PAUSE = PauseController()

# ─────────────────────────────────────────────────────────────────────────────
#  TRANSLATIONS
# ─────────────────────────────────────────────────────────────────────────────

TRANSLATIONS = {
    "fr": {
        "pillow_installing": "⚙️  Pillow n'est pas installé. Installation automatique en cours...",
        "pillow_ok": "✅ Pillow installé avec succès !\n",
        "pillow_fail": "❌ Impossible d'installer Pillow.\n   Lance manuellement : pip install Pillow\n   La conversion 128x32 sera désactivée.\n",
        "main_title": "RetroBoxLED Toolkit for Recalbox",
        "dl_title": "🌐  DOSSIER _defaults (images systèmes)",
        "dl_missing": "   ℹ️  Aucun dossier _defaults/ trouvé dans sd_card/systems/.",
        "dl_exists": "   ℹ️  Le dossier _defaults/ existe déjà dans sd_card/systems/.",
        "dl_ask_download": "Télécharger _defaults/ depuis GitHub (RetroBoxLED) ?",
        "dl_ask_update": "Mettre à jour _defaults/ depuis GitHub (RetroBoxLED) ?",
        "dl_skip": "   ⏭️  Téléchargement ignoré.",
        "dl_starting": "⬇️  Téléchargement des fichiers depuis GitHub...",
        "dl_file_ok": lambda n, i, t: f"   {i:4d}/{t} ✅ {n}",
        "dl_file_err": lambda n, e: f"   ⚠️  {n} — {e}",
        "dl_file_skip": lambda n, i, t: f"   {i:4d}/{t} ⏭️  {n} (deja present, conserve)",
        "dl_done": lambda n: f"✅ {n} fichiers téléchargés dans _defaults/",
        "dl_fail_api": "❌ API GitHub inaccessible. Vérifiez votre connexion internet.",
        "github_rate_limit_msg": lambda reset_time: (
            f"⏳ Limite de requêtes GitHub atteinte (quota horaire de l'API, "
            f"60 requêtes/heure sans connexion). Réessayez après {reset_time}."
        ),
        "gifpack_title": "📦 Téléchargement du pack gratuit de GIFs...",
        "gifpack_fail_api": lambda e: f"❌ API GitHub inaccessible ({e}). Vérifiez votre connexion internet.",
        "gifpack_fail_subfolder": lambda sub, e: f"   ⚠️  Sous-dossier '{sub}' ignoré ({e})",
        "gifpack_starting": lambda total, nsub: f"   {total} GIFs trouvés dans {nsub} sous-dossier(s)",
        "gifpack_file_ok": lambda n, i, t: f"   {i:4d}/{t} ✅ {n}",
        "gifpack_file_err": lambda n, e: f"   ⚠️  {n} — {e}",
        "gifpack_done": lambda n: f"✅ {n} GIFs téléchargés",
        "gifpack_playlist_ok": "✅ Playlist vitrine 'RpiTeaM eLLuiGi.txt' téléchargée et sélectionnée par défaut",
        "gifpack_playlist_fail": lambda e: f"⚠️  Échec téléchargement playlist vitrine ({e}) — repli sur ALL.txt",
        "all_playlist_created": lambda n: f"✅ Playlist 'ALL.txt' créée ({n} dossier(s), tous les GIFs de la carte)",
        "all_playlist_skipped_empty": "ℹ️  Aucun GIF sur la carte — playlist 'ALL.txt' non créée, aucune playlist par défaut",
        "dl_replacing": "🗑️  Remplacement du _defaults/ existant...",
        "main_prompt": "Que voulez-vous faire ?",
        "main_opt1": "Extraction gamelist + Conversion 128x32 + Build cache  (TOUT)",
        "main_opt2": "Seulement extraire les images des gamelists",
        "main_opt3": "Seulement convertir des images en 128x32",
        "main_opt4": "Seulement construire le games_cache.bin",
        "main_opt7": "Interface graphique (Tkinter)",
        "mode9_title": "MODE 9 — Installer les scripts Recalbox",
        "mode9_short_title": "Installer les scripts Recalbox",
        "mode9_desc": "Installe/met à jour les scripts utilisateur Recalbox (WiFi Recovery, Config Web, pont marquee) directement sur le partage réseau de la Recalbox.",
        "mode9_ip_prompt": "Adresse IP ou nom réseau de la Recalbox",
        "mode9_autodetect_ok": lambda host: f"✅ Recalbox détectée automatiquement : {host}",
        "mode9_result_ok": lambda n: f"   ✅ {n}",
        "mode9_result_fail": lambda n, e: f"   ❌ {n} — {e}",
        "mode9_share_unreachable": lambda host: f"❌ Partage réseau introuvable ({host or 'aucune cible'}) — vérifie que la Recalbox est allumée et joignable.",
        "mode9_guest_blocked": "❌ Accès invité SMB bloqué par la stratégie de sécurité Windows (pas un problème réseau/IP) — active \"Autoriser les connexions invité non sécurisées\" : PowerShell en Administrateur -> Set-SmbClientConfiguration -EnableInsecureGuestLogons $true -Force, puis réessaie.",
        "mode9_smb_fallback_ssh": "⚠️  Partage SMB injoignable — tentative de repli via SSH (identifiants Recalbox par défaut)...",
        "paramiko_installing": "⚙️  paramiko n'est pas installé (nécessaire pour le repli SSH). Installation automatique en cours...",
        "paramiko_ok": "✅ paramiko installé avec succès !\n",
        "paramiko_fail": "❌ Impossible d'installer paramiko.\n   Lance manuellement : pip install paramiko\n   Le repli SSH sera désactivé.\n",
        "ssh_connect_fail": lambda host, e: f"❌ Connexion SSH impossible ({host}) — {e}",
        "mode1_scripts_staged": "\n📥  Préparation locale des scripts Recalbox (dossier temporaire)...",
        "mode1_scripts_phase": "\n📂  Installation des scripts Recalbox...",
        "mode1_scripts_skip": "\n⏭️  Recalbox non confirmée — scripts non installés automatiquement (copie manuelle possible depuis le dossier temporaire, ou Mode 9 plus tard).",
        "mode1_scripts_installed_via": lambda m: f"   ℹ️  Installés via {'SMB' if m == 'smb' else 'SSH (repli)'}.",
        "main_choice": "Votre choix (1-9) : ",
        "main_opt_quit": "QUITTER",
        "main_warn": "⚠️  Tape un chiffre entre 0 et 9.\n",
        "back": "↩  Retour en arrière",
        "back_main": "\n  ↩  Retour au menu principal...",
        "back_roms": "\n  ↩  Retour au choix du dossier roms...",
        "yes_no": "(o/n)",
        "yes_vals": ("o", "oui", "y", "yes"),
        "no_vals": ("n", "non", "no"),
        "warn_yn": "⚠️  Tape o ou n.\n",
        "warn_choice": "⚠️  Tape 0, 1 ou 2.\n",
        "after_menu": "Que voulez-vous faire ensuite ?",
        "after_opt1": "Retour au menu principal",
        "after_opt6": "Copier sur la carte SD maintenant  (mode 8)",
        "after_opt_files": "Copier les fichiers générés sur la carte SD",
        "press_enter": "Appuie sur Entrée pour fermer...",
        "press_enter_cont": "Appuie sur Entrée pour continuer quand même...",
        "path_local": "  1  →  Lecteur local  (ex: D:\\Recalbox\\roms)",
        "path_network": "  2  →  Réseau / NAS   (ex: \\\\192.168.1.1\\share\\roms)",
        "path_choice": "Votre choix (0, 1 ou 2) : ",
        "path_local_lbl": "Chemin du dossier  (0 pour revenir) : ",
        "path_net_lbl": "Chemin réseau      (0 pour revenir) : ",
        "path_not_found": "❌ Dossier introuvable. Vérifie le chemin et réessaie.\n",
        "sd_erase_ask": "Voulez-vous l'effacer complètement avant de continuer ?",
        "sd_erased": "🗑️  Dossier effacé et recréé.",
        "sd_erase_err": "❌ Erreur lors de l'effacement. Ferme les fichiers ouverts dans sd_card/ et réessaie.",
        "sd_kept": "   ℹ️  Contenu conservé, les fichiers existants seront écrasés ou ignorés.",
        "ext_title": "🚀 ÉTAPE 1/3 — EXTRACTION DES IMAGES",
        "ext_roms_where": "\n📍 OÙ SE TROUVENT VOS ROMS ?",
        "ext_systems": "systèmes avec gamelist.xml détectés",
        "ext_games_found": "jeux trouvés",
        "ext_xml_err": "ERREUR XML",
        "ext_already": "déjà présente",
        "ext_missing_tag": "MANQUANT",
        "ext_summary_copied": "copiées",
        "ext_summary_skip": "déjà présentes",
        "ext_summary_miss": "manquantes",
        "ext_log_header": "=== IMAGES MANQUANTES ===\n",
        "ext_log_source": "Source : ",
        "ext_log_summary": "=== RÉSUMÉ ===\n",
        "ext_log_games": "Jeux parcourus    : ",
        "ext_log_copied": "Images copiées    : ",
        "ext_log_skipped": "Déjà présentes    : ",
        "ext_log_missing": "Images manquantes : ",
        "conv_title": "🖼️  ÉTAPE 2/3 — CONVERSION 128x32",
        "conv_png_only": "\n⚠️  INFO : Seuls les fichiers PNG seront convertis en 128x32.\n          Les GIF sont conservés tels quels.\n",
        "conv_gif_info": "GIF trouvés → convertis en raw565pack/meta.",
        "conv_png_count": "PNG à convertir en 128x32",
        "conv_summary_done": "PNG convertis",
        "conv_summary_err": "erreurs",
        "conv_summary_gif": "GIF raw565pack/meta",
        "conv_no_pillow": "❌ Pillow n'est pas installé. Installe-le avec : pip install Pillow",
        "conv_src_where": "\n📍 OÙ SE TROUVENT LES IMAGES À CONVERTIR ?\n   (dossier systems/ contenant les sous-dossiers par système)",
        "cache_title": "💾 ÉTAPE 3/3 — BUILD GAMES CACHE",
        "cache_scan": "[INFO] Scan de : ",
        "cache_found_sys": "systèmes,",
        "cache_found_games": "jeux trouvés",
        "cache_no_sys": "⚠️  Aucun système trouvé. Vérifiez le dossier systems/.",
        "cache_size": "Ko",
        "cache_sys_where": "\n📍 OÙ SE TROUVE LE DOSSIER SYSTEMS ?",
        "cache_sys_detected": "✅ Dossier systems/ détecté : ",
        "cache_sys_use": "Utiliser ce dossier ?",
        "cache_sys_missing": "⚠️  Aucun dossier systems/ trouvé dans ",
        "mode1_title": "MODE 1 (AUTO) — Extraction images depuis Recalbox + Conversion raw 128x32 + Cache + copie vers la SD",
        "mode2_title": "MODE 2 — Téléchargement systems/_defaults via GitHub uniquement",
        "mode3_title": "MODE 3 — Extraction des images depuis Gamelist.xml uniquement (dossier ROMS)",
        "mode4_title": "MODE 4 — Conversion PNG→raw565 + GIF→raw565pack/meta depuis un dossier spécifié",
        "mode5_title": "MODE 5 — Conversion en 128x32 uniquement",
        "mode6_title": "MODE 6 — Génération de games_cache.bin uniquement",
        "mode7_title": "MODE 7 — Génération de systems_cache.dat uniquement",
        "done": "🎉 TERMINÉ !",
        "done_sd": "📂 Dossier SD card       : ",
        "done_cache": "💾 Cache                 : ",
        "done_log": "📋 Log images manquantes : ",
        "done_copy_sd": "Copiez le contenu de ce dossier à la racine de votre carte SD.",
        "done_extracted": "📂 Images extraites dans : ",
        "done_log2": "📋 Log : ",
        "done_converted": "📂 Images converties dans : ",
        "done_cache2": "💾 Cache généré : ",
        "done_copy_cache": "Copiez ces fichiers à la racine de votre carte SD.",
        "done_cache_files": "💾 Fichiers générés :",
        "src_ok": "✅ Dossier : ",
        "roms_ok": "✅ Dossier ROMs : ",
        "sysc_title": "MODE 7 — Génération de systems_cache.dat",
        "sysc_no_defaults": "⚠️  Aucun dossier _defaults/ trouvé dans systems/.",
        "sysc_found": lambda n: f"   📂 {n} systèmes trouvés dans _defaults/",
        "sysc_line": lambda t, n: f"   {'✅' if t != '?' else '⚠️ '} {t}  {n}",
        "sysc_unknown": "⚠️  Pas de .gif ni .png trouvé pour ce système.",
        "sysc_done": lambda n, p: f"✅ {n} systèmes écrits dans {p}",
        "sysc_copy": "Copiez systems_cache.dat à la racine de votre carte SD.",
        "sysc_hint": "   (L'ESP32 l'utilisera au prochain démarrage sans rescanner)",
        "flash_title": "MODE 8 — Copier sur la carte SD",
        "flash_no_sdcard": "⚠️  Le dossier sd_card/ est vide ou absent. Lancez un autre mode d'abord.",
        "flash_no_win": "⚠️  Ce mode est uniquement disponible sur Windows.",
        "flash_admin_warn": "⚠️  Droits administrateur requis. Relancez le script en tant qu'Administrateur.",
        "flash_drives_title": "\n💾  LECTEURS DISPONIBLES (amovibles / carte SD) :",
        "flash_no_drives": "⚠️  Aucun lecteur amovible détecté. Insérez votre carte SD et réessayez.",
        "flash_drive_choice": "Choisissez le lecteur de destination (0 pour revenir) : ",
        "flash_drive_warn": "⚠️  Choix invalide.\n",
        "flash_drive_sel": lambda d, s: f"\n✅ Destination : {d}  ({s})",
        "flash_mode_title": "\n⚙️  MODE DE COPIE",
        "flash_mode_opt1": "Formater en FAT32 puis copier  (ATTENTION : efface tout sur la SD)",
        "flash_mode_opt2": "Copier uniquement — écraser les fichiers existants",
        "flash_mode_opt3": "Copier uniquement — ignorer les fichiers existants (garder ce qui est déjà là)",
        "flash_mode_choice": "Votre choix (0-2) : ",
        "flash_mode_warn": "⚠️  Tape 0, 1 ou 2.\n",
        "flash_fmt_confirm": lambda d: f"⚠️  TOUTES LES DONNÉES sur {d} seront effacées. Êtes-vous sûr ?",
        "flash_fmt_abort": "   ↩  Formatage annulé.",
        "flash_fmt_start": lambda d: f"🗑️  Formatage de {d} en FAT32...",
        "flash_fmt_ok": "✅ Formatage terminé.",
        "flash_fmt_err": lambda e: f"❌ Erreur de formatage : {e}",
        "flash_copy_start": lambda s, d: f"\n📋 Copie de {s} → {d} (robocopy /MT:32)...",
        "flash_copy_ok": "✅ Copie terminée.",
        "flash_copy_err": lambda c, copied, total: f"⚠️ robocopy retour {c} — {copied}/{total} copiés (vérifiez la sortie ci-dessus)",
        "main_opt6": "Copier sd_card/ sur la carte SD  (rapide, robocopy)",
        "main_opt5": "Générer systems_cache.dat  (index systèmes ESP32)",
        "pause_hint": "   ⏸️  Appuie sur [ESC] pour mettre en pause",
        "pause_title": "\n⏸️  PAUSE",
        "pause_opt1": "Continuer",
        "pause_opt2": "Passer à l'étape suivante",
        "pause_opt3": "Arrêter le script",
        "pause_choice": "Votre choix (1-3) : ",
        "pause_warn": "⚠️  Tape 1, 2 ou 3.\n",
        "pause_resuming": "▶️  Reprise...",
        "pause_skipping": "⏭️  Passage à l'étape suivante...",
        "pause_stopping": "🛑  Arrêt demandé.",
        "sys_sel_title": "🎮  SYSTÈMES DÉTECTÉS",
        "sys_sel_none": "⚠️  Aucun système avec gamelist.xml trouvé dans ce dossier.",
        "sys_sel_prompt": "Quels systèmes traiter ?",
        "sys_sel_opt_all": "Tous les systèmes",
        "sys_sel_opt_pick": "Choisir les systèmes à traiter",
        "sys_sel_pick_hint": "Entrez les numéros séparés par des virgules (ex: 1,3,5) ou 0 pour tout sélectionner :",
        "sys_sel_warn": "⚠️  Sélection invalide. Réessayez.\n",
        "sys_sel_selected": lambda n: f"✅ {n} système(s) sélectionné(s).",
        "main_opt_advanced": "Menu avancé  (modes 2 à 8)",
        "advanced_title": "MODES AVANCÉS",
        "advanced_prompt": "Choisissez un mode avancé :",
        "advanced_back": "Retour au menu principal",
        "main_opt2_desc": "Extraction + téléchargement _defaults depuis GitHub (uniquement)",
        "popup_conv_title": "RetroBoxLED — Conversion 128x32",
        "popup_conv_msg": lambda out_dir: f"Préparation conversion 128x32.\n\nSortie :\n{out_dir}",
        "popup_conv_explore": "Explorer dossier",
        "popup_conv_continue": "Continuer",
        "conv_gif_converted": lambda n: f"\n   🎞️  Conversion GIF -> raw565pack/meta : {n} fichiers",
        "cache_build_header": lambda ts, tf: f"\n--- games_cache.bin ({ts} systèmes, {tf} jeux) ---",
        "cache_build_done": lambda name, ns, ng: f"   {name}  ({ns} systèmes, {ng} jeux)",
        "mode1_dl_phase": "\n⬇️  Téléchargement _defaults depuis GitHub...",
        "mode1_removed_files": lambda n: f"\n🗑️  {n} fichiers .png/.gif supprimés (remplacés par raw565)",
        "mode1_removed_errors": lambda n: f"⚠️  {n} fichiers .png/.gif n'ont pas pu être supprimés",
        "mode1_conv_errors": lambda n: f"\n⚠️  {n} erreur(s) de conversion (voir détails ci-dessus)",
        "mode2_no_other_step": "ℹ️  (aucune autre étape n'est exécutée)",
        "mode2_copy_hint": "↩  Vous pouvez ensuite copier sd_card/ sur la carte SD.",
        "copy_files_total": lambda n, dst: f"   📊 {n} fichiers à copier vers {dst}",
        "copy_files_from": lambda n, src: f"   📊 {n} fichiers à copier depuis {src}",
        "copy_sd_unreachable": lambda copied, total: f"   ❌ CARTE SD INACCESSIBLE — copie arrêtée à {copied}/{total} fichiers",
        "copy_file_error": lambda rel, detail: f"   ❌ ERREUR: {rel} — {detail}",
        "copy_progress_line": lambda icon, copied, total, rel, tag: f"   {icon} {copied}/{total} — {rel} ({tag})",
        "copy_file_progress": lambda copied, total, line: f"   📄 Copie {copied}/{total} : {line}",
        "copy_tag_skipped": "ignoré",
        "copy_tag_ok": "OK",
        "copy_interrupted": lambda copied, total: f"   ⚠️  Copie interrompue : {copied}/{total} fichiers copiés",
        "copy_done_with_failed": lambda copied, total, nfailed: f"   ⚠️  Copie terminée : {copied}/{total} fichiers OK, {nfailed} en échec",
        "copy_done_all": lambda copied, total: f"   ✅ Copie terminée : {copied}/{total} fichiers",
        "copy_status_interrupted": "Interrompu",
        "copy_status_done": "Terminé",
        "retry_total": lambda n, dst: f"   📊 {n} fichier(s) en échec à réessayer vers {dst}",
        "retry_sd_unreachable": "   ❌ CARTE SD INACCESSIBLE — retry arrêté",
        "retry_file_ok": lambda i, total, relp: f"   ✅ {i}/{total} — {relp}",
        "retry_done_with_failed": lambda copied, total, nfailed: f"   ⚠️  Retry terminé : {copied}/{total} OK, {nfailed} en échec",
        "retry_done_all": lambda copied, total: f"   ✅ Retry terminé : {copied}/{total} fichiers",
        "m1_no_systems": "   Aucun système avec gamelist.xml trouvé.",
        "m1_systems_detected": lambda n: f"   {n} systèmes détectés.",
        "m1_tag_xml": lambda tag: f"   Tag XML : <{tag}>",
        "m1_profile": lambda desc: f"   Profil : {desc}",
        "m1_analyzing": lambda sys_name: f"   [{sys_name}] Analyse en cours...",
        "m1_xml_error": lambda e: f"      Erreur XML : {e}",
        "m1_xml_error_sys": lambda sys_name, e: f"      Erreur XML [{sys_name}] : {e}",
        "m1_games_summary": lambda games, present, missing: f"      {games} jeux, {present} présentes, {missing} manquantes",
        "m1_no_games": "      Aucun jeu trouvé",
        "m1_more_others": lambda n: f"         ... et {n} autre(s)",
        "m1_result_summary": lambda games, present, missing: f"RESULTAT : {games} jeux scannés, {present} images présentes, {missing} manquantes",
        "cache_file_line": lambda path, ns, ng: f"   💾 {path}  ({ns} systèmes, {ng} jeux)",
        "progress_conv_idle": "aucun PNG ni GIF à convertir",
        "progress_conv_running": "conversion en cours",
        "progress_conv_raw_running": "raw conversion en cours",
    },
    "en": {
        "pillow_installing": "⚙️  Pillow is not installed. Installing automatically...",
        "pillow_ok": "✅ Pillow installed successfully!\n",
        "pillow_fail": "❌ Could not install Pillow.\n   Run manually: pip install Pillow\n   128x32 conversion will be disabled.\n",
        "main_title": "RetroBoxLED Toolkit for Recalbox",
        "dl_title": "🌐  _defaults FOLDER (system images)",
        "dl_missing": "   ℹ️  No _defaults/ folder found in sd_card/systems/.",
        "dl_exists": "   ℹ️  _defaults/ already exists in sd_card/systems/.",
        "dl_ask_download": "Download _defaults/ from GitHub (RetroBoxLED)?",
        "dl_ask_update": "Update _defaults/ from GitHub (RetroBoxLED)?",
        "dl_skip": "   ⏭️  Download skipped.",
        "dl_starting": "⬇️  Downloading files from GitHub...",
        "dl_file_ok": lambda n, i, t: f"   {i:4d}/{t} ✅ {n}",
        "dl_file_err": lambda n, e: f"   ⚠️  {n} — {e}",
        "dl_file_skip": lambda n, i, t: f"   {i:4d}/{t} ⏭️  {n} (already present, kept)",
        "dl_done": lambda n: f"✅ {n} files downloaded into _defaults/",
        "dl_fail_api": "❌ GitHub API unreachable. Check your internet connection.",
        "github_rate_limit_msg": lambda reset_time: (
            f"⏳ GitHub API rate limit reached (hourly quota, 60 requests/hour "
            f"unauthenticated). Try again after {reset_time}."
        ),
        "gifpack_title": "📦 Downloading the free GIF pack...",
        "gifpack_fail_api": lambda e: f"❌ GitHub API unreachable ({e}). Check your internet connection.",
        "gifpack_fail_subfolder": lambda sub, e: f"   ⚠️  Subfolder '{sub}' skipped ({e})",
        "gifpack_starting": lambda total, nsub: f"   {total} GIFs found in {nsub} subfolder(s)",
        "gifpack_file_ok": lambda n, i, t: f"   {i:4d}/{t} ✅ {n}",
        "gifpack_file_err": lambda n, e: f"   ⚠️  {n} — {e}",
        "gifpack_done": lambda n: f"✅ {n} GIFs downloaded",
        "gifpack_playlist_ok": "✅ Showcase playlist 'RpiTeaM eLLuiGi.txt' downloaded and set as default",
        "gifpack_playlist_fail": lambda e: f"⚠️  Failed to download showcase playlist ({e}) — falling back to ALL.txt",
        "all_playlist_created": lambda n: f"✅ 'ALL.txt' playlist created ({n} folder(s), every GIF on the card)",
        "all_playlist_skipped_empty": "ℹ️  No GIFs on the card — 'ALL.txt' playlist not created, no default playlist",
        "dl_replacing": "🗑️  Replacing existing _defaults/...",
        "main_prompt": "What do you want to do?",
        "main_opt1": "Gamelist extraction + 128x32 conversion + Build cache  (ALL)",
        "main_opt2": "Gamelist image extraction only",
        "main_opt3": "128x32 conversion only",
        "main_opt4": "Build games_cache.bin only",
        "main_opt7": "Graphical interface (Tkinter)",
        "mode9_title": "MODE 9 — Install Recalbox scripts",
        "mode9_short_title": "Install Recalbox scripts",
        "mode9_desc": "Installs/updates the Recalbox user scripts (WiFi Recovery, Web Config, marquee bridge) directly on the Recalbox network share.",
        "mode9_ip_prompt": "Recalbox IP address or network name",
        "mode9_autodetect_ok": lambda host: f"✅ Recalbox auto-detected: {host}",
        "mode9_result_ok": lambda n: f"   ✅ {n}",
        "mode9_result_fail": lambda n, e: f"   ❌ {n} — {e}",
        "mode9_share_unreachable": lambda host: f"❌ Network share not found ({host or 'no target'}) — check that the Recalbox is powered on and reachable.",
        "mode9_guest_blocked": "❌ SMB guest access blocked by Windows security policy (not a network/IP problem) — enable \"insecure guest logons\": PowerShell as Administrator -> Set-SmbClientConfiguration -EnableInsecureGuestLogons $true -Force, then try again.",
        "mode9_smb_fallback_ssh": "⚠️  SMB share unreachable — trying SSH fallback (default Recalbox credentials)...",
        "paramiko_installing": "⚙️  paramiko is not installed (required for the SSH fallback). Installing automatically...",
        "paramiko_ok": "✅ paramiko installed successfully!\n",
        "paramiko_fail": "❌ Could not install paramiko.\n   Run manually: pip install paramiko\n   The SSH fallback will be disabled.\n",
        "ssh_connect_fail": lambda host, e: f"❌ SSH connection failed ({host}) — {e}",
        "mode1_scripts_staged": "\n📥  Staging Recalbox scripts locally (temp folder)...",
        "mode1_scripts_phase": "\n📂  Installing Recalbox scripts...",
        "mode1_scripts_skip": "\n⏭️  Recalbox not confirmed — scripts not installed automatically (manual copy possible from the temp folder, or Mode 9 later).",
        "mode1_scripts_installed_via": lambda m: f"   ℹ️  Installed via {'SMB' if m == 'smb' else 'SSH (fallback)'}.",
        "main_choice": "Your choice (1-9): ",
        "main_opt_quit": "QUIT",
        "main_warn": "⚠️  Enter a number between 0 and 9.\n",
        "back": "↩  Go back",
        "back_main": "\n  ↩  Back to main menu...",
        "back_roms": "\n  ↩  Back to ROMs folder selection...",
        "yes_no": "(y/n)",
        "yes_vals": ("y", "yes", "o", "oui"),
        "no_vals": ("n", "no", "non"),
        "warn_yn": "⚠️  Type y or n.\n",
        "warn_choice": "⚠️  Type 0, 1 or 2.\n",
        "after_menu": "What do you want to do next?",
        "after_opt1": "Back to main menu",
        "after_opt6": "Copy to SD card now  (mode 8)",
        "after_opt_files": "Copy generated files to SD card",
        "press_enter": "Press Enter to close...",
        "press_enter_cont": "Press Enter to continue anyway...",
        "path_local": "  1  →  Local drive  (e.g.: D:\\Recalbox\\roms)",
        "path_network": "  2  →  Network / NAS  (e.g.: \\\\192.168.1.1\\share\\roms)",
        "path_choice": "Your choice (0, 1 or 2): ",
        "path_local_lbl": "Folder path  (0 to go back): ",
        "path_net_lbl": "Network path (0 to go back): ",
        "path_not_found": "❌ Folder not found. Check the path and try again.\n",
        "sd_erase_ask": "Do you want to completely erase it before continuing?",
        "sd_erased": "🗑️  Folder erased and recreated.",
        "sd_erase_err": "❌ Error while erasing. Close any open files in sd_card/ and try again.",
        "sd_kept": "   ℹ️  Content kept, existing files will be overwritten or skipped.",
        "ext_title": "🚀 STEP 1/3 — IMAGE EXTRACTION",
        "ext_roms_where": "\n📍 WHERE ARE YOUR ROMS?",
        "ext_systems": "systems with gamelist.xml detected",
        "ext_games_found": "games found",
        "ext_xml_err": "XML ERROR",
        "ext_already": "already exists",
        "ext_missing_tag": "MISSING",
        "ext_summary_copied": "copied",
        "ext_summary_skip": "already present",
        "ext_summary_miss": "missing",
        "ext_log_header": "=== MISSING IMAGES ===\n",
        "ext_log_source": "Source: ",
        "ext_log_summary": "=== SUMMARY ===\n",
        "ext_log_games": "Games scanned   : ",
        "ext_log_copied": "Images copied   : ",
        "ext_log_skipped": "Already present : ",
        "ext_log_missing": "Missing images  : ",
        "conv_title": "🖼️  STEP 2/3 — 128x32 CONVERSION",
        "conv_png_only": "\n⚠️  INFO: Only PNG files will be converted to 128x32.\n          GIFs are kept as-is.\n",
        "conv_gif_info": "GIF found → converted to raw565pack/meta.",
        "conv_png_count": "PNG to convert to 128x32",
        "conv_summary_done": "PNG converted",
        "conv_summary_err": "errors",
        "conv_summary_gif": "GIF raw565pack/meta",
        "conv_no_pillow": "❌ Pillow is not installed. Install it with: pip install Pillow",
        "conv_src_where": "\n📍 WHERE ARE THE IMAGES TO CONVERT?\n   (systems/ folder containing subfolders per system)",
        "cache_title": "💾 STEP 3/3 — BUILD GAMES CACHE",
        "cache_scan": "[INFO] Scanning: ",
        "cache_found_sys": "systems,",
        "cache_found_games": "games found",
        "cache_no_sys": "⚠️  No systems found. Check the systems/ folder.",
        "cache_size": "KB",
        "cache_sys_where": "\n📍 WHERE IS THE SYSTEMS FOLDER?",
        "cache_sys_detected": "✅ systems/ folder detected: ",
        "cache_sys_use": "Use this folder?",
        "cache_sys_missing": "⚠️  No systems/ folder found in ",
        "mode1_title": "MODE 1 (AUTO) — Recalbox image extraction + 128x32 raw conversion + Cache + copy to SD",
        "mode2_title": "MODE 2 — systems/_defaults download via GitHub only",
        "mode3_title": "MODE 3 — Gamelist Extraction only",
        "mode4_title": "MODE 4 — PNG→raw565 + GIF→raw565pack/meta",
        "mode5_title": "MODE 5 — 128x32 conversion only",
        "mode6_title": "MODE 6 — Build Games Cache only",
        "mode7_title": "MODE 7 — Build systems_cache.dat only",
        "done": "🎉 DONE!",
        "done_sd": "📂 SD card folder        : ",
        "done_cache": "💾 Cache                 : ",
        "done_log": "📋 Missing images log    : ",
        "done_copy_sd": "Copy the contents of this folder to the root of your SD card.",
        "done_extracted": "📂 Images extracted to: ",
        "done_log2": "📋 Log: ",
        "done_converted": "📂 Images converted in: ",
        "done_cache2": "💾 Cache generated: ",
        "done_copy_cache": "Copy these files to the root of your SD card.",
        "done_cache_files": "💾 Generated files:",
        "src_ok": "✅ Folder: ",
        "roms_ok": "✅ ROMs folder: ",
        "sysc_title": "MODE 7 — Build systems_cache.dat",
        "sysc_no_defaults": "⚠️  No _defaults/ folder found in systems/.",
        "sysc_found": lambda n: f"   📂 {n} systems found in _defaults/",
        "sysc_line": lambda t, n: f"   {'✅' if t != '?' else '⚠️ '} {t}  {n}",
        "sysc_unknown": "⚠️  No .gif or .png found for this system.",
        "sysc_done": lambda n, p: f"✅ {n} systems written to {p}",
        "sysc_copy": "Copy systems_cache.dat to the root of your SD card.",
        "sysc_hint": "   (The ESP32 will use this on next boot instead of rescanning)",
        "flash_title": "MODE 8 — Copy to SD card",
        "flash_no_sdcard": "⚠️  sd_card/ folder is empty or missing. Run another mode first.",
        "flash_no_win": "⚠️  This mode is only available on Windows.",
        "flash_admin_warn": "⚠️  Administrator rights required. Please relaunch as Administrator.",
        "flash_drives_title": "\n💾  AVAILABLE DRIVES (removable / SD card) :",
        "flash_no_drives": "⚠️  No removable drive detected. Insert your SD card and try again.",
        "flash_drive_choice": "Choose destination drive (0 to go back): ",
        "flash_drive_warn": "⚠️  Invalid choice.\n",
        "flash_drive_sel": lambda d, s: f"\n✅ Destination: {d}  ({s})",
        "flash_mode_title": "\n⚙️  COPY MODE",
        "flash_mode_opt1": "Format FAT32 then copy  (WARNING: erases everything on the SD)",
        "flash_mode_opt2": "Copy only — overwrite existing files",
        "flash_mode_opt3": "Copy only — skip existing files (keep what's already there)",
        "flash_mode_choice": "Your choice (0-2): ",
        "flash_mode_warn": "⚠️  Type 0, 1 or 2.\n",
        "flash_fmt_confirm": lambda d: f"⚠️  ALL DATA on {d} will be erased. Are you sure?",
        "flash_fmt_abort": "   ↩  Format cancelled.",
        "flash_fmt_start": lambda d: f"🗑️  Formatting {d} in FAT32...",
        "flash_fmt_ok": "✅ Format complete.",
        "flash_fmt_err": lambda e: f"❌ Format error: {e}",
        "flash_copy_start": lambda s, d: f"\n📋 Copying {s} → {d} (robocopy /MT:32)...",
        "flash_copy_ok": "✅ Copy complete.",
        "flash_copy_err": lambda c, copied, total: f"⚠️ robocopy returned {c} — {copied}/{total} copied (check output above)",
        "main_opt6": "Copy sd_card/ to SD card  (fast, robocopy)",
        "main_opt5": "Build systems_cache.dat  (ESP32 system index)",
        "pause_hint": "   ⏸️  Press [ESC] to pause",
        "pause_title": "\n⏸️  PAUSED",
        "pause_opt1": "Continue",
        "pause_opt2": "Skip to next step",
        "pause_opt3": "Stop the script",
        "pause_choice": "Your choice (1-3): ",
        "pause_warn": "⚠️  Type 1, 2 or 3.\n",
        "pause_resuming": "▶️  Resuming...",
        "pause_skipping": "⏭️  Skipping to next step...",
        "pause_stopping": "🛑  Stop requested.",
        "sys_sel_title": "🎮  DETECTED SYSTEMS",
        "sys_sel_none": "⚠️  No system with gamelist.xml found in this folder.",
        "sys_sel_prompt": "Which systems to process?",
        "sys_sel_opt_all": "All systems",
        "sys_sel_opt_pick": "Choose specific systems",
        "sys_sel_pick_hint": "Enter numbers separated by commas (e.g. 1,3,5) or 0 to select all:",
        "sys_sel_warn": "⚠️  Invalid selection. Try again.\n",
        "sys_sel_selected": lambda n: f"✅ {n} system(s) selected.",
        "main_opt_advanced": "Advanced menu  (modes 2 to 8)",
        "advanced_title": "ADVANCED MODES",
        "advanced_prompt": "Choose an advanced mode:",
        "advanced_back": "Back to main menu",
        "main_opt2_desc": "Extraction + download _defaults from GitHub (only)",
        "popup_conv_title": "RetroBoxLED — 128x32 Conversion",
        "popup_conv_msg": lambda out_dir: f"Preparing 128x32 conversion.\n\nOutput:\n{out_dir}",
        "popup_conv_explore": "Open folder",
        "popup_conv_continue": "Continue",
        "conv_gif_converted": lambda n: f"\n   🎞️  GIF conversion -> raw565pack/meta: {n} files",
        "cache_build_header": lambda ts, tf: f"\n--- games_cache.bin ({ts} systems, {tf} games) ---",
        "cache_build_done": lambda name, ns, ng: f"   {name}  ({ns} systems, {ng} games)",
        "mode1_dl_phase": "\n⬇️  Downloading _defaults from GitHub...",
        "mode1_removed_files": lambda n: f"\n🗑️  {n} .png/.gif files removed (replaced by raw565)",
        "mode1_removed_errors": lambda n: f"⚠️  {n} .png/.gif files could not be removed",
        "mode1_conv_errors": lambda n: f"\n⚠️  {n} conversion error(s) (see details above)",
        "mode2_no_other_step": "ℹ️  (no other step is executed)",
        "mode2_copy_hint": "↩  You can then copy sd_card/ to the SD card.",
        "copy_files_total": lambda n, dst: f"   📊 {n} files to copy to {dst}",
        "copy_files_from": lambda n, src: f"   📊 {n} files to copy from {src}",
        "copy_sd_unreachable": lambda copied, total: f"   ❌ SD CARD UNREACHABLE — copy stopped at {copied}/{total} files",
        "copy_file_error": lambda rel, detail: f"   ❌ ERROR: {rel} — {detail}",
        "copy_progress_line": lambda icon, copied, total, rel, tag: f"   {icon} {copied}/{total} — {rel} ({tag})",
        "copy_file_progress": lambda copied, total, line: f"   📄 Copying {copied}/{total}: {line}",
        "copy_tag_skipped": "skipped",
        "copy_tag_ok": "OK",
        "copy_interrupted": lambda copied, total: f"   ⚠️  Copy interrupted: {copied}/{total} files copied",
        "copy_done_with_failed": lambda copied, total, nfailed: f"   ⚠️  Copy finished: {copied}/{total} files OK, {nfailed} failed",
        "copy_done_all": lambda copied, total: f"   ✅ Copy finished: {copied}/{total} files",
        "copy_status_interrupted": "Interrupted",
        "copy_status_done": "Done",
        "retry_total": lambda n, dst: f"   📊 {n} failed file(s) to retry to {dst}",
        "retry_sd_unreachable": "   ❌ SD CARD UNREACHABLE — retry stopped",
        "retry_file_ok": lambda i, total, relp: f"   ✅ {i}/{total} — {relp}",
        "retry_done_with_failed": lambda copied, total, nfailed: f"   ⚠️  Retry finished: {copied}/{total} OK, {nfailed} failed",
        "retry_done_all": lambda copied, total: f"   ✅ Retry finished: {copied}/{total} files",
        "m1_no_systems": "   No system with gamelist.xml found.",
        "m1_systems_detected": lambda n: f"   {n} systems detected.",
        "m1_tag_xml": lambda tag: f"   XML tag: <{tag}>",
        "m1_profile": lambda desc: f"   Profile: {desc}",
        "m1_analyzing": lambda sys_name: f"   [{sys_name}] Analyzing...",
        "m1_xml_error": lambda e: f"      XML error: {e}",
        "m1_xml_error_sys": lambda sys_name, e: f"      XML error [{sys_name}]: {e}",
        "m1_games_summary": lambda games, present, missing: f"      {games} games, {present} present, {missing} missing",
        "m1_no_games": "      No games found",
        "m1_more_others": lambda n: f"         ... and {n} more",
        "m1_result_summary": lambda games, present, missing: f"RESULT: {games} games scanned, {present} images present, {missing} missing",
        "cache_file_line": lambda path, ns, ng: f"   💾 {path}  ({ns} systems, {ng} games)",
        "progress_conv_idle": "no PNG or GIF to convert",
        "progress_conv_running": "conversion in progress",
        "progress_conv_raw_running": "raw conversion in progress",
    },
    "es": {
        "pillow_installing": "⚙️  Pillow no está instalado. Instalando automáticamente...",
        "pillow_ok": "✅ ¡Pillow instalado correctamente!\n",
        "pillow_fail": "❌ No se pudo instalar Pillow.\n   Ejecútalo manualmente: pip install Pillow\n   La conversión 128x32 estará desactivada.\n",
        "main_title": "RetroBoxLED Toolkit for Recalbox",
        "dl_title": "🌐  CARPETA _defaults (imágenes de sistemas)",
        "dl_missing": "   ℹ️  No se encontró carpeta _defaults/ en sd_card/systems/.",
        "dl_exists": "   ℹ️  La carpeta _defaults/ ya existe en sd_card/systems/.",
        "dl_ask_download": "¿Descargar _defaults/ desde GitHub (RetroBoxLED)?",
        "dl_ask_update": "¿Actualizar _defaults/ desde GitHub (RetroBoxLED)?",
        "dl_skip": "   ⏭️  Descarga omitida.",
        "dl_starting": "⬇️  Descargando archivos desde GitHub...",
        "dl_file_ok": lambda n, i, t: f"   {i:4d}/{t} ✅ {n}",
        "dl_file_err": lambda n, e: f"   ⚠️  {n} — {e}",
        "dl_file_skip": lambda n, i, t: f"   {i:4d}/{t} ⏭️  {n} (ya presente, conservado)",
        "dl_done": lambda n: f"✅ {n} archivos descargados en _defaults/",
        "dl_fail_api": "❌ API de GitHub inaccesible. Verifica tu conexión a internet.",
        "github_rate_limit_msg": lambda reset_time: (
            f"⏳ Límite de solicitudes de GitHub alcanzado (cuota horaria de la "
            f"API, 60 solicitudes/hora sin conexión). Vuelve a intentarlo "
            f"después de las {reset_time}."
        ),
        "gifpack_title": "📦 Descargando el pack gratuito de GIFs...",
        "gifpack_fail_api": lambda e: f"❌ API de GitHub inaccesible ({e}). Verifica tu conexión a internet.",
        "gifpack_fail_subfolder": lambda sub, e: f"   ⚠️  Subcarpeta '{sub}' omitida ({e})",
        "gifpack_starting": lambda total, nsub: f"   {total} GIFs encontrados en {nsub} subcarpeta(s)",
        "gifpack_file_ok": lambda n, i, t: f"   {i:4d}/{t} ✅ {n}",
        "gifpack_file_err": lambda n, e: f"   ⚠️  {n} — {e}",
        "gifpack_done": lambda n: f"✅ {n} GIFs descargados",
        "gifpack_playlist_ok": "✅ Playlist de muestra 'RpiTeaM eLLuiGi.txt' descargada y seleccionada por defecto",
        "gifpack_playlist_fail": lambda e: f"⚠️  Fallo al descargar la playlist de muestra ({e}) — se usará ALL.txt",
        "all_playlist_created": lambda n: f"✅ Playlist 'ALL.txt' creada ({n} carpeta(s), todos los GIFs de la tarjeta)",
        "all_playlist_skipped_empty": "ℹ️  No hay GIFs en la tarjeta — playlist 'ALL.txt' no creada, sin playlist por defecto",
        "dl_replacing": "🗑️  Reemplazando _defaults/ existente...",
        "main_prompt": "¿Qué desea hacer?",
        "main_opt1": "Extracción gamelist + Conversión 128x32 + Build cache  (TODO)",
        "main_opt2": "Solo extraer imágenes de los gamelists",
        "main_opt3": "Solo convertir imágenes a 128x32",
        "main_opt4": "Solo construir games_cache.bin",
        "main_opt7": "Interfaz gráfica (Tkinter)",
        "mode9_title": "MODO 9 — Instalar scripts de Recalbox",
        "mode9_short_title": "Instalar scripts de Recalbox",
        "mode9_desc": "Instala/actualiza los scripts de usuario de Recalbox (WiFi Recovery, Config Web, puente marquee) directamente en el recurso compartido de red de la Recalbox.",
        "mode9_ip_prompt": "Dirección IP o nombre de red de la Recalbox",
        "mode9_autodetect_ok": lambda host: f"✅ Recalbox detectada automáticamente: {host}",
        "mode9_result_ok": lambda n: f"   ✅ {n}",
        "mode9_result_fail": lambda n, e: f"   ❌ {n} — {e}",
        "mode9_share_unreachable": lambda host: f"❌ Recurso compartido no encontrado ({host or 'sin destino'}) — comprueba que la Recalbox esté encendida y accesible.",
        "mode9_guest_blocked": "❌ Acceso invitado SMB bloqueado por la política de seguridad de Windows (no es un problema de red/IP) — activa \"inicios de sesión de invitado no seguros\": PowerShell como Administrador -> Set-SmbClientConfiguration -EnableInsecureGuestLogons $true -Force, luego vuelve a intentarlo.",
        "mode9_smb_fallback_ssh": "⚠️  Recurso compartido SMB inalcanzable — probando repliegue por SSH (credenciales por defecto de Recalbox)...",
        "paramiko_installing": "⚙️  paramiko no está instalado (necesario para el repliegue SSH). Instalando automáticamente...",
        "paramiko_ok": "✅ ¡paramiko instalado correctamente!\n",
        "paramiko_fail": "❌ No se pudo instalar paramiko.\n   Ejecútalo manualmente: pip install paramiko\n   El repliegue SSH estará desactivado.\n",
        "ssh_connect_fail": lambda host, e: f"❌ Conexión SSH fallida ({host}) — {e}",
        "mode1_scripts_staged": "\n📥  Preparando localmente los scripts de Recalbox (carpeta temporal)...",
        "mode1_scripts_phase": "\n📂  Instalando scripts de Recalbox...",
        "mode1_scripts_skip": "\n⏭️  Recalbox no confirmada — scripts no instalados automáticamente (copia manual posible desde la carpeta temporal, o Modo 9 más tarde).",
        "mode1_scripts_installed_via": lambda m: f"   ℹ️  Instalados vía {'SMB' if m == 'smb' else 'SSH (repliegue)'}.",
        "main_choice": "Su eleccion (1-9): ",
        "main_opt_quit": "SALIR",
        "main_warn": "⚠️  Escribe un número entre 0 y 9.\n",
        "back": "↩  Volver atrás",
        "back_main": "\n  ↩  Volver al menú principal...",
        "back_roms": "\n  ↩  Volver a la selección de carpeta ROMs...",
        "yes_no": "(s/n)",
        "yes_vals": ("s", "si", "sí", "y", "yes", "o", "oui"),
        "no_vals": ("n", "no", "non"),
        "warn_yn": "⚠️  Escribe s o n.\n",
        "warn_choice": "⚠️  Escribe 0, 1 o 2.\n",
        "after_menu": "¿Qué desea hacer a continuación?",
        "after_opt1": "Volver al menú principal",
        "after_opt6": "Copiar a la tarjeta SD ahora  (modo 8)",
        "after_opt_files": "Copiar los archivos generados a la tarjeta SD",
        "press_enter": "Pulsa Intro para cerrar...",
        "press_enter_cont": "Pulsa Intro para continuar de todas formas...",
        "path_local": "  1  →  Disco local  (ej: D:\\Recalbox\\roms)",
        "path_network": "  2  →  Red / NAS    (ej: \\\\192.168.1.1\\share\\roms)",
        "path_choice": "Su elección (0, 1 o 2): ",
        "path_local_lbl": "Ruta de la carpeta  (0 para volver): ",
        "path_net_lbl": "Ruta de red         (0 para volver): ",
        "path_not_found": "❌ Carpeta no encontrada. Verifica la ruta e inténtalo de nuevo.\n",
        "sd_erase_ask": "¿Desea borrarla completamente antes de continuar?",
        "sd_erased": "🗑️  Carpeta borrada y recreada.",
        "sd_erase_err": "❌ Error al borrar. Cierra los archivos abiertos en sd_card/ e inténtalo de nuevo.",
        "sd_kept": "   ℹ️  Contenido conservado, los archivos existentes serán sobreescritos o ignorados.",
        "ext_title": "🚀 PASO 1/3 — EXTRACCIÓN DE IMÁGENES",
        "ext_roms_where": "\n📍 ¿DÓNDE ESTÁN SUS ROMS?",
        "ext_systems": "sistemas con gamelist.xml detectados",
        "ext_games_found": "juegos encontrados",
        "ext_xml_err": "ERROR XML",
        "ext_already": "ya existe",
        "ext_missing_tag": "FALTA",
        "ext_summary_copied": "copiadas",
        "ext_summary_skip": "ya presentes",
        "ext_summary_miss": "faltantes",
        "ext_log_header": "=== IMÁGENES FALTANTES ===\n",
        "ext_log_source": "Fuente: ",
        "ext_log_summary": "=== RESUMEN ===\n",
        "ext_log_games": "Juegos analizados : ",
        "ext_log_copied": "Imágenes copiadas : ",
        "ext_log_skipped": "Ya presentes      : ",
        "ext_log_missing": "Imágenes faltantes: ",
        "conv_title": "🖼️  PASO 2/3 — CONVERSIÓN 128x32",
        "conv_png_only": "\n⚠️  INFO: Solo los archivos PNG serán convertidos a 128x32.\n          Los GIF se conservan tal cual.\n",
        "conv_gif_info": "GIF encontrados → convertidos a raw565pack/meta.",
        "conv_png_count": "PNG a convertir a 128x32",
        "conv_summary_done": "PNG convertidos",
        "conv_summary_err": "errores",
        "conv_summary_gif": "GIF raw565pack/meta",
        "conv_no_pillow": "❌ Pillow no está instalado. Instálalo con: pip install Pillow",
        "conv_src_where": "\n📍 ¿DÓNDE ESTÁN LAS IMÁGENES A CONVERTIR?\n   (carpeta systems/ con subcarpetas por sistema)",
        "cache_title": "💾 PASO 3/3 — BUILD GAMES CACHE",
        "cache_scan": "[INFO] Analizando: ",
        "cache_found_sys": "sistemas,",
        "cache_found_games": "juegos encontrados",
        "cache_no_sys": "⚠️  No se encontraron sistemas. Verifica la carpeta systems/.",
        "cache_size": "KB",
        "cache_sys_where": "\n📍 ¿DÓNDE ESTÁ LA CARPETA SYSTEMS?",
        "cache_sys_detected": "✅ Carpeta systems/ detectada: ",
        "cache_sys_use": "¿Usar esta carpeta?",
        "cache_sys_missing": "⚠️  No se encontró carpeta systems/ en ",
        "mode1_title": "MODO 1 (AUTO) — Extracción de imágenes desde Recalbox + Conversión raw 128x32 + Cache + copia a la SD",
        "mode2_title": "MODO 2 — descarga systems/_defaults via GitHub (solo)",
        "mode3_title": "MODO 3 — Solo Extracción Gamelist",
        "mode4_title": "MODO 4 — PNG→raw565 + GIF→raw565pack/meta",
        "mode5_title": "MODO 5 — Solo Conversión 128x32",
        "mode6_title": "MODO 6 — Solo Build Games Cache",
        "mode7_title": "MODO 7 — Solo Build systems_cache.dat",
        "done": "🎉 ¡TERMINADO!",
        "done_sd": "📂 Carpeta SD card       : ",
        "done_cache": "💾 Cache                 : ",
        "done_log": "📋 Log imágenes faltantes: ",
        "done_copy_sd": "Copia el contenido de esta carpeta en la raíz de tu tarjeta SD.",
        "done_extracted": "📂 Imágenes extraídas en: ",
        "done_log2": "📋 Log: ",
        "done_converted": "📂 Imágenes convertidas en: ",
        "done_cache2": "💾 Cache generado: ",
        "done_copy_cache": "Copia estos archivos en la raíz de tu tarjeta SD.",
        "done_cache_files": "💾 Archivos generados:",
        "src_ok": "✅ Carpeta: ",
        "roms_ok": "✅ Carpeta ROMs: ",
        "sysc_title": "MODO 7 — Generar systems_cache.dat",
        "sysc_no_defaults": "⚠️  No se encontró carpeta _defaults/ en systems/.",
        "sysc_found": lambda n: f"   📂 {n} sistemas encontrados en _defaults/",
        "sysc_line": lambda t, n: f"   {'✅' if t != '?' else '⚠️ '} {t}  {n}",
        "sysc_unknown": "⚠️  No se encontró .gif ni .png para este sistema.",
        "sysc_done": lambda n, p: f"✅ {n} sistemas escritos en {p}",
        "sysc_copy": "Copia systems_cache.dat en la raíz de tu tarjeta SD.",
        "sysc_hint": "   (El ESP32 lo usará en el próximo arranque sin rescanear)",
        "flash_title": "MODO 8 — Copiar a la tarjeta SD",
        "flash_no_sdcard": "⚠️  La carpeta sd_card/ está vacía o no existe. Ejecuta otro modo primero.",
        "flash_no_win": "⚠️  Este modo solo está disponible en Windows.",
        "flash_admin_warn": "⚠️  Se requieren derechos de administrador. Relanza el script como Administrador.",
        "flash_drives_title": "\n💾  UNIDADES DISPONIBLES (extraíbles / tarjeta SD) :",
        "flash_no_drives": "⚠️  No se detectó ninguna unidad extraíble. Inserta tu tarjeta SD e inténtalo de nuevo.",
        "flash_drive_choice": "Elige la unidad de destino (0 para volver): ",
        "flash_drive_warn": "⚠️  Elección inválida.\n",
        "flash_drive_sel": lambda d, s: f"\n✅ Destino: {d}  ({s})",
        "flash_mode_title": "\n⚙️  MODO DE COPIA",
        "flash_mode_opt1": "Formatear en FAT32 y copiar  (ATENCIÓN: borra todo en la SD)",
        "flash_mode_opt2": "Solo copiar — sobreescribir archivos existentes",
        "flash_mode_opt3": "Solo copiar — ignorar archivos existentes (conservar lo que ya está)",
        "flash_mode_choice": "Su elección (0-2): ",
        "flash_mode_warn": "⚠️  Escribe 0, 1 o 2.\n",
        "flash_fmt_confirm": lambda d: f"⚠️  TODOS LOS DATOS en {d} serán borrados. ¿Estás seguro?",
        "flash_fmt_abort": "   ↩  Formateo cancelado.",
        "flash_fmt_start": lambda d: f"🗑️  Formateando {d} en FAT32...",
        "flash_fmt_ok": "✅ Formateo completado.",
        "flash_fmt_err": lambda e: f"❌ Error de formateo: {e}",
        "flash_copy_start": lambda s, d: f"\n📋 Copiando {s} → {d} (robocopy /MT:32)...",
        "flash_copy_ok": "✅ Copia completada.",
        "flash_copy_err": lambda c, copied, total: f"⚠️ robocopy devolvió {c} — {copied}/{total} copiados (revisa la salida arriba)",
        "main_opt6": "Copiar sd_card/ a la tarjeta SD  (rápido, robocopy)",
        "main_opt5": "Generar systems_cache.dat  (índice de sistemas ESP32)",
        "pause_hint": "   ⏸️  Pulsa [ESC] para pausar",
        "pause_title": "\n⏸️  PAUSADO",
        "pause_opt1": "Continuar",
        "pause_opt2": "Saltar al siguiente paso",
        "pause_opt3": "Detener el script",
        "pause_choice": "Su elección (1-3): ",
        "pause_warn": "⚠️  Escribe 1, 2 o 3.\n",
        "pause_resuming": "▶️  Reanudando...",
        "pause_skipping": "⏭️  Saltando al siguiente paso...",
        "pause_stopping": "🛑  Parada solicitada.",
        "sys_sel_title": "🎮  SISTEMAS DETECTADOS",
        "sys_sel_none": "⚠️  No se encontró ningún sistema con gamelist.xml en esta carpeta.",
        "sys_sel_prompt": "¿Qué sistemas procesar?",
        "sys_sel_opt_all": "Todos los sistemas",
        "sys_sel_opt_pick": "Elegir sistemas específicos",
        "sys_sel_pick_hint": "Introduce los números separados por comas (ej: 1,3,5) o 0 para todos:",
        "sys_sel_warn": "⚠️  Selección inválida. Inténtalo de nuevo.\n",
        "sys_sel_selected": lambda n: f"✅ {n} sistema(s) seleccionado(s).",
        "main_opt_advanced": "Menú avanzado  (modos 2 a 8)",
        "advanced_title": "MODOS AVANZADOS",
        "advanced_prompt": "Elija un modo avanzado:",
        "advanced_back": "Volver al menú principal",
        "main_opt2_desc": "Extracción + descarga _defaults desde GitHub (solo)",
        "popup_conv_title": "RetroBoxLED — Conversión 128x32",
        "popup_conv_msg": lambda out_dir: f"Preparando conversión 128x32.\n\nSalida:\n{out_dir}",
        "popup_conv_explore": "Abrir carpeta",
        "popup_conv_continue": "Continuar",
        "conv_gif_converted": lambda n: f"\n   🎞️  Conversión GIF -> raw565pack/meta: {n} archivos",
        "cache_build_header": lambda ts, tf: f"\n--- games_cache.bin ({ts} sistemas, {tf} juegos) ---",
        "cache_build_done": lambda name, ns, ng: f"   {name}  ({ns} sistemas, {ng} juegos)",
        "mode1_dl_phase": "\n⬇️  Descargando _defaults desde GitHub...",
        "mode1_removed_files": lambda n: f"\n🗑️  {n} archivos .png/.gif eliminados (reemplazados por raw565)",
        "mode1_removed_errors": lambda n: f"⚠️  {n} archivos .png/.gif no se pudieron eliminar",
        "mode1_conv_errors": lambda n: f"\n⚠️  {n} error(es) de conversión (ver detalles arriba)",
        "mode2_no_other_step": "ℹ️  (no se ejecuta ningún otro paso)",
        "mode2_copy_hint": "↩  A continuación puedes copiar sd_card/ en la tarjeta SD.",
        "copy_files_total": lambda n, dst: f"   📊 {n} archivos a copiar en {dst}",
        "copy_files_from": lambda n, src: f"   📊 {n} archivos a copiar desde {src}",
        "copy_sd_unreachable": lambda copied, total: f"   ❌ TARJETA SD INACCESIBLE — copia detenida en {copied}/{total} archivos",
        "copy_file_error": lambda rel, detail: f"   ❌ ERROR: {rel} — {detail}",
        "copy_progress_line": lambda icon, copied, total, rel, tag: f"   {icon} {copied}/{total} — {rel} ({tag})",
        "copy_file_progress": lambda copied, total, line: f"   📄 Copiando {copied}/{total}: {line}",
        "copy_tag_skipped": "omitido",
        "copy_tag_ok": "OK",
        "copy_interrupted": lambda copied, total: f"   ⚠️  Copia interrumpida: {copied}/{total} archivos copiados",
        "copy_done_with_failed": lambda copied, total, nfailed: f"   ⚠️  Copia terminada: {copied}/{total} archivos OK, {nfailed} con error",
        "copy_done_all": lambda copied, total: f"   ✅ Copia terminada: {copied}/{total} archivos",
        "copy_status_interrupted": "Interrumpido",
        "copy_status_done": "Terminado",
        "retry_total": lambda n, dst: f"   📊 {n} archivo(s) con error para reintentar en {dst}",
        "retry_sd_unreachable": "   ❌ TARJETA SD INACCESIBLE — reintento detenido",
        "retry_file_ok": lambda i, total, relp: f"   ✅ {i}/{total} — {relp}",
        "retry_done_with_failed": lambda copied, total, nfailed: f"   ⚠️  Reintento terminado: {copied}/{total} OK, {nfailed} con error",
        "retry_done_all": lambda copied, total: f"   ✅ Reintento terminado: {copied}/{total} archivos",
        "m1_no_systems": "   No se encontró ningún sistema con gamelist.xml.",
        "m1_systems_detected": lambda n: f"   {n} sistemas detectados.",
        "m1_tag_xml": lambda tag: f"   Etiqueta XML: <{tag}>",
        "m1_profile": lambda desc: f"   Perfil: {desc}",
        "m1_analyzing": lambda sys_name: f"   [{sys_name}] Analizando...",
        "m1_xml_error": lambda e: f"      Error XML: {e}",
        "m1_xml_error_sys": lambda sys_name, e: f"      Error XML [{sys_name}]: {e}",
        "m1_games_summary": lambda games, present, missing: f"      {games} juegos, {present} presentes, {missing} faltantes",
        "m1_no_games": "      No se encontraron juegos",
        "m1_more_others": lambda n: f"         ... y {n} más",
        "m1_result_summary": lambda games, present, missing: f"RESULTADO: {games} juegos escaneados, {present} imágenes presentes, {missing} faltantes",
        "cache_file_line": lambda path, ns, ng: f"   💾 {path}  ({ns} sistemas, {ng} juegos)",
        "progress_conv_idle": "no hay PNG ni GIF para convertir",
        "progress_conv_running": "conversión en curso",
        "progress_conv_raw_running": "conversión raw en curso",
    },
}

# Global translation dict (set in main after language selection)
T = TRANSLATIONS["fr"]
# Code langue courant ("fr"/"en"/"es"), tenu a jour partout ou T est
# reassigne (select_language() ci-dessous, RecalBoxDMD_GUI.py::
# _set_toolkit_language()) -- transmis au DMD via write_dmd_language()
# (meme langue pour l'outil et l'appareil, cf demande utilisateur).
CURRENT_LANG = "fr"


def tr(key):
    return T[key]


# ─────────────────────────────────────────────────────────────────────────────
#  INSTALLATION AUTOMATIQUE DES DÉPENDANCES
# ─────────────────────────────────────────────────────────────────────────────

PIL_AVAILABLE = False


def ensure_dependencies():
    global PIL_AVAILABLE
    try:
        from PIL import Image

        PIL_AVAILABLE = True
        return
    except ImportError:
        print(tr("pillow_installing"))
        import subprocess

        try:
            subprocess.check_call(
                [sys.executable, "-m", "pip", "install", "Pillow"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            print(tr("pillow_ok"))
            PIL_AVAILABLE = True
        except subprocess.CalledProcessError:
            print(tr("pillow_fail"))
            PIL_AVAILABLE = False


def _ensure_paramiko() -> bool:
    """
    Installe paramiko (pip) si absent -- meme mecanisme que
    ensure_dependencies()/Pillow, mais installe seulement a la demande
    (repli SSH declenche uniquement si le partage SMB est injoignable,
    pas systematiquement au demarrage) : la plupart des utilisateurs n'en
    ont jamais besoin. Retourne True si paramiko est disponible (deja
    installe ou installation reussie).
    """
    try:
        import paramiko  # noqa: F401

        return True
    except ImportError:
        print(tr("paramiko_installing"))
        import subprocess

        try:
            subprocess.check_call(
                [sys.executable, "-m", "pip", "install", "paramiko"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            print(tr("paramiko_ok"))
            return True
        except subprocess.CalledProcessError:
            print(tr("paramiko_fail"))
            return False


# ─────────────────────────────────────────────────────────────────────────────
#  CONSTANTES
# ─────────────────────────────────────────────────────────────────────────────

TARGET_W = 128
TARGET_H = 32
EXTENSIONS_CACHE = {".gif": 0x67, ".png": 0x70, ".raw565": 0x70, ".raw565pack": 0x67}
LETTERS = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ"
NB_LETTERS = len(LETTERS)

# ─────────────────────────────────────────────────────────────────────────────
#  UTILITAIRES COMMUNS
# ─────────────────────────────────────────────────────────────────────────────


def sep(char="═", width=70):
    print(char * width)


def title(text):
    sep()
    print(f"  {text}")
    sep()


def ask_yes_no(question):
    yn = tr("yes_no")
    yes = tr("yes_vals")
    no = tr("no_vals")
    while True:
        r = input(f"{question} {yn} : ").strip().lower()
        if r in yes:
            return True
        if r in no:
            return False
        print(tr("warn_yn"))


def ask_path(must_exist=True):
    """Demande un chemin local ou réseau. Retourne Path ou None (retour arrière)."""
    while True:
        print()
        print(tr("path_local"))
        print(tr("path_network"))
        print(f"  0  →  {tr('back')}")
        print()
        choix = input(tr("path_choice")).strip()
        if choix == "0":
            return None
        if choix not in ("1", "2"):
            print(tr("warn_choice"))
            continue
        lbl = tr("path_local_lbl") if choix == "1" else tr("path_net_lbl")
        chemin = input(lbl).strip().strip('"')
        if chemin == "0":
            continue
        p = Path(chemin)
        if not must_exist or (p.exists() and p.is_dir()):
            return p
        print(tr("path_not_found"))


def sanitize_filename(name: str) -> str:
    for ch in r'\/:*?"<>|':
        name = name.replace(ch, "_")
    name = name.replace(" ", "")
    return name.strip()


# --------------------------------------------------
# Calcul d'index bigramme pour le cache (compatible ESP32, NB_IDX=703)
# --------------------------------------------------
def _calc_bigram_idx(name: str) -> int:
    """Calcule l'index bigramme (0-702) a partir du nom du jeu.

    Doit reproduire EXACTEMENT bigramIndex() (RecalBox_DMD.ino) -- c'est le
    firmware qui relit games_cache.bin au runtime, cette fonction ne fait
    que le construire. Un desaccord entre les deux formules rend le cache
    inutilisable (voir changelog v30 : 100% de desaccord avant ce fix,
    seul le cas particulier "1er caractere non-alphabetique -> 0" etait
    identique des deux cotes, par coincidence).
    """
    if not name:
        return 0
    c1 = name[0].upper()
    if not ("A" <= c1 <= "Z"):
        return 0
    i1 = ord(c1) - ord("A")
    base = 1 + i1 * 27
    if len(name) < 2:
        return base
    c2 = name[1].upper()
    if not ("A" <= c2 <= "Z"):
        return base
    return base + (ord(c2) - ord("A")) + 1


# --------------------------------------------------
# Sous-dossiers alphabétiques A..Z et #
# Convertit un chemin de destination pour écrire dans
# un sous-dossier A..Z ou # basé sur la première lettre
# du nom de fichier (sans extension).
#
# Exemples:
#   dst="/systems/nes/zelda.raw565"  -> "/systems/nes/Z/zelda.raw565"
#   dst="/systems/nes/123.raw565"    -> "/systems/nes/#/123.raw565"
#   dst avec _defaults/              -> inchangé
# --------------------------------------------------
def _alpha_subdir(dst: Path) -> Path:
    name = dst.stem
    first = name[0].upper() if name else "?"
    subdir = first if first.isalpha() else "#"
    new_dir = dst.parent / subdir
    new_dir.mkdir(parents=True, exist_ok=True)
    return new_dir / dst.name


# --------------------------------------------------
# Applique _alpha_subdir à un chemin de destination
# si ce n'est pas un chemin _defaults/
#
# Idempotent : si dst est déjà dans son bon sous-dossier alphabétique
# (ex: raw565 écrit à côté d'un PNG source déjà bucketisé par
# extract_system), on ne réimbrique pas un 2e niveau A..Z/#.
# --------------------------------------------------
def _alpha_subdir_if_needed(dst: Path) -> Path:
    if "_defaults" in dst.parts:
        return dst
    name = dst.stem
    first = name[0].upper() if name else "?"
    expected_subdir = first if first.isalpha() else "#"
    if dst.parent.name == expected_subdir:
        return dst
    return _alpha_subdir(dst)


def _bucket_letter_for_stem(stem: str) -> str:
    """
    Regle du bucket alphabetique (1ere lettre du nom de fichier SANS
    extension, majuscule, '#' si non-alpha ou nom vide) -- meme regle que
    _alpha_subdir()/_alpha_subdir_if_needed() ci-dessus, factorisee ici en
    fonction reutilisable pour le comptage par bucket de
    build_systems_cache() (flag "L" par sous-dossier plutot que par
    systeme entier -- portage du worktree dev/slow-flag-per-bucket,
    branche dev de test).
    """
    first = stem[0].upper() if stem else "?"
    return first if first.isalpha() else "#"


# ─────────────────────────────────────────────────────────────────────────────
#  SD CARD
# ─────────────────────────────────────────────────────────────────────────────


def get_sd_card_dir(script_dir: Path) -> Path:
    """
    Emplacement GUI/Tk : dossier temporaire dans %LOCALAPPDATA%/Temp.
    On utilise un sous-dossier pour éviter de polluer le temp global.
    """
    local_appdata = os.environ.get("LOCALAPPDATA")
    if local_appdata:
        base = Path(local_appdata) / "Temp"
    else:
        # fallback (devrait être rare sur Windows)
        base = Path(tempfile.gettempdir())  # type: ignore[name-defined]

    return base / "RecalBoxDMD"


def prepare_sd_card(sd_dir: Path, interactive: bool = True):
    if sd_dir.exists():
        items = list(sd_dir.iterdir())
        if items:
            print(f"\n⚠️  '{sd_dir}' ({len(items)} items)")
            if not interactive:
                # GUI: pas de question clavier. On garde le contenu.
                print(tr("sd_kept"))
                return
            if ask_yes_no(tr("sd_erase_ask")):
                try:
                    shutil.rmtree(sd_dir)
                    sd_dir.mkdir(parents=True)
                    print(tr("sd_erased"))
                except Exception as e:
                    print(f"{tr('sd_erase_err')}\n   {e}")
                    input(tr("press_enter_cont"))
                    sd_dir.mkdir(parents=True, exist_ok=True)
            else:
                print(tr("sd_kept"))
    else:
        sd_dir.mkdir(parents=True, exist_ok=True)


def write_dmd_language(sd_dir: Path, lang: str) -> None:
    """
    Ecrit/patch la cle "language=" dans sd_dir/config.ini -- lecture-
    modification-ecriture (meme logique que writeConfigFlag() cote firmware
    RecalBox_DMD.ino) : cree le fichier s'il n'existe pas encore, sinon ne
    touche QUE la ligne "language=", tout le reste (wifi_*, playlist,
    recalbox_ip, etc. si deja presents) est preserve tel quel.
    Objectif : que le DMD demarre dans la langue de l'utilisateur des le
    premier boot (page AP + bannieres ecran), sans ressaisie manuelle.
    """
    if lang not in ("fr", "en", "es"):
        return
    cfg_path = sd_dir / "config.ini"
    lines: list[str] = []
    found = False
    if cfg_path.exists():
        lines = cfg_path.read_text(encoding="utf-8").splitlines()
        for i, line in enumerate(lines):
            if line.startswith("language="):
                lines[i] = f"language={lang}"
                found = True
                break
    if not found:
        lines.append(f"language={lang}")
    cfg_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_dmd_recalbox_ip(sd_dir: Path, ip: str) -> None:
    """
    Ecrit/patch la cle "recalbox_ip=" dans sd_dir/config.ini (meme logique
    lecture-modification-ecriture que write_dmd_language()). Cette cle est
    deja lue par le firmware (RecalBox_DMD.ino) pour se connecter au broker
    MQTT de la Recalbox, et pre-remplit le champ "IP Recalbox" de la page
    web config -- normalement auto-detectee par mDNS au premier boot, mais
    seulement si la Recalbox est allumee et joignable a ce moment-la.
    Ecrire directement l'IP deja validee par l'utilisateur en Mode 1 (RB
    confirmee + scripts installes avec succes) evite de dependre de cette
    detection mDNS, meme si la Recalbox est eteinte au moment du premier
    boot/de la config web.
    """
    if not ip:
        return
    cfg_path = sd_dir / "config.ini"
    lines: list[str] = []
    found = False
    if cfg_path.exists():
        lines = cfg_path.read_text(encoding="utf-8").splitlines()
        for i, line in enumerate(lines):
            if line.startswith("recalbox_ip="):
                lines[i] = f"recalbox_ip={ip}"
                found = True
                break
    if not found:
        lines.append(f"recalbox_ip={ip}")
    cfg_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_dmd_first_boot(sd_dir: Path) -> None:
    """
    Force "first_boot=1" dans sd_dir/config.ini (meme logique lecture-
    modification-ecriture que write_dmd_language()) -- demande
    utilisateur : le Mode 1 doit garantir que la carte SD produite
    declenche bien le parcours "premier demarrage" en 2 temps sur le
    DMD (1. WiFi via smartphone, 2. config DMD via navigateur, voir
    mode1_next_steps_msg cote GUI), meme si le dossier de travail
    contenait deja un config.ini residuel d'une session precedente ou
    d'un test materiel reel (ou le firmware aurait pu ecrire
    first_boot=0 lui-meme -- prepare_sd_card() en mode GUI ne vide
    jamais un dossier de travail deja rempli). Sans cet appel explicite,
    rien dans l'outil PC n'ecrit jamais cette cle : elle ne serait
    activee que par defaut (variable g_firstBoot=true cote firmware) SI
    la cle est totalement absente -- un residu "first_boot=0" resterait
    sinon silencieusement en place.
    """
    cfg_path = sd_dir / "config.ini"
    lines: list[str] = []
    found = False
    if cfg_path.exists():
        lines = cfg_path.read_text(encoding="utf-8").splitlines()
        for i, line in enumerate(lines):
            if line.startswith("first_boot="):
                lines[i] = "first_boot=1"
                found = True
                break
    if not found:
        lines.append("first_boot=1")
    cfg_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_dmd_default_playlist(sd_dir: Path, name: str) -> None:
    """
    Ecrit/patch la cle "playlist=" dans sd_dir/config.ini (meme logique
    lecture-modification-ecriture que write_dmd_language()/
    write_dmd_recalbox_ip()) -- deja lue par le firmware (loadConfig(),
    RecalBox_DMD.ino : `if(key=="playlist" && value.length()) playlistName
    = value;`) pour determiner la playlist active au demarrage
    (playlistSourcePath = "/playlists/" + playlistName).

    name : nom de FICHIER seul (ex: "ALL.txt" ou GITHUB_DEFAULT_PLAYLIST_NAME),
    pas un chemin. name="" (ou falsy) retire la cle existante au lieu
    d'en ecrire une vide -- le firmware ignore de toute facon une valeur
    vide (`value.length()` ci-dessus), meme effet, mais un config.ini sans
    cle orpheline est plus propre.
    """
    cfg_path = sd_dir / "config.ini"
    lines: list[str] = []
    if cfg_path.exists():
        lines = [
            line for line in cfg_path.read_text(encoding="utf-8").splitlines()
            if not line.startswith("playlist=")
        ]
    if name:
        lines.append(f"playlist={name}")
    cfg_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


# ─────────────────────────────────────────────────────────────────────────────
#  EXTRACTION GAMELIST
# ─────────────────────────────────────────────────────────────────────────────

_INVALID_XML_CHARS = re.compile(
    r"[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]" r"|&#(?:x[0-9a-fA-F]+|\d+);"
)


def _is_valid_codepoint(m: re.Match) -> bool:
    s = m.group()
    if not s.startswith("&#"):
        return False
    inner = s[2:-1]
    code = int(inner[1:], 16) if inner.startswith("x") else int(inner)
    return (
        code == 0x9
        or code == 0xA
        or code == 0xD
        or 0x20 <= code <= 0xD7FF
        or 0xE000 <= code <= 0xFFFD
        or 0x10000 <= code <= 0x10FFFF
    )


def sanitize_xml(raw: bytes) -> bytes:
    text = raw.decode("utf-8", errors="replace")
    cleaned = _INVALID_XML_CHARS.sub(
        lambda m: "" if not _is_valid_codepoint(m) else m.group(), text
    )
    return cleaned.encode("utf-8")


def resolve_image_path(sys_dir: Path, raw_path: str) -> Path:
    p = raw_path.strip()
    if p.startswith("/"):
        return Path(p)
    return sys_dir / p


def parse_gamelist(gamelist_path: Path):
    raw = gamelist_path.read_bytes()
    cleaned = sanitize_xml(raw)
    root = ET.fromstring(cleaned)
    return root.findall(".//game")


def ask_extraction_config():
    """
    Depuis la MAJ Recalbox, la balise <logo> remplace <thumbnail>/<image>.
    Mais selon les gamelist.xml (anciennes versions / variantes), certains systèmes
    peuvent ne contenir que <image> ou <thumbnail>.

    Retourne [(tag, folder)] dans un ordre de fallback :
      - logo (prioritaire)
      - image
      - thumbnail
    """
    return [("logo", "")]


def extract_system(
    sys_dir,
    systems_out,
    tag_configs,
    sys_index,
    total_systems,
    log_file,
    progress_cb=None,
    progress_global_total_images: int = 0,
    progress_global_done_offset: int = 0,
):
    sys_name = sys_dir.name
    print(f"\n[{sys_index}/{total_systems}] 📁 {sys_name}")

    try:
        games = parse_gamelist(sys_dir / "gamelist.xml")
    except ET.ParseError as e:
        msg = f"[{sys_name}] {tr('ext_xml_err')} : {e}"
        print(f"   ❌ {msg}")
        log_file.write(msg + "\n")
        return 0, 0, 0, 0

    total = len(games)
    copied = 0
    skipped = 0
    missing = 0
    print(f"   🎮 {total} {tr('ext_games_found')}")
    total_images_global = max(progress_global_total_images, 1)

    for i, game in enumerate(games, 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break
        path_elem = game.find("path")
        if path_elem is None:
            missing += 1
            continue

        raw_path = unquote(path_elem.text or "").strip()
        if not raw_path:
            missing += 1
            continue

        game_name = sanitize_filename(Path(raw_path).stem)

        for tag, folder in tag_configs:
            img_elem = game.find(tag)
            if img_elem is None or not (img_elem.text or "").strip():
                missing += 1
                continue

            image_raw = unquote(img_elem.text.strip())
            src_image = resolve_image_path(sys_dir, image_raw)
            ext = src_image.suffix or ".png"

            dst_dir = systems_out / sys_name / folder
            dst_dir.mkdir(parents=True, exist_ok=True)
            dst_image = dst_dir / f"{game_name}{ext}"
            dst_image = _alpha_subdir_if_needed(dst_image)

            current_file = f"{game_name}{ext}"
            if progress_cb is not None:
                # Détail “images copiées/total + fichier en cours”.
                # On envoie la progression globale (toutes les images, tous systèmes)
                # pour que le % soit cohérent.
                progress_cb(
                    "extraction_imgs",
                    progress_global_done_offset + copied + skipped,
                    progress_global_total_images,
                    f"{copied}/{total} — {current_file}",
                )

            if dst_image.exists():
                skipped += 1
                print(
                    f"   {i:4d}/{total} ⏭️  [{folder}] {game_name}{ext} ({tr('ext_already')})"
                )
                continue

            if not src_image.exists():
                missing += 1
                print(
                    f"   {i:4d}/{total} ⚠️  {tr('ext_missing_tag')} ({tag}): {src_image.name}"
                )
                log_file.write(f"[{sys_name}] {game_name} ({tag}) → {src_image}\n")
                continue

            shutil.copy2(src_image, dst_image)
            copied += 1

            if progress_cb is not None:
                # Progression globale après copie.
                progress_cb(
                    "extraction_imgs",
                    progress_global_done_offset + copied + skipped,
                    progress_global_total_images,
                    f"{copied}/{total} — {current_file}",
                )

            print(f"   {i:4d}/{total} ✅ [{folder}] {game_name}{ext}")
            time.sleep(0.003)
            # Image trouvée → ne pas essayer les autres balises
            break

    print(
        f"   → ✅ {copied} {tr('ext_summary_copied')} | "
        f"⏭️  {skipped} {tr('ext_summary_skip')} | "
        f"⚠️  {missing} {tr('ext_summary_miss')}"
    )
    return total, copied, skipped, missing


def ask_system_selection(roms_root: Path):
    """
    Liste les systèmes détectés dans roms_root et propose à l'utilisateur
    de choisir lesquels traiter. Retourne la liste des Path sélectionnés,
    ou None si l'utilisateur revient en arrière.
    """
    systems = sorted(
        [
            d
            for d in roms_root.iterdir()
            if d.is_dir() and (d / "gamelist.xml").exists()
        ],
        key=lambda d: d.name.lower(),
    )

    sep("─")
    print(f"\n{tr('sys_sel_title')}")
    sep("─")

    if not systems:
        print(tr("sys_sel_none"))
        return None

    for i, s in enumerate(systems, 1):
        print(f"  {i:3d}  →  {s.name}")
    print()
    print(f"  {tr('sys_sel_prompt')}")
    print()
    print(f"  1  →  {tr('sys_sel_opt_all')}")
    print(f"  2  →  {tr('sys_sel_opt_pick')}")
    print(f"  0  →  {tr('back')}")
    print()

    while True:
        raw = input("  > ").strip()
        if raw == "0":
            return None
        if raw == "1":
            print(tr("sys_sel_selected")(len(systems)))
            return systems
        if raw == "2":
            break
        print(tr("sys_sel_warn"))

    # Sélection manuelle
    print()
    print(f"  {tr('sys_sel_pick_hint')}")
    print()
    while True:
        raw = input("  > ").strip()
        if raw == "0":
            print(tr("sys_sel_selected")(len(systems)))
            return systems
        parts = [p.strip() for p in raw.split(",") if p.strip()]
        selected = []
        valid = True
        seen = set()
        for p in parts:
            if not p.isdigit():
                valid = False
                break
            idx = int(p)
            if idx < 1 or idx > len(systems) or idx in seen:
                valid = False
                break
            seen.add(idx)
            selected.append(systems[idx - 1])
        if valid and selected:
            print(tr("sys_sel_selected")(len(selected)))
            return selected
        print(tr("sys_sel_warn"))


def run_extraction(
    roms_root,
    systems_out,
    tag_configs,
    log_file,
    selected_systems=None,
    progress_cb=None,
    listen_keyboard: bool = True,
):
    if selected_systems is not None:
        systems = selected_systems
    else:
        systems = [
            d
            for d in roms_root.iterdir()
            if d.is_dir() and (d / "gamelist.xml").exists()
        ]
    total_systems = len(systems)
    print(f"\n📂 {total_systems} {tr('ext_systems')}")
    print(tr("pause_hint"))

    # Pour que le % reflète la progression globale (images),
    # on calcule le total global de "games" sur tous les systèmes.
    global_total_images = 0
    for sys_dir in systems:
        try:
            games = parse_gamelist(sys_dir / "gamelist.xml")
            global_total_images += len(games)
        except Exception:
            # Si un système a un XML invalide, il ne contribuera pas au total.
            pass
    global_total_images = max(global_total_images, 1)

    # compteur global d’images "faites" = copiées + déjà présentes (skipped)
    global_done_images = 0

    PAUSE.start(listen_keyboard=listen_keyboard)
    grand = {"games": 0, "copied": 0, "skipped": 0, "missing": 0, "done": 0}
    for idx, sys_dir in enumerate(systems, 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break

        if progress_cb is not None:
            sys_label = f"{idx}/{total_systems} — {sys_dir.name}"
            progress_cb(
                "extraction",
                global_done_images,
                global_total_images,
                sys_label,
            )

        g, c, s, m = extract_system(
            sys_dir,
            systems_out,
            tag_configs,
            idx,
            total_systems,
            log_file,
            progress_cb=progress_cb,
            progress_global_total_images=global_total_images,
            progress_global_done_offset=global_done_images,
        )

        global_done_images += c + s

        grand["games"] += g
        grand["copied"] += c
        grand["skipped"] += s
        grand["missing"] += m
        if g > 0:
            grand["done"] += 1

    PAUSE.stop()

    return grand, total_systems


# ─────────────────────────────────────────────────────────────────────────────
#  CONVERSION 128x32
# ─────────────────────────────────────────────────────────────────────────────


def convert_image_file(src: Path, dst: Path):
    from PIL import Image

    with Image.open(src) as img:
        img = img.convert("RGBA")
        orig_w, orig_h = img.size

        ratio = min(TARGET_W / orig_w, TARGET_H / orig_h)
        new_w = int(orig_w * ratio)
        new_h = int(orig_h * ratio)

        resized = img.resize((new_w, new_h), Image.LANCZOS)
        canvas = Image.new("RGBA", (TARGET_W, TARGET_H), (0, 0, 0, 255))
        offset_x = (TARGET_W - new_w) // 2
        offset_y = (TARGET_H - new_h) // 2
        canvas.paste(resized, (offset_x, offset_y), resized)

        rgb_img = canvas.convert("RGB")
        rgb_img.save(dst, "PNG", optimize=False, interlace=False)

        # Génère aussi un raw565 (4096 pixels * 2 bytes = 8192 bytes)
        raw_path = dst.with_suffix(".raw565")
        raw_bytes = rgb_img.tobytes()  # row-major
        with open(raw_path, "wb") as f:
            for i in range(0, len(raw_bytes), 3):
                r, g, b = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack("<H", rgb565))


def convert_png_to_raw565_only(src_png: Path, dst_raw565=None) -> Path:
    """
    Convertit un PNG en .raw565 (RGB565) sans modifier le PNG d'origine.
    Même logique de centrage/redimensionnement que convert_image_file().
    """
    from PIL import Image

    raw_path = dst_raw565 if dst_raw565 is not None else src_png.with_suffix(".raw565")

    with Image.open(src_png) as img:
        img = img.convert("RGBA")
        orig_w, orig_h = img.size

        ratio = min(TARGET_W / orig_w, TARGET_H / orig_h)
        new_w = max(1, int(orig_w * ratio))
        new_h = max(1, int(orig_h * ratio))

        resized = img.resize((new_w, new_h), Image.LANCZOS)
        canvas = Image.new("RGBA", (TARGET_W, TARGET_H), (0, 0, 0, 255))
        offset_x = (TARGET_W - new_w) // 2
        offset_y = (TARGET_H - new_h) // 2
        canvas.paste(resized, (offset_x, offset_y), resized)

        rgb_img = canvas.convert("RGB")
        raw_bytes = rgb_img.tobytes()  # row-major

        # Écrire dans le sous-dossier alphabétique (sauf _defaults)
        sub_raw_path = _alpha_subdir_if_needed(raw_path)

        with open(sub_raw_path, "wb") as f:
            for i in range(0, len(raw_bytes), 3):
                r, g, b = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack("<H", rgb565))

    return sub_raw_path


def _detect_background_color(img_rgba) -> tuple[int, int, int]:
    """
    Detecte la couleur de fond par mediane sur des blocs aux 4 coins de
    l'image -- ne suppose JAMAIS un fond sombre (repond a la question
    reelle "quelle est la couleur de fond, sombre ou pas" plutot que de
    forcer du noir comme convert_png_to_raw565_only()).
    """
    w, h = img_rgba.size
    n = min(4, w, h)
    px = img_rgba.load()
    samples: list[tuple[int, int, int]] = []
    for cx, cy in ((0, 0), (w - n, 0), (0, h - n), (w - n, h - n)):
        for x in range(cx, cx + n):
            for y in range(cy, cy + n):
                r, g, b, a = px[x, y]
                if a > 0:
                    samples.append((r, g, b))
    if not samples:
        return (0, 0, 0)
    rs = sorted(s[0] for s in samples)
    gs = sorted(s[1] for s in samples)
    bs = sorted(s[2] for s in samples)
    mid = len(samples) // 2
    return (rs[mid], gs[mid], bs[mid])


def _flatten_background_noise(img_rgba, bg_color: tuple[int, int, int], threshold: float = 24.0):
    """
    Aplatit en couleur de fond EXACTE tout pixel opaque dont la distance
    couleur a bg_color est sous threshold -- retire le bruit
    d'antialiasing colore residuel autour d'un fond cense etre uni, sans
    toucher aux details nets (contraste eleve avec le fond).
    """
    img = img_rgba.copy()
    px = img.load()
    w, h = img.size
    br, bgn, bb = bg_color
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            dist = ((r - br) ** 2 + (g - bgn) ** 2 + (b - bb) ** 2) ** 0.5
            if dist < threshold:
                px[x, y] = (br, bgn, bb, a)
    return img


def _pixel_perfect_fit_dims(orig_w: int, orig_h: int, target_w: int, target_h: int):
    """
    Dimensions "fit" a echelle ENTIERE (jamais de redimensionnement
    fractionnaire) : agrandissement -> facteur entier (NEAREST, aucun
    flou), reduction -> diviseur entier (BOX, moyenne les blocs de pixels
    au lieu du LANCZOS fractionnaire de convert_png_to_raw565_only()).
    Reimplementation pure Python du mode "fit" de
    DMDEngine._pixel_perfect_dims() (project dmd_gif_converter).
    """
    import math

    from PIL import Image

    if orig_w <= target_w and orig_h <= target_h:
        factor = max(1, min(target_w // orig_w, target_h // orig_h))
        return orig_w * factor, orig_h * factor, Image.NEAREST

    divisor = max(1, math.ceil(max(orig_w / target_w, orig_h / target_h)))
    new_w = max(1, orig_w // divisor)
    new_h = max(1, orig_h // divisor)
    return new_w, new_h, Image.BOX


def convert_png_to_raw565_cleaned(src_png: Path, dst_raw565=None, background_threshold: float = 24.0) -> Path:
    """
    Variante "nettoyee" de convert_png_to_raw565_only(), reservee a
    l'image de secours (_defaults) : redimensionnement pixel-perfect
    (_pixel_perfect_fit_dims) + aplatissement du bruit de fond
    (_flatten_background_noise) sur un canvas de letterbox rempli de la
    couleur de fond DETECTEE (pas force en noir). Perimetre volontairement
    limite a l'image de secours -- convert_png_to_raw565_only() reste
    inchangee et utilisee partout ailleurs (visuels ROM).
    """
    from PIL import Image

    raw_path = dst_raw565 if dst_raw565 is not None else src_png.with_suffix(".raw565")

    with Image.open(src_png) as img:
        img = img.convert("RGBA")
        orig_w, orig_h = img.size

        bg_color = _detect_background_color(img)
        new_w, new_h, resample = _pixel_perfect_fit_dims(orig_w, orig_h, TARGET_W, TARGET_H)
        resized = img.resize((new_w, new_h), resample)
        cleaned = _flatten_background_noise(resized, bg_color, background_threshold)

        canvas = Image.new("RGBA", (TARGET_W, TARGET_H), (*bg_color, 255))
        offset_x = (TARGET_W - new_w) // 2
        offset_y = (TARGET_H - new_h) // 2
        canvas.paste(cleaned, (offset_x, offset_y), cleaned)

        rgb_img = canvas.convert("RGB")
        raw_bytes = rgb_img.tobytes()  # row-major

        sub_raw_path = _alpha_subdir_if_needed(raw_path)
        with open(sub_raw_path, "wb") as f:
            for i in range(0, len(raw_bytes), 3):
                r, g, b = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack("<H", rgb565))

    return sub_raw_path


def set_default_fallback_image(src_image: Path, sd_dir: Path) -> Path:
    """
    Convertit src_image (PNG ou autre format lisible par Pillow) en
    systems/_defaults/default.raw565 dans sd_dir -- l'image de secours que
    le firmware affiche quand aucun visuel specifique n'est trouve
    (chemin code en dur cote firmware : /systems/_defaults/default.raw565).

    Utilise convert_png_to_raw565_cleaned() (pixel-perfect + aplatissement
    du bruit de fond) plutot que convert_png_to_raw565_only() -- couvre a
    la fois les visuels bundles ET toute image personnalisee choisie par
    l'utilisateur (meme fonction dans les 2 cas). Le dossier "_defaults"
    reste exclu du bucketing alphabetique par _alpha_subdir_if_needed(),
    donc le chemin de sortie n'est jamais modifie/deplace.
    """
    dst = sd_dir / "systems" / "_defaults" / "default.raw565"
    dst.parent.mkdir(parents=True, exist_ok=True)
    return convert_png_to_raw565_cleaned(Path(src_image), dst)


def convert_gif_to_raw565pack_meta(
    gif_path: Path, dst_raw565pack=None, dst_meta=None
) -> None:
    """
    Génère un pack unique:
      - NAME.raw565pack : frames RGB565 concaténées (frame0, frame1, ...)
      - NAME.meta        : suite uint16 little-endian = delay_ms par frame

    Important: on fait un "centrage + redimensionnement" identique aux PNG
    sur un canvas 128x32. Le timing est conservé via frame.info['duration'].
    """
    from PIL import Image, ImageSequence

    raw_pack_path = (
        dst_raw565pack
        if dst_raw565pack is not None
        else gif_path.with_suffix(".raw565pack")
    )
    meta_path = dst_meta if dst_meta is not None else gif_path.with_suffix(".meta")

    frame_bytes = TARGET_W * TARGET_H * 2  # 128*32*2 = 8192

    with Image.open(gif_path) as im:
        im.seek(0)

        # Ouvre en écriture; on reconstruit à chaque conversion.
        # Sous-dossier alphabétique (sauf _defaults)
        sub_raw_pack_path = _alpha_subdir_if_needed(raw_pack_path)
        sub_meta_path = _alpha_subdir_if_needed(meta_path)

        with open(sub_raw_pack_path, "wb") as pack_f, open(
            sub_meta_path, "wb"
        ) as meta_f:
            for frame in ImageSequence.Iterator(im):
                # Frame -> canvas 128x32
                frame_rgba = frame.convert("RGBA")
                orig_w, orig_h = frame_rgba.size

                ratio = min(TARGET_W / orig_w, TARGET_H / orig_h)
                new_w = max(1, int(orig_w * ratio))
                new_h = max(1, int(orig_h * ratio))

                resized = frame_rgba.resize((new_w, new_h), Image.LANCZOS)
                canvas = Image.new("RGBA", (TARGET_W, TARGET_H), (0, 0, 0, 255))
                offset_x = (TARGET_W - new_w) // 2
                offset_y = (TARGET_H - new_h) // 2
                canvas.paste(resized, (offset_x, offset_y), resized)

                rgb_img = canvas.convert("RGB")
                raw_bytes = rgb_img.tobytes()

                for i in range(0, len(raw_bytes), 3):
                    r, g, b = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    pack_f.write(struct.pack("<H", rgb565))

                # delay_ms per frame (fallback = durée globale GIF)
                fallback_ms = im.info.get("duration", 50)
                try:
                    fallback_ms = int(fallback_ms)
                except Exception:
                    fallback_ms = 10
                if fallback_ms <= 0:
                    fallback_ms = 10
                if fallback_ms > 0xFFFF:
                    fallback_ms = 0xFFFF

                delay_ms = frame.info.get("duration", fallback_ms)
                try:
                    delay_ms = int(delay_ms)
                except Exception:
                    delay_ms = fallback_ms
                if delay_ms <= 0:
                    delay_ms = fallback_ms
                if delay_ms > 0xFFFF:
                    delay_ms = 0xFFFF

                meta_f.write(struct.pack("<H", delay_ms))


def open_folder_in_explorer(folder: Path) -> None:
    try:
        folder = folder.resolve()
    except Exception:
        folder = folder

    try:
        if os.name == "nt":
            os.startfile(str(folder))  # type: ignore[attr-defined]
            return
    except Exception:
        pass

    # fallback (non-Windows)
    try:
        import subprocess

        subprocess.Popen(["xdg-open", str(folder)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)  # type: ignore[call-arg]
    except Exception:
        pass


def popup_confirm_before_conversion(out_dir: Path) -> bool:
    """
    Popup Tkinter (bloquant) :
      - Continuer -> True
      - Explorer -> ouvre out_dir
      - X -> False
    Si Tkinter échoue, on retourne True (fallback).
    """
    try:
        import tkinter as tk
    except Exception:
        return True

    ok_holder = {"ok": False}

    root = tk.Tk()
    root.title(tr("popup_conv_title"))
    root.resizable(False, False)

    # Message
    msg = tk.Label(
        root,
        text=tr("popup_conv_msg")(str(out_dir)),
        justify="left",
        padx=14,
        pady=12,
    )
    msg.pack()

    buttons = tk.Frame(root, padx=10, pady=10)
    buttons.pack()

    def on_explore():
        open_folder_in_explorer(out_dir)

    def on_continue():
        ok_holder["ok"] = True
        root.destroy()

    def on_close():
        root.destroy()

    explore_btn = tk.Button(
        buttons, text=tr("popup_conv_explore"), width=18, command=on_explore
    )
    explore_btn.grid(row=0, column=0, padx=8, pady=6)

    cont_btn = tk.Button(buttons, text=tr("popup_conv_continue"), width=18, command=on_continue)
    cont_btn.grid(row=0, column=1, padx=8, pady=6)

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()
    return bool(ok_holder["ok"])


def run_conversion(
    systems_dir: Path,
    progress_cb=None,
    listen_keyboard: bool = True,
    output_dir: Path | None = None,
    system_names=None,
):
    if not PIL_AVAILABLE:
        print(tr("conv_no_pillow"))
        return

    print(tr("conv_png_only"))

    png_files = list(systems_dir.rglob("*.png"))
    gif_files = list(systems_dir.rglob("*.gif"))

    if system_names is not None:
        wanted = set(system_names)

        def system_key(p: Path) -> str:
            rel = p.relative_to(systems_dir)
            # Cas "images directement dans systems_dir" => rel.parts == (filename,)
            # On mappe alors le "système" vers systems_dir.name.
            if len(rel.parts) <= 1:
                return systems_dir.name
            return rel.parts[0]

        png_files = [p for p in png_files if system_key(p) in wanted]
        gif_files = [p for p in gif_files if system_key(p) in wanted]
    total_png = len(png_files)
    total_gif = len(gif_files)
    total = total_png + total_gif
    done = errors = 0

    # Assure que l'UI passe en "Conversion" même si total == 0
    if progress_cb is not None:
        progress_cb(
            "conversion",
            0,
            max(total, 1),
            tr("progress_conv_idle") if total == 0 else tr("progress_conv_running"),
        )

    if gif_files:
        print(f"   🎞️  {len(gif_files)} {tr('conv_gif_info')}")
    print(f"   🖼️  {total_png} {tr('conv_png_count')}")
    print(tr("pause_hint"))
    sep("─")

    PAUSE.start(listen_keyboard=listen_keyboard)
    for i, src in enumerate(png_files, 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break

        if progress_cb is not None:
            # Envoi : system|filename (pour que la GUI affiche system en ligne 1,
            # et le filename tronqué en ligne 2 sans pousser la mise en page)
            progress_cb("conversion", i, total, f"{src.parent.name}|{src.name}")

        try:
            # On convertit aussi les PNG dans _defaults/ pour générer leur raw565.
            # (Sinon le firmware peut afficher le PNG mais pas le mask lors du fallback raw.)
            if output_dir is None:
                convert_image_file(src, src)
            else:
                rel = src.relative_to(systems_dir)
                dst_png = output_dir / rel
                dst_png.parent.mkdir(parents=True, exist_ok=True)
                convert_image_file(src, dst_png)
            done += 1
            print(f"   {i:5d}/{total} ✅ {src.relative_to(systems_dir)}")
        except Exception as e:
            errors += 1
            print(f"   {i:5d}/{total} ❌ {src.relative_to(systems_dir)} — {e}")

    # GIF -> raw565pack + meta (timing via frame duration)
    gif_done = 0
    gif_errors = 0
    if gif_files and not PAUSE.should_stop():
        print(tr("conv_gif_converted")(len(gif_files)))
        for j, gif_src in enumerate(gif_files, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break

            if progress_cb is not None:
                progress_cb(
                    "conversion",
                    total_png + j,
                    max(total, 1),
                    f"{gif_src.parent.name}|{gif_src.name}",
                )

            try:
                convert_gif_to_raw565pack_meta(gif_src)
                gif_done += 1
                print(
                    f"   {j:5d}/{len(gif_files)} ✅ {gif_src.relative_to(systems_dir)}"
                )
            except Exception as e:
                gif_errors += 1
                errors += 1
                print(
                    f"   {j:5d}/{len(gif_files)} ❌ {gif_src.relative_to(systems_dir)} — {e}"
                )

    PAUSE.stop()

    sep("─")
    print(
        f"✅ {done} {tr('conv_summary_done')} | "
        f"❌ {errors} {tr('conv_summary_err')} | "
        f"🎞️  {gif_done} {tr('conv_summary_gif')}"
    )


def run_conversion_raw_only(
    systems_dir: Path,
    progress_cb=None,
    listen_keyboard: bool = True,
    output_dir=None,
    system_names=None,
):
    """
    Conversion raw-only :
      - PNG  -> *.raw565 (sans ré-écrire les PNG)
      - GIF  -> *.raw565pack + *.meta (timing via duration frame)
    """
    if not PIL_AVAILABLE:
        print(tr("conv_no_pillow"))
        return 0

    png_files = list(systems_dir.rglob("*.png"))
    gif_files = list(systems_dir.rglob("*.gif"))

    if system_names is not None:
        wanted = set(system_names)
        png_files = [
            p for p in png_files if p.relative_to(systems_dir).parts[0] in wanted
        ]
        gif_files = [
            p for p in gif_files if p.relative_to(systems_dir).parts[0] in wanted
        ]
    total_png = len(png_files)
    total_gif = len(gif_files)
    total = total_png + total_gif
    done = errors = 0

    if progress_cb is not None:
        progress_cb(
            "conversion",
            0,
            max(total, 1),
            tr("progress_conv_idle") if total == 0 else tr("progress_conv_raw_running"),
        )

    print(f"\n   🖼️  {total_png} {tr('conv_png_count')}")
    if gif_files:
        print(f"   🎞️  {total_gif} {tr('conv_gif_info')} (pack/meta)")
    print(tr("pause_hint"))
    sep("─")

    PAUSE.start(listen_keyboard=listen_keyboard)

    for i, src in enumerate(png_files, 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break

        if progress_cb is not None:
            progress_cb(
                "conversion",
                i,
                max(total, 1),
                f"{src.parent.name}|{src.name}",
            )

        try:
            if output_dir is None:
                convert_png_to_raw565_only(src)
            else:
                rel = src.relative_to(systems_dir)
                dst_png = output_dir / rel
                dst_png.parent.mkdir(parents=True, exist_ok=True)
                convert_png_to_raw565_only(
                    src, dst_raw565=dst_png.with_suffix(".raw565")
                )
            done += 1
            print(f"   {i:5d}/{total} ✅ {src.relative_to(systems_dir)}")
        except Exception as e:
            errors += 1
            print(f"   {i:5d}/{total} ❌ {src.relative_to(systems_dir)} — {e}")

    gif_done = 0
    if gif_files and not PAUSE.should_stop():
        print(tr("conv_gif_converted")(len(gif_files)))
        for j, gif_src in enumerate(gif_files, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break

            if progress_cb is not None:
                progress_cb(
                    "conversion",
                    total_png + j,
                    max(total, 1),
                    f"{gif_src.parent.name}|{gif_src.name}",
                )

            try:
                if output_dir is None:
                    convert_gif_to_raw565pack_meta(gif_src)
                else:
                    rel = gif_src.relative_to(systems_dir)
                    dst_png = output_dir / rel
                    dst_png.parent.mkdir(parents=True, exist_ok=True)
                    convert_gif_to_raw565pack_meta(
                        gif_src,
                        dst_raw565pack=dst_png.with_suffix(".raw565pack"),
                        dst_meta=dst_png.with_suffix(".meta"),
                    )
                gif_done += 1
                print(
                    f"   {j:5d}/{len(gif_files)} ✅ {gif_src.relative_to(systems_dir)}"
                )
            except Exception as e:
                errors += 1
                print(
                    f"   {j:5d}/{len(gif_files)} ❌ {gif_src.relative_to(systems_dir)} — {e}"
                )

    PAUSE.stop()

    sep("─")
    print(
        f"✅ {done} {tr('conv_summary_done')} | "
        f"❌ {errors} {tr('conv_summary_err')} | "
        f"🎞️  {gif_done} {tr('conv_summary_gif')}"
    )
    return errors


# ─────────────────────────────────────────────────────────────────────────────
#  BUILD GAMES CACHE — index bigramme 702 entrees
#
#  Structure de l'index par systeme : 702 x 4 bytes (offsets absolus)
#  Index 0       = '#'  (chiffres, tirets, etc.)
#  Index 1       = 'A'  (jeux commencant par A + caractere non-lettre)
#  Index 2..27   = 'AA'..'AZ'
#  Index 28      = 'B'
#  Index 29..54  = 'BA'..'BZ'
#  ...
#  Index 703     = 'Z'
#  Total         = 1 + 26 * 27 = 703  (indices 0..702)
# ─────────────────────────────────────────────────────────────────────────────

NB_IDX = 703  # nombre total d'entrees dans la table bigramme


def bigram_index(name):
    """
    Calcule l'index bigramme (0..702) pour un nom de jeu.
    - Commence par non-lettre  -> 0  (#)
    - Commence par A seul (ex: "a1")   -> 1
    - Commence par AA..AZ      -> 2..27
    - Commence par B seul      -> 28
    - etc.
    """
    if not name:
        return 0
    c1 = name[0].upper()
    if not c1.isalpha():
        return 0  # '#'
    i1 = ord(c1) - ord("A")  # 0..25
    base = 1 + i1 * 27  # base pour la lettre c1

    if len(name) < 2:
        return base  # lettre seule

    c2 = name[1].upper()
    if not c2.isalpha():
        return base  # lettre seule (2eme char non-lettre)

    i2 = ord(c2) - ord("A")  # 0..25
    return base + i2 + 1  # base + 1..26


def collect_games_for_folder(systems_dir: Path, folder: str):
    """
    Collecte les jeux pour un dossier image specifique.
    folder = "" pour la racine du systeme,
    folder = "" pour la racine du système (pas de sous-dossier).
    Retourne un dict { sysname: defaultdict(list) }
    """
    result = {}
    for sysname in sorted(os.listdir(systems_dir)):
        syspath = systems_dir / sysname
        if not syspath.is_dir() or sysname.lower() == "_defaults":
            continue

        if folder:
            scan_dir = syspath / folder
            if not scan_dir.exists() or not scan_dir.is_dir():
                continue
            scan_dirs = [scan_dir]
        else:
            scan_dirs = [syspath]

        games = {}
        try:
            for scan_dir in scan_dirs:
                # Parcours récursif des sous-dossiers (A-Z, #, etc.)
                for fpath in scan_dir.rglob("*"):
                    if not fpath.is_file():
                        continue
                    fname = fpath.name
                    name, ext = os.path.splitext(fname)
                    ext = ext.lower()
                    if ext not in EXTENSIONS_CACHE:
                        continue
                    ftype = EXTENSIONS_CACHE[ext]
                    key = name.lower()
                    if key not in games or ftype == 0x67:
                        games[key] = (name, ftype)
        except PermissionError:
            continue

        if not games:
            continue

        by_idx = defaultdict(list)
        for key in sorted(games.keys()):
            _orig, ftype = games[key]
            by_idx[bigram_index(key)].append((key, ftype))

        result[sysname] = by_idx

    return result


def _write_cache_binary(data: dict, output_path: Path):
    """Ecrit un games_cache.bin avec index bigramme 702 entrees."""
    total_systems = len(data)
    total_games = sum(len(gl) for by_idx in data.values() for gl in by_idx.values())

    if total_systems == 0:
        print(tr("cache_no_sys"))
        return 0, 0

    HEADER_SIZE = 4 + total_systems * 36
    data_buf = bytearray()
    sys_offsets = {}

    for sysname, by_idx in data.items():
        sys_offsets[sysname] = len(data_buf)
        letter_table_pos = len(data_buf)
        data_buf += b"\x00" * (NB_IDX * 4)

        idx_offsets = [0] * NB_IDX
        for li in range(NB_IDX):
            games = by_idx.get(li, [])
            if not games:
                continue
            idx_offsets[li] = HEADER_SIZE + len(data_buf)
            for gamename, gtype in games:
                name_bytes = gamename.lower().encode("utf-8") + b"\x00"
                data_buf += bytes([gtype]) + name_bytes

        for li in range(NB_IDX):
            pos = letter_table_pos + li * 4
            data_buf[pos : pos + 4] = struct.pack("<I", idx_offsets[li])

    with open(output_path, "wb") as f:
        f.write(struct.pack("<I", total_systems))
        for sysname in data.keys():
            name_b = sysname.encode("utf-8")[:31].ljust(32, b"\x00")
            offset = HEADER_SIZE + sys_offsets[sysname]
            f.write(name_b)
            f.write(struct.pack("<I", offset))
        f.write(data_buf)

    size_kb = os.path.getsize(output_path) / 1024
    print(f"[OK] {output_path}")
    print(
        f"     {total_systems} {tr('cache_found_sys')} {total_games} {tr('cache_found_games')} | {size_kb:.1f} {tr('cache_size')}"
    )
    return total_systems, total_games


def build_cache(systems_dir: Path, output_dir: Path, progress_cb=None):
    """
    Genere UN SEUL games_cache.bin contenant toutes les images
    de tous les systemes, y compris celles dans les sous-dossiers
    alphabetiques (#, A..Z). Les fichiers images sont trouves
    recursivement dans chaque dossier systeme.
    """
    print(f"{tr('cache_scan')}{systems_dir}")

    data: dict[str, dict[int, list[tuple[str, int]]]] = {}
    total_files = 0

    for sysname in os.listdir(systems_dir):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break
        syspath = systems_dir / sysname
        if not syspath.is_dir() or sysname.lower() == "_defaults":
            continue

        by_idx: dict[int, list[tuple[str, int]]] = {}
        # Parcours recursif pour trouver toutes les images (dans #, A..Z ou racine)
        for ext, code in EXTENSIONS_CACHE.items():
            for f in syspath.rglob(f"*{ext}"):
                try:
                    fs = f.stat()
                    stem = f.stem
                    # Index bigramme compatible avec _write_cache_binary (NB_IDX=703)
                    li = _calc_bigram_idx(stem)
                    by_idx.setdefault(li, []).append((stem, code))
                    total_files += 1
                except OSError:
                    pass

        if by_idx:
            data[sysname] = by_idx

        if progress_cb is not None:
            progress_cb("cache", len(data), 1, sysname)

    if not data:
        print(tr("cache_no_sys"))
        return []

    total_sys = len(data)
    output_path = output_dir / "games_cache.bin"
    print(tr("cache_build_header")(total_sys, total_files))
    nb_sys, nb_games = _write_cache_binary(data, output_path)

    print(tr("cache_build_done")(output_path.name, nb_sys, nb_games))
    return [(output_path, nb_sys, nb_games)]


def _ask_roms_and_config():
    """Boucle commune : demande roms + sélection systèmes.
    Depuis la MAJ Recalbox, la config image est fixe : balise <logo> -> racine du système.
    Retourne (roms_root, tag_configs, selected_systems) ou (None, None, None)."""
    while True:
        print(tr("ext_roms_where"))
        sep("─")
        roms_root = ask_path()
        if roms_root is None:
            return None, None, None
        print(f"{tr('roms_ok')}{roms_root}")

        selected_systems = ask_system_selection(roms_root)
        if selected_systems is None:
            print(tr("back_roms"))
            continue

        tag_configs = ask_extraction_config()  # retourne toujours [("logo", "")]
        return roms_root, tag_configs, selected_systems


def _write_log(log_file, roms_root, grand):
    log_file.write(tr("ext_log_header"))
    log_file.write(f"{tr('ext_log_source')}{roms_root}\n\n")
    log_file.write(tr("ext_log_summary"))
    log_file.write(f"{tr('ext_log_games')}{grand['games']}\n")
    log_file.write(f"{tr('ext_log_copied')}{grand['copied']}\n")
    log_file.write(f"{tr('ext_log_skipped')}{grand['skipped']}\n")
    log_file.write(f"{tr('ext_log_missing')}{grand['missing']}\n")


def mode_full(sd_dir: Path):
    title(tr("mode1_title"))
    prepare_sd_card(sd_dir)
    # Meme langue que l'outil, transmise au DMD via config.ini -- pour que le
    # premier lancement se fasse deja dans la langue de l'utilisateur (page
    # AP + bannieres ecran), sans ressaisie manuelle. Uniquement en Mode 1
    # (pipeline complet "premier lancement"), pas dans les autres modes.
    write_dmd_language(sd_dir, CURRENT_LANG)

    # ── Installation/mise a jour des scripts Recalbox ───────────────────────
    # Deplacee en tout premier (avant extraction/téléchargement/conversion) :
    # le script marquee est indispensable au bon fonctionnement de
    # l'appareil, pas juste une option -- autant tenter cette etape tant que
    # l'utilisateur est encore devant son ecran plutot que de l'enterrer en
    # fin de pipeline. Detection auto ("RECALBOX") en priorite, sinon repli
    # sur la derniere cible enregistree (Mode 9). Si aucune des deux ne
    # donne de resultat, message explicite (pas juste un skip silencieux) :
    # l'utilisateur peut relancer plus tard via le Mode 9 dedie.
    recalbox_target = detect_recalbox_share() or prefs.get("recalbox_ip")
    if recalbox_target:
        print(tr("mode1_scripts_phase"))
        download_recalbox_scripts(recalbox_target, listen_keyboard=True)
    else:
        print(tr("mode1_scripts_skip"))

    systems_out = sd_dir / "systems"
    systems_out.mkdir(parents=True, exist_ok=True)

    roms_root, tag_configs, selected_systems = _ask_roms_and_config()
    if roms_root is None:
        print(tr("back_main"))
        return

    sep("─")
    print(f"\n{tr('ext_title')}")
    log_path = sd_dir / "images_manquantes.txt"
    with open(log_path, "w", encoding="utf-8") as log_file:
        grand, _ = run_extraction(
            roms_root, systems_out, tag_configs, log_file, selected_systems
        )
        _write_log(log_file, roms_root, grand)

    sep("─")
    if PAUSE.should_stop():
        sep()
        print(tr("done"))
        print(tr("pause_stopping"))
        return

    # ── Téléchargement _defaults depuis GitHub (AVANT la conversion) ────────
    print(tr("mode1_dl_phase"))
    download_defaults(sd_dir)

    sep("─")
    if PAUSE.should_stop():
        sep()
        print(tr("done"))
        print(tr("pause_stopping"))
        return

    # ── Conversion raw565 (PNG et GIF) ──────────────────────────────────────
    print(f"\n{tr('conv_title')}")
    PAUSE.state = PAUSE.RUNNING
    conv_errors = run_conversion_raw_only(systems_out)

    # ── Suppression des .png et .gif après conversion réussie ──────────────
    removed_files = 0
    removed_errors = 0
    for ext in ("*.png", "*.gif"):
        for f in Path(systems_out).rglob(ext):
            raw565_path = f.parent / (
                f.stem + (".raw565" if f.suffix == ".png" else ".raw565pack")
            )
            if raw565_path.exists():
                try:
                    f.unlink()
                    removed_files += 1
                except Exception:
                    removed_errors += 1
    print(tr("mode1_removed_files")(removed_files))
    if removed_errors:
        print(tr("mode1_removed_errors")(removed_errors))

    sep("─")
    if PAUSE.should_stop():
        sep()
        print(tr("done"))
        print(tr("pause_stopping"))
        return
    print(f"\n{tr('cache_title')}")
    build_cache(systems_out, sd_dir)

    # ── Auto-génération systems_cache.dat ────────────────────────────────────
    sep("─")
    print(f"\n{tr('sysc_title')}")
    sysc_out = sd_dir / "systems_cache.dat"
    build_systems_cache(systems_out, sysc_out)

    sep()
    print(tr("done"))
    print(f"{tr('done_sd')}{sd_dir}")
    print(f"{tr('done_log')}{log_path}")
    if conv_errors > 0:
        print(tr("mode1_conv_errors")(conv_errors))
    print(f"\n   {tr('done_copy_sd')}")


def mode_extract_download_defaults_only(sd_dir: Path):
    """
    Nouveau mode 2 (intercalé):
      - Extraction gamelists (images)
      - Téléchargement GitHub de systems/_defaults
      - Rien d'autre (pas conversion, pas build cache, pas systems_cache.dat)
    """
    title(tr("mode2_title"))
    prepare_sd_card(sd_dir)

    systems_out = sd_dir / "systems"
    systems_out.mkdir(parents=True, exist_ok=True)

    roms_root, tag_configs, selected_systems = _ask_roms_and_config()
    if roms_root is None:
        print(tr("back_main"))
        return

    sep("─")
    print(f"\n{tr('ext_title')}")
    log_path = sd_dir / "images_manquantes.txt"
    with open(log_path, "w", encoding="utf-8") as log_file:
        grand, _ = run_extraction(
            roms_root,
            systems_out,
            tag_configs,
            log_file,
            selected_systems=selected_systems,
            listen_keyboard=True,
        )
        _write_log(log_file, roms_root, grand)

    if PAUSE.should_stop():
        sep()
        print(tr("done"))
        print(tr("pause_stopping"))
        return

    # Téléchargement _defaults uniquement (GitHub)
    sep("─")
    download_defaults(sd_dir)

    sep()
    print(tr("done"))
    print(f"{tr('done_sd')}{sd_dir}")
    print(f"{tr('done_log')}{log_path}")
    print(f"\n   {tr('mode2_no_other_step')}")
    print(f"\n   {tr('mode2_copy_hint')}")


def mode_extract_only(sd_dir: Path):
    title(tr("mode3_title"))
    prepare_sd_card(sd_dir)
    systems_out = sd_dir / "systems"
    systems_out.mkdir(parents=True, exist_ok=True)

    roms_root, tag_configs, selected_systems = _ask_roms_and_config()
    if roms_root is None:
        print(tr("back_main"))
        return

    log_path = sd_dir / "images_manquantes.txt"
    with open(log_path, "w", encoding="utf-8") as log_file:
        grand, _ = run_extraction(
            roms_root, systems_out, tag_configs, log_file, selected_systems
        )
        _write_log(log_file, roms_root, grand)

    sep()
    print(tr("done"))
    print(f"{tr('done_extracted')}{systems_out}")
    print(f"{tr('done_log2')}{log_path}")


def mode_convert_only(sd_dir: Path):
    title(tr("mode4_title"))

    if not PIL_AVAILABLE:
        print(tr("conv_no_pillow"))
        return

    print(tr("conv_src_where"))
    sep("─")
    src_dir = ask_path()
    if src_dir is None:
        print(tr("back_main"))
        return
    print(f"{tr('src_ok')}{src_dir}")

    run_conversion_raw_only(src_dir)

    sep()
    print(tr("done"))
    print(f"{tr('done_converted')}{src_dir}")


def mode_convert_128_only(sd_dir: Path):
    title(tr("mode5_title"))

    if not PIL_AVAILABLE:
        print(tr("conv_no_pillow"))
        return

    print(tr("conv_src_where"))
    sep("─")
    src_dir = ask_path()
    if src_dir is None:
        print(tr("back_main"))
        return
    print(f"{tr('src_ok')}{src_dir}")

    run_conversion(src_dir)

    sep()
    print(tr("done"))
    print(f"{tr('done_converted')}{src_dir}")


def mode_cache_only(sd_dir: Path):
    title(tr("mode5_title"))

    default_systems = sd_dir / "systems"
    if default_systems.exists():
        print(f"{tr('cache_sys_detected')}{default_systems}")
        if ask_yes_no(tr("cache_sys_use")):
            systems_dir = default_systems
        else:
            print(tr("cache_sys_where"))
            sep("─")
            systems_dir = ask_path()
            if systems_dir is None:
                print(tr("back_main"))
                return
    else:
        print(f"{tr('cache_sys_missing')}{sd_dir}")
        print(tr("cache_sys_where"))
        sep("─")
        systems_dir = ask_path()
        if systems_dir is None:
            print(tr("back_main"))
            return

    sd_dir.mkdir(parents=True, exist_ok=True)
    generated = build_cache(systems_dir, sd_dir)

    sep()
    print(tr("done"))
    if generated:
        print(tr("done_cache_files"))
        for path, nb_sys, nb_games in generated:
            print(tr("cache_file_line")(path, nb_sys, nb_games))
        print(f"\n   {tr('done_copy_cache')}")
    return [path for path, _, _ in generated] if generated else []


def _describe_github_api_error(e: Exception) -> tuple:
    """
    Message utilisateur pour un echec d'appel a l'API Contents GitHub
    (api.github.com -- LISTING des dossiers, PAS raw.githubusercontent.com
    qui sert les fichiers eux-memes et n'a pas ce quota). Detecte
    specifiquement le quota horaire (60 requetes/heure sans
    authentification -- reellement rencontre en test le 2026-08-04 a
    force d'appels repetes) via l'en-tete HTTP "X-RateLimit-Remaining"
    de la reponse 403, et indique l'heure LOCALE exacte de
    reinitialisation ("X-RateLimit-Reset", timestamp Unix) plutot qu'un
    message d'erreur brut peu actionnable.

    Retourne (est_rate_limit: bool, detail: str) -- detail est soit
    l'heure formattee HH:MM (si rate-limit detecte, a injecter dans
    tr("github_rate_limit_msg")), soit str(e) tel quel sinon (a injecter
    dans le message d'erreur generique habituel de l'appelant).
    """
    import urllib.error
    import datetime

    if isinstance(e, urllib.error.HTTPError) and e.code == 403:
        remaining = e.headers.get("X-RateLimit-Remaining")
        reset_ts = e.headers.get("X-RateLimit-Reset")
        if remaining == "0" and reset_ts:
            try:
                reset_dt = datetime.datetime.fromtimestamp(int(reset_ts))
                return True, reset_dt.strftime("%H:%M")
            except (ValueError, OSError, OverflowError):
                pass
    return False, str(e)


# ─────────────────────────────────────────────────────────────────────────────
#  TÉLÉCHARGEMENT PARALLÈLE (utilitaire partagé download_defaults()/download_gif_pack())
# ─────────────────────────────────────────────────────────────────────────────

_PARALLEL_DOWNLOAD_MAX_WORKERS = 16


def _parallel_download_batch(
    tasks: list,
    progress_cb,
    stage: str,
    skip_aborts: bool = True,
    max_workers: int = _PARALLEL_DOWNLOAD_MAX_WORKERS,
) -> tuple:
    """
    Telecharge une liste de fichiers EN PARALLELE
    (concurrent.futures.ThreadPoolExecutor) au lieu d'un par un --
    utilitaire partage par download_defaults()/download_gif_pack().

    Gain mesure en conditions reelles (2026-08-04, 600 fichiers du pack
    GIFs, ~94 Mo) : ~130s en sequentiel -> ~7-10s en parallele (16
    threads). Le cout dominant n'est PAS la bande passante mais la
    latence reseau (connexion HTTPS) repetee a chaque fichier, qui se
    chevauche au lieu de s'additionner des qu'on parallelise. Verifie a
    pleine echelle (600/600 fichiers reussis, 0 echec) : aucune
    limitation cote raw.githubusercontent.com meme a 16 requetes
    simultanees -- SEULE l'API Contents (api.github.com, utilisee pour
    LISTER les dossiers, jamais ici) a un vrai quota documente (60
    requetes/heure sans authentification), inchange par cette fonction.

    tasks : liste de tuples (raw_url, dst_path, label) -- deja filtree
    par l'appelant (fichiers a sauter selon overwrite_existing_files
    exclus en amont, jamais teste ici).

    PAUSE (wait_if_paused/should_stop/should_skip) verifiee UNE FOIS avant
    de lancer le lot entier (plus de sens de le refaire fichier par
    fichier : le lot entier se termine desormais en quelques secondes).
    skip_aborts : True -> should_skip() interrompt le lot (aucun fichier
    telecharge), comme should_stop() (download_defaults()) ; False ->
    should_skip() est traite comme une simple demande de reprise, le lot
    se telecharge quand meme (download_gif_pack(), comportement
    preexistant conserve tel quel).

    Retourne (nb_reussis, [labels_en_echec]). Pas d'impression individuelle
    par fichier ici (delegue a l'appelant, pour son propre tr()) -- avec
    des completions qui arrivent desormais dans un ordre non-sequentiel,
    un compte-rendu par fichier perd beaucoup de son interet et noierait
    les logs pour une operation qui dure maintenant quelques secondes.
    """
    import concurrent.futures as cf
    import urllib.request

    total = len(tasks)
    if total == 0:
        return 0, []

    PAUSE.wait_if_paused()
    if PAUSE.should_stop() or (skip_aborts and PAUSE.should_skip()):
        return 0, []
    if PAUSE.should_skip():
        PAUSE.request_resume()

    def _dl_one(item):
        raw_url, dst, label = item
        try:
            urllib.request.urlretrieve(raw_url, dst)
            return True, label, None
        except Exception as e:
            return False, label, str(e)

    done = 0
    failed: list = []
    stopped = False
    ex = cf.ThreadPoolExecutor(max_workers=max_workers)
    try:
        futures = {ex.submit(_dl_one, item): item for item in tasks}
        for i, fut in enumerate(cf.as_completed(futures), 1):
            ok, label, _err = fut.result()
            if ok:
                done += 1
            else:
                failed.append(label)
            if progress_cb:
                progress_cb(stage, i, total, label)
            if PAUSE.should_stop():
                stopped = True
                break
    finally:
        ex.shutdown(wait=not stopped, cancel_futures=stopped)

    return done, failed


# ─────────────────────────────────────────────────────────────────────────────
#  TÉLÉCHARGEMENT _defaults DEPUIS GITHUB
# ─────────────────────────────────────────────────────────────────────────────

GITHUB_API_URL = "https://api.github.com/repos/shan-aya/RecalBoxDMD/contents/carte%20SD/systems/_defaults"
GITHUB_RAW_BASE = "https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/main/carte%20SD/systems/_defaults"


def download_defaults(
    sd_dir: Path,
    progress_cb=None,
    listen_keyboard: bool = True,
    replace_existing=None,
    download_missing=None,
    overwrite_existing_files: bool = True,
):
    """
    Propose de télécharger _defaults/ depuis GitHub.
    - Si absent  → propose de télécharger
    - Si présent → propose de mettre à jour (remplace)
    - Dans les deux cas, l'utilisateur peut refuser

    overwrite_existing_files : si False, un fichier deja present dans
    _defaults/ est CONSERVE tel quel (pas retelecharge) -- SAUF
    "default.raw565", toujours ecrase (c'est le fichier que l'utilisateur
    peut personnaliser via set_default_fallback_image()/le bouton "Image de
    secours" ; on garantit qu'il refletera toujours le dernier choix
    applique, quel que soit ce reglage). Ne supprime plus tout le dossier
    (auparavant : shutil.rmtree) -- seuls les fichiers effectivement
    retelecharges sont ecrases, fichier par fichier.
    """
    import urllib.request
    import json

    defaults_dir = sd_dir / "systems" / "_defaults"
    exists = defaults_dir.exists() and any(defaults_dir.iterdir())

    sep("─")
    print(f"\n{tr('dl_title')}")
    sep("─")
    print(f"   ↪ defaults_dir = {defaults_dir}")
    print(f"   ↪ GITHUB_API_URL = {GITHUB_API_URL}")
    print(f"   ↪ GITHUB_RAW_BASE = {GITHUB_RAW_BASE}")

    # Si lancé depuis la GUI (listen_keyboard=False), on évite ask_yes_no()
    # et on s'appuie sur replace_existing/download_missing fournis.
    if exists:
        print(tr("dl_exists"))
        if replace_existing is None:
            if listen_keyboard:
                replace_existing = ask_yes_no(tr("dl_ask_update"))
            else:
                replace_existing = False  # safe default: ne pas effacer
        if not replace_existing:
            print(tr("dl_skip"))
            return
        print(tr("dl_replacing"))
        defaults_dir.mkdir(parents=True, exist_ok=True)
    else:
        print(tr("dl_missing"))
        if download_missing is None:
            if listen_keyboard:
                download_missing = ask_yes_no(tr("dl_ask_download"))
            else:
                download_missing = True  # GUI : on télécharge toujours (l'utilisateur a cliqué Démarrer)
        if not download_missing:
            print(tr("dl_skip"))
            return

        # (si on arrive ici: soit on remplace, soit il faut télécharger)
        defaults_dir.mkdir(parents=True, exist_ok=True)

    # Récupère la liste des fichiers via l'API GitHub
    try:
        req = urllib.request.Request(
            GITHUB_API_URL, headers={"User-Agent": "recalbox-toolkit"}
        )
        with urllib.request.urlopen(req, timeout=15) as resp:
            files = json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        is_rl, detail = _describe_github_api_error(e)
        if is_rl:
            print(tr("github_rate_limit_msg")(detail))
        else:
            print(tr("dl_fail_api"))
            print(f"   {detail}")
        return

    # Filtre uniquement les fichiers image/resources (png, gif, raw565)
    media_files = [
        f
        for f in files
        if f.get("type") == "file"
        and Path(f["name"]).suffix.lower() in (".png", ".gif", ".raw565")
    ]

    total = len(media_files)
    print(tr("dl_starting"))

    # Filtre overwrite/skip applique EN AMONT (une seule fois, avant le lot
    # parallele) -- rien d'autre ne cree ces fichiers entretemps, pas besoin
    # de le re-verifier fichier par fichier pendant le telechargement.
    # "default.raw565" est toujours retelecharge/ecrase (voir docstring) ;
    # les autres fichiers respectent le choix overwrite/skip de
    # l'utilisateur s'ils existent deja.
    tasks = []
    skipped = 0
    for f in media_files:
        fname = f["name"]
        dst = defaults_dir / fname
        if (
            not overwrite_existing_files
            and fname != "default.raw565"
            and dst.exists()
        ):
            skipped += 1
            continue
        raw_url = f"{GITHUB_RAW_BASE}/{urllib.request.quote(fname)}"
        tasks.append((raw_url, dst, fname))
    if skipped:
        print(f"   ⏭️  {skipped} fichier(s) deja present(s), conserve(s)")

    PAUSE.start(listen_keyboard=listen_keyboard)
    try:
        done, failed = _parallel_download_batch(
            tasks, progress_cb, "download_defaults", skip_aborts=True
        )
        for label in failed:
            print(tr("dl_file_err")(label, "échec téléchargement"))
    finally:
        PAUSE.stop()

    print(tr("dl_done")(done))


# ─────────────────────────────────────────────────────────────────────────────
#  BANQUE DE GIFS (pack gratuit GitHub) -- plan Mode 1 / banque de GIFs
# ─────────────────────────────────────────────────────────────────────────────
GITHUB_GIFS_API_URL = "https://api.github.com/repos/shan-aya/RecalBoxDMD/contents/carte%20SD/gifs"
GITHUB_GIFS_RAW_BASE = "https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/main/carte%20SD/gifs"


def download_gif_pack(sd_dir: Path, progress_cb=None, listen_keyboard: bool = True,
                       overwrite_existing_files: bool = True) -> tuple[int, int]:
    """
    Telecharge le pack gratuit de GIFs (~600) depuis
    github.com/shan-aya/RecalBoxDMD ("carte SD/gifs") vers
    <sd_dir>/gifs/<sous-dossier>/. Contrairement a download_defaults()
    (un seul niveau, plat), ce dossier contient plusieurs sous-dossiers
    thematiques : liste le niveau racine via l'API Contents (1 requete),
    puis 1 requete Contents supplementaire PAR sous-dossier trouve
    (type=="dir"), chaque echec de sous-dossier etant non bloquant (log +
    continue, meme philosophie best-effort que download_defaults). Meme
    convention que download_defaults()/_copy_to_drive : PAUSE.should_stop()/
    should_skip(), progress_cb(stage, i, total, nom). Retourne
    (fichiers_telecharges, sous_dossiers_trouves).
    """
    import urllib.request
    import json

    sep("─")
    print(f"\n{tr('gifpack_title')}")

    try:
        req = urllib.request.Request(GITHUB_GIFS_API_URL, headers={"User-Agent": "RecalBoxDMD-tool"})
        with urllib.request.urlopen(req) as resp:
            root_entries = json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        is_rl, detail = _describe_github_api_error(e)
        print(tr("github_rate_limit_msg")(detail) if is_rl else tr("gifpack_fail_api")(detail))
        return 0, 0

    subfolders = [e["name"] for e in root_entries if e.get("type") == "dir"]

    manifest: list[tuple[str, str]] = []
    for sub in subfolders:
        try:
            sub_url = f"{GITHUB_GIFS_API_URL}/{urllib.request.quote(sub)}"
            req = urllib.request.Request(sub_url, headers={"User-Agent": "RecalBoxDMD-tool"})
            with urllib.request.urlopen(req) as resp:
                sub_entries = json.loads(resp.read().decode("utf-8"))
            for entry in sub_entries:
                if entry.get("type") == "file" and entry["name"].lower().endswith(".gif"):
                    manifest.append((sub, entry["name"]))
        except Exception as e:
            is_rl, detail = _describe_github_api_error(e)
            if is_rl:
                # Inutile de continuer a interroger les sous-dossiers
                # restants : le quota etant epuise, ils echoueraient tous
                # de la meme facon -- un seul message clair plutot qu'une
                # repetition par sous-dossier restant.
                print(tr("github_rate_limit_msg")(detail))
                break
            print(tr("gifpack_fail_subfolder")(sub, detail))
            continue

    total = len(manifest)
    print(tr("gifpack_starting")(total, len(subfolders)))

    dest_root = sd_dir / PLAYLIST_GIFS_DIRNAME
    dest_root.mkdir(parents=True, exist_ok=True)
    # Pre-cree tous les sous-dossiers de destination AVANT le telechargement
    # parallele : plusieurs threads pourraient sinon tenter de creer le
    # meme sous-dossier au meme instant (mkdir(exist_ok=True) est sur en
    # soi, mais autant l'eviter completement).
    for sub in subfolders:
        (dest_root / sub).mkdir(parents=True, exist_ok=True)

    tasks = []
    skipped = 0
    for sub, fname in manifest:
        dst = dest_root / sub / fname
        label = f"{sub}/{fname}"
        if not overwrite_existing_files and dst.exists():
            skipped += 1
            continue
        raw_url = f"{GITHUB_GIFS_RAW_BASE}/{urllib.request.quote(sub)}/{urllib.request.quote(fname)}"
        tasks.append((raw_url, dst, label))
    if skipped:
        print(f"   ⏭️  {skipped} fichier(s) deja present(s), conserve(s)")

    PAUSE.start(listen_keyboard=listen_keyboard)
    try:
        done, failed = _parallel_download_batch(
            tasks, progress_cb, "download_gif_pack", skip_aborts=False
        )
        for label in failed:
            print(tr("gifpack_file_err")(label, "échec téléchargement"))
    finally:
        PAUSE.stop()

    print(tr("gifpack_done")(done))
    return done, len(subfolders)


# Playlist "vitrine" fournie avec le pack (carte SD/playlists/ du depot,
# PAS carte SD/gifs/ -- un dossier different, jamais couvert par
# download_gif_pack() ci-dessus qui ne liste que gifs/). Nom garde tel
# quel (espaces reels, PAS %20) car c'est le nom de fichier reel attendu
# sur la carte SD -- le firmware le compare tel quel (loadConfig(),
# "playlist=" -> playlistSourcePath = "/playlists/" + playlistName).
GITHUB_DEFAULT_PLAYLIST_NAME = "RpiTeaM eLLuiGi.txt"
GITHUB_DEFAULT_PLAYLIST_RAW_URL = (
    "https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/main/"
    "carte%20SD/playlists/RpiTeaM%20eLLuiGi.txt"
)


def download_default_gif_playlist(sd_dir: Path) -> bool:
    """
    Telecharge la playlist "vitrine" du pack GitHub
    (GITHUB_DEFAULT_PLAYLIST_NAME, carte SD/playlists/ du depot) vers
    <sd_dir>/playlists/ -- best-effort comme download_gif_pack() : aucune
    exception ne remonte, juste un message + False en cas d'echec (reseau,
    fichier absent du depot, etc.), laissant l'appelant se rabattre sur
    ALL.txt (voir create_all_gifs_playlist()).
    """
    import urllib.request

    pl_dir = sd_dir / PLAYLIST_DIR_NAME
    pl_dir.mkdir(parents=True, exist_ok=True)
    dst = pl_dir / GITHUB_DEFAULT_PLAYLIST_NAME
    try:
        req = urllib.request.Request(
            GITHUB_DEFAULT_PLAYLIST_RAW_URL, headers={"User-Agent": "RecalBoxDMD-tool"}
        )
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = resp.read()
        dst.write_bytes(data)
        print(tr("gifpack_playlist_ok"))
        return True
    except Exception as e:
        print(tr("gifpack_playlist_fail")(e))
        return False


def create_all_gifs_playlist(sd_dir: Path, name: str = "ALL") -> Optional[str]:
    """
    Cree <sd_dir>/playlists/<name>.txt referencant TOUS les dossiers de
    <sd_dir>/gifs/ EN ENTIER (marqueur "# FULL:" avec tous les dossiers,
    cf write_playlist()) -- playlist de secours "tout inclus" utilisee
    comme playlist par defaut du Mode 1 quand la playlist vitrine du pack
    GitHub (GITHUB_DEFAULT_PLAYLIST_NAME) n'est pas disponible (pack non
    demande, ou telechargement echoue) mais que la carte contient quand
    meme des GIFs (import personnel via l'onglet Playlist, ou pack
    partiellement telecharge). Ne cree RIEN et retourne None si gifs/ est
    vide ou absent (aucun dossier trouve) -- jamais de playlist "vide"
    inutile, et le config.ini associe (write_dmd_default_playlist()) doit
    alors rester sans playlist par defaut. Retourne "<name>.txt" si creee.
    """
    folders = [n for n, _count in list_playlist_gif_folders(sd_dir)]
    if not folders:
        print(tr("all_playlist_skipped_empty"))
        return None
    entries = build_playlist_entries_from_folders(sd_dir, folders)
    write_playlist(sd_dir, name, entries, full_folders=folders)
    print(tr("all_playlist_created")(len(folders)))
    return f"{name}.txt"


# ─────────────────────────────────────────────────────────────────────────────
#  INSTALLATION DES SCRIPTS RECALBOX (userscripts) DEPUIS GITHUB
# ─────────────────────────────────────────────────────────────────────────────
#
# Remplace l'ancienne tentative d'installation par le DMD lui-meme (client
# FTP ecrit a la main cote firmware) : la Recalbox cible ne fait tourner
# aucun serveur FTP (confirme en test reel, port 21 refuse). Windows resout
# nativement le partage SMB de la Recalbox comme un chemin de fichier normal
# (\\<host>\share) -- donc une simple copie de fichier suffit, aucune
# bibliotheque FTP/SMB/SSH necessaire. Meme mecanisme de telechargement que
# download_defaults() ci-dessus (API Contents GitHub + raw.githubusercontent.com),
# mais la destination est un chemin UNC sur la Recalbox plutot qu'un dossier
# local sur la carte SD.

GITHUB_SCRIPTS_API_BASE = "https://api.github.com/repos/shan-aya/RecalBoxDMD/contents/tools"
GITHUB_SCRIPTS_RAW_BASE = "https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/main/tools"

# tools/ est a plat sur GitHub (pas de sous-dossier scripts/manual|events) --
# route chaque .sh vers userscripts/manual (lancement manuel depuis Recalbox)
# ou userscripts/ (scripts d'evenement) selon son nom.
_MANUAL_SCRIPT_MARKERS = ("Config Web DMD", "WiFi Recovery", "Reboot DMD")


def _is_manual_script(fname: str) -> bool:
    return any(marker in fname for marker in _MANUAL_SCRIPT_MARKERS)


def detect_recalbox_share() -> Optional[str]:
    r"""
    Tente de resoudre le partage reseau de la Recalbox via son nom machine
    par defaut ("RECALBOX", resolution NetBIOS Windows native) -- aucune
    bibliotheque tierce, aucun scan reseau. Retourne "RECALBOX" si
    \\RECALBOX\share repond, sinon None (repli sur les prefs / la saisie
    manuelle). Le nom de machine par defaut de Recalbox n'est pas modifiable
    depuis ce module -- si l'utilisateur l'a personnalise, la detection
    echoue simplement et on retombe sur le mecanisme manuel existant.
    """
    try:
        return "RECALBOX" if Path(r"\\RECALBOX\share").exists() else None
    except Exception:
        return None


def resolve_recalbox_ip(host: str) -> str:
    """
    Resout `host` (nom NetBIOS type "RECALBOX" ou IP deja litterale) en
    adresse IP numerique, pour affichage utilisateur. Le nom d'hote
    "RECALBOX" (detection auto) est identique quelle que soit la Recalbox
    physique allumee sur le reseau -- seule l'IP permet a l'utilisateur de
    distinguer laquelle est visee quand plusieurs sont allumees. Retourne
    `host` tel quel si la resolution echoue (ne devrait pas arriver juste
    apres un test de joignabilite reussi).
    """
    import socket
    try:
        return socket.gethostbyname(host)
    except OSError:
        return host


def is_recalbox_reachable(host: str, timeout: float = 1.5) -> bool:
    """
    Teste reellement la joignabilite de `host` (connexion TCP au port SMB
    445), contrairement a un simple "il y a une IP en cache dans les prefs"
    qui ne prouve pas que la Recalbox est actuellement allumee/sur le
    reseau. Timeout court : appele sur le thread principal du GUI avant de
    lancer le pipeline Mode 1.
    """
    if not host:
        return False
    import socket
    try:
        with socket.create_connection((host, 445), timeout=timeout):
            return True
    except OSError:
        return False


def _list_recalbox_script_files():
    """Interroge l'API GitHub (tools/, a plat) pour la liste des scripts
    .sh disponibles. Retourne None (message deja imprime) si l'appel API
    echoue, sinon la liste des entrees fichier (dicts "name"/"type")."""
    import urllib.request
    import json

    try:
        req = urllib.request.Request(
            GITHUB_SCRIPTS_API_BASE, headers={"User-Agent": "recalbox-toolkit"}
        )
        with urllib.request.urlopen(req, timeout=15) as resp:
            files = json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        is_rl, detail = _describe_github_api_error(e)
        if is_rl:
            print(tr("github_rate_limit_msg")(detail))
        else:
            print(tr("dl_fail_api"))
            print(f"   {detail}")
        return None

    return [
        f for f in files
        if f.get("type") == "file" and f.get("name", "").lower().endswith(".sh")
    ]


def stage_recalbox_scripts_locally(dest_dir: Path, progress_cb=None) -> tuple:
    r"""
    Telecharge les scripts utilisateur Recalbox depuis GitHub vers un
    dossier LOCAL (dest_dir/manual/ pour les scripts a lancement manuel,
    dest_dir/ pour les scripts d'evenement) -- meme arborescence que le
    partage reseau \\<host>\share\userscripts\..., pour que l'utilisateur
    puisse les copier lui-meme plus tard (glisser-deposer) si l'install
    reseau automatique (install_staged_scripts_to_share) a echoue ou n'a
    pas ete tentee. Ne necessite aucun acces reseau a une Recalbox, juste
    Internet (GitHub). Retourne (fichiers_ok, fichiers_total) ; (0, 0) si
    l'appel API GitHub echoue.
    """
    import urllib.request

    script_files = _list_recalbox_script_files()
    if not script_files:
        return (0, 0)

    manual_dir = dest_dir / "manual"
    manual_dir.mkdir(parents=True, exist_ok=True)
    dest_dir.mkdir(parents=True, exist_ok=True)

    ok_count = 0
    total_count = len(script_files)
    for i, f in enumerate(script_files, 1):
        fname = f["name"]
        if progress_cb is not None:
            progress_cb("stage_recalbox_scripts", i, total_count, fname)
        raw_url = f"{GITHUB_SCRIPTS_RAW_BASE}/{urllib.request.quote(fname)}"
        dst_dir = manual_dir if _is_manual_script(fname) else dest_dir
        dst = dst_dir / fname
        try:
            urllib.request.urlretrieve(raw_url, dst)
            ok_count += 1
        except Exception as e:
            print(tr("mode9_result_fail")(fname, e))

    return (ok_count, total_count)


def install_staged_scripts_to_share(staged_dir: Path, recalbox_host: str, progress_cb=None) -> tuple:
    r"""
    Copie vers le partage reseau \\<recalbox_host>\share\userscripts\...
    des scripts DEJA telecharges localement par stage_recalbox_scripts_locally
    (evite un second telechargement GitHub quand une mise en scene locale a
    deja ete faite -- toujours le cas en Mode 1 desormais). Retourne
    (fichiers_ok, fichiers_total) ; (0, 0) si recalbox_host est vide, le
    dossier local est vide, ou le partage est injoignable.
    """
    if not recalbox_host or not staged_dir.exists():
        return (0, 0)

    share_root = Path(rf"\\{recalbox_host}\share")
    try:
        os.listdir(share_root)
    except OSError as e:
        # winerror 1272 = Windows bloque l'acces invite SMB non authentifie
        # (EnableInsecureGuestLogons=False, defaut Windows 10/11) -- distinct
        # d'un partage reellement injoignable, message dedie pour eviter que
        # l'utilisateur cherche du cote reseau/IP alors que RB repond bien.
        if getattr(e, "winerror", None) == 1272:
            print(tr("mode9_guest_blocked"))
        else:
            print(tr("mode9_share_unreachable")(recalbox_host))
        return (0, 0)

    manual_src = staged_dir / "manual"
    src_files = []
    if manual_src.exists():
        src_files += [(p, True) for p in sorted(manual_src.glob("*.sh"))]
    src_files += [(p, False) for p in sorted(staged_dir.glob("*.sh"))]

    manual_dst = share_root / "userscripts" / "manual"
    events_dst = share_root / "userscripts"
    manual_dst.mkdir(parents=True, exist_ok=True)
    events_dst.mkdir(parents=True, exist_ok=True)

    ok_count = 0
    total_count = len(src_files)
    for i, (src, is_manual) in enumerate(src_files, 1):
        fname = src.name
        if progress_cb is not None:
            progress_cb("install_recalbox_scripts", i, total_count, fname)
        dst = (manual_dst if is_manual else events_dst) / fname
        try:
            shutil.copyfile(src, dst)
            ok_count += 1
            print(tr("mode9_result_ok")(fname))
        except Exception as e:
            print(tr("mode9_result_fail")(fname, e))

    return (ok_count, total_count)


# Identifiants par defaut Recalbox (SSH root, documentes publiquement par le
# projet Recalbox lui-meme) -- utilises uniquement pour le repli SSH/SFTP
# quand le partage SMB est injoignable (ex: segmentation VLAN qui bloque
# 445/139 mais autorise 22, cas reel rencontre par un utilisateur).
RECALBOX_SSH_USER = "root"
RECALBOX_SSH_PASSWORD = "recalboxroot"
RECALBOX_SSH_USERSCRIPTS_PATH = "/recalbox/share/userscripts"


def is_recalbox_ssh_reachable(host: str, timeout: float = 1.5) -> bool:
    """Teste la joignabilite reelle du port SSH (22) -- meme logique que
    is_recalbox_reachable() (port 445/SMB)."""
    if not host:
        return False
    import socket
    try:
        with socket.create_connection((host, 22), timeout=timeout):
            return True
    except OSError:
        return False


def _ssh_connect_recalbox(recalbox_host: str):
    """Ouvre une connexion SSH vers la Recalbox (identifiants par defaut).
    Retourne le client paramiko connecte, ou None si paramiko est
    indisponible ou la connexion echoue (message deja imprime)."""
    if not _ensure_paramiko():
        return None
    import paramiko

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(
            recalbox_host,
            username=RECALBOX_SSH_USER,
            password=RECALBOX_SSH_PASSWORD,
            timeout=5, banner_timeout=5, auth_timeout=5,
            # look_for_keys/allow_agent=False : par defaut paramiko essaie
            # d'abord toute cle privee locale (~/.ssh) puis un agent SSH
            # (Pageant, etc.) AVANT le mot de passe -- si l'utilisateur a des
            # cles SSH configurees pour d'autres usages (GitHub, serveurs de
            # travail...), ces tentatives peuvent epuiser le nombre max de
            # tentatives d'authentification du serveur dropbear de la
            # Recalbox avant meme que le mot de passe root/root soit essaye,
            # ce qui remonte "Authentication failed" alors que le mot de
            # passe est correct (confirme par un outil tiers fonctionnel
            # avec les memes identifiants). Recalbox est un systeme a
            # identifiants fixes connus : jamais besoin d'essayer une cle.
            look_for_keys=False, allow_agent=False,
        )
        return client
    except Exception as e:
        print(tr("ssh_connect_fail")(recalbox_host, e))
        return None


def _sftp_ensure_userscripts_dirs(sftp) -> str:
    """Cree (si besoin) les dossiers userscripts/ et userscripts/manual/
    distants. Retourne le chemin du sous-dossier manual/."""
    manual_dst = f"{RECALBOX_SSH_USERSCRIPTS_PATH}/manual"
    for remote_dir in (RECALBOX_SSH_USERSCRIPTS_PATH, manual_dst):
        try:
            sftp.mkdir(remote_dir)
        except IOError:
            pass  # existe deja
    return manual_dst


def install_staged_scripts_via_ssh(staged_dir: Path, recalbox_host: str, progress_cb=None) -> tuple:
    r"""
    Repli SSH/SFTP de install_staged_scripts_to_share() quand le partage
    SMB (\\<host>\share) est injoignable -- ex: segmentation VLAN qui
    bloque les ports 445/139 mais autorise le port 22 (cas reel
    rencontre). Copie les scripts DEJA telecharges localement
    (stage_recalbox_scripts_locally) vers
    /recalbox/share/userscripts/... via SFTP, avec les identifiants par
    defaut Recalbox (root/root). Retourne (fichiers_ok, fichiers_total) ;
    (0, 0) si paramiko indisponible, hote vide, dossier local vide, ou
    connexion SSH impossible.
    """
    if not recalbox_host or not staged_dir.exists():
        return (0, 0)

    manual_src = staged_dir / "manual"
    src_files = []
    if manual_src.exists():
        src_files += [(p, True) for p in sorted(manual_src.glob("*.sh"))]
    src_files += [(p, False) for p in sorted(staged_dir.glob("*.sh"))]
    if not src_files:
        return (0, 0)

    client = _ssh_connect_recalbox(recalbox_host)
    if client is None:
        return (0, 0)

    ok_count = 0
    total_count = len(src_files)
    try:
        sftp = client.open_sftp()
        try:
            manual_dst = _sftp_ensure_userscripts_dirs(sftp)
            for i, (src, is_manual) in enumerate(src_files, 1):
                fname = src.name
                if progress_cb is not None:
                    progress_cb("install_recalbox_scripts", i, total_count, fname)
                remote_path = f"{manual_dst if is_manual else RECALBOX_SSH_USERSCRIPTS_PATH}/{fname}"
                try:
                    sftp.put(str(src), remote_path)
                    sftp.chmod(remote_path, 0o755)
                    ok_count += 1
                    print(tr("mode9_result_ok")(fname))
                except Exception as e:
                    print(tr("mode9_result_fail")(fname, e))
        finally:
            sftp.close()
    finally:
        client.close()

    return (ok_count, total_count)


def install_recalbox_scripts(staged_dir: Path, recalbox_host: str, progress_cb=None) -> tuple:
    """
    Installe les scripts deja mis en scene localement (staged_dir) vers la
    Recalbox -- SMB en priorite (install_staged_scripts_to_share), repli
    SSH/SFTP automatique (install_staged_scripts_via_ssh) si le partage
    SMB est injoignable. Utilisee par le Mode 1. Retourne
    (fichiers_ok, fichiers_total, methode) ou methode vaut "smb", "ssh",
    ou None (aucune des deux methodes n'a abouti).
    """
    if not recalbox_host:
        return (0, 0, None)

    if is_recalbox_reachable(recalbox_host):
        ok, total = install_staged_scripts_to_share(staged_dir, recalbox_host, progress_cb=progress_cb)
        if total > 0:
            return (ok, total, "smb")

    print(tr("mode9_smb_fallback_ssh"))
    ok, total = install_staged_scripts_via_ssh(staged_dir, recalbox_host, progress_cb=progress_cb)
    if total > 0:
        return (ok, total, "ssh")

    return (0, 0, None)


def download_recalbox_scripts(recalbox_host: str, progress_cb=None, listen_keyboard: bool = True):
    r"""
    Installe/met a jour les scripts utilisateur Recalbox (WiFi Recovery DMD,
    Config Web DMD, pont marquee) en les telechargeant depuis GitHub
    (dossier tools/, a plat -- aucun sous-dossier scripts/manual|events sur
    le depot) directement vers le partage reseau \\<recalbox_host>\share,
    route par nom de fichier vers userscripts/manual (scripts a lancement
    manuel, ex: "Config Web DMD...") ou userscripts/ (scripts d'evenement,
    ex: "WiFi Recovery DMD.sh", le pont marquee). Retourne un tuple
    (fichiers_ok, fichiers_total). Si recalbox_host est vide, retourne
    (0, 0) sans rien faire -- geree comme un skip silencieux par les
    appelants. Utilisee par le Mode 9 (installe directement, sans mise en
    scene locale) -- le Mode 1 utilise desormais
    stage_recalbox_scripts_locally() + install_recalbox_scripts().
    Repli SSH/SFTP automatique (_download_recalbox_scripts_via_ssh) si le
    partage SMB est injoignable -- ex: segmentation VLAN qui bloque
    445/139 mais autorise 22 (cas reel rencontre).
    """
    import urllib.request

    if not recalbox_host:
        return (0, 0)

    # Diagnostic : repr() plutot qu'un simple print -- un caractere invisible
    # (espace de largeur nulle U+200B, etc.) issu d'un copier-coller ne serait
    # jamais visible dans un print() normal ni retire par .strip() (qui ne
    # retire que les espaces Unicode "classiques"), mais apparaitrait ici en
    # toutes lettres (​...). Ajoute suite a un cas reel ou meme nom/IP
    # confirmes identiques cote utilisateur, le SMB par IP echouait sans
    # raison nette identifiee par ailleurs.
    print(f"[DEBUG] recalbox_host={recalbox_host!r}")

    # Pre-verification rapide (socket, ~1.5s) avant os.listdir() sur le
    # chemin UNC -- meme garde deja utilisee par install_recalbox_scripts()
    # (Mode 1) mais qui manquait ici (Mode 9). Sans elle, un hote saisi en
    # IP directe (par opposition a un nom NetBIOS resolu localement) peut
    # faire attendre le redirecteur SMB de Windows tres longtemps avant de
    # lever une erreur -- donnant l'impression que "l'IP ne marche pas" alors
    # que le repli SSH, plus bas, aurait tres bien fonctionne s'il avait ete
    # atteint plus vite.
    share_root = Path(rf"\\{recalbox_host}\share")
    if not is_recalbox_reachable(recalbox_host):
        print(tr("mode9_share_unreachable")(recalbox_host))
        return _download_recalbox_scripts_via_ssh(
            recalbox_host, progress_cb=progress_cb, listen_keyboard=listen_keyboard
        )
    try:
        os.listdir(share_root)
    except OSError as e:
        # winerror 1272 = Windows bloque l'acces invite SMB non authentifie
        # (EnableInsecureGuestLogons=False, defaut Windows 10/11) -- distinct
        # d'un partage reellement injoignable, message dedie pour eviter que
        # l'utilisateur cherche du cote reseau/IP alors que RB repond bien.
        # Dans les deux cas, tentative de repli SSH avant d'abandonner.
        if getattr(e, "winerror", None) == 1272:
            print(tr("mode9_guest_blocked"))
        else:
            print(tr("mode9_share_unreachable")(recalbox_host))
        return _download_recalbox_scripts_via_ssh(
            recalbox_host, progress_cb=progress_cb, listen_keyboard=listen_keyboard
        )

    script_files = _list_recalbox_script_files()
    if not script_files:
        return (0, 0)

    manual_dir = share_root / "userscripts" / "manual"
    events_dir = share_root / "userscripts"
    manual_dir.mkdir(parents=True, exist_ok=True)
    events_dir.mkdir(parents=True, exist_ok=True)

    ok_count = 0
    total_count = len(script_files)

    PAUSE.start(listen_keyboard=listen_keyboard)
    try:
        for i, f in enumerate(script_files, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break
            fname = f["name"]
            if progress_cb is not None:
                progress_cb("install_recalbox_scripts", i, total_count, fname)
            raw_url = f"{GITHUB_SCRIPTS_RAW_BASE}/{urllib.request.quote(fname)}"
            dst_dir = manual_dir if _is_manual_script(fname) else events_dir
            dst = dst_dir / fname
            try:
                urllib.request.urlretrieve(raw_url, dst)
                ok_count += 1
                print(tr("mode9_result_ok")(fname))
            except Exception as e:
                print(tr("mode9_result_fail")(fname, e))
    finally:
        PAUSE.stop()

    return (ok_count, total_count)


def _download_recalbox_scripts_via_ssh(recalbox_host: str, progress_cb=None, listen_keyboard: bool = True) -> tuple:
    r"""
    Repli SSH/SFTP de download_recalbox_scripts() (Mode 9) quand le
    partage SMB est injoignable -- meme liste de scripts (GitHub) et meme
    routage manual/ vs racine, mais ecrits directement via SFTP dans
    /recalbox/share/userscripts/... (identifiants par defaut Recalbox)
    plutot que \\<host>\share. Retourne (fichiers_ok, fichiers_total) ;
    (0, 0) si paramiko indisponible ou connexion SSH impossible.
    """
    import urllib.request

    script_files = _list_recalbox_script_files()
    if not script_files:
        return (0, 0)

    client = _ssh_connect_recalbox(recalbox_host)
    if client is None:
        return (0, 0)

    ok_count = 0
    total_count = len(script_files)
    try:
        sftp = client.open_sftp()
        try:
            manual_dst = _sftp_ensure_userscripts_dirs(sftp)
            PAUSE.start(listen_keyboard=listen_keyboard)
            try:
                for i, f in enumerate(script_files, 1):
                    PAUSE.wait_if_paused()
                    if PAUSE.should_stop() or PAUSE.should_skip():
                        break
                    fname = f["name"]
                    if progress_cb is not None:
                        progress_cb("install_recalbox_scripts", i, total_count, fname)
                    raw_url = f"{GITHUB_SCRIPTS_RAW_BASE}/{urllib.request.quote(fname)}"
                    remote_path = f"{manual_dst if _is_manual_script(fname) else RECALBOX_SSH_USERSCRIPTS_PATH}/{fname}"
                    try:
                        with urllib.request.urlopen(raw_url, timeout=15) as resp:
                            data = resp.read()
                        with sftp.open(remote_path, "wb") as rf:
                            rf.write(data)
                        sftp.chmod(remote_path, 0o755)
                        ok_count += 1
                        print(tr("mode9_result_ok")(fname))
                    except Exception as e:
                        print(tr("mode9_result_fail")(fname, e))
            finally:
                PAUSE.stop()
        finally:
            sftp.close()
    finally:
        client.close()

    return (ok_count, total_count)


def mode_install_recalbox_scripts_console():
    """Mode avance (console) : installe/met a jour les scripts Recalbox seuls,
    sans rejouer tout le pipeline Mode 1."""
    title(tr("mode9_title"))
    print(tr("mode9_desc"))
    print()

    target = detect_recalbox_share()
    if target:
        print(tr("mode9_autodetect_ok")(target))
    else:
        default_host = prefs.get("recalbox_ip") or ""
        prompt = tr("mode9_ip_prompt")
        if default_host:
            prompt = f"{prompt} [{default_host}]"
        raw = input(f"  {prompt} : ").strip()
        target = raw or default_host
        if not target:
            print(tr("mode9_share_unreachable")(""))
            return
        prefs.set("recalbox_ip", target)

    ok_count, total = download_recalbox_scripts(target, listen_keyboard=True)
    sep()
    print(tr("done"))
    if total > 0:
        print(f"  {ok_count}/{total}")


# ─────────────────────────────────────────────────────────────────────────────
#  BUILD SYSTEMS CACHE  (systems_cache.dat pour l'ESP32)
# ─────────────────────────────────────────────────────────────────────────────


def build_systems_cache(
    systems_dir: Path,
    output_path: Path,
    progress_cb=None,
    slow_threshold: Optional[int] = None,
):
    """
    Génère systems_cache.dat au format attendu par l'ESP32 :
      g nes
      p snes
      g neogeo
    Scanne systems_dir/_defaults/ — un fichier par système (gif prioritaire).

    slow_threshold : nombre de fichiers .raw565/.raw565pack/.meta au-dela
    duquel un systeme recoit le flag "L" (lent). None (par defaut) = lu
    depuis la preference utilisateur "slow_threshold" (onglet Parametres,
    v33) -- source unique pour tous les appelants (GUI Mode 1/3/8, CLI),
    repli 5000 sur toute erreur de lecture/conversion.
    """
    if slow_threshold is None:
        try:
            slow_threshold = int(prefs.get("slow_threshold") or 5000)
        except (TypeError, ValueError):
            slow_threshold = 5000

    defaults_dir = systems_dir / "_defaults"
    if not defaults_dir.exists():
        print(tr("sysc_no_defaults"))
        return 0

    # Collecte tous les noms de systèmes présents dans _defaults/ et détermine p/g/b
    # Format attendu par le firmware (buildSysDefaultCache) :
    #   p : <sys>.raw565
    #   g : <sys>.raw565pack + <sys>.meta
    #   b : <sys>.raw565 + (<sys>.raw565pack + <sys>.meta)
    #
    # Compat legacy (PNG/GIF) : si raw565/raw565pack/meta manquants, on retombe sur
    #   - .gif -> g
    #   - .png -> p

    entries: dict[str, tuple[str, str]] = {}

    # Firmware : sysName vient des dossiers systèmes dans /systems (hors _defaults),
    # puis il teste /systems/_defaults/<sysName>.raw565 etc.
    # Donc on doit aligner la liste des "keys" sur les noms de dossiers dans systems_dir.
    sys_case: dict[str, str] = {}  # sys_lower -> sys_name (case d'origine)
    sys_lowers: set[str] = set()

    def add_sys_name(sys_name: str) -> None:
        sys_lower = sys_name.lower()
        sys_lowers.add(sys_lower)
        sys_case.setdefault(sys_lower, sys_name)

    # 1) noms depuis /systems (dossiers directs), pour matcher firmware
    for p in systems_dir.iterdir():
        if not p.is_dir():
            continue
        if p.name.lower() == "_defaults":
            continue
        add_sys_name(p.name)

    # 2) "default" peut n'avoir aucun dossier /systems/default, mais exister dans _defaults
    default_base = defaults_dir / "default"
    if default_base.with_suffix(".raw565").exists() or (
        default_base.with_suffix(".raw565pack").exists()
        and default_base.with_suffix(".meta").exists()
    ):
        add_sys_name("default")

    # 3) complément : tout stem observable dans _defaults (legacy/compat)
    for f in defaults_dir.iterdir():
        if not f.is_file():
            continue
        suf = f.suffix.lower()
        if suf in (".gif", ".png", ".raw565", ".raw565pack", ".meta"):
            add_sys_name(f.stem)

    # sysName vient de _defaults (ici: uniquement les raw565 présents)
    # classification b/g/p est faite en regardant systems/<sysName>/ (pas _defaults)
    defaults_raw565_stems: set[str] = set()
    for f in defaults_dir.iterdir():
        if not f.is_file():
            continue
        if f.suffix.lower() == ".raw565":
            defaults_raw565_stems.add(f.stem.lower())

    # DEBUG: vérifier que le dossier d'entrée contient bien des raw565pack/meta
    raw565pack_total = 0
    meta_total = 0
    raw565_total = 0
    try:
        for sys_entry in systems_dir.iterdir():
            if not sys_entry.is_dir():
                continue
            if sys_entry.name.lower() == "_defaults":
                continue
            for entry in sys_entry.iterdir():
                if not entry.is_file():
                    continue
                suf = entry.suffix.lower()
                if suf == ".raw565":
                    raw565_total += 1
                elif suf == ".raw565pack":
                    raw565pack_total += 1
                elif suf == ".meta":
                    meta_total += 1
    except Exception:
        pass

    print(
        f"[DEBUG] build_systems_cache systems_dir={systems_dir} "
        f"raw565={raw565_total} raw565pack={raw565pack_total} meta={meta_total}"
    )

    # Debug ciblé (algorithme firmware) : p/g/b déduits UNIQUEMENT via systems/<sys>/ (sans _defaults, sans meta)
    debug_sys_names = ["3do", "amiga600", "64dd"]
    for dbg_name in debug_sys_names:
        system_dir_dbg = systems_dir / dbg_name

        sys_key = dbg_name.lower()
        has_raw565 = False
        has_raw565pack = False

        if system_dir_dbg.exists() and system_dir_dbg.is_dir():
            for entry in system_dir_dbg.iterdir():
                if not entry.is_file():
                    continue
                entry_name = entry.name.lower()
                if entry_name == f"{sys_key}.raw565":
                    has_raw565 = True
                elif entry_name == f"{sys_key}.raw565pack":
                    has_raw565pack = True

        has_gif_pack = has_raw565pack

        ftype_dbg = "?"
        if has_gif_pack and has_raw565:
            ftype_dbg = "b"
        elif has_gif_pack:
            ftype_dbg = "g"
        elif has_raw565:
            ftype_dbg = "p"

        print(
            f"[DEBUG] classify {dbg_name}: "
            f"systems raw565={'Y' if has_raw565 else 'N'} | "
            f"systems raw565pack={'Y' if has_raw565pack else 'N'} "
            f"=> ftype={ftype_dbg}"
        )

    for sys_lower in sys_lowers:
        name = sys_case[sys_lower]

        # p/g/b avec b = p + g
        # - p : systèmes/<_defaults>/<sys>.raw565 (côté _defaults)
        # - g : systèmes/<sys> contient au moins un *.raw565pack (récursif)
        base_defaults = defaults_dir / name
        system_dir = systems_dir / name

        # raw565 (png) doit exister dans systems/<sys>/ (pas uniquement dans _defaults)
        has_raw565 = False
        if system_dir.exists() and system_dir.is_dir():
            for _root, _dirs, files in os.walk(system_dir):
                for fn in files:
                    if fn.lower().endswith(".raw565"):
                        has_raw565 = True
                        break
                if has_raw565:
                    break

        has_raw565pack = False
        if system_dir.exists() and system_dir.is_dir():
            for _root, _dirs, files in os.walk(system_dir):
                for fn in files:
                    if fn.lower().endswith(".raw565pack"):
                        has_raw565pack = True
                        break
                if has_raw565pack:
                    break

        has_gif_pack = has_raw565pack

        if has_gif_pack and has_raw565:
            ftype = "B"
        elif has_gif_pack:
            ftype = "g"
        elif has_raw565:
            ftype = "p"
        else:
            continue

        if name.lower() == "64dd":
            # Debug aligné avec la logique ftype:
            # - raw565 depuis systems/_defaults/<sys>.raw565
            # - raw565pack/meta depuis systems/<sys>/
            defaults_base = defaults_dir / name
            system_base = systems_dir / name

            dbg_has_raw565 = defaults_base.with_suffix(".raw565").exists()
            dbg_has_raw565pack = False
            if system_base.exists() and system_base.is_dir():
                for _root, _dirs, files in os.walk(system_base):
                    for fn in files:
                        if fn.lower().endswith(".raw565pack"):
                            dbg_has_raw565pack = True
                            break
                    if dbg_has_raw565pack:
                        break

            dbg_has_gif_pack = dbg_has_raw565pack

            print(
                f"[DEBUG] 64dd: "
                f"defaults/raw565={ 'Y' if dbg_has_raw565 else 'N' } | "
                f"systems/raw565pack={ 'Y' if dbg_has_raw565pack else 'N' } | "
                f"has_gif_pack={ 'Y' if dbg_has_gif_pack else 'N' } => ftype={ftype}"
            )

        entries[sys_lower] = (name, ftype)

    count = len(entries)
    print(tr("sysc_found")(count))

    stems = sorted(entries.keys())
    total = len(stems)
    with open(output_path, "w", encoding="utf-8", newline="\n") as out:
        for idx, stem in enumerate(stems, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break

            name, ftype = entries[stem]

            # Flag "lent" (L) par SOUS-DOSSIER ALPHABETIQUE (bucket #/A..Z),
            # pas par systeme entier (portage du worktree
            # dev/slow-flag-per-bucket, branche dev de test) : compte non
            # recursivement (os.scandir) chaque bucket + les eventuels
            # fichiers residuels laisses "a plat" directement sous
            # system_dir (contenu genere par une version anterieure de
            # l'outil, avant l'ajout du bucketing alphabetique) -- ceux-ci
            # sont assignes a leur bucket via _bucket_letter_for_stem(), la
            # MEME regle que celle qui les aurait ranges au moment de
            # l'ecriture. Seuil = slow_threshold (reglable, onglet
            # Parametres, v33), OR logique entre les 3 extensions, identique
            # a l'ancienne logique par-systeme mais applique par bucket.
            system_dir = systems_dir / name

            def count_ext_over_per_bucket(base: Path, ext_lower: str, limit: int) -> dict:
                counts = {letter: 0 for letter in LETTERS}
                for letter in LETTERS:
                    bucket_dir = base / letter
                    if not bucket_dir.is_dir():
                        continue
                    for entry in os.scandir(bucket_dir):
                        if entry.is_file() and entry.name.lower().endswith(ext_lower):
                            counts[letter] += 1
                # Residus non bucketises (a plat directement sous system_dir)
                for entry in os.scandir(base):
                    if entry.is_file() and entry.name.lower().endswith(ext_lower):
                        letter = _bucket_letter_for_stem(Path(entry.name).stem)
                        counts[letter] += 1
                return {letter: (c > limit) for letter, c in counts.items()}

            raw565_over_b = {letter: False for letter in LETTERS}
            raw565pack_over_b = {letter: False for letter in LETTERS}
            meta_over_b = {letter: False for letter in LETTERS}
            if system_dir.exists() and system_dir.is_dir():
                raw565_over_b = count_ext_over_per_bucket(system_dir, ".raw565", slow_threshold)
                raw565pack_over_b = count_ext_over_per_bucket(system_dir, ".raw565pack", slow_threshold)
                meta_over_b = count_ext_over_per_bucket(system_dir, ".meta", slow_threshold)

            bucket_flags = "".join(
                "L" if (raw565_over_b[letter] or raw565pack_over_b[letter] or meta_over_b[letter]) else "N"
                for letter in LETTERS
            )
            slow_flag = "L" if "L" in bucket_flags else "N"
            out.write(f"{ftype} {name} {slow_flag} {bucket_flags}\n")
            print(tr("sysc_line")(ftype, name))

            if progress_cb is not None:
                progress_cb("systems_cache", idx, total, stem)

    return count


def mode_systems_cache(sd_dir: Path):
    """Mode 5 : Génère systems_cache.dat depuis sd_card/systems/_defaults/"""
    title(tr("sysc_title"))

    default_systems = sd_dir / "systems"
    if default_systems.exists():
        print(f"{tr('cache_sys_detected')}{default_systems}")
        if ask_yes_no(tr("cache_sys_use")):
            systems_dir = default_systems
        else:
            print(tr("cache_sys_where"))
            sep("─")
            systems_dir = ask_path()
            if systems_dir is None:
                print(tr("back_main"))
                return
    else:
        print(f"{tr('cache_sys_missing')}{sd_dir}")
        print(tr("cache_sys_where"))
        sep("─")
        systems_dir = ask_path()
        if systems_dir is None:
            print(tr("back_main"))
            return

    sd_dir.mkdir(parents=True, exist_ok=True)
    output_path = sd_dir / "systems_cache.dat"

    count = build_systems_cache(systems_dir, output_path)

    sep()
    print(tr("done"))
    if count > 0:
        print(tr("sysc_done")(count, output_path))
        print(f"   💾 {output_path}")
        print(f"\n   {tr('sysc_copy')}")
        print(tr("sysc_hint"))
        return [output_path]
    return []


# ─────────────────────────────────────────────────────────────────────────────
#  MODE 6 — COPIE RAPIDE SUR CARTE SD (Windows / robocopy)
# ─────────────────────────────────────────────────────────────────────────────


def _list_removable_drives():
    """
    Liste les lecteurs amovibles sur Windows via WMI (wmic).
    Retourne une liste de tuples (lettre, label, taille_lisible).
    """
    import subprocess

    drives = []
    try:
        out = subprocess.check_output(
            [
                "wmic",
                "logicaldisk",
                "where",
                "drivetype=2",
                "get",
                "DeviceID,VolumeName,Size",
                "/format:csv",
            ],
            text=True,
            stderr=subprocess.DEVNULL,
        )
        for line in out.splitlines():
            line = line.strip()
            if not line or line.startswith("Node"):
                continue
            parts = line.split(",")
            if len(parts) < 4:
                continue
            _, device, size_str, label = parts[0], parts[1], parts[2], parts[3]
            letter = device.strip()
            label = label.strip() or "NO LABEL"
            try:
                size_gb = int(size_str.strip()) / (1024**3)
                size_s = f"{size_gb:.1f} GB"
            except Exception:
                size_s = "? GB"
            if letter:
                drives.append((letter, label, size_s))
    except Exception:
        pass
    return drives


def _list_removable_drives_ex():
    """
    Fonction soeur de _list_removable_drives() (celle-ci NON modifiee : 4
    sites d'appel existants font un unpacking strict a 3 valeurs) --
    rajoute le systeme de fichiers via wmic logicaldisk ... get
    DeviceID,FileSystem,VolumeName,Size /format:csv. Attention : les
    colonnes /format:csv sortent triees par ordre ALPHABETIQUE du nom de
    propriete demande, pas par l'ordre donne a "get" -- verifie
    empiriquement (wmic direct hors Python) : "DeviceID,FileSystem,
    VolumeName,Size" -> ordre reel "Node,DeviceID,FileSystem,Size,
    VolumeName". Retourne une liste de tuples (lettre, label, taille_lisible,
    filesystem).
    """
    import subprocess

    drives = []
    try:
        out = subprocess.check_output(
            [
                "wmic",
                "logicaldisk",
                "where",
                "drivetype=2",
                "get",
                "DeviceID,FileSystem,VolumeName,Size",
                "/format:csv",
            ],
            text=True,
            stderr=subprocess.DEVNULL,
        )
        for line in out.splitlines():
            line = line.strip()
            if not line or line.startswith("Node"):
                continue
            parts = line.split(",")
            if len(parts) < 5:
                continue
            device, fs, size_str, label = parts[1], parts[2], parts[3], parts[4]
            letter = device.strip()
            label = label.strip() or "NO LABEL"
            fs = fs.strip() or "?"
            try:
                size_gb = int(size_str.strip()) / (1024**3)
                size_s = f"{size_gb:.1f} GB"
            except Exception:
                size_s = "? GB"
            if letter:
                drives.append((letter, label, size_s, fs))
    except Exception:
        pass
    return drives


def check_drive_fat32_and_min_size(letter: str, min_gb: float = 8.0) -> tuple[bool, str]:
    """
    Verifie qu'un lecteur amovible est formate en FAT32 et fait au moins
    min_gb. Utilise shutil.disk_usage() (octets exacts) plutot que de
    re-parser la chaine deja arrondie "X.X GB" de _list_removable_drives_ex().
    Retourne (True, "ok") si valide, sinon (False, raison) :
    "drive_not_found" (lettre absente de la liste des lecteurs amovibles),
    "filesystem=<fs>" (pas FAT32), "size_gb=<n>" (sous le seuil).
    """
    norm = letter.strip().rstrip("\\")
    if not norm.endswith(":"):
        norm += ":"

    drives = _list_removable_drives_ex()
    found = None
    for d_letter, _label, _size_s, fs in drives:
        if d_letter.rstrip("\\").upper() == norm.upper():
            found = fs
            break
    if found is None:
        return False, "drive_not_found"
    if found.upper() != "FAT32":
        return False, f"filesystem={found}"

    try:
        total, _used, _free = shutil.disk_usage(f"{norm}\\")
    except OSError:
        return False, "drive_not_found"
    total_gb = total / (1024**3)
    if total_gb < min_gb:
        return False, f"size_gb={total_gb:.1f}"
    return True, "ok"


def _copy_one_file(src_file: Path, dst_file: Path, overwrite: bool) -> tuple[str, str]:
    """
    Copie atomique d'un seul fichier : ecrit dans un fichier temporaire
    "<nom>.part" a cote de la destination, puis os.replace() vers le nom
    final seulement une fois la copie terminee. Un crash ou une carte SD
    debranchee en cours d'ecriture laisse au pire un ".part" orphelin
    (jamais pris pour "deja copie" au prochain lancement), jamais un
    fichier final corrompu.

    Retourne (status, detail) avec status in {"copied", "skipped", "failed"}.
    """
    tmp_file = dst_file.with_name(dst_file.name + ".part")
    try:
        if dst_file.exists() and not overwrite:
            try:
                if dst_file.stat().st_size == src_file.stat().st_size:
                    return "skipped", ""
            except OSError:
                pass
            # Taille differente (ou stat impossible) : fichier incomplet
            # d'une copie precedente -> on le recopie.
        try:
            if tmp_file.exists():
                tmp_file.unlink()
        except OSError:
            pass
        shutil.copy2(src_file, tmp_file)
        os.replace(tmp_file, dst_file)
        return "copied", ""
    except PermissionError:
        return (
            "failed",
            "PERMISSION REFUSEE (carte SD protegee en ecriture ou lecteur verrouille ?)",
        )
    except OSError as e:
        return "failed", str(e)
    except Exception as e:
        return "failed", str(e)
    finally:
        # Nettoyage best-effort d'un .part residuel (echec de cette
        # tentative, ou orphelin d'un crash precedent sur ce fichier).
        try:
            if tmp_file.exists():
                tmp_file.unlink()
        except Exception:
            pass


def _write_copy_manifest(manifest_path: Optional[Path], data: dict) -> None:
    if manifest_path is None:
        return
    try:
        payload = dict(data)
        payload["timestamp"] = time.strftime("%Y-%m-%d %H:%M:%S")
        manifest_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    except Exception:
        pass


def read_copy_manifest(manifest_path: Path) -> Optional[dict]:
    """Lit le manifest de copie SD s'il existe, sinon None."""
    try:
        if not manifest_path.exists():
            return None
        return json.loads(manifest_path.read_text(encoding="utf-8"))
    except Exception:
        return None


# Dossiers top-level de src a ne JAMAIS copier vers la carte SD du DMD --
# "recalbox_userscripts" (mise en scene locale des scripts Recalbox, Mode
# 1) est destine a une copie MANUELLE vers le partage reseau
# \\<host>\share\userscripts de la Recalbox, pas vers la SD.
_SD_COPY_EXCLUDED_DIRS = ("recalbox_userscripts",)


def _walk_for_sd_copy(src: Path):
    """os.walk(src) qui saute les dossiers top-level de _SD_COPY_EXCLUDED_DIRS."""
    for root, dirs, files in os.walk(str(src)):
        if Path(root) == src:
            dirs[:] = [d for d in dirs if d not in _SD_COPY_EXCLUDED_DIRS]
        yield root, dirs, files


def _copy_to_drive(
    src: Path,
    dst: str,
    overwrite: bool,
    progress_cb=None,
    manifest_path: Optional[Path] = None,
):
    """
    Copie fichier par fichier avec progression et logs.
    - 100k+ fichiers viable : print toutes les 50 lignes (évite de noyer le Text widget)
    - Erreurs claires : fichier → raison (ex: carte SD protégée, espace insuffisant)
    - Écriture atomique (voir _copy_one_file) + skip par comparaison de
      taille : une reprise après crash/débranchement ne saute jamais un
      fichier corrompu/partiel.
    - Arrêt immédiat si la destination devient inaccessible (carte SD
      débranchée en cours de copie), au lieu d'échouer fichier par fichier.
    - manifest_path : si fourni, un état JSON est écrit (et mis à jour
      périodiquement) pour permettre de détecter une copie interrompue au
      prochain lancement.
    - Retourne (copied, failed_files, interrupted) : interrupted=True si la
      destination est devenue inaccessible en cours de copie (à distinguer
      d'échecs fichier par fichier, qui remplissent failed_files).
    """
    dst_root_path = Path(dst)
    total_files = sum(len(files) for _, _, files in _walk_for_sd_copy(src))
    copied = 0
    skipped = 0
    failed: list[str] = []
    interrupted = False
    print(tr("copy_files_total")(total_files, dst))
    log_throttle = 0
    manifest_throttle = 0
    # Throttle par TEMPS (pas par comptage : la taille/nombre de fichiers
    # varie trop) pour eviter d'inonder la file d'evenements Tk (root.after)
    # avec des milliers de petits fichiers .raw565 -> barre de progression
    # qui "tremblote". Le dernier appel (apres la boucle, plus bas) reste
    # toujours envoye sans throttle -> garantit 100% + libelle final.
    last_progress_cb_time = 0.0
    PROGRESS_CB_MIN_INTERVAL = 0.1

    def _manifest(status: str) -> None:
        _write_copy_manifest(
            manifest_path,
            {
                "source": str(src),
                "destination": str(dst),
                "total_files": total_files,
                "copied": copied,
                "skipped": skipped,
                "failed": failed,
                "status": status,
            },
        )

    _manifest("in_progress")

    for root, dirs, files in _walk_for_sd_copy(src):
        if not dst_root_path.exists():
            print(tr("copy_sd_unreachable")(copied, total_files))
            interrupted = True
            break

        rel = Path(root).relative_to(src)
        dst_root = dst_root_path / rel
        dst_root.mkdir(parents=True, exist_ok=True)
        for fname in files:
            if fname.endswith(".part"):
                continue

            # Verifie a chaque fichier (pas seulement a chaque dossier) :
            # plusieurs milliers de fichiers peuvent partager un seul
            # dossier (ex: systems/<sys>/<Lettre>/), donc une carte SD
            # debranchee en cours de route doit interrompre immediatement,
            # pas apres avoir echoue sur tout le reste du dossier.
            if not dst_root_path.exists():
                print(
                    f"   ❌ CARTE SD INACCESSIBLE — copie arrêtée à {copied}/{total_files} fichiers"
                )
                interrupted = True
                break

            src_file = Path(root) / fname
            dst_file = dst_root / fname

            status, detail = _copy_one_file(src_file, dst_file, overwrite)
            log_throttle += 1
            if status == "failed":
                print(tr("copy_file_error")(rel / fname, detail))
                failed.append(str(rel / fname))
            else:
                copied += 1
                if status == "skipped":
                    skipped += 1
                if log_throttle % 50 == 0:
                    tag = tr("copy_tag_skipped") if status == "skipped" else tr("copy_tag_ok")
                    print(tr("copy_progress_line")('⏭️ ' if status == 'skipped' else '✅', copied, total_files, rel / fname, tag))

            if progress_cb:
                now = time.monotonic()
                if now - last_progress_cb_time >= PROGRESS_CB_MIN_INTERVAL:
                    last_progress_cb_time = now
                    progress_cb("copy_sd", copied, total_files, str(rel / fname))

            manifest_throttle += 1
            if manifest_throttle >= 25:
                manifest_throttle = 0
                _manifest("in_progress")

        if interrupted:
            break

    final_status = "interrupted" if interrupted else "completed"
    _manifest(final_status)

    if interrupted:
        print(tr("copy_interrupted")(copied, total_files))
    elif failed:
        print(tr("copy_done_with_failed")(copied, total_files, len(failed)))
    else:
        print(tr("copy_done_all")(copied, total_files))
    if progress_cb:
        progress_cb(
            "copy_sd",
            copied,
            total_files,
            tr("copy_status_interrupted") if interrupted else tr("copy_status_done"),
        )
    return copied, failed, interrupted


def _copy_specific_files(
    src: Path,
    dst: str,
    rel_paths: list[str],
    overwrite: bool = True,
    progress_cb=None,
) -> tuple[int, list[str]]:
    """
    Copie uniquement les fichiers indiqués (chemins relatifs à src) vers
    dst. Utilisé pour réessayer seulement les fichiers en échec d'une
    copie précédente, sans reparcourir toute l'arborescence.
    """
    dst_root_path = Path(dst)
    total = len(rel_paths)
    copied = 0
    failed: list[str] = []
    print(tr("retry_total")(total, dst))
    for i, relp in enumerate(rel_paths, 1):
        if not dst_root_path.exists():
            print(tr("retry_sd_unreachable"))
            failed.extend(rel_paths[i - 1 :])
            break

        src_file = src / relp
        dst_file = dst_root_path / relp
        dst_file.parent.mkdir(parents=True, exist_ok=True)

        status, detail = _copy_one_file(src_file, dst_file, overwrite)
        if status == "failed":
            print(tr("copy_file_error")(relp, detail))
            failed.append(relp)
        else:
            copied += 1
            print(tr("retry_file_ok")(i, total, relp))
        if progress_cb:
            progress_cb("copy_sd_retry", i, total, relp)

    if failed:
        print(tr("retry_done_with_failed")(copied, total, len(failed)))
    else:
        print(tr("retry_done_all")(copied, total))
    return copied, failed


def _robocopy(src: Path, dst: str, overwrite: bool = True, progress_cb=None) -> None:
    """
    Lance robocopy avec /MT:32 pour une copie rapide.
    overwrite=True  → /IS /IT (écrase les fichiers identiques aussi)
    overwrite=False → /XC /XN /XO (ignore fichiers plus récents/identiques/anciens)
    Autorise la récupération d'erreur (/R:3 /W:5).
    Liste les fichiers copiés dans les logs.
    """
    import subprocess

    src_str = str(src)
    # On retire /NFL et /NP pour voir les fichiers dans la sortie.
    # "recalbox_userscripts" (mise en scene locale des scripts Recalbox,
    # Mode 1) est exclu : destine a etre copie a la main vers le partage
    # reseau \\<host>\share\userscripts de la Recalbox, pas vers la carte
    # SD du DMD.
    flags = [
        "/E", "/MT:32", "/NJH", "/R:3", "/W:5",
        "/XD", "logs", "log", "recalbox_userscripts",
    ]
    if overwrite:
        flags += ["/IS", "/IT"]
    else:
        flags += ["/XC", "/XN", "/XO"]

    cmd = ["robocopy", src_str, dst] + flags
    print(tr("flash_copy_start")(src_str, dst))

    # Compter les fichiers source
    total_files = sum(len(files) for _, _, files in os.walk(src_str))
    print(tr("copy_files_from")(total_files, src_str))
    copied = 0

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
    )

    for line in proc.stdout:
        line = line.rstrip()
        if not line:
            continue
        # Robocopy liste les fichiers sous forme "nouveau fichier  →  chemin"
        if "→" in line or "\t" in line:
            copied += 1
            # Afficher le fichier proprement sans interférence de spinner
            print(tr("copy_file_progress")(copied, total_files, line.strip()))
            if progress_cb:
                progress_cb("copy_sd", copied, total_files, line.strip()[:55])

    proc.wait()
    if proc.returncode is None:
        proc.returncode = 0

    if proc.returncode < 4:
        print(tr("copy_done_all")(copied, total_files))
        if progress_cb:
            progress_cb("copy_sd", copied, total_files, tr("copy_status_done"))
    else:
        msg = tr("flash_copy_err")(proc.returncode, copied, total_files)
        print(f"   {msg}")
        if progress_cb:
            progress_cb("copy_sd", copied, total_files, f"⚠️ code {proc.returncode}")
        raise RuntimeError(msg)


def _flash_files(files: list, sd_dir: Path):
    """Copie une liste de fichiers précis sur une carte SD (Windows uniquement)."""
    import subprocess

    if sys.platform != "win32":
        print(tr("flash_no_win"))
        return

    if not files:
        print(tr("flash_no_sdcard"))
        return

    while True:
        print(tr("flash_drives_title"))
        drives = _list_removable_drives()

        if not drives:
            print(tr("flash_no_drives"))
            input(tr("press_enter"))
            return

        for i, (letter, label, size) in enumerate(drives, 1):
            print(f"  {i}  →  {letter}\\  [{label}]  {size}")
        print(f"  0  →  {tr('back')}")
        print()

        raw = input(tr("flash_drive_choice")).strip()
        if raw == "0":
            print(tr("back_main"))
            return
        if not raw.isdigit() or not (1 <= int(raw) <= len(drives)):
            print(tr("flash_drive_warn"))
            continue

        letter, label, size = drives[int(raw) - 1]
        dst_drive = Path(f"{letter}\\")
        print(tr("flash_drive_sel")(f"{letter}\\  [{label}]", size))
        print()

        for src in files:
            dst = dst_drive / src.name
            print(f"📋 {src.name}  →  {dst}")
            try:
                import shutil as _shutil

                _shutil.copy2(src, dst)
                print(f"   ✅ OK")
            except Exception as e:
                print(f"   ❌ {e}")
        print()
        print(tr("flash_copy_ok"))
        return


def mode_flash_sd(sd_dir: Path):
    """Mode 6 : Copie rapide du contenu sd_card/ sur une carte SD via robocopy."""
    title(tr("flash_title"))

    # ── Vérification Windows ──────────────────────────────────────────────────
    if sys.platform != "win32":
        print(tr("flash_no_win"))
        return

    # ── Vérification sd_card/ non vide ───────────────────────────────────────
    if not sd_dir.exists() or not any(sd_dir.iterdir()):
        print(tr("flash_no_sdcard"))
        return

    while True:
        # ── Liste des lecteurs amovibles ──────────────────────────────────────
        print(tr("flash_drives_title"))
        drives = _list_removable_drives()

        if not drives:
            print(tr("flash_no_drives"))
            input(tr("press_enter"))
            return

        for i, (letter, label, size) in enumerate(drives, 1):
            print(f"  {i}  →  {letter}\\  [{label}]  {size}")
        print(f"  0  →  {tr('back')}")
        print()

        raw = input(tr("flash_drive_choice")).strip()
        if raw == "0":
            print(tr("back_main"))
            return
        if not raw.isdigit() or not (1 <= int(raw) <= len(drives)):
            print(tr("flash_drive_warn"))
            continue

        letter, label, size = drives[int(raw) - 1]
        dst_drive = f"{letter}\\"
        print(tr("flash_drive_sel")(f"{letter}\\  [{label}]", size))

        # ── Choix du mode de copie ────────────────────────────────────────────
        print(tr("flash_mode_title"))
        print(f"  1  →  {tr('flash_mode_opt2')}")
        print(f"  2  →  {tr('flash_mode_opt3')}")
        print(f"  0  →  {tr('back')}")
        print()

        while True:
            raw2 = input(tr("flash_mode_choice")).strip()
            if raw2 == "0":
                break
            if raw2 in ("1", "2"):
                break
            print(tr("flash_mode_warn"))

        if raw2 == "0":
            continue  # retour au choix du lecteur

        # ── Copie robocopy ────────────────────────────────────────────────────
        overwrite = raw2 == "1"
        _robocopy(sd_dir, dst_drive, overwrite)

        sep()
        print(tr("done"))
        return


# ─────────────────────────────────────────────────────────────────────────────
#  SÉLECTION DE LANGUE
# ─────────────────────────────────────────────────────────────────────────────


def select_language():
    global T, CURRENT_LANG
    print()
    print("╔══════════════════════════════════════════════════════════════════╗")
    print("║          RetroBoxLED Toolkit for Recalbox                        ║")
    print("╚══════════════════════════════════════════════════════════════════╝")
    print()
    print("  Choisissez votre langue / Choose your language / Elija su idioma")
    print()
    print("  1  →  English")
    print("  2  →  Français")
    print("  3  →  Español")
    print()
    while True:
        raw = input("  > ").strip()
        if raw == "1":
            T = TRANSLATIONS["en"]
            CURRENT_LANG = "en"
            return
        if raw == "2":
            T = TRANSLATIONS["fr"]
            CURRENT_LANG = "fr"
            return
        if raw == "3":
            T = TRANSLATIONS["es"]
            CURRENT_LANG = "es"
            return
        print("  ⚠️  1 / 2 / 3\n")


# ─────────────────────────────────────────────────────────────────────────────
#  POINT D'ENTRÉE
# ─────────────────────────────────────────────────────────────────────────────


def show_main_menu():
    """Affiche le menu principal (onglet Main) et retourne le choix."""
    print()
    sep()
    print(f"  {tr('main_title')}  —  {tr('advanced_back')}")
    sep()
    print()
    print(f"  {tr('main_prompt')}")
    print()
    print(f"  1  →  {tr('main_opt1')}")
    print(f"  2  →  {tr('main_opt_advanced')}")
    print(f"  0  →  {tr('main_opt_quit')}")
    print()
    while True:
        choix = input(tr("main_choice")).strip()
        if choix in ("0", "1", "2"):
            break
        print(tr("main_warn"))
    print()
    return choix


def show_advanced_menu():
    """Affiche le menu avancé (onglet Avancé) et retourne le choix."""
    print()
    sep()
    print(f"  {tr('advanced_title')}")
    sep()
    print()
    print(f"  {tr('advanced_prompt')}")
    print()
    print(f"  2  →  {tr('main_opt2_desc')}")
    print(f"  3  →  {tr('main_opt2')}")
    print(f"  4  →  {tr('main_opt3')}")
    print(f"  5  →  {tr('main_opt4')}")
    print(f"  6  →  {tr('main_opt5')}")
    print(f"  7  →  {tr('main_opt6')}")
    print(f"  8  →  {tr('main_opt7')}")
    print(f"  9  →  {tr('mode9_short_title')}")
    print(f"  0  →  {tr('advanced_back')}")
    print()
    while True:
        choix = input(tr("main_choice")).strip()
        if choix in ("0", "2", "3", "4", "5", "6", "7", "8", "9"):
            break
        print(tr("main_warn"))
    print()
    return choix


def show_after_menu(choix, generated_files):
    """Menu de fin après exécution d'un mode."""
    print()
    sep("─")
    print(f"  {tr('after_menu')}")
    print()
    print(f"  1  →  {tr('after_opt1')}")
    if choix in ("6", "7"):
        if generated_files:
            print(f"  2  →  {tr('after_opt_files')}")
    else:
        print(f"  2  →  {tr('after_opt6')}")
    print(f"  0  →  {tr('press_enter').replace('...', '')}")
    print()

    valid = ["0", "1"]
    if choix not in ("6", "7") or generated_files:
        valid.append("2")

    while True:
        raw = input("  > ").strip()
        if raw in valid:
            break
        print(tr("main_warn"))
    return raw


def run_mode(choix, sd_dir):
    """Exécute le mode choisi et retourne les fichiers générés."""
    generated_files = []
    if choix == "1":
        mode_full(sd_dir)
    elif choix == "2":
        mode_extract_download_defaults_only(sd_dir)
    elif choix == "3":
        mode_extract_only(sd_dir)
    elif choix == "4":
        mode_convert_only(sd_dir)
    elif choix == "5":
        mode_convert_128_only(sd_dir)
    elif choix == "6":
        generated_files = mode_cache_only(sd_dir) or []
    elif choix == "7":
        generated_files = mode_systems_cache(sd_dir) or []
    elif choix == "8":
        mode_flash_sd(sd_dir)
    elif choix == "9":
        mode_install_recalbox_scripts_console()
    return generated_files


def main():
    select_language()
    ensure_dependencies()

    script_dir = Path(__file__).parent
    sd_dir = get_sd_card_dir(script_dir)

    # Choix initial : menu principal (onglet Main)
    choix = show_main_menu()

    while True:
        if choix == "0":
            break

        # Si choix == 2 en menu principal → basculer vers le menu avancé
        if choix == "2":
            choix = show_advanced_menu()
            if choix == "0":
                # Retour au menu principal
                choix = show_main_menu()
            continue

        generated_files = run_mode(choix, sd_dir)

        # ── Menu de fin ───────────────────────────────────────────────────────
        raw = show_after_menu(choix, generated_files)

        if raw == "0":
            break
        elif raw == "1":
            # Retour au menu principal
            choix = show_main_menu()
        elif raw == "2":
            if choix in ("5", "6") and generated_files:
                _flash_files(generated_files, sd_dir)
                # Menu de fin après copie ciblée
                print()
                sep("─")
                print(f"  {tr('after_menu')}")
                print()
                print(f"  1  →  {tr('after_opt1')}")
                print(f"  0  →  {tr('press_enter').replace('...', '')}")
                print()
                while True:
                    raw2 = input("  > ").strip()
                    if raw2 in ("0", "1"):
                        break
                    print(tr("main_warn"))
                if raw2 == "0":
                    break
                elif raw2 == "1":
                    choix = show_main_menu()
            else:
                choix = show_advanced_menu()


RECALBOX_PROFILES = {
    "10.x": {
        "tag": "logo",
        "img_subdir": "media/wheels",
        "extensions": [".png"],
        "description": "Recalbox 10.x -- logo dans media/wheels/",
    },
    "9.x": {
        # La marquee doit etre scrapee dans le champ "Selectionnez le type
        # de vignette" (pas "Selectionnez le type d'image", qui sert
        # generalement a un autre visuel) : voir aide "Comment scraper ?"
        # dans le GUI Mode 1.
        "tag": "thumbnail",
        "img_subdir": "media/thumbnails",
        "extensions": [".png", ".gif"],
        "description": "Recalbox 9.x -- marquee dans media/thumbnails/ (champ vignette du scraper)",
    },
    "legacy": {
        "tag": "image",
        "img_subdir": "media/images",
        "extensions": [".png", ".gif"],
        "description": "Legacy -- marquee dans media/images/ (champ image du scraper)",
    },
}


def _detect_profile_systems(
    roms_root: Path, selected_systems: Optional[list[Path]] = None
) -> list[Path]:
    """Liste les dossiers systeme cibles (chacun avec son propre gamelist.xml)."""
    if selected_systems:
        return [s for s in selected_systems if (s / "gamelist.xml").exists()]
    systems: list[Path] = []
    for d in roms_root.iterdir():
        if d.is_dir() and (d / "gamelist.xml").exists():
            systems.append(d)
    systems.sort(key=lambda p: p.name.lower())
    return systems


def list_scrape_media_files(
    roms_root: Path,
    selected_systems: Optional[list[Path]] = None,
    profile_name: str = "10.x",
) -> dict:
    """
    Parcourt chaque systeme cible (roms_root contient un sous-dossier par
    systeme, chacun avec son propre gamelist.xml ET son propre dossier
    media) et liste les fichiers presents dans
    "<systeme>/<img_subdir du profil>/" (images, thumbnails ou
    media/wheels selon le profil Recalbox choisi).

    Retourne un apercu (sans rien supprimer) utilise pour la confirmation
    avant nettoyage :
      {
          "profile": profile_name,
          "img_subdir": "media/wheels",
          "by_system": {sys_name: [Path, ...], ...},
          "total": n,
      }
    """
    profile = RECALBOX_PROFILES.get(profile_name, RECALBOX_PROFILES["10.x"])
    img_subdir = profile["img_subdir"]
    extensions = profile["extensions"]

    systems = _detect_profile_systems(roms_root, selected_systems)

    by_system: dict[str, list[Path]] = {}
    total = 0
    for sys_dir in systems:
        img_dir = sys_dir / img_subdir
        if not img_dir.exists():
            continue
        files: list[Path] = []
        for ext in extensions:
            files.extend(img_dir.glob(f"*{ext}"))
        if files:
            by_system[sys_dir.name] = files
            total += len(files)

    return {
        "profile": profile_name,
        "img_subdir": img_subdir,
        "by_system": by_system,
        "total": total,
    }


def clean_scrape_media_folders(
    roms_root: Path,
    selected_systems: Optional[list[Path]] = None,
    profile_name: str = "10.x",
    progress_cb=None,
) -> dict:
    """
    Supprime, systeme par systeme, les fichiers image du dossier media
    correspondant au profil Recalbox choisi (ex: "media/wheels" pour
    "10.x"). Ne supprime QUE les fichiers listes par
    list_scrape_media_files() (extensions du profil) -- jamais
    gamelist.xml, les ROMs, ni les dossiers media des AUTRES profils.
    Le dossier lui-meme est conserve (seuls les fichiers a l'interieur
    sont retires), pour que le prochain scrape Recalbox reparte de zero
    dans ce dossier.

    A appeler uniquement apres confirmation explicite de l'utilisateur
    (operation destructive sur le partage ROMs reel).
    """
    preview = list_scrape_media_files(roms_root, selected_systems, profile_name)
    by_system = preview["by_system"]

    deleted = 0
    errors: list[str] = []
    by_system_deleted: dict[str, int] = {}

    total_systems = max(len(by_system), 1)
    for i, (sys_name, files) in enumerate(by_system.items(), 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop():
            break

        sys_deleted = 0
        for f in files:
            try:
                f.unlink()
                sys_deleted += 1
                deleted += 1
            except OSError as e:
                errors.append(f"{sys_name}/{f.name} : {e}")

        by_system_deleted[sys_name] = sys_deleted
        if progress_cb is not None:
            progress_cb("clean_scrape_media", i, total_systems, sys_name)

    return {
        "profile": profile_name,
        "img_subdir": preview["img_subdir"],
        "deleted": deleted,
        "errors": errors,
        "by_system": by_system_deleted,
    }


def check_missing_images_gamelist(
    roms_root: Path,
    selected_systems: Optional[list[Path]] = None,
    profile_name: str = "10.x",
    progress_cb=None,
) -> dict:
    """
    Parcourt les gamelist.xml du dossier ROMs et verifie pour chaque jeu
    si le fichier image reference par la balise <logo>/<image>/<thumbnail>
    (selon le profil Recalbox) existe reellement sur le disque.

    Le gamelist.xml donne le chemin exact de l'image (resolu via
    resolve_image_path, relatif a sys_dir ou absolu) : c'est exactement ce
    que Recalbox charge, donc pas besoin de deviner/reconstruire un nom de
    fichier a partir du nom de la ROM.
    """
    profile = RECALBOX_PROFILES.get(profile_name, RECALBOX_PROFILES["10.x"])
    tag = profile["tag"]

    systems: list[Path] = []
    if selected_systems:
        systems = [s for s in selected_systems if (s / "gamelist.xml").exists()]
    else:
        for d in roms_root.iterdir():
            if d.is_dir() and (d / "gamelist.xml").exists():
                systems.append(d)
        systems.sort(key=lambda p: p.name.lower())

    if not systems:
        print(tr("m1_no_systems"))
        return {
            "profile": profile_name,
            "roms_root": roms_root,
            "total_games": 0,
            "total_missing": 0,
            "total_present": 0,
            "systems": {},
            "missing_flat": [],
        }

    print(tr("m1_systems_detected")(len(systems)))
    print(tr("m1_tag_xml")(tag))
    print(tr("m1_profile")(profile["description"]))
    print()

    # Pre-comptage du nombre total de jeux (pour la progression globale).
    global_total_games = 0
    for sys_dir in systems:
        try:
            global_total_games += len(parse_gamelist(sys_dir / "gamelist.xml"))
        except ET.ParseError:
            pass
    global_total_games = max(global_total_games, 1)
    global_done_games = 0

    result: dict = {}
    total_games = 0
    total_missing = 0
    total_present = 0
    missing_flat: list[tuple[str, str, Path]] = []

    for sys_dir in systems:
        sys_name = sys_dir.name
        print(tr("m1_analyzing")(sys_name))

        try:
            games = parse_gamelist(sys_dir / "gamelist.xml")
        except ET.ParseError as e:
            print(tr("m1_xml_error")(e))
            continue

        sys_games = 0
        sys_missing = 0
        sys_present = 0
        sys_missing_list: list[tuple[str, Path]] = []

        for game in games:
            PAUSE.wait_if_paused()
            if PAUSE.should_stop():
                break

            global_done_games += 1
            if progress_cb is not None:
                progress_cb(
                    "mode8_check", global_done_games, global_total_games, sys_name
                )

            path_elem = game.find("path")
            if path_elem is None:
                continue
            raw_path = unquote(path_elem.text or "").strip()
            if not raw_path:
                continue

            game_name = Path(raw_path).stem

            img_elem = game.find(tag)
            if img_elem is None or not (img_elem.text or "").strip():
                continue

            image_raw = unquote(img_elem.text.strip())
            expected = resolve_image_path(sys_dir, image_raw)

            if expected.exists():
                sys_present += 1
            else:
                sys_missing += 1
                sys_missing_list.append((game_name, expected))
                missing_flat.append((sys_name, game_name, expected))

            sys_games += 1

        total_games += sys_games
        total_missing += sys_missing
        total_present += sys_present

        result[sys_name] = {
            "games": sys_games,
            "missing": sys_missing,
            "present": sys_present,
            "missing_list": sys_missing_list,
        }

        if sys_games > 0:
            print(tr("m1_games_summary")(sys_games, sys_present, sys_missing))
        else:
            print(tr("m1_no_games"))

        if sys_missing_list:
            for gname, _ in sys_missing_list[:3]:
                print(f"         {gname}")
            if len(sys_missing_list) > 3:
                print(tr("m1_more_others")(len(sys_missing_list) - 3))

    print()
    print(f"{'='*50}")
    print(tr("m1_result_summary")(total_games, total_present, total_missing))
    print(f"{'='*50}")

    return {
        "profile": profile_name,
        "roms_root": roms_root,
        "total_games": total_games,
        "total_missing": total_missing,
        "total_present": total_present,
        "systems": result,
        "missing_flat": missing_flat,
    }


def generate_missing_images_report(result: dict, sd_dir: Path) -> Path:
    """Genere un rapport texte dans sd_dir/reports/."""
    report_dir = sd_dir / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)
    report_path = report_dir / "mode8_report_{}.txt".format(
        time.strftime("%Y%m%d_%H%M%S")
    )
    lines = [
        "=" * 60,
        "MODE 8 REPORT - Missing images (gamelist.xml verification)",
        "=" * 60,
        "Date         : " + time.strftime("%Y-%m-%d %H:%M:%S"),
        "Profile      : " + str(result.get("profile", "N/A")),
        "ROMs folder  : " + str(result.get("roms_root", "N/A")),
        "Total games  : " + str(result["total_games"]),
        "Present      : " + str(result["total_present"]),
        "Missing      : " + str(result["total_missing"]),
        "",
    ]
    systems = result.get("systems", {})
    if systems:
        lines.append("DETAIL BY SYSTEM:")
        lines.append("-" * 40)
        for sys_name in sorted(systems.keys()):
            sys_data = systems[sys_name]
            if sys_data["missing"] == 0:
                continue
            lines.append(
                "[{}] ({} missing out of {} game(s)):".format(
                    sys_name, sys_data["missing"], sys_data["games"]
                )
            )
            for game_name, expected_path in sys_data["missing_list"]:
                lines.append("   {} -> {}".format(game_name, expected_path))
            lines.append("")
    elif result.get("missing_flat"):
        lines.append("DETAIL OF MISSING IMAGES:")
        lines.append("-" * 40)
        for sys_name, game_name, expected_path in result["missing_flat"]:
            lines.append("[{}] {}".format(sys_name, game_name))
            lines.append("  -> {}".format(expected_path))
        lines.append("")
        lines.append("Total missing: " + str(result["total_missing"]))
    else:
        lines.append("No missing images!")
    lines.append("=" * 60)
    report_path.write_text("\n".join(lines), encoding="utf-8")
    print("Report generated: " + str(report_path))
    return report_path


def _expected_final_paths(
    systems_out: Path, sys_name: str, game_name: str, src_ext: str
) -> list[Path]:
    """
    Chemin(s) attendu(s) sur le support final pour un jeu donne, avec le
    meme bucketing alphabetique A..Z/# que la conversion reelle
    (voir _alpha_subdir_if_needed) :
      - PNG -> <game_name>.raw565
      - GIF -> <game_name>.raw565pack + <game_name>.meta
    """
    base = systems_out / sys_name
    if src_ext.lower() == ".gif":
        return [
            _alpha_subdir_if_needed(base / f"{game_name}.raw565pack"),
            _alpha_subdir_if_needed(base / f"{game_name}.meta"),
        ]
    return [_alpha_subdir_if_needed(base / f"{game_name}.raw565")]


def check_final_media(
    roms_root: Path,
    systems_out: Path,
    sd_root: Optional[Path] = None,
    selected_systems: Optional[list[Path]] = None,
    profile_name: str = "10.x",
    progress_cb=None,
) -> dict:
    """
    Etend la verification Mode 8 (gamelist.xml) en comparant avec ce qui
    existe reellement sur le support final :
      - systems_out : dossier temporaire de staging (sd_dir/systems)
        toujours verifie -> revele les echecs de conversion (PNG/GIF source
        mal formate, conversion non executee).
      - sd_root : racine de la carte SD physique (optionnel). Si fourni,
        verifie en plus systems_out vs sd_root/systems -> revele les
        echecs de copie vers la SD.

    Chaque jeu recoit un statut :
      OK                 : image trouvee au niveau attendu par la demande
      MISSING_SOURCE      : image absente cote ROMs (gamelist.xml)
      MISSING_CONVERTED   : source presente mais absente du dossier temporaire
      MISSING_ON_SD       : absente du cote SD (verification basee sur la SD)

    Si le sous-dossier d'un systeme (ex: 3do, amiga600...) est totalement
    absent d'un des deux cotes (dossier temporaire OU SD), ce cote est
    ignore pour ce systeme : on ne compare qu'avec l'autre cote, plutot que
    de signaler chaque jeu comme "absent" a cause d'un dossier entier
    manquant (qui n'a simplement pas encore ete extrait/copie a cet
    endroit). Si les deux cotes manquent ce systeme, les jeux ne sont pas
    verifies (comptes a part dans "not_checked").
    """
    profile = RECALBOX_PROFILES.get(profile_name, RECALBOX_PROFILES["10.x"])
    tag = profile["tag"]
    sd_systems_out = (sd_root / "systems") if sd_root is not None else None

    systems: list[Path] = []
    if selected_systems:
        systems = [s for s in selected_systems if (s / "gamelist.xml").exists()]
    else:
        for d in roms_root.iterdir():
            if d.is_dir() and (d / "gamelist.xml").exists():
                systems.append(d)
        systems.sort(key=lambda p: p.name.lower())

    # Pre-comptage du nombre total de jeux (pour la progression globale).
    global_total_games = 0
    for sys_dir in systems:
        try:
            global_total_games += len(parse_gamelist(sys_dir / "gamelist.xml"))
        except ET.ParseError:
            pass
    global_total_games = max(global_total_games, 1)
    global_done_games = 0

    entries: list[dict] = []
    counts = {
        "ok": 0,
        "missing_source": 0,
        "missing_converted": 0,
        "missing_on_sd": 0,
        "not_checked": 0,
    }

    for sys_dir in systems:
        sys_name = sys_dir.name
        try:
            games = parse_gamelist(sys_dir / "gamelist.xml")
        except ET.ParseError as e:
            print(tr("m1_xml_error_sys")(sys_name, e))
            continue

        # Un sous-dossier de systeme totalement absent d'un cote (temp ou
        # SD) n'est pas une "image manquante" jeu par jeu : c'est ce cote
        # entier qui n'a pas encore ete extrait/copie pour ce systeme.
        staging_has_system = (systems_out / sys_name).exists()
        sd_has_system = (
            (sd_systems_out / sys_name).exists()
            if sd_systems_out is not None
            else None
        )

        for game in games:
            PAUSE.wait_if_paused()
            if PAUSE.should_stop():
                break

            global_done_games += 1
            if progress_cb is not None:
                progress_cb(
                    "final_media", global_done_games, global_total_games, sys_name
                )

            path_elem = game.find("path")
            if path_elem is None:
                continue
            raw_path = unquote(path_elem.text or "").strip()
            if not raw_path:
                continue

            rom_stem = Path(raw_path).stem
            game_name = sanitize_filename(rom_stem)

            img_elem = game.find(tag)
            if img_elem is None or not (img_elem.text or "").strip():
                continue

            image_raw = unquote(img_elem.text.strip())
            src_image = resolve_image_path(sys_dir, image_raw)

            if not src_image.exists():
                entries.append(
                    {
                        "system": sys_name,
                        "game": rom_stem,
                        "status": "MISSING_SOURCE",
                        "source_path": str(src_image),
                        "staging_path": "",
                        "sd_path": "",
                        "reason": "Image missing from the ROMs folder (gamelist.xml)",
                    }
                )
                counts["missing_source"] += 1
                continue

            expected = _expected_final_paths(
                systems_out, sys_name, game_name, src_image.suffix
            )

            # Dossier du systeme absent des DEUX cotes : rien a comparer.
            if not staging_has_system and not sd_has_system:
                counts["not_checked"] += 1
                continue

            # Dossier du systeme absent cote temporaire seulement : on
            # verifie uniquement contre la SD (pas de motif "absent du
            # dossier temporaire").
            if not staging_has_system and sd_systems_out is not None and sd_has_system:
                sd_expected = [
                    sd_systems_out / p.relative_to(systems_out) for p in expected
                ]
                sd_missing = [p for p in sd_expected if not p.exists()]
                if sd_missing:
                    entries.append(
                        {
                            "system": sys_name,
                            "game": rom_stem,
                            "status": "MISSING_ON_SD",
                            "source_path": str(src_image),
                            "staging_path": "",
                            "sd_path": str(sd_expected[0]),
                            "reason": (
                                "Missing from the SD card (copy not done or "
                                "failed)"
                            ),
                        }
                    )
                    counts["missing_on_sd"] += 1
                else:
                    counts["ok"] += 1
                continue

            # A partir d'ici, le dossier du systeme existe cote temporaire :
            # verification normale du dossier temporaire.
            staging_missing = [p for p in expected if not p.exists()]
            if staging_missing:
                entries.append(
                    {
                        "system": sys_name,
                        "game": rom_stem,
                        "status": "MISSING_CONVERTED",
                        "source_path": str(src_image),
                        "staging_path": str(expected[0]),
                        "sd_path": "",
                        "reason": (
                            "raw565 conversion missing from the staging folder "
                            "(malformed PNG/GIF source, or conversion not "
                            "executed)"
                        ),
                    }
                )
                counts["missing_converted"] += 1
                continue

            # Dossier du systeme absent cote SD seulement : on ne signale
            # pas chaque jeu comme "absent de la SD" (le systeme entier n'a
            # simplement pas encore ete copie) -- verification basee sur le
            # dossier temporaire uniquement, comme si la SD n'etait pas
            # demandee pour ce systeme.
            if sd_systems_out is not None and sd_has_system:
                sd_expected = [
                    sd_systems_out / p.relative_to(systems_out) for p in expected
                ]
                sd_missing = [p for p in sd_expected if not p.exists()]
                if sd_missing:
                    entries.append(
                        {
                            "system": sys_name,
                            "game": rom_stem,
                            "status": "MISSING_ON_SD",
                            "source_path": str(src_image),
                            "staging_path": str(expected[0]),
                            "sd_path": str(sd_expected[0]),
                            "reason": (
                                "Present in the staging folder but missing "
                                "from the SD card (copy not done or failed)"
                            ),
                        }
                    )
                    counts["missing_on_sd"] += 1
                    continue

            counts["ok"] += 1

    return {
        "profile": profile_name,
        "roms_root": roms_root,
        "systems_out": systems_out,
        "sd_root": sd_root,
        "entries": entries,
        "counts": counts,
        "total": sum(counts.values()),
    }


def generate_final_media_report(result: dict, sd_dir: Path) -> tuple[Path, Path]:
    """
    Genere le rapport final Mode 8 :
      - un .csv structure (system, game, status, source_path, staging_path,
        sd_path, reason), directement exploitable pour regenerer les
        images manquantes ;
      - un .txt de resume lisible.
    Retourne (csv_path, txt_path).
    """
    import csv

    report_dir = sd_dir / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime("%Y%m%d_%H%M%S")
    csv_path = report_dir / f"mode8_final_report_{stamp}.csv"
    txt_path = report_dir / f"mode8_final_report_{stamp}.txt"

    fieldnames = [
        "system",
        "game",
        "status",
        "source_path",
        "staging_path",
        "sd_path",
        "reason",
    ]
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for entry in result["entries"]:
            writer.writerow(entry)

    counts = result["counts"]
    lines = [
        "=" * 60,
        "MODE 8 FINAL REPORT - ROMs / staging folder / SD comparison",
        "=" * 60,
        "Date              : " + time.strftime("%Y-%m-%d %H:%M:%S"),
        "Profile           : " + str(result.get("profile", "N/A")),
        "ROMs folder       : " + str(result.get("roms_root", "N/A")),
        "Staging folder    : " + str(result.get("systems_out", "N/A")),
        "Physical SD       : " + str(result.get("sd_root") or "(not checked)"),
        "",
        "Total games checked      : " + str(result["total"]),
        "OK                       : " + str(counts["ok"]),
        "Missing (ROMs)           : " + str(counts["missing_source"]),
        "Missing (conversion)     : " + str(counts["missing_converted"]),
        "Missing (SD copy)        : " + str(counts["missing_on_sd"]),
        "Not checked (system folder missing on both sides): "
        + str(counts.get("not_checked", 0)),
        "",
        "Detailed report (CSV, usable to regenerate missing images):",
        "   " + str(csv_path),
        "",
    ]

    problems = [e for e in result["entries"] if e["status"] != "OK"]
    if problems:
        lines.append("PROBLEM DETAILS:")
        lines.append("-" * 40)
        for e in problems:
            lines.append(f"[{e['system']}] {e['game']} -- {e['status']}")
            lines.append(f"   {e['reason']}")
        lines.append("")
    else:
        lines.append("No problems detected!")
    lines.append("=" * 60)
    txt_path.write_text("\n".join(lines), encoding="utf-8")

    print("Final CSV report generated: " + str(csv_path))
    print("Final TXT report generated: " + str(txt_path))
    return csv_path, txt_path


# ─────────────────────────────────────────────────────────────────────────────
# PLAYLISTS (PC-tool builder) -- onglet PLAYLIST du GUI
#
# Format consomme par le firmware (RecalBox_DMD.ino) : <SD>/playlists/<nom>.txt,
# une ligne = un chemin absolu "/gifs/<dossier>/<fichier>.gif". Le firmware
# reconstruit lui-meme (a chaque boot, via comparaison de hash) les fichiers
# compagnons <nom>.cache/<nom>.sig/<nom>.idx -- l'outil PC n'ecrit donc QUE
# le .txt. Les gifs eux-memes vivent dans <SD>/gifs/<dossier>/, distinct du
# dossier <SD>/systems/ utilise par ailleurs dans ce fichier (cache
# EmulationStation, sans rapport avec les playlists).
#
# RECONSTRUIT le 2026-08-03 : le worktree d'origine (dev-cache-externalisation)
# a ete supprime par erreur (--force sur un git worktree remove, fichiers
# jamais suivis par git) ; reconstruction fidele depuis la memoire projet
# detaillee (project_recalbox_dmd_playlist_tab.md), etat final directement,
# sans rejouer chaque etape intermediaire de l'historique.
# ─────────────────────────────────────────────────────────────────────────────
PLAYLIST_GIFS_DIRNAME = "gifs"
PLAYLIST_DIR_NAME = "playlists"
# Marqueur "dossier complet" en tete de playlist -- meme format que le
# firmware (RecalBox_DMD, branche dev-tous-txt-filter, handleWebConfig
# AddToPlaylistsBatch()) : "# FULL:dossier1,dossier2" (comma-separated,
# pas d'espaces), une ligne de commentaire ordinaire pour le parseur de
# playlist (deja ignoree par read_playlist()/isValidPlaylistLine() cote
# firmware, qui sautent toute ligne commencant par '#'). Liste les
# dossiers inclus INTEGRALEMENT (tout leur contenu au moment de
# l'ecriture) par opposition a une selection personnalisee de fichiers.
# Objectif : quand de nouveaux fichiers sont ajoutes a un dossier, ne
# proposer l'ajout automatique qu'aux playlists qui le referencent en
# entier (evite d'ajouter a tort un fichier a une playlist qui n'avait
# demande qu'une selection personnalisee de ce dossier).
PLAYLIST_FULL_MARKER_PREFIX = "# FULL:"


def list_playlist_gif_folders(sd_root: Path) -> list[tuple[str, int]]:
    """
    Liste les sous-dossiers de <sd_root>/gifs/ avec le nombre de fichiers
    .gif qu'ils contiennent chacun. Retourne une liste de tuples
    (nom_dossier, nb_gifs) triee par nom.
    """
    gifs_dir = sd_root / PLAYLIST_GIFS_DIRNAME
    if not gifs_dir.exists():
        return []
    out = []
    for entry in sorted(gifs_dir.iterdir(), key=lambda p: p.name.lower()):
        if not entry.is_dir():
            continue
        count = sum(1 for f in entry.iterdir() if f.is_file() and f.suffix.lower() == ".gif")
        out.append((entry.name, count))
    return out


def list_gif_files_in_folder(sd_root: Path, folder_name: str) -> list[str]:
    """Liste les fichiers .gif (noms seuls) d'un dossier <sd_root>/gifs/<folder_name>/."""
    folder = sd_root / PLAYLIST_GIFS_DIRNAME / folder_name
    if not folder.exists() or not folder.is_dir():
        return []
    return sorted(
        (f.name for f in folder.iterdir() if f.is_file() and f.suffix.lower() == ".gif"),
        key=str.lower,
    )


def list_existing_playlists(sd_root: Path) -> list[str]:
    """Liste les playlists existantes (<sd_root>/playlists/*.txt), noms sans extension, tries."""
    pl_dir = sd_root / PLAYLIST_DIR_NAME
    if not pl_dir.exists():
        return []
    return sorted(
        (f.stem for f in pl_dir.iterdir() if f.is_file() and f.suffix.lower() == ".txt"),
        key=str.lower,
    )


def _parse_full_folders_marker(raw_text: str) -> list[str]:
    """Extrait la liste de dossiers du marqueur '# FULL:' en tete de fichier
    (1ere ligne uniquement), [] si absent."""
    first_line = raw_text.splitlines()[0] if raw_text else ""
    if not first_line.startswith(PLAYLIST_FULL_MARKER_PREFIX):
        return []
    rest = first_line[len(PLAYLIST_FULL_MARKER_PREFIX):].strip()
    if not rest:
        return []
    return [f for f in rest.split(",") if f]


def read_playlist(sd_root: Path, name: str) -> list[str]:
    """Lit une playlist existante, retourne la liste des chemins /gifs/... (lignes non-commentaires)."""
    path = sd_root / PLAYLIST_DIR_NAME / f"{name}.txt"
    if not path.exists():
        return []
    entries = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        entries.append(line)
    return entries


def read_playlist_full_folders(sd_root: Path, name: str) -> list[str]:
    """Lit le marqueur '# FULL:...' en tete d'une playlist existante, [] si absent/introuvable."""
    path = sd_root / PLAYLIST_DIR_NAME / f"{name}.txt"
    if not path.exists():
        return []
    try:
        raw = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []
    return _parse_full_folders_marker(raw)


def write_playlist(
    sd_root: Path, name: str, entries: list[str], full_folders: Optional[list[str]] = None
) -> Path:
    """
    Ecrit <sd_root>/playlists/<name>.txt : une ligne = un chemin
    "/gifs/<dossier>/<fichier>.gif". Si full_folders est fourni et non
    vide, ecrit le marqueur "# FULL:dossier1,dossier2" en 1ere ligne
    (voir PLAYLIST_FULL_MARKER_PREFIX) -- retrocompatible si vide/None
    (aucun marqueur ecrit, comportement des anciennes playlists).
    """
    pl_dir = sd_root / PLAYLIST_DIR_NAME
    pl_dir.mkdir(parents=True, exist_ok=True)
    out_path = pl_dir / f"{name}.txt"
    lines = []
    if full_folders:
        lines.append(PLAYLIST_FULL_MARKER_PREFIX + ",".join(full_folders))
    lines.extend(entries)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def delete_playlist(sd_root: Path, name: str) -> bool:
    """Supprime une playlist existante. Retourne True si supprimee, False si absente."""
    path = sd_root / PLAYLIST_DIR_NAME / f"{name}.txt"
    if not path.exists():
        return False
    path.unlink()
    return True


def build_playlist_entries_from_folders(sd_root: Path, folder_names: list[str]) -> list[str]:
    """Construit la liste d'entrees /gifs/<dossier>/<fichier>.gif pour un
    ensemble de dossiers pris INTEGRALEMENT (tous leurs .gif)."""
    entries = []
    for folder_name in folder_names:
        for fname in list_gif_files_in_folder(sd_root, folder_name):
            entries.append(f"/{PLAYLIST_GIFS_DIRNAME}/{folder_name}/{fname}")
    return entries


def build_playlist_entries_from_files(files: list[tuple[str, str]]) -> list[str]:
    """Construit la liste d'entrees /gifs/<dossier>/<fichier>.gif a partir
    d'une liste de tuples (folder_name, file_name) -- selection personnalisee."""
    return [f"/{PLAYLIST_GIFS_DIRNAME}/{folder}/{fname}" for folder, fname in files]


def find_gif_folders_recursive(root_path: Path) -> list[Path]:
    """
    Parcourt root_path RECURSIVEMENT (tous niveaux, pas seulement ses
    sous-dossiers immediats) et retourne la liste de tous les dossiers
    (root_path lui-meme inclus s'il est concerne) qui contiennent
    DIRECTEMENT au moins un fichier .gif. Un dossier "vide" de gifs
    n'est jamais retourne pour lui-meme, meme s'il a des sous-dossiers
    qui, eux, en contiennent -- seuls ces sous-dossiers le sont
    (demande utilisateur : "si un dossier contient des sous-dossiers
    avec des gifs, les lister... un dossier vide n'est pas ajoute").

    Utilise par "Ajouter un dossier PC..." (RecalBoxDMD_GUI.py) pour
    proposer en une seule case a cocher, en un seul passage, absolument
    tous les dossiers de GIFs presents sous la racine choisie -- plutot
    que de forcer l'utilisateur a choisir un dossier a la fois (le
    selecteur natif Windows ne permet qu'un seul dossier par appel,
    limitation du shell) ou a naviguer niveau par niveau.

    Trie par chemin relatif (insensible a la casse) pour un affichage
    stable et previsible, dossiers moins profonds d'abord a chemin egal.
    """
    found: list[Path] = []
    for dirpath, _dirnames, filenames in os.walk(root_path):
        if any(f.lower().endswith(".gif") for f in filenames):
            found.append(Path(dirpath))
    found.sort(key=lambda p: str(p.relative_to(root_path)).lower())
    return found


def copy_external_gifs_to_sd(
    sd_root: Path, src_folder: Path, dest_folder_name: str, progress_cb=None,
    only_files: Optional[list[str]] = None,
) -> tuple[int, int, int]:
    """
    Copie les .gif de src_folder (dossier PC externe) vers
    <sd_root>/gifs/<dest_folder_name>/. Gestion de collision par fichier :
    - meme nom, contenu identique (filecmp) -> skip
    - meme nom, contenu different -> renommage auto _2/_3/... (jamais
      d'ecrasement silencieux)
    only_files : si fourni, ne copie QUE les fichiers dont le nom exact
    figure dans cette liste (selection partielle validee par
    l'utilisateur sur un dossier "en attente" -- voir
    _on_playlist_copy_pending_clicked() cote GUI) ; None (defaut) copie
    tout le dossier, comportement inchange.
    Retourne (copies, ignores_identiques, renommes).
    """
    dst_folder = sd_root / PLAYLIST_GIFS_DIRNAME / dest_folder_name
    dst_folder.mkdir(parents=True, exist_ok=True)

    only_set = set(only_files) if only_files is not None else None
    src_files = sorted(
        (
            f for f in src_folder.iterdir()
            if f.is_file() and f.suffix.lower() == ".gif"
            and (only_set is None or f.name in only_set)
        ),
        key=lambda p: p.name.lower(),
    )
    total = len(src_files)
    copied = 0
    skipped_identical = 0
    renamed = 0

    for idx, src_file in enumerate(src_files, 1):
        dst_file = dst_folder / src_file.name
        if dst_file.exists():
            try:
                if filecmp.cmp(src_file, dst_file, shallow=False):
                    skipped_identical += 1
                    if progress_cb:
                        progress_cb("playlist_copy", idx, total, src_file.name)
                    continue
            except OSError:
                pass
            # Contenu different : renommage auto _2, _3, ...
            stem, suffix = src_file.stem, src_file.suffix
            n = 2
            while dst_file.exists():
                dst_file = dst_folder / f"{stem}_{n}{suffix}"
                n += 1
            renamed += 1

        shutil.copy2(src_file, dst_file)
        copied += 1
        if progress_cb:
            progress_cb("playlist_copy", idx, total, src_file.name)

    return copied, skipped_identical, renamed


def regenerate_playlist_gifs_cache(sd_root: Path) -> Path:
    """
    Scanne TOUT <sd_root>/gifs/ (tous les dossiers, sans selection) et
    ecrit <sd_root>/playlists/cache_master_gifs.dat -- meme format qu'une
    playlist (une ligne = /gifs/<dossier>/<fichier>.gif), extension .dat
    volontairement HORS de la convention .txt pour que list_existing_playlists()
    (et le firmware, qui ne scanne que les *.txt) ne le proposent jamais
    comme playlist selectionnable : ce n'est PAS une playlist, c'est un
    index de reference.

    ATTENTION -- ce fichier N'EST PAS inerte pour le firmware (verifie
    dans web_config.h, plan "cache_master_gifs", TOUS_MASTER_PATH =
    "/playlists/cache_master_gifs.dat", handleWebConfigGeneratePlaylist()/
    filterMasterIntoFile()/appendMatchingLines()) : le DMD lui-meme lit ce
    fichier pour accelerer SA PROPRE generation de playlist (chemin
    "hybride" -- filtrage texte quasi instantane pour tout dossier deja
    present ici, au lieu d'un scan SD complet), et considere un dossier
    "deja en cache" des qu'AU MOINS une ligne "/gifs/<dossier>/" y figure
    (simple recherche de sous-chaine, `fileContainsNeedle()`, independante
    du marqueur "# FULL:" -- celui-ci n'est qu'un artefact du format
    playlist standard, ignore sans erreur par le parseur firmware qui ne
    traite que les lignes commencant par "/gifs/"). Toute mutation du
    contenu de gifs/ faite par cet outil (import, suppression) doit donc
    rester cohérente avec ce fichier pour ne pas faire generer au DMD une
    playlist incomplete a partir d'un cache perime -- voir
    append_to_master_gifs_cache(), appelee automatiquement apres un import
    (_playlist_import_worker, RecalBoxDMD_GUI.py).
    """
    folders = [name for name, _count in list_playlist_gif_folders(sd_root)]
    entries = build_playlist_entries_from_folders(sd_root, folders)
    pl_dir = sd_root / PLAYLIST_DIR_NAME
    pl_dir.mkdir(parents=True, exist_ok=True)
    out_path = pl_dir / "cache_master_gifs.dat"
    lines = [PLAYLIST_FULL_MARKER_PREFIX + ",".join(folders)] if folders else []
    lines.extend(entries)
    out_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    return out_path


def append_to_master_gifs_cache(sd_root: Path, folder_name: str, filenames: list[str]) -> None:
    """
    Ajoute INCONDITIONNELLEMENT les fichiers nouvellement ajoutes d'un
    dossier a cache_master_gifs.dat -- contrepartie cote PC de la cible
    d'ajout inconditionnelle du firmware lors d'un upload web
    (handleWebConfigAddToPlaylistsBatch(), web_config.h, commentaire :
    "cache_master_gifs.dat est desormais une cible d'ajout INCONDITIONNELLE
    lors d'un upload"). Sans cette synchronisation, importer des GIFs via
    "Ajouter un dossier PC..." dans un dossier DEJA present au moins une
    fois dans le cache laisserait le DMD croire ce dossier entierement a
    jour (voir regenerate_playlist_gifs_cache() pour le detail du
    mecanisme de detection firmware) et generer une playlist incomplete
    (chemin "hybride" rapide) tant que l'utilisateur ne clique pas
    manuellement sur "Regenerer le cache playlist". Cree le fichier au
    besoin (bootstrap organique, meme comportement que le firmware) ;
    n'ecrit jamais/ne modifie jamais le marqueur "# FULL:" existant (le
    firmware ne le fait pas non plus dans ce fichier). Deduplique par
    ligne exacte (idempotent si rappelee sur les memes fichiers).
    """
    if not filenames:
        return
    cache_path = sd_root / PLAYLIST_DIR_NAME / "cache_master_gifs.dat"
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    existing: set[str] = set()
    if cache_path.exists():
        try:
            existing = {
                line.strip()
                for line in cache_path.read_text(encoding="utf-8", errors="replace").splitlines()
                if line.strip().startswith(f"/{PLAYLIST_GIFS_DIRNAME}/")
            }
        except OSError:
            existing = set()
    new_lines = [
        entry
        for fname in filenames
        if (entry := f"/{PLAYLIST_GIFS_DIRNAME}/{folder_name}/{fname}") not in existing
    ]
    if not new_lines:
        return
    with open(cache_path, "a", encoding="utf-8") as f:
        for line in new_lines:
            f.write(line + "\n")


def update_playlists_referencing_folder(sd_root: Path, folder_name: str, filenames: list[str]) -> int:
    """
    Ajoute automatiquement les nouveaux fichiers (filenames, noms seuls
    avec extension .gif) d'un dossier a TOUTES les playlists existantes
    qui referencent ce dossier "en entier" (marqueur '# FULL:' contenant
    folder_name). Playlists SANS marqueur (anciennes, avant l'ajout du
    marqueur) : repli retrocompatible -- mise a jour si AU MOINS une
    entree existante reference deja ce dossier (comportement legacy,
    incapable de distinguer complet/personnalise). Playlists AVEC
    marqueur mais qui ne listent PAS ce dossier : jamais mises a jour
    (selection personnalisee de ce dossier, intention explicite de
    l'utilisateur). cache_master_gifs.dat (.dat, pas .txt) est exclu par
    construction de list_existing_playlists(). Retourne le nombre de
    playlists mises a jour.
    """
    if not filenames:
        return 0
    updated = 0
    new_entries = [f"/{PLAYLIST_GIFS_DIRNAME}/{folder_name}/{fname}" for fname in filenames]
    for pl_name in list_existing_playlists(sd_root):
        path = sd_root / PLAYLIST_DIR_NAME / f"{pl_name}.txt"
        try:
            raw = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        full_folders = _parse_full_folders_marker(raw)
        entries = read_playlist(sd_root, pl_name)
        prefix = f"/{PLAYLIST_GIFS_DIRNAME}/{folder_name}/"

        if full_folders:
            if folder_name not in full_folders:
                continue  # selection personnalisee de ce dossier, ne pas toucher
        else:
            if not any(e.startswith(prefix) for e in entries):
                continue  # playlist ancienne, ne referencait pas ce dossier

        existing_set = set(entries)
        to_add = [e for e in new_entries if e not in existing_set]
        if not to_add:
            continue
        entries.extend(to_add)
        write_playlist(sd_root, pl_name, entries, full_folders=full_folders or None)
        updated += 1
    return updated


def remove_playlist_entries(sd_root: Path, folder_name: str, filenames: Optional[list[str]] = None) -> int:
    """
    Retire d'une playlist toute entree correspondant a un dossier
    supprime (filenames=None -> tout le dossier) ou a des fichiers
    specifiques supprimes (filenames fourni). Symetrique de
    update_playlists_referencing_folder(). Retourne le nombre de
    playlists modifiees.
    """
    updated = 0
    prefix = f"/{PLAYLIST_GIFS_DIRNAME}/{folder_name}/"
    if filenames is not None:
        to_remove = {f"{prefix}{fname}" for fname in filenames}
    else:
        to_remove = None  # tout le dossier

    for pl_name in list_existing_playlists(sd_root):
        entries = read_playlist(sd_root, pl_name)
        full_folders = read_playlist_full_folders(sd_root, pl_name)
        if to_remove is None:
            new_entries = [e for e in entries if not e.startswith(prefix)]
            new_full_folders = [f for f in full_folders if f != folder_name]
        else:
            new_entries = [e for e in entries if e not in to_remove]
            new_full_folders = full_folders
        # Les 2 aspects (entrees / marqueur FULL) sont verifies
        # INDEPENDAMMENT -- un dossier peut disparaitre du marqueur FULL
        # meme si, pour une raison quelconque (etat incoherent), aucune
        # entree de ligne ne le referencait plus au moment du controle ;
        # ne pas coupler les 2 conditions en une seule (bug trouve par
        # test lors de la reconstruction du 2026-08-03).
        if new_entries == entries and new_full_folders == full_folders:
            continue
        write_playlist(sd_root, pl_name, new_entries, full_folders=new_full_folders or None)
        updated += 1
    return updated


def delete_gif_file(sd_root: Path, folder_name: str, filename: str) -> bool:
    """Supprime physiquement un fichier .gif puis nettoie automatiquement
    toutes les playlists qui le referencaient."""
    target = sd_root / PLAYLIST_GIFS_DIRNAME / folder_name / filename
    if not target.exists():
        return False
    target.unlink()
    remove_playlist_entries(sd_root, folder_name, [filename])
    return True


def delete_gif_folder(sd_root: Path, folder_name: str) -> bool:
    """Supprime physiquement un dossier de GIFs entier puis nettoie
    automatiquement toutes les playlists qui le referencaient."""
    target = sd_root / PLAYLIST_GIFS_DIRNAME / folder_name
    if not target.exists():
        return False
    shutil.rmtree(target)
    remove_playlist_entries(sd_root, folder_name, None)
    return True


if __name__ == "__main__":
    import RecalBoxDMD_GUI as gui
    from pathlib import Path

    script_dir = Path(__file__).parent
    sd_dir = get_sd_card_dir(script_dir)

    toolkit_module = sys.modules[__name__]
    app = gui.RetroBoxLEDGui(toolkit_module, sd_dir)
    app.run()
