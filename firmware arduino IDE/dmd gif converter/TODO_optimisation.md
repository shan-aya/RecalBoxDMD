# TODO — Optimisation / Debug / Correction — DMD GIF Creator

Checklist issue de l'analyse complète du programme (2026-07-15). Objectif : ne rien oublier
lors des prochaines passes d'optimisation. Cocher au fur et à mesure. Toute modification doit
suivre le protocole `safe-modify` (backup + en-tête de version) avant d'éditer un fichier.

## 🧹 Code mort

- [x] **Fait (2026-07-15)** — `dmd_converter.py` : suppression de `render_dmd_frame`
      (ex-lignes ~2393-2438) et `render_text_image` (ex-lignes ~3257-3323), définis dans la
      classe `DMDConverter` mais totalement shadowés par les monkey-patchs de fin de fichier
      (`DMDConverter.render_dmd_frame = _pipeline_render_dmd_frame` et
      `DMDConverter.render_text_image = _pipeline_render_text_image`) → jamais exécutés.
      Vérifié : `py_compile` OK + `import dmd_converter` OK après suppression.

## 🐌 Performance — boucles pixel-par-pixel à vectoriser (numpy)

Par ordre de priorité estimée (le plus coûteux en premier) :

- [x] **Fait (2026-07-15)** — `dmd_text_animations.py` (`starwars_scroll`,
      `bounce_scroll`, `scroll_wave`) — vectorisation numpy complète des 3 fonctions,
      suppression du bug `np.array(text_img)` recréé à chaque pixel. Rendu vérifié
      **strictement identique** (comparaison pixel par pixel ancien/nouveau code) via
      `_backups/dmd_text_animations_2026-07-15_19-14-22.bak`. Gains mesurés :
      `scroll_wave` ×7.7, `starwars_scroll` ×154, `bounce_scroll` ×458.
- [x] **Fait (2026-07-15)** — `dmd_manual_effects.py` (`wave_effect`) — double boucle
      pixel par pixel remplacée par indexation numpy vectorisée. Rendu vérifié
      **strictement identique** via `_backups/dmd_manual_effects_2026-07-15_19-18-05.bak`.
      Gain mesuré : ×12.8.
- [~] **Analysé (2026-07-15), non modifié** — `dmd_text_effects.py:95-113` (`effect_neon`) :
      mesuré à ~18ms/appel (les autres effets texte statiques : `effect_metal` 0.28ms,
      `effect_gradient` 0.20ms, `effect_ice` 2.6ms, `effect_graffiti` 5.3ms, `effect_3d`
      0.4ms, `effect_fire` 2.2ms — négligeables). Contrairement à `wave_effect`/
      `scroll_wave`, ces effets s'exécutent **une seule fois par rendu de texte** (pas par
      frame d'animation), donc l'impact cumulé est bien plus faible. Une réécriture
      vectorisée fidèle est risquée : ~175 appels `draw.text()` qui se chevauchent avec
      anti-aliasing PIL, pas d'accès public fiable à l'image sous-jacente depuis l'objet
      `draw` pour reproduire le rendu en numpy sans risque de dérive visuelle. Laissé tel
      quel — gain potentiel (~15ms, ponctuel) jugé insuffisant pour le risque.
- [ ] `dmd_text_effects.py:68-77` (`effect_ice`) et `:116-137` (`effect_graffiti`) — texte
      dessiné 8 à 24 fois avec offsets (approche par répétition plutôt que masque). Mesuré
      à 2.6ms et 5.3ms/appel (appel unique par rendu, pas par frame) — impact réel faible,
      priorité basse.
