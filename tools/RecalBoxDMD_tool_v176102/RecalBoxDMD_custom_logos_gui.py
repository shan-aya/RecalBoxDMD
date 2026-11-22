"""Onglet « Personnalisation » du toolkit RecalBoxDMD (2026-10-28) : remplacer le logo d'un jeu par sa propre image.

Interface seule : toute la logique (cles, conversion, controles, store JSON, application sur la SD, cache) est dans custom_logos.py (testee sans Tk).
Trois pieces :
- CustomLogosTab : l'onglet. Liste des jeux d'un systeme (gamelist.xml du dossier ROMs, sinon les visuels presents sur la SD) avec leur etat
  (OK / sans logo / noir / personnalise), apercu LED « sur la SD » et « remplacant », Valider / Retirer / Appliquer.
- ImagePicker : fenetre de CHOIX de l'image (dossier + miniatures, collage depuis le presse-papiers, mode de conversion, fond, apercu en direct, controles).
- BrowseWindow : fenetre qui PARCOURT les logos existants d'un systeme en miniatures (au lieu de chercher / trier dans une liste) ; un clic selectionne le jeu
  dans l'onglet.
Construite par RetroBoxLEDGui._build_custom_tab() ; la langue suit gui.lang_var, les couleurs gui._theme_colors().
"""
import os
import re
import threading
import time
import tkinter as tk
from datetime import datetime
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import custom_logos as cl
import pi_logos as pil

