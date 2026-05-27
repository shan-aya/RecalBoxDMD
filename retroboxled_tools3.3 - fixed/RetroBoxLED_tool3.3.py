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
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from urllib.parse import unquote

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

    def start(self, listen_keyboard: bool = True):
        self.state = self.RUNNING
        self._active = True
        self._thread = None
        if listen_keyboard:
            self._thread = threading.Thread(target=self._listen, daemon=True)
            self._thread.start()

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

    def _listen(self):
        """Thread background : lit stdin ligne par ligne."""
        import sys

        while self._active:
            try:
                # On lit stdin sans bloquer le thread principal
                # msvcrt sur Windows, sinon select sur Unix
                if sys.platform == "win32":
                    import msvcrt

                    if msvcrt.kbhit():
                        ch = msvcrt.getwch()
                        if ord(ch) == 27:  # ESC
                            self._on_pause()
                    time.sleep(0.1)
                else:
                    import select

                    if select.select([sys.stdin], [], [], 0.1)[0]:
                        ch = sys.stdin.read(1)
                        if ord(ch) == 27:  # ESC
                            self._on_pause()
            except Exception:
                time.sleep(0.1)

    def _on_pause(self):
        with self._lock:
            if self.state != self.RUNNING:
                return
            self.state = self.PAUSED

        print(tr("pause_title"))
        sep("─")
        print(f"  1  →  {tr('pause_opt1')}")
        print(f"  2  →  {tr('pause_opt2')}")
        print(f"  3  →  {tr('pause_opt3')}")
        print()

        while True:
            raw = input(tr("pause_choice")).strip()
            if raw == "1":
                print(tr("pause_resuming"))
                with self._lock:
                    self.state = self.RUNNING
                break
            elif raw == "2":
                print(tr("pause_skipping"))
                with self._lock:
                    self.state = self.SKIP
                break
            elif raw == "3":
                print(tr("pause_stopping"))
                with self._lock:
                    self.state = self.STOP
                break
            else:
                print(tr("pause_warn"))

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
        "dl_done": lambda n: f"✅ {n} fichiers téléchargés dans _defaults/",
        "dl_fail_api": "❌ API GitHub inaccessible. Vérifiez votre connexion internet.",
        "dl_replacing": "🗑️  Remplacement du _defaults/ existant...",
        "main_prompt": "Que voulez-vous faire ?",
        "main_opt1": "Extraction gamelist + Conversion 128x32 + Build cache  (TOUT)",
        "main_opt2": "Seulement extraire les images des gamelists",
        "main_opt3": "Seulement convertir des images en 128x32",
        "main_opt4": "Seulement construire le games_cache.bin",
        "main_opt7": "Interface graphique (Tkinter)",
        "main_choice": "Votre choix (1-9) : ",
        "main_opt_quit": "Quitter",
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
        "conv_gif_info": "GIF trouvés → conservés sans modification.",
        "conv_png_count": "PNG à convertir en 128x32",
        "conv_summary_done": "PNG convertis",
        "conv_summary_err": "erreurs",
        "conv_summary_gif": "GIF conservés",
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
        "mode1_title": "MODE 1 — Extraction + Conversion 128x32 + Build Cache",
        "mode2_title": "MODE 2 — Téléchargement systems/_defaults via GitHub uniquement",
        "mode3_title": "MODE 3 — Extraction Gamelist uniquement",
        "mode4_title": "MODE 4 — PNG→raw565 + GIF→raw565pack/meta",
        "mode5_title": "MODE 5 — Conversion 128x32 uniquement",
        "mode6_title": "MODE 6 — Build Games Cache uniquement",
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
        "flash_copy_err": lambda c: f"⚠️  Robocopy terminé avec le code {c} (vérifiez la sortie ci-dessus).",
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
        "dl_done": lambda n: f"✅ {n} files downloaded into _defaults/",
        "dl_fail_api": "❌ GitHub API unreachable. Check your internet connection.",
        "dl_replacing": "🗑️  Replacing existing _defaults/...",
        "main_prompt": "What do you want to do?",
        "main_opt1": "Gamelist extraction + 128x32 conversion + Build cache  (ALL)",
        "main_opt2": "Gamelist image extraction only",
        "main_opt3": "128x32 conversion only",
        "main_opt4": "Build games_cache.bin only",
        "main_opt7": "Graphical interface (Tkinter)",
        "main_choice": "Your choice (1-9): ",
        "main_opt_quit": "Quit",
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
        "conv_gif_info": "GIF found → kept without modification.",
        "conv_png_count": "PNG to convert to 128x32",
        "conv_summary_done": "PNG converted",
        "conv_summary_err": "errors",
        "conv_summary_gif": "GIF kept",
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
        "mode1_title": "MODE 1 — Extraction + 128x32 Conversion + Build Cache",
        "mode2_title": "MODE 2 — systems/_defaults download via GitHub only",
        "mode3_title": "MODE 3 — Gamelist Extraction only",
        "mode4_title": "MODE 4 — PNG→raw565 + GIF→raw565pack/meta",
        "mode5_title": "MODE 5 — 128x32 conversion only",
        "mode6_title": "MODE 6 — Build Games Cache only",
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
        "flash_copy_err": lambda c: f"⚠️  Robocopy finished with code {c} (check output above).",
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
        "dl_done": lambda n: f"✅ {n} archivos descargados en _defaults/",
        "dl_fail_api": "❌ API de GitHub inaccesible. Verifica tu conexión a internet.",
        "dl_replacing": "🗑️  Reemplazando _defaults/ existente...",
        "main_prompt": "¿Qué desea hacer?",
        "main_opt1": "Extracción gamelist + Conversión 128x32 + Build cache  (TODO)",
        "main_opt2": "Solo extraer imágenes de los gamelists",
        "main_opt3": "Solo convertir imágenes a 128x32",
        "main_opt4": "Solo construir games_cache.bin",
        "main_opt7": "Interfaz gráfica (Tkinter)",
        "main_choice": "Su eleccion (1-9): ",
        "main_opt_quit": "Salir",
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
        "conv_gif_info": "GIF encontrados → conservados sin modificación.",
        "conv_png_count": "PNG a convertir a 128x32",
        "conv_summary_done": "PNG convertidos",
        "conv_summary_err": "errores",
        "conv_summary_gif": "GIF conservados",
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
        "mode1_title": "MODO 1 — Extracción + Conversión 128x32 + Build Cache",
        "mode2_title": "MODO 2 — descarga systems/_defaults via GitHub (solo)",
        "mode3_title": "MODO 3 — Solo Extracción Gamelist",
        "mode4_title": "MODO 4 — PNG→raw565 + GIF→raw565pack/meta",
        "mode5_title": "MODO 5 — Solo Conversión 128x32",
        "mode6_title": "MODO 6 — Solo Build Games Cache",
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
        "flash_copy_err": lambda c: f"⚠️  Robocopy terminó con código {c} (revisa la salida anterior).",
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
    },
}

