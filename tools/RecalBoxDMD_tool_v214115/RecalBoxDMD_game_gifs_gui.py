# -*- coding: utf-8 -*-
"""Fenetre « GIFs du jeu » (Mode 21) : voir, pour chaque jeu d'un systeme, quels GIFs de /gifs le DMD jouera (detection par TITRE = regle du firmware, gifmatch.py),
corriger une detection fausse en RENOMMANT le GIF, importer un GIF pour un jeu qui n'en a pas, lister les jeux sans GIF. Logique : game_gifs.py (sans Tk, testee).

Aucune liste a maintenir : le nom du fichier GIF est la seule donnee. Bibliotheque de 500 000 roms : un systeme a la fois, detection en arriere-plan.
"""
import shutil
import tempfile
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import custom_logos as cl
import game_gifs as gg
import game_gifs_store as gs
import gifmatch as gm

MAX_SHOWN = 2500          # lignes inserees au plus (le reste s'atteint en filtrant)
ALL_KEY = "\x00all"       # valeur interne de « Tous » dans la liste des systemes
PV_W, PV_H = 512, 128    # apercu : 128x32 x 4 (meme echelle que les apercus de l'onglet de remplacement de logo)
MAX_PREVIEW_FRAMES = 150

T = {
    "fr": {
        "title": "GIFs du jeu — détection par titre", "sd": "Carte SD du DMD :", "browse": "Parcourir…", "auto": "Détecter", "load": "Charger",
        "system": "Système :", "sys_all": "Tous", "filter": "Filtrer :", "view": "Afficher :", "v_all": "Tous les jeux", "v_none": "Sans GIF", "v_has": "Avec GIF", "v_dir": "Copié dans /gifs_jeux", "v_weak": "Détection approximative (à vérifier)",
        "col_title": "Jeu", "col_gifs": "GIFs", "col_dir": "Dossier", "sec_dir": "Dans le dossier /gifs_jeux (lu par le DMD en mode dossier) :", "sec_sd": "Sur la carte SD, dans /gifs (correspondance sûre) :", "sec_pi": "Sur le Raspberry Pi, dans /gifs (correspondance sûre) :", "no_sd": "Choisissez la carte SD du DMD (dossier qui contient « gifs »).", "no_games": "Aucune liste de jeux sur cette carte SD (instantané du Mode 1 absent) : lancez le Mode 1 sur cette carte, puis revenez ici.", "src_snap": "Liste des jeux : dernier Mode 1 de cette carte ({date})", "src_sd": "Pas d'instantané du Mode 1 : liste = jeux ayant un visuel sur la SD (titres = noms de fichier)",
        "clear_btn": "Tout supprimer", "tip_clear_btn": "Supprime TOUS les GIFs du dossier /gifs_jeux (après confirmation) : le DMD repasse en mode auto. Les originaux de /gifs ne sont pas touchés.", "clear_confirm": "Supprimer les {n} GIF(s) du dossier /gifs_jeux ?\n\nLe dossier sera vidé et le DMD repassera en mode auto. Les originaux de /gifs ne sont pas touchés.", "clear_done": "Dossier vidé : {n} GIF(s) supprimé(s). Retour au mode auto.", "clear_empty": "Le dossier /gifs_jeux est déjà vide (ou absent) : rien à supprimer.",
        "loading": "Lecture des jeux et détection…", "stats": "{n} jeux · {has} avec GIF · {none} sans GIF · {gifs} GIFs dans /gifs", "shown": " (affichés : {s})",
        "sel_none": "Sélectionnez un jeu.", "sel_title": "Jeu : {name}", "none_found": "Aucun GIF ne correspond à ce titre.",
        "sure": "correspondance sûre (nom identique au titre, au préfixe système et au numéro près)", "rank3": "nom identique au titre", "rank2": "le nom du GIF est contenu dans le titre (approximatif : il peut aussi servir à un autre jeu)", "rank1": "le titre est contenu dans le nom du GIF (incertain)",
        "rename": "Renommer…", "import": "Importer un GIF…", "rename_title": "Renommer le GIF", "rename_ask": "Nouveau nom (.gif) :",
        "rename_ok_match": "Après renommage, ce GIF correspondra EXACTEMENT au titre « {name} ».", "rename_weak": "Après renommage, ce GIF ne correspondra pas à ce jeu.",
        "rename_suggest": "Proposer le titre", "ok": "Valider", "cancel": "Annuler", "renamed": "Renommé : {old} → {new}\nPlaylists mises à jour : {pl} · cache maître : {ma} ligne(s).",
        "mode_folder": "Dossier dédié /gifs_jeux présent : le DMD ne lit QUE lui ({n} GIF(s)).", "mode_auto": "Pas de dossier /gifs_jeux : mode auto (correspondances strictes sur /gifs).",
        "help_btn": "? Aide", "help_text": "Trois modes de lecture sur le DMD, selon le contenu du dossier /gifs_jeux :\n\n• AUTO — pas de dossier, ou dossier sans GIF : le DMD cherche dans /gifs les GIFs dont le nom est exactement le titre du jeu.\n\n• EXCLUSIF — le dossier contient au moins un GIF (réglage « mode mixte » décoché) : le DMD ne lit QUE le dossier ; un jeu absent du dossier n'a aucun GIF.\n\n• MIXTE — case « Mode mixte » cochée dans la configuration web du DMD (page Affichage > GIFs du jeu, avec son « ? ») : un jeu qui a des GIFs dans le dossier ne lit QUE ceux-là (aucune recherche automatique pour lui) ; un jeu qui n'en a aucun dans le dossier retombe sur la recherche automatique dans /gifs.\n\nLe réglage « mixte » est dans le DMD (ou le Pi), pas dans la boîte à outils : cette page n'affiche que le contenu du dossier.",
        "news": " — nouveautés : {gifs} GIF(s), {games} jeu(x)", "news_first": " — aucun contrôle encore : tout est à examiner",
        "playing": "GIFs joués pour ce jeu :", "cands": "Candidats dans /gifs (nom approximatif, à vérifier) :", "cand_hint": "Aucun GIF sûr. Des candidats existent : sélectionnez-en un, vérifiez l'aperçu, puis copiez-le dans le dossier.",
        "copy_btn": "Copier", "remove_btn": "Retirer", "copied": "Copié : {sd}", "removed": "Retiré du dossier : {sd}", "copy_fail": "Copie impossible : {why}",
        "renamed2": "Renommé : {old} → {new}", "confirm_remove": "Retirer « {name} » du dossier ?\n(la copie seulement ; l'original de /gifs n'est pas touché)",
        "name_ok": "Ce nom correspond au titre « {name} ».", "name_bad": "Ce nom ne correspond pas au titre : il doit être identique au titre, à un numéro 01, 02 près.",
        "in_folder": "(dans le dossier)", "import_hint": "Le GIF est COPIÉ dans /gifs_jeux/<lettre>/ ; laissez le nom proposé (titre du jeu) ou saisissez-en un qui correspond au titre.",
        "sync_btn": "Ajout auto…", "sync_title": "Mettre à jour le dossier /gifs_jeux", "sync_wait": "Analyse de la bibliothèque (tous les systèmes)…",
        "sync_intro": "GIFs de /gifs qui correspondent SÛREMENT à un jeu de la bibliothèque et ne sont pas encore dans le dossier. Cliquez sur une ligne pour la cocher/décocher. Les originaux ne sont pas touchés.",
        "sync_c_sel": "", "sync_c_title": "Jeu", "sync_c_sys": "Système", "sync_c_old": "GIF d'origine", "sync_c_new": "Copie", "sync_c_flag": "Nouveau",
        "sync_flag_gif": "GIF", "sync_flag_game": "jeu", "sync_stats": "{n} copie(s) proposée(s), {sel} cochée(s), {mb} Mo ({gifs} nouveau(x) GIF(s), {games} nouveau(x) jeu(x)).",
        "sync_none": "Rien à copier : tous les GIFs qui correspondent sûrement à un jeu sont déjà dans le dossier.", "sync_space": "Espace libre sur la carte : {free} Mo.",
        "sync_confirm": "Copier {n} GIF(s) dans /gifs_jeux ?\nLes originaux ne sont pas modifiés ; l'index du dossier sera réécrit.", "sync_done": "{n} GIF(s) copié(s) dans le dossier.",
        "sync_failed": "{n} échec(s), par ex. {first}", "sync_all": "Tout cocher", "sync_nothing": "Tout décocher", "sync_apply": "Copier la sélection", "sync_seen": "Marquer comme vu", "sync_seen_done": "Nouveautés marquées comme vues.",
        "src_kind_sd": "Carte SD", "src_kind_pi": "Raspberry Pi (Wi-Fi)", "pi_ip": "IP du Raspberry Pi :", "no_pi": "Saisissez l'adresse IP du Raspberry Pi (ou cliquez sur Détecter).",
        "no_pi_found": "Aucun Raspberry Pi trouvé sur le réseau.", "pi_unreachable": "Raspberry Pi injoignable, ou démon trop ancien (il faut la version 3.87 ou plus) : {why}",
        "pi_too_old": "Le Raspberry Pi est en version {v} : cette page demande le démon 3.87 ou plus (canal ALPHA ; les versions 3.83 et antérieures n'ont pas les GIFs du jeu). Passez le Pi sur le canal alpha (page Réglages du DMD), puis réessayez.",
        "no_games_pi": "Aucune liste de jeux sur ce Raspberry Pi (instantané du Mode 1 absent) : lancez le Mode 1 vers le Pi, puis revenez ici.", "src_snap_pi": "Liste des jeux : dernier Mode 1 du Pi ({date})",
        "snap_btn": "Recréer l'instantané", "snap_empty": "Aucun gamelist.xml exploitable dans le dossier ROMs : instantané non créé.",
        "snap_need_roms": "L'instantané du Mode 1 est toujours absent sur cette cible. Pour le recréer SANS refaire de Mode 1, cliquez sur « Recréer l'instantané » et choisissez le dossier ROMs de la Recalbox.",
        "snap_auto_info": "L'instantané du Mode 1 est absent sur cette cible. Il peut être recréé sans refaire de Mode 1, à partir des gamelist.xml du dossier ROMs de la Recalbox : choisissez (ou confirmez) ce dossier dans la fenêtre suivante, puis il sera envoyé / écrit sur la cible.",
        "snap_pick_roms": "Dossier ROMs de la Recalbox (contient les dossiers de systèmes avec leur gamelist.xml)", "snap_no_gamelist": "Aucun gamelist.xml trouvé dans :\n{path}\n\nChoisissez le dossier ROMs de la Recalbox (par ex. \\\\RECALBOX\\share\\roms), celui qui contient les dossiers de systèmes.",
        "snap_done_pi": "Instantané recréé et envoyé au Raspberry Pi : {n} système(s), {g} jeu(x).", "snap_done_sd": "Instantané recréé et écrit sur la carte SD : {n} système(s), {g} jeu(x).",
        "snap_pi_old": "Le démon de ce Raspberry Pi est trop ancien pour recevoir l'instantané en Wi-Fi (mise à jour du démon requise). Solution : retirez la carte, branchez-la au PC, choisissez « Carte SD » et recréez l'instantané.",
        "excl_pi_note": "Raspberry Pi : le fichier est enregistré directement sur le Pi, il n'y a rien à envoyer.",
        "err": "Impossible : {why}", "import_title": "Importer un GIF", "import_dest": "Dossier de destination dans /gifs :", "import_name": "Nom du fichier :", "import_pick": "Choisir le GIF…",
        "import_done": "Importé : {sd}", "pick_gif": "GIF à importer", "rank_lbl": "détection", "preview_none": "(aucun aperçu)", "frames": "{n} image(s)", "sd_gifs0": "Aucun GIF trouvé dans « gifs » de cette carte.",
        "mkdir_btn": "Créer /gifs_jeux", "mkdir_done": "Dossier /gifs_jeux créé. Il est vide : le DMD reste en mode auto tant qu'il ne contient aucun GIF.", "mkdir_exists": "Le dossier /gifs_jeux existe déjà.",
        "check_all": "Tous", "check_none": "Aucun",
        "tip_excl_btn_list": "Liste, retire ou envoie au DMD les règles d'exclusion.", "tip_copy_btn": "Copie les GIFs cochés de /gifs dans /gifs_jeux, sous le nom du jeu. Les originaux ne sont pas touchés.", "tip_remove_btn": "Supprime de /gifs_jeux les GIFs cochés (les originaux de /gifs ne sont pas touchés).", "tip_rename": "Renomme le GIF coché de /gifs_jeux ; le nom doit correspondre au titre du jeu. Un seul GIF coché.", "tip_excl_btn_gif": "Le DMD ne jouera plus les GIFs cochés pour le jeu sélectionné.", "tip_check_all": "Coche tous les GIFs de la liste.", "tip_check_none": "Décoche tous les GIFs de la liste.", "tip_import": "Importe un GIF de votre PC pour le jeu sélectionné : choix de l'image, conversion au format du DMD si besoin, puis copie dans /gifs_jeux (créé au besoin).", "tip_sync_btn": "Analyse tous les systèmes et prépare la copie dans /gifs_jeux de tous les GIFs dont le nom correspond SÛREMENT à un jeu (règle stricte). Une liste de contrôle est montrée avant toute copie.", "refreshed": "Liste actualisée : rien de nouveau depuis la dernière fois.", "bright": "Luminosité",
        "zone_title": "GIFs de ce jeu (cochez les fichiers visés par les boutons du dessous)",
        "conv_native": "GIF déjà au format du DMD (128×32) : copié tel quel.", "conv_done": "Converti au format du DMD (128×32) : {n} image(s), {s} s.", "conv_resampled": " (1 image sur {k}, durée conservée)", "conv_fail": "Conversion impossible : {why}", "add_title": "Ajouter un GIF — {name}",
        "excl_btn_gif": "Exclure", "excl_btn_all": "Aucun GIF", "excl_btn_list": "Exclusions…", "excl_title": "Exclusions de GIFs", "excl_col_title": "Jeu (titre)", "excl_col_gif": "GIF exclu",
        "excl_all_label": "(aucun GIF)", "excl_glob_title": "(tous les jeux)", "excl_remove": "Retirer", "excl_send": "Envoyer au DMD (Wi-Fi)", "excl_sent": "Exclusions envoyées au DMD {ip}.", "excl_send_fail": "Envoi impossible : {detail}", "excl_no_dmd": "Aucun DMD trouvé sur le réseau.",
        "excl_none": "Aucune exclusion.", "excl_added": "Exclusion enregistrée : {what}", "excl_info": "Exclusions pour ce titre : {list}", "excl_file": "Fichier : playlists/game_gifs_exclude.txt sur la carte SD (créé par le toolkit, lu par le DMD à chaque recherche).",
        "close": "Fermer", "confirm_rename": "Renommer « {old} » en « {new} » ?\nLes playlists et le cache maître qui le citent seront mis à jour.",
    },
    "en": {
        "title": "Game GIFs — title matching", "sd": "DMD SD card:", "browse": "Browse…", "auto": "Detect", "load": "Load",
        "system": "System:", "sys_all": "All", "filter": "Filter:", "view": "Show:", "v_all": "All games", "v_none": "No GIF", "v_has": "With GIF", "v_dir": "Copied into /gifs_jeux", "v_weak": "Approximate match (to check)",
        "col_title": "Game", "col_gifs": "GIFs", "col_dir": "Folder", "sec_dir": "In the /gifs_jeux folder (read by the DMD in folder mode):", "sec_sd": "On the SD card, in /gifs (sure match):", "sec_pi": "On the Raspberry Pi, in /gifs (sure match):", "no_sd": "Choose the DMD SD card (the folder that contains “gifs”).", "no_games": "No game list on this SD card (no Mode 1 snapshot): run Mode 1 on this card, then come back.", "src_snap": "Game list: last Mode 1 of this card ({date})", "src_sd": "No Mode 1 snapshot: list = games that have a visual on the SD (titles = file names)",
        "clear_btn": "Delete all", "tip_clear_btn": "Deletes ALL the GIFs of the /gifs_jeux folder (after confirmation): the DMD goes back to auto mode. The originals in /gifs are not touched.", "clear_confirm": "Delete the {n} GIF(s) of the /gifs_jeux folder?\n\nThe folder will be emptied and the DMD will go back to auto mode. The originals in /gifs are not touched.", "clear_done": "Folder emptied: {n} GIF(s) deleted. Back to auto mode.", "clear_empty": "The /gifs_jeux folder is already empty (or missing): nothing to delete.",
        "loading": "Reading games and matching…", "stats": "{n} games · {has} with GIF · {none} without · {gifs} GIFs on the SD", "shown": " (shown: {s})",
        "sel_none": "Select a game.", "sel_title": "Game: {name}", "none_found": "No GIF matches this title.",
        "sure": "reliable match (name identical to the title, ignoring the system prefix and the number)", "rank3": "name identical to the title", "rank2": "the GIF name is contained in the title (approximate: it may also serve another game)", "rank1": "the title is contained in the GIF name (uncertain)",
        "rename": "Rename…", "import": "Import a GIF…", "rename_title": "Rename the GIF", "rename_ask": "New name (.gif):",
        "rename_ok_match": "After renaming, this GIF will match the title “{name}” EXACTLY.", "rename_weak": "After renaming, this GIF will not match this game.",
        "rename_suggest": "Use the title", "ok": "OK", "cancel": "Cancel", "renamed": "Renamed: {old} → {new}\nPlaylists updated: {pl} · master cache: {ma} line(s).",
        "mode_folder": "Dedicated /gifs_jeux folder present: the DMD reads ONLY it ({n} GIF(s)).", "mode_auto": "No /gifs_jeux folder: auto mode (strict matches on /gifs).",
        "help_btn": "? Help", "help_text": "Three playback modes on the DMD, depending on what the /gifs_jeux folder contains:\n\n• AUTO — no folder, or a folder with no GIF: the DMD looks in /gifs for GIFs whose name is exactly the game title.\n\n• EXCLUSIVE — the folder holds at least one GIF (\"mixed mode\" setting unticked): the DMD reads ONLY the folder; a game missing from it gets no GIF.\n\n• MIXED — \"Mixed mode\" ticked in the DMD web configuration (Display page > Game GIFs, with its \"?\"): a game that has GIFs in the folder plays ONLY those (no automatic search for it); a game with none in the folder falls back to the automatic search in /gifs.\n\nThe \"mixed\" setting lives in the DMD (or the Pi), not in the toolkit: this page only shows the folder contents.",
        "news": " — new: {gifs} GIF(s), {games} game(s)", "news_first": " — nothing checked yet: everything is to review",
        "playing": "GIFs played for this game:", "cands": "Candidates in /gifs (approximate name, to check):", "cand_hint": "No sure GIF. Candidates exist: pick one, check the preview, then copy it into the folder.",
        "copy_btn": "Copy", "remove_btn": "Remove", "copied": "Copied: {sd}", "removed": "Removed from the folder: {sd}", "copy_fail": "Copy failed: {why}",
        "renamed2": "Renamed: {old} → {new}", "confirm_remove": "Remove “{name}” from the folder?\n(the copy only; the original in /gifs is not touched)",
        "name_ok": "This name matches the title “{name}”.", "name_bad": "This name does not match the title: it must be identical to the title, apart from a 01, 02 number.",
        "in_folder": "(in the folder)", "import_hint": "The GIF is COPIED into /gifs_jeux/<letter>/; keep the proposed name (the game title) or type one that matches the title.",
        "sync_btn": "Auto add…", "sync_title": "Update the /gifs_jeux folder", "sync_wait": "Analysing the library (all systems)…",
        "sync_intro": "GIFs in /gifs that SURELY match a game of the library and are not in the folder yet. Click a row to tick/untick it. Originals are not touched.",
        "sync_c_sel": "", "sync_c_title": "Game", "sync_c_sys": "System", "sync_c_old": "Original GIF", "sync_c_new": "Copy", "sync_c_flag": "New",
        "sync_flag_gif": "GIF", "sync_flag_game": "game", "sync_stats": "{n} copy(ies) proposed, {sel} ticked, {mb} MB ({gifs} new GIF(s), {games} new game(s)).",
        "sync_none": "Nothing to copy: every GIF that surely matches a game is already in the folder.", "sync_space": "Free space on the card: {free} MB.",
        "sync_confirm": "Copy {n} GIF(s) into /gifs_jeux?\nOriginals are not modified; the folder index will be rewritten.", "sync_done": "{n} GIF(s) copied into the folder.",
        "sync_failed": "{n} failure(s), e.g. {first}", "sync_all": "Tick all", "sync_nothing": "Untick all", "sync_apply": "Copy selection", "sync_seen": "Mark as seen", "sync_seen_done": "News marked as seen.",
        "src_kind_sd": "SD card", "src_kind_pi": "Raspberry Pi (Wi-Fi)", "pi_ip": "Raspberry Pi IP:", "no_pi": "Enter the Raspberry Pi's IP address (or click Detect).",
        "no_pi_found": "No Raspberry Pi found on the network.", "pi_unreachable": "Raspberry Pi unreachable, or daemon too old (version 3.87 or later is required): {why}",
        "pi_too_old": "The Raspberry Pi runs version {v}: this page needs daemon 3.87 or later (ALPHA channel; versions 3.83 and earlier do not have the game GIFs). Switch the Pi to the alpha channel (DMD Settings page), then try again.",
        "no_games_pi": "No game list on this Raspberry Pi (Mode 1 snapshot missing): run Mode 1 towards the Pi, then come back.", "src_snap_pi": "Game list: last Mode 1 of the Pi ({date})",
        "snap_btn": "Rebuild snapshot", "snap_empty": "No usable gamelist.xml in the ROMs folder: snapshot not created.",
        "snap_need_roms": "The Mode 1 snapshot is still missing on this target. To rebuild it WITHOUT redoing Mode 1, click \"Rebuild snapshot\" and choose the Recalbox ROMs folder.",
        "snap_auto_info": "The Mode 1 snapshot is missing on this target. It can be rebuilt without redoing Mode 1, from the gamelist.xml files of the Recalbox ROMs folder: choose (or confirm) that folder in the next window, then it will be sent / written to the target.",
        "snap_pick_roms": "Recalbox ROMs folder (contains the system folders with their gamelist.xml)", "snap_no_gamelist": "No gamelist.xml found in:\n{path}\n\nChoose the Recalbox ROMs folder (e.g. \\\\RECALBOX\\share\\roms), the one that contains the system folders.",
        "snap_done_pi": "Snapshot rebuilt and sent to the Raspberry Pi: {n} system(s), {g} game(s).", "snap_done_sd": "Snapshot rebuilt and written to the SD card: {n} system(s), {g} game(s).",
        "snap_pi_old": "This Raspberry Pi's daemon is too old to receive the snapshot over Wi-Fi (daemon update required). Workaround: remove the card, plug it into the PC, choose \"SD card\" and rebuild the snapshot.",
        "excl_pi_note": "Raspberry Pi: the file is saved directly on the Pi, nothing to send.",
        "err": "Not possible: {why}", "import_title": "Import a GIF", "import_dest": "Destination folder in /gifs:", "import_name": "File name:", "import_pick": "Choose the GIF…",
        "import_done": "Imported: {sd}", "pick_gif": "GIF to import", "rank_lbl": "match", "preview_none": "(no preview)", "frames": "{n} frame(s)", "sd_gifs0": "No GIF found in this card's “gifs” folder.",
        "mkdir_btn": "Create /gifs_jeux", "mkdir_done": "/gifs_jeux folder created. It is empty: the DMD stays in auto mode until it contains a GIF.", "mkdir_exists": "The /gifs_jeux folder already exists.",
        "check_all": "All", "check_none": "None",
        "tip_excl_btn_list": "Lists, removes or sends the exclusion rules to the DMD.", "tip_copy_btn": "Copies the ticked GIFs from /gifs into /gifs_jeux, named after the game. Originals are not touched.", "tip_remove_btn": "Deletes the ticked GIFs from /gifs_jeux (the originals in /gifs are not touched).", "tip_rename": "Renames the ticked GIF of /gifs_jeux; the name must match the game title. One ticked GIF only.", "tip_excl_btn_gif": "The DMD will no longer play the ticked GIFs for the selected game.", "tip_check_all": "Ticks every GIF in the list.", "tip_check_none": "Unticks every GIF in the list.", "tip_import": "Imports a GIF from your PC for the selected game: pick the image, converted to the DMD format if needed, then copied into /gifs_jeux (created if needed).", "tip_sync_btn": "Scans every system and prepares copying into /gifs_jeux every GIF whose name SURELY matches a game (strict rule). A review list is shown before anything is copied.", "refreshed": "List refreshed: nothing new since last time.", "bright": "Brightness",
        "zone_title": "GIFs for this game (tick the files the buttons below act on)",
        "conv_native": "GIF already in the DMD format (128×32): copied as is.", "conv_done": "Converted to the DMD format (128×32): {n} frame(s), {s} s.", "conv_resampled": " (1 frame out of {k}, duration kept)", "conv_fail": "Conversion impossible: {why}", "add_title": "Add a GIF — {name}",
        "excl_btn_gif": "Exclude", "excl_btn_all": "No GIF", "excl_btn_list": "Exclusions…", "excl_title": "GIF exclusions", "excl_col_title": "Game (title)", "excl_col_gif": "Excluded GIF",
        "excl_all_label": "(no GIF)", "excl_glob_title": "(all games)", "excl_remove": "Remove", "excl_send": "Send to the DMD (Wi-Fi)", "excl_sent": "Exclusions sent to the DMD {ip}.", "excl_send_fail": "Could not send: {detail}", "excl_no_dmd": "No DMD found on the network.",
        "excl_none": "No exclusions.", "excl_added": "Exclusion saved: {what}", "excl_info": "Exclusions for this title: {list}", "excl_file": "File: playlists/game_gifs_exclude.txt on the SD card (created by the toolkit, read by the DMD at every search).",
        "close": "Close", "confirm_rename": "Rename “{old}” to “{new}”?\nPlaylists and the master cache that mention it will be updated.",
    },
    "es": {
        "title": "GIFs del juego — detección por título", "sd": "Tarjeta SD del DMD:", "browse": "Examinar…", "auto": "Detectar", "load": "Cargar",
        "system": "Sistema:", "sys_all": "Todos", "filter": "Filtrar:", "view": "Mostrar:", "v_all": "Todos los juegos", "v_none": "Sin GIF", "v_has": "Con GIF", "v_dir": "Copiado en /gifs_jeux", "v_weak": "Detección aproximada (a verificar)",
        "col_title": "Juego", "col_gifs": "GIFs", "col_dir": "Carpeta", "sec_dir": "En la carpeta /gifs_jeux (leída por el DMD en modo carpeta):", "sec_sd": "En la tarjeta SD, en /gifs (coincidencia segura):", "sec_pi": "En la Raspberry Pi, en /gifs (coincidencia segura):", "no_sd": "Elija la tarjeta SD del DMD (la carpeta que contiene «gifs»).", "no_games": "No hay lista de juegos en esta tarjeta SD (sin instantánea del Modo 1): ejecute el Modo 1 en esta tarjeta y vuelva aquí.", "src_snap": "Lista de juegos: último Modo 1 de esta tarjeta ({date})", "src_sd": "Sin instantánea del Modo 1: lista = juegos con imagen en la SD (títulos = nombres de archivo)",
        "clear_btn": "Eliminar todo", "tip_clear_btn": "Elimina TODOS los GIFs de la carpeta /gifs_jeux (tras confirmación): el DMD vuelve al modo auto. Los originales de /gifs no se tocan.", "clear_confirm": "¿Eliminar los {n} GIF(s) de la carpeta /gifs_jeux?\n\nLa carpeta se vaciará y el DMD volverá al modo auto. Los originales de /gifs no se tocan.", "clear_done": "Carpeta vaciada: {n} GIF(s) eliminado(s). Vuelta al modo auto.", "clear_empty": "La carpeta /gifs_jeux ya está vacía (o no existe): nada que eliminar.",
        "loading": "Leyendo juegos y detectando…", "stats": "{n} juegos · {has} con GIF · {none} sin GIF · {gifs} GIFs en la SD", "shown": " (mostrados: {s})",
        "sel_none": "Seleccione un juego.", "sel_title": "Juego: {name}", "none_found": "Ningún GIF coincide con este título.",
        "sure": "coincidencia segura (nombre idéntico al título, salvo el prefijo del sistema y el número)", "rank3": "nombre idéntico al título", "rank2": "el nombre del GIF está contenido en el título (aproximado: también puede servir a otro juego)", "rank1": "el título está contenido en el nombre del GIF (incierto)",
        "rename": "Renombrar…", "import": "Importar un GIF…", "rename_title": "Renombrar el GIF", "rename_ask": "Nuevo nombre (.gif):",
        "rename_ok_match": "Tras renombrar, este GIF coincidirá EXACTAMENTE con el título «{name}».", "rename_weak": "Tras renombrar, este GIF no coincidirá con este juego.",
        "rename_suggest": "Usar el título", "ok": "Aceptar", "cancel": "Cancelar", "renamed": "Renombrado: {old} → {new}\nListas actualizadas: {pl} · caché maestra: {ma} línea(s).",
        "mode_folder": "Carpeta dedicada /gifs_jeux presente: el DMD lee SOLO ella ({n} GIF).", "mode_auto": "Sin carpeta /gifs_jeux: modo auto (coincidencias estrictas en /gifs).",
        "help_btn": "? Ayuda", "help_text": "Tres modos de reproducción en el DMD, según el contenido de la carpeta /gifs_jeux:\n\n• AUTO — sin carpeta, o carpeta sin GIF: el DMD busca en /gifs los GIFs cuyo nombre es exactamente el título del juego.\n\n• EXCLUSIVO — la carpeta tiene al menos un GIF (ajuste « modo mixto » desmarcado): el DMD lee SOLO la carpeta; un juego ausente de ella no tiene GIF.\n\n• MIXTO — casilla « Modo mixto » marcada en la configuración web del DMD (página Pantalla > GIFs del juego, con su « ? »): un juego que tiene GIFs en la carpeta reproduce SOLO esos (sin búsqueda automática); un juego sin ninguno en la carpeta vuelve a la búsqueda automática en /gifs.\n\nEl ajuste « mixto » está en el DMD (o la Pi), no en la caja de herramientas: esta página solo muestra el contenido de la carpeta.",
        "news": " — novedades: {gifs} GIF, {games} juego(s)", "news_first": " — aún sin revisar: todo está por examinar",
        "playing": "GIF reproducidos para este juego:", "cands": "Candidatos en /gifs (nombre aproximado, a verificar):", "cand_hint": "Ningún GIF seguro. Hay candidatos: elija uno, compruebe la vista previa y cópielo a la carpeta.",
        "copy_btn": "Copiar", "remove_btn": "Quitar", "copied": "Copiado: {sd}", "removed": "Quitado de la carpeta: {sd}", "copy_fail": "No se pudo copiar: {why}",
        "renamed2": "Renombrado: {old} → {new}", "confirm_remove": "¿Quitar «{name}» de la carpeta?\n(solo la copia; el original de /gifs no se toca)",
        "name_ok": "Este nombre coincide con el título «{name}».", "name_bad": "Este nombre no coincide con el título: debe ser idéntico al título, salvo un número 01, 02.",
        "in_folder": "(en la carpeta)", "import_hint": "El GIF se COPIA en /gifs_jeux/<letra>/; deje el nombre propuesto (título del juego) o escriba uno que coincida con el título.",
        "sync_btn": "Añadir auto…", "sync_title": "Actualizar la carpeta /gifs_jeux", "sync_wait": "Analizando la biblioteca (todos los sistemas)…",
        "sync_intro": "GIF de /gifs que coinciden CON SEGURIDAD con un juego de la biblioteca y aún no están en la carpeta. Haga clic en una fila para marcarla/desmarcarla. Los originales no se tocan.",
        "sync_c_sel": "", "sync_c_title": "Juego", "sync_c_sys": "Sistema", "sync_c_old": "GIF original", "sync_c_new": "Copia", "sync_c_flag": "Nuevo",
        "sync_flag_gif": "GIF", "sync_flag_game": "juego", "sync_stats": "{n} copia(s) propuesta(s), {sel} marcada(s), {mb} MB ({gifs} GIF nuevo(s), {games} juego(s) nuevo(s)).",
        "sync_none": "Nada que copiar: todos los GIF que coinciden con seguridad con un juego ya están en la carpeta.", "sync_space": "Espacio libre en la tarjeta: {free} MB.",
        "sync_confirm": "¿Copiar {n} GIF en /gifs_jeux?\nLos originales no se modifican; se reescribirá el índice de la carpeta.", "sync_done": "{n} GIF copiado(s) en la carpeta.",
        "sync_failed": "{n} fallo(s), p. ej. {first}", "sync_all": "Marcar todo", "sync_nothing": "Desmarcar todo", "sync_apply": "Copiar la selección", "sync_seen": "Marcar como visto", "sync_seen_done": "Novedades marcadas como vistas.",
        "src_kind_sd": "Tarjeta SD", "src_kind_pi": "Raspberry Pi (Wi-Fi)", "pi_ip": "IP de la Raspberry Pi:", "no_pi": "Introduzca la dirección IP de la Raspberry Pi (o pulse Detectar).",
        "no_pi_found": "No se encontró ninguna Raspberry Pi en la red.", "pi_unreachable": "Raspberry Pi inaccesible, o demonio demasiado antiguo (hace falta la versión 3.87 o superior): {why}",
        "pi_too_old": "La Raspberry Pi está en la versión {v}: esta página necesita el demonio 3.87 o superior (canal ALPHA; las versiones 3.83 y anteriores no tienen los GIFs del juego). Pase la Pi al canal alpha (página Ajustes del DMD) y vuelva a intentarlo.",
        "no_games_pi": "Ninguna lista de juegos en esta Raspberry Pi (falta la instantánea del Modo 1): ejecute el Modo 1 hacia la Pi y vuelva.", "src_snap_pi": "Lista de juegos: último Modo 1 de la Pi ({date})",
        "snap_btn": "Recrear la instantánea", "snap_empty": "Ningún gamelist.xml utilizable en la carpeta ROMs: instantánea no creada.",
        "snap_need_roms": "Sigue faltando la instantánea del Modo 1 en este destino. Para recrearla SIN repetir el Modo 1, pulse «Recrear la instantánea» y elija la carpeta ROMs de Recalbox.",
        "snap_auto_info": "Falta la instantánea del Modo 1 en este destino. Se puede recrear sin repetir el Modo 1, a partir de los gamelist.xml de la carpeta ROMs de Recalbox: elija (o confirme) esa carpeta en la ventana siguiente y se enviará / escribirá en el destino.",
        "snap_pick_roms": "Carpeta ROMs de Recalbox (contiene las carpetas de sistemas con su gamelist.xml)", "snap_no_gamelist": "No se encontró ningún gamelist.xml en:\n{path}\n\nElija la carpeta ROMs de Recalbox (p. ej. \\\\RECALBOX\\share\\roms), la que contiene las carpetas de sistemas.",
        "snap_done_pi": "Instantánea recreada y enviada a la Raspberry Pi: {n} sistema(s), {g} juego(s).", "snap_done_sd": "Instantánea recreada y escrita en la tarjeta SD: {n} sistema(s), {g} juego(s).",
        "snap_pi_old": "El demonio de esta Raspberry Pi es demasiado antiguo para recibir la instantánea por Wi-Fi (se requiere actualizarlo). Solución: retire la tarjeta, conéctela al PC, elija «Tarjeta SD» y recree la instantánea.",
        "excl_pi_note": "Raspberry Pi: el archivo se guarda directamente en la Pi, no hay nada que enviar.",
        "err": "No es posible: {why}", "import_title": "Importar un GIF", "import_dest": "Carpeta de destino en /gifs:", "import_name": "Nombre del archivo:", "import_pick": "Elegir el GIF…",
        "import_done": "Importado: {sd}", "pick_gif": "GIF a importar", "rank_lbl": "detección", "preview_none": "(sin vista previa)", "frames": "{n} imagen(es)", "sd_gifs0": "Ningún GIF en la carpeta «gifs» de esta tarjeta.",
        "mkdir_btn": "Crear /gifs_jeux", "mkdir_done": "Carpeta /gifs_jeux creada. Está vacía: el DMD sigue en modo auto mientras no contenga ningún GIF.", "mkdir_exists": "La carpeta /gifs_jeux ya existe.",
        "check_all": "Todos", "check_none": "Ninguno",
        "tip_excl_btn_list": "Lista, quita o envía al DMD las reglas de exclusión.", "tip_copy_btn": "Copia los GIFs marcados de /gifs a /gifs_jeux, con el nombre del juego. Los originales no se tocan.", "tip_remove_btn": "Elimina de /gifs_jeux los GIFs marcados (los originales de /gifs no se tocan).", "tip_rename": "Renombra el GIF marcado de /gifs_jeux; el nombre debe coincidir con el título del juego. Un solo GIF marcado.", "tip_excl_btn_gif": "El DMD ya no reproducirá los GIFs marcados para el juego seleccionado.", "tip_check_all": "Marca todos los GIFs de la lista.", "tip_check_none": "Desmarca todos los GIFs de la lista.", "tip_import": "Importa un GIF de su PC para el juego seleccionado: elección de la imagen, conversión al formato del DMD si hace falta y copia en /gifs_jeux (se crea si hace falta).", "tip_sync_btn": "Analiza todos los sistemas y prepara la copia en /gifs_jeux de todos los GIFs cuyo nombre coincide SEGURO con un juego (regla estricta). Se muestra una lista de control antes de copiar.", "refreshed": "Lista actualizada: nada nuevo desde la última vez.", "bright": "Brillo",
        "zone_title": "GIFs de este juego (marque los archivos a los que se aplican los botones de abajo)",
        "conv_native": "GIF ya en el formato del DMD (128×32): copiado tal cual.", "conv_done": "Convertido al formato del DMD (128×32): {n} imagen(es), {s} s.", "conv_resampled": " (1 imagen de cada {k}, duración conservada)", "conv_fail": "Conversión imposible: {why}", "add_title": "Añadir un GIF — {name}",
        "excl_btn_gif": "Excluir", "excl_btn_all": "Ningún GIF", "excl_btn_list": "Exclusiones…", "excl_title": "Exclusiones de GIFs", "excl_col_title": "Juego (título)", "excl_col_gif": "GIF excluido",
        "excl_all_label": "(ningún GIF)", "excl_glob_title": "(todos los juegos)", "excl_remove": "Quitar", "excl_send": "Enviar al DMD (Wi-Fi)", "excl_sent": "Exclusiones enviadas al DMD {ip}.", "excl_send_fail": "No se pudo enviar: {detail}", "excl_no_dmd": "No se encontró ningún DMD en la red.",
        "excl_none": "Sin exclusiones.", "excl_added": "Exclusión guardada: {what}", "excl_info": "Exclusiones para este título: {list}", "excl_file": "Archivo: playlists/game_gifs_exclude.txt en la tarjeta SD (creado por el toolkit, leído por el DMD en cada búsqueda).",
        "close": "Cerrar", "confirm_rename": "¿Renombrar «{old}» a «{new}»?\nSe actualizarán las listas y la caché maestra que lo citan.",
    },
}