T = {
    "fr": {
        "intro": "Repérez les jeux dont la SD affiche une carte texte (ou rien) et choisissez un remplaçant. Chaque remplacement est conservé et réappliqué après chaque copie vers la SD.",
        "src_kind_sd": "Carte SD", "src_kind_pi": "Raspberry Pi (Wi-Fi)", "lbl_sd": "Carte SD :", "lbl_pi_ip": "IP du Raspberry Pi :", "detect": "Détecter",
        "no_drive_found": "Aucune carte SD du DMD détectée : insérez-la ou choisissez son dossier avec « Parcourir… ».", "no_pi_connected": "Le Raspberry Pi n'est pas connecté : vérifiez qu'il est allumé et sur le même réseau, puis cliquez sur « Détecter ». Une nouvelle tentative de connexion vient d'être lancée.", "no_pi_found": "Aucun Raspberry Pi joignable : saisissez son adresse IP (démon 3.75 ou plus récent).",
        "bad_folder": "Ce dossier n'existe pas : {path}",
        "sd": "Carte SD / Pi :", "refresh": "Actualiser", "folder": "Dossier…", "roms": "Dossier ROMs (gamelist.xml) :", "browse": "Parcourir…", "load": "Charger",
        "system": "Système :", "filter": "Filtre :", "f_card": "Carte texte", "f_avail": "Logo dispo", "f_missing": "Sans visuel", "f_dark": "Noirs", "f_custom": "Perso", "f_ok": "Logo",
        "col_name": "Jeu", "col_state": "État",
        "st_ok": "OK", "st_anim": "animé", "st_card": "Carte texte", "st_avail": "logo dispo", "st_missing": "sans visuel", "st_dark": "NOIR ⚠", "st_custom": "perso",
        "cur": "Existant (sur la SD)", "view": "Afficher :", "v_all": "Tous les jeux", "v_todo": "À améliorer (pas de vrai logo)", "v_avail": "Logo dispo sur la Recalbox", "v_card": "Cartes texte", "v_missing": "Sans visuel", "v_dark": "Logos noirs", "v_custom": "Personnalisés par vous", "v_ok": "Logos d'origine", "ex_logo": "logo", "ex_card": "carte texte", "ex_none": "aucun visuel", "ex_dark": "logo noir", "ex_custom": "personnalisé", "ex_anim": "animation", "note_repl": "Remplaçant : {src}", "note_repl_none": "Remplaçant : aucun choisi (« Choisir un remplaçant… » ou « Carte texte »).", "note_repl_rb": "Remplaçant : logo de la Recalbox ({src}).", "note_repl_saved": "Remplaçant : {src} (enregistré).", "note_orig_kept": "Original : l'image d'avant toute personnalisation (conservée).", "repl": "Remplaçant", "none": "—", "led": "Luminosité LED",
        "pick": "Choisir un remplaçant…", "lbl_source": "Source :", "lbl_library": "Bibliothèque (Recalbox) :", "tools": "Outils ▾", "orig_tab": "Original", "clip_range": "Extrait : images {a} à {b} sur {n}", "clip_start": "Début ici", "clip_end": "Fin ici", "clip_all": "Tout le GIF", "clip_snap": "Capturer cette image", "clip_pos": "Position : image {p}", "clip_resample": "i {c} images (maximum {m}) : 1 image sur {k} est gardée, la durée totale est conservée. Pour tout garder, raccourcissez l'extrait.", "pk_up": "↑ Dossier parent", "pk_sub": "Inclure les sous-dossiers", "pk_suggest": "Nom du jeu", "pk_found": "{d} dossier(s), {i} image(s)", "pk_more": "Afficher la suite ({n} restantes)", "pk_cap": "Premiers {n} résultats : précisez la recherche.", "orig_same": "Original : identique à l'existant (jamais personnalisé).", "browse_logos": "Parcourir les logos…", "validate": "Valider", "remove": "Réinitialiser à l'original", "cancel_idle": "Annuler", "cancel_choice": "Annuler le choix", "cancel_prev": "Annuler : logo précédent", "cancel_orig": "Annuler : rendre l'original", "cancel_del": "Annuler : retirer la perso",
        "ask_twins": "{n} autre(s) ROM de ce système portent aussi le nom « {name} » et ont chacune leur propre logo (c'est celui du ROM LANCÉ que le DMD affiche) :\n\n{roms}\n\nAppliquer ce remplaçant aussi à ces ROM ?", "twins_done": "+ {n} autre(s) ROM de même nom.",
        "choice_cancelled": "Choix annulé : rien n'a été modifié.", "undone_prev": "Logo précédent rétabli : {key}",
        "confirm_prev": "Revenir au logo précédent de « {name} » ?\n\nLe remplaçant actuel est abandonné (un seul niveau d'annulation : ensuite, « Annuler » rendra l'original).", "apply_all": "Appliquer tout à la SD", "batch": "Importer un dossier…", "info_anim": "animation : {n} images, {s} s", "ask_anim": "Ce fichier contient une animation de {n} images.\n\nOui = garder l'animation (écrite en .raw565pack + .meta)\nNon = utiliser seulement la 1re image", "too_many": "Animation refusée : {err}", "card": "Carte texte", "cards": "Cartes : sans logo + noirs", "cards_ask": "Système « {sys} » : générer une carte texte (nom du jeu) pour {n} jeu(x) sans logo ou à logo noir ?\nLes originaux sont gardés : « Retirer la perso » les rend.", "cards_none": "Aucun jeu sans logo ni à logo noir dans ce système (la détection des noirs peut prendre quelques secondes après le chargement).", "cards_done": "Cartes texte : {n} écrite(s) sur la SD, {e} erreur(s).", "pk_export": "Exporter paquet…", "pk_import": "Importer paquet…", "pack_empty": "Aucune personnalisation à exporter.", "pack_exported": "Paquet exporté : {n} logo(s) → {path}", "pack_bad": "Paquet refusé : {err}", "pack_ask": "Paquet : {n} logo(s) valides, {bad} rejeté(s) (entrées invalides).\n{known} existent déjà dans vos personnalisations.\n\nOui = importer et remplacer les existants\nNon = importer seulement les nouveaux\nAnnuler = ne rien faire", "pack_done": "Paquet importé : {a} ajouté(s), {r} remplacé(s), {s} conservé(s).", "pack_types": "Paquet de logos",
        "no_sd": "Choisissez d'abord la carte SD (ou un dossier qui en tient lieu).", "no_sel": "Sélectionnez d'abord un jeu dans la liste.",
        "all_sys": "Tous les systèmes", "source_snap": "{n} jeux · liste du Mode 1 du {date}", "source_all": "{n} jeux · {s} systèmes",
        "snap_extra": "⚠ {n} hors liste (SD)", "av_bar": "Comparaison avec la Recalbox : {n} jeu(x) affiché(s) en carte texte, sans visuel ou en logo noir sur la SD ont un logo sur la Recalbox.", "av_cur_dark": "logo noir", "av_btn": "Voir les nouveaux logos…", "av_head": "NOUVEAUX LOGOS SUR LA RECALBOX", "av_hint": "cochez ceux à utiliser ; rien n'est modifié avant « Appliquer »", "av_all": "Tout cocher", "av_none": "Tout décocher", "av_sys": "Système :", "av_every": "Tous ({n} systèmes)", "av_col_game": "Jeu", "av_col_cur": "Sur la SD (actuel)", "av_col_new": "Sur la Recalbox (proposé)", "av_cur_card": "carte texte", "av_cur_none": "sans visuel", "av_apply": "Appliquer les {n} cochés", "av_count": "{n} coché(s) sur {t} — l'original de chaque jeu est conservé (Réinitialiser possible)", "av_close": "Fermer", "av_done": "{n} logo(s) de la Recalbox appliqué(s) ({e} erreur(s)).", "av_title": "Nouveaux logos sur la Recalbox", "av_note": "Seuls les jeux déjà présents sur la SD sont proposés.", "rw_title": "Choisir le remplaçant", "rw_head": "CHOISIR LE REMPLAÇANT", "rw_hint": "rien n'est modifié avant « Utiliser ce remplaçant », puis « Valider »", "rw_target": "CIBLE (ce qui sera remplacé)", "rw_now": "Actuellement : {state}", "rw_where": "Où chercher le remplaçant", "rw_s_roms": "Logos des ROMs (gamelist.xml)", "rw_s_sd": "Carte SD / Pi : logos des autres jeux", "rw_s_gifs": "Carte SD / Pi : dossier gifs", "rw_s_file": "Fichier, dossier, presse-papiers…", "rw_s_card": "Carte texte (titre au choix)", "rw_system": "Système :", "rw_found": "{n} résultat(s)", "rw_preview": "Aperçu du remplaçant (au survol)", "rw_mode": "Mode d'adaptation :", "rw_mode_note": "(images des ROMs seulement)", "rw_gif_note": "Astuce : pour retravailler un GIF (découpe, recadrage, vitesse), utilisez DMD GIF Creator (onglet GIF), puis choisissez ici le fichier obtenu.", "rw_anim_title": "GIF animé", "rw_anim_n": "{n} images", "rw_anim_all": "Tout le GIF (animation)", "rw_anim_one": "Une seule image (curseur) :", "rw_anim_pos": "Image {k} / {n}", "rw_use": "Utiliser ce remplaçant", "rw_hoverhint": "Survolez une image ; cliquez pour la choisir.", "rw_empty": "Rien à proposer ici.", "rw_na_roms": "Indiquez le dossier ROMs de la Recalbox (gamelist.xml) dans la fenêtre principale.", "rw_na_sd": "Aucune carte SD / Pi chargée.", "rw_na_gifs": "Le dossier gifs n'est lu que sur une carte SD (pas sur un Pi).", "rw_pi_loading": "Lecture du dossier gifs du Pi (Wi-Fi)…", "rw_pi_ko": "Dossier gifs du Pi illisible : {err}", "rw_file_text": "Choisir une image, un GIF ou un dossier sur le PC ou le réseau, ou coller depuis le presse-papiers. Le sélecteur propose les modes d'adaptation.", "rw_file_btn": "Ouvrir le sélecteur…", "rw_card_title": "Titre de la carte :", "rw_err": "Remplaçant illisible : {err}", "snap_cmp": "Bibliothèque Recalbox : {new} jeu(x) de plus et {gone} de moins depuis le scrape de la SD", "snap_cmp_ok": "Bibliothèque Recalbox = scrape de la SD", "cap": "{shown} premiers affichés sur {total} : filtrez pour affiner.", "snap_done": "Liste de référence créée pour {n} système(s).",
        "snap_ask": "{n} système(s) de la SD n'ont pas de liste de référence (SD scrapée avant cette fonction).\n\nLa créer depuis le dossier ROMs actuel ? Les images de la SD ne sont pas modifiées.\nLa liste reflète le gamelist.xml d'aujourd'hui, pas celui du scrape : l'état de chaque jeu reste calculé d'après les fichiers de la SD, et les écarts apparaissent comme « hors liste » ou « sans logo ».",
        "pick_system": "Choisissez d'abord un système précis (pas « Tous les systèmes ») pour importer un dossier.", "no_sd_states": "Aucune carte SD choisie : l'état des logos (sans logo, noir…) n'est pas connu.", "off_sd": "Ce système n'a pas de dossier sur la SD : lancez d'abord le Mode 1 pour le créer.", "roms_none": "Aucun gamelist.xml trouvé dans « {path} » ni dans ses sous-dossiers.",
        "source_gl": "{n} jeux (gamelist.xml)", "source_sd": "{n} jeux (visuels présents sur la SD ; indiquez le dossier ROMs pour voir aussi les jeux sans logo)",
        "stats": "{shown} affichés",
        "loading": "Chargement…", "step_roms": "Lecture du dossier de la bibliothèque Recalbox…", "step_pi": "Interrogation du Raspberry Pi : {done} / {total} systèmes ({pct} %)", "sd_bad_store": "Le fichier custom/logos.json est illisible :\n{err}\n\nIl n'est pas modifié. Corrigez-le ou renommez-le, puis actualisez.",
        "w_dark": "⚠ image presque noire sur la dalle", "w_transparent": "transparence aplatie sur le fond choisi", "w_upscaled": "image plus petite que la dalle (agrandie)", "w_narrow": "proportions très éloignées de 4:1",
        "info_item": "{mode} · fond {bg} · source {src}", "confirm_dark": "Cette image est presque noire sur la dalle : le logo risque d'être invisible.\n\nValider quand même ?",
        "collision": "Attention : une autre ROM donne le même nom de visuel ({key}) une fois les espaces retirés. Le logo sera partagé par les deux.",
        "saved": "Enregistré et écrit sur la SD : {key}", "removed_restored": "Original rétabli : {key}", "removed_deleted": "Personnalisation retirée (aucun original sauvegardé, le logo par défaut s'affichera) : {key}",
        "confirm_remove": "Retirer la personnalisation de « {name} » ?\n\n{what}", "what_restored": "L'original sauvegardé sera rétabli.", "what_deleted": "Aucun original n'a été sauvegardé : le fichier sera supprimé et le DMD affichera le logo par défaut.",
        "applied": "Appliqué : {w} réécrit(s), {u} déjà bon(s), {o} orphelin(s), {e} erreur(s).", "nothing": "Aucune personnalisation à appliquer.", "err": "Erreur : {err}",
        "pk_title": "Choisir l'image de remplacement — {name}", "pk_folder": "Dossier d'images :", "pk_paste": "Coller", "pk_none": "Aucune image dans ce dossier.",
        "pk_clip_none": "Le presse-papiers ne contient pas d'image.", "pk_use": "Utiliser cette image", "cancel": "Annuler", "pk_filter": "Filtre :",
        "m_pixel": "Pixel-perfect (net, fidèle)", "m_fit": "Ajuster (tout visible)", "m_fill": "Étirer", "m_crop": "Rogner (remplit la dalle)", "bg": "Fond :", "bg_pick": "Choisir…", "bg_black": "Noir", "mode": "Conversion :",
        "pk_pick_first": "Cliquez sur une image à gauche.", "pk_note": "Le fond sert aux zones transparentes et aux bandes des modes « Ajuster », « Étirer » et « Rogner ».",
        "batch_pick": "Dossier d'images nommées comme les jeux (pacman.png → Pac-Man)", "batch_empty": "Aucune image de ce dossier ne correspond à un jeu du système « {sys} » ({unk} sans jeu, {amb} ambiguës).", "batch_ask": "Système « {sys} » : {n} image(s) à importer (conversion pixel-perfect, fond noir).\n{amb} ambiguë(s) et {unk} sans jeu seront ignorées.\n\nImporter ?", "batch_done": "Import : {n} logo(s) écrits sur la SD, {e} erreur(s), {d} presque noir(s).", "batch_dark": "À vérifier (presque noirs) : {keys}",
        "br_title": "Parcourir les logos existants", "br_systems": "Systèmes", "br_more": "Afficher la suite", "br_none": "Aucun jeu.",
        "send_pi": "Envoyer au Pi (Wi-Fi)", "restore_sd": "Remettre en état mes logos sur la carte (après scrape / maj / Mode 1)", "restore_pi": "Remettre en état mes logos sur le Pi (renvoi Wi-Fi)", "backup": "Sauvegarder toutes mes personnalisations (zip)…", "restore_zip": "Restaurer depuis une sauvegarde (zip)…", "backup_empty": "Aucune personnalisation à sauvegarder.", "multi_title": "{n} jeux sélectionnés", "multi_hint": "Sélection multiple : « Carte texte » et « Annuler » s'appliquent au groupe.", "cancel_multi": "Annuler ({n})", "multi_cards_ask": "Appliquer une carte texte à {n} jeu(x) ?\n\n{known} déjà personnalisé(s) : leur remplaçant sera remplacé (annulable avec « Annuler »).", "multi_cards_done": "{n} carte(s) texte appliquée(s) ({e} erreur(s)).", "multi_cancel_ask": "Annuler la personnalisation de {n} jeu(x) ?\n\n• {prev} reviennent à leur version précédente\n• {orig} retrouvent leur logo d'origine\n• {dele} sont retirés (aucun original sauvegardé : le DMD retombe sur son logo par défaut).", "multi_cancel_done": "{n} personnalisation(s) annulée(s) ({e} erreur(s)).", "multi_pi_done": "Pi {ip} : {n} retrait(s) envoyé(s).", "restore_none": "Aucune personnalisation n'a été trouvée sur la carte ni sur ce PC.\n\nRestaurer depuis une sauvegarde (zip) ?", "backup_done": "{n} personnalisation(s) sauvegardée(s) dans {path}", "backup_bad": "Ce fichier n'est pas une sauvegarde utilisable :\n{err}", "backup_ask": "Restaurer {n} personnalisation(s) depuis cette sauvegarde ?\n\n{known} remplaceront celles déjà présentes ; les autres sont conservées.\n{bad} entrée(s) invalide(s) ignorée(s).", "backup_restored": "Sauvegarde restaurée : {n} personnalisation(s) ({r} remplacée(s)).", "pi_title": "Raspberry Pi", "pi_search": "Recherche du Raspberry Pi…",
        "pi_no_dmd": "Aucun DMD trouvé sur le réseau. Vérifiez que le Raspberry Pi est allumé et sur le même réseau que ce PC.",
        "pi_not_pi": "Le DMD trouvé ({ip}) n'est pas un Raspberry Pi : l'envoi par Wi-Fi ne concerne que le Pi.\n\nPour un DMD ESP32, retirez sa carte SD et utilisez « Appliquer tout à la SD ».",
        "pi_too_old": "Le démon du Raspberry Pi ({ip}) est en version {v} : l'envoi des logos exige la 3.75 ou plus. Mettez-le à jour (Mode 17, ou bouton « Mettre à jour » de la page Réglages).",
        "pi_nothing": "Aucune personnalisation à envoyer au Pi.",
        "pi_confirm": "Envoyer {n} logo(s) au Raspberry Pi {ip} (démon {v}) ?\n\nLes logos déjà identiques sur le Pi ne sont pas renvoyés. Pour chaque jeu, le Pi garde son logo d'origine : « Retirer la perso » le rendra.{anim}",
        "pi_anim_note": "\n\n{a} animation(s) ne seront pas envoyées : ce démon ne reçoit que les images fixes (mettez le Pi à jour — démon 3.82 ou plus — ou utilisez la carte SD).",
        "pi_bg": "Lecture du Pi en arrière-plan : {done} / {total}", "loading_row": "⏳ Chargement de la liste…", "loading_row_pct": "⏳ Chargement… {done} / {total} systèmes ({pct} %) — les premiers s'affichent déjà", "loading_pct": "Chargement… {done}/{total} systèmes ({pct} %) · {games} jeux", "pi_done": "Pi {ip} : {s} envoyé(s), {u} déjà à jour, {a} animation(s) ignorée(s), {e} erreur(s).", "pi_names": "{n} jeu(x) non envoyé(s) : leur nom contient un caractère que le Pi ne sait pas enregistrer (espace insécable, accent…) — à écrire par la carte SD du Pi.", "cards_pi_skip": "\n\n{n} autre(s) jeu(x) seront ignorés : leur nom ne peut pas être enregistré sur le Pi (caractère non ASCII, ex. espace insécable).",
        "pi_cache_ko": " ⚠ Le Pi n'a pas pu compléter son index (games_cache.bin) : un logo ajouté à un jeu sans logo peut rester invisible tant que le Mode 6 n'a pas été relancé sur sa carte.",
        "pi_errors": "Erreurs d'envoi : {list}",
        "cur_pi": "Existant (sur le Raspberry Pi)", "pi_entry": "Raspberry Pi  {ip}  (Wi-Fi, {v})", "pi_reading": "Lecture du Pi {ip} : {done}/{total}…", "pi_pushing": "Envoi au Pi {ip}…",
        "pi_read_done": "Pi {ip} lu : {n} logo(s), {d} copié(s).", "pi_read_ko": "⚠ Pi {ip} : lecture incomplète ({e} erreur(s)) — l'état des logos peut être faux. « Actualiser » pour réessayer.",
        "pi_need_roms": "Indiquez le dossier ROMs (gamelist.xml) : le Pi ne fournit pas la liste des jeux, seulement leurs logos.",
        "pi_restore_ask": "Cette personnalisation avait été envoyée au Raspberry Pi.\n\nLa retirer aussi du Pi (par Wi-Fi) ?",
        "pi_restored": "Pi {ip} : original rétabli : {key}", "pi_deleted": "Pi {ip} : personnalisation retirée : {key}", "pi_restore_ko": "Pi : le retrait de {key} a échoué.",
    },
    "en": {
        "intro": "Spot the games the SD shows as a text card (or nothing) and choose a replacement. Every replacement is kept and re-applied after each copy to the SD.",
        "src_kind_sd": "SD card", "src_kind_pi": "Raspberry Pi (Wi-Fi)", "lbl_sd": "SD card:", "lbl_pi_ip": "Raspberry Pi IP:", "detect": "Detect",
        "no_drive_found": "No DMD SD card detected: insert it or pick its folder with « Browse… ».", "no_pi_connected": "The Raspberry Pi is not connected: check that it is on and on the same network, then click « Detect ». A new connection attempt has just been started.", "no_pi_found": "No Raspberry Pi reachable: enter its IP address (daemon 3.75 or later).",
        "bad_folder": "This folder does not exist: {path}",
        "sd": "SD card / Pi:", "refresh": "Refresh", "folder": "Folder…", "roms": "ROMs folder (gamelist.xml):", "browse": "Browse…", "load": "Load",
        "system": "System:", "filter": "Filter:", "f_card": "Text card", "f_avail": "Logo avail.", "f_missing": "No image", "f_dark": "Black", "f_custom": "Custom", "f_ok": "Logo",
        "col_name": "Game", "col_state": "Status",
        "st_ok": "OK", "st_anim": "animated", "st_card": "Text card", "st_avail": "logo avail.", "st_missing": "no image", "st_dark": "BLACK ⚠", "st_custom": "custom",
        "cur": "Existing (on the SD)", "view": "Show:", "v_all": "All games", "v_todo": "To improve (no real logo)", "v_avail": "Logo available on the Recalbox", "v_card": "Text cards", "v_missing": "No image", "v_dark": "Black logos", "v_custom": "Customized by you", "v_ok": "Original logos", "ex_logo": "logo", "ex_card": "text card", "ex_none": "no image", "ex_dark": "black logo", "ex_custom": "customized", "ex_anim": "animation", "note_repl": "Replacement: {src}", "note_repl_none": "Replacement: none chosen (« Choose a replacement… » or « Text card »).", "note_repl_rb": "Replacement: Recalbox logo ({src}).", "note_repl_saved": "Replacement: {src} (saved).", "note_orig_kept": "Original: the image from before any customization (kept).", "repl": "Replacement", "none": "—", "led": "LED brightness",
        "pick": "Choose a replacement…", "lbl_source": "Source:", "lbl_library": "Library (Recalbox):", "tools": "Tools ▾", "orig_tab": "Original", "clip_range": "Clip: frames {a} to {b} of {n}", "clip_start": "Start here", "clip_end": "End here", "clip_all": "Whole GIF", "clip_snap": "Capture this frame", "clip_pos": "Position: frame {p}", "clip_resample": "i {c} frames (maximum {m}): 1 frame in {k} is kept, the total duration is preserved. To keep them all, shorten the clip.", "pk_up": "↑ Parent folder", "pk_sub": "Include subfolders", "pk_suggest": "Game name", "pk_found": "{d} folder(s), {i} image(s)", "pk_more": "Show more ({n} left)", "pk_cap": "First {n} results: refine the search.", "orig_same": "Original: same as the existing one (never customized).", "browse_logos": "Browse logos…", "validate": "Validate", "remove": "Reset to original", "cancel_idle": "Cancel", "cancel_choice": "Cancel the choice", "cancel_prev": "Cancel: previous logo", "cancel_orig": "Cancel: restore original", "cancel_del": "Cancel: remove custom",
        "ask_twins": "{n} other ROM(s) of this system are also named “{name}” and each has its own logo (the DMD shows the logo of the ROM that was LAUNCHED):\n\n{roms}\n\nApply this replacement to those ROMs too?", "twins_done": "+ {n} other ROM(s) with the same name.",
        "choice_cancelled": "Choice cancelled: nothing was changed.", "undone_prev": "Previous logo restored: {key}",
        "confirm_prev": "Go back to the previous logo of “{name}”?\n\nThe current replacement is dropped (one undo level only: afterwards, « Cancel » will restore the original).", "apply_all": "Apply all to the SD", "batch": "Import a folder…", "info_anim": "animation: {n} frames, {s} s", "ask_anim": "This file contains an animation of {n} frames.\n\nYes = keep the animation (written as .raw565pack + .meta)\nNo = use only the first frame", "too_many": "Animation refused: {err}", "card": "Text card", "cards": "Cards: no logo + black", "cards_ask": "System « {sys} »: generate a text card (game name) for {n} game(s) without a logo or with a black logo?\nOriginals are kept: « Remove custom » restores them.", "cards_none": "No game without a logo or with a black logo in this system (black detection may take a few seconds after loading).", "cards_done": "Text cards: {n} written to the SD, {e} error(s).", "pk_export": "Export pack…", "pk_import": "Import pack…", "pack_empty": "No customization to export.", "pack_exported": "Pack exported: {n} logo(s) → {path}", "pack_bad": "Pack rejected: {err}", "pack_ask": "Pack: {n} valid logo(s), {bad} rejected (invalid entries).\n{known} already exist in your customizations.\n\nYes = import and replace existing ones\nNo = import only the new ones\nCancel = do nothing", "pack_done": "Pack imported: {a} added, {r} replaced, {s} kept.", "pack_types": "Logo pack",
        "no_sd": "First choose the SD card (or a folder standing in for it).", "no_sel": "First select a game in the list.",
        "all_sys": "All systems", "source_snap": "{n} games · list from the Mode 1 of {date}", "source_all": "{n} games · {s} systems",
        "snap_extra": "⚠ {n} not listed (SD)", "av_bar": "Comparison with the Recalbox: {n} game(s) shown as a text card, with no image or as a black logo on the SD have a logo on the Recalbox.", "av_cur_dark": "black logo", "av_btn": "See the new logos…", "av_head": "NEW LOGOS ON THE RECALBOX", "av_hint": "tick the ones to use; nothing changes before « Apply »", "av_all": "Tick all", "av_none": "Untick all", "av_sys": "System:", "av_every": "All ({n} systems)", "av_col_game": "Game", "av_col_cur": "On the SD (current)", "av_col_new": "On the Recalbox (proposed)", "av_cur_card": "text card", "av_cur_none": "no image", "av_apply": "Apply the {n} ticked", "av_count": "{n} ticked out of {t} — each game's original is kept (Reset possible)", "av_close": "Close", "av_done": "{n} Recalbox logo(s) applied ({e} error(s)).", "av_title": "New logos on the Recalbox", "av_note": "Only games already on the SD are offered.", "rw_title": "Choose the replacement", "rw_head": "CHOOSE THE REPLACEMENT", "rw_hint": "nothing changes before « Use this replacement », then « Validate »", "rw_target": "TARGET (what will be replaced)", "rw_now": "Currently: {state}", "rw_where": "Where to look for the replacement", "rw_s_roms": "ROM logos (gamelist.xml)", "rw_s_sd": "SD card / Pi: other games' logos", "rw_s_gifs": "SD card / Pi: gifs folder", "rw_s_file": "File, folder, clipboard…", "rw_s_card": "Text card (any title)", "rw_system": "System:", "rw_found": "{n} result(s)", "rw_preview": "Replacement preview (on hover)", "rw_mode": "Fit mode:", "rw_mode_note": "(ROM images only)", "rw_gif_note": "Tip: to rework a GIF (cut, crop, speed), use DMD GIF Creator (GIF tab), then choose the resulting file here.", "rw_anim_title": "Animated GIF", "rw_anim_n": "{n} frames", "rw_anim_all": "Whole GIF (animation)", "rw_anim_one": "A single frame (slider):", "rw_anim_pos": "Frame {k} / {n}", "rw_use": "Use this replacement", "rw_hoverhint": "Hover an image; click to choose it.", "rw_empty": "Nothing to offer here.", "rw_na_roms": "Set the Recalbox ROMs folder (gamelist.xml) in the main window.", "rw_na_sd": "No SD card / Pi loaded.", "rw_na_gifs": "The gifs folder is only read on an SD card (not on a Pi).", "rw_pi_loading": "Reading the Pi's gifs folder (Wi-Fi)…", "rw_pi_ko": "Pi gifs folder unreadable: {err}", "rw_file_text": "Pick an image, a GIF or a folder on the PC or network, or paste from the clipboard. The picker offers the fit modes.", "rw_file_btn": "Open the picker…", "rw_card_title": "Card title:", "rw_err": "Unreadable replacement: {err}", "snap_cmp": "Recalbox library: {new} more and {gone} fewer games than the SD scrape", "snap_cmp_ok": "Recalbox library = SD scrape", "cap": "First {shown} of {total} shown: filter to narrow.", "snap_done": "Reference list created for {n} system(s).",
        "snap_ask": "{n} system(s) on the SD have no reference list (SD scraped before this feature).\n\nCreate it from the current ROMs folder? SD images are not modified.\nThe list reflects today's gamelist.xml, not the one used at scrape time: each game's state is still computed from the files on the SD, and differences show up as « not listed » or « no logo ».",
        "pick_system": "First choose a single system (not « All systems ») to import a folder.", "no_sd_states": "No SD card chosen: logo states (no logo, black…) are unknown.", "off_sd": "This system has no folder on the SD: run Mode 1 first to create it.", "roms_none": "No gamelist.xml found in « {path} » or its subfolders.",
        "source_gl": "{n} games (gamelist.xml)", "source_sd": "{n} games (visuals present on the SD; set the ROMs folder to also see games without a logo)",
        "stats": "{shown} shown",
        "loading": "Loading…", "step_roms": "Reading the Recalbox library folder…", "step_pi": "Querying the Raspberry Pi: {done} / {total} systems ({pct} %)", "sd_bad_store": "The file custom/logos.json is unreadable:\n{err}\n\nIt has not been modified. Fix or rename it, then refresh.",
        "w_dark": "⚠ image almost black on the panel", "w_transparent": "transparency flattened onto the chosen background", "w_upscaled": "image smaller than the panel (enlarged)", "w_narrow": "proportions far from 4:1",
        "info_item": "{mode} · background {bg} · source {src}", "confirm_dark": "This image is almost black on the panel: the logo may be invisible.\n\nValidate anyway?",
        "collision": "Warning: another ROM gives the same visual name ({key}) once spaces are removed. The logo will be shared by both.",
        "saved": "Saved and written to the SD: {key}", "removed_restored": "Original restored: {key}", "removed_deleted": "Customization removed (no original saved, the default logo will show): {key}",
        "confirm_remove": "Remove the customization of “{name}”?\n\n{what}", "what_restored": "The saved original will be restored.", "what_deleted": "No original was saved: the file will be deleted and the DMD will show the default logo.",
        "applied": "Applied: {w} rewritten, {u} already good, {o} orphan(s), {e} error(s).", "nothing": "No customization to apply.", "err": "Error: {err}",
        "pk_title": "Choose the replacement image — {name}", "pk_folder": "Image folder:", "pk_paste": "Paste", "pk_none": "No image in this folder.",
        "pk_clip_none": "The clipboard holds no image.", "pk_use": "Use this image", "cancel": "Cancel", "pk_filter": "Filter:",
        "m_pixel": "Pixel-perfect (sharp, faithful)", "m_fit": "Fit (all visible)", "m_fill": "Stretch", "m_crop": "Crop (fills the panel)", "bg": "Background:", "bg_pick": "Choose…", "bg_black": "Black", "mode": "Conversion:",
        "pk_pick_first": "Click an image on the left.", "pk_note": "The background fills transparent areas and the bars of the “Fit”, “Stretch” and “Crop” modes.",
        "batch_pick": "Folder of images named like the games (pacman.png → Pac-Man)", "batch_empty": "No image in this folder matches a game of system « {sys} » ({unk} without game, {amb} ambiguous).", "batch_ask": "System « {sys} »: {n} image(s) to import (pixel-perfect conversion, black background).\n{amb} ambiguous and {unk} without a game will be skipped.\n\nImport?", "batch_done": "Import: {n} logo(s) written to the SD, {e} error(s), {d} almost black.", "batch_dark": "To check (almost black): {keys}",
        "br_title": "Browse existing logos", "br_systems": "Systems", "br_more": "Show more", "br_none": "No games.",
        "send_pi": "Send to the Pi (Wi-Fi)", "restore_sd": "Restore my logos on the card (after scrape / update / Mode 1)", "restore_pi": "Restore my logos on the Pi (Wi-Fi resend)", "backup": "Back up all my customizations (zip)…", "restore_zip": "Restore from a backup (zip)…", "backup_empty": "No customization to back up.", "multi_title": "{n} games selected", "multi_hint": "Multiple selection: “Text card” and “Cancel” apply to the group.", "cancel_multi": "Cancel ({n})", "multi_cards_ask": "Apply a text card to {n} game(s)?\n\n{known} already customized: their replacement will be replaced (can be undone with “Cancel”).", "multi_cards_done": "{n} text card(s) applied ({e} error(s)).", "multi_cancel_ask": "Cancel the customization of {n} game(s)?\n\n• {prev} go back to their previous version\n• {orig} get their original logo back\n• {dele} are removed (no saved original: the DMD falls back to its default logo).", "multi_cancel_done": "{n} customization(s) cancelled ({e} error(s)).", "multi_pi_done": "Pi {ip}: {n} removal(s) sent.", "restore_none": "No customization was found on the card or on this PC.\n\nRestore from a backup (zip)?", "backup_done": "{n} customization(s) backed up to {path}", "backup_bad": "This file is not a usable backup:\n{err}", "backup_ask": "Restore {n} customization(s) from this backup?\n\n{known} will replace the ones already present; the others are kept.\n{bad} invalid entr(y/ies) ignored.", "backup_restored": "Backup restored: {n} customization(s) ({r} replaced).", "pi_title": "Raspberry Pi", "pi_search": "Looking for the Raspberry Pi…",
        "pi_no_dmd": "No DMD found on the network. Check that the Raspberry Pi is on and on the same network as this PC.",
        "pi_not_pi": "The DMD found ({ip}) is not a Raspberry Pi: sending over Wi-Fi only applies to the Pi.\n\nFor an ESP32 DMD, take its SD card out and use « Apply all to the SD ».",
        "pi_too_old": "The Raspberry Pi daemon ({ip}) is version {v}: sending logos requires 3.75 or later. Update it (Mode 17, or the “Update” button on the Settings page).",
        "pi_nothing": "No customization to send to the Pi.",
        "pi_confirm": "Send {n} logo(s) to the Raspberry Pi {ip} (daemon {v})?\n\nLogos already identical on the Pi are not sent again. For each game the Pi keeps its original logo: “Remove custom” will restore it.{anim}",
        "pi_anim_note": "\n\n{a} animation(s) will not be sent: this daemon only receives still images (update the Pi — daemon 3.82 or later — or use the SD card).",
        "pi_bg": "Reading the Pi in the background: {done} / {total}", "loading_row": "⏳ Loading the list…", "loading_row_pct": "⏳ Loading… {done} / {total} systems ({pct} %) — the first ones are already shown", "loading_pct": "Loading… {done}/{total} systems ({pct} %) · {games} games", "pi_done": "Pi {ip}: {s} sent, {u} already up to date, {a} animation(s) skipped, {e} error(s).", "pi_names": "{n} game(s) not sent: their name contains a character the Pi cannot store (non-breaking space, accent…) — write them through the Pi's SD card.", "cards_pi_skip": "\n\n{n} other game(s) will be skipped: their name cannot be stored on the Pi (non-ASCII character, e.g. non-breaking space).",
        "pi_cache_ko": " ⚠ The Pi could not complete its index (games_cache.bin): a logo added to a game that had none may stay invisible until Mode 6 is run again on its card.",
        "pi_errors": "Send errors: {list}",
        "cur_pi": "Existing (on the Raspberry Pi)", "pi_entry": "Raspberry Pi  {ip}  (Wi-Fi, {v})", "pi_reading": "Reading the Pi {ip}: {done}/{total}…", "pi_pushing": "Sending to the Pi {ip}…",
        "pi_read_done": "Pi {ip} read: {n} logo(s), {d} copied.", "pi_read_ko": "⚠ Pi {ip}: incomplete read ({e} error(s)) — logo states may be wrong. Click “Refresh” to retry.",
        "pi_need_roms": "Set the ROMs folder (gamelist.xml): the Pi does not provide the game list, only their logos.",
        "pi_restore_ask": "This customization had been sent to the Raspberry Pi.\n\nRemove it from the Pi as well (over Wi-Fi)?",
        "pi_restored": "Pi {ip}: original restored: {key}", "pi_deleted": "Pi {ip}: customization removed: {key}", "pi_restore_ko": "Pi: removing {key} failed.",
    },
    "es": {
        "intro": "Localice los juegos que la SD muestra como tarjeta de texto (o nada) y elija un sustituto. Cada sustitución se conserva y se reaplica tras cada copia a la SD.",
        "src_kind_sd": "Tarjeta SD", "src_kind_pi": "Raspberry Pi (Wi-Fi)", "lbl_sd": "Tarjeta SD:", "lbl_pi_ip": "IP de la Raspberry Pi:", "detect": "Detectar",
        "no_drive_found": "No se detectó ninguna tarjeta SD del DMD: insértala o elige su carpeta con « Examinar… ».", "no_pi_connected": "La Raspberry Pi no está conectada: compruebe que está encendida y en la misma red, y pulse « Detectar ». Se acaba de iniciar un nuevo intento de conexión.", "no_pi_found": "Ninguna Raspberry Pi accesible: introduce su dirección IP (demonio 3.75 o posterior).",
        "bad_folder": "Esta carpeta no existe: {path}",
        "sd": "Tarjeta SD / Pi:", "refresh": "Actualizar", "folder": "Carpeta…", "roms": "Carpeta de ROMs (gamelist.xml):", "browse": "Examinar…", "load": "Cargar",
        "system": "Sistema:", "filter": "Filtro:", "f_card": "Tarjeta texto", "f_avail": "Logo disp.", "f_missing": "Sin imagen", "f_dark": "Negros", "f_custom": "Perso", "f_ok": "Logo",
        "col_name": "Juego", "col_state": "Estado",
        "st_ok": "OK", "st_anim": "animado", "st_card": "Tarjeta texto", "st_avail": "logo disp.", "st_missing": "sin imagen", "st_dark": "NEGRO ⚠", "st_custom": "perso",
        "cur": "Existente (en la SD)", "view": "Mostrar:", "v_all": "Todos los juegos", "v_todo": "A mejorar (sin logo real)", "v_avail": "Logo disponible en la Recalbox", "v_card": "Tarjetas de texto", "v_missing": "Sin imagen", "v_dark": "Logos negros", "v_custom": "Personalizados por usted", "v_ok": "Logos originales", "ex_logo": "logo", "ex_card": "tarjeta de texto", "ex_none": "sin imagen", "ex_dark": "logo negro", "ex_custom": "personalizado", "ex_anim": "animación", "note_repl": "Sustituto: {src}", "note_repl_none": "Sustituto: ninguno elegido (« Elegir un sustituto… » o « Tarjeta de texto »).", "note_repl_rb": "Sustituto: logo de la Recalbox ({src}).", "note_repl_saved": "Sustituto: {src} (guardado).", "note_orig_kept": "Original: la imagen de antes de toda personalización (conservada).", "repl": "Sustituto", "none": "—", "led": "Brillo LED",
        "pick": "Elegir un sustituto…", "lbl_source": "Fuente:", "lbl_library": "Biblioteca (Recalbox):", "tools": "Herramientas ▾", "orig_tab": "Original", "clip_range": "Fragmento: imágenes {a} a {b} de {n}", "clip_start": "Inicio aquí", "clip_end": "Fin aquí", "clip_all": "Todo el GIF", "clip_snap": "Capturar esta imagen", "clip_pos": "Posición: imagen {p}", "clip_resample": "i {c} imágenes (máximo {m}): se conserva 1 de cada {k}, la duración total se mantiene. Para conservarlas todas, acorte el fragmento.", "pk_up": "↑ Carpeta superior", "pk_sub": "Incluir subcarpetas", "pk_suggest": "Nombre del juego", "pk_found": "{d} carpeta(s), {i} imagen(es)", "pk_more": "Mostrar más ({n} restantes)", "pk_cap": "Primeros {n} resultados: afine la búsqueda.", "orig_same": "Original: igual al existente (nunca personalizado).", "browse_logos": "Explorar logos…", "validate": "Validar", "remove": "Restablecer el original", "cancel_idle": "Cancelar", "cancel_choice": "Cancelar la elección", "cancel_prev": "Cancelar: logo anterior", "cancel_orig": "Cancelar: devolver original", "cancel_del": "Cancelar: quitar perso",
        "ask_twins": "Otras {n} ROM de este sistema también se llaman «{name}» y cada una tiene su propio logo (el DMD muestra el de la ROM LANZADA):\n\n{roms}\n\n¿Aplicar este reemplazo también a esas ROM?", "twins_done": "+ {n} ROM más con el mismo nombre.",
        "choice_cancelled": "Elección cancelada: no se modificó nada.", "undone_prev": "Logo anterior restablecido: {key}",
        "confirm_prev": "¿Volver al logo anterior de «{name}»?\n\nSe abandona el reemplazo actual (un solo nivel de cancelación: después, « Cancelar » devolverá el original).", "apply_all": "Aplicar todo a la SD", "batch": "Importar una carpeta…", "info_anim": "animación: {n} imágenes, {s} s", "ask_anim": "Este archivo contiene una animación de {n} imágenes.\n\nSí = conservar la animación (escrita en .raw565pack + .meta)\nNo = usar solo la 1.ª imagen", "too_many": "Animación rechazada: {err}", "card": "Tarjeta de texto", "cards": "Tarjetas: sin logo + negros", "cards_ask": "Sistema « {sys} »: ¿generar una tarjeta de texto (nombre del juego) para {n} juego(s) sin logo o con logo negro?\nSe conservan los originales: « Quitar perso » los restablece.", "cards_none": "Ningún juego sin logo ni con logo negro en este sistema (la detección de negros puede tardar unos segundos tras la carga).", "cards_done": "Tarjetas de texto: {n} escrita(s) en la SD, {e} error(es).", "pk_export": "Exportar paquete…", "pk_import": "Importar paquete…", "pack_empty": "No hay personalizaciones que exportar.", "pack_exported": "Paquete exportado: {n} logo(s) → {path}", "pack_bad": "Paquete rechazado: {err}", "pack_ask": "Paquete: {n} logo(s) válidos, {bad} rechazado(s) (entradas no válidas).\n{known} ya existen en sus personalizaciones.\n\nSí = importar y reemplazar los existentes\nNo = importar solo los nuevos\nCancelar = no hacer nada", "pack_done": "Paquete importado: {a} añadido(s), {r} reemplazado(s), {s} conservado(s).", "pack_types": "Paquete de logos",
        "no_sd": "Elija primero la tarjeta SD (o una carpeta que la sustituya).", "no_sel": "Seleccione primero un juego de la lista.",
        "all_sys": "Todos los sistemas", "source_snap": "{n} juegos · lista del Modo 1 del {date}", "source_all": "{n} juegos · {s} sistemas",
        "snap_extra": "⚠ {n} fuera de lista (SD)", "av_bar": "Comparación con la Recalbox: {n} juego(s) mostrado(s) como tarjeta de texto, sin imagen o con logo negro en la SD tienen un logo en la Recalbox.", "av_cur_dark": "logo negro", "av_btn": "Ver los nuevos logos…", "av_head": "NUEVOS LOGOS EN LA RECALBOX", "av_hint": "marque los que desea usar; nada cambia antes de « Aplicar »", "av_all": "Marcar todo", "av_none": "Desmarcar todo", "av_sys": "Sistema:", "av_every": "Todos ({n} sistemas)", "av_col_game": "Juego", "av_col_cur": "En la SD (actual)", "av_col_new": "En la Recalbox (propuesto)", "av_cur_card": "tarjeta de texto", "av_cur_none": "sin imagen", "av_apply": "Aplicar los {n} marcados", "av_count": "{n} marcado(s) de {t} — se conserva el original de cada juego (Reiniciar posible)", "av_close": "Cerrar", "av_done": "{n} logo(s) de la Recalbox aplicado(s) ({e} error(es)).", "av_title": "Nuevos logos en la Recalbox", "av_note": "Solo se ofrecen los juegos que ya están en la SD.", "rw_title": "Elegir el sustituto", "rw_head": "ELEGIR EL SUSTITUTO", "rw_hint": "nada cambia antes de « Usar este sustituto » y luego « Validar »", "rw_target": "OBJETIVO (lo que se sustituirá)", "rw_now": "Actualmente: {state}", "rw_where": "Dónde buscar el sustituto", "rw_s_roms": "Logos de las ROM (gamelist.xml)", "rw_s_sd": "Tarjeta SD / Pi: logos de otros juegos", "rw_s_gifs": "Tarjeta SD / Pi: carpeta gifs", "rw_s_file": "Archivo, carpeta, portapapeles…", "rw_s_card": "Tarjeta de texto (título a elegir)", "rw_system": "Sistema:", "rw_found": "{n} resultado(s)", "rw_preview": "Vista previa del sustituto (al pasar)", "rw_mode": "Modo de ajuste:", "rw_mode_note": "(solo imágenes de las ROM)", "rw_gif_note": "Consejo: para retocar un GIF (recorte, encuadre, velocidad), use DMD GIF Creator (pestaña GIF) y elija aquí el archivo resultante.", "rw_anim_title": "GIF animado", "rw_anim_n": "{n} imágenes", "rw_anim_all": "Todo el GIF (animación)", "rw_anim_one": "Una sola imagen (control):", "rw_anim_pos": "Imagen {k} / {n}", "rw_use": "Usar este sustituto", "rw_hoverhint": "Pase sobre una imagen; haga clic para elegirla.", "rw_empty": "Nada que proponer aquí.", "rw_na_roms": "Indique la carpeta de ROM de la Recalbox (gamelist.xml) en la ventana principal.", "rw_na_sd": "Ninguna tarjeta SD / Pi cargada.", "rw_na_gifs": "La carpeta gifs solo se lee en una tarjeta SD (no en una Pi).", "rw_pi_loading": "Leyendo la carpeta gifs de la Pi (Wi-Fi)…", "rw_pi_ko": "Carpeta gifs de la Pi ilegible: {err}", "rw_file_text": "Elija una imagen, un GIF o una carpeta en el PC o la red, o pegue desde el portapapeles. El selector ofrece los modos de ajuste.", "rw_file_btn": "Abrir el selector…", "rw_card_title": "Título de la tarjeta:", "rw_err": "Sustituto ilegible: {err}", "snap_cmp": "Biblioteca Recalbox: {new} juego(s) más y {gone} menos que el scrape de la SD", "snap_cmp_ok": "Biblioteca Recalbox = scrape de la SD", "cap": "Primeros {shown} de {total}: filtre para acotar.", "snap_done": "Lista de referencia creada para {n} sistema(s).",
        "snap_ask": "{n} sistema(s) de la SD no tienen lista de referencia (SD creada antes de esta función).\n\n¿Crearla desde la carpeta de ROMs actual? Las imágenes de la SD no se modifican.\nLa lista refleja el gamelist.xml de hoy, no el del scrape: el estado de cada juego se sigue calculando con los archivos de la SD y las diferencias aparecen como « fuera de lista » o « sin logo ».",
        "pick_system": "Elija primero un sistema concreto (no « Todos los sistemas ») para importar una carpeta.", "no_sd_states": "Ninguna tarjeta SD elegida: se desconoce el estado de los logos (sin logo, negro…).", "off_sd": "Este sistema no tiene carpeta en la SD: ejecute primero el Modo 1 para crearla.", "roms_none": "No se encontró ningún gamelist.xml en « {path} » ni en sus subcarpetas.",
        "source_gl": "{n} juegos (gamelist.xml)", "source_sd": "{n} juegos (visuales presentes en la SD; indique la carpeta de ROMs para ver también los juegos sin logo)",
        "stats": "{shown} mostrados",
        "loading": "Cargando…", "step_roms": "Leyendo la carpeta de la biblioteca Recalbox…", "step_pi": "Consultando la Raspberry Pi: {done} / {total} sistemas ({pct} %)", "sd_bad_store": "El archivo custom/logos.json es ilegible:\n{err}\n\nNo se ha modificado. Corríjalo o renómbrelo y actualice.",
        "w_dark": "⚠ imagen casi negra en la pantalla", "w_transparent": "transparencia aplanada sobre el fondo elegido", "w_upscaled": "imagen más pequeña que la pantalla (ampliada)", "w_narrow": "proporciones muy alejadas de 4:1",
        "info_item": "{mode} · fondo {bg} · origen {src}", "confirm_dark": "Esta imagen es casi negra en la pantalla: el logo podría ser invisible.\n\n¿Validar de todos modos?",
        "collision": "Atención: otra ROM da el mismo nombre de visual ({key}) sin espacios. El logo será compartido por ambas.",
        "saved": "Guardado y escrito en la SD: {key}", "removed_restored": "Original restituido: {key}", "removed_deleted": "Personalización quitada (sin original guardado, se mostrará el logo por defecto): {key}",
        "confirm_remove": "¿Quitar la personalización de «{name}»?\n\n{what}", "what_restored": "Se restituirá el original guardado.", "what_deleted": "No se guardó ningún original: el archivo se borrará y el DMD mostrará el logo por defecto.",
        "applied": "Aplicado: {w} reescrito(s), {u} ya correcto(s), {o} huérfano(s), {e} error(es).", "nothing": "Ninguna personalización que aplicar.", "err": "Error: {err}",
        "pk_title": "Elegir la imagen de sustitución — {name}", "pk_folder": "Carpeta de imágenes:", "pk_paste": "Pegar", "pk_none": "No hay imágenes en esta carpeta.",
        "pk_clip_none": "El portapapeles no contiene ninguna imagen.", "pk_use": "Usar esta imagen", "cancel": "Cancelar", "pk_filter": "Filtro:",
        "m_pixel": "Pixel-perfect (nítido, fiel)", "m_fit": "Ajustar (todo visible)", "m_fill": "Estirar", "m_crop": "Recortar (llena la pantalla)", "bg": "Fondo:", "bg_pick": "Elegir…", "bg_black": "Negro", "mode": "Conversión:",
        "pk_pick_first": "Haga clic en una imagen a la izquierda.", "pk_note": "El fondo rellena las zonas transparentes y las bandas de los modos «Ajustar», «Estirar» y «Recortar».",
        "batch_pick": "Carpeta de imágenes con el nombre de los juegos (pacman.png → Pac-Man)", "batch_empty": "Ninguna imagen de esta carpeta corresponde a un juego del sistema « {sys} » ({unk} sin juego, {amb} ambiguas).", "batch_ask": "Sistema « {sys} »: {n} imagen(es) por importar (conversión pixel-perfect, fondo negro).\n{amb} ambigua(s) y {unk} sin juego se ignorarán.\n\n¿Importar?", "batch_done": "Importación: {n} logo(s) escritos en la SD, {e} error(es), {d} casi negro(s).", "batch_dark": "A revisar (casi negros): {keys}",
        "br_title": "Explorar los logos existentes", "br_systems": "Sistemas", "br_more": "Mostrar más", "br_none": "Ningún juego.",
        "send_pi": "Enviar a la Pi (Wi-Fi)", "restore_sd": "Restaurar mis logos en la tarjeta (tras scrape / actualización / Modo 1)", "restore_pi": "Restaurar mis logos en la Pi (reenvío Wi-Fi)", "backup": "Guardar todas mis personalizaciones (zip)…", "restore_zip": "Restaurar desde una copia de seguridad (zip)…", "backup_empty": "No hay personalizaciones que guardar.", "multi_title": "{n} juegos seleccionados", "multi_hint": "Selección múltiple: «Tarjeta de texto» y «Cancelar» se aplican al grupo.", "cancel_multi": "Cancelar ({n})", "multi_cards_ask": "¿Aplicar una tarjeta de texto a {n} juego(s)?\n\n{known} ya personalizado(s): su reemplazo será reemplazado (se puede deshacer con «Cancelar»).", "multi_cards_done": "{n} tarjeta(s) de texto aplicada(s) ({e} error(es)).", "multi_cancel_ask": "¿Cancelar la personalización de {n} juego(s)?\n\n• {prev} vuelven a su versión anterior\n• {orig} recuperan su logo original\n• {dele} se eliminan (sin original guardado: el DMD vuelve a su logo por defecto).", "multi_cancel_done": "{n} personalización(es) cancelada(s) ({e} error(es)).", "multi_pi_done": "Pi {ip}: {n} eliminación(es) enviada(s).", "restore_none": "No se encontró ninguna personalización en la tarjeta ni en este PC.\n\n¿Restaurar desde una copia de seguridad (zip)?", "backup_done": "{n} personalización(es) guardada(s) en {path}", "backup_bad": "Este archivo no es una copia de seguridad utilizable:\n{err}", "backup_ask": "¿Restaurar {n} personalización(es) desde esta copia?\n\n{known} reemplazarán las ya presentes; las demás se conservan.\n{bad} entrada(s) no válida(s) ignorada(s).", "backup_restored": "Copia restaurada: {n} personalización(es) ({r} reemplazada(s)).", "pi_title": "Raspberry Pi", "pi_search": "Buscando la Raspberry Pi…",
        "pi_no_dmd": "No se encontró ningún DMD en la red. Compruebe que la Raspberry Pi está encendida y en la misma red que este PC.",
        "pi_not_pi": "El DMD encontrado ({ip}) no es una Raspberry Pi: el envío por Wi-Fi solo vale para la Pi.\n\nPara un DMD ESP32, saque su tarjeta SD y use « Aplicar todo a la SD ».",
        "pi_too_old": "El demonio de la Raspberry Pi ({ip}) es la versión {v}: el envío de logos exige la 3.75 o superior. Actualícelo (Modo 17, o el botón «Actualizar» de la página Ajustes).",
        "pi_nothing": "No hay personalizaciones que enviar a la Pi.",
        "pi_confirm": "¿Enviar {n} logo(s) a la Raspberry Pi {ip} (demonio {v})?\n\nLos logos ya idénticos en la Pi no se reenvían. Para cada juego la Pi conserva su logo original: « Quitar perso » lo restablecerá.{anim}",
        "pi_anim_note": "\n\n{a} animación(es) no se enviarán: este demonio solo recibe imágenes fijas (actualice la Pi — demonio 3.82 o superior — o use la tarjeta SD).",
        "pi_bg": "Leyendo la Pi en segundo plano: {done} / {total}", "loading_row": "⏳ Cargando la lista…", "loading_row_pct": "⏳ Cargando… {done} / {total} sistemas ({pct} %) — los primeros ya se muestran", "loading_pct": "Cargando… {done}/{total} sistemas ({pct} %) · {games} juegos", "pi_done": "Pi {ip}: {s} enviado(s), {u} ya al día, {a} animación(es) omitida(s), {e} error(es).", "pi_names": "{n} juego(s) no enviado(s): su nombre contiene un carácter que la Pi no puede guardar (espacio de no separación, acento…) — escríbalos con la tarjeta SD de la Pi.", "cards_pi_skip": "\n\n{n} juego(s) más se omitirán: su nombre no se puede guardar en la Pi (carácter no ASCII, p. ej. espacio de no separación).",
        "pi_cache_ko": " ⚠ La Pi no pudo completar su índice (games_cache.bin): un logo añadido a un juego sin logo puede seguir invisible hasta que se vuelva a ejecutar el Modo 6 en su tarjeta.",
        "pi_errors": "Errores de envío: {list}",
        "cur_pi": "Existente (en la Raspberry Pi)", "pi_entry": "Raspberry Pi  {ip}  (Wi-Fi, {v})", "pi_reading": "Leyendo la Pi {ip}: {done}/{total}…", "pi_pushing": "Enviando a la Pi {ip}…",
        "pi_read_done": "Pi {ip} leída: {n} logo(s), {d} copiado(s).", "pi_read_ko": "⚠ Pi {ip}: lectura incompleta ({e} error(es)) — el estado de los logos puede ser incorrecto. « Actualizar » para reintentar.",
        "pi_need_roms": "Indique la carpeta de ROMs (gamelist.xml): la Pi no da la lista de juegos, solo sus logos.",
        "pi_restore_ask": "Esta personalización se había enviado a la Raspberry Pi.\n\n¿Quitarla también de la Pi (por Wi-Fi)?",
        "pi_restored": "Pi {ip}: original restituido: {key}", "pi_deleted": "Pi {ip}: personalización quitada: {key}", "pi_restore_ko": "Pi: no se pudo quitar {key}.",
    },
}