# Global translation dict (set in main after language selection)
T = TRANSLATIONS["fr"]


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


# ─────────────────────────────────────────────────────────────────────────────
#  CONSTANTES
# ─────────────────────────────────────────────────────────────────────────────

TARGET_W = 128
TARGET_H = 32
EXTENSIONS_CACHE = {".gif": 0x67, ".png": 0x70}
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

    return base / "RetroBoxLED"


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
    Depuis la MAJ Recalbox, la balise <logo> remplace <thumbnail>/<image>
    Les images sont copiées directement dans systems/<sys>/ (pas de sous-dossier).
    Retourne [(tag, folder)] fixe.
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
        pixels = rgb_img.getdata()  # row-major
        with open(raw_path, "wb") as f:
            for r, g, b in pixels:
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack("<H", rgb565))


def convert_png_to_raw565_only(src_png: Path) -> Path:
    """
    Convertit un PNG en .raw565 (RGB565) sans modifier le PNG d'origine.
    Même logique de centrage/redimensionnement que convert_image_file().
    """
    from PIL import Image

    raw_path = src_png.with_suffix(".raw565")

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
        pixels = rgb_img.getdata()  # row-major

        with open(raw_path, "wb") as f:
            for r, g, b in pixels:
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack("<H", rgb565))

    return raw_path


