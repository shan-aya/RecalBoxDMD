from __future__ import annotations

# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v15
#
# v15 — 2026-07-20 — safe-modify — Renommage de l'app "DMD GIF Converter" →
#      "DMD GIF Creator" + montée de version v3.0.0 (voir dmd_converter.py
#      v75 pour le détail complet) : clé `TEXT_MAP` "DMD Converter v2.7.4"
#      → "DMD Creator v3.0.0" (doit correspondre EXACTEMENT au texte du
#      widget d'en-tête une fois `APP_VERSION` bumpé, sinon la traduction
#      de ce libellé casse silencieusement au changement de langue).
#
# v14 — 2026-07-20 — safe-modify — Audit i18n complet de toute l'interface
#      (demande explicite "analyse toute l'interface et termine/corrige/
#      remplace les traductions manquantes ou erronées") : 5 entrées
#      ajoutées pour des widgets construits avec un texte français jamais
#      enregistré ici ni dans lang_fr.json — restaient figés en français
#      quelle que soit la langue (voir dmd_converter.py v74 pour le détail
#      complet, y compris les ~40 messagebox et le menu contextuel corrigés
#      directement via lang_manager.get(), hors du périmètre de ce
#      fichier) : "Seuil lettrage (px):" (letter_threshold_label), "🗑
#      Vider" (clear_image_list), "📂 Glisser-déposer\nun dossier ou des
#      images ici" (image_drop_hint), "🔄 Nouvelle proposition"
#      (regenerate_proposition — corrige au passage "New proposition",
#      codé en dur en anglais dans le code source), "🎨 Remplissage
#      (ACTIF)"/"🧹 Gomme Magique (ACTIF)" (fill_active/eraser_active,
#      variantes d'état actif jusque-là non enregistrées bien que leurs
#      libellés de base le soient déjà), "Animation" (animation_frame_label,
#      titre de cadre bare, distinct de "Animation:" déjà existant).
#
# v13 — 2026-07-19 — safe-modify — Sélecteur "Mode de cadrage" à 3 boutons
#      radio (demande explicite utilisateur "le mode zoom auto et tracking
#      auto s'excluent l'un l'autre... peut-être clarifier ce
#      fonctionnement ? [...] fait moi des propositions", option "sélecteur
#      3 modes" retenue via AskUserQuestion) : remplace les entrées
#      "🎯 Suivi automatique (tracking)" (video_roi_tracking) et
#      "🪄 Resize auto" (video_resize_auto) par "Mode de cadrage :"
#      (video_crop_mode_label), "🎯 Suivi automatique"
#      (video_crop_mode_tracking), "🪄 Cadrage auto (zoom)"
#      (video_crop_mode_auto_zoom), "✋ Manuel" (video_crop_mode_manual).
#
# v12 — 2026-07-19 — safe-modify — Refonte du tracking (zones multiples +
#      zoom keyframé, fonction phare de l'onglet VIDEO) : nouvelles entrées
#      "➕ Point ici" (video_roi_add_point), "🔍 Zoom ici"
#      (video_roi_add_zoom_point), "🗑️ Supprimer ce point"
#      (video_roi_delete_point), "📜 Historique de cadrage"
#      (video_roi_history_label). "↶ Annuler"/"↷ Rétablir" réutilisent TELS
#      QUELS les clés "undo"/"redo" déjà existantes (mêmes libellés exacts
#      que les boutons undo/redo de l'onglet MANUEL) — aucune nouvelle
#      entrée nécessaire pour ces 2 boutons.
#
# v11 — 2026-07-19 — safe-modify — Lecteur vidéo intégré + refonte du trim
#      (demande utilisateur) : clé "video_play_original" ("▶️ Lire
#      l'original") retirée — bouton supprimé, remplacé par un clic dans le
#      cadre "Lecture". Clé "video_roi_edit_time_label" ("Temps de
#      cadrage:") retirée — le scrubber dédié est remplacé par un 3e
#      marqueur directement sur la frise de trim. Nouvelle entrée "Lecture"
#      (label du cadre lecteur, clé "video_playback_label").
#
# v10 — 2026-07-19 — safe-modify — Cadrage manuel multi-instants + zoom
#      crop/resize (demande utilisateur) : "🔄 Redessiner ROI" renommé
#      "🔄 Recentrer" (clé TEXT_MAP "video_roi_reset" inchangée, valeur mise
#      à jour dans les 3 lang_*.json). Nouvelles entrées : "🗑️ Effacer
#      points", "Temps de cadrage:", "Zoom cadrage:", "(0% = resize seul,
#      100% = crop serré)", "🪄 Resize auto".
#
# v9 — 2026-07-19 — safe-modify — Onglet VIDEO : 2 nouveaux cadres info
#      (demande utilisateur, "ajoute un cadre info avec les données de la
#      vidéo d'origine" / "... du gif à exporter") : "ℹ️ Vidéo Source",
#      "ℹ️ GIF à exporter". Même pattern que "Informations Image" déjà
#      présent en AUTO/MANUEL (clé TEXT_MAP "info_image").
#
# v8 — 2026-07-19 — safe-modify — Cadrage manuel VIDEO sans tracking (demande
#      utilisateur) : renommage "Zone d'intérêt (suivie automatiquement)" →
#      "Zone d'intérêt (cadrage vidéo)" (le tracking devient optionnel, le
#      libellé ne doit plus l'affirmer inconditionnellement) — clé TEXT_MAP
#      "video_roi_label" inchangée, seule sa valeur change dans les 3
#      lang_*.json. Nouvelles entrées : "Aperçu du cadrage" (label de la
#      prévisualisation live 128×32), "🎯 Suivi automatique (tracking)"
#      (case à cocher activant/désactivant le tracker).
#
# v7 — 2026-07-19 — safe-modify — Ajout des libellés des nouveaux contrôles
#      VIDEO (dmd_converter.py v47, demande utilisateur : bouton lecture
#      originale, durée par défaut, estimation de poids) : "▶️ Lire
#      l'original", "Poids GIF estimé :". Le label "Durée (s):" est déjà
#      présent dans TEXT_MAP (clé "duration", onglet AUTO) et réutilisé tel
#      quel par le nouveau champ Durée de l'onglet VIDEO.
#
# v6 — 2026-07-19 — safe-modify — Ajout des libellés statiques du nouvel
#      onglet VIDEO (dmd_converter.py v45) : "📹 Charger Vidéo", "Sélection
#      (trim)", "Zone d'intérêt (suivie automatiquement)", "🔄 Redessiner
#      ROI", "🪄 Qualité automatique", "🎬 Générer Aperçu", "Aperçu Animation
#      (Vidéo)". Les labels déjà partagés par les autres onglets ("FPS:",
#      "Couleurs GIF:", "Boucle:", "Répétitions:", "Mode DMD / Forcer
#      pixel-perfect", "💾 Exporter GIF") sont réutilisés tels quels par
#      l'onglet VIDEO, aucune nouvelle entrée nécessaire pour eux (déjà
#      présents dans TEXT_MAP).
#
# v5 — 2026-07-18 — safe-modify — Audit complet de TEXT_MAP (149 entrées) contre
#      les widgets réels de dmd_converter.py (demande explicite de suite de TODO).
#      2 bugs réels corrigés (libellé renommé dans le code mais jamais mis à jour
#      dans TEXT_MAP ni les 3 lang_*.json, donc plus aucune retraduction possible
#      dans aucune langue pour ces 2 widgets, même mécanisme que le fix v3) :
#      - "🔓 Réautoriser sélection" → "🔓 Réautoriser" (bouton renommé, "sélection"
#        retiré du libellé réel depuis un moment).
#      - "Contrôles Avancés" → "⚙️ Contrôles Avancés" (le cadre affiche l'emoji
#        ⚙️ en préfixe depuis sa création, jamais reporté dans TEXT_MAP).
#      46 entrées mortes retirées de TEXT_MAP : 5 doublons legacy dont le libellé
#      vivant est conservé par ailleurs ("👁️ Preview"/"▶️ Preview" → remplacés par
#      "🎬 Prévisualiser", "⏹️ Stop" → "⛔ Interrompre", "↶ Undo" → "↶ Annuler",
#      "💾 Exporter" → "💾 Exporter GIF") + 41 entrées sans aucun libellé vivant
#      nulle part (boutons passés en icône seule depuis v20 : "📁 Dossier"/
#      "🖼️ Images"/"✓ Tout"/"✗ Rien"/"⇄ Inverser" ; panneau "Source:" supprimé en
#      v19 ; libellé "Langue:" remplacé par le cadre combiné "🌍 Langue / Language
#      / Idioma" ; "🔒 Verrouiller"/"🔄 Reset"/"🔄 Rafraîchir" jamais utilisés ; et
#      23 libellés français d'animations/directions/boucle/easing — ces derniers
#      n'ont JAMAIS pu fonctionner : les Combobox concernées utilisent des slugs
#      anglais bruts comme valeurs réelles ("fade_in", "horizontal", "normal"...),
#      et de toute façon la classe de widget "TCombobox" n'est pas traitée par
#      `_update_widget_texts` dans dmd_converter.py). Les 39 clés i18n cibles
#      correspondantes (2 clés "reset"/"refresh" n'existaient déjà plus) retirées
#      des 3 `lang_*.json` en parallèle (182 → 143 clés chacun) — vérifié qu'aucune
#      n'est appelée directement via `lang_manager.get("clé")` ailleurs dans le
#      code avant suppression. Vérifié en réel : compile check + JSON valide (143
#      clés × 3) + test headless (les 2 corrections se retraduisent bien dans les
#      3 langues) + smoke test complet (6 onglets, cycle des 3 langues, cycle des
#      2 thèmes, aucune exception).
# v4 — 2026-07-17 — safe-modify — Clé "Récursif": "recursive" retirée de TEXT_MAP :
#      la case à cocher correspondante a été supprimée de dmd_converter.py (v19,
#      scan de dossier désormais toujours récursif), la clé i18n devient orpheline.
# v3 — 2026-07-17 — safe-modify — TEXT_MAP : clé "Forcer pixel-perfect" renommée en
#      "Mode DMD / Forcer pixel-perfect" pour rester synchronisée avec le libellé
#      réel du widget (dmd_converter.py), sinon le mécanisme de retraduction au
#      changement de langue ne retrouve plus la clé i18n pixel_perfect.
# v2 — 2026-07-15 — safe-modify — TEXT_MAP : clé stale "DMD Converter v2.0" mise à jour
#      vers "DMD Converter v2.7.4" (version actuelle, dmd_converter.py APP_VERSION).
#      Cette clé désynchronisée empêchait la ré-application de la traduction du header
#      applicatif au changement de langue. ⚠️ Cette clé redeviendra stale au prochain
#      bump de version — voir TODO_optimisation.md pour la limite architecturale.
# v1 — 2026-07-15 — safe-modify — Version de base (backup original conservé dans
#      _backups/dmd_ui_constants_2026-07-15_20-10-27.bak)
# ============================================