IMG_EXTS = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".tif", ".tiff"}
PV_SCALE = 3                         # echelle des apercus LED de la fenetre principale (384 x 96, comme la maquette validee)
PV_W, PV_H = 128 * PV_SCALE, 32 * PV_SCALE
AVAIL_BG, AVAIL_EDGE, AVAIL_BTN = "#EFE3F5", "#B58BC9", "#E1CFF0"      # bandeau « logos disponibles sur la Recalbox » (fixe : lisible quel que soit le theme)
STATE_ORDER = ("avail", "card", "missing", "dark", "custom", "ok")
STATE_COLORS = {"avail": "#7B1FA2", "card": "#B26A00", "missing": "#8A6D3B", "dark": "#C62828", "custom": "#1B7F3B", "anim": "#1F5FBF", "ok": "#000000"}


def _prefs():
    try:
        import RecalBoxDMD_prefs as p
        return p
    except Exception:   # noqa: BLE001
        return None


def _pref_get(key, default=""):
    p = _prefs()
    try:
        return (p.get(key) if p else None) or default
    except Exception:   # noqa: BLE001
        return default


def _pref_set(key, value):
    p = _prefs()
    try:
        if p:
            p.set(key, value)
    except Exception:   # noqa: BLE001
        pass


MAX_SHOWN = 2500   # lignes inserees au plus dans la liste (le reste s'atteint en filtrant)
ALL = "*"     # entree « Tous les systemes » du menu Systeme (self.system vaut ALL ; le texte affiche est T["all_sys"])


def _row_category(r):
    if r["state"] in ("custom", "missing", "avail", "card"):
        return r["state"]
    if r.get("dark"):
        return "avail" if r.get("rb_logo") else "dark"      # logo NOIR : « Logo dispo » si la Recalbox en possede un vrai, sinon « Noir »
    return "ok"


class _ViewFilter:
    """Menu « Afficher » : UN choix, formule en toutes lettres et suivi de son effectif (« Cartes texte (3) »), a la place d'une rangee de cases a cocher dont le role
    ne se devine pas avant de les essayer. Sert la fenetre principale et le parcours des logos."""
    KEYS = ("all", "todo", "avail", "card", "missing", "dark", "custom", "ok")
    _CATS = {"all": None, "todo": ("avail", "card", "missing", "dark"), "avail": ("avail",), "card": ("card",), "missing": ("missing",), "dark": ("dark",),
             "custom": ("custom",), "ok": ("ok",)}

    def __init__(self, parent, t, on_change, bg, fg, width=32):
        self.t, self.on_change, self.key, self.counts = t, on_change, "all", {}
        self.label = tk.Label(parent, text=t("view"), bg=bg, fg=fg)
        self.var = tk.StringVar()
        self.cb = ttk.Combobox(parent, textvariable=self.var, state="readonly", width=width)
        self.cb.bind("<<ComboboxSelected>>", self._picked)
        self.refresh()

    def _text(self, k):
        n = self.counts.get(k)
        return self.t("v_" + k) + (f" ({n})" if n is not None else "")

    def refresh(self, counts=None):
        if counts is not None:
            self.counts = counts
        self.label.config(text=self.t("view"))
        self.cb["values"] = [self._text(k) for k in self.KEYS]
        self.var.set(self._text(self.key))

    def _picked(self, _e=None):
        i = self.cb.current()
        if 0 <= i < len(self.KEYS):
            self.key = self.KEYS[i]
        self.refresh()
        self.on_change()

    def shows(self, cat):
        cats = self._CATS[self.key]
        return cats is None or cat in cats

    def reset(self):
        self.key = "all"
        self.refresh()

    @staticmethod
    def count(rows):
        c = {k: 0 for k in ("avail", "card", "missing", "dark", "custom", "ok")}
        for r in rows:
            c[_row_category(r)] += 1
        c["all"] = len(rows)
        c["todo"] = c["avail"] + c["card"] + c["missing"] + c["dark"]
        return c


def _is_avail(r):
    """Jeu dont la Recalbox possede un logo a proposer : carte texte / sans visuel / logo noir (le noir n'est connu qu'apres le scan en arriere-plan)."""
    return _row_category(r) == "avail" and bool(r.get("rb_logo"))


class _Base:
    """Langue, couleurs et execution en arriere-plan communes aux trois pieces."""

    def __init__(self, gui):
        self.gui = gui
        c = {}
        try:
            c = gui._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        self.bg = c.get("bg_main", "#F3F3F3")
        self.fg = c.get("fg_text", "#000000")
        self.bg_action = c.get("bg_button_action", "#FFD400")

    def t(self, name, /, **kw):
        try:
            lang = self.gui.lang_var.get()
        except Exception:   # noqa: BLE001
            lang = "fr"
        s = T.get(lang, T["fr"]).get(name) or T["fr"][name]
        return s.format(**kw) if kw else s

    def _btn(self, parent, key, command, **kw):
        opts = {"bg": "#FFFFFF", "fg": "black", "bd": 2, "relief": "solid", "padx": 8, "pady": 3}
        opts.update(kw)
        if "bg" in kw and "fg" not in kw:       # couleur d'action du theme : texte noir sur fond clair, blanc sur fond sombre
            opts["fg"] = _fg_on(kw["bg"])
        return tk.Button(parent, text=self.t(key), command=command, **opts)

    def bg_run(self, widget, work, done):
        """work() hors du thread d'interface ; done(resultat, erreur) dans le thread d'interface."""
        def run():
            try:
                res, err = work(), None
            except Exception as e:   # noqa: BLE001
                res, err = None, e
            try:
                widget.after(0, lambda: done(res, err))
            except Exception:   # noqa: BLE001
                pass   # fenetre fermee entre-temps
        threading.Thread(target=run, daemon=True).start()


def _photo(img):
    from PIL import ImageTk
    return ImageTk.PhotoImage(img)


def _fg_on(bg_hex: str) -> str:
    """Noir sur fond clair, blanc sur fond sombre (luminance relative)."""
    try:
        h = bg_hex.lstrip("#")
        r, g, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    except (ValueError, IndexError):
        return "black"
    return "black" if (0.299 * r + 0.587 * g + 0.114 * b) > 140 else "white"


def _clip_px(widget, text, max_px):
    """`text` coupe avec « … » pour tenir dans max_px pixels avec la police de `widget` (la taille d'un cadre ne doit pas dependre du nom qu'il affiche)."""
    import tkinter.font as tkfont
    try:
        font = tkfont.Font(root=widget, font=widget.cget("font"))
        if font.measure(text) <= max_px:
            return text
        while text and font.measure(text + "…") > max_px:
            text = text[:-1]
        return text + "…"
    except tk.TclError:
        return text[:30]


class _StubButton:
    """Remplace un bouton retire de la fenetre (son action est dans un menu) : accepte les config(state=...) du code qui le pilotait encore."""

    def config(self, *a, **k):
        pass

    configure = config


