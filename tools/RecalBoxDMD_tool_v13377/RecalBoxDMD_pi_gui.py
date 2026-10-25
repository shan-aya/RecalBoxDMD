# ============================================
# RecalBoxDMD_pi_gui.py -- fenetres des Modes 16 (preparer une carte Raspberry Pi) et 17 (installer / mettre a jour le Pi).
# v1 - 2026-10-19 - safe-modify - Creation. La logique est dans RecalBoxDMD_pi.py ; ici seulement les fenetres (meme principe
#      que RecalBoxDMD_theme_logos_gui.py : open_window(gui, kind)). Les mots de passe ne sont jamais memorises.
# ============================================
import os
import queue
import threading
import time
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import RecalBoxDMD_pi as pi
import RecalBoxDMD_prefs as prefs

T = {
    "fr": {
        "title16": "Préparer une carte Raspberry Pi",
        "title17": "Installer le DMD sur le Raspberry Pi",
        "intro16": "Prépare une carte SD complète pour un Raspberry Pi, à la place de Raspberry Pi Imager : système (Raspberry Pi OS Lite téléchargé et vérifié), Wi-Fi, utilisateur, SSH, logiciel du DMD et une partition MÉDIA (étiquette RBPIDMD ; FAT32 jusqu'à 30 Gio, exFAT au-delà) lisible sous Windows pour le contenu du DMD. Au premier démarrage le Pi s'installe tout seul, sans réseau ni PC. Carte de 16 Go minimum ; 32 Go laissent environ 23 Go au contenu.",
        "intro17": "Installe (ou met à jour) le logiciel du DMD sur un Raspberry Pi déjà démarré et connecté au réseau, puis copie le contenu de la carte SD du DMD "
                   "Un fichier ou un dossier déjà présent sur le Pi n'est remplacé qu'avec votre accord (un config.ini existant a sa propre question).",
        "disk": "Carte SD à écrire", "refresh": "Actualiser", "no_disk": "Aucune carte détectée — insérez-la puis « Actualiser ».",
        "image": "Image du système", "image_auto": "Automatique : Raspberry Pi OS Lite 32 bits (Bookworm), téléchargée et vérifiée",
        "image_file": "Ou un fichier .img / .img.xz :", "browse": "Parcourir…",
        "hostname": "Nom du Pi", "user": "Utilisateur", "pw": "Mot de passe", "pw2": "Confirmer",
        "ssid": "Wi-Fi (SSID)", "wifipw": "Mot de passe Wi-Fi", "country": "Pays Wi-Fi", "rbip": "IP de la Recalbox",
        "wifi_hint": "Laissez le Wi-Fi vide pour un branchement par câble Ethernet.",
        "payload_fetch": "Recherche de la dernière version du logiciel du DMD sur GitHub…", "payload_ok": "Logiciel du DMD : {v} ({p})", "payload_missing": "Logiciel du DMD introuvable : le Pi sera préparé sans lui (il faudra alors le copier avec le Mode 17).",
        "go16": "Écrire la carte", "go17": "Installer / mettre à jour", "close": "Fermer",
        "host": "Adresse du Pi", "content": "Contenu de la carte SD du DMD", "sync": "Copier aussi le contenu (systems, playlists, GIF…)",
        "no_content": "Installer uniquement le logiciel — copier le contenu plus tard (MODE 1 ou onglet AVANCÉ).",
        "need_pw": "Saisissez un mot de passe (6 caractères minimum), identique dans les deux champs.",
        "need_disk": "Choisissez la carte SD à écrire.", "bad_host": "Nom du Pi invalide (minuscules, chiffres et tirets).",
        "bad_user": "Utilisateur invalide (minuscules, chiffres, - et _).",
        "confirm16": "ATTENTION : tout le contenu de la carte suivante va être EFFACÉ.\n\n{disk}\n\nConfirmez-vous que c'est bien la carte du Raspberry Pi ?",
        "busy_close": "Une opération est en cours. Fermer la fenêtre ?", "content_stats": "{n} fichier(s), {size}", "setup_title": "Mode 1 — Raspberry Pi : carte et réglages", "setup_intro": "Insérez une carte SD vierge (elle sera entièrement effacée) et choisissez les réglages du Pi. Le Mode 1 prépare ensuite tout seul la carte complète : système, logiciel DMD et contenu, prête à être insérée dans le Pi.", "setup_ok": "Valider", "setup_confirm": "La carte « {disk} » sera ENTIÈREMENT EFFACÉE pour devenir la carte du Raspberry Pi. Continuer ?", "w_start": "Écriture du système sur la carte (en parallèle de la préparation du contenu)…", "w_wait": "Attente de la fin de l'écriture de la carte…", "w_copy": "Copie du contenu dans la partition média ({media})…", "w_ok": "Carte prête.", "keep_chk": "Conserver la partition média (réparer le système sans effacer les médias)", "keep_none": "Aucune partition média « RBPIDMD » valide détectée : la carte sera entièrement refaite.", "keep_ok": "Partition média détectée : {label}, {fs}, {size} Gio. Conservation possible.", "keep_checking": "Analyse de la carte…", "confirm16_keep": "Le SYSTÈME de la carte suivante va être réécrit.\nLa partition média (RBPIDMD) et son contenu seront CONSERVÉS.\n\n{disk}\n\nConfirmez-vous que c'est bien la carte du Raspberry Pi ?", "w_repair": "\n\nRéparation possible : lancez le Mode 16 (préparer une carte Raspberry Pi) pour refaire la carte, puis copiez le dossier de travail avec le panneau « Copier sur la carte SD » (cible Raspberry Pi). Attention : le Mode 16 efface TOUTE la carte, partition média comprise, sauf si vous cochez « Conserver la partition média ». Le contenu préparé par le Mode 1 reste dans le dossier de travail.", "w_repair_cause": "\n\nCause probable : Windows ou le lecteur USB a coupé la connexion au disque en pleine écriture (défaut intermittent).", "w_fail": "Échec de la préparation de la carte : {err}", "w_final": "La carte du Raspberry Pi est prête.\n\nPartition média {media} : contenu copié ({n} fichiers).\n\nÉjectez-la proprement, insérez-la dans le Pi et alimentez-le : il installe tout seul son logiciel (environ 5 minutes), puis affiche l'écran de configuration.", "wifi_scan": "Rechercher", "wifi_scanning": "Recherche des réseaux Wi-Fi…", "wifi_found": "{n} réseau(x) Wi-Fi 2,4 GHz trouvé(s) : choisissez dans la liste.", "wifi_none": "Aucun réseau trouvé (carte Wi-Fi du PC absente ou désactivée) : saisissez le nom.", "rb_scan": "Détecter", "rb_hint": "Plusieurs Recalbox : séparez les adresses par des virgules (4 maximum). Le DMD suit la première qui répond et bascule seul sur une autre si elle s'éteint.", "rb_scanning": "Recherche des Recalbox sur le réseau…", "rb_none": "Aucune Recalbox trouvée sur ce réseau : saisissez les adresses.", "rb_found": "{n} Recalbox trouvée(s) : {ips}. (?) = non confirmée par sa page web.", "layout": "Répartition prévue : système au démarrage {boot} Gio | média {media} Gio ({fs}) | système Linux {root} Gio (après agrandissement)", "layout_small": "Carte trop petite : il faut au moins 8 Go.", "target_label": "Cible :", "target_esp": "DMD ESP32 (carte SD)", "target_pi": "Raspberry Pi", "md_title": "Carte du Raspberry Pi", "md_prompt": "Choisissez la partition MÉDIA de la carte du Raspberry Pi (étiquette RBPIDMD, FAT32 ou exFAT). Elle est créée par le Mode 16 : sans carte préparée, utilisez le bouton « Préparer une carte… ».", "md_refresh": "Rafraîchir", "md_prepare": "Préparer une carte Raspberry Pi (Mode 16)…", "md_ok": "OK", "md_cancel": "Annuler", "md_none": "Sélectionnez une partition.", "md_err_label": "Ce n'est pas une partition média du Pi (étiquette « {label} » au lieu de RBPIDMD ; RECALBOXDMD est la carte de l'ESP32). Préparez la carte avec le Mode 16.", "md_err_fs": "Format {fs} non pris en charge : il faut FAT32 ou exFAT.", "opt_absent": "(facultatifs absents : {opt})", "map_title_idle": "Carte des dossiers du Pi", "map_btn": "Analyser le Pi", "map_before": "Carte du Pi — avant la copie", "map_after": "Carte du Pi — après la copie", "map_now": "Carte du Pi — état actuel", "ask_cfg": "Un config.ini existe déjà sur le Pi ET dans le dossier choisi.\n\nL'ÉCRASER ? Les réglages actuels du Pi (Wi-Fi, playlist, widgets, bouton…) seraient perdus.\n\nOui = écraser, Non = garder celui du Pi.", "ask_replace": "Ces éléments existent déjà sur le Pi :\n{list}\n\nRemplacer les fichiers existants qui diffèrent ?\n\nOui = remplacer, Non = ne copier que les fichiers manquants.", "only_sw": "Installer uniquement le logiciel (copier le contenu plus tard avec le MODE 1 ou l'onglet AVANCÉ)", "only_sw_note": "Seul le logiciel sera installé ; le contenu (images, GIF, playlists…) est à copier plus tard.", "wait": "Veuillez patienter…", "force": "Forcer la copie malgré les éléments manquants", "need_choice": "Choisissez un dossier de contenu, ou cochez « Installer uniquement le logiciel ».", "partial_hdr": "Il manque : {miss}", "empty_hdr": "Aucun des éléments attendus n'est dans ce dossier. Il manque : {miss}", "content_none": "Aucun fichier à copier dans ce dossier (systems, gifs, playlists, widgets).", "log_no_content": "Contenu : rien à copier (case décochée ou aucun dossier de contenu) — seul le logiciel a été installé.", "log_bad_folder": "Contenu : dossier introuvable : {p}", "dmd_install": "Installation du logiciel...", "dmd_copy": "Copie du contenu {pct}%", "step_find": "Recherche de l'image Raspberry Pi OS…", "step_dl": "Téléchargement de l'image… {pct} %", "step_dl_ok": "Image prête et vérifiée : {name}",
        "step_uac": "Autorisation administrateur demandée (fenêtre Windows) : acceptez pour écrire la carte.", "uac_refused": "Autorisation refusée : la carte n'a pas été touchée.",
        "step_write": "Écriture et vérification de la carte… {pct} %", "ok16": "Carte prête.\n\nLa partition média est {media} (étiquette RBPIDMD) : elle reçoit le contenu du DMD (images, GIF, playlists…), par exemple avec le Mode 1.\n\nÉjectez la carte, mettez-la dans le Raspberry Pi et alimentez-le : il s'installe tout seul (environ 3 à 4 minutes, avec deux redémarrages automatiques), puis le DMD affiche la page de configuration.",
        "fail16": "Échec : {err}\n\nLa carte peut être inutilisable : recommencez.",
        "step_find_pi": "Recherche du Pi sur le réseau…", "pi_found": "Pi trouvé : {host}", "pi_not_found": "Pi introuvable : {err}\n\nVérifiez qu'il est alimenté depuis 3 minutes et sur le même réseau que ce PC.",
        "pw_refused": "Mot de passe refusé par le Pi.", "step_upload": "Envoi du logiciel…", "step_install": "Installation (peut durer une minute)…",
        "step_reboot": "Redémarrage du Pi (paramètres d'affichage modifiés)…", "step_sync": "Copie du contenu… {done}/{total}", "sync_res": "Contenu : {sent} fichier(s) envoyé(s), {kept} conservé(s), {skipped} identique(s), {errors} erreur(s).",
        "ok17": "Terminé. Le DMD du Raspberry Pi tourne.\n\nPage de configuration : http://{host}/\n{first}", "first_yes": "Premier démarrage : le DMD attend la configuration (ouvrez la page ci-dessus).",
        "first_no": "Le DMD est déjà configuré.", "fail17": "Échec : {err}", "no_payload": "Logiciel du DMD introuvable : impossible d'installer.",
    },
    "en": {
        "title16": "Prepare a Raspberry Pi card",
        "title17": "Install the DMD on the Raspberry Pi",
        "intro16": "Prepares a complete SD card for a Raspberry Pi, replacing Raspberry Pi Imager: system (Raspberry Pi OS Lite, downloaded and verified), Wi-Fi, user, SSH, the DMD software and a MEDIA partition (label RBPIDMD; FAT32 up to 30 GiB, exFAT beyond) readable from Windows for the DMD content. On first boot the Pi installs itself, with no network and no PC. Card of 16 GB minimum; 32 GB leaves about 23 GB for the content.",
        "intro17": "Installs (or updates) the DMD software on a Raspberry Pi that is already running and on the network, then copies the DMD SD card content "
                   "A file or folder already on the Pi is only replaced with your agreement (an existing config.ini has its own question).",
        "disk": "SD card to write", "refresh": "Refresh", "no_disk": "No card detected — insert it, then « Refresh ».",
        "image": "System image", "image_auto": "Automatic: Raspberry Pi OS Lite 32-bit (Bookworm), downloaded and verified",
        "image_file": "Or an .img / .img.xz file:", "browse": "Browse…",
        "hostname": "Pi name", "user": "User", "pw": "Password", "pw2": "Confirm",
        "ssid": "Wi-Fi (SSID)", "wifipw": "Wi-Fi password", "country": "Wi-Fi country", "rbip": "Recalbox IP",
        "wifi_hint": "Leave Wi-Fi empty for an Ethernet cable connection.",
        "payload_fetch": "Looking for the latest DMD software on GitHub…", "payload_ok": "DMD software: {v} ({p})", "payload_missing": "DMD software not found: the Pi will be prepared without it (copy it later with Mode 17).",
        "go16": "Write the card", "go17": "Install / update", "close": "Close",
        "host": "Pi address", "content": "DMD SD card content", "sync": "Also copy the content (systems, playlists, GIFs…)",
        "no_content": "Install the software only — copy the content later (MODE 1 or ADVANCED tab).",
        "need_pw": "Enter a password (6 characters minimum), identical in both fields.",
        "need_disk": "Choose the SD card to write.", "bad_host": "Invalid Pi name (lowercase letters, digits and hyphens).",
        "bad_user": "Invalid user (lowercase letters, digits, - and _).",
        "confirm16": "WARNING: everything on the following card will be ERASED.\n\n{disk}\n\nDo you confirm this is the Raspberry Pi card?",
        "busy_close": "An operation is in progress. Close the window?", "content_stats": "{n} file(s), {size}", "setup_title": "Mode 1 — Raspberry Pi: card and settings", "setup_intro": "Insert a blank SD card (it will be completely erased) and choose the Pi settings. Mode 1 then prepares the whole card by itself: system, DMD software and content, ready to go into the Pi.", "setup_ok": "Confirm", "setup_confirm": "The card “{disk}” will be COMPLETELY ERASED to become the Raspberry Pi card. Continue?", "w_start": "Writing the system to the card (in parallel with preparing the content)…", "w_wait": "Waiting for the card write to finish…", "w_copy": "Copying the content to the media partition ({media})…", "w_ok": "Card ready.", "keep_chk": "Keep the media partition (repair the system without erasing the media)", "keep_none": "No valid “RBPIDMD” media partition found: the whole card will be rebuilt.", "keep_ok": "Media partition found: {label}, {fs}, {size} GiB. Keeping it is possible.", "keep_checking": "Analyzing the card…", "confirm16_keep": "The SYSTEM of the following card will be rewritten.\nThe media partition (RBPIDMD) and its content will be KEPT.\n\n{disk}\n\nDo you confirm this is the Raspberry Pi card?", "w_repair": "\n\nPossible repair: run Mode 16 (prepare a Raspberry Pi card) to rebuild the card, then copy the working folder with the “Copy to SD card” panel (Raspberry Pi target). Note: Mode 16 erases the WHOLE card, media partition included, unless you tick “Keep the media partition”. The content prepared by Mode 1 stays in the working folder.", "w_repair_cause": "\n\nProbable cause: Windows or the USB reader dropped the connection to the disk in the middle of the write (intermittent fault).", "w_fail": "Card preparation failed: {err}", "w_final": "The Raspberry Pi card is ready.\n\nMedia partition {media}: content copied ({n} files).\n\nEject it safely, insert it in the Pi and power it: it installs its software by itself (about 5 minutes), then shows the setup screen.", "wifi_scan": "Scan", "wifi_scanning": "Looking for Wi-Fi networks…", "wifi_found": "{n} 2.4 GHz Wi-Fi network(s) found: pick one from the list.", "wifi_none": "No network found (PC Wi-Fi adapter missing or off): type the name.", "rb_scan": "Detect", "rb_hint": "Several Recalbox: separate the addresses with commas (4 maximum). The DMD follows the first one that answers and switches by itself to another if it goes off.", "rb_scanning": "Looking for Recalbox on the network…", "rb_none": "No Recalbox found on this network: type the addresses.", "rb_found": "{n} Recalbox found: {ips}. (?) = not confirmed by its web page.", "layout": "Planned layout: boot system {boot} GiB | media {media} GiB ({fs}) | Linux system {root} GiB (after growth)", "layout_small": "Card too small: at least 8 GB needed.", "target_label": "Target:", "target_esp": "ESP32 DMD (SD card)", "target_pi": "Raspberry Pi", "md_title": "Raspberry Pi card", "md_prompt": "Choose the MEDIA partition of the Raspberry Pi card (label RBPIDMD, FAT32 or exFAT). It is created by Mode 16: without a prepared card, use the “Prepare a card…” button.", "md_refresh": "Refresh", "md_prepare": "Prepare a Raspberry Pi card (Mode 16)…", "md_ok": "OK", "md_cancel": "Cancel", "md_none": "Select a partition.", "md_err_label": "This is not a Pi media partition (label “{label}” instead of RBPIDMD; RECALBOXDMD is the ESP32 card). Prepare the card with Mode 16.", "md_err_fs": "Format {fs} not supported: FAT32 or exFAT required.", "opt_absent": "(optional items absent: {opt})", "map_title_idle": "Pi folders map", "map_btn": "Analyze the Pi", "map_before": "Pi map — before the copy", "map_after": "Pi map — after the copy", "map_now": "Pi map — current state", "ask_cfg": "A config.ini already exists on the Pi AND in the chosen folder.\n\nOVERWRITE it? The Pi's current settings (Wi-Fi, playlist, widgets, button…) would be lost.\n\nYes = overwrite, No = keep the Pi's.", "ask_replace": "These items already exist on the Pi:\n{list}\n\nReplace the existing files that differ?\n\nYes = replace, No = only copy the missing files.", "only_sw": "Install the software only (copy the content later with MODE 1 or the ADVANCED tab)", "only_sw_note": "Only the software will be installed; copy the content (images, GIFs, playlists…) later.", "wait": "Please wait…", "force": "Force the copy despite the missing items", "need_choice": "Choose a content folder, or tick “Install the software only”.", "partial_hdr": "Missing: {miss}", "empty_hdr": "None of the expected items is in this folder. Missing: {miss}", "content_none": "No file to copy in this folder (systems, gifs, playlists, widgets).", "log_no_content": "Content: nothing to copy (box unticked or no content folder) — only the software was installed.", "log_bad_folder": "Content: folder not found: {p}", "dmd_install": "Installing the software...", "dmd_copy": "Copying content {pct}%", "step_find": "Looking for the Raspberry Pi OS image…", "step_dl": "Downloading the image… {pct}%", "step_dl_ok": "Image ready and verified: {name}",
        "step_uac": "Administrator permission requested (Windows prompt): accept to write the card.", "uac_refused": "Permission refused: the card was not touched.",
        "step_write": "Writing and verifying the card… {pct}%", "ok16": "Card ready.\n\nThe media partition is {media} (label RBPIDMD): it receives the DMD content (images, GIFs, playlists…), for example with Mode 1.\n\nEject the card, put it in the Raspberry Pi and power it on: it installs itself (about 3 to 4 minutes, with two automatic restarts), then the DMD shows the configuration page.",
        "fail16": "Failed: {err}\n\nThe card may be unusable: start again.",
        "step_find_pi": "Looking for the Pi on the network…", "pi_found": "Pi found: {host}", "pi_not_found": "Pi not found: {err}\n\nCheck that it has been powered for 3 minutes and is on the same network as this PC.",
        "pw_refused": "Password refused by the Pi.", "step_upload": "Sending the software…", "step_install": "Installing (may take a minute)…",
        "step_reboot": "Restarting the Pi (display settings changed)…", "step_sync": "Copying content… {done}/{total}", "sync_res": "Content: {sent} file(s) sent, {kept} kept, {skipped} identical, {errors} error(s).",
        "ok17": "Done. The Raspberry Pi DMD is running.\n\nConfiguration page: http://{host}/\n{first}", "first_yes": "First boot: the DMD is waiting for configuration (open the page above).",
        "first_no": "The DMD is already configured.", "fail17": "Failed: {err}", "no_payload": "DMD software not found: cannot install.",
    },
    "es": {
        "title16": "Preparar una tarjeta Raspberry Pi",
        "title17": "Instalar el DMD en la Raspberry Pi",
        "intro16": "Prepara una tarjeta SD completa para una Raspberry Pi, en lugar de Raspberry Pi Imager: sistema (Raspberry Pi OS Lite descargado y verificado), Wi-Fi, usuario, SSH, software del DMD y una partición de MEDIOS (etiqueta RBPIDMD; FAT32 hasta 30 GiB, exFAT por encima) legible desde Windows para el contenido del DMD. En el primer arranque la Pi se instala sola, sin red ni PC. Tarjeta de 16 GB como mínimo; 32 GB dejan unos 23 GB para el contenido.",
        "intro17": "Instala (o actualiza) el software del DMD en una Raspberry Pi ya arrancada y conectada a la red, y copia el contenido de la tarjeta SD del DMD "
                   "Un archivo o carpeta que ya está en la Pi solo se reemplaza con su acuerdo (un config.ini existente tiene su propia pregunta).",
        "disk": "Tarjeta SD a escribir", "refresh": "Actualizar", "no_disk": "Ninguna tarjeta detectada — insértela y pulse « Actualizar ».",
        "image": "Imagen del sistema", "image_auto": "Automática: Raspberry Pi OS Lite 32 bits (Bookworm), descargada y verificada",
        "image_file": "O un archivo .img / .img.xz:", "browse": "Examinar…",
        "hostname": "Nombre de la Pi", "user": "Usuario", "pw": "Contraseña", "pw2": "Confirmar",
        "ssid": "Wi-Fi (SSID)", "wifipw": "Contraseña Wi-Fi", "country": "País Wi-Fi", "rbip": "IP de la Recalbox",
        "wifi_hint": "Deje el Wi-Fi vacío para una conexión por cable Ethernet.",
        "payload_fetch": "Buscando la última versión del software del DMD en GitHub…", "payload_ok": "Software del DMD: {v} ({p})", "payload_missing": "Software del DMD no encontrado: la Pi se preparará sin él (cópielo después con el Modo 17).",
        "go16": "Escribir la tarjeta", "go17": "Instalar / actualizar", "close": "Cerrar",
        "host": "Dirección de la Pi", "content": "Contenido de la tarjeta SD del DMD", "sync": "Copiar también el contenido (systems, playlists, GIF…)",
        "no_content": "Instalar solo el software — copiar el contenido más tarde (MODO 1 o pestaña AVANZADO).",
        "need_pw": "Escriba una contraseña (6 caracteres mínimo), idéntica en los dos campos.",
        "need_disk": "Elija la tarjeta SD a escribir.", "bad_host": "Nombre de la Pi no válido (minúsculas, cifras y guiones).",
        "bad_user": "Usuario no válido (minúsculas, cifras, - y _).",
        "confirm16": "ATENCIÓN: todo el contenido de la siguiente tarjeta se BORRARÁ.\n\n{disk}\n\n¿Confirma que es la tarjeta de la Raspberry Pi?",
        "busy_close": "Hay una operación en curso. ¿Cerrar la ventana?", "content_stats": "{n} archivo(s), {size}", "setup_title": "Modo 1 — Raspberry Pi: tarjeta y ajustes", "setup_intro": "Inserte una tarjeta SD vacía (se borrará por completo) y elija los ajustes de la Pi. El Modo 1 prepara luego toda la tarjeta: sistema, software DMD y contenido, lista para la Pi.", "setup_ok": "Validar", "setup_confirm": "La tarjeta «{disk}» se BORRARÁ POR COMPLETO para convertirse en la tarjeta de la Raspberry Pi. ¿Continuar?", "w_start": "Escribiendo el sistema en la tarjeta (en paralelo a la preparación del contenido)…", "w_wait": "Esperando el final de la escritura de la tarjeta…", "w_copy": "Copiando el contenido a la partición de medios ({media})…", "w_ok": "Tarjeta lista.", "keep_chk": "Conservar la partición de medios (reparar el sistema sin borrar los medios)", "keep_none": "No se encontró ninguna partición de medios «RBPIDMD» válida: se rehará toda la tarjeta.", "keep_ok": "Partición de medios encontrada: {label}, {fs}, {size} GiB. Se puede conservar.", "keep_checking": "Analizando la tarjeta…", "confirm16_keep": "El SISTEMA de la siguiente tarjeta se reescribirá.\nLa partición de medios (RBPIDMD) y su contenido se CONSERVARÁN.\n\n{disk}\n\n¿Confirma que es la tarjeta de la Raspberry Pi?", "w_repair": "\n\nReparación posible: ejecute el Modo 16 (preparar una tarjeta Raspberry Pi) para rehacer la tarjeta y luego copie la carpeta de trabajo con el panel «Copiar a la tarjeta SD» (destino Raspberry Pi). Atención: el Modo 16 borra TODA la tarjeta, partición de medios incluida, salvo que marque «Conservar la partición de medios». El contenido preparado por el Modo 1 sigue en la carpeta de trabajo.", "w_repair_cause": "\n\nCausa probable: Windows o el lector USB cortó la conexión con el disco en plena escritura (fallo intermitente).", "w_fail": "Falló la preparación de la tarjeta: {err}", "w_final": "La tarjeta de la Raspberry Pi está lista.\n\nPartición de medios {media}: contenido copiado ({n} archivos).\n\nExpúlsela con seguridad, insértela en la Pi y enciéndala: instala su software sola (unos 5 minutos) y luego muestra la pantalla de configuración.", "wifi_scan": "Buscar", "wifi_scanning": "Buscando redes Wi-Fi…", "wifi_found": "{n} red(es) Wi-Fi de 2,4 GHz encontrada(s): elija de la lista.", "wifi_none": "Ninguna red encontrada (adaptador Wi-Fi del PC ausente o apagado): escriba el nombre.", "rb_scan": "Detectar", "rb_hint": "Varias Recalbox: separe las direcciones con comas (4 como máximo). El DMD sigue a la primera que responde y cambia solo a otra si se apaga.", "rb_scanning": "Buscando Recalbox en la red…", "rb_none": "Ninguna Recalbox encontrada en esta red: escriba las direcciones.", "rb_found": "{n} Recalbox encontrada(s): {ips}. (?) = no confirmada por su página web.", "layout": "Distribución prevista: sistema de arranque {boot} GiB | medios {media} GiB ({fs}) | sistema Linux {root} GiB (tras ampliación)", "layout_small": "Tarjeta demasiado pequeña: se necesitan al menos 8 GB.", "target_label": "Destino:", "target_esp": "DMD ESP32 (tarjeta SD)", "target_pi": "Raspberry Pi", "md_title": "Tarjeta de la Raspberry Pi", "md_prompt": "Elija la partición de MEDIOS de la tarjeta de la Raspberry Pi (etiqueta RBPIDMD, FAT32 o exFAT). La crea el Modo 16: sin tarjeta preparada, use el botón « Preparar una tarjeta… ».", "md_refresh": "Actualizar", "md_prepare": "Preparar una tarjeta Raspberry Pi (Modo 16)…", "md_ok": "OK", "md_cancel": "Cancelar", "md_none": "Seleccione una partición.", "md_err_label": "No es una partición de medios de la Pi (etiqueta « {label} » en lugar de RBPIDMD; RECALBOXDMD es la tarjeta del ESP32). Prepare la tarjeta con el Modo 16.", "md_err_fs": "Formato {fs} no admitido: se necesita FAT32 o exFAT.", "opt_absent": "(opcionales ausentes: {opt})", "map_title_idle": "Mapa de carpetas de la Pi", "map_btn": "Analizar la Pi", "map_before": "Mapa de la Pi — antes de la copia", "map_after": "Mapa de la Pi — después de la copia", "map_now": "Mapa de la Pi — estado actual", "ask_cfg": "Ya existe un config.ini en la Pi Y en la carpeta elegida.\n\n¿SOBRESCRIBIRLO? Los ajustes actuales de la Pi (Wi-Fi, lista, widgets, botón…) se perderían.\n\nSí = sobrescribir, No = conservar el de la Pi.", "ask_replace": "Estos elementos ya existen en la Pi:\n{list}\n\n¿Reemplazar los archivos existentes que difieran?\n\nSí = reemplazar, No = copiar solo los archivos que faltan.", "only_sw": "Instalar solo el software (copiar el contenido más tarde con el MODO 1 o la pestaña AVANZADO)", "only_sw_note": "Solo se instalará el software; copie el contenido (imágenes, GIF, listas…) más tarde.", "wait": "Espere, por favor…", "force": "Forzar la copia a pesar de los elementos que faltan", "need_choice": "Elija una carpeta de contenido, o marque « Instalar solo el software ».", "partial_hdr": "Falta: {miss}", "empty_hdr": "Ninguno de los elementos esperados está en esta carpeta. Falta: {miss}", "content_none": "Ningún archivo que copiar en esta carpeta (systems, gifs, playlists, widgets).", "log_no_content": "Contenido: nada que copiar (casilla desmarcada o sin carpeta de contenido) — solo se instaló el software.", "log_bad_folder": "Contenido: carpeta no encontrada: {p}", "dmd_install": "Instalando el software...", "dmd_copy": "Copiando contenido {pct}%", "step_find": "Buscando la imagen de Raspberry Pi OS…", "step_dl": "Descargando la imagen… {pct} %", "step_dl_ok": "Imagen lista y verificada: {name}",
        "step_uac": "Permiso de administrador solicitado (ventana de Windows): acepte para escribir la tarjeta.", "uac_refused": "Permiso denegado: no se ha tocado la tarjeta.",
        "step_write": "Escribiendo y verificando la tarjeta… {pct} %", "ok16": "Tarjeta lista.\n\nLa partición de medios es {media} (etiqueta RBPIDMD): recibe el contenido del DMD (imágenes, GIF, listas…), por ejemplo con el Modo 1.\n\nExpulse la tarjeta, póngala en la Raspberry Pi y enciéndala: se instala sola (unos 3 a 4 minutos, con dos reinicios automáticos), y después el DMD muestra la página de configuración.",
        "fail16": "Error: {err}\n\nLa tarjeta puede estar inservible: empiece de nuevo.",
        "step_find_pi": "Buscando la Pi en la red…", "pi_found": "Pi encontrada: {host}", "pi_not_found": "Pi no encontrada: {err}\n\nCompruebe que lleva 3 minutos encendida y está en la misma red que este PC.",
        "pw_refused": "Contraseña rechazada por la Pi.", "step_upload": "Enviando el software…", "step_install": "Instalando (puede tardar un minuto)…",
        "step_reboot": "Reiniciando la Pi (ajustes de pantalla modificados)…", "step_sync": "Copiando el contenido… {done}/{total}", "sync_res": "Contenido: {sent} archivo(s) enviado(s), {kept} conservado(s), {skipped} idéntico(s), {errors} error(es).",
        "ok17": "Terminado. El DMD de la Raspberry Pi funciona.\n\nPágina de configuración: http://{host}/\n{first}", "first_yes": "Primer arranque: el DMD espera la configuración (abra la página anterior).",
        "first_no": "El DMD ya está configurado.", "fail17": "Error: {err}", "no_payload": "Software del DMD no encontrado: no se puede instalar.",
    },
}