- [x] **Fait (2026-07-15)** — `dmd_converter.py` — `flood_fill` : BFS avec `queue.pop(0)`
      remplacé par un flood fill vectorisé numpy (masque candidat + dilatation itérative).
      **`magic_eraser`** : double boucle Python remplacée par un diff couleur vectorisé
      numpy. **Bug découvert et corrigé au passage** (confirmé avec l'utilisateur avant
      correction) : le calcul de diff couleur original (`sum(abs(a-b) for a,b in
      zip(tuple(arr[y,x]), target_color))`) opérait sur des `numpy.uint8` qui "bouclent"
      en cas de soustraction négative (ex. `50-200` → `106` au lieu de `-150`), faussant
      silencieusement la comparaison à la tolérance dans les deux fonctions. La nouvelle
      version calcule le diff en entiers corrects. Vérifié : le résultat vectorisé
      correspond exactement à une référence Python en `int` (algorithme correct, sans le
      bug), sur 5 cas de test (tailles/tolérances variées) — voir
      `_backups/dmd_converter_2026-07-15_19-23-14.bak` pour l'ancien comportement (buggé).
      Gains mesurés : `flood_fill` ×4 à ×6, `magic_eraser` ×60 à ×80.
      ⚠️ Comportement utilisateur légèrement modifié : pour une même tolérance réglée sur
      le curseur, le nombre de pixels remplis/effacés peut différer de l'ancienne version
      buguée (désormais plus cohérent avec ce que règle l'utilisateur).
- [x] **Fait (2026-07-15)** — `dmd_converter.py` — `generate_morphing_animation` :
      dimensions cible (scale/new_w/new_h/x/y) hissées hors des boucles (constantes pour
      toute la fonction) ; la frame de pause finale n'est plus redimensionnée/collée
      qu'une seule fois au lieu de `int(fps*0.5)` fois à l'identique (frames partagées,
      même pattern que `dmd_engine.py` mode "static"). Vérifié : rendu strictement
      identique sur 2 cas de test. Gain modeste (×1.3-2.5, la boucle d'interpolation
      elle-même reste incompressible car chaque frame a un contenu différent).

## 🔁 Redondance de calcul (pas de boucle pixel, mais travail dupliqué)

- [x] **Fait (2026-07-15)** — `dmd_converter.py` — `process_images` : suppression du
      second appel redondant à `DMDEngine.create_animation_frames` sur l'image brute
      re-ouverte. **Bug découvert et corrigé** : ce recalcul utilisait les dimensions de
      l'image **avant** redimensionnement DMD au lieu de celles réellement utilisées pour
      le rendu, donnant parfois un nom de fichier `_horizontal`/`_vertical` incorrect
      (confirmé sur un cas de test : image 800×100 en mode `fit`/`auto` → l'ancien code
      aurait nommé le fichier `_horizontal.gif` alors que le rendu réel est statique).
      Fix : `render_dmd_frame` (`dmd_pipeline_quality.py`) expose maintenant la direction
      réellement calculée via un paramètre optionnel `return_direction=True`
      (rétrocompatible — vérifié que les 3 autres appels de `render_dmd_frame` dans
      `dmd_converter.py`, qui ne passent pas ce paramètre, continuent de recevoir le
      2-tuple d'origine sans changement). Ne s'applique concrètement que lorsque
      `add_anim_to_name` est activé et `direction="auto"`.
      Voir `_backups/dmd_pipeline_quality_2026-07-15_19-38-42.bak` et
      `_backups/dmd_converter_2026-07-15_19-38-42.bak`.
- [x] **Analysé et partiellement traité (2026-07-15)** — `dmd_pipeline_quality.py`
      `evaluate_quality` : en creusant, `dmd_frames`/`ref_frames` génèrent bien un jeu
      complet de frames d'animation chacun, mais **seul `[0]` de chaque est utilisé** —
      un échantillonnage mort de jusqu'à 4 frames (`dmd_sample`) était calculé sans
      jamais servir (`dmd_sample[0]` valant toujours `dmd_frames[0]`), supprimé (code
      mort, zéro changement de comportement, vérifié sur 4 cas de test dont
      `horizontal`/`fill` — scores strictement identiques).
      La régénération complète des DEUX jeux de frames (candidat optimisé vs référence
      brute) reste nécessaire telle quelle : il ne s'agit pas de la même computation
      répétée deux fois, mais de deux pipelines différents (candidat optimisé vs
      référence non optimisée) dont on ne garde que la 1ère frame — éviter cette
      régénération demanderait de donner à `create_animation_frames` un mode "une seule
      frame" (changement de l'API partagée, plus risqué) ou de dupliquer sa logique
      localement (duplication fragile) ; non fait ici, gain incertain par rapport au
      risque pour une fonction déjà appelée sur des images 128×32 (`cleanup=False`).
- [~] **Non traité — mis en pause (2026-07-15)** — `dmd_converter.py` —
      `auto_analyze_and_preview` (threading) et par extension tout le sous-système
      Auto/IA (`generate_settings_variants`, `evaluate_quality`). L'utilisateur a indiqué
      que la partie auto-analyse/scoring ne le satisfait pas dans son état actuel et
      nécessite une **refonte complète**, pas une optimisation incrémentale. Pas de sens
      à threader/optimiser du code qui va être réécrit — à reprendre uniquement dans le
      cadre de cette refonte, une fois le nouveau design du scoring défini avec
      l'utilisateur.

## 🧬 Duplication de code à factoriser

- [x] **Fait (2026-07-15)** — `dmd_pipeline_quality.py` — noyau de convolution Sobel
      extrait dans une fonction module-level `_sobel_magnitude`, réutilisée par
      `sobel_mag` et `active_edge_strength`. Vérifié : scores `evaluate_quality`
      strictement identiques avant/après (mode avancé + legacy, 3 cas de test).
- [x] **Fait (2026-07-15)** — `dmd_manual_effects.py` — bloc "canvas noir 128×32 +
      centrage" factorisé dans un helper `_centered_canvas(content, bg_color,
      extra_offset)`, réutilisé par 12 méthodes (`fade_effect`, `zoom_effect`,
      `rotate_effect`, `wave_effect`, `bounce_effect`, `flash_effect`, `spiral_effect`,
      `shake_effect`, `pulse_effect`, `glitch_effect`, `pixelate_effect`,
      `blur_transition_effect`, `color_shift_effect`). `slide_effect` non touché
      (positions custom, pas du centrage pur). Vérifié : rendu strictement identique sur
      12/13 effets (seed fixée pour les effets aléatoires).
      ⚠️ Bug préexistant découvert au passage (confirmé présent à l'identique dans
      l'ancien ET le nouveau code, donc pas une régression de cette factorisation) :
      `color_shift_effect` plantait avec `OverflowError` sur numpy récent — corrigé dans
      la foulée, voir section Anomalies.
- [x] **Fait (2026-07-15)** — `dmd_engine.py` (`create_animation_frames`) — branches
      horizontale/verticale factorisées dans un helper `_scroll_axis_frames`. Vérifié :
      rendu strictement identique sur 6 cas de test (horizontal/vertical/static/auto,
      vitesses et cleanup variés).

## 🗂️ Imports dupliqués / mal placés

- [x] **Fait (2026-07-15)** — `dmd_converter.py` — `dmd_text_effects`/`dmd_text_animations`
      étaient importés deux fois ("Rebind final"). Vérifié qu'aucune classe
      `TextEffects`/`TextAnimations` n'est redéfinie entre les deux blocs (grep sur tout
      le fichier) → le second bloc était bien mort, supprimé.
- [x] **Fait (2026-07-15)** — `dmd_converter.py` — `dmd_gif_exporter` était importé
      localement à 3 endroits (`process_images`, `manual_export`, `export_text_gif`) →
      consolidé en un seul import en tête de fichier. Vérifié : import du module complet
      OK, `export_frames_to_gif` disponible au niveau module.

## 🐛 Anomalies / incohérences mineures

- [x] **Découvert et corrigé (2026-07-15)** — `dmd_manual_effects.py`
      (`color_shift_effect`) : plantait avec `OverflowError: Python integer 256 out of
      bounds for uint8` sur la version actuelle de numpy (`h_arr = (h_arr + int(angle)) %
      256` où `h_arr` est un tableau `uint8` — les versions récentes de numpy refusent ce
      cast implicite), rendant l'effet totalement inutilisable. Fix : cast en `int` avant
      l'addition/modulo. Vérifié : génère maintenant les frames sans erreur.

- [x] **Fait (2026-07-15), plus profond que prévu** — le header applicatif ne se
      retraduisait en réalité **jamais** au changement de langue, pour 3 raisons
      cumulées découvertes en creusant ce point :
      1. `dmd_converter.py` (`setup_ui`) : la regex de normalisation de version était
         **double-échappée** (`r"v\\d+\\.\\d+..."`) donc ne matchait jamais rien (no-op
         silencieux) — corrigée avec le pattern simple-échappement déjà utilisé et
         fonctionnel dans `update_title`/`apply_version`.
      2. `dmd_ui_constants.py` (`TEXT_MAP`) : clé stale `"DMD Converter v2.0"` alors que
         `APP_VERSION = "2.7.4"` → mise à jour vers `"DMD Converter v2.7.4"`.
      3. `lang_fr.json`/`lang_en.json`/`lang_es.json` : `app_header` codait en dur
         `"v2.7.3"` (et `app_title` aussi, bien que ce dernier soit corrigé au runtime
         par un regex fonctionnel ailleurs) → resynchronisés sur `v2.7.4`.
      Vérifié : le texte généré au runtime correspond maintenant exactement à la clé
      `TEXT_MAP` et à la valeur `app_header` des 3 langues.
      ⚠️ Limite architecturale non résolue : ce mécanisme de correspondance texte-exact
      redeviendra stale à chaque futur bump de `APP_VERSION` si les 3 lang_*.json et
      `TEXT_MAP` ne sont pas mis à jour en même temps. Une vraie correction demanderait
      de faire lire le header via `lang_manager.get("app_header")` + `apply_version()`
      (comme le titre de fenêtre) plutôt que par correspondance de texte exact — non
      fait ici (changement plus large, hors scope de ce nettoyage).
- [x] **Fait (2026-07-15)** — `lang_es.json` — clé `image_info` manquante ajoutée
      (`"Información de la Imagen"`). Les 3 fichiers de langue ont maintenant 184 clés
      chacun (vérifié par parsing JSON).
- [x] **Fait (2026-07-15)** — `lang_fr.json` (clé `reauthorize` dupliquée) et
      `lang_es.json` (clé `lock_batch` dupliquée) — doublons supprimés (valeurs
      identiques dans les deux cas, donc aucun changement de traduction). Vérifié :
      184 clés uniques dans les 3 fichiers, JSON valide, import de l'app OK.
- [x] **Fait (2026-07-15)** — `dmd_converter.py` — `print("DEBUG: process_images
      démarré")` remplacé par `logger.info(...)`, cohérent avec le reste du logging de
      l'app (visible dans l'onglet DEBUG / export de logs).
- [x] **Fait (2026-07-15)** — `dmd_converter.py` — bloc `if __name__ == "__main__":` :
      une erreur fatale écrit maintenant un fichier `crash_YYYYMMDD_HHMMSS.log` (traceback
      complet) dans le dossier de config utilisateur, en plus du print console existant.
      Vérifié en conditions réelles (exception simulée) : fichier créé avec la trace
      complète, lisible.
- [x] **Fait (2026-07-15)** — `dmd_manual_effects.py` — `flash_effect` : `flash_interval`
      plafonné à un minimum de 1. Avant le fix, un `flashes` élevé par rapport à
      `num_frames` (ex. duration=1.0, fps=10, flashes=10) faisait dégénérer l'effet en
      écran blanc fixe pendant toute l'animation (confirmé : 10 frames, 0 transition,
      100% blanc) — l'image n'était plus jamais visible. Vérifié : cas normaux
      strictement identiques (3 cas de test) ; cas dégénéré corrigé (alterne
      correctement, 9 transitions sur 10 frames).
