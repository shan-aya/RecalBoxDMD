"""
RecalBoxDMD_themes.py — Système de thèmes pour l'interface GUI.

Chaque thème est défini dans tools/themes/<nom>/theme.py
et peut inclure une image de fond tools/themes/<nom>/bg.png

Fonctions exportées :
  list_themes() -> list[str]
  get_theme(name) -> dict
  apply(name, gui_instance) -> None
  random_theme(exclude: list[str] = None) -> str
  load_preference() -> str | None
  save_preference(name: str) -> None
"""

import os
import sys
import random
import tkinter as tk
from tkinter import ttk
from pathlib import Path

# Dossier racine des thèmes
THEMES_DIR = Path(__file__).parent / "themes"

# Cache des thèmes chargés
_themes_cache: dict[str, dict] = {}

# Fichier de préférence
PREF_FILE = Path(__file__).parent / "theme_pref.txt"


def list_themes() -> list[str]:
    """Retourne la liste des noms de thèmes disponibles."""
    if not THEMES_DIR.exists():
        return ["default"]
    themes = []
    for d in sorted(THEMES_DIR.iterdir()):
        if d.is_dir() and (d / "theme.py").exists():
            themes.append(d.name)
    return themes if themes else ["default"]


def get_theme(name: str) -> dict:
    """Charge et retourne le dictionnaire THEME du thème donné."""
    if name in _themes_cache:
        return _themes_cache[name]

    theme_path = THEMES_DIR / name / "theme.py"
    if not theme_path.exists():
        # Fallback sur default
        theme_path = THEMES_DIR / "default" / "theme.py"
        if not theme_path.exists():
            return _default_theme_dict()
        name = "default"

    try:
        import importlib.util

        spec = importlib.util.spec_from_file_location(f"theme_{name}", theme_path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        theme = getattr(mod, "THEME", _default_theme_dict())
        # Ajouter le bg_path si bg.png existe
        bg_path = theme_path.parent / "bg.png"
        if bg_path.exists():
            theme["bg_path"] = str(bg_path)
        _themes_cache[name] = theme
        return theme
    except Exception:
        return _default_theme_dict()


def _default_theme_dict() -> dict:
    """Thème par défaut (blanc/gris actuel)."""
    return {
        "name": "default",
        "author": "RecalBoxDMD",
        "version": "1.0",
        "colors": {
            "bg_main": "#F3F3F3",
            "bg_frame": "#FFFFFF",
            "fg_text": "#000000",
            "fg_text_light": "#555555",
            "fg_heading": "#000000",
            "bg_button_start": "#00D084",
            "fg_button_start": "#000000",
            "bg_button_danger": "#FF5C5C",
            "bg_button_action": "#FFD400",
            "bg_button_normal": "#FFFFFF",
            "bg_listbox": "#FFFFFF",
            "fg_listbox": "#000000",
            "bg_text": "#FFFFFF",
            "fg_textbox": "#000000",
            "bg_tab": "#F0F0F0",
            "bg_tab_selected": "#FFFFFF",
            "bg_progress": "#E0E0E0",
            "fg_progress": "#00D084",
            "bg_silk": "#F3F3F3",
            "fg_silk": "#000000",
            "bg_notebook": "#F3F3F3",
        },
        "fonts": {
            "default_size": 10,
        },
    }


def _walk_and_apply(widget, theme: dict, path: str = ""):
    """Applique récursivement les couleurs du thème aux widgets."""
    colors = theme.get("colors", {})
    try:
        bg = widget.cget("bg")
    except Exception:
        bg = None

    widget_type = widget.winfo_class()
    parent_path = path

    try:
        if widget_type == "Frame":
            if bg and bg != "#F3F3F3" and bg != "#FFFFFF":
                pass  # Garder sa couleur si déjà custom
            else:
                widget.configure(bg=colors.get("bg_main", "#F3F3F3"))

        elif widget_type in ("Label",):
            if not path.endswith("_title"):
                fg = colors.get("fg_text", "#000000")
                widget.configure(bg=colors.get("bg_main", "#F3F3F3"), fg=fg)
        elif widget_type in ("Button",):
            if path.endswith("danger") or path.endswith("stop"):
                widget.configure(
                    bg=colors.get("bg_button_danger", "#FF5C5C"), fg="#000000"
                )
            elif (
                path.endswith("action")
                or path.endswith("skip")
                or path.endswith("detect")
            ):
                widget.configure(
                    bg=colors.get("bg_button_action", "#FFD400"), fg="#000000"
                )
            elif path.endswith("start"):
                widget.configure(
                    bg=colors.get("bg_button_start", "#00D084"),
                    fg=colors.get("fg_button_start", "#000000"),
                )
            elif path.endswith("pick") or path.endswith("explore"):
                widget.configure(
                    bg=colors.get("bg_button_normal", "#FFFFFF"),
                    fg=colors.get("fg_text", "#000000"),
                )
            else:
                fg = colors.get("fg_text", "#000000")
                bg_btn = colors.get("bg_button_action", "#FFD400")
                widget.configure(bg=bg_btn, fg=fg)
        elif widget_type in ("Radiobutton",):
            widget.configure(
                bg=colors.get("bg_main", "#F3F3F3"),
                fg=colors.get("fg_text", "#000000"),
                activebackground=colors.get("bg_frame", "#E7E7E7"),
            )
        elif widget_type in ("Checkbutton",):
            widget.configure(
                bg=colors.get("bg_main", "#F3F3F3"),
                fg=colors.get("fg_text", "#000000"),
            )
        elif widget_type in ("Listbox",):
            widget.configure(
                bg=colors.get("bg_listbox", "#FFFFFF"),
                fg=colors.get("fg_listbox", "#000000"),
                selectbackground=colors.get("bg_button_action", "#FFD400"),
            )
        elif widget_type in ("Text",):
            widget.configure(
                bg=colors.get("bg_text", "#FFFFFF"),
                fg=colors.get("fg_textbox", "#000000"),
            )
        elif widget_type in ("Scrollbar",):
            widget.configure(
                bg=colors.get("bg_frame", "#CCCCCC"),
                troughcolor=colors.get("bg_main", "#F3F3F3"),
            )
    except Exception:
        pass

    # Appliquer aux enfants
    for child in widget.winfo_children():
        child_path = path + "_" + child.winfo_name() if path else child.winfo_name()
        _walk_and_apply(child, theme, child_path)


def _destroy_slice_labels(widget):
    """Detruit les anciens labels de slice sur un widget."""
    if hasattr(widget, "_slice_labels"):
        for lbl in widget._slice_labels:
            try:
                lbl.destroy()
            except Exception:
                pass
        widget._slice_labels = []


def _slice_widgets_later(gui):
    """Parcourt les widgets et crop l'image de fond sur chaque Frame."""
    bg_img = getattr(gui, "_bg_pil_img", None)
    if bg_img is None:
        return

    def _do_slice(w, root_win):
        for child in w.winfo_children():
            _do_slice(child, root_win)
            cls = child.winfo_class()
            if cls == "Frame":
                _destroy_slice_labels(child)
                try:
                    fx = child.winfo_rootx() - root_win.winfo_rootx()
                    fy = child.winfo_rooty() - root_win.winfo_rooty()
                    fw = child.winfo_width()
                    fh = child.winfo_height()
                    if fw > 0 and fh > 0:
                        from PIL import Image, ImageTk

                        cropped = bg_img.crop((fx, fy, fx + fw, fy + fh))
                        photo = ImageTk.PhotoImage(cropped)
                        lbl = tk.Label(
                            child, image=photo, borderwidth=0, highlightthickness=0
                        )
                        lbl.place(relx=0, rely=0, relwidth=1, relheight=1)
                        lbl.lower()
                        if not hasattr(child, "_slice_labels"):
                            child._slice_labels = []
                        child._slice_labels.append(lbl)
                        child._slice_labels.append(photo)
                except Exception:
                    pass
            elif cls in ("Label",):
                _destroy_slice_labels(child)

    _do_slice(gui.root, gui.root)


def apply(name: str, gui) -> None:
    """Applique le thème donné à toute l'interface GUI."""
    theme = get_theme(name)
    colors = theme.get("colors", {})

    # Appliquer aux principaux conteneurs
    gui.root.configure(bg=colors.get("bg_main", "#F3F3F3"))

    # Notebook (onglets)
    try:
        gui.nb_top.configure(bg=colors.get("bg_main", "#F3F3F3"))
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TNotebook", background=colors.get("bg_main", "#F3F3F3"))
        style.configure("TNotebook.Tab", background=colors.get("bg_tab", "#F0F0F0"))
        style.map(
            "TNotebook.Tab",
            background=[("selected", colors.get("bg_tab_selected", "#FFFFFF"))],
        )
    except Exception:
        pass

    # Barre de progression
    try:
        gui.progress.configure(
            style="TProgressbar" if hasattr(gui, "progress") else None
        )
        if hasattr(gui, "progress"):
            style = ttk.Style()
            style.configure(
                "TProgressbar",
                background=colors.get("fg_progress", "#00D084"),
                troughcolor=colors.get("bg_progress", "#E0E0E0"),
            )
    except Exception:
        pass

    # Silk label
    try:
        gui.silk_label.configure(
            bg=colors.get("bg_silk", "#F3F3F3"),
            fg=colors.get("fg_silk", "#000000"),
        )
    except Exception:
        pass

    _walk_and_apply(gui.root, theme, "root")

    # Appliquer l'image de fond (Label derriere tout)
    bg_path = theme.get("bg_path")
    if bg_path:
        try:
            from PIL import Image, ImageTk

            img = Image.open(bg_path)
            gui._bg_pil_img = img

            # Label racine avec l'image en fond
            if not hasattr(gui, "_bg_root_label"):
                gui._bg_root_label = tk.Label(gui.root)
                gui._bg_root_label.place(relx=0, rely=0, relwidth=1, relheight=1)
                gui._bg_root_label.lower()
            gui._bg_photo = ImageTk.PhotoImage(img)
            gui._bg_root_label.configure(image=gui._bg_photo)

            # Redimensionner l'image lors du resize de la fenêtre
            def _on_resize(event=None):
                try:
                    w = gui.root.winfo_width()
                    h = gui.root.winfo_height()
                    if w > 1 and h > 1 and gui._bg_pil_img is not None:
                        resized = gui._bg_pil_img.copy().resize((w, h), Image.LANCZOS)
                        gui._bg_photo = ImageTk.PhotoImage(resized)
                        gui._bg_root_label.configure(image=gui._bg_photo)
                except Exception:
                    pass

            gui.root.bind("<Configure>", _on_resize)
            gui.root.after(200, _on_resize)
        except Exception:
            pass
    else:
        gui._bg_pil_img = None
        if hasattr(gui, "_bg_root_label"):
            try:
                gui._bg_root_label.destroy()
            except Exception:
                pass
            gui._bg_root_label = None
            gui._bg_photo = None


def random_theme(exclude: list[str] = None) -> str:
    """Retourne un nom de thème aléatoire (hors ceux dans exclude)."""
    themes = list_themes()
    if exclude:
        themes = [t for t in themes if t not in exclude]
    if not themes:
        return "default"
    return random.choice(themes)


def load_preference() -> str | None:
    """Charge la préférence utilisateur depuis theme_pref.txt."""
    try:
        if PREF_FILE.exists():
            name = PREF_FILE.read_text(encoding="utf-8").strip()
            if name in list_themes():
                return name
    except Exception:
        pass
    return None


def save_preference(name: str) -> None:
    """Sauvegarde la préférence utilisateur dans theme_pref.txt."""
    try:
        PREF_FILE.write_text(name.strip(), encoding="utf-8")
    except Exception:
        pass


def clear_preference() -> None:
    """Supprime la préférence (retour au random)."""
    try:
        if PREF_FILE.exists():
            PREF_FILE.unlink()
    except Exception:
        pass
