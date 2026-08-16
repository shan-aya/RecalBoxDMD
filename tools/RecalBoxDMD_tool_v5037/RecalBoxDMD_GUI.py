#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v50
#
# v50 — 2026-08-15 — safe-modify — Nouveau dialogue _prompt_systems_image_lang_dialog()
#      (choix EN/FR/ES des images systemes/genres _defaults, avec miniature
#      comparative bundlee tools/assets/lang_preview/compare_systems_lang.png)
#      pose en Mode 1 (_on_start_clicked, avec les autres pre-vol) ET en
#      Mode 2 onglet Avance (telechargement _defaults seul). Choix persiste
#      (prefs "systems_image_lang", pre-selectionne au dialogue suivant) et
#      transmis a toolkit.download_defaults(..., lang=...) dans
#      _pipeline_mode_1()/_pipeline_mode_2(). Voir RecalBoxDMD_tool.py v36 et
#      RecalBoxDMD_prefs.py v8.
#
# v49 — 2026-08-13 — safe-modify — Texte long du panneau Mode 9 (FR/EN/ES)
#      mis a jour pour citer les 6 scripts installes (ajout de Reboot DMD,
#      deja omis avant ce fix, et des 2 nouveaux scripts Luminosite DMD
#      +10%/-10%, cf. firmware v77 / RecalBoxDMD_tool.py v35).
#
# v48 — 2026-08-11 — safe-modify — Le fix v47 (after_idle) etait
#      INSUFFISANT -- signale par l'utilisateur : le bug persistait. Cause
#      complementaire identifiee : `self.root.after(200, lambda ...:
#      themes.apply(...))` (deja present, ligne ~2426, re-application du
#      theme 200ms apres le demarrage) declenche desormais aussi
#      _update_mode_desc() via l'ajout RecalBoxDMD_themes.py v6 -- 2 appels
#      concurrents (le mien via after_idle, celui-la via after(200)) sans
#      garantie que les DEUX mesurent contre une largeur de widget fiable.
#      Fix plus robuste, independant du timing exact : _insert_autolink_text()
#      verifie desormais text_widget.winfo_width() APRES update() -- si la
#      largeur rendue est suspecte (<50px, widget pas encore reellement
#      dispose), la hauteur n'est PAS modifiee cette fois (ancienne valeur
#      conservee) plutot que de calculer un nombre de lignes aberrant contre
#      une largeur quasi nulle. Un appel ulterieur (changement de mode/
#      theme/onglet, tres frequent dans ce fichier) la corrigera avec une
#      largeur fiable.
#
# v47 — 2026-08-11 — safe-modify — Bug reel signale par l'utilisateur sur
#      v45/v46 (conversion mode_desc_label en Text) : au tout premier
#      affichage de l'onglet Main, le cadre Progression partage (sous le
#      Notebook) disparaissait, et le cadre "systemes a traiter" semblait
#      agrandi -- un aller-retour Main -> Avance -> Main corrigeait la
#      taille. Cause : l'appel initial a _update_mode_desc() (fin de
#      _build_mode_area_advanced) s'execute PENDANT __init__(), avant que
#      mainloop() ne demarre -- la fenetre n'est donc pas encore mappee, et
#      self.root.update() dans _insert_autolink_text() ne peut pas obtenir
#      une largeur en pixels fiable pour le widget Text, qui se voit donc
#      assigner une hauteur bien trop grande. Un changement d'onglet
#      redeclenche _update_mode_desc() une fois la fenetre reellement
#      mappee, d'ou la correction observee. Fix : appel initial differe via
#      self.root.after_idle(self._update_mode_desc) (se declenche au tout
#      debut de mainloop(), fenetre deja mappee) au lieu d'un appel
#      synchrone pendant la construction.
#
# v46 — 2026-08-11 — safe-modify — Retours utilisateur sur le v45 :
#      (1) Mode 11 (pack GIFs) unifie sur le meme fonctionnement que Mode 2
#      -- passe desormais par btn_start_adv/_worker_main
#      (_pipeline_mode_11(), reutilise toolkit.download_gif_pack()) au lieu
#      d'un panneau/bouton/thread dedies, retires entierement
#      (_mode11_frame et handlers associes). Detail Mode 11 adapte ("cliquez
#      sur Demarrer" au lieu de "Telecharger le pack"). (2) Popup
#      "ecraser les fichiers _defaults existants ?" (Mode 2) :
#      messagebox.askyesno() (rendu Windows natif fixe, ne suit pas le
#      theme) remplace par self._themed_yesno(). (3) Boutons
#      Demarrer/Quitter (Main + Avance) en MAJUSCULES (start_btn ici,
#      main_opt_quit dans RecalBoxDMD_tool.py v34, 3 langues chacun).
#      (4) Detail Mode 9 raccourci (derniere phrase "Ce mode fait aussi
#      partie du Mode 1..." retiree, 3 langues) : le panneau Mode 9 +
#      description debordaient sur le cadre Progression partage en bas de
#      fenetre (fenetre a taille fixe, voir _build_mode_area_advanced) --
#      libere de la hauteur verticale.
#
# v45 — 2026-08-11 — safe-modify — Trois ajouts demandes par l'utilisateur :
#      (1) Onglet Parametres : nouveau champ numerique "seuil flag L"
#      (_slow_threshold_var, ttk.Spinbox), permet d'affiner selon la
#      vitesse reelle de la carte SD de l'utilisateur le seuil utilise par
#      build_systems_cache() (RecalBoxDMD_tool.py v33) pour decider quels
#      systemes recoivent le flag "L" (lent). Persiste dans
#      RecalBoxDMD_prefs.json (v7). (2) Nouveau Mode 11 (sous-menu separe
#      de la categorie DOWNLOAD FROM GITHUB, a cote de Mode 2) : telecharge
#      le pack gratuit de ~600 GIFs GitHub, action autonome (bouton +
#      thread propre, meme principe que Mode 9/10) reutilisant integralement
#      toolkit.download_gif_pack() -- Mode 2 reste inchange (uniquement
#      _defaults). (3) Liens cliquables : mode_desc_label/_adv (Main +
#      Avance) convertis de Label+StringVar a Text avec auto-lien
#      (_insert_autolink_text(), regex _AUTOLINK_URL_RE) pour rendre
#      cliquable la mention du pack "ultimate" externe (~11000 animations,
#      rpiteam.carrd.co + forum Arcadia) dans le detail du Mode 11 et la
#      popup pack GIFs du Mode 1 (_themed_yesno(..., linked=True), nouveau
#      helper _make_linked_text()). RecalBoxDMD_themes.py v6 complementaire
#      (widgets Text _theme_as_panel + refresh au changement de theme).
#
# v44 — 2026-08-11 — safe-modify — Bug reel signale par l'utilisateur sur
#      v43 (accordeon) : tous les sous-menus (corps de categorie) s'ouvraient
#      sous le bouton Quitter au lieu de sous leur propre en-tete. Cause :
#      Tkinter place un widget re-pack()e (pack_forget() puis pack() a
#      nouveau, ou pack() appele pour la 1ere fois APRES d'autres widgets
#      deja empaquetes) en DERNIERE position parmi les enfants de son
#      parent -- pas a sa position "logique" dans le code. Comme les
#      "body" de categorie non ouverte au demarrage n'etaient empaquetes
#      qu'au clic (donc apres coup, apres que spacer/path_box/Demarrer/
#      Quitter aient deja ete empaquetes dans "left"), ils atterrissaient
#      tout en bas. Fix : chaque categorie recoit desormais son propre
#      petit conteneur (category_frame) empaquete UNE SEULE FOIS a la
#      construction, dans l'ordre -- le dépli/repli du corps ne touche
#      plus que l'ordre interne a ce conteneur (en-tete + corps, 2
#      enfants), jamais l'ordre parmi les enfants de "left". Pas encore
#      teste (nouveau lancement necessaire).
#
# v43 — 2026-08-11 — safe-modify — Onglet Avance : refonte de la colonne
#      des modes (8 radios plats) en accordeon a 5 categories thematiques
#      cliquables (une seule depliee a la fois) -- demande utilisateur pour
#      gagner de la place dans la colonne fixe (363px) et pouvoir ajouter
#      de futurs modes sans que la liste devienne ingerable. Un menu a
#      survol (fly-out) a ete explicitement ecarte : chaque mode expose
#      des parametres (dossier, panneaux dedies) qui doivent rester des
#      widgets persistants dans right_adv. Taxonomie validee avec
#      l'utilisateur : DOWNLOAD FROM GITHUB (Mode 2), GAMELIST.XML
#      (Mode 3+8), IMAGES TOOLS (Mode 4+5+10), CACHES (Mode 6+7),
#      SCRIPTS RECALBOX (Mode 9). Nouveau Mode 10 : promotion du panneau
#      "Choisir son image de secours" (auparavant simple bouton sous
#      Mode 2) en mode selectionnable a part entiere, meme principe
#      autonome que Mode 9 (pas de pipeline, action directe via son
#      propre bouton). Nouvelles methodes _accordion_category_for_mode/
#      _accordion_header_text/_on_accordion_toggle ; reutilise le
#      mecanisme pack()/pack_forget() + _reslice_after_mode_change() deja
#      en place pour tous les panneaux de detail par mode. Pas encore
#      teste (lancement reel de l'app).
#
# v42 — 2026-08-07 — safe-modify — Fix bug signale par l'utilisateur : le
#      cadre Progression (partage, sous le Notebook) semblait "disparu" en
#      Mode 3/8 de l'onglet Avance. Cause reelle identifiee par
#      l'utilisateur : _on_pipeline_finished() appelle
#      _start_mode6_blinking() (revele le cadre "copie SD",
#      _mode6_ui_frame_adv) sans condition de mode -- sauf Mode 8, deja
#      protege par un "return" specifique. Une fois revele par un Mode
#      1/6/7 anterieur dans la MEME session, rien ne le masquait en
#      changeant de mode ensuite : il restait visible et repoussait le
#      panneau specifique de Mode 3/8 (droite/bas de "outer") hors de la
#      zone allouee a l'onglet, jusqu'a chevaucher le cadre Progression en
#      dessous (fenetre fixe 1100x750, non redimensionnable -- rien ne
#      contient ce debordement). Fix : _on_mode_changed() masque
#      explicitement _mode6_ui_frame_adv en entrant en Mode 3 ou Mode 8 --
#      ces 2 modes (extraction seule / verification) ne produisent de
#      toute facon rien de pret a copier sur la carte SD.
#
# v41 — 2026-08-06 — safe-modify — Fix bug signale par l'utilisateur :
#      _pipeline_mode_2 (Mode 2) videait tout le contenu de sd_dir/systems/
#      (sauf _defaults) avant de relancer l'extraction, effacant au passage
#      les dossiers systemes deja presents dans le dossier temporaire (ex:
#      conversions GIF/raw565pack deja faites a la main). Retire ce
#      nettoyage prealable -- l'extraction ajoute/ecrase uniquement les
#      fichiers qu'elle produit elle-meme, le reste du contenu existant de
#      systems/ n'est plus jamais efface par le Mode 2.
#
# v40 — 2026-07-23 — safe-modify — Mode 1 (_pipeline_mode_1) : utilise
#      desormais toolkit.install_recalbox_scripts() au lieu de
#      install_staged_scripts_to_share() -- meme comportement si le
#      partage SMB est joignable, mais repli SSH/SFTP automatique
#      (identifiants Recalbox par defaut) si injoignable, voir
#      RecalBoxDMD_tool.py v24. La methode reellement utilisee (smb/ssh)
#      est loguee (mode1_scripts_installed_via).
#
# v39 — 2026-07-22 — safe-modify — Demande utilisateur : l'IP Recalbox
#      validee en Mode 1 (confirmee joignable par l'utilisateur, PAS juste
#      detectee) est desormais ecrite dans config.ini (toolkit.
#      write_dmd_recalbox_ip(), v23) des que l'installation des scripts
#      reussit reellement -- pour que le champ "IP Recalbox" de la page web
#      config soit deja pre-rempli au premier boot du DMD, meme si la
#      Recalbox est eteinte a ce moment-la (l'auto-detection mDNS du
#      firmware necessite qu'elle soit joignable). Nouveau
#      self._mode1_scripts_target_ip (IP numerique resolue, distincte de
#      self._mode1_scripts_target qui peut etre "RECALBOX" -- nom NetBIOS
#      resolvable par Windows mais pas forcement par l'ESP32) alimente dans
#      les deux branches de confirmation (auto-detection confirmee, saisie
#      manuelle reussie) de _on_start_clicked(), consomme dans
#      _pipeline_mode_1() seulement si l'install reseau a reellement
#      abouti (pas juste tentee).
#
# v38 — 2026-07-22 — safe-modify — 2 bugs trouves suite test reel du v37 :
#      (1) Popup de rappel en fin de process (_on_pipeline_finished)
#      reutilisait mode1_rb_unreachable_msg tel quel -- sa question
#      "voulez-vous ressaisir l'IP manuellement ?" n'a aucun sens a ce
#      stade (aucune saisie interactive en cours). Fix : nouveau message
#      DEDIE mode1_rb_reminder_title/msg (fr/en/es) -- garde uniquement
#      l'orientation Mode 9 + le rappel de consequence (playlist/horloge
#      uniquement) + le chemin du dossier local, sans la question de
#      ressaisie.
#      (2) Le descriptif Mode 1 enrichi d'une section "Marche a suivre"
#      (v36) decalait toute l'interface vers le bas une fois le panneau
#      "copie SD" revele -- les boutons du cadre progress disparaissaient
#      de la vue (confirme le risque de debordement deja signale, jamais
#      verifiable visuellement de mon cote). Fix : nouveau flag
#      self._mode1_sd_copy_active, active dans _start_mode6_blinking()
#      (panneau copie SD revele) et remis a False en debut de
#      _on_start_clicked() (nouveau run) -- _update_mode_desc() tronque le
#      descriptif Mode 1 a la section "Marche a suivre" (marqueurs
#      _STEPS_SECTION_MARKERS) uniquement quand ce flag est actif, libérant
#      la place necessaire.
#
# v37 — 2026-07-22 — safe-modify — Refonte complete du flux d'installation
#      des scripts Recalbox en Mode 1 (demande explicite utilisateur,
#      process precedent juge insuffisant) :
#      1. detection auto -> popup proposant l'IP detectee -> "oui" = on
#         garde cette cible.
#      2. "non" (ou rien detecte) -> nouvelle popup de saisie manuelle
#         (_prompt_recalbox_ip_dialog, Entry themee, remplace tout
#         simpledialog natif) + test de joignabilite reelle -> "ok" = on
#         garde cette cible.
#      3. echec (auto ou manuel) -> popup "Recalbox injoignable"
#         (_themed_choice(), nouveau : boutons personnalises au lieu du
#         Oui/Non generique) avec 2 choix : "Ressaisir l'IP" (reboucle sur
#         l'etape 2) ou "Mode 9 plus tard" (abandon -- JAMAIS de saisie
#         forcee ni d'arret du pipeline, comportement explicitement
#         demande).
#      4. Les scripts sont desormais TOUJOURS mis en scene localement
#         (toolkit.stage_recalbox_scripts_locally(), dossier
#         sd_dir/recalbox_userscripts/ avec sous-dossier manual/ -- meme
#         arborescence que le partage reseau), que la Recalbox ait ete
#         confirmee ou non -- pour que l'utilisateur puisse copier les
#         scripts a la main si l'install reseau echoue. L'install reseau
#         elle-meme (si une cible a ete confirmee) copie desormais DEPUIS
#         cette mise en scene locale (toolkit.install_staged_scripts_to_share())
#         au lieu de re-telecharger depuis GitHub.
#      5. Nouvelle popup de rappel en fin de Mode 1 (_on_pipeline_finished) :
#         si les scripts n'ont pas ete installes automatiquement (cible
#         jamais confirmee, injoignable, ou echec reseau inattendu),
#         reaffiche le meme texte que le popup "injoignable" du pre-vol
#         (mode1_rb_unreachable_msg), avec le chemin du dossier local et
#         l'IP visee -- pour que l'information ne soit pas manquee si
#         l'utilisateur etait parti pendant le traitement.
#      Nouvelles cles UI_TRANSLATIONS (fr/en/es, parite 137/137/137
#      verifiee) : mode1_manual_ip_title/prompt/ok/cancel,
#      mode1_rb_unreachable_title/msg, mode1_rb_retry_ip_btn,
#      mode1_rb_use_mode9_btn. Cles mode1_rb_not_found_title/msg (round
#      precedent) supprimees, devenues inutilisees par ce nouveau flux.
#      Bug trouve juste apres (meme session) : le bloc RB+image de secours
#      ci-dessus s'affichait AVANT la verification "aucun systeme
#      selectionne" (sys_sel_warn_empty) -- ces prompts pouvaient donc
#      apparaitre pour rien si l'utilisateur devait ensuite corriger sa
#      selection de systemes. Fix : bloc "if mode == '1':" deplace apres
#      la validation complete dossier ROMs + selection systemes (juste
#      avant la construction de GuiConfig), au lieu de juste apres la
#      validation du dossier ROMs seul.
#
# v36 — 2026-07-22 — safe-modify — Suite retours test reel du v35 :
#      (1) _themed_info()/_themed_yesno() (et la galerie existante
#      "Image de secours", _on_default_image_picker_clicked()) utilisaient
#      encore des couleurs figees (#F3F3F3 quasi-blanc, "black") au lieu du
#      theme actif -- restait blanc/clair meme avec un theme sombre actif.
#      Fix : nouveau helper _theme_colors() (lit themes.get_theme(nom)
#      ["colors"] via self._current_theme_name) branche sur les 3 dialogues
#      (bg/fg/bg_action/bg_normal). (2) Bug confirme : le pre-vol Mode 1
#      (target = detect_recalbox_share() or prefs.get("recalbox_ip")) ne
#      testait jamais reellement la joignabilite -- une IP en cache dans les
#      prefs (quasi toujours presente apres un premier usage du Mode 9) le
#      rendait "trouve" meme si la Recalbox etait actuellement eteinte,
#      donc ni prompt "RB non detectee" ni test de connexion visible pour
#      l'utilisateur. Fix : nouvelle toolkit.is_recalbox_reachable(host)
#      (connexion TCP port 445, timeout court) appelee en plus du simple
#      test de presence avant d'afficher/sauter le prompt. (3) Coherence
#      des libelles de mode : mode1_title (fr/en/es) harmonise ("MODE 1
#      (AUTO) — ...", contenu complet identique dans les 3 langues -- l'EN
#      etait plus court, il manquait "depuis Recalbox"/"copie vers la SD").
#      mode9_short_title (fr/en/es, onglet Avance) prefixe par "MODE 9 —"/
#      "MODO 9 —", comme les modes 2 a 8 (manquait uniquement pour le 9).
#      (4) Descriptif Mode 1 (_detail_templates, fr/en/es) enrichi d'une
#      section "Marche a suivre" (comme les autres modes) et d'une mention
#      des scripts Recalbox/langue desormais transmis en debut de pipeline
#      -- garde volontairement compact (longueur comparable au Mode 3) pour
#      eviter de deborder sur le panneau "copie SD" ancre en bas de la meme
#      colonne. (5) Bug confirme : dans _on_start_clicked(), le pre-vol
#      Mode 1 (RB/image de secours) s'executait AVANT _get_roms_root_or_warn()
#      -- si aucun dossier ROMs n'etait choisi, le prompt "image de secours"
#      s'affichait avant l'alerte "dossier ROMs manquant". Fix : bloc pre-vol
#      Mode 1 deplace APRES la verification/alerte dossier ROMs (l'ordre
#      d'affichage suit maintenant : alerte dossier ROMs -> pre-vol RB/image
#      de secours -> pipeline). (6) Bug confirme : meme apres le fix
#      is_recalbox_reachable(), _pipeline_mode_1() re-detectait sa PROPRE
#      cible (detect_recalbox_share() or prefs.get("recalbox_ip")) au lieu
#      de reutiliser celle deja testee dans _on_start_clicked() -- deux
#      resolutions separees pouvaient tomber sur des cibles differentes, et
#      l'install pouvait etre tentee/echouer silencieusement (log console
#      uniquement) meme quand le pre-vol avait determine que ce n'etait pas
#      joignable. Plus grave avec plusieurs Recalbox allumees sur le meme
#      reseau : la cible auto-detectee/en cache n'est pas forcement celle
#      visee par l'utilisateur. Fix : nouveau prompt de confirmation
#      _themed_yesno (mode1_rb_confirm_title/msg, fr/en/es) affichant la
#      cible detectee et demandant validation explicite AVANT tout install ;
#      la cible validee (ou None si refusee/injoignable) est stockee dans
#      self._mode1_scripts_target et reutilisee telle quelle par
#      _pipeline_mode_1() (plus de re-detection, plus de tentative
#      silencieuse sur une cible non confirmee). (7) Descriptif Mode 1
#      (_detail_templates, fr/en/es) : etape "Detection des systemes"
#      retiree de "Marche a suivre" -- devenue automatique depuis un moment
#      (_maybe_autodetect_systems, declenchee des que le dossier ROMs est
#      choisi), la lister comme etape manuelle induisait en erreur.
#      (8) Popup "Comment scraper ?" (_on_mode1_scrape_help_clicked) : meme
#      souci de couleurs figees (#F3F3F3/black/#00D084) que les dialogues
#      deja corriges -- branche sur _theme_colors(). (9) Suite a la
#      decouverte du (8), les 7 autres popups Toplevel restants avec le
#      meme defaut ont ete corriges (branches sur _theme_colors()) :
#      _show_mode_done_popup, _cleanup_sd_dir (popup progression), 2x
#      _on_mode6_flash_error/_on_mode6_flash_done (mode6_retry_title,
#      mode6_done, doublons Main/Avance), _on_quit_app_clicked (3 popups :
#      confirmation initiale, traitement en cours, resume final). Les 11
#      Toplevel du fichier utilisent desormais tous _theme_colors().
#      (10) Captures "Comment scraper ?" toujours en francais quelle que
#      soit la langue de l'interface -- aucune capture EN/ES equivalente
#      du menu SCRAPEUR trouvee en ligne (wiki Recalbox en rendu JS non
#      exploitable, forums sans ce screenshot precis). Fix : 6 nouvelles
#      images generees localement (tools/assets/scrape_help/*_en.png,
#      *_es.png) via un script PIL ponctuel -- bandeau traduit colle
#      au-dessus de la capture FR d'origine (inchangee), pointant vers les
#      memes encadres rouge/pointilles. _mode1_scrape_help_image_path()
#      choisit la variante *_en/*_es si l'interface est dans cette langue
#      et que le fichier existe, sinon repli sur la capture FR de base.
#      (11) Popup "Recalbox detectee" (pre-vol Mode 1) : affichait le nom
#      NetBIOS "RECALBOX" (identique quelle que soit la Recalbox physique
#      allumee sur le reseau -- ne permettait pas de distinguer laquelle
#      est visee quand plusieurs sont allumees). Fix : affiche desormais
#      l'IP resolue (toolkit.resolve_recalbox_ip()). Reponse "Non" : au
#      lieu de ne rien faire (silencieux), affiche maintenant un message
#      dedie (mode1_rb_declined_title/msg, fr/en/es) orientant vers le
#      Mode 9 -- sans jamais redemander une IP manuelle ni interrompre le
#      pipeline Mode 1 (comportement explicitement demande).
#
# v35 — 2026-07-21 — safe-modify — Suite retours test reel du v34 :
#      (1) Bug trouve : _on_language_changed() n'est declenchee QUE par le
#      menu deroulant de langue (command=), jamais appelee au demarrage --
#      self.lang_var affichait deja la bonne langue sauvegardee mais tous
#      les libelles qu'elle rafraichit (nom "Mode 1", bouton "Quitter", etc.)
#      restaient en francais (valeur de construction initiale) tant que
#      l'utilisateur n'avait pas re-touche le selecteur. Fix : appel
#      explicite self._on_language_changed() ajoute en toute fin de
#      __init__(), une fois tous les widgets construits.
#      (2) Le pre-vol Mode 1 (v34, messagebox.showinfo/askyesno) n'etait ni
#      dans la langue de l'interface (boutons Oui/Non d'un messagebox natif
#      rendus par Windows dans la langue systeme, pas controlable depuis
#      Python) ni dans le style visuel de l'appli (fond gris natif). Fix :
#      nouveaux _themed_info()/_themed_yesno() (Toplevel custom, meme
#      convention que la galerie "Image de secours" existante), utilises a
#      la place de messagebox pour ces 2 prompts.
#
# v34 — 2026-07-21 — safe-modify — Retours utilisateur post-test reel Mode 1 :
#      (1) bug confirme -- write_dmd_language() n'etait branchee que dans
#      mode_full() (CLI), jamais dans _pipeline_mode_1() (le pipeline
#      REELLEMENT utilise par le GUI, qui ne passe pas par mode_full()) :
#      aucun config.ini n'apparaissait dans le dossier temporaire. Corrige :
#      appel ajoute en tete de _pipeline_mode_1(). (2) Installation des
#      scripts Recalbox deplacee en tout premier dans _pipeline_mode_1()
#      (avant extraction/conversion/etc., juste apres write_dmd_language) --
#      le script marquee est indispensable au bon fonctionnement de
#      l'appareil, pas une simple option skip-able silencieusement. (3)
#      Nouveau pre-vol synchrone dans _on_start_clicked() (thread principal,
#      avant le lancement du worker Mode 1) : si aucune cible Recalbox n'est
#      detectee, messagebox explicite (au lieu du skip silencieux precedent)
#      orientant vers le Mode 9 (onglet Avance) ; et si aucune image de
#      secours personnalisee n'est encore enregistree, proposition explicite
#      d'en choisir une maintenant (sinon visuel par defaut du projet comme
#      avant). Reutilise _on_default_image_picker_clicked() existant, rendu
#      bloquant (self.root.wait_window(dlg) ajoute) pour cet usage synchrone
#      -- sans effet sur son usage normal en bouton. 4 nouvelles cles
#      UI_TRANSLATIONS (fr/en/es, parite verifiee 127/127/127).
#
# v33 — 2026-07-21 — safe-modify — DMD multilingue (demande utilisateur) :
#      _set_toolkit_language() propage desormais aussi tkmod.CURRENT_LANG
#      (pas seulement tkmod.T) -- utilise par RecalBoxDMD_tool.py::
#      write_dmd_language() pour transmettre la meme langue que le GUI au
#      DMD via config.ini lors du Mode 1.
#
# v32 — 2026-07-21 — safe-modify — backup pris avant cet increment (voir
#      _backups/_index.md, checkpoint 2026-07-21_00-31-49) : nouveau Mode 9
#      "Installer les scripts Recalbox" (onglet Avance), remplace cote outil
#      Windows le mecanisme FTP abandonne cote firmware (voir
#      RecalBoxDMD_tool.py v14). Panneau autonome (`_mode9_frame`, construit
#      dans `_build_mode_area_advanced()`) avec champ IP/nom reseau
#      (pre-rempli via `prefs.get("recalbox_ip")` puis, en arriere-plan,
#      `detect_recalbox_share()` -- jamais appele sur le thread principal,
#      un `Path(UNC).exists()` peut prendre plusieurs secondes si le nom ne
#      resout pas), bouton "Installer / Mettre a jour" dedie (thread propre
#      `_mode9_install_worker`, meme idiome que `_mode6_flash_worker` :
#      QueueWriter vers les logs, callback `_progress_cb`), label de
#      resultat. `_on_mode_changed()` : "9" ajoute au groupe qui desactive
#      le selecteur ROMs (2/6/7/9), panneau affiche/masque + `btn_start_adv`
#      desactive quand mode=="9" (place APRES le bloc Mode 8, dont le else
#      remet sinon toujours ce bouton a l'etat normal). `_on_language_changed()`
#      et `_detail_templates` (fr/en/es) mis a jour pour le mode 9.
#      Correctif au passage : le label de resultat du panneau change de
#      hauteur (vide <-> 1-2 lignes), ce qui perimait le decoupage de fond
#      du panneau (meme bug "bandes blanches" que documente pour
#      `_start_mode6_blinking`) -- `_reslice_mode9_frame()`
#      (`themes.slice_single_frame`) appelee apres chaque changement du
#      texte de resultat. Egalement : nouvelle phase dans `_pipeline_mode_1`
#      juste apres `download_defaults()`, meme resolution de cible que le
#      Mode 9 (detection auto puis prefs), skip silencieux si aucune cible.
#      Nouvelles cles `UI_TRANSLATIONS` (fr/en/es, parite verifiee
#      123/123/123) : mode9_short_title/mode9_panel_title/mode9_host_label/
#      mode9_btn_install/mode9_btn_running/mode9_summary. Verifie en
#      conditions reelles (lancement de l'outil, capture d'ecran + pilotage
#      souris/clavier Windows) : affichage du panneau en FR/EN, clic
#      Installer avec champ vide (message d'erreur correct, focus sur le
#      champ), avec hote injoignable (bouton "en cours" -> thread -> message
#      d'echec correct, bouton reactive, plus de bande blanche apres le
#      correctif). Pas de test reseau contre la vraie Recalbox dans cette
#      passe (depot GitHub scripts/ pas encore pousse).
#
# v31 — 2026-07-13 — safe-modify — pas de nouveau backup pris avant cet
#      increment (couvert par le backup `2026-07-13_18-16-01` ci-dessus,
#      point de restauration valide pour tout le lot v30+v31) -- suite au
#      retrait prevu de `default.raw565` du depot GitHub (pour eviter les
#      ecrasements par des utilisateurs tiers) :
#      (1) La popup "Choisir son image de secours" (_apply_choice, dans
#      _on_default_image_picker_clicked) applique desormais le choix
#      IMMEDIATEMENT dans le dossier de travail (sd_dir/systems/_defaults/),
#      sans condition sur l'existence prealable de sd_dir/systems (l'ancien
#      garde-fou "if systems_out.exists()" empechait toute application tant
#      qu'un Mode 1/2 n'avait pas deja tourne une fois) -- y compris pour le
#      choix "Visuel par defaut du projet" (is_reset), qui applique
#      desormais reellement PROJECT_DEFAULT_IMAGE_FILENAME
#      (default_RB.png) au lieu de ne rien faire en attendant un
#      retelechargement GitHub qui n'aura plus lieu.
#      (2) `_apply_custom_default_fallback()` (appelee en fin de pipeline,
#      apres download_defaults(), en Mode 1 ET Mode 2) utilise desormais ce
#      meme visuel reserve comme dernier recours quand aucun choix
#      personnalise n'est enregistre (prefs "default_fallback_image" vide) --
#      auparavant elle ne faisait rien dans ce cas, en supposant a tort que
#      download_defaults() avait deja fourni default.raw565 depuis GitHub.
#      Nouvelle constante de classe RetroBoxLEDGui.PROJECT_DEFAULT_IMAGE_FILENAME
#      ("default_RB.png"), partagee entre le picker et cette fonction.
#      (3) Bouton "Choisir son image de secours" retire de l'onglet Main
#      (Mode 1) : puisque le choix s'applique desormais immediatement au
#      dossier de travail (point 1), plus besoin de l'exposer une 2e fois en
#      Mode 1 -- il reste uniquement dans l'onglet Avance (Mode 2), pour
#      eviter d'avoir a introduire un mode dedie supplementaire ("Mode 9").
#      L'ordre pipeline Mode 1/Mode 2 (download_defaults() puis
#      _apply_custom_default_fallback() avant la fin du worker, donc avant
#      que _on_pipeline_finished ne revele le panneau "Copier sur la carte
#      SD") etait deja correct et n'a pas change.
#      (4) Traductions (fr/en/es) : "default_image_applied_next_run_msg"
#      (devenu obsolete, l'application n'est plus jamais differee) remplacee
#      par "default_image_apply_failed_msg" (erreur reelle d'ecriture) ;
#      "default_image_reset_applied_msg" mis a jour pour refleter
#      l'application immediate au lieu d'un retelechargement GitHub differe.
#
# v30 — 2026-07-13 — safe-modify — Popup "Choisir son image de secours" : la
#      tuile "Visuel par defaut du projet" (reset -> chaine vide,
#      default_fallback_image="") affiche desormais une vraie miniature
#      (tools/assets/default_images/default_RB.png) au lieu d'un simple
#      libelle sans image. Ce fichier reserve (constante
#      PROJECT_DEFAULT_IMAGE_FILENAME) est exclu de la boucle de scan de la
#      galerie (n'apparait plus une 2e fois comme choix normal) et protege
#      contre un ecrasement accidentel par _on_import (la boucle
#      anti-collision refuse desormais aussi ce nom de fichier precis, meme
#      si l'utilisateur importe un fichier qui porte coincidentalement ce
#      nom une fois assaini).
#
# v29 — 2026-07-13 — safe-modify — Popup "Choisir son image de secours" :
#      l'import d'une image personnalisee (_on_import) l'enregistre desormais
#      dans tools/assets/default_images/ (le meme "assets_dir" que celui
#      scanne pour peupler la galerie), au lieu de l'ancien chemin cache
#      prefs.PREFS_FILE.parent/"custom_default_source.png" (un seul fichier,
#      ecrase a chaque nouvel import, jamais visible dans la galerie). Le nom
#      de fichier est derive du nom d'origine (assaini via re.sub, extension
#      forcee en .png puisque l'image est reconvertie en PNG), avec suffixe
#      numerique en cas de collision. L'image importee apparait donc comme
#      une tuile normale de la galerie des la prochaine ouverture de la
#      popup, reutilisable sans avoir a la reimporter.
#
# v28 — 2026-07-13 — safe-modify — (1) Popup de sortie ("Quitter") : nouvelle
#      case a cocher "Conserver le dossier temporaire (ne pas supprimer)"
#      (_quit_keep_temp_dir, initialisee a False, memorisee entre deux
#      ouvertures de la popup dans la meme session). Les deux points d'appel
#      de _cleanup_sd_dir() (chemin direct "aucun traitement en cours" et
#      _wait_for_threads_then_exit(), apres arret d'un worker) verifient
#      desormais ce flag avant de supprimer sd_dir ; le message de la popup
#      finale reflete le resultat reel (quit_app_cleanup_done vs le nouveau
#      quit_app_kept_temp_dir).
#      (2) Texte du bouton partage "Image de secours" renomme en "Choisir
#      son image de secours" (FR) / "Choose your fallback image" (EN) /
#      "Elegir su imagen de respaldo" (ES) -- cle default_image_btn, utilisee
#      a la fois comme libelle du bouton (Main + Avance/Mode 2) et comme
#      titre des popups de confirmation associees.
#      (3) Langue par defaut si aucune preference n'est encore sauvegardee :
#      "en" au lieu de "fr" (fallback de validation de _saved_lang dans
#      __init__, cote RecalBoxDMD_prefs.py voir v5 -- _DEFAULTS["language"]).
#
# v27 — 2026-07-13 — safe-modify — Reorganisation de mise en page suite aux
#      retours utilisateur sur le selecteur d'image de secours (v26) :
#      (1) Le panneau "Version Recalbox" + le bouton "Image de secours" sont
#      deplaces dans la colonne gauche de l'onglet Main (entre le mode et
#      "Choisir dossier ROMs"), pour ne plus allonger la colonne droite et
#      masquer le panneau "Copier sur la carte SD" en dessous.
#      (2) En Mode 2 (onglet Avance), le bouton "Image de secours"
#      (_default_image_frame_adv) est desormais empile en haut (comme le
#      panneau Mode 3) au lieu du bas, pour apparaitre juste sous le cadre
#      "Details du mode selectionne".
#      (3) Le panneau "Copier sur la carte SD" (mode 6) est duplique dans
#      l'onglet Avance a une position equivalente a celle du Main (nouveau
#      _build_mode6_panel() partage ; _mode6_instances() retourne l'etat de
#      chaque instance construite -- Main et Avance -- pour que
#      _refresh_mode6_drives/_sync_mode6_texts/_start_mode6_blinking/
#      _stop_mode6_blinking/_on_mode6_flash_error/_on_mode6_flash_done/
#      _on_mode6_retry_failed_clicked/_on_language_changed bouclent sur les
#      deux au lieu de ne toucher qu'un seul widget code en dur).
#      _on_pipeline_finished ne force plus le retour sur l'onglet Main (le
#      mecanisme v25 _poll_processing_done() s'en charge deja) ; le panneau
#      mode6 et les boutons "Explorer" sont mis a jour quel que soit l'onglet
#      actif a la fin d'un traitement.
#      Rendre visible un panneau via pack() sans <<NotebookTabChanged>> ne
#      lui donne pas sa tranche de fond de theme -- corrige en appelant
#      themes.slice_single_frame(self, frame) (deja utilise pour le panneau
#      Mode 8) juste apres le premier pack() de chaque instance mode6 dans
#      _start_mode6_blinking() (la reslice globale _reslice_after_mode_change
#      provoque une corruption visuelle -- boutons dupliques, colonne gauche
#      cassee -- et a ete evitee ici).
#      (4) Correction visibilite du cadre progression : reveler le panneau
#      mode6 faisait grandir le Notebook (579px -> 635px), ne laissant que
#      4px avant le bas de la fenetre fixe (1100x750) et masquant les
#      boutons Pause/Reprise/Passe/Stop. La fenetre NE DOIT PAS etre
#      agrandie (bg.png de chaque theme est etire via PIL pour remplir
#      exactement 1100x750 ; changer ces dimensions imposerait de
#      regenerer/recadrer le fond de chaque theme). Corrige en reduisant le
#      gabarit du panneau mode6 lui-meme : Listbox drive_list height 5->3,
#      pady interne du frame 8->5, pady exterieur du panneau (les 3 sites
#      d'appel) 12->6, pady bouton/explore_btn reduits. Verifie : hauteur du
#      Notebook inchangee (579px) avant/apres reveal, marge de ~60px
#      retrouvee avant le bord de la fenetre.
#      (5) Mode 2 : si des fichiers existent deja dans systems/_defaults au
#      moment du clic "Demarrer", une boite de dialogue (mode2_overwrite_*)
#      demande a l'utilisateur d'ecraser ou de conserver (_on_start_clicked,
#      execute sur le thread principal avant le lancement du worker,
#      resultat stocke dans self._mode2_overwrite_existing) ; ce choix est
#      transmis a toolkit.download_defaults(..., overwrite_existing_files=...)
#      dans _pipeline_mode_2. "default.raw565" est toujours re-ecrase quel
#      que soit ce choix (voir RecalBoxDMD_tool.py v12). _pipeline_mode_2 ne
#      vide plus tout systems/ via shutil.rmtree avant l'extraction : le
#      sous-dossier _defaults est desormais preserve explicitement (sinon le
#      rmtree global aurait supprime les fichiers existants avant meme que
#      download_defaults() ne puisse decider d etre les conserver, rendant
#      le choix "Conserver" inoperant).
#
# v26 — 2026-07-11 — safe-modify — Selecteur d'image de secours
#      (default.raw565) : nouveau bouton partage (_build_default_image_button)
#      place dans l'onglet Main (a cote du panneau "Version Recalbox") et
#      dans le panneau Mode 2 de l'onglet Avance (affiche/masque comme
#      _mode3_profile_frame). Popup galerie (_on_default_image_picker_clicked)
#      listant les propositions de tools/assets/default_images/*.png (scan
#      dynamique) + tuile "visuel par defaut du projet" + import d'image
#      personnalisee (premier usage de filedialog.askopenfilename dans ce
#      fichier). Choix persiste (RecalBoxDMD_prefs "default_fallback_image"),
#      applique immediatement au dossier de travail si deja present, et
#      reapplique systematiquement apres chaque toolkit.download_defaults(...)
#      dans _pipeline_mode_1/_pipeline_mode_2 (qui reecrit sinon tout
#      systems/_defaults/ depuis GitHub) via _apply_custom_default_fallback().
#      Reutilise toolkit.set_default_fallback_image()/convert_png_to_raw565_only()
#      sans aucune modification de ces fonctions.
#
# v25 — 2026-07-11 — safe-modify — Retour automatique vers l'onglet (et
#      implicitement le mode, via _last_adv_mode deja gere par
#      _on_tab_changed) d'origine une fois un traitement termine.
#      _start_worker() memorise l'onglet actif lors du tout premier
#      demarrage (transition idle -> occupe), bascule sur Logs, puis
#      _poll_processing_done() (sondage 300ms) restaure l'onglet d'origine
#      des que _is_processing() redevient faux. Le pipeline principal
#      (_on_start_clicked) route desormais par _start_worker() au lieu
#      d'un threading.Thread direct, pour beneficier du meme mecanisme ;
#      l'ancien self.nb_top.select(self.tab_logs) manuel est retire
#      (redondant, _start_worker() le fait deja). Verifie : les 8 modes du
#      pipeline principal partagent tous _worker_main(), qui redirige
#      stdout/stderr vers les logs sans condition -- aucun n'est
#      "silencieux". Seule l'analyse prealable du bouton "Nettoyer avant
#      scrape" (Mode 1) ne loggue rien ; elle beneficie neanmoins du meme
#      cycle bascule/verrou/retour, sans impact notable vu sa duree tres
#      courte.
#
# v24 — 2026-07-11 — safe-modify — Suite a des remontees utilisateurs
#      (gel de l'appli en changeant d'onglet pendant un traitement,
#      fermeture par le X qui ne nettoie pas le dossier temporaire,
#      nettoyage parfois absent) :
#      (1) Suivi generique des threads d'arriere-plan : self._active_workers
#      (set) + _start_worker()/_is_processing(). Plusieurs operations
#      (comparaison finale Mode 8, retry flash Mode 6, nettoyage avant
#      scrape Mode 1) creaient un threading.Thread local jamais suivi nulle
#      part -- invisibles pour toute detection "traitement en cours".
#      (2) _on_tab_changed() bloque desormais le changement d'onglet (force
#      un retour sur Logs, index 2) tant que _is_processing() est vrai :
#      le decoupage de fond + le rendu du nouvel onglet pendant qu'un
#      worker ecrit intensivement dans les logs pouvait geler l'UI.
#      (3) _on_close_attempt() (bouton X, WM_DELETE_WINDOW) delegue
#      desormais entierement a _on_quit_app_clicked() au lieu d'une
#      implementation separee plus ancienne qui, dans le cas "aucun
#      traitement en cours", ne nettoyait JAMAIS le dossier temporaire et
#      ne proposait pas de l'explorer avant nettoyage.
#      (4) _cleanup_sd_dir() renforce : popup "Nettoyage en cours..."
#      (affichee avant l'appel bloquant, pour eviter qu'un utilisateur ne
#      tue l'appli via le gestionnaire de taches en la croyant gelee --
#      interrompant un rmtree en cours et laissant le dossier a moitie
#      supprime) + tentatives avec attente entre chaque (transitoire de
#      verrouillage de fichier Windows) + gestion des attributs
#      lecture-seule (os.chmod avant reessai).
#
# v23 — 2026-07-11 — safe-modify — Audit + correctif des traductions EN/ES
#      incompletes (rapporte par l'utilisateur). Cause principale :
#      _on_language_changed() ne rafraichissait que l'onglet Main ; la
#      quasi-totalite de l'onglet Avance (titres, boutons, panneau Mode 8,
#      panneaux "Version Recalbox") restait figee dans la langue de
#      demarrage apres un changement a chaud. Ajout du rafraichissement de
#      tous ces widgets + des noms d'onglets (Avance/Parametres/AIDE,
#      auparavant toujours en francais). Migration vers UI_TRANSLATIONS de
#      nombreux textes codes en dur (titres/messages des messagebox
#      Attention/Erreur, boutons Explorer SD/dossier temp/Fermer des popups
#      de fin de traitement, libelles NAS, filtre de niveau de logs et ses
#      3 valeurs -- la logique de filtrage interne comparait ces valeurs en
#      dur, corrigee pour comparer aux cles traduites courantes -- bouton
#      "Ouvrir dans le navigateur"). Cle "mode7_pick_btn" manquante ajoutee
#      au bloc "es" (provoquait un KeyError en espagnol sur le Mode 7). Les
#      3 blocs de UI_TRANSLATIONS ont desormais exactement les memes 103
#      cles (verifie par script).
#
# v22 — 2026-07-11 — safe-modify — Ajout du selecteur "Version Recalbox"
#      dans l'onglet Parametres (meme StringVar partagee
#      self._mode1_profile_var que Mode 1/3). Le combobox du Mode 8
#      (jusque-la independant, non persiste) utilise desormais cette meme
#      variable au lieu de son propre _mode8_version_var : un changement de
#      version depuis n'importe lequel des 4 emplacements (Parametres,
#      Mode 1, Mode 3, Mode 8) se repercute partout et persiste dans
#      RecalBoxDMD_prefs.json.
#
# v21 — 2026-07-11 — safe-modify — Renommage des identifiants de profil
#      Recalbox affiches/stockes : "10.1" -> "10.x", "9.2" -> "9.x" (partout
#      : combobox Mode 1/3 partagee, combobox Mode 8, valeurs par defaut,
#      cles de traduction mode1_scrape_help_*, mapping image d'aide au
#      scrape). "legacy" inchange. Meme changement cote
#      RecalBoxDMD_tool.py (RECALBOX_PROFILES) et RecalBoxDMD_prefs.py
#      (valeur par defaut de "recalbox_profile").
# v20 — 2026-07-11 — safe-modify — Correctif cosmetique : cadre pointille
#      (focus ring Tk) qui entourait le radio du mode par defaut (Mode 1 en
#      Main, Mode 2 en Avance) au demarrage. Ajout de
#      highlightthickness=0/takefocus=0 sur tous les Radiobutton de mode
#      (Main + Avance) -- ces boutons restent cliquables a la souris,
#      seule la navigation clavier (Tab) sur ces radios est retiree.
# v19 — 2026-07-10 — safe-modify — (1) Remplacement de la capture generique
#      (wiki) par 3 captures reelles fournies par l'utilisateur, annotees :
#      v10_logo.png ("SELECT LOGO TYPE"=CLEAR), v9_marquee.png
#      ("Selectionnez le type de vignette"=MARQUEE), v9_image_type.png
#      ("Selectionnez le type d'image"=LOGO DETOURE, profil legacy) --
#      textes d'aide FR/EN/ES mis a jour avec les libelles exacts vus dans
#      ces captures. (2) Ajout du meme systeme de profil "Version Recalbox"
#      au Mode 3 (onglet Avance, extraction gamelist.xml seule) : panneau
#      partage extrait dans _build_recalbox_profile_panel() (reutilise par
#      Mode 1 ET Mode 3, meme StringVar _mode1_profile_var), affiche/masque
#      selon le mode via _on_mode_changed(). _mode1_get_selected_systems()
#      detecte desormais l'onglet actif (Main/Avance) pour cibler la bonne
#      listbox de selection systemes. _on_mode1_clean_clicked()/
#      _mode1_run_clean() prennent le bouton cliquant en parametre (2
#      boutons "Nettoyer" distincts, un par onglet). _pipeline_mode_3
#      n'utilise plus le tag "logo" fixe : un seul tag XML selon le profil
#      choisi, comme _pipeline_mode_1.
# v18 — 2026-07-10 — safe-modify — Mode 1 : nouveau panneau "Version
#      Recalbox" (colonne droite, onglet Main) reutilisant RECALBOX_PROFILES
#      -- combobox persistee (RecalBoxDMD_prefs "recalbox_profile"), bouton
#      "Comment scraper ?" (instructions + capture d'ecran annotee par
#      profil) et bouton "Nettoyer les dossiers avant scrape" (apercu du
#      nombre de fichiers via list_scrape_media_files, confirmation
#      obligatoire, suppression via clean_scrape_media_folders -- cible le
#      dossier media de CHAQUE systeme du dossier ROMs, pas un dossier
#      unique). _pipeline_mode_1 n'utilise plus le fallback aveugle
#      logo/image/thumbnail : un seul tag XML selon le profil choisi.
# v17 — 2026-07-10 — safe-modify — Correctif "Explorer la carte SD" (popup
#      de fin de copie mode 6) : self._mode6_selected_drive etait
#      initialise a None en __init__ et jamais assigne ensuite -> le
#      bouton appelait os.startfile("") (avale silencieusement par le
#      except). Assigne desormais dans _mode6_flash_worker et
#      _mode6_flash_retry_failed_worker (meme lecteur que _mode6_last_dst_drive).
# v16 — 2026-07-10 — safe-modify — Reprise de la copie SD (mode 6) : au
#      clic sur "Lancer la copie", detection d'une copie interrompue via
#      _copy_progress.json (lecture par read_copy_manifest) -> message
#      specifique proposant de reprendre (skip fichiers deja copies) ou
#      repartir de zero. En cas d'echecs, nouveau dialogue 3 choix
#      (Reessayer tout / Reessayer seulement les echecs / Annuler) au lieu
#      du askretrycancel binaire -- le retry cible utilise
#      _copy_specific_files() sur les seuls fichiers en echec.
#
# v15 — 2026-07-10 — safe-modify — _build_progress_frame() : les 2 lignes
#      d'info (progress_var 10pt, progress_sub_var 9pt) utilisaient
#      width=55 (caracteres) -- a police differente, "55 caracteres" ne
#      correspond pas a la meme largeur en pixels, donc la 2e ligne
#      (police plus petite) etait visiblement plus courte que la 1ere.
#      Remplace par sticky="we" sur la colonne 0 (deja figee a
#      minsize=420 via frm.grid_columnconfigure) : les deux lignes ont
#      desormais la meme largeur en pixels, independamment de la police.
#
# v14 — 2026-07-10 — safe-modify — outer.grid_columnconfigure(1, ...) :
#      ajout de minsize=290 (Main ET Avance). Mesure (diag_all_cols.py) :
#      grid_remove() sur middle/middle_adv (modes 2/6/7) retire TOUT
#      l'espace de la colonne 1, ce qui decale right_adv de ~110px vers la
#      gauche (rootx 711 en modes 2/6/7 contre ~808 en modes 3/4/5/8) --
#      cause reelle du cadre "Details du mode selectionne" pas a la meme
#      position/dimension d'un mode a l'autre. Les correctifs precedents
#      (sticky "nsw") empechaient l'etirement mais pas ce retrait complet
#      de colonne. minsize reserve la largeur de la colonne 1 meme quand
#      son widget est absent, gardant "left" et "right"/"right_adv" a une
#      position fixe quel que soit le mode.
#
# v13 — 2026-07-10 — safe-modify — self.right/self.right_adv (colonne
#      "Details du mode selectionne") : sticky "nsew" -> "nsw", meme
#      correctif et meme cause que v12 sur "left" (mesure : right_adv
#      passait de 298px a ~415px quand middle_adv est masque via
#      grid_remove). Ici l'effet etait plus visible qu'un simple ecart de
#      layout : le decoupage de l'image de fond (slice) est capture a une
#      largeur donnee -- un changement de largeur entre deux passages du
#      decoupage decale le crop et laisse apparaitre des bandes blanches
#      (fond par defaut du Label) la ou l'image ne couvre plus le cadre
#      elargi.
#
# v12 — 2026-07-10 — safe-modify — (1) left.grid sticky "nsew" -> "nsw"
#      (onglets Main ET Avance) : mesure -> quand middle/middle_adv est
#      masque via grid_remove() (modes 2/6/7), le poids de grille
#      redistribuait sa largeur aux colonnes restantes et "left" passait
#      de 363px (contenu naturel) a ~480px de rendu reel, alors que
#      path_box/Demarrer/Quitter (fill="x") suivaient -- largeur
#      incoherente d'un mode a l'autre. Sans le "e" de sticky, la colonne
#      garde sa largeur naturelle (363px, verifie identique sur tous les
#      modes 2 a 8) quel que soit l'etat des colonnes voisines.
#      (2) _update_sys_box_decor() : mode 8 ajoute a decorative_mode (en
#      plus de 3/4/5) -- il utilise aussi sys_list/sys_list_adv, donc doit
#      beneficier du meme decoupage decoratif tant que le cadre n'est pas
#      peuple de systemes.
#
# v11 — 2026-07-10 — safe-modify — Suite du lot v10 (meme backup) :
#      (1) anchor="w" ajoute aux 7 Radiobutton de mode (option du widget,
#      pas seulement de pack()) : sans elle, fill="x" centrait
#      indicateur+texte dans la largeur etiree, donnant l'impression que
#      les libelles courts (Mode 5/6) etaient decales par rapport aux
#      longs.
#      (2) _mode8_frame recoit desormais son decoupage de fond (via le
#      nouveau themes.slice_single_frame(), cf RecalBoxDMD_themes.py v3) :
#      il devient visible via un changement de mode radio, jamais un
#      changement d'onglet, donc le cycle global de decoupage ne le
#      voyait jamais.
#      (3) Nouveau _update_sys_box_decor() : pour les modes 3/4/5, tant
#      que sys_list/sys_list_adv est vide, le cadre englobant affiche le
#      decoupage decoratif du theme (via themes.slice_frame_overlay(),
#      place au-dessus de la Listbox vide -- necessaire car elle est
#      opaque et couvre tout le cadre, contrairement au panneau Mode 8 ou
#      les interstices suffisaient). Des que des systemes sont detectes,
#      bascule sur le fond opaque bg_listbox du theme actif (lisibilite).
#      Stocke desormais sys_list_box/sys_list_box_adv (avant : variables
#      locales "box" non accessibles hors constructeur).
#
# v10 — 2026-07-10 — safe-modify — Onglet Avance :
#                    (1) titres de mode encore raccourcis + police 12pt
#                    (mesure via tkfont : colonne "left" = 363px, la
#                    version precedente des titres courts n'aurait pas
#                    tenu sur 1 ligne a une police plus grande) + pack
#                    fill="x" pour que les 7 radios aient exactement la
#                    meme largeur que path_box/Demarrer/Quitter en dessous.
#                    (2) left.grid sticky "nw" -> "nsew" : le spacer deja
#                    present pousse maintenant reellement path_box/
#                    Demarrer/Quitter vers le bas de la colonne (ne
#                    fonctionnait pas avant car "left" ne s'etirait pas).
#                    (3) Mode 8 : suppression de _mode8_btn ("Lancer la
#                    verification", redondant -- il appelait deja
#                    _on_start_clicked, exactement comme btn_start_adv).
#                    btn_start_adv reste maintenant actif en mode 8 et son
#                    texte devient "Demarrer la verification" au lieu
#                    d'etre desactive.
# v9 — 2026-07-10 — safe-modify — Retour arriere du slot a hauteur fixe
#                    ajoute en v8 : il faisait deborder root (reqheight 813
#                    > 750, hauteur fixe de la fenetre), poussant les
#                    boutons du cadre Progression hors de la fenetre.
#                    Remplace par :
#                    (1) titres de mode courts sur 1 seule ligne pour les 7
#                    radios de l'onglet Avance (nouvelles cles
#                    UI_TRANSLATIONS "mode2_short_title".."mode8_short_title",
#                    ne touche pas mode2_title etc. utilisees par le CLI) --
#                    des lors que toutes les lignes font 1 ligne, le pack()
#                    naturel redevient parfaitement regulier sans slot
#                    artificiel ; (2) enrichissement de _detail_templates
#                    avec une "Marche a suivre" par mode, pour occuper
#                    l'espace vertical libere dans le panneau "Details du
#                    mode selectionne".
# v8 — 2026-07-10 — safe-modify — Onglet Avance, colonne des 7 radios de
#                    mode (2 a 8) : chaque radio est desormais packee dans
#                    un slot de hauteur fixe (pack_propagate(False)) au lieu
#                    de rb.pack(anchor="w", pady=2) directement -- les
#                    libelles longs (Mode 2/3/4) wrappaient sur 2 lignes et
#                    les courts (Mode 5/6/7/8) sur 1 seule, ce qui donnait un
#                    espacement vertical irregulier entre les options
#                    (wraplength inchange a 350px : c'est la largeur
#                    disponible reelle de la colonne, l'augmenter faisait
#                    deborder le texte hors du cadre).
# v7 — 2026-07-10 — safe-modify — sys_list/sys_list_adv : couleur de
#                    selection initiale alignee sur le nouveau bleu fort
#                    (#1565C0) utilise par RecalBoxDMD_themes.py pour
#                    toutes les Listbox (lisibilite de la selection)
# v6 — 2026-07-10 — safe-modify — _on_mode_changed() : le cadre de
#                    detection/selection des systemes (middle/middle_adv)
#                    est maintenant reellement masque pour les modes 2, 6
#                    et 7 (qui utilisent le dossier temporaire, pas de
#                    choix de dossier ROMs) -- le else de show_sys_col ne
#                    faisait rien auparavant
# v5 — 2026-07-10 — safe-modify — Fin de Mode 8 ("Lancer la verification"
#                    et "Detecter les images manquantes sur la SD") :
#                    retour automatique sur l'onglet Avance + ouverture
#                    automatique du rapport correspondant. Renommage du
#                    bouton "Comparer avec le support final" en "Detecter
#                    les images manquantes sur la SD"
# v4 — 2026-07-10 — safe-modify — _progress_cb_ui() : ajout des libelles
#                    "mode8_check"/"final_media" (barre de progression
#                    alimentee pendant la verification Mode 8, qui restait
#                    figee car check_missing_images_gamelist() n'appelait
#                    jamais progress_cb())
# v3 — 2026-07-10 — safe-modify — Ajout bouton "Comparer avec le support
#                    final" dans le panneau Mode 8 : lance
#                    check_final_media()/generate_final_media_report()
#                    pour distinguer images manquantes cote ROMs / non
#                    converties / non copiees sur SD
# v2 — 2026-07-10 — safe-modify — _poll_logs ne se replanifie plus si la
#                    fenêtre est détruite (corrige TclError "invalid command
#                    name ..._poll_logs" à la fermeture de l'appli)
# v1 — 2026-07-10 — Version de base (avant safe-modify)
# ============================================
import queue
from secrets import choice
import sys
import threading
import subprocess
import os
import shutil
import stat
import time
import re
import webbrowser
from dataclasses import dataclass
from pathlib import Path
import RecalBoxDMD_prefs as prefs
import RecalBoxDMD_themes as themes
import RecalBoxDMD_md_renderer as md_renderer
from typing import Callable, Optional, Sequence, cast

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, font as tkfont


@dataclass(frozen=True)
class GuiConfig:
    mode_choice: str  # "1".."5"
    roms_root: Path
    systems_selected: Optional[Sequence[Path]]
    nas_user: str
    nas_password: str
    nas_path_is_unc: bool


# 2026-08-11 -- regex d'auto-lien pour les popups/details de mode contenant
# des URLs (pack ultimate GitHub/forum Arcadia). Volontairement PAS un rendu
# markdown complet (reserve a RecalBoxDMD_md_renderer/l'onglet Aide) : ce
# texte contient des caracteres (underscores dans _defaults, parentheses...)
# qu'un vrai parseur markdown interpreterait a tort -- seules les URLs
# http(s) deviennent des liens cliquables, tout le reste est affiche
# verbatim. Voir _insert_autolink_text().
_AUTOLINK_URL_RE = re.compile(r"https?://[^\s<>\)\]]+")

UI_TRANSLATIONS = {
    "fr": {
        "sys_all_selected": "Aucun système sélectionné — tous les systèmes seront traités.",
        "sys_sel_opt_all": "Tout sélectionner",
        "sys_sel_opt_none": "Ne rien sélectionner",
        "sys_sel_warn_empty": "Aucun système sélectionné. Cliquez sur « Tout sélectionner » ou sélectionnez au moins un système.",
        "quit_app": "Quitter l'application",
        "quit_app_warning_title": "Quitter et nettoyer",
        "quit_app_warning": "Quitter l'application va supprimer les dossiers temporaires (sd_card). Continuer ?",
        "quit_app_keep_temp_checkbox": "Conserver le dossier temporaire (ne pas supprimer)",
        "quit_app_stopped_worker": "Traitement en cours : arrêt demandé avant la fermeture.",
        "quit_app_cleanup_done": "Dossiers temporaires supprimés.",
        "quit_app_kept_temp_dir": "Dossier temporaire conservé (non supprimé).",
        "cleanup_in_progress": "Nettoyage en cours...",
        "open_output_prompt": "Ouvrir le dossier de sortie contenant les éléments produits ?",
        "open_output_yes": "Oui",
        "open_output_no": "Non",
        "mode6_panel_title": "Copier sur la carte SD",
        "mode6_btn_start": "Démarrer la copie",
        "mode6_no_drives": "Aucun lecteur amovible détecté. Insérez votre carte SD puis réessayez.",
        "mode6_drive_choice_none": "Aucun lecteur sélectionné — sélection du premier lecteur disponible.",
        "mode6_overwrite_title": "Options",
        "mode6_overwrite_yes": "Écraser les fichiers existants",
        "mode6_overwrite_no": "Conserver les fichiers existants (ignorer)",
        "mode6_resume_title": "Copie précédente interrompue",
        "mode6_resume_msg": (
            "Une copie précédente vers ce lecteur semble interrompue "
            "({copied}/{total} fichiers copiés le {date}).\n\n"
            "Cliquez sur Oui pour REPARTIR DE ZÉRO (tout écraser).\n"
            "Cliquez sur Non pour REPRENDRE où elle s'est arrêtée "
            "(ignorer les fichiers déjà copiés)."
        ),
        "mode6_retry_title": "Erreur de copie",
        "mode6_retry_all_btn": "Réessayer tout",
        "mode6_retry_failed_btn": "Réessayer seulement les échecs ({n})",
        "mode6_retry_cancel_btn": "Annuler",
        "mode6_btn_running": "Copie en cours…",
        "mode6_done": "Copie sur la carte SD terminée.",
        "progress_title": "Progression",
        "btn_pause": "Pause",
        "btn_resume": "Reprise",
        "btn_skip": "Passe",
        "btn_stop": "Stop",
        "logs_details_title": "Détails / sortie",
        "language_title": "Langue",
        "mode_label": "Mode",
        "roms_pick_btn": "Choisir dossier ROMs",
        "mode7_pick_btn": 'Choisir le dossier "systems" contenant "_defaults"',
        "images_pick_btn": "Choix des dossiers Images",
        "start_btn": "DÉMARRER",
        "detect_systems_btn": "Détection des systèmes (gamelist.xml)",
        "select_images_btn": "Sélection des dossiers images",
        "systems_to_process_lbl": "Systèmes à traiter (clic pour sélectionner)",
        "mode_detail_title": "Détails du mode selectionné",
        "mode6_drives_title": "Lecteurs amovibles (carte SD)",
        "mode6_explore_output_btn": "Explorer le dossier de sortie",
        "mode8_panel_title": "Verification des images manquantes",
        "mode8_btn_check": "Démarrer la vérification",
        "mode8_btn_running": "Verification en cours...",
        "mode8_done": "Verification terminee.",
        "mode8_report": "Rapport genere :",
        "mode8_open_report": "Ouvrir le rapport",
        "mode8_version_label": "Version Recalbox",
        "mode9_panel_title": "Installation des scripts Recalbox",
        "mode9_host_label": "Recalbox (IP ou nom réseau)",
        "mode9_btn_install": "Installer / Mettre à jour",
        "mode9_btn_running": "Installation en cours...",
        "mode9_summary": lambda ok, total: (
            f"✅ {ok}/{total} fichier(s) installé(s)"
            if ok == total
            else f"⚠️ {ok}/{total} fichier(s) installé(s)"
        ),
        "mode1_profile_label": "Version Recalbox (scrape marquee/logo)",
        "slow_threshold_label": "Seuil flag L (systèmes lents)",
        "slow_threshold_hint": (
            "Nombre de fichiers convertis au-delà duquel un système est "
            "marqué \"lent\" (écran d'attente au lancement). À augmenter "
            "si votre carte SD est rapide, à réduire si elle est lente."
        ),
        "mode1_scrape_help_btn": "Comment scraper ?",
        "mode1_clean_btn": "Nettoyer les dossiers avant scrape",
        "mode1_clean_confirm_title": "Confirmer le nettoyage",
        "mode1_clean_confirm_msg": (
            "{count} fichier(s) seront supprimés dans le dossier « {folder} » "
            "de {n_systems} système(s) du dossier ROMs sélectionné.\n\n"
            "Cette action est irréversible et ne supprime que ces images déjà "
            "scrapées (pas les ROMs, ni les gamelist.xml). Continuer ?"
        ),
        "mode1_clean_none_msg": (
            "Aucun fichier trouvé dans « {folder} » pour les systèmes "
            "sélectionnés — rien à nettoyer."
        ),
        "mode1_clean_done_msg": (
            "{deleted} fichier(s) supprimé(s) dans « {folder} » "
            "({n_systems} système(s))."
        ),
        "mode1_clean_errors_msg": "\n\n{n_errors} erreur(s) rencontrée(s) (voir logs).",
        "mode1_clean_need_roms": (
            "Choisis d'abord un dossier ROMs (Détection des systèmes conseillée)."
        ),
        "mode1_scrape_help_title": "Comment scraper la marquee/logo ?",
        "mode1_scrape_help_10.x": (
            "Depuis Recalbox 10.x, le menu SCRAPEUR a un nouveau champ dédié "
            "« SELECT LOGO TYPE » (nom encore en anglais sur les versions "
            "alpha/bêta) — règle-le sur « CLEAR » (voir l'encadré rouge sur "
            "la capture ci-dessous, tirée de ton propre menu).\n\n"
            "Ne règle PAS « SÉLECTIONNEZ LE TYPE D'IMAGE » (encadré en "
            "pointillés) sur cette valeur : ce champ sert au visuel "
            "principal (écran de jeu, jaquette...), pas au logo.\n\n"
            "Les fichiers seront stockés dans « media/wheels/ » de chaque "
            "système, référencés par la balise <logo> du gamelist.xml — "
            "c'est exactement ce que ce profil va chercher."
        ),
        "mode1_scrape_help_9.x": (
            "Sur les versions de Recalbox sans champ « logo » dédié, règle "
            "« SÉLECTIONNEZ LE TYPE DE VIGNETTE » sur « MARQUEE » (encadré "
            "rouge sur la capture ci-dessous, tirée de ton propre menu) — "
            "PAS « SÉLECTIONNEZ LE TYPE D'IMAGE » (encadré en pointillés), "
            "qui sert à un autre visuel.\n\n"
            "Les fichiers seront stockés dans « media/thumbnails/ » de "
            "chaque système, référencés par la balise <thumbnail> — c'est ce "
            "que ce profil va chercher (recommandé)."
        ),
        "mode1_scrape_help_legacy": (
            "Profil « legacy » : la marquee est récupérée via le champ "
            "« SÉLECTIONNEZ LE TYPE D'IMAGE » (encadré rouge sur la capture "
            "ci-dessous), réglé sur « LOGO DÉTOURÉ » ou « MARQUEE » — "
            "dossier « media/images/ », balise <image>.\n\n"
            "À utiliser seulement si ta configuration Recalbox scrape "
            "réellement le logo dans ce champ précis — sinon préfère le "
            "profil « 9.x » (champ Vignette)."
        ),
        "tab_advanced": "Avancé",
        "tab_playlist": "Playlist",
        "tab_params": "Paramètres",
        "tab_help": "AIDE",
        "playlist_sd_section_title": "Carte SD",
        "playlist_refresh_drives_btn": "🔄 Rafraîchir",
        "playlist_explanation_text_normal": (
            "Cochez un dossier entier pour prendre tous ses GIFs, ou cliquez sur son "
            "nom pour cocher les fichiers un par un (survol = aperçu animé). Une "
            "ligne orange signale une sélection partielle (certains fichiers "
            "seulement) ; le nom du dossier actuellement affiché à droite est "
            "souligné. « Ajouter un dossier PC... » importe un ou plusieurs dossiers "
            "depuis l'ordinateur (sélection multiple). « Supprimer » efface les "
            "dossiers/fichiers cochés (destructif, confirmation demandée). "
            "« Construire la playlist » enregistre la sélection sous le nom choisi. "
            "Le bouton orange « Régénérer le cache playlist » régénère "
            "cache_master_gifs.dat, un récapitulatif de tous les GIFs de la carte "
            "(usage interne, invisible pour le firmware)."
        ),
        "playlist_explanation_text_mode1_add": (
            "Étape du Mode 1 — ajout de GIFs personnels : cliquez sur « Ajouter "
            "un dossier PC... » (plusieurs dossiers possibles, un par un) — les "
            "dossiers ajoutés apparaissent « en attente », déselectionnez si "
            "besoin les fichiers à ne pas garder, puis cliquez sur « Copier la "
            "sélection » pour les copier réellement. Une fois prêt, cliquez sur "
            "le bouton orange clignotant « Continuer » pour poursuivre le Mode 1."
        ),
        "playlist_explanation_text_mode1_playlist": (
            "Étape du Mode 1 — construction d'une playlist : cochez les "
            "dossiers/fichiers à inclure, donnez un nom à la playlist puis "
            "cliquez sur « Construire la playlist ». Une fois prête, cliquez sur "
            "le bouton orange clignotant « Continuer » pour poursuivre le Mode 1 "
            "avec cette playlist."
        ),
        "playlist_name_label": "Nom de la playlist (existante ou nouvelle)",
        "playlist_delete_btn": "Supprimer",
        "playlist_folders_title": "Dossiers (gifs/)",
        "playlist_select_all_btn": "Tout",
        "playlist_select_none_btn": "Rien",
        "playlist_add_external_folder_btn": "Ajouter un dossier PC...",
        "playlist_delete_gif_btn": "Supprimer",
        "playlist_files_title": "Fichiers (cochez ceux à inclure)",
        "playlist_build_btn": "Construire la playlist",
        "playlist_copy_pending_btn": "Copier la sélection",
        "playlist_copy_pending_nothing_msg": "Cochez au moins un fichier à copier.",
        "playlist_regen_cache_btn": "Régénérer le\ncache playlist",
        "playlist_temp_note_add": lambda tmp_dir: (
            "Mode temporaire (Mode 1) — ajout de GIFs : cliquez sur « Ajouter un "
            "dossier PC... », déselectionnez si besoin les fichiers à ne pas "
            "garder, puis « Copier la sélection ». Cliquez ensuite sur "
            "« Continuer » (bouton orange clignotant) pour poursuivre."
        ),
        "playlist_temp_note_playlist": lambda tmp_dir: (
            "Mode temporaire (Mode 1) — construction de playlist : sélectionnez "
            "vos dossiers/fichiers, nommez la playlist puis « Construire la "
            "playlist ». Cliquez ensuite sur « Continuer » (bouton orange "
            "clignotant) pour poursuivre le Mode 1."
        ),
        "playlist_temp_popup_title": "GIFs personnalisés",
        "playlist_temp_popup_msg": lambda tmp_dir: (
            "Cliquez sur « Ajouter un dossier PC... » ci-dessous pour choisir un ou "
            "plusieurs dossiers de GIFs depuis votre ordinateur.\n\n"
            "Une fois terminé, cliquez sur « Régénérer le cache playlist » (bouton "
            "orange clignotant) pour continuer : le Mode 1 reprendra automatiquement."
        ),
        "playlist_pending_not_copied_title": "Dossiers non copiés",
        "playlist_pending_not_copied_msg": lambda n, names: (
            f"{n} dossier(s) ajouté(s) ({names}) n'ont pas encore été copiés "
            "— ils seront ignorés si vous continuez.\n\n"
            "Continuer quand même ?"
        ),
        "playlist_no_sd_msg": "Choisissez d'abord une carte SD.",
        "playlist_no_gif_folders_found_msg": "Aucun dossier contenant des .gif n'a été trouvé dans ce dossier (ni ses sous-dossiers).",
        "playlist_multi_folder_dialog_title": "Ajouter des dossiers PC",
        "playlist_subfolder_checklist_prompt": (
            "Cochez les sous-dossiers de « {root} » à importer :"
        ),
        "playlist_import_dialog_title": "Ajouter un dossier PC",
        "playlist_import_dialog_prompt": "Nom du dossier de destination (dans gifs/) :",
        "playlist_import_done_msg_multi": (
            "{n} dossier(s) importé(s) : {names}\n\n"
            "{copied} fichier(s) copié(s) au total, {skipped} identique(s) ignoré(s), "
            "{renamed} renommé(s) (conflit de nom).\n{n_updated} playlist(s) mise(s) à jour automatiquement."
        ),
        "playlist_import_done_msg": (
            "{copied} fichier(s) copié(s), {skipped} identique(s) ignoré(s), "
            "{renamed} renommé(s) (conflit de nom).\n{n_updated} playlist(s) mise(s) à jour automatiquement."
        ),
        "playlist_delete_gif_nothing_msg": "Cochez au moins un dossier ou un fichier à supprimer.",
        "playlist_delete_gif_confirm_title_sd": "Confirmer la suppression",
        "playlist_delete_gif_confirm_msg_sd": (
            "Cette action supprime définitivement les dossiers/fichiers cochés de la carte SD, "
            "et les retire des playlists qui les référençaient.\n\nContinuer ?"
        ),
        "playlist_delete_gif_confirm_title_temp": "Retirer du dossier temporaire",
        "playlist_delete_gif_confirm_msg_temp": (
            "Cette action supprime les dossiers/fichiers cochés du dossier temporaire local "
            "(aucun impact sur votre carte SD, ces fichiers n'y ont pas encore été copiés).\n\n"
            "Continuer ?"
        ),
        "playlist_name_required_msg": "Indiquez un nom de playlist.",
        "playlist_build_empty_msg": "Cochez au moins un dossier ou un fichier.",
        "playlist_build_done_msg": "Playlist enregistrée : {n} entrée(s).",
        "playlist_delete_confirm_title": "Supprimer la playlist",
        "playlist_delete_confirm_msg": "Supprimer définitivement la playlist « {name} » ?",
        "playlist_regen_done_msg": "Cache régénéré : {nf} dossier(s), {ne} fichier(s).",
        "msg_error_title": "Erreur",
        "msg_warning_title": "Attention",
        "mode1_rb_confirm_title": "Recalbox détectée",
        "mode1_rb_confirm_msg": lambda target: (
            f"Recalbox détectée : {target}\n\n"
            "Si plusieurs Recalbox sont allumées sur le réseau, vérifiez "
            "qu'il s'agit bien de la bonne avant de continuer.\n\n"
            "Installer les scripts sur cette cible maintenant ?"
        ),
        "mode1_rb_declined_title": "Installation des scripts annulée",
        "mode1_rb_declined_msg": (
            "Les scripts Recalbox (marquee) ne seront pas installés "
            "maintenant.\n\n"
            "Utilisez le Mode 9 (onglet Avancé) pour les installer plus "
            "tard, en indiquant la bonne cible."
        ),
        "mode1_manual_ip_title": "Adresse Recalbox",
        "mode1_manual_ip_prompt": (
            "Entrez l'adresse IP ou le nom réseau de votre Recalbox :"
        ),
        "mode1_manual_ip_ok": "OK",
        "mode1_manual_ip_cancel": "Annuler",
        "mode1_rb_unreachable_title": "Recalbox injoignable",
        "mode1_rb_unreachable_msg": lambda ip, staged_dir: (
            f"Recalbox injoignable ({ip}) — vérifiez qu'elle est bien "
            "allumée et joignable sur le réseau, vérifiez son IP.\n\n"
            "Souhaitez-vous ressaisir l'IP manuellement, ou passer en "
            "Mode 9 (onglet Avancé) pour installer les scripts plus "
            "tard ?\n\n"
            "Attention : sans ces scripts, le DMD ne sera pas relié à la "
            "Recalbox et ne fonctionnera qu'en mode playlist/horloge.\n\n"
            f"Vous pouvez aussi copier vous-même ces scripts depuis "
            f"« {staged_dir} » vers le dossier « share/userscripts » de "
            "votre Recalbox à la fin du Mode 1."
        ),
        "mode1_rb_retry_ip_btn": "Ressaisir l'IP",
        "mode1_rb_use_mode9_btn": "Mode 9 plus tard",
        "mode1_rb_reminder_title": "Rappel : scripts Recalbox non installés",
        "mode1_rb_reminder_msg": lambda ip, staged_dir: (
            f"Les scripts Recalbox n'ont pas été installés automatiquement "
            f"({ip} injoignable ou non confirmée).\n\n"
            "Passez en Mode 9 (onglet Avancé) pour les installer plus "
            "tard.\n\n"
            "Attention : sans ces scripts, le DMD ne sera pas relié à la "
            "Recalbox et ne fonctionnera qu'en mode playlist/horloge.\n\n"
            f"Vous pouvez aussi copier vous-même ces scripts depuis "
            f"« {staged_dir} » vers le dossier « share/userscripts » de "
            "votre Recalbox."
        ),
        "mode1_fallback_image_title": "Image de secours",
        "mode1_fallback_image_msg": (
            "Voulez-vous choisir une image de secours personnalisée "
            "maintenant ?\n\n"
            "Si vous répondez non, le visuel par défaut du projet sera "
            "utilisé — vous pourrez changer ce choix plus tard depuis "
            "l'onglet Avancé."
        ),
        "sdcard_dialog_title": "Carte SD",
        "sdcard_dialog_prompt": lambda min_gb: (
            f"Insérez une carte SD d'au moins {min_gb:.0f} Go, formatée en "
            "FAT32, puis sélectionnez-la ci-dessous.\n\n"
            "Utilisez « Rafraîchir » si elle n'apparaît pas tout de suite."
        ),
        "sdcard_dialog_refresh_btn": "🔄 Rafraîchir",
        "sdcard_dialog_ok_btn": "OK",
        "sdcard_dialog_cancel_btn": "Annuler",
        "sdcard_dialog_none_selected": "Sélectionnez d'abord un lecteur dans la liste.",
        "sdcard_dialog_err_notfound": "Ce lecteur n'est plus disponible — rafraîchissez la liste.",
        "sdcard_dialog_err_fs": lambda fs: (
            f"Système de fichiers non pris en charge ({fs}) — la carte doit être formatée en FAT32."
        ),
        "sdcard_dialog_err_size": lambda size_gb, min_gb: (
            f"Carte trop petite ({size_gb} Go) — {min_gb:.0f} Go minimum requis."
        ),
        "gifpack_q_title": "Pack de GIFs gratuit",
        "gifpack_q_msg": (
            "Télécharger le pack gratuit de 600 GIFs (thèmes variés) "
            "depuis GitHub ?\n\n"
            "Vous pourrez aussi ajouter vos propres GIFs à l'étape suivante.\n\n"
            "Pour un pack bien plus complet (pack ultimate, ~11000 animations "
            "pixel-perfect pour DMD), voir https://rpiteam.carrd.co/ et le "
            "forum Arcadia : "
            "https://www.neo-arcadia.com/forum/viewtopic.php?t=67065"
        ),
        "gifpack_q_yes": "Oui",
        "gifpack_q_no": "Non",
        "lang_images_title": "Langue des images système",
        "lang_images_msg": (
            "Dans quelle langue veux-tu les images système/genres "
            "(_defaults/) ? Les genres pas encore traduits dans la langue "
            "choisie restent affichés en anglais.\n\n"
            "Aperçu (exemple ci-dessous) :"
        ),
        "lang_images_en": "🇬🇧 Anglais (EN)",
        "lang_images_fr": "🇫🇷 Français (FR)",
        "lang_images_es": "🇪🇸 Espagnol (ES)",
        "customgifs_q_title": "GIFs personnalisés",
        "customgifs_q_msg": (
            "Voulez-vous ajouter vos propres GIFs (onglet Playlist) avant "
            "de continuer ?"
        ),
        "customgifs_q_yes": "Oui",
        "customgifs_q_later": "Plus tard",
        "mode1_build_playlist_q_title": "Créer une playlist ?",
        "mode1_build_playlist_q_msg": (
            "Voulez-vous créer une ou plusieurs playlists maintenant à "
            "partir de vos GIFs ?\n\n"
            "Facultatif : une playlist par défaut sera de toute façon "
            "sélectionnée automatiquement (le pack téléchargé et/ou "
            "ALL.txt) — vous pourrez toujours en créer une plus tard."
        ),
        "playlist_temp_continue_btn": "Continuer",
        "btn_close": "Fermer",
        "explore_sd_btn": "📂 Explorer la carte SD",
        "explore_temp_btn": "📁 Explorer le dossier temporaire",
        "help_open_browser_btn": "Ouvrir dans le navigateur",
        "logs_level_label": "Niveau :",
        "logs_level_all": "Tout",
        "logs_level_warn_err": "Alertes+Erreurs",
        "logs_level_err": "Erreurs",
        "nas_creds_title": "Identifiants NAS (UNC)",
        "nas_user_label": "Utilisateur NAS",
        "nas_password_label": "Mot de passe NAS",
        "roms_root_missing_msg": "Choisis un dossier ROMs d'abord.",
        "roms_root_notfound_msg": "Dossier ROMs introuvable.",
        "processing_in_progress_msg": "Traitement déjà en cours.",
        "no_systems_detected_msg": "Aucun système détecté (dossiers images introuvables).",
        "unc_access_error_msg": (
            "Accès réseau impossible (lecture gamelist.xml ou images). "
            "Renseignez NAS user/mdp, puis cliquez « Détecter systèmes »."
        ),
        "what_to_do_next": "Que voulez-vous faire ?",
        "mode1_next_steps_title": "Prochaines étapes",
        "mode1_next_steps_msg": (
            "1. Insérez la carte SD dans le DMD.\n\n"
            "2. Démarrez-le (raccordez-le à une source d'alimentation).\n\n"
            "3. Le DMD basculera automatiquement en mode configuration, en 2 temps :\n"
            "   • D'abord pour configurer votre WiFi, depuis votre smartphone.\n"
            "   • Puis pour configurer le DMD lui-même (création des playlists, "
            "choix de la playlist à lancer au démarrage, etc.), depuis un "
            "navigateur internet."
        ),
        "processing_done_title": "✅ Terminé !",
        "existing_files_sd_title": "Fichiers existants sur la SD",
        "existing_files_sd_msg": (
            "Des fichiers existent déjà sur {letter}:\\\n\n"
            "Cliquez sur Oui pour ÉCRASER tous les fichiers existants.\n"
            "Cliquez sur Non pour IGNORER et garder les fichiers actuels."
        ),
        "default_image_btn": "Choisir son image de secours",
        "default_image_dialog_title": "Choisir l'image de secours",
        "default_image_dialog_intro": (
            "Cette image s'affiche sur le panneau DMD quand aucun visuel "
            "specifique n'est trouve pour un jeu ou un systeme. Choisissez "
            "une proposition ci-dessous, ou importez votre propre image "
            "(elle sera automatiquement redimensionnee/adaptee au format "
            "128x32)."
        ),
        "default_image_reset_label": "Visuel par défaut du projet",
        "default_image_import_btn": "Importer mon image...",
        "default_image_choose_btn": "Choisir",
        "default_image_applied_now_msg": (
            "Image de secours appliquee : {name}\n\n"
            "Deja mise a jour dans le dossier de travail en cours."
        ),
        "default_image_apply_failed_msg": (
            "Echec de l'application de l'image de secours dans le dossier "
            "de travail : {name}\n\n"
            "Verifiez les journaux (onglet Logs)."
        ),
        "default_image_reset_applied_msg": (
            "Retour au visuel par defaut du projet.\n\n"
            "Deja mis a jour dans le dossier de travail en cours."
        ),
        "mode2_overwrite_title": "Fichiers _defaults déjà présents",
        "mode2_overwrite_msg": (
            "Des fichiers existent déjà dans le dossier _defaults.\n\n"
            "Cliquez sur Oui pour les ÉCRASER (récupérer les dernières "
            "versions depuis GitHub).\n"
            "Cliquez sur Non pour CONSERVER les fichiers actuels.\n\n"
            "Dans les deux cas, l'image de secours (default.raw565) sera "
            "mise à jour selon votre choix dans « Image de secours »."
        ),
        "mode8_btn_compare_final": "Detecter les images manquantes sur la SD",
        "mode8_btn_compare_running": "Comparaison en cours...",
        "mode8_compare_ask_sd_title": "Comparer avec la SD physique ?",
        "mode8_compare_ask_sd": (
            "Comparer aussi avec une carte SD physique (en plus du dossier "
            "temporaire) ?\n\nOui : choisir le lecteur de la carte SD.\n"
            "Non : comparer uniquement avec le dossier temporaire."
        ),
        "mode8_open_final_report": "Ouvrir le rapport final",
        "mode8_final_done": "Comparaison finale terminee.",
        # Titres courts (1 ligne) pour les 7 radios de mode de l'onglet
        # Avance : mode2_title etc. (toolkit.tr) sont trop longs et cassent
        # sur 2 lignes de facon inegale d'une option a l'autre.
        "mode2_short_title": "MODE 2 — _defaults (GitHub)",
        "mode3_short_title": "MODE 3 — Extraction (gamelist)",
        "mode4_short_title": "MODE 4 — PNG/GIF → raw565",
        "mode5_short_title": "MODE 5 — 128x32",
        "mode6_short_title": "MODE 6 — games_cache.bin",
        "mode7_short_title": "MODE 7 — systems_cache.dat",
        "mode8_short_title": "MODE 8 — Images manquantes",
        "mode9_short_title": "MODE 9 — Installer les scripts Recalbox",
        "mode10_short_title": "MODE 10 — Image de secours",
        "mode11_short_title": "MODE 11 — Pack 600 GIFs (GitHub)",
        # En-tetes de categorie de l'accordeon (2026-08-11, v43) -- noms
        # techniques donnes par l'utilisateur, gardes identiques dans les
        # 3 langues (FR/EN/ES).
        "accordion_cat_github": "DOWNLOAD FROM GITHUB",
        "accordion_cat_gamelist": "GAMELIST.XML",
        "accordion_cat_images": "IMAGES TOOLS",
        "accordion_cat_caches": "CACHES",
        "accordion_cat_scripts": "SCRIPTS RECALBOX",
    },
    "en": {
        "sys_all_selected": "No system selected — all systems will be processed.",
        "sys_sel_opt_all": "Select all",
        "sys_sel_opt_none": "Select none",
        "sys_sel_warn_empty": "No system selected. Click « Select all » or select at least one system.",
        "quit_app": "Quit application",
        "quit_app_warning_title": "Quit and cleanup",
        "quit_app_warning": "Quitting will delete temporary folders (sd_card). Continue ?",
        "quit_app_keep_temp_checkbox": "Keep the temporary folder (don't delete)",
        "quit_app_stopped_worker": "Processing is running: stop requested before closing.",
        "quit_app_cleanup_done": "Temporary folders deleted.",
        "quit_app_kept_temp_dir": "Temporary folder kept (not deleted).",
        "cleanup_in_progress": "Cleaning up...",
        "open_output_prompt": "Open the output folder containing produced elements?",
        "open_output_yes": "Yes",
        "open_output_no": "No",
        "mode6_panel_title": "Choice 8 — Copy to SD card",
        "mode6_btn_start": "Start copy",
        "mode6_no_drives": "No removable drive detected. Insert your SD card and try again.",
        "mode6_drive_choice_none": "No drive selected — picking the first available drive.",
        "mode6_overwrite_title": "Options",
        "mode6_overwrite_yes": "Overwrite existing files",
        "mode6_overwrite_no": "Keep existing files (skip)",
        "mode6_resume_title": "Previous copy interrupted",
        "mode6_resume_msg": (
            "A previous copy to this drive appears interrupted "
            "({copied}/{total} files copied on {date}).\n\n"
            "Click Yes to START OVER (overwrite everything).\n"
            "Click No to RESUME where it left off "
            "(skip files already copied)."
        ),
        "mode6_retry_title": "Copy error",
        "mode6_retry_all_btn": "Retry all",
        "mode6_retry_failed_btn": "Retry failed files only ({n})",
        "mode6_retry_cancel_btn": "Cancel",
        "mode6_btn_running": "Flashing…",
        "mode6_done": "SD card copy completed.",
        "progress_title": "Progress",
        "btn_pause": "Pause",
        "btn_resume": "Resume",
        "btn_skip": "Skip",
        "btn_stop": "Stop",
        "logs_details_title": "Details / output",
        "language_title": "Language",
        "mode_label": "Mode",
        "roms_pick_btn": "Choose ROMs folder",
        "mode7_pick_btn": 'Choose the "systems" folder containing "_defaults"',
        "images_pick_btn": "Choose images folders",
        "start_btn": "START",
        "detect_systems_btn": "Detect systems (gamelist.xml)",
        "select_images_btn": "Select image folders",
        "systems_to_process_lbl": "Systems to process (click to select)",
        "mode_detail_title": "Mode details",
        "mode6_drives_title": "Removable drives (SD card)",
        "mode6_explore_output_btn": "Explore output folder",
        "mode8_panel_title": "Missing images check",
        "mode8_btn_check": "Start check",
        "mode8_btn_running": "Checking...",
        "mode8_done": "Check completed.",
        "mode8_report": "Report generated:",
        "mode8_open_report": "Open report",
        "mode8_version_label": "Recalbox version",
        "mode9_panel_title": "Recalbox scripts installation",
        "mode9_host_label": "Recalbox (IP or network name)",
        "mode9_btn_install": "Install / Update",
        "mode9_btn_running": "Installing...",
        "mode9_summary": lambda ok, total: (
            f"✅ {ok}/{total} file(s) installed"
            if ok == total
            else f"⚠️ {ok}/{total} file(s) installed"
        ),
        "mode1_profile_label": "Recalbox version (marquee/logo scrape)",
        "slow_threshold_label": "Slow-flag threshold (slow systems)",
        "slow_threshold_hint": (
            "Number of converted files above which a system is marked "
            "\"slow\" (loading screen on launch). Raise it if your SD "
            "card is fast, lower it if it's slow."
        ),
        "mode1_scrape_help_btn": "How to scrape?",
        "mode1_clean_btn": "Clean folders before scraping",
        "mode1_clean_confirm_title": "Confirm cleanup",
        "mode1_clean_confirm_msg": (
            "{count} file(s) will be deleted in the \"{folder}\" folder of "
            "{n_systems} system(s) in the selected ROMs folder.\n\n"
            "This is irreversible and only removes these already-scraped "
            "images (not the ROMs, nor the gamelist.xml files). Continue?"
        ),
        "mode1_clean_none_msg": (
            "No files found in \"{folder}\" for the selected systems — "
            "nothing to clean."
        ),
        "mode1_clean_done_msg": (
            "{deleted} file(s) deleted in \"{folder}\" ({n_systems} system(s))."
        ),
        "mode1_clean_errors_msg": "\n\n{n_errors} error(s) encountered (see logs).",
        "mode1_clean_need_roms": (
            "Choose a ROMs folder first (system detection recommended)."
        ),
        "mode1_scrape_help_title": "How to scrape the marquee/logo?",
        "mode1_scrape_help_10.x": (
            "Since Recalbox 10.x, the SCRAPER menu has a new dedicated "
            "\"SELECT LOGO TYPE\" field (still untranslated on alpha/beta "
            "builds) — set it to \"CLEAR\" (red box on the screenshot below, "
            "taken from an actual Recalbox menu; the labels shown are in "
            "French — \"Sélectionnez le type d'image\" = image type).\n\n"
            "Do NOT set that value on \"Sélectionnez le type d'image\" / "
            "image type (dashed box): that field is for the main visual "
            "(screenshot, box art...), not the logo.\n\n"
            "Files are stored in \"media/wheels/\" for each system, "
            "referenced by the <logo> tag in gamelist.xml — exactly what "
            "this profile looks for."
        ),
        "mode1_scrape_help_9.x": (
            "On Recalbox versions without a dedicated \"logo\" field, set "
            "\"Sélectionnez le type de vignette\" / thumbnail type to "
            "\"MARQUEE\" (red box on the screenshot below, from an actual "
            "Recalbox menu — labels shown are in French) — NOT "
            "\"Sélectionnez le type d'image\" / image type (dashed box), "
            "used for another visual.\n\n"
            "Files are stored in \"media/thumbnails/\" for each system, "
            "referenced by the <thumbnail> tag — what this profile looks "
            "for (recommended)."
        ),
        "mode1_scrape_help_legacy": (
            "\"legacy\" profile: the marquee is retrieved via "
            "\"Sélectionnez le type d'image\" / image type (red box on the "
            "screenshot below), set to \"LOGO DÉTOURÉ\" (Clear Logo) or "
            "\"MARQUEE\" — folder \"media/images/\", <image> tag.\n\n"
            "Only use this if your Recalbox setup really scrapes the logo "
            "into that exact field — otherwise prefer the \"9.x\" profile "
            "(thumbnail field)."
        ),
        "tab_advanced": "Advanced",
        "tab_playlist": "Playlist",
        "tab_params": "Settings",
        "tab_help": "HELP",
        "playlist_sd_section_title": "SD card",
        "playlist_refresh_drives_btn": "🔄 Refresh",
        "playlist_explanation_text_normal": (
            "Check a whole folder to take all its GIFs, or click its name to check "
            "files one by one (hover = animated preview). An orange row means a "
            "partial selection (some files only); the currently displayed folder's "
            "name is underlined. \"Add a PC folder...\" imports one or more folders "
            "from your computer (multi-select). \"Delete\" removes the checked "
            "folders/files (destructive, confirmation required). \"Build playlist\" "
            "saves the selection under the chosen name. The orange \"Regenerate "
            "playlist cache\" button regenerates cache_master_gifs.dat, a summary "
            "of all GIFs on the card (internal use, invisible to the firmware)."
        ),
        "playlist_explanation_text_mode1_add": (
            "Mode 1 step — adding custom GIFs: click \"Add a PC folder...\" "
            "(several folders possible, one at a time) — added folders show up "
            "as \"pending\", uncheck any files you don't want to keep, then "
            "click \"Copy selection\" to actually copy them. Once ready, click "
            "the blinking orange \"Continue\" button to resume Mode 1."
        ),
        "playlist_explanation_text_mode1_playlist": (
            "Mode 1 step — building a playlist: check the folders/files to "
            "include, name the playlist, then click \"Build playlist\". Once "
            "ready, click the blinking orange \"Continue\" button to resume "
            "Mode 1 with this playlist."
        ),
        "playlist_name_label": "Playlist name (existing or new)",
        "playlist_delete_btn": "Delete",
        "playlist_folders_title": "Folders (gifs/)",
        "playlist_select_all_btn": "All",
        "playlist_select_none_btn": "None",
        "playlist_add_external_folder_btn": "Add a PC folder...",
        "playlist_delete_gif_btn": "Delete",
        "playlist_files_title": "Files (check those to include)",
        "playlist_build_btn": "Build playlist",
        "playlist_copy_pending_btn": "Copy selection",
        "playlist_copy_pending_nothing_msg": "Check at least one file to copy.",
        "playlist_regen_cache_btn": "Regenerate\nplaylist cache",
        "playlist_temp_note_add": lambda tmp_dir: (
            "Temporary mode (Mode 1) — adding GIFs: click \"Add a PC folder...\", "
            "uncheck any files you don't want to keep, then \"Copy selection\". "
            "Then click \"Continue\" (blinking orange button) to resume."
        ),
        "playlist_temp_note_playlist": lambda tmp_dir: (
            "Temporary mode (Mode 1) — building a playlist: select your "
            "folders/files, name the playlist, then \"Build playlist\". Then "
            "click \"Continue\" (blinking orange button) to resume Mode 1."
        ),
        "playlist_temp_popup_title": "Custom GIFs",
        "playlist_temp_popup_msg": lambda tmp_dir: (
            "Click \"Add a PC folder...\" below to choose one or more GIF folders "
            "from your computer.\n\n"
            "When done, click \"Regenerate playlist cache\" (blinking orange "
            "button) below to continue: Mode 1 will resume automatically."
        ),
        "playlist_pending_not_copied_title": "Folders not copied",
        "playlist_pending_not_copied_msg": lambda n, names: (
            f"{n} added folder(s) ({names}) haven't been copied yet — they "
            "will be ignored if you continue.\n\n"
            "Continue anyway?"
        ),
        "playlist_no_sd_msg": "Choose an SD card first.",
        "playlist_no_gif_folders_found_msg": "No folder containing .gif files was found in this folder (or its subfolders).",
        "playlist_multi_folder_dialog_title": "Add PC folders",
        "playlist_subfolder_checklist_prompt": (
            "Check the subfolders of \"{root}\" to import:"
        ),
        "playlist_import_dialog_title": "Add a PC folder",
        "playlist_import_dialog_prompt": "Destination folder name (inside gifs/):",
        "playlist_import_done_msg_multi": (
            "{n} folder(s) imported: {names}\n\n"
            "{copied} file(s) copied in total, {skipped} identical skipped, "
            "{renamed} renamed (name conflict).\n{n_updated} playlist(s) automatically updated."
        ),
        "playlist_import_done_msg": (
            "{copied} file(s) copied, {skipped} identical skipped, "
            "{renamed} renamed (name conflict).\n{n_updated} playlist(s) automatically updated."
        ),
        "playlist_delete_gif_nothing_msg": "Check at least one folder or file to delete.",
        "playlist_delete_gif_confirm_title_sd": "Confirm deletion",
        "playlist_delete_gif_confirm_msg_sd": (
            "This permanently deletes the checked folders/files from the SD card, "
            "and removes them from any playlist referencing them.\n\nContinue?"
        ),
        "playlist_delete_gif_confirm_title_temp": "Remove from temporary folder",
        "playlist_delete_gif_confirm_msg_temp": (
            "This removes the checked folders/files from the local temporary "
            "folder (no impact on your SD card -- these files haven't been "
            "copied there yet).\n\nContinue?"
        ),
        "playlist_name_required_msg": "Enter a playlist name.",
        "playlist_build_empty_msg": "Check at least one folder or file.",
        "playlist_build_done_msg": "Playlist saved: {n} entrie(s).",
        "playlist_delete_confirm_title": "Delete playlist",
        "playlist_delete_confirm_msg": "Permanently delete playlist \"{name}\"?",
        "playlist_regen_done_msg": "Cache regenerated: {nf} folder(s), {ne} file(s).",
        "msg_error_title": "Error",
        "msg_warning_title": "Warning",
        "mode1_rb_confirm_title": "Recalbox detected",
        "mode1_rb_confirm_msg": lambda target: (
            f"Recalbox detected: {target}\n\n"
            "If several Recalbox units are powered on on the network, make "
            "sure this is the right one before continuing.\n\n"
            "Install the scripts on this target now?"
        ),
        "mode1_rb_declined_title": "Scripts installation cancelled",
        "mode1_rb_declined_msg": (
            "The Recalbox scripts (marquee) will not be installed now.\n\n"
            "Use Mode 9 (Advanced tab) to install them later, pointing to "
            "the right target."
        ),
        "mode1_manual_ip_title": "Recalbox address",
        "mode1_manual_ip_prompt": (
            "Enter your Recalbox's IP address or network name:"
        ),
        "mode1_manual_ip_ok": "OK",
        "mode1_manual_ip_cancel": "Cancel",
        "mode1_rb_unreachable_title": "Recalbox unreachable",
        "mode1_rb_unreachable_msg": lambda ip, staged_dir: (
            f"Recalbox unreachable ({ip}) — check that it's turned on and "
            "reachable on the network, check its IP.\n\n"
            "Do you want to re-enter the IP manually, or switch to Mode 9 "
            "(Advanced tab) to install the scripts later?\n\n"
            "Warning: without these scripts, the DMD won't be linked to "
            "the Recalbox and will only work in playlist/clock mode.\n\n"
            f"You can also manually copy these scripts yourself from "
            f"\"{staged_dir}\" to the \"share/userscripts\" folder on your "
            "Recalbox at the end of Mode 1."
        ),
        "mode1_rb_retry_ip_btn": "Re-enter IP",
        "mode1_rb_use_mode9_btn": "Mode 9 later",
        "mode1_rb_reminder_title": "Reminder: Recalbox scripts not installed",
        "mode1_rb_reminder_msg": lambda ip, staged_dir: (
            f"The Recalbox scripts were not installed automatically "
            f"({ip} unreachable or not confirmed).\n\n"
            "Switch to Mode 9 (Advanced tab) to install them later.\n\n"
            "Warning: without these scripts, the DMD won't be linked to "
            "the Recalbox and will only work in playlist/clock mode.\n\n"
            f"You can also manually copy these scripts yourself from "
            f"\"{staged_dir}\" to the \"share/userscripts\" folder on your "
            "Recalbox."
        ),
        "mode1_fallback_image_title": "Fallback image",
        "mode1_fallback_image_msg": (
            "Do you want to choose a custom fallback image now?\n\n"
            "If you answer no, the project's default visual will be used "
            "— you can change this choice later from the Advanced tab."
        ),
        "sdcard_dialog_title": "SD card",
        "sdcard_dialog_prompt": lambda min_gb: (
            f"Insert an SD card of at least {min_gb:.0f} GB, formatted as "
            "FAT32, then select it below.\n\n"
            "Use \"Refresh\" if it doesn't show up right away."
        ),
        "sdcard_dialog_refresh_btn": "🔄 Refresh",
        "sdcard_dialog_ok_btn": "OK",
        "sdcard_dialog_cancel_btn": "Cancel",
        "sdcard_dialog_none_selected": "Select a drive from the list first.",
        "sdcard_dialog_err_notfound": "This drive is no longer available — refresh the list.",
        "sdcard_dialog_err_fs": lambda fs: (
            f"Unsupported file system ({fs}) — the card must be formatted as FAT32."
        ),
        "sdcard_dialog_err_size": lambda size_gb, min_gb: (
            f"Card too small ({size_gb} GB) — {min_gb:.0f} GB minimum required."
        ),
        "gifpack_q_title": "Free GIF pack",
        "gifpack_q_msg": (
            "Download the free pack of 600 GIFs (assorted themes) from "
            "GitHub?\n\n"
            "You can also add your own GIFs in the next step.\n\n"
            "For a much larger pack (ultimate pack, ~11,000 pixel-perfect "
            "DMD animations), see https://rpiteam.carrd.co/ and the "
            "Arcadia forum: "
            "https://www.neo-arcadia.com/forum/viewtopic.php?t=67065"
        ),
        "gifpack_q_yes": "Yes",
        "gifpack_q_no": "No",
        "lang_images_title": "System images language",
        "lang_images_msg": (
            "Which language do you want the system/genre images "
            "(_defaults/) in? Genres not yet translated into the chosen "
            "language stay displayed in English.\n\n"
            "Preview (example below):"
        ),
        "lang_images_en": "🇬🇧 English (EN)",
        "lang_images_fr": "🇫🇷 French (FR)",
        "lang_images_es": "🇪🇸 Spanish (ES)",
        "customgifs_q_title": "Custom GIFs",
        "customgifs_q_msg": (
            "Do you want to add your own GIFs (Playlist tab) before "
            "continuing?"
        ),
        "customgifs_q_yes": "Yes",
        "customgifs_q_later": "Later",
        "mode1_build_playlist_q_title": "Create a playlist?",
        "mode1_build_playlist_q_msg": (
            "Do you want to create one or more playlists now from your "
            "GIFs?\n\n"
            "Optional: a default playlist will be selected automatically "
            "either way (the downloaded pack and/or ALL.txt) — you can "
            "always create one later."
        ),
        "playlist_temp_continue_btn": "Continue",
        "btn_close": "Close",
        "explore_sd_btn": "📂 Browse SD card",
        "explore_temp_btn": "📁 Browse temporary folder",
        "help_open_browser_btn": "Open in browser",
        "logs_level_label": "Level:",
        "logs_level_all": "All",
        "logs_level_warn_err": "Warnings+Errors",
        "logs_level_err": "Errors",
        "nas_creds_title": "NAS credentials (UNC)",
        "nas_user_label": "NAS user",
        "nas_password_label": "NAS password",
        "roms_root_missing_msg": "Choose a ROMs folder first.",
        "roms_root_notfound_msg": "ROMs folder not found.",
        "processing_in_progress_msg": "Processing already in progress.",
        "no_systems_detected_msg": "No system detected (image folders not found).",
        "unc_access_error_msg": (
            "Network access failed (reading gamelist.xml or images). "
            "Enter the NAS user/password, then click \"Detect systems\"."
        ),
        "what_to_do_next": "What would you like to do?",
        "mode1_next_steps_title": "Next steps",
        "mode1_next_steps_msg": (
            "1. Insert the SD card into the DMD.\n\n"
            "2. Power it on (connect it to a power source).\n\n"
            "3. The DMD will automatically switch to configuration mode, in 2 stages:\n"
            "   • First to set up your WiFi, from your smartphone.\n"
            "   • Then to configure the DMD itself (creating playlists, choosing "
            "which playlist starts by default, etc.), from a web browser."
        ),
        "processing_done_title": "✅ Done!",
        "existing_files_sd_title": "Existing files on the SD card",
        "existing_files_sd_msg": (
            "Files already exist on {letter}:\\\n\n"
            "Click Yes to OVERWRITE all existing files.\n"
            "Click No to SKIP and keep the current files."
        ),
        "default_image_btn": "Choose your fallback image",
        "default_image_dialog_title": "Choose the fallback image",
        "default_image_dialog_intro": (
            "This image is shown on the DMD panel when no specific artwork "
            "is found for a game or system. Pick a proposal below, or "
            "import your own image (it will be automatically resized/"
            "adapted to the 128x32 format)."
        ),
        "default_image_reset_label": "Project's default artwork",
        "default_image_import_btn": "Import my image...",
        "default_image_choose_btn": "Choose",
        "default_image_applied_now_msg": (
            "Fallback image applied: {name}\n\n"
            "Already updated in the current working folder."
        ),
        "default_image_apply_failed_msg": (
            "Failed to apply the fallback image to the working folder: "
            "{name}\n\n"
            "Check the Logs tab."
        ),
        "default_image_reset_applied_msg": (
            "Reverted to the project's default artwork.\n\n"
            "Already updated in the current working folder."
        ),
        "mode2_overwrite_title": "Existing _defaults files",
        "mode2_overwrite_msg": (
            "Files already exist in the _defaults folder.\n\n"
            "Click Yes to OVERWRITE them (fetch the latest versions from "
            "GitHub).\n"
            "Click No to KEEP the current files.\n\n"
            "Either way, the fallback image (default.raw565) will be "
            "updated according to your \"Fallback image\" choice."
        ),
        "mode8_btn_compare_final": "Detect missing images on SD card",
        "mode8_btn_compare_running": "Comparing...",
        "mode8_compare_ask_sd_title": "Compare with physical SD card?",
        "mode8_compare_ask_sd": (
            "Also compare with a physical SD card (in addition to the "
            "temporary folder)?\n\nYes: choose the SD card drive.\n"
            "No: compare with the temporary folder only."
        ),
        "mode8_open_final_report": "Open final report",
        "mode8_final_done": "Final comparison completed.",
        "mode2_short_title": "MODE 2 — _defaults (GitHub)",
        "mode3_short_title": "MODE 3 — Extraction (gamelist)",
        "mode4_short_title": "MODE 4 — PNG/GIF → raw565",
        "mode5_short_title": "MODE 5 — 128x32",
        "mode6_short_title": "MODE 6 — games_cache.bin",
        "mode7_short_title": "MODE 7 — systems_cache.dat",
        "mode8_short_title": "MODE 8 — Missing images",
        "mode9_short_title": "MODE 9 — Install Recalbox scripts",
        "mode10_short_title": "MODE 10 — Fallback image",
        "mode11_short_title": "MODE 11 — 600 GIFs pack (GitHub)",
        "accordion_cat_github": "DOWNLOAD FROM GITHUB",
        "accordion_cat_gamelist": "GAMELIST.XML",
        "accordion_cat_images": "IMAGES TOOLS",
        "accordion_cat_caches": "CACHES",
        "accordion_cat_scripts": "SCRIPTS RECALBOX",
    },
    "es": {
        "sys_all_selected": "No se seleccionó ningún sistema — se procesarán todos los sistemas.",
        "sys_sel_opt_all": "Seleccionar todo",
        "sys_sel_opt_none": "No seleccionar",
        "sys_sel_warn_empty": "Ningún sistema seleccionado. Haz clic en « Seleccionar todo » o selecciona al menos un sistema.",
        "quit_app": "Salir de la aplicación",
        "quit_app_warning_title": "Salir y limpiar",
        "quit_app_warning": "Al salir se borrarán las carpetas temporales (sd_card). ¿Continuar?",
        "quit_app_keep_temp_checkbox": "Conservar la carpeta temporal (no eliminar)",
        "quit_app_stopped_worker": "Hay un proceso en ejecución: se solicitará la detención antes de cerrar.",
        "quit_app_cleanup_done": "Carpetas temporales eliminadas.",
        "quit_app_kept_temp_dir": "Carpeta temporal conservada (no eliminada).",
        "cleanup_in_progress": "Limpiando...",
        "open_output_prompt": "¿Abrir la carpeta de salida con los elementos generados?",
        "open_output_yes": "Sí",
        "open_output_no": "No",
        "mode6_panel_title": "Opción 8 — Copiar a la tarjeta SD",
        "mode6_btn_start": "Iniciar la copia",
        "mode6_no_drives": "No se detectó ninguna unidad extraíble. Inserta tu tarjeta SD y vuelve a intentar.",
        "mode6_drive_choice_none": "No se seleccionó ninguna unidad — se elige la primera disponible.",
        "mode6_overwrite_title": "Opciones",
        "mode6_overwrite_yes": "Sobrescribir archivos existentes",
        "mode6_overwrite_no": "Conservar archivos existentes (omitir)",
        "mode6_resume_title": "Copia anterior interrumpida",
        "mode6_resume_msg": (
            "Una copia anterior a esta unidad parece interrumpida "
            "({copied}/{total} archivos copiados el {date}).\n\n"
            "Haz clic en Sí para EMPEZAR DE NUEVO (sobrescribir todo).\n"
            "Haz clic en No para REANUDAR donde se detuvo "
            "(omitir los archivos ya copiados)."
        ),
        "mode6_retry_title": "Error de copia",
        "mode6_retry_all_btn": "Reintentar todo",
        "mode6_retry_failed_btn": "Reintentar solo los fallidos ({n})",
        "mode6_retry_cancel_btn": "Cancelar",
        "mode6_btn_running": "Flasheando…",
        "mode6_done": "Copia en tarjeta SD completada.",
        "progress_title": "Progreso",
        "btn_pause": "Pausar",
        "btn_resume": "Reanudar",
        "btn_skip": "Saltar",
        "btn_stop": "Parar",
        "logs_details_title": "Detalles / salida",
        "language_title": "Idioma",
        "mode_label": "Modo",
        "roms_pick_btn": "Elegir carpeta ROMs",
        "images_pick_btn": "Elegir carpetas de imágenes",
        "start_btn": "INICIAR",
        "detect_systems_btn": "Detectar sistemas (gamelist.xml)",
        "select_images_btn": "Selección de carpetas de imágenes",
        "systems_to_process_lbl": "Sistemas a procesar (clic para seleccionar)",
        "mode_detail_title": "Detalles del modo",
        "mode6_drives_title": "Unidades extraíbles (tarjeta SD)",
        "mode6_explore_output_btn": "Explorar la carpeta de salida",
        "mode8_panel_title": "Verificacion de imagenes faltantes",
        "mode8_btn_check": "Iniciar verificacion",
        "mode8_btn_running": "Verificando...",
        "mode8_done": "Verificacion completada.",
        "mode8_report": "Informe generado:",
        "mode8_open_report": "Abrir informe",
        "mode8_version_label": "Version de Recalbox",
        "mode9_panel_title": "Instalación de los scripts de Recalbox",
        "mode9_host_label": "Recalbox (IP o nombre de red)",
        "mode9_btn_install": "Instalar / Actualizar",
        "mode9_btn_running": "Instalando...",
        "mode9_summary": lambda ok, total: (
            f"✅ {ok}/{total} archivo(s) instalado(s)"
            if ok == total
            else f"⚠️ {ok}/{total} archivo(s) instalado(s)"
        ),
        "mode1_profile_label": "Version de Recalbox (scrape de marquee/logo)",
        "slow_threshold_label": "Umbral flag L (sistemas lentos)",
        "slow_threshold_hint": (
            "Número de archivos convertidos a partir del cual un sistema "
            "se marca como \"lento\" (pantalla de espera al iniciar). "
            "Auméntelo si su tarjeta SD es rápida, redúzcalo si es lenta."
        ),
        "mode1_scrape_help_btn": "Como hacer el scrape?",
        "mode1_clean_btn": "Limpiar carpetas antes del scrape",
        "mode1_clean_confirm_title": "Confirmar limpieza",
        "mode1_clean_confirm_msg": (
            "Se eliminaran {count} archivo(s) en la carpeta \"{folder}\" de "
            "{n_systems} sistema(s) de la carpeta ROMs seleccionada.\n\n"
            "Esta accion es irreversible y solo elimina estas imagenes ya "
            "escrapeadas (no las ROMs ni los gamelist.xml). Continuar?"
        ),
        "mode1_clean_none_msg": (
            "No se encontraron archivos en \"{folder}\" para los sistemas "
            "seleccionados — nada que limpiar."
        ),
        "mode1_clean_done_msg": (
            "{deleted} archivo(s) eliminado(s) en \"{folder}\" "
            "({n_systems} sistema(s))."
        ),
        "mode1_clean_errors_msg": "\n\n{n_errors} error(es) encontrado(s) (ver logs).",
        "mode1_clean_need_roms": (
            "Elige primero una carpeta ROMs (se recomienda detectar sistemas)."
        ),
        "mode1_scrape_help_title": "Como hacer el scrape de la marquee/logo?",
        "mode1_scrape_help_10.x": (
            "Desde Recalbox 10.x, el menu SCRAPEUR tiene un nuevo campo "
            "dedicado \"SELECT LOGO TYPE\" (aun sin traducir en versiones "
            "alpha/beta) — ajustalo a \"CLEAR\" (recuadro rojo en la captura "
            "de abajo, tomada de un menu real de Recalbox; las etiquetas "
            "mostradas estan en frances — \"Sélectionnez le type d'image\" "
            "= tipo de imagen).\n\n"
            "NO ajustes ese valor en \"Sélectionnez le type d'image\" / tipo "
            "de imagen (recuadro punteado): ese campo es para el visual "
            "principal (captura, caratula...), no para el logo.\n\n"
            "Los archivos se guardan en \"media/wheels/\" de cada sistema, "
            "referenciados por la etiqueta <logo> del gamelist.xml — "
            "exactamente lo que busca este perfil."
        ),
        "mode1_scrape_help_9.x": (
            "En versiones de Recalbox sin campo \"logo\" dedicado, ajusta "
            "\"Sélectionnez le type de vignette\" / tipo de miniatura a "
            "\"MARQUEE\" (recuadro rojo en la captura de abajo, de un menu "
            "real de Recalbox — etiquetas en frances) — NO \"Sélectionnez "
            "le type d'image\" / tipo de imagen (recuadro punteado), "
            "usado para otro visual.\n\n"
            "Los archivos se guardan en \"media/thumbnails/\" de cada "
            "sistema, referenciados por la etiqueta <thumbnail> — lo que "
            "busca este perfil (recomendado)."
        ),
        "mode1_scrape_help_legacy": (
            "Perfil \"legacy\": la marquee se obtiene mediante "
            "\"Sélectionnez le type d'image\" / tipo de imagen (recuadro "
            "rojo en la captura de abajo), ajustado a \"LOGO DÉTOURÉ\" "
            "(Clear Logo) o \"MARQUEE\" — carpeta \"media/images/\", "
            "etiqueta <image>.\n\n"
            "Usalo solo si tu configuracion de Recalbox realmente escrapea "
            "el logo en ese campo exacto — si no, prefiere el perfil "
            "\"9.x\" (campo miniatura)."
        ),
        "tab_advanced": "Avanzado",
        "tab_playlist": "Playlist",
        "tab_params": "Configuración",
        "tab_help": "AYUDA",
        "playlist_sd_section_title": "Tarjeta SD",
        "playlist_refresh_drives_btn": "🔄 Actualizar",
        "playlist_explanation_text_normal": (
            "Marca una carpeta entera para tomar todos sus GIFs, o haz clic en su "
            "nombre para marcar archivos uno a uno (pasar el ratón = vista previa "
            "animada). Una fila naranja indica una selección parcial (solo algunos "
            "archivos); el nombre de la carpeta mostrada actualmente está "
            "subrayado. «Añadir una carpeta del PC...» importa una o varias "
            "carpetas desde el ordenador (selección múltiple). «Eliminar» borra "
            "las carpetas/archivos marcados (destructivo, se pide confirmación). "
            "«Construir playlist» guarda la selección con el nombre elegido. El "
            "botón naranja «Regenerar caché de playlist» regenera "
            "cache_master_gifs.dat, un resumen de todos los GIFs de la tarjeta "
            "(uso interno, invisible para el firmware)."
        ),
        "playlist_explanation_text_mode1_add": (
            "Paso del Modo 1 — añadir GIFs personalizados: haz clic en «Añadir "
            "una carpeta del PC...» (varias carpetas posibles, una a una) — las "
            "carpetas añadidas aparecen «pendientes», desmarca los archivos que "
            "no quieras conservar y luego haz clic en «Copiar selección» para "
            "copiarlos de verdad. Cuando esté listo, haz clic en el botón "
            "naranja parpadeante «Continuar» para reanudar el Modo 1."
        ),
        "playlist_explanation_text_mode1_playlist": (
            "Paso del Modo 1 — construir una playlist: marca las "
            "carpetas/archivos a incluir, nombra la playlist y haz clic en "
            "«Construir playlist». Cuando esté lista, haz clic en el botón "
            "naranja parpadeante «Continuar» para reanudar el Modo 1 con esta "
            "playlist."
        ),
        "playlist_name_label": "Nombre de la playlist (existente o nueva)",
        "playlist_delete_btn": "Eliminar",
        "playlist_folders_title": "Carpetas (gifs/)",
        "playlist_select_all_btn": "Todo",
        "playlist_select_none_btn": "Nada",
        "playlist_add_external_folder_btn": "Añadir una carpeta del PC...",
        "playlist_delete_gif_btn": "Eliminar",
        "playlist_files_title": "Archivos (marca los que incluir)",
        "playlist_build_btn": "Construir playlist",
        "playlist_copy_pending_btn": "Copiar selección",
        "playlist_copy_pending_nothing_msg": "Marca al menos un archivo para copiar.",
        "playlist_regen_cache_btn": "Regenerar caché\nde playlist",
        "playlist_temp_note_add": lambda tmp_dir: (
            "Modo temporal (Modo 1) — añadir GIFs: haz clic en «Añadir una "
            "carpeta del PC...», desmarca los archivos que no quieras "
            "conservar y luego «Copiar selección». Después haz clic en "
            "«Continuar» (botón naranja parpadeante) para reanudar."
        ),
        "playlist_temp_note_playlist": lambda tmp_dir: (
            "Modo temporal (Modo 1) — construir playlist: selecciona tus "
            "carpetas/archivos, nombra la playlist y luego «Construir "
            "playlist». Después haz clic en «Continuar» (botón naranja "
            "parpadeante) para reanudar el Modo 1."
        ),
        "playlist_temp_popup_title": "GIFs personalizados",
        "playlist_temp_popup_msg": lambda tmp_dir: (
            "Haz clic en «Añadir una carpeta del PC...» abajo para elegir una o "
            "varias carpetas de GIFs desde tu ordenador.\n\n"
            "Cuando termines, haz clic en «Regenerar caché de playlist» (botón "
            "naranja parpadeante) para continuar: el Modo 1 se reanudará "
            "automáticamente."
        ),
        "playlist_pending_not_copied_title": "Carpetas no copiadas",
        "playlist_pending_not_copied_msg": lambda n, names: (
            f"{n} carpeta(s) añadida(s) ({names}) aún no se han copiado — se "
            "ignorarán si continúas.\n\n"
            "¿Continuar de todos modos?"
        ),
        "playlist_no_sd_msg": "Elige primero una tarjeta SD.",
        "playlist_no_gif_folders_found_msg": "No se encontró ninguna carpeta con archivos .gif en esta carpeta (ni en sus subcarpetas).",
        "playlist_multi_folder_dialog_title": "Añadir carpetas del PC",
        "playlist_subfolder_checklist_prompt": (
            "Marca las subcarpetas de «{root}» a importar:"
        ),
        "playlist_import_dialog_title": "Añadir una carpeta del PC",
        "playlist_import_dialog_prompt": "Nombre de la carpeta de destino (dentro de gifs/):",
        "playlist_import_done_msg_multi": (
            "{n} carpeta(s) importada(s): {names}\n\n"
            "{copied} archivo(s) copiado(s) en total, {skipped} idéntico(s) omitido(s), "
            "{renamed} renombrado(s) (conflicto de nombre).\n{n_updated} playlist(s) actualizada(s) automáticamente."
        ),
        "playlist_import_done_msg": (
            "{copied} archivo(s) copiado(s), {skipped} idéntico(s) omitido(s), "
            "{renamed} renombrado(s) (conflicto de nombre).\n{n_updated} playlist(s) actualizada(s) automáticamente."
        ),
        "playlist_delete_gif_nothing_msg": "Marca al menos una carpeta o archivo para eliminar.",
        "playlist_delete_gif_confirm_title_sd": "Confirmar eliminación",
        "playlist_delete_gif_confirm_msg_sd": (
            "Esto elimina permanentemente las carpetas/archivos marcados de la tarjeta SD, "
            "y los retira de las playlists que los referenciaban.\n\n¿Continuar?"
        ),
        "playlist_delete_gif_confirm_title_temp": "Quitar de la carpeta temporal",
        "playlist_delete_gif_confirm_msg_temp": (
            "Esto elimina las carpetas/archivos marcados de la carpeta temporal "
            "local (sin impacto en tu tarjeta SD -- estos archivos aún no se han "
            "copiado allí).\n\n¿Continuar?"
        ),
        "playlist_name_required_msg": "Indica un nombre de playlist.",
        "playlist_build_empty_msg": "Marca al menos una carpeta o archivo.",
        "playlist_build_done_msg": "Playlist guardada: {n} entrada(s).",
        "playlist_delete_confirm_title": "Eliminar playlist",
        "playlist_delete_confirm_msg": "¿Eliminar permanentemente la playlist «{name}»?",
        "playlist_regen_done_msg": "Caché regenerada: {nf} carpeta(s), {ne} archivo(s).",
        "msg_error_title": "Error",
        "msg_warning_title": "Atención",
        "mode1_rb_confirm_title": "Recalbox detectada",
        "mode1_rb_confirm_msg": lambda target: (
            f"Recalbox detectada: {target}\n\n"
            "Si hay varias Recalbox encendidas en la red, comprueba que sea "
            "la correcta antes de continuar.\n\n"
            "¿Instalar los scripts en este destino ahora?"
        ),
        "mode1_rb_declined_title": "Instalación de scripts cancelada",
        "mode1_rb_declined_msg": (
            "Los scripts de Recalbox (marquee) no se instalarán ahora.\n\n"
            "Usa el Modo 9 (pestaña Avanzado) para instalarlos más tarde, "
            "indicando el destino correcto."
        ),
        "mode1_manual_ip_title": "Dirección de la Recalbox",
        "mode1_manual_ip_prompt": (
            "Introduce la IP o el nombre de red de tu Recalbox:"
        ),
        "mode1_manual_ip_ok": "OK",
        "mode1_manual_ip_cancel": "Cancelar",
        "mode1_rb_unreachable_title": "Recalbox inalcanzable",
        "mode1_rb_unreachable_msg": lambda ip, staged_dir: (
            f"Recalbox inalcanzable ({ip}) — comprueba que esté encendida "
            "y alcanzable en la red, comprueba su IP.\n\n"
            "¿Quieres volver a introducir la IP manualmente, o pasar al "
            "Modo 9 (pestaña Avanzado) para instalar los scripts más "
            "tarde?\n\n"
            "Atención: sin estos scripts, el DMD no estará vinculado a la "
            "Recalbox y solo funcionará en modo playlist/reloj.\n\n"
            f"También puedes copiar tú mismo estos scripts desde "
            f"«{staged_dir}» a la carpeta «share/userscripts» de tu "
            "Recalbox al final del Modo 1."
        ),
        "mode1_rb_retry_ip_btn": "Reintroducir IP",
        "mode1_rb_use_mode9_btn": "Modo 9 más tarde",
        "mode1_rb_reminder_title": "Recordatorio: scripts de Recalbox no instalados",
        "mode1_rb_reminder_msg": lambda ip, staged_dir: (
            f"Los scripts de Recalbox no se instalaron automáticamente "
            f"({ip} inalcanzable o no confirmada).\n\n"
            "Pasa al Modo 9 (pestaña Avanzado) para instalarlos más "
            "tarde.\n\n"
            "Atención: sin estos scripts, el DMD no estará vinculado a la "
            "Recalbox y solo funcionará en modo playlist/reloj.\n\n"
            f"También puedes copiar tú mismo estos scripts desde "
            f"«{staged_dir}» a la carpeta «share/userscripts» de tu "
            "Recalbox."
        ),
        "mode1_fallback_image_title": "Imagen de respaldo",
        "mode1_fallback_image_msg": (
            "¿Quieres elegir una imagen de respaldo personalizada ahora?\n\n"
            "Si respondes que no, se usará el visual por defecto del "
            "proyecto — podrás cambiar esta elección más tarde desde la "
            "pestaña Avanzado."
        ),
        "sdcard_dialog_title": "Tarjeta SD",
        "sdcard_dialog_prompt": lambda min_gb: (
            f"Inserta una tarjeta SD de al menos {min_gb:.0f} GB, formateada "
            "en FAT32, y selecciónala abajo.\n\n"
            "Usa «Actualizar» si no aparece de inmediato."
        ),
        "sdcard_dialog_refresh_btn": "🔄 Actualizar",
        "sdcard_dialog_ok_btn": "OK",
        "sdcard_dialog_cancel_btn": "Cancelar",
        "sdcard_dialog_none_selected": "Selecciona primero una unidad de la lista.",
        "sdcard_dialog_err_notfound": "Esta unidad ya no está disponible — actualiza la lista.",
        "sdcard_dialog_err_fs": lambda fs: (
            f"Sistema de archivos no compatible ({fs}) — la tarjeta debe estar formateada en FAT32."
        ),
        "sdcard_dialog_err_size": lambda size_gb, min_gb: (
            f"Tarjeta demasiado pequeña ({size_gb} GB) — se requieren {min_gb:.0f} GB como mínimo."
        ),
        "gifpack_q_title": "Pack gratuito de GIFs",
        "gifpack_q_msg": (
            "¿Descargar el pack gratuito de 600 GIFs (temas variados) "
            "desde GitHub?\n\n"
            "También podrás añadir tus propios GIFs en el siguiente paso.\n\n"
            "Para un pack mucho más completo (pack ultimate, ~11000 "
            "animaciones pixel-perfect para DMD), consulte "
            "https://rpiteam.carrd.co/ y el foro Arcadia: "
            "https://www.neo-arcadia.com/forum/viewtopic.php?t=67065"
        ),
        "gifpack_q_yes": "Sí",
        "gifpack_q_no": "No",
        "lang_images_title": "Idioma de las imágenes de sistemas",
        "lang_images_msg": (
            "¿En qué idioma quieres las imágenes de sistemas/géneros "
            "(_defaults/)? Los géneros aún no traducidos al idioma elegido "
            "se muestran en inglés.\n\n"
            "Vista previa (ejemplo abajo):"
        ),
        "lang_images_en": "🇬🇧 Inglés (EN)",
        "lang_images_fr": "🇫🇷 Francés (FR)",
        "lang_images_es": "🇪🇸 Español (ES)",
        "customgifs_q_title": "GIFs personalizados",
        "customgifs_q_msg": (
            "¿Quieres añadir tus propios GIFs (pestaña Playlist) antes de "
            "continuar?"
        ),
        "customgifs_q_yes": "Sí",
        "customgifs_q_later": "Más tarde",
        "mode1_build_playlist_q_title": "¿Crear una playlist?",
        "mode1_build_playlist_q_msg": (
            "¿Quieres crear una o varias playlists ahora a partir de tus "
            "GIFs?\n\n"
            "Opcional: de todos modos se seleccionará una playlist por "
            "defecto automáticamente (el pack descargado y/o ALL.txt) — "
            "siempre podrás crear una más tarde."
        ),
        "playlist_temp_continue_btn": "Continuar",
        "btn_close": "Cerrar",
        "explore_sd_btn": "📂 Explorar la tarjeta SD",
        "explore_temp_btn": "📁 Explorar la carpeta temporal",
        "help_open_browser_btn": "Abrir en el navegador",
        "logs_level_label": "Nivel:",
        "logs_level_all": "Todo",
        "logs_level_warn_err": "Alertas+Errores",
        "logs_level_err": "Errores",
        "nas_creds_title": "Credenciales NAS (UNC)",
        "nas_user_label": "Usuario NAS",
        "nas_password_label": "Contraseña NAS",
        "roms_root_missing_msg": "Elija primero una carpeta ROMs.",
        "roms_root_notfound_msg": "Carpeta ROMs no encontrada.",
        "processing_in_progress_msg": "Ya hay un procesamiento en curso.",
        "no_systems_detected_msg": "Ningún sistema detectado (carpetas de imágenes no encontradas).",
        "unc_access_error_msg": (
            "Acceso de red imposible (lectura de gamelist.xml o imagenes). "
            "Introduzca el usuario/contraseña NAS y luego haga clic en "
            "« Detectar sistemas »."
        ),
        "what_to_do_next": "¿Qué desea hacer?",
        "mode1_next_steps_title": "Próximos pasos",
        "mode1_next_steps_msg": (
            "1. Inserta la tarjeta SD en el DMD.\n\n"
            "2. Enciéndelo (conéctalo a una fuente de alimentación).\n\n"
            "3. El DMD pasará automáticamente al modo de configuración, en 2 fases:\n"
            "   • Primero para configurar tu WiFi, desde tu smartphone.\n"
            "   • Luego para configurar el propio DMD (creación de playlists, "
            "elección de la playlist de inicio, etc.), desde un navegador web."
        ),
        "processing_done_title": "✅ ¡Terminado!",
        "existing_files_sd_title": "Archivos existentes en la SD",
        "existing_files_sd_msg": (
            "Ya existen archivos en {letter}:\\\n\n"
            "Haga clic en Sí para SOBRESCRIBIR todos los archivos existentes.\n"
            "Haga clic en No para OMITIR y conservar los archivos actuales."
        ),
        "mode7_pick_btn": 'Elegir la carpeta "systems" que contiene "_defaults"',
        "default_image_btn": "Elegir su imagen de respaldo",
        "default_image_dialog_title": "Elegir la imagen de respaldo",
        "default_image_dialog_intro": (
            "Esta imagen se muestra en el panel DMD cuando no se encuentra "
            "ningun visual especifico para un juego o sistema. Elija una "
            "propuesta a continuacion, o importe su propia imagen (se "
            "redimensionara/adaptara automaticamente al formato 128x32)."
        ),
        "default_image_reset_label": "Imagen predeterminada del proyecto",
        "default_image_import_btn": "Importar mi imagen...",
        "default_image_choose_btn": "Elegir",
        "default_image_applied_now_msg": (
            "Imagen de respaldo aplicada: {name}\n\n"
            "Ya actualizada en la carpeta de trabajo actual."
        ),
        "default_image_apply_failed_msg": (
            "Error al aplicar la imagen de respaldo en la carpeta de "
            "trabajo: {name}\n\n"
            "Revise la pestana Registros."
        ),
        "default_image_reset_applied_msg": (
            "Vuelto a la imagen predeterminada del proyecto.\n\n"
            "Ya actualizada en la carpeta de trabajo actual."
        ),
        "mode2_overwrite_title": "Archivos _defaults ya presentes",
        "mode2_overwrite_msg": (
            "Ya existen archivos en la carpeta _defaults.\n\n"
            "Haga clic en Sí para SOBRESCRIBIRLOS (obtener las últimas "
            "versiones desde GitHub).\n"
            "Haga clic en No para CONSERVAR los archivos actuales.\n\n"
            "En ambos casos, la imagen de respaldo (default.raw565) se "
            "actualizará según su elección en «Imagen de respaldo»."
        ),
        "mode8_btn_compare_final": "Detectar imagenes faltantes en la SD",
        "mode8_btn_compare_running": "Comparando...",
        "mode8_compare_ask_sd_title": "Comparar con la tarjeta SD fisica?",
        "mode8_compare_ask_sd": (
            "Comparar tambien con una tarjeta SD fisica (ademas de la "
            "carpeta temporal)?\n\nSi: elegir la unidad de la tarjeta SD.\n"
            "No: comparar solo con la carpeta temporal."
        ),
        "mode8_open_final_report": "Abrir informe final",
        "mode8_final_done": "Comparacion final completada.",
        "mode2_short_title": "MODO 2 — _defaults (GitHub)",
        "mode3_short_title": "MODO 3 — Extracción (gamelist)",
        "mode4_short_title": "MODO 4 — PNG/GIF → raw565",
        "mode5_short_title": "MODO 5 — 128x32",
        "mode6_short_title": "MODO 6 — games_cache.bin",
        "mode7_short_title": "MODO 7 — systems_cache.dat",
        "mode8_short_title": "MODO 8 — Imágenes faltantes",
        "mode9_short_title": "MODO 9 — Instalar scripts de Recalbox",
        "mode10_short_title": "MODO 10 — Imagen de respaldo",
        "mode11_short_title": "MODO 11 — Pack 600 GIFs (GitHub)",
        "accordion_cat_github": "DOWNLOAD FROM GITHUB",
        "accordion_cat_gamelist": "GAMELIST.XML",
        "accordion_cat_images": "IMAGES TOOLS",
        "accordion_cat_caches": "CACHES",
        "accordion_cat_scripts": "SCRIPTS RECALBOX",
    },
}


class QueueWriter:
    def __init__(self, q: "queue.Queue[str]"):
        self._q = q
        self._buf = ""

    def write(self, s: str) -> None:
        if not s:
            return
        self._buf += s
        while "\n" in self._buf:
            line, self._buf = self._buf.split("\n", 1)
            self._q.put(line + "\n")

    def flush(self) -> None:
        if self._buf:
            self._q.put(self._buf)
            self._buf = ""


def _unc_root(unc_path: str) -> str:
    p = unc_path.strip()
    if not p.startswith("\\\\"):
        return p
    parts = p.split("\\")
    if len(parts) < 4:
        return p
    return "\\\\" + parts[2] + "\\" + parts[3]


def _net_use_connect(unc_root: str, username: str, password: str) -> None:
    cmd = ["net", "use", unc_root, f"/user:{username}", password, "/persistent:no"]
    subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def _net_use_disconnect(unc_root: str) -> None:
    cmd = ["net", "use", unc_root, "/delete", "/y"]
    subprocess.call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


class RetroBoxLEDGui:
    # Nom de fichier reserve pour le visuel par defaut du projet
    # (tools/assets/default_images/default_RB.png). Depuis le retrait de
    # default.raw565 du depot GitHub (pour eviter les ecrasements), c'est ce
    # fichier bundle avec l'outil qui fait office de "visuel par defaut du
    # projet" -- ni propose comme choix normal de la galerie, ni ecrasable
    # par un import personnalise (voir _on_default_image_picker_clicked),
    # utilise en dernier recours par _apply_custom_default_fallback().
    PROJECT_DEFAULT_IMAGE_FILENAME = "default_RB.png"

    def __init__(self, toolkit_module, sd_dir: Path):
        self.tkmod = toolkit_module
        self.sd_dir = sd_dir
        # Dossier final choisi par l'utilisateur (copie depuis sd_dir/tems de travail).
        self._final_output_dir: Optional[Path] = None

        self.root = tk.Tk()
        self.root.title("RecalBoxDMD Toolkit - GUI")
        self.root.configure(bg="#F3F3F3")

        self.style = ttk.Style(self.root)
        try:
            self.style.theme_use("clam")
        except Exception:
            pass

        self._log_q: "queue.Queue[str]" = queue.Queue()
        # Redirection PERMANENTE et GLOBALE de stdout/stderr vers ce meme
        # log (2026-08-05, bug signale par l'utilisateur : progression de
        # copie/extraction visible dans un terminal externe, "c'est pas
        # voulu"). Jusqu'ici, chaque worker (~12 sites) faisait lui-meme un
        # swap temporaire sys.stdout=QueueWriter(...)/restore en fin de
        # fonction -- fragile par construction : tout nouveau code qui
        # print() en dehors de ce motif (ou tout thread annexe demarre sans
        # passer par ce swap) fuit directement vers la console attachee au
        # process. En dev (lance via python.exe depuis un terminal), ce
        # terminal existe deja ; sur l'executable PyInstaller,
        # RecalBoxDMD_GUI.spec avait console=True (corrige en meme temps,
        # console=False) -- dans les deux cas, un vrai terminal recevait
        # cette sortie au lieu du panneau Logs de l'appli. Redirection ici,
        # une seule fois, avant la creation du moindre thread de travail :
        # rend tous les swaps locaux existants inoffensifs (ils permutent
        # desormais entre deux QueueWriter differents pointant vers la MEME
        # file, sans jamais revenir a un vrai flux console) et couvre aussi
        # tout code futur qui oublierait ce motif.
        sys.stdout = QueueWriter(self._log_q)  # type: ignore[assignment]
        sys.stderr = QueueWriter(self._log_q)  # type: ignore[assignment]
        self._worker: Optional[threading.Thread] = None
        # Suivi generique des threads d'arriere-plan secondaires
        # (comparaison finale Mode 8, retry flash Mode 6, nettoyage avant
        # scrape Mode 1...), en plus de self._worker (pipeline principal)
        # et self._mode6_flash_thread (deja suivis individuellement) --
        # voir _start_worker()/_is_processing().
        self._active_workers: set[threading.Thread] = set()
        # Onglet actif memorise au tout premier _start_worker() (transition
        # idle -> occupe), pour y revenir automatiquement une fois
        # _is_processing() redevenu faux -- voir _poll_processing_done().
        self._origin_tab_idx: Optional[int] = None
        self._sys_list_adjusting = False
        self._last_real_system_indices: set[int] = set()
        self._last_sys_click_index: Optional[int] = None
        # index réel le plus proche cliqué dans la Listbox (0..n-1), y compris index système >=3
        self._last_sys_clicked_index_any: Optional[int] = None

        # Charger la langue préférée depuis le fichier JSON centralisé
        _saved_lang = prefs.get("language")
        if _saved_lang not in ("fr", "en", "es"):
            _saved_lang = "en"

        self.lang_var = tk.StringVar(value=_saved_lang)

        # ── Mode 6 (flash) : clignotement + UI lecteurs (sans saisie clavier)
        self._mode6_blinking = False
        self._mode6_blink_job = None
        self._mode6_drives = []
        self._mode6_selected_drive = None
        self._mode6_overwrite = True
        self._mode6_ui_frame = None
        self._mode6_drive_list = None
        self._mode6_btn = None
        # Instance dupliquee (onglet Avance), construite plus tard par
        # _build_mode_area_advanced() -- voir _mode6_instances().
        self._mode6_ui_frame_adv = None
        self._mode6_drive_list_adv = None
        self._mode6_btn_adv = None
        self._mode6_explore_output_btn_adv = None
        self._mode6_panel_title_lbl_adv = None
        self._mode6_drives_title_lbl_adv = None
        self._mode6_active_drive_list = None
        self._advanced_tab_first_visit = True
        self._mode6_flash_thread: Optional[threading.Thread] = None
        # Case a cocher de la popup de sortie ("Quitter") : si True, le
        # dossier de sortie temporaire (sd_dir) n'est PAS supprime a la
        # fermeture. Memorise entre deux ouvertures de la popup dans la
        # meme session (l'utilisateur ne doit pas la re-cocher a chaque fois
        # s'il annule puis rouvre la popup).
        self._quit_keep_temp_dir = False

        self._theme_var = tk.StringVar(value="")
        # Label "Aléatoire" localisé
        lang = self.lang_var.get()
        if lang == "en":
            random_lbl = "Random"
        elif lang == "es":
            random_lbl = "Aleatorio"
        else:
            random_lbl = "Aléatoire"

        pref = themes.load_preference()
        if pref and pref != "random":
            self._chosen_theme_at_start = pref
            self._theme_var.set(pref)
        else:
            # Mode aleatoire
            self._chosen_theme_at_start = themes.random_theme()
            self._theme_var.set(random_lbl)

        # Mode 1 : version Recalbox (profil logo/image/thumbnail), persistee
        _saved_profile = prefs.get("recalbox_profile")
        self._mode1_profile_var = tk.StringVar(
            value=_saved_profile if _saved_profile in self.tkmod.RECALBOX_PROFILES else "10.x"
        )

        # Seuil flag "L" (build_systems_cache(), onglet Parametres, v45) --
        # persiste dans RecalBoxDMD_prefs.json (v7). Repli 5000 sur toute
        # valeur invalide (fichier prefs corrompu/edite a la main).
        try:
            _saved_threshold = int(prefs.get("slow_threshold") or 5000)
        except (TypeError, ValueError):
            _saved_threshold = 5000
        self._slow_threshold_var = tk.IntVar(value=_saved_threshold)

        # ── Onglet PLAYLIST : etat separe de self.sd_dir (qui designe le
        # dossier de travail LOCAL de ce toolkit, sans rapport avec la
        # carte SD physique choisie ici). Doit etre initialise AVANT
        # _build_top_tabs() (qui construit l'onglet Playlist et reference
        # ces attributs).
        self._playlist_sd_root: Optional[Path] = None
        self._playlist_drives: list = []
        self._playlist_name_var = tk.StringVar(value="")
        # True tant que le texte de _playlist_name_var est notre propre
        # suggestion automatique (nom du dossier unique coche), jamais une
        # saisie manuelle -- voir _playlist_maybe_autofill_name() et
        # _on_playlist_name_field_clicked() (efface uniquement dans ce cas).
        self._playlist_name_autofilled: bool = False
        # cle = nom_dossier -> tk.IntVar tristate : 0 = aucun fichier coche,
        # 1 = "dossier entier" (onvalue, tous les fichiers), 2 = selection
        # partielle (tristatevalue, case grisee/mixte). Recalcule a partir
        # de _playlist_checked par _playlist_recompute_folder_state() a
        # chaque changement d'un fichier individuel ; source de verite
        # inverse (dossier -> fichiers) quand l'utilisateur clique
        # directement la case dossier (cf _on_playlist_folder_check_toggled).
        self._playlist_folder_checked: dict = {}
        # cle = nom_dossier -> (row, cb, lbl) widgets reellement affiches
        # (quand ce dossier est liste), pour pouvoir teinter la ligne
        # entiere en place (selection partielle = fond orange clair) sans
        # reconstruire toute la liste, des qu'un fichier individuel est
        # (de)coche -- voir _playlist_update_folder_checkbox_visual().
        # Teinte la ligne entiere plutot que le seul indicateur de case a
        # cocher : sur Windows, le theme natif de tk.Checkbutton ignore
        # silencieusement l'option selectcolor pour la valeur tristate
        # (verifie empiriquement), un simple changement de couleur de case
        # ne serait donc pas visible.
        self._playlist_folder_checkbox_widgets: dict = {}
        # cle = (nom_dossier, nom_fichier) -> tk.BooleanVar (fichier coche
        # individuellement) ; persiste tant que l'app tourne (jamais
        # reinitialise par un simple changement d'onglet), pour pouvoir
        # melanger des fichiers coches dans plusieurs dossiers visites
        # successivement (selection personnalisee cumulative) et retrouver
        # un dossier dans l'etat ou il a ete quitte.
        self._playlist_checked: dict = {}
        self._playlist_folder_counts: dict = {}
        self._playlist_browsed_folder: Optional[str] = None
        self._playlist_gif_preview_win = None
        self._playlist_gif_preview_frames: list = []
        self._playlist_gif_preview_job = None
        self._playlist_gif_preview_idx = 0
        self._playlist_regen_blink_job = None
        self._playlist_regen_blink_on = False
        # Cache memoire (duree de vie de l'appli) des listes dossiers/fichiers
        # de l'onglet Playlist -- evite de re-scanner physiquement la carte
        # SD (lent sur lecteur USB) a chaque re-affichage d'un dossier deja
        # vu ; invalide explicitement apres toute mutation reelle (import,
        # suppression, telechargement du pack Mode 1) via
        # _playlist_invalidate_cache(). Cle dossiers = str(sd_root) ; cle
        # fichiers = (str(sd_root), nom_dossier).
        self._playlist_folders_cache: dict = {}
        self._playlist_files_cache: dict = {}
        # Dossiers PC "en attente" (sous-mode "add" du mode temporaire
        # Mode1) : ajoutes a la liste Dossiers/Fichiers pour revue
        # (deselection de fichiers individuels) mais PAS ENCORE copies
        # sur disque -- cle = nom affiche -> {"src": Path source PC,
        # "files": [noms .gif]}. Vide/copie via "Copier la selection"
        # (_on_playlist_copy_pending_clicked). Voir
        # _playlist_list_folders_cached()/_playlist_list_files_cached()
        # (fusionnent ces entrees avec les dossiers reels) et
        # _playlist_refresh_temp_submode_ui().
        self._playlist_pending_folders: dict = {}

        # ── Pre-vol Mode 1 : banque de GIFs (pack GitHub + GIFs perso via
        # l'onglet Playlist). _mode1_verified_sd_letter sert a pre-
        # selectionner le lecteur dans _refresh_mode6_drives() une fois le
        # pipeline termine.
        self._mode1_verified_sd_letter: Optional[str] = None
        self._mode1_download_pack: bool = False
        # Thread du telechargement du pack GIFs demarre en arriere-plan
        # des la reponse "oui" a la question (voir
        # _start_gifpack_background_download()) -- _pipeline_mode_1()
        # l'attend (join) au lieu de retelecharger. None tant qu'aucun
        # telechargement n'a ete lance cette session.
        self._mode1_gifpack_bg_thread: Optional[threading.Thread] = None
        self._mode1_add_custom_gifs: bool = False
        self._mode1_deferred_launch_cb: Optional[Callable[[], None]] = None
        self._playlist_temp_mode: bool = False
        # Sous-mode du mode temporaire Mode1 : "add" (ajout de dossiers PC
        # "en attente" de copie, etat initial) ou "playlist" (construction
        # effective d'une playlist, apres reponse "oui" a la proposition
        # -- voir _on_playlist_regen_cache_clicked). Pilote l'affichage du
        # champ nom de playlist, le texte du mode temporaire, le cadre
        # d'explication et le bouton "Construire la playlist"/"Copier la
        # selection" -- voir _playlist_refresh_temp_submode_ui().
        self._playlist_temp_submode: str = "add"
        # Proposition (facultative, posee UNE SEULE FOIS par passage en
        # mode temporaire) de construire une playlist maintenant a partir
        # des GIFs ajoutes -- voir _on_playlist_regen_cache_clicked().
        # Reinitialisee a chaque entree en mode temporaire
        # (_enter_playlist_temp_mode).
        self._mode1_playlist_proposal_answered: bool = False

        self._build_top_tabs()
        self._build_mode_area(self.tab_main)  # Mode 1 seulement
        self._build_mode_area_advanced(self.tab_advanced)  # Modes 2-7
        self._build_progress_frame(self.root)

        self._poll_logs()

        # Forcer rebuild affichage logs avec le filtre par défaut (Alertes+Erreurs)
        self.root.after(50, lambda: self._rebuild_log_display(go_end=True))

        # Appliquer le thème APRÈS la construction de tous les widgets
        # Un premier appel maintenant, un second apres idle pour les onglets pas encore realises
        _th = self._chosen_theme_at_start
        themes.apply(_th, self)
        self.root.update_idletasks()
        self.root.after(200, lambda t=_th: themes.apply(t, self))
        del self._chosen_theme_at_start

        # Sérigraphie (bas à droite)
        self.silk_label = tk.Label(
            self.root,
            text="GUI - Shan_ayA 2026",
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        self.silk_label.place(relx=1.0, rely=1.0, anchor="se", x=-102, y=-10)

        # Reslice l'image de fond quand l'utilisateur change d'onglet
        # (les onglets non-visibles n'ont pas encore leurs vraies dimensions)
        self.nb_top.bind("<<NotebookTabChanged>>", self._on_tab_changed)

        # Mémorise le dernier mode choisi dans l'onglet Avancé : la bascule
        # automatique sur le mode 2 ne doit avoir lieu qu'à la toute première
        # ouverture de cet onglet, pas à chaque changement d'onglet.
        self._adv_tab_first_open = True
        self._last_adv_mode = "2"
        self._prev_tab_idx = 0

        self.root.protocol("WM_DELETE_WINDOW", self._on_close_attempt)

        # Bug 2 : taille minimale fixe pour que le changement de langue
        # ne redimensionne pas la fenêtre.
        # Reste a 750 (pas d'agrandissement) : les images de fond par theme
        # (tools/themes/<nom>/bg.png) sont etirees dynamiquement a la taille
        # de la fenetre (themes.apply()), donc agrandir la fenetre les
        # deformerait visuellement (aspect ratio non preserve) pour TOUS
        # les themes -- il faudrait regenerer chaque bg.png. Le contenu de
        # l'onglet Main est plutot resserre pour tenir dans 750px (voir
        # _build_mode6_panel : listbox lecteurs reduite, paddings reduits).
        self.root.minsize(1100, 750)
        self.root.geometry("1100x750")
        # Interdire le redimensionnement en plein écran / maximisé.
        self.root.resizable(False, False)
        self.root.grid_propagate(True)
        self.root.pack_propagate(True)

        # Bug trouve en test reel (2026-07-21) : _on_language_changed() n'est
        # declenchee QUE par le menu deroulant (command=), jamais appelee au
        # demarrage -- self.lang_var affichait deja la bonne langue
        # sauvegardee (_saved_lang), mais tous les libelles qu'elle rafraichit
        # (nom "Mode 1", bouton "Quitter", etc.) restaient sur leur valeur de
        # construction initiale (francais, tkmod.T par defaut) tant que
        # l'utilisateur n'avait pas re-touche le selecteur. Fix : appliquer
        # explicitement la langue sauvegardee une fois tous les widgets construits.
        self._on_language_changed()

    # ──────────────────────────────────────────────────────────────────────────
    # TOP TABS (compact logs + parameters)
    # ──────────────────────────────────────────────────────────────────────────
    def _build_top_tabs(self) -> None:
        ui = self._get_ui_t()
        self.nb_top = ttk.Notebook(self.root)
        self.nb_top.pack(fill="both", expand=True, padx=10, pady=(8, 2))

        self.tab_main = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_playlist = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_advanced = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_logs = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_params = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_help = tk.Frame(self.nb_top, bg="#F3F3F3")

        # "Main"/"Logs" : identiques dans les 3 langues, pas besoin de cle.
        # Ordre : Main=0, Playlist=1, Avance=2, Logs=3, Parametres=4, Aide=5
        # (voir _LOGS_TAB_INDEX et _on_tab_changed).
        self.nb_top.add(self.tab_main, text="Main")
        self.nb_top.add(self.tab_playlist, text=ui["tab_playlist"])
        self.nb_top.add(self.tab_advanced, text=ui["tab_advanced"])
        self.nb_top.add(self.tab_logs, text="Logs")
        self.nb_top.add(self.tab_params, text=ui["tab_params"])
        self.nb_top.add(self.tab_help, text=ui["tab_help"])

        self._build_playlist_tab(self.tab_playlist)
        self._build_logs_tab(self.tab_logs)
        self._build_params_tab(self.tab_params)
        self._build_help_tab(self.tab_help)

    # ──────────────────────────────────────────────────────────────────────────
    # ONGLET PLAYLIST -- construction de playlists DMD (gifs/) depuis la
    # carte SD ou un dossier PC externe. Reconstruit le 2026-08-03 (voir
    # changelog RecalBoxDMD_tool.py v29).
    # ──────────────────────────────────────────────────────────────────────────
    def _build_playlist_tab(self, parent: tk.Frame) -> None:
        ui = self._get_ui_t()

        top_row = tk.Frame(parent, bg="#F3F3F3")
        top_row.pack(fill="x", padx=10, pady=(6, 3))

        self._playlist_sd_frame = tk.Frame(top_row, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=4)
        self._playlist_sd_frame.pack(side="left", fill="y")
        self._playlist_sd_title_lbl = tk.Label(
            self._playlist_sd_frame, text=ui["playlist_sd_section_title"], bg="#F3F3F3",
            fg="black", font=("TkDefaultFont", 11, "bold"),
        )
        self._playlist_sd_title_lbl.pack(anchor="w")

        sd_row = tk.Frame(self._playlist_sd_frame, bg="#F3F3F3")
        sd_row.pack(fill="x", pady=(4, 0))
        # width=42 (~x2 de la largeur par defaut de Tkinter, 20 caracteres)
        # -- demande utilisateur : les noms de lecteur longs (lettre +
        # etiquette + taille) etaient tronques dans la Listbox etroite.
        self._playlist_drive_list = tk.Listbox(
            sd_row, selectmode="browse", height=3, width=42, bg="white", fg="black",
            borderwidth=2, relief="solid", exportselection=False,
        )
        self._playlist_drive_list.pack(side="left", fill="both", expand=True)
        sd_scroll = tk.Scrollbar(sd_row, orient="vertical", command=self._playlist_drive_list.yview)
        sd_scroll.pack(side="right", fill="y")
        self._playlist_drive_list.configure(yscrollcommand=sd_scroll.set)
        self._playlist_drive_list.bind("<<ListboxSelect>>", self._on_playlist_drive_selected)

        self._playlist_refresh_drives_btn = tk.Button(
            self._playlist_sd_frame, text=ui["playlist_refresh_drives_btn"], command=self._refresh_playlist_drives,
            bg="#FFFFFF", fg="black", bd=2, relief="solid", padx=8, pady=3,
        )
        self._playlist_refresh_drives_btn.pack(anchor="w", pady=(4, 0))

        # Note du mode temporaire (Mode 1 / banque de GIFs) -- masquee par
        # defaut, affichee uniquement par _enter_playlist_temp_mode().
        # Plus de bouton "Annuler" ici : deplace/fusionne dans la rangee du
        # bas (remplace "Quitter" pendant le mode temporaire, voir
        # _enter_playlist_temp_mode()) pour eviter un 2e bouton redondant.
        self._playlist_temp_note_lbl = tk.Label(
            top_row, text="", bg="#FFF3CD", fg="black", bd=2, relief="solid",
            font=("TkDefaultFont", 8), justify="left", anchor="nw", wraplength=260,
            padx=6, pady=4,
        )

        self._playlist_explanation_frame = tk.Frame(top_row, bg="#F3F3F3", bd=1, relief="solid")
        self._playlist_explanation_frame.pack(side="left", fill="both", expand=True, padx=(8, 0))
        # Texte pose par _playlist_refresh_explanation_text() (2 variantes
        # -- usage direct de l'onglet / etape du Mode 1 -- selon le
        # contexte, cf demande utilisateur "plus de volume d'information").
        self._playlist_explanation_lbl = tk.Label(
            self._playlist_explanation_frame, text="", bg="#F3F3F3",
            fg="black", font=("TkDefaultFont", 8), justify="left", anchor="nw",
            wraplength=560,
        )
        self._playlist_explanation_lbl.pack(fill="both", expand=True, padx=6, pady=4)
        self._playlist_refresh_explanation_text()

        # ── Dossiers (gauche) / Fichiers a cocher (droite) ──
        # Case a cocher "dossier entier" par dossier : le choix
        # complet/personnalise se fait silencieusement par dossier au
        # moment de Construire, selon ce qui est reellement coche.
        body = tk.Frame(parent, bg="#F3F3F3")
        body.pack(fill="both", expand=True, padx=10, pady=3)

        folders_col = tk.Frame(body, bg="#F3F3F3")
        folders_col.pack(side="left", fill="both", expand=True, padx=(0, 6))

        # ── Nom de playlist (existante ou nouvelle) + Supprimer ──
        # Aligne sur folders_col (pas sur toute la largeur du corps,
        # dossiers+fichiers) -- demande utilisateur : laisse au cadre
        # Fichiers, qui n'a rien au-dessus de son propre titre, la place
        # de s'etendre plus haut avec l'espace ainsi libere.
        name_frame = tk.Frame(folders_col, bg="#F3F3F3")
        name_frame.pack(fill="x")
        # Reference gardee pour pouvoir masquer ce cadre en sous-mode
        # "ajout dossier PC" du mode temporaire Mode1 (voir
        # _playlist_refresh_temp_submode_ui()) -- champ sans objet tant
        # qu'on ne nomme pas encore une playlist.
        self._playlist_name_frame = name_frame
        self._playlist_name_lbl = tk.Label(
            name_frame, text=ui["playlist_name_label"], bg="#F3F3F3",
            fg="black", font=("TkDefaultFont", 9, "bold"),
        )
        self._playlist_name_lbl.pack(anchor="w")
        name_row = tk.Frame(name_frame, bg="#F3F3F3")
        name_row.pack(fill="x", pady=(2, 6))
        self._playlist_name_combo = ttk.Combobox(
            name_row, textvariable=self._playlist_name_var, values=["---"],
            state="normal", cursor="xterm",
        )
        self._playlist_delete_btn = tk.Button(
            name_row, text=ui["playlist_delete_btn"], command=self._on_playlist_delete_clicked,
            bg="#FF5C5C", fg="black", bd=2, relief="solid", padx=8, pady=3,
        )
        self._playlist_delete_btn.pack(side="right")
        self._playlist_name_combo.pack(side="left", fill="x", expand=True, padx=(0, 6))
        self._playlist_name_combo.bind("<<ComboboxSelected>>", self._on_playlist_selected)
        # Un clic dans le champ efface la suggestion automatique (nom de
        # dossier pre-rempli) -- voir _on_playlist_name_field_clicked().
        self._playlist_name_combo.bind("<Button-1>", self._on_playlist_name_field_clicked)

        folders_title_row = tk.Frame(folders_col, bg="#F3F3F3")
        folders_title_row.pack(fill="x")
        self._playlist_folders_title_row = folders_title_row
        self._playlist_folders_title_lbl = tk.Label(
            folders_title_row, text=ui["playlist_folders_title"], bg="#F3F3F3",
            fg="black", font=("TkDefaultFont", 9, "bold"),
        )
        self._playlist_folders_title_lbl.pack(side="left", anchor="w")
        self._playlist_folders_select_none_btn = tk.Button(
            folders_title_row, text=ui["playlist_select_none_btn"], command=self._on_playlist_folders_select_none,
            bg="#FFFFFF", fg="black", bd=1, relief="solid", padx=4, pady=1, font=("TkDefaultFont", 7),
        )
        self._playlist_folders_select_none_btn.pack(side="right")
        self._playlist_folders_select_all_btn = tk.Button(
            folders_title_row, text=ui["playlist_select_all_btn"], command=self._on_playlist_folders_select_all,
            bg="#FFFFFF", fg="black", bd=1, relief="solid", padx=4, pady=1, font=("TkDefaultFont", 7),
        )
        self._playlist_folders_select_all_btn.pack(side="right", padx=(0, 4))

        # Fond des cadres dossiers/fichiers = couleur "surface de contenu"
        # du theme actif (bg_listbox/fg_listbox, deja concue pour rester
        # lisible en toutes circonstances) au lieu d'un blanc fixe --
        # demande utilisateur, attention au ton-sur-ton. Voir
        # _playlist_content_colors()/_playlist_apply_theme_colors().
        content_bg, content_fg = self._playlist_content_colors()
        self._playlist_folder_outer = tk.Frame(folders_col, bg=content_bg, bd=2, relief="solid")
        # Exempte ce cadre (et tout son contenu : Canvas, Frame interne,
        # lignes individuelles) du decoupage decoratif du fond de theme
        # (_slice_widgets_later(), RecalBoxDMD_themes.py) -- sinon un
        # fragment de l'image de fond ecrase silencieusement la couleur
        # pleine ci-dessus a chaque changement d'onglet, rendant le cadre
        # "troue" au lieu d'uniformement colore (retour utilisateur,
        # capture d'ecran).
        self._playlist_folder_outer._no_bg_slice = True
        self._playlist_folder_outer.pack(fill="both", expand=True, pady=(2, 0))
        # Un Listbox ne peut pas heberger de vraies cases a cocher
        # (limitation Tkinter) -> Canvas+Frame scrollable, meme pattern
        # que la checklist fichiers ci-dessous. Hauteur bornee
        # explicitement (budget vertical serre partage par tous les
        # onglets d'un ttk.Notebook a taille fixe -- jamais
        # fill=both/expand=True seul sans hauteur, cf. lecon v44).
        self._playlist_folder_canvas = tk.Canvas(self._playlist_folder_outer, bg=content_bg, height=140, highlightthickness=0)
        folders_scroll = tk.Scrollbar(self._playlist_folder_outer, orient="vertical", command=self._playlist_folder_canvas.yview)
        self._playlist_folder_canvas.configure(yscrollcommand=folders_scroll.set)
        self._playlist_folder_canvas.pack(side="left", fill="both", expand=True)
        folders_scroll.pack(side="right", fill="y")
        self._playlist_folder_inner = tk.Frame(self._playlist_folder_canvas, bg=content_bg)
        _folder_inner_win = self._playlist_folder_canvas.create_window((0, 0), window=self._playlist_folder_inner, anchor="nw")
        self._playlist_folder_inner.bind(
            "<Configure>",
            lambda e: self._playlist_folder_canvas.configure(scrollregion=self._playlist_folder_canvas.bbox("all")),
        )
        # Sans ceci, la fenetre-Canvas ne prend QUE la largeur naturelle de
        # son contenu (le plus long nom de dossier) -- plus etroite que la
        # largeur reellement visible du cadre des que celui-ci est plus
        # large, laissant une bande a droite non couverte par le fond des
        # lignes (demande utilisateur : "l'espace entre les noms des
        # dossiers doit etre lui aussi colore"). Etire la fenetre-Canvas
        # (donc chaque ligne, packee en fill="x" a l'interieur) sur toute
        # la largeur visible reelle a chaque redimensionnement.
        self._playlist_folder_canvas.bind(
            "<Configure>",
            lambda e: self._playlist_folder_canvas.itemconfig(_folder_inner_win, width=e.width),
        )

        def _folder_wheel(event):
            self._playlist_folder_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        def _folder_wheel_bind(event):
            self._playlist_folder_canvas.bind_all("<MouseWheel>", _folder_wheel)

        def _folder_wheel_unbind(event):
            self._playlist_folder_canvas.unbind_all("<MouseWheel>")

        self._playlist_folder_canvas.bind("<Enter>", _folder_wheel_bind)
        self._playlist_folder_canvas.bind("<Leave>", _folder_wheel_unbind)

        folders_btn_row = tk.Frame(folders_col, bg="#F3F3F3")
        folders_btn_row.pack(fill="x", pady=(4, 0))
        self._playlist_add_external_btn = tk.Button(
            folders_btn_row, text=ui["playlist_add_external_folder_btn"], command=self._on_playlist_add_external_folder_clicked,
            bg="#FFFFFF", fg="black", bd=2, relief="solid", padx=8, pady=3,
        )
        self._playlist_add_external_btn.pack(side="left", fill="x", expand=True)
        self._playlist_delete_gif_btn = tk.Button(
            folders_btn_row, text=ui["playlist_delete_gif_btn"], command=self._on_playlist_delete_gif_clicked,
            bg="#FF5C5C", fg="black", bd=2, relief="solid", padx=8, pady=3,
        )
        self._playlist_delete_gif_btn.pack(side="left", padx=(6, 0))

        files_col = tk.Frame(body, bg="#F3F3F3")
        files_col.pack(side="left", fill="both", expand=True, padx=(6, 0))
        files_title_row = tk.Frame(files_col, bg="#F3F3F3")
        files_title_row.pack(fill="x")
        self._playlist_files_title_lbl = tk.Label(
            files_title_row, text=ui["playlist_files_title"], bg="#F3F3F3",
            fg="black", font=("TkDefaultFont", 9, "bold"),
        )
        self._playlist_files_title_lbl.pack(side="left", anchor="w")
        self._playlist_files_select_none_btn = tk.Button(
            files_title_row, text=ui["playlist_select_none_btn"], command=self._on_playlist_files_select_none,
            bg="#FFFFFF", fg="black", bd=1, relief="solid", padx=4, pady=1, font=("TkDefaultFont", 7),
        )
        self._playlist_files_select_none_btn.pack(side="right")
        self._playlist_files_select_all_btn = tk.Button(
            files_title_row, text=ui["playlist_select_all_btn"], command=self._on_playlist_files_select_all,
            bg="#FFFFFF", fg="black", bd=1, relief="solid", padx=4, pady=1, font=("TkDefaultFont", 7),
        )
        self._playlist_files_select_all_btn.pack(side="right", padx=(0, 4))

        # Rien au-dessus de files_title_row (contrairement a folders_col,
        # qui porte desormais name_frame) -- le cadre Fichiers dispose donc
        # de tout l'espace vertical libere pour s'etendre plus haut que le
        # cadre Dossiers (hauteur ajustee/verifiee par capture d'ecran
        # reelle, boutons du cadre Progression restant visibles).
        self._playlist_files_outer = tk.Frame(files_col, bg=content_bg, bd=2, relief="solid")
        self._playlist_files_outer._no_bg_slice = True  # voir _playlist_folder_outer ci-dessus
        self._playlist_files_outer.pack(fill="both", expand=True, pady=(2, 0))
        self._playlist_checklist_canvas = tk.Canvas(self._playlist_files_outer, bg=content_bg, height=280, highlightthickness=0)
        files_scroll = tk.Scrollbar(self._playlist_files_outer, orient="vertical", command=self._playlist_checklist_canvas.yview)
        self._playlist_checklist_canvas.configure(yscrollcommand=files_scroll.set)
        self._playlist_checklist_canvas.pack(side="left", fill="both", expand=True)
        files_scroll.pack(side="right", fill="y")
        self._playlist_checklist_inner = tk.Frame(self._playlist_checklist_canvas, bg=content_bg)
        _files_inner_win = self._playlist_checklist_canvas.create_window((0, 0), window=self._playlist_checklist_inner, anchor="nw")
        self._playlist_checklist_inner.bind(
            "<Configure>",
            lambda e: self._playlist_checklist_canvas.configure(scrollregion=self._playlist_checklist_canvas.bbox("all")),
        )
        # Meme etirement sur toute la largeur visible que pour le Canvas
        # dossiers ci-dessus (voir son commentaire).
        self._playlist_checklist_canvas.bind(
            "<Configure>",
            lambda e: self._playlist_checklist_canvas.itemconfig(_files_inner_win, width=e.width),
        )

        def _playlist_checklist_wheel(event):
            self._playlist_checklist_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        def _playlist_checklist_bind_wheel(event):
            self._playlist_checklist_canvas.bind_all("<MouseWheel>", _playlist_checklist_wheel)

        def _playlist_checklist_unbind_wheel(event):
            self._playlist_checklist_canvas.unbind_all("<MouseWheel>")
            self._hide_gif_preview()

        self._playlist_checklist_canvas.bind("<Enter>", _playlist_checklist_bind_wheel)
        self._playlist_checklist_canvas.bind("<Leave>", _playlist_checklist_unbind_wheel)

        # ── Rangee du bas : Construire / Regenerer le cache / Quitter ──
        bottom_row = tk.Frame(parent, bg="#F3F3F3")
        bottom_row.pack(fill="x", padx=10, pady=(3, 6))

        self._playlist_build_btn = tk.Button(
            bottom_row, text=ui["playlist_build_btn"], command=self._on_playlist_build_clicked,
            bg="#00D084", fg="black", bd=2, relief="solid", padx=10, pady=4,
            font=("TkDefaultFont", 11, "bold"),
        )
        self._playlist_build_btn.pack(side="left", fill="x", expand=True)
        self._playlist_build_btn._fixed_theme_colors = ("#00D084", "#000000")

        self._playlist_regen_cache_btn = tk.Button(
            bottom_row, text=ui["playlist_regen_cache_btn"].replace("\n", " "), command=self._on_playlist_regen_cache_clicked,
            bg="#FF9800", fg="black", bd=2, relief="solid", padx=10, pady=4,
            font=("TkDefaultFont", 11, "bold"),
        )
        self._playlist_regen_cache_btn.pack(side="left", fill="x", expand=True, padx=(8, 0))
        self._playlist_regen_cache_btn._fixed_theme_colors = ("#FF9800", "#000000")

        self._playlist_quit_btn = tk.Button(
            bottom_row,
            text=(self.tkmod.tr("main_opt_quit") if hasattr(self.tkmod, "tr") else "Quit"),
            command=self._on_quit_app_clicked,
            bg="#FF5C5C", fg="black", bd=2, relief="solid", padx=10, pady=4,
            font=("TkDefaultFont", 11, "bold"),
        )
        self._playlist_quit_btn.pack(side="left", fill="x", expand=True, padx=(8, 0))
        self._playlist_quit_btn._fixed_theme_colors = ("#FF5C5C", "#000000")

    def _enter_playlist_temp_mode(self) -> None:
        """Bascule l'onglet Playlist en mode temporaire : cible le dossier
        de travail LOCAL (self.sd_dir) au lieu d'une carte SD reelle,
        masque le selecteur de lecteur, affiche une note expliquant la
        marche a suivre. Utilise par le pre-vol du Mode 1 (question
        "GIFs perso" = Oui)."""
        ui = self._get_ui_t()
        self._playlist_temp_mode = True
        self._playlist_temp_submode = "add"
        self._playlist_pending_folders = {}
        self._playlist_sd_root = self.sd_dir
        self._playlist_sd_frame.pack_forget()
        self._playlist_temp_note_lbl.pack(side="left", fill="y", before=self._playlist_explanation_frame)
        self._playlist_checked.clear()
        self._playlist_folder_checked.clear()
        self._playlist_browsed_folder = None
        self._clear_playlist_checklist()
        self._refresh_playlist_existing_list()
        self._refresh_playlist_folders()
        # Le bouton du bas reste "Quitter" normal tout du long du mode
        # temporaire (demande utilisateur) : "Annuler l'ajout" n'a plus
        # d'utilite maintenant que rien n'est copie tant que l'utilisateur
        # n'a pas explicitement valide sa selection (voir "Copier la
        # selection", _on_playlist_copy_pending_clicked) -- il suffit de
        # ne pas valider pour que rien ne soit jamais ecrit sur disque.
        #
        # "Regenerer le cache playlist" -> "Continuer" en mode temporaire
        # (demande utilisateur) : ce bouton ne regenere plus reellement le
        # cache ici (redondant, voir _continue_mode1_from_temp_playlist())
        # -- son unique role devient de poursuivre le Mode 1, avec une
        # proposition facultative de construire une playlist au passage
        # (voir _on_playlist_regen_cache_clicked()). Reinitialise a
        # chaque entree en mode temporaire (peut se reproduire plusieurs
        # fois dans la meme session si l'utilisateur relance le Mode 1).
        self._mode1_playlist_proposal_answered = False
        self._playlist_regen_cache_btn.configure(text=ui["playlist_temp_continue_btn"])
        # Applique le texte/l'affichage du sous-mode "add" : note, champ
        # nom masque, cadre d'explication, bouton "Copier la selection"
        # (voir _playlist_refresh_temp_submode_ui). Le clignotement de
        # "Continuer" ne demarre PLUS ici (demande utilisateur) : il
        # n'indique quoi que ce soit tant qu'aucune copie n'a ete
        # reellement validee -- voir _on_playlist_copy_pending_done().
        self._playlist_refresh_temp_submode_ui()
        self.nb_top.select(self.tab_playlist)
        self._themed_info(ui["playlist_temp_popup_title"], ui["playlist_temp_popup_msg"](self.sd_dir))

    def _exit_playlist_temp_mode(self) -> None:
        """Retour au fonctionnement normal de l'onglet Playlist (carte SD
        reelle choisie manuellement)."""
        ui = self._get_ui_t()
        self._playlist_temp_mode = False
        self._playlist_temp_submode = "add"
        self._playlist_pending_folders = {}
        self._playlist_temp_note_lbl.pack_forget()
        # Le champ nom peut avoir ete masque (sous-mode "add") -- toujours
        # visible hors mode temporaire.
        self._playlist_name_frame.pack(fill="x", before=self._playlist_folders_title_row)
        self._playlist_sd_frame.pack(side="left", fill="y", before=self._playlist_explanation_frame)
        self._playlist_refresh_explanation_text()
        self._playlist_build_btn.configure(
            text=ui["playlist_build_btn"], command=self._on_playlist_build_clicked, state="normal",
        )
        self._playlist_regen_cache_btn.configure(text=ui["playlist_regen_cache_btn"].replace("\n", " "))
        self._refresh_playlist_drives()
        self._stop_playlist_regen_cache_blink()

    def _continue_mode1_from_temp_playlist(self) -> None:
        """Poursuit le Mode 1 depuis le mode temporaire de l'onglet
        Playlist (GIFs perso) -- sans regeneration de cache intermediaire
        (redondante : _pipeline_mode_1() en fait de toute facon une
        complete et systematique juste avant la copie SD, couvrant pack +
        GIFs perso + playlists en un seul scan). Seul chemin de sortie du
        mode temporaire desormais ("Continuer", _on_playlist_regen_cache_clicked,
        avec ou sans construction de playlist prealable -- "Annuler
        l'ajout" a ete retire, plus d'utilite : rien n'est jamais copie
        sur disque tant que l'utilisateur n'a pas explicitement valide sa
        selection via "Copier la selection")."""
        self._exit_playlist_temp_mode()
        self.nb_top.select(self.tab_main)
        cb, self._mode1_deferred_launch_cb = self._mode1_deferred_launch_cb, None
        if cb:
            cb()

    def _start_playlist_regen_cache_blink(self) -> None:
        """Fait clignoter le bouton "Regenerer le cache playlist" (orange
        <-> jaune vif) tant que le mode temporaire du Mode 1 est actif --
        c'est l'action que l'utilisateur doit faire pour que le Mode 1
        reprenne automatiquement, facilement manquee sinon au milieu du
        reste de l'onglet."""
        self._stop_playlist_regen_cache_blink()
        self._playlist_regen_blink_on = False

        def _tick():
            if not getattr(self, "_playlist_temp_mode", False):
                self._playlist_regen_blink_job = None
                return
            self._playlist_regen_blink_on = not self._playlist_regen_blink_on
            color = "#FFEB3B" if self._playlist_regen_blink_on else "#FF9800"
            try:
                self._playlist_regen_cache_btn.configure(bg=color)
                self._playlist_regen_cache_btn._fixed_theme_colors = (color, "#000000")
            except tk.TclError:
                self._playlist_regen_blink_job = None
                return
            self._playlist_regen_blink_job = self.root.after(500, _tick)

        self._playlist_regen_blink_job = self.root.after(500, _tick)

    def _stop_playlist_regen_cache_blink(self) -> None:
        job = getattr(self, "_playlist_regen_blink_job", None)
        if job:
            try:
                self.root.after_cancel(job)
            except Exception:
                pass
            self._playlist_regen_blink_job = None
        try:
            self._playlist_regen_cache_btn.configure(bg="#FF9800")
            self._playlist_regen_cache_btn._fixed_theme_colors = ("#FF9800", "#000000")
        except (tk.TclError, AttributeError):
            pass

    def _refresh_playlist_drives(self) -> None:
        # En mode temporaire (banque de GIFs, Mode 1), _playlist_sd_root
        # cible volontairement self.sd_dir (pas une carte reelle) --
        # _on_tab_changed() appelle cette methode a chaque selection de
        # l'onglet Playlist : sans ce garde, le rafraichissement
        # ecraserait silencieusement _playlist_sd_root avec le dernier
        # lecteur reel connu.
        if getattr(self, "_playlist_temp_mode", False):
            return
        try:
            drives = self.tkmod._list_removable_drives()  # type: ignore[attr-defined]
        except Exception:
            drives = []
        self._playlist_drives = list(drives)
        self._playlist_drive_list.delete(0, "end")
        for i, (letter, label, size) in enumerate(self._playlist_drives):
            self._playlist_drive_list.insert("end", f"{i+1} → {letter}\\  [{label}]  {size}")

        if not self._playlist_drives:
            self._playlist_sd_root = None
            return

        sel_idx = 0
        last_drive = prefs.get("playlist_last_drive")
        if last_drive:
            for i, (letter, _label, _size) in enumerate(self._playlist_drives):
                if letter == last_drive:
                    sel_idx = i
                    break
        self._playlist_drive_list.selection_clear(0, "end")
        self._playlist_drive_list.selection_set(sel_idx)
        self._on_playlist_drive_selected()

    def _on_playlist_drive_selected(self, event=None) -> None:
        sel = self._playlist_drive_list.curselection()
        if not sel or sel[0] >= len(self._playlist_drives):
            return
        letter = self._playlist_drives[sel[0]][0]
        new_root = Path(f"{letter}\\")
        prefs.set("playlist_last_drive", letter)
        if self._playlist_sd_root == new_root:
            # Meme lecteur deja actif : _refresh_playlist_drives() ->
            # _on_playlist_drive_selected() est appele a CHAQUE fois que
            # l'onglet Playlist redevient visible (_on_tab_changed), pas
            # seulement lors d'un vrai changement de lecteur. Sans ce
            # garde, revenir sur l'onglet effacait silencieusement toutes
            # les coches en cours et relancait un scan disque complet a
            # chaque fois (lent sur lecteur USB).
            return
        self._playlist_sd_root = new_root
        self._playlist_checked.clear()
        self._playlist_folder_checked.clear()
        self._playlist_browsed_folder = None
        self._playlist_folders_cache.clear()
        self._playlist_files_cache.clear()
        self._clear_playlist_checklist()
        self._refresh_playlist_existing_list()
        self._refresh_playlist_folders()

    def _refresh_playlist_existing_list(self) -> None:
        if self._playlist_sd_root is None:
            self._playlist_name_combo["values"] = ["---"]
            return
        try:
            names = self.tkmod.list_existing_playlists(self._playlist_sd_root)
        except Exception:
            names = []
        self._playlist_name_combo["values"] = ["---"] + names

    def _on_playlist_selected(self, event=None) -> None:
        """Une entree a ete choisie dans le menu deroulant. "---" = aucune
        selection (playlist nouvelle a nommer). Sinon, recharge la
        playlist existante : re-coche les dossiers listes dans le
        marqueur '# FULL:' comme "dossier entier", et les fichiers
        individuels pour le reste (avec recalcul de l'etat partiel/
        tristate des dossiers ainsi touches)."""
        name = self._playlist_name_var.get()
        if name == "---" or not name or self._playlist_sd_root is None:
            return
        # Nom reel choisi dans le menu (pas une suggestion) -- ne doit
        # plus jamais etre efface/ecrase par _playlist_maybe_autofill_name().
        self._playlist_name_autofilled = False
        full_folders = set(self.tkmod.read_playlist_full_folders(self._playlist_sd_root, name))
        entries = self.tkmod.read_playlist(self._playlist_sd_root, name)

        self._playlist_checked.clear()
        for var in self._playlist_folder_checked.values():
            var.set(0)
        for folder in full_folders:
            if folder in self._playlist_folder_checked:
                self._playlist_folder_checked[folder].set(1)

        prefix_len = len(f"/{self.tkmod.PLAYLIST_GIFS_DIRNAME}/")
        touched_folders = set()
        for entry in entries:
            rest = entry[prefix_len:] if entry.startswith(f"/{self.tkmod.PLAYLIST_GIFS_DIRNAME}/") else entry
            if "/" not in rest:
                continue
            folder, fname = rest.split("/", 1)
            if folder in full_folders:
                continue  # deja couvert par la case "dossier entier"
            key = (folder, fname)
            var = self._playlist_checked.get(key)
            if var is None:
                var = tk.BooleanVar(value=False)
                self._playlist_checked[key] = var
            var.set(True)
            touched_folders.add(folder)

        for folder_name in touched_folders:
            self._playlist_recompute_folder_state(folder_name)

        if self._playlist_browsed_folder:
            self._refresh_playlist_files_for_folder(self._playlist_browsed_folder)

    def _clear_playlist_checklist(self) -> None:
        for child in list(self._playlist_checklist_inner.winfo_children()):
            child.destroy()

    def _playlist_list_folders_cached(self) -> list:
        """Liste (nom, nb_gifs) des dossiers gifs/ du lecteur courant,
        depuis le cache memoire de session si deja connu (evite un
        rescan disque a chaque re-affichage -- lent sur lecteur USB).
        Voir _playlist_invalidate_cache().

        Sous-mode "add" du mode temporaire Mode1 (demande utilisateur) :
        n'affiche QUE les dossiers "en attente" de copie
        (self._playlist_pending_folders) -- pas le contenu deja reel de
        gifs/ (pack GitHub deja telecharge en arriere-plan, dossiers
        perso deja copies lors d'un lot precedent, etc.), qui n'ont rien
        a faire dans une liste dediee a ce qu'on est en train d'ajouter.
        Sous-mode "playlist" (et mode normal, carte SD reelle) : liste
        habituelle, TOUT gifs/ (necessaire pour construire une playlist a
        partir de l'ensemble disponible, pack + perso)."""
        pending = [
            (name, len(info["files"]))
            for name, info in getattr(self, "_playlist_pending_folders", {}).items()
        ]
        if (
            getattr(self, "_playlist_temp_mode", False)
            and getattr(self, "_playlist_temp_submode", "add") == "add"
        ):
            return pending
        root = self._playlist_sd_root
        if root is None:
            return pending
        key = str(root)
        cached = self._playlist_folders_cache.get(key)
        if cached is not None:
            folders = cached
        else:
            try:
                folders = self.tkmod.list_playlist_gif_folders(root)
            except Exception:
                folders = []
            self._playlist_folders_cache[key] = folders
        return pending + folders

    def _playlist_list_files_cached(self, folder_name: str) -> list:
        """Idem _playlist_list_folders_cached() pour les fichiers .gif
        d'un dossier precis. Un dossier "en attente" (pas encore copie)
        renvoie son listing deja connu (source PC), sans toucher a
        self._playlist_sd_root."""
        pending = getattr(self, "_playlist_pending_folders", {}).get(folder_name)
        if pending is not None:
            return pending["files"]
        root = self._playlist_sd_root
        if root is None:
            return []
        key = (str(root), folder_name)
        cached = self._playlist_files_cache.get(key)
        if cached is not None:
            return cached
        try:
            files = self.tkmod.list_gif_files_in_folder(root, folder_name)
        except Exception:
            files = []
        self._playlist_files_cache[key] = files
        return files

    def _playlist_invalidate_cache(self, folder_name: Optional[str] = None) -> None:
        """A appeler apres toute mutation reelle du contenu de gifs/
        (import, suppression, telechargement du pack Mode 1) : force un
        vrai rescan disque au prochain affichage. folder_name=None
        invalide tout le lecteur courant (liste des dossiers + tous les
        caches de fichiers), sinon seulement ce dossier + la liste des
        dossiers (son compteur a change)."""
        root = self._playlist_sd_root
        key_root = str(root) if root is not None else None
        self._playlist_folders_cache.pop(key_root, None)
        if folder_name is None:
            self._playlist_files_cache = {
                k: v for k, v in self._playlist_files_cache.items() if k[0] != key_root
            }
        else:
            self._playlist_files_cache.pop((key_root, folder_name), None)

    def _playlist_content_colors(self) -> tuple[str, str]:
        """Couleur de fond/texte "surface de contenu" du theme actif
        (bg_listbox/fg_listbox) pour les cadres Dossiers/Fichiers de
        l'onglet Playlist -- ces cadres restent hors du systeme de theming
        generique (_walk_and_apply ignore les Canvas, et traite tout Frame
        en bg="white" comme une couleur fixe volontaire a preserver, cf.
        _playlist_apply_theme_colors()). bg_listbox/fg_listbox sont deja
        concues par le systeme de themes pour rester lisibles en toutes
        circonstances (jamais de blanc fixe, illisible en ton-sur-ton sur
        certains themes clairs)."""
        c = self._theme_colors()
        return c.get("bg_listbox", "#FFFFFF"), c.get("fg_listbox", "#000000")

    def _playlist_apply_theme_colors(self) -> None:
        """Reapplique les couleurs de contenu (fond dossiers/fichiers) du
        theme actuellement actif. A appeler a la construction de l'onglet
        ET a chaque changement de theme (_on_theme_selected). Les listes
        sont reconstruites avec les nouvelles couleurs, mais depuis le
        cache memoire de session deja peuple -> aucun rescan disque
        (cf _playlist_list_*_cached())."""
        if not getattr(self, "_playlist_folder_outer", None):
            return
        bg, _fg = self._playlist_content_colors()
        for w in (
            self._playlist_folder_outer, self._playlist_folder_canvas, self._playlist_folder_inner,
            self._playlist_files_outer, self._playlist_checklist_canvas, self._playlist_checklist_inner,
        ):
            try:
                w.configure(bg=bg)
            except tk.TclError:
                pass
        if self._playlist_sd_root is not None:
            self._refresh_playlist_folders()
            if self._playlist_browsed_folder:
                self._refresh_playlist_files_for_folder(self._playlist_browsed_folder)

    def _playlist_refresh_explanation_text(self) -> None:
        """Bascule le texte du cadre d'explication (a droite de la carte
        SD) entre 2 versions selon le contexte -- demande utilisateur,
        "trop succinct" auparavant : synthese complete du fonctionnement
        de l'onglet (dossiers/fichiers, codes couleur, tous les boutons)
        en usage direct, focus sur les 2 actions a faire pour continuer le
        Mode 1 (Ajouter un dossier PC... + Regenerer le cache) en mode
        temporaire."""
        if not getattr(self, "_playlist_explanation_lbl", None):
            return
        ui = self._get_ui_t()
        if getattr(self, "_playlist_temp_mode", False):
            submode = getattr(self, "_playlist_temp_submode", "add")
            key = (
                "playlist_explanation_text_mode1_playlist"
                if submode == "playlist"
                else "playlist_explanation_text_mode1_add"
            )
        else:
            key = "playlist_explanation_text_normal"
        self._playlist_explanation_lbl.configure(text=ui[key])

    def _playlist_refresh_temp_submode_ui(self) -> None:
        """Applique l'affichage propre au sous-mode temporaire Mode1 en
        cours (self._playlist_temp_submode) : "add" (ajout de dossiers PC
        "en attente" de copie, etat initial) ou "playlist" (construction
        effective d'une playlist, apres reponse "oui" a la proposition --
        voir _on_playlist_regen_cache_clicked). Masque/affiche le champ
        nom de playlist (sans objet tant qu'on ne nomme pas encore une
        playlist), adapte le texte du mode temporaire et le cadre
        d'explication, et le bouton "Construire la playlist" : devient
        "Copier la selection" en sous-mode "add" (valide la copie reelle
        des dossiers en attente, voir _on_playlist_copy_pending_clicked ;
        grise tant qu'aucun dossier n'est en attente), redevient
        "Construire la playlist" normal en sous-mode "playlist"."""
        if not getattr(self, "_playlist_temp_mode", False):
            return
        ui = self._get_ui_t()
        submode = getattr(self, "_playlist_temp_submode", "add")
        if submode == "playlist":
            self._playlist_name_frame.pack(fill="x", before=self._playlist_folders_title_row)
            self._playlist_temp_note_lbl.configure(text=ui["playlist_temp_note_playlist"](self.sd_dir))
            self._playlist_build_btn.configure(
                text=ui["playlist_build_btn"], command=self._on_playlist_build_clicked, state="normal",
            )
        else:
            self._playlist_name_frame.pack_forget()
            self._playlist_temp_note_lbl.configure(text=ui["playlist_temp_note_add"](self.sd_dir))
            has_pending = bool(self._playlist_pending_folders)
            self._playlist_build_btn.configure(
                text=ui["playlist_copy_pending_btn"], command=self._on_playlist_copy_pending_clicked,
                state="normal" if has_pending else "disabled",
            )
        self._playlist_refresh_explanation_text()
        # La liste Dossiers affichee differe selon le sous-mode (voir
        # _playlist_list_folders_cached()) -- reconstruite ici pour
        # refleter immediatement le changement de sous-mode (ex: passage
        # "add" -> "playlist" doit faire apparaitre tout gifs/, pas
        # seulement les dossiers restes en attente).
        self._refresh_playlist_folders()
        if self._playlist_browsed_folder:
            self._refresh_playlist_files_for_folder(self._playlist_browsed_folder)

    def _refresh_playlist_folders(self) -> None:
        for child in list(self._playlist_folder_inner.winfo_children()):
            child.destroy()
        self._playlist_folder_counts.clear()
        self._playlist_folder_checkbox_widgets.clear()

        # Reteinte les conteneurs (pas seulement les lignes) a CHAQUE
        # rafraichissement, pas seulement a la construction initiale ou a
        # un changement de theme manuel -- bug reel observe (capture
        # d'ecran utilisateur) : au tout premier lancement, cette methode
        # peut s'executer AVANT que le theme de demarrage (aleatoire ou
        # sauvegarde) ne soit reellement applique (themes.apply() est
        # differe via root.after() dans __init__), figeant durablement les
        # conteneurs Dossiers sur le theme par defaut (blanc) alors que le
        # cadre Fichiers, peuple plus tard (au premier clic utilisateur,
        # theme deja charge), affichait la bonne couleur -- d'ou le
        # contraste blanc/sombre observe entre les 2 cadres.
        content_bg, content_fg = self._playlist_content_colors()
        # Le cadre Fichiers est teint ICI AUSSI (pas seulement dans
        # _refresh_playlist_files_for_folder(), qui ne s'execute qu'au
        # premier clic sur un dossier) : sans ce filet, il pouvait rester
        # visible avec sa couleur par defaut (jamais rafraichi) tant
        # qu'aucun dossier n'avait encore ete parcouru -- notamment juste
        # apres l'entree en mode temporaire (Mode 1), ou le cadre
        # Dossiers/Fichiers doit deja etre coherent avant toute action.
        for w in (
            self._playlist_folder_outer, self._playlist_folder_canvas, self._playlist_folder_inner,
            self._playlist_files_outer, self._playlist_checklist_canvas, self._playlist_checklist_inner,
        ):
            try:
                w.configure(bg=content_bg)
            except tk.TclError:
                pass

        if self._playlist_sd_root is None:
            return
        folders = self._playlist_list_folders_cached()

        for folder_name, count in folders:
            self._playlist_folder_counts[folder_name] = count
            var = self._playlist_folder_checked.get(folder_name)
            if var is None:
                var = tk.IntVar(value=0)
                self._playlist_folder_checked[folder_name] = var

            row = tk.Frame(self._playlist_folder_inner, bg=content_bg)
            row.pack(fill="x", anchor="w")
            cb = tk.Checkbutton(
                row, variable=var, bg=content_bg,
                onvalue=1, offvalue=0, tristatevalue=2,
                command=lambda f=folder_name: self._on_playlist_folder_check_toggled(f),
            )
            cb.pack(side="left")
            lbl = tk.Label(
                row, text=f"{folder_name}  ({count})", bg=content_bg, fg=content_fg,
                font=("TkDefaultFont", 9), cursor="hand2", anchor="w",
            )
            # anchor="w" doit etre passe au CONSTRUCTEUR du Label (controle
            # l'alignement du TEXTE dans sa propre zone, "center" par
            # defaut) -- pas seulement a .pack(anchor="w") (qui ne
            # controle que le positionnement du WIDGET lui-meme dans
            # l'espace alloue par le packer, ineffectif ici puisque
            # fill="x"+expand=True lui font deja occuper toute la largeur
            # disponible). Sans lui, le texte apparaissait centre dans une
            # etiquette pourtant deja pleine largeur.
            lbl.pack(side="left", fill="x", expand=True, anchor="w")
            lbl.bind("<Button-1>", lambda e, f=folder_name: self._on_playlist_folder_name_clicked(f))
            self._playlist_folder_checkbox_widgets[folder_name] = (row, cb, lbl)
            self._playlist_update_folder_checkbox_visual(folder_name)

        # Dossiers deja parcourus (cases fichier individuelles connues) :
        # recalcule leur etat complet/partiel/aucun a partir des cases
        # fichier reelles -- le nombre de fichiers a pu changer depuis
        # (import/suppression) et la case dossier doit rester coherente.
        touched_folders = {folder for (folder, _fname) in self._playlist_checked.keys()}
        for folder_name in touched_folders:
            if folder_name in self._playlist_folder_counts:
                self._playlist_recompute_folder_state(folder_name)

    def _playlist_update_folder_checkbox_visual(self, folder_name: str) -> None:
        """Teinte toute la ligne "dossier" en orange clair quand sa
        selection est partielle (etat tristate=2), couleur de contenu du
        theme sinon -- une "coche differente" bien visible pour
        l'utilisateur. Souligne aussi le nom du dossier actuellement
        parcouru (celui dont les fichiers sont affiches a droite). Met a
        jour les widgets en place (sans reconstruire toute la liste) pour
        rester reactif a chaque (de)coche de fichier individuel ou
        changement de dossier parcouru.

        Teinte la ligne entiere plutot que le seul indicateur natif de la
        case a cocher (option Tk `selectcolor`) : verifie empiriquement
        que Windows/Tk ignore silencieusement `selectcolor` pour la valeur
        tristate sur ce theme -- un simple changement de couleur de case
        ne serait donc pas visible a l'ecran."""
        widgets = self._playlist_folder_checkbox_widgets.get(folder_name)
        if widgets is None:
            return
        row, cb, lbl = widgets
        var = self._playlist_folder_checked.get(folder_name)
        state = var.get() if var is not None else 0
        if state == 2:
            bg_color, fg_color = "#FFE0A3", "black"
        else:
            bg_color, fg_color = self._playlist_content_colors()
        for w in (row, cb):
            try:
                w.configure(bg=bg_color)
            except tk.TclError:
                pass
        is_browsed = folder_name == self._playlist_browsed_folder
        try:
            lbl.configure(
                bg=bg_color, fg=fg_color,
                font=("TkDefaultFont", 9, "underline" if is_browsed else "normal"),
            )
        except tk.TclError:
            pass

    def _on_playlist_folder_check_toggled(self, folder_name: str) -> None:
        """Case "dossier entier" cliquee directement par l'utilisateur :
        source de verite dossier -> fichiers, force TOUS les fichiers du
        dossier a suivre le nouvel etat coche/decoche (un clic utilisateur
        ne peut jamais amener la case sur la valeur tristate=2, reservee a
        _playlist_recompute_folder_state())."""
        checked = self._playlist_folder_checked[folder_name].get() == 1
        self._playlist_update_folder_checkbox_visual(folder_name)
        self._playlist_apply_folder_state_to_files(folder_name, checked)
        self._playlist_maybe_autofill_name()

    def _playlist_maybe_autofill_name(self) -> None:
        """Pre-remplit le champ de nom de playlist avec le nom du dossier
        des qu'un seul dossier a au moins une coche active (dossier
        entier ou fichiers individuels) -- pratique pour creer une
        nouvelle playlist a partir d'un seul dossier sans avoir a taper
        son nom. N'ecrase jamais un nom deja tape manuellement par
        l'utilisateur (voir _playlist_name_autofilled et
        _on_playlist_name_field_clicked, qui n'efface que la suggestion
        automatique)."""
        selected_folders = {
            folder for folder, var in self._playlist_folder_checked.items() if var.get() == 1
        }
        selected_folders |= {
            folder for (folder, _fname), var in self._playlist_checked.items() if var.get()
        }
        current = self._playlist_name_var.get()
        if len(selected_folders) == 1:
            folder = next(iter(selected_folders))
            if current == "" or self._playlist_name_autofilled:
                self._playlist_name_var.set(folder)
                self._playlist_name_autofilled = True
        elif self._playlist_name_autofilled:
            self._playlist_name_var.set("")
            self._playlist_name_autofilled = False

    def _on_playlist_name_field_clicked(self, event=None) -> None:
        """Un clic dans le champ de saisie efface la suggestion
        automatique (nom de dossier pre-rempli) pour laisser
        l'utilisateur taper un nom personnalise -- n'efface jamais un nom
        deja tape manuellement (voir _playlist_maybe_autofill_name)."""
        if self._playlist_name_autofilled:
            self._playlist_name_var.set("")
            self._playlist_name_autofilled = False

    def _playlist_apply_folder_state_to_files(self, folder_name: str, checked: bool) -> None:
        """Coche/decoche TOUS les fichiers connus d'un dossier (source de
        verite dossier -> fichiers), en creant au besoin les variables
        fichier manquantes (dossier jamais parcouru individuellement).
        Rafraichit l'affichage si ce dossier est actuellement parcouru."""
        for fname in self._playlist_list_files_cached(folder_name):
            key = (folder_name, fname)
            var = self._playlist_checked.get(key)
            if var is None:
                var = tk.BooleanVar(value=checked)
                self._playlist_checked[key] = var
            else:
                var.set(checked)
        if self._playlist_browsed_folder == folder_name:
            self._refresh_playlist_files_for_folder(folder_name)

    def _playlist_recompute_folder_state(self, folder_name: str) -> None:
        """Recalcule l'etat (0=aucun, 1=complet, 2=partiel/tristate) de la
        case "dossier entier" a partir de l'etat reel des cases fichier
        individuelles (source de verite fichiers -> dossier), suite au
        (de)cochage d'un fichier individuel. Ne fait rien si le dossier
        n'a pas encore ete liste (nb de fichiers inconnu)."""
        total = self._playlist_folder_counts.get(folder_name, 0)
        if total <= 0:
            return
        checked = sum(
            1 for (folder, _fname), var in self._playlist_checked.items()
            if folder == folder_name and var.get()
        )
        var = self._playlist_folder_checked.get(folder_name)
        if var is None:
            return
        if checked <= 0:
            var.set(0)
        elif checked >= total:
            var.set(1)
        else:
            var.set(2)
        self._playlist_update_folder_checkbox_visual(folder_name)
        self._playlist_maybe_autofill_name()

    def _on_playlist_file_check_toggled(self, folder_name: str) -> None:
        self._playlist_recompute_folder_state(folder_name)

    def _on_playlist_folder_name_clicked(self, folder_name: str) -> None:
        previous = self._playlist_browsed_folder
        self._playlist_browsed_folder = folder_name
        # Souligne le nom du nouveau dossier parcouru, retire le
        # soulignement de l'ancien (demande utilisateur).
        if previous and previous != folder_name:
            self._playlist_update_folder_checkbox_visual(previous)
        self._playlist_update_folder_checkbox_visual(folder_name)
        self._refresh_playlist_files_for_folder(folder_name)

    def _playlist_gif_path_for(self, folder_name: str, fname: str) -> Path:
        """Chemin reel d'un .gif pour l'apercu au survol : source PC pour
        un dossier "en attente" (pas encore copie, voir
        self._playlist_pending_folders), sinon emplacement habituel sous
        self._playlist_sd_root."""
        pending = self._playlist_pending_folders.get(folder_name)
        if pending is not None:
            return pending["src"] / fname
        return self._playlist_sd_root / self.tkmod.PLAYLIST_GIFS_DIRNAME / folder_name / fname

    def _refresh_playlist_files_for_folder(self, folder_name: str) -> None:
        self._clear_playlist_checklist()
        # Meme reteinte systematique des conteneurs qu'en tete de
        # _refresh_playlist_folders() (voir son commentaire pour la cause
        # racine du bug corrige).
        content_bg, content_fg = self._playlist_content_colors()
        for w in (self._playlist_files_outer, self._playlist_checklist_canvas, self._playlist_checklist_inner):
            try:
                w.configure(bg=content_bg)
            except tk.TclError:
                pass
        if self._playlist_sd_root is None:
            return
        files = self._playlist_list_files_cached(folder_name)

        folder_var = self._playlist_folder_checked.get(folder_name)
        folder_is_full = bool(folder_var is not None and folder_var.get() == 1)

        for fname in files:
            key = (folder_name, fname)
            var = self._playlist_checked.get(key)
            if var is None:
                var = tk.BooleanVar(value=folder_is_full)
                self._playlist_checked[key] = var

            row = tk.Frame(self._playlist_checklist_inner, bg=content_bg)
            row.pack(fill="x", anchor="w")
            cb = tk.Checkbutton(
                row, variable=var, bg=content_bg,
                command=lambda f=folder_name: self._on_playlist_file_check_toggled(f),
            )
            cb.pack(side="left")
            lbl = tk.Label(row, text=fname, bg=content_bg, fg=content_fg, font=("TkDefaultFont", 9), anchor="w")
            lbl.pack(side="left", fill="x", expand=True, anchor="w")
            gif_path = self._playlist_gif_path_for(folder_name, fname)
            for w in (row, lbl):
                w.bind("<Enter>", lambda e, p=gif_path: self._show_gif_preview(e, p))
                w.bind("<Leave>", self._hide_gif_preview)

    def _on_playlist_folders_select_all(self) -> None:
        for folder_name, var in list(self._playlist_folder_checked.items()):
            var.set(1)
            self._playlist_apply_folder_state_to_files(folder_name, True)

    def _on_playlist_folders_select_none(self) -> None:
        for folder_name, var in list(self._playlist_folder_checked.items()):
            var.set(0)
            self._playlist_apply_folder_state_to_files(folder_name, False)

    def _on_playlist_files_select_all(self) -> None:
        if not self._playlist_browsed_folder:
            return
        for (folder, _fname), var in self._playlist_checked.items():
            if folder == self._playlist_browsed_folder:
                var.set(True)
        self._playlist_recompute_folder_state(self._playlist_browsed_folder)

    def _on_playlist_files_select_none(self) -> None:
        if not self._playlist_browsed_folder:
            return
        for (folder, _fname), var in self._playlist_checked.items():
            if folder == self._playlist_browsed_folder:
                var.set(False)
        self._playlist_recompute_folder_state(self._playlist_browsed_folder)

    def _show_gif_preview(self, event, gif_path: Path) -> None:
        self._hide_gif_preview()
        if not gif_path.exists():
            return
        try:
            from PIL import Image, ImageSequence, ImageTk
            im = Image.open(gif_path)
            frames = []
            for frame in ImageSequence.Iterator(im):
                frames.append((ImageTk.PhotoImage(frame.convert("RGBA")), frame.info.get("duration", 100)))
        except Exception:
            return
        if not frames:
            return

        win = tk.Toplevel(self.root)
        win.overrideredirect(True)
        x = self.root.winfo_pointerx() + 16
        y = self.root.winfo_pointery() + 16
        win.geometry(f"+{x}+{y}")
        lbl = tk.Label(win, image=frames[0][0], bd=1, relief="solid")
        lbl.pack()
        win.lift()

        self._playlist_gif_preview_win = win
        self._playlist_gif_preview_frames = frames
        self._playlist_gif_preview_idx = 0
        self._playlist_gif_preview_lbl = lbl
        self._advance_gif_preview()

    def _advance_gif_preview(self) -> None:
        if not self._playlist_gif_preview_win or not self._playlist_gif_preview_frames:
            return
        frames = self._playlist_gif_preview_frames
        idx = self._playlist_gif_preview_idx % len(frames)
        img, delay = frames[idx]
        try:
            self._playlist_gif_preview_lbl.configure(image=img)
        except Exception:
            return
        self._playlist_gif_preview_idx = idx + 1
        self._playlist_gif_preview_job = self.root.after(max(20, delay), self._advance_gif_preview)

    def _hide_gif_preview(self, event=None) -> None:
        if self._playlist_gif_preview_job:
            try:
                self.root.after_cancel(self._playlist_gif_preview_job)
            except Exception:
                pass
            self._playlist_gif_preview_job = None
        if self._playlist_gif_preview_win:
            try:
                self._playlist_gif_preview_win.destroy()
            except Exception:
                pass
            self._playlist_gif_preview_win = None
        self._playlist_gif_preview_frames = []

    def _prompt_playlist_subfolder_checklist_dialog(self, root_path: Path, subfolders: list) -> list:
        """Reprend le systeme deja utilise dans les autres onglets pour
        choisir des dossiers (racine ROMs/Images : 1 selecteur natif puis
        case a cocher par dossier detecte, cf. _pick_roms_directory() +
        sys_list) plutot qu'un dialogue "Parcourir..." repete a la main
        (demande utilisateur explicite) : root_path a deja ete choisi par
        l'appelant via le selecteur de dossier natif Windows (qui ne
        permet de toute facon pas de choisir plusieurs dossiers en une
        fois -- limitation du shell lui-meme, pas de Tkinter). subfolders
        est deja la liste COMPLETE des dossiers de GIFs trouves sous
        root_path, a n'importe quel niveau de profondeur (cf.
        find_gif_folders_recursive()) -- cette boite ne fait que les
        proposer en case a cocher, avec leur chemin relatif a root_path
        en etiquette pour les distinguer si plusieurs partagent le meme
        nom propre a des niveaux differents. Coches par defaut (comme
        "Tout selectionner" habituel), "Tout"/"Rien" pour ajuster vite.
        Retourne la liste des dossiers coches ([] si annule)."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        bg_listbox = c.get("bg_listbox", "#FFFFFF")
        fg_listbox = c.get("fg_listbox", "#000000")

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["playlist_multi_folder_dialog_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=ui["playlist_subfolder_checklist_prompt"].format(root=str(root_path)),
            bg=bg, fg=fg, font=("TkDefaultFont", 9), wraplength=440, justify="left",
        ).pack(anchor="w", pady=(0, 8))

        select_row = tk.Frame(body, bg=bg)
        select_row.pack(fill="x", pady=(0, 4))
        select_none_btn = tk.Button(
            select_row, text=ui["playlist_select_none_btn"],
            bg=bg_normal, fg=fg, bd=1, relief="solid", padx=4, pady=1, font=("TkDefaultFont", 7),
        )
        select_none_btn.pack(side="right")
        select_all_btn = tk.Button(
            select_row, text=ui["playlist_select_all_btn"],
            bg=bg_normal, fg=fg, bd=1, relief="solid", padx=4, pady=1, font=("TkDefaultFont", 7),
        )
        select_all_btn.pack(side="right", padx=(0, 4))

        # Meme pattern Canvas+Frame scrollable que la checklist "Dossiers
        # (gifs/)" de l'onglet (un Listbox ne peut pas heberger de vraies
        # cases a cocher).
        list_outer = tk.Frame(body, bg=bg_listbox, bd=2, relief="solid")
        # Meme exemption que les cadres Dossiers/Fichiers de l'onglet
        # Playlist (RecalBoxDMD_themes._slice_widgets_later) : sans elle,
        # un fragment de l'image de fond du theme s'incruste derriere les
        # lignes a cocher (constate reellement, capture d'ecran).
        list_outer._no_bg_slice = True
        list_outer.pack(fill="both", expand=True, pady=(0, 10))
        canvas = tk.Canvas(list_outer, bg=bg_listbox, height=240, width=440, highlightthickness=0)
        scroll = tk.Scrollbar(list_outer, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=scroll.set)
        canvas.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        inner = tk.Frame(canvas, bg=bg_listbox)
        inner_win = canvas.create_window((0, 0), window=inner, anchor="nw")
        inner.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind("<Configure>", lambda e: canvas.itemconfig(inner_win, width=e.width))

        def _wheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        canvas.bind("<Enter>", lambda e: canvas.bind_all("<MouseWheel>", _wheel))
        canvas.bind("<Leave>", lambda e: canvas.unbind_all("<MouseWheel>"))

        vars_by_path: dict = {}
        for sub in subfolders:
            var = tk.BooleanVar(value=True)
            vars_by_path[sub] = var
            # Chemin RELATIF a la racine choisie (pas juste le nom propre)
            # -- find_gif_folders_recursive() peut remonter des dossiers a
            # des niveaux de profondeur differents (ex: "Arcade" a la
            # racine ET "Divers/Halloween" plus bas) ; le nom seul les
            # rendrait indiscernables/ambigus a l'ecran. root_path lui-meme
            # (rel == ".") s'affiche sous son propre nom.
            try:
                rel = sub.relative_to(root_path)
                label_text = str(rel) if str(rel) != "." else sub.name
            except ValueError:
                label_text = sub.name
            row = tk.Frame(inner, bg=bg_listbox)
            row.pack(fill="x", anchor="w")
            tk.Checkbutton(row, variable=var, bg=bg_listbox).pack(side="left")
            tk.Label(
                row, text=label_text, bg=bg_listbox, fg=fg_listbox,
                font=("TkDefaultFont", 9), anchor="w",
            ).pack(side="left", fill="x", expand=True, anchor="w")

        select_all_btn.configure(command=lambda: [v.set(True) for v in vars_by_path.values()])
        select_none_btn.configure(command=lambda: [v.set(False) for v in vars_by_path.values()])

        result = {"ok": False}

        def _ok(event=None):
            result["ok"] = True
            dlg.destroy()

        def _cancel():
            dlg.destroy()

        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")
        tk.Button(
            btns, text=ui["mode1_manual_ip_ok"], command=_ok,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", expand=True, fill="x", padx=(0, 4))
        tk.Button(
            btns, text=ui["mode1_manual_ip_cancel"], command=_cancel,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="right", expand=True, fill="x", padx=(4, 0))
        dlg.protocol("WM_DELETE_WINDOW", _cancel)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)

        if not result["ok"]:
            return []
        return [p for p, v in vars_by_path.items() if v.get()]

    def _prompt_playlist_folder_name_dialog(self, default: str = "") -> Optional[str]:
        """Popup themee avec un champ de saisie pour le nom du sous-dossier
        de destination (import PC externe). Retourne le nom saisi (strip),
        ou None si annule/vide."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        result = {"value": None}
        dlg = tk.Toplevel(self.root)
        dlg.title(ui["playlist_import_dialog_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=ui["playlist_import_dialog_prompt"], bg=bg, fg=fg,
            font=("TkDefaultFont", 9), wraplength=380, justify="left",
        ).pack(anchor="w", pady=(0, 10))
        entry_var = tk.StringVar(value=default)
        entry = tk.Entry(body, textvariable=entry_var, font=("TkDefaultFont", 10), cursor="xterm")
        entry.pack(fill="x", pady=(0, 12))
        entry.select_range(0, "end")
        entry.focus_set()

        def _ok(event=None):
            val = entry_var.get().strip()
            result["value"] = val or None
            dlg.destroy()

        def _cancel():
            result["value"] = None
            dlg.destroy()

        entry.bind("<Return>", _ok)
        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")
        tk.Button(
            btns, text=ui["mode1_manual_ip_ok"], command=_ok,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", expand=True, fill="x", padx=(0, 4))
        tk.Button(
            btns, text=ui["mode1_manual_ip_cancel"], command=_cancel,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="right", expand=True, fill="x", padx=(4, 0))
        dlg.protocol("WM_DELETE_WINDOW", _cancel)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)
        return result["value"]

    def _on_playlist_add_external_folder_clicked(self) -> None:
        """Reprend le systeme deja utilise dans les autres onglets pour
        choisir des dossiers (racine choisie via le selecteur natif Windows
        -- un seul a la fois, comme _pick_roms_directory() : le shell
        Windows lui-meme ne permet pas de choisir plusieurs dossiers en un
        seul appel) puis case a cocher, cf.
        _prompt_playlist_subfolder_checklist_dialog(). Selection en un
        seul passage (demande utilisateur) : au lieu de ne lister QUE les
        sous-dossiers immediats, cherche RECURSIVEMENT (tous niveaux,
        find_gif_folders_recursive()) tous les dossiers qui contiennent
        reellement des .gif -- un dossier sans gif direct (meme s'il a des
        sous-dossiers qui en ont) n'est jamais propose lui-meme, seuls ses
        sous-dossiers concernes le sont. Nom de destination = nom propre
        du dossier trouve (pas son chemin complet, meme convention que la
        detection de "systemes" ROMs)."""
        ui = self._get_ui_t()
        if self._is_processing():
            messagebox.showwarning(ui["msg_warning_title"], ui["processing_in_progress_msg"])
            return
        if self._playlist_sd_root is None:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_no_sd_msg"])
            return
        last_ext = prefs.get("playlist_last_external_folder") or str(Path.home())
        root_str = filedialog.askdirectory(initialdir=last_ext, title=ui["playlist_import_dialog_title"])
        if not root_str:
            return
        root_path = Path(root_str)
        prefs.set("playlist_last_external_folder", str(root_path))

        gif_folders = self.tkmod.find_gif_folders_recursive(root_path)
        if not gif_folders:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_no_gif_folders_found_msg"])
            return

        chosen = self._prompt_playlist_subfolder_checklist_dialog(root_path, gif_folders)
        if not chosen:
            return

        if getattr(self, "_playlist_temp_mode", False) and getattr(self, "_playlist_temp_submode", "add") == "add":
            # Sous-mode "add" du mode temporaire Mode1 (demande
            # utilisateur) : ne copie PAS encore -- ajoute seulement a la
            # liste "en attente" pour revue (l'utilisateur peut deselectionner
            # des fichiers avant la copie reelle). La copie effective se
            # fait via "Copier la selection" (_on_playlist_copy_pending_clicked),
            # qui remplace "Construire la playlist" dans ce sous-mode.
            for p in chosen:
                files = sorted(
                    (f.name for f in p.iterdir() if f.is_file() and f.suffix.lower() == ".gif"),
                    key=str.lower,
                )
                self._playlist_pending_folders[p.name] = {"src": p, "files": files}
                # Tout coche par defaut (comme dans la checklist de
                # sous-dossiers qui vient de servir) -- l'utilisateur
                # deselectionne ce qu'il ne veut pas garder.
                self._playlist_folder_checked[p.name] = tk.IntVar(value=1)
                self._playlist_checked = {
                    k: v for k, v in self._playlist_checked.items() if k[0] != p.name
                }
            self._refresh_playlist_folders()
            if self._playlist_browsed_folder in self._playlist_pending_folders:
                self._refresh_playlist_files_for_folder(self._playlist_browsed_folder)
            self._playlist_refresh_temp_submode_ui()
            return

        pairs = [(p, p.name) for p in chosen]
        self._start_worker(self._playlist_import_worker, args=(pairs,))

    def _playlist_import_worker(self, pairs: list) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        results: list = []
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            for src_path, dest_name in pairs:
                copied, skipped, renamed = self.tkmod.copy_external_gifs_to_sd(
                    self._playlist_sd_root, src_path, dest_name, progress_cb=self._progress_cb
                )
                new_files = self.tkmod.list_gif_files_in_folder(self._playlist_sd_root, dest_name)
                n_updated = self.tkmod.update_playlists_referencing_folder(self._playlist_sd_root, dest_name, new_files)
                # Contrepartie PC de la cible d'ajout inconditionnelle du
                # firmware lors d'un upload web (cf. docstring
                # append_to_master_gifs_cache()) : sans ceci, le DMD
                # pourrait generer une playlist "hybride" incomplete pour
                # ce dossier tant que le cache maitre n'est pas regenere
                # manuellement.
                self.tkmod.append_to_master_gifs_cache(self._playlist_sd_root, dest_name, new_files)
                results.append((dest_name, copied, skipped, renamed, n_updated))
                print(f"[GUI] Import '{dest_name}' termine : {copied} copies, {skipped} identiques ignores, {renamed} renommes, {n_updated} playlist(s) mise(s) a jour")
        except Exception as e:
            print(f"[GUI] Erreur import dossier externe : {e}")
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
        self.root.after(0, self._on_playlist_import_done, results)

    def _on_playlist_import_done(self, results: list) -> None:
        # Contenu physique de chaque dest_name modifie sur disque -> le
        # cache memoire de session (liste des dossiers + fichiers de ces
        # dossiers) doit etre invalide avant tout re-affichage, sinon
        # l'ancien comptage/liste (perime) resterait affiche.
        for dest_name, *_rest in results:
            self._playlist_invalidate_cache(dest_name)
        self._refresh_playlist_folders()
        if self._playlist_browsed_folder:
            self._refresh_playlist_files_for_folder(self._playlist_browsed_folder)
        if not results:
            return
        # Note : en mode temporaire Mode1 sous-mode "add", "Ajouter un
        # dossier PC..." n'emprunte plus ce chemin (copie immediate) --
        # voir _on_playlist_add_external_folder_clicked() (dossiers "en
        # attente", copie differee via _on_playlist_copy_pending_clicked).
        # Ce handler ne reste emprunte qu'en mode normal (carte SD reelle)
        # et en sous-mode "playlist" (comportement inchange, immediat).
        ui = self._get_ui_t()
        names = ", ".join(r[0] for r in results)
        total_copied = sum(r[1] for r in results)
        total_skipped = sum(r[2] for r in results)
        total_renamed = sum(r[3] for r in results)
        total_updated = sum(r[4] for r in results)
        self._themed_info(
            ui["playlist_import_dialog_title"],
            ui["playlist_import_done_msg_multi"].format(
                n=len(results), names=names, copied=total_copied, skipped=total_skipped,
                renamed=total_renamed, n_updated=total_updated,
            ),
        )

    def _on_playlist_copy_pending_clicked(self) -> None:
        """Bouton "Copier la selection" (remplace "Construire la
        playlist" en sous-mode "add" du mode temporaire Mode1) : copie
        REELLEMENT vers le dossier temporaire les fichiers coches des
        dossiers "en attente" (self._playlist_pending_folders) -- dossier
        entier si sa case est cochee (1), sinon seulement les fichiers
        individuellement coches (aucun coche -> dossier laisse tel quel,
        toujours en attente). Seule action qui ecrit reellement sur
        disque dans ce sous-mode -- voir
        _on_playlist_add_external_folder_clicked(), qui n'ajoute plus
        qu'a la liste "en attente" sans copier."""
        ui = self._get_ui_t()
        if self._is_processing():
            messagebox.showwarning(ui["msg_warning_title"], ui["processing_in_progress_msg"])
            return
        if not self._playlist_pending_folders:
            return
        tasks = []
        for name, info in self._playlist_pending_folders.items():
            folder_var = self._playlist_folder_checked.get(name)
            if folder_var is not None and folder_var.get() == 1:
                tasks.append((name, info["src"], None))  # dossier entier
                continue
            checked = [
                fname for (folder, fname), var in self._playlist_checked.items()
                if folder == name and var.get()
            ]
            if checked:
                tasks.append((name, info["src"], checked))
            # sinon : rien de coche pour ce dossier -- laisse en attente
        if not tasks:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_copy_pending_nothing_msg"])
            return
        self._start_worker(self._playlist_copy_pending_worker, args=(tasks,))

    def _playlist_copy_pending_worker(self, tasks: list) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        results: list = []
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            for name, src_path, only_files in tasks:
                copied, skipped, renamed = self.tkmod.copy_external_gifs_to_sd(
                    self._playlist_sd_root, src_path, name,
                    progress_cb=self._progress_cb, only_files=only_files,
                )
                new_files = self.tkmod.list_gif_files_in_folder(self._playlist_sd_root, name)
                n_updated = self.tkmod.update_playlists_referencing_folder(self._playlist_sd_root, name, new_files)
                # Meme contrepartie que _playlist_import_worker (voir son
                # commentaire) : synchronise cache_master_gifs.dat.
                self.tkmod.append_to_master_gifs_cache(self._playlist_sd_root, name, new_files)
                results.append((name, copied, skipped, renamed, n_updated))
                print(f"[GUI] Copie '{name}' terminee : {copied} copies, {skipped} identiques ignores, {renamed} renommes, {n_updated} playlist(s) mise(s) a jour")
        except Exception as e:
            print(f"[GUI] Erreur copie de la selection : {e}")
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
        self.root.after(0, self._on_playlist_copy_pending_done, results, [t[0] for t in tasks])

    def _on_playlist_copy_pending_done(self, results: list, processed_names: list) -> None:
        """Les dossiers traites (au moins 1 fichier coche, voir le garde
        dans _on_playlist_copy_pending_clicked) ne sont plus "en attente"
        -- ils redeviennent des dossiers reels normaux. Demarre le
        clignotement de "Continuer" ICI (demande utilisateur) : plus au
        moment de l'entree en mode temporaire, seulement une fois qu'une
        copie reelle a effectivement eu lieu, pour indiquer quoi faire
        ensuite."""
        for name in processed_names:
            self._playlist_pending_folders.pop(name, None)
            self._playlist_invalidate_cache(name)
        self._refresh_playlist_folders()
        if self._playlist_browsed_folder in processed_names:
            self._refresh_playlist_files_for_folder(self._playlist_browsed_folder)
        self._playlist_refresh_temp_submode_ui()
        total_copied = sum(r[1] for r in results)
        if total_copied > 0:
            self._start_playlist_regen_cache_blink()
        if not results:
            return
        ui = self._get_ui_t()
        names = ", ".join(r[0] for r in results)
        total_skipped = sum(r[2] for r in results)
        total_renamed = sum(r[3] for r in results)
        total_updated = sum(r[4] for r in results)
        self._themed_info(
            ui["playlist_import_dialog_title"],
            ui["playlist_import_done_msg_multi"].format(
                n=len(results), names=names, copied=total_copied, skipped=total_skipped,
                renamed=total_renamed, n_updated=total_updated,
            ),
        )

    def _on_playlist_delete_gif_clicked(self) -> None:
        """Suppression basee directement sur les cases a cocher : un
        dossier dont la case "dossier entier" est cochee -> suppression
        du dossier complet ; un fichier coche individuellement (dossier
        associe non coche en entier) -> suppression de ce seul fichier.
        Les playlists qui referencaient les elements supprimes sont mises
        a jour automatiquement."""
        ui = self._get_ui_t()
        if self._is_processing():
            messagebox.showwarning(ui["msg_warning_title"], ui["processing_in_progress_msg"])
            return
        if self._playlist_sd_root is None:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_no_sd_msg"])
            return

        # var.get() == 1 explicitement : une valeur 2 (tristate/partiel)
        # est truthy en Python mais NE signifie PAS "dossier entier" --
        # utiliser une verite tronquee ici supprimerait par erreur tout
        # le dossier alors que seule une partie des fichiers est cochee.
        folders_to_delete = [f for f, var in self._playlist_folder_checked.items() if var.get() == 1]
        files_to_delete: dict = {}
        for (folder, fname), var in self._playlist_checked.items():
            if folder in folders_to_delete:
                continue
            if var.get():
                files_to_delete.setdefault(folder, []).append(fname)

        if not folders_to_delete and not files_to_delete:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_delete_gif_nothing_msg"])
            return

        # 2 versions du message d'alerte selon la localisation reelle des
        # dossiers coches (demande utilisateur) : carte SD reelle
        # (suppression definitive et destructive) vs dossier temporaire
        # local du mode Mode 1 (aucun impact sur la carte SD, ces fichiers
        # ne s'y trouvent pas encore) -- l'ancien message unique affirmait
        # a tort "de la carte SD" meme en mode temporaire.
        suffix = "temp" if getattr(self, "_playlist_temp_mode", False) else "sd"
        if not self._themed_yesno(
            ui[f"playlist_delete_gif_confirm_title_{suffix}"],
            ui[f"playlist_delete_gif_confirm_msg_{suffix}"],
        ):
            return

        self._start_worker(self._playlist_delete_gif_worker, args=(folders_to_delete, files_to_delete))

    def _playlist_delete_gif_worker(self, folders_to_delete: list, files_to_delete: dict) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            for folder in folders_to_delete:
                self.tkmod.delete_gif_folder(self._playlist_sd_root, folder)
                print(f"[GUI] Dossier supprime : {folder}")
            for folder, fnames in files_to_delete.items():
                for fname in fnames:
                    self.tkmod.delete_gif_file(self._playlist_sd_root, folder, fname)
                print(f"[GUI] {len(fnames)} fichier(s) supprime(s) dans {folder}")
        except Exception as e:
            print(f"[GUI] Erreur suppression : {e}")
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
        self.root.after(0, self._on_playlist_delete_gif_done, folders_to_delete, files_to_delete)

    def _on_playlist_delete_gif_done(self, folders_to_delete: list, files_to_delete: dict) -> None:
        # Contenu physique modifie sur disque -> cache memoire de session
        # perime pour ces dossiers (et purge des variables de coche
        # correspondant a des fichiers/dossiers desormais inexistants,
        # sinon _playlist_recompute_folder_state() continuerait a les
        # compter comme "coches").
        for folder in folders_to_delete:
            self._playlist_invalidate_cache(folder)
            self._playlist_folder_checked.pop(folder, None)
            for key in [k for k in self._playlist_checked if k[0] == folder]:
                self._playlist_checked.pop(key, None)
        for folder, fnames in files_to_delete.items():
            self._playlist_invalidate_cache(folder)
            for fname in fnames:
                self._playlist_checked.pop((folder, fname), None)
            if folder not in folders_to_delete:
                self._playlist_recompute_folder_state(folder)
        self._playlist_browsed_folder = None
        self._refresh_playlist_folders()
        self._clear_playlist_checklist()

    def _on_playlist_build_clicked(self) -> None:
        ui = self._get_ui_t()
        if self._is_processing():
            messagebox.showwarning(ui["msg_warning_title"], ui["processing_in_progress_msg"])
            return
        if self._playlist_sd_root is None:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_no_sd_msg"])
            return
        name = self._playlist_name_var.get().strip()
        if not name or name == "---":
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_name_required_msg"])
            self._playlist_name_combo.focus_set()
            return

        # var.get() == 1 explicitement (voir _on_playlist_delete_gif_clicked
        # ci-dessus pour l'explication du piege tristate=2).
        full_folders = [f for f, var in self._playlist_folder_checked.items() if var.get() == 1]
        custom_files = [
            (folder, fname) for (folder, fname), var in self._playlist_checked.items()
            if var.get() and folder not in full_folders
        ]

        if not full_folders and not custom_files:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_build_empty_msg"])
            return

        self._start_worker(self._playlist_build_worker, args=(name, full_folders, custom_files))

    def _playlist_build_worker(self, name: str, full_folders: list, custom_files: list) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        n_entries = 0
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            entries = self.tkmod.build_playlist_entries_from_folders(self._playlist_sd_root, full_folders)
            entries.extend(self.tkmod.build_playlist_entries_from_files(custom_files))
            n_entries = len(entries)
            self.tkmod.write_playlist(self._playlist_sd_root, name, entries, full_folders=full_folders or None)
            print(f"[GUI] Playlist '{name}' ecrite : {n_entries} entree(s), {len(full_folders)} dossier(s) complet(s)")
        except Exception as e:
            print(f"[GUI] Erreur construction playlist : {e}")
            n_entries = -1
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
        self.root.after(0, self._on_playlist_build_done, name, n_entries)

    def _on_playlist_build_done(self, name: str, n_entries: int) -> None:
        self._refresh_playlist_existing_list()
        self._playlist_name_combo.set(name)
        if n_entries < 0:
            return
        ui = self._get_ui_t()
        self._themed_info(ui["playlist_build_btn"], ui["playlist_build_done_msg"].format(n=n_entries))

    def _on_playlist_delete_clicked(self) -> None:
        ui = self._get_ui_t()
        name = self._playlist_name_var.get().strip()
        if not name or name == "---" or self._playlist_sd_root is None:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_name_required_msg"])
            return
        if not self._themed_yesno(ui["playlist_delete_confirm_title"], ui["playlist_delete_confirm_msg"].format(name=name)):
            return
        self.tkmod.delete_playlist(self._playlist_sd_root, name)
        self._playlist_name_var.set("---")
        self._refresh_playlist_existing_list()

    def _on_playlist_regen_cache_clicked(self) -> None:
        ui = self._get_ui_t()
        if self._is_processing():
            messagebox.showwarning(ui["msg_warning_title"], ui["processing_in_progress_msg"])
            return
        if self._playlist_sd_root is None:
            messagebox.showwarning(ui["msg_warning_title"], ui["playlist_no_sd_msg"])
            return

        if getattr(self, "_playlist_temp_mode", False):
            if getattr(self, "_playlist_pending_folders", None):
                # Dossiers ajoutes mais pas encore copies (sous-mode
                # "add", voir _on_playlist_copy_pending_clicked) --
                # avertit avant de les laisser de cote ; l'utilisateur
                # peut choisir de continuer quand meme (ils sont alors
                # simplement abandonnes, rien n'a ete ecrit sur disque
                # pour eux).
                pending_names = ", ".join(self._playlist_pending_folders.keys())
                if not self._themed_yesno(
                    ui["playlist_pending_not_copied_title"],
                    ui["playlist_pending_not_copied_msg"](len(self._playlist_pending_folders), pending_names),
                ):
                    return
                self._playlist_pending_folders = {}
                self._refresh_playlist_folders()
                self._playlist_refresh_temp_submode_ui()

            # Mode 1, mode temporaire : ce bouton ("Continuer") ne
            # regenere plus le cache lui-meme ici (redondant, voir
            # _continue_mode1_from_temp_playlist()) -- il sert seulement a
            # poursuivre le Mode 1. 1ere fois seulement (par entree en
            # mode temporaire) : propose facultativement de construire une
            # playlist maintenant a partir des GIFs ajoutes (demande
            # utilisateur -- "pas obligatoire, peut se faire apres coup
            # vu qu'on a 2 playlists de repli").
            if not getattr(self, "_mode1_playlist_proposal_answered", False):
                self._mode1_playlist_proposal_answered = True
                if self._themed_yesno(ui["mode1_build_playlist_q_title"], ui["mode1_build_playlist_q_msg"]):
                    # Sous-mode "playlist" : champ nom visible, texte/cadre
                    # d'explication adaptes, bouton "Construire la playlist"
                    # restaure (voir _playlist_refresh_temp_submode_ui).
                    self._playlist_temp_submode = "playlist"
                    self._playlist_refresh_temp_submode_ui()
                    return  # laisse l'utilisateur construire ; il re-cliquera "Continuer" une fois pret
            self._continue_mode1_from_temp_playlist()
            return

        self._start_worker(self._playlist_regen_cache_worker, args=(self._playlist_sd_root,))

    def _playlist_regen_cache_worker(self, sd_root: Path) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        result: Optional[Path] = None
        n_folders = 0
        n_entries = 0
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            n_folders = len(self.tkmod.list_playlist_gif_folders(sd_root))
            result = self.tkmod.regenerate_playlist_gifs_cache(sd_root)
            try:
                n_entries = len([l for l in result.read_text(encoding="utf-8").splitlines() if l.strip()])
            except OSError:
                n_entries = 0
        except Exception as e:
            print(f"[GUI] Erreur regeneration cache playlist : {e}")
            result = None
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

        self.root.after(0, self._on_playlist_regen_cache_done, result, n_folders, n_entries)

    def _on_playlist_regen_cache_done(self, result: Optional[Path], n_folders: int, n_entries: int) -> None:
        if result is None:
            return
        ui = self._get_ui_t()
        self._themed_info(
            ui["playlist_regen_cache_btn"].replace("\n", " "),
            ui["playlist_regen_done_msg"].format(nf=n_folders, ne=n_entries),
        )
        # Enchainement mode temporaire (banque de GIFs, Mode 1) : garde
        # derriere getattr(..., False) -- zero changement de comportement
        # pour un usage normal de cet onglet (carte SD reelle).
        if getattr(self, "_playlist_temp_mode", False):
            self._exit_playlist_temp_mode()
            self.nb_top.select(self.tab_main)
            cb, self._mode1_deferred_launch_cb = self._mode1_deferred_launch_cb, None
            if cb:
                cb()

    def _build_logs_tab(self, parent: tk.Frame) -> None:
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])

        controls = tk.Frame(parent, bg="#F3F3F3", bd=0)
        controls.pack(fill="x", padx=10, pady=(10, 6))

        # width=10 fixe : meme correctif/meme raison que _build_progress_frame
        # (2026-08-05, demande utilisateur -- boutons non uniformes, visible
        # simultanement avec ceux du cadre Progression sur l'onglet Logs).
        BTN_W = 10
        self.btn_pause = tk.Button(
            controls,
            text=ui["btn_pause"],
            command=self._on_pause_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_pause.grid(row=0, column=0, padx=6, pady=4, sticky="w")

        self.btn_resume = tk.Button(
            controls,
            text=ui["btn_resume"],
            command=self._on_resume_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_resume.grid(row=0, column=1, padx=6, pady=4, sticky="w")

        self.btn_skip = tk.Button(
            controls,
            text=ui["btn_skip"],
            command=self._on_skip_clicked,
            bg="#FF5C5C",
            fg="black",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_skip.grid(row=0, column=2, padx=6, pady=4, sticky="w")

        self.btn_stop = tk.Button(
            controls,
            text=ui["btn_stop"],
            command=self._on_stop_clicked,
            bg="#B100FF",
            fg="white",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_stop.grid(row=0, column=3, padx=6, pady=4, sticky="w")

        # ── Sélecteur de niveau de log ──
        filter_frame = tk.Frame(parent, bg="#F3F3F3")
        filter_frame.pack(fill="x", padx=10, pady=(0, 4))

        self.log_level_lbl = tk.Label(
            filter_frame,
            text=ui["logs_level_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        self.log_level_lbl.pack(side="left")

        self.log_level_var = tk.StringVar(value=ui["logs_level_warn_err"])
        self.log_level_combo = ttk.Combobox(
            filter_frame,
            textvariable=self.log_level_var,
            values=[ui["logs_level_all"], ui["logs_level_warn_err"], ui["logs_level_err"]],
            state="readonly",
            width=18,
        )
        self.log_level_combo.pack(side="left", padx=(5, 0))
        self.log_level_combo.bind(
            "<<ComboboxSelected>>", lambda e: self._rebuild_log_display(go_end=True)
        )

        # Compteur de lignes de log
        self.log_count_lbl = tk.Label(
            filter_frame, text="", bg="#F3F3F3", fg="#666666", font=("TkDefaultFont", 8)
        )
        self.log_count_lbl.pack(side="right")

        self.logs_details_title_lbl = tk.Label(
            parent,
            text=ui["logs_details_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self.logs_details_title_lbl.pack(anchor="w", padx=10, pady=(2, 6))

        # Make logs area occupy most available space
        text_frame = tk.Frame(parent, bg="#F3F3F3")
        text_frame.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        self.text = tk.Text(
            text_frame,
            height=1,
            wrap="none",
            bg="white",
            fg="black",
            borderwidth=3,
            relief="solid",
        )
        # expand
        self.text.pack(side="left", fill="both", expand=True)

        self._log_scroll_y = tk.Scrollbar(
            text_frame, orient="vertical", command=self._on_log_yscroll
        )
        self._log_scroll_y.pack(side="right", fill="y")
        self.text.configure(yscrollcommand=self._log_scroll_y.set)

        # Bind molette de la souris pour détecter aussi le scroll
        def _on_mousewheel(event):
            try:
                if event.delta > 0:
                    self.text.yview_scroll(-3, "units")
                else:
                    self.text.yview_scroll(3, "units")
                self._on_log_yscroll("moveto", *self.text.yview())
            except Exception:
                pass
            return "break"

        self.text.bind("<MouseWheel>", _on_mousewheel)

        scroll_x = tk.Scrollbar(
            text_frame, orient="horizontal", command=self.text.xview
        )
        scroll_x.pack(side="bottom", fill="x")
        self.text.configure(xscrollcommand=scroll_x.set)

    def _build_params_tab(self, parent: tk.Frame) -> None:
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        self.params_language_title_lbl = tk.Label(
            parent,
            text=ui["language_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.params_language_title_lbl.pack(anchor="w", padx=10, pady=(12, 6))

        box = tk.Frame(parent, bg="#F3F3F3")
        box.pack(anchor="w", padx=10, pady=6)

        langs = [("fr", "Français"), ("en", "English"), ("es", "Español")]
        for i, (code, label) in enumerate(langs):
            rb = tk.Radiobutton(
                box,
                text=label,
                value=code,
                variable=self.lang_var,
                command=self._on_language_changed,
                bg="#F3F3F3",
                fg="black",
                activebackground="#E7E7E7",
                font=("TkDefaultFont", 10, "bold"),
            )
            rb.grid(row=i, column=0, sticky="w", pady=2)

        tk.Label(
            box,
            text="Theme / Theme / Tema",
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        ).grid(row=len(langs) + 1, column=0, sticky="w", pady=(12, 4))
        self._theme_combobox = ttk.Combobox(
            box,
            textvariable=self._theme_var,
            values=self._get_theme_list(),
            state="readonly",
            width=24,
        )
        self._theme_combobox.grid(row=len(langs) + 2, column=0, sticky="w", pady=2)
        self._theme_combobox.bind("<<ComboboxSelected>>", self._on_theme_selected)

        # Version Recalbox (profil logo/thumbnail/image, Mode 1/3/8) --
        # meme variable partagee que les panneaux Mode 1/3/8 : la modifier
        # ici la modifie partout (et persiste dans RecalBoxDMD_prefs.json).
        self._params_profile_lbl = tk.Label(
            box,
            text=ui["mode1_profile_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self._params_profile_lbl.grid(row=len(langs) + 3, column=0, sticky="w", pady=(12, 4))
        self._params_profile_combobox = ttk.Combobox(
            box,
            textvariable=self._mode1_profile_var,
            values=list(self.tkmod.RECALBOX_PROFILES.keys()),
            state="readonly",
            width=24,
        )
        self._params_profile_combobox.grid(row=len(langs) + 4, column=0, sticky="w", pady=2)
        self._params_profile_combobox.bind(
            "<<ComboboxSelected>>", self._on_mode1_profile_selected
        )

        # Seuil flag "L" (build_systems_cache(), v45) -- affine selon la
        # vitesse reelle de la carte SD de l'utilisateur le nombre de
        # fichiers convertis au-dela duquel un systeme est marque "lent".
        self._params_slow_threshold_lbl = tk.Label(
            box,
            text=ui["slow_threshold_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self._params_slow_threshold_lbl.grid(row=len(langs) + 5, column=0, sticky="w", pady=(12, 4))
        self._params_slow_threshold_spin = ttk.Spinbox(
            box,
            from_=100,
            to=50000,
            increment=100,
            textvariable=self._slow_threshold_var,
            width=10,
            command=self._on_slow_threshold_changed,
        )
        self._params_slow_threshold_spin.grid(row=len(langs) + 6, column=0, sticky="w", pady=2)
        self._params_slow_threshold_spin.bind("<Return>", self._on_slow_threshold_changed)
        self._params_slow_threshold_spin.bind("<FocusOut>", self._on_slow_threshold_changed)
        self._params_slow_threshold_hint_lbl = tk.Label(
            box,
            text=ui["slow_threshold_hint"],
            bg="#F3F3F3",
            fg="#555555",
            font=("TkDefaultFont", 8),
            wraplength=260,
            justify="left",
        )
        self._params_slow_threshold_hint_lbl.grid(row=len(langs) + 7, column=0, sticky="w", pady=(2, 4))

    def _build_help_tab(self, parent: tk.Frame) -> None:
        ui = self._get_ui_t()
        self.tab_help_bg = "#F3F3F3"
        container = tk.Frame(parent, bg=self.tab_help_bg)
        container.pack(fill="both", expand=True, padx=10, pady=10)

        # Toolbar: titre + bouton navigateur
        toolbar = tk.Frame(container, bg=self.tab_help_bg)
        toolbar.pack(fill="x", anchor="w")

        self.help_title_lbl = tk.Label(
            toolbar,
            text=ui["tab_help"],
            bg=self.tab_help_bg,
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.help_title_lbl.pack(side="left")

        self.help_open_browser_btn = tk.Button(
            toolbar,
            text=ui["help_open_browser_btn"],
            command=self._open_help_in_browser,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=8,
            pady=2,
            font=("TkDefaultFont", 9, "bold"),
        )
        self.help_open_browser_btn.pack(side="right", padx=(10, 0))

        text_frame = tk.Frame(container, bg=self.tab_help_bg)
        text_frame.pack(fill="both", expand=True, pady=(8, 0))

        scroll_y = tk.Scrollbar(text_frame, orient="vertical")
        scroll_y.pack(side="right", fill="y")

        self.help_text = tk.Text(
            text_frame,
            wrap="word",
            bg="white",
            fg="black",
            borderwidth=3,
            relief="solid",
            yscrollcommand=scroll_y.set,
        )
        self.help_text.pack(side="left", fill="both", expand=True)
        scroll_y.configure(command=self.help_text.yview)
        # Bloquer la saisie clavier sans désactiver le widget (pour garder tag_bind des liens)
        self.help_text.bind("<Key>", lambda e: "break")

        self._refresh_help_tab_content()

    def _refresh_help_tab_content(self) -> None:
        """Affiche le README.md avec rendu markdown via la bibliothèque standard."""
        if not getattr(self, "help_text", None):
            return

        lang = (
            getattr(self, "lang_var", None).get()
            if getattr(self, "lang_var", None)
            else "fr"
        )
        if lang == "en":
            readme_name = "README.md"
        elif lang == "es":
            readme_name = "README.es.md"
        else:
            readme_name = "README.fr.md"

        if getattr(self, "help_title_lbl", None):
            self.help_title_lbl.config(text=f"{self._get_ui_t()['tab_help']} ({readme_name})")

        base_dir = Path(getattr(sys, "_MEIPASS", Path(__file__).parent))
        readme_path = base_dir / readme_name
        try:
            readme_text = readme_path.read_text(encoding="utf-8", errors="replace")
        except Exception as e:
            readme_text = f"Impossible de lire {readme_name} : {e}"

        md_renderer.render_markdown_in_text(
            self.help_text,
            readme_text,
            on_external_link=lambda url: webbrowser.open_new_tab(url),
            on_anchor_link=self._help_scroll_to_anchor,
        )

    def _help_scroll_to_anchor(self, anchor: str) -> None:
        """Scroll vers l'ancre donnee.
        Le widget est en state=disabled (pour bloquer la saisie).
        On passe en normal le temps de see(), puis on restaure disabled.
        """
        import unicodedata as _ud

        def _norm(s):
            """Normalise : minuscules, enleve accents, garde [a-z0-9 espaces]."""
            n = _ud.normalize("NFKD", s.lower().replace("-", " ").replace("_", " "))
            n = "".join(c for c in n if not _ud.combining(c))
            n = "".join(c if c.isalnum() or c == " " else " " for c in n)
            return " ".join(n.split())

        def _scroll_to(pos):
            """Scroll to position, handling disabled state.
            see() doit etre suivi d'un cycle event loop pour prendre effet,
            donc on repasse en disabled via after_idle()."""
            tw = self.help_text
            was_disabled = tw.cget("state") == "disabled"
            if was_disabled:
                tw.configure(state="normal")
            tw.see(pos)
            if was_disabled:
                tw.after_idle(lambda t=tw: t.configure(state="disabled"))
                # Deja repasser en normal ici pour que le clic suivant
                # sur le tag_bind puisse re-entrer dans cette fonction
                # sans ambiguité

        try:
            tw = self.help_text
            anchor_norm = _norm(anchor)

            # 1) Mark exact
            if ("anchor_" + anchor) in tw.mark_names():
                _scroll_to("anchor_" + anchor)
                return

            # 2) Mark normalise
            for mn in tw.mark_names():
                if mn.startswith("anchor_"):
                    mid = mn[7:]
                    if _norm(mid) == anchor_norm:
                        _scroll_to(mn)
                        return

            # 3) search()
            for q in (anchor, anchor.replace("-", " ").replace("_", " ")):
                pos = tw.search(q, "1.0", stopindex="end", nocase=True)
                if pos:
                    _scroll_to(pos)
                    return

            # 4) Ligne par ligne
            content = tw.get("1.0", "end")
            lines = content.split("\n")
            for i, line in enumerate(lines):
                line_norm = _norm(line)
                if anchor_norm and anchor_norm in line_norm:
                    _scroll_to(f"{i + 1}.0")
                    return

            # 5) Fallback partiel
            for i, line in enumerate(lines):
                line_norm = _norm(line)
                if line_norm and (
                    line_norm.startswith(anchor_norm)
                    or anchor_norm.startswith(line_norm)
                ):
                    _scroll_to(f"{i + 1}.0")
                    return

            # 6) Debut
            _scroll_to("1.0")
        except Exception:
            pass

    def _open_help_in_browser(self) -> None:
        """Ouvre le README.md correspondant à la langue dans le navigateur."""
        lang = self.lang_var.get()
        if lang == "en":
            readme_name = "README.md"
        elif lang == "es":
            readme_name = "README.es.md"
        else:
            readme_name = "README.fr.md"
        base_dir = Path(getattr(sys, "_MEIPASS", Path(__file__).parent))
        readme_path = base_dir / readme_name
        try:
            webbrowser.open_new_tab(str(readme_path.resolve()))
        except Exception as e:
            messagebox.showerror(
                self._get_ui_t()["msg_error_title"],
                f"Impossible d'ouvrir {readme_name} : {e}",
            )

    def _build_mode_area(self, parent: tk.Frame) -> None:
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        outer = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=10, pady=10)
        outer.pack(fill="both", expand=True, padx=10, pady=(10, 8))

        outer.grid_columnconfigure(0, weight=1)
        # minsize=290 : reserve la largeur de la colonne "middle" meme
        # quand son widget est masque via grid_remove() (elle perdrait
        # sinon tout son espace, decalant "right" -- cf. v14).
        outer.grid_columnconfigure(1, weight=1, minsize=290)
        outer.grid_columnconfigure(2, weight=1)
        outer.grid_rowconfigure(0, weight=1)

        # Left column: mode + ROMs path + start
        left = tk.Frame(outer, bg="#F3F3F3")
        # sticky="nsw" (pas de "e") : la colonne s'etire verticalement mais
        # PAS horizontalement -- si une colonne voisine est masquee
        # (grid_remove), le poids de grille ne doit pas faire gonfler
        # cette colonne et changer la largeur de path_box/Demarrer/Quitter.
        left.grid(row=0, column=0, sticky="nsw", padx=(0, 10))

        self.mode_title_lbl = tk.Label(
            left,
            text=ui["mode_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_title_lbl.pack(anchor="w")

        self.mode_var = tk.StringVar(value="1")
        self._mode_radios: dict[str, tk.Radiobutton] = {}

        # Modes 2 a 8 : description d'origine + "Marche a suivre" (sequence
        # de boutons reelle, cf. _on_mode_changed/_on_start_clicked) pour
        # occuper l'espace vertical libere par les titres 1 ligne des radios
        # et par l'ancrage en bas du panneau Mode 8 (voir right_adv).
        self._detail_templates = {
            "fr": {
                "1": "Le mode AUTO extrait les images à partir de vos gamelists, convertit les PNG en 128x32 (raw565) et les GIF en raw565pack/meta, construit le cache, télécharge les images par défaut et génère systems_cache.dat. Installe aussi les scripts Recalbox et transmet la langue au DMD, en tout début de pipeline.\n\nImportant : choisissez d'abord la « Version Recalbox » ci-dessous (10.x / 9.x / legacy) — elle détermine quelle balise du gamelist.xml est utilisée (logo/thumbnail/image). Cliquez « Comment scraper ? » pour savoir quoi cocher dans l'onglet Scraper de Recalbox.\n\nMarche à suivre :\n1. « Choisir dossier ROMs » (détection des systèmes automatique)\n2. Sélectionnez les systèmes à traiter\n3. « Démarrer »",
                "2": "Le mode 2 télécharge uniquement les images situées dans “systems/_defaults” depuis GitHub. Il ne réalise aucune extraction ni conversion d’images.\n\nMarche à suivre :\n1. Cliquez directement sur « Démarrer ».\n2. Choisissez la langue des images système/genres (EN/FR/ES, avec aperçu comparatif) — les genres pas encore traduits dans la langue choisie restent en anglais.\n3. La galerie d'image de secours s'ouvre systématiquement — choisissez-en une, ou fermez sans choisir pour revenir au visuel par défaut du projet.\nAucun dossier ROMs ni sélection de systèmes n'est nécessaire (bouton désactivé).",
                "3": "Le mode 3 récupère exclusivement les images présentes dans votre dossier ROMS, en se basant sur le fichier gamelist.xml.\n\nImportant : choisissez d'abord la « Version Recalbox » ci-dessous (10.x / 9.x / legacy) — elle détermine quelle balise du gamelist.xml est utilisée. Cliquez « Comment scraper ? » pour savoir quoi cocher dans l'onglet Scraper de Recalbox.\n\nMarche à suivre :\n1. « Choisir dossier ROMs »\n2. « Détection des systèmes (gamelist.xml) »\n3. Sélectionnez les systèmes à traiter\n4. « Démarrer »",
                "4": "Le mode 4 convertit les images PNG en raw565 et les GIF en raw565pack accompagnés de méta-données. Cette conversion concerne uniquement les formats raw.\n\nMarche à suivre :\n1. « Choisir dossier IMAGES »\n2. « Sélection des dossiers images »\n3. Sélectionnez les dossiers à convertir\n4. « Démarrer » (un dossier de sortie vous sera demandé)",
                "5": "Le mode 5 convertit les images PNG et raw565 pour les redimensionner en 128x32 pixels.\n\nMarche à suivre :\n1. « Choisir dossier IMAGES »\n2. « Sélection des dossiers images »\n3. Sélectionnez les dossiers à convertir\n4. « Démarrer » (un dossier de sortie vous sera demandé)",
                "6": "Le mode 6 génère uniquement le fichier games_cache.bin, qui correspond au cache des jeux.\n\nMarche à suivre :\nExécutez d'abord le Mode 3 (extraction gamelist.xml) si ce n'est pas déjà fait, puis revenez ici et cliquez directement sur « Démarrer » — aucun dossier à choisir.",
                "7": "Le mode 7 génère uniquement le fichier systems_cache.dat, qui représente l’index des systèmes.\n\nMarche à suivre :\nExécutez d'abord le Mode 2 (téléchargement _defaults) si ce n'est pas déjà fait, puis revenez ici et cliquez directement sur « Démarrer » — aucun dossier à choisir.",
                "8": "Le mode 8 vérifie les images manquantes en parcourant les gamelist.xml du dossier ROMs. Le rapport liste les images absentes avec le chemin attendu selon le profil Recalbox sélectionné.\n\nMarche à suivre :\n1. « Choisir dossier ROMs »\n2. Choisissez la « Version Recalbox »\n3. « Lancer la vérification »\n4. « Ouvrir le rapport »\nOptionnel : « Comparer avec le support final » puis « Ouvrir le rapport final ».",
                "9": "Installe/met à jour les scripts utilisateur Recalbox (WiFi Recovery, Config Web, Reboot, Luminosité +10%/-10%, pont marquee) directement sur le partage réseau de la Recalbox (\\\\<ip>\\share), sans passer par le DMD.\n\nMarche à suivre :\n1. Vérifiez/saisissez l'adresse IP ou le nom réseau de la Recalbox (pré-rempli si détecté automatiquement ou déjà utilisé).\n2. « Installer / Mettre à jour »\n3. Sur la Recalbox : START > PARAMÈTRES AVANCÉS > SCRIPTS UTILISATEUR.",
                "10": "Choisissez l'image de secours (default.raw565) affichée quand aucune image spécifique n'est disponible pour un jeu ou un système. Action autonome et immédiate, sans dossier ROMs ni pipeline.\n\nMarche à suivre :\n1. « Choisir son image de secours »\n2. Sélectionnez une image de la galerie ou importez la vôtre.\nLe choix s'applique immédiatement au dossier de travail.",
                "11": "Le mode 11 télécharge uniquement le pack gratuit de 600 GIFs (thèmes variés) depuis GitHub dans /gifs/. Indépendant du Mode 2 (qui télécharge « _defaults »). Il ne réalise aucune extraction ni conversion d’images.\n\nPour un pack bien plus complet (pack ultimate, ~11000 animations pixel-perfect pour DMD), voir https://rpiteam.carrd.co/ et le forum Arcadia : https://www.neo-arcadia.com/forum/viewtopic.php?t=67065\n\nMarche à suivre :\n1. Cliquez directement sur « Démarrer ».\nAucun dossier ROMs ni sélection de systèmes n'est nécessaire (bouton désactivé).",
            },
            "en": {
                "1": "Auto Mode extracts images from your gamelists, converts PNG to 128x32 (raw565) and GIF to raw565pack/meta, builds the cache, downloads the default images and generates systems_cache.dat. Also installs the Recalbox scripts and sends the language to the DMD, right at the start of the pipeline.\n\nImportant: pick the \"Recalbox version\" below first (10.x / 9.x / legacy) — it determines which gamelist.xml tag is used (logo/thumbnail/image). Click \"How to scrape?\" to see exactly what to enable in Recalbox's Scraper tab.\n\nSteps:\n1. « Choose ROMs folder » (systems auto-detected)\n2. Select the systems to process\n3. « Start »",
                "2": "Mode 2: downloads “systems/_defaults” from GitHub only (no extraction or conversion).\n\nSteps:\n1. Click « Start » directly.\n2. Choose the system/genre images language (EN/FR/ES, with a comparison preview) — genres not yet translated into the chosen language stay in English.\n3. The fallback image gallery always opens — pick one, or close without choosing to revert to the project's default visual.\nNo ROMs folder or system selection needed (button disabled).",
                "3": "Mode 3: pulls images only from your ROM folder via gamelist.xml.\n\nImportant: pick the \"Recalbox version\" below first (10.x / 9.x / legacy) — it determines which gamelist.xml tag is used. Click \"How to scrape?\" to see what to enable in Recalbox's Scraper tab.\n\nSteps:\n1. « Choose ROMs folder »\n2. « Detect systems (gamelist.xml) »\n3. Select the systems to process\n4. « Start »",
                "4": "Mode 4: converts PNG → raw565 and GIF → raw565pack + meta (raw-only conversion).\n\nSteps:\n1. « Choose images folder »\n2. « Select image folders »\n3. Select the folders to convert\n4. « Start » (you'll be asked for an output folder)",
                "5": "Mode 5: converts PNG and raw565 images to 128x32.\n\nSteps:\n1. « Choose images folder »\n2. « Select image folders »\n3. Select the folders to convert\n4. « Start » (you'll be asked for an output folder)",
                "6": "Mode 6: generates only games_cache.bin (games cache).\n\nSteps:\nRun Mode 3 first (gamelist extraction) if not done yet, then come back and click « Start » directly — no folder to choose.",
                "7": "Mode 7: generates only systems_cache.dat (systems index).\n\nSteps:\nRun Mode 2 first (_defaults download) if not done yet, then come back and click « Start » directly — no folder to choose.",
                "8": "Mode 8: checks missing images by scanning gamelist.xml in the ROMs folder. The report lists missing images with the expected path according to the selected Recalbox profile.\n\nSteps:\n1. « Choose ROMs folder »\n2. Pick the « Recalbox version »\n3. « Start check »\n4. « Open report »\nOptional: « Compare with final media » then « Open final report ».",
                "9": "Installs/updates the Recalbox user scripts (WiFi Recovery, Web Config, Reboot, Brightness +10%/-10%, marquee bridge) directly on the Recalbox network share (\\\\<ip>\\share), without going through the DMD.\n\nSteps:\n1. Check/enter the Recalbox IP address or network name (pre-filled if auto-detected or already used).\n2. « Install / Update »\n3. On the Recalbox: START > ADVANCED SETTINGS > USER SCRIPTS.",
                "10": "Choose the fallback image (default.raw565) shown when no specific image is available for a game or system. Standalone, immediate action, no ROMs folder or pipeline involved.\n\nSteps:\n1. « Choose your fallback image »\n2. Pick an image from the gallery or import your own.\nThe choice is applied immediately to the working folder.",
                "11": "Mode 11 downloads only the free pack of 600 GIFs (assorted themes) from GitHub into /gifs/. Independent from Mode 2 (which downloads \"_defaults\"). No extraction or conversion.\n\nFor a much larger pack (ultimate pack, ~11,000 pixel-perfect DMD animations), see https://rpiteam.carrd.co/ and the Arcadia forum: https://www.neo-arcadia.com/forum/viewtopic.php?t=67065\n\nSteps:\n1. Click « Start » directly.\nNo ROMs folder or system selection needed (button disabled).",
            },
            "es": {
                "1": "Modo 1 (AUTO): extrae imágenes desde tus gamelists, convierte PNG a 128x32 (raw565) y GIF a raw565pack/meta, crea la caché, descarga las imágenes por defecto y genera systems_cache.dat. También instala los scripts de Recalbox y transmite el idioma al DMD, al principio del proceso.\n\nImportante: elige primero la « Versión de Recalbox » abajo (10.x / 9.x / legacy) — determina la etiqueta del gamelist.xml usada (logo/thumbnail/image). Haz clic en « Cómo hacer el scrape? » para saber qué activar en la pestaña Scraper de Recalbox.\n\nPasos:\n1. « Elegir carpeta ROMs » (detección de sistemas automática)\n2. Seleccione los sistemas a procesar\n3. « Iniciar »",
                "2": "Modo 2: descarga “systems/_defaults” desde GitHub solo (sin extracción ni conversión).\n\nPasos:\n1. Haga clic directamente en « Iniciar ».\n2. Elija el idioma de las imágenes de sistemas/géneros (EN/FR/ES, con vista previa comparativa) — los géneros aún no traducidos al idioma elegido se muestran en inglés.\n3. La galería de imagen de respaldo se abre siempre — elija una, o ciérrela sin elegir para volver al visual predeterminado del proyecto.\nNo se necesita carpeta ROMs ni selección de sistemas (botón desactivado).",
                "3": "Modo 3: extrae solo imágenes desde tu carpeta ROMs vía gamelist.xml.\n\nImportante: elige primero la « Versión de Recalbox » abajo (10.x / 9.x / legacy) — determina la etiqueta del gamelist.xml usada. Haz clic en « Cómo hacer el scrape? » para saber qué activar en la pestaña Scraper de Recalbox.\n\nPasos:\n1. « Elegir carpeta ROMs »\n2. « Detectar sistemas (gamelist.xml) »\n3. Seleccione los sistemas a procesar\n4. « Iniciar »",
                "4": "Modo 4: convierte PNG → raw565 y GIF → raw565pack + meta (conversión “raw-only”).\n\nPasos:\n1. « Elegir carpeta de imágenes »\n2. « Selección de carpetas de imágenes »\n3. Seleccione las carpetas a convertir\n4. « Iniciar » (se le pedirá una carpeta de salida)",
                "5": "Modo 5: convierte las imágenes PNG y raw565 a 128x32.\n\nPasos:\n1. « Elegir carpeta de imágenes »\n2. « Selección de carpetas de imágenes »\n3. Seleccione las carpetas a convertir\n4. « Iniciar » (se le pedirá una carpeta de salida)",
                "6": "Modo 6: genera solo games_cache.bin (caché de juegos).\n\nPasos:\nEjecute primero el Modo 3 (extracción gamelist) si no lo ha hecho, luego vuelva aquí y haga clic directamente en « Iniciar » — no hay que elegir carpeta.",
                "7": "Modo 7: genera solo systems_cache.dat (índice de sistemas).\n\nPasos:\nEjecute primero el Modo 2 (descarga _defaults) si no lo ha hecho, luego vuelva aquí y haga clic directamente en « Iniciar » — no hay que elegir carpeta.",
                "8": "Modo 8: verifica las imagenes faltantes escaneando los gamelist.xml en la carpeta ROMs. El informe enumera las imagenes faltantes con la ruta esperada segun el perfil de Recalbox seleccionado.\n\nPasos:\n1. « Elegir carpeta ROMs »\n2. Elija la « Versión de Recalbox »\n3. « Iniciar verificación »\n4. « Abrir informe »\nOpcional: « Comparar con el soporte final » luego « Abrir informe final ».",
                "9": "Instala/actualiza los scripts de usuario de Recalbox (WiFi Recovery, Config Web, Reboot, Brillo +10%/-10%, puente marquee) directamente en el recurso compartido de red de la Recalbox (\\\\<ip>\\share), sin pasar por el DMD.\n\nPasos:\n1. Compruebe/introduzca la IP o el nombre de red de la Recalbox (rellenado automáticamente si se detecta o ya se usó).\n2. « Instalar / Actualizar »\n3. En la Recalbox: START > CONFIGURACIÓN AVANZADA > SCRIPTS DE USUARIO.",
                "10": "Elija la imagen de respaldo (default.raw565) que se muestra cuando no hay una imagen especifica disponible para un juego o sistema. Accion autonoma e inmediata, sin carpeta ROMs ni proceso.\n\nPasos:\n1. « Elegir su imagen de respaldo »\n2. Seleccione una imagen de la galeria o importe la suya.\nLa eleccion se aplica de inmediato a la carpeta de trabajo.",
                "11": "El modo 11 descarga solo el pack gratuito de 600 GIFs (temas variados) desde GitHub en /gifs/. Independiente del Modo 2 (que descarga «_defaults»). Sin extracción ni conversión.\n\nPara un pack mucho más completo (pack ultimate, ~11000 animaciones pixel-perfect para DMD), consulte https://rpiteam.carrd.co/ y el foro Arcadia: https://www.neo-arcadia.com/forum/viewtopic.php?t=67065\n\nPasos:\n1. Haga clic directamente en « Iniciar ».\nNo se necesita carpeta ROMs ni selección de sistemas (botón desactivado).",
            },
        }

        # Titles from toolkit translations
        modes = [
            ("1", self.tkmod.tr("mode1_title")),
        ]

        for m, label in modes:
            rb = tk.Radiobutton(
                left,
                text=label,
                variable=self.mode_var,
                value=m,
                bg="#F3F3F3",
                fg="black",
                activebackground="#E7E7E7",
                font=("TkDefaultFont", 10, "bold"),
                wraplength=350,
                justify="left",
                highlightthickness=0,
                takefocus=0,
                command=self._on_mode_changed,
            )
            rb.pack(anchor="w", pady=2)
            self._mode_radios[m] = rb

        # Radios invisibles pour modes 2-11 (garder mode_var stable entre onglets)
        for hidden_m in ("2", "3", "4", "5", "6", "7", "8", "9", "10", "11"):
            rb_hidden = tk.Radiobutton(
                left, variable=self.mode_var, value=hidden_m,
                state="disabled", takefocus=0,
            )
            rb_hidden.pack_forget()

        # ── Mode 1 : version Recalbox (scrape marquee/logo) -- colonne
        # gauche, entre le mode et le dossier ROMs (pour ne pas allonger la
        # colonne droite et masquer le panneau copie SD en dessous).
        # Toujours visible (seul le Mode 1 est propose ici).
        # Pas de bouton "Choisir son image de secours" ici : le choix est
        # applique immediatement au dossier temporaire des qu'il est fait
        # (voir _apply_choice dans _on_default_image_picker_clicked), donc
        # inutile de dupliquer le bouton en Mode 1 -- il reste uniquement en
        # Mode 2 (voir _build_mode_area_advanced), pour ne pas avoir a
        # introduire un mode dedie supplementaire.
        self._mode1_profile_frame = self._build_recalbox_profile_panel(left)
        self._mode1_profile_frame.pack(fill="x", pady=(12, 0))
        self._mode1_clean_btn = self._mode1_profile_frame.clean_btn

        # Espaceur qui pousse les boutons en bas
        spacer = tk.Frame(left, bg="#F3F3F3")
        spacer.pack(fill="both", expand=True)

        # ROMs folder
        path_box = tk.Frame(left, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8)
        path_box.pack(fill="x", pady=(12, 0))

        self.roms_path_var = tk.StringVar(value=str(self.sd_dir / "systems"))

        self.btn_pick_roms = tk.Button(
            path_box,
            text=ui["roms_pick_btn"],
            command=self._pick_roms_directory,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_pick_roms.pack(fill="x")

        tk.Label(
            path_box,
            textvariable=self.roms_path_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9),
            wraplength=350,
            justify="left",
        ).pack(anchor="w", pady=(8, 0))

        self.btn_start = tk.Button(
            left,
            text=ui["start_btn"],
            command=self._on_start_clicked,
            bg="#00D084",
            fg="black",
            bd=2,
            relief="solid",
            padx=12,
            pady=8,
            font=("TkDefaultFont", 12, "bold"),
        )
        self.btn_start.pack(fill="x", pady=(12, 0))
        self.btn_start._fixed_theme_colors = ("#00D084", "#000000")

        self.btn_quit_app = tk.Button(
            left,
            text=(
                self.tkmod.tr("main_opt_quit") if hasattr(self.tkmod, "tr") else "Quit"
            ),
            command=self._on_quit_app_clicked,
            bg="#FF5C5C",
            fg="black",
            bd=2,
            relief="solid",
            padx=12,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_quit_app.pack(fill="x", pady=(8, 0))
        self.btn_quit_app._fixed_theme_colors = ("#FF5C5C", "#000000")

        # Middle column: systems detection (only for modes 1/2)
        self.middle = tk.Frame(outer, bg="#F3F3F3", bd=0)
        self.middle.grid(row=0, column=1, sticky="nsew", padx=10)

        self.btn_detect_systems = tk.Button(
            self.middle,
            text=ui["detect_systems_btn"],
            command=self._on_detect_systems_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=8,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_detect_systems.pack(fill="x")

        self.systems_to_process_lbl = tk.Label(
            self.middle,
            text=ui["systems_to_process_lbl"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self.systems_to_process_lbl.pack(anchor="w", pady=(10, 6))

        box = tk.Frame(self.middle, bg="#F3F3F3")
        box.pack(fill="both", expand=True)
        self.sys_list_box = box

        self.sys_list = tk.Listbox(
            box,
            selectmode="multiple",
            height=7,
            width=28,
            bg="white",
            fg="black",
            selectbackground="#1565C0",
            selectforeground="white",
            borderwidth=3,
            relief="solid",
        )
        self.sys_list.pack(side="left", fill="both", expand=True)

        scroll = tk.Scrollbar(box, orient="vertical", command=self.sys_list.yview)
        scroll.pack(side="right", fill="y")
        self.sys_list.configure(yscrollcommand=scroll.set)
        self.sys_list.bind(
            "<<ListboxSelect>>",
            self._on_systems_listbox_select_changed,
        )
        self.sys_list.bind(
            "<Button-1>",
            self._on_sys_list_button1_clicked,
        )

        # Right column: mode details
        self.right = tk.Frame(outer, bg="#F3F3F3")
        # sticky="nsw" (pas de "e") : meme raison que pour "left" -- si une
        # colonne voisine est masquee (grid_remove), le poids de grille ne
        # doit pas faire gonfler cette colonne. Ici l'effet est pire qu'un
        # simple ecart de largeur : le decoupage de fond (slice) est
        # capture a une largeur donnee, un changement de largeur decale le
        # crop et laisse apparaitre des bandes blanches.
        self.right.grid(row=0, column=2, sticky="nsw", padx=(10, 0))
        # Spacer push le panneau mode6 en bas
        self.right.grid_rowconfigure(99, weight=1)

        self.mode_detail_title_lbl = tk.Label(
            self.right,
            text=ui["mode_detail_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_detail_title_lbl.pack(anchor="w", pady=(0, 6))

        # 2026-08-11 -- Text (au lieu de Label+StringVar) pour rendre
        # cliquables les liens du detail Mode 2/11 (pack "ultimate", voir
        # _update_mode_desc()/_insert_autolink_text()). bd=0 (pas de carte
        # visible, contrairement a _make_linked_text() par defaut) pour
        # rester visuellement proche du Label d'origine, qui se fondait
        # dans le panneau.
        self.mode_desc_label = tk.Text(
            self.right, wrap="word", bg="#F3F3F3", fg="black",
            font=("TkDefaultFont", 10), width=34, height=1,
            bd=0, highlightthickness=0, cursor="arrow", padx=0, pady=0,
        )
        self.mode_desc_label._theme_as_panel = True  # voir RecalBoxDMD_themes.py _walk_and_apply()
        self.mode_desc_label.bind("<Key>", lambda e: "break")
        self.mode_desc_label.pack(anchor="w", fill="x")

        # ── Mode 6 panel (hidden until previous pipeline is finished)
        # Ancré en bas de right pour ne pas être poussé par le texte variable.
        self._mode6_ui_frame = self._build_mode6_panel(self.right)
        self._mode6_ui_frame.pack(side="bottom", fill="x", pady=(6, 0))
        self._mode6_ui_frame.pack_forget()
        self._mode6_panel_title_lbl = self._mode6_ui_frame.panel_title_lbl
        self._mode6_drives_title_lbl = self._mode6_ui_frame.drives_title_lbl
        self._mode6_drive_list = self._mode6_ui_frame.drive_list
        self._mode6_btn = self._mode6_ui_frame.btn
        self._mode6_explore_output_btn = self._mode6_ui_frame.explore_btn

        self._on_mode_changed()

    def _build_mode_area_advanced(self, parent: tk.Frame) -> None:
        """Onglet Avancé : modes 2 a 7."""
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        outer = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=10, pady=10)
        outer.pack(fill="both", expand=True, padx=10, pady=(10, 8))
        outer.grid_columnconfigure(0, weight=1)
        # minsize=290 : reserve la largeur de la colonne "middle_adv" meme
        # quand son widget est masque via grid_remove() (modes 2/6/7) --
        # sans cela, right_adv se decalait de ~110px vers la gauche (cf.
        # v14 : rootx 711 vs ~808 mesure).
        outer.grid_columnconfigure(1, weight=1, minsize=290)
        outer.grid_columnconfigure(2, weight=1)
        outer.grid_rowconfigure(0, weight=1)
        left = tk.Frame(outer, bg="#F3F3F3")
        # sticky="nsw" : le "s" (avec "n") est necessaire pour que le
        # spacer plus bas (avant path_box) puisse reellement pousser
        # path_box/Demarrer/Quitter vers le bas de la colonne. Le "e" est
        # volontairement absent : mesure -> quand middle_adv est masque
        # (modes 2/6/7, grid_remove), le poids de grille redistribue sa
        # largeur aux colonnes restantes et "left" gonflait de 363px de
        # contenu naturel a ~480px de rendu reel, alors que path_box/
        # Demarrer/Quitter (fill="x") suivaient -- largeur incoherente
        # d'un mode a l'autre. Sans "e", la colonne garde sa largeur
        # naturelle quel que soit l'etat des colonnes voisines.
        left.grid(row=0, column=0, sticky="nsw", padx=(0, 10))
        self.mode_title_lbl_adv = tk.Label(
            left,
            text=ui["mode_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_title_lbl_adv.pack(anchor="w")
        # Label pour messages info/erreur (modes 2, 6, 7)
        self._adv_msg_label = tk.Label(
            left,
            text="",
            bg="#F3F3F3",
            fg="#666666",
            font=("TkDefaultFont", 9),
            wraplength=320,
            justify="left",
        )
        # Ne pas pack par défaut - packé par _on_mode_changed
        self._mode_radios_adv: dict[str, tk.Radiobutton] = {}
        # Mode 1 invisible (pour que mode_var reste stable en changeant d'onglet)
        self._mode1_hidden_rb = tk.Radiobutton(
            left, variable=self.mode_var, value="1",
            state="disabled", takefocus=0,
        )
        self._mode1_hidden_rb.pack_forget()
        # Titres courts (1 seule ligne) dedies a cette colonne : mode2_title
        # etc. (partages avec le CLI via self.tkmod.tr) sont trop longs et
        # cassent sur 2 lignes de facon inegale d'une option a l'autre. Ces
        # cles GUI-only n'affectent pas les banners CLI.
        #
        # Accordeon thematique (2026-08-11, v43) -- remplace la liste plate
        # de 8 radios par 5 categories cliquables (une seule depliee a la
        # fois), pour gagner de la place dans cette colonne fixe (363px) et
        # pouvoir ajouter de futurs modes sans que la liste devienne
        # ingerable (ajouter un mode = l'ajouter sous sa categorie, le
        # nombre d'en-tetes visibles ne grandit pas). Un menu a survol
        # (fly-out) a ete explicitement ecarte par l'utilisateur : chaque
        # mode expose des parametres (choix de dossier, panneaux dedies)
        # qui doivent rester des widgets persistants dans right_adv, pas
        # transitoires. Reutilise le meme mecanisme pack()/pack_forget()
        # deja employe partout dans ce fichier (panneaux de detail par
        # mode, voir _on_mode_changed) -- risque le plus faible vis-a-vis
        # du decoupage d'image de fond par theme.
        self._accordion_categories: list[tuple[str, str, list[str]]] = [
            ("github", "accordion_cat_github", ["2", "11"]),
            ("gamelist", "accordion_cat_gamelist", ["3", "8"]),
            ("images", "accordion_cat_images", ["4", "5", "10"]),
            ("caches", "accordion_cat_caches", ["6", "7"]),
            ("scripts", "accordion_cat_scripts", ["9"]),
        ]
        mode_short_titles = {
            "2": ui["mode2_short_title"],
            "3": ui["mode3_short_title"],
            "4": ui["mode4_short_title"],
            "5": ui["mode5_short_title"],
            "6": ui["mode6_short_title"],
            "7": ui["mode7_short_title"],
            "8": ui["mode8_short_title"],
            "9": ui["mode9_short_title"],
            "10": ui["mode10_short_title"],
            "11": ui["mode11_short_title"],
        }
        self._accordion_headers: dict[str, tk.Label] = {}
        self._accordion_bodies: dict[str, tk.Frame] = {}
        self._accordion_open_category = (
            self._accordion_category_for_mode(self.mode_var.get())
            or self._accordion_categories[0][0]
        )
        for cat_key, cat_ui_key, cat_modes in self._accordion_categories:
            # Conteneur dedie par categorie, empaquete UNE SEULE FOIS ici,
            # dans l'ordre -- toujours visible, jamais reempaquete. Sans ca,
            # rouvrir une categorie plus tard (body.pack() appele apres
            # coup, au clic sur l'en-tete) placerait le corps a la FIN de
            # la liste d'empilement de "left" (Tkinter : un widget
            # re-pack() va toujours en dernier parmi les enfants de son
            # parent, pas a sa position "logique") -- bug reel observe :
            # tous les sous-menus s'ouvraient sous le bouton Quitter, deja
            # empaquete avant eux. En isolant chaque categorie dans son
            # propre petit conteneur, le dépli/repli du corps ne touche
            # que l'ordre interne a CE conteneur (2 enfants : en-tete
            # toujours au-dessus, corps en dessous), jamais l'ordre parmi
            # les enfants de "left".
            category_frame = tk.Frame(left, bg="#F3F3F3")
            category_frame.pack(anchor="w", fill="x")

            header = tk.Label(
                category_frame,
                text=self._accordion_header_text(cat_key, cat_ui_key, ui),
                bg="#E7E7E7",
                fg="black",
                font=("TkDefaultFont", 11, "bold"),
                anchor="w",
                cursor="hand2",
                padx=6,
                pady=4,
            )
            header.pack(anchor="w", fill="x", pady=(4, 0))
            header.bind("<Button-1>", lambda e, k=cat_key: self._on_accordion_toggle(k))
            self._accordion_headers[cat_key] = header

            body = tk.Frame(category_frame, bg="#F3F3F3")
            self._accordion_bodies[cat_key] = body
            for m in cat_modes:
                rb = tk.Radiobutton(
                    body,
                    text=mode_short_titles[m],
                    variable=self.mode_var,
                    value=m,
                    bg="#F3F3F3",
                    fg="black",
                    activebackground="#E7E7E7",
                    font=("TkDefaultFont", 12, "bold"),
                    wraplength=280,
                    justify="left",
                    anchor="w",
                    highlightthickness=0,
                    takefocus=0,
                    command=self._on_mode_changed,
                )
                # fill="x"/anchor="w" : meme raisonnement que l'ancienne
                # liste plate (voir historique v-- ci-dessus) -- padx=(14,0)
                # indente les radios sous leur en-tete de categorie.
                rb.pack(anchor="w", fill="x", pady=2, padx=(14, 0))
                self._mode_radios_adv[m] = rb
            if cat_key == self._accordion_open_category:
                body.pack(anchor="w", fill="x")
        # Espaceur qui pousse les boutons en bas
        spacer = tk.Frame(left, bg="#F3F3F3")
        spacer.pack(fill="both", expand=True)
        path_box = tk.Frame(left, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8)
        path_box.pack(fill="x", pady=(12, 0))
        self.roms_path_var_adv = tk.StringVar(value=str(self.sd_dir / "systems"))
        self.btn_pick_roms_adv = tk.Button(
            path_box,
            text=ui["roms_pick_btn"],
            command=self._pick_roms_directory,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_pick_roms_adv.pack(fill="x")
        tk.Label(
            path_box,
            textvariable=self.roms_path_var_adv,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9),
            wraplength=350,
            justify="left",
        ).pack(anchor="w", pady=(8, 0))
        self.btn_start_adv = tk.Button(
            left,
            text=ui["start_btn"],
            command=self._on_start_clicked,
            bg="#00D084",
            fg="black",
            bd=2,
            relief="solid",
            padx=12,
            pady=8,
            font=("TkDefaultFont", 12, "bold"),
        )
        self.btn_start_adv.pack(fill="x", pady=(12, 0))
        self.btn_start_adv._fixed_theme_colors = ("#00D084", "#000000")
        self.btn_quit_app_adv = tk.Button(
            left,
            text=(
                self.tkmod.tr("main_opt_quit") if hasattr(self.tkmod, "tr") else "Quit"
            ),
            command=self._on_quit_app_clicked,
            bg="#FF5C5C",
            fg="black",
            bd=2,
            relief="solid",
            padx=12,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_quit_app_adv.pack(fill="x", pady=(8, 0))
        self.btn_quit_app_adv._fixed_theme_colors = ("#FF5C5C", "#000000")
        self.middle_adv = tk.Frame(outer, bg="#F3F3F3", bd=0)
        self.middle_adv.grid(row=0, column=1, sticky="nsew", padx=10)
        self.btn_detect_systems_adv = tk.Button(
            self.middle_adv,
            text=ui["detect_systems_btn"],
            command=self._on_detect_systems_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=8,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_detect_systems_adv.pack(fill="x")
        self.systems_to_process_lbl_adv = tk.Label(
            self.middle_adv,
            text=ui["systems_to_process_lbl"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self.systems_to_process_lbl_adv.pack(anchor="w", pady=(10, 6))
        box = tk.Frame(self.middle_adv, bg="#F3F3F3")
        box.pack(fill="both", expand=True)
        self.sys_list_box_adv = box
        self.sys_list_adv = tk.Listbox(
            box,
            selectmode="multiple",
            height=7,
            width=28,
            bg="white",
            fg="black",
            selectbackground="#1565C0",
            selectforeground="white",
            borderwidth=3,
            relief="solid",
        )
        self.sys_list_adv.pack(side="left", fill="both", expand=True)
        scroll = tk.Scrollbar(box, orient="vertical", command=self.sys_list_adv.yview)
        scroll.pack(side="right", fill="y")
        self.sys_list_adv.configure(yscrollcommand=scroll.set)
        self.sys_list_adv.bind(
            "<<ListboxSelect>>", self._on_systems_listbox_select_changed
        )
        self.sys_list_adv.bind("<Button-1>", self._on_sys_list_button1_clicked)
        self.right_adv = tk.Frame(outer, bg="#F3F3F3")
        # sticky="nsw" : voir le commentaire equivalent sur self.right
        # (onglet Main) -- meme cause (largeur qui varie quand middle_adv
        # est masque), meme effet (bandes blanches dans le decoupage de
        # fond).
        self.right_adv.grid(row=0, column=2, sticky="nsw", padx=(10, 0))
        self.mode_detail_title_lbl_adv = tk.Label(
            self.right_adv,
            text=ui["mode_detail_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_detail_title_lbl_adv.pack(anchor="w", pady=(0, 6))
        # 2026-08-11 -- Text (voir commentaire equivalent sur mode_desc_label,
        # onglet Simple) pour les liens cliquables (pack "ultimate" du
        # detail Mode 2/11).
        self.mode_desc_label_adv = tk.Text(
            self.right_adv, wrap="word", bg="#F3F3F3", fg="black",
            font=("TkDefaultFont", 10), width=34, height=1,
            bd=0, highlightthickness=0, cursor="arrow", padx=0, pady=0,
        )
        self.mode_desc_label_adv._theme_as_panel = True
        self.mode_desc_label_adv.bind("<Key>", lambda e: "break")
        self.mode_desc_label_adv.pack(anchor="w", fill="x")

        # Initialiser la description pour le mode par défaut -- differe via
        # after_idle() (2026-08-11) : appelee ICI, la fenetre n'est pas
        # encore mappee/affichee (mainloop() n'a pas encore demarre,
        # self.root.update() dans _insert_autolink_text() ne peut donc pas
        # obtenir une largeur en pixels fiable pour mode_desc_label). Bug
        # reel observe : hauteur du widget Text calculee bien trop grande
        # au tout premier affichage de l'onglet Main (cadre "systemes a
        # traiter" visuellement agrandi, cadre Progression partage pousse
        # hors de la fenetre a taille fixe) -- corrige des le premier
        # changement d'onglet (Main -> Avance -> Main), qui redeclenche
        # _update_mode_desc() une fois la fenetre reellement mappee. En
        # differant l'appel initial via after_idle (declenche au tout debut
        # de mainloop(), fenetre deja mappee), la mesure est fiable des le
        # premier affichage, sans devoir changer d'onglet.
        self.root.after_idle(self._update_mode_desc)

        # -- Mode 8 panel --
        # Ancre en bas de right_adv (meme idiome que _mode6_ui_frame dans
        # l'onglet Main) : le panneau reste colle au bas du cadre au lieu de
        # suivre la longueur variable du texte de description, ce qui libere
        # l'espace vertical intermediaire.
        self._mode8_frame = tk.Frame(
            self.right_adv, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8
        )
        self._mode8_frame.pack(side="bottom", fill="x", pady=(12, 0))
        self._mode8_frame.pack_forget()
        self._mode8_panel_title_lbl = tk.Label(
            self._mode8_frame,
            text=ui["mode8_panel_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        )
        self._mode8_panel_title_lbl.pack(anchor="w")
        self._mode8_version_lbl = tk.Label(
            self._mode8_frame,
            text=ui["mode8_version_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        self._mode8_version_lbl.pack(anchor="w", pady=(8, 4))
        # Partage la meme variable/preference que Mode 1/3/Parametres :
        # changer la version n'importe ou la change partout.
        self._mode8_version_combo = ttk.Combobox(
            self._mode8_frame,
            textvariable=self._mode1_profile_var,
            values=list(self.tkmod.RECALBOX_PROFILES.keys()),
            state="readonly",
            width=10,
        )
        self._mode8_version_combo.pack(anchor="w")
        self._mode8_version_combo.bind(
            "<<ComboboxSelected>>", self._on_mode1_profile_selected
        )
        # Pas de bouton "Lancer la verification" ici : redondant avec
        # btn_start_adv (colonne de gauche), qui appelle deja
        # _on_start_clicked et se renomme "Demarrer la verification" en
        # mode 8 (voir _on_mode_changed). Un seul bouton de declenchement.
        self._mode8_open_report_btn = tk.Button(
            self._mode8_frame,
            text=ui["mode8_open_report"],
            command=self._on_mode8_open_report_clicked,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
            state="disabled",
        )
        self._mode8_open_report_btn.pack(fill="x", pady=(10, 0))

        self._mode8_compare_final_btn = tk.Button(
            self._mode8_frame,
            text=ui["mode8_btn_compare_final"],
            command=self._on_mode8_compare_final_clicked,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
            state="disabled",
        )
        self._mode8_compare_final_btn.pack(fill="x", pady=(10, 6))
        self._mode8_open_final_report_btn = tk.Button(
            self._mode8_frame,
            text=ui["mode8_open_final_report"],
            command=self._on_mode8_open_final_report_clicked,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
            state="disabled",
        )
        self._mode8_open_final_report_btn.pack(fill="x")

        # -- Mode 3 panel (version Recalbox, meme panneau que Mode 1) --
        # Cache par defaut (mode par defaut de l'onglet Avance = 2),
        # affiche uniquement quand mode == "3" (_on_mode_changed).
        self._mode3_profile_frame = self._build_recalbox_profile_panel(self.right_adv)
        self._mode3_profile_frame.pack(side="bottom", fill="x", pady=(12, 0))
        self._mode3_profile_frame.pack_forget()
        self._mode3_clean_btn = self._mode3_profile_frame.clean_btn

        # -- Image de secours (default.raw565) : bouton partage, visible
        # uniquement en Mode 2 (telechargement _defaults) dans cet onglet.
        # Empaquete cote "haut" (pas side="bottom") pour apparaitre juste
        # sous "Details du mode selectionne", pas ancre en bas de right_adv.
        self._default_image_frame_adv = tk.Frame(self.right_adv, bg="#F3F3F3")
        self._default_image_btn_adv = self._build_default_image_button(
            self._default_image_frame_adv
        )
        self._default_image_btn_adv.pack(fill="x")
        self._default_image_frame_adv.pack(fill="x", pady=(12, 0))
        self._default_image_frame_adv.pack_forget()

        # -- Copier sur la carte SD (meme panneau que l'onglet Main) --
        # Cache par defaut, revele par _start_mode6_blinking() une fois un
        # traitement termine -- pas lie a un mode particulier (peut suivre
        # n'importe quel mode lance depuis cet onglet).
        self._mode6_ui_frame_adv = self._build_mode6_panel(self.right_adv)
        self._mode6_ui_frame_adv.pack(side="bottom", fill="x", pady=(6, 0))
        self._mode6_ui_frame_adv.pack_forget()
        self._mode6_panel_title_lbl_adv = self._mode6_ui_frame_adv.panel_title_lbl
        self._mode6_drives_title_lbl_adv = self._mode6_ui_frame_adv.drives_title_lbl
        self._mode6_drive_list_adv = self._mode6_ui_frame_adv.drive_list
        self._mode6_btn_adv = self._mode6_ui_frame_adv.btn
        self._mode6_explore_output_btn_adv = self._mode6_ui_frame_adv.explore_btn

        # -- Mode 9 panel (installer/mettre a jour les scripts Recalbox) --
        # Panneau autonome (bouton dedie + thread propre, meme idiome que le
        # panneau Mode 6 juste au-dessus) plutot que reutiliser
        # btn_start_adv/_worker_main (comme Mode 8) : l'action ne correspond
        # a aucun des pipelines ROMs->SD existants (pas de cfg.roms_root,
        # pas de selection de systemes).
        self._mode9_frame = tk.Frame(
            self.right_adv, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8
        )
        self._mode9_frame.pack(side="bottom", fill="x", pady=(12, 0))
        self._mode9_frame.pack_forget()
        self._mode9_panel_title_lbl = tk.Label(
            self._mode9_frame,
            text=ui["mode9_panel_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        )
        self._mode9_panel_title_lbl.pack(anchor="w")
        self._mode9_host_lbl = tk.Label(
            self._mode9_frame,
            text=ui["mode9_host_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        self._mode9_host_lbl.pack(anchor="w", pady=(8, 2))
        self.mode9_host_var = tk.StringVar(value=prefs.get("recalbox_ip") or "")
        self._mode9_host_entry = tk.Entry(
            self._mode9_frame, textvariable=self.mode9_host_var, width=24
        )
        self._mode9_host_entry.pack(anchor="w")
        self._mode9_install_btn = tk.Button(
            self._mode9_frame,
            text=ui["mode9_btn_install"],
            command=self._on_mode9_install_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self._mode9_install_btn.pack(fill="x", pady=(10, 0))
        self.mode9_result_var = tk.StringVar(value="")
        self._mode9_result_lbl = tk.Label(
            self._mode9_frame,
            textvariable=self.mode9_result_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9),
            wraplength=280,
            justify="left",
        )
        self._mode9_result_lbl.pack(anchor="w", pady=(6, 0))
        self._mode9_thread: Optional[threading.Thread] = None
        # Detection auto en arriere-plan : Path(UNC).exists() peut prendre
        # plusieurs secondes si le nom ne resout pas -- jamais appele sur le
        # thread principal (ni a la construction du GUI, ni au clic radio).
        self._start_mode9_autodetect()

    # language + mode logic
    # ---------------------------------------------------------
    def _set_toolkit_language(self, lang: str) -> None:
        # toolkit has TRANSLATIONS + global T
        try:
            self.tkmod.T = self.tkmod.TRANSLATIONS[lang]
            self.tkmod.CURRENT_LANG = lang
        except Exception:
            return

    def _on_language_changed(self) -> None:
        lang = self.lang_var.get()
        if lang not in ("fr", "en", "es"):
            return
        self._set_toolkit_language(lang)
        # Sauvegarder la préférence de langue dans le fichier JSON centralisé
        prefs.set("language", lang)

        ui = self._get_ui_t()

        # Noms des onglets (Main/Logs identiques dans les 3 langues)
        try:
            self.nb_top.tab(self.tab_playlist, text=ui["tab_playlist"])
            self.nb_top.tab(self.tab_advanced, text=ui["tab_advanced"])
            self.nb_top.tab(self.tab_params, text=ui["tab_params"])
            self.nb_top.tab(self.tab_help, text=ui["tab_help"])
        except Exception:
            pass

        # ── Onglet PLAYLIST : jamais repris jusqu'ici -- restait fige dans
        # la langue de demarrage apres un changement a chaud (rapporte par
        # l'utilisateur : boutons restes en FR apres bascule EN/ES).
        if getattr(self, "_playlist_sd_title_lbl", None):
            self._playlist_sd_title_lbl.config(text=ui["playlist_sd_section_title"])
        if getattr(self, "_playlist_refresh_drives_btn", None):
            self._playlist_refresh_drives_btn.config(text=ui["playlist_refresh_drives_btn"])
        if getattr(self, "_playlist_temp_note_lbl", None) and getattr(self, "_playlist_temp_mode", False):
            submode = getattr(self, "_playlist_temp_submode", "add")
            note_key = "playlist_temp_note_playlist" if submode == "playlist" else "playlist_temp_note_add"
            self._playlist_temp_note_lbl.config(text=ui[note_key](self.sd_dir))
        if getattr(self, "_playlist_explanation_lbl", None):
            self._playlist_refresh_explanation_text()
        if getattr(self, "_playlist_name_lbl", None):
            self._playlist_name_lbl.config(text=ui["playlist_name_label"])
        if getattr(self, "_playlist_delete_btn", None):
            self._playlist_delete_btn.config(text=ui["playlist_delete_btn"])
        if getattr(self, "_playlist_folders_title_lbl", None):
            self._playlist_folders_title_lbl.config(text=ui["playlist_folders_title"])
        if getattr(self, "_playlist_folders_select_none_btn", None):
            self._playlist_folders_select_none_btn.config(text=ui["playlist_select_none_btn"])
        if getattr(self, "_playlist_folders_select_all_btn", None):
            self._playlist_folders_select_all_btn.config(text=ui["playlist_select_all_btn"])
        if getattr(self, "_playlist_add_external_btn", None):
            self._playlist_add_external_btn.config(text=ui["playlist_add_external_folder_btn"])
        if getattr(self, "_playlist_delete_gif_btn", None):
            self._playlist_delete_gif_btn.config(text=ui["playlist_delete_gif_btn"])
        if getattr(self, "_playlist_files_title_lbl", None):
            self._playlist_files_title_lbl.config(text=ui["playlist_files_title"])
        if getattr(self, "_playlist_files_select_none_btn", None):
            self._playlist_files_select_none_btn.config(text=ui["playlist_select_none_btn"])
        if getattr(self, "_playlist_files_select_all_btn", None):
            self._playlist_files_select_all_btn.config(text=ui["playlist_select_all_btn"])
        if getattr(self, "_playlist_build_btn", None):
            # En mode temporaire (Mode 1), sous-mode "add", ce bouton
            # porte le texte "Copier la selection" (voir
            # _playlist_refresh_temp_submode_ui) -- ne jamais le faire
            # revenir a "Construire la playlist" par une simple bascule
            # de langue tant que ce sous-mode est actif.
            if getattr(self, "_playlist_temp_mode", False) and getattr(self, "_playlist_temp_submode", "add") == "add":
                self._playlist_build_btn.config(text=ui["playlist_copy_pending_btn"])
            else:
                self._playlist_build_btn.config(text=ui["playlist_build_btn"])
        if getattr(self, "_playlist_regen_cache_btn", None):
            # Pendant le mode temporaire (Mode 1), ce bouton porte le
            # texte "Continuer" (voir _enter_playlist_temp_mode) -- ne
            # jamais le faire revenir a "Regenerer le cache playlist" par
            # une simple bascule de langue tant que le mode est actif.
            if getattr(self, "_playlist_temp_mode", False):
                self._playlist_regen_cache_btn.config(text=ui["playlist_temp_continue_btn"])
            else:
                self._playlist_regen_cache_btn.config(text=ui["playlist_regen_cache_btn"].replace("\n", " "))
        if getattr(self, "_playlist_quit_btn", None):
            try:
                self._playlist_quit_btn.config(text=self.tkmod.tr("main_opt_quit"))
            except Exception:
                pass

        # update titles for radios (toolkit) - Main tab
        self._mode_radios["1"].config(text=self.tkmod.tr("mode1_title"))
        # update titles for radios - Advanced tab (titres courts 1 ligne,
        # GUI-only : voir UI_TRANSLATIONS "modeN_short_title")
        if hasattr(self, "_mode_radios_adv"):
            self._mode_radios_adv["2"].config(text=ui["mode2_short_title"])
            self._mode_radios_adv["3"].config(text=ui["mode3_short_title"])
            self._mode_radios_adv["4"].config(text=ui["mode4_short_title"])
            self._mode_radios_adv["5"].config(text=ui["mode5_short_title"])
            self._mode_radios_adv["6"].config(text=ui["mode6_short_title"])
            self._mode_radios_adv["7"].config(text=ui["mode7_short_title"])
            self._mode_radios_adv["8"].config(text=ui["mode8_short_title"])
            self._mode_radios_adv["9"].config(text=ui["mode9_short_title"])
            self._mode_radios_adv["10"].config(text=ui["mode10_short_title"])
            self._mode_radios_adv["11"].config(text=ui["mode11_short_title"])
        # En-tetes de categorie de l'accordeon (2026-08-11, v43)
        if hasattr(self, "_accordion_categories"):
            for cat_key, cat_ui_key, _cat_modes in self._accordion_categories:
                if cat_key in self._accordion_headers:
                    self._accordion_headers[cat_key].config(
                        text=self._accordion_header_text(cat_key, cat_ui_key, ui)
                    )

        # update main UI labels/buttons (ours)
        if getattr(self, "params_language_title_lbl", None):
            self.params_language_title_lbl.config(text=ui["language_title"])

        if getattr(self, "mode_title_lbl", None):
            self.mode_title_lbl.config(text=ui["mode_label"])

        mode = self.mode_var.get()

        if getattr(self, "btn_pick_roms", None):
            self.btn_pick_roms.config(
                text=(
                    ui.get(
                        "images_pick_btn",
                        ui["roms_pick_btn"],
                    )
                    if mode in ("4", "5")
                    else ui["roms_pick_btn"]
                )
            )

        if getattr(self, "btn_start", None):
            self.btn_start.config(text=ui["start_btn"])

        if getattr(self, "btn_start_adv", None):
            self.btn_start_adv.config(
                text=ui["mode8_btn_check"] if mode == "8" else ui["start_btn"]
            )

        if getattr(self, "btn_detect_systems", None):
            self.btn_detect_systems.config(
                text=(
                    ui.get(
                        "select_images_btn",
                        ui["detect_systems_btn"],
                    )
                    if mode in ("4", "5")
                    else ui["detect_systems_btn"]
                )
            )

        if getattr(self, "systems_to_process_lbl", None):
            self.systems_to_process_lbl.config(text=ui["systems_to_process_lbl"])

        if getattr(self, "mode_detail_title_lbl", None):
            self.mode_detail_title_lbl.config(text=ui["mode_detail_title"])

        if getattr(self, "progress_title_lbl", None):
            self.progress_title_lbl.config(text=ui["progress_title"])

        # Boutons Logs
        if getattr(self, "btn_pause", None):
            self.btn_pause.config(text=ui["btn_pause"])
        if getattr(self, "btn_resume", None):
            self.btn_resume.config(text=ui["btn_resume"])
        if getattr(self, "btn_skip", None):
            self.btn_skip.config(text=ui["btn_skip"])
        if getattr(self, "btn_stop", None):
            self.btn_stop.config(text=ui["btn_stop"])

        # Boutons Progress (onglet Main)
        if getattr(self, "btn_pause_progress", None):
            self.btn_pause_progress.config(text=ui["btn_pause"])
        if getattr(self, "btn_resume_progress", None):
            self.btn_resume_progress.config(text=ui["btn_resume"])
        if getattr(self, "btn_skip_progress", None):
            self.btn_skip_progress.config(text=ui["btn_skip"])
        if getattr(self, "btn_stop_progress", None):
            self.btn_stop_progress.config(text=ui["btn_stop"])

        if getattr(self, "btn_quit_app", None):
            # bouton "Quitter" : tkmod.tr() fournit le texte localisé
            try:
                self.btn_quit_app.config(text=self.tkmod.tr("main_opt_quit"))
            except Exception:
                pass

        if getattr(self, "logs_details_title_lbl", None):
            self.logs_details_title_lbl.config(text=ui["logs_details_title"])

        for inst in self._mode6_instances():
            if inst["drives_title_lbl"]:
                inst["drives_title_lbl"].config(text=ui["mode6_drives_title"])
            if inst["explore_btn"]:
                inst["explore_btn"].config(text=ui["mode6_explore_output_btn"])

        # mode details / titre panneau
        if getattr(self, "mode_detail_title_lbl", None):
            self.mode_detail_title_lbl.config(text=ui["mode_detail_title"])

        # ── Onglet Avance : widgets jamais repris jusqu'ici (restaient
        # figes dans la langue de demarrage apres un changement a chaud) ──
        if getattr(self, "mode_title_lbl_adv", None):
            self.mode_title_lbl_adv.config(text=ui["mode_label"])

        if getattr(self, "btn_pick_roms_adv", None):
            self.btn_pick_roms_adv.config(
                text=(
                    ui["mode7_pick_btn"]
                    if mode == "7"
                    else (
                        ui.get("images_pick_btn", ui["roms_pick_btn"])
                        if mode in ("4", "5")
                        else ui["roms_pick_btn"]
                    )
                )
            )

        if getattr(self, "btn_detect_systems_adv", None):
            self.btn_detect_systems_adv.config(
                text=(
                    ui.get("select_images_btn", ui["detect_systems_btn"])
                    if mode in ("4", "5")
                    else ui["detect_systems_btn"]
                )
            )

        if getattr(self, "systems_to_process_lbl_adv", None):
            self.systems_to_process_lbl_adv.config(text=ui["systems_to_process_lbl"])

        if getattr(self, "mode_detail_title_lbl_adv", None):
            self.mode_detail_title_lbl_adv.config(text=ui["mode_detail_title"])

        if getattr(self, "btn_quit_app_adv", None):
            try:
                self.btn_quit_app_adv.config(text=self.tkmod.tr("main_opt_quit"))
            except Exception:
                pass

        # ── Panneau Mode 8 ──
        if getattr(self, "_mode8_panel_title_lbl", None):
            self._mode8_panel_title_lbl.config(text=ui["mode8_panel_title"])
        if getattr(self, "_mode8_version_lbl", None):
            self._mode8_version_lbl.config(text=ui["mode8_version_label"])
        if getattr(self, "_mode8_open_report_btn", None):
            self._mode8_open_report_btn.config(text=ui["mode8_open_report"])
        if getattr(self, "_mode8_compare_final_btn", None):
            self._mode8_compare_final_btn.config(text=ui["mode8_btn_compare_final"])
        if getattr(self, "_mode8_open_final_report_btn", None):
            self._mode8_open_final_report_btn.config(text=ui["mode8_open_final_report"])

        # ── Panneau Mode 9 ──
        if getattr(self, "_mode9_panel_title_lbl", None):
            self._mode9_panel_title_lbl.config(text=ui["mode9_panel_title"])
        if getattr(self, "_mode9_host_lbl", None):
            self._mode9_host_lbl.config(text=ui["mode9_host_label"])
        if getattr(self, "_mode9_install_btn", None):
            self._mode9_install_btn.config(text=ui["mode9_btn_install"])

        # ── Panneaux "Version Recalbox" (Mode 1 Main, Mode 3 Avance) ──
        for frame_attr in ("_mode1_profile_frame", "_mode3_profile_frame"):
            frame = getattr(self, frame_attr, None)
            if frame is None:
                continue
            if getattr(frame, "title_lbl", None):
                frame.title_lbl.config(text=ui["mode1_profile_label"])
            if getattr(frame, "help_btn", None):
                frame.help_btn.config(text=ui["mode1_scrape_help_btn"])
            if getattr(frame, "clean_btn", None):
                frame.clean_btn.config(text=ui["mode1_clean_btn"])

        # ── Onglet Parametres : label version Recalbox ──
        if getattr(self, "_params_profile_lbl", None):
            self._params_profile_lbl.config(text=ui["mode1_profile_label"])

        # ── Onglet Parametres : label + aide seuil flag "L" (v45) ──
        if getattr(self, "_params_slow_threshold_lbl", None):
            self._params_slow_threshold_lbl.config(text=ui["slow_threshold_label"])
        if getattr(self, "_params_slow_threshold_hint_lbl", None):
            self._params_slow_threshold_hint_lbl.config(text=ui["slow_threshold_hint"])

        # ── Bouton "Image de secours" (Main + Avance/Mode 2) ──
        if getattr(self, "_default_image_btn", None):
            self._default_image_btn.config(text=ui["default_image_btn"])
        if getattr(self, "_default_image_btn_adv", None):
            self._default_image_btn_adv.config(text=ui["default_image_btn"])

        # ── Bouton "Ouvrir dans le navigateur" (onglet AIDE) ──
        if getattr(self, "help_open_browser_btn", None):
            self.help_open_browser_btn.config(text=ui["help_open_browser_btn"])

        # ── Filtre de niveau de logs ──
        if getattr(self, "log_level_lbl", None):
            self.log_level_lbl.config(text=ui["logs_level_label"])
        if getattr(self, "log_level_combo", None):
            _level_map_old_to_new = {
                "Tout": "logs_level_all",
                "Alertes+Erreurs": "logs_level_warn_err",
                "Erreurs": "logs_level_err",
                "All": "logs_level_all",
                "Warnings+Errors": "logs_level_warn_err",
                "Errors": "logs_level_err",
                "Todo": "logs_level_all",
                "Alertas+Errores": "logs_level_warn_err",
                "Errores": "logs_level_err",
            }
            new_values = [
                ui["logs_level_all"],
                ui["logs_level_warn_err"],
                ui["logs_level_err"],
            ]
            current_level = self.log_level_var.get()
            key = _level_map_old_to_new.get(current_level, "logs_level_warn_err")
            self.log_level_combo.configure(values=new_values)
            self.log_level_var.set(ui[key])
            self._rebuild_log_display(go_end=True)

        # ── Identifiants NAS (si le panneau a ete construit) ──
        if getattr(self, "creds_frame", None):
            for child in self.creds_frame.winfo_children():
                key = getattr(child, "_i18n_key", None)
                if key:
                    try:
                        child.config(text=ui[key])
                    except Exception:
                        pass

        # Mettre a jour la combobox theme avec le label localisé
        current_theme = self._theme_var.get()
        themes_list = themes.list_themes()
        if lang == "en":
            random_lbl = "Random"
        elif lang == "es":
            random_lbl = "Aleatorio"
        else:
            random_lbl = "Aléatoire"
        if current_theme not in themes_list:
            self._theme_var.set(random_lbl)
        self._theme_combobox.configure(values=[random_lbl] + themes_list)

        # update description + mode6 texts
        self._update_mode_desc()

        # Re-découper l'image de fond pour les modes qui masquent/affichent des widgets
        try:
            from RecalBoxDMD_themes import _slice_widgets_later

            _slice_widgets_later(self)
        except Exception:
            pass
        self._sync_mode6_texts()

        # refresh help tab (README language + links)
        self._refresh_help_tab_content()

    # Marqueurs de debut de la section "Marche a suivre"/"Steps"/"Pasos"
    # dans _detail_templates -- utilises pour la tronquer une fois la copie
    # SD atteinte (voir _update_mode_desc). Rester en phase avec le texte
    # de ces sections dans _detail_templates si jamais reformule.
    _STEPS_SECTION_MARKERS = ("\n\nMarche à suivre", "\n\nSteps:", "\n\nPasos:")

    def _update_mode_desc(self) -> None:
        mode = self.mode_var.get()
        lang = self.lang_var.get()
        text = self._detail_templates.get(lang, self._detail_templates["fr"]).get(
            mode, ""
        )
        # Une fois le panneau "copie SD" revele (_start_mode6_blinking), le
        # texte descriptif complet (avec "Marche a suivre") decale toute
        # l'interface vers le bas -- les boutons du cadre progress
        # disparaissent de la vue. Bug remonte : on retire cette section
        # pour liberer la place, seulement une fois arrive a cette etape.
        if mode == "1" and getattr(self, "_mode1_sd_copy_active", False):
            for marker in self._STEPS_SECTION_MARKERS:
                idx = text.find(marker)
                if idx != -1:
                    text = text[:idx]
                    break
        # 2026-08-11 -- mode_desc_label(_adv) est un Text (auto-lien, voir
        # _insert_autolink_text()), plus un Label+StringVar -- garde
        # defensif (getattr) : cette methode peut etre appelee par
        # themes.apply() avant que ces widgets existent (application du
        # theme sauvegarde, potentiellement avant construction complete de
        # l'onglet Avance).
        if getattr(self, "mode_desc_label", None) is not None:
            self._insert_autolink_text(self.mode_desc_label, text)
        if getattr(self, "mode_desc_label_adv", None) is not None:
            self._insert_autolink_text(self.mode_desc_label_adv, text)

    def _accordion_category_for_mode(self, mode: str) -> Optional[str]:
        """Retourne la cle de categorie de l'accordeon contenant ce mode,
        ou None si le mode n'en fait pas partie (ex: Mode 1, onglet Main)."""
        for cat_key, _cat_ui_key, cat_modes in getattr(self, "_accordion_categories", []):
            if mode in cat_modes:
                return cat_key
        return None

    def _accordion_header_text(self, cat_key: str, cat_ui_key: str, ui: dict) -> str:
        is_open = getattr(self, "_accordion_open_category", None) == cat_key
        prefix = "▾" if is_open else "▸"
        return f"{prefix} {ui.get(cat_ui_key, cat_key)}"

    def _on_accordion_toggle(self, cat_key: str) -> None:
        """Replie la categorie actuellement ouverte et deplie celle cliquee
        (un seul en-tete de categorie developpe a la fois). Un clic sur
        l'en-tete deja ouvert ne fait rien -- toujours garder une categorie
        visible plutot que tout replier."""
        if getattr(self, "_accordion_open_category", None) == cat_key:
            return
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        prev = self._accordion_open_category
        self._accordion_open_category = cat_key
        for c_key, c_ui_key, _c_modes in self._accordion_categories:
            if c_key == prev and prev in self._accordion_bodies:
                self._accordion_bodies[prev].pack_forget()
                self._accordion_headers[prev].config(
                    text=self._accordion_header_text(prev, c_ui_key, ui)
                )
            elif c_key == cat_key:
                self._accordion_bodies[cat_key].pack(anchor="w", fill="x")
                self._accordion_headers[cat_key].config(
                    text=self._accordion_header_text(cat_key, c_ui_key, ui)
                )
        # Deplier/replier change la hauteur du contenu de "left" -- meme
        # traitement que tout changement de mode (voir _on_mode_changed),
        # sinon le decoupage de l'image de fond reste perime (bandes
        # blanches).
        self.root.update_idletasks()
        self.root.after(30, self._reslice_after_mode_change)

    def _on_mode_changed(self) -> None:
        mode = self.mode_var.get()
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])

        # Auto-ouvre la categorie de l'accordeon contenant le mode
        # selectionne (changement depuis l'onglet Main, ou premier
        # affichage de l'onglet Avance) -- sans ca, le radio actif pourrait
        # se trouver dans une categorie visuellement repliee, invisible.
        if hasattr(self, "_accordion_categories"):
            target_cat = self._accordion_category_for_mode(mode)
            if target_cat and target_cat != getattr(self, "_accordion_open_category", None):
                self._on_accordion_toggle(target_cat)

        # Libellés spécifiques bouton pick (onglet Main)
        if getattr(self, "btn_pick_roms", None):
            self.btn_pick_roms.config(
                text=(
                    ui["mode7_pick_btn"]
                    if mode == "7"
                    else (
                        ui["images_pick_btn"]
                        if mode in ("4", "5")
                        else ui["roms_pick_btn"]
                    )
                )
            )

        if getattr(self, "btn_detect_systems", None):
            self.btn_detect_systems.config(
                text=(
                    ui["select_images_btn"]
                    if mode in ("4", "5")
                    else ui["detect_systems_btn"]
                )
            )

        # Gestion du bouton choisir dossier dans l'onglet Avancé
        # Mode 2 : masqué (téléchargement GitHub) + message dans le path_box label
        # Mode 6 : masqué + popup (doit exécuter mode 3 d'abord)
        # Mode 7 : masqué + popup (doit exécuter mode 2 d'abord)
        # Mode 4/5 : visible, label "Choisir dossier IMAGES"
        # Mode 3 : visible, label normal
        # Mode 1 : visible, label normal
        if hasattr(self, "btn_pick_roms_adv"):
            # Modes 2/6/7/9/10/11 : bouton désactivé (mais gardé visible pour la stabilité du fond)
            if mode in ("2", "6", "7", "9", "10", "11"):
                try:
                    self.btn_pick_roms_adv.config(state="disabled")
                except Exception:
                    pass
                # Chemin temporaire pour les besoins internes
                temp_path = str(self.sd_dir / "systems")
                if hasattr(self, "roms_path_var_adv"):
                    self.roms_path_var_adv.set(temp_path)
                if hasattr(self, "roms_path_var"):
                    self.roms_path_var.set(temp_path)
                # Désactiver aussi le bouton de l'onglet Main
                try:
                    if (
                        hasattr(self, "btn_pick_roms")
                        and self.btn_pick_roms.winfo_exists()
                    ):
                        self.btn_pick_roms.config(state="disabled")
                except Exception:
                    pass
            else:
                # Modes 1, 3, 4, 5 : afficher le bouton
                try:
                    self.btn_pick_roms_adv.config(state="normal")
                except Exception:
                    pass
                try:
                    if (
                        hasattr(self, "btn_pick_roms")
                        and self.btn_pick_roms.winfo_exists()
                    ):
                        self.btn_pick_roms.config(state="normal")
                except Exception:
                    pass
                # Restaurer le chemin normal dans le label
                normal_path = str(self.sd_dir / "systems")
                if hasattr(self, "roms_path_var_adv"):
                    if (
                        self.roms_path_var_adv.get() != normal_path
                        and not self.roms_path_var_adv.get().startswith("Mode")
                    ):
                        pass  # keep user-chosen path
                    else:
                        self.roms_path_var_adv.set(normal_path)
                if hasattr(self, "roms_path_var"):
                    if (
                        self.roms_path_var.get() != normal_path
                        and not self.roms_path_var.get().startswith("Mode")
                    ):
                        pass  # keep user-chosen path
                    else:
                        self.roms_path_var.set(normal_path)
                # Label selon le mode
                if mode in ("4", "5"):
                    try:
                        self.btn_pick_roms_adv.config(
                            text=ui.get("images_pick_btn", "Choisir dossier IMAGES")
                        )
                    except Exception:
                        pass
                else:
                    try:
                        self.btn_pick_roms_adv.config(text=ui["roms_pick_btn"])
                    except Exception:
                        pass

        # Colonne systèmes visible seulement pour modes qui en ont besoin.
        show_sys_col = mode in ("1", "3", "4", "5", "8")
        if show_sys_col:
            try:
                self.middle.grid()
                self.btn_detect_systems.config(state="normal")
            except Exception:
                pass
            try:
                if hasattr(self, "middle_adv") and self.middle_adv.winfo_exists():
                    self.middle_adv.grid()
                if hasattr(self, "btn_detect_systems_adv"):
                    self.btn_detect_systems_adv.config(state="normal")
            except Exception:
                pass
            # auto-detect silencieuse en différé pour ne pas bloquer l'UI
            try:
                self.root.after_idle(self._maybe_autodetect_systems)
            except Exception:
                pass
        else:
            # Modes 2, 6, 7 : pas de choix de dossier ROMs (dossier
            # temporaire utilise directement), donc pas de detection ni de
            # selection des systemes -> masquer la colonne.
            try:
                self.middle.grid_remove()
            except Exception:
                pass
            try:
                if hasattr(self, "middle_adv") and self.middle_adv.winfo_exists():
                    self.middle_adv.grid_remove()
            except Exception:
                pass

            # Mode 8 : afficher le panneau de vérification. btn_start_adv
            # reste actif et se renomme -- il appelle deja _on_start_clicked,
            # exactement comme le faisait l'ancien bouton "Lancer la
            # verification" du panneau (supprime, redondant).
        if mode == "8":
            if hasattr(self, "_mode8_frame"):
                self._mode8_frame.pack(side="bottom", fill="x", pady=(12, 0))
            if hasattr(self, "btn_start_adv"):
                self.btn_start_adv.config(state="normal", text=ui["mode8_btn_check"])
        else:
            if hasattr(self, "_mode8_frame"):
                self._mode8_frame.pack_forget()
            if hasattr(self, "btn_start_adv"):
                self.btn_start_adv.config(state="normal", text=ui["start_btn"])

        # Mode 9 : afficher le panneau dedie (installer/mettre a jour les
        # scripts Recalbox). btn_start_adv n'a rien a faire ici -- l'action
        # est declenchee par le bouton propre du panneau (voir
        # _on_mode9_install_clicked), donc desactive plutot que reutilise
        # (a la difference du Mode 8, qui partage btn_start_adv). Place
        # APRES le bloc Mode 8 ci-dessus : son "else" remet toujours
        # btn_start_adv a state="normal" quel que soit le mode courant, donc
        # ce bloc doit s'executer en dernier pour que la desactivation tienne.
        if mode == "9":
            if hasattr(self, "_mode9_frame"):
                self._mode9_frame.pack(side="bottom", fill="x", pady=(12, 0))
            if hasattr(self, "btn_start_adv"):
                self.btn_start_adv.config(state="disabled", text=ui["start_btn"])
        else:
            if hasattr(self, "_mode9_frame"):
                self._mode9_frame.pack_forget()
            # Mode 10 (image de secours) : action autonome via son propre
            # bouton (voir _default_image_frame_adv plus bas), pas de
            # pipeline -- meme raison que Mode 9 ci-dessus, desactive
            # btn_start_adv plutot que le reutiliser. Mode 11 (pack GIFs)
            # n'en fait PLUS partie depuis l'unification avec Mode 2
            # (2026-08-11, demande utilisateur) : passe desormais par
            # btn_start_adv comme tous les autres modes "pipeline".
            if mode == "10" and hasattr(self, "btn_start_adv"):
                self.btn_start_adv.config(state="disabled", text=ui["start_btn"])

        # Mode 3 : afficher le panneau "Version Recalbox" (partage avec le
        # Mode 1) pour choisir le profil logo/thumbnail/image avant
        # l'extraction gamelist.xml seule.
        if mode == "3":
            if hasattr(self, "_mode3_profile_frame"):
                self._mode3_profile_frame.pack(side="bottom", fill="x", pady=(12, 0))
        else:
            if hasattr(self, "_mode3_profile_frame"):
                self._mode3_profile_frame.pack_forget()

        # Mode 3/8 : masquer le cadre "copie SD" (_mode6_ui_frame_adv) s'il
        # est reste visible d'un traitement precedent dans cette session
        # (Mode 1/6/7, via _start_mode6_blinking()) -- rien ne le masquait
        # jusqu'ici en changeant de mode, alors que Mode 3 (extraction
        # seule) et Mode 8 (verification) ne produisent rien de pret a
        # copier sur la carte SD. Sa presence repoussait le panneau
        # specifique du mode hors de la zone visible de l'onglet, jusqu'a
        # chevaucher le cadre Progression partage en dessous (bug "cadre
        # Progression disparu" signale par l'utilisateur).
        if mode in ("3", "8"):
            if hasattr(self, "_mode6_ui_frame_adv"):
                self._mode6_ui_frame_adv.pack_forget()

        # Mode 10 : afficher le bouton "Image de secours" -- promu en mode
        # a part entiere (2026-08-11, v43, etait auparavant affiche sous
        # Mode 2), regroupe sous la categorie IMAGES TOOLS de l'accordeon.
        # Empaquete juste sous "Details du mode selectionne" (pas ancre en
        # bas, contrairement aux panneaux Mode 3/Mode 8).
        if mode == "10":
            if hasattr(self, "_default_image_frame_adv"):
                self._default_image_frame_adv.pack(fill="x", pady=(12, 0))
        else:
            if hasattr(self, "_default_image_frame_adv"):
                self._default_image_frame_adv.pack_forget()


        self._update_mode_desc()
        # Un changement de mode peut modifier la largeur naturelle de
        # colonnes entieres (masquage de middle/middle_adv, libelles de
        # bouton/description qui changent de longueur d'un mode a l'autre
        # -- ex: right_adv mesure a 278px en mode 5 contre 318px en mode
        # 8). Le decoupage de l'image de fond (slice) est capture a une
        # largeur donnee : tant qu'on ne le refait pas, un cadre dont la
        # largeur a change affiche un decoupage perime -> bandes blanches.
        # _slice_widgets_later() ne se relance normalement que sur
        # <<NotebookTabChanged>>, jamais sur un simple changement de mode
        # radio dans le meme onglet : on le force ici, une fois la
        # nouvelle geometrie connue (apres le prochain rendu).
        self.root.update_idletasks()
        self.root.after(30, self._reslice_after_mode_change)

    def _reslice_after_mode_change(self) -> None:
        from RecalBoxDMD_themes import _slice_widgets_later

        _slice_widgets_later(self)
        # _update_sys_box_decor() doit s'appliquer APRES le decoupage
        # generique ci-dessus : le cadre de selection systeme (Listbox)
        # a besoin d'un traitement particulier (decoupage AU-DESSUS d'une
        # Listbox vide, ou fond opaque une fois peuplee) que le decoupage
        # generique, place derriere les enfants, ne peut pas reproduire.
        self._update_sys_box_decor()

    def _update_sys_box_decor(self) -> None:
        """Modes 3/4/5/8 : tant que le cadre de selection systeme (Listbox
        sys_list/sys_list_adv) est vide -- aucun dossier ROMs/Images choisi
        ou aucun systeme detecte -- on y montre le decoupage decoratif de
        l'image de fond du theme (meme rendu que les autres cadres).
        Des qu'il se peuple de vrais systemes, on repasse sur le fond
        opaque du theme (bg_listbox) pour garder la liste lisible.
        Pour les autres modes (2, 6, 7, 1), le cadre garde son fond
        opaque habituel (pas de Listbox a montrer)."""
        mode = self.mode_var.get()
        decorative_mode = mode in ("3", "4", "5", "8")
        theme = themes.get_theme(
            getattr(self, "_current_theme_name", None) or "default"
        )
        bg_listbox = theme.get("colors", {}).get("bg_listbox", "#FFFFFF")

        for box_attr, lb_attr in (
            ("sys_list_box", "sys_list"),
            ("sys_list_box_adv", "sys_list_adv"),
        ):
            box = getattr(self, box_attr, None)
            lb = getattr(self, lb_attr, None)
            if box is None or not box.winfo_exists():
                continue
            has_systems = bool(
                lb is not None and lb.winfo_exists() and lb.size() > 3
            )
            if decorative_mode and not has_systems:
                self.root.update_idletasks()
                # La Listbox (fill=both/expand=True) couvre quasiment tout
                # le cadre "box" : le decoupage doit epouser ses bornes a
                # elle et etre place PAR-DESSUS (elle est vide, rien
                # d'important a cacher), sinon il resterait invisible
                # derriere son fond opaque.
                themes.slice_frame_overlay(self, box, geometry_widget=lb)
            else:
                # make_frame_opaque() detruit deja les labels de decoupage
                # existants avant de reappliquer le fond plat.
                themes.make_frame_opaque(box, bg_listbox)

    def _maybe_autodetect_systems(self) -> None:
        path_str = self.roms_path_var.get().strip()
        if not path_str:
            return
        roms_root = Path(path_str)
        if not roms_root.exists():
            return

        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        is_unc = str(roms_root).startswith("\\\\")

        mode = self.mode_var.get()
        try:
            if mode in ("1", "3", "8"):
                systems = self._find_systems(roms_root)
            else:
                # En mode 4/5, roms_root pointe le dossier "images" libre choisi par l'utilisateur.
                # Donc on ne force plus un sous-dossier /images.
                systems = self._find_systems_images(roms_root)
        except Exception:
            if is_unc:
                self._show_credentials_if_needed(True)
                messagebox.showwarning(
                    ui["msg_warning_title"],
                    ui["unc_access_error_msg"],
                )
                return
            raise

        # Bug 6 : peupler les DEUX listboxes (Main ET Avancé)
        for listbox_attr in ("sys_list", "sys_list_adv"):
            lb = getattr(self, listbox_attr, None)
            if lb is None:
                continue
            if not lb.winfo_exists():
                continue
            lb.delete(0, "end")

            # 0="Tout sélectionner", 1="Ne rien sélectionner", 2="" separator
            lb.insert("end", ui["sys_sel_opt_all"])
            lb.insert("end", ui["sys_sel_opt_none"])
            lb.insert("end", " ")

            for sys_path in systems:
                lb.insert("end", sys_path.name)

            # Garantir l'affichage depuis le haut
            try:
                lb.yview_moveto(0.0)
            except Exception:
                pass

        self._update_sys_box_decor()

    def _find_systems(self, roms_root: Path) -> list[Path]:
        systems: list[Path] = []
        for d in roms_root.iterdir():
            if d.is_dir() and (d / "gamelist.xml").exists():
                systems.append(d)
        systems.sort(key=lambda p: p.name.lower())
        return systems

    def _find_systems_images(self, images_root: Path) -> list[Path]:
        """
        Détection des "systèmes" pour modes 4/5 :
        - chaque système est un sous-dossier contenant au moins un *.png ou *.gif (directement sous le dossier système).
        """
        if not images_root.exists() or not images_root.is_dir():
            return []

        # Fallback : si l'utilisateur a pointé directement un "dossier système"
        # (des *.png/*.gif sont directement au 1er niveau), on le considère comme 1 système.
        try:
            if any(images_root.glob("*.png")) or any(images_root.glob("*.gif")):
                return [images_root]
        except Exception:
            pass

        systems: list[Path] = []
        for d in images_root.iterdir():
            if not d.is_dir():
                continue

            # Certains dossiers ont une structure plus profonde (archives / sous-sous-dossiers),
            # donc on fait rglob au lieu de glob direct.
            try:
                has_png = next(d.rglob("*.png"), None) is not None
                has_gif = next(d.rglob("*.gif"), None) is not None
            except Exception:
                continue

            if has_png or has_gif:
                systems.append(d)

        systems.sort(key=lambda p: p.name.lower())
        return systems

    def _get_event_listbox(self, event: object) -> object:
        """Determine which listbox triggered the event, default to sys_list."""
        lb = self.sys_list
        if event is not None:
            try:
                widget = getattr(event, "widget", None)
                if widget is not None and widget.winfo_exists():
                    if hasattr(self, "sys_list_adv") and widget is self.sys_list_adv:
                        lb = self.sys_list_adv
            except Exception:
                pass
        return lb

    def _sync_listbox_selections(self, src_list) -> None:
        """Copy selections from src_list to the other listbox."""
        other = self.sys_list_adv if src_list is self.sys_list else self.sys_list
        if not other or not other.winfo_exists():
            return
        try:
            if other.size() != src_list.size():
                return
            old_flag = self._sys_list_adjusting
            self._sys_list_adjusting = True
            other.selection_clear(0, "end")
            for i in src_list.curselection():
                other.selection_set(i)
            self._sys_list_adjusting = old_flag
        except Exception:
            self._sys_list_adjusting = False

    def _on_sys_list_button1_clicked(self, _event: object = None) -> None:
        # Déterminer quelle listbox a reçu l'événement
        lb = self._get_event_listbox(_event)
        # capture l'intention (index cliqué: 0/1/2) et la sélection "réelle" (indices >= 3 : systèmes)
        self._last_sys_click_index = None
        self._last_sys_clicked_index_any = None
        try:
            if _event is not None and hasattr(_event, "y"):
                click_index = lb.nearest(getattr(_event, "y"))
                if isinstance(click_index, int):
                    self._last_sys_clicked_index_any = click_index
                    if click_index in (0, 1, 2):
                        self._last_sys_click_index = click_index
        except Exception:
            self._last_sys_click_index = None
            self._last_sys_clicked_index_any = None

        try:
            selected = list(lb.curselection())
            self._last_real_system_indices = {i for i in selected if i >= 3}
        except Exception:
            self._last_real_system_indices = set()

    def _on_systems_listbox_select_changed(self, _event: object = None) -> None:
        if self._sys_list_adjusting:
            return

        # Déterminer quelle listbox a reçu l'événement
        lb = self._get_event_listbox(_event)

        total_items = lb.size()
        # Pas de place pour sentinelles + séparateur + systèmes
        if total_items <= 3:
            return

        try:
            self._sys_list_adjusting = True

            selected = list(lb.curselection())
            selected_set = set(selected)

            total_systems = total_items - 3
            systems_start = 3
            systems_end_exclusive = systems_start + total_systems

            # Si on a cliqué un système réel (index >= 3), on veut autoriser le mode manuel :
            # - on retire uniquement les sentinelles
            # - on ne force jamais "Tout/Ne rien"
            if (
                self._last_sys_clicked_index_any is not None
                and self._last_sys_clicked_index_any >= systems_start
            ):
                lb.selection_clear(0)
                lb.selection_clear(1)
                lb.selection_clear(2)
                self._last_sys_click_index = None
                self._last_sys_clicked_index_any = None
                return

            # Cas délicat : au clic, Tk peut laisser transitoirement 0 et 1 sélectionnés.
            # On tranche alors selon l'index réellement cliqué (via _on_sys_list_button1_clicked).
            if 0 in selected_set and 1 in selected_set:
                intent = self._last_sys_click_index
                if intent == 0:
                    # "Tout sélectionner"
                    lb.selection_clear(0, "end")
                    lb.selection_set(0)
                    lb.selection_clear(1)
                    lb.selection_clear(2)
                    for i in range(systems_start, systems_end_exclusive):
                        lb.selection_set(i)
                    self._last_sys_click_index = None
                    self._last_sys_clicked_index_any = None
                    return

                if intent == 1:
                    # "Ne rien sélectionner"
                    lb.selection_clear(0, "end")
                    lb.selection_set(1)
                    lb.selection_clear(0)
                    lb.selection_clear(2)
                    self._last_sys_click_index = None
                    self._last_sys_clicked_index_any = None
                    return

                # Sinon (intention None ou clic sur une ligne système) => mode manuel :
                lb.selection_clear(0)
                lb.selection_clear(1)
                lb.selection_clear(2)
                self._last_sys_click_index = None
                self._last_sys_clicked_index_any = None
                return

            if 1 in selected_set:
                # "Ne rien sélectionner"
                lb.selection_clear(0, "end")
                lb.selection_set(1)
                lb.selection_clear(0)
                lb.selection_clear(2)
                self._last_sys_click_index = None
                return

            if 0 in selected_set:
                # "Tout sélectionner"
                lb.selection_clear(0, "end")
                lb.selection_set(0)
                lb.selection_clear(1)
                lb.selection_clear(2)
                for i in range(systems_start, systems_end_exclusive):
                    lb.selection_set(i)
                self._last_sys_click_index = None
                return

            # Sélection manuelle : on retire sentinelles 0/1 (et on s'assure que la ligne vide n'est pas sélectionnée)
            lb.selection_clear(0)
            lb.selection_clear(1)
            lb.selection_clear(2)

            # Si l'utilisateur a sélectionné tous les systèmes individuellement => on met "Tout sélectionner"
            real_selected = [i for i in selected if i >= systems_start]
            if len(real_selected) == total_systems and total_systems > 0:
                lb.selection_set(0)

        finally:
            self._sys_list_adjusting = False

    # ──────────────────────────────────────────────────────────────────────────
    # inputs
    # ──────────────────────────────────────────────────────────────────────────
    def _pick_roms_directory(self) -> None:
        mode = self.mode_var.get()
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        title = ui["images_pick_btn"] if mode in ("4", "5") else "Choisir dossier ROMs"
        # (fallback FR) si le user force une langue où on n'a pas le key FR exact
        if mode in ("4", "5"):
            # title utilise déjà ui["images_pick_btn"]
            pass
        p = filedialog.askdirectory(title=title)
        if not p:
            return
        # Bug 6 : mettre à jour les DEUX variables de chemin (main ET avancé)
        self.roms_path_var.set(p)
        if hasattr(self, "roms_path_var_adv"):
            self.roms_path_var_adv.set(p)

        is_unc = str(p).startswith("\\\\")

        # Bug 6 : différer _on_mode_changed() pour éviter le blocage UI
        # (le scan rglob dans _maybe_autodetect_systems peut être lent)
        if self.mode_var.get() in ("1", "3", "4", "5", "8"):
            self.root.after_idle(self._on_mode_changed)
        else:
            # Pour les modes 2/6/7, juste mettre à jour les textes du bouton
            self._update_mode_desc()

    def _get_roms_root_or_warn(self) -> Optional[Path]:
        ui = self._get_ui_t()
        path_str = self.roms_path_var.get().strip()
        if not path_str:
            messagebox.showwarning(ui["msg_warning_title"], ui["roms_root_missing_msg"])
            return None
        roms_root = Path(path_str)
        if not roms_root.exists():
            messagebox.showwarning(ui["msg_warning_title"], ui["roms_root_notfound_msg"])
            return None
        return roms_root

    def _on_detect_systems_clicked(self) -> None:
        roms_root = self._get_roms_root_or_warn()
        if roms_root is None:
            return

        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        is_unc = str(roms_root).startswith("\\\\")

        try:
            if self.mode_var.get() in ("1", "3"):
                systems = self._find_systems(roms_root)
            else:
                systems = self._find_systems_images(roms_root)
        except Exception:
            if is_unc:
                self._show_credentials_if_needed(True)
                messagebox.showwarning(
                    ui["msg_warning_title"],
                    ui["unc_access_error_msg"],
                )
                return
            raise

        # Bug 6 : peupler les DEUX listboxes (Main ET Avancé)
        for listbox_attr in ("sys_list", "sys_list_adv"):
            lb = getattr(self, listbox_attr, None)
            if lb is None:
                continue
            if not lb.winfo_exists():
                continue
            lb.delete(0, "end")

            lb.insert("end", ui["sys_sel_opt_all"])
            lb.insert("end", ui["sys_sel_opt_none"])
            lb.insert("end", " ")

            for sys_path in systems:
                lb.insert("end", sys_path.name)

            try:
                lb.yview_moveto(0.0)
            except Exception:
                pass

        self._update_sys_box_decor()

    # ──────────────────────────────────────────────────────────────────────────
    # NAS credentials (only if UNC at start)
    # ──────────────────────────────────────────────────────────────────────────
    def _ensure_credentials_widgets(self) -> None:
        if hasattr(self, "creds_frame"):
            return

        ui = self._get_ui_t()

        # credentials panel under the mode details (right panel)
        self.creds_frame = tk.Frame(
            self.right, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8
        )
        self.creds_frame.pack(fill="x", pady=(12, 0))

        lbl_title = tk.Label(
            self.creds_frame,
            text=ui["nas_creds_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        )
        lbl_title.grid(row=0, column=0, columnspan=2, sticky="w")
        lbl_title._i18n_key = "nas_creds_title"

        lbl_user = tk.Label(
            self.creds_frame,
            text=ui["nas_user_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        lbl_user.grid(row=1, column=0, sticky="w", pady=(8, 2))
        lbl_user._i18n_key = "nas_user_label"

        self.ent_nas_user = tk.Entry(self.creds_frame, width=22)
        self.ent_nas_user.grid(row=2, column=0, sticky="w", pady=2)

        lbl_pass = tk.Label(
            self.creds_frame,
            text=ui["nas_password_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        lbl_pass.grid(row=1, column=1, sticky="w", pady=(8, 2), padx=(10, 0))
        lbl_pass._i18n_key = "nas_password_label"

        self.ent_nas_pass = tk.Entry(self.creds_frame, width=22, show="*")
        self.ent_nas_pass.grid(row=2, column=1, sticky="w", pady=2)

    def _show_credentials_if_needed(self, is_unc: bool) -> None:
        self._ensure_credentials_widgets()
        if is_unc:
            self.creds_frame.pack(fill="x", pady=(12, 0))
        else:
            self.creds_frame.pack_forget()

    def _get_nas_credentials_or_warn(self) -> Optional[tuple[str, str]]:
        self._ensure_credentials_widgets()
        user = self.ent_nas_user.get().strip()
        pwd = self.ent_nas_pass.get()
        if not user or not pwd:
            # Pas de messagebox au clic "Démarrer" : l'utilisateur doit saisir
            # les identifiants dans le panneau NAS.
            try:
                if not user:
                    self.ent_nas_user.focus_set()
                else:
                    self.ent_nas_pass.focus_set()
            except Exception:
                pass
            return None
        return user, pwd

    # ──────────────────────────────────────────────────────────────────────────
    # progress
    # ──────────────────────────────────────────────────────────────────────────
    def _build_progress_frame(self, parent: tk.Frame) -> None:
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])

        frm = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=10, pady=10)
        frm.pack(fill="x", padx=10, pady=(0, 10))
        # Fixe la largeur de la colonne texte (évite que les labels longs "poussent" la barre boutons)
        frm.grid_columnconfigure(0, minsize=420, weight=0)
        frm.grid_columnconfigure(1, weight=1)

        self.progress_title_lbl = tk.Label(
            frm,
            text=ui["progress_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        )
        self.progress_title_lbl.grid(row=0, column=0, sticky="w")

        self.progress_var = tk.StringVar(value="—")
        self.progress_sub_var = tk.StringVar(value="")
        self.progress_pct_var = tk.StringVar(value="0%")

        # Largeur FIXE : sticky="we" sur la colonne 0 (deja figee a
        # minsize=420 par frm.grid_columnconfigure ci-dessus) -- pas de
        # width= en caracteres, qui donnerait une largeur en pixels
        # differente entre les 2 lignes puisqu'elles n'ont pas la meme
        # taille de police (10pt vs 9pt).
        tk.Label(
            frm,
            textvariable=self.progress_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
            wraplength=0,
            justify="left",
            anchor="w",
        ).grid(row=1, column=0, sticky="we", pady=4)

        tk.Label(
            frm,
            textvariable=self.progress_sub_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
            # Ne jamais wrapper : sinon la hauteur bouge et repousse la barre/boutons.
            wraplength=9999,
            justify="left",
            anchor="w",
        ).grid(row=2, column=0, sticky="we")

        self.progress = ttk.Progressbar(
            frm, orient="horizontal", length=600, mode="determinate"
        )
        self.progress.grid(row=0, column=1, rowspan=3, padx=10, sticky="we")
        self.progress.configure(maximum=100)

        # Affiche un % centré sur la barre (ne déplace pas la mise en page)
        self.progress_pct_label = tk.Label(
            frm,
            textvariable=self.progress_pct_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self.progress_pct_label.place(
            in_=self.progress, relx=0.5, rely=0.5, anchor="center"
        )

        # Buttons under progression (main tab)
        controls = tk.Frame(frm, bg="#F3F3F3")
        controls.grid(row=3, column=0, columnspan=2, sticky="we", pady=(10, 0))

        # width=10 fixe (2026-08-05, demande utilisateur : boutons non
        # uniformes) -- en caracteres (unite Tk pour un Button texte, pas
        # des pixels), suffisant pour la plus longue traduction des 4
        # libelles sur les 3 langues ("Reanudar", 8 caracteres) + marge.
        # Sans cette largeur fixe, chaque bouton se dimensionne sur son
        # propre texte (ex. "Pause" vs "Reprise"/"Resume"/"Reanudar"),
        # visiblement inegal cote a cote.
        BTN_W = 10
        self.btn_pause_progress = tk.Button(
            controls,
            text=ui["btn_pause"],
            command=self._on_pause_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_pause_progress.grid(row=0, column=0, padx=6, pady=4, sticky="w")

        self.btn_resume_progress = tk.Button(
            controls,
            text=ui["btn_resume"],
            command=self._on_resume_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_resume_progress.grid(row=0, column=1, padx=6, pady=4, sticky="w")

        self.btn_skip_progress = tk.Button(
            controls,
            text=ui["btn_skip"],
            command=self._on_skip_clicked,
            bg="#FF5C5C",
            fg="black",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_skip_progress.grid(row=0, column=2, padx=6, pady=4, sticky="w")

        self.btn_stop_progress = tk.Button(
            controls,
            text=ui["btn_stop"],
            command=self._on_stop_clicked,
            bg="#B100FF",
            fg="white",
            bd=2,
            relief="solid",
            width=BTN_W,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_stop_progress.grid(row=0, column=3, padx=6, pady=4, sticky="w")

    def _progress_cb_ui(self, kind: str, idx: int, total: int, label: str = "") -> None:
        total = max(total, 1)
        pct = int((idx / total) * 100)
        self.progress.configure(maximum=100, value=pct)
        self.progress_pct_var.set(f"{pct}%")

        # Ligne 1/2 stables :
        # - "extraction" : LINE 1 = système, LINE 2 vide
        # - "extraction_imgs" : LINE 2 seulement (pas de modification de la ligne 1)
        # - autres kinds : LINE 1 = étape, LINE 2 = label tronqué
        if kind == "extraction":
            # Ligne 1 : système (sans idx/total afin que l'affichage ne dérive pas)
            sys_name = label or ""
            max_sys_len = 38
            if len(sys_name) > max_sys_len:
                sys_name = sys_name[: max_sys_len - 1] + "…"

            title = "Extraction"
            if sys_name:
                title = f"{title} — {sys_name}"

            self._last_progress_detail = title
            self.progress_var.set(title)

            # Ligne 2 : vide, sera remplie par extraction_imgs
            self.progress_sub_var.set("")
            return

        if kind == "copy_sd":
            # Copie SD : titre fixe, ligne 2 = fichier en cours
            self.progress_var.set("Copie SD")
            # Tronque la ligne COMPLETE (prefixe "idx/total " inclus), pas
            # seulement le nom de fichier -- bug corrige 2026-08-05 (demande
            # utilisateur : cette ligne "pousse" la barre du haut et la
            # barre de progression). Le prefixe grandit avec le nombre
            # total de fichiers ("2842/2842 " = 10 caracteres, jamais compte
            # dans l'ancienne limite de 55 appliquee au seul nom de fichier)
            # -- sur un gros lot (banque de GIFs, ~2800 fichiers), le texte
            # complet depassait la largeur fixe de la colonne (minsize=420),
            # forcant la colonne (et donc la barre de progression juste a
            # cote, en colonne 1) a s'elargir.
            prefix = f"{idx}/{total} "
            shown = label or ""
            max_total_len = 55
            max_shown_len = max(0, max_total_len - len(prefix))
            if len(shown) > max_shown_len:
                shown = shown[: max(0, max_shown_len - 1)] + "…"
            self.progress_sub_var.set(prefix + shown)
            return

        if kind == "extraction_imgs":
            # Ligne 2 uniquement : images copiées/total + fichier en cours (tronqué)
            shown = label or ""
            max_len = 55
            if len(shown) > max_len:
                shown = shown[: max_len - 1] + "…"
            self.progress_sub_var.set(shown)
            return

        # Autres étapes
        if kind == "conversion":
            title = f"Conversion {idx}/{total}"
        elif kind == "cache":
            title = f"Cache {idx}/{total}"
        elif kind == "download_defaults":
            title = f"Download defaults {idx}/{total}"
        elif kind == "systems_cache":
            title = f"systems_cache {idx}/{total}"
        elif kind == "mode8_check":
            title = f"Verification Mode 8 — {idx}/{total}"
        elif kind == "final_media":
            title = f"Comparaison support final — {idx}/{total}"
        elif kind == "clean_scrape_media":
            title = f"Nettoyage avant scrape — {idx}/{total}"
        else:
            title = f"{kind} {idx}/{total}"

        shown = label or ""
        max_len = 55
        if len(shown) > max_len:
            shown = shown[: max_len - 1] + "…"

        self._last_progress_detail = title
        self.progress_var.set(title)
        self.progress_sub_var.set(shown)
        return

    def _progress_cb(self, kind: str, idx: int, total: int, label: str = "") -> None:
        self.root.after(0, self._progress_cb_ui, kind, idx, total, label)

    # ──────────────────────────────────────────────────────────────────────────
    # logs polling
    # ──────────────────────────────────────────────────────────────────────────
    # ── Log window buffer ───────────────────────────────────────────────
    _LOG_WINDOW_SIZE = 500  # lignes visibles max dans le widget Text
    _LOG_STEP = 250  # de combien on recule/avance au scroll

    def _poll_logs(self) -> None:
        need_rebuild = False
        try:
            while True:
                line = self._log_q.get_nowait()
                if self._append_log(line):
                    need_rebuild = True
        except queue.Empty:
            pass
        if need_rebuild:
            ui = self._get_ui_t()
            level_filter = (
                self.log_level_var.get()
                if hasattr(self, "log_level_var")
                else ui["logs_level_all"]
            )
            if level_filter != ui["logs_level_all"]:
                self._rebuild_log_display(go_end=True)
        # Ne pas se replanifier si la fenêtre a été détruite (fermeture de
        # l'appli), sinon Tkinter lève "invalid command name ..._poll_logs".
        try:
            if self.root.winfo_exists():
                self.root.after(100, self._poll_logs)
        except tk.TclError:
            pass

    def _detect_log_level(self, s: str) -> str:
        """Détecte le niveau de log à partir de la chaîne.
        Cherche d'abord des emojis, puis des mots-clés."""
        s_upper = s.upper()
        # Priorité emoji (❌ = erreur, ⚠️ = warning, ✅ = success/info)
        if "❌" in s or "ERREUR" in s_upper or "ERROR" in s_upper:
            return "ERROR"
        if (
            "⚠️" in s
            or "WARNING" in s_upper
            or "WARN" in s_upper
            or "ALERTE" in s_upper
        ):
            return "WARNING"
        if "DEBUG" in s_upper:
            return "DEBUG"
        return "INFO"

    def _append_log(self, s: str) -> bool:
        # Stocker toutes les lignes en mémoire avec leur niveau
        if not hasattr(self, "_log_lines"):
            self._log_lines = []  # chaque entrée : {"text": str, "level": str}
            self._log_display_start = 0

        level = self._detect_log_level(s)
        self._log_lines.append({"text": s, "level": level})
        self._write_log_file(s)

        ui = self._get_ui_t()
        level_filter = (
            self.log_level_var.get()
            if hasattr(self, "log_level_var")
            else ui["logs_level_all"]
        )
        total = len(self._log_lines)

        # Si un filtre est actif, reconstruire tout l'affichage filtré à chaque ligne
        if level_filter != ui["logs_level_all"]:
            need_rebuild = False
            if level_filter == ui["logs_level_err"] and level == "ERROR":
                need_rebuild = True
            elif level_filter == ui["logs_level_warn_err"] and level in ("ERROR", "WARNING"):
                need_rebuild = True
            self._update_log_count()
            return need_rebuild

        # Mode "Tout" : ajout direct avec fenêtre glissante
        if self._log_display_start + self._LOG_WINDOW_SIZE >= total - 1:
            self.text.insert("end", s)
            self.text.see("end")
            try:
                lines_in_widget = int(self.text.index("end-1c").split(".")[0])
                if lines_in_widget > self._LOG_WINDOW_SIZE:
                    self._log_display_start = max(0, total - self._LOG_WINDOW_SIZE)
                    self._rebuild_log_display(go_end=True)
            except Exception:
                pass

        self._update_log_count()
        return False

    def _update_log_count(self) -> None:
        """Met à jour le label compteur de log."""
        try:
            total = len(self._log_lines) if hasattr(self, "_log_lines") else 0
            errors = sum(1 for l in self._log_lines if l["level"] == "ERROR")
            warnings = sum(1 for l in self._log_lines if l["level"] == "WARNING")
            self.log_count_lbl.config(
                text=f"{total} lignes  ({errors} ⚠️ {warnings} ⚡)"
            )
        except Exception:
            pass

    def _rebuild_log_display(self, go_end: bool = False) -> None:
        """Reconstruit le widget en filtrant par niveau et en fenêtre glissante."""
        try:
            self.text.delete("1.0", "end")
            if not hasattr(self, "_log_lines") or not self._log_lines:
                return

            ui = self._get_ui_t()
            level_filter = (
                self.log_level_var.get()
                if hasattr(self, "log_level_var")
                else ui["logs_level_all"]
            )

            # Construire une liste des indices filtrés
            filtered_indices = []
            for i, entry in enumerate(self._log_lines):
                if isinstance(entry, dict):
                    lvl = entry.get("level", "INFO")
                else:
                    lvl = "INFO"
                if level_filter == ui["logs_level_all"]:
                    filtered_indices.append(i)
                elif level_filter == ui["logs_level_warn_err"] and lvl in ("WARNING", "ERROR"):
                    filtered_indices.append(i)
                elif level_filter == ui["logs_level_err"] and lvl == "ERROR":
                    filtered_indices.append(i)

            total_filtered = len(filtered_indices)
            if total_filtered == 0:
                self._update_log_count()
                return

            # En mode filtré : afficher TOUT (pas de fenêtre glissante)
            if level_filter != ui["logs_level_all"]:
                for i in range(total_filtered):
                    idx = filtered_indices[i]
                    entry = self._log_lines[idx]
                    text = entry["text"] if isinstance(entry, dict) else entry
                    self.text.insert("end", text)
                if go_end:
                    self.text.see("end")
                else:
                    self.text.see("1.0")
                self._update_log_count()
                return

            # Mode "Tout" : fenêtre glissante normale
            start = max(0, min(self._log_display_start, total_filtered - 1))
            end = min(start + self._LOG_WINDOW_SIZE, total_filtered)

            for i in range(start, end):
                idx = filtered_indices[i]
                entry = self._log_lines[idx]
                text = entry["text"] if isinstance(entry, dict) else entry
                self.text.insert("end", text)

            if go_end:
                self.text.see("end")
            else:
                self.text.see("1.0")

            self._update_log_count()
        except Exception:
            pass

    def _on_log_yscroll(self, *args) -> None:
        """Callback de la scrollbar verticale des logs.
        Délègue d'abord le défilement normal au widget, puis
        vérifie si on doit recharger des lignes plus anciennes/récentes.
        """
        try:
            # Déléguer le défilement normal
            self.text.yview(*args)
        except Exception:
            pass

        # Vérifier la position après défilement
        try:
            if not hasattr(self, "_log_lines") or not self._log_lines:
                return
            frac = self.text.yview()
            first_vis = float(frac[0])
            last_vis = float(frac[1])

            # Calculer le nombre total d'éléments affichables (filtrés ou non)
            ui = self._get_ui_t()
            level_filter = (
                self.log_level_var.get()
                if hasattr(self, "log_level_var")
                else ui["logs_level_all"]
            )
            if level_filter == ui["logs_level_all"]:
                total = len(self._log_lines)
            else:
                total = sum(
                    1
                    for l in self._log_lines
                    if isinstance(l, dict)
                    and (
                        level_filter == ui["logs_level_warn_err"]
                        and l.get("level", "") in ("WARNING", "ERROR")
                        or level_filter == ui["logs_level_err"]
                        and l.get("level", "") == "ERROR"
                    )
                )

            # Scroll vers le haut (atteint le début visible)
            if first_vis <= 0.002 and self._log_display_start > 0:
                new_start = max(0, self._log_display_start - self._LOG_STEP)
                self._log_display_start = new_start
                self._rebuild_log_display(go_end=False)
                try:
                    self.text.yview_moveto(0.0)
                except Exception:
                    pass

            # Scroll vers le bas (atteint la fin visible)
            elif (
                last_vis >= 0.998
                and self._log_display_start + self._LOG_WINDOW_SIZE < total
            ):
                new_start = min(
                    total - self._LOG_WINDOW_SIZE,
                    self._log_display_start + self._LOG_STEP,
                )
                self._log_display_start = new_start
                self._rebuild_log_display(go_end=True)
        except Exception:
            pass

    def _write_log_file(self, s: str) -> None:
        """Écrit une ligne dans le fichier de log complet."""
        try:
            if not hasattr(self, "_log_file_path") or self._log_file_path is None:
                logs_dir = self.sd_dir / "logs"
                logs_dir.mkdir(parents=True, exist_ok=True)
                import datetime

                ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                self._log_file_path = logs_dir / f"session_{ts}.log"
            with open(self._log_file_path, "a", encoding="utf-8") as f:
                f.write(s)
        except Exception:
            pass

    def _open_log_file(self) -> None:
        """Ouvre le fichier de log complet dans le bloc-notes."""
        try:
            os.startfile(str(self._log_file_path))
        except Exception as e:
            print(f"Impossible d'ouvrir le fichier de log : {e}")

    # ──────────────────────────────────────────────────────────────────────────
    # pause/skip/stop
    # ──────────────────────────────────────────────────────────────────────────
    def _on_pause_clicked(self) -> None:
        self._safe_invoke_pause(self.tkmod.PAUSE.request_pause, "pause")

    def _on_resume_clicked(self) -> None:
        self._safe_invoke_pause(self.tkmod.PAUSE.request_resume, "resume")

    def _on_skip_clicked(self) -> None:
        self._safe_invoke_pause(self.tkmod.PAUSE.request_skip, "skip")

    def _on_stop_clicked(self) -> None:
        self._safe_invoke_pause(self.tkmod.PAUSE.request_stop, "stop")

    def _safe_invoke_pause(self, fn: Callable[[], None], kind: str) -> None:
        try:
            fn()
            # Ne pas écraser la progression détaillée (elle vient des progress_cb du worker).
            # On ne met un label générique que si aucun détail n'a jamais été affiché.
            detail = getattr(self, "_last_progress_detail", None)
            if not detail:
                self.progress_var.set(f"Commande: {kind}")
        except Exception as e:
            messagebox.showerror(self._get_ui_t()["msg_error_title"], str(e))

    def _theme_colors(self) -> dict:
        """Couleurs du theme actif (RecalBoxDMD_themes.get_theme()["colors"]),
        pour que les popups custom (galerie image de secours, _themed_info/
        _themed_yesno) suivent le theme choisi (clair/sombre) au lieu de
        couleurs fixes -- retour utilisateur en test reel (2026-07-21) :
        popup blanche affichee alors que le theme actif est sombre."""
        theme = themes.get_theme(getattr(self, "_current_theme_name", None) or "default")
        return theme.get("colors", {})

    def _themed_info(self, title: str, message: str) -> None:
        """
        Popup d'information au style/theme de l'appli (meme convention que
        la galerie "Image de secours", _on_default_image_picker_clicked) au
        lieu de messagebox.showinfo() : les boutons d'un messagebox natif
        sont rendus par Windows dans la langue systeme (pas forcement celle
        de l'appli) et son apparence (gris natif fixe) ne suit pas le theme
        actif -- retour utilisateur en test reel (2026-07-21).
        """
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        dlg = tk.Toplevel(self.root)
        dlg.title(title)
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=message, bg=bg, fg=fg,
            font=("TkDefaultFont", 9), wraplength=420, justify="left",
        ).pack(anchor="w", pady=(0, 12))
        tk.Button(
            body, text=ui["btn_close"], command=dlg.destroy,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(fill="x")
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)

    def _themed_yesno(self, title: str, message: str, linked: bool = False) -> bool:
        """Meme principe que _themed_info() mais Oui/Non, retourne le choix.
        linked=True (2026-08-11) : le message est affiche via un widget Text
        avec auto-lien (_insert_autolink_text()) au lieu d'un simple Label --
        utilise quand le message contient une URL a rendre cliquable (ex:
        pack GIFs Mode 1, qui pointe vers le pack "ultimate" externe).
        Comportement inchange (Label) pour tous les autres appelants."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        result = {"value": False}
        dlg = tk.Toplevel(self.root)
        dlg.title(title)
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        if linked:
            msg_text = self._make_linked_text(body, bg=bg, fg=fg, width=56)
            msg_text.pack(anchor="w", fill="x", pady=(0, 12))
            self._insert_autolink_text(msg_text, message)
        else:
            tk.Label(
                body, text=message, bg=bg, fg=fg,
                font=("TkDefaultFont", 9), wraplength=420, justify="left",
            ).pack(anchor="w", pady=(0, 12))
        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")

        def _yes():
            result["value"] = True
            dlg.destroy()

        def _no():
            result["value"] = False
            dlg.destroy()

        tk.Button(
            btns, text=ui["open_output_yes"], command=_yes,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", expand=True, fill="x", padx=(0, 4))
        tk.Button(
            btns, text=ui["open_output_no"], command=_no,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="right", expand=True, fill="x", padx=(4, 0))
        dlg.protocol("WM_DELETE_WINDOW", _no)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)
        return result["value"]

    def _make_linked_text(self, parent: tk.Widget, bg: str, fg: str, width: int) -> tk.Text:
        """Cree un widget Text en lecture seule (liens cliquables actifs,
        pas de saisie -- meme pattern que self.help_text, voir
        _build_help_tab()/aide), stylise en petite carte (bord fin, comme
        path_box ailleurs dans ce fichier). A remplir via
        _insert_autolink_text()."""
        tw = tk.Text(
            parent, wrap="word", bg=bg, fg=fg, bd=2, relief="solid",
            highlightthickness=0, font=("TkDefaultFont", 9),
            width=width, height=1, cursor="arrow", padx=6, pady=6,
        )
        tw.bind("<Key>", lambda e: "break")
        return tw

    def _insert_autolink_text(self, text_widget: tk.Text, raw_text: str, link_fg: Optional[str] = None) -> None:
        """Insere raw_text tel quel dans text_widget (deja en wrap="word"),
        en rendant cliquables (ouverture navigateur) les URLs http(s)
        qu'il contient -- auto-lien par regex, PAS un rendu markdown complet
        (voir _AUTOLINK_URL_RE). Ajuste aussi la hauteur du widget au
        nombre de lignes affichees apres retour a la ligne (les widgets
        Text, contrairement a Label, n'ont pas de hauteur auto)."""
        if link_fg is None:
            try:
                link_fg = md_renderer._derive_markdown_colors(
                    text_widget, text_widget.cget("bg"), text_widget.cget("fg")
                )["link"]
            except Exception:
                link_fg = "#1565C0"
        text_widget.configure(state="normal")
        text_widget.delete("1.0", "end")
        pos = 0
        link_n = 0
        for m in _AUTOLINK_URL_RE.finditer(raw_text):
            if m.start() > pos:
                text_widget.insert("end", raw_text[pos:m.start()])
            url = m.group(0)
            # Ponctuation de fin de phrase collee a l'URL (ex. "...67065.")
            # ne fait pas partie du lien.
            trail = ""
            while url and url[-1] in ".,;:)":
                trail = url[-1] + trail
                url = url[:-1]
            link_n += 1
            tag = f"autolink_{id(text_widget)}_{link_n}"
            text_widget.insert("end", url, (tag,))
            text_widget.tag_configure(tag, foreground=link_fg, underline=True)
            text_widget.tag_bind(tag, "<Button-1>", lambda e, u=url: webbrowser.open_new_tab(u))
            text_widget.tag_bind(tag, "<Enter>", lambda e, w=text_widget: w.configure(cursor="hand2"))
            text_widget.tag_bind(tag, "<Leave>", lambda e, w=text_widget: w.configure(cursor="arrow"))
            if trail:
                text_widget.insert("end", trail)
            pos = m.end()
        if pos < len(raw_text):
            text_widget.insert("end", raw_text[pos:])
        # update() (pas juste update_idletasks()) : necessaire pour qu'un
        # widget tout juste cree dans un Toplevel pas encore mappe/affiche
        # obtienne une largeur en pixels fiable avant la mesure -- sinon
        # wrap="word" se rabat sur une largeur quasi nulle et
        # .count(displaylines) donne un resultat aberrant (bug reel trouve
        # en test : popup mesuree a plusieurs milliers de lignes de haut,
        # placee hors ecran). Widgets deja mappes de longue date (ex.
        # mode_desc_label, rappele a chaque changement de mode) ne sont pas
        # affectes par ce risque mais update() reste sans danger pour eux.
        text_widget.update()
        # Garde-fou supplementaire (2026-08-11) : meme apres update(), un
        # widget dont la fenetre parente n'a pas encore termine sa toute
        # premiere passe de geometrie peut renvoyer une largeur quasi nulle
        # (bug reel observe : cadre Progression partage pousse hors de la
        # fenetre a taille fixe des le premier affichage de l'onglet Main,
        # mode_desc_label mesure contre une largeur non fiable). Si la
        # largeur rendue est suspecte, on n'ajuste PAS la hauteur cette
        # fois -- un appel ulterieur (changement de mode/theme/onglet, deja
        # frequent dans ce fichier) la corrigera avec une largeur fiable.
        # Mieux vaut une hauteur temporairement legerement fausse (ancienne
        # valeur conservee) qu'un calcul aberrant contre une largeur ~0.
        widget_w = text_widget.winfo_width()
        if widget_w < 50:
            return
        lines = text_widget.count("1.0", "end", "displaylines")
        text_widget.configure(height=(lines[0] if lines else 1))

    def _themed_choice(self, title: str, message: str, yes_label: str, no_label: str) -> bool:
        """Comme _themed_yesno() mais avec des libelles de bouton
        personnalises (utilise quand la question n'est pas un simple
        oui/non generique, ex: "ressaisir l'IP" / "passer en Mode 9")."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        result = {"value": False}
        dlg = tk.Toplevel(self.root)
        dlg.title(title)
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=message, bg=bg, fg=fg,
            font=("TkDefaultFont", 9), wraplength=420, justify="left",
        ).pack(anchor="w", pady=(0, 12))
        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")

        def _yes():
            result["value"] = True
            dlg.destroy()

        def _no():
            result["value"] = False
            dlg.destroy()

        tk.Button(
            btns, text=yes_label, command=_yes,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", expand=True, fill="x", padx=(0, 4))
        tk.Button(
            btns, text=no_label, command=_no,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="right", expand=True, fill="x", padx=(4, 0))
        dlg.protocol("WM_DELETE_WINDOW", _no)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)
        return result["value"]

    def _prompt_systems_image_lang_dialog(self) -> str:
        """Popup themee : choix de la langue des images systemes/genres
        (_defaults/) telechargees depuis GitHub -- "en"/"fr"/"es". Affiche
        une miniature comparative bundlee (tools/assets/lang_preview/) a
        titre d'exemple. Pre-selectionne le dernier choix sauvegarde
        (prefs "systems_image_lang") ; le choix fait ici est aussitot
        persiste, quel que soit le bouton clique. Fermer le dialogue (X)
        conserve le dernier choix sans le changer."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        current = prefs.get("systems_image_lang") or "en"
        result = {"value": current}

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["lang_images_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=ui["lang_images_msg"], bg=bg, fg=fg,
            font=("TkDefaultFont", 9), wraplength=460, justify="left",
        ).pack(anchor="w", pady=(0, 10))

        img_path = Path(__file__).resolve().parent / "assets" / "lang_preview" / "compare_systems_lang.png"
        if img_path.exists():
            try:
                from PIL import Image, ImageTk

                img = Image.open(img_path)
                max_w = 460
                if img.width > max_w:
                    ratio = max_w / img.width
                    img = img.resize((max_w, int(img.height * ratio)))
                photo = ImageTk.PhotoImage(img)
                lbl_img = tk.Label(body, image=photo, bg=bg, bd=2, relief="solid")
                lbl_img.image = photo  # garder une reference (evite le garbage collect)
                lbl_img.pack(pady=(0, 12))
            except Exception:
                pass

        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")

        def _pick(value: str):
            result["value"] = value
            prefs.set("systems_image_lang", value)
            dlg.destroy()

        for value, key in (("en", "lang_images_en"), ("fr", "lang_images_fr"), ("es", "lang_images_es")):
            is_current = value == current
            tk.Button(
                btns, text=ui[key], command=lambda v=value: _pick(v),
                bg=bg_action if is_current else bg_normal, fg="#000000" if is_current else fg,
                bd=2, relief="solid", padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
            ).pack(side="left", expand=True, fill="x", padx=2)

        dlg.protocol("WM_DELETE_WINDOW", dlg.destroy)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)
        return result["value"]

    def _prompt_recalbox_ip_dialog(self, default: str = "") -> Optional[str]:
        """Popup themee avec un champ de saisie pour l'IP/nom reseau de la
        Recalbox (remplace un simpledialog.askstring natif, non theme et
        non traduisible). Retourne la valeur saisie (strip), ou None si
        annule/vide."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        result = {"value": None}
        dlg = tk.Toplevel(self.root)
        dlg.title(ui["mode1_manual_ip_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=ui["mode1_manual_ip_prompt"], bg=bg, fg=fg,
            font=("TkDefaultFont", 9), wraplength=380, justify="left",
        ).pack(anchor="w", pady=(0, 10))
        entry_var = tk.StringVar(value=default)
        entry = tk.Entry(body, textvariable=entry_var, font=("TkDefaultFont", 10))
        entry.pack(fill="x", pady=(0, 12))
        entry.select_range(0, "end")
        entry.focus_set()

        def _ok(event=None):
            val = entry_var.get().strip()
            result["value"] = val or None
            dlg.destroy()

        def _cancel():
            result["value"] = None
            dlg.destroy()

        entry.bind("<Return>", _ok)
        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")
        tk.Button(
            btns, text=ui["mode1_manual_ip_ok"], command=_ok,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", expand=True, fill="x", padx=(0, 4))
        tk.Button(
            btns, text=ui["mode1_manual_ip_cancel"], command=_cancel,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="right", expand=True, fill="x", padx=(4, 0))
        dlg.protocol("WM_DELETE_WINDOW", _cancel)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)
        return result["value"]

    def _prompt_sd_card_dialog(self, min_gb: float = 8.0) -> Optional[str]:
        """Popup themee : liste les lecteurs amovibles (via
        _list_removable_drives_ex()), valide FAT32 + taille minimale au
        clic sur OK (check_drive_fat32_and_min_size), avec bouton
        "Rafraichir" pour rescanner sans fermer le dialogue. Retourne la
        lettre validee ("X:"), ou None si annule/ferme."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        result = {"value": None}
        drives: list = []

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["sdcard_dialog_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)
        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)
        tk.Label(
            body, text=ui["sdcard_dialog_prompt"](min_gb), bg=bg, fg=fg,
            font=("TkDefaultFont", 9), wraplength=420, justify="left",
        ).pack(anchor="w", pady=(0, 10))

        list_frame = tk.Frame(body, bg=bg)
        list_frame.pack(fill="both", expand=True, pady=(0, 8))
        listbox = tk.Listbox(list_frame, height=6, exportselection=False, font=("TkDefaultFont", 9))
        listbox.pack(side="left", fill="both", expand=True)
        scrollbar = ttk.Scrollbar(list_frame, orient="vertical", command=listbox.yview)
        scrollbar.pack(side="right", fill="y")
        listbox.configure(yscrollcommand=scrollbar.set)

        error_lbl = tk.Label(
            body, text="", bg=bg, fg="#D32F2F",
            font=("TkDefaultFont", 9), wraplength=420, justify="left",
        )
        error_lbl.pack(anchor="w", pady=(0, 8))

        def _rescan():
            error_lbl.configure(text="")
            listbox.delete(0, "end")
            drives.clear()
            try:
                found = self.tkmod._list_removable_drives_ex()  # type: ignore[attr-defined]
            except Exception:
                found = []
            drives.extend(found)
            for letter, label, size_s, fs in drives:
                listbox.insert("end", f"{letter}  [{label}]  {size_s}  {fs}")
            if drives:
                listbox.selection_set(0)

        def _ok():
            sel = listbox.curselection()
            if not sel or sel[0] >= len(drives):
                error_lbl.configure(text=ui["sdcard_dialog_none_selected"])
                return
            letter = drives[sel[0]][0]
            ok, reason = self.tkmod.check_drive_fat32_and_min_size(letter, min_gb=min_gb)
            if not ok:
                if reason == "drive_not_found":
                    error_lbl.configure(text=ui["sdcard_dialog_err_notfound"])
                elif reason.startswith("filesystem="):
                    error_lbl.configure(text=ui["sdcard_dialog_err_fs"](reason.split("=", 1)[1]))
                elif reason.startswith("size_gb="):
                    error_lbl.configure(text=ui["sdcard_dialog_err_size"](reason.split("=", 1)[1], min_gb))
                else:
                    error_lbl.configure(text=reason)
                return
            result["value"] = letter
            dlg.destroy()

        def _cancel():
            result["value"] = None
            dlg.destroy()

        btns_top = tk.Frame(body, bg=bg)
        btns_top.pack(fill="x", pady=(0, 8))
        tk.Button(
            btns_top, text=ui["sdcard_dialog_refresh_btn"], command=_rescan,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=4, font=("TkDefaultFont", 9),
        ).pack(fill="x")

        btns = tk.Frame(body, bg=bg)
        btns.pack(fill="x")
        tk.Button(
            btns, text=ui["sdcard_dialog_ok_btn"], command=_ok,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", expand=True, fill="x", padx=(0, 4))
        tk.Button(
            btns, text=ui["sdcard_dialog_cancel_btn"], command=_cancel,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(side="right", expand=True, fill="x", padx=(4, 0))
        dlg.protocol("WM_DELETE_WINDOW", _cancel)

        _rescan()
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        self.root.wait_window(dlg)
        return result["value"]

    # ──────────────────────────────────────────────────────────────────────────
    # worker
    # ──────────────────────────────────────────────────────────────────────────
    def _on_start_clicked(self) -> None:
        if self._worker and self._worker.is_alive():
            ui0 = self._get_ui_t()
            messagebox.showwarning(ui0["msg_warning_title"], ui0["processing_in_progress_msg"])
            return

        mode = self.mode_var.get()

        # Nouveau run : reaffiche le descriptif complet (avec "Marche a
        # suivre") si un run precedent l'avait tronque une fois la copie
        # SD atteinte (voir _start_mode6_blinking/_update_mode_desc).
        if getattr(self, "_mode1_sd_copy_active", False):
            self._mode1_sd_copy_active = False
            self._update_mode_desc()

        # Mode 2/11 : pas besoin de dossier ROMs (telechargements GitHub
        # uniquement -- _defaults pour Mode 2, pack ~600 GIFs pour Mode 11).
        if mode in ("2", "11"):
            roms_root = self.sd_dir / "systems"
            is_unc = False

            if mode == "2":
                ui2 = self._get_ui_t()
                # Si des fichiers existent deja dans _defaults/, proposer
                # d'ecraser (recuperer les dernieres versions) ou de conserver
                # les fichiers actuels. Choix lu sur le thread principal (avant
                # de lancer le worker) car _pipeline_mode_2 s'execute dans un
                # thread d'arriere-plan, ou une messagebox ne serait pas sure.
                # "default.raw565" est toujours ecrase quel que soit ce choix
                # (voir toolkit.download_defaults).
                self._mode2_overwrite_existing = True
                defaults_dir = self.sd_dir / "systems" / "_defaults"
                if defaults_dir.exists() and any(defaults_dir.iterdir()):
                    # Popup themee (2026-08-11) -- messagebox.askyesno() est
                    # un rendu Windows natif fixe, ne suit pas le theme
                    # clair/sombre actif.
                    self._mode2_overwrite_existing = self._themed_yesno(
                        ui2["mode2_overwrite_title"], ui2["mode2_overwrite_msg"]
                    )

                # Image de secours personnalisee (default.raw565) -- ajoutee
                # ici car le Mode 2 telecharge aussi _defaults/
                # (download_defaults() re-ecrase default.raw565 a chaque
                # fois, voir sa docstring) et n'offrait jusqu'ici aucun
                # moyen de la definir sans passer par le Mode 1 complet.
                # Contrairement au Mode 1 (question oui/non, une seule fois
                # tant qu'aucun choix n'est enregistre), la galerie est
                # proposee SYSTEMATIQUEMENT ici, a chaque lancement du Mode
                # 2 -- demande explicite utilisateur. Fermer sans choisir
                # (bouton Fermer ou X) retombe alors sur le visuel par
                # defaut du projet (reset_on_close=True) plutot que de
                # laisser silencieusement un ancien choix personnalise en
                # place.
                self._on_default_image_picker_clicked(reset_on_close=True)

                # Langue des images systemes/genres telechargees (voir
                # toolkit.download_defaults(lang=...)) -- posee que le
                # telechargement soit un premier remplissage ou une mise a
                # jour, dans les deux cas download_defaults() est appele.
                self._systems_image_lang = self._prompt_systems_image_lang_dialog()
        else:
            # Verifie/alerte sur le dossier ROMs AVANT tout prompt Mode 1
            # (RB/image de secours, ci-dessous) : si aucun dossier n'est
            # choisi, l'alerte dossier ROMs doit etre la premiere chose vue
            # par l'utilisateur -- bug remonte ou le prompt fallback image
            # s'affichait avant l'alerte dossier ROMs manquant.
            roms_root = self._get_roms_root_or_warn()
            if roms_root is None:
                return
            is_unc = str(roms_root).startswith("\\\\")

        # Dossier de sortie (demandé pour Mode 4/5 afin d'éviter d'écrire dans le temp par défaut)
        self._final_output_dir = None
        if mode in ("4", "5"):
            lang = self.lang_var.get()
            title = (
                "Choisir dossier de sortie"
                if lang in ("fr", "es")
                else "Choose output folder"
            )
            out_dir = filedialog.askdirectory(title=title)
            if not out_dir:
                return
            self._final_output_dir = Path(out_dir)

        systems_selected: Optional[list[Path]] = None
        nas_user = ""
        nas_password = ""
        connect_unc_for_worker = False

        # On construit la liste système selon le mode, pour appliquer la sélection
        # Tout / Rien / Manuel.
        if mode in ("1", "3", "4", "5", "8"):
            try:
                systems_all = (
                    self._find_systems_images(roms_root)
                    if mode in ("4", "5")
                    else self._find_systems(roms_root)
                )
            except Exception:
                if not is_unc:
                    raise
                # Accès UNC nécessaire
                self._show_credentials_if_needed(True)
                creds = self._get_nas_credentials_or_warn()
                if creds is None:
                    return
                nas_user, nas_password = creds
                connect_unc_for_worker = True

                unc = str(roms_root)
                net_root = _unc_root(unc)
                try:
                    _net_use_connect(net_root, nas_user, nas_password)
                    systems_all = (
                        self._find_systems(roms_root)
                        if mode in ("1", "3")
                        else self._find_systems_images(roms_root)
                    )
                finally:
                    _net_use_disconnect(net_root)

            if not systems_all:
                _ui_early = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
                messagebox.showwarning(
                    _ui_early["msg_warning_title"],
                    _ui_early["no_systems_detected_msg"],
                )
                return

            # Bug 6 : lire la sélection depuis la listbox active (celle qui est visible)
            active_list = self.sys_list
            if hasattr(self, "sys_list_adv") and self.sys_list_adv.winfo_exists():
                try:
                    if self.tab_advanced.winfo_ismapped():
                        active_list = self.sys_list_adv
                except Exception:
                    pass
            selected_indices = list(active_list.curselection())

            # Sentinel mapping:
            # 0="Tout sélectionner" (italique)
            # 1="Ne rien sélectionner" (italique)
            # 2="" separator
            # systèmes à partir de 3
            ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])

            # Sélection robuste :
            # - "Ne rien sélectionner" (sentinel index 1) => vide (même si 0 est “collé”)
            # - si des lignes système réelles sont sélectionnées (>=3) => on ignore sentinel 0
            # - sinon => sentinel 0 => tous
            real_indices = [i - 3 for i in selected_indices if i >= 3]

            # Priorité : si l'utilisateur a réellement sélectionné des systèmes (>=3),
            # on ignore la sentinelle "Ne rien sélectionner" (index 1) tant que real_indices est non vide.
            if real_indices:
                systems_selected = [
                    systems_all[i] for i in real_indices if 0 <= i < len(systems_all)
                ]
            else:
                # Aucun système réel sélectionné :
                # - si "Tout sélectionner" (index 0) => on prend tous
                # - sinon => aucun
                systems_selected = systems_all if 0 in selected_indices else []

            # Si aucun système réel n’est sélectionné => avertir + return (pas de traitement)
            if not systems_selected:
                messagebox.showwarning(ui["msg_warning_title"], ui["sys_sel_warn_empty"])
                try:
                    self.sys_list.focus_set()
                except Exception:
                    pass
                return

        if mode == "1":
            # Deplace ICI (thread principal, apres validation du dossier
            # ROMs ET du choix des systemes a traiter -- bug remonte : ces
            # prompts s'affichaient avant l'alerte "aucun systeme
            # selectionne", donc pouvaient apparaitre pour rien si
            # l'utilisateur devait finalement corriger sa selection) au
            # lieu d'etre silencieuses/conditionnelles en plein pipeline :
            # le script marquee est indispensable au bon fonctionnement de
            # l'appareil (pas juste une option), donc son eventuelle
            # impossibilite (Recalbox non allumee/non detectee) doit etre
            # annoncee explicitement -- pas de blocage du pipeline pour
            # autant, juste une information + orientation vers le Mode 9
            # (Avance) pour la faire plus tard. Meme logique pour l'image de
            # secours : proposee explicitement au lieu d'etre silencieusement
            # remplacee par le visuel par defaut du projet.
            ui_pre = self._get_ui_t()
            # Cible retenue pour le pipeline (None = pas d'installation
            # reseau automatique -- les scripts sont neanmoins toujours
            # mis en scene localement, voir _pipeline_mode_1) : source
            # unique de verite, ne JAMAIS refaire une detection dans
            # _pipeline_mode_1() (piege deja rencontre -- deux resolutions
            # separees peuvent tomber sur des cibles differentes).
            #
            # Flux demande explicitement par l'utilisateur :
            # 1. detection auto -> proposition IP -> "oui" = on poursuit
            # 2. "non" (ou rien detecte) -> saisie manuelle IP + test de
            #    joignabilite -> "ok" = on poursuit
            # 3. echec (auto ou manuel) -> popup "injoignable" avec 2
            #    choix : ressaisir l'IP (boucle sur 2) ou passer en Mode 9
            #    plus tard (abandon, PAS de saisie forcee, PAS d'arret du
            #    pipeline).
            self._mode1_scripts_target = None
            # IP numerique resolue de la cible validee -- distincte de
            # _mode1_scripts_target qui peut etre "RECALBOX" (nom NetBIOS,
            # resolvable par Windows mais pas forcement par l'ESP32) : c'est
            # cette IP qui doit etre ecrite dans config.ini (recalbox_ip=,
            # deja utilisee par le firmware pour le MQTT + pre-remplissage
            # de la page web config).
            self._mode1_scripts_target_ip = None
            staged_dir_preview = self.sd_dir / "recalbox_userscripts"

            target = self.tkmod.detect_recalbox_share() or prefs.get("recalbox_ip")
            confirmed = False
            if target and self.tkmod.is_recalbox_reachable(target):
                # Affiche l'IP (pas "RECALBOX", identique quelle que soit
                # la Recalbox physique detectee via son nom NetBIOS) pour
                # que l'utilisateur puisse verifier qu'il s'agit bien de LA
                # sienne parmi plusieurs sur le reseau.
                display_ip = self.tkmod.resolve_recalbox_ip(target)
                if self._themed_yesno(
                    ui_pre["mode1_rb_confirm_title"],
                    ui_pre["mode1_rb_confirm_msg"](display_ip),
                ):
                    self._mode1_scripts_target = target
                    self._mode1_scripts_target_ip = display_ip
                    confirmed = True

            if not confirmed:
                manual_default = target or ""
                while True:
                    manual_ip = self._prompt_recalbox_ip_dialog(default=manual_default)
                    if not manual_ip:
                        # Saisie annulee/vide : pas de saisie forcee, pas
                        # d'arret du pipeline -- juste informer et
                        # continuer normalement.
                        self._themed_info(
                            ui_pre["mode1_rb_declined_title"],
                            ui_pre["mode1_rb_declined_msg"],
                        )
                        break
                    if self.tkmod.is_recalbox_reachable(manual_ip):
                        prefs.set("recalbox_ip", manual_ip)
                        self._mode1_scripts_target = manual_ip
                        self._mode1_scripts_target_ip = self.tkmod.resolve_recalbox_ip(manual_ip)
                        break
                    manual_default = manual_ip
                    retry = self._themed_choice(
                        ui_pre["mode1_rb_unreachable_title"],
                        ui_pre["mode1_rb_unreachable_msg"](manual_ip, staged_dir_preview),
                        ui_pre["mode1_rb_retry_ip_btn"],
                        ui_pre["mode1_rb_use_mode9_btn"],
                    )
                    if not retry:
                        self._themed_info(
                            ui_pre["mode1_rb_declined_title"],
                            ui_pre["mode1_rb_declined_msg"],
                        )
                        break
                    # sinon : reboucle et redemande une IP

            if not prefs.get("default_fallback_image"):
                if self._themed_yesno(
                    ui_pre["mode1_fallback_image_title"], ui_pre["mode1_fallback_image_msg"]
                ):
                    self._on_default_image_picker_clicked()

            # Langue des images systemes/genres telechargees depuis GitHub
            # (voir toolkit.download_defaults(lang=...)) -- meme question
            # qu'en Mode 2 (onglet Avance), posee ici pour le Mode 1 auto.
            self._systems_image_lang = self._prompt_systems_image_lang_dialog()

            # Banque de GIFs (pack GitHub + GIFs perso via l'onglet
            # Playlist) : question posee ICI (thread principal, avant
            # tout worker), pendant que l'utilisateur est encore devant
            # son ecran -- pas juste avant la copie SD finale. Carte
            # re-verifiee a CHAQUE lancement (jamais de cache d'une
            # verification precedente). Annuler ce dialogue abandonne
            # completement le lancement du Mode 1 (aucune re-tentative
            # automatique).
            sd_letter = self._prompt_sd_card_dialog(min_gb=8.0)
            if not sd_letter:
                return
            self._mode1_verified_sd_letter = sd_letter

            self._mode1_gifpack_bg_thread = None
            self._mode1_download_pack = self._themed_yesno(
                ui_pre["gifpack_q_title"], ui_pre["gifpack_q_msg"], linked=True
            )
            if self._mode1_download_pack:
                # Demande utilisateur : le telechargement du pack doit
                # demarrer DES le clic sur "oui" ici, pas seulement une
                # fois le pipeline reellement lance (potentiellement bien
                # plus tard, apres tout le detour par l'onglet Playlist
                # pour les GIFs perso) -- necessaire pour que les dossiers
                # du pack soient deja disponibles au moment de la phase de
                # construction de playlist (sous-mode "playlist"), qui
                # intervient juste apres l'ajout de dossiers personnels.
                # _pipeline_mode_1() attend ce thread (join) au lieu de
                # relancer le telechargement -- voir plus bas.
                self._start_gifpack_background_download()
            self._mode1_add_custom_gifs = self._themed_choice(
                ui_pre["customgifs_q_title"], ui_pre["customgifs_q_msg"],
                ui_pre["customgifs_q_yes"], ui_pre["customgifs_q_later"],
            )

        # Pour les autres modes, on conserve le comportement précédent (mais sans forcer NAS user/mdp ici).
        cfg = self._build_gui_config(
            mode, roms_root, systems_selected, nas_user, nas_password, is_unc, connect_unc_for_worker
        )

        if mode == "1" and self._mode1_add_custom_gifs:
            # Le lancement du pipeline est differe : cfg est deja construit
            # (capture toute la selection faite jusqu'ici), le worker ne
            # demarre qu'une fois l'utilisateur revenu de l'onglet Playlist
            # (mode temporaire) apres avoir regenere le cache playlist.
            self._mode1_deferred_launch_cb = lambda: self._launch_pipeline_worker(cfg)
            self._enter_playlist_temp_mode()
            return

        self._launch_pipeline_worker(cfg)

    def _build_gui_config(
        self,
        mode: str,
        roms_root: Optional[Path],
        systems_selected: Optional[Sequence[Path]],
        nas_user: Optional[str],
        nas_password: Optional[str],
        is_unc: bool,
        connect_unc_for_worker: bool,
    ) -> "GuiConfig":
        return GuiConfig(
            mode_choice=mode,
            roms_root=roms_root,
            systems_selected=cast(Optional[Sequence[Path]], systems_selected),
            nas_user=nas_user,
            nas_password=nas_password,
            nas_path_is_unc=(is_unc and connect_unc_for_worker),
        )

    def _start_gifpack_background_download(self) -> None:
        """Lance le telechargement du pack GitHub (~600 GIFs) en arriere-
        plan DES la reponse "oui" a la question (voir _on_start_clicked),
        sans attendre le lancement reel du pipeline Mode 1 -- necessaire
        pour que les dossiers du pack soient deja disponibles au moment
        de la phase de construction de playlist (sous-mode "playlist" de
        l'onglet Playlist), qui peut intervenir bien avant que le
        pipeline ne demarre reellement (detour par l'ajout de GIFs
        perso). Un seul worker actif a ce moment-la (le pipeline
        principal n'a pas encore demarre) -- pas de risque de conflit
        avec l'etat global PAUSE/redirection stdout. _pipeline_mode_1()
        attend ce thread (join) plutot que de retelecharger."""
        self.sd_dir.mkdir(parents=True, exist_ok=True)
        self._mode1_gifpack_bg_thread = self._start_worker(self._gifpack_bg_worker, args=())

    def _gifpack_bg_worker(self) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            try:
                self.tkmod.download_gif_pack(self.sd_dir, progress_cb=self._progress_cb, listen_keyboard=False)
            except Exception as e:
                print(f"[GUI] Échec téléchargement pack GIFs (arrière-plan, non bloquant) : {e}")
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

    def _launch_pipeline_worker(self, cfg: "GuiConfig") -> None:
        self.progress_var.set("Démarrage...")
        self.progress.configure(maximum=100, value=0)
        self.text.delete("1.0", "end")

        # _start_worker() bascule sur Logs, memorise l'onglet d'origine et
        # programme son retour automatique une fois le traitement termine.
        self._worker = self._start_worker(self._worker_main, args=(cfg,))

    def _worker_main(self, cfg: GuiConfig) -> None:
        toolkit = self.tkmod
        net_root = None

        if cfg.nas_path_is_unc:
            unc = str(cfg.roms_root)
            net_root = _unc_root(unc)
            try:
                _net_use_connect(net_root, cfg.nas_user, cfg.nas_password)
                print(f"[NAS] net use connect: {net_root}")
            except Exception as e:
                print(f"[NAS] net use connect failed: {e}")
                net_root = None

        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]

            try:
                toolkit.ensure_dependencies()
            except Exception:
                pass

            toolkit.PAUSE.request_resume()

            # Bug reel trouve (2026-08-04, utilisateur : "apres avoir
            # valide le dl des 600 gifs, pas de messages dans le log ni
            # de succes ni d'echec") : une exception non rattrapee ICI se
            # propage hors de ce bloc try -- le "finally" ci-dessous
            # restaure sys.stdout/sys.stderr AVANT que l'excepthook par
            # defaut du thread (threading.excepthook) n'imprime la
            # traceback, qui part donc vers la vraie console (invisible
            # pour l'utilisateur, log GUI vide) au lieu du log -- plus
            # AUCUNE etape suivante du pipeline ne s'execute, sans le
            # moindre message. Corrige en rattrapant explicitement ici,
            # PENDANT que stdout est encore redirige : desormais toute
            # panne imprevue d'une etape du pipeline reste au moins
            # visible dans le log au lieu de tuer le thread en silence.
            try:
                mode = cfg.mode_choice
                if mode == "1":
                    self._pipeline_mode_1(toolkit, cfg)
                elif mode == "2":
                    self._pipeline_mode_2(toolkit, cfg)
                elif mode == "3":
                    self._pipeline_mode_3(toolkit, cfg)
                elif mode == "4":
                    self._pipeline_mode_4(toolkit, cfg)
                elif mode == "5":
                    self._pipeline_mode_5(toolkit, cfg)
                elif mode == "6":
                    self._pipeline_mode_6(toolkit, cfg)
                elif mode in ("2", "7"):
                    self._pipeline_mode_7(toolkit, cfg)
                elif mode == "8":
                    self._pipeline_mode_8(toolkit, cfg)
                elif mode == "11":
                    self._pipeline_mode_11(toolkit, cfg)
                else:
                    print(f"Mode inconnu: {mode}")
            except Exception:
                import traceback
                print("[GUI] ERREUR INATTENDUE : le pipeline s'est arrêté prématurément.")
                print(traceback.format_exc())

            # Si l'utilisateur n'a pas demandé Stop, alors on "révèle" le choix 6
            # avec clignotement (via root.after car on est dans un thread worker).
            if not toolkit.PAUSE.should_stop():
                self.root.after(0, self._on_pipeline_finished)
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
            if net_root:
                try:
                    _net_use_disconnect(net_root)
                    print(f"[NAS] net use disconnect: {net_root}")
                except Exception:
                    pass

    # ──────────────────────────────────────────────────────────────────────────
    # pipelines
    # ──────────────────────────────────────────────────────────────────────────
    def _pipeline_mode_1(self, toolkit, cfg: GuiConfig) -> None:
        sd_dir = self.sd_dir
        sd_dir.mkdir(parents=True, exist_ok=True)
        toolkit.prepare_sd_card(sd_dir, interactive=False)

        # Meme langue que le GUI, transmise au DMD via config.ini -- pour
        # que le premier lancement se fasse deja dans la langue de
        # l'utilisateur (page AP + bannieres ecran), sans ressaisie manuelle.
        toolkit.write_dmd_language(sd_dir, toolkit.CURRENT_LANG)

        # Force le parcours "premier demarrage" (WiFi puis config DMD en 2
        # temps) sur la carte produite -- demande utilisateur. Voir
        # write_dmd_first_boot() : un residu "first_boot=0" (session/test
        # materiel precedent, dossier de travail non vide reutilise) ne
        # doit jamais empecher silencieusement ce parcours sur une carte
        # fraichement construite par le Mode 1.
        toolkit.write_dmd_first_boot(sd_dir)

        # Installation/mise a jour des scripts Recalbox (Mode 9) : deplacee
        # ICI, en tout premier (avant extraction/conversion/etc.), au lieu
        # d'etre enterree en fin de pipeline. Le script marquee est
        # indispensable au bon fonctionnement de l'appareil (pas juste une
        # option). La cible n'est PAS re-detectee ici : elle a deja ete
        # testee (joignabilite reelle) ET validee explicitement par
        # l'utilisateur dans _on_start_clicked() (plusieurs Recalbox peuvent
        # etre allumees sur le reseau -- ne jamais installer silencieusement
        # sur une cible auto-detectee/en cache sans confirmation).
        #
        # Les scripts sont TOUJOURS d'abord mis en scene localement (meme
        # arborescence que le partage reseau, prete a copier a la main) --
        # que la Recalbox ait ete confirmee joignable ou non -- pour que
        # l'utilisateur puisse les copier lui-meme si l'install reseau
        # automatique echoue/n'a pas ete tentee (demande explicite
        # utilisateur, suite au bug "installation silencieuse alors
        # qu'impossible car RB eteinte").
        self._mode1_scripts_staged_dir = sd_dir / "recalbox_userscripts"
        print(toolkit.tr("mode1_scripts_staged"))
        staged_ok, staged_total = toolkit.stage_recalbox_scripts_locally(
            self._mode1_scripts_staged_dir, progress_cb=self._progress_cb
        )

        target = getattr(self, "_mode1_scripts_target", None)
        self._mode1_scripts_installed_ok = False
        if target and staged_ok:
            print(toolkit.tr("mode1_scripts_phase"))
            # install_recalbox_scripts() tente SMB en priorite, puis repli
            # SSH/SFTP automatique (identifiants Recalbox par defaut) si le
            # partage SMB est injoignable -- ex: segmentation VLAN qui
            # bloque 445/139 mais autorise 22 (cas reel rencontre).
            live_ok, live_total, install_method = toolkit.install_recalbox_scripts(
                self._mode1_scripts_staged_dir, target, progress_cb=self._progress_cb
            )
            self._mode1_scripts_installed_ok = live_total > 0 and live_ok == live_total
            if install_method:
                print(toolkit.tr("mode1_scripts_installed_via")(install_method))
            if self._mode1_scripts_installed_ok:
                # IP validee par l'utilisateur (pas juste detectee),
                # ecrite dans config.ini pour que le champ "IP Recalbox" de
                # la page web config soit deja pre-rempli au premier boot,
                # meme si la Recalbox est eteinte a ce moment-la (l'auto-
                # detection mDNS du firmware necessite qu'elle soit
                # joignable). Ecrite seulement si l'install a reellement
                # reussi -- une cible confirmee mais dont l'install a
                # echoue ne garantit pas que c'est la bonne IP durable.
                target_ip = getattr(self, "_mode1_scripts_target_ip", None)
                if target_ip:
                    toolkit.write_dmd_recalbox_ip(sd_dir, target_ip)
        else:
            print(toolkit.tr("mode1_scripts_skip"))

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        systems_out = sd_dir / "systems"
        if systems_out.exists():
            shutil.rmtree(systems_out)
        systems_out.mkdir(parents=True, exist_ok=True)

        profile_name = "10.x"
        try:
            profile_name = self._mode1_profile_var.get()
        except Exception:
            pass
        profile = toolkit.RECALBOX_PROFILES.get(
            profile_name, toolkit.RECALBOX_PROFILES["10.x"]
        )
        print(f"[GUI] Version Recalbox : {profile_name} ({profile['description']})")
        tag_configs = [(profile["tag"], "")]
        selected_systems = cfg.systems_selected

        log_path = sd_dir / "images_manquantes.txt"
        with open(log_path, "w", encoding="utf-8") as log_file:
            grand, _ = toolkit.run_extraction(
                cfg.roms_root,
                systems_out,
                tag_configs,
                log_file,
                selected_systems=selected_systems,
                progress_cb=self._progress_cb,
                listen_keyboard=False,
            )
            toolkit._write_log(log_file, cfg.roms_root, grand)

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        toolkit.run_conversion(
            systems_out, progress_cb=self._progress_cb, listen_keyboard=False
        )

        # Nettoyage des .png/.gif après conversion (remplacés par raw565/raw565pack)
        removed_files = 0
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
                        pass
        if removed_files > 0:
            print(f"[GUI] Nettoyage: {removed_files} fichiers .png/.gif supprimés")

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        toolkit.build_cache(systems_out, sd_dir, progress_cb=self._progress_cb)

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        toolkit.download_defaults(
            sd_dir,
            progress_cb=self._progress_cb,
            listen_keyboard=False,
            replace_existing=True,
            download_missing=True,
            lang=getattr(self, "_systems_image_lang", None) or prefs.get("systems_image_lang") or "en",
        )
        self._apply_custom_default_fallback(sd_dir)

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        # Banque de GIFs (pack GitHub + GIFs perso via l'onglet Playlist) --
        # voir pre-vol dans _on_start_clicked. Place a cote du seul autre
        # telechargement GitHub optionnel du pipeline (meme convention :
        # best-effort, non bloquant). regenerate_playlist_gifs_cache() est
        # appele des qu'une des deux sources a ete demandee -- redondant
        # avec la regeneration deja faite via l'onglet Playlist (etape
        # differee, _on_playlist_regen_cache_done) si l'utilisateur a
        # ajoute des GIFs perso, mais sans risque (scan complet idempotent)
        # et c'est justement ce qui permet de capter les deux sources
        # ensemble si l'utilisateur a repondu Oui aux deux questions.
        if getattr(self, "_mode1_download_pack", False):
            # Le telechargement a normalement deja ete lance en
            # arriere-plan des la reponse "oui" a la question (voir
            # _start_gifpack_background_download(), appele depuis
            # _on_start_clicked()) -- on attend juste qu'il termine au
            # lieu de le relancer (evite un double telechargement
            # concurrent). Repli defensif (thread absent, ne devrait pas
            # arriver) : lance en synchrone ici, comme avant. try/except
            # explicite dans les deux cas : cette etape est documentee
            # "best-effort, non bloquante" (commentaire ci-dessus) mais ne
            # l'etait pas reellement -- une exception non rattrapee ici
            # interrompait AUSSI systems_cache.dat, le nettoyage final et
            # tout le reste du pipeline.
            bg_thread = getattr(self, "_mode1_gifpack_bg_thread", None)
            try:
                if bg_thread is not None:
                    if bg_thread.is_alive():
                        print("[GUI] Pack GIFs : attente de la fin du téléchargement démarré en arrière-plan...")
                    bg_thread.join()
                else:
                    toolkit.download_gif_pack(sd_dir, progress_cb=self._progress_cb, listen_keyboard=False)
            except Exception as e:
                print(f"[GUI] Échec téléchargement pack GIFs (non bloquant) : {e}")
        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()
        if getattr(self, "_mode1_download_pack", False) or getattr(self, "_mode1_add_custom_gifs", False):
            toolkit.regenerate_playlist_gifs_cache(sd_dir)

        # Playlist par defaut (demande utilisateur) : si le pack a ete
        # demande, telecharge la playlist "vitrine" du depot
        # (GITHUB_DEFAULT_PLAYLIST_NAME, carte SD/playlists/) et la
        # selectionne comme playlist par defaut. "ALL.txt" (tous les
        # dossiers de gifs/, marqueur "# FULL:") est TOUJOURS tentee en
        # plus (pack ou pas, tant que gifs/ contient reellement quelque
        # chose -- ne cree rien sinon) et devient la playlist par defaut
        # de repli si la playlist vitrine est absente (pack non demande,
        # ou telechargement echoue). Si gifs/ est entierement vide (aucun
        # pack, aucun GIF perso ajoute), aucune playlist n'est creee et le
        # config.ini reste sans playlist par defaut.
        default_playlist_name = ""
        if getattr(self, "_mode1_download_pack", False):
            if toolkit.download_default_gif_playlist(sd_dir):
                default_playlist_name = toolkit.GITHUB_DEFAULT_PLAYLIST_NAME
        all_playlist_name = toolkit.create_all_gifs_playlist(sd_dir)
        if not default_playlist_name:
            default_playlist_name = all_playlist_name or ""
        toolkit.write_dmd_default_playlist(sd_dir, default_playlist_name)

        sysc_out = sd_dir / "systems_cache.dat"
        toolkit.build_systems_cache(
            systems_out, sysc_out, progress_cb=self._progress_cb
        )

        # Nettoyer les dossiers logs/log inutiles avant copie SD
        for d in (self.sd_dir / "logs", self.sd_dir / "log"):
            if d.exists():
                shutil.rmtree(d)
                print(f"[GUI] Dossier {d.name} supprimé du cache de travail")

        print("[GUI] DONE mode 1")

    def _pipeline_mode_2(self, toolkit, cfg: GuiConfig) -> None:
        sd_dir = self.sd_dir
        sd_dir.mkdir(parents=True, exist_ok=True)
        toolkit.prepare_sd_card(sd_dir, interactive=False)

        # v41 : ne vide plus systems/ avant l'extraction (ni "_defaults", ni
        # le reste) -- un nettoyage prealable effacait les dossiers systemes
        # deja presents dans le dossier temporaire (ex: conversions
        # GIF/raw565pack deja faites a la main par l'utilisateur), meme
        # probleme que celui deja corrige pour "_defaults" en v26
        # (RecalBoxDMD_tool.py). run_extraction() ajoute/ecrase uniquement
        # les fichiers qu'elle produit elle-meme ; tout le reste du contenu
        # existant de systems/ est desormais preserve.
        systems_out = sd_dir / "systems"
        systems_out.mkdir(parents=True, exist_ok=True)

        tag_configs = [("logo", "")]
        selected_systems = cfg.systems_selected

        log_path = sd_dir / "images_manquantes.txt"
        with open(log_path, "w", encoding="utf-8") as log_file:
            grand, _ = toolkit.run_extraction(
                cfg.roms_root,
                systems_out,
                tag_configs,
                log_file,
                selected_systems=selected_systems,
                progress_cb=self._progress_cb,
                listen_keyboard=False,
            )
            toolkit._write_log(log_file, cfg.roms_root, grand)

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        toolkit.download_defaults(
            sd_dir,
            progress_cb=self._progress_cb,
            listen_keyboard=False,
            replace_existing=True,
            download_missing=True,
            overwrite_existing_files=getattr(self, "_mode2_overwrite_existing", True),
            lang=getattr(self, "_systems_image_lang", None) or prefs.get("systems_image_lang") or "en",
        )
        self._apply_custom_default_fallback(sd_dir)

        print("[GUI] DONE mode 2 (extract + download _defaults)")

    def _pipeline_mode_11(self, toolkit, cfg: GuiConfig) -> None:
        # Pack GitHub (~600 GIFs) -- unifie sur le meme fonctionnement que
        # Mode 2 (2026-08-11, demande utilisateur) : passe desormais par
        # btn_start_adv/_worker_main comme tous les autres modes, au lieu
        # d'un bouton/thread dedies. Reutilise integralement
        # toolkit.download_gif_pack() (deja utilise par le pack GIFs de
        # Mode 1).
        sd_dir = self.sd_dir
        sd_dir.mkdir(parents=True, exist_ok=True)
        toolkit.download_gif_pack(sd_dir, progress_cb=self._progress_cb, listen_keyboard=False)
        print("[GUI] DONE mode 11 (download pack GIFs GitHub)")

    def _pipeline_mode_3(self, toolkit, cfg: GuiConfig) -> None:
        sd_dir = self.sd_dir
        sd_dir.mkdir(parents=True, exist_ok=True)
        toolkit.prepare_sd_card(sd_dir, interactive=False)

        systems_out = sd_dir / "systems"
        if systems_out.exists():
            shutil.rmtree(systems_out)
        systems_out.mkdir(parents=True, exist_ok=True)

        profile_name = "10.x"
        try:
            profile_name = self._mode1_profile_var.get()
        except Exception:
            pass
        profile = toolkit.RECALBOX_PROFILES.get(
            profile_name, toolkit.RECALBOX_PROFILES["10.x"]
        )
        print(f"[GUI] Version Recalbox : {profile_name} ({profile['description']})")
        tag_configs = [(profile["tag"], "")]
        selected_systems = cfg.systems_selected

        log_path = sd_dir / "images_manquantes.txt"
        with open(log_path, "w", encoding="utf-8") as log_file:
            grand, _ = toolkit.run_extraction(
                cfg.roms_root,
                systems_out,
                tag_configs,
                log_file,
                selected_systems=selected_systems,
                progress_cb=self._progress_cb,
                listen_keyboard=False,
            )
            toolkit._write_log(log_file, cfg.roms_root, grand)

        print("[GUI] DONE mode 3 (extract only)")

    def _pipeline_mode_4(self, toolkit, cfg: GuiConfig) -> None:
        """
        Mode 4 (raw-only) :
          - source = le dossier libre choisi par l'utilisateur (cfg.roms_root)
          - sortie  = self.sd_dir/systems
        On respecte la sélection GUI (cfg.systems_selected) en copiant uniquement
        les systèmes sélectionnés, puis on lance toolkit.run_conversion_raw_only().
        """
        systems_out_root = (
            self._final_output_dir
            if self._final_output_dir is not None
            else self.sd_dir
        )
        systems_out = systems_out_root / "systems"
        systems_out.mkdir(parents=True, exist_ok=True)

        selected_systems = cfg.systems_selected
        if not selected_systems:
            # Sécurité : si rien n'est sélectionné, on ne fait rien.
            print("[GUI] Mode 4: Aucun système sélectionné.")
            return

        # Nettoyage sortie uniquement (pas de staging du contenu png/gif).
        # On supprime les systèmes sélectionnés dans le dossier de sortie,
        # puis le toolkit lit directement cfg.roms_root et écrit raw565/raw565pack/meta dans systems_out.
        system_names = [p.name for p in selected_systems if p is not None]

        for src_system_dir in selected_systems:
            dst_system_dir = systems_out / src_system_dir.name
            shutil.rmtree(dst_system_dir, ignore_errors=True)
            dst_system_dir.mkdir(parents=True, exist_ok=True)

        toolkit.run_conversion_raw_only(
            cfg.roms_root,
            output_dir=systems_out,
            system_names=system_names,
            progress_cb=self._progress_cb,
            listen_keyboard=False,
        )

        print("[GUI] DONE mode 4 (conversion raw-only)")

    def _pipeline_mode_5(self, toolkit, cfg: GuiConfig) -> None:
        """
        Mode 5 (128x32) doit convertir à partir de :
          entrée  = cfg.roms_root/images
          sortie  = self.sd_dir/systems

        et respecter la sélection GUI (cfg.systems_selected) en copiant UNIQUEMENT
        les systèmes sélectionnés (sans déplacer la source).
        """
        systems_out_root = (
            self._final_output_dir
            if self._final_output_dir is not None
            else self.sd_dir
        )
        systems_out = systems_out_root / "systems"
        systems_out.mkdir(parents=True, exist_ok=True)

        selected_systems = cfg.systems_selected
        if not selected_systems:
            print("[GUI] Mode 5: Aucun système sélectionné.")
            return

        system_names = [p.name for p in selected_systems if p is not None]

        # Nettoyage uniquement dans le dossier de sortie (pas de staging png/gif)
        for system_dir in selected_systems:
            dst_system_dir = systems_out / system_dir.name
            shutil.rmtree(dst_system_dir, ignore_errors=True)
            dst_system_dir.mkdir(parents=True, exist_ok=True)

        toolkit.run_conversion(
            cfg.roms_root,
            progress_cb=self._progress_cb,
            listen_keyboard=False,
            output_dir=systems_out,
            system_names=system_names,
        )

        print("[GUI] DONE mode 5 (conversion 128x32)")

    def _pipeline_mode_6(self, toolkit, cfg: GuiConfig) -> None:
        systems_out = self.sd_dir / "systems"
        if not systems_out.exists():
            print("[GUI] Mode 6: dossier systems/ introuvable.")
            return

        toolkit.build_cache(systems_out, self.sd_dir, progress_cb=self._progress_cb)
        print("[GUI] DONE mode 6 (build games_cache only)")

    def _pipeline_mode_7(self, toolkit, cfg: GuiConfig) -> None:
        # Entrée : dossier "systems" contenant "_defaults"
        systems_in = cfg.roms_root

        # Sortie : emplacement de systems_cache.dat
        systems_out_root = (
            self._final_output_dir
            if self._final_output_dir is not None
            else self.sd_dir
        )
        sysc_out = systems_out_root / "systems_cache.dat"

        if not systems_in.exists():
            print("[GUI] Mode 7: dossier systems/ d’entrée introuvable.")
            return

        toolkit.build_systems_cache(systems_in, sysc_out, progress_cb=self._progress_cb)
        print("[GUI] DONE mode 7 (build systems_cache.dat only)")

    # ──────────────────────────────────────────────────────────────────────────
    # Fin de traitement pipeline modes 1 / modes 7 : popup 3 choix
    # ──────────────────────────────────────────────────────────────────────────
    def _show_mode_done_popup(self, dst_drive: str = "") -> None:
        """Popup 3 choix après mode 1 ou mode 7 : Explorer SD / Explorer temp / Fermer"""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        dlg = tk.Toplevel(self.root)
        dlg.title(ui["mode6_done"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg=bg)

        lbl = tk.Label(
            dlg,
            text=f"{ui['processing_done_title']}\n\n{ui['what_to_do_next']}",
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 11),
            padx=20,
            pady=12,
        )
        lbl.pack(fill="both", expand=True)

        btn_frame = tk.Frame(dlg, bg=bg, padx=14, pady=10)
        btn_frame.pack(fill="x")

        def open_sd():
            dlg.destroy()
            if dst_drive:
                try:
                    os.startfile(dst_drive)
                except Exception:
                    pass

        def open_temp():
            dlg.destroy()
            try:
                os.startfile(str(self.sd_dir))
            except Exception:
                pass

        def close_dlg():
            dlg.destroy()

        if dst_drive:
            btn_sd = tk.Button(
                btn_frame,
                text=ui["explore_sd_btn"],
                command=open_sd,
                bg=bg_action,
                fg="#000000",
                bd=2,
                relief="solid",
                padx=10,
                pady=6,
                font=("TkDefaultFont", 10, "bold"),
            )
            btn_sd.pack(fill="x", pady=2)

        btn_temp = tk.Button(
            btn_frame,
            text=ui["explore_temp_btn"],
            command=open_temp,
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        btn_temp.pack(fill="x", pady=2)

        btn_close = tk.Button(
            btn_frame,
            text=ui["btn_close"],
            command=close_dlg,
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        btn_close.pack(fill="x", pady=2)

    # ──────────────────────────────────────────────────────────────────────────
    # mode 7 + fin de traitement + actions finaux
    # ──────────────────────────────────────────────────────────────────────────
    def _get_ui_t(self) -> dict[str, str]:
        lang = self.lang_var.get()
        return UI_TRANSLATIONS.get(lang, UI_TRANSLATIONS["fr"])

    def _start_worker(self, target, args: tuple = (), kwargs: Optional[dict] = None) -> threading.Thread:
        """Lance `target` dans un thread daemon et le suit dans
        self._active_workers, pour que _is_processing() (utilisee pour
        bloquer le changement d'onglet et securiser la fermeture) sache
        qu'un traitement tourne, quels que soient le mode/bouton qui l'a
        declenche. A utiliser pour toute operation d'arriere-plan avec
        sortie logs, plutot qu'un threading.Thread(...) local non suivi.

        Au tout premier demarrage (transition idle -> occupe), memorise
        l'onglet actif et bascule sur Logs ; un sondage restaurera
        l'onglet d'origine des que plus aucun traitement ne tournera (voir
        _poll_processing_done())."""
        was_idle = not self._is_processing()

        def _run() -> None:
            try:
                target(*args, **(kwargs or {}))
            finally:
                self._active_workers.discard(t)

        t = threading.Thread(target=_run, daemon=True)
        self._active_workers.add(t)
        t.start()

        if was_idle:
            try:
                sel = self.nb_top.select()
                self._origin_tab_idx = self.nb_top.index(sel) if sel else None
            except Exception:
                self._origin_tab_idx = None
            self.nb_top.select(self._LOGS_TAB_INDEX)
            self.root.after(300, self._poll_processing_done)

        return t

    def _poll_processing_done(self) -> None:
        """Tant qu'un traitement tourne, se rappelle toutes les 300ms.
        Une fois termine, revient sur l'onglet actif au moment du
        demarrage (_on_tab_changed restaure lui-meme le mode approprie
        pour cet onglet, via _last_adv_mode ou le forcage a "1" sur
        Main)."""
        if self._is_processing():
            self.root.after(300, self._poll_processing_done)
            return

        origin_tab = self._origin_tab_idx
        self._origin_tab_idx = None
        if origin_tab is not None and origin_tab != self._LOGS_TAB_INDEX:
            try:
                self.nb_top.select(origin_tab)
            except Exception:
                pass

    def _is_processing(self) -> bool:
        """Vrai si le pipeline principal, le flash SD (Mode 6), ou tout
        autre traitement lance via _start_worker() (comparaison finale
        Mode 8, retry flash, nettoyage avant scrape Mode 1...) est encore
        en cours."""
        if self._worker and self._worker.is_alive():
            return True
        if self._mode6_flash_thread and self._mode6_flash_thread.is_alive():
            return True
        return any(t.is_alive() for t in list(self._active_workers))

    def _cleanup_sd_dir(self) -> None:
        if not self.sd_dir.exists():
            return

        ui = self._get_ui_t()
        # Feedback visuel avant l'operation bloquante : sans lui, une
        # suppression un peu longue (dossier avec des milliers de petits
        # fichiers image) donne l'impression que l'appli est gelee, ce qui
        # pousse certains utilisateurs a la tuer via le gestionnaire de
        # taches -- interrompant le nettoyage en cours et laissant le
        # dossier temporaire partiellement supprime.
        dlg = None
        try:
            c = self._theme_colors()
            bg = c.get("bg_main", "#F3F3F3")
            fg = c.get("fg_text", "#000000")
            dlg = tk.Toplevel(self.root)
            dlg.title(ui["quit_app_warning_title"])
            dlg.resizable(False, False)
            dlg.transient(self.root)
            dlg.configure(bg=bg)
            tk.Label(
                dlg,
                text=ui["cleanup_in_progress"],
                bg=bg,
                fg=fg,
                justify="left",
                padx=20,
                pady=16,
            ).pack()
            self._center_toplevel(dlg)
            self.root.update_idletasks()
            dlg.update()  # forcer l'affichage avant l'appel bloquant ci-dessous
        except Exception:
            dlg = None

        self._rmtree_with_retry(self.sd_dir)

        if dlg is not None:
            try:
                dlg.destroy()
            except Exception:
                pass

    def _rmtree_with_retry(self, path: Path, attempts: int = 4, delay: float = 0.4) -> bool:
        """shutil.rmtree avec quelques tentatives : sous Windows, un
        fichier tout juste referme (log, image en cours d'ecriture,
        antivirus, explorateur qui a un handle dessus) peut rester
        verrouille une fraction de seconde apres la fin du worker -- une
        seule tentative immediate est la cause la plus probable d'un
        nettoyage silencieusement incomplet."""

        def _on_rm_error(func, p, exc_info):
            # Cas frequent sous Windows : fichier copie en lecture-seule.
            # Retirer l'attribut puis reessayer avant d'abandonner ce fichier.
            try:
                os.chmod(p, stat.S_IWRITE)
                func(p)
            except Exception:
                pass

        last_error: Optional[Exception] = None
        for attempt in range(attempts):
            try:
                if not path.exists():
                    return True
                shutil.rmtree(path, onerror=_on_rm_error)
                return not path.exists()
            except Exception as e:
                last_error = e
                if attempt < attempts - 1:
                    time.sleep(delay)
        return last_error is None

    def _refresh_mode6_drives(self) -> None:
        try:
            drives = self.tkmod._list_removable_drives()  # type: ignore[attr-defined]
        except Exception:
            drives = []
        self._mode6_drives = list(drives)
        # Pre-selectionne le lecteur deja verifie FAT32/taille en debut de
        # Mode 1 (self._mode1_verified_sd_letter, voir _prompt_sd_card_
        # dialog) au lieu du 1er lecteur trouve -- evite a l'utilisateur de
        # re-choisir une carte deja validee. Meme principe que la
        # pre-selection deja utilisee par _refresh_playlist_drives (pref
        # "playlist_last_drive"). Retombe sur l'index 0 si absent/introuvable
        # (comportement inchange pour tout lancement hors Mode 1).
        sel_idx = 0
        verified_letter = getattr(self, "_mode1_verified_sd_letter", None)
        if verified_letter:
            for i, (letter, _label, _size) in enumerate(self._mode6_drives):
                if letter == verified_letter:
                    sel_idx = i
                    break
        for inst in self._mode6_instances():
            dl = inst["drive_list"]
            dl.delete(0, "end")
            for i, (letter, label, size) in enumerate(self._mode6_drives):
                dl.insert("end", f"{i+1} → {letter}\\\\  [{label}]  {size}")
            if self._mode6_drives:
                dl.selection_set(sel_idx)

    def _sync_mode6_texts(self) -> None:
        ui = self._get_ui_t()
        for inst in self._mode6_instances():
            if inst["panel_title_lbl"]:
                inst["panel_title_lbl"].config(text=ui["mode6_panel_title"])
            if inst["btn"]:
                inst["btn"].config(text=ui["mode6_btn_start"])

    def _get_theme_list(self) -> list[str]:
        """Retourne la liste des thèmes avec 'Aléatoire' en tête localisé."""
        random_label = "Aléatoire"  # fallback
        lang = self.lang_var.get()
        if lang == "en":
            random_label = "Random"
        elif lang == "es":
            random_label = "Aleatorio"
        return [random_label] + themes.list_themes()

    def _on_theme_selected(self, event=None) -> None:
        choice = self._theme_var.get()
        lang = self.lang_var.get()
        if lang == "en":
            random_lbl = "Random"
        elif lang == "es":
            random_lbl = "Aleatorio"
        else:
            random_lbl = "Aléatoire"
        if choice == random_lbl:
            # Mode aleatoire : sauvegarder "random" en pref
            themes.save_preference("random")
            themes.apply(themes.random_theme(), self)
        else:
            themes.save_preference(choice)
            themes.apply(choice, self)
        # Le cadre de selection systeme peut etre en fond opaque (deja
        # peuple) avec la couleur bg_listbox de l'ANCIEN theme : la
        # remettre a jour avec les couleurs du nouveau theme. Doit
        # s'executer APRES le decoupage global differe de themes.apply()
        # (root.after(400, ...)), sinon celui-ci l'ecraserait -- meme
        # delai que _reslice_after_mode_change, avec une marge.
        self.root.after(450, self._update_sys_box_decor)
        # Idem pour les cadres Dossiers/Fichiers de l'onglet Playlist (hors
        # du systeme de theming generique -- Canvas ignores, Frame en
        # bg="white" traite comme une couleur fixe volontaire).
        self.root.after(450, self._playlist_apply_theme_colors)

    def _start_mode6_blinking(self) -> None:
        instances = self._mode6_instances()
        if not instances:
            return
        # Retire la section "Marche a suivre" du descriptif Mode 1 une fois
        # la copie SD atteinte (bug d'interface decalee vers le bas -- les
        # boutons du cadre progress disparaissaient de la vue, voir
        # _update_mode_desc).
        self._mode1_sd_copy_active = True
        self._update_mode_desc()
        newly_shown = False
        for inst in instances:
            if inst["frame"] and not inst["frame"].winfo_ismapped():
                inst["frame"].pack(side="bottom", fill="x", pady=(6, 0))
                newly_shown = True
        if newly_shown:
            # Sans cela, le decoupage d'image de fond ne couvre pas la zone
            # de ce panneau : il reste invisible derriere le fond tant
            # qu'un changement d'onglet ne redeclenche pas _on_tab_changed()
            # -> _slice_widgets_later(). Necessaire ici car
            # _on_pipeline_finished() ne force plus de bascule d'onglet
            # (v25+ : l'onglet d'origine est deja restaure par
            # _poll_processing_done()). On utilise slice_single_frame()
            # (meme solution que le panneau Mode 8, cf. changelog v11 --
            # "devient visible sans changement d'onglet") plutot que le
            # decoupage global _slice_widgets_later()/_reslice_after_mode_change :
            # ce dernier reparcourt TOUT l'arbre (y compris les cadres de
            # l'onglet Avance non mappes) et corrompt le decoupage
            # existant des cadres deja stables (teste : boutons dupliques).
            self.root.update_idletasks()
            for inst in instances:
                if inst["frame"]:
                    try:
                        themes.slice_single_frame(self, inst["frame"])
                    except Exception:
                        pass
        self._mode6_blinking = True

        self._refresh_mode6_drives()
        ui = self._get_ui_t()
        for inst in instances:
            if inst["btn"]:
                inst["btn"].config(state="normal", text=ui["mode6_btn_start"])

        def _tick() -> None:
            if not self._mode6_blinking:
                return
            # toggle bg for blink (les 2 instances basculent ensemble, un
            # seul timer partage -- pas de derive entre Main et Avance)
            for inst in self._mode6_instances():
                btn = inst["btn"]
                if not btn:
                    continue
                current = btn.cget("bg")
                new_bg = "#FFFFFF" if current != "#FFFFFF" else "#FFD400"
                btn.config(bg=new_bg)
            self._mode6_blink_job = self.root.after(400, _tick)

        self._mode6_blink_job = self.root.after(250, _tick)

    def _stop_mode6_blinking(self) -> None:
        self._mode6_blinking = False
        if self._mode6_blink_job is not None:
            try:
                self.root.after_cancel(self._mode6_blink_job)
            except Exception:
                pass
        self._mode6_blink_job = None
        for inst in self._mode6_instances():
            if inst["btn"]:
                inst["btn"].config(bg="#FFD400")

    # ──────────────────────────────────────────────────────────────────────────
    # Mode 1 / Mode 3 : version Recalbox (aide au scrape + nettoyage avant scrape)
    # ──────────────────────────────────────────────────────────────────────────
    def _build_recalbox_profile_panel(self, parent: tk.Frame) -> tk.Frame:
        """
        Construit le panneau "Version Recalbox" (combobox partagee
        self._mode1_profile_var + bouton aide scrape + bouton nettoyage).
        Reutilise a l'identique par le Mode 1 (onglet Main, toujours
        visible) et le Mode 3 (onglet Avance, visible seulement en mode 3)
        -- meme profil, meme logique d'extraction/nettoyage cote toolkit.
        """
        ui = self._get_ui_t()
        frame = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8)

        title_lbl = tk.Label(
            frame,
            text=ui["mode1_profile_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        title_lbl.pack(anchor="w")

        combo = ttk.Combobox(
            frame,
            textvariable=self._mode1_profile_var,
            values=list(self.tkmod.RECALBOX_PROFILES.keys()),
            state="readonly",
            width=10,
        )
        combo.pack(anchor="w", pady=(4, 6))
        combo.bind("<<ComboboxSelected>>", self._on_mode1_profile_selected)

        help_btn = tk.Button(
            frame,
            text=ui["mode1_scrape_help_btn"],
            command=self._on_mode1_scrape_help_clicked,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=8,
            pady=4,
            font=("TkDefaultFont", 9, "bold"),
        )
        help_btn.pack(fill="x", pady=(0, 6))

        clean_btn = tk.Button(
            frame,
            text=ui["mode1_clean_btn"],
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=8,
            pady=4,
            font=("TkDefaultFont", 9, "bold"),
        )
        clean_btn.config(command=lambda b=clean_btn: self._on_mode1_clean_clicked(b))
        clean_btn.pack(fill="x")

        frame.title_lbl = title_lbl
        frame.combo = combo
        frame.help_btn = help_btn
        frame.clean_btn = clean_btn
        return frame

    def _build_default_image_button(self, parent: tk.Frame) -> tk.Button:
        """
        Bouton partage (Main + onglet Avance/Mode 2) qui ouvre la popup de
        choix de l'image de secours (default.raw565). Meme reutilisation
        que _build_recalbox_profile_panel, en plus simple (un seul bouton).
        """
        ui = self._get_ui_t()
        btn = tk.Button(
            parent,
            text=ui["default_image_btn"],
            command=self._on_default_image_picker_clicked,
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=8,
            pady=4,
            font=("TkDefaultFont", 9, "bold"),
        )
        return btn

    def _build_mode6_panel(self, parent: tk.Frame) -> tk.Frame:
        """
        Construit le panneau "Copier sur la carte SD" (liste des lecteurs +
        bouton clignotant + bouton Explorer). Reutilise a l'identique par
        l'onglet Main et l'onglet Avance : meme etat partage
        (self._mode6_drives, self._mode6_selected_drive,
        self._mode6_blinking...), widgets propres a chaque instance -- voir
        _mode6_instances().
        """
        ui = self._get_ui_t()
        frame = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=5)

        panel_title_lbl = tk.Label(
            frame,
            text=ui["mode6_panel_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        )
        panel_title_lbl.pack(anchor="w")

        drives_title_lbl = tk.Label(
            frame,
            text=ui["mode6_drives_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        drives_title_lbl.pack(anchor="w", pady=(4, 2))

        drives_box = tk.Frame(frame, bg="#F3F3F3")
        drives_box.pack(fill="x")

        # height=3 (au lieu de 5) : sur l'onglet Main, ce panneau s'ajoute a
        # la hauteur deja augmentee par les panneaux "Version Recalbox" +
        # "Image de secours" (deplaces en colonne gauche) -- sans cette
        # reduction, le contenu total depassait les 750px fixes de la
        # fenetre et repoussait le cadre Progression hors ecran (mesure :
        # ~56px de depassement). Le defilement (scrollbar) reste disponible
        # au-dela de 3 lecteurs.
        drive_list = tk.Listbox(
            drives_box,
            selectmode="browse",
            height=3,
            width=26,
            bg="white",
            fg="black",
            borderwidth=3,
            relief="solid",
            # exportselection=False : ce panneau existe en double (Main +
            # Avance, meme constructeur appele deux fois). Par defaut
            # (exportselection=True), Tkinter n'autorise qu'UNE selection
            # active a la fois entre TOUS les Listbox de l'appli -- appeler
            # selection_set() sur l'instance Avance efface silencieusement
            # celle deja posee sur l'instance Main, rendant la
            # pre-selection du lecteur verifie (Mode 1) invisible sur
            # l'onglet vers lequel l'utilisateur revient reellement.
            exportselection=False,
        )
        drive_list.pack(side="left", fill="both", expand=True)

        scroll = tk.Scrollbar(drives_box, orient="vertical", command=drive_list.yview)
        scroll.pack(side="right", fill="y")
        drive_list.configure(yscrollcommand=scroll.set)

        btn = tk.Button(
            frame,
            text=ui["mode6_btn_start"],
            command=lambda: self._on_mode6_button_clicked(drive_list),
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=4,
            font=("TkDefaultFont", 11, "bold"),
            state="disabled",
        )
        btn.pack(fill="x", pady=(6, 0))

        # Bouton "Explorer le dossier de sortie" (active une fois la copie terminee)
        explore_btn = tk.Button(
            frame,
            text=ui["mode6_explore_output_btn"],
            command=lambda: os.startfile(  # type: ignore[attr-defined]
                str(self._final_output_dir if self._final_output_dir else self.sd_dir)
            ),
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
            state="disabled",
        )
        explore_btn.pack(fill="x", pady=(4, 4))

        frame.panel_title_lbl = panel_title_lbl
        frame.drives_title_lbl = drives_title_lbl
        frame.drive_list = drive_list
        frame.btn = btn
        frame.explore_btn = explore_btn
        return frame

    def _mode6_instances(self) -> list[dict]:
        """
        Chaque onglet (Main, Avance) affiche sa propre copie du panneau
        "copie SD", mais partage le meme etat
        (self._mode6_drives/_mode6_selected_drive/_mode6_blinking...).
        Retourne la liste des instances de widgets actuellement construites
        (1 ou 2 selon l'avancement de la construction du GUI).
        """
        instances = []
        if getattr(self, "_mode6_ui_frame", None) is not None:
            instances.append(
                {
                    "frame": self._mode6_ui_frame,
                    "btn": self._mode6_btn,
                    "drive_list": self._mode6_drive_list,
                    "explore_btn": self._mode6_explore_output_btn,
                    "panel_title_lbl": self._mode6_panel_title_lbl,
                    "drives_title_lbl": self._mode6_drives_title_lbl,
                }
            )
        if getattr(self, "_mode6_ui_frame_adv", None) is not None:
            instances.append(
                {
                    "frame": self._mode6_ui_frame_adv,
                    "btn": self._mode6_btn_adv,
                    "drive_list": self._mode6_drive_list_adv,
                    "explore_btn": self._mode6_explore_output_btn_adv,
                    "panel_title_lbl": self._mode6_panel_title_lbl_adv,
                    "drives_title_lbl": self._mode6_drives_title_lbl_adv,
                }
            )
        return instances

    # ──────────────────────────────────────────────────────────────────────────
    # Mode 9 : installer/mettre a jour les scripts Recalbox (partage SMB)
    # ──────────────────────────────────────────────────────────────────────────
    def _start_mode9_autodetect(self) -> None:
        def _worker() -> None:
            try:
                host = self.tkmod.detect_recalbox_share()
            except Exception:
                host = None
            if host:
                self.root.after(0, self._on_mode9_autodetect_result, host)

        threading.Thread(target=_worker, daemon=True).start()

    def _on_mode9_autodetect_result(self, host: str) -> None:
        if not getattr(self, "mode9_host_var", None):
            return
        # Ne pas ecraser une valeur deja presente (saisie manuelle ou pref
        # deja chargee) -- la detection auto ne fait que proposer un
        # premier remplissage quand le champ est vide.
        if not self.mode9_host_var.get().strip():
            self.mode9_host_var.set(host)
            prefs.set("recalbox_ip", host)
        if getattr(self, "mode9_result_var", None) is not None:
            self.mode9_result_var.set(self.tkmod.tr("mode9_autodetect_ok")(host))
            self._reslice_mode9_frame()

    def _on_mode9_install_clicked(self) -> None:
        if self._mode9_thread and self._mode9_thread.is_alive():
            return
        ui = self._get_ui_t()
        host = self.mode9_host_var.get().strip()
        if not host:
            self.mode9_result_var.set(self.tkmod.tr("mode9_share_unreachable")(""))
            try:
                self._mode9_host_entry.focus_set()
            except Exception:
                pass
            return
        prefs.set("recalbox_ip", host)
        self._mode9_install_btn.config(state="disabled", text=ui["mode9_btn_running"])
        self.mode9_result_var.set("")
        self._mode9_thread = threading.Thread(
            target=self._mode9_install_worker, args=(host,), daemon=True
        )
        self._mode9_thread.start()

    def _mode9_install_worker(self, host: str) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        error: Optional[str] = None
        ok, total = 0, 0
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]
            ok, total = self.tkmod.download_recalbox_scripts(
                host, progress_cb=self._progress_cb, listen_keyboard=False
            )
            # Ecriture de recalbox_ip dans self.sd_dir tentee puis retiree
            # (2026-08-05) : le Mode 9 n'a pas besoin (ni ne suppose) que la
            # vraie carte SD soit inseree dans le PC -- un config.ini ecrit
            # dans le dossier de travail local ici serait sans rapport avec
            # la carte reelle de l'utilisateur, et risquerait d'ecraser
            # silencieusement son config.ini au prochain Mode 1/6 (copie SD)
            # avec des valeurs perimees. Contrairement a Mode 1, qui
            # construit une carte complete dans ce meme dossier de travail
            # avant de la copier, le Mode 9 est une action ponctuelle
            # (scripts seuls) sans lien garanti avec ce dossier.
        except Exception as e:
            print(f"❌ [GUI] Erreur installation scripts Recalbox : {e}")
            error = str(e)
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
        self.root.after(0, self._on_mode9_install_done, ok, total, error)

    def _on_mode9_install_done(
        self, ok: int, total: int, error: Optional[str]
    ) -> None:
        ui = self._get_ui_t()
        if getattr(self, "_mode9_install_btn", None):
            self._mode9_install_btn.config(state="normal", text=ui["mode9_btn_install"])
        if error:
            self.mode9_result_var.set(f"❌ {error}")
        elif total == 0:
            host = self.mode9_host_var.get().strip()
            self.mode9_result_var.set(self.tkmod.tr("mode9_share_unreachable")(host))
        else:
            self.mode9_result_var.set(ui["mode9_summary"](ok, total))
        self._reslice_mode9_frame()

    def _reslice_mode9_frame(self) -> None:
        # Le label de resultat change de hauteur (vide <-> 1-2 lignes), ce
        # qui perime le decoupage de fond du panneau (meme cause que
        # _start_mode6_blinking -- "bandes blanches" -- voir son commentaire
        # pour le detail). slice_single_frame() ne recadre que ce panneau,
        # sans reparcourir tout l'onglet (evite de perturber les autres
        # cadres deja stables).
        if not getattr(self, "_mode9_frame", None):
            return
        self.root.update_idletasks()
        try:
            themes.slice_single_frame(self, self._mode9_frame)
        except Exception:
            pass

    def _on_mode1_profile_selected(self, event=None) -> None:
        prefs.set("recalbox_profile", self._mode1_profile_var.get())

    def _on_slow_threshold_changed(self, event=None) -> None:
        # Repli sur la derniere valeur valide si l'utilisateur a tape
        # quelque chose de non numerique dans le Spinbox (get() sur un
        # IntVar leve tk.TclError dans ce cas, plutot que de planter la GUI).
        try:
            value = self._slow_threshold_var.get()
        except tk.TclError:
            return
        prefs.set("slow_threshold", str(value))

    def _mode1_scrape_help_image_path(self, profile_name: str) -> Optional[Path]:
        # Captures reelles fournies par l'utilisateur (menu SCRAPEUR de son
        # propre Recalbox), annotees : encadre rouge = champ recommande,
        # encadre en pointilles = champ a ne PAS utiliser pour la
        # marquee/logo. Le menu Recalbox lui-meme reste toujours affiche en
        # francais sur la capture (aucune capture EN/ES equivalente
        # trouvable en ligne) -- un bandeau traduit (genere localement,
        # fichiers *_en.png/*_es.png) est colle au-dessus pour les
        # interfaces EN/ES, pointant vers les memes encadres.
        mapping = {
            # 10.x (alpha) : nouveau champ dedie "SELECT LOGO TYPE"
            "10.x": "v10_logo.png",
            # 9.x (et versions sans champ logo dedie) : "SELECTIONNEZ LE
            # TYPE DE VIGNETTE" = MARQUEE
            "9.x": "v9_marquee.png",
            # legacy : "SELECTIONNEZ LE TYPE D'IMAGE" = LOGO DETOURE/MARQUEE
            "legacy": "v9_image_type.png",
        }
        fname = mapping.get(profile_name)
        if not fname:
            return None
        assets_dir = Path(__file__).resolve().parent / "assets" / "scrape_help"
        lang = self.lang_var.get()
        if lang in ("en", "es"):
            stem, ext = fname.rsplit(".", 1)
            localized = assets_dir / f"{stem}_{lang}.{ext}"
            if localized.exists():
                return localized
        return assets_dir / fname

    def _on_mode1_scrape_help_clicked(self) -> None:
        ui = self._get_ui_t()
        profile_name = self._mode1_profile_var.get()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#00D084")

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["mode1_scrape_help_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)

        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)

        tk.Label(
            body,
            text=f"{ui['mode1_scrape_help_title']}  —  Recalbox {profile_name}",
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 12, "bold"),
        ).pack(anchor="w", pady=(0, 8))

        help_text = ui.get(f"mode1_scrape_help_{profile_name}", "")
        tk.Label(
            body,
            text=help_text,
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 10),
            wraplength=480,
            justify="left",
        ).pack(anchor="w", pady=(0, 12))

        img_path = self._mode1_scrape_help_image_path(profile_name)
        if img_path and img_path.exists():
            try:
                from PIL import Image, ImageTk

                img = Image.open(img_path)
                max_w = 560
                if img.width > max_w:
                    ratio = max_w / img.width
                    img = img.resize((max_w, int(img.height * ratio)))
                photo = ImageTk.PhotoImage(img)
                lbl_img = tk.Label(body, image=photo, bg=bg, bd=2, relief="solid")
                lbl_img.image = photo  # garder une reference (evite le garbage collect)
                lbl_img.pack(pady=(0, 12))
            except Exception:
                pass

        tk.Button(
            body,
            text="OK",
            command=dlg.destroy,
            bg=bg_action,
            fg="#000000",
            bd=2,
            relief="solid",
            padx=16,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        ).pack()

        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()

    def _apply_custom_default_fallback(self, sd_dir: Path) -> None:
        """
        Reapplique le choix persistant de l'utilisateur pour default.raw565
        apres un toolkit.download_defaults(...). Le depot GitHub ne fournit
        plus ce fichier (default.raw565 retire pour eviter les ecrasements) :
        si aucun choix personnalise n'est enregistre (prefs vide = "visuel
        par defaut du projet"), le visuel reserve bundle avec l'outil
        (tools/assets/default_images/<PROJECT_DEFAULT_IMAGE_FILENAME>) sert
        de dernier recours, pour garantir que
        systems/_defaults/default.raw565 existe toujours a l'issue du
        pipeline (Mode 1 ou Mode 2).
        """
        custom = prefs.get("default_fallback_image")
        if custom:
            src = Path(custom)
            if not src.exists():
                print(f"[GUI] Image de secours personnalisee introuvable : {src}")
                return
        else:
            src = (
                Path(__file__).resolve().parent
                / "assets" / "default_images" / self.PROJECT_DEFAULT_IMAGE_FILENAME
            )
            if not src.exists():
                print(f"[GUI] Visuel par defaut du projet introuvable : {src}")
                return
        try:
            self.tkmod.set_default_fallback_image(src, sd_dir)
            print(f"[GUI] Image de secours appliquee : {src.name}")
        except Exception as e:
            print(f"[GUI] Echec application image de secours : {e}")

    def _on_default_image_picker_clicked(self, reset_on_close: bool = False) -> None:
        """reset_on_close : si True, fermer le dialogue SANS choisir
        explicitement une tuile (bouton Fermer ou X de la fenetre) applique
        quand meme le "visuel par defaut du projet" (equivalent a cliquer la
        tuile de reset) au lieu de ne rien faire. Utilise par le Mode 2
        (galerie proposee systematiquement a chaque lancement -- fermer sans
        choisir doit alors retomber sur un etat connu/par defaut plutot que
        de laisser silencieusement l'ancien choix personnalise en place).
        False (comportement d'origine, inchange) pour l'usage bouton normal
        (onglet Avance, Mode 10) : fermer sans choisir ne change rien."""
        ui = self._get_ui_t()
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["default_image_dialog_title"])
        dlg.configure(bg=bg)
        dlg.transient(self.root)
        dlg.resizable(False, False)

        body = tk.Frame(dlg, bg=bg, padx=16, pady=16)
        body.pack(fill="both", expand=True)

        tk.Label(
            body,
            text=ui["default_image_dialog_title"],
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 12, "bold"),
        ).pack(anchor="w", pady=(0, 6))

        tk.Label(
            body,
            text=ui["default_image_dialog_intro"],
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 9),
            wraplength=480,
            justify="left",
        ).pack(anchor="w", pady=(0, 12))

        gallery = tk.Frame(body, bg=bg)
        gallery.pack(fill="both")

        # Garder une reference sur chaque PhotoImage (sinon garbage collect)
        self._default_image_photo_refs = []

        def _apply_choice(src_path, display_name: str, is_reset: bool = False) -> None:
            if is_reset:
                prefs.set("default_fallback_image", "")
                # Le "visuel par defaut du projet" n'est plus fourni par
                # GitHub (default.raw565 retire du depot pour eviter les
                # ecrasements) : l'image reservee PROJECT_DEFAULT_IMAGE_FILENAME
                # (default_RB.png, deja bundlee avec l'outil) en fait
                # desormais office directement.
                effective_src = assets_dir / PROJECT_DEFAULT_IMAGE_FILENAME
            else:
                prefs.set("default_fallback_image", str(src_path))
                effective_src = Path(src_path)

            # Applique immediatement dans le dossier de travail (temporaire),
            # sans attendre un Mode 1/Mode 2 : set_default_fallback_image()
            # cree systems/_defaults/ au besoin (mkdir parents=True).
            applied_now = False
            try:
                self.tkmod.set_default_fallback_image(effective_src, self.sd_dir)
                applied_now = True
            except Exception as e:
                print(f"[GUI] Echec application immediate image de secours : {e}")

            dlg.destroy()
            if not applied_now:
                # Garde uniquement la popup d'ERREUR (information utile,
                # echec silencieux serait trompeur) -- les confirmations de
                # succes (reset/choix applique) ont ete retirees a la
                # demande utilisateur : le choix est deja visible/applique
                # immediatement, une popup supplementaire etait de trop.
                messagebox.showerror(
                    ui["default_image_btn"],
                    ui["default_image_apply_failed_msg"].format(name=display_name),
                )

        PROJECT_DEFAULT_IMAGE_FILENAME = self.PROJECT_DEFAULT_IMAGE_FILENAME
        assets_dir = Path(__file__).resolve().parent / "assets" / "default_images"

        # Tuile "visuel par defaut du projet"
        row = tk.Frame(gallery, bg=bg, bd=1, relief="solid", padx=8, pady=8)
        row.pack(fill="x", pady=4)
        project_default_thumb = assets_dir / PROJECT_DEFAULT_IMAGE_FILENAME
        if project_default_thumb.exists():
            try:
                from PIL import Image, ImageTk

                img = Image.open(project_default_thumb)
                thumb_w = 160
                ratio = thumb_w / img.width
                thumb = img.resize(
                    (thumb_w, max(1, int(img.height * ratio))), Image.LANCZOS
                )
                photo = ImageTk.PhotoImage(thumb)
                self._default_image_photo_refs.append(photo)
                tk.Label(row, image=photo, bg=bg).pack(side="left")
            except Exception:
                pass
        tk.Label(
            row,
            text=ui["default_image_reset_label"],
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 10, "bold"),
        ).pack(side="left", padx=(8, 0) if project_default_thumb.exists() else (0, 0))
        tk.Button(
            row,
            text=ui["default_image_choose_btn"],
            command=lambda: _apply_choice(None, "", is_reset=True),
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
            padx=8,
            pady=2,
        ).pack(side="right")

        # Tuiles des propositions fournies (tools/assets/default_images/*.png)
        if assets_dir.exists():
            for png_path in sorted(assets_dir.glob("*.png")):
                if png_path.name == PROJECT_DEFAULT_IMAGE_FILENAME:
                    # Deja affiche ci-dessus (tuile "visuel par defaut du
                    # projet") -- ne pas le proposer une 2e fois comme choix
                    # normal de la galerie.
                    continue
                row = tk.Frame(gallery, bg=bg, bd=1, relief="solid", padx=8, pady=8)
                row.pack(fill="x", pady=4)
                try:
                    from PIL import Image, ImageTk

                    img = Image.open(png_path)
                    thumb_w = 160
                    ratio = thumb_w / img.width
                    thumb = img.resize(
                        (thumb_w, max(1, int(img.height * ratio))), Image.LANCZOS
                    )
                    photo = ImageTk.PhotoImage(thumb)
                    self._default_image_photo_refs.append(photo)
                    tk.Label(row, image=photo, bg=bg).pack(side="left")
                except Exception:
                    pass
                tk.Label(
                    row,
                    text=png_path.stem,
                    bg=bg,
                    fg=fg,
                    font=("TkDefaultFont", 9),
                ).pack(side="left", padx=(8, 0))
                tk.Button(
                    row,
                    text=ui["default_image_choose_btn"],
                    command=lambda p=png_path: _apply_choice(p, p.name),
                    bg=bg_normal,
                    fg=fg,
                    bd=2,
                    relief="solid",
                    padx=8,
                    pady=2,
                ).pack(side="right")

        # Import d'une image personnalisee
        def _on_import() -> None:
            path_str = filedialog.askopenfilename(
                title=ui["default_image_import_btn"],
                filetypes=[("Images", "*.png *.jpg *.jpeg *.bmp *.gif")],
            )
            if not path_str:
                return
            src = Path(path_str)
            # Ajoutee au dossier partage des propositions
            # (tools/assets/default_images/, meme "assets_dir" que la galerie
            # ci-dessus) plutot qu'a un chemin cache pres des prefs : l'image
            # importee devient une tuile de la galerie comme les autres,
            # reutilisable directement sans avoir a la reimporter.
            assets_dir.mkdir(parents=True, exist_ok=True)
            base_name = re.sub(r"[^A-Za-z0-9_-]+", "_", src.stem).strip("_") or "custom"
            dst = assets_dir / f"{base_name}.png"
            counter = 1
            # Le nom reserve PROJECT_DEFAULT_IMAGE_FILENAME (visuel par
            # defaut du projet) ne doit jamais etre ecrase par un import,
            # meme si l'utilisateur importe un fichier qui porte
            # coincidentalement ce nom.
            while dst.exists() or dst.name == PROJECT_DEFAULT_IMAGE_FILENAME:
                dst = assets_dir / f"{base_name}_{counter}.png"
                counter += 1
            try:
                from PIL import Image

                with Image.open(src) as im:
                    im.convert("RGB").save(dst, "PNG")
            except Exception as e:
                messagebox.showerror(ui["msg_error_title"], str(e))
                return
            _apply_choice(dst, dst.name)

        tk.Button(
            body,
            text=ui["default_image_import_btn"],
            command=_on_import,
            bg=bg_action,
            fg="#000000",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        ).pack(fill="x", pady=(12, 6))

        def _on_close():
            if reset_on_close:
                _apply_choice(None, "", is_reset=True)
            else:
                dlg.destroy()

        tk.Button(
            body,
            text=ui["btn_close"],
            command=_on_close,
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        ).pack(fill="x")

        dlg.protocol("WM_DELETE_WINDOW", _on_close)
        dlg.update_idletasks()
        self._center_toplevel(dlg)
        dlg.grab_set()
        # Attend la fermeture (utilise aussi en pre-vol synchrone Mode 1
        # depuis _on_start_clicked, avant le lancement du worker -- sans
        # ceci, cet appel retournerait immediatement sans attendre le choix
        # de l'utilisateur).
        self.root.wait_window(dlg)

    def _mode1_get_selected_systems(self, roms_root: Path) -> Optional[list]:
        """
        Reprend la selection Tout/Rien/Manuel de la listbox active (Main
        self.sys_list, ou Avance self.sys_list_adv pour le Mode 3 -- meme
        detection d'onglet visible que _on_start_clicked) pour cibler le
        nettoyage sur les memes systemes que l'extraction.
        Retourne None si aucune detection n'a ete faite (-> tous les systemes
        du dossier ROMs), ou [] si l'utilisateur a explicitement choisi
        "Ne rien selectionner".
        """
        active_list = self.sys_list
        if hasattr(self, "sys_list_adv") and self.sys_list_adv.winfo_exists():
            try:
                if self.tab_advanced.winfo_ismapped():
                    active_list = self.sys_list_adv
            except Exception:
                pass

        try:
            if active_list.size() == 0:
                return None
            systems_all = self._find_systems(roms_root)
        except Exception:
            return None
        if not systems_all:
            return None

        selected_indices = list(active_list.curselection())
        real_indices = [i - 3 for i in selected_indices if i >= 3]
        if real_indices:
            return [systems_all[i] for i in real_indices if 0 <= i < len(systems_all)]
        if 1 in selected_indices:
            return []
        return None

    def _on_mode1_clean_clicked(self, btn: tk.Button) -> None:
        ui = self._get_ui_t()
        roms_root = self._get_roms_root_or_warn()
        if roms_root is None:
            return

        profile_name = self._mode1_profile_var.get()
        selected_systems = self._mode1_get_selected_systems(roms_root)
        if selected_systems == []:
            messagebox.showinfo(ui["mode1_clean_btn"], ui["sys_sel_opt_none"])
            return

        btn.config(state="disabled")

        def _preview_worker():
            preview = None
            error = None
            try:
                preview = self.tkmod.list_scrape_media_files(
                    roms_root, selected_systems, profile_name
                )
            except Exception as e:
                error = e

            def _after_preview():
                btn.config(state="normal")
                if error is not None:
                    messagebox.showerror(ui["msg_error_title"], str(error))
                    return
                if preview["total"] == 0:
                    messagebox.showinfo(
                        ui["mode1_clean_btn"],
                        ui["mode1_clean_none_msg"].format(folder=preview["img_subdir"]),
                    )
                    return
                msg = ui["mode1_clean_confirm_msg"].format(
                    count=preview["total"],
                    folder=preview["img_subdir"],
                    n_systems=len(preview["by_system"]),
                )
                if messagebox.askyesno(ui["mode1_clean_confirm_title"], msg):
                    self._mode1_run_clean(roms_root, selected_systems, profile_name, btn)

            self.root.after(0, _after_preview)

        self._start_worker(_preview_worker)

    def _mode1_run_clean(
        self, roms_root: Path, selected_systems, profile_name: str, btn: tk.Button
    ) -> None:
        ui = self._get_ui_t()
        btn.config(state="disabled")

        def _worker():
            log_writer = QueueWriter(self._log_q)
            old_stdout = sys.stdout
            old_stderr = sys.stderr
            result = None
            error = None
            try:
                sys.stdout = log_writer  # type: ignore[assignment]
                sys.stderr = log_writer  # type: ignore[assignment]
                print("=" * 60)
                print(" Nettoyage des dossiers avant scrape")
                print(f" Profil : {profile_name}")
                print("=" * 60)
                result = self.tkmod.clean_scrape_media_folders(
                    roms_root,
                    selected_systems,
                    profile_name,
                    progress_cb=self._progress_cb,
                )
                print(
                    f"Supprimes : {result['deleted']}  |  "
                    f"Erreurs : {len(result['errors'])}"
                )
            except Exception as e:
                error = e
            finally:
                sys.stdout = old_stdout
                sys.stderr = old_stderr

            def _done():
                btn.config(state="normal")
                if error is not None:
                    messagebox.showerror(ui["msg_error_title"], str(error))
                    return
                msg = ui["mode1_clean_done_msg"].format(
                    deleted=result["deleted"],
                    folder=result["img_subdir"],
                    n_systems=len(result["by_system"]),
                )
                if result["errors"]:
                    msg += ui["mode1_clean_errors_msg"].format(
                        n_errors=len(result["errors"])
                    )
                messagebox.showinfo(ui["mode1_clean_btn"], msg)

            self.root.after(0, _done)

        self._start_worker(_worker)

    def _center_toplevel(self, win: tk.Toplevel) -> None:
        # Centre la popup au milieu de la fenêtre principale

        try:
            root_x = self.root.winfo_x()
            root_y = self.root.winfo_y()
            root_w = self.root.winfo_width()
            root_h = self.root.winfo_height()
            w = win.winfo_reqwidth()
            h = win.winfo_reqheight()
            x = root_x + (root_w - w) // 2
            y = root_y + (root_h - h) // 2
            win.geometry(f"+{x}+{y}")
        except Exception:
            pass

    def _on_mode8_check_clicked(self):
        self._on_start_clicked()

    def _on_mode8_open_report_clicked(self):
        import os

        if hasattr(self, "_mode8_report_path") and self._mode8_report_path:
            os.startfile(str(self._mode8_report_path))

    def _on_mode8_compare_final_clicked(self):
        if not getattr(self, "_mode8_last_roms_root", None):
            return
        ui = self._get_ui_t()
        sd_root = None
        if messagebox.askyesno(
            ui["mode8_compare_ask_sd_title"], ui["mode8_compare_ask_sd"]
        ):
            chosen = filedialog.askdirectory(title=ui["mode8_compare_ask_sd_title"])
            if chosen:
                sd_root = Path(chosen)

        self._mode8_compare_final_btn.config(
            state="disabled", text=ui["mode8_btn_compare_running"]
        )
        self._start_worker(self._mode8_compare_final_worker, args=(sd_root,))

    def _mode8_compare_final_worker(self, sd_root) -> None:
        ui = self._get_ui_t()
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        success = False
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]

            print("=" * 60)
            print(" MODE 8 - Comparaison avec le support final")
            print("=" * 60)
            systems_out = self.sd_dir / "systems"
            print("Dossier temporaire : " + str(systems_out))
            print("SD physique        : " + str(sd_root or "(non verifiee)"))
            print()

            result = self.tkmod.check_final_media(
                self._mode8_last_roms_root,
                systems_out,
                sd_root=sd_root,
                selected_systems=self._mode8_last_selected_systems,
                profile_name=self._mode8_last_profile,
                progress_cb=self._progress_cb,
            )

            print()
            print("=" * 60)
            print(" RESULTAT DE LA COMPARAISON FINALE")
            print("=" * 60)
            counts = result["counts"]
            print("Total jeux verifies     : " + str(result["total"]))
            print("OK                      : " + str(counts["ok"]))
            print("Manquantes (ROMs)       : " + str(counts["missing_source"]))
            print("Manquantes (conversion) : " + str(counts["missing_converted"]))
            print("Manquantes (copie SD)   : " + str(counts["missing_on_sd"]))

            csv_path, txt_path = self.tkmod.generate_final_media_report(
                result, self.sd_dir
            )
            self._mode8_final_report_path = txt_path
            self._mode8_final_report_csv_path = csv_path
            success = True

            print()
            print(ui["mode8_final_done"])
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

            def _on_done():
                self._mode8_compare_final_btn.config(
                    state="normal", text=ui["mode8_btn_compare_final"]
                )
                if success:
                    self._mode8_open_final_report_btn.config(state="normal")
                    # Revenir sur l'onglet Avance et ouvrir automatiquement
                    # le rapport de comparaison finale.
                    self.nb_top.select(self.tab_advanced)
                    try:
                        os.startfile(str(self._mode8_final_report_path))
                    except Exception:
                        pass

            self.root.after(0, _on_done)

    def _on_mode8_open_final_report_clicked(self):
        import os

        if hasattr(self, "_mode8_final_report_path") and self._mode8_final_report_path:
            os.startfile(str(self._mode8_final_report_path))

    def _pipeline_mode_8(self, toolkit, cfg):
        roms_root = cfg.roms_root
        selected = cfg.systems_selected
        print("=" * 60)
        print(" MODE 8 - Verification des images manquantes")
        print("=" * 60)
        print()
        selected_systems = list(selected) if selected else None
        profile = "10.x"
        try:
            profile = self._mode1_profile_var.get()
        except Exception:
            pass
        print("Version Recalbox : " + str(profile))
        print("Dossier ROMs     : " + str(roms_root))
        print()
        result = toolkit.check_missing_images_gamelist(
            roms_root, selected_systems, profile, progress_cb=self._progress_cb
        )
        print()
        print("=" * 60)
        print(" RESULTAT DE LA VERIFICATION")
        print("=" * 60)
        print("Jeux scannes       : " + str(result["total_games"]))
        print("Images presentes   : " + str(result["total_present"]))
        print("Images manquantes  : " + str(result["total_missing"]))
        if result["total_missing"] > 0:
            print()
            print("DETAIL DES IMAGES MANQUANTES :")
            for sys_name, game_name, expected_path in result["missing_flat"]:
                print(
                    "   ["
                    + str(sys_name)
                    + "] "
                    + str(game_name)
                    + " -> "
                    + str(expected_path)
                )
        report_path = toolkit.generate_missing_images_report(result, self.sd_dir)
        self._mode8_report_path = report_path
        self._mode8_last_roms_root = roms_root
        self._mode8_last_selected_systems = selected_systems
        self._mode8_last_profile = profile
        self.root.after(
            0,
            lambda: (
                self._mode8_open_report_btn.config(state="normal"),
                self._mode8_compare_final_btn.config(state="normal"),
            ),
        )
        print()
        print("Mode 8 termine.")

    def _on_pipeline_finished(self) -> None:
        current_mode = self.mode_var.get()
        if current_mode == "1" and not getattr(self, "_mode1_scripts_installed_ok", False):
            # Scripts Recalbox non installes automatiquement (RB non
            # confirmee, injoignable, ou installation reseau echouee) --
            # rappel explicite en fin de process (demande utilisateur).
            # Message DEDIE (mode1_rb_reminder_msg), pas mode1_rb_unreachable_msg :
            # ce dernier pose la question "voulez-vous ressaisir l'IP ?"
            # qui n'a pas de sens ici (aucune saisie interactive en cours a
            # ce stade) -- garde uniquement l'orientation Mode 9 + le
            # rappel de consequence (mode playlist/horloge uniquement) + le
            # chemin du dossier local pret a copier a la main.
            ui_end = self._get_ui_t()
            staged_dir = getattr(
                self, "_mode1_scripts_staged_dir", self.sd_dir / "recalbox_userscripts"
            )
            target_display = (
                getattr(self, "_mode1_scripts_target", None)
                or prefs.get("recalbox_ip")
                or "?"
            )
            self._themed_info(
                ui_end["mode1_rb_reminder_title"],
                ui_end["mode1_rb_reminder_msg"](target_display, staged_dir),
            )
        if current_mode == "8":
            # Revenir sur l'onglet Avance et ouvrir automatiquement le
            # rapport de verification Mode 8.
            self.nb_top.select(self.tab_advanced)
            if getattr(self, "_mode8_report_path", None):
                try:
                    os.startfile(str(self._mode8_report_path))
                except Exception:
                    pass
            return
        # Ne force plus l'onglet Main : _poll_processing_done() (v25) a deja
        # restaure l'onglet d'origine (Main ou Avance) une fois le
        # traitement termine. Le panneau "copie SD" existe desormais dans
        # les deux onglets (_start_mode6_blinking gere les 2 instances).
        self._start_mode6_blinking()
        # Activer aussi "Explorer le dossier de sortie" dès que les fichiers existent
        for inst in self._mode6_instances():
            if inst["explore_btn"]:
                try:
                    inst["explore_btn"].config(state="normal")
                except Exception:
                    pass

    def _on_mode6_button_clicked(self, drive_list_widget=None) -> None:
        # drive_list_widget : quelle instance (Main ou Avance) a declenche
        # le clic -- determine quelle selection de lecteur lire. Retenue
        # pour que les reessais (retry tout/echecs) relisent la meme
        # instance sans redemander a l'utilisateur de re-cliquer dans le
        # bon onglet.
        active_drive_list = drive_list_widget or self._mode6_drive_list
        self._mode6_active_drive_list = active_drive_list

        # stop blinking and start flash (non-interactif)
        if self._mode6_blinking:
            self._stop_mode6_blinking()

        ui_running = self._get_ui_t()["mode6_btn_running"]
        for inst in self._mode6_instances():
            if inst["btn"]:
                inst["btn"].config(state="disabled", text=ui_running)

        # determine chosen drive
        chosen_index: int | None = None
        try:
            sel = list(active_drive_list.curselection())
            if sel:
                chosen_index = sel[0]
        except Exception:
            chosen_index = None

        if (
            chosen_index is None
            or chosen_index < 0
            or chosen_index >= len(self._mode6_drives)
        ):
            self._refresh_mode6_drives()
            chosen_index = 0 if self._mode6_drives else None

        if chosen_index is None:
            ui_now = self._get_ui_t()
            messagebox.showwarning(ui_now["msg_warning_title"], ui_now["mode6_no_drives"])
            for inst in self._mode6_instances():
                if inst["btn"]:
                    inst["btn"].config(state="normal")
            return

        letter, _label, _size = self._mode6_drives[chosen_index]
        dst_drive = f"{letter}\\\\"
        ui = self._get_ui_t()

        # Une copie precedente vers CE lecteur a-t-elle ete interrompue ?
        # (_copy_progress.json est ecrit/mis a jour par _copy_to_drive)
        manifest_path = self.sd_dir / "_copy_progress.json"
        manifest = None
        try:
            manifest = self.tkmod.read_copy_manifest(manifest_path)
        except Exception:
            manifest = None
        resumable = (
            manifest is not None
            and manifest.get("status") != "completed"
            and manifest.get("destination") == dst_drive
            and manifest.get("source") == str(self.sd_dir)
        )

        if resumable and manifest is not None:
            msg = ui["mode6_resume_msg"].format(
                copied=manifest.get("copied", "?"),
                total=manifest.get("total_files", "?"),
                date=manifest.get("timestamp", "?"),
            )
            answer = messagebox.askyesno(ui["mode6_resume_title"], msg)
        else:
            # Detecter fichiers existants sur la destination
            # Demander a l'utilisateur s'il veut ecraser les fichiers existants
            answer = messagebox.askyesno(
                ui["existing_files_sd_title"],
                ui["existing_files_sd_msg"].format(letter=letter),
            )
        overwrite = answer
        self._mode6_flash_thread = threading.Thread(
            target=self._mode6_flash_worker,
            args=(dst_drive, overwrite),
            daemon=True,
        )
        self._mode6_flash_thread.start()

    def _mode6_flash_worker(self, dst_drive: str, overwrite: bool) -> None:
        # Rediriger stdout/stderr vers les logs GUI
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]

            # Reste en mode DETERMINATE (pas indeterminate + .start()) :
            # _copy_to_drive() ci-dessous envoie une vraie progression
            # (copied/total_files, deja throttlee par le temps cote outil)
            # via progress_cb -> _progress_cb_ui(), qui appelle
            # self.progress.configure(value=pct) a chaque mise a jour.
            # Combiner ca avec l'animation automatique interne du mode
            # indeterminate (qui deplace le curseur toute seule sur un
            # minuteur, independamment de "value") faisait litteralement
            # concurrence aux 2 mecanismes sur le meme widget -- l'un fait
            # rebondir le curseur, l'autre le fait sauter a la vraie
            # position toutes les ~100ms, d'ou le "va-et-vient"/tremblement
            # constate par l'utilisateur. Determinate + progression globale
            # reelle (idx/total, pas un curseur "actif" factice par fichier).
            self.root.after(
                0,
                lambda: (
                    self.progress.configure(mode="determinate", maximum=100, value=0),
                    self.progress_pct_var.set("0%"),
                ),
            )

            manifest_path = self.sd_dir / "_copy_progress.json"
            result = self.tkmod._copy_to_drive(
                self.sd_dir,
                dst_drive,
                overwrite,
                progress_cb=self._progress_cb,  # type: ignore[arg-type]
                manifest_path=manifest_path,
            )
            if isinstance(result, tuple) and len(result) == 3:
                copied, failed, interrupted = result
            elif isinstance(result, tuple):
                copied, failed = result[0], result[1]
                interrupted = False
            else:
                copied, failed, interrupted = result, [], False

            self._mode6_last_dst_drive = dst_drive
            self._mode6_selected_drive = dst_drive
            self._mode6_last_failed = list(failed)

            self.root.after(
                0,
                lambda: (
                    self.progress.stop(),
                    self.progress.configure(mode="determinate"),
                    self.progress.configure(value=100),
                    self.progress_pct_var.set("100%"),
                ),
            )
            if interrupted:
                raise RuntimeError(
                    "La carte SD est devenue inaccessible en cours de copie "
                    "(débranchée ?). Rebranchez-la puis réessayez : la copie "
                    "reprendra où elle s'est arrêtée."
                )
            if failed:
                raise RuntimeError(
                    f"{len(failed)} fichier(s) en échec. "
                    "Vérifiez que la carte SD n'est pas protégée en écriture "
                    "et qu'il reste de l'espace libre."
                )
        except Exception as e:
            print(f"❌ [GUI] Erreur copie SD : {e}")
            self.root.after(0, self._on_mode6_flash_error, str(e))
            return
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

        self.root.after(0, self._on_mode6_flash_done)

    def _on_mode6_flash_error(self, error_msg: str) -> None:
        """Affiche une boîte de dialogue d'erreur avec possibilité de réessayer
        (tout, ou seulement les fichiers en échec s'il y en a)."""
        print(f"❌ Copie SD échouée : {error_msg}")
        # Réactiver le bouton pour permettre une nouvelle tentative
        ui_start = self._get_ui_t()["mode6_btn_start"]
        for inst in self._mode6_instances():
            if inst["btn"]:
                inst["btn"].config(state="normal", text=ui_start)

        ui = self._get_ui_t()
        failed = list(getattr(self, "_mode6_last_failed", []) or [])
        dst_drive = getattr(self, "_mode6_last_dst_drive", None)

        if not failed or not dst_drive:
            # Rien de precis a re-tenter isolement (ex: SD inaccessible des
            # le debut) -> dialogue simple.
            retry = messagebox.askretrycancel(
                ui["mode6_retry_title"],
                f"❌ La copie vers la carte SD a échoué.\n\n{error_msg}\n\n"
                "Voulez-vous réessayer ?",
            )
            if retry:
                self._on_mode6_button_clicked(
                    getattr(self, "_mode6_active_drive_list", None)
                )
            return

        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        bg_danger = c.get("bg_button_danger", "#F5F5F5")

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["mode6_retry_title"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg=bg)

        msg = f"❌ La copie vers la carte SD a échoué.\n\n{error_msg}"
        tk.Label(
            dlg, text=msg, bg=bg, fg=fg, justify="left", padx=14, pady=12, wraplength=380
        ).pack(fill="both", expand=True)

        btn_frame = tk.Frame(dlg, bg=bg)
        btn_frame.pack(fill="x", padx=14, pady=(0, 12))

        def _choose(action: str) -> None:
            dlg.destroy()
            if action == "all":
                self._on_mode6_button_clicked(
                    getattr(self, "_mode6_active_drive_list", None)
                )
            elif action == "failed":
                self._on_mode6_retry_failed_clicked(dst_drive, failed)

        tk.Button(
            btn_frame, text=ui["mode6_retry_failed_btn"].format(n=len(failed)),
            command=lambda: _choose("failed"),
            bg=bg_action, fg="#000000", bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(fill="x", pady=(0, 6))
        tk.Button(
            btn_frame, text=ui["mode6_retry_all_btn"], command=lambda: _choose("all"),
            bg=bg_normal, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(fill="x", pady=(0, 6))
        tk.Button(
            btn_frame, text=ui["mode6_retry_cancel_btn"], command=lambda: _choose("cancel"),
            bg=bg_danger, fg=fg, bd=2, relief="solid",
            padx=10, pady=6, font=("TkDefaultFont", 10, "bold"),
        ).pack(fill="x")

        self._center_toplevel(dlg)

    def _on_mode6_retry_failed_clicked(self, dst_drive: str, failed: list) -> None:
        ui_running = self._get_ui_t()["mode6_btn_running"]
        for inst in self._mode6_instances():
            if inst["btn"]:
                inst["btn"].config(state="disabled", text=ui_running)
        # Suit le meme schema que le flash initial (_mode6_flash_thread,
        # deja verifie par _is_processing()) plutot que _start_worker() :
        # le Mode 6 reste volontairement sur l'onglet Main pendant le
        # flash/retry (feedback dans l'UI meme -- bouton clignotant, liste
        # des lecteurs), pas de bascule automatique vers Logs a eviter ici.
        self._mode6_flash_thread = threading.Thread(
            target=self._mode6_flash_retry_failed_worker,
            args=(dst_drive, failed),
            daemon=True,
        )
        self._mode6_flash_thread.start()

    def _mode6_flash_retry_failed_worker(self, dst_drive: str, failed: list) -> None:
        log_writer = QueueWriter(self._log_q)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        try:
            sys.stdout = log_writer  # type: ignore[assignment]
            sys.stderr = log_writer  # type: ignore[assignment]

            self.root.after(
                0,
                lambda: (
                    self.progress.configure(mode="indeterminate"),
                    self.progress.start(10),
                    self.progress_pct_var.set("…"),
                ),
            )

            copied, still_failed = self.tkmod._copy_specific_files(
                self.sd_dir,
                dst_drive,
                failed,
                overwrite=True,
                progress_cb=self._progress_cb,  # type: ignore[arg-type]
            )
            self._mode6_last_dst_drive = dst_drive
            self._mode6_selected_drive = dst_drive
            self._mode6_last_failed = list(still_failed)

            self.root.after(
                0,
                lambda: (
                    self.progress.stop(),
                    self.progress.configure(mode="determinate"),
                    self.progress.configure(value=100),
                    self.progress_pct_var.set("100%"),
                ),
            )
            if still_failed:
                raise RuntimeError(
                    f"{len(still_failed)} fichier(s) toujours en échec. "
                    "Vérifiez que la carte SD n'est pas protégée en écriture "
                    "et qu'il reste de l'espace libre."
                )
        except Exception as e:
            print(f"❌ [GUI] Erreur retry copie SD : {e}")
            self.root.after(0, self._on_mode6_flash_error, str(e))
            return
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

        self.root.after(0, self._on_mode6_flash_done)

    def _on_mode6_flash_done(self) -> None:
        ui = self._get_ui_t()
        for inst in self._mode6_instances():
            if inst["btn"]:
                inst["btn"].config(state="normal", text=ui["mode6_btn_start"])
            if inst["explore_btn"]:
                inst["explore_btn"].config(state="normal")

        # Popup avec 3 choix : Explorer SD, Explorer temp, Fermer
        dst_drive = self._mode6_selected_drive or ""
        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")
        dlg = tk.Toplevel(self.root)
        dlg.title(ui["mode6_done"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg=bg)
        self._center_toplevel(dlg)

        lbl = tk.Label(
            dlg,
            text=f"{ui['mode6_done']}\n\n{ui['what_to_do_next']}",
            bg=bg,
            fg=fg,
            font=("TkDefaultFont", 11),
            padx=20,
            pady=12,
        )
        lbl.pack(fill="both", expand=True)

        btn_frame = tk.Frame(dlg, bg=bg, padx=14, pady=10)
        btn_frame.pack(fill="x")

        def open_sd():
            try:
                os.startfile(dst_drive)
            except Exception:
                pass

        def open_temp():
            try:
                os.startfile(str(self.sd_dir))
            except Exception:
                pass

        def close_dlg():
            dlg.destroy()
            # Mode 1 entierement termine (copie physique vers la carte SD
            # comprise, pas seulement la generation locale) : demande
            # utilisateur explicite -- affiche la marche a suivre physique
            # (inserer la carte, demarrer le DMD, les 2 phases automatiques
            # de configuration WiFi puis DMD). Uniquement pour le Mode 1 :
            # ce meme panneau "copie SD" est reutilise par d'autres modes
            # (6, etc.) pour lesquels cette suite n'a pas de sens.
            if self.mode_var.get() == "1":
                self._themed_info(ui["mode1_next_steps_title"], ui["mode1_next_steps_msg"])

        btn_sd = tk.Button(
            btn_frame,
            text=ui["explore_sd_btn"],
            command=open_sd,
            bg=bg_action,
            fg="#000000",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        btn_sd.pack(fill="x", pady=2)

        btn_temp = tk.Button(
            btn_frame,
            text=ui["explore_temp_btn"],
            command=open_temp,
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        btn_temp.pack(fill="x", pady=2)

        btn_close = tk.Button(
            btn_frame,
            text=ui["btn_close"],
            command=close_dlg,
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
        )
        btn_close.pack(fill="x", pady=2)

    def _on_quit_app_clicked(self) -> None:
        ui = self._get_ui_t()
        # Remplace messagebox.askokcancel par une popup custom avec bouton "Explorer"
        result_holder: dict[str, bool] = {"ok": False}

        lang = self.lang_var.get()
        cancel_lbl = "Annuler" if lang != "en" else "Cancel"
        cancel_lbl = "Cancelar" if lang == "es" else cancel_lbl
        ok_lbl = "OK"

        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")
        bg_normal = c.get("bg_button_normal", "#FFFFFF")

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["quit_app_warning_title"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg=bg)

        lbl = tk.Label(
            dlg,
            text=ui["quit_app_warning"],
            bg=bg,
            fg=fg,
            justify="left",
            padx=14,
            pady=12,
        )
        lbl.pack(fill="both", expand=True)

        keep_temp_var = tk.BooleanVar(value=self._quit_keep_temp_dir)
        keep_temp_chk = tk.Checkbutton(
            dlg,
            text=ui["quit_app_keep_temp_checkbox"],
            variable=keep_temp_var,
            bg=bg,
            fg=fg,
            activebackground=bg,
            activeforeground=fg,
            selectcolor=bg,
            anchor="w",
        )
        keep_temp_chk.pack(fill="x", padx=14, pady=(0, 8))

        btns = tk.Frame(dlg, bg=bg, padx=10, pady=10)
        btns.pack()

        def explore_output_dir() -> None:
            try:
                os.startfile(str(self.sd_dir))  # type: ignore[attr-defined]
            except Exception:
                pass

        def on_cancel() -> None:
            result_holder["ok"] = False
            try:
                dlg.destroy()
            except Exception:
                pass

        def on_ok() -> None:
            result_holder["ok"] = True
            self._quit_keep_temp_dir = bool(keep_temp_var.get())
            try:
                dlg.destroy()
            except Exception:
                pass

        explore_btn = tk.Button(
            btns,
            text=ui.get("mode6_explore_output_btn", "Explorer le dossier de sortie"),
            width=28,
            command=explore_output_dir,
            bg=bg_normal,
            fg=fg,
            bd=2,
            relief="solid",
        )
        explore_btn.grid(row=0, column=0, padx=6)

        b_cancel = tk.Button(
            btns, text=cancel_lbl, width=14, command=on_cancel,
            bg=bg_normal, fg=fg, bd=2, relief="solid",
        )
        b_cancel.grid(row=0, column=1, padx=6)

        b_ok = tk.Button(
            btns, text=ok_lbl, width=10, command=on_ok,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
        )
        b_ok.grid(row=0, column=2, padx=6)

        self._center_toplevel(dlg)
        # Centre la popup
        try:
            self.root.update_idletasks()
            dlg.update_idletasks()
            w = dlg.winfo_width()
            h = dlg.winfo_height()
            if w > 1 and h > 1:
                root_x = self.root.winfo_rootx()
                root_y = self.root.winfo_rooty()
                root_w = self.root.winfo_width()
                root_h = self.root.winfo_height()
                x = root_x + (root_w - w) // 2
                y = root_y + (root_h - h) // 2
                dlg.geometry(f"+{x}+{y}")
        except Exception:
            pass

        self.root.wait_window(dlg)

        if not result_holder["ok"]:
            return

        # Always stop blinking immediately (UI-only)
        self._stop_mode6_blinking()

        # If processing is running, show a custom dialog (Annuler / Explorer / OK)
        worker_alive = bool(self._worker and self._worker.is_alive())

        if self._is_processing():
            c = self._theme_colors()
            bg = c.get("bg_main", "#F3F3F3")
            fg = c.get("fg_text", "#000000")
            bg_action = c.get("bg_button_action", "#FFD400")
            bg_normal = c.get("bg_button_normal", "#FFFFFF")

            dlg = tk.Toplevel(self.root)
            dlg.title(ui["quit_app_warning_title"])
            dlg.resizable(False, False)
            dlg.transient(self.root)
            dlg.grab_set()
            dlg.configure(bg=bg)

            msg = (
                ui["quit_app_stopped_worker"]
                if worker_alive
                else ui["quit_app_warning"]
            )
            lbl = tk.Label(dlg, text=msg, bg=bg, fg=fg, justify="left", padx=14, pady=12)
            lbl.pack(fill="both", expand=True)

            btns = tk.Frame(dlg, bg=bg, padx=10, pady=10)
            btns.pack()

            lang = self.lang_var.get()
            cancel_lbl = "Annuler" if lang != "en" else "Cancel"
            cancel_lbl = "Cancelar" if lang == "es" else cancel_lbl

            def explore_output_dir() -> None:
                try:
                    os.startfile(str(self.sd_dir))  # type: ignore[attr-defined]
                except Exception:
                    pass

            def on_cancel() -> None:
                try:
                    dlg.destroy()
                except Exception:
                    pass

            def on_ok() -> None:
                # Request stop only on OK
                if worker_alive:
                    try:
                        self.tkmod.PAUSE.request_stop()
                    except Exception:
                        pass
                try:
                    dlg.destroy()
                except Exception:
                    pass
                self.root.after(300, self._wait_for_threads_then_exit)

            b_cancel = tk.Button(
                btns, text=cancel_lbl, width=14, command=on_cancel,
                bg=bg_normal, fg=fg, bd=2, relief="solid",
            )
            b_cancel.grid(row=0, column=1, padx=6)

            ok_btn = tk.Button(
                btns, text="OK", width=10, command=on_ok,
                bg=bg_action, fg="#000000", bd=2, relief="solid",
            )
            ok_btn.grid(row=0, column=2, padx=6)

            self._center_toplevel(dlg)
            # Centre la popup
            try:
                self.root.update_idletasks()
                dlg.update_idletasks()
                w = dlg.winfo_width()
                h = dlg.winfo_height()
                if w > 1 and h > 1:
                    root_x = self.root.winfo_rootx()
                    root_y = self.root.winfo_rooty()
                    root_w = self.root.winfo_width()
                    root_h = self.root.winfo_height()
                    x = root_x + (root_w - w) // 2
                    y = root_y + (root_h - h) // 2
                    dlg.geometry(f"+{x}+{y}")
            except Exception:
                pass

            self.root.wait_window(dlg)
            return

        # No running work -> cleanup now (sauf si l'utilisateur a coche
        # "Conserver le dossier temporaire" dans la popup precedente)
        if not self._quit_keep_temp_dir:
            self._cleanup_sd_dir()

        c = self._theme_colors()
        bg = c.get("bg_main", "#F3F3F3")
        fg = c.get("fg_text", "#000000")
        bg_action = c.get("bg_button_action", "#FFD400")

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["quit_app_warning_title"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg=bg)

        self._center_toplevel(dlg)

        lbl = tk.Label(
            dlg,
            text=(
                ui["quit_app_kept_temp_dir"]
                if self._quit_keep_temp_dir
                else ui["quit_app_cleanup_done"]
            ),
            bg=bg,
            fg=fg,
            justify="left",
            padx=14,
            pady=12,
        )
        lbl.pack(fill="both", expand=True)

        btns = tk.Frame(dlg, bg=bg, padx=10, pady=10)
        btns.pack()

        def explore_output_dir() -> None:
            try:
                os.startfile(str(self.sd_dir))  # type: ignore[attr-defined]
            except Exception:
                pass

        def on_ok() -> None:
            try:
                dlg.destroy()
            except Exception:
                pass
            try:
                self.root.destroy()
            except Exception:
                pass

        def on_close() -> None:
            on_ok()

        dlg.protocol("WM_DELETE_WINDOW", on_close)

        ok_btn = tk.Button(
            btns, text="OK", width=10, command=on_ok,
            bg=bg_action, fg="#000000", bd=2, relief="solid",
        )
        ok_btn.grid(row=0, column=1, padx=6)

    def _wait_for_threads_then_exit(self) -> None:
        if self._is_processing():
            self.root.after(300, self._wait_for_threads_then_exit)
            return

        if not self._quit_keep_temp_dir:
            self._cleanup_sd_dir()
        try:
            self.root.destroy()
        except Exception:
            pass

    # ──────────────────────────────────────────────────────────────────────────
    # close
    # ──────────────────────────────────────────────────────────────────────────
    def _on_close_attempt(self) -> None:
        # Fermeture par le X (WM_DELETE_WINDOW) : deleguer entierement au
        # meme flux que le bouton "Quitter" (_on_quit_app_clicked), qui
        # propose d'explorer le dossier temporaire et nettoie
        # systematiquement avant de fermer. L'ancienne implementation
        # separee ici ne nettoyait JAMAIS le dossier temporaire dans le
        # cas "aucun traitement en cours" (self.root.destroy() direct) et
        # ne proposait pas d'explorer avant nettoyage -- cause probable
        # des remontees "dossier temporaire parfois non nettoye".
        self._on_quit_app_clicked()

    # Index de l'onglet Logs dans nb_top (Main=0, Playlist=1, Avance=2,
    # Logs=3, Parametres=4, AIDE=5) -- seul onglet utile pendant un
    # traitement (texte de log + Pause/Reprise/Passe/Stop).
    _LOGS_TAB_INDEX = 3

    def _on_tab_changed(self, event=None) -> None:
        """Reslice l'image de fond quand l'onglet change,
        car les onglets non-visibles n'avaient pas leurs vraies dimensions."""
        # Bloquer le changement d'onglet tant qu'un traitement tourne :
        # remonte par des utilisateurs, changer d'onglet en cours de
        # traitement pouvait geler l'appli (decoupage de fond + rendu du
        # nouvel onglet en meme temps qu'un worker ecrit intensivement
        # dans les logs). On revient de force sur Logs.
        if self._is_processing():
            try:
                sel = self.nb_top.select()
                tab_idx = self.nb_top.index(sel) if sel else self._LOGS_TAB_INDEX
            except Exception:
                tab_idx = self._LOGS_TAB_INDEX
            if tab_idx != self._LOGS_TAB_INDEX:
                self.nb_top.select(self._LOGS_TAB_INDEX)
                return

        from RecalBoxDMD_themes import _slice_widgets_later

        # Forcer la mise a jour des dimensions des widgets de l'onglet
        self.root.update_idletasks()
        _slice_widgets_later(self)

        # Bug 6 : quand on change d'onglet, pré-sélectionner le mode approprié.
        # La bascule automatique sur le mode 2 ne doit avoir lieu qu'à la toute
        # première ouverture de l'onglet Avancé : ensuite, le mode
        # précédemment sélectionné dans cet onglet doit être restauré.
        try:
            sel = self.nb_top.select()
            tab_idx = self.nb_top.index(sel) if sel else 0

            # On quitte l'onglet Avancé (index 2) : mémoriser le mode en cours.
            if self._prev_tab_idx == 2 and tab_idx != 2:
                self._last_adv_mode = self.mode_var.get()

            if tab_idx == 0:
                # Onglet Main → passer en mode 1
                if self.mode_var.get() != "1":
                    self.mode_var.set("1")
            elif tab_idx == 1:
                # Onglet Playlist : rafraichit la liste des lecteurs SD (une
                # carte a pu etre branchee/debranchee pendant qu'un autre
                # onglet etait actif) et la liste des playlists existantes.
                # Ne relance PAS le scan des dossiers/fichiers (couteux,
                # casserait une selection de cases en cours) -- uniquement
                # au choix explicite d'un lecteur (_on_playlist_drive_selected).
                self._refresh_playlist_drives()
            elif tab_idx == 2:
                if self._adv_tab_first_open:
                    # Première ouverture de l'onglet Avancé → mode 2 par défaut
                    self._adv_tab_first_open = False
                    self._last_adv_mode = "2"
                    if self.mode_var.get() != "2":
                        self.mode_var.set("2")
                else:
                    # Ouvertures suivantes → restaurer le dernier mode utilisé
                    if self.mode_var.get() != self._last_adv_mode:
                        self.mode_var.set(self._last_adv_mode)

            self._prev_tab_idx = tab_idx
            self.root.after_idle(self._on_mode_changed)
        except Exception:
            pass

    def run(self) -> None:
        self.root.mainloop()


def run_gui(toolkit_module, sd_dir: Path) -> None:
    RetroBoxLEDGui(toolkit_module, sd_dir).run()


if __name__ == "__main__":
    import RecalBoxDMD_tool as toolkit

    sd = toolkit.get_sd_card_dir(Path(__file__).parent)
    RetroBoxLEDGui(toolkit, sd).run()