def convert_gif_to_raw565pack_meta(gif_path: Path) -> None:
    """
    Génère un pack unique:
      - NAME.raw565pack : frames RGB565 concaténées (frame0, frame1, ...)
      - NAME.meta        : suite uint16 little-endian = delay_ms par frame

    Important: on fait un "centrage + redimensionnement" identique aux PNG
    sur un canvas 128x32. Le timing est conservé via frame.info['duration'].
    """
    from PIL import Image, ImageSequence

    raw_pack_path = gif_path.with_suffix(".raw565pack")
    meta_path = gif_path.with_suffix(".meta")

    frame_bytes = TARGET_W * TARGET_H * 2  # 128*32*2 = 8192

    with Image.open(gif_path) as im:
        im.seek(0)

        # Ouvre en écriture; on reconstruit à chaque conversion.
        with open(raw_pack_path, "wb") as pack_f, open(meta_path, "wb") as meta_f:
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
                pixels = rgb_img.getdata()

                for r, g, b in pixels:
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    pack_f.write(struct.pack("<H", rgb565))

                # delay_ms per frame
                delay_ms = frame.info.get("duration", 50)
                try:
                    delay_ms = int(delay_ms)
                except Exception:
                    delay_ms = 50
                if delay_ms <= 0:
                    delay_ms = 50
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
    root.title("RetroBoxLED — Conversion 128x32")
    root.resizable(False, False)

    # Message
    msg = tk.Label(
        root,
        text=f"Préparation conversion 128x32.\n\nSortie :\n{str(out_dir)}",
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
        buttons, text="Explorer dossier", width=18, command=on_explore
    )
    explore_btn.grid(row=0, column=0, padx=8, pady=6)

    cont_btn = tk.Button(buttons, text="Continuer", width=18, command=on_continue)
    cont_btn.grid(row=0, column=1, padx=8, pady=6)

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()
    return bool(ok_holder["ok"])


def run_conversion(
    systems_dir: Path,
    progress_cb=None,
    listen_keyboard: bool = True,
):
    if not PIL_AVAILABLE:
        print(tr("conv_no_pillow"))
        return

    print(tr("conv_png_only"))

    png_files = list(systems_dir.rglob("*.png"))
    gif_files = list(systems_dir.rglob("*.gif"))
    total = len(png_files)
    done = errors = 0

    # Assure que l'UI passe en "Conversion" même si total == 0
    if progress_cb is not None:
        progress_cb(
            "conversion",
            0,
            total,
            "aucun PNG à convertir" if total == 0 else "conversion en cours",
        )

    if gif_files:
        print(f"   🎞️  {len(gif_files)} {tr('conv_gif_info')}")
    print(f"   🖼️  {total} {tr('conv_png_count')}")
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
            convert_image_file(src, src)
            done += 1
            print(f"   {i:5d}/{total} ✅ {src.relative_to(systems_dir)}")
        except Exception as e:
            errors += 1
            print(f"   {i:5d}/{total} ❌ {src.relative_to(systems_dir)} — {e}")

    # GIF -> raw565pack + meta (timing via frame duration)
    gif_done = 0
    gif_errors = 0
    if gif_files and not PAUSE.should_stop():
        print(f"\n   🎞️  Conversion GIF -> raw565pack/meta : {len(gif_files)} fichiers")
        for j, gif_src in enumerate(gif_files, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break
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
        f"🎞️  {len(gif_files)} {tr('conv_summary_gif')}"
    )


