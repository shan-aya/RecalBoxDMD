#!/usr/bin/env python3
import queue
from secrets import choice
import sys
import threading
import subprocess
import os
import shutil
import re
import webbrowser
from dataclasses import dataclass
from pathlib import Path
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


UI_TRANSLATIONS = {
    "fr": {
        "sys_all_selected": "Aucun système sélectionné — tous les systèmes seront traités.",
        "sys_sel_opt_all": "Tout sélectionner",
        "sys_sel_opt_none": "Ne rien sélectionner",
        "sys_sel_warn_empty": "Aucun système sélectionné. Cliquez sur « Tout sélectionner » ou sélectionnez au moins un système.",
        "quit_app": "Quitter l'application",
        "quit_app_warning_title": "Quitter et nettoyer",
        "quit_app_warning": "Quitter l'application va supprimer les dossiers temporaires (sd_card). Continuer ?",
        "quit_app_stopped_worker": "Traitement en cours : arrêt demandé avant la fermeture.",
        "quit_app_cleanup_done": "Dossiers temporaires supprimés.",
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
        "start_btn": "Démarrer",
        "detect_systems_btn": "Détection des systèmes (gamelist.xml)",
        "select_images_btn": "Sélection des dossiers images",
        "systems_to_process_lbl": "Systèmes à traiter (clic pour sélectionner)",
        "mode_detail_title": "Détails du mode selectionné",
        "mode6_drives_title": "Lecteurs amovibles (carte SD)",
        "mode6_explore_output_btn": "Explorer le dossier de sortie",
    },
    "en": {
        "sys_all_selected": "No system selected — all systems will be processed.",
        "sys_sel_opt_all": "Select all",
        "sys_sel_opt_none": "Select none",
        "sys_sel_warn_empty": "No system selected. Click « Select all » or select at least one system.",
        "quit_app": "Quit application",
        "quit_app_warning_title": "Quit and cleanup",
        "quit_app_warning": "Quitting will delete temporary folders (sd_card). Continue ?",
        "quit_app_stopped_worker": "Processing is running: stop requested before closing.",
        "quit_app_cleanup_done": "Temporary folders deleted.",
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
        "start_btn": "Start",
        "detect_systems_btn": "Detect systems (gamelist.xml)",
        "select_images_btn": "Select image folders",
        "systems_to_process_lbl": "Systems to process (click to select)",
        "mode_detail_title": "Mode details",
        "mode6_drives_title": "Removable drives (SD card)",
        "mode6_explore_output_btn": "Explore output folder",
    },
    "es": {
        "sys_all_selected": "No se seleccionó ningún sistema — se procesarán todos los sistemas.",
        "sys_sel_opt_all": "Seleccionar todo",
        "sys_sel_opt_none": "No seleccionar",
        "sys_sel_warn_empty": "Ningún sistema seleccionado. Haz clic en « Seleccionar todo » o selecciona al menos un sistema.",
        "quit_app": "Salir de la aplicación",
        "quit_app_warning_title": "Salir y limpiar",
        "quit_app_warning": "Al salir se borrarán las carpetas temporales (sd_card). ¿Continuar?",
        "quit_app_stopped_worker": "Hay un proceso en ejecución: se solicitará la detención antes de cerrar.",
        "quit_app_cleanup_done": "Carpetas temporales eliminadas.",
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
        "start_btn": "Iniciar",
        "detect_systems_btn": "Detectar sistemas (gamelist.xml)",
        "select_images_btn": "Selección de carpetas de imágenes",
        "systems_to_process_lbl": "Sistemas a procesar (clic para seleccionar)",
        "mode_detail_title": "Detalles del modo",
        "mode6_drives_title": "Unidades extraíbles (tarjeta SD)",
        "mode6_explore_output_btn": "Explorar la carpeta de salida",
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
        self._worker: Optional[threading.Thread] = None
        self._sys_list_adjusting = False
        self._last_real_system_indices: set[int] = set()
        self._last_sys_click_index: Optional[int] = None
        # index réel le plus proche cliqué dans la Listbox (0..n-1), y compris index système >=3
        self._last_sys_clicked_index_any: Optional[int] = None

        self.lang_var = tk.StringVar(value="fr")

        # ── Mode 6 (flash) : clignotement + UI lecteurs (sans saisie clavier)
        self._mode6_blinking = False
        self._mode6_blink_job = None
        self._mode6_drives = []
        self._mode6_selected_drive = None
        self._mode6_overwrite = True
        self._mode6_ui_frame = None
        self._mode6_drive_list = None
        self._mode6_btn = None
        self._mode6_flash_thread: Optional[threading.Thread] = None

        self._theme_var = tk.StringVar(value="")
        pref = themes.load_preference()
        if pref:
            chosen = pref
        else:
            chosen = "random"
        self._theme_var.set(chosen)
        themes.apply(chosen if chosen != "random" else themes.random_theme(), self)

        self._build_top_tabs()
        self._build_mode_area(self.tab_main)  # Mode 1 seulement
        self._build_mode_area_advanced(self.tab_advanced)  # Modes 2-7
        self._build_progress_frame(self.root)

        self._poll_logs()

        # Sérigraphie (bas à droite)
        self.silk_label = tk.Label(
            self.root,
            text="GUI - Shan_ayA 2026",
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        self.silk_label.place(relx=1.0, rely=1.0, anchor="se", x=-102, y=-10)

        self.root.protocol("WM_DELETE_WINDOW", self._on_close_attempt)

    # ──────────────────────────────────────────────────────────────────────────
    # TOP TABS (compact logs + parameters)
    # ──────────────────────────────────────────────────────────────────────────
    def _build_top_tabs(self) -> None:
        self.nb_top = ttk.Notebook(self.root)
        self.nb_top.pack(fill="both", expand=True, padx=10, pady=(8, 2))

        self.tab_main = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_advanced = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_logs = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_params = tk.Frame(self.nb_top, bg="#F3F3F3")
        self.tab_help = tk.Frame(self.nb_top, bg="#F3F3F3")

        self.nb_top.add(self.tab_main, text="Main")
        self.nb_top.add(self.tab_advanced, text="Avancé")
        self.nb_top.add(self.tab_logs, text="Logs")
        self.nb_top.add(self.tab_params, text="Paramètres")
        self.nb_top.add(self.tab_help, text="AIDE")

        self._build_logs_tab(self.tab_logs)
        self._build_params_tab(self.tab_params)
        self._build_help_tab(self.tab_help)

    def _build_logs_tab(self, parent: tk.Frame) -> None:
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])

        controls = tk.Frame(parent, bg="#F3F3F3", bd=0)
        controls.pack(fill="x", padx=10, pady=(10, 6))

        self.btn_pause = tk.Button(
            controls,
            text=ui["btn_pause"],
            command=self._on_pause_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
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
            padx=10,
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
            padx=10,
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
            padx=10,
            pady=4,
            font=("TkDefaultFont", 10, "bold"),
        )
        self.btn_stop.grid(row=0, column=3, padx=6, pady=4, sticky="w")

        self.logs_details_title_lbl = tk.Label(
            parent,
            text=ui["logs_details_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self.logs_details_title_lbl.pack(anchor="w", padx=10, pady=(0, 6))

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

        scroll_y = tk.Scrollbar(text_frame, orient="vertical", command=self.text.yview)
        scroll_y.pack(side="right", fill="y")
        self.text.configure(yscrollcommand=scroll_y.set)

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

    def _build_help_tab(self, parent: tk.Frame) -> None:
        self.tab_help_bg = "#F3F3F3"
        container = tk.Frame(parent, bg=self.tab_help_bg)
        container.pack(fill="both", expand=True, padx=10, pady=10)

        # Toolbar: titre + bouton navigateur
        toolbar = tk.Frame(container, bg=self.tab_help_bg)
        toolbar.pack(fill="x", anchor="w")

        self.help_title_lbl = tk.Label(
            toolbar,
            text="AIDE",
            bg=self.tab_help_bg,
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.help_title_lbl.pack(side="left")

        self.help_open_browser_btn = tk.Button(
            toolbar,
            text="Ouvrir dans le navigateur",
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
            self.help_title_lbl.config(text=f"AIDE ({readme_name})")

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
        try:
            self.help_text.configure(state="normal")
            self.help_text.see("anchor_" + anchor)
            self.help_text.configure(state="disabled")
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
            messagebox.showerror("Erreur", f"Impossible d'ouvrir {readme_name} : {e}")

    def _build_mode_area(self, parent: tk.Frame) -> None:
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        outer = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=10, pady=10)
        outer.pack(fill="both", expand=True, padx=10, pady=(10, 8))

        outer.grid_columnconfigure(0, weight=1)
        outer.grid_columnconfigure(1, weight=1)
        outer.grid_columnconfigure(2, weight=1)
        outer.grid_rowconfigure(0, weight=1)

        # Left column: mode + ROMs path + start
        left = tk.Frame(outer, bg="#F3F3F3")
        left.grid(row=0, column=0, sticky="nsew", padx=(0, 10))

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

        self._detail_templates = {
            "fr": {
                "1": "Mode 1 : extrait les images depuis vos gamelists, convertit PNG en 128x32 (et génère raw565) + convertit GIF en raw565pack/meta, construit le cache, télécharge _defaults, puis génère systems_cache.dat.",
                "2": "Mode 2 : télécharge les images “systems/_defaults” depuis GitHub uniquement (aucune extraction ni conversion).",
                "3": "Mode 3 : récupère uniquement les images depuis votre dossier ROMS via gamelist.xml.",
                "4": "Mode 4 : convertit PNG → raw565 et GIF → raw565pack + meta (conversion “raw-only”).",
                "5": "Mode 5 : convertit les images PNG et raw565 en 128x32.",
                "6": "Mode 6 : génère uniquement games_cache.bin (cache jeux).",
                "7": "Mode 7 : génère uniquement systems_cache.dat (index systèmes).",
            },
            "en": {
                "1": "Mode 1: extracts images from your gamelists, converts PNG to 128x32 (and generates raw565) + converts GIF to raw565pack/meta, builds the cache, downloads _defaults, then generates systems_cache.dat.",
                "2": "Mode 2: downloads “systems/_defaults” from GitHub only (no extraction or conversion).",
                "3": "Mode 3: pulls images only from your ROM folder via gamelist.xml.",
                "4": "Mode 4: converts PNG → raw565 and GIF → raw565pack + meta (raw-only conversion).",
                "5": "Mode 5: converts PNG and raw565 images to 128x32.",
                "6": "Mode 6: generates only games_cache.bin (games cache).",
                "7": "Mode 7: generates only systems_cache.dat (systems index).",
            },
            "es": {
                "1": "Modo 1: extrae imágenes desde tus gamelists, convierte PNG a 128x32 (y genera raw565) + convierte GIF a raw565pack/meta, crea la caché, descarga _defaults y luego genera systems_cache.dat.",
                "2": "Modo 2: descarga “systems/_defaults” desde GitHub solo (sin extracción ni conversión).",
                "3": "Modo 3: extrae solo imágenes desde tu carpeta ROMs vía gamelist.xml.",
                "4": "Modo 4: convierte PNG → raw565 y GIF → raw565pack + meta (conversión “raw-only”).",
                "5": "Modo 5: convierte las imágenes PNG y raw565 a 128x32.",
                "6": "Modo 6: genera solo games_cache.bin (caché de juegos).",
                "7": "Modo 7: genera solo systems_cache.dat (índice de sistemas).",
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
                wraplength=240,
                justify="left",
                command=self._on_mode_changed,
            )
            rb.pack(anchor="w", pady=2)
            self._mode_radios[m] = rb

        # Espaceur qui pousse les boutons en bas
        spacer = tk.Frame(left, bg="#F3F3F3")
        spacer.pack(fill="both", expand=True)

        # ROMs folder
        path_box = tk.Frame(left, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8)
        path_box.pack(fill="x", pady=(12, 0))

        self.roms_path_var = tk.StringVar(value="")

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
            wraplength=260,
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

        self.sys_list = tk.Listbox(
            box,
            selectmode="multiple",
            height=7,
            width=28,
            bg="white",
            fg="black",
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
        self.right.grid(row=0, column=2, sticky="nw", padx=(10, 0))

        self.mode_detail_title_lbl = tk.Label(
            self.right,
            text=ui["mode_detail_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_detail_title_lbl.pack(anchor="w", pady=(0, 6))

        self.mode_desc_var = tk.StringVar(value="")
        self.mode_desc_label = tk.Label(
            self.right,
            textvariable=self.mode_desc_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10),
            wraplength=270,
            justify="left",
        )
        self.mode_desc_label.pack(anchor="w", fill="x")

        # ── Mode 6 panel (hidden until previous pipeline is finished)
        self._mode6_ui_frame = tk.Frame(
            self.right, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8
        )
        self._mode6_ui_frame.pack(fill="x", pady=(12, 0))
        self._mode6_ui_frame.pack_forget()

        self._mode6_panel_title_lbl = tk.Label(
            self._mode6_ui_frame,
            text=UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])[
                "mode6_panel_title"
            ],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        )
        self._mode6_panel_title_lbl.pack(anchor="w")

        self._mode6_drives_title_lbl = tk.Label(
            self._mode6_ui_frame,
            text=ui["mode6_drives_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
        )
        self._mode6_drives_title_lbl.pack(anchor="w", pady=(8, 4))

        drives_box = tk.Frame(self._mode6_ui_frame, bg="#F3F3F3")
        drives_box.pack(fill="x")

        self._mode6_drive_list = tk.Listbox(
            drives_box,
            selectmode="browse",
            height=5,
            width=26,
            bg="white",
            fg="black",
            borderwidth=3,
            relief="solid",
        )
        self._mode6_drive_list.pack(side="left", fill="both", expand=True)

        scroll = tk.Scrollbar(
            drives_box, orient="vertical", command=self._mode6_drive_list.yview
        )
        scroll.pack(side="right", fill="y")
        self._mode6_drive_list.configure(yscrollcommand=scroll.set)

        self._mode6_overwrite_var = tk.BooleanVar(value=True)
        self._mode6_overwrite_title_lbl = tk.Label(
            self._mode6_ui_frame,
            text=UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])[
                "mode6_overwrite_title"
            ],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        )
        self._mode6_overwrite_title_lbl.pack(anchor="w", pady=(10, 4))

        opt_box = tk.Frame(self._mode6_ui_frame, bg="#F3F3F3")
        opt_box.pack(anchor="w")

        self._mode6_rb_overwrite_yes = tk.Radiobutton(
            opt_box,
            text=UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])[
                "mode6_overwrite_yes"
            ],
            variable=self._mode6_overwrite_var,
            value=True,
            bg="#F3F3F3",
            fg="black",
            activebackground="#E7E7E7",
            font=("TkDefaultFont", 9, "bold"),
            anchor="w",
        )
        self._mode6_rb_overwrite_yes.pack(anchor="w")

        self._mode6_rb_overwrite_no = tk.Radiobutton(
            opt_box,
            text=UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])[
                "mode6_overwrite_no"
            ],
            variable=self._mode6_overwrite_var,
            value=False,
            bg="#F3F3F3",
            fg="black",
            activebackground="#E7E7E7",
            font=("TkDefaultFont", 9, "bold"),
            anchor="w",
        )
        self._mode6_rb_overwrite_no.pack(anchor="w")

        self._mode6_btn = tk.Button(
            self._mode6_ui_frame,
            text=UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])[
                "mode6_btn_start"
            ],
            command=self._on_mode6_button_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 11, "bold"),
            state="disabled",
        )
        self._mode6_btn.pack(fill="x", pady=(10, 0))

        # Bouton "Explorer le dossier de sortie" (activé une fois la copie terminée)
        self._mode6_explore_output_btn = tk.Button(
            self._mode6_ui_frame,
            text=ui["mode6_explore_output_btn"],
            command=lambda: os.startfile(  # type: ignore[attr-defined]
                str(self._final_output_dir if self._final_output_dir else self.sd_dir)
            ),
            bg="#FFFFFF",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
            pady=6,
            font=("TkDefaultFont", 10, "bold"),
            state="disabled",
        )
        self._mode6_explore_output_btn.pack(fill="x", pady=(0, 6))

        self._on_mode_changed()

    def _build_mode_area_advanced(self, parent: tk.Frame) -> None:
        """Onglet Avancé : modes 2 a 7."""
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        outer = tk.Frame(parent, bg="#F3F3F3", bd=2, relief="solid", padx=10, pady=10)
        outer.pack(fill="both", expand=True, padx=10, pady=(10, 8))
        outer.grid_columnconfigure(0, weight=1)
        outer.grid_columnconfigure(1, weight=1)
        outer.grid_columnconfigure(2, weight=1)
        outer.grid_rowconfigure(0, weight=1)
        left = tk.Frame(outer, bg="#F3F3F3")
        left.grid(row=0, column=0, sticky="nw", padx=(0, 10))
        self.mode_title_lbl_adv = tk.Label(
            left,
            text=ui["mode_label"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_title_lbl_adv.pack(anchor="w")
        self._mode_radios_adv: dict[str, tk.Radiobutton] = {}
        modes = [
            ("2", self.tkmod.tr("mode2_title")),
            ("3", self.tkmod.tr("mode3_title")),
            ("4", self.tkmod.tr("mode4_title")),
            ("5", self.tkmod.tr("mode5_title")),
            ("6", self.tkmod.tr("mode6_title")),
            ("7", self.tkmod.tr("sysc_title")),
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
                wraplength=240,
                justify="left",
                command=self._on_mode_changed,
            )
            rb.pack(anchor="w", pady=2)
            self._mode_radios_adv[m] = rb
        # Espaceur qui pousse les boutons en bas
        spacer = tk.Frame(left, bg="#F3F3F3")
        spacer.pack(fill="both", expand=True)
        path_box = tk.Frame(left, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8)
        path_box.pack(fill="x", pady=(12, 0))
        self.roms_path_var_adv = tk.StringVar(value="")
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
            wraplength=260,
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
        self.sys_list_adv = tk.Listbox(
            box,
            selectmode="multiple",
            height=7,
            width=28,
            bg="white",
            fg="black",
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
        self.right_adv.grid(row=0, column=2, sticky="nw", padx=(10, 0))
        self.mode_detail_title_lbl_adv = tk.Label(
            self.right_adv,
            text=ui["mode_detail_title"],
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 12, "bold"),
        )
        self.mode_detail_title_lbl_adv.pack(anchor="w", pady=(0, 6))
        self.mode_desc_var_adv = tk.StringVar(value="")
        self.mode_desc_label_adv = tk.Label(
            self.right_adv,
            textvariable=self.mode_desc_var_adv,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10),
            wraplength=270,
            justify="left",
        )
        self.mode_desc_label_adv.pack(anchor="w", fill="x")

        # Initialiser la description pour le mode par défaut
        self._update_mode_desc()

    # language + mode logic
    # ---------------------------------------------------------
    def _set_toolkit_language(self, lang: str) -> None:
        # toolkit has TRANSLATIONS + global T
        try:
            self.tkmod.T = self.tkmod.TRANSLATIONS[lang]
        except Exception:
            return

    def _on_language_changed(self) -> None:
        lang = self.lang_var.get()
        if lang not in ("fr", "en", "es"):
            return
        self._set_toolkit_language(lang)

        ui = self._get_ui_t()

        # update titles for radios (toolkit) - Main tab
        self._mode_radios["1"].config(text=self.tkmod.tr("mode1_title"))
        # update titles for radios - Advanced tab
        if hasattr(self, "_mode_radios_adv"):
            self._mode_radios_adv["2"].config(text=self.tkmod.tr("mode2_title"))
            self._mode_radios_adv["3"].config(text=self.tkmod.tr("mode3_title"))
            self._mode_radios_adv["4"].config(text=self.tkmod.tr("mode4_title"))
            self._mode_radios_adv["5"].config(text=self.tkmod.tr("mode5_title"))
            self._mode_radios_adv["6"].config(text=self.tkmod.tr("mode6_title"))
            self._mode_radios_adv["7"].config(text=self.tkmod.tr("sysc_title"))

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

        if getattr(self, "_mode6_drives_title_lbl", None):
            self._mode6_drives_title_lbl.config(text=ui["mode6_drives_title"])

        if getattr(self, "_mode6_explore_output_btn", None):
            self._mode6_explore_output_btn.config(text=ui["mode6_explore_output_btn"])

        # mode details / titre panneau
        if getattr(self, "mode_detail_title_lbl", None):
            self.mode_detail_title_lbl.config(text=ui["mode_detail_title"])

        # update description + mode6 texts
        self._update_mode_desc()
        self._sync_mode6_texts()

        # refresh help tab (README language + links)
        self._refresh_help_tab_content()

    def _update_mode_desc(self) -> None:
        mode = self.mode_var.get()
        lang = self.lang_var.get()
        text = self._detail_templates.get(lang, self._detail_templates["fr"]).get(
            mode, ""
        )
        self.mode_desc_var.set(text)
        if hasattr(self, "mode_desc_var_adv"):
            self.mode_desc_var_adv.set(text)

    def _on_mode_changed(self) -> None:
        mode = self.mode_var.get()
        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])

        # Libellés spécifiques
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

        if mode in ("1", "3", "4", "5"):
            self.middle.grid()
            self.btn_detect_systems.config(state="normal")
            # auto-detect if possible
            self._maybe_autodetect_systems()
        else:
            self.middle.grid_remove()

        self._update_mode_desc()

    def _maybe_autodetect_systems(self) -> None:
        path_str = self.roms_path_var.get().strip()
        if not path_str:
            return
        roms_root = Path(path_str)
        if not roms_root.exists():
            return

        ui = UI_TRANSLATIONS.get(self.lang_var.get(), UI_TRANSLATIONS["fr"])
        is_unc = str(roms_root).startswith("\\\\")

        try:
            if self.mode_var.get() in ("1", "3"):
                systems = self._find_systems(roms_root)
            else:
                # En mode 4/5, roms_root pointe le dossier "images" libre choisi par l'utilisateur.
                # Donc on ne force plus un sous-dossier /images.
                systems = self._find_systems_images(roms_root)
        except Exception:
            if is_unc:
                self._show_credentials_if_needed(True)
                messagebox.showwarning(
                    "Attention",
                    "Accès réseau impossible (lecture gamelist.xml ou images). Renseignez NAS user/mdp, puis cliquez « Détecter systèmes ».",
                )
                return
            raise

        self.sys_list.delete(0, "end")

        # 0="Tout sélectionner", 1="Ne rien sélectionner", 2="" separator
        # Note: tk.Listbox ne supporte pas itemconfig(font=...), donc on ne force pas l'italique ici.
        self.sys_list.insert("end", ui["sys_sel_opt_all"])
        self.sys_list.insert("end", ui["sys_sel_opt_none"])
        # Ne pas insérer "" : certains thèmes/implémentations peuvent rendre la hauteur peu fiable.
        self.sys_list.insert("end", " ")

        for sys_path in systems:
            self.sys_list.insert("end", sys_path.name)

        # Garantir l'affichage depuis le haut (sinon on peut “ne voir” qu'un seul item).
        try:
            self.sys_list.yview_moveto(0.0)
        except Exception:
            pass

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

    def _on_sys_list_button1_clicked(self, _event: object = None) -> None:
        # capture l'intention (index cliqué: 0/1/2) et la sélection "réelle" (indices >= 3 : systèmes)
        self._last_sys_click_index = None
        self._last_sys_clicked_index_any = None
        try:
            if _event is not None and hasattr(_event, "y"):
                click_index = self.sys_list.nearest(getattr(_event, "y"))
                if isinstance(click_index, int):
                    self._last_sys_clicked_index_any = click_index
                    if click_index in (0, 1, 2):
                        self._last_sys_click_index = click_index
        except Exception:
            self._last_sys_click_index = None
            self._last_sys_clicked_index_any = None

        try:
            selected = list(self.sys_list.curselection())
            self._last_real_system_indices = {i for i in selected if i >= 3}
        except Exception:
            self._last_real_system_indices = set()

    def _on_systems_listbox_select_changed(self, _event: object = None) -> None:
        if self._sys_list_adjusting:
            return

        total_items = self.sys_list.size()
        # Pas de place pour sentinelles + séparateur + systèmes
        if total_items <= 3:
            return

        try:
            self._sys_list_adjusting = True

            selected = list(self.sys_list.curselection())
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
                self.sys_list.selection_clear(0)
                self.sys_list.selection_clear(1)
                self.sys_list.selection_clear(2)
                self._last_sys_click_index = None
                self._last_sys_clicked_index_any = None
                return

            # Cas délicat : au clic, Tk peut laisser transitoirement 0 et 1 sélectionnés.
            # On tranche alors selon l'index réellement cliqué (via _on_sys_list_button1_clicked).
            if 0 in selected_set and 1 in selected_set:
                intent = self._last_sys_click_index
                if intent == 0:
                    # "Tout sélectionner"
                    self.sys_list.selection_clear(0, "end")
                    self.sys_list.selection_set(0)
                    self.sys_list.selection_clear(1)
                    self.sys_list.selection_clear(2)
                    for i in range(systems_start, systems_end_exclusive):
                        self.sys_list.selection_set(i)
                    self._last_sys_click_index = None
                    self._last_sys_clicked_index_any = None
                    return

                if intent == 1:
                    # "Ne rien sélectionner"
                    self.sys_list.selection_clear(0, "end")
                    self.sys_list.selection_set(1)
                    self.sys_list.selection_clear(0)
                    self.sys_list.selection_clear(2)
                    self._last_sys_click_index = None
                    self._last_sys_clicked_index_any = None
                    return

                # Sinon (intention None ou clic sur une ligne système) => mode manuel :
                self.sys_list.selection_clear(0)
                self.sys_list.selection_clear(1)
                self.sys_list.selection_clear(2)
                self._last_sys_click_index = None
                self._last_sys_clicked_index_any = None
                return

            if 1 in selected_set:
                # "Ne rien sélectionner"
                self.sys_list.selection_clear(0, "end")
                self.sys_list.selection_set(1)
                self.sys_list.selection_clear(0)
                self.sys_list.selection_clear(2)
                self._last_sys_click_index = None
                return

            if 0 in selected_set:
                # "Tout sélectionner"
                self.sys_list.selection_clear(0, "end")
                self.sys_list.selection_set(0)
                self.sys_list.selection_clear(1)
                self.sys_list.selection_clear(2)
                for i in range(systems_start, systems_end_exclusive):
                    self.sys_list.selection_set(i)
                self._last_sys_click_index = None
                return

            # Sélection manuelle : on retire sentinelles 0/1 (et on s'assure que la ligne vide n'est pas sélectionnée)
            self.sys_list.selection_clear(0)
            self.sys_list.selection_clear(1)
            self.sys_list.selection_clear(2)

            # Si l'utilisateur a sélectionné tous les systèmes individuellement => on met "Tout sélectionner"
            real_selected = [i for i in selected if i >= systems_start]
            if len(real_selected) == total_systems and total_systems > 0:
                self.sys_list.selection_set(0)

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
        self.roms_path_var.set(p)

        is_unc = str(p).startswith("\\\\")

        # On essaie de peupler la liste des systèmes directement.
        # Si l'accès réseau échoue (gamelist.xml), on affichera user/mdp plus tard.
        if self.mode_var.get() in ("1", "2", "3", "4", "5"):
            self._on_mode_changed()

    def _get_roms_root_or_warn(self) -> Optional[Path]:
        path_str = self.roms_path_var.get().strip()
        if not path_str:
            messagebox.showwarning("Attention", "Choisis un dossier ROMs d'abord.")
            return None
        roms_root = Path(path_str)
        if not roms_root.exists():
            messagebox.showwarning("Attention", "Dossier ROMs introuvable.")
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
                    "Attention",
                    "Accès réseau impossible (lecture gamelist.xml ou images). Renseignez NAS user/mdp, puis relancez « Détecter systèmes ».",
                )
                return
            raise

        self.sys_list.delete(0, "end")

        # 0="Tout sélectionner", 1="Ne rien sélectionner", 2="" separator
        # Note: tk.Listbox ne supporte pas itemconfig(font=...), donc on ne force pas l'italique ici.
        self.sys_list.insert("end", ui["sys_sel_opt_all"])
        self.sys_list.insert("end", ui["sys_sel_opt_none"])
        # Ne pas insérer "" : certains thèmes/implémentations peuvent rendre la hauteur peu fiable.
        self.sys_list.insert("end", " ")

        for sys_path in systems:
            self.sys_list.insert("end", sys_path.name)

        # Garantir l'affichage depuis le haut (sinon on peut “ne voir” qu'un seul item).
        try:
            self.sys_list.yview_moveto(0.0)
        except Exception:
            pass

    # ──────────────────────────────────────────────────────────────────────────
    # NAS credentials (only if UNC at start)
    # ──────────────────────────────────────────────────────────────────────────
    def _ensure_credentials_widgets(self) -> None:
        if hasattr(self, "creds_frame"):
            return

        # credentials panel under the mode details (right panel)
        self.creds_frame = tk.Frame(
            self.right, bg="#F3F3F3", bd=2, relief="solid", padx=8, pady=8
        )
        self.creds_frame.pack(fill="x", pady=(12, 0))

        tk.Label(
            self.creds_frame,
            text="Identifiants NAS (UNC)",
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 11, "bold"),
        ).grid(row=0, column=0, columnspan=2, sticky="w")

        tk.Label(
            self.creds_frame,
            text="NAS user",
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        ).grid(row=1, column=0, sticky="w", pady=(8, 2))

        self.ent_nas_user = tk.Entry(self.creds_frame, width=22)
        self.ent_nas_user.grid(row=2, column=0, sticky="w", pady=2)

        tk.Label(
            self.creds_frame,
            text="NAS password",
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
        ).grid(row=1, column=1, sticky="w", pady=(8, 2), padx=(10, 0))

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

        # Largeur FIXE (empêche Tk d'élargir la colonne si le texte est long)
        tk.Label(
            frm,
            textvariable=self.progress_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 10, "bold"),
            width=55,
            wraplength=9999,
            justify="left",
            anchor="w",
        ).grid(row=1, column=0, sticky="w", pady=4)

        tk.Label(
            frm,
            textvariable=self.progress_sub_var,
            bg="#F3F3F3",
            fg="black",
            font=("TkDefaultFont", 9, "bold"),
            width=55,
            # Ne jamais wrapper : sinon la hauteur bouge et repousse la barre/boutons.
            wraplength=9999,
            justify="left",
            anchor="w",
        ).grid(row=2, column=0, sticky="w")

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

        self.btn_pause_progress = tk.Button(
            controls,
            text=ui["btn_pause"],
            command=self._on_pause_clicked,
            bg="#FFD400",
            fg="black",
            bd=2,
            relief="solid",
            padx=10,
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
            padx=10,
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
            padx=10,
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
            padx=10,
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
    def _poll_logs(self) -> None:
        try:
            while True:
                line = self._log_q.get_nowait()
                self._append_log(line)
        except queue.Empty:
            pass
        self.root.after(100, self._poll_logs)

    def _append_log(self, s: str) -> None:
        self.text.insert("end", s)
        self.text.see("end")
        try:
            if int(self.text.index("end-1c").split(".")[0]) > 400:
                self.text.delete("1.0", "200.0")
        except Exception:
            pass

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
            messagebox.showerror("Erreur", str(e))

    # ──────────────────────────────────────────────────────────────────────────
    # worker
    # ──────────────────────────────────────────────────────────────────────────
    def _on_start_clicked(self) -> None:
        if self._worker and self._worker.is_alive():
            messagebox.showwarning("Attention", "Traitement déjà en cours.")
            return

        mode = self.mode_var.get()

        # Mode 7 : n'a pas besoin de roms_root (index systems/_defaults uniquement)
        if mode == "7":
            roms_root = self._get_roms_root_or_warn()
            if roms_root is None:
                return
            is_unc = False
        else:
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
        if mode in ("1", "3", "4", "5"):
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
                messagebox.showwarning(
                    "Attention",
                    "Aucun système détecté (dossiers images introuvables).",
                )
                return

            selected_indices = list(self.sys_list.curselection())

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
                messagebox.showwarning("Attention", ui["sys_sel_warn_empty"])
                try:
                    self.sys_list.focus_set()
                except Exception:
                    pass
                return

        # Pour les autres modes, on conserve le comportement précédent (mais sans forcer NAS user/mdp ici).
        cfg = GuiConfig(
            mode_choice=mode,
            roms_root=roms_root,
            systems_selected=cast(Optional[Sequence[Path]], systems_selected),
            nas_user=nas_user,
            nas_password=nas_password,
            nas_path_is_unc=(is_unc and connect_unc_for_worker),
        )

        # Basculer sur l'onglet Logs au démarrage du traitement
        self.nb_top.select(self.tab_logs)

        self.progress_var.set("Démarrage...")
        self.progress.configure(maximum=100, value=0)
        self.text.delete("1.0", "end")

        self._worker = threading.Thread(
            target=self._worker_main, args=(cfg,), daemon=True
        )
        self._worker.start()

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
            elif mode == "7":
                self._pipeline_mode_7(toolkit, cfg)
            else:
                print(f"Mode inconnu: {mode}")

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

        systems_out = sd_dir / "systems"
        if systems_out.exists():
            shutil.rmtree(systems_out)
        systems_out.mkdir(parents=True, exist_ok=True)

        tag_configs = [("logo", ""), ("image", ""), ("thumbnail", "")]
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
        )

        if toolkit.PAUSE.should_stop():
            print("[GUI] Stop demandé.")
            return
        if toolkit.PAUSE.should_skip():
            toolkit.PAUSE.request_resume()

        sysc_out = sd_dir / "systems_cache.dat"
        toolkit.build_systems_cache(
            systems_out, sysc_out, progress_cb=self._progress_cb
        )

        print("[GUI] DONE mode 1")

    def _pipeline_mode_2(self, toolkit, cfg: GuiConfig) -> None:
        sd_dir = self.sd_dir
        sd_dir.mkdir(parents=True, exist_ok=True)
        toolkit.prepare_sd_card(sd_dir, interactive=False)

        systems_out = sd_dir / "systems"
        if systems_out.exists():
            shutil.rmtree(systems_out)
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
        )

        print("[GUI] DONE mode 2 (extract + download _defaults)")

    def _pipeline_mode_3(self, toolkit, cfg: GuiConfig) -> None:
        sd_dir = self.sd_dir
        sd_dir.mkdir(parents=True, exist_ok=True)
        toolkit.prepare_sd_card(sd_dir, interactive=False)

        systems_out = sd_dir / "systems"
        if systems_out.exists():
            shutil.rmtree(systems_out)
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
    # mode 7 + fin de traitement + actions finaux
    # ──────────────────────────────────────────────────────────────────────────
    def _get_ui_t(self) -> dict[str, str]:
        lang = self.lang_var.get()
        return UI_TRANSLATIONS.get(lang, UI_TRANSLATIONS["fr"])

    def _cleanup_sd_dir(self) -> None:
        try:
            if self.sd_dir.exists():
                shutil.rmtree(self.sd_dir)
        except Exception:
            pass

    def _refresh_mode6_drives(self) -> None:
        try:
            drives = self.tkmod._list_removable_drives()  # type: ignore[attr-defined]
        except Exception:
            drives = []
        self._mode6_drives = list(drives)
        self._mode6_drive_list.delete(0, "end")
        for i, (letter, label, size) in enumerate(self._mode6_drives):
            self._mode6_drive_list.insert(
                "end", f"{i+1} → {letter}\\\\  [{label}]  {size}"
            )
        if self._mode6_drives:
            self._mode6_drive_list.selection_set(0)
        else:
            # keep empty
            pass

    def _sync_mode6_texts(self) -> None:
        ui = self._get_ui_t()

        if self._mode6_panel_title_lbl:
            self._mode6_panel_title_lbl.config(text=ui["mode6_panel_title"])

        if self._mode6_overwrite_title_lbl:
            self._mode6_overwrite_title_lbl.config(text=ui["mode6_overwrite_title"])

        if getattr(self, "_mode6_rb_overwrite_yes", None):
            self._mode6_rb_overwrite_yes.config(text=ui["mode6_overwrite_yes"])

        if getattr(self, "_mode6_rb_overwrite_no", None):
            self._mode6_rb_overwrite_no.config(text=ui["mode6_overwrite_no"])

        if self._mode6_btn:
            self._mode6_btn.config(text=ui["mode6_btn_start"])

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
        # Déterminer le label "Aléatoire" selon la langue
        lang = self.lang_var.get()
        if lang == "en":
            random_lbl = "Random"
        elif lang == "es":
            random_lbl = "Aleatorio"
        else:
            random_lbl = "Aléatoire"
        if choice in (random_lbl, "random"):
            choice = themes.random_theme()
            self._theme_var.set(choice)
        if choice:
            themes.save_preference(choice)
            themes.apply(choice, self)

    def _start_mode6_blinking(self) -> None:
        if not self._mode6_ui_frame or not self._mode6_btn:
            return
        if not self._mode6_ui_frame.winfo_ismapped():
            self._mode6_ui_frame.pack(fill="x", pady=(12, 0))
        self._mode6_blinking = True

        self._refresh_mode6_drives()
        self._mode6_btn.config(state="normal")
        self._mode6_btn.config(text=self._get_ui_t()["mode6_btn_start"])

        def _tick() -> None:
            if not self._mode6_blinking:
                return
            # toggle bg for blink
            current = self._mode6_btn.cget("bg")
            new_bg = "#FFFFFF" if current != "#FFFFFF" else "#FFD400"
            self._mode6_btn.config(bg=new_bg)
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
        if self._mode6_btn:
            self._mode6_btn.config(bg="#FFD400")

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

    def _on_pipeline_finished(self) -> None:
        # Revenir sur l'onglet Main quand le traitement est fini
        self.nb_top.select(self.tab_main)
        # habilite mode 6 + clignotement une fois traitement terminé
        self._start_mode6_blinking()
        # Activer aussi "Explorer le dossier de sortie" dès que les fichiers existent
        if getattr(self, "_mode6_explore_output_btn", None):
            try:
                self._mode6_explore_output_btn.config(state="normal")
            except Exception:
                pass

    def _on_mode6_button_clicked(self) -> None:
        # stop blinking and start flash (non-interactif)
        if self._mode6_blinking:
            self._stop_mode6_blinking()

        if self._mode6_btn:
            self._mode6_btn.config(
                state="disabled", text=self._get_ui_t()["mode6_btn_running"]
            )

        # determine chosen drive
        chosen_index: int | None = None
        try:
            sel = list(self._mode6_drive_list.curselection())
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
            messagebox.showwarning("Attention", self._get_ui_t()["mode6_no_drives"])
            if self._mode6_btn:
                self._mode6_btn.config(state="normal")
            return

        letter, _label, _size = self._mode6_drives[chosen_index]
        dst_drive = f"{letter}\\\\"
        overwrite = bool(self._mode6_overwrite_var.get())

        self._mode6_flash_thread = threading.Thread(
            target=self._mode6_flash_worker,
            args=(dst_drive, overwrite),
            daemon=True,
        )
        self._mode6_flash_thread.start()

    def _mode6_flash_worker(self, dst_drive: str, overwrite: bool) -> None:
        try:
            # non-interactive: use toolkit internal helpers
            # Mode 7: on anime la barre de progression pendant la copie SD (robocopy).
            self.root.after(
                0,
                lambda: (
                    self.progress.configure(mode="indeterminate"),
                    self.progress.start(10),
                    self.progress_pct_var.set("…"),
                ),
            )

            self.tkmod._robocopy(self.sd_dir, dst_drive, overwrite)  # type: ignore[attr-defined]

            # Copie terminée -> barre à 100%
            self.root.after(
                0,
                lambda: (
                    self.progress.stop(),
                    self.progress.configure(mode="determinate"),
                    self.progress.configure(value=100),
                    self.progress_pct_var.set("100%"),
                ),
            )
        except Exception as e:
            print(f"[GUI] Mode6 flash error: {e}")
        finally:
            self.root.after(0, self._on_mode6_flash_done)

    def _on_mode6_flash_done(self) -> None:
        ui = self._get_ui_t()
        if self._mode6_btn:
            self._mode6_btn.config(state="disabled", text=ui["mode6_done"])
            if getattr(self, "_mode6_explore_output_btn", None):
                self._mode6_explore_output_btn.config(state="normal")

            # Optionally open output folder
        out_dir = self.sd_dir

        if messagebox.askyesno(ui["open_output_prompt"], ui["open_output_prompt"]):
            try:
                os.startfile(str(out_dir))  # type: ignore[attr-defined]
            except Exception:
                pass
        # no auto re-blink

    def _on_quit_app_clicked(self) -> None:
        ui = self._get_ui_t()
        # Remplace messagebox.askokcancel par une popup custom avec bouton "Explorer"
        result_holder: dict[str, bool] = {"ok": False}

        lang = self.lang_var.get()
        cancel_lbl = "Annuler" if lang != "en" else "Cancel"
        cancel_lbl = "Cancelar" if lang == "es" else cancel_lbl
        ok_lbl = "OK"

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["quit_app_warning_title"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()

        lbl = tk.Label(
            dlg,
            text=ui["quit_app_warning"],
            justify="left",
            padx=14,
            pady=12,
        )
        lbl.pack(fill="both", expand=True)

        btns = tk.Frame(dlg, padx=10, pady=10)
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
            try:
                dlg.destroy()
            except Exception:
                pass

        explore_btn = tk.Button(
            btns,
            text=ui.get("mode6_explore_output_btn", "Explorer le dossier de sortie"),
            width=28,
            command=explore_output_dir,
        )
        explore_btn.grid(row=0, column=0, padx=6)

        b_cancel = tk.Button(btns, text=cancel_lbl, width=14, command=on_cancel)
        b_cancel.grid(row=0, column=1, padx=6)

        b_ok = tk.Button(btns, text=ok_lbl, width=10, command=on_ok)
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
        flash_alive = bool(
            self._mode6_flash_thread and self._mode6_flash_thread.is_alive()
        )

        if worker_alive or flash_alive:
            dlg = tk.Toplevel(self.root)
            dlg.title(ui["quit_app_warning_title"])
            dlg.resizable(False, False)
            dlg.transient(self.root)
            dlg.grab_set()

            msg = (
                ui["quit_app_stopped_worker"]
                if worker_alive
                else ui["quit_app_warning"]
            )
            lbl = tk.Label(dlg, text=msg, justify="left", padx=14, pady=12)
            lbl.pack(fill="both", expand=True)

            btns = tk.Frame(dlg, padx=10, pady=10)
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

            b_cancel = tk.Button(btns, text=cancel_lbl, width=14, command=on_cancel)
            b_cancel.grid(row=0, column=1, padx=6)

            ok_btn = tk.Button(btns, text="OK", width=10, command=on_ok)
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

        # No running work -> cleanup now
        self._cleanup_sd_dir()

        dlg = tk.Toplevel(self.root)
        dlg.title(ui["quit_app_warning_title"])
        dlg.resizable(False, False)
        dlg.transient(self.root)
        dlg.grab_set()

        self._center_toplevel(dlg)

        lbl = tk.Label(
            dlg,
            text=ui["quit_app_cleanup_done"],
            justify="left",
            padx=14,
            pady=12,
        )
        lbl.pack(fill="both", expand=True)

        btns = tk.Frame(dlg, padx=10, pady=10)
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

        ok_btn = tk.Button(btns, text="OK", width=10, command=on_ok)
        ok_btn.grid(row=0, column=1, padx=6)

    def _wait_for_threads_then_exit(self) -> None:
        worker_alive = bool(self._worker and self._worker.is_alive())
        flash_alive = bool(
            self._mode6_flash_thread and self._mode6_flash_thread.is_alive()
        )

        if worker_alive or flash_alive:
            self.root.after(300, self._wait_for_threads_then_exit)
            return

        self._cleanup_sd_dir()
        try:
            self.root.destroy()
        except Exception:
            pass

    # ──────────────────────────────────────────────────────────────────────────
    # close
    # ──────────────────────────────────────────────────────────────────────────
    def _on_close_attempt(self) -> None:
        ui = self._get_ui_t()
        lang = self.lang_var.get()
        cancel_lbl = "Annuler" if lang != "en" else "Cancel"
        cancel_lbl = "Cancelar" if lang == "es" else cancel_lbl
        ok_lbl = "OK"

        def explore_output_dir() -> None:
            try:
                os.startfile(str(self.sd_dir))  # type: ignore[attr-defined]
            except Exception:
                pass

        # Dialog modale (au lieu de messagebox.askokcancel) pour ajouter le bouton "explorer"
        if self._worker and self._worker.is_alive():
            dlg = tk.Toplevel(self.root)
            dlg.title(ui["quit_app_warning_title"])
            dlg.resizable(False, False)
            dlg.transient(self.root)
            dlg.grab_set()

            msg = (
                "Traitement en cours.\n\n"
                "Clique « Annuler » pour garder la fenêtre ouverte.\n"
                "Clique « OK » pour demander l’arrêt du script et fermer."
            )

            lbl = tk.Label(dlg, text=msg, justify="left", padx=14, pady=12)
            lbl.pack(fill="both", expand=True)

            btns = tk.Frame(dlg, padx=10, pady=10)
            btns.pack()

            def on_cancel() -> None:
                try:
                    dlg.destroy()
                except Exception:
                    pass

            def on_explore() -> None:
                explore_output_dir()

            def on_ok() -> None:
                try:
                    self.tkmod.PAUSE.request_stop()
                except Exception:
                    pass
                try:
                    dlg.destroy()
                except Exception:
                    pass
                try:
                    self.root.destroy()
                except Exception:
                    pass

            b_cancel = tk.Button(btns, text=cancel_lbl, width=14, command=on_cancel)
            b_cancel.grid(row=0, column=0, padx=6)

            b_explore = tk.Button(
                btns,
                text=ui.get("mode6_explore_output_btn", "Explorer dossier de sortie"),
                width=24,
                command=on_explore,
            )
            b_explore.grid(row=0, column=1, padx=6)

            b_ok = tk.Button(btns, text=ok_lbl, width=10, command=on_ok)
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
            return

        self.root.destroy()

    def run(self) -> None:
        self.root.mainloop()


def run_gui(toolkit_module, sd_dir: Path) -> None:
    RetroBoxLEDGui(toolkit_module, sd_dir).run()


if __name__ == "__main__":
    import RetroBoxLED_toolkit as toolkit  # type: ignore

    sd = toolkit.get_sd_card_dir(Path(__file__).parent)
    RetroBoxLEDGui(toolkit, sd).run()