def _pref_get(key, default=""):
    try:
        import RecalBoxDMD_prefs as p
        return p.get(key) or default
    except Exception:   # noqa: BLE001
        return default


def _pref_set(key, value):
    try:
        import RecalBoxDMD_prefs as p
        p.set(key, value)
    except Exception:   # noqa: BLE001
        pass


def load_frames(path, max_frames=MAX_PREVIEW_FRAMES):
    """[(image PIL RGB, delai ms)] d'un GIF (les `max_frames` premieres images) ; [] si illisible."""
    from PIL import Image, ImageSequence
    out = []
    try:
        with Image.open(str(path)) as im:
            for i, fr in enumerate(ImageSequence.Iterator(im)):
                if i >= max_frames:
                    break
                out.append((fr.convert("RGB").copy(), max(20, int(fr.info.get("duration", 100) or 100))))
    except Exception:   # noqa: BLE001
        return []
    return out


class _Tip:
    """Aide au survol : texte calcule au moment de l'affichage (suit la langue)."""

    def __init__(self, widget, text_fn):
        self.w, self.fn, self.top, self.job = widget, text_fn, None, None
        widget.bind("<Enter>", self._enter, add="+")
        widget.bind("<Leave>", self._hide, add="+")
        widget.bind("<ButtonPress>", self._hide, add="+")

    def _enter(self, _e):
        self._hide()
        self.job = self.w.after(450, self._show)

    def _show(self):
        self.job = None
        try:
            txt = self.fn()
            if not txt:
                return
            self.top = tk.Toplevel(self.w)
            self.top.wm_overrideredirect(True)
            self.top.wm_geometry(f"+{self.w.winfo_rootx() + 8}+{self.w.winfo_rooty() + self.w.winfo_height() + 4}")
            tk.Label(self.top, text=txt, bg="#FFFFE1", fg="black", relief="solid", bd=1, justify="left", wraplength=380, padx=6, pady=3).pack()
        except tk.TclError:
            self.top = None

    def _hide(self, _e=None):
        if self.job:
            try:
                self.w.after_cancel(self.job)
            except tk.TclError:
                pass
            self.job = None
        if self.top is not None:
            try:
                self.top.destroy()
            except tk.TclError:
                pass
            self.top = None