from typing import Dict, List

WINDOWS_FONTS: List[str] = [
    "Agency FB",
    "Algerian",
    "AniMe Vision - MB_EN",
    "Arabic Transparent",
    "Arial",
    "Arial Baltic",
    "Arial Black",
    "Arial CE",
    "Arial CYR",
    "Arial Greek",
    "Arial Narrow",
    "Arial Rounded MT Bold",
    "Arial TUR",
    "BadaBoom BB",
    "Bahnschrift",
    "Bahnschrift Condensed",
    "Bahnschrift Light",
    "Bahnschrift Light Condensed",
    "Bahnschrift Light SemiCondensed",
    "Bahnschrift SemiBold",
    "Bahnschrift SemiBold Condensed",
    "Bahnschrift SemiBold SemiConden",
    "Bahnschrift SemiCondensed",
    "Bahnschrift SemiLight",
    "Bahnschrift SemiLight Condensed",
    "Bahnschrift SemiLight SemiConde",
    "Baskerville Old Face",
    "Bauhaus 93",
    "Bell MT",
    "Berlin Sans FB",
    "Berlin Sans FB Demi",
    "Bernard MT Condensed",
    "Blackadder ITC",
    "Bodoni MT",
    "Bodoni MT Black",
    "Bodoni MT Condensed",
    "Bodoni MT Poster Compressed",
    "Book Antiqua",
    "Bookman Old Style",
    "Bookshelf Symbol 7",
    "Bradley Hand ITC",
    "Britannic Bold",
    "Broadway",
    "Brush Script MT",
    "Calibri",
    "Calibri Light",
    "Californian FB",
    "Calisto MT",
    "Cambria",
    "Cambria Math",
    "Candara",
    "Candara Light",
    "Cascadia Code",
    "Cascadia Code ExtraLight",
    "Cascadia Code Light",
    "Cascadia Code SemiBold",
    "Cascadia Code SemiLight",
    "Cascadia Mono",
    "Cascadia Mono ExtraLight",
    "Cascadia Mono Light",
    "Cascadia Mono SemiBold",
    "Cascadia Mono SemiLight",
    "Castellar",
    "Centaur",
    "Century",
    "Century Gothic",
    "Century Schoolbook",
    "Chiller",
    "Colonna MT",
    "Comic Sans MS",
    "Consolas",
    "Constantia",
    "Cooper Black",
    "Copperplate Gothic Bold",
    "Copperplate Gothic Light",
    "Corbel",
    "Corbel Light",
    "Courier",
    "Courier New",
    "Courier New Baltic",
    "Courier New CE",
    "Courier New CYR",
    "Courier New Greek",
    "Courier New TUR",
    "Curlz MT",
    "DIN Alternate",
    "DIN Condensed",
    "Dubai",
    "Dubai Light",
    "Dubai Medium",
    "Ebrima",
    "Edwardian Script ITC",
    "Elephant",
    "Engravers MT",
    "Eras Bold ITC",
    "Eras Demi ITC",
    "Eras Light ITC",
    "Eras Medium ITC",
    "Felix Titling",
    "Fixedsys",
    "Footlight MT Light",
    "Forte",
    "Franklin Gothic Book",
    "Franklin Gothic Demi",
    "Franklin Gothic Demi Cond",
    "Franklin Gothic Heavy",
    "Franklin Gothic Medium",
    "Franklin Gothic Medium Cond",
    "Freestyle Script",
    "French Script MT",
    "Gabriola",
    "Gadugi",
    "Garamond",
    "Georgia",
    "Gigi",
    "Gill Sans MT",
    "Gill Sans MT Condensed",
    "Gill Sans MT Ext Condensed Bold",
    "Gill Sans Ultra Bold",
    "Gill Sans Ultra Bold Condensed",
    "Gloucester MT Extra Condensed",
    "Goudy Old Style",
    "Goudy Stout",
    "Haettenschweiler",
    "Harlow Solid Italic",
    "HarmonyOS Sans SC",
    "Harrington",
    "High Tower Text",
    "Impact",
    "Imprint MT Shadow",
    "Informal Roman",
    "Ink Free",
    "Javanese Text",
    "Jokerman",
    "Juice ITC",
    "Karmatic Arcade",
    "Kristen ITC",
    "Kunstler Script",
    "Lato",
    "Lato Light",
    "Lato Semibold",
    "Leelawadee UI",
    "Leelawadee UI Semilight",
    "Lucida Bright",
    "Lucida Calligraphy",
    "Lucida Console",
    "Lucida Fax",
    "Lucida Handwriting",
    "Lucida Sans",
    "Lucida Sans Typewriter",
    "Lucida Sans Unicode",
    "MS Gothic",
    "MS Outlook",
    "MS PGothic",
    "MS Reference Sans Serif",
    "MS Reference Specialty",
    "MS Sans Serif",
    "MS Serif",
    "MS UI Gothic",
    "MT Extra",
    "MV Boli",
    "Magneto",
    "Maiandra GD",
    "Malgun Gothic",
    "Malgun Gothic Semilight",
    "Marlett",
    "Matura MT Script Capitals",
    "Microsoft Himalaya",
    "Microsoft JhengHei",
    "Microsoft JhengHei Light",
    "Microsoft JhengHei UI",
    "Microsoft JhengHei UI Light",
    "Microsoft New Tai Lue",
    "Microsoft PhagsPa",
    "Microsoft Sans Serif",
    "Microsoft Tai Le",
    "Microsoft YaHei",
    "Microsoft YaHei Light",
    "Microsoft YaHei UI",
    "Microsoft YaHei UI Light",
    "Microsoft Yi Baiti",
    "MingLiU-ExtB",
    "MingLiU_HKSCS-ExtB",
    "MingLiU_MSCS-ExtB",
    "Mistral",
    "Modern",
    "Modern No. 20",
    "Mongolian Baiti",
    "Monotype Corsiva",
    "Myanmar Text",
    "NSimSun",
    "NanumGothic",
    "Niagara Engraved",
    "Niagara Solid",
    "Nirmala Text",
    "Nirmala Text Semilight",
    "Nirmala UI",
    "Nirmala UI Semilight",
    "Noto Sans JP",
    "OCR A Extended",
    "Old English Text MT",
    "Onyx",
    "PMingLiU-ExtB",
    "Palace Script MT",
    "Palatino Linotype",
    "Papyrus",
    "Parchment",
    "Perpetua",
    "Perpetua Titling MT",
    "Playbill",
    "Poor Richard",
    "Pristina",
    "ROG Fonts",
    "ROG Fonts STRIX SCAR",
    "ROG Fonts v1.6",
    "Rage Italic",
    "Ravie",
    "Roboto",
    "Rockwell",
    "Rockwell Condensed",
    "Rockwell Extra Bold",
    "Roman",
    "Sans Serif Collection",
    "Script",
    "Script MT Bold",
    "Segoe Fluent Icons",
    "Segoe MDL2 Assets",
    "Segoe Print",
    "Segoe Script",
    "Segoe UI",
    "Segoe UI Black",
    "Segoe UI Emoji",
    "Segoe UI Historic",
    "Segoe UI Light",
    "Segoe UI Semibold",
    "Segoe UI Semilight",
    "Segoe UI Symbol",
    "Segoe UI Variable Display",
    "Segoe UI Variable Display Light",
    "Segoe UI Variable Display Semib",
    "Segoe UI Variable Display Semil",
    "Segoe UI Variable Small",
    "Segoe UI Variable Small Light",
    "Segoe UI Variable Small Semibol",
    "Segoe UI Variable Small Semilig",
    "Segoe UI Variable Text",
    "Segoe UI Variable Text Light",
    "Segoe UI Variable Text Semibold",
    "Segoe UI Variable Text Semiligh",
    "Shaka Pow",
    "Showcard Gothic",
    "SimSun",
    "SimSun-ExtB",
    "SimSun-ExtG",
    "Sitka Banner",
    "Sitka Banner Semibold",
    "Sitka Display",
    "Sitka Display Semibold",
    "Sitka Heading",
    "Sitka Heading Semibold",
    "Sitka Small",
    "Sitka Small Semibold",
    "Sitka Subheading",
    "Sitka Subheading Semibold",
    "Sitka Text",
    "Sitka Text Semibold",
    "Small Fonts",
    "Snap ITC",
    "Source Han Sans JP",
    "Source Han Sans JP Normal",
    "Stencil",
    "Sylfaen",
    "Symbol",
    "System",
    "Tahoma",
    "Tempus Sans ITC",
    "Terminal",
    "Times New Roman",
    "Times New Roman Baltic",
    "Times New Roman CE",
    "Times New Roman CYR",
    "Times New Roman Greek",
    "Times New Roman TUR",
    "Trebuchet MS",
    "Tw Cen MT",
    "Tw Cen MT Condensed",
    "Tw Cen MT Condensed Extra Bold",
    "Verdana",
    "Viner Hand ITC",
    "Vivaldi",
    "Vladimir Script",
    "Webdings",
    "Wide Latin",
    "Wingdings",
    "Wingdings 2",
    "Wingdings 3",
    "Xolonium",
    "Yu Gothic",
    "Yu Gothic Light",
    "Yu Gothic Medium",
    "Yu Gothic UI",
    "Yu Gothic UI Light",
    "Yu Gothic UI Semibold",
    "Yu Gothic UI Semilight",
]