class CustomLogosTab(_Base):
    def __init__(self, gui, parent):
        super().__init__(gui)
        self.parent = parent
        self.sd_root = None
        self.store = cl.new_store()
        self.store_ok = True
        self.systems = []
        self.system = ""
        self.sd_systems = set()      # systemes qui ont un dossier sur la SD (les autres viennent du dossier ROMs : lecture seule)
        self.roms_map = {}           # {systeme: gamelist.xml} du dossier ROMs
        self.snap = cl.new_snapshot()    # liste des jeux du dernier Mode 1 (custom/games.json de la SD)
        self.rows = []
        self.by_iid = {}
        self.pending = None          # (item, rapport, apercu) de l'image choisie, pas encore validee
        self._photos = {}
        self._token = 0              # invalide les chargements / scans devenus perimes
        self._drives = []
        self._i18n = []              # (widget, cle, option) reecrits au changement de langue
        self._toggles = []           # cases a cocher : leur couleur de fond suit le theme (voir apply_theme)
        self._select_after_load = None
        self.pi_ip = None            # adresse du Raspberry Pi quand il est la source (liste « Carte SD / Pi ») : sd_root est alors son MIROIR local (voir pi_logos.mirror_root)
        self._pi_info = None         # Pi detecte (info de GET /load) : propose dans la liste
        self._pi_synced = set()      # systemes deja lus sur le Pi pendant cette selection
        self._pi_lists = {}          # {systeme: (le Pi le possede?, stems .raw565, stems .raw565pack)} d'apres /logos-list (demon >= 3.91) ; {} = demon sans la route
        self._rows_cache = {}        # {systeme: (lignes, source, infos)} : systemes deja visites, affiches tout de suite puis recalcules en fond (voir _load_system)
        self._pi_note = ""
        self._cur_lbl = None
        self._clean_old_temp()
        self._build()
        self.refresh_drives(scan=False)      # au lancement : seulement le Pi deja connu (pas de balayage du reseau) ; « Actualiser » balaie

    @staticmethod
    def _clean_old_temp():
        """Les GIF du Pi telecharges pour l'apercu vont dans %TEMP%\\gifs_pi_* ; un toolkit ferme brutalement les laissait (30 dossiers constates le 2026-11-17). Au lancement : on supprime ceux
        de plus de 12 h (ceux d'une session en cours sont plus recents)."""
        import shutil
        import tempfile
        try:
            limit = time.time() - 12 * 3600
            for d in Path(tempfile.gettempdir()).glob("gifs_pi_*"):
                try:
                    if d.is_dir() and d.stat().st_mtime < limit:
                        shutil.rmtree(d, ignore_errors=True)
                except OSError:
                    continue
        except Exception:   # noqa: BLE001
            pass

    # ------------------------------------------------------------------ construction
    def _label(self, parent, key, **kw):
        w = tk.Label(parent, text=self.t(key), bg=self.bg, fg=self.fg, **kw)
        self._i18n.append((w, key))
        return w

    def _button(self, parent, key, command, **kw):
        b = self._btn(parent, key, command, **kw)
        self._i18n.append((b, key))
        return b

    def _build(self):
        p = self.parent
        BW = 13              # largeur (caracteres) des boutons des deux lignes du haut : identiques et alignes en colonnes
        top = tk.Frame(p, bg=self.bg)
        top.pack(fill="x", padx=10, pady=(6, 1))
        top.columnconfigure(1, weight=1)
        # Cible : meme methode que l'onglet « GIFs de jeux » (boutons radio Carte SD / Raspberry Pi (Wi-Fi), champ, Parcourir, Detecter)
        # (les deux boutons radio prennent la place de l'ancien libelle de la colonne 0 : l'onglet n'a pas une ligne de hauteur de plus, voir « hauteur utile »)
        kinds = tk.Frame(top, bg=self.bg)
        kinds.grid(row=0, column=0, sticky="w")
        self.src_mode = tk.StringVar(value="pi" if _pref_get("custom_last_drive") == "PI" else "sd")
        for val, key in (("sd", "src_kind_sd"), ("pi", "src_kind_pi")):
            rb = tk.Radiobutton(kinds, text=self.t(key), variable=self.src_mode, value=val, bg=self.bg, fg=self.fg, selectcolor=self.bg, activebackground=self.bg,
                                command=self._on_src_mode)
            rb.pack(side="left", padx=(0, 14))
            self._i18n.append((rb, key))
            self._toggles.append(rb)
        self.drive_var = tk.StringVar()
        target_entry = tk.Entry(top, textvariable=self.drive_var)
        target_entry.grid(row=0, column=1, padx=6, pady=1, sticky="we")
        target_entry.bind("<Return>", lambda e: self._apply_typed_target())
        self.browse_sd_btn = self._button(top, "browse", self._pick_sd_folder, padx=6, pady=0, bd=1, width=BW)      # bd=1 / pady=0 : les deux rangees du haut gagnent ~12 px (hauteur utile de l'onglet)
        self.browse_sd_btn.grid(row=0, column=2, padx=(0, 4))
        self._button(top, "detect", self._auto_target, padx=6, pady=0, bd=1, width=BW).grid(row=0, column=3)
        self.send_pi_btn = _StubButton()      # « Envoyer tout au Pi » est dans le menu Outils ; _on_send_pi / _pi_pushed pilotent encore l'etat de ce bouton
        self._label(top, "lbl_library").grid(row=1, column=0, sticky="w")
        self.roms_var = tk.StringVar(value=_pref_get("custom_roms_dir"))
        roms_entry = tk.Entry(top, textvariable=self.roms_var)
        roms_entry.grid(row=1, column=1, padx=6, pady=1, sticky="we")
        roms_entry.bind("<Return>", lambda e: self._reload())
        self._button(top, "browse", self._pick_roms, padx=6, pady=0, bd=1, width=BW).grid(row=1, column=2, padx=(0, 4))
        self._button(top, "load", self._reload, padx=6, pady=0, bd=1, width=BW).grid(row=1, column=3)

        bar = tk.Frame(p, bg=self.bg)
        bar.pack(fill="x", padx=10, pady=(4, 2))
        self._label(bar, "system").pack(side="left")
        self.system_var = tk.StringVar()
        self.system_combo = ttk.Combobox(bar, textvariable=self.system_var, state="readonly", width=18)
        self.system_combo.pack(side="left", padx=(4, 14))
        self.system_combo.bind("<<ComboboxSelected>>", lambda e: self._load_system(self._sys_arg(), use_cache=True))
        self.view = _ViewFilter(bar, self.t, self._refill, self.bg, self.fg)        # « Afficher : » -- UN menu clair au lieu de six cases a cocher
        self.view.label.pack(side="left")
        self.view.cb.pack(side="left", padx=(4, 14))
        self._label(bar, "filter").pack(side="left")
        self.filter_var = tk.StringVar()
        self.filter_var.trace_add("write", lambda *a: self._refill_soon())
        tk.Entry(bar, textvariable=self.filter_var, width=16).pack(side="left", padx=(4, 0))
        self.stats_lbl = tk.Label(bar, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8))
        self.stats_lbl.pack(side="right")

        self.avail_bar = tk.Frame(p, bg=AVAIL_BG, highlightthickness=1, highlightbackground=AVAIL_EDGE)      # affiche seulement s'il y a des logos a proposer (voir _update_avail_bar)
        self.avail_lbl = tk.Label(self.avail_bar, text="", bg=AVAIL_BG, fg="#000000", font=("TkDefaultFont", 8), anchor="w")
        self.avail_lbl.pack(side="left", padx=6, pady=2)
        self.avail_btn = tk.Button(self.avail_bar, text="", bg=AVAIL_BTN, fg="#000000", activebackground=AVAIL_BTN, padx=8, pady=0, font=("TkDefaultFont", 8), command=self._on_avail)
        self.avail_btn.pack(side="right", padx=6, pady=1)
        self.avail_lbl._fixed_theme_colors = (AVAIL_BG, "#000000")      # bandeau lilas : exempte du recoloriage du theme (RecalBoxDMD_themes._walk_and_apply)
        self.avail_btn._fixed_theme_colors = (AVAIL_BTN, "#000000")
        body = tk.Frame(p, bg=self.bg)
        body.pack(fill="both", expand=True, padx=10, pady=2)
        self._body = body

        left = tk.Frame(body, bg=self.bg)           # la liste prend toute la hauteur : les boutons d'action sont avec l'apercu, a droite
        left.pack(side="left", fill="both", expand=True)
        left._no_bg_slice = True
        treebox = tk.Frame(left, bg=self.bg)
        treebox.pack(side="top", fill="both", expand=True)
        self.tree = ttk.Treeview(treebox, columns=("name", "state"), show="headings", height=7, selectmode="extended")
        self.tree.heading("name", text=self.t("col_name"))
        self.tree.heading("state", text=self.t("col_state"))
        self.tree.column("name", width=420, anchor="w")
        self.tree.column("state", width=110, anchor="center", stretch=False)
        sb = tk.Scrollbar(treebox, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=sb.set)
        self.tree.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        for cat, col in STATE_COLORS.items():
            self.tree.tag_configure(cat, foreground=col)
        self.tree.tag_configure("hint", foreground="#7A7A7A")          # ligne d'attente « Chargement... » (voir _loading_hint)
        self.tree.tag_configure("hover", background="#CFE3FF")   # barre de survol : meme bleu que l'onglet GIFs de jeux
        self._hover_marked = None
        self.tree.bind("<<TreeviewSelect>>", lambda e: self._on_select())
        self._hover_iid, self._hover_job = None, None
        self.tree.bind("<Motion>", self._on_hover)         # l'apercu se remplit au SURVOL de la liste, sans attendre la selection
        self.tree.bind("<Leave>", self._on_hover_leave)
        self.source_lbl = tk.Label(p, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), anchor="w")

        right = tk.Frame(body, bg=self.bg)
        right.pack(side="left", fill="y", padx=(10, 0))
        right._no_bg_slice = True      # colonne DE DONNEES (apercus, legendes, boutons) : fond plein de la couleur du theme, jamais l'image decoupee derriere (voir apply_theme) ; le reste de l'onglet suit le theme
        self._data_col = right
        trow = tk.Frame(right, bg=self.bg, width=PV_W + 2, height=22)
        trow.pack(fill="x")
        trow.pack_propagate(False)       # taille figee : un nom long ne doit JAMAIS decaler les apercus (le titre est coupe en pixels, voir _set_title)
        self.title_lbl = tk.Label(trow, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 10, "bold"), anchor="w")
        self.title_lbl.pack(side="left")
        self.key_lbl = tk.Label(trow, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), anchor="e")
        self.key_lbl.pack(side="right")
        self.canvases = {}
        lb = self._label(right, "cur", font=("TkDefaultFont", 8, "bold"), anchor="w")      # l'EXISTANT (SD / Pi) et ce qu'il est : « Existant (sur la SD) — carte texte »
        lb.pack(anchor="w", fill="x", pady=(4, 0))
        self._cur_lbl = lb
        cv = tk.Canvas(right, width=PV_W, height=PV_H, bg="black", highlightthickness=1, highlightbackground="#888888")
        cv._no_bg_slice = True
        cv.pack(anchor="w")
        self.canvases["cur"] = cv
        self.nb = ttk.Notebook(right)           # Remplacant | Original : l'original est montre pour reinitialiser en connaissance de cause
        self.nb.pack(anchor="w", pady=(4, 0))
        for name, key in (("repl", "repl"), ("orig", "orig_tab")):
            fr = tk.Frame(self.nb, bg=self.bg)
            cv = tk.Canvas(fr, width=PV_W, height=PV_H, bg="black", highlightthickness=1, highlightbackground="#888888")
            cv._no_bg_slice = True
            cv.pack(padx=2, pady=2)
            self.canvases[name] = cv
            self.nb.add(fr, text=self.t(key))
        # legendes sous les apercus : tailles FIXES (2 lignes puis 1), rien ne se decale quelle que soit la longueur du texte
        self.note_repl = tk.Label(right, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), justify="left", anchor="nw", wraplength=PV_W, height=2)
        self.note_repl.pack(fill="x", anchor="w", pady=(2, 0))
        self.info_lbl = tk.Label(right, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), justify="left", anchor="nw", wraplength=PV_W, height=1)
        self.info_lbl.pack(fill="x", anchor="w")        # ligne « Original : … »
        try:
            bright = max(0.0, min(1.0, float(_pref_get("custom_led_brightness", "0.5"))))
        except ValueError:
            bright = 0.5
        self.bright_var = tk.DoubleVar(value=bright)
        self._bright_job = None
        brow = tk.Frame(right, bg=self.bg)
        brow.pack(fill="x", pady=(2, 0))
        self._label(brow, "led", font=("TkDefaultFont", 8)).pack(side="left")
        ttk.Scale(brow, from_=0.0, to=1.0, variable=self.bright_var, orient="horizontal", length=160, command=self._on_bright).pack(side="left", padx=6)
        # TOUS les boutons d'action du jeu selectionne, ensemble sous ses apercus : grille 2 x 2, memes dimensions
        act = tk.Frame(right, bg=self.bg)
        act.pack(fill="x", pady=(6, 0))
        act.columnconfigure(0, weight=1, uniform="act")
        act.columnconfigure(1, weight=1, uniform="act")
        self.pick_btn = self._button(act, "pick", self._on_pick, pady=3)
        self.pick_btn.grid(row=0, column=0, sticky="we", padx=(0, 3), pady=(0, 4))
        self.card_btn = self._button(act, "card", self._on_card, pady=3)        # carte texte du jeu SELECTIONNE, en un clic (passe en attente comme tout remplacant)
        self.card_btn.grid(row=0, column=1, sticky="we", padx=(3, 0), pady=(0, 4))
        self.validate_btn = self._button(act, "validate", self._on_validate, bg=self.bg_action, pady=3)
        self.validate_btn.grid(row=1, column=0, sticky="we", padx=(0, 3))
        # « Annuler » : un seul bouton dont le libelle et l'effet suivent la situation (voir _cancel_mode) ; pas dans _i18n : _update_buttons reecrit son texte
        self.cancel_btn = self._btn(act, "cancel_idle", self._on_cancel, pady=3)
        self.cancel_btn.grid(row=1, column=1, sticky="we", padx=(3, 0))

        foot = tk.Frame(p, bg=self.bg)
        # PIED (menu Outils + statut) empaquete en BAS et AVANT le corps : sans cela le corps (colonne d'apercus de ~460 px) prend toute la place et le pied, empaquete en dernier,
        # est rogne des que le bandeau « Comparaison avec la Recalbox » est affiche (constate le 2026-11-17 : bouton Outils invisible)
        foot.pack(side="bottom", fill="x", padx=10, pady=(3, 4), before=body)
        self._foot = foot
        self.tools_btn = tk.Menubutton(foot, text=self.t("tools"), bg="#FFFFFF", fg="black", activebackground="#E6E6E6", activeforeground="black", bd=2, relief="solid", padx=8, pady=3)
        self.tools_btn.pack(side="left")
        self._i18n.append((self.tools_btn, "tools"))
        self.tools_menu = tk.Menu(self.tools_btn, tearoff=0)
        self.tools_btn.configure(menu=self.tools_menu)
        self._tools = (("browse_logos", self._on_browse), ("cards", self._on_cards_bulk), ("batch", self._on_batch), ("restore", self._on_restore), ("backup", self._on_backup), ("restore_zip", self._on_backup_restore), None,
                       ("pk_export", self._on_pack_export), ("pk_import", self._on_pack_import))
        for it in self._tools:
            if it is None:
                self.tools_menu.add_separator()
            else:
                self.tools_menu.add_command(label=self._tool_label(it[0]), command=it[1])
        self.status_lbl = tk.Label(foot, text="", bg=self.bg, fg=self.fg, anchor="w", justify="left", wraplength=800)
        self.status_lbl.pack(side="left", padx=10, fill="x", expand=True)
        self.source_lbl.pack(in_=foot, side="right")
        self._update_buttons()
        p.bind("<Map>", lambda e: self.apply_theme() if e.widget is p else None)
    def set_wide(self, wide):
        """Le cadre Progression est retire (onglet agrandi) : le logo, le cafe et le Discord occupent le coin bas-droit de la fenetre ; le pied de l'onglet
        s'arrete avant eux (sinon le compteur de jeux a droite passerait dessous)."""
        self._foot.pack_configure(padx=(10, 330 if wide else 10))

    def apply_theme(self):
        """Le theme du toolkit est applique APRES la construction de l'onglet (couleurs lues ici = theme par defaut) : le theme ne regle pas la couleur du carre
        des cases a cocher, dont la coche prend la couleur du texte (blanche en theme sombre, donc invisible sur le carre blanc par defaut). Le carre
        prend donc la couleur de fond COURANTE du theme ; appele a chaque affichage de l'onglet et apres un changement de theme."""
        c = {}
        try:
            c = self.gui._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        self.bg = c.get("bg_main", self.bg)
        self.fg = c.get("fg_text", self.fg)
        self.bg_action = c.get("bg_button_action", self.bg_action)
        for cb in self._toggles:
            try:
                cb.configure(selectcolor=self.bg)
            except tk.TclError:
                pass

        def solid(w):
            """La colonne de donnees est pleine : cadres et libelles prennent la couleur COURANTE du theme (le theme ne decoupe pas son image dans un sous-arbre _no_bg_slice)."""
            for ch in w.winfo_children():
                try:
                    cls = ch.winfo_class()
                    if cls == "Frame":
                        ch.config(bg=self.bg)
                    elif cls == "Label":
                        ch.config(bg=self.bg, fg=self.fg)
                except tk.TclError:
                    pass
                solid(ch)
        dc = getattr(self, "_data_col", None)
        if dc is not None:
            try:
                dc.config(bg=self.bg)
            except tk.TclError:
                pass
            solid(dc)


    def apply_language(self):
        for w, key in self._i18n:
            try:
                w.configure(text=self.t(key))
            except tk.TclError:
                pass
        self.tree.heading("name", text=self.t("col_name"))
        self.tree.heading("state", text=self.t("col_state"))
        self._refresh_tools_labels()
        self.view.refresh()
        self.nb.tab(0, text=self.t("repl"))
        self.nb.tab(1, text=self.t("orig_tab"))
        self.system_combo["values"] = [self.t("all_sys")] + self.systems
        if self.system == ALL:
            self.system_var.set(self.t("all_sys"))
        self._refill()
        self._redraw()
        self._update_buttons()          # le libelle du bouton « Annuler » change avec la langue

    # ------------------------------------------------------------------ SD / sources
    # Cible choisie comme dans l'onglet « GIFs de jeux » : boutons radio Carte SD / Raspberry Pi (Wi-Fi), un champ (dossier de la carte ou IP du Pi), Parcourir, Detecter.
    def refresh_drives(self, scan=True):
        """Au lancement (scan=False) : retrouve la cible memorisee sans balayer le reseau ; scan=True = bouton Detecter."""
        self._on_src_mode(initial=not scan)

    def follow_target(self, pi: bool):
        """Cible globale du toolkit (bandeau ESP32 / Pi) : la source suit la bascule ; le choix manuel dans cet onglet reste possible ensuite."""
        want = "pi" if pi else "sd"
        if self.src_mode.get() != want:
            self.src_mode.set(want)
            self._on_src_mode()

    def _on_src_mode(self, initial=False):
        pi = self.src_mode.get() == "pi"
        self._refresh_tools_labels()      # le libelle « Remettre en etat » depend de la source
        self.browse_sd_btn.configure(state="disabled" if pi else "normal")
        if initial and self.sd_root:
            return
        self.drive_var.set(_pref_get("custom_pi_ip") if pi else "")
        self._auto_target(scan=not initial, quiet=True)

    def _auto_target(self, scan=True, quiet=False):
        """Bouton Detecter / changement de cible. SD : la carte du DMD (comme le Mode 1) ; Pi : l'adresse saisie ou memorisee, puis balayage du reseau si scan."""
        if self.src_mode.get() == "pi":
            typed = self.drive_var.get().strip() or _pref_get("custom_pi_ip")

            def work_pi():
                found = pil.find_supported_pi(typed or None, scan=scan)
                if found:
                    pil.migrate_mirror(found["ip"], log=self._log)      # une seule fois : ancien miroir (%APPDATA%) -> dossier de l'application
                return found

            def done_pi(res, err):
                self._log(f"Cible Raspberry Pi {typed or '?'} (balayage {'oui' if scan else 'non'}) : " + (f"trouve {res['ip']} (demon {res.get('version') or '?'})" if (err is None and res) else
                                                                                                          (f"erreur {type(err).__name__}: {err}" if err is not None else "aucun Pi joignable / demon sans routes de logos")))
                if self.src_mode.get() != "pi":
                    return
                if err is None and res:
                    if self.pi_ip == res["ip"] and self.sd_root:
                        return
                    self._pi_info = res
                    self.drive_var.set(res["ip"])
                    _pref_set("custom_last_drive", "PI")
                    return self._select_pi(res)
                if not quiet:
                    messagebox.showinfo(self.t("sd"), self.t("no_pi_found"), parent=self.parent)
                else:
                    self.status_lbl.config(text=self.t("no_pi_found") if typed or scan else "")
            self.bg_run(self.parent, work_pi, done_pi)
            return

        def work_sd():
            return self.gui._find_dmd_sd_root()

        def done_sd(root, err):
            if self.src_mode.get() != "sd":
                return
            if err is not None or not root:
                last = _pref_get("custom_last_drive")
                if last and last != "PI" and os.path.isdir(last.rstrip("\\/") + "\\"):
                    root = last.rstrip("\\/") + "\\"
            if root:
                return self._use_sd_root(root)
            if not quiet:
                messagebox.showinfo(self.t("sd"), self.t("no_drive_found"), parent=self.parent)
            else:
                self.status_lbl.config(text=self.t("no_drive_found"))
        self.bg_run(self.parent, work_sd, done_sd)

    def _use_sd_root(self, root):
        """Carte SD (ou dossier qui en tient lieu) comme cible."""
        root = str(root)
        self.drive_var.set(root)
        _pref_set("custom_last_drive", root.rstrip("\\/"))
        if self.sd_root and not self.pi_ip and os.path.normcase(str(self.sd_root).rstrip("\\/")) == os.path.normcase(root.rstrip("\\/")):
            return          # deja la cible : pas de rechargement
        self._set_sd_root(root)

    def _apply_typed_target(self):
        """Entree dans le champ : le dossier tape (SD) ou l'adresse IP tapee (Pi)."""
        if self.src_mode.get() == "pi":
            return self._auto_target(scan=False)
        d = self.drive_var.get().strip()
        if d and os.path.isdir(d):
            self._use_sd_root(d)
        elif d:
            messagebox.showinfo(self.t("sd"), self.t("bad_folder", path=d), parent=self.parent)

    def _select_pi(self, info):
        """Le Raspberry Pi comme « carte SD » : son miroir local (forme d'une carte, dans le dossier de l'application : pi_mirror/<ip>/) sert de racine ; les logos y sont copies depuis le Pi."""
        ip = info["ip"]
        _pref_set("custom_pi_ip", ip)
        root = pil.mirror_root(ip)
        try:
            root.mkdir(parents=True, exist_ok=True)
        except OSError as e:
            return messagebox.showerror(self.t("pi_title"), self.t("err", err=e), parent=self.parent)
        self._set_sd_root(str(root), pi_ip=ip)

    def _pick_sd_folder(self):
        d = filedialog.askdirectory(parent=self.parent, title=self.t("folder"))
        if d:
            self._use_sd_root(d)          # dossier choisi a la main : plus de Pi comme source

    def _pick_roms(self):
        d = filedialog.askdirectory(parent=self.parent, title=self.t("roms"), initialdir=self.roms_var.get() or None)
        if d:
            self.roms_var.set(d)
            self._reload()

    def _set_cur_label(self):
        """Titre de l'apercu « actuel » : « Sur la SD » ou « Sur le Pi »."""
        key = "cur_pi" if self.pi_ip else "cur"
        if self._cur_lbl is not None:
            self._i18n = [(w, (key if w is self._cur_lbl else k)) for w, k in self._i18n]
            try:
                self._cur_lbl.configure(text=self.t(key))
            except tk.TclError:
                pass

    def _set_sd_root(self, root, pi_ip=None):
        self.sd_root = root
        self.pi_ip = pi_ip
        self._pi_synced = set()
        self._pi_lists = {}
        self._rows_cache = {}        # autre cible = autres lignes
        self._pi_note = ""
        self._set_cur_label()
        self.store_ok = True
        try:
            self.store = cl.merge_stores(cl.load_store(cl.store_path(root)), cl.load_store(cl.mirror_path()))
        except ValueError as e:
            self.store, self.store_ok = cl.new_store(), False
            messagebox.showerror(self.t("sd"), self.t("sd_bad_store", err=e), parent=self.parent)
        self.snap = cl.load_snapshot(cl.snapshot_path(root))
        self._refresh_systems()
        if pi_ip:
            self._pi_fetch_snapshot(pi_ip, root)

    def _pi_fetch_snapshot(self, ip, root):
        """Pi en Wi-Fi : recupere l'instantane du Pi (demon >= 3.81, GET /games-snapshot) pour reconnaitre ses cartes texte ; en arriere-plan, silencieux si le demon n'a pas
        la route (le Pi reste lu comme avant : « Logo dispo » seulement pour un jeu SANS visuel)."""
        def work():
            return pil.fetch_snapshot(ip, root)

        def done(res, err):
            if err is not None or res != "ok" or self.pi_ip != ip:
                return
            self.snap = cl.load_snapshot(cl.snapshot_path(root))
            self._refresh_systems()
        self.bg_run(self.parent, work, done)

    def _reload(self):
        _pref_set("custom_roms_dir", self.roms_var.get().strip())
        self._refresh_systems(offer=True)

    def _sys_arg(self):
        """Valeur du menu Systeme : ALL pour « Tous les systemes », sinon le nom du systeme."""
        v = self.system_var.get()
        return ALL if v == self.t("all_sys") else v

    def _sys_label(self):
        return self.t("all_sys") if self.system == ALL else self.system

    def _refresh_systems(self, offer=False):
        """Menu Systeme = dossiers de la SD + systemes du dossier ROMs (sous-dossiers avec un gamelist.xml, ou le dossier lui-meme s'il est un systeme).
        Le dossier ROMs suffit : sans SD (ou avec une SD vide) on peut deja parcourir ses jeux, seules les personnalisations demandent la SD.
        offer : au clic sur Charger, propose de creer l'instantane des systemes d'une ancienne SD qui n'en ont pas (voir _offer_snapshot)."""
        roms, sd, keep = self.roms_var.get().strip(), self.sd_root, self.system
        self._token += 1
        token = self._token
        self.stats_lbl.config(text=self.t("loading"))

        last = {"t": 0.0}

        def step(text, force=True):
            """Thread de fond -> libelle d'etat (au plus ~5 mises a jour par seconde) : le chargement n'est plus un « Chargement… » fige de plusieurs secondes."""
            now = time.time()
            if not force and now - last["t"] < 0.2:
                return
            last["t"] = now
            try:
                self.parent.after(0, lambda: token == self._token and (self.stats_lbl.config(text=text), self._loading_hint(text)))      # libelle + ligne grise dans la liste
            except Exception:   # noqa: BLE001
                pass

        def work():
            if roms:
                step(self.t("step_roms"))
            sd_systems, roms_map = (cl.list_systems_sd(sd) if sd else []), (cl.roms_systems(roms) if roms else {})
            if self.pi_ip and sd:
                # PI : « systeme du Pi » = un systeme QUE LE PI POSSEDE (present dans son instantane custom/games.json, ou dont le miroir contient deja des logos lus sur lui). Avant
                # (2026-11-17), tous les systemes du dossier ROMs etaient crees vides dans le miroir et comptes comme « du Pi » : sur le Pi de banc (3 systemes sur 88) « Tous les
                # systemes » montrait 70 936 jeux dont 18 389 « sans visuel » et 49 577 « Logo dispo » qui ne correspondaient a rien sur le Pi, et « Cartes » en aurait propose des
                # milliers (le demon refuse un systeme inconnu). Les autres systemes restent proposes dans le menu en LECTURE SEULE (off_sd).
                snap_systems = {s.lower() for s in self.snap["systems"]}
                # demon >= 3.91 : /logos-list donne, par systeme, « le Pi le possede-t-il ? » et la LISTE exacte de ses logos en un seul aller-retour -> etat exact sans lire jeu par jeu
                cands = sorted(set(roms_map) | set(cl.list_systems_sd(sd)) | set(self.snap["systems"]) | set(pil.KNOWN_SYSTEMS))      # + noms connus : sans dossier de ROMs ni miroir (version installee)
                lists = pil.list_many(self.pi_ip, cands, progress=lambda i, n: step(self.t("step_pi", done=i, total=n, pct=int(100 * i / max(n, 1))), force=(i >= n)))
                self._pi_lists = lists
                if lists:
                    sd_systems = [s for s in cands if lists.get(s, (False,))[0]]
                else:       # demon plus ancien : regle de repli (instantane du Pi, ou logos deja lus dans le miroir)
                    sd_systems = [s for s in cl.list_systems_sd(sd) if s.lower() in snap_systems or cl.existing_logos(sd, s)]
            missing = cl.snapshot_missing_systems(sd, sd_systems, roms_map, self.snap) if (offer and sd and roms_map) else []
            return sd_systems, roms_map, missing

        def done(res, err):
            if token != self._token:
                return
            sd_systems, roms_map, missing = res if err is None else ([], {}, [])
            self.sd_systems, self.roms_map = set(sd_systems), roms_map
            self.systems = sorted(self.sd_systems | set(roms_map), key=str.lower)
            self._rows_cache = {}        # liste des systemes / dossier ROMs / listes du Pi rechargees : les lignes memorisees peuvent etre perimees
            self.system_combo["values"] = [self.t("all_sys")] + self.systems
            pick = keep if (keep == ALL or keep in self.systems) else (ALL if self.systems else "")      # ouverture de l'onglet : « Tous les systemes » par defaut
            self.system_var.set(self.t("all_sys") if pick == ALL else pick)
            if err is not None:
                self.status_lbl.config(text=self.t("err", err=err))
            elif roms and not roms_map:
                self.status_lbl.config(text=self.t("roms_none", path=roms))
            if missing and self._offer_snapshot(missing, pick):
                return          # la creation recharge la liste elle-meme
            self._load_system(pick, note=err is None and not (roms and not roms_map))
            self._bg_fill_start()        # Pi : peuple le miroir en arriere-plan
        self.bg_run(self.parent, work, done)

    # ---- Pi : miroir peuple en arriere-plan + logo recupere au survol / a la selection
    def _bg_fill_start(self):
        """Pi (demon >= 3.91, listes /logos-list connues) : telecharge en arriere-plan, 2 a la fois, les logos que le Pi possede et que le miroir n'a pas encore ; le systeme affiche passe en premier.
        Redemarre (jeton) a chaque changement de cible / de liste ; le toolkit reste utilisable pendant ce temps (aperçus : voir _ensure_logo)."""
        self._bg_token = getattr(self, "_bg_token", 0) + 1
        token = self._bg_token
        if not (self.pi_ip and self._pi_lists and self.sd_root):
            return
        ip, root, lists = self.pi_ip, self.sd_root, dict(self._pi_lists)
        first = self.system if self.system in lists else None
        self._bg_first = first
        status = {"total": 0, "done": 0}

        def prog():
            try:
                self.parent.after(0, lambda: self.source_lbl.config(text=self.t("pi_bg", done=status["done"], total=status["total"])) if self._bg_token == token else None)
            except Exception:   # noqa: BLE001
                pass

        def work():
            from concurrent.futures import ThreadPoolExecutor
            order = ([first] if first else []) + [s for s in sorted(lists) if s != first]
            todo = []
            for s in order:
                has, st, _an = lists[s]
                if not has or self._bg_token != token:
                    continue
                local = cl.existing_logos(root, s)
                todo += [(s, stem) for stem in sorted(st) if stem not in local and pil.name_ok(stem)]
            status["total"] = len(todo)
            lock = threading.Lock()

            def one(it):
                if self._bg_token != token:
                    return
                s, stem = it
                raw = pil.get_logo(ip, s, stem)
                if raw and len(raw) == cl.RAW_SIZE:
                    pil._atomic_write(cl.target_path(root, f"{s}/{stem}"), raw)
                with lock:
                    status["done"] += 1
                    n = status["done"]
                if n % 40 == 0:
                    prog()
            with ThreadPoolExecutor(max_workers=2) as ex:
                list(ex.map(one, todo))
            return status["total"]

        def done(res, err):
            if self._bg_token != token:
                return
            self.source_lbl.config(text="")
            if err is None and res:
                self._log(f"Miroir du Pi peuple en arriere-plan : {res} logo(s) lu(s)")
                self._load_system(self.system, note=False)        # les apercus des jeux lus apparaissent
        self.bg_run(self.parent, work, done)

    def _ensure_logo(self, r):
        """Ligne « OK » dont le logo est sur le Pi (liste /logos-list) mais pas encore dans le miroir : on le recupere TOUT DE SUITE (apercu au survol / a la selection), sans attendre le
        remplissage de fond."""
        if not (r and self.pi_ip and not r.get("path") and r.get("state") == "ok"):
            return
        lst = self._pi_lists.get(r["system"])
        stem = r["stem"]
        if not (lst and stem.lower() in lst[1] and pil.name_ok(stem)):
            return
        fetching = self.__dict__.setdefault("_fetching", set())
        if r["key"] in fetching:
            return
        fetching.add(r["key"])
        ip, root = self.pi_ip, self.sd_root

        def work():
            raw = pil.get_logo(ip, r["system"], stem)
            if not raw or len(raw) != cl.RAW_SIZE:
                return None
            p = cl.target_path(root, r["key"])
            pil._atomic_write(p, raw)
            return p

        def done(p, err):
            fetching.discard(r["key"])
            if err is not None or p is None or self.pi_ip != ip:
                return
            r["path"] = p
            sel = getattr(self, "row", None)
            if sel is not None and sel["key"] == r["key"]:
                self._redraw()
            elif self._hover_iid and self.by_iid.get(self._hover_iid) is r:
                self._apply_hover()
        self.bg_run(self.parent, work, done)

    def _offer_snapshot(self, missing, pick):
        """SD scrapee avant l'instantane : propose de le creer depuis le gamelist.xml du dossier ROMs (lu maintenant, pas celui du scrape). True si accepte."""
        if not messagebox.askyesno(self.t("sd"), self.t("snap_ask", n=len(missing)), parent=self.parent):
            return False
        self.stats_lbl.config(text=self.t("loading"))
        sd, roms_map = self.sd_root, dict(self.roms_map)

        def done(res, err):
            msg = self.t("err", err=err) if err is not None else self.t("snap_done", n=len(res))
            if err is None:
                self.snap = cl.load_snapshot(cl.snapshot_path(sd))
                self._log(msg)
            self._load_system(pick, note=False)
            self.status_lbl.config(text=msg)
        self.bg_run(self.parent, lambda: cl.convert_snapshot(sd, roms_map, missing, log=self._log), done)
        return True

    # ------------------------------------------------------------------ liste des jeux
    def _pi_sync(self, system, games):
        """Hors thread d'interface : copie dans le miroir les logos du Pi pour ces jeux (pi_logos.sync_system). Le systeme n'est memorise comme lu que si tout a repondu."""
        non_game = cl._non_game_fn()
        stems = []
        for g in games:
            stem = g.get("stem") or cl.stem_for_rom(g["rom"])
            if not non_game(g["name"], stem, g.get("hidden", False)):
                stems.append(stem)
        ip = self.pi_ip

        def prog(done, total):
            try:
                self.parent.after(0, lambda: self.stats_lbl.config(text=self.t("pi_reading", ip=ip, done=done, total=total)))
            except Exception:   # noqa: BLE001
                pass
        lst = self._pi_lists.get(system)
        rep = pil.sync_system(ip, self.sd_root, system, stems, progress_cb=prog, present=(lst[1] if lst else None))
        if rep["errors"]:
            self._pi_note = self.t("pi_read_ko", ip=ip, e=rep["errors"])
        else:
            self._pi_synced.add(system)
            self._pi_note = self.t("pi_read_done", ip=ip, n=rep["present"], d=rep["downloaded"])

    def _rows_for(self, system, sync=True):
        """(lignes, source, infos) d'UN systeme -- hors thread d'interface. Source, par ordre de preference : l'instantane du dernier Mode 1 (sur la SD, coherent avec
        les images qu'elle contient), le gamelist.xml du dossier ROMs, sinon les visuels presents sur la SD. Avec l'instantane, le gamelist.xml sert a comparer."""
        info = {}
        gl_games = None
        if system in self.roms_map:
            try:
                gl_games = cl.games_from_gamelist(self.roms_map[system])
            except Exception:   # noqa: BLE001
                gl_games = None
        snap_games = cl.games_from_snapshot(self.snap, system) if self.sd_root else None
        if snap_games is not None:
            games, source = snap_games, "snap"
            info["date"] = self.snap["systems"][system]["date"]
            info["extras"] = len(cl.snapshot_extras(self.sd_root, system, games, self.store))
            # la liste est celle de l'EXISTANT (le scrape de la SD) ; la Recalbox ne sert qu'a reperer ses LOGOS (rb_logos, plus bas)
        elif gl_games is not None:
            games, source = gl_games, "gl"
        else:
            games, source = (cl.games_from_sd(self.sd_root, system) if self.sd_root else []), "sd"
        if (self.pi_ip and sync and system != ALL and system not in self._pi_synced and system in self.sd_systems      # jamais de lecture d'un systeme que le Pi n'a pas (des milliers de requetes pour rien)
                and not self._pi_lists.get(system)):
            # SANS la liste du demon (< 3.91) : lecture jeu par jeu AVANT l'affichage (bloquant, plusieurs secondes). AVEC la liste (>= 3.91) : l'etat vient de la liste, les apercus sont
            # recuperes au survol (_ensure_logo) et par le remplissage de fond (_bg_fill_start) : la liste s'affiche tout de suite.
            self._pi_sync(system, games)
        rb = None
        if system in self.roms_map and self.sd_root:
            try:
                rb = cl.rb_logos(self.roms_map[system], self._rb_tag())      # logos que la Recalbox possede aujourd'hui (balise du profil du Mode 1)
            except Exception:   # noqa: BLE001
                rb = None
        rows = cl.system_rows(self.sd_root, system, games, self.store, rb)
        lst = self._pi_lists.get(system) if self.pi_ip else None
        if lst:
            # Pi : la liste du demon (/logos-list) FAIT FOI sur l'existence d'un logo, sans lire le Pi jeu par jeu ni remplir le miroir : un jeu qui y figure est « OK » meme si son fichier n'est
            # pas encore dans le miroir (apercu vide jusqu'a l'ouverture du systeme) ; un fichier du miroir que le Pi n'a plus est perime
            present = lst[1] | lst[2]
            for r in rows:
                if r["state"] == "custom":
                    continue
                on_pi = r["stem"].lower() in present
                if on_pi and r["state"] in ("missing", "avail"):
                    r["state"] = "ok"
                elif not on_pi and r["state"] in ("ok", "card", "anim"):
                    r["state"], r["path"], r["was_card"] = ("avail" if r.get("rb_logo") else "missing"), None, False
        if system not in self.sd_systems:
            for r in rows:
                r["off_sd"] = True
        return rows, source, info

    @staticmethod
    def _rb_tag():
        """Balise du gamelist.xml qui porte le logo, selon le profil Recalbox choisi au Mode 1 (« logo » / « thumbnail » / « image »)."""
        tags = {"11.x-10.x": "logo", "9.x": "thumbnail", "legacy": "image"}
        return tags.get(_pref_get("recalbox_profile", "11.x-10.x"), "logo")

    def compute_rows(self, system, progress=None):
        """(lignes, source, infos) -- hors thread d'interface. ALL = tous les systemes du menu, agreges (chaque ligne garde son systeme). progress(fait, total, lignes_deja_calculees) :
        appele apres chaque systeme de « Tous les systemes » (le thread d'interface en fait un pourcentage et un affichage PROGRESSIF)."""
        if system != ALL:
            return self._rows_for(system)
        rows, tot = [], {"systems": 0, "snap": 0, "extras": 0, "new": 0, "gone": 0, "cmp": False}
        todo = [s for s in list(self.systems) if not (self.pi_ip and s not in self.sd_systems)]      # Pi : « Tous les systemes » = ceux que le Pi possede (les autres : voir _refresh_systems)
        for i, s in enumerate(todo, 1):
            r, src, inf = self._rows_for(s, sync=False)      # « Tous les systemes » : jamais de lecture du Pi jeu par jeu (des milliers de requetes) -- liste du demon + miroir
            rows += r
            if progress:
                progress(i, len(todo), rows)
            tot["systems"] += 1
            tot["snap"] += src == "snap"
            for k in ("extras", "new", "gone"):
                tot[k] += inf.get(k, 0)
            tot["cmp"] = tot["cmp"] or inf.get("cmp", False)
        rows.sort(key=lambda r: (r["name"].lower(), r["system"]))
        return rows, "all", tot

    def _notes(self, source, info, n):
        """(texte du pied de liste, texte de statut) d'un chargement."""
        if source == "snap":
            foot = self.t("source_snap", n=n, date=info.get("date", "")[:10])
        elif source == "all":
            foot = self.t("source_all", n=n, s=info.get("systems", 0))
        else:
            foot = self.t("source_gl", n=n) if source == "gl" else ""
        msgs = []
        if not self.sd_root:
            msgs.append(self.t("no_sd_states"))
        elif source == "sd":
            msgs.append(self.t("source_sd", n=n))
        if self.pi_ip and self._pi_note:
            msgs.append(self._pi_note)
        if self.pi_ip and source == "sd":
            msgs.append(self.t("pi_need_roms"))
        if info.get("extras"):
            msgs.append(self.t("snap_extra", n=info["extras"]))
        if info.get("cmp"):
            msgs.append(self.t("snap_cmp", new=info["new"], gone=info["gone"]) if info["new"] or info["gone"] else self.t("snap_cmp_ok"))
        return foot, " · ".join(msgs)

    @staticmethod
    def _rows_sig(rows):
        """Signature d'une liste de lignes (cle, etat, fichier present?) : sert a savoir si le recalcul de fond a change quelque chose."""
        return tuple((r["key"], r["state"], bool(r.get("path"))) for r in rows)

    def _load_system(self, system, note=True, use_cache=False):
        """Charge la liste. note=False apres une action (le statut qu'elle vient d'afficher ne doit pas etre ecrase). use_cache (choix d'un systeme dans le menu) : un systeme DEJA VISITE
        s'affiche tout de suite depuis le cache de lignes, puis il est recalcule en arriere-plan ; si rien n'a change l'affichage (et la selection) ne bouge pas. Les actions (valider, annuler,
        importer...) rechargent SANS cache : leurs lignes doivent etre a jour."""
        self.system = system
        self._token += 1
        token = self._token
        cached = self._rows_cache.get(system) if (use_cache and system) else None
        self.rows, self.by_iid = [], {}
        self.tree.delete(*self.tree.get_children())
        self._show_row(None)
        if not system:
            self.stats_lbl.config(text="")
            self.source_lbl.config(text="")
            return
        if cached:
            self.rows = cached[0]
            foot, _status = self._notes(cached[1], cached[2], len(self.rows))
            self.source_lbl.config(text=foot)
            self._refill()
        else:
            self.stats_lbl.config(text=self.t("loading"))
            self._loading_hint(self.t("loading_row"))      # ligne d'attente DANS la liste : un cadre blanc muet pendant plusieurs secondes laissait croire a un blocage
        last = {"t": 0.0}
        finished = {"v": False}

        def on_progress(i, n, rows_so_far):
            """Thread de fond : au plus un affichage progressif par seconde et demie (reconstruire la liste a chaque systeme couterait plus que le calcul)."""
            now = time.time()
            if i < n and now - last["t"] < 1.5:
                return
            last["t"] = now
            snap = sorted(rows_so_far, key=lambda r: (r["name"].lower(), r["system"]))
            try:
                self.parent.after(0, lambda: partial(i, n, snap))
            except Exception:   # noqa: BLE001
                pass

        def partial(i, n, snap):
            if token != self._token or finished["v"]:
                return          # chargement remplace ou deja termine : un affichage partiel tardif ne doit pas ecraser le resultat final
            self.rows = snap
            self._refill()                                  # les premiers systemes s'affichent deja (le reste arrive au fur et a mesure)
            self.stats_lbl.config(text=self.t("loading_pct", done=i, total=n, pct=int(100 * i / max(n, 1)), games=len(snap)))
            self._loading_hint(self.t("loading_row_pct", done=i, total=n, pct=int(100 * i / max(n, 1))))

        def done(res, err):
            if token != self._token:
                return
            finished["v"] = True
            if err is not None:
                self.status_lbl.config(text=self.t("err", err=err))
                return
            rows, source, info = res
            if cached and self._rows_sig(rows) == self._rows_sig(cached[0]):
                self._rows_cache[system] = (cached[0], source, info)      # rien n'a change : on garde l'affichage, la selection et les drapeaux deja calcules (noir...)
                return
            self._rows_cache[system] = res
            self.rows = rows
            foot, status = self._notes(source, info, len(rows))
            self.source_lbl.config(text=foot)
            if note and status:
                self.status_lbl.config(text=status)
            self._refill()
            self._start_dark_scan(token)
            if self._select_after_load:
                self.select_key(self._select_after_load)
                self._select_after_load = None
            if self.pi_ip and self._pi_lists and system != ALL and system != getattr(self, "_bg_first", None):
                self._bg_fill_start()                       # le systeme choisi passe en tete du remplissage de fond
        self.bg_run(self.parent, lambda: self.compute_rows(system, progress=on_progress if (system == ALL and not cached) else None), done)

    def _loading_hint(self, text):
        """Ligne grise EN TETE de la liste (« Chargement... 23 / 88 systemes (26 %) ») tant qu'un chargement est en cours ; _refill la remplace par les vraies lignes."""
        for iid in self.tree.get_children():
            if "hint" in self.tree.item(iid, "tags"):
                self.tree.delete(iid)
        self.tree.insert("", 0, values=(text, ""), tags=("hint",))

    def _start_dark_scan(self, token):
        rows = list(self.rows)

        def work():
            # 1) le drapeau « carte texte » de l'instantane est verifie sur le fichier reel (un vrai logo a pu remplacer la carte depuis le scrape) ;
            # 2) les fausses cartes redevenues « ok » passent aussi le controle « noir »
            fake = {r["key"] for r, real in cl.scan_cards(rows) if not real}
            return fake, [r["key"] for r, dark in cl.scan_dark(rows, extra_keys=fake) if dark]

        def done(res, err):
            if token != self._token or err is not None:
                return
            fake, dk = res[0], set(res[1])
            n_fake = cl.apply_card_check(self.rows, fake)
            for r in self.rows:
                r["dark"] = r["key"] in dk
            if n_fake:
                self._log(f"[Etat] {n_fake} jeu(x) marque(s) « carte texte » par l'instantane ont en fait un vrai logo sur la cible : remis a « OK »")
            self._refill()
        self.bg_run(self.parent, work, done)

    def _state_text(self, r):
        cat = _row_category(r)
        if cat == "ok" and r["state"] == "anim":
            return self.t("st_anim")
        return self.t("st_" + cat)

    def _refill_soon(self):
        """Filtre texte : rafraichit 200 ms apres la derniere frappe (la liste « Tous les systemes » est longue a reconstruire)."""
        job = getattr(self, "_refill_job", None)
        if job:
            try:
                self.parent.after_cancel(job)
            except tk.TclError:
                pass
        self._refill_job = self.parent.after(200, self._refill)

    def _refill(self):
        if not hasattr(self, "tree"):
            return
        sel = self.tree.selection()
        sel_key = self.by_iid.get(sel[0], {}).get("key") if sel else None
        flt = self.filter_var.get().strip().lower()
        shown = 0
        self.tree.delete(*self.tree.get_children())
        self.by_iid = {}
        self._hover_marked = None
        counts = {"custom": 0, "missing": 0, "dark": 0, "card": 0, "avail": 0}
        matched = 0
        # plusieurs ROM peuvent porter le MEME nom (versions, formats .chd / .cue : 3DO « Shock Wave » x2) et ont chacune leur logo : le ROM est affiche pour les distinguer
        same_name = {}
        for r in self.rows:
            same_name[(r["system"], r["name"].lower())] = same_name.get((r["system"], r["name"].lower()), 0) + 1
        for r in self.rows:
            cat = _row_category(r)
            if cat in counts:
                counts[cat] += 1
            if not self.view.shows(cat) or (flt and flt not in r["name"].lower() and flt not in r["stem"].lower() and flt not in r["system"].lower()):
                continue
            matched += 1
            if shown >= MAX_SHOWN:      # « Tous les systemes » : des dizaines de milliers de lignes ; le Treeview les insere une a une
                continue
            tag = "anim" if (cat == "ok" and r["state"] == "anim") else cat
            label = f"{r['name']}   ({r['system']})" if self.system == ALL else r["name"]
            if same_name[(r["system"], r["name"].lower())] > 1:
                rom = Path(r["rom"]).stem
                label += f"   [{rom if len(rom) <= 44 else rom[:41] + '…'}]"
            iid = self.tree.insert("", "end", values=(label, self._state_text(r)), tags=(tag,))
            self.by_iid[iid] = r
            shown += 1
            if r["key"] == sel_key:
                self.tree.selection_set(iid)
                self.tree.see(iid)
        if matched > shown:
            self.status_lbl.config(text=self.t("cap", shown=shown, total=matched))
        self.view.refresh(_ViewFilter.count(self.rows))        # effectifs a jour dans le menu « Afficher »
        self._update_avail_bar(counts["avail"])
        self.stats_lbl.config(text=self.t("stats", shown=shown, custom=counts["custom"], missing=counts["missing"], dark=counts["dark"], card=counts["card"], avail=counts["avail"]) if self.rows else "")

    def _update_avail_bar(self, n):
        """Bandeau de comparaison avec la Recalbox : visible seulement s'il y a des jeux de la SD (carte texte / sans visuel) dont la Recalbox possede un logo."""
        if n and self.sd_root and self.rows:
            self.avail_lbl.config(text=self.t("av_bar", n=n))
            self.avail_btn.config(text=self.t("av_btn"))
            if not self.avail_bar.winfo_ismapped():
                self.avail_bar.pack(fill="x", padx=10, pady=(2, 0), before=self._body)
        else:
            self.avail_bar.pack_forget()

    def _on_avail(self):
        if not (self.sd_root and self.store_ok):
            return
        NewLogosWindow(self)

    def select_key(self, key):
        for iid, r in self.by_iid.items():
            if r["key"].lower() == key.lower():
                self.tree.selection_set(iid)
                self.tree.see(iid)
                return True
        return False

    def goto(self, system, stem):
        """Appelee par la fenetre de parcours : selectionne le jeu (change de systeme si besoin)."""
        key = f"{system}/{stem}"
        if self.system != ALL and system != self.system:
            self.system_var.set(system)
            self._select_after_load = key
            self._load_system(system)
        elif not self.select_key(key):
            self.view.reset()           # le jeu est masque par le menu « Afficher » : on revient a « Tous les jeux »
            self.filter_var.set("")
            self._refill()
            self.select_key(key)

    # ------------------------------------------------------------------ selection / apercus
    def _selected(self):
        """Le jeu selectionne quand UN SEUL l'est (None si aucun ou plusieurs : voir _selected_rows)."""
        sel = self.tree.selection()
        return self.by_iid.get(sel[0]) if len(sel) == 1 else None

    def _selected_rows(self):
        return [self.by_iid[i] for i in self.tree.selection() if i in self.by_iid]

    def _on_select(self):
        self.pending = None
        rows = self._selected_rows()
        self._multi = rows if len(rows) > 1 else []
        if self._multi:           # selection multiple : pas d'apercu, les actions de groupe (carte texte, annuler) s'appliquent a tous
            self._show_row(None)
            self._set_title(self.t("multi_title", n=len(rows)))
            self.status_lbl.config(text=self.t("multi_hint"))
            return
        r = self._selected()
        if r and _is_avail(r) and self.sd_root and self.store_ok and not r.get("off_sd"):
            try:      # la Recalbox a un logo pour ce jeu : le remplacant est deja pret, il ne reste qu'a valider
                self.pending = cl.make_best_item(r["system"], r["rom"], r["rb_logo"], name=r["name"], mode="pixel", bg=(0, 0, 0), stem=r["stem"])
            except Exception:   # noqa: BLE001
                self.pending = None
        self._show_row(r)

    def _show_row(self, row):
        self.row = row
        if row is None:
            self._set_title()
        else:
            self._set_title(row["name"], row["key"])
            if row.get("off_sd"):
                self.status_lbl.config(text=self.t("off_sd"))
            self._ensure_logo(row)
        self._redraw()
        self._update_buttons()

    def _current_image(self):
        r = getattr(self, "row", None)
        return cl.read_logo_image(r["path"]) if r and r.get("path") else None

    def _replacement_image(self):
        r = getattr(self, "row", None)
        if not r:
            return None, None
        if self.pending:
            return self.pending[2], self.pending
        it = cl.find_item(self.store, r["key"])
        return (cl.png_b64_to_image(it["png_b64"]), None) if it else (None, None)

    def _draw(self, name, img):
        cv = self.canvases[name]
        cv.delete("all")
        if img is None:
            cv.create_text(PV_W // 2, PV_H // 2, text=self.t("none"), fill="#888888", font=("TkDefaultFont", 16))
            return
        ph = _photo(cl.led_style(img, PV_SCALE, self.bright_var.get()))
        self._photos[name] = ph
        cv.create_image(0, 0, image=ph, anchor="nw")

    def _anim_start(self, item):
        """Joue l'animation d'une entree dans le cadre « Remplaçant » (images rendues a la demande, delai >= 100 ms : le rendu LED prend du temps)."""
        self._anim_token = getattr(self, "_anim_token", 0) + 1
        if not (item and cl.is_anim(item)):
            return
        try:
            files = dict(cl.item_files(item))
        except ValueError:
            return
        pack, delays, token = files[".raw565pack"], item["delays"], self._anim_token
        cache = {}

        def step(i):
            if token != self._anim_token:
                return
            try:
                if i not in cache:
                    cache[i] = _photo(cl.led_style(cl.raw565_to_image(pack[i * cl.RAW_SIZE:(i + 1) * cl.RAW_SIZE]), PV_SCALE, self.bright_var.get()))
                self._photos["repl"] = cache[i]
                cv = self.canvases["repl"]
                cv.delete("all")
                cv.create_image(0, 0, image=cache[i], anchor="nw")
                self.parent.after(max(100, delays[i]), lambda: step((i + 1) % len(delays)))
            except tk.TclError:
                pass

        self.parent.after(max(100, delays[0]), lambda: step(1))

    def _anim_cur_start(self, path):
        """Aperçu « Existant » : si le visuel present sur la SD / le Pi est une ANIMATION (.raw565pack), il joue en boucle comme le remplaçant (sinon on n'en voyait que la 1re image)."""
        import struct
        self._anim_cur_token = getattr(self, "_anim_cur_token", 0) + 1
        if not path or not str(path).lower().endswith(".raw565pack"):
            return
        token = self._anim_cur_token
        try:
            data = Path(path).read_bytes()
            n = len(data) // cl.RAW_SIZE
            if n < 2 or len(data) % cl.RAW_SIZE:
                return
            delays = [100] * n
            meta = Path(path).with_suffix(".meta")
            if meta.is_file():
                raw = meta.read_bytes()
                if len(raw) == 2 * n:
                    delays = [max(1, d) for d in struct.unpack(f"<{n}H", raw)]
        except OSError:
            return
        cache = {}

        def step(i):
            if token != self._anim_cur_token:
                return
            try:
                if i not in cache:
                    cache[i] = _photo(cl.led_style(cl.raw565_to_image(data[i * cl.RAW_SIZE:(i + 1) * cl.RAW_SIZE]), PV_SCALE, self.bright_var.get()))
                self._photos["cur_anim"] = cache[i]
                cv = self.canvases["cur"]
                cv.delete("all")
                cv.create_image(0, 0, image=cache[i], anchor="nw")
                self.parent.after(max(100, delays[i]), lambda: step((i + 1) % n))
            except tk.TclError:
                pass

        self.parent.after(max(100, delays[0]), lambda: step(1))

    def _on_bright(self, _v=None):
        """Curseur de luminosite LED (comme DMD GIF Creator) : redessin differe tant que le curseur bouge, preference memorisee au relachement."""
        if self._bright_job is not None:
            self.parent.after_cancel(self._bright_job)

        def go():
            self._bright_job = None
            _pref_set("custom_led_brightness", f"{self.bright_var.get():.2f}")
            self._redraw()
            picker = getattr(self, "picker", None)
            if picker is not None:
                try:
                    picker.refresh_preview()
                except tk.TclError:
                    self.picker = None
        self._bright_job = self.parent.after(90, go)

    def _set_title(self, name="", key=""):
        """Titre et cle au-dessus des apercus, coupes EN PIXELS (ellipse) : la zone garde une taille fixe quel que soit le nom survole."""
        self.title_lbl.config(text=_clip_px(self.title_lbl, name, PV_W - 150))
        self.key_lbl.config(text=_clip_px(self.key_lbl, key, 140))

    def _cur_caption(self, r):
        """Legende de l'aperçu « Existant » : « Existant (sur la SD) — carte texte » : ce que le DMD affiche pour ce jeu."""
        base = self.t("cur_pi" if self.pi_ip else "cur")
        if not r:
            return base
        cat = _row_category(r)
        if cat == "avail":
            what = "ex_dark" if r.get("dark") else ("ex_card" if r.get("was_card") else "ex_none")
        elif cat == "ok":
            what = "ex_anim" if r.get("state") == "anim" else "ex_logo"
        else:
            what = {"card": "ex_card", "missing": "ex_none", "dark": "ex_dark", "custom": "ex_custom"}.get(cat, "ex_logo")
        return base + " — " + self.t(what)

    def _original_image(self, r):
        """(image, personnalise) : l'image d'avant toute personnalisation (celle que « Réinitialiser » rendrait) ; sans personnalisation, l'existant lui-meme."""
        it = cl.find_item(self.store, r["key"]) if r else None
        if it and it.get("orig_b64"):
            return cl.png_b64_to_image(it["orig_b64"]), True
        return (cl.read_logo_image(r["path"]) if r and r.get("path") else None), False

    def _redraw(self, row=None, hover=False):
        """Redessine les trois apercus (Existant, Remplacant, Original). hover : ligne SURVOLEE (ni selectionnee ni en attente : rien d'en attente n'est montre)."""
        r = row if row is not None else getattr(self, "row", None)
        pend = None if hover else self.pending
        self._draw("cur", cl.read_logo_image(r["path"]) if r and r.get("path") else None)
        self._anim_cur_start(r.get("path") if r else None)
        it = cl.find_item(self.store, r["key"]) if r else None
        if pend:
            repl = pend[2]
        elif it:
            repl = cl.png_b64_to_image(it["png_b64"])
        elif hover and r and _is_avail(r):      # seulement les jeux « Logo dispo » : la Recalbox possede un logo pour presque tous les jeux, ce n'est un remplacant que si l'existant est une carte / un logo noir
            repl = cl.preview_logo(r["rb_logo"])        # « Logo dispo » : le logo de la Recalbox, montre des le survol
        else:
            repl = None
        self._draw("repl", repl)
        orig, customised = self._original_image(r)
        self._draw("orig", orig)
        self._cur_lbl.config(text=self._cur_caption(r))
        # legendes SOUS les apercus (tailles fixes) : ce que montre chaque onglet, en clair (plus de « pixel · fond #000000 · source… »)
        if pend:
            item, rep, _img = pend
            src = item["src"] + (" · " + self.t("info_anim", n=len(item["delays"]), s=f"{sum(item['delays']) / 1000:.1f}") if cl.is_anim(item) else "")
            note = self.t("note_repl_rb", src=item["src"]) if (r and r.get("rb_logo") and Path(r["rb_logo"]).name == item["src"]) else self.t("note_repl", src=src)
            warns = [self.t("w_" + w) for w in rep["warnings"] if w != "transparent"]
            if warns:
                note += "\n⚠ " + warns[0]
        elif it:
            note = self.t("note_repl_saved", src=it.get("src", "?"))
        elif r and r.get("rb_logo") and _is_avail(r):
            note = self.t("note_repl_rb", src=Path(r["rb_logo"]).name)
        else:
            note = self.t("note_repl_none") if r else ""
        self.note_repl.config(text=note)
        self.info_lbl.config(text=(self.t("note_orig_kept") if customised else self.t("orig_same")) if r else "")
        self._anim_start(pend[0] if pend else it)

    def _on_hover(self, e):
        """Survol de la liste : apercu de la ligne sous le curseur (debounce 40 ms) ; hors ligne ou sur la selection : retour a la selection."""
        iid = self.tree.identify_row(e.y)
        if iid == self._hover_iid:
            return
        self._hover_iid = iid
        self._hover_mark(iid)          # la barre coloree suit tout de suite (comme dans l'onglet GIFs de jeux) ; l'apercu, lui, est debounce
        if self._hover_job:
            self.parent.after_cancel(self._hover_job)
        self._hover_job = self.parent.after(40, self._apply_hover)

    def _hover_mark(self, iid):
        """Deplace la barre coloree (etiquette « hover ») sur la ligne survolee ; iid vide = plus aucune. Les autres etiquettes (couleur d'etat) sont conservees."""
        old = getattr(self, "_hover_marked", None)
        if old == iid:
            return
        for k, add in ((old, False), (iid, True)):
            if not k:
                continue
            try:
                tags = [t for t in self.tree.item(k, "tags") if t != "hover"]
                self.tree.item(k, tags=tags + (["hover"] if add else []))
            except tk.TclError:
                pass                    # ligne disparue (liste reconstruite)
        self._hover_marked = iid or None

    def _on_hover_leave(self, _e):
        self._hover_iid = None
        self._hover_mark(None)
        if self._hover_job:
            self.parent.after_cancel(self._hover_job)
        self._hover_job = self.parent.after(40, self._apply_hover)

    def _apply_hover(self):
        self._hover_job = None
        r = self.by_iid.get(self._hover_iid) if self._hover_iid else None
        sel = getattr(self, "row", None)
        if r is None or (sel is not None and r["key"] == sel["key"]):
            self._set_title(sel["name"] if sel else "", sel["key"] if sel else "")
            self._redraw()
        else:
            self._set_title(r["name"], r["key"])
            self._ensure_logo(r)          # Pi : logo pas encore dans le miroir -> recupere au survol
            self._redraw(row=r, hover=True)
    def _update_buttons(self):
        multi = [r for r in getattr(self, "_multi", []) if not r.get("off_sd")]
        if getattr(self, "_multi", []):
            ok = bool(multi and self.sd_root and self.store_ok)
            self.pick_btn.config(state="disabled")
            self.validate_btn.config(state="disabled")
            self.card_btn.config(state="normal" if ok else "disabled")
            n_c = sum(1 for r in multi if cl.find_item(self.store, r["key"])) if ok else 0
            self.cancel_btn.config(text=self.t("cancel_multi", n=n_c), state="normal" if n_c else "disabled")
            return
        r = getattr(self, "row", None)
        has = r is not None and self.sd_root is not None and self.store_ok and not r.get("off_sd")
        self.pick_btn.config(state="normal" if has else "disabled")
        self.card_btn.config(state="normal" if has else "disabled")
        self.validate_btn.config(state="normal" if has and self.pending else "disabled")
        mode = self._cancel_mode()
        self.cancel_btn.config(text=self.t("cancel_" + (mode or "idle")), state="normal" if mode else "disabled")

    def _cancel_mode(self):
        """Ce que « Annuler » fait MAINTENANT pour le jeu selectionne : 'choice' = abandonner le remplacant choisi mais pas encore valide ; 'prev' = revenir a la
        personnalisation precedente (un niveau) ; 'orig' = rendre le logo d'origine sauvegarde ; 'del' = retirer la personnalisation (aucun original sauvegarde : le DMD
        retombe sur le logo par defaut) ; None = rien a annuler."""
        r = getattr(self, "row", None)
        if not (r and self.sd_root and self.store_ok and not r.get("off_sd")):
            return None
        if self.pending:
            return "choice"
        it = cl.find_item(self.store, r["key"])
        if it is None:
            return None
        if cl.has_previous(it):
            return "prev"
        return "orig" if (it.get("orig_b64") or it.get("orig_pack_z")) else "del"

    # ------------------------------------------------------------------ actions
    def _no_target(self):
        """Aucune cible connectee (sd_root vide) : message ADAPTE au mode choisi (en mode Pi, « choisissez la carte SD » etait faux et trompeur) et, en mode Pi, nouvelle tentative de connexion
        en arriere-plan (adresse saisie ou memorisee) pour que l'action suivante trouve le Pi."""
        if self.src_mode.get() == "pi":
            self._auto_target(scan=True, quiet=True)
            return messagebox.showinfo(self.t("sd"), self.t("no_pi_connected"), parent=self.parent)
        return messagebox.showinfo(self.t("sd"), self.t("no_sd"), parent=self.parent)

    def _on_pick(self):
        if not self.sd_root:
            return self._no_target()
        if getattr(self, "row", None) is None:
            return messagebox.showinfo(self.t("sd"), self.t("no_sel"), parent=self.parent)
        ReplacementWindow(self, self.row)

    def _open_legacy_picker(self):
        """Source « Fichier, dossier, presse-papiers » : le selecteur d'image historique (miniatures d'un dossier, coller, modes d'adaptation, controles)."""
        self.picker = ImagePicker(self, self.row, self._on_picked)

    def _set_pending(self, tup):
        self.pending = tup
        self._redraw()
        self._update_buttons()

    def _on_raw_picked(self, cand):
        """Remplacant deja au format de la dalle (logo d'un autre jeu de la SD, dossier gifs) : clone exact, sans re-conversion."""
        r = self.row
        try:
            self._set_pending(cl.make_raw_item(r["system"], r["rom"], cand["path"], name=r["name"], stem=r["stem"]))
        except Exception as e:   # noqa: BLE001
            self.pending = None
            messagebox.showerror(self.t("sd"), self.t("rw_err", err=e), parent=self.parent)
            self._redraw()
            self._update_buttons()

    def _on_raw_frame_picked(self, cand, index):
        """UNE image (index, 0 = la 1re) d'une animation deja au format de la dalle (.raw565pack d'un autre jeu / du dossier gifs) : image fixe, octets repris tels quels."""
        r = self.row
        try:
            self._set_pending(cl.make_raw_frame_item(r["system"], r["rom"], cand["path"], index, name=r["name"], stem=r["stem"]))
        except Exception as e:   # noqa: BLE001
            self.pending = None
            messagebox.showerror(self.t("sd"), self.t("rw_err", err=e), parent=self.parent)
            self._redraw()
            self._update_buttons()

    def _on_picked(self, path, mode, bg, sel=None):
        """sel : None (fichier) | ("anim", debut, fin) extrait choisi dans le selecteur | ("still", n) image capturee."""
        r = self.row
        a, b = 0, None
        if sel and sel[0] == "still":
            animate, a, b = True, sel[1], sel[1]
        elif sel:
            animate, a, b = True, sel[1], sel[2]
        else:
            n = cl.gif_frame_count(path)
            animate = n > 1 and messagebox.askyesno(self.t("sd"), self.t("ask_anim", n=n), parent=self.parent)
        try:
            self.pending = cl.make_best_item(r["system"], r["rom"], path, name=r["name"], mode=mode, bg=bg, stem=r["stem"], animate=animate, start=a, end=b, fit=True)
        except ValueError as e:
            self.pending = None
            messagebox.showerror(self.t("sd"), self.t("too_many", err=e), parent=self.parent)
        except Exception as e:   # noqa: BLE001
            self.pending = None
            messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        self._redraw()
        self._update_buttons()

    def _on_card_multi(self):
        rows = [r for r in self._multi if not r.get("off_sd") and (not self.pi_ip or pil.name_ok(r["stem"]))]
        if not rows:
            return
        known = sum(1 for r in rows if cl.find_item(self.store, r["key"]))
        if not messagebox.askyesno(self.t("sd"), self.t("multi_cards_ask", n=len(rows), known=known), parent=self.parent):
            return
        self.status_lbl.config(text=self.t("loading"))
        self.parent.update_idletasks()
        try:
            res = cl.import_cards_for(self.sd_root, self.store, rows, log=self._log)
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        msg = self.t("multi_cards_done", n=res["done"], e=len(res["errors"]))
        self.status_lbl.config(text=msg)
        self._log(msg)
        for k, err in res["errors"]:
            self._log(f"  {k} : {err}")
        self._pi_after_write([r["key"] for r in rows])
        self._load_system(self.system, note=False)

    def _on_cancel_multi(self):
        """« Annuler » sur un GROUPE de jeux : meme logique que pour un seul (version precedente, sinon original, sinon retrait), une seule confirmation."""
        plan = []
        for r in self._multi:
            it = None if r.get("off_sd") else cl.find_item(self.store, r["key"])
            if it is not None:
                plan.append((r, it, "prev" if cl.has_previous(it) else ("orig" if (it.get("orig_b64") or it.get("orig_pack_z")) else "del")))
        if not plan:
            return
        cnt = {k: sum(1 for _r, _i, kind in plan if kind == k) for k in ("prev", "orig", "del")}
        if not messagebox.askyesno(self.t("sd"), self.t("multi_cancel_ask", n=len(plan), prev=cnt["prev"], orig=cnt["orig"], dele=cnt["del"]), parent=self.parent):
            return
        self.status_lbl.config(text=self.t("loading"))
        self.parent.update_idletasks()
        done_prev, removed, errors = [], [], 0
        for r, it, kind in plan:
            snapshot = dict(it)               # l'entree avant retrait : le Pi en a besoin (original, animation)
            try:
                if kind == "prev":
                    cl.undo_previous(self.sd_root, self.store, r["key"], log=self._log)
                    done_prev.append(r["key"])
                else:
                    cl.restore_original(self.sd_root, self.store, r["key"])
                    removed.append(snapshot)
            except Exception as e:   # noqa: BLE001
                errors += 1
                self._log(f"  {r['key']} : {e}")
        try:
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        msg = self.t("multi_cancel_done", n=len(done_prev) + len(removed), e=errors)
        self.status_lbl.config(text=msg)
        self._log(msg)
        if self.pi_ip:
            if done_prev:
                self._pi_after_write(done_prev)
            if removed:
                self._pi_restore_many(removed)
        self._load_system(self.system, note=False)

    def _pi_restore_many(self, items):
        """Retire PLUSIEURS personnalisations sur le Pi en une seule recherche du Pi, puis un seul rafraichissement."""
        def work():
            ip, info, why = self._find_pi()
            if why != "ok":
                return ip, info, why, 0
            n, games = 0, []
            for it in items:
                out = pil.restore_on_pi(ip, it, log=self._log)
                if out in ("restored", "deleted"):
                    n += 1
                    games.append(cl.split_key(it["key"]))
            pil.refresh(ip, games)
            return ip, info, why, n

        def done(res, err):
            if err is not None:
                self.status_lbl.config(text=self.t("err", err=err))
                return
            ip, info, why, n = res
            if why != "ok":
                return messagebox.showwarning(self.t("pi_title"), self._pi_problem(ip, info, why), parent=self.parent)
            msg = self.t("multi_pi_done", ip=ip, n=n)
            self.status_lbl.config(text=msg)
            self._log(msg)
        self.bg_run(self.parent, work, done)

    def _on_card(self):
        if getattr(self, "_multi", []):
            return self._on_card_multi()
        if not (self.sd_root and self.store_ok):
            return self._no_target()
        r = getattr(self, "row", None)
        if r is None:
            return messagebox.showinfo(self.t("sd"), self.t("no_sel"), parent=self.parent)
        try:
            self.pending = cl.make_card_item(r["system"], r["rom"], r["name"], stem=r["stem"])
        except Exception as e:   # noqa: BLE001
            self.pending = None
            messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        self._redraw()
        self._update_buttons()

    def _on_cards_bulk(self):
        if not (self.sd_root and self.store_ok):
            return self._no_target()
        todo = [r for r in self.rows if not r.get("off_sd") and (r["state"] == "missing" or (r.get("dark") and r["state"] != "custom"))]
        skipped_names = 0
        if self.pi_ip:        # Pi : un jeu dont le nom n'est pas enregistrable (ex. espace insecable des ROM vpinball) ne peut pas recevoir de carte par Wi-Fi : inutile de la creer
            ok_todo = [r for r in todo if pil.name_ok(r["stem"])]
            skipped_names, todo = len(todo) - len(ok_todo), ok_todo
        if not todo:
            return messagebox.showinfo(self.t("sd"), self.t("cards_none"), parent=self.parent)
        if not messagebox.askyesno(self.t("sd"), self.t("cards_ask", sys=self._sys_label(), n=len(todo)) + (self.t("cards_pi_skip", n=skipped_names) if skipped_names else ""), parent=self.parent):
            return
        try:
            res = cl.import_cards(self.sd_root, self.store, [r for r in self.rows if not r.get("off_sd") and (not self.pi_ip or pil.name_ok(r["stem"]))],
                                  dark_keys=[r["key"] for r in todo if r.get("dark")], log=self._log)
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        msg = self.t("cards_done", n=res["done"], e=len(res["errors"]))
        self.status_lbl.config(text=msg)
        self._log(msg)
        for k, err in res["errors"]:
            self._log(f"  {k} : {err}")
        self._pi_after_write()
        self._load_system(self.system, note=False)

    def _on_browse(self):
        if not self.sd_root:
            return self._no_target()
        BrowseWindow(self)

    def _log(self, msg):
        try:
            self.gui._log_q.put(f"[Personnalisation] {msg}\n")
        except Exception:   # noqa: BLE001
            pass

    def _save_stores(self):
        cl.save_store(cl.store_path(self.sd_root), self.store)
        cl.save_store(cl.mirror_path(), self.store)

    def _twins(self, r):
        """Autres ROM du MEME systeme et du MEME nom que `r` (versions, .chd / .cue...) : chacune a son logo, et c'est celui du ROM LANCE que le DMD affiche."""
        if not r or r.get("off_sd"):
            return []
        return [o for o in self.rows if o["system"] == r["system"] and o["name"].lower() == r["name"].lower() and o["key"] != r["key"] and not o.get("off_sd")
                and o["state"] != "custom"]

    def _on_validate(self):
        if not (self.pending and self.sd_root and self.store_ok):
            return
        item, rep, _img = self.pending
        if "dark" in rep["warnings"] and not messagebox.askyesno(self.t("sd"), self.t("confirm_dark"), parent=self.parent):
            return
        # copie PROPRE de l'entree AVANT l'ecriture (commit_item y range l'original du jeu et la version precedente, qui ne valent que pour CE jeu) : sert aux autres ROM de meme nom
        base = {k: v for k, v in item.items() if not (k in ("prev", "pi_sent", "pi_orig_done") or k.startswith("orig_"))}
        twins = self._twins(getattr(self, "row", None))
        apply_twins = bool(twins) and messagebox.askyesno(
            self.t("sd"), self.t("ask_twins", name=twins[0]["name"], n=len(twins), roms="\n".join("• " + t["rom"] for t in twins[:8])), parent=self.parent)
        try:
            warns = cl.commit_item(self.sd_root, self.store, item, log=self._log)
            extra = []
            for t in (twins if apply_twins else []):
                tw = dict(base)
                tw.update(key=t["key"], rom=t["rom"], name=t["name"])
                cl.split_key(tw["key"])
                cl.commit_item(self.sd_root, self.store, tw, log=self._log)
                extra.append(tw["key"])
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
            return
        if "collision" in warns:
            messagebox.showwarning(self.t("sd"), self.t("collision", key=item["key"]), parent=self.parent)
        self.pending = None
        self.status_lbl.config(text=self.t("saved", key=item["key"]) + (" " + self.t("twins_done", n=len(extra)) if extra else ""))
        self._log(self.t("saved", key=item["key"]) + ((" " + self.t("twins_done", n=len(extra))) if extra else ""))
        self._pi_after_write([item["key"]] + extra)
        self._load_system(self.system, note=False)
        self._select_after_load = item["key"]

    def _on_cancel(self):
        """« Annuler », selon la situation : le choix en attente, sinon le dernier changement valide (un niveau), sinon retour a l'original (voir _cancel_mode)."""
        if getattr(self, "_multi", []):
            return self._on_cancel_multi()
        mode = self._cancel_mode()
        r = getattr(self, "row", None)
        if mode is None or r is None:
            return
        if mode == "choice":
            self.pending = None
            self.status_lbl.config(text=self.t("choice_cancelled"))
            self._redraw()
            self._update_buttons()
            return
        if mode == "prev":
            if not messagebox.askyesno(self.t("sd"), self.t("confirm_prev", name=r["name"]), parent=self.parent):
                return
            try:
                cl.undo_previous(self.sd_root, self.store, r["key"], log=self._log)
                self._save_stores()
            except Exception as e:   # noqa: BLE001
                messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
                return
            msg = self.t("undone_prev", key=r["key"])
            self.status_lbl.config(text=msg)
            self._log(msg)
            self._pi_after_write([r["key"]])             # Pi : le logo precedent y est renvoye comme apres une validation (SD : rien a envoyer)
            self._select_after_load = r["key"]
            self._load_system(self.system, note=False)
            return
        self._on_remove(r)

    def _on_remove(self, r):
        """Retire la personnalisation du jeu `r` : rend l'original sauvegarde (animation ou image) ou, sans original, supprime le fichier perso."""
        if not (r and self.sd_root and self.store_ok):
            return
        it = cl.find_item(self.store, r["key"])
        if it is None:
            return
        what = self.t("what_restored" if it.get("orig_b64") else "what_deleted")
        if not messagebox.askyesno(self.t("sd"), self.t("confirm_remove", name=r["name"], what=what), parent=self.parent):
            return
        try:
            res = cl.restore_original(self.sd_root, self.store, r["key"])
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
            return
        msg = self.t("removed_restored" if res == "restored" else "removed_deleted", key=r["key"])
        self.status_lbl.config(text=msg)
        self._log(msg)
        self._select_after_load = r["key"]
        self._load_system(self.system, note=False)
        if self.pi_ip:
            self._pi_restore(dict(it))          # le Pi est la source : le retrait le suit sans question
        elif it.get("pi_sent") and messagebox.askyesno(self.t("pi_title"), self.t("pi_restore_ask"), parent=self.parent):
            self._pi_restore(dict(it))

    def _on_batch(self):
        if not (self.sd_root and self.store_ok):
            return self._no_target()
        if self.system == ALL:
            return messagebox.showinfo(self.t("sd"), self.t("pick_system"), parent=self.parent)
        if not self.system or not self.rows:
            return messagebox.showinfo(self.t("sd"), self.t("no_sel"), parent=self.parent)
        if self.rows[0].get("off_sd"):
            return messagebox.showinfo(self.t("sd"), self.t("off_sd"), parent=self.parent)
        folder = filedialog.askdirectory(title=self.t("batch_pick"), initialdir=_pref_get("custom_img_dir") or None, parent=self.parent)
        if not folder:
            return
        try:
            plan = cl.plan_batch(folder, self.rows)
        except OSError as e:
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        n, amb, unk = len(plan["matched"]), len(plan["ambiguous"]), len(plan["unknown"])
        if not n:
            return messagebox.showinfo(self.t("sd"), self.t("batch_empty", sys=self.system, unk=unk, amb=amb), parent=self.parent)
        if not messagebox.askyesno(self.t("sd"), self.t("batch_ask", sys=self.system, n=n, amb=amb, unk=unk), parent=self.parent):
            return
        self.status_lbl.config(text=self.t("loading"))
        self.parent.update_idletasks()
        try:
            res = cl.import_batch(self.sd_root, self.store, plan["matched"], log=self._log)
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        msg = self.t("batch_done", n=res["done"], e=len(res["errors"]), d=len(res["dark"]))
        self.status_lbl.config(text=msg)
        self._log(msg)
        for name, err in res["errors"]:
            self._log(f"  {name} : {err}")
        if res["dark"]:
            self._log(self.t("batch_dark", keys=", ".join(res["dark"][:20])))
        self._pi_after_write()
        self._load_system(self.system, note=False)

    def _on_pack_export(self):
        if not self.store_ok or not self.store["items"]:
            return messagebox.showinfo(self.t("sd"), self.t("pack_empty"), parent=self.parent)
        path = filedialog.asksaveasfilename(parent=self.parent, defaultextension=".zip", initialfile="logos_perso.zip", filetypes=[(self.t("pack_types"), "*.zip")])
        if not path:
            return
        try:
            n = cl.export_pack(self.store, path)
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        msg = self.t("pack_exported", n=n, path=path)
        self.status_lbl.config(text=msg)
        self._log(msg)

    def _on_pack_import(self):
        if not (self.sd_root and self.store_ok):
            return self._no_target()
        path = filedialog.askopenfilename(parent=self.parent, filetypes=[(self.t("pack_types"), "*.zip")])
        if not path:
            return
        try:
            items, bad = cl.read_pack(path)
        except ValueError as e:
            return messagebox.showerror(self.t("sd"), self.t("pack_bad", err=e), parent=self.parent)
        for key, why in bad:
            self._log(f"  {key} : {why}")
        known = sum(1 for it in items if cl.find_item(self.store, it["key"]) is not None)
        if not items:
            return messagebox.showinfo(self.t("sd"), self.t("pack_bad", err=f"0 / {len(bad)}"), parent=self.parent)
        ans = messagebox.askyesnocancel(self.t("sd"), self.t("pack_ask", n=len(items), bad=len(bad), known=known), parent=self.parent)
        if ans is None:
            return
        try:
            res = cl.import_pack(self.sd_root, self.store, items, overwrite=ans, log=self._log)
            self._save_stores()
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        msg = self.t("pack_done", a=res["added"], r=res["replaced"], s=len(res["skipped"]))
        self.status_lbl.config(text=msg)
        self._log(msg)
        self._pi_after_write()
        if self.system:
            self._load_system(self.system, note=False)

    def _on_backup(self):
        """Sauvegarde COMPLETE (logos, animations, originaux, version precedente) dans un zip relisible par « Restaurer depuis une sauvegarde »."""
        if not self.store_ok or not self.store["items"]:
            return messagebox.showinfo(self.t("sd"), self.t("backup_empty"), parent=self.parent)
        path = filedialog.asksaveasfilename(parent=self.parent, defaultextension=".zip", initialfile=time.strftime("logos_perso_sauvegarde_%Y%m%d.zip"), filetypes=[(self.t("pack_types"), "*.zip")])
        if not path:
            return
        try:
            n = cl.export_backup(self.store, path)
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(self.t("sd"), self.t("err", err=e), parent=self.parent)
        _pref_set("custom_last_backup", path)
        msg = self.t("backup_done", n=n, path=path)
        self.status_lbl.config(text=msg)
        self._log(msg)

    def _on_backup_restore(self):
        if not (self.sd_root and self.store_ok):
            return self._no_target()
        last = _pref_get("custom_last_backup") or ""
        opts = {"initialdir": os.path.dirname(last), "initialfile": os.path.basename(last)} if last and os.path.isfile(last) else {}
        path = filedialog.askopenfilename(parent=self.parent, filetypes=[(self.t("pack_types"), "*.zip")], **opts)
        if not path:
            return
        try:
            items, bad = cl.read_backup(path)
        except ValueError as e:
            return messagebox.showerror(self.t("sd"), self.t("backup_bad", err=e), parent=self.parent)
        for key, why in bad:
            self._log(f"  {key} : {why}")
        if not items:
            return messagebox.showinfo(self.t("sd"), self.t("backup_bad", err=f"0 / {len(bad)}"), parent=self.parent)
        known = sum(1 for it in items if cl.find_item(self.store, it["key"]) is not None)
        if not messagebox.askyesno(self.t("sd"), self.t("backup_ask", n=len(items), known=known, bad=len(bad)), parent=self.parent):
            return
        self.status_lbl.config(text=self.t("loading"))

        def done(res, err):
            if err is not None:
                self.status_lbl.config(text=self.t("err", err=err))
                return
            try:
                self._save_stores()
            except Exception as e:   # noqa: BLE001
                self.status_lbl.config(text=self.t("err", err=e))
                return
            msg = self.t("backup_restored", n=res["restored"], r=res["replaced"])
            self.status_lbl.config(text=msg)
            self._log(msg)
            self._pi_after_write()
            if self.system:
                self._load_system(self.system, note=False)
        self.bg_run(self.parent, lambda: cl.restore_backup(self.sd_root, self.store, items, log=self._log), done)

    def _tool_label(self, key):
        """Libelle d'une entree du menu Outils ; « restore » s'adapte a la source (carte SD / Raspberry Pi Wi-Fi)."""
        if key == "restore":
            return self.t("restore_pi" if self.src_mode.get() == "pi" else "restore_sd")
        return self.t(key)

    def _refresh_tools_labels(self):
        menu = getattr(self, "tools_menu", None)
        if menu is None:
            return
        idx = 0
        for it in self._tools:
            if it is not None:
                menu.entryconfigure(idx, label=self._tool_label(it[0]))
            idx += 1

    def _on_restore(self):
        """REMISE EN ETAT de mes logos (a lancer apres un scrape / une mise a jour / un Mode 1 qui a ecrase les logos, ou apres une panne reseau) :
        source Carte SD -> reecrit toutes mes personnalisations sur la carte ; source Pi (Wi-Fi) -> les renvoie au Pi (empreintes comparees)."""
        if self.store_ok and not self.store["items"]:
            # 3e voie : ni la carte ni le miroir du PC ne contiennent de personnalisation -> proposer la derniere sauvegarde (zip)
            if messagebox.askyesno(self.t("sd"), self.t("restore_none"), parent=self.parent):
                self._on_backup_restore()
            return
        if self.src_mode.get() == "pi":
            return self._on_send_pi()
        return self._on_apply_all()

    def _on_apply_all(self):
        if not self.sd_root:
            return self._no_target()
        if not self.store_ok:
            return
        self.status_lbl.config(text=self.t("loading"))

        def done(res, err):
            if err is not None:
                self.status_lbl.config(text=self.t("err", err=err))
            elif res is None:
                self.status_lbl.config(text=self.t("nothing"))
            else:
                self.status_lbl.config(text=self.t("applied", w=len(res["written"]), u=len(res["unchanged"]), o=len(res["orphans"]), e=len(res["errors"])))
                self._pi_after_write()
                self._load_system(self.system, note=False)
        self.bg_run(self.parent, lambda: cl.apply_on_drive(self.sd_root, log=self._log), done)

    # ------------------------------------------------------------------ Raspberry Pi (Wi-Fi)
    def _find_pi(self):
        """Hors thread d'interface : (ip, info, motif) ; motif = 'ok' | 'no_dmd' | 'not_pi' | 'too_old'. L'adresse du Pi trouve est memorisee."""
        tool = pil._tool()
        known = _pref_get("custom_pi_ip")
        info = pil.probe(known) if known else None
        if not (info and info.get("is_pi")):
            info = tool.find_pi_dmd(known_ip=known or None)
        if not info:
            other = tool.find_dmd(known_ip=known or None)
            return (other or {}).get("ip"), other, ("not_pi" if other else "no_dmd")
        why = pil.supported(info)[1]
        if why == "ok":
            _pref_set("custom_pi_ip", info["ip"])
        return info["ip"], info, why

    def _pi_problem(self, ip, info, why):
        """Message utilisateur d'un motif d'echec de _find_pi."""
        if why == "no_dmd":
            return self.t("pi_no_dmd")
        if why == "not_pi":
            return self.t("pi_not_pi", ip=ip)
        return self.t("pi_too_old", ip=ip, v=(info or {}).get("version") or "?")

    def _on_send_pi(self):
        if not self.store_ok:
            return
        if not self.store["items"]:
            return messagebox.showinfo(self.t("pi_title"), self.t("pi_nothing"), parent=self.parent)
        self.send_pi_btn.config(state="disabled")
        self.status_lbl.config(text=self.t("pi_search"))

        def found(res, err):
            if err is not None:
                self.send_pi_btn.config(state="normal")
                self.status_lbl.config(text=self.t("err", err=err))
                return
            ip, info, why = res
            if why != "ok":
                self.send_pi_btn.config(state="normal")
                self.status_lbl.config(text="")
                return messagebox.showwarning(self.t("pi_title"), self._pi_problem(ip, info, why), parent=self.parent)
            can_anim = pil.anim_supported(info)       # demon >= 3.82 : les animations partent aussi (pack + delais)
            items = [it for it in self.store["items"] if can_anim or not cl.is_anim(it)]
            anims = len(self.store["items"]) - len(items)
            if not items:
                self.send_pi_btn.config(state="normal")
                self.status_lbl.config(text="")
                return messagebox.showinfo(self.t("pi_title"), self.t("pi_nothing") + (self.t("pi_anim_note", a=anims) if anims else ""), parent=self.parent)
            anim = self.t("pi_anim_note", a=anims) if anims else ""
            if not messagebox.askyesno(self.t("pi_title"), self.t("pi_confirm", n=len(items), ip=ip, v=info.get("version") or "?", anim=anim), parent=self.parent):
                self.send_pi_btn.config(state="normal")
                self.status_lbl.config(text="")
                return
            self.status_lbl.config(text=self.t("loading"))
            mirror = self.sd_root
            self.bg_run(self.parent, lambda: pil.push_store(ip, self.store, log=self._log, mirror=mirror), lambda r, e: self._pi_pushed(ip, r, e))
        self.bg_run(self.parent, self._find_pi, found)

    def _pi_after_write(self, keys=None):
        """Quand le Pi est la source : ce qui vient d'etre ecrit dans le miroir part aussitot vers le Pi (empreinte comparee : seul ce qui differe est envoye). Sans effet avec une carte SD."""
        if not (self.pi_ip and self.store_ok):
            return
        ip = self.pi_ip
        self.status_lbl.config(text=self.t("pi_pushing", ip=ip))
        mirror = self.sd_root
        self.bg_run(self.parent, lambda: pil.push_store(ip, self.store, keys=keys, log=self._log, mirror=mirror), lambda r, e: self._pi_pushed(ip, r, e, quiet=True))

    def _pi_pushed(self, ip, rep, err, quiet=False):
        """Fin d'un envoi au Pi. quiet : pas de boite de dialogue quand tout va bien (envoi automatique apres une ecriture)."""
        self.send_pi_btn.config(state="normal")
        if err is not None:
            self.status_lbl.config(text=self.t("err", err=err))
            return
        try:
            self._save_stores()          # l'envoi note l'original du Pi (orig_b64) et « pi_sent » dans les entrees
        except Exception as e:   # noqa: BLE001
            self._log(self.t("err", err=e))
        msg = self.t("pi_done", ip=ip, s=len(rep["sent"]), u=len(rep["unchanged"]), a=len(rep["skipped_anim"]), e=len(rep["errors"]))
        if rep.get("skipped_name"):          # noms que le Pi ne sait pas enregistrer : information, pas une erreur (pas de fenetre d'avertissement)
            msg += " " + self.t("pi_names", n=len(rep["skipped_name"]))
            self._log("Noms non enregistrables par le Pi : " + ", ".join(rep["skipped_name"][:15]) + (" ..." if len(rep["skipped_name"]) > 15 else ""))
        self.status_lbl.config(text=msg)
        self._log(msg)
        detail = ""
        if rep["errors"]:
            detail += "\n\n" + self.t("pi_errors", list=", ".join(f"{k} ({c})" for k, c in rep["errors"][:10]))
            self._log(detail.strip())
        if (rep["sent"] or rep["unchanged"]) and not (rep["refresh"] and rep["refresh"].get("cache") == "ok"):
            detail += "\n\n" + self.t("pi_cache_ko").strip()
        if detail or not quiet:
            (messagebox.showwarning if detail else messagebox.showinfo)(self.t("pi_title"), msg + detail, parent=self.parent)
        if not quiet:
            self._load_system(self.system, note=False)
        elif rep.get("mirror_changed"):
            # le miroir vient de perdre le .raw565 fixe que le Pi n'a plus (il n'a que l'animation) : « Existant » doit montrer ce que le Pi affiche, on recharge la liste
            row = getattr(self, "row", None)
            if row is not None:
                self._select_after_load = row["key"]
            self._load_system(self.system, note=False)

    def _pi_restore(self, item):
        """Retire `item` du Pi (rend l'original garde, sinon supprime le fichier) puis rafraichit le Pi."""
        self.status_lbl.config(text=self.t("pi_search"))

        def work():
            ip, info, why = self._find_pi()
            if why != "ok":
                return ip, info, why, None
            res = pil.restore_on_pi(ip, item, log=self._log)
            pil.refresh(ip, [cl.split_key(item["key"])] if res == "restored" else [])
            return ip, info, why, res

        def done(res, err):
            if err is not None:
                self.status_lbl.config(text=self.t("err", err=err))
                return
            ip, info, why, out = res
            if why != "ok":
                self.status_lbl.config(text="")
                return messagebox.showwarning(self.t("pi_title"), self._pi_problem(ip, info, why), parent=self.parent)
            if out == "restored":
                msg = self.t("pi_restored", ip=ip, key=item["key"])
            elif out == "deleted":
                msg = self.t("pi_deleted", ip=ip, key=item["key"])
            else:
                msg = self.t("pi_restore_ko", key=item["key"])
            self.status_lbl.config(text=msg)
            self._log(msg)
            (messagebox.showinfo if out in ("restored", "deleted") else messagebox.showwarning)(self.t("pi_title"), msg, parent=self.parent)
        self.bg_run(self.parent, work, done)


class ImagePicker(_Base):
    """Choix de l'image de remplacement : miniatures d'un dossier (ou collage), mode de conversion, fond, apercu LED en direct et controles."""

    def __init__(self, tab, row, on_pick):
        super().__init__(tab.gui)
        self.tab, self.row, self.on_pick = tab, row, on_pick
        self.src_path = None
        self.src_rgba = None
        self._thumbs = []
        self._photos = {}
        self._load_token = 0
        self._entries, self._shown, self._more_btn = [], 0, None
        w = self.win = tk.Toplevel(tab.gui.root)
        w.title(self.t("pk_title", name=row["name"]))
        w.configure(bg=self.bg)
        w.geometry("1020x720")
        w.minsize(900, 700)
        w.transient(tab.gui.root)

        left = tk.Frame(w, bg=self.bg)
        left.pack(side="left", fill="both", expand=True, padx=(10, 5), pady=10)
        top = tk.Frame(left, bg=self.bg)
        top.pack(fill="x")
        tk.Label(top, text=self.t("pk_folder"), bg=self.bg, fg=self.fg).pack(side="left")
        self.folder_var = tk.StringVar(value=_pref_get("custom_img_dir") or str(Path.home() / "Pictures"))
        path_entry = tk.Entry(top, textvariable=self.folder_var)
        path_entry.pack(side="left", fill="x", expand=True, padx=4)
        path_entry.bind("<Return>", lambda e: self._load_folder())
        self._btn(top, "pk_up", self._up, padx=6, pady=1).pack(side="left")
        self._btn(top, "browse", self._browse, padx=6, pady=1).pack(side="left", padx=(4, 0))
        self._btn(top, "pk_paste", self._paste, padx=6, pady=1).pack(side="left", padx=(4, 0))
        f = tk.Frame(left, bg=self.bg)
        f.pack(fill="x", pady=(4, 2))
        tk.Label(f, text=self.t("pk_filter"), bg=self.bg, fg=self.fg).pack(side="left")
        self.flt_var = tk.StringVar()
        self._flt_job = None
        self.flt_var.trace_add("write", lambda *a: self._load_soon())
        tk.Entry(f, textvariable=self.flt_var, width=24).pack(side="left", padx=4)
        self._btn(f, "pk_suggest", self._suggest, padx=6, pady=1).pack(side="left")
        self.rec_var = tk.BooleanVar(value=False)
        self.rec_var.trace_add("write", lambda *a: self._load_folder())
        tk.Checkbutton(f, text=self.t("pk_sub"), variable=self.rec_var, bg=self.bg, fg=self.fg, activebackground=self.bg, selectcolor=self.bg).pack(side="left", padx=(8, 0))
        self.msg_lbl = tk.Label(left, text="", bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8))
        self.msg_lbl.pack(fill="x")

        host = tk.Frame(left, bg="#2B2B2B", bd=1, relief="solid")
        host._no_bg_slice = True
        host.pack(fill="both", expand=True)
        self.canvas = tk.Canvas(host, bg="#2B2B2B", highlightthickness=0)
        sb = tk.Scrollbar(host, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=sb.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        self.inner = tk.Frame(self.canvas, bg="#2B2B2B")
        self.inner._no_bg_slice = True
        self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.inner.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.bind("<Enter>", lambda e: self.canvas.bind_all("<MouseWheel>", lambda ev: self.canvas.yview_scroll(int(-ev.delta / 120), "units")))
        self.canvas.bind("<Leave>", lambda e: self.canvas.unbind_all("<MouseWheel>"))

        right = tk.Frame(w, bg=self.bg)
        right.pack(side="left", fill="y", padx=(5, 10), pady=10)
        tk.Label(right, text=row["name"], bg=self.bg, fg=self.fg, font=("TkDefaultFont", 10, "bold"), wraplength=400, justify="left").pack(anchor="w")
        tk.Label(right, text=self.t("repl"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "bold")).pack(anchor="w", pady=(6, 0))
        self.prev_cv = tk.Canvas(right, width=384, height=96, bg="black", highlightthickness=1, highlightbackground="#888888")
        self.prev_cv._no_bg_slice = True
        self.prev_cv.pack(anchor="w")
        tk.Label(right, text=self.t("mode"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "bold")).pack(anchor="w", pady=(8, 0))
        self.mode_var = tk.StringVar(value="pixel")
        for m in cl.MODES:
            tk.Radiobutton(right, text=self.t("m_" + m), value=m, variable=self.mode_var, command=self._update_preview, bg=self.bg, fg=self.fg,
                           activebackground=self.bg, selectcolor=self.bg).pack(anchor="w")
        bgrow = tk.Frame(right, bg=self.bg)
        bgrow.pack(fill="x", pady=(6, 0))
        tk.Label(bgrow, text=self.t("bg"), bg=self.bg, fg=self.fg).pack(side="left")
        self.bg_rgb = (0, 0, 0)
        self.swatch = tk.Label(bgrow, width=4, bg="#000000", relief="solid", bd=1)
        self.swatch.pack(side="left", padx=4)
        self._btn(bgrow, "bg_pick", self._pick_bg, padx=6, pady=1).pack(side="left")
        self._btn(bgrow, "bg_black", lambda: self._set_bg((0, 0, 0)), padx=6, pady=1).pack(side="left", padx=(4, 0))
        self.clip = tk.Frame(right, bg=self.bg)
        self.clip_lbl = tk.Label(self.clip, text="", bg=self.bg, fg=self.fg, anchor="w", justify="left", wraplength=384, font=("TkDefaultFont", 8, "bold"))
        self.clip_lbl.pack(fill="x")
        self.pos_var = tk.IntVar(value=0)
        self._pos_job = None
        self.pos_scale = tk.Scale(self.clip, orient="horizontal", showvalue=False, variable=self.pos_var, command=lambda v: self._on_pos(), bg=self.bg, fg=self.fg, highlightthickness=0, length=384)
        self.pos_scale.pack(fill="x")
        self.pos_lbl = tk.Label(self.clip, text="", bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8))
        self.pos_lbl.pack(fill="x")
        crow = tk.Frame(self.clip, bg=self.bg)
        crow.pack(fill="x", pady=(2, 0))
        self._btn(crow, "clip_start", lambda: self._clip_set(a=True), padx=6, pady=1).pack(side="left")
        self._btn(crow, "clip_end", lambda: self._clip_set(b=True), padx=6, pady=1).pack(side="left", padx=(4, 0))
        self._btn(crow, "clip_all", self._clip_all, padx=6, pady=1).pack(side="left", padx=(4, 0))
        self._btn(crow, "clip_snap", lambda: self._use(("still", self.pos_var.get())), padx=6, pady=1, bg=self.bg_action).pack(side="left", padx=(8, 0))
        self.n_frames, self.clip_a, self.clip_b = 1, 0, 0
        self._note_lbl = tk.Label(right, text=self.t("pk_note"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), wraplength=384, justify="left")
        self._note_lbl.pack(anchor="w", pady=(4, 0))
        self.info = tk.Label(right, text=self.t("pk_pick_first"), bg=self.bg, fg=self.fg, justify="left", anchor="nw", wraplength=384, height=5)
        self.info.pack(fill="x", pady=(6, 0))
        btns = tk.Frame(right, bg=self.bg)
        btns.pack(fill="x", side="bottom")
        self.use_btn = self._btn(btns, "pk_use", self._use, bg=self.bg_action, state="disabled")
        self.use_btn.pack(side="left", fill="x", expand=True)
        self._btn(btns, "cancel", w.destroy).pack(side="left", padx=(4, 0))

        self._draw_preview(None)
        self._load_folder()

    PAGE = 150            # miniatures creees par lot (un pack de 3 600 GIF n'est pas charge d'un coup)
    MAX_RESULTS = 4000    # plafond d'une recherche dans les sous-dossiers

    def _browse(self):
        d = filedialog.askdirectory(parent=self.win, initialdir=self.folder_var.get() or None)
        if d:
            self.folder_var.set(d)
            self._load_folder()

    def _up(self):
        """Dossier parent (jusqu'a la racine du lecteur)."""
        cur = Path(self.folder_var.get().strip() or ".")
        if cur.parent != cur:
            self.folder_var.set(str(cur.parent))
            self._load_folder()

    def _enter(self, path):
        """Clic sur une tuile de dossier : on y entre (la recherche dans les sous-dossiers s'arrete)."""
        self.rec_var.set(False)
        self.folder_var.set(str(path))
        self._load_folder()

    def _suggest(self):
        """Le filtre prend le NOM DU JEU (sans mentions entre parentheses / crochets) et la recherche couvre les sous-dossiers : tous les remplacants possibles dans un pack."""
        name = re.sub(r"[\(\[].*?[\)\]]", " ", self.row.get("name") or self.row.get("stem") or "")
        self.flt_var.set(re.sub(r"\s+", " ", name).strip())
        self.rec_var.set(True)

    def _load_soon(self):
        """Filtre : relance 250 ms apres la derniere frappe."""
        if self._flt_job:
            try:
                self.win.after_cancel(self._flt_job)
            except tk.TclError:
                pass
        self._flt_job = self.win.after(250, self._load_folder)

    @staticmethod
    def _tokens(text):
        return [x for x in re.split(r"[^a-z0-9]+", text.lower()) if x]

    def _load_folder(self):
        """Dossier (et sous-dossiers, au choix) -> tuiles de dossiers puis miniatures par lots. Le parcours du disque se fait hors du thread d'interface (pack de 11 000 GIF, NAS) ;
        jeton = abandon si le dossier / le filtre change. Le filtre compare des MOTS : « sonic 2 » trouve « CONSOLE_Sonic_The_Hedgehog_2.gif »."""
        import os as _os
        self._load_token += 1
        token = self._load_token
        for c in self.inner.winfo_children():
            c.destroy()
        self._thumbs.clear()
        self._entries, self._shown, self._more_btn = [], 0, None
        folder = Path(self.folder_var.get().strip() or ".")
        toks = self._tokens(self.flt_var.get())
        recursive = bool(self.rec_var.get())
        self.msg_lbl.config(text=self.t("loading"))

        def match(name):
            words = self._tokens(name)
            def hit(k, w):
                if k.isdigit():                      # « 2 » = « 02 » mais pas « 1942 » ni « sonic2adv »
                    return w.isdigit() and int(w) == int(k)
                return w.startswith(k) or (len(k) >= 3 and k in w)
            return all(any(hit(k, w) for w in words) for k in toks)

        def scan():
            dirs, files, capped = [], [], False
            try:
                if recursive:
                    for dp, _dn, fn in _os.walk(folder):
                        if token != self._load_token:
                            return None
                        for f in fn:
                            if Path(f).suffix.lower() in IMG_EXTS and match(f):
                                files.append(Path(dp) / f)
                                if len(files) >= self.MAX_RESULTS:
                                    capped = True
                                    break
                        if capped:
                            break
                else:
                    with _os.scandir(folder) as it:
                        for e in it:
                            if e.is_dir():
                                if not toks or match(e.name):
                                    dirs.append(Path(e.path))
                            elif Path(e.name).suffix.lower() in IMG_EXTS and match(e.name):
                                files.append(Path(e.path))
            except OSError:
                pass
            dirs.sort(key=lambda p: p.name.lower())
            files.sort(key=lambda p: (p.name.lower(), str(p).lower()))
            return dirs, files, capped

        def done(res, err):
            if token != self._load_token or err is not None or res is None:
                return
            dirs, files, capped = res
            self._entries = [("dir", d) for d in dirs] + [("img", f) for f in files]
            text = self.t("pk_found", d=len(dirs), i=len(files)) if self._entries else self.t("pk_none")
            if capped:
                text += "  -  " + self.t("pk_cap", n=self.MAX_RESULTS)
            self.msg_lbl.config(text=text)
            if files:
                _pref_set("custom_img_dir", str(folder))
            self._more()
        self.bg_run(self.win, scan, done)

    def _more(self):
        """Lot suivant : tuiles de dossiers immediates, miniatures lues dans un thread."""
        token = self._load_token
        chunk = self._entries[self._shown:self._shown + self.PAGE]
        base = self._shown
        self._shown += len(chunk)
        if self._more_btn is not None:
            try:
                self._more_btn.destroy()
            except tk.TclError:
                pass
            self._more_btn = None
        slots = []
        for i, (kind, path) in enumerate(chunk):
            if kind == "dir":
                self._place_dir(path, base + i)
            else:
                slots.append((path, base + i))
        if self._shown < len(self._entries):
            rest = len(self._entries) - self._shown
            self._more_btn = self._btn(self.inner, "pk_more", self._more)
            self._more_btn.config(text=self.t("pk_more", n=rest))
            self._more_btn.grid(row=(self._shown + 2) // 3 + 1, column=0, columnspan=3, pady=6)

        def place(path, pil, n):
            if token == self._load_token:
                self._place_thumb(path, pil, n)

        def work():
            for path, n in slots:
                if token != self._load_token:
                    return
                try:
                    pil = self._make_thumb(path)
                except Exception:   # noqa: BLE001
                    continue
                try:
                    self.win.after(0, lambda p=path, im=pil, k=n: place(p, im, k))
                except Exception:   # noqa: BLE001
                    return          # fenetre fermee
        if slots:
            threading.Thread(target=work, daemon=True).start()

    def _place_dir(self, path, n):
        """Tuile de DOSSIER : un clic y entre."""
        cell = tk.Frame(self.inner, bg="#3A3A4A", padx=4, pady=4, cursor="hand2")
        cell._no_bg_slice = True
        cell.grid(row=n // 3, column=n % 3, padx=1, pady=1)
        ico = tk.Label(cell, text="\U0001F4C1", bg="#3A3A4A", fg="#F5C542", font=("TkDefaultFont", 20), width=7, height=2)
        ico.pack()
        cap = tk.Label(cell, text=path.name if len(path.name) < 22 else path.name[:19] + "\u2026", bg="#3A3A4A", fg="#FFFFFF", font=("TkDefaultFont", 8, "bold"))
        cap.pack()
        for wdg in (cell, ico, cap):
            wdg.bind("<Button-1>", lambda e, p=path: self._enter(p))
    @staticmethod
    def _make_thumb(path):
        from PIL import Image
        with Image.open(path) as im:
            try:
                im.seek(0)
            except EOFError:
                pass
            im = im.convert("RGBA")
        im.thumbnail((136, 64))
        base = Image.new("RGB", im.size, (40, 40, 40))
        base.paste(im, (0, 0), im)
        return base

    def _place_thumb(self, path, base, n):
        ph = _photo(base)
        self._photos[str(path)] = ph
        cell = tk.Frame(self.inner, bg="#2B2B2B", padx=4, pady=4, cursor="hand2")
        cell._no_bg_slice = True
        cell.grid(row=n // 3, column=n % 3)
        lab = tk.Label(cell, image=ph, bg="#2B2B2B", bd=1, relief="solid", width=140, height=68)
        lab.pack()
        cap = tk.Label(cell, text=path.name if len(path.name) < 22 else path.name[:19] + "…", bg="#2B2B2B", fg="#DDDDDD", font=("TkDefaultFont", 7))
        cap.pack()
        for wdg in (cell, lab, cap):
            wdg.bind("<Button-1>", lambda e, p=path: self._select(p))
        self._thumbs.append(cell)

    def _paste(self):
        try:
            from PIL import ImageGrab
            clip = ImageGrab.grabclipboard()
        except Exception:   # noqa: BLE001
            clip = None
        if isinstance(clip, list) and clip:
            p = Path(clip[0])
            if p.suffix.lower() in IMG_EXTS and p.is_file():
                self.folder_var.set(str(p.parent))
                self._load_folder()
                return self._select(p)
            clip = None
        if clip is None or isinstance(clip, list):
            self.msg_lbl.config(text=self.t("pk_clip_none"))
            return
        d = cl.mirror_path().parent / "pasted"
        d.mkdir(parents=True, exist_ok=True)
        p = d / f"clip_{datetime.now():%Y%m%d_%H%M%S}.png"
        clip.save(p)
        self.msg_lbl.config(text="")
        self._select(p)

    def _select(self, path):
        try:
            tool = cl._tool()
            with tool._open_image_tolerant(path) as im:
                try:
                    im.seek(0)
                except EOFError:
                    pass
                self.src_rgba = im.convert("RGBA")
            self.src_path = Path(path)
        except Exception as e:   # noqa: BLE001
            self.src_path = self.src_rgba = None
            self.info.config(text=self.t("err", err=e))
            self.use_btn.config(state="disabled")
            self._hide_clip()
            return
        n = cl.gif_frame_count(path)
        if n > 1:
            self._show_clip(n)
        else:
            self._hide_clip()
        self._update_preview()

    def _hide_clip(self):
        self.n_frames, self.clip_a, self.clip_b = 1, 0, 0
        self.clip.pack_forget()

    def _show_clip(self, n):
        """GIF anime : extrait (debut / fin) et capture d'une image fixe. Par defaut tout le GIF (borne a MAX_ANIM_FRAMES)."""
        self.n_frames, self.clip_a, self.clip_b = n, 0, n - 1
        self.pos_scale.config(from_=0, to=n - 1)
        self.pos_var.set(0)
        self.clip.pack(fill="x", pady=(6, 0), before=self._note_lbl)
        self._clip_refresh()

    def _clip_refresh(self):
        self.clip_lbl.config(text=self.t("clip_range", a=self.clip_a + 1, b=self.clip_b + 1, n=self.n_frames))
        self.pos_lbl.config(text=self.t("clip_pos", p=self.pos_var.get() + 1))

    def _clip_set(self, a=False, b=False):
        p = self.pos_var.get()
        if a:
            self.clip_a = p
            self.clip_b = max(self.clip_b, p)
        if b:
            self.clip_b = p
            self.clip_a = min(self.clip_a, p)
        self._clip_refresh()
        self._update_preview()

    def _clip_all(self):
        self.clip_a, self.clip_b = 0, self.n_frames - 1
        self._clip_refresh()
        self._update_preview()

    def _on_pos(self):
        """Curseur de position : apercu de cette image (relance 80 ms apres le dernier mouvement)."""
        if self.n_frames <= 1:
            return
        self._clip_refresh()
        if self._pos_job:
            try:
                self.win.after_cancel(self._pos_job)
            except tk.TclError:
                pass
        self._pos_job = self.win.after(80, self._load_pos)

    def _load_pos(self):
        try:
            self.src_rgba = cl.gif_frame(self.src_path, self.pos_var.get())
        except Exception:   # noqa: BLE001
            return
        self._update_preview(still=True)

    def _pick_bg(self):
        from tkinter import colorchooser
        c = colorchooser.askcolor(color="#%02x%02x%02x" % self.bg_rgb, parent=self.win)
        if c and c[0]:
            self._set_bg(tuple(int(v) for v in c[0]))

    def _set_bg(self, rgb):
        self.bg_rgb = rgb
        self.swatch.config(bg="#%02x%02x%02x" % rgb)
        self._update_preview()

    def _draw_preview(self, img):
        self.prev_cv.delete("all")
        if img is None:
            self.prev_cv.create_text(192, 48, text=self.t("none"), fill="#888888", font=("TkDefaultFont", 16))
            return
        self._last_out = img
        ph = _photo(cl.led_style(img, 3, self.tab.bright_var.get()))
        self._photos["prev"] = ph
        self.prev_cv.create_image(0, 0, image=ph, anchor="nw")

    def refresh_preview(self):
        """Redessin avec la luminosite LED de l'onglet (curseur partage)."""
        self._anim_cache = {}
        if getattr(self, "_last_out", None) is not None:
            self._draw_preview(self._last_out)

    MAX_PREVIEW_FRAMES = 120      # images rendues pour la boucle d'apercu (echantillonnage 1 sur k au-dela, delais sommes : meme duree)

    def _anim_stop(self):
        self._anim_token = getattr(self, "_anim_token", 0) + 1

    def _anim_start(self):
        """Joue en BOUCLE l'extrait choisi d'un GIF anime dans le cadre « Remplacant » (rendu LED, images rendues a la demande, delai >= 100 ms)."""
        self._anim_stop()
        self._anim_cache = {}
        if self.n_frames <= 1 or self.src_path is None:
            return
        from PIL import ImageSequence
        token, mode, bg = self._anim_token, self.mode_var.get(), self.bg_rgb
        a, b = self.clip_a, self.clip_b
        step = max(1, -(-(b - a + 1) // self.MAX_PREVIEW_FRAMES))
        frames, delays = [], []
        try:
            with cl._tool()._open_image_tolerant(self.src_path) as im:
                fallback = im.info.get("duration", 100)
                for i, fr in enumerate(ImageSequence.Iterator(im)):
                    if i < a:
                        continue
                    if i > b:
                        break
                    try:
                        d = int(fr.info.get("duration", fallback))
                    except (TypeError, ValueError):
                        d = 100
                    d = max(20, d if d > 0 else 100)
                    if (i - a) % step:
                        delays[-1] += d
                        continue
                    frames.append(cl.render(fr.convert("RGBA"), mode, tuple(bg)))
                    delays.append(d)
        except Exception:   # noqa: BLE001
            return
        if len(frames) < 2:
            return

        def tick(i):
            if token != self._anim_token:
                return
            try:
                if i not in self._anim_cache:
                    self._anim_cache[i] = _photo(cl.led_style(frames[i], 3, self.tab.bright_var.get()))
                self._photos["prev"] = self._anim_cache[i]
                self._last_out = frames[i]
                self.prev_cv.delete("all")
                self.prev_cv.create_image(0, 0, image=self._anim_cache[i], anchor="nw")
                self.win.after(max(100, delays[i]), lambda: tick((i + 1) % len(frames)))
            except tk.TclError:
                pass        # fenetre fermee
        self.win.after(max(100, delays[0]), lambda: tick(1))

    def _update_preview(self, still=False):
        """still=True : image fixe (curseur de position : on regarde UNE image, la boucle est suspendue jusqu'au prochain changement d'extrait, de mode ou de fond)."""
        if self.src_rgba is None:
            return
        try:
            out = cl.render(self.src_rgba, self.mode_var.get(), self.bg_rgb)
            rep = cl.analyze(self.src_rgba, out)
        except Exception as e:   # noqa: BLE001
            self.info.config(text=self.t("err", err=e))
            self.use_btn.config(state="disabled")
            return
        self._draw_preview(out)
        if still:
            self._anim_stop()
        else:
            self._anim_start()
        lines = [f"{self.src_path.name} — {rep['src_size'][0]}×{rep['src_size'][1]}"] + [self.t("w_" + w) for w in rep["warnings"]]
        count = self.clip_b - self.clip_a + 1
        if self.n_frames > 1 and count > cl.MAX_ANIM_FRAMES:          # pas de coupe : 1 image sur k, la duree totale est conservee
            lines.append(self.t("clip_resample", c=count, m=cl.MAX_ANIM_FRAMES, k=-(-count // cl.MAX_ANIM_FRAMES)))
        self.info.config(text="\n".join(lines))
        self.use_btn.config(state="normal")

    def _use(self, sel=None):
        """sel : ("still", n) = capture de l'image n ; None = l'extrait choisi (ou le fichier tel quel si ce n'est pas un GIF anime)."""
        if self.src_path is None:
            return
        if sel is None and self.n_frames > 1:
            sel = ("anim", self.clip_a, self.clip_b)
        path, mode, bg = self.src_path, self.mode_var.get(), self.bg_rgb
        self._anim_stop()
        self.win.destroy()
        self.on_pick(path, mode, bg, sel)


class BrowseWindow(_Base):
    """Parcours des logos existants d'un systeme en miniatures (128x32) : on voit tout, y compris les jeux sans logo, au lieu de chercher / trier."""
    PAGE = 48
    COLS = 4

    def __init__(self, tab):
        super().__init__(tab.gui)
        self.tab = tab
        self.rows = []
        self.shown = 0
        self._photos = []
        self._token = 0
        w = self.win = tk.Toplevel(tab.gui.root)
        w.title(self.t("br_title"))
        w.configure(bg=self.bg)
        w.geometry("860x620")
        w.minsize(700, 460)
        w.transient(tab.gui.root)

        left = tk.Frame(w, bg=self.bg)
        left.pack(side="left", fill="y", padx=(10, 5), pady=10)
        tk.Label(left, text=self.t("br_systems"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 9, "bold")).pack(anchor="w")
        self.lb = tk.Listbox(left, width=18, exportselection=False, bg="#2B2B2B", fg="#EEEEEE", selectbackground="#3D7AD6", selectforeground="white", highlightthickness=0)
        sbl = tk.Scrollbar(left, orient="vertical", command=self.lb.yview)
        self.lb.configure(yscrollcommand=sbl.set)
        self.lb.pack(side="left", fill="y")
        sbl.pack(side="left", fill="y")
        for s in tab.systems:
            self.lb.insert("end", s)
        self.lb.bind("<<ListboxSelect>>", lambda e: self._on_system())

        right = tk.Frame(w, bg=self.bg)
        right.pack(side="left", fill="both", expand=True, padx=(5, 10), pady=10)
        f = tk.Frame(right, bg=self.bg)
        f.pack(fill="x")
        tk.Label(f, text=self.t("filter") if False else tab.t("filter"), bg=self.bg, fg=self.fg).pack(side="left")
        self.flt_var = tk.StringVar()
        self.flt_var.trace_add("write", lambda *a: self._render())
        tk.Entry(f, textvariable=self.flt_var, width=22).pack(side="left", padx=4)
        self.view = _ViewFilter(f, tab.t, self._render, self.bg, self.fg, width=30)     # meme menu « Afficher » que la fenetre principale
        self.view.label.pack(side="left", padx=(10, 0))
        self.view.cb.pack(side="left", padx=4)
        host = tk.Frame(right, bg="#202020", bd=1, relief="solid")
        host._no_bg_slice = True
        host.pack(fill="both", expand=True, pady=(6, 0))
        self.canvas = tk.Canvas(host, bg="#202020", highlightthickness=0)
        sb = tk.Scrollbar(host, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=sb.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        self.inner = tk.Frame(self.canvas, bg="#202020")
        self.inner._no_bg_slice = True
        self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.inner.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.bind("<Enter>", lambda e: self.canvas.bind_all("<MouseWheel>", lambda ev: self.canvas.yview_scroll(int(-ev.delta / 120), "units")))
        self.canvas.bind("<Leave>", lambda e: self.canvas.unbind_all("<MouseWheel>"))
        self.more_btn = self._btn(right, "br_more", self._more)
        self.msg = tk.Label(right, text="", bg=self.bg, fg=self.fg)
        self.msg.pack(anchor="w")
        if tab.system in tab.systems:
            i = tab.systems.index(tab.system)
            self.lb.selection_set(i)
            self.lb.see(i)
            self._on_system()

    def _on_system(self):
        sel = self.lb.curselection()
        if not sel:
            return
        system = self.lb.get(sel[0])
        self._token += 1
        token = self._token
        self.msg.config(text=self.t("loading"))

        def done(res, err):
            if token != self._token:
                return
            self.rows = [] if err is not None else res[0]
            if system == self.tab.system:       # la liste de l'onglet a les etats « noir » deja calcules
                dark = {r["key"] for r in self.tab.rows if r.get("dark")}
                for r in self.rows:
                    r["dark"] = r["key"] in dark
            self.system = system
            self.msg.config(text="")
            self._render()
        self.bg_run(self.win, lambda: self.tab.compute_rows(system), done)

    def _visible(self):
        flt = self.flt_var.get().strip().lower()
        return [r for r in self.rows if self.view.shows(_row_category(r)) and (not flt or flt in r["name"].lower() or flt in r["stem"].lower())]

    def _render(self):
        for c in self.inner.winfo_children():
            c.destroy()
        self._photos.clear()
        self.shown = 0
        self.more_btn.pack_forget()
        self.view.refresh(_ViewFilter.count(self.rows))
        self.vis = self._visible()
        self.msg.config(text="" if self.vis else self.t("br_none"))
        self._more()

    def _more(self):
        chunk = self.vis[self.shown:self.shown + self.PAGE]
        for r in chunk:
            n = self.shown
            self._cell(r, n)
            self.shown += 1
        self.more_btn.pack_forget()
        if self.shown < len(self.vis):
            self.more_btn.pack(anchor="w", pady=4)

    def _cell(self, r, n):
        cat = _row_category(r)
        cell = tk.Frame(self.inner, bg="#202020", padx=4, pady=4, cursor="hand2")
        cell._no_bg_slice = True
        cell.grid(row=n // self.COLS, column=n % self.COLS)
        img = cl.read_logo_image(r["path"]) if r.get("path") else None
        if img is not None:
            ph = _photo(img)
            self._photos.append(ph)
            lab = tk.Label(cell, image=ph, bg="black", bd=1, relief="solid")
        else:      # aucun visuel : meme taille qu'une miniature (une etiquette de texte n'aurait pas la meme)
            from PIL import Image, ImageDraw
            blank = Image.new("RGB", (cl.W, cl.H), (0, 0, 0))
            ImageDraw.Draw(blank).text((cl.W // 2 - 3, cl.H // 2 - 5), "—", fill=(150, 150, 150))
            ph = _photo(blank)
            self._photos.append(ph)
            lab = tk.Label(cell, image=ph, bg="black", bd=1, relief="solid")
        lab.pack()
        cap = tk.Label(cell, text=r["name"] if len(r["name"]) < 26 else r["name"][:23] + "…", bg="#202020", fg="#FFFFFF" if cat == "ok" else {"missing": "#FFB74D", "card": "#FFB74D", "avail": "#CE93D8", "dark": "#FF8A80", "custom": "#81C784"}.get(cat, "#FFFFFF"), font=("TkDefaultFont", 7), wraplength=150)
        cap.pack()
        for wdg in (cell, lab, cap):
            wdg.bind("<Button-1>", lambda e, rr=r: self.tab.goto(rr["system"], rr["stem"]))


AVAIL_HEAD = "#7B1FA2"      # en-tete de la fenetre de revue des logos de la Recalbox (violet : meme couleur que l'etat « Logo dispo »)


class NewLogosWindow(_Base):
    """Revue des logos que la Recalbox possede MAINTENANT pour des jeux que la SD affiche en carte texte (ou sans visuel) : on coche ceux a utiliser, rien n'est
    modifie avant « Appliquer ». L'application passe par l'import en lot (original conserve, store enregistre sur la SD et dans %APPDATA% = changements conserves,
    reappliques apres chaque copie, ecriture sur la SD) puis, si le Pi est la source, par un envoi au Pi en un lot."""
    PAGE = 40
    COLS = (34, 250, 150, 30, 150, 110)       # largeurs (px) des colonnes : case, jeu, SD actuel, fleche, Recalbox, etat

    def __init__(self, tab):
        super().__init__(tab.gui)
        self.tab = tab
        self.cands = [r for r in tab.rows if _is_avail(r) and not r.get("off_sd")]
        w = self.win = tk.Toplevel(tab.gui.root)
        w.title(self.t("av_title"))
        w.configure(bg=self.bg)
        w.geometry("940x560")
        w.minsize(800, 400)
        w.transient(tab.gui.root)
        self.vars = {r["key"]: tk.BooleanVar(master=w, value=True) for r in self.cands}
        for v in self.vars.values():
            v.trace_add("write", lambda *a: self._count())
        self._photos = []
        self._token = 0
        self.shown = 0
        self.vis = []

        head = tk.Frame(w, bg=AVAIL_HEAD)
        head.pack(fill="x")
        tk.Label(head, text=self.t("av_head"), bg=AVAIL_HEAD, fg="#FFFFFF", font=("TkDefaultFont", 11, "bold")).pack(side="left", padx=10, pady=4)
        tk.Label(head, text=self.t("av_hint"), bg=AVAIL_HEAD, fg="#EBDDF5").pack(side="left", padx=6)

        bar = tk.Frame(w, bg=self.bg)
        bar.pack(fill="x", padx=10, pady=6)
        self._btn(bar, "av_all", lambda: self._check_all(True)).pack(side="left")
        self._btn(bar, "av_none", lambda: self._check_all(False)).pack(side="left", padx=(4, 12))
        systems = sorted({r["system"] for r in self.cands})
        self.sys_var = tk.StringVar(master=w)
        self._every = self.t("av_every", n=len(systems))
        if len(systems) > 1:
            tk.Label(bar, text=self.t("av_sys"), bg=self.bg, fg=self.fg).pack(side="left")
            cb = ttk.Combobox(bar, textvariable=self.sys_var, values=[self._every] + systems, state="readonly", width=22)
            cb.pack(side="left", padx=4)
            cb.bind("<<ComboboxSelected>>", lambda e: self._render())
        self.sys_var.set(self._every)
        tk.Label(bar, text=self.t("av_note"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "italic")).pack(side="right")

        cols = tk.Frame(w, bg=self.bg)
        cols.pack(fill="x", padx=10)
        for i, (key, width) in enumerate(zip((None, "av_col_game", "av_col_cur", None, "av_col_new", None), self.COLS)):
            cols.grid_columnconfigure(i, minsize=width)
            if key:
                tk.Label(cols, text=self.t(key), bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8, "bold")).grid(row=0, column=i, sticky="w")

        host = tk.Frame(w, bg="#FFFFFF", bd=1, relief="solid")
        host._no_bg_slice = True
        host.pack(fill="both", expand=True, padx=10, pady=(2, 4))
        self.canvas = tk.Canvas(host, bg="#FFFFFF", highlightthickness=0)
        sb = tk.Scrollbar(host, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=sb.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        self.inner = tk.Frame(self.canvas, bg="#FFFFFF")
        self.inner._no_bg_slice = True
        win_id = self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.inner.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.bind("<Configure>", lambda e: self.canvas.itemconfigure(win_id, width=e.width))
        self.canvas.bind("<Enter>", lambda e: self.canvas.bind_all("<MouseWheel>", lambda ev: self.canvas.yview_scroll(int(-ev.delta / 120), "units")))
        self.canvas.bind("<Leave>", lambda e: self.canvas.unbind_all("<MouseWheel>"))
        self.more_btn = self._btn(host, "br_more", self._more)

        foot = tk.Frame(w, bg=self.bg)
        foot.pack(fill="x", padx=10, pady=(2, 8))
        self.count_lbl = tk.Label(foot, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8))
        self.count_lbl.pack(side="left")
        self._btn(foot, "av_close", w.destroy).pack(side="right")
        self.apply_btn = tk.Button(foot, text="", bg=self.bg_action, fg=self.fg, padx=10, command=self._apply)
        self.apply_btn.pack(side="right", padx=6)
        self._count()
        self._render()

    def _check_all(self, value):
        for r in self.vis:
            self.vars[r["key"]].set(value)

    def _count(self):
        n = sum(1 for v in self.vars.values() if v.get())
        self.apply_btn.config(text=self.t("av_apply", n=n), state="normal" if n else "disabled")
        self.count_lbl.config(text=self.t("av_count", n=n, t=len(self.cands)))

    def _render(self):
        self._token += 1
        for c in self.inner.winfo_children():
            c.destroy()
        self._photos.clear()
        self.shown = 0
        self.more_btn.pack_forget()
        sel = self.sys_var.get()
        self.vis = [r for r in self.cands if sel == self._every or r["system"] == sel]
        self._more()

    def _more(self):
        chunk = self.vis[self.shown:self.shown + self.PAGE]
        slots = []
        for r in chunk:
            slots.append(self._row(r, self.shown))
            self.shown += 1
        self.more_btn.pack_forget()
        if self.shown < len(self.vis):
            self.more_btn.pack(anchor="w", pady=4)
        token = self._token

        def work():
            return [(cl.read_logo_image(r["path"]) if r.get("path") else None, cl.preview_logo(r["rb_logo"])) for r in chunk]

        def done(res, err):
            if token != self._token or err is not None:
                return
            for (cur_lbl, new_lbl), (cur, new) in zip(slots, res):
                for lbl, img in ((cur_lbl, cur), (new_lbl, new)):
                    try:
                        ph = _photo(img if img is not None else self._dash())
                        self._photos.append(ph)
                        lbl.configure(image=ph)
                    except tk.TclError:
                        pass           # ligne detruite entre-temps (nouveau filtre)
        self.bg_run(self.win, work, done)

    @staticmethod
    def _dash():
        from PIL import Image, ImageDraw
        blank = Image.new("RGB", (cl.W, cl.H), (0, 0, 0))
        ImageDraw.Draw(blank).text((cl.W // 2 - 3, cl.H // 2 - 5), "—", fill=(150, 150, 150))
        return blank

    def _row(self, r, i):
        bg = "#FFFFFF" if i % 2 == 0 else "#F6F2FA"
        fr = tk.Frame(self.inner, bg=bg)
        fr.pack(fill="x")
        for c, width in enumerate(self.COLS):
            fr.grid_columnconfigure(c, minsize=width)
        tk.Checkbutton(fr, variable=self.vars[r["key"]], bg=bg, fg="#000000", activebackground=bg, selectcolor="#FFFFFF").grid(row=0, column=0, rowspan=1, padx=(4, 0))
        name = tk.Frame(fr, bg=bg)
        name.grid(row=0, column=1, sticky="w")
        tk.Label(name, text=r["name"] if len(r["name"]) < 34 else r["name"][:31] + "…", bg=bg, fg="#000000", font=("TkDefaultFont", 9, "bold"), anchor="w").pack(anchor="w")
        tk.Label(name, text=r["system"], bg=bg, fg="#555555", font=("TkDefaultFont", 8), anchor="w").pack(anchor="w")
        ph = _photo(self._dash())
        self._photos.append(ph)
        cur = tk.Label(fr, image=ph, bg="black", bd=1, relief="solid")
        cur.grid(row=0, column=2, padx=2, pady=3)
        tk.Label(fr, text="→", bg=bg, fg=AVAIL_HEAD, font=("TkDefaultFont", 12, "bold")).grid(row=0, column=3)
        new = tk.Label(fr, image=ph, bg="black", bd=1, relief="solid")
        new.grid(row=0, column=4, padx=2, pady=3)
        tk.Label(fr, text=self.t("av_cur_dark" if r.get("dark") else "av_cur_card" if r.get("was_card") else "av_cur_none"), bg=bg, fg="#B26A00", font=("TkDefaultFont", 8), anchor="w").grid(row=0, column=5, sticky="w", padx=6)
        return cur, new

    def _apply(self):
        """Valide les logos coches : import en lot (une seule ecriture du store), puis envoi au Pi en un lot si le Pi est la source."""
        from pathlib import Path
        tab = self.tab
        sel = [r for r in self.cands if self.vars[r["key"]].get()]
        if not sel or not (tab.sd_root and tab.store_ok):
            return
        tab.status_lbl.config(text=tab.t("loading"))
        self.win.update_idletasks()
        try:
            res = cl.import_batch(tab.sd_root, tab.store, [(Path(r["rb_logo"]), r) for r in sel], log=tab._log)
            tab._save_stores()
        except Exception as e:   # noqa: BLE001
            return messagebox.showerror(tab.t("sd"), tab.t("err", err=e), parent=self.win)
        msg = tab.t("av_done", n=res["done"], e=len(res["errors"]))
        tab.status_lbl.config(text=msg)
        tab._log(msg)
        keys = [r["key"] for r in sel if cl.find_item(tab.store, r["key"]) is not None]
        tab._pi_after_write(keys)
        tab._load_system(tab.system, note=False)
        self.win.destroy()


class ReplacementWindow(_Base):
    """Fenetre « Choisir le remplacant » : la CIBLE (le jeu et son visuel actuel sur la SD / le Pi) reste figee dans un bandeau, les SOURCES du remplacant sont a
    gauche (logos des ROMs de la Recalbox, logos des autres jeux de la SD = clone, dossier gifs, fichier / dossier / presse-papiers, carte texte), les miniatures
    au centre, l'apercu du remplacant (au survol) a droite. Rien n'est modifie ici : « Utiliser ce remplacant » place le choix en attente dans la fenetre
    principale, ou « Valider » l'ecrit (et l'envoie au Pi)."""
    PAGE, COLS = 60, 3
    HEAD = "#1F5FBF"       # bleu : la fenetre principale est grise, la revue des logos de la Recalbox violette
    SOURCES = (("roms", "rw_s_roms"), ("sd", "rw_s_sd"), ("gifs", "rw_s_gifs"), ("file", "rw_s_file"), ("card", "rw_s_card"))

    def __init__(self, tab, row):
        super().__init__(tab.gui)
        self.tab, self.row = tab, row
        self.cands, self.vis, self.shown = [], [], 0
        self.sel = None
        self._photos, self._token, self._pv_photo = [], 0, None
        self.source = None
        w = self.win = tk.Toplevel(tab.gui.root)
        w.title(self.t("rw_title"))
        w.configure(bg=self.bg)
        w.geometry("1120x620")
        w.minsize(980, 500)
        w.transient(tab.gui.root)

        head = tk.Frame(w, bg=self.HEAD)
        head.pack(fill="x")
        tk.Label(head, text=self.t("rw_head"), bg=self.HEAD, fg="#FFFFFF", font=("TkDefaultFont", 11, "bold")).pack(side="left", padx=10, pady=4)
        tk.Label(head, text=self.t("rw_hint"), bg=self.HEAD, fg="#DDE8FF").pack(side="left", padx=6)

        tgt = tk.Frame(w, bg="#E3E8EC", highlightthickness=1, highlightbackground="#9AA5AD")      # CIBLE : fixe, jamais melangee a ce qu'on parcourt
        tgt.pack(fill="x", padx=10, pady=(6, 4))
        cv = tk.Canvas(tgt, width=258, height=66, bg="black", highlightthickness=1, highlightbackground="#888888")
        cv._no_bg_slice = True
        cv.pack(side="left", padx=6, pady=4)
        self._draw_on(cv, cl.read_logo_image(row["path"]) if row.get("path") else None)
        txt = tk.Frame(tgt, bg="#E3E8EC")
        txt.pack(side="left", padx=6)
        tk.Label(txt, text=self.t("rw_target"), bg="#E3E8EC", fg="#000000", font=("TkDefaultFont", 8, "bold")).pack(anchor="w")
        tk.Label(txt, text=f"{row['name']}  —  {row['system']}", bg="#E3E8EC", fg="#000000", font=("TkDefaultFont", 10, "bold")).pack(anchor="w")
        where = tab.pi_ip and f"Raspberry Pi {tab.pi_ip}" or "SD"
        tk.Label(txt, text=self.t("rw_now", state=tab._state_text(row)) + f"      →  {where}", bg="#E3E8EC", fg="#000000", font=("TkDefaultFont", 8)).pack(anchor="w")

        body = tk.Frame(w, bg=self.bg)
        body.pack(fill="both", expand=True, padx=10, pady=2)
        src = tk.Frame(body, bg=self.bg)
        src.pack(side="left", fill="y")
        tk.Label(src, text=self.t("rw_where"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "bold")).pack(anchor="w")
        self.lb = tk.Listbox(src, width=34, height=len(self.SOURCES), exportselection=False, activestyle="none", selectbackground=self.HEAD, selectforeground="white")
        for _k, key in self.SOURCES:
            self.lb.insert("end", self.t(key))
        self.lb.pack(anchor="w")
        self.lb.bind("<<ListboxSelect>>", lambda e: self._on_source())

        right = tk.Frame(body, bg=self.bg)           # empaquete AVANT le centre : il garde sa largeur quand la fenetre se reduit
        right.pack(side="right", fill="y", padx=(8, 0))
        tk.Label(right, text=self.t("rw_preview"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "bold")).pack(anchor="w")
        self.pv_cv = tk.Canvas(right, width=258, height=66, bg="black", highlightthickness=1, highlightbackground="#888888")
        self.pv_cv._no_bg_slice = True
        self.pv_cv.pack(anchor="w")
        self.pv_lbl = tk.Label(right, text=self.t("rw_hoverhint"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), justify="left", anchor="nw", wraplength=258, height=3)      # 3 lignes fixes : un nom long ne decale pas ce qui est dessous
        self.pv_lbl.pack(anchor="w", pady=2)
        self.mode_box = tk.Frame(right, bg=self.bg)
        tk.Label(self.mode_box, text=self.t("rw_mode"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "bold")).pack(anchor="w")
        self.mode_var = tk.StringVar(master=w, value=self.t("m_pixel"))
        self.mode_cb = ttk.Combobox(self.mode_box, textvariable=self.mode_var, values=[self.t("m_" + m) for m in cl.MODES], state="readonly", width=26)
        self.mode_cb.pack(anchor="w")
        self.mode_cb.bind("<<ComboboxSelected>>", lambda e: self._show_preview(self.sel))
        tk.Label(self.mode_box, text=self.t("rw_mode_note"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8)).pack(anchor="w")
        # GIF anime selectionne : « Tout le GIF » ou UNE image (curseur) -- affiche seulement pour une animation (voir _clip_setup) ; cree AVANT la note : mode_box se place avant le dernier enfant
        self.clip_mode = tk.StringVar(master=w, value="all")
        self.clip_pos = tk.IntVar(master=w, value=1)
        self._clip_n, self._clip_token, self._clip_job = 0, 0, None
        self.clip_box = tk.Frame(right, bg=self.bg)
        tk.Label(self.clip_box, text=self.t("rw_anim_title"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8, "bold")).pack(anchor="w")
        self.clip_info = tk.Label(self.clip_box, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8))
        self.clip_info.pack(anchor="w")
        for val, key in (("all", "rw_anim_all"), ("frame", "rw_anim_one")):
            tk.Radiobutton(self.clip_box, text=self.t(key), variable=self.clip_mode, value=val, command=self._clip_changed, bg=self.bg, fg=self.fg,
                           selectcolor=self.bg, activebackground=self.bg).pack(anchor="w")
        self.clip_scale = tk.Scale(self.clip_box, orient="horizontal", from_=1, to=2, showvalue=False, variable=self.clip_pos, command=self._clip_moved, length=250,
                                   bg=self.bg, fg=self.fg, highlightthickness=0)
        self.clip_scale.pack(anchor="w")
        self.clip_lbl = tk.Label(self.clip_box, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8))
        self.clip_lbl.pack(anchor="w")
        self.gif_note = tk.Label(right, text=self.t("rw_gif_note"), bg="#FFF4D6", fg="#000000", font=("TkDefaultFont", 8), justify="left", wraplength=258, highlightthickness=1,
                                 highlightbackground="#D9B84A")
        self.gif_note.pack(anchor="w", fill="x", pady=(14, 0))

        mid = tk.Frame(body, bg=self.bg)
        mid.pack(side="left", fill="both", expand=True, padx=8)
        self.ctl = tk.Frame(mid, bg=self.bg)
        self.ctl.pack(fill="x")
        tk.Label(self.ctl, text=self.t("rw_system"), bg=self.bg, fg=self.fg).pack(side="left")
        self.sys_var = tk.StringVar(master=w)
        self.sys_cb = ttk.Combobox(self.ctl, textvariable=self.sys_var, state="readonly", width=15)
        self.sys_cb.pack(side="left", padx=4)
        self.sys_cb.bind("<<ComboboxSelected>>", lambda e: self._load())
        tk.Label(self.ctl, text=tab.t("filter"), bg=self.bg, fg=self.fg).pack(side="left", padx=(8, 0))
        self.flt_var = tk.StringVar(master=w)
        self.flt_var.trace_add("write", lambda *a: self._render())
        tk.Entry(self.ctl, textvariable=self.flt_var, width=13).pack(side="left", padx=4)
        self.found_lbl = tk.Label(self.ctl, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8))
        self.found_lbl.pack(side="left", padx=8)

        self.host = tk.Frame(mid, bg="#FFFFFF", bd=1, relief="solid")
        self.host._no_bg_slice = True
        self.host.pack(fill="both", expand=True, pady=4)
        self.canvas = tk.Canvas(self.host, bg="#FFFFFF", highlightthickness=0)
        sb = tk.Scrollbar(self.host, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=sb.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        self.inner = tk.Frame(self.canvas, bg="#FFFFFF")
        self.inner._no_bg_slice = True
        self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.inner.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.bind("<Enter>", lambda e: self.canvas.bind_all("<MouseWheel>", lambda ev: self.canvas.yview_scroll(int(-ev.delta / 120), "units")))
        self.canvas.bind("<Leave>", lambda e: self.canvas.unbind_all("<MouseWheel>"))
        self.more_btn = self._btn(self.inner, "br_more", self._more)
        self.panel = tk.Frame(self.host, bg="#FFFFFF")       # messages, source « fichier », source « carte texte » (remplace la grille)
        self.panel_lbl = tk.Label(self.panel, text="", bg="#FFFFFF", fg="#000000", justify="left", wraplength=480, anchor="w")
        self.panel_lbl.pack(anchor="w", padx=14, pady=(14, 6))
        self.panel_btn = tk.Button(self.panel, text="", command=self._legacy, bg=self.bg_action, fg=self.fg, padx=10)
        self.card_row = tk.Frame(self.panel, bg="#FFFFFF")
        tk.Label(self.card_row, text=self.t("rw_card_title"), bg="#FFFFFF", fg="#000000").pack(side="left")
        self.card_var = tk.StringVar(master=w, value=row["name"])
        self.card_var.trace_add("write", lambda *a: self._card_soon())
        tk.Entry(self.card_row, textvariable=self.card_var, width=40).pack(side="left", padx=6)
        self._card_job = None

        foot = tk.Frame(w, bg=self.bg)
        foot.pack(fill="x", padx=10, pady=(2, 8))
        self._btn(foot, "cancel", w.destroy).pack(side="right")
        self.use_btn = tk.Button(foot, text=self.t("rw_use"), bg=self.bg_action, fg=self.fg, padx=10, command=self._use, state="disabled")
        self.use_btn.pack(side="right", padx=6)

        self.lb.selection_set(0)
        self._on_source()

    # ------------------------------------------------------------------ utilitaires
    def _draw_on(self, cv, img):
        cv.delete("all")
        if img is None:
            cv.create_text(129, 33, text=self.t("none"), fill="#888888", font=("TkDefaultFont", 14))
            return
        ph = _photo(cl.led_style(img, 2, self.tab.bright_var.get()))
        self._photos.append(ph)
        cv.create_image(0, 0, image=ph, anchor="nw")

    def _mode(self):
        for m in cl.MODES:
            if self.t("m_" + m) == self.mode_var.get():
                return m
        return "pixel"

    def _img(self, c, mode="pixel"):
        """Image 128x32 d'un candidat : fichier de la Recalbox (converti comme un logo) ou visuel deja au format de la dalle (1re image)."""
        if c["kind"] == "file":
            try:
                return cl.preview_logo(self._local(c), mode=mode)
            except OSError:
                return None          # GIF du Pi non telecharge (reseau) : tuile vide
        if c["kind"] == "card":
            return c["item"][2]
        return cl.read_logo_image(c["path"])

    # ------------------------------------------------------------------ sources
    def _notice(self, text, button=None, card=False):
        """Remplace la grille par un message (source indisponible), le selecteur de fichiers ou la saisie du titre de la carte."""
        self.ctl.pack_forget()
        self.canvas.pack_forget()
        self.panel.pack(fill="both", expand=True)
        self.panel_lbl.config(text=text)
        self.panel_btn.pack_forget()
        self.card_row.pack_forget()
        if button:
            self.panel_btn.config(text=button)
            self.panel_btn.pack(anchor="w", padx=14, pady=4)
        if card:
            self.card_row.pack(anchor="w", padx=14, pady=4)

    def _grid_mode(self, with_system=True):
        self.panel.pack_forget()
        self.canvas.pack(side="left", fill="both", expand=True)
        for w in (self.ctl,):
            w.pack_forget()
            w.pack(fill="x", before=self.host)
        for child in self.ctl.winfo_children()[:2]:
            (child.pack if with_system else child.pack_forget)(**({"side": "left", "padx": 4} if with_system else {}))

    def _on_source(self):
        sel = self.lb.curselection()
        if not sel:
            return
        self.source = self.SOURCES[sel[0]][0]
        self._token += 1
        self.cands, self.vis = [], []
        self._set_sel(None)
        tab = self.tab
        if self.source == "roms":
            systems = sorted(tab.roms_map)
            if not systems:
                return self._notice(self.t("rw_na_roms"))
            self._systems(systems)
        elif self.source == "sd":
            systems = sorted(tab.sd_systems)
            if not (tab.sd_root and systems):
                return self._notice(self.t("rw_na_sd"))
            self._systems(systems)
        elif self.source == "gifs":
            if not tab.sd_root:
                return self._notice(self.t("rw_na_sd"))
            if tab.pi_ip:
                return self._load_pi_gifs()
            self._grid_mode(with_system=False)
            self.cands = [{"kind": "raw" if g["kind"] == "raw" else "pack", "path": g["path"], "label": g["label"], "sub": "gifs"} for g in cl.list_gifs(tab.sd_root)]
            self._render()
        elif self.source == "file":
            self._notice(self.t("rw_file_text"), button=self.t("rw_file_btn"))
        else:
            self._notice("", card=True)
            self._card_now()

    def _pi_store_get(self):
        """Magasin du Pi, PARTAGE par l'onglet : le GIF choisi est relu plus tard (extrait, capture d'une image) alors que cette fenetre est deja fermee, son cache
        telecharge doit donc lui survivre (dossier temporaire « gifs_pi_* » libere au changement de Pi)."""
        tab = self.tab
        st = getattr(tab, "_pi_gif_store", None)
        if st is None or st.ip != tab.pi_ip:
            if st is not None:
                st.close()
            import game_gifs_store as gs
            st = tab._pi_gif_store = gs.PiStore(tab.pi_ip)
        return st

    def _load_pi_gifs(self):
        """Dossier /gifs du Raspberry Pi (route /gg-ls, demon >= 3.87) : ce sont les GIF d'ORIGINE (<dossier>/<nom>.gif, un niveau), pas des .raw565 comme sur la SD ; ils sont
        proposes comme des fichiers (apercu et conversion identiques a un GIF choisi sur le PC) et telecharges a la demande (miniatures par lots, puis au choix)."""
        self._grid_mode(with_system=False)
        self.found_lbl.config(text=self.t("rw_pi_loading"))
        token = self._token

        def work():
            store = self._pi_store_get()
            ok, items = store.ls("/gifs")
            if not ok:
                return []
            out = []
            for name, is_dir, _sz in items:
                if not is_dir or name.startswith("_"):
                    continue
                ok2, sub = store.ls(f"/gifs/{name}")
                for fn, d2, _s2 in sub if ok2 else []:
                    if not d2 and fn.lower().endswith((".gif", ".webp", ".png")):
                        out.append({"kind": "file", "path": None, "store_path": f"/gifs/{name}/{fn}", "label": f"{name}/{Path(fn).stem}", "sub": "gifs Pi"})
            return sorted(out, key=lambda c: c["label"].lower())

        def done(res, err):
            if token != self._token or self.source != "gifs":
                return
            if err is not None:
                return self._notice(self.t("rw_pi_ko", err=err))
            self.cands = res or []
            self._render()
        self.bg_run(self.win, work, done)

    def _local(self, c):
        """Chemin local d'un candidat ; un GIF du Pi est telecharge ici (cache temporaire de la fenetre) la premiere fois."""
        if c.get("store_path") and not c.get("path"):
            c["path"] = self._pi_store_get().local_file(c["store_path"])
        return c["path"]

    def _systems(self, systems):
        self._grid_mode(with_system=True)
        self.sys_cb["values"] = systems
        self.sys_var.set(self.row["system"] if self.row["system"] in systems else systems[0])
        self._load()

    def _load(self):
        """Candidats du systeme choisi (hors thread d'interface : un dossier ROMs sur le reseau peut etre lent)."""
        tab, system, source, row = self.tab, self.sys_var.get(), self.source, self.row
        self._token += 1
        token = self._token
        self.found_lbl.config(text=self.t("loading"))

        def work():
            out = []
            if source == "roms":
                xml = tab.roms_map[system]
                rb = cl.rb_logos(xml, tab._rb_tag())
                for g in cl.games_from_gamelist(xml):
                    stem = (g.get("stem") or cl.stem_for_rom(g["rom"])).lower()
                    if stem in rb and not (system == row["system"] and stem == row["stem"].lower()):
                        out.append({"kind": "file", "path": rb[stem], "label": g["name"], "sub": system})
            else:
                for stem, (path, kind) in cl.existing_logos(tab.sd_root, system).items():
                    if not (system == row["system"] and stem == row["stem"].lower()):
                        out.append({"kind": "raw" if kind == "raw" else "pack", "path": path, "label": path.stem, "sub": system})
            return sorted(out, key=lambda c: c["label"].lower())

        def done(res, err):
            if token != self._token:
                return
            self.cands = [] if err is not None else res
            self._render()
        self.bg_run(self.win, work, done)

    # ------------------------------------------------------------------ grille
    def _render(self):
        self._token += 1
        for c in self.inner.winfo_children():
            if c is not self.more_btn:
                c.destroy()
        self.more_btn.grid_forget()
        self._photos.clear()
        self.shown = 0
        flt = self.flt_var.get().strip().lower()
        self.vis = [c for c in self.cands if not flt or flt in c["label"].lower()]
        self.found_lbl.config(text=self.t("rw_found", n=len(self.vis)) if self.vis else self.t("rw_empty"))
        self._more()

    def _more(self):
        from PIL import Image
        chunk = self.vis[self.shown:self.shown + self.PAGE]
        labels = []
        for c in chunk:
            n = self.shown
            cell = tk.Frame(self.inner, bg="#FFFFFF", padx=3, pady=3, highlightthickness=2, highlightbackground="#FFFFFF", cursor="hand2")
            cell.grid(row=n // self.COLS, column=n % self.COLS, sticky="w", padx=2, pady=2)
            ph = _photo(NewLogosWindow._dash())
            self._photos.append(ph)
            img_lbl = tk.Label(cell, image=ph, bg="black", bd=1, relief="solid")
            img_lbl.pack(anchor="w")
            cap = tk.Label(cell, text=c["label"] if len(c["label"]) < 28 else c["label"][:25] + "…", bg="#FFFFFF", fg="#000000", font=("TkDefaultFont", 8), anchor="w")
            cap.pack(anchor="w")
            c["_cell"] = cell
            for wdg in (cell, img_lbl, cap):
                wdg.bind("<Enter>", lambda e, cc=c: (self._hover_cell(cc), self._show_preview(cc)))
                wdg.bind("<Leave>", lambda e, cc=c: (self._hover_cell(None), self._show_preview(self.sel)))
                wdg.bind("<Button-1>", lambda e, cc=c: self._set_sel(cc))
            labels.append(img_lbl)
            self.shown += 1
        self.more_btn.grid_forget()
        if self.shown < len(self.vis):
            self.more_btn.grid(row=(self.shown // self.COLS) + 1, column=0, columnspan=self.COLS, pady=6)
        token = self._token

        def work():
            res = []
            for c in chunk:
                img = self._img(c)
                c["img"] = img
                res.append(img.resize((cl.W + 32, cl.H + 8), Image.NEAREST) if img is not None else None)
            return res

        def done(res, err):
            if token != self._token or err is not None:
                return
            for lbl, img in zip(labels, res):
                try:
                    ph = _photo(img if img is not None else NewLogosWindow._dash())
                    self._photos.append(ph)
                    lbl.configure(image=ph)
                except tk.TclError:
                    pass
        self.bg_run(self.win, work, done)

    # ------------------------------------------------------------------ apercu / choix
    # ---- GIF anime : apercu anime et choix « tout le GIF » / une image
    ANIM_MAX = 120          # images gardees pour l'apercu anime (au-dela : 1 image sur k, delais additionnes : meme duree)

    def _is_animated_cand(self, c):
        """Candidat qui peut etre une animation : paquet .raw565pack, ou GIF / WebP (les PNG des logos de la Recalbox sont fixes)."""
        if c is None:
            return False
        if c["kind"] == "pack":
            return True
        return c["kind"] == "file" and (c.get("store_path") or str(c.get("path") or "")).lower().endswith((".gif", ".webp"))

    def _anim_stop(self):
        self._anim_token = getattr(self, "_anim_token", 0) + 1
        job = getattr(self, "_anim_job", None)
        if job:
            try:
                self.win.after_cancel(job)
            except tk.TclError:
                pass
        self._anim_job = None

    def _load_frames(self, c, mode):
        """([images RGB 128x32], [delais ms]) d'un candidat anime, hors thread d'interface ; au plus ANIM_MAX images (echantillonnage, delais additionnes)."""
        import struct
        if c["kind"] == "pack":
            path = c["path"]
            n = cl.pack_frame_count(path)
            delays = [100] * n
            meta = Path(path).with_suffix(".meta")
            if n and meta.is_file():
                raw = meta.read_bytes()
                if len(raw) == 2 * n:
                    delays = [max(1, d) for d in struct.unpack(f"<{n}H", raw)]
            step = max(1, -(-n // self.ANIM_MAX))
            frames, out = [], []
            for i in range(0, n, step):
                img = cl.pack_frame_image(path, i)
                if img is not None:
                    frames.append(img)
                    out.append(sum(delays[i:i + step]))
            return frames, out
        from PIL import ImageSequence
        path = self._local(c)
        frames, out = [], []
        with cl._tool()._open_image_tolerant(Path(path)) as im:
            n = max(1, int(getattr(im, "n_frames", 1)))
            step = max(1, -(-n // self.ANIM_MAX))
            fallback = im.info.get("duration", 50)
            for i, fr in enumerate(ImageSequence.Iterator(im)):
                try:
                    d = int(fr.info.get("duration", fallback))
                except (TypeError, ValueError):
                    d = 10
                d = max(1, d if d > 0 else 10)
                if i % step == 0:
                    frames.append(cl.render(fr.convert("RGBA"), mode, (0, 0, 0)))
                    out.append(d)
                else:
                    out[-1] += d
        return frames, out

    def _anim_for(self, c):
        """Joue l'animation du candidat dans l'apercu de droite (boucle, delais du GIF, >= 60 ms). Pas d'animation quand UNE image est choisie."""
        self._anim_stop()
        if not self._is_animated_cand(c) or (c is self.sel and self._clip_n > 1 and self.clip_mode.get() == "frame"):
            return
        token, mode = self._anim_token, self._mode()

        def work():
            cached = c.get("_anim")
            if cached and cached[0] == mode:
                return cached
            frames, delays = self._load_frames(c, mode)
            return (mode, frames, delays)

        def done(res, err):
            if token != self._anim_token or err is not None or not res or len(res[1]) < 2:
                return
            c["_anim"] = res
            frames, delays = res[1], res[2]
            photos = [None] * len(frames)

            def step(i):
                if token != self._anim_token:
                    return
                try:
                    if photos[i] is None:
                        photos[i] = _photo(cl.led_style(frames[i], 2, self.tab.bright_var.get()))
                    self._anim_photos = photos            # reference gardee : Tk perd l'image sinon
                    self.pv_cv.delete("all")
                    self.pv_cv.create_image(0, 0, image=photos[i], anchor="nw")
                    self._anim_job = self.win.after(max(60, delays[i]), lambda: step((i + 1) % len(frames)))
                except tk.TclError:
                    pass
            step(1)
        self.bg_run(self.win, work, done)

    def _clip_setup(self, c):
        """Candidat choisi : s'il est anime (plusieurs images), propose « Tout le GIF » ou UNE image (curseur) dans le cadre de selection."""
        self._clip_token += 1
        token = self._clip_token
        self.clip_mode.set("all")
        self.clip_pos.set(1)
        self._clip_n = 0
        self.clip_box.pack_forget()
        if not self._is_animated_cand(c):
            return

        def work():
            if c["kind"] == "pack":
                return cl.pack_frame_count(c["path"])
            return cl.gif_frame_count(self._local(c))

        def done(n, err):
            if token != self._clip_token or err is not None or not n or n < 2:
                return
            self._clip_n = n
            self.clip_scale.config(to=n)
            self.clip_info.config(text=self.t("rw_anim_n", n=n))
            self.clip_lbl.config(text=self.t("rw_anim_pos", k=1, n=n))
            self.clip_box.pack(anchor="w", fill="x", pady=(8, 0), before=self.gif_note)
        self.bg_run(self.win, work, done)

    def _clip_changed(self):
        self._show_preview(self.sel)

    def _clip_moved(self, _v=None):
        """Curseur deplace : choisit « une image » et montre CETTE image (rendu differe : relire une image d'un gros GIF prend du temps)."""
        if self._clip_n < 2:
            return
        self.clip_mode.set("frame")
        self.clip_lbl.config(text=self.t("rw_anim_pos", k=self.clip_pos.get(), n=self._clip_n))
        if self._clip_job:
            try:
                self.win.after_cancel(self._clip_job)
            except tk.TclError:
                pass
        self._clip_job = self.win.after(120, lambda: (setattr(self, "_clip_job", None), self._show_preview(self.sel)))

    def _frame_img(self, c, mode):
        """Image 128x32 de l'image choisie au curseur (candidat choisi)."""
        k = self.clip_pos.get() - 1
        try:
            if c["kind"] == "pack":
                return cl.pack_frame_image(c["path"], k)
            return cl.preview_frame(self._local(c), k, mode)
        except OSError:
            return None

    def _show_preview(self, c):
        if c is None:
            self._anim_stop()
            self.pv_cv.delete("all")
            self.pv_lbl.config(text=self.t("rw_hoverhint"))
            self.mode_box.pack_forget()
            return
        if c is self.sel and self._clip_n > 1 and self.clip_mode.get() == "frame" and self._is_animated_cand(c):
            img = self._frame_img(c, self._mode())
        else:
            img = c.get("img") if (c["kind"] != "file" or self._mode() == "pixel") else None
            if img is None:
                img = self._img(c, self._mode())
        self._draw_on(self.pv_cv, img)
        self._anim_for(c)
        self.pv_lbl.config(text=f"{c['label']} — {c.get('sub', '')}".rstrip(" —"))
        if c["kind"] == "file":
            self.mode_box.pack(anchor="w", pady=(8, 0), before=self.pv_lbl.master.winfo_children()[-1])
        else:
            self.mode_box.pack_forget()

    def _hover_cell(self, c):
        """Cadre colore de la tuile survolee (bleu clair, comme l'onglet GIFs de jeux) ; la tuile CHOISIE garde son cadre d'en-tete."""
        old = getattr(self, "_hov", None)
        if old is not None and old is not self.sel and old.get("_cell") is not None:
            try:
                old["_cell"].configure(highlightbackground="#FFFFFF")
            except tk.TclError:
                pass
        self._hov = c
        if c is not None and c is not self.sel and c.get("_cell") is not None:
            try:
                c["_cell"].configure(highlightbackground="#7FB2FF")
            except tk.TclError:
                pass

    def _set_sel(self, c):
        if self.sel is not None and self.sel.get("_cell") is not None:
            try:
                self.sel["_cell"].configure(highlightbackground="#FFFFFF")
            except tk.TclError:
                pass
        self.sel = c
        if c is not None and c.get("_cell") is not None:
            c["_cell"].configure(highlightbackground=self.HEAD)
        self.use_btn.config(state="normal" if c is not None else "disabled")
        self._clip_setup(c)             # AVANT l'apercu : remet « Tout le GIF » et masque / affiche le cadre du choix d'image
        self._show_preview(c)

    def _card_soon(self):
        if self._card_job:
            self.win.after_cancel(self._card_job)
        self._card_job = self.win.after(250, self._card_now)

    def _card_now(self):
        self._card_job = None
        if self.source != "card":
            return
        title = self.card_var.get().strip()
        if not title:
            return self._set_sel(None)
        try:
            tup = cl.make_card_item(self.row["system"], self.row["rom"], title, stem=self.row["stem"])
        except Exception as e:   # noqa: BLE001
            self.panel_lbl.config(text=self.t("rw_err", err=e))
            return self._set_sel(None)
        self.panel_lbl.config(text="")
        self._set_sel({"kind": "card", "item": tup, "label": title, "sub": ""})

    def _legacy(self):
        self.win.destroy()
        self.tab._open_legacy_picker()

    def _use(self):
        c = self.sel
        if c is None:
            return
        tab, mode = self.tab, self._mode()          # le mode est lu AVANT la destruction de la fenetre (les variables Tk disparaissent avec elle)
        one = self._clip_n > 1 and self.clip_mode.get() == "frame"
        k, n = self.clip_pos.get() - 1, self._clip_n
        if c["kind"] == "file":
            try:
                path = str(self._local(c))          # un GIF du Pi est telecharge AVANT la destruction de la fenetre (son cache en depend)
                if self._is_animated_cand(c) and n < 2:
                    n = cl.gif_frame_count(path)    # le comptage d'arriere-plan n'est pas fini : on compte ici
            except OSError as e:
                return messagebox.showerror(self.t("sd"), self.t("rw_pi_ko", err=e), parent=self.win)
        self._anim_stop()
        self.win.destroy()
        if c["kind"] == "file":
            # choix fait DANS la fenetre : plus de question apres coup (« animer ? ») -- tout le GIF, ou l'image du curseur
            sel = (("still", k) if one else ("anim", 0, n - 1)) if n > 1 else None
            tab._on_picked(path, mode, (0, 0, 0), sel=sel)
        elif c["kind"] == "card":
            tab._set_pending(c["item"])
        elif one:
            tab._on_raw_frame_picked(c, k)
        else:
            tab._on_raw_picked(c)