class _Ticker:
    """Ligne de message (confirmations d'action) sur la ligne de mode, en haut : taille fixe, le texte trop long DEFILE (pause, defilement jusqu'a la fin, pause, retour)."""

    def __init__(self, parent, bg, fg, font):
        self.cv = tk.Canvas(parent, height=18, bg=bg, highlightthickness=0)
        self.item = self.cv.create_text(2, 9, text="", anchor="w", fill=fg, font=font)
        self.text, self.job = "", None
        self.cv.bind("<Configure>", lambda e: self._restart())

    def configure(self, text=None, **kw):
        if "fg" in kw:
            self.cv.itemconfigure(self.item, fill=kw["fg"])
        if text is not None:
            self.text = text
            self.cv.itemconfigure(self.item, text=text)
            self._restart()

    config = configure

    def cget(self, key):
        return self.text if key == "text" else self.cv.cget(key)

    def _cancel(self):
        if self.job is not None:
            try:
                self.cv.after_cancel(self.job)
            except tk.TclError:
                pass
            self.job = None

    def _restart(self):
        self._cancel()
        try:
            self.cv.coords(self.item, 2, 9)
            box = self.cv.bbox(self.item)
            if not box:
                return
            width, view = box[2] - box[0], self.cv.winfo_width()
            if view > 1 and width + 6 > view:
                self.job = self.cv.after(1500, lambda: self._step(2, view - width - 6))
        except tk.TclError:
            pass

    def _step(self, x, end):
        """Fait defiler vers la gauche jusqu'a x = end (fin du texte visible), puis pause et retour au debut."""
        try:
            if x > end:
                x -= 2
                self.cv.coords(self.item, x, 9)
                self.job = self.cv.after(30, lambda: self._step(x, end))
            else:
                self.job = self.cv.after(1500, self._restart)
        except tk.TclError:
            pass