_LAST_PI = {}   # (hote, utilisateur, mot de passe) du dernier Mode 16 de cette session : memoire seulement, jamais ecrit sur disque
COUNTRIES = ["FR", "BE", "CH", "CA", "ES", "DE", "IT", "GB", "US", "PT", "NL", "LU"]
LOCALE = {"fr": ("fr", "Europe/Paris"), "en": ("gb", "Europe/London"), "es": ("es", "Europe/Madrid")}


def open_window(gui, kind: str):
    """Point d'entree appele par le GUI principal : kind = "prepare" (Mode 16) ou "install" (Mode 17)."""
    lang = "fr"
    try:
        lang = gui.lang_var.get()
    except Exception:
        pass
    tr = T.get(lang, T["fr"])
    return PiWindow(gui, tr, lang if lang in T else "fr", kind)


class PiWindow:
    def __init__(self, gui, tr, lang, kind):
        self.gui, self.tr, self.lang, self.kind = gui, tr, lang, kind
        self.q = queue.Queue()
        self.busy = False
        self.closed = False
        self.disks = []
        self.payload = pi.find_payload(prefs.get("pi_payload_dir") or "")
        c = {}
        try:
            c = gui._theme_colors()
        except Exception:
            pass
        self.bg = c.get("bg_main", "#F3F3F3")
        self.fg = c.get("fg_text", "#000000")
        self.fg_light = c.get("fg_text_light", "#555555")
        self.bg_action = c.get("bg_button_action", "#FFD400")
        self.bg_normal = c.get("bg_button_normal", "#FFFFFF")
        w = self.win = tk.Toplevel(gui.root)
        self.result = None
        w.title(tr["setup_title"] if kind == "setup" else tr["title16"] if kind == "prepare" else tr["title17"])
        w.configure(bg=self.bg)
        w.transient(gui.root)
        w.resizable(False, False)
        body = tk.Frame(w, bg=self.bg, padx=16, pady=12)
        body.pack(fill="both", expand=True)
        tk.Label(body, text=tr["setup_intro"] if kind == "setup" else tr["intro16"] if kind == "prepare" else tr["intro17"], bg=self.bg, fg=self.fg, wraplength=620, justify="left",
                 anchor="w").pack(anchor="w", pady=(0, 8))
        form = tk.Frame(body, bg=self.bg)
        form.pack(fill="x")
        self.row = 0
        if kind in ("prepare", "setup"):
            self._build_prepare(form)
        else:
            self._build_install(form)
        self.lbl_payload = tk.Label(body, bg=self.bg, fg=self.fg, wraplength=620, justify="left", anchor="w")
        self.lbl_payload.pack(anchor="w", pady=(6, 2))
        if self.payload:
            self.lbl_payload.config(text=tr["payload_ok"].format(v=pi.payload_version(self.payload) or "?", p=self.payload))
        else:
            self.lbl_payload.config(text=tr["payload_missing"] if kind != "install" else tr["no_payload"], fg=self._warn())
        if not (prefs.get("pi_payload_dir") or os.environ.get("RECALBOXDMD_PI_PAYLOAD")):
            self.win.after(300, self._start_fetch_payload)     # charge utile la plus recente depuis GitHub (releases pi-v...)
        bar = tk.Frame(body, bg=self.bg)
        bar.pack(fill="x", pady=(6, 4))
        self.btn_go = tk.Button(bar, text=tr["setup_ok"] if kind == "setup" else tr["go16"] if kind == "prepare" else tr["go17"], bg=self.bg_action, fg="#000000", bd=2, relief="solid",
                                padx=12, pady=5, font=("TkDefaultFont", 10, "bold"), command=self._go)
        self.btn_go.pack(side="left")
        if kind == "install":
            self._update_start()
        tk.Button(bar, text=tr["close"], bg=self.bg_normal, fg=self.fg, bd=2, relief="solid", padx=12, pady=5,
                  font=("TkDefaultFont", 10, "bold"), command=self._close).pack(side="left", padx=(8, 0))
        self.pbar = ttk.Progressbar(body, orient="horizontal", mode="determinate", maximum=100)
        self.pbar.pack(fill="x", pady=(4, 2))
        self.status = tk.Label(body, bg=self.bg, fg=self.fg, wraplength=620, justify="left", anchor="w", text="")
        self.status.pack(anchor="w", fill="x")
        lb = tk.Frame(body, bg=self.bg)
        lb.pack(fill="x", pady=(4, 0))
        self.log = tk.Text(lb, height=6, width=84, state="disabled", bg="#101010", fg="#d0d0d0")
        sb = ttk.Scrollbar(lb, orient="vertical", command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        self.log.pack(side="left", fill="x", expand=True)
        sb.pack(side="left", fill="y")
        w.protocol("WM_DELETE_WINDOW", self._close)
        w.after(100, self._drain)
        w.update_idletasks()
        try:
            gui._center_toplevel(w)
        except Exception:
            pass
        if kind in ("prepare", "setup"):
            self._refresh_disks()

    # ----------------------------------------------------------------------------------------------- formulaires
    def _warn(self):
        """Rouge lisible sur fond clair comme sur fond sombre."""
        try:
            h = self.bg.lstrip("#")
            lum = (int(h[0:2], 16) * 299 + int(h[2:4], 16) * 587 + int(h[4:6], 16) * 114) / 1000
        except Exception:
            lum = 255
        return "#B00020" if lum > 128 else "#FF8A80"

    def _label(self, parent, text):
        tk.Label(parent, text=text, bg=self.bg, fg=self.fg, anchor="w").grid(row=self.row, column=0, sticky="w", pady=2, padx=(0, 10))

    def _entry(self, parent, var, width=30, show=""):
        e = tk.Entry(parent, textvariable=var, width=width, show=show)
        e.grid(row=self.row, column=1, sticky="w", pady=2)
        return e

    def _build_prepare(self, f):
        tr = self.tr
        self._label(f, tr["disk"])
        self.disk_var = tk.StringVar()
        self.cb_disk = ttk.Combobox(f, textvariable=self.disk_var, state="readonly", width=52)
        self.cb_disk.grid(row=self.row, column=1, sticky="w", pady=2)
        tk.Button(f, text=tr["refresh"], command=self._refresh_disks, bd=2, relief="solid", bg=self.bg_normal, fg=self.fg).grid(row=self.row, column=2, padx=6)
        self.cb_disk.bind("<<ComboboxSelected>>", lambda e: self._update_layout())
        self.row += 1
        self.lbl_layout = tk.Label(f, text="", bg=self.bg, fg=self.fg_light, anchor="w", justify="left", wraplength=560)
        self.lbl_layout.grid(row=self.row, column=0, columnspan=3, sticky="w", pady=(0, 4))
        self.row += 1
        self.keep_var = tk.BooleanVar(value=False)
        self.keep_info = None
        if self.kind == "prepare":
            self.chk_keep = tk.Checkbutton(f, text=tr["keep_chk"], variable=self.keep_var, state="disabled", bg=self.bg, fg=self.fg, activebackground=self.bg,
                                           selectcolor=self.bg, anchor="w", wraplength=560, justify="left")
            self.chk_keep.grid(row=self.row, column=0, columnspan=3, sticky="w")
            self.row += 1
            self.lbl_keep = tk.Label(f, text="", bg=self.bg, fg=self.fg_light, anchor="w", justify="left", wraplength=560)
            self.lbl_keep.grid(row=self.row, column=0, columnspan=3, sticky="w", pady=(0, 4))
            self.row += 1
        self._label(f, tr["image"])
        tk.Label(f, text=tr["image_auto"], bg=self.bg, fg=self.fg, anchor="w", wraplength=440, justify="left").grid(row=self.row, column=1, columnspan=2, sticky="w")
        self.row += 1
        self._label(f, tr["image_file"])
        self.img_var = tk.StringVar()
        self._entry(f, self.img_var, 44)
        tk.Button(f, text=tr["browse"], command=self._browse_img, bd=2, relief="solid", bg=self.bg_normal, fg=self.fg).grid(row=self.row, column=2, padx=6)
        self.row += 1
        self.host_var = tk.StringVar(value=pi.default_hostname(prefs.get("pi_hostname") or ""))
        self.user_var = tk.StringVar(value=prefs.get("pi_user") or pi.DEFAULT_USER)
        self._pair(f, (tr["hostname"], self.host_var, 16, ""), (tr["user"], self.user_var, 14, ""))
        self.pw_var, self.pw2_var = tk.StringVar(), tk.StringVar()
        self._pair(f, (tr["pw"], self.pw_var, 16, "*"), (tr["pw2"], self.pw2_var, 14, "*"))
        self.ssid_var = tk.StringVar(value=pi.current_wifi_ssid())
        self.wpw_var = tk.StringVar()
        row = tk.Frame(f, bg=self.bg)
        row.grid(row=self.row, column=0, columnspan=3, sticky="w", pady=2)
        tk.Label(row, text=tr["ssid"], bg=self.bg, fg=self.fg, width=14, anchor="w").pack(side="left", padx=(0, 6))
        self.cb_ssid = ttk.Combobox(row, textvariable=self.ssid_var, values=[self.ssid_var.get()] if self.ssid_var.get() else [], width=18)
        self.cb_ssid.pack(side="left")
        tk.Button(row, text=tr["wifi_scan"], command=self._scan_wifi, bd=2, relief="solid", bg=self.bg_normal, fg=self.fg).pack(side="left", padx=(6, 18))
        tk.Label(row, text=tr["wifipw"], bg=self.bg, fg=self.fg, anchor="w").pack(side="left", padx=(0, 6))
        tk.Entry(row, textvariable=self.wpw_var, width=14, show="*").pack(side="left")
        self.row += 1
        self.country_var = tk.StringVar(value=prefs.get("pi_wifi_country") or "FR")
        self.rb_var = tk.StringVar(value=prefs.get("recalbox_ip") or "")
        row = tk.Frame(f, bg=self.bg)
        row.grid(row=self.row, column=0, columnspan=3, sticky="w", pady=2)
        tk.Label(row, text=tr["country"], bg=self.bg, fg=self.fg, width=14, anchor="w").pack(side="left")
        ttk.Combobox(row, textvariable=self.country_var, values=COUNTRIES, width=5).pack(side="left", padx=(0, 18))
        tk.Label(row, text=tr["rbip"], bg=self.bg, fg=self.fg, anchor="w").pack(side="left", padx=(0, 6))
        tk.Entry(row, textvariable=self.rb_var, width=26).pack(side="left")
        tk.Button(row, text=tr["rb_scan"], command=self._scan_recalbox, bd=2, relief="solid", bg=self.bg_normal, fg=self.fg).pack(side="left", padx=(6, 0))
        self.row += 1
        self.lbl_scan = tk.Label(f, text=tr["rb_hint"], bg=self.bg, fg=self.fg_light, anchor="w", justify="left", wraplength=560)
        self.lbl_scan.grid(row=self.row, column=0, columnspan=3, sticky="w")
        self.row += 1
        tk.Label(f, text=tr["wifi_hint"], bg=self.bg, fg=self.fg_light, anchor="w").grid(row=self.row, column=0, columnspan=3, sticky="w")
        self.row += 1

    def _scan_wifi(self):
        self.lbl_scan.config(text=self.tr["wifi_scanning"])

        def work():
            nets = pi.wifi_networks()
            self.win.after(0, lambda: done(nets))

        def done(nets):
            self.cb_ssid.config(values=nets)
            self.lbl_scan.config(text=self.tr["wifi_found"].format(n=len(nets)) if nets else self.tr["wifi_none"])
        threading.Thread(target=work, daemon=True).start()

    def _scan_recalbox(self):
        self.lbl_scan.config(text=self.tr["rb_scanning"])

        def work():
            res = pi.scan_recalbox_lan()
            self.win.after(0, lambda: done(res))

        def done(res):
            if not res:
                self.lbl_scan.config(text=self.tr["rb_none"])
                return
            self.rb_var.set(", ".join(ip for ip, _ in res))
            self.lbl_scan.config(text=self.tr["rb_found"].format(n=len(res), ips=", ".join(ip if ok else ip + " (?)" for ip, ok in res)))
        threading.Thread(target=work, daemon=True).start()

    def _pair(self, f, a, b):
        """Deux champs sur la meme ligne : (libelle, variable, largeur, masque) x 2."""
        row = tk.Frame(f, bg=self.bg)
        row.grid(row=self.row, column=0, columnspan=3, sticky="w", pady=2)
        for i, (lab, var, wd, show) in enumerate((a, b)):
            tk.Label(row, text=lab, bg=self.bg, fg=self.fg, width=14 if i == 0 else 0, anchor="w").pack(side="left", padx=(0 if i == 0 else 18, 6))
            tk.Entry(row, textvariable=var, width=wd, show=show).pack(side="left")
        self.row += 1
    def _build_install(self, f):
        tr = self.tr
        lp = _LAST_PI
        self.host_var = tk.StringVar(value=(lp.get("host") + ".local") if lp.get("host") else pi.default_hostname(prefs.get("pi_host") or "") + ("" if "." in pi.default_hostname(prefs.get("pi_host") or "") else ".local"))
        self._label(f, tr["host"])
        self._entry(f, self.host_var, 30)
        self.row += 1
        self.user_var = tk.StringVar(value=lp.get("user") or prefs.get("pi_user") or pi.DEFAULT_USER)
        self._label(f, tr["user"])
        self._entry(f, self.user_var, 20)
        self.row += 1
        self.pw_var = tk.StringVar(value=lp.get("pw", ""))
        self._label(f, tr["pw"])
        e = self._entry(f, self.pw_var, 24, "*")
        e.focus_set()
        self.row += 1
        # contenu : par defaut le dossier de travail du toolkit s'il contient une carte preparee ; « Parcourir » : un dossier de sauvegarde de carte SD, par exemple
        try:
            work = Path(self.gui.sd_dir)
        except Exception:
            work = None
        self.sd_root = str(work) if work and work.is_dir() and (work / "systems").is_dir() else None
        self.only_sw = tk.BooleanVar(value=not self.sd_root)
        self.force_var = tk.BooleanVar(value=False)
        self.scan_state = "none"      # none / scanning / complete / partial / empty
        self._scan_gen = 0
        self._blink_on = True
        tk.Checkbutton(f, text=tr["only_sw"], variable=self.only_sw, command=self._on_only_sw, bg=self.bg, fg=self.fg,
                       activebackground=self.bg, selectcolor=self.bg, anchor="w", wraplength=520, justify="left").grid(row=self.row, column=0, columnspan=3, sticky="w", pady=(6, 2))
        self.row += 1
        self._label(f, tr["content"])
        self.content_var = tk.StringVar(value=self.sd_root or "")
        self.ce = self._entry(f, self.content_var, 44)
        self.ce.bind("<Return>", lambda e: self._refresh_content())
        self.ce.bind("<FocusOut>", lambda e: self._refresh_content())
        self.btn_browse = tk.Button(f, text=tr["browse"], command=self._browse_content, bd=2, relief="solid", bg=self.bg_normal, fg=self.fg)
        self.btn_browse.grid(row=self.row, column=2, padx=6)
        self.row += 1
        self.lbl_stats = tk.Label(f, text="", bg=self.bg, fg=self.fg_light, anchor="w", justify="left", wraplength=560)
        self.lbl_stats.grid(row=self.row, column=0, columnspan=3, sticky="w")
        self.row += 1
        self.chk_force = tk.Checkbutton(f, text=tr["force"], variable=self.force_var, command=self._update_start, bg=self.bg, fg=self.fg,
                                        activebackground=self.bg, selectcolor=self.bg, anchor="w")
        self.chk_force.grid(row=self.row, column=0, columnspan=3, sticky="w")
        self.chk_force.grid_remove()
        self.row += 1
        self._last_root = None
        # carte des dossiers du Pi : present = vert, absent = rouge, facultatif absent = orange (avant puis apres la copie)
        self.map_title = tk.Label(f, text=tr["map_title_idle"], bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 10, "bold"))
        self.map_title.grid(row=self.row, column=0, columnspan=2, sticky="w", pady=(8, 2))
        tk.Button(f, text=tr["map_btn"], command=self._analyze_pi, bd=2, relief="solid", bg=self.bg_normal, fg=self.fg).grid(row=self.row, column=2, padx=6, pady=(8, 2))
        self.row += 1
        mf = tk.Frame(f, bg=self.bg)
        mf.grid(row=self.row, column=0, columnspan=3, sticky="w")
        self.map_labels = {}
        for i, (lab, _rel, _opt) in enumerate(pi.EXPECTED_CONTENT):
            ml = tk.Label(mf, text="\u25CB " + lab, bg=self.bg, fg=self.fg_light, anchor="w", width=27)
            ml.grid(row=i // 3, column=i % 3, sticky="w", padx=(0, 6))
            self.map_labels[lab] = ml
        self.row += 1
        self._on_only_sw()
        self._blink()

    def _show_map(self, res, phase):
        title = {"before": self.tr["map_before"], "after": self.tr["map_after"], "scan": self.tr["map_now"]}.get(phase, self.tr["map_title_idle"])
        self.map_title.config(text=title)
        for lab, _rel, opt in pi.EXPECTED_CONTENT:
            ok = res["flags"].get(lab)
            col = "#43A047" if ok else ("#FB8C00" if opt else "#E53935")
            self.map_labels[lab].config(text=("\u25CF " if ok else "\u25CB ") + lab, fg=col)

    def _analyze_pi(self):
        if self.busy:
            return
        host, user, pw = self.host_var.get().strip(), self.user_var.get().strip(), self.pw_var.get()
        if not pw:
            messagebox.showwarning(self.win.title(), self.tr["need_pw"], parent=self.win)
            return
        self._post("busy", True)

        def work():
            c = None
            try:
                self._post("status", self.tr["step_find_pi"])
                c, addr = pi.find_pi(host, user, pw, lambda m: self._post("log", m))
                res = pi.remote_check(c, "/opt/recalboxdmd/sd")
                self._post("pimap", res, "scan")
                self._post("log", "Carte du Pi : present = " + ", ".join(res["found"]) + " | absent = " + ", ".join(res["missing"] + res["optional_missing"]))
                self._post("status", self.tr["map_now"])
            except PermissionError:
                self._post("error", self.tr["pw_refused"])
            except Exception as e:   # noqa: BLE001
                self._post("error", self.tr["pi_not_found"].format(err=e))
            finally:
                if c is not None:
                    try:
                        c.close()
                    except Exception:   # noqa: BLE001
                        pass
                self._post("busy", False)
        threading.Thread(target=work, daemon=True).start()

    def _ask(self, text):
        """Question oui / non posee depuis le fil de travail (la fenetre s'affiche dans le fil principal)."""
        ev = threading.Event()
        box = {"v": False}
        self._post("ask", text, ev, box)
        ev.wait()
        return box["v"]

    # --- etat du contenu ---------------------------------------------------------------------------------------------------------------
    def _on_only_sw(self):
        sw = self.only_sw.get()
        st = "disabled" if sw else "normal"
        self.ce.config(state=st)
        self.btn_browse.config(state=st)
        if sw:
            self.lbl_stats.config(text=self.tr["only_sw_note"])
            self.chk_force.grid_remove()
        else:
            self._last_root = None
            self._refresh_content()
        self._update_start()

    def _refresh_content(self):
        if self.only_sw.get():
            return
        root = self.content_var.get().strip().strip('"')
        if root == self._last_root:
            return
        self._last_root = root
        self._scan_gen += 1
        gen = self._scan_gen
        if not root:
            self.scan_state = "none"
            self.lbl_stats.config(text=self.tr["need_choice"])
            self.chk_force.grid_remove()
            self._update_start()
            return
        self.scan_state = "scanning"
        self.force_var.set(False)
        self.chk_force.grid_remove()
        self.lbl_stats.config(text=self.tr["wait"])
        self._update_start()
        threading.Thread(target=self._scan_worker, args=(root, gen), daemon=True).start()

    def _scan_worker(self, root, gen):
        try:
            if not Path(root).is_dir():
                self.q.put(("scan", gen, "empty", self.tr["log_bad_folder"].format(p=root)))
                return
            chk = pi.check_content(Path(root))
            plan = pi.content_plan(Path(root))
            n, size = len(plan), sum(s for _r, _f, s in plan)
            miss = ", ".join(chk["missing"])
            opt = (" " + self.tr["opt_absent"].format(opt=", ".join(chk["optional_missing"]))) if chk["optional_missing"] else ""
            if not chk["found"]:
                self.q.put(("scan", gen, "empty", self.tr["empty_hdr"].format(miss=", ".join(chk["missing"] + chk["optional_missing"]))))
            elif chk["missing"]:
                self.q.put(("scan", gen, "partial", self.tr["partial_hdr"].format(miss=miss) + "\n" + self.tr["content_stats"].format(n=n, size=pi.fmt_size(size)) + opt))
            else:
                self.q.put(("scan", gen, "complete", self.tr["content_stats"].format(n=n, size=pi.fmt_size(size)) + opt))
        except Exception as e:   # noqa: BLE001
            self.q.put(("scan", gen, "empty", f"{type(e).__name__}: {e}"))

    def _start_fetch_payload(self):
        if self.closed:
            return
        self.lbl_payload.config(text=self.tr["payload_fetch"], fg=self.fg_light)

        def work():
            try:
                p = pi.fetch_payload(lambda m: self.q.put(("log", m)))
            except Exception as e:   # noqa: BLE001
                self.q.put(("log", f"Charge utile : {type(e).__name__}: {e}"))
                p = None
            self.q.put(("payload", str(p) if p else ""))

        threading.Thread(target=work, daemon=True).start()

    def _update_start(self):
        if self.kind != "install" or not hasattr(self, "btn_go"):
            return
        ok = self.only_sw.get() or self.scan_state == "complete" or (self.scan_state == "partial" and self.force_var.get())
        self.btn_go.config(state="normal" if (ok and not self.busy) else "disabled")

    def _blink(self):
        """« Veuillez patienter... » clignote a la place du resultat pendant l'analyse du dossier."""
        if self.closed:
            return
        try:
            if self.kind == "install" and self.scan_state == "scanning":
                self._blink_on = not self._blink_on
                self.lbl_stats.config(fg=self.fg if self._blink_on else self.bg)
            elif self.kind == "install":
                self.lbl_stats.config(fg=self._state_color())
            self.win.after(450, self._blink)
        except tk.TclError:
            pass

    def _state_color(self):
        return {"complete": "#43A047", "partial": "#FB8C00", "empty": self._warn()}.get(self.scan_state, self.fg_light)

    def _find_sd_root(self):
        try:
            for d in self.gui.tkmod._list_removable_drives_ex(any_target=True):
                letter = d[0].rstrip("\\") + "\\"
                if os.path.isdir(os.path.join(letter, "systems", "_defaults")):
                    return letter
        except Exception:
            pass
        return None

    def _browse_img(self):
        p = filedialog.askopenfilename(parent=self.win, filetypes=[("Raspberry Pi image", "*.img *.xz"), ("*", "*.*")])
        if p:
            self.img_var.set(p)

    def _browse_content(self):
        p = filedialog.askdirectory(parent=self.win)
        if p:
            self.content_var.set(p)
            self._refresh_content()

    def _update_layout(self):
        """Apercu de la repartition qui sera ecrite sur la carte choisie (memes regles que write_image_layout : FAT32 jusqu'a 30 Gio, exFAT au-dela)."""
        try:
            disk = self.disks[self.cb_disk.current()]
        except Exception:
            self.lbl_layout.config(text="")
            return
        e1 = {"start": 16384, "size": 1048576}      # bootfs de l'image Raspberry Pi OS (512 Mio)
        e2 = {"start": 1064960, "size": 3866624}    # racine de l'image (~1,8 Gio), agrandie au 1er demarrage
        try:
            pl = pi.plan_media_layout(disk["size"] // 512, e1, e2)
        except ValueError:
            self.lbl_layout.config(text=self.tr["layout_small"], fg=self._warn())
            return
        if self.kind == "prepare":
            self._check_keep(disk["number"])
        gio = lambda s: s * 512 / 1073741824   # noqa: E731
        root_final = (disk["size"] // 512 - pl["root"][0]) * 512 / 1073741824
        self.lbl_layout.config(text=self.tr["layout"].format(boot=f"{gio(pl['boot'][1]):.1f}", media=f"{gio(pl['media'][1]):.1f}", fs=pl["fs"], root=f"{root_final:.1f}"), fg=self.fg_light)

    def _check_keep(self, number):
        """Analyse (sans droits administrateur) de la partition media deja presente sur la carte choisie : active l'option de conservation si elle est valide."""
        self.keep_var.set(False)
        self.chk_keep.config(state="disabled")
        self.lbl_keep.config(text=self.tr["keep_checking"])

        def work():
            try:
                em = pi.existing_media(number)
            except Exception:   # noqa: BLE001
                em = None
            self.q.put(("keep", number, em))
        threading.Thread(target=work, daemon=True).start()

    def _refresh_disks(self):
        def work():
            try:
                self.q.put(("disks", pi.list_target_disks()))
            except Exception as e:   # noqa: BLE001
                self.q.put(("log", f"disques : {e}"))
                self.q.put(("disks", []))
        threading.Thread(target=work, daemon=True).start()

    # ----------------------------------------------------------------------------------------------- file d'evenements
    def _post(self, kind, *a):
        self.q.put((kind, *a))

    def _drain(self):
        if self.closed:
            return
        try:
            while True:
                item = self.q.get_nowait()
                k = item[0]
                if k == "log":
                    print(f"[Mode {16 if self.kind == 'prepare' else 17}] {item[1]}", flush=True)   # onglet Logs du toolkit
                    self.log.config(state="normal")
                    self.log.insert("end", item[1] + "\n")
                    self.log.see("end")
                    self.log.config(state="disabled")
                elif k == "status":
                    self.status.config(text=item[1])
                    print(f"[Mode {16 if self.kind == 'prepare' else 17}] {item[1].splitlines()[0] if item[1] else ''}", flush=True)
                elif k == "pct":
                    self.pbar.config(mode="determinate")
                    self.pbar["value"] = item[1]
                elif k == "anim":
                    if item[1]:
                        self.pbar.config(mode="indeterminate")
                        self.pbar.start(12)
                    else:
                        self.pbar.stop()
                        self.pbar.config(mode="determinate")
                elif k == "busy":
                    self.busy = item[1]
                    if item[1]:
                        self.btn_go.config(state="disabled")
                    else:
                        self._update_start() if self.kind == "install" else self.btn_go.config(state="normal")
                elif k == "disks":
                    self.disks = item[1]
                    names = [f"{d['number']} — {d['model']} — {pi.fmt_size(d['size'])} ({d['bus']})" for d in self.disks]
                    self.cb_disk["values"] = names
                    if names:
                        self.cb_disk.current(0)
                        self._update_layout()
                    else:
                        self.disk_var.set("")
                        self.status.config(text=self.tr["no_disk"])
                elif k == "payload":
                    if item[1]:
                        self.payload = Path(item[1])
                    if self.payload:
                        self.lbl_payload.config(text=self.tr["payload_ok"].format(v=pi.payload_version(self.payload) or "?", p=self.payload), fg=self.fg)
                    else:
                        self.lbl_payload.config(text=self.tr["payload_missing"] if self.kind != "install" else self.tr["no_payload"], fg=self._warn())
                    if self.kind == "install":
                        self._update_start()
                elif k == "scan":
                    if item[1] == self._scan_gen:   # resultat d'un ancien dossier : ignore
                        self.scan_state = item[2]
                        self.lbl_stats.config(text=item[3], fg=self._state_color())
                        if item[2] == "partial":
                            self.chk_force.grid()
                        else:
                            self.chk_force.grid_remove()
                            self.force_var.set(False)
                        self._update_start()
                elif k == "keep":
                    try:
                        cur = self.disks[self.cb_disk.current()]["number"]
                    except Exception:
                        cur = None
                    if item[1] == cur:      # resultat d'une carte deja deselectionnee : ignore
                        em = item[2]
                        ok = bool(em) and em["fs"] in ("FAT32", "exFAT") and em["label"].upper() == "RBPIDMD"
                        self.keep_info = em if ok else None
                        self.chk_keep.config(state="normal" if ok else "disabled")
                        self.lbl_keep.config(text=self.tr["keep_ok"].format(label=em["label"], fs=em["fs"], size=f"{em['size'] * 512 / 1073741824:.1f}") if ok else self.tr["keep_none"],
                                             fg=self.fg_light)
                elif k == "pimap":
                    self._show_map(item[1], item[2])
                elif k == "ask":
                    item[3]["v"] = messagebox.askyesno(self.win.title(), item[1], icon="warning", parent=self.win)
                    item[2].set()
                elif k == "done":
                    messagebox.showinfo(self.win.title(), item[1], parent=self.win)
                elif k == "error":
                    messagebox.showerror(self.win.title(), item[1], parent=self.win)
        except queue.Empty:
            pass
        self.win.after(100, self._drain)

    def _close(self):
        if self.busy and not messagebox.askyesno(self.win.title(), self.tr["busy_close"], parent=self.win):
            return
        self.closed = True
        self.win.destroy()

    def _go(self):
        if self.busy:
            return
        if self.kind in ("prepare", "setup"):
            self._go_prepare()
        else:
            self._go_install()

    # ----------------------------------------------------------------------------------------------- Mode 16
    def _go_prepare(self):
        tr = self.tr
        host, user, pw = self.host_var.get().strip(), self.user_var.get().strip(), self.pw_var.get()
        if not self.disks or not self.disk_var.get():
            messagebox.showwarning(self.win.title(), tr["need_disk"], parent=self.win)
            return
        if not pi.valid_hostname(host):
            messagebox.showwarning(self.win.title(), tr["bad_host"], parent=self.win)
            return
        if not pi.valid_username(user):
            messagebox.showwarning(self.win.title(), tr["bad_user"], parent=self.win)
            return
        if len(pw) < 6 or pw != self.pw2_var.get():
            messagebox.showwarning(self.win.title(), tr["need_pw"], parent=self.win)
            return
        disk = self.disks[self.cb_disk.current()]
        keep = self.kind == "prepare" and self.keep_var.get() and self.keep_info is not None
        if not messagebox.askyesno(self.win.title(), (tr["setup_confirm"] if self.kind == "setup" else tr["confirm16_keep"] if keep else tr["confirm16"]).format(disk=self.disk_var.get()), icon="warning", parent=self.win):
            return
        ssid, wpw, country = self.ssid_var.get().strip(), self.wpw_var.get(), (self.country_var.get() or "FR").strip().upper()[:2]
        keymap, tz = LOCALE.get(self.lang, LOCALE["fr"])
        if self.kind == "setup":
            prefs.set("pi_hostname", host)
            prefs.set("pi_user", user)
            prefs.set("pi_wifi_country", country)
            self.result = {"disk": disk, "host": host, "user": user, "pw": pw, "ssid": ssid, "wpw": wpw, "country": country, "keymap": keymap, "tz": tz,
                           "img_override": self.img_var.get().strip(), "rb_ip": self.rb_var.get().strip(), "payload": self.payload}
            self.closed = True
            self.win.destroy()
            return
        prefs.set("pi_hostname", host)
        prefs.set("pi_user", user)
        prefs.set("pi_wifi_country", country)
        img_override = self.img_var.get().strip()
        rb_ip = self.rb_var.get().strip()
        self._post("busy", True)
        self._post("pct", 0)
        threading.Thread(target=self._prepare_worker, args=(disk, host, user, pw, ssid, wpw, country, keymap, tz, img_override, rb_ip, keep), daemon=True).start()

    def _prepare_worker(self, disk, host, user, pw, ssid, wpw, country, keymap, tz, img_override, rb_ip, keep=False):
        tr = self.tr
        try:
            if img_override:
                image = Path(img_override)
                if not image.is_file():
                    raise RuntimeError(f"image introuvable : {image}")
            else:
                self._post("status", tr["step_find"])
                self._post("anim", True)
                info = pi.find_latest_image()
                self._post("log", f"image : {info['name']}")
                self._post("anim", False)
                last = [-1]

                def prog(d, t):
                    pct = int(d * 100 / t)
                    if pct != last[0]:
                        last[0] = pct
                        self._post("pct", pct)
                        self._post("status", tr["step_dl"].format(pct=pct))
                image = pi.download_image(info, prog, lambda m: self._post("log", m))
                self._post("log", tr["step_dl_ok"].format(name=image.name))
            toml = pi.build_custom_toml(host, user, pi.sha512_crypt(pw), ssid, wpw, country, keymap, tz)
            job = {"disk_number": disk["number"], "expected_model": disk["model"], "expected_size": disk["size"], "image_path": str(image),
                   "custom_toml": toml, "payload_dir": str(self.payload) if self.payload else "",
                   "config_ini": pi.make_config_seed(rb_ip, self.lang), "keep_media": bool(keep)}
            jp = pi.write_job(job)
            self._post("status", tr["step_uac"])
            self._post("pct", 0)
            if not pi.launch_elevated(jp):
                try:
                    jp.unlink()
                except OSError:
                    pass
                raise RuntimeError(tr["uac_refused"])
            pos, pct = 0, 0
            media = "?:"
            res = None
            t0 = __import__("time").time()
            while True:
                lines, pos, fin, res = pi.read_progress(jp, pos)
                for ln in lines:
                    if ln.startswith("MEDIA "):
                        media = ln[6:].strip()
                    if ln.startswith("PCT "):
                        pct = int(ln[4:])
                        self._post("pct", pct)
                        self._post("status", tr["step_write"].format(pct=pct))
                    else:
                        self._post("log", ln)
                if fin:
                    break
                if self.closed:
                    return
                if not lines and pos == 0 and __import__("time").time() - t0 > 180:
                    raise RuntimeError(tr["uac_refused"])
                __import__("time").sleep(0.7)
            for ext in (".log", ".done"):
                try:
                    Path(str(jp) + ext).unlink()
                except OSError:
                    pass
            if res != "OK":
                raise RuntimeError(res.replace("ERR: ", "", 1) if res else "?")
            _LAST_PI.update({"host": host, "user": user, "pw": pw})
            self._post("status", tr["ok16"].split("\n")[0])
            self._post("done", tr["ok16"].format(media=media))
        except Exception as e:   # noqa: BLE001
            self._post("anim", False)
            self._post("status", str(e))
            self._post("error", tr["fail16"].format(err=e))
        finally:
            self._post("busy", False)

    # ----------------------------------------------------------------------------------------------- Mode 17
    def _go_install(self):
        tr = self.tr
        if not self.payload:
            messagebox.showwarning(self.win.title(), tr["no_payload"], parent=self.win)
            return
        host, user, pw = self.host_var.get().strip(), self.user_var.get().strip(), self.pw_var.get()
        if not pw:
            messagebox.showwarning(self.win.title(), tr["need_pw"], parent=self.win)
            return
        prefs.set("pi_host", host)
        prefs.set("pi_user", user)
        content = "" if self.only_sw.get() else self.content_var.get().strip().strip('"')
        self._post("busy", True)
        self._post("pct", 0)
        threading.Thread(target=self._install_worker, args=(host, user, pw, content), daemon=True).start()

    def _install_worker(self, host, user, pw, content):
        tr = self.tr
        log = lambda m: self._post("log", m)   # noqa: E731
        c = None
        try:
            self._post("status", tr["step_find_pi"])
            self._post("anim", True)
            try:
                c, addr = pi.find_pi(host, user, pw, log)
            except PermissionError:
                raise RuntimeError(tr["pw_refused"])
            except RuntimeError as e:
                raise RuntimeError(tr["pi_not_found"].format(err=e))
            self._post("anim", False)
            log(tr["pi_found"].format(host=addr))
            pi.dmd_say(addr, tr["dmd_install"], "DMD INSTALL")
            self._post("status", tr["step_upload"])
            stage = f"/home/{user}/recalboxdmd_stage"
            pi.upload_payload(c, self.payload, stage, "", log)
            sd = "/opt/recalboxdmd/sd"
            self._post("status", tr["step_install"])
            self._post("anim", True)
            code, out = pi.run_remote(c, f"cd {stage} && bash install.sh --sd {sd}", log, sudo_pw=pw, timeout=900)
            self._post("anim", False)
            if code != 0:
                raise RuntimeError(f"install.sh : code {code}")
            if "REDEMARRAGE NECESSAIRE" in out:
                self._post("status", tr["step_reboot"])
                self._post("anim", True)
                try:
                    pi.run_remote(c, "reboot", None, sudo_pw=pw, timeout=15)
                except Exception:   # noqa: BLE001
                    pass
                try:
                    c.close()
                except Exception:   # noqa: BLE001
                    pass
                c = pi.wait_pi_back(addr, user, pw, 300, log)
                self._post("anim", False)
                if c is None:
                    raise RuntimeError("le Pi ne répond plus après le redémarrage")
                log(tr["pi_found"].format(host=addr))
            sd_map = pi.remote_check(c, sd)
            self._post("pimap", sd_map, "before")
            log("Carte du Pi avant la copie : present = " + (", ".join(sd_map["found"]) or "(rien)") + " | absent = " + (", ".join(sd_map["missing"] + sd_map["optional_missing"]) or "(rien)"))
            if not content:
                log(tr["log_no_content"])
            elif not Path(content).is_dir():
                log(tr["log_bad_folder"].format(p=content))
            if content and Path(content).is_dir():
                self._post("pct", 0)

                last_say = [0.0]

                def prog(d, t):
                    pct = int(d * 100 / max(1, t))
                    self._post("pct", pct)
                    self._post("status", tr["step_sync"].format(done=d, total=t))
                    now = __import__("time").time()
                    if now - last_say[0] > 4:   # le DMD annonce l'avancement (au plus toutes les 4 s : son serveur web est mono-tache)
                        last_say[0] = now
                        pi.dmd_say(addr, tr["dmd_copy"].format(pct=pct), "DMD INSTALL")
                pi.dmd_say(addr, tr["dmd_copy"].format(pct=0), "DMD INSTALL")
                src_chk = pi.check_content(Path(content))
                overwrite_cfg = False
                if "config.ini" in sd_map["found"] and "config.ini" in src_chk["found"]:   # un config.ini existe des deux cotes : les reglages du Pi seraient perdus
                    overwrite_cfg = self._ask(tr["ask_cfg"])
                both = [x for x in src_chk["found"] if x in sd_map["found"] and x != "config.ini"]
                replace = False
                if both:
                    replace = self._ask(tr["ask_replace"].format(list=", ".join(both)))
                st = pi.sync_content(c, Path(content), sd, log, prog, lambda: self.closed, replace=replace, overwrite_config=overwrite_cfg)
                log(tr["sync_res"].format(**st))
                after = pi.remote_check(c, sd)
                self._post("pimap", after, "after")
                log("Carte du Pi apres la copie : present = " + (", ".join(after["found"]) or "(rien)") + " | absent = " + (", ".join(after["missing"] + after["optional_missing"]) or "(rien)"))
                pi.run_remote(c, "systemctl restart recalboxdmd", log, sudo_pw=pw, timeout=60)
            pi.run_remote(c, f"rm -rf {stage}", None)
            self._post("pct", 100)
            pi.dmd_resume(addr)   # rend l'ecran au DMD (premier demarrage : ecran de configuration)
            chk = None
            for _ in range(10):
                chk = pi.pi_web_check(addr)
                if chk:
                    break
                __import__("time").sleep(2)
            first = ""
            if chk:
                first = tr["first_yes"] if chk["first_boot"] == "1" else tr["first_no"]
            msg = tr["ok17"].format(host=addr, first=first)
            self._post("status", msg.split("\n")[0])
            self._post("done", msg)
        except Exception as e:   # noqa: BLE001
            self._post("anim", False)
            self._post("status", str(e))
            self._post("error", tr["fail17"].format(err=e))
        finally:
            if c is not None:
                try:
                    c.close()
                except Exception:   # noqa: BLE001
                    pass
            self._post("busy", False)


def _gui_lang(gui) -> str:
    try:
        lang = gui.lang_var.get()
    except Exception:
        lang = "fr"
    return lang if lang in T else "fr"


def target_text(gui, key: str) -> str:
    return T[_gui_lang(gui)][key]


class CardBuild:
    """Ecriture de la carte du Pi en arriere-plan pendant que le Mode 1 prepare le contenu (meme logique que le Mode 16 : image, disposition, formatage,
    amorcage hors ligne). Le contenu est copie ensuite dans la partition media par finish_card()."""

    def __init__(self, gui, setup):
        self.gui, self.setup = gui, setup
        self.done = threading.Event()
        self.media = None
        self.error = None
        self.tr = T[_gui_lang(gui)]
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def _log(self, m):
        self.gui._log_q.put(f"[Pi] {m}\n")

    def _run(self):
        s, tr = self.setup, self.tr
        try:
            self._log(tr["w_start"])
            if s["img_override"]:
                image = Path(s["img_override"])
                if not image.is_file():
                    raise RuntimeError(f"image introuvable : {image}")
            else:
                info = pi.find_latest_image()
                self._log(f"image : {info['name']}")
                last = [-10]

                def prog(d, t):
                    pct = int(d * 100 / t)
                    if pct >= last[0] + 10:
                        last[0] = pct
                        self._log(tr["step_dl"].format(pct=pct))
                image = pi.download_image(info, prog, self._log)
            toml = pi.build_custom_toml(s["host"], s["user"], pi.sha512_crypt(s["pw"]), s["ssid"], s["wpw"], s["country"], s["keymap"], s["tz"])
            disk = s["disk"]
            job = {"disk_number": disk["number"], "expected_model": disk["model"], "expected_size": disk["size"], "image_path": str(image),
                   "custom_toml": toml, "payload_dir": str(s["payload"]) if s["payload"] else "",
                   "config_ini": pi.make_config_seed(s["rb_ip"], _gui_lang(self.gui))}
            jp = pi.write_job(job)
            self._log(tr["step_uac"])
            if not pi.launch_elevated(jp):
                try:
                    jp.unlink()
                except OSError:
                    pass
                raise RuntimeError(tr["uac_refused"])
            pos, last_pct, media, res = 0, -10, "", None
            t0 = time.time()
            while True:
                lines, pos, fin, res = pi.read_progress(jp, pos)
                for ln in lines:
                    if ln.startswith("MEDIA "):
                        media = ln[6:].strip()
                    elif ln.startswith("PCT "):
                        pct = int(ln[4:])
                        if pct >= last_pct + 10:
                            last_pct = pct
                            self._log(tr["step_write"].format(pct=pct))
                    else:
                        self._log(ln)
                if fin:
                    break
                if not lines and pos == 0 and time.time() - t0 > 180:
                    raise RuntimeError(tr["uac_refused"])
                time.sleep(0.7)
            for ext in (".log", ".done"):
                try:
                    Path(str(jp) + ext).unlink()
                except OSError:
                    pass
            if res != "OK":
                raise RuntimeError(res.replace("ERR: ", "", 1) if res else "?")
            if not media or media == "?:":
                raise RuntimeError("lettre de la partition media inconnue")
            self.media = media
            _LAST_PI.update({"host": s["host"], "user": s["user"], "pw": s["pw"]})
        except Exception as e:   # noqa: BLE001
            self.error = str(e)
        finally:
            self.done.set()


def start_card_build(gui):
    """Lance l'ecriture de la carte si le Mode 1 vise le Pi (reglages saisis par prompt_pi_setup)."""
    setup = getattr(gui, "_pi_setup", None)
    gui._pi_card = CardBuild(gui, setup) if setup else None


def finish_card(gui, toolkit, sd_dir):
    """Fin du Mode 1 Pi (thread du pipeline, sortie dans le journal) : attend la carte, copie le contenu dans la partition media, renvoie le message final."""
    tr = T[_gui_lang(gui)]
    build = getattr(gui, "_pi_card", None)
    if build is None:
        return None
    print(f"[Pi] {tr['w_wait']}")
    build.done.wait()
    if build.error:
        hint = tr["w_repair"]
        if "Errno 9" in build.error or "Bad file descriptor" in build.error or "Errno 13" in build.error or "Permission" in build.error:
            hint = tr["w_repair_cause"] + hint
        raise RuntimeError(tr["w_fail"].format(err=build.error) + hint)
    media = build.media
    ips = toolkit.split_recalbox_ips(build.setup["rb_ip"])
    if ips:
        toolkit.write_dmd_recalbox_ip(sd_dir, ips[0], ips[1:])   # la liste choisie dans l'assistant prime meme si l'installation des scripts a echoue
    print(f"[Pi] {tr['w_copy'].format(media=media)}")
    manifest = Path(os.environ.get("TEMP", ".")) / "RecalBoxDMD_pi_copy.json"   # hors du dossier copie : il ne doit pas atterrir sur la carte
    try:
        manifest.unlink()
    except OSError:
        pass
    res = toolkit._copy_to_drive(sd_dir, media + "\\", True, progress_cb=gui._progress_cb, manifest_path=manifest)
    copied, failed, interrupted = (res + (False,))[:3] if isinstance(res, tuple) else (res, [], False)
    if toolkit.PAUSE.should_stop():
        raise RuntimeError("copie arrêtée à la demande : la partition média est incomplète")
    if interrupted or failed:
        raise RuntimeError(f"copie incomplete ({len(failed)} fichier(s) en echec{', carte retiree ?' if interrupted else ''})")
    print(f"[Pi] {tr['w_ok']}")
    return tr["w_final"].format(media=media, n=copied)


def prompt_pi_setup(gui):
    """Mode 1 pour le Raspberry Pi : carte vierge + reglages du Pi (nom, mot de passe, Wi-Fi, pays, Recalbox). Renvoie le dictionnaire des reglages ou None."""
    w = open_window(gui, "setup")
    gui.root.wait_window(w.win)
    return w.result