TEXT_MAP: Dict[str, str] = {
    # Boutons principaux
    "🚀 Traiter tout": "process_all",
    "✅ Traiter sélection": "process_sel",
    "📂 Charger": "load",
    "✂️ Crop 128×32": "crop",
    "DMD Creator v3.0.0": "app_header",
    "💾 Exporter GIF": "export_gif",
    "📚 Multi-images": "multi_images",
    "🎬 Morphing": "morphing",
    "🎬 Générer": "generate_preview",
    "📹 Charger Vidéo": "video_load",
    "Lecture": "video_playback_label",
    "🔄 Recentrer": "video_roi_reset",
    "➕ Point ici": "video_roi_add_point",
    "🔍 Zoom ici": "video_roi_add_zoom_point",
    "🗑️ Supprimer ce point": "video_roi_delete_point",
    "🗑️ Effacer points": "video_roi_clear_keyframes",
    "🎬 Générer Aperçu": "video_generate_preview",
    "🎬 Prévisualiser": "preview_btn",
    "🗑️ Effacer": "clear",
    "🔒 Verrouiller pour le batch": "lock_batch",
    "🔓 Réautoriser": "reauthorize",
    # Labels et sections
    "Images": "images",
    "Paramètres Globaux": "global_params",
    "FPS:": "fps",
    "Durée (s):": "duration",
    "Vitesse scroll:": "scroll_speed",
    "Contraste:": "contrast",
    "Saturation:": "saturation",
    "Couleurs GIF:": "gif_colors",
    "Informations Image": "image_info",
    "Image Originale": "original_image",
    "Aperçu DMD Principal (128x32)": "dmd_preview",
    "Propositions IA": "ia_proposals",
    "Effets Temps Réel": "realtime_effects",
    "Luminosité:": "brightness",
    "Netteté:": "sharpness",
    "Filtres": "filters",
    "Outils Dessin": "draw_tools",
    "Couleur": "color",
    "Tolérance:": "tolerance",
    "Édition": "edition",
    "Aperçu Animation DMD": "anim_preview",
    "Sélection (trim)": "video_trim_label",
    "Zone d'intérêt (cadrage vidéo)": "video_roi_label",
    "Aperçu du cadrage": "video_crop_preview_label",
    "📜 Historique de cadrage": "video_roi_history_label",
    "ℹ️ Vidéo Source": "video_source_info_label",
    "ℹ️ GIF à exporter": "video_gif_info_label",
    "Zoom cadrage:": "video_crop_zoom_label",
    "(-100% = resize seul, 0 = crop serré, +100% = zoom)": "video_crop_zoom_hint",
    "Mode de cadrage :": "video_crop_mode_label",
    "🎯 Suivi automatique": "video_crop_mode_tracking",
    "🪄 Cadrage auto (zoom)": "video_crop_mode_auto_zoom",
    "✋ Manuel": "video_crop_mode_manual",
    "Paramètres GIF Vidéo": "video_gif_params",
    "🪄 Qualité automatique": "video_quality_auto",
    "Aperçu Animation (Vidéo)": "video_preview_label",
    "Poids GIF estimé :": "video_gif_size_label",
    "Animation:": "animation",
    "Direction:": "direction",
    "Vitesse:": "speed",
    "Boucle:": "loop",
    "Répétitions:": "repetitions",
    "Texte": "text",
    "Police": "font",
    "Famille:": "family",
    "Taille:": "size",
    "Gras": "bold",
    "Italique": "italic",
    "Couleur texte": "text_color",
    "Couleur fond": "bg_color",
    "Effets Texte": "text_effects",
    "Effet:": "effect",
    "Effet couleur:": "color_effect",
    "Type:": "type",
    "Apparence": "appearance",
    "Thème:": "theme",
    "🌙 Sombre": "dark",
    "☀️ Clair": "light",
    "Comportement": "behavior",
    "Ajouter type d'animation au nom de fichier": "add_anim_name",
    "Mode DMD / Forcer pixel-perfect": "pixel_perfect",
    "Quitter": "quit",
    "⛔ Interrompre": "stop",
    "↶ Annuler": "undo",
    "↷ Rétablir": "redo",
    "Auto-ajuster": "auto_adjust",
    "Aperçu Animation": "preview_animation",
    "Entrez du texte et générez": "enter_text_generate",
    "ℹ️ Informations Image": "info_image",
    "Export": "export",
    "Qualité par défaut:": "default_quality",
    "Performance": "performance",
    "Activer cache IA": "enable_cache",
    "🗑️ Vider cache": "clear_cache",
    "Logs": "logs",
    "Sauvegarder logs automatiquement": "auto_save_logs",
    "📄 Exporter logs": "export_logs",
    "🗑️ Effacer logs": "clear_logs",
    "Auto-scroll": "autoscroll",
    "Filtrer:": "filter",
    "Prêt": "ready",
    "Fond noir": "black_bg",
    "Images chargées (morphing)": "loaded_images",
    "Votre texte ici...": "your_text_here",
    # Filtres
    "Flou": "blur",
    "Flou Gaussien": "gaussian_blur",
    "Contours": "edges",
    "Relief": "emboss",
    "Détails+": "detail",
    "Inverser": "invert",
    "Miroir H": "mirror_h",
    "Miroir V": "mirror_v",
    "Rotation 90°": "rotate_90",
    "N&B": "grayscale",
    "Posteriser": "posterize",
    "Solariser": "solarize",
    "Égaliser": "equalize",
    "Auto-contraste": "autocontrast",
    # Animations
    # Directions
    # Boucles
    # Outils dessin
    "🎨 Remplissage": "fill",
    "🧹 Gomme Magique": "eraser",
    "🎨 Remplissage (ACTIF)": "fill_active",
    "🧹 Gomme Magique (ACTIF)": "eraser_active",
    # Contrôles avancés
    "⚙️ Contrôles Avancés": "advanced_controls",
    "Easing:": "easing",
    "Opacité:": "opacity",
    "Délai début (s):": "delay_start",
    "Inverser direction": "reverse_direction",
    "Rebond aux bords": "bounce_edges",
    "Animations & Paramètres": "animations_params",
    # Audit i18n complet (v13→v14) : widgets construits avec un texte
    # littéral jamais enregistré ici ni dans lang_fr.json — restaient donc
    # figés en français quelle que soit la langue sélectionnée, quel que
    # soit le mécanisme (ni TEXT_MAP, ni le repli reverse_map de
    # _update_widget_texts qui scanne les VALEURS déjà chargées dans
    # lang_manager.translations, absent ici puisque ces textes n'y étaient
    # simplement jamais entrés).
    "Seuil lettrage (px):": "letter_threshold_label",
    "🗑 Vider": "clear_image_list",
    "📂 Glisser-déposer\nun dossier ou des images ici": "image_drop_hint",
    "🔄 Nouvelle proposition": "regenerate_proposition",
    "Animation": "animation_frame_label",
}