class GameGifsWindow:
    def __init__(self, app, parent=None):
        self.app = app
        self.embedded = parent is not None   # page de l'onglet Personnalisation (sinon fenetre separee)
        self.root = app.root
        self.sd_root = None
        self.snap = cl.new_snapshot()   # instantane du dernier Mode 1 de la carte (custom/games.json)
        self.system = ""
        self.games = []             # jeux du systeme courant
        self.gifs = []              # GIFs de la SD
        self.active = []            # GIFs que le DMD lit reellement (dossier dedie s'il existe, sinon /gifs)
        self.det = {}               # {indice de jeu: (3, [gif])} : regle STRICTE sur /gifs de la carte SD ou du Pi (c'est ce que la liste des jeux affiche, jamais le dossier)
        self.det_dir = {}           # idem sur le dossier dedie /gifs_jeux (mode dossier seulement) : affiche dans la zone des GIFs du jeu, pre-coche
        self.cand = {}              # {indice de jeu: (rang, [gif])} : candidats approximatifs de /gifs (regle large) a verifier
        self._cands = []
        self.excl_text = ""         # contenu de playlists/game_gifs_exclude.txt (applique a la detection, comme le DMD)
        self.shown = []             # indices affiches (ordre de la liste)
        self.sel = None
        self._folder_mode = False
        self.autoload = True        # choisir Carte SD / Pi charge la liste tout seul
        self._token = 0
        self._anim_after = None
        self._anim_frames = []
        self._anim_i = 0
        self._photo = None
        c = {}
        try:
            c = app._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        self.bg = c.get("bg_main", "#F3F3F3")
        self.fg = c.get("fg_text", "#000000")
        if self.embedded:
            self.win = tk.Frame(parent, bg=self.bg)
            self.win.pack(fill="both", expand=True)
            self._build()
        else:
            self.win = tk.Toplevel(self.root)
            self.win.configure(bg=self.bg)
            self.win.geometry("1180x700")
            self.win.minsize(900, 520)
            self._build()
            self.win.title(self.t("title"))
            self.win.protocol("WM_DELETE_WINDOW", self._close)
        sd = ""
        try:
            sd = app._find_dmd_sd_root() or ""
        except Exception:   # noqa: BLE001
            pass
        self.sd_var.set(sd or _pref_get("game_gifs_sd"))
        if self.sd_var.get():
            self.win.after(200, self._reload)

    # ------------------------------------------------------------------ theme
    _SEMANTIC_FG = {"#1B7F3B", "#B26A00", "#C62828", "#333333", "#AAAAAA"}      # couleurs de texte voulues (vert / orange / rouge...) : jamais recolorees

    def _colors(self) -> dict:
        """Couleurs COURANTES du theme (boutons compris) : lues a chaque creation de bouton et a chaque apply_theme, jamais figees."""
        c = {}
        try:
            c = self.app._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        return {"bg_button_normal": c.get("bg_button_normal", "#FFFFFF"), "fg_text": c.get("fg_text", self.fg), "bg_main": c.get("bg_main", self.bg)}

    def apply_theme(self):
        """Le theme est applique APRES la construction de la page (page construite au premier affichage) et peut changer ensuite : les cadres dont la couleur n'est plus celle
        du theme (le theme garde un cadre « deja personnalise ») ne suivaient pas. On relit les couleurs COURANTES et on recolore cadres, libelles, boutons radio, curseur et
        bandeau de message ; les zones blanches voulues (_no_bg_slice : liste des GIFs) et les textes de couleur sont conserves."""
        c = {}
        try:
            c = self.app._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        self.bg = c.get("bg_main", self.bg)
        self.fg = c.get("fg_text", self.fg)

        def walk(w):
            for ch in w.winfo_children():
                if getattr(ch, "_no_bg_slice", False):
                    continue
                try:
                    cls = ch.winfo_class()
                    if cls in ("Frame", "Canvas") and ch is not getattr(self, "canvas", None):
                        ch.configure(bg=self.bg)
                    elif cls == "Label":
                        opts = {"bg": self.bg}
                        if str(ch.cget("fg")).upper() not in self._SEMANTIC_FG:
                            opts["fg"] = self.fg
                        ch.configure(**opts)
                    elif cls == "Button":
                        bc = self._colors()
                        ch.configure(bg=bc["bg_button_normal"], fg=bc["fg_text"], activebackground=bc["bg_button_normal"], activeforeground=bc["fg_text"])
                    elif cls in ("Radiobutton", "Checkbutton"):
                        ch.configure(bg=self.bg, fg=self.fg, activebackground=self.bg, selectcolor=self.bg)
                    elif cls == "Scale":
                        ch.configure(bg=self.bg, fg=self.fg)
                except tk.TclError:
                    pass
                walk(ch)
        try:
            self.win.configure(bg=self.bg)
        except tk.TclError:
            pass
        walk(self.win)

    # ------------------------------------------------------------------ textes
    def t(self, key, /, **kw):
        try:
            lang = self.app.lang_var.get()
        except Exception:   # noqa: BLE001
            lang = "fr"
        s = T.get(lang, T["fr"]).get(key) or T["fr"][key]
        return s.format(**kw) if kw else s

    # ------------------------------------------------------------------ construction
    def _btn(self, parent, key, command, **kw):
        c = self._colors()
        opts = {"bg": c["bg_button_normal"], "fg": c["fg_text"], "activebackground": c["bg_button_normal"], "activeforeground": c["fg_text"], "bd": 2, "relief": "solid", "padx": 8, "pady": 2}
        opts.update(kw)
        b = tk.Button(parent, text=self.t(key), command=command, **opts)
        if "tip_" + key in T["fr"]:
            _Tip(b, lambda: self.t("tip_" + key))
        return b

    def _build(self):
        w = self.win
        top = tk.Frame(w, bg=self.bg)
        top.pack(fill="x", padx=10, pady=(4, 0))
        top.columnconfigure(1, weight=1)
        kinds = tk.Frame(top, bg=self.bg)
        kinds.grid(row=0, column=0, columnspan=2, sticky="w")
        self.src_mode = tk.StringVar(value="pi" if getattr(self.app, "_target_is_pi", lambda: False)() else "sd")   # defaut = cible globale du toolkit
        for val, key in (("sd", "src_kind_sd"), ("pi", "src_kind_pi")):
            tk.Radiobutton(kinds, text=self.t(key), variable=self.src_mode, value=val, bg=self.bg, fg=self.fg, selectcolor=self.bg, activebackground=self.bg,
                           command=self._on_src_mode).pack(side="left", padx=(0, 14))
        self._btn(kinds, "help_btn", self._show_help, pady=0).pack(side="left", padx=(6, 0))
        self._btn(kinds, "snap_btn", lambda: self._rebuild_snapshot(False), pady=0).pack(side="left", padx=(6, 0))
        self.src_lbl = tk.Label(top, text="", bg=self.bg, fg=self.fg, anchor="e", font=("TkDefaultFont", 8))
        self.src_lbl.grid(row=0, column=2, columnspan=3, sticky="e")
        self.sd_lbl = tk.Label(top, text=self.t("sd"), bg=self.bg, fg=self.fg)
        self.sd_lbl.grid(row=1, column=0, sticky="w")
        self.sd_var = tk.StringVar()
        tk.Entry(top, textvariable=self.sd_var).grid(row=1, column=1, padx=6, sticky="we")
        self.browse_btn = self._btn(top, "browse", self._pick_sd, width=11, pady=0)
        self.browse_btn.grid(row=1, column=2, padx=(0, 4))
        self._btn(top, "auto", self._auto_sd, width=11, pady=0).grid(row=1, column=3, padx=(0, 4))
        self._btn(top, "load", self._reload, width=11, pady=0).grid(row=1, column=4)
        moderow = tk.Frame(top, bg=self.bg)
        moderow.grid(row=2, column=0, columnspan=5, sticky="we", pady=(2, 0))
        self.mode_lbl = tk.Label(moderow, text="", bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 9, "bold"), justify="left")
        self.mode_lbl.pack(side="left")
        self.msg = _Ticker(moderow, self.bg, "#1B7F3B", ("TkDefaultFont", 9))
        self.msg.cv.pack(side="left", fill="x", expand=True, padx=(14, 0))

        bar = tk.Frame(w, bg=self.bg)
        bar.pack(fill="x", padx=10, pady=(4, 0))
        tk.Label(bar, text=self.t("system"), bg=self.bg, fg=self.fg).pack(side="left")
        self.system_var = tk.StringVar()
        self.system_combo = ttk.Combobox(bar, textvariable=self.system_var, state="readonly", width=16)
        self.system_combo.pack(side="left", padx=(4, 12))
        self.system_combo.bind("<<ComboboxSelected>>", lambda ev: self._load_system(self._sys_key(self.system_var.get())))
        tk.Label(bar, text=self.t("view"), bg=self.bg, fg=self.fg).pack(side="left")
        self.view_keys = ["v_all", "v_none", "v_has", "v_dir", "v_weak"]
        self.view_var = tk.StringVar(value=self.t("v_all"))
        self.view_combo = ttk.Combobox(bar, textvariable=self.view_var, state="readonly", width=18, values=[self.t(k) for k in self.view_keys])
        self.view_combo.pack(side="left", padx=(4, 12))
        self.view_combo.bind("<<ComboboxSelected>>", lambda ev: self._refill())
        tk.Label(bar, text=self.t("filter"), bg=self.bg, fg=self.fg).pack(side="left")
        self.filter_var = tk.StringVar()
        self.filter_var.trace_add("write", lambda *a: self._refill_soon())
        tk.Entry(bar, textvariable=self.filter_var, width=18).pack(side="left", padx=(4, 0))
        self.stats = tk.Label(w, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), anchor="w")
        self.stats.pack(fill="x", padx=10)

        body = tk.PanedWindow(w, orient="horizontal", bg=self.bg, sashwidth=6)
        body.pack(fill="both", expand=True, padx=10, pady=(2, 4))
        left = tk.Frame(body, bg=self.bg)
        body.add(left, minsize=280, width=340)
        self.tree = ttk.Treeview(left, columns=("gifs", "dir"), show="tree headings", selectmode="browse")
        self.tree.heading("#0", text=self.t("col_title"))
        self.tree.heading("gifs", text=self.t("col_gifs"))
        self.tree.heading("dir", text=self.t("col_dir"))
        self.tree.column("#0", width=200, stretch=True)
        self.tree.column("gifs", width=56, anchor="center", stretch=False)
        self.tree.column("dir", width=66, anchor="center", stretch=False)
        sb = ttk.Scrollbar(left, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=sb.set)
        self.tree.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        self.tree.bind("<<TreeviewSelect>>", lambda ev: self._on_select())
        self.tree.tag_configure("none", foreground="#C62828")
        self.tree.tag_configure("weak", foreground="#B26A00")

        right = tk.Frame(body, bg=self.bg)
        body.add(right, minsize=520)
        toprow = tk.Frame(right, bg=self.bg)          # « Jeu : ... » a gauche, actions sur le JEU a droite (petits boutons)
        toprow.pack(fill="x", pady=(0, 2))
        self.sel_lbl = tk.Label(toprow, text=self.t("sel_none"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 11, "bold"), anchor="w")
        self.sel_lbl.pack(side="left", fill="x", expand=True)
        sg = {"pady": 0, "padx": 8, "font": ("TkDefaultFont", 8), "bd": 1}
        clear_btn = self._btn(toprow, "clear_btn", self._clear_folder, **sg)
        clear_btn.pack(side="right", padx=(4, 0))
        sync_btn = self._btn(toprow, "sync_btn", self._sync, **sg)
        sync_btn.pack(side="right", padx=(4, 0))
        excl_list_btn = self._btn(toprow, "excl_btn_list", self._open_exclusions, **sg)
        excl_list_btn.pack(side="right", padx=(4, 0))
        self.import_btn = self._btn(toprow, "import", self._import, **sg)
        self.import_btn.pack(side="right", padx=(4, 0))
        # zone des GIFs du jeu : cases a cocher (GIFs joues + candidats) et Tous / Aucun
        head = tk.Frame(right, bg=self.bg)
        head.pack(fill="x", pady=(2, 0))
        tk.Label(head, text=self.t("zone_title"), bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 9, "bold")).pack(side="left")
        small = {"pady": 0, "padx": 4, "width": 5, "font": ("TkDefaultFont", 7), "bd": 1}
        self._btn(head, "check_none", self._check_none, **small).pack(side="right")
        self._btn(head, "check_all", self._check_all, **small).pack(side="right", padx=3)
        # bas du panneau : empile depuis le bas (pack side=bottom AVANT la zone extensible) -> la zone des GIFs prend toute la place restante, sans rien masquer
        self.frames_lbl = tk.Label(right, text="", bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8))
        self.frames_lbl.pack(side="bottom", fill="x")
        pvbox = tk.Frame(right, bg=self.bg)
        pvbox.pack(side="bottom", anchor="w")
        self.canvas = tk.Canvas(pvbox, width=PV_W, height=PV_H, bg="#000000", highlightthickness=1, highlightbackground="#888888")
        self.canvas.pack(side="left")
        try:
            self.bright = max(0.1, min(1.0, float(_pref_get("custom_led_brightness") or 0.5)))
        except ValueError:
            self.bright = 0.5
        self.bright_scale = tk.Scale(pvbox, from_=1.0, to=0.1, resolution=0.05, orient="vertical", showvalue=False, length=PV_H, width=14, command=self._on_bright,
                                     bg=self.bg, fg=self.fg, highlightthickness=0, bd=0)
        self.bright_scale.set(self.bright)
        self.bright_scale.pack(side="left", padx=(6, 0))
        _Tip(self.bright_scale, lambda: self.t("bright"))
        acts = tk.Frame(right, bg=self.bg)           # actions sur les FICHIERS coches, collees a la ligne de texte, alignees a droite
        acts.pack(side="bottom", fill="x", pady=(0, 6))
        sm = {"pady": 0, "padx": 6, "width": 9, "font": ("TkDefaultFont", 8), "bd": 1}
        self.copy_btn = self._btn(acts, "copy_btn", self._copy_to_folder, **sm)
        self.remove_btn = self._btn(acts, "remove_btn", self._remove_from_folder, **sm)
        self.rename_btn = self._btn(acts, "rename", self._rename, **sm)
        self.excl_gif_btn = self._btn(acts, "excl_btn_gif", self._exclude_gif, **sm)
        for b_ in (self.excl_gif_btn, self.rename_btn, self.remove_btn, self.copy_btn):     # pack a droite : l'ordre visible reste Copier / Retirer / Renommer / Exclure
            b_.pack(side="right", padx=(4, 0))
        txt = tk.Frame(right, bg=self.bg)
        txt.pack(side="bottom", fill="x")
        self.rank_lbl = tk.Label(txt, text="", bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 9))
        self.rank_lbl.pack(fill="x")
        self.excl_lbl = tk.Label(txt, text="", bg=self.bg, fg="#B26A00", anchor="w", justify="left", wraplength=700, font=("TkDefaultFont", 8))
        host = tk.Frame(right, bg="#FFFFFF", bd=1, relief="solid")
        host._no_bg_slice = True
        host.pack(side="top", fill="both", expand=True)
        self.rows_cv = tk.Canvas(host, height=40, bg="#FFFFFF", highlightthickness=0)
        self.rows_cv._no_bg_slice = True
        vsb = ttk.Scrollbar(host, orient="vertical", command=self.rows_cv.yview)
        self.rows_cv.configure(yscrollcommand=vsb.set)
        self.rows_cv.pack(side="left", fill="both", expand=True)
        vsb.pack(side="right", fill="y")
        self.rows_in = tk.Frame(self.rows_cv, bg="#FFFFFF")
        self.rows_in._no_bg_slice = True
        self.rows_cv.create_window((0, 0), window=self.rows_in, anchor="nw", tags="inner")
        self.rows_in.bind("<Configure>", lambda e: self.rows_cv.configure(scrollregion=self.rows_cv.bbox("all")))
        self.rows_cv.bind("<Configure>", lambda e: self.rows_cv.itemconfigure("inner", width=e.width))

        def wheel_on(_e):
            self.rows_cv.bind_all("<MouseWheel>", lambda ev: self.rows_cv.yview_scroll(int(-ev.delta / 120), "units"))

        def wheel_off(e):
            w = self.win.winfo_containing(e.x_root, e.y_root)
            if w is None or not str(w).startswith(str(host)):
                self.rows_cv.unbind_all("<MouseWheel>")
        host.bind("<Enter>", wheel_on)
        host.bind("<Leave>", wheel_off)
        self.rows = []
        self._prev = None
        if not self.embedded:
            self._btn(w, "close", self._close).pack(anchor="e", padx=10, pady=(0, 8))

    # ------------------------------------------------------------------ choix des dossiers
    def follow_target(self, pi: bool):
        """Cible globale du toolkit (bandeau ESP32 / Pi) : la source suit la bascule ; le choix manuel dans cette page reste possible ensuite."""
        want = "pi" if pi else "sd"
        if self.src_mode.get() != want:
            self.src_mode.set(want)
            self._on_src_mode()

    def _on_src_mode(self):
        pi = self.src_mode.get() == "pi"
        self.sd_lbl.configure(text=self.t("pi_ip") if pi else self.t("sd"))
        self.browse_btn.configure(state="disabled" if pi else "normal")
        self.sd_var.set(_pref_get("game_gifs_pi") if pi else (_pref_get("game_gifs_sd") or ""))
        if not self.autoload:
            return
        val = self.sd_var.get().strip()
        if val and (pi or (Path(val) / "gifs").is_dir()):
            self._reload()                       # la liste se charge toute seule des qu'on choisit la source
        else:
            self._auto_sd(quiet=True)

    def _pick_sd(self):
        if self.src_mode.get() == "pi":
            return
        d = filedialog.askdirectory(parent=self.win, title=self.t("sd"))
        if d:
            self.sd_var.set(d)
            self._reload()

    def _auto_sd(self, quiet=False):
        if self.src_mode.get() == "pi":
            tk_mod = getattr(self.app, "tkmod", None)
            self.win.configure(cursor="watch")
            self.win.update_idletasks()
            try:
                pi = tk_mod.find_pi_dmd(_pref_get("game_gifs_pi") or _pref_get("dmd_ip")) if tk_mod else None
            except Exception:   # noqa: BLE001
                pi = None
            finally:
                self.win.configure(cursor="")
            if pi and pi.get("ip"):
                self.sd_var.set(pi["ip"])
                self._reload()
            elif quiet:
                self.mode_lbl.configure(text=self.t("no_pi_found"), fg="#C62828")
            else:
                messagebox.showinfo(self.t("title"), self.t("no_pi_found"), parent=self.win)
            return
        try:
            d = self.app._find_dmd_sd_root() or ""
        except Exception:   # noqa: BLE001
            d = ""
        if d:
            self.sd_var.set(d)
            self._reload()
        elif quiet:
            self.mode_lbl.configure(text=self.t("no_sd"), fg="#C62828")
        else:
            messagebox.showinfo(self.t("title"), self.t("no_sd"), parent=self.win)

    def _pi_fail_msg(self, err) -> str:
        """Message d'un echec sur le Pi : si le demon repond mais n'a pas les routes /gg-* (version < 3.87, ex. la 3.83 du canal stable), on dit QUELLE version
        il a et qu'il faut le canal alpha, au lieu du message generique « injoignable ou trop ancien »."""
        try:
            import pi_logos as _pl
            info = _pl.probe(self.sd_var.get().strip())
            if info and info.get("is_pi"):
                _num = _pl._tool().fw_version_num
                if _num(info.get("version")) < _num("3.87"):
                    return self.t("pi_too_old", v=info.get("version"))
        except Exception:   # noqa: BLE001
            pass
        return self.t("pi_unreachable", why=str(err))

    # ------------------------------------------------------------------ chargement
    def _reload(self):
        """La liste des jeux vient de la CARTE SD du DMD (instantane du dernier Mode 1 : custom/games.json, comme l'onglet Personnalisation) ou du Pi en Wi-Fi (GET /games-snapshot),
        pas d'un dossier ROMs qui peut differer de ce que le DMD possede. Sur SD sans instantane : les jeux qui ont un visuel sur la SD (titre = nom de fichier)."""
        pi = self.src_mode.get() == "pi"
        val = self.sd_var.get().strip()
        if not pi:
            if not val or not (Path(val) / "gifs").is_dir():
                messagebox.showinfo(self.t("title"), self.t("no_sd"), parent=self.win)
                return
            _pref_set("game_gifs_sd", val)
            store = gs.SdStore(val)
            self.snap = cl.load_snapshot(cl.snapshot_path(val))
            snap_missing = not cl.snapshot_path(val).is_file()
            names = sorted(set(self.snap["systems"]) | set(cl.list_systems_sd(val)), key=str.lower)
        else:
            if not val:
                messagebox.showinfo(self.t("title"), self.t("no_pi"), parent=self.win)
                return
            store = gs.PiStore(val)
            self.win.configure(cursor="watch")
            self.win.update_idletasks()
            try:
                store.ls("/gifs")                 # 404 = demon trop ancien (routes /gg-* absentes) ; erreur reseau = injoignable
                data = store.snapshot_bytes()
            except OSError as e:
                messagebox.showerror(self.t("title"), self._pi_fail_msg(e), parent=self.win)
                return
            finally:
                self.win.configure(cursor="")
            _pref_set("game_gifs_pi", val)
            snap_missing = not data
            self.snap = cl.new_snapshot()
            if data:
                tmpd = Path(tempfile.mkdtemp(prefix="gifs_snap_"))
                try:
                    (tmpd / "games.json").write_bytes(data)
                    self.snap = cl.load_snapshot(tmpd / "games.json")
                finally:
                    shutil.rmtree(tmpd, ignore_errors=True)
            names = sorted(self.snap["systems"], key=str.lower)
        self._release_store()
        self.sd_root = store
        if snap_missing and getattr(self, "_auto_snap_key", None) != (pi, val):   # instantane absent : recreation automatique (une seule tentative par cible, pas de boucle)
            self._auto_snap_key = (pi, val)
            if self._rebuild_snapshot(True):
                return
        self.system_combo["values"] = [self.t("sys_all")] + names
        if not names:
            messagebox.showinfo(self.t("title"), self.t("no_games_pi" if pi else "no_games"), parent=self.win)
            return
        keep = self.system if (self.system in names or self.system == ALL_KEY) else names[0]
        self.system_var.set(self.t("sys_all") if keep == ALL_KEY else keep)
        self._load_system(keep)

    def _rebuild_snapshot(self, auto=False) -> bool:
        """Recree custom/games.json SANS refaire de Mode 1 : liste des jeux = gamelist.xml du dossier ROMs (celui de la fenetre principale), puis ecriture sur la carte SD
        (fusion : le plus recent gagne par systeme) ou envoi au Pi en Wi-Fi (POST /gg-put, demon recent). `auto` : appel de _reload() quand l'instantane est absent ; renvoie
        True si un traitement asynchrone a pris la main (l'appelant n'affiche alors rien : _reload() est relance a la fin)."""
        store = self.sd_root
        pi = self.src_mode.get() == "pi"
        if store is None:
            messagebox.showinfo(self.t("title"), self.t("no_pi" if pi else "no_sd"), parent=self.win)
            return False
        if auto:
            messagebox.showinfo(self.t("title"), self.t("snap_auto_info"), parent=self.win)
        # TOUJOURS demander / confirmer le dossier ROMs (celui de la fenetre principale est propose) : un dossier ROMs perime ou d'une autre Recalbox donnerait une liste fausse
        cur = ""
        try:
            cur = self.app.roms_path_var.get().strip()
        except Exception:   # noqa: BLE001
            pass
        roms = filedialog.askdirectory(parent=self.win, title=self.t("snap_pick_roms"), initialdir=cur if cur and Path(cur).exists() else None, mustexist=True)
        if not roms:                                  # annule
            if auto:
                messagebox.showinfo(self.t("title"), self.t("snap_need_roms"), parent=self.win)
            return pi if auto else False
        roms = str(Path(roms))
        if not cl.roms_systems(roms):
            messagebox.showinfo(self.t("title"), self.t("snap_no_gamelist", path=roms), parent=self.win)
            return pi if auto else False
        try:
            self.app.roms_path_var.set(roms)          # memorise le choix pour les autres fonctions du toolkit
        except Exception:   # noqa: BLE001
            pass

        def work():
            roms_map = cl.roms_systems(roms)
            only = set(cl.list_systems_sd(store.root)) if store.kind == "sd" else None   # carte : seulement les systemes qu'elle possede ; Pi : tous (non listables)
            snap = cl.build_snapshot_from_roms(roms_map, only, log=print)
            if not snap["systems"]:
                return None
            if store.kind == "sd":
                path = cl.snapshot_path(store.root)
                merged = cl.merge_snapshots(cl.load_snapshot(path), snap)
                cl.save_snapshot(path, merged)
                snap = merged
            else:
                try:
                    store.put("/custom/games.json", cl.snapshot_bytes(snap))
                except OSError as e:
                    if str(e).startswith("400"):      # demon sans l'autorisation d'ecriture de /custom/games.json
                        raise RuntimeError(self.t("snap_pi_old")) from None
                    raise
            return snap

        def finished(snap):
            if snap is None:
                messagebox.showinfo(self.t("title"), self.t("snap_empty"), parent=self.win)
                if auto and not pi:
                    self._reload()
                return
            n, g = len(snap["systems"]), sum(len(e["games"]) for e in snap["systems"].values())
            messagebox.showinfo(self.t("title"), self.t("snap_done_pi" if pi else "snap_done_sd", n=n, g=g), parent=self.win)
            self._reload()

        self._do(work, finished)
        return True

    def _sys_key(self, shown):
        return ALL_KEY if shown == self.t("sys_all") else shown

    def _release_store(self):
        """Libere l'ancien magasin (cache de telechargement du Pi) et rend la main au Pi (il ignore les commandes de jeu tant qu'une page de configuration est « ouverte »)."""
        old = self.sd_root
        if old is not None and getattr(old, "kind", "") == "pi":
            try:
                import urllib.request
                urllib.request.urlopen(urllib.request.Request(f"http://{old.ip}/dmd-resume", data=b"", method="POST"), timeout=5).read()
            except Exception:   # noqa: BLE001
                pass
            old.close()

    def _load_system(self, system):
        self.system = system
        self._token += 1
        token = self._token
        self.stats.configure(text=self.t("loading"))
        for i in self.tree.get_children():
            self.tree.delete(i)
        self._clear_detail()

        def work():
            if system == ALL_KEY:                        # « Tous » : les jeux de tous les systemes (instantane + visuels de la SD)
                lib = self._lib_snap()
                games = [g for s in sorted(lib["systems"], key=str.lower) for g in (cl.games_from_snapshot(lib, s) or [])]
                _dates = [str(e.get("date", "")) for e in lib["systems"].values() if e.get("date")]
                source = ("snap", max(_dates).replace("T", " ")[:16] if _dates else "")   # « Tous » : date du Mode 1 le plus recent (avant : « ? »)
            else:
                games = cl.games_from_snapshot(self.snap, system)
                source = ("snap", self.snap["systems"][system].get("date", "")) if games is not None else ("sd", "")
            if games is None:
                games = cl.games_from_sd(self.sd_root.root, system) if self.sd_root.kind == "sd" else []
            games = [g for g in games if not g.get("hidden")]
            seen, uniq = set(), []                       # plusieurs ROMs du meme jeu (regions, versions) : la regle ne regarde que le TITRE -> une seule ligne
            for g in games:
                key = gm.norm(g["name"]) or g["name"].lower()
                if key in seen:
                    continue
                seen.add(key)
                uniq.append(g)
            games = uniq
            return (games,) + self._analyse(games) + (source,)

        def done(res, err):
            if token != self._token:
                return
            if err is not None:
                self.stats.configure(text="")
                messagebox.showerror(self.t("title"), str(err), parent=self.win)
                return
            self.games, self.gifs, self.active, self.det, self.det_dir, self.cand, self.excl_text, nw, source = res
            self.src_lbl.configure(text=self.t("src_snap_pi" if self.sd_root.kind == "pi" else "src_snap", date=source[1] or "?") if source[0] == "snap" else self.t("src_sd"), fg=(self.fg if source[0] == "snap" else "#B26A00"))
            self._set_mode_label(nw)
            self._refill()

        self._bg(work, done)

    def _analyse(self, games):
        """(GIFs de /gifs, GIFs lus par le DMD, detection STRICTE sur /gifs (carte SD ou Pi), detection STRICTE sur le dossier dedie (mode dossier, sinon vide),
        candidats approximatifs de /gifs, exclusions, nouveautes). La liste des jeux montre toujours la carte SD / le Pi ; le dossier n'apparait que dans la zone des GIFs du jeu."""
        src = gg.list_gif_files(self.sd_root)
        active = gg.active_gifs(self.sd_root)
        excl = gg.read_exclusions_text(self.sd_root)
        det_dir = gg.detect_strict(games, active, excl) if active is not src and gg.has_folder(self.sd_root) else {}
        return src, active, gg.detect_strict(games, src, excl), det_dir, gg.detect(games, src, excl), excl, gg.news(self.sd_root, self._lib_snap(), src)

    def _show_help(self):
        """Aide : les trois modes de lecture du DMD (auto / exclusif / mixte) et ou se regle le mode mixte (configuration web du DMD ou du Pi)."""
        messagebox.showinfo(self.t("title"), self.t("help_text"), parent=self.win)

    def _set_mode_label(self, nw):
        self._folder_mode = gg.has_folder(self.sd_root)
        txt = self.t("mode_folder", n=len(self.active)) if self._folder_mode else self.t("mode_auto")
        if nw["first"] or nw["gifs"] or nw["games"]:
            txt += self.t("news_first") if nw["first"] else self.t("news", gifs=nw["gifs"], games=nw["games"])
        self.mode_lbl.configure(text=txt, fg=("#1B7F3B" if self._folder_mode else self.fg))

    def _do(self, work, ok, fail=None, parent=None):
        """Execute `work()` HORS du thread d'interface (curseur d'attente), puis `ok(resultat)` dans l'interface. Une erreur (Pi eteint ou injoignable, disque...) affiche un message
        au lieu de figer la fenetre pendant les delais reseau ; `fail()` est appele apres le message (nettoyage)."""
        self.win.configure(cursor="watch")

        def done(res, err):
            self.win.configure(cursor="")
            if err is not None:
                why = self._pi_fail_msg(err) if (isinstance(err, OSError) and getattr(self.sd_root, "kind", "") == "pi") else self.t("err", why=str(err))
                messagebox.showerror(self.t("title"), why, parent=parent or self.win)
                if fail:
                    fail()
                return
            ok(res)
        self._bg(work, done)

    def _bg(self, work, done):
        def run():
            try:
                res, err = work(), None
            except Exception as e:   # noqa: BLE001
                res, err = None, e
            try:
                self.win.after(0, lambda: done(res, err))
            except Exception:   # noqa: BLE001
                pass
        threading.Thread(target=run, daemon=True).start()

    # ------------------------------------------------------------------ liste
    _refill_job = None

    def _refill_soon(self):
        if self._refill_job:
            try:
                self.win.after_cancel(self._refill_job)
            except Exception:   # noqa: BLE001
                pass
        self._refill_job = self.win.after(250, self._refill)

    def _view_key(self):
        try:
            return self.view_keys[list(self.view_combo["values"]).index(self.view_var.get())]
        except ValueError:
            return "v_all"

    def _refill(self):
        for i in self.tree.get_children():
            self.tree.delete(i)
        flt = gm.norm(self.filter_var.get())
        raw_flt = self.filter_var.get().strip().lower()
        vk = self._view_key()
        self.shown = []
        n_has = sum(1 for i in range(len(self.games)) if i in self.det)
        for i, g in enumerate(self.games):
            d = self.det.get(i)
            if vk == "v_none" and d:
                continue
            if vk == "v_dir" and i not in self.det_dir:        # au moins un GIF dans /gifs_jeux
                continue
            if vk == "v_has" and not d:
                continue
            if vk == "v_weak" and not (not d and i in self.cand):
                continue
            if raw_flt and raw_flt not in g["name"].lower() and (not flt or flt not in gm.norm(g["name"])):
                continue
            self.shown.append(i)
        for i in self.shown[:MAX_SHOWN]:
            d = self.det.get(i)
            label = f"{len(d[1])}" if d else ("?" if i in self.cand else "—")
            dd = self.det_dir.get(i)
            dlabel = (f"{len(dd[1])}" if dd else "—") if self._folder_mode else ""      # colonne « Dossier » : GIFs de /gifs_jeux (mode dossier seulement)
            self.tree.insert("", "end", iid=str(i), text=self.games[i]["name"], values=(label, dlabel), tags=(() if d else (("weak",) if i in self.cand else ("none",))))
        tail = self.t("shown", s=len(self.shown)) if len(self.shown) != len(self.games) else ""
        self.stats.configure(text=self.t("stats", n=len(self.games), has=n_has, none=len(self.games) - n_has, gifs=len(self.gifs)) + tail + (f" — {self.t('sd_gifs0')}" if not self.gifs else ""))

    def _set_excl(self, text):
        self.excl_lbl.configure(text=text)
        if text:
            self.excl_lbl.pack(fill="x")
        else:
            self.excl_lbl.pack_forget()

    def _clear_detail(self):
        self.sel = None
        self.sel_lbl.configure(text=self.t("sel_none"))
        self._fill_rows([], [])
        self._cands = []
        self.rank_lbl.configure(text="")
        self.msg.configure(text="")
        self._stop_anim()
        self.canvas.delete("all")
        self.frames_lbl.configure(text="")
        self._update_buttons()
        self._set_excl("")

    def _on_select(self):
        s = self.tree.selection()
        if not s:
            return
        self.sel = int(s[0])
        g = self.games[self.sel]
        self.sel_lbl.configure(text=self.t("sel_title", name=g["name"]))
        dd, ds = self.det_dir.get(self.sel), self.det.get(self.sel)
        play = (list(dd[1]) if dd else []) + (list(ds[1]) if ds else [])      # d'abord ceux du dossier (pre-coches), puis ceux de la carte SD / du Pi
        playing = {x["sd"] for x in play}
        self._cands = [x for x in (self.cand.get(self.sel, (0, []))[1]) if x["sd"] not in playing]
        self._fill_rows(play, self._cands)
        self._show_excl_info(g["name"])
        self._stop_anim()
        self.canvas.delete("all")
        self.frames_lbl.configure(text="")
        if self.rows:
            self._preview_row(self.rows[0])
        else:
            self.rank_lbl.configure(text=self.t("none_found"), fg=self.fg)
        self._update_buttons()

    # ---- zone a cases a cocher
    def _fill_rows(self, play, cands):
        for w in self.rows_in.winfo_children():
            w.destroy()
        self.rows, self._prev = [], None

        def section(text):
            tk.Label(self.rows_in, text=text, bg="#E6E6E6", fg="#333333", anchor="w", font=("TkDefaultFont", 8)).pack(fill="x")

        def add(gif, kind):
            folder = gif["sd"].startswith(f"/{gg.FOLDER}/")
            var = tk.BooleanVar(value=folder)          # un GIF qui est deja dans le dossier est pre-coche
            fr = tk.Frame(self.rows_in, bg="#FFFFFF")
            fr.pack(fill="x")
            cb = tk.Checkbutton(fr, variable=var, bg="#FFFFFF", activebackground="#FFFFFF", bd=0, highlightthickness=0, command=self._update_buttons)
            cb.pack(side="left")
            lb = tk.Label(fr, text=gif["sd"] + ("  " + self.t("in_folder") if folder else ""), bg="#FFFFFF", fg="black", anchor="w", cursor="hand2")
            lb.pack(side="left", fill="x", expand=True)
            row = {"gif": gif, "kind": kind, "var": var, "lbl": lb, "folder": folder}
            lb.bind("<Button-1>", lambda e, r=row: self._toggle_row(r))        # clic sur le nom = coche / decoche
            for w in (fr, cb, lb):                                               # la barre coloree et l'apercu suivent le survol
                w.bind("<Enter>", lambda e, r=row: self._hover_row(r), add="+")
            self.rows.append(row)
        in_dir = [g for g in play if g["sd"].startswith(f"/{gg.FOLDER}/")]
        in_src = [g for g in play if not g["sd"].startswith(f"/{gg.FOLDER}/")]
        if in_dir:
            section(self.t("sec_dir"))
            for gif in in_dir:
                add(gif, "play")
        if in_src:
            section(self.t("sec_pi" if getattr(self.sd_root, "kind", "") == "pi" else "sec_sd"))
            for gif in in_src:
                add(gif, "play")
        if cands:
            section(self.t("cands"))
            for gif in cands:
                add(gif, "cand")
        self.rows_cv.yview_moveto(0)

    def _toggle_row(self, row):
        row["var"].set(not row["var"].get())
        self._update_buttons()

    def _hover_row(self, row):
        """Survol : la barre coloree suit tout de suite ; l'apercu (rendu LED, couteux) demarre apres 150 ms sans changer de ligne."""
        if self._prev is row:
            return
        if self._prev is not None:
            try:
                self._prev["lbl"].configure(bg="#FFFFFF")
            except tk.TclError:
                pass
        self._prev = row
        row["lbl"].configure(bg="#CFE3FF")
        if getattr(self, "_hover_after", None):
            try:
                self.win.after_cancel(self._hover_after)
            except Exception:   # noqa: BLE001
                pass
        self._hover_after = self.win.after(150, lambda r=row: self._preview_row(r) if self._prev is r else None)

    def _preview_row(self, row):
        if self._prev is not None and self._prev is not row:
            try:
                self._prev["lbl"].configure(bg="#FFFFFF")
            except tk.TclError:
                pass
        self._prev = row
        row["lbl"].configure(bg="#CFE3FF")
        self.rank_lbl.configure(text=self.t("rank3") if row["kind"] == "play" else self.t("cand_hint"), fg=self.fg if row["kind"] == "play" else "#B26A00")
        self._play(gg.local_file(self.sd_root, row["gif"]))

    def _checked(self):
        return [r for r in self.rows if r["var"].get()]

    def _check_all(self):
        for r in self.rows:
            r["var"].set(True)
        self._update_buttons()

    def _check_none(self):
        for r in self.rows:
            r["var"].set(False)
        self._update_buttons()

    def _update_buttons(self):
        ok = self.sel is not None
        ch = self._checked()
        in_f = [r for r in ch if r["folder"]]
        out_f = [r for r in ch if not r["folder"]]
        plays = [r for r in ch if r["kind"] == "play"]
        st = lambda cond: "normal" if cond else "disabled"   # noqa: E731
        self.import_btn.configure(state=st(ok))
        self.copy_btn.configure(state=st(ok and out_f))
        self.remove_btn.configure(state=st(ok and in_f))
        self.rename_btn.configure(state=st(ok and len(in_f) == 1))
        self.excl_gif_btn.configure(state=st(ok and plays))

    def _rename_target(self):
        in_f = [r["gif"] for r in self._checked() if r["folder"]]
        return in_f[0] if len(in_f) == 1 else None

    def _stop_anim(self):
        if self._anim_after:
            try:
                self.win.after_cancel(self._anim_after)
            except Exception:   # noqa: BLE001
                pass
            self._anim_after = None

    def _play(self, path):
        """Apercu = SIMULATION DU RENDU DMD comme l'onglet de remplacement de logo : chaque image est rendue en 128x32 (custom_logos.render, memes regles) puis dessinee
        en points LED (custom_logos.led_style) a la luminosite choisie ; images rendues a la demande (delai >= 100 ms : le rendu LED prend du temps)."""
        self._stop_anim()
        frames = load_frames(path)
        self.canvas.delete("all")
        if not frames:
            self.canvas.create_text(PV_W // 2, PV_H // 2, text=self.t("preview_none"), fill="#AAAAAA")
            self.frames_lbl.configure(text="")
            return
        self._anim_frames = [(cl.render(im.convert("RGBA"), "fit", (0, 0, 0)), ms) for im, ms in frames]
        self._anim_i, self._led_cache = 0, {}
        self.frames_lbl.configure(text=self.t("frames", n=len(frames)) + f" · {Path(path).name}")
        self._tick()

    def _tick(self):
        if not self._anim_frames:
            return
        n = len(self._anim_frames)
        i = self._anim_i % n
        img, ms = self._anim_frames[i]
        try:
            if i not in self._led_cache:
                from PIL import ImageTk
                self._led_cache[i] = ImageTk.PhotoImage(cl.led_style(img, 4, self.bright))
            self._photo = self._led_cache[i]
            self.canvas.delete("all")
            self.canvas.create_image(0, 0, image=self._photo, anchor="nw")
        except tk.TclError:
            return
        self._anim_i += 1
        self._anim_after = self.win.after(max(100, ms), self._tick)

    _bright_job = None

    def _on_bright(self, v=None):
        """Curseur de luminosite LED (meme preference que l'onglet de remplacement de logo) : redessin differe tant que le curseur bouge."""
        try:
            self.bright = float(self.bright_scale.get())
        except (ValueError, tk.TclError):
            return
        if self._bright_job is not None:
            try:
                self.win.after_cancel(self._bright_job)
            except Exception:   # noqa: BLE001
                pass

        def go():
            self._bright_job = None
            _pref_set("custom_led_brightness", f"{self.bright:.2f}")
            self._led_cache = {}
        self._bright_job = self.win.after(120, go)

    # ------------------------------------------------------------------ renommer
    def _rename(self):
        """Renomme un GIF DU DOSSIER dedie ; le nouveau nom doit correspondre au titre du jeu (regle stricte)."""
        gif = self._rename_target()
        if gif is None:
            return
        game = self.games[self.sel]
        title_n = gm.norm(game["name"])
        dlg = tk.Toplevel(self.win)
        dlg.configure(bg=self.bg)
        dlg.title(self.t("rename_title"))
        dlg.transient(self.win.winfo_toplevel())
        tk.Label(dlg, text=gif["sd"], bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", padx=12, pady=(10, 2))
        tk.Label(dlg, text=self.t("rename_ask"), bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", padx=12)
        var = tk.StringVar(value=gif["name"])
        ent = tk.Entry(dlg, textvariable=var, width=60)
        ent.pack(fill="x", padx=12, pady=2)
        info = tk.Label(dlg, text="", bg=self.bg, anchor="w", justify="left", wraplength=520)
        info.pack(fill="x", padx=12, pady=4)

        def refresh(*a):
            nm = var.get().strip()
            why = gg.check_new_name(nm)
            if why:
                info.configure(text=self.t("err", why=why), fg="#C62828")
                ok_btn.configure(state="disabled")
                return
            good = gm.strict_match(title_n, nm)
            ok_btn.configure(state="normal" if good else "disabled")
            info.configure(text=self.t("name_ok", name=game["name"]) if good else self.t("name_bad"), fg=("#1B7F3B" if good else "#C62828"))

        row = tk.Frame(dlg, bg=self.bg)
        row.pack(fill="x", padx=12, pady=6)
        taken = {g["name"].lower() for g in self.active if g["sd"] != gif["sd"]}
        self._btn(row, "rename_suggest", lambda: var.set(gg.export_name(game["name"], taken) or var.get())).pack(side="left")
        ok_btn = self._btn(row, "ok", lambda: go())
        ok_btn.pack(side="right")
        self._btn(row, "cancel", dlg.destroy).pack(side="right", padx=6)
        var.trace_add("write", refresh)
        refresh()

        def go():
            new = var.get().strip()

            def finished(res):
                if not res["ok"]:
                    messagebox.showerror(self.t("rename_title"), self.t("err", why=res["error"]), parent=dlg)
                    return
                dlg.destroy()
                self._after_change(self.t("renamed2", old=gif["name"], new=Path(res["new_sd"]).name))
            self._do(lambda: gg.rename_in_folder(self.sd_root, gif["sd"], new, game["name"]), finished, parent=dlg)

        ent.focus_set()
        dlg.grab_set()

    def _clear_folder(self):
        """« Tout supprimer » : vide /gifs_jeux apres confirmation (les originaux de /gifs ne sont pas touches)."""
        if self.sd_root is None:
            messagebox.showinfo(self.t("title"), self.t("no_pi" if self.src_mode.get() == "pi" else "no_sd"), parent=self.win)
            return
        if not self._folder_mode:
            messagebox.showinfo(self.t("title"), self.t("clear_empty"), parent=self.win)
            return
        n = len(self.active)
        if not messagebox.askyesno(self.t("title"), self.t("clear_confirm", n=n), icon="warning", parent=self.win):
            return
        self._do(lambda: gg.clear_folder(self.sd_root), lambda cnt: self._after_change(self.t("clear_done", n=cnt)))

    def _import(self):
        """Ajoute un GIF pour le jeu choisi avec le MEME selecteur que l'onglet de remplacement de logo (miniatures, extrait, modes pixel / ajuster / remplir / recadrer, fond) ;
        le fichier est converti au format de la dalle si besoin (game_gifs.convert_gif = meme moteur custom_logos.render), puis copie dans /gifs_jeux/<lettre>/."""
        if self.sel is None:
            return
        if self.sd_root is None:
            messagebox.showinfo(self.t("title"), self.t("no_pi" if self.src_mode.get() == "pi" else "no_sd"), parent=self.win)
            return
        game = self.games[self.sel]
        import RecalBoxDMD_custom_logos_gui as clg

        class _Host:                       # ce que le selecteur attend de l'onglet qui l'ouvre
            gui = self.app
            bright_var = tk.DoubleVar(value=0.5)
        clg.ImagePicker(_Host, {"name": game["name"], "stem": game["name"]}, lambda path, mode, bg, sel: self._import_picked(game, path, mode, bg, sel))

    def _import_picked(self, game, path, mode, bg, sel):
        import custom_logos as cl
        tmpd = Path(tempfile.mkdtemp(prefix="gg_add_"))
        tmp = tmpd / "import.gif"
        n_src = cl.gif_frame_count(path)
        whole = sel is None or (sel[0] == "anim" and sel[1] == 0 and sel[2] >= n_src - 1)
        self.win.configure(cursor="watch")

        def work():
            if whole and gg.native_ok(path):
                shutil.copyfile(path, tmp)
                return {"native": True}
            a, b = (sel[1], sel[2]) if sel and sel[0] == "anim" else (0, None)
            still = sel[1] if sel and sel[0] == "still" else None
            return gg.convert_gif(path, tmp, mode=mode, bg=bg, start=a, end=b, still=still)

        def done(rep, err):
            self.win.configure(cursor="")
            if err is not None:
                shutil.rmtree(tmpd, ignore_errors=True)
                messagebox.showerror(self.t("title"), self.t("conv_fail", why=str(err)), parent=self.win)
                return
            if rep.get("native"):
                note = self.t("conv_native")
            else:
                note = self.t("conv_done", n=rep["frames"], s=f"{rep['duration_ms'] / 1000:.1f}") + (self.t("conv_resampled", k=rep["resampled"]) if rep["resampled"] else "")
            self._import_dialog(game, str(tmp), note, tmpd)

        self._bg(work, done)

    def _import_dialog(self, game, src, note, tmpd):
        """Lit d'abord (en arriere-plan) les noms deja pris dans le dossier : un Pi eteint ne fige plus la fenetre."""
        self._do(lambda: {g["name"].lower() for g in gg.list_folder_gifs(self.sd_root)},
                 lambda taken: self._import_dialog_build(game, src, note, tmpd, taken),
                 fail=lambda: shutil.rmtree(tmpd, ignore_errors=True))

    def _import_dialog_build(self, game, src, note, tmpd, taken):
        """Nom du fichier (doit correspondre au titre) puis copie dans le dossier dedie (SD ou Pi) ; `tmpd` (fichier converti) est supprime a la fermeture."""
        title_n = gm.norm(game["name"])
        dlg = tk.Toplevel(self.win)
        dlg.configure(bg=self.bg)
        dlg.title(self.t("add_title", name=game["name"]))
        dlg.bind("<Destroy>", lambda e: shutil.rmtree(tmpd, ignore_errors=True) if e.widget is dlg else None)
        dlg.transient(self.win.winfo_toplevel())
        cv = tk.Canvas(dlg, width=PV_W // 2, height=PV_H // 2, bg="#000000", highlightthickness=1)
        cv.pack(padx=12, pady=(10, 4))
        frames = load_frames(src, 40)
        if frames:
            from PIL import Image, ImageTk
            im = frames[0][0]
            k = min((PV_W // 2) / im.size[0], (PV_H // 2) / im.size[1])
            ph = ImageTk.PhotoImage(im.resize((max(1, int(im.size[0] * k)), max(1, int(im.size[1] * k))), Image.NEAREST))
            cv.image = ph
            cv.create_image(PV_W // 4, PV_H // 4, image=ph)
        tk.Label(dlg, text=note, bg=self.bg, fg="#1B7F3B", anchor="w", justify="left", wraplength=520).pack(fill="x", padx=12, pady=(0, 4))
        tk.Label(dlg, text=self.t("import_hint"), bg=self.bg, fg=self.fg, anchor="w", justify="left", wraplength=520).pack(fill="x", padx=12)
        tk.Label(dlg, text=self.t("import_name"), bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", padx=12)
        nvar = tk.StringVar(value=gg.export_name(game["name"], taken))
        tk.Entry(dlg, textvariable=nvar, width=60).pack(fill="x", padx=12, pady=2)
        info = tk.Label(dlg, text="", bg=self.bg, anchor="w", justify="left", wraplength=520)
        info.pack(fill="x", padx=12, pady=4)
        row = tk.Frame(dlg, bg=self.bg)
        row.pack(fill="x", padx=12, pady=8)
        ok_btn = self._btn(row, "ok", lambda: go())

        def refresh(*a):
            nm = nvar.get().strip()
            nm = nm if nm.lower().endswith(".gif") else nm + ".gif"
            why = gg.check_new_name(nm)
            good = not why and gm.strict_match(title_n, nm) and nm.lower() not in taken
            ok_btn.configure(state="normal" if good else "disabled")
            info.configure(text=self.t("err", why=why) if why else (self.t("name_ok", name=game["name"]) if good else self.t("name_bad")), fg=("#1B7F3B" if good else "#C62828"))

        def go():
            name = nvar.get().strip()
            ok_btn.configure(state="disabled")

            def finished(res):
                if not res["ok"]:
                    messagebox.showerror(self.t("import_title"), self.t("err", why=res["error"]), parent=dlg)
                    refresh()
                    return
                dlg.destroy()
                self._after_change(self.t("import_done", sd=res["new_sd"]))
            self._do(lambda: gg.import_gif(self.sd_root, src, game["name"], name=name), finished, fail=refresh, parent=dlg)

        nvar.trace_add("write", refresh)
        refresh()
        ok_btn.pack(side="right")
        self._btn(row, "cancel", dlg.destroy).pack(side="right", padx=6)
        dlg.grab_set()

    def _copy_to_folder(self):
        """Copie les GIFs COCHES de /gifs (candidats ou GIFs joues en mode auto) dans le dossier dedie, sous le nom du jeu. Les originaux ne sont pas touches."""
        todo = [r["gif"] for r in self._checked() if not r["folder"]]
        if not todo or self.sel is None:
            return
        title = self.games[self.sel]["name"]

        def work():
            r = gg.add_many(self.sd_root, [{"src_sd": g["sd"], "title": title} for g in todo])
            for it, dest in r["done"]:
                gg.record_export(self.sd_root, it["src_sd"], dest, title)
            return r

        def finished(r):
            if r["failed"]:
                messagebox.showerror(self.t("title"), self.t("copy_fail", why=r["failed"][0][1]), parent=self.win)
            if r["done"]:
                self._after_change(self.t("copied", sd=", ".join(d for _i, d in r["done"])))
        self._do(work, finished)

    def _remove_from_folder(self):
        todo = [r["gif"] for r in self._checked() if r["folder"]]
        if not todo:
            return
        if not messagebox.askyesno(self.t("title"), self.t("confirm_remove", name=todo[0]["name"] if len(todo) == 1 else f"{len(todo)} GIFs"), parent=self.win):
            return
        def work():
            for g in todo:
                r = gg.remove_from_folder(self.sd_root, g["sd"])
                if not r["ok"]:
                    return r
            return {"ok": True}

        def finished(r):
            if not r["ok"]:
                messagebox.showerror(self.t("title"), self.t("err", why=r["error"]), parent=self.win)
            self._after_change(self.t("removed", sd=", ".join(g["sd"] for g in todo)))
        self._do(work, finished)

    def _lib_snap(self):
        """Bibliotheque complete pour l'ajout auto / les nouveautes : l'instantane du Mode 1 + (carte SD seulement) les systemes absents de l'instantane, lus par leurs visuels
        sur la SD (titre = nom de fichier) comme la liste des jeux. Sans cela, une SD sans instantane donnait une liste vide."""
        snap = self.snap or cl.new_snapshot()
        if self.sd_root is None or getattr(self.sd_root, "kind", "") != "sd":
            return snap
        key = (id(snap), str(self.sd_root.root))
        if getattr(self, "_lib_cache", (None,))[0] == key:
            return self._lib_cache[1]
        systems = dict(snap.get("systems", {}))
        for s in cl.list_systems_sd(self.sd_root.root):
            if s in systems and systems[s].get("games"):
                continue
            games = cl.games_from_sd(self.sd_root.root, s)
            if games:
                systems[s] = {"date": "", "games": [{"n": g["name"], "r": g["rom"]} for g in games]}
        lib = {**snap, "systems": systems}
        self._lib_cache = (key, lib)
        return lib

    def _get_titles(self):
        if getattr(self, "_titles_snap", None) is not self.snap:
            self._titles = gg.all_titles(self.snap)
            self._titles_snap = self.snap
        return self._titles

    # ------------------------------------------------------------------ mise a jour du dossier (copies des GIFs surs + nouveautes)
    def _sync(self):
        if not self.sd_root:
            messagebox.showinfo(self.t("title"), self.t("no_sd"), parent=self.win)
            return
        dlg = tk.Toplevel(self.win)
        dlg.configure(bg=self.bg)
        dlg.title(self.t("sync_title"))
        dlg.transient(self.win.winfo_toplevel())
        dlg.geometry("1000x540")
        tk.Label(dlg, text=self.t("sync_intro"), bg=self.bg, fg=self.fg, anchor="w", justify="left", wraplength=960).pack(fill="x", padx=12, pady=(10, 4))
        cols = ("sel", "title", "sys", "old", "new", "flag")
        tv = ttk.Treeview(dlg, columns=cols, show="headings", selectmode="browse")
        for c, w in (("sel", 40), ("title", 230), ("sys", 90), ("old", 260), ("new", 230), ("flag", 90)):
            tv.heading(c, text=self.t("sync_c_" + {"sel": "sel", "title": "title", "sys": "sys", "old": "old", "new": "new", "flag": "flag"}[c]))
            tv.column(c, width=w, anchor="w", stretch=(c in ("title", "old", "new")))
        tv.pack(side="top", fill="both", expand=True, padx=12, pady=4)
        status = tk.Label(dlg, text=self.t("sync_wait"), bg=self.bg, fg=self.fg, anchor="w", justify="left", wraplength=960)
        status.pack(fill="x", padx=12)
        row = tk.Frame(dlg, bg=self.bg)
        row.pack(fill="x", padx=12, pady=8)
        plan, checked = [], {}
        src_gifs = []
        lib = self.snap                        # bibliotheque de jeux (remplacee par la version complete une fois calculee en arriere-plan)

        def paint():
            tv.delete(*tv.get_children())
            for i, r in enumerate(plan):
                flag = " ".join(x for x in ((self.t("sync_flag_gif") if r["new_gif"] else ""), (self.t("sync_flag_game") if r["new_game"] else "")) if x)
                tv.insert("", "end", iid=str(i), values=("☑" if checked.get(i) else "☐", r["title"], r["system"], r["src"]["name"], r["new"], flag))
            sel = [plan[i] for i, v in checked.items() if v]
            mb = sum(r["size"] for r in sel) / 1048576
            fb = self.sd_root.free()
            free = None if fb is None else fb / 1048576
            ok = free is None or mb * 1.1 < free
            txt = self.t("sync_stats", n=len(plan), sel=len(sel), mb=f"{mb:.1f}", gifs=sum(1 for r in plan if r["new_gif"]), games=len({r["title"] for r in plan if r["new_game"]})) if plan else self.t("sync_none")
            if free is not None and plan:
                txt += " " + self.t("sync_space", free=f"{free:.0f}")
            status.configure(text=txt, fg=(self.fg if ok else "#C62828"))
            apply_btn.configure(state="normal" if (sel and ok) else "disabled")

        def toggle(ev):
            iid = tv.identify_row(ev.y)
            if iid:
                checked[int(iid)] = not checked.get(int(iid))
                paint()

        def all_set(v):
            for i in range(len(plan)):
                checked[i] = bool(v)
            paint()

        def apply():
            rows = [plan[i] for i, v in checked.items() if v]
            if not rows or not messagebox.askyesno(self.t("sync_title"), self.t("sync_confirm", n=len(rows)), icon="warning", default="no", parent=dlg):
                return
            res = gg.apply_sync(self.sd_root, lib, src_gifs, rows)
            dlg.destroy()
            msg = self.t("sync_done", n=res["done"])
            if res["failed"]:
                msg += "\n" + self.t("sync_failed", n=len(res["failed"]), first=f"{res['failed'][0][0]['title']} : {res['failed'][0][1]}")
            self._after_change(msg)

        def seen():
            gg.mark_seen(self.sd_root, lib, src_gifs)
            dlg.destroy()
            self._after_change(self.t("sync_seen_done"))

        tv.bind("<Button-1>", toggle)
        self._btn(row, "sync_all", lambda: all_set(True)).pack(side="left")
        self._btn(row, "sync_nothing", lambda: all_set(False)).pack(side="left", padx=6)
        self._btn(row, "sync_seen", seen).pack(side="left", padx=6)
        apply_btn = self._btn(row, "sync_apply", apply)
        apply_btn.pack(side="right")
        apply_btn.configure(state="disabled")
        self._btn(row, "close", dlg.destroy).pack(side="right", padx=6)

        def work():
            lb = self._lib_snap()
            src = gg.list_gif_files(self.sd_root)
            return src, gg.sync_plan(self.sd_root, lb, src, gg.read_exclusions_text(self.sd_root)), lb

        def done(res, err):
            nonlocal lib
            if not dlg.winfo_exists():
                return
            if err is not None:
                status.configure(text=self.t("err", why=str(err)), fg="#C62828")
                return
            src_gifs[:] = res[0]
            plan[:] = res[1]
            lib = res[2]
            checked.clear()
            checked.update({i: True for i in range(len(plan))})
            paint()

        self._bg(work, done)
        dlg.grab_set()

    def _entries_for(self, title):
        """Regles du fichier qui visent ce titre (ou tous les jeux) : [(titre affiche, GIF)]."""
        tn = gm.norm(title)
        out = []
        for e in gg.parse_entries(self.excl_text):
            if e["title"] == "*" or (len(tn) >= 3 and gm.norm(e["title"]) == tn):
                out.append((e["title"], e["gif"]))
        return out

    def _show_excl_info(self, title):
        ents = self._entries_for(title)
        if not ents:
            self._set_excl("")
            return
        parts = [self.t("excl_all_label") if g == "*" else (g if t != "*" else f"{g} {self.t('excl_glob_title')}") for t, g in ents]
        self._set_excl(self.t("excl_info", list=", ".join(parts)))

    def _exclude_gif(self):
        """Exclut pour ce jeu les GIFs joues COCHES."""
        todo = [r["gif"] for r in self._checked() if r["kind"] == "play"]
        if not todo or self.sel is None:
            return
        title = self.games[self.sel]["name"]
        def work():
            try:
                gg.create_folder(self.sd_root)          # le dossier /gifs_jeux est cree des qu'un fichier lie a cette fonction est ecrit
            except OSError:
                pass
            for g in todo:
                try:
                    gg.add_exclusion(self.sd_root, title, g["name"])
                except ValueError as e:
                    return str(e)
            return ""

        def finished(why):
            if why:
                messagebox.showerror(self.t("excl_title"), self.t("err", why=why), parent=self.win)
            self._after_change(self.t("excl_added", what=f"{title} ; " + ", ".join(g["name"] for g in todo)))
        self._do(work, finished)

    def _open_exclusions(self):
        if not self.sd_root:
            messagebox.showinfo(self.t("title"), self.t("no_sd"), parent=self.win)
            return
        dlg = tk.Toplevel(self.win)
        dlg.configure(bg=self.bg)
        dlg.title(self.t("excl_title"))
        dlg.geometry("760x420")
        dlg.transient(self.win.winfo_toplevel())
        tk.Label(dlg, text=self.t("excl_file"), bg=self.bg, fg=self.fg, anchor="w", wraplength=720, justify="left", font=("TkDefaultFont", 8)).pack(fill="x", padx=12, pady=(10, 4))
        tree = ttk.Treeview(dlg, columns=("gif",), show="tree headings", selectmode="browse")
        tree.heading("#0", text=self.t("excl_col_title"))
        tree.heading("gif", text=self.t("excl_col_gif"))
        tree.column("#0", width=300)
        tree.column("gif", width=380)
        tree.pack(fill="both", expand=True, padx=12, pady=4)
        msg = tk.Label(dlg, text="", bg=self.bg, fg="#1B7F3B", anchor="w", wraplength=720, justify="left")
        msg.pack(fill="x", padx=12)
        self._excl_dlg = dlg
        self._excl_tree = tree

        def fill():
            for i in tree.get_children():
                tree.delete(i)
            text = gg.read_exclusions_text(self.sd_root)
            ents = gg.parse_entries(text)
            for k, e in enumerate(ents):
                tree.insert("", "end", iid=str(k), text=(self.t("excl_glob_title") if e["title"] == "*" else e["title"]), values=(self.t("excl_all_label") if e["gif"] == "*" else e["gif"],))
            if not ents:
                msg.configure(text=self.t("excl_none"), fg=self.fg)
            self._excl_entries = ents

        def remove():
            s = tree.selection()
            if not s:
                return
            e = self._excl_entries[int(s[0])]
            gg.remove_exclusion(self.sd_root, e["title"], e["gif"])
            fill()
            self._after_change(self.t("excl_none") if not gg.parse_entries(gg.read_exclusions_text(self.sd_root)) else "")

        def send():
            text = gg.read_exclusions_text(self.sd_root)

            def work():
                tk_mod = getattr(self.app, "tkmod", None)
                dmd = tk_mod.find_dmd(_pref_get("dmd_ip")) if tk_mod else None
                if not dmd:
                    return None, ""
                ok, detail = gg.send_exclusions(dmd["ip"], text)
                return dmd["ip"], (ok, detail)

            def done(res, err):
                if err is not None:
                    msg.configure(text=self.t("excl_send_fail", detail=str(err)), fg="#C62828")
                elif res[0] is None:
                    msg.configure(text=self.t("excl_no_dmd"), fg="#C62828")
                elif res[1][0]:
                    msg.configure(text=self.t("excl_sent", ip=res[0]), fg="#1B7F3B")
                else:
                    msg.configure(text=self.t("excl_send_fail", detail=res[1][1]), fg="#C62828")
            self._bg(work, done)

        row = tk.Frame(dlg, bg=self.bg)
        row.pack(fill="x", padx=12, pady=8)
        self._btn(row, "excl_remove", remove).pack(side="left", padx=(0, 8))
        if self.sd_root.kind == "sd":
            self._btn(row, "excl_send", send).pack(side="left")
        else:
            tk.Label(row, text=self.t("excl_pi_note"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), anchor="w", justify="left", wraplength=480).pack(side="left")
        self._btn(row, "close", dlg.destroy).pack(side="right")
        fill()

    def _after_change(self, message):
        """Relit /gifs et le dossier dedie et refait la detection du systeme (un GIF a ete copie, retire, renomme ou vient d'arriver), puis reselectionne le jeu courant."""
        keep = self.sel
        self._token += 1
        token = self._token

        def work():
            return self._analyse(self.games)

        def done(res, err):
            if token != self._token or err is not None:
                return
            self.gifs, self.active, self.det, self.det_dir, self.cand, self.excl_text, nw = res
            self._set_mode_label(nw)
            self._refill()
            self.msg.configure(text=message)
            if keep is not None and str(keep) in self.tree.get_children():
                self.tree.selection_set(str(keep))
                self.tree.see(str(keep))
                self.msg.configure(text=message)

        self._bg(work, done)

    def _close(self):
        self._stop_anim()
        self._release_store()
        try:
            self.win.destroy()
        except Exception:   # noqa: BLE001
            pass


def open_window(app):
    """Mode 21 : ouvre la fenetre (une seule a la fois)."""
    w = getattr(app, "_game_gifs_win", None)
    try:
        if w is not None and w.win.winfo_exists():
            w.win.lift()
            return w
    except Exception:   # noqa: BLE001
        pass
    app._game_gifs_win = GameGifsWindow(app)
    return app._game_gifs_win