- [x] **Fait (2026-07-18)** — `dmd_ui_constants.py` (`TEXT_MAP`) audité entièrement
      (149 entrées) contre le code réel. 2 bugs trouvés et corrigés (libellé
      renommé dans le code, jamais mis à jour dans `TEXT_MAP`/les 3 `lang_*.json`
      → retraduction cassée dans toutes les langues) : bouton "🔓 Réautoriser"
      (avait encore "sélection" en trop) et cadre "⚙️ Contrôles Avancés" (emoji
      manquant). 46 entrées mortes retirées de `TEXT_MAP` + 39 clés i18n retirées
      des 3 `lang_*.json` (182 → 143 clés chacun) — détail complet dans le
      changelog v5 de `dmd_ui_constants.py`.
- [x] **Repéré en documentant l'app (2026-07-18), implémenté le jour même (v33)** —
      `dmd_converter.py`, onglet MANUEL, cadre "⚙️ Contrôles Avancés"
      (`setup_manual_tab`) — 5 widgets créés et lisibles à l'écran mais **jamais
      lus** par `generate_manual_animation` (`manual_easing`, `manual_delay_start`,
      `manual_reverse`, `manual_bounce_edges`, `manual_opacity`). Utilisateur
      consulté (implémenter vs retirer de l'UI) → a choisi d'implémenter les 5.
      Réalisé de façon générique (post-traitement sur la liste de frames déjà
      générée, valable pour les 18 types d'animation) : `_apply_easing`
      (ré-échantillonnage de l'ordre de lecture selon une courbe, bounceOut de
      Robert Penner pour "bounce"), `_apply_bounce_edges` (repli aller-retour +
      ré-échantillonnage, même nombre de frames), inversion simple pour
      `manual_reverse`, frames statiques ajoutées une fois en tête pour le délai,
      fondu vers le noir en tout dernier pour l'opacité. Vérifié en réel (headless,
      chaque contrôle isolé contre une baseline).

## 📌 Notes de contexte

- Toutes les références ligne ci-dessus sont celles **avant** la suppression du code mort
  (2026-07-15) ; les numéros de ligne dans `dmd_converter.py` ont changé (~-125 lignes après
  les deux suppressions). Se fier aux noms de fonctions/méthodes pour les retrouver.
- Le fichier est en cours de "démonolithisation" progressive (commentaires `# --- Modular ...
  (refactor étape N) ---`) — toute intervention doit vérifier si un monkey-patch de fin de
  fichier ne redirige pas déjà la méthode concernée, comme c'était le cas pour le code mort
  déjà nettoyé.
- Backups safe-modify de `dmd_converter.py` disponibles dans `_backups/` (voir
  `_backups/_index.md`) avant toute intervention supplémentaire.