def run_conversion_raw_only(
    systems_dir: Path,
    progress_cb=None,
    listen_keyboard: bool = True,
):
    """
    Conversion raw-only :
      - PNG  -> *.raw565 (sans ré-écrire les PNG)
      - GIF  -> *.raw565pack + *.meta (timing via duration frame)
    """
    if not PIL_AVAILABLE:
        print(tr("conv_no_pillow"))
        return

    png_files = list(systems_dir.rglob("*.png"))
    gif_files = list(systems_dir.rglob("*.gif"))
    total = len(png_files)
    done = errors = 0

    if progress_cb is not None:
        progress_cb(
            "conversion",
            0,
            max(total, 1),
            "aucun PNG à convertir" if total == 0 else "raw conversion en cours",
        )

    print(f"\n   🖼️  {total} {tr('conv_png_count')}")
    if gif_files:
        print(f"   🎞️  {len(gif_files)} {tr('conv_gif_info')} (pack/meta)")
    print(tr("pause_hint"))
    sep("─")

    PAUSE.start(listen_keyboard=listen_keyboard)

    for i, src in enumerate(png_files, 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break

        if progress_cb is not None:
            progress_cb("conversion", i, max(total, 1), f"{src.parent.name}|{src.name}")

        try:
            convert_png_to_raw565_only(src)
            done += 1
            print(f"   {i:5d}/{total} ✅ {src.relative_to(systems_dir)}")
        except Exception as e:
            errors += 1
            print(f"   {i:5d}/{total} ❌ {src.relative_to(systems_dir)} — {e}")

    gif_done = 0
    if gif_files and not PAUSE.should_stop():
        print(f"\n   🎞️  Conversion GIF -> raw565pack/meta : {len(gif_files)} fichiers")
        for j, gif_src in enumerate(gif_files, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break
            try:
                convert_gif_to_raw565pack_meta(gif_src)
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
        f"🎞️  {len(gif_files)} {tr('conv_summary_gif')}"
    )


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
            for subdir in scan_dirs:
                for fname in os.listdir(subdir):
                    if (subdir / fname).is_dir():
                        continue
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
    Genere un games_cache.bin par dossier image detecte.
    - games_cache.bin            : images a la racine
    - games_cache_FOLDER.bin     : images dans le sous-dossier FOLDER
    Index bigramme 703 entrees pour des tranches plus petites en RAM.
    """
    print(f"{tr('cache_scan')}{systems_dir}")

    image_folders = set()
    for sysname in os.listdir(systems_dir):
        syspath = systems_dir / sysname
        if not syspath.is_dir() or sysname.lower() == "_defaults":
            continue
        for d in syspath.iterdir():
            if d.is_dir():
                image_folders.add(d.name)

    folders_to_build = [""] + sorted(image_folders)

    generated = []
    total_folders = len(folders_to_build)
    for folder_idx, folder in enumerate(folders_to_build, 1):
        PAUSE.wait_if_paused()
        if PAUSE.should_stop() or PAUSE.should_skip():
            break
        if progress_cb is not None:
            progress_cb(
                "cache", folder_idx, total_folders, folder if folder else "racine"
            )
        data = collect_games_for_folder(systems_dir, folder)
        if not data:
            continue

        fname = f"games_cache_{folder}.bin" if folder else "games_cache.bin"
        output_path = output_dir / fname
        label = f"[{folder}]" if folder else "[racine]"
        print(f"\n--- Cache {label} ---")
        nb_sys, nb_games = _write_cache_binary(data, output_path)
        if nb_sys > 0:
            generated.append((fname, nb_sys, nb_games))

    print(f"\n[INFO] {len(generated)} cache(s) genere(s) :")
    for fname, nb_sys, nb_games in generated:
        print(f"   {fname}  ({nb_sys} systemes, {nb_games} jeux)")
    return [
        (output_dir / fname, nb_sys, nb_games) for fname, nb_sys, nb_games in generated
    ]


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
    print(f"\n{tr('conv_title')}")
    PAUSE.state = PAUSE.RUNNING
    if not popup_confirm_before_conversion(systems_out):
        return
    run_conversion(systems_out)

    sep("─")
    if PAUSE.should_stop():
        sep()
        print(tr("done"))
        print(tr("pause_stopping"))
        return
    print(f"\n{tr('cache_title')}")
    build_cache(systems_out, sd_dir)

    # ── Téléchargement _defaults depuis GitHub ───────────────────────────────
    sep("─")
    download_defaults(sd_dir)

    # ── Auto-génération systems_cache.dat ────────────────────────────────────
    sep("─")
    print(f"\n{tr('sysc_title')}")
    sysc_out = sd_dir / "systems_cache.dat"
    build_systems_cache(systems_out, sysc_out)

    sep()
    print(tr("done"))
    print(f"{tr('done_sd')}{sd_dir}")
    print(f"{tr('done_log')}{log_path}")
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
    print(f"\n   {tr('dl_done')}(aucune autre étape n'est exécutée)")
    print("\n   ↩  Vous pouvez ensuite copier sd_card/ sur la carte SD.")


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
            print(f"   💾 {path}  ({nb_sys} syst., {nb_games} jeux)")
        print(f"\n   {tr('done_copy_cache')}")
    return [path for path, _, _ in generated] if generated else []


# ─────────────────────────────────────────────────────────────────────────────
#  TÉLÉCHARGEMENT _defaults DEPUIS GITHUB
# ─────────────────────────────────────────────────────────────────────────────

GITHUB_API_URL = (
    "https://api.github.com/repos/Jamyz/RetroBoxLED/contents/systems/_defaults"
)
GITHUB_RAW_BASE = (
    "https://raw.githubusercontent.com/Jamyz/RetroBoxLED/main/systems/_defaults"
)


def download_defaults(
    sd_dir: Path,
    progress_cb=None,
    listen_keyboard: bool = True,
    replace_existing=None,
    download_missing=None,
):
    """
    Propose de télécharger _defaults/ depuis GitHub.
    - Si absent  → propose de télécharger
    - Si présent → propose de mettre à jour (remplace)
    - Dans les deux cas, l'utilisateur peut refuser
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
        shutil.rmtree(defaults_dir)
        defaults_dir.mkdir(parents=True, exist_ok=True)
    else:
        print(tr("dl_missing"))
        if download_missing is None:
            if listen_keyboard:
                download_missing = ask_yes_no(tr("dl_ask_download"))
            else:
                download_missing = False  # safe default: ne pas télécharger
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
        print(tr("dl_fail_api"))
        print(f"   {e}")
        return

    # Filtre uniquement les fichiers png/gif
    media_files = [
        f
        for f in files
        if f.get("type") == "file"
        and Path(f["name"]).suffix.lower() in (".png", ".gif")
    ]

    total = len(media_files)
    print(tr("dl_starting"))
    done = 0

    PAUSE.start(listen_keyboard=listen_keyboard)
    try:
        for i, f in enumerate(media_files, 1):
            PAUSE.wait_if_paused()
            if PAUSE.should_stop() or PAUSE.should_skip():
                break

            fname = f["name"]
            if progress_cb is not None:
                progress_cb("download_defaults", i, total, fname)

            raw_url = f"{GITHUB_RAW_BASE}/{urllib.request.quote(fname)}"
            dst = defaults_dir / fname
            try:
                urllib.request.urlretrieve(raw_url, dst)
                done += 1
                print(tr("dl_file_ok")(fname, i, total))
            except Exception as e:
                print(tr("dl_file_err")(fname, e))
    finally:
        PAUSE.stop()

    print(tr("dl_done")(done))


# ─────────────────────────────────────────────────────────────────────────────
#  BUILD SYSTEMS CACHE  (systems_cache.dat pour l'ESP32)
# ─────────────────────────────────────────────────────────────────────────────


def build_systems_cache(
    systems_dir: Path,
    output_path: Path,
    progress_cb=None,
):
    """
    Génère systems_cache.dat au format attendu par l'ESP32 :
      g nes
      p snes
      g neogeo
    Scanne systems_dir/_defaults/ — un fichier par système (gif prioritaire).
    """
    defaults_dir = systems_dir / "_defaults"
    if not defaults_dir.exists():
        print(tr("sysc_no_defaults"))
        return 0

    # Collecte tous les noms de systèmes présents dans _defaults/
    entries = {}
    for f in defaults_dir.iterdir():
        if f.is_file() and f.suffix.lower() in (".gif", ".png"):
            stem = f.stem.lower()
            ftype = "g" if f.suffix.lower() == ".gif" else "p"
            # gif prioritaire sur png
            if stem not in entries or ftype == "g":
                entries[stem] = (f.stem, ftype)

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

            # LENT : plus de 800 PNG OU GIF (mais pas les deux)
            # -> XOR : (png_over ^ gif_over)
            system_dir = systems_dir / name

            def count_ext_over(base: Path, ext_lower: str, limit: int) -> bool:
                # Retourne True dès qu'on dépasse "limit"
                count = 0
                for _root, _dirs, files in os.walk(base):
                    for fn in files:
                        if fn.lower().endswith(ext_lower):
                            count += 1
                            if count > limit:
                                return True
                return False

            png_over = False
            gif_over = False
            if system_dir.exists() and system_dir.is_dir():
                png_over = count_ext_over(system_dir, ".png", 800)
                gif_over = count_ext_over(system_dir, ".gif", 800)

            slow_flag = "L" if (png_over or gif_over) else "N"
            out.write(f"{ftype} {name} {slow_flag}\n")
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


def _robocopy(src: Path, dst: str, overwrite: bool):
    """
    Lance robocopy avec /MT:32 pour une copie rapide.
    overwrite=True  → /IS /IT (écrase les fichiers identiques aussi)
    overwrite=False → /XC /XN /XO (ignore fichiers plus récents/identiques/anciens)
    """
    import subprocess

    src_str = str(src)
    flags = ["/E", "/MT:32", "/NFL", "/NJH", "/NP"]
    if overwrite:
        flags += ["/IS", "/IT"]
    else:
        flags += ["/XC", "/XN", "/XO"]

    cmd = ["robocopy", src_str, dst] + flags
    print(tr("flash_copy_start")(src_str, dst))

    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    spinner = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
    i = 0
    while proc.poll() is None:
        print(
            f"\r   {spinner[i % len(spinner)]}  Copie en cours...", end="", flush=True
        )
        i += 1
        time.sleep(0.1)
    print("\r" + " " * 30 + "\r", end="")  # efface la ligne spinner

    if proc.returncode < 4:
        print(tr("flash_copy_ok"))
    else:
        print(tr("flash_copy_err")(proc.returncode))


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
    global T
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
            return
        if raw == "2":
            T = TRANSLATIONS["fr"]
            return
        if raw == "3":
            T = TRANSLATIONS["es"]
            return
        print("  ⚠️  1 / 2 / 3\n")


# ─────────────────────────────────────────────────────────────────────────────
#  POINT D'ENTRÉE
# ─────────────────────────────────────────────────────────────────────────────


def main():
    select_language()
    ensure_dependencies()

    script_dir = Path(__file__).parent
    sd_dir = get_sd_card_dir(script_dir)

    print()
    sep()
    print(f"  {tr('main_title')}")
    sep()
    print()
    print(f"  {tr('main_prompt')}")
    print()
    print(f"  1  →  {tr('main_opt1')}")
    print(f"  2  →  Extraction + téléchargement _defaults depuis GitHub (uniquement)")
    print(f"  3  →  {tr('main_opt2')}")
    print(f"  4  →  {tr('main_opt3')}")
    print(f"  5  →  {tr('main_opt4')}")
    print(f"  6  →  {tr('main_opt5')}")
    print(f"  7  →  {tr('main_opt6')}")
    print(f"  8  →  {tr('main_opt7')}")
    print(f"  0  →  {tr('main_opt_quit')}")
    print()

    while True:
        choix = input(tr("main_choice")).strip()
        if choix in ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9"):
            break
        print(tr("main_warn"))

    print()

    while True:
        if choix == "0":
            break
        generated_files = []  # fichiers ciblés pour modes 5 et 6
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
            import RetroBoxLED_gui as gui

            gui.run_gui(sys.modules[__name__], sd_dir)
            return

        # ── Menu de fin ───────────────────────────────────────────────────────
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

        if raw == "0":
            break
        elif raw == "1":
            # Retour au menu principal
            print()
            sep()
            print(f"  {tr('main_title')}")
            sep()
            print()
            print(f"  {tr('main_prompt')}")
            print()
            print(f"  1  →  {tr('main_opt1')}")
            print(
                f"  2  →  Extraction + téléchargement _defaults depuis GitHub (uniquement)"
            )
            print(f"  3  →  {tr('main_opt2')}")
            print(f"  4  →  {tr('main_opt3')}")
            print(f"  5  →  {tr('main_opt4')}")
            print(f"  6  →  {tr('main_opt5')}")
            print(f"  7  →  {tr('main_opt6')}")
            print(f"  8  →  {tr('main_opt7')}")
            print(f"  0  →  {tr('main_opt_quit')}")
            print()
            while True:
                choix = input(tr("main_choice")).strip()
                if choix in ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9"):
                    break
                print(tr("main_warn"))
            print()
            if choix == "0":
                break
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
                    choix = "0"
                elif raw2 == "1":
                    print()
                    sep()
                    print(f"  {tr('main_title')}")
                    sep()
                    print()
                    print(f"  {tr('main_prompt')}")
                    print()
                    print(f"  1  →  {tr('main_opt1')}")
                    print(
                        f"  2  →  Extraction + téléchargement _defaults desde GitHub (uniquement)"
                    )
                    print(f"  3  →  {tr('main_opt2')}")
                    print(f"  4  →  {tr('main_opt3')}")
                    print(f"  5  →  {tr('main_opt4')}")
                    print(f"  6  →  {tr('main_opt5')}")
                    print(f"  7  →  {tr('main_opt6')}")
                    print(f"  8  →  {tr('main_opt7')}")
                    print(f"  0  →  {tr('main_opt_quit')}")
                    print()
                    while True:
                        choix = input(tr("main_choice")).strip()
                        if choix in ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9"):
                            break
                        print(tr("main_warn"))
                    print()
                    if choix == "0":
                        choix = "0"
            else:
                choix = "7"


if __name__ == "__main__":
    import RetroBoxLED_gui as gui
    from pathlib import Path

    script_dir = Path(__file__).parent
    sd_dir = get_sd_card_dir(script_dir)

    toolkit_module = sys.modules[__name__]
    app = gui.RetroBoxLEDGui(toolkit_module, sd_dir)
    app.run()
