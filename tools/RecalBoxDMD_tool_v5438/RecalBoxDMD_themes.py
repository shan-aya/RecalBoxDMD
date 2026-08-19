# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v6
#
# v6 — 2026-08-11 — safe-modify — Panneaux "detail de mode" (RecalBoxDMD_GUI.py,
#      mode_desc_label/_adv) convertis de Label a Text pour des liens
#      cliquables (pack "ultimate" externe, Mode 2/11) -- deux ajouts ici :
#      (1) _walk_and_apply() : les Text portant l'attribut Python
#      _theme_as_panel=True se fondent dans le panneau (bg_main/fg_text)
#      au lieu de prendre l'aspect "carte" blanche habituel des Text
#      (bg_text/fg_textbox). (2) apply() : appelle gui._update_mode_desc()
#      apres le decoupage de fond, meme raison que _refresh_help_tab_content()
#      juste au-dessus -- les couleurs de lien sont figees dans les tags
#      Tkinter au rendu, il faut regenerer le contenu pour suivre le theme.
#
# v5 — 2026-08-03 — safe-modify — RECONSTRUCTION (worktree dev-tous-txt-
#      filter, apres perte accidentelle de dev-cache-externalisation,
#      voir changelog RecalBoxDMD_tool.py v29) : mecanisme
#      `_fixed_theme_colors` dans _walk_and_apply() — court-circuit tout
#      en tete de fonction pour un widget portant l'attribut Python
#      `_fixed_theme_colors = (bg, fg)` pose apres sa creation. Cause
#      racine (deja diagnostiquee dans la version perdue, reconfirmee
#      ici) : la deduction de role par SUFFIXE DE NOM DE WIDGET
#      ("...danger"/"...start"/"...action" plus bas dans cette fonction)
#      suppose qu'un name= explicite a ete passe au constructeur du
#      widget — verifie qu'aucun bouton du projet ne le fait (Tkinter
#      attribue des noms auto-generes "!button"/"!button2"... qui ne
#      matchent jamais ces suffixes), donc TOUS les boutons du projet
#      retombaient silencieusement sur la couleur generique du theme
#      actif, ecrasant toute couleur fixe voulue (Demarrer vert, Quitter
#      rouge, etc.) a chaque application de theme. py_compile OK.
#
# v4 — 2026-07-10 — safe-modify — Ajout slice_frame_overlay() : variante de
#      slice_single_frame() qui place le decoupage AU-DESSUS des enfants
#      (au lieu de derriere). Necessaire pour le cadre de selection
#      systeme : sa Listbox (fill=both/expand=True) couvre la quasi-
#      totalite du cadre, donc un decoupage place derriere elle (comme
#      pour le panneau Mode 8) restait invisible -- confirme par capture
#      d'ecran zoomee (cadre reellement gris plat, pas juste un motif trop
#      discret comme pour Mode 8).
# v3 — 2026-07-10 — safe-modify — Ajout slice_single_frame()/
#      make_frame_opaque() : decoupage de fond cible sur UN cadre, hors du
#      cycle global _slice_widgets_later() (qui ne se relance que sur
#      <<NotebookTabChanged>>). Sert a (a) appliquer enfin le decoupage au
#      panneau Mode 8 (_mode8_frame), qui devient visible via un simple
#      changement de mode radio -- jamais un changement d'onglet -- et ne
#      recevait donc jamais son fond decoupe ; (b) basculer dynamiquement
#      le cadre de selection systeme (modes 3/4/5) entre fond decoratif
#      (tant qu'il est vide) et fond opaque du theme (des qu'il est
#      peuple de vrais systemes, pour la lisibilite).
# v2 — 2026-07-10 — safe-modify — apply() : ajout du style ttk "TCombobox"
#      (fieldbackground/foreground/background/arrowcolor + popdown Listbox
#      via option_add). _walk_and_apply() ne couvrait que les widgets tk
#      classiques (Label, Button, Listbox...) : les ttk.Combobox (niveau de
#      log, version Recalbox, selecteur de theme) restaient blanc/gris
#      "Windows" par defaut sur fond de theme sombre.
# v1 — 2026-07-10 — safe-modify — Listbox : selectbackground/selectforeground
#      fixes (bleu fort + texte blanc) au lieu de bg_button_action, qui est
#      quasi identique a bg_main/bg_listbox sur les themes sombres
#      (Atari2600, Megadrive, N64, Neogeo) et rendait la selection illisible
#      dans la colonne systemes (sys_list/sys_list_adv) et le choix de
#      lecteur SD (_mode6_drive_list)
# ============================================
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

import RecalBoxDMD_prefs as prefs

# Dossier racine des thèmes
THEMES_DIR = Path(__file__).parent / "themes"

# Cache des thèmes chargés
_themes_cache: dict[str, dict] = {}


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


def _walk_and_apply(widget, theme: dict, path: str = "", has_bg: bool = False):
    """Applique récursivement les couleurs du thème aux widgets.
    Si has_bg=True, les Frames gardent leur bg transparent pour laisser voir l'image de fond slicee.
    """
    colors = theme.get("colors", {})

    # Court-circuit couleurs fixes : deduction du role par SUFFIXE DE NOM
    # DE WIDGET ("...danger"/"...start"/"...action" plus bas) ne fonctionne
    # QUE si le widget a recu un name= explicite a sa creation -- verifie
    # empiriquement qu'aucun bouton du projet ne le fait (Tkinter attribue
    # des noms auto-generes "!button"/"!button2"... qui ne matchent jamais
    # ces suffixes). Tous les boutons retombaient donc silencieusement sur
    # la couleur generique du theme actif, ecrasant toute couleur fixe
    # voulue (Demarrer vert, Quitter rouge, etc.). Un widget peut desormais
    # poser l'attribut Python _fixed_theme_colors = (bg, fg) apres sa
    # creation pour etre exempte de cette logique de role, quel que soit
    # son nom Tk reel.
    fixed = getattr(widget, "_fixed_theme_colors", None)
    if fixed is not None:
        fixed_bg, fixed_fg = fixed
        try:
            widget.configure(bg=fixed_bg, fg=fixed_fg)
        except Exception:
            pass
        # Meme convention de recursion que plus bas (path par winfo_name()) :
        # un widget a couleurs fixes garde ses propres enfants soumis au
        # theme normalement, seul CE widget est exempte.
        for child in widget.winfo_children():
            child_path = path + "_" + child.winfo_name() if path else child.winfo_name()
            _walk_and_apply(child, theme, child_path)
        return

    try:
        bg = widget.cget("bg")
    except Exception:
        bg = None

    widget_type = widget.winfo_class()
    parent_path = path

    try:
        if widget_type == "Frame":
            if has_bg:
                pass  # Laisser transparent pour que slice bg s'affiche
            elif bg and bg != "#F3F3F3" and bg != "#FFFFFF":
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
                selectcolor=colors.get("fg_heading", "#FF6600"),
            )
        elif widget_type in ("Checkbutton",):
            widget.configure(
                bg=colors.get("bg_main", "#F3F3F3"),
                fg=colors.get("fg_text", "#000000"),
            )
        elif widget_type in ("Listbox",):
            # Couleurs de selection fixes (pas liees au theme) : sur les
            # themes sombres, bg_button_action est quasi identique a
            # bg_main/bg_listbox, ce qui rendait la ligne selectionnee
            # illisible. Un bleu fort + texte blanc garde un contraste
            # net sur tous les fonds (clairs et sombres).
            widget.configure(
                bg=colors.get("bg_listbox", "#FFFFFF"),
                fg=colors.get("fg_listbox", "#000000"),
                selectbackground="#1565C0",
                selectforeground="#FFFFFF",
            )
        elif widget_type in ("Text",):
            # 2026-08-11 -- exception pour les panneaux "detail de mode"
            # (Text au lieu de Label depuis RecalBoxDMD_GUI.py, pour rendre
            # cliquables les liens qu'ils peuvent contenir) : ils doivent se
            # fondre dans le panneau environnant (bg_main/fg_text, comme un
            # Label) au lieu de prendre l'aspect "carte" blanche habituel
            # des Text (bg_text/fg_textbox, ex: onglet Aide). Marque via
            # l'attribut Python _theme_as_panel = True pose a la creation.
            if getattr(widget, "_theme_as_panel", False):
                widget.configure(
                    bg=colors.get("bg_main", "#F3F3F3"),
                    fg=colors.get("fg_text", "#000000"),
                )
            else:
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
            # Court-circuit : un widget (et tout son sous-arbre) peut se
            # marquer _no_bg_slice = True apres sa creation pour rester
            # TOUJOURS a fond plein, jamais decoupe -- utilise par l'onglet
            # Playlist (cadres Dossiers/Fichiers, RecalBoxDMD_GUI.py) dont
            # le fond doit suivre une couleur de contenu unie et lisible
            # (bg_listbox) plutot que l'image de fond decorative, y
            # compris sur les lignes individuelles a l'interieur (meme
            # mecanisme recursif que ci-dessous, donc un flag pose sur le
            # seul conteneur exterieur suffit a proteger tout le sous-arbre).
            if getattr(child, "_no_bg_slice", False):
                _destroy_slice_labels(child)
                continue
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


def _slice_frame_impl(gui, frame, raised: bool, geometry_widget=None) -> None:
    """geometry_widget : widget dont on epouse les bornes (crop + place),
    si different de `frame` (ex: une Listbox, qui ne peut pas parenter de
    Label -- le Label est alors cree sur `frame`, son parent Tk valide,
    mais positionne via place(in_=geometry_widget)."""
    bg_img = getattr(gui, "_bg_pil_img", None)
    if bg_img is None:
        return
    target = geometry_widget if geometry_widget is not None else frame
    _destroy_slice_labels(frame)
    try:
        root_win = gui.root
        fx = target.winfo_rootx() - root_win.winfo_rootx()
        fy = target.winfo_rooty() - root_win.winfo_rooty()
        fw = target.winfo_width()
        fh = target.winfo_height()
        if fw > 0 and fh > 0:
            from PIL import Image, ImageTk

            cropped = bg_img.crop((fx, fy, fx + fw, fy + fh))
            photo = ImageTk.PhotoImage(cropped)
            lbl = tk.Label(frame, image=photo, borderwidth=0, highlightthickness=0)
            lbl.place(in_=target, relx=0, rely=0, relwidth=1, relheight=1)
            if raised:
                lbl.lift()
            else:
                lbl.lower()
            frame._slice_labels = [lbl, photo]
    except Exception:
        pass


def slice_single_frame(gui, frame) -> None:
    """(Re)applique le decoupage de l'image de fond du theme sur UN SEUL
    Frame, sans repasser par tout l'arbre de widgets. Le decoupage est mis
    DERRIERE les enfants existants (ex: panneau Mode 8, dont les labels/
    boutons occupent la plupart de la surface -- seuls les interstices
    laissent voir le decoupage).

    _slice_widgets_later() ne se relance que sur <<NotebookTabChanged>> :
    un cadre qui devient visible (pack) ou change de taille suite a un
    simple changement de mode radio, DANS le meme onglet, ne recoit donc
    jamais son decoupage tant que l'utilisateur ne change pas d'onglet.
    Cette fonction comble ce trou pour un cadre precis.
    """
    _slice_frame_impl(gui, frame, raised=False)


def slice_frame_overlay(gui, frame, geometry_widget=None) -> None:
    """Comme slice_single_frame(), mais le decoupage est place AU-DESSUS
    des enfants existants. Utile quand ces enfants sont eux-memes opaques
    et couvrent (quasi) toute la surface du cadre -- ex: une Listbox vide
    en fill=both/expand=True, qui ne laisserait sinon voir aucun interstice
    pour un decoupage place derriere elle. A n'utiliser que lorsque
    l'enfant couvert n'a rien d'important a afficher (ex: liste vide).

    geometry_widget : si fourni (ex: la Listbox elle-meme, qui ne peut pas
    parenter de Label), le decoupage epouse ses bornes exactes plutot que
    celles de `frame` -- le Label reste cree sur `frame` (parent Tk
    valide) mais positionne par-dessus geometry_widget via place(in_=...).
    """
    _slice_frame_impl(gui, frame, raised=True, geometry_widget=geometry_widget)


def make_frame_opaque(frame, bg_color: str) -> None:
    """Retire le decoupage decoratif d'un Frame et le repasse en fond plat
    (lisibilite) -- utilise par ex. quand le cadre de selection systeme se
    peuple de vrais elements de liste (modes 3/4/5)."""
    _destroy_slice_labels(frame)
    try:
        frame.configure(bg=bg_color)
    except Exception:
        pass


def apply(name: str, gui) -> None:
    """Applique le thème donné à toute l'interface GUI."""
    theme = get_theme(name)
    colors = theme.get("colors", {})
    # Memorise le theme reellement applique (nom concret, jamais "random" :
    # la resolution a deja eu lieu chez l'appelant) pour que d'autres
    # fonctions (ex: _update_sys_box_decor) puissent retrouver ses couleurs
    # sans dependre de l'affichage du combobox (qui peut montrer "Aléatoire").
    gui._current_theme_name = name

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

    # Combobox (ttk.Combobox : niveau de log, version Recalbox, selecteur de
    # theme). Non couvert par _walk_and_apply(), qui ne traite que les
    # widgets tk classiques (Label, Button, Listbox...) : sans ce bloc le
    # champ reste blanc/gris "Windows" par defaut sur fond de theme sombre.
    try:
        style = ttk.Style()
        style.theme_use("clam")
        cb_field = colors.get("bg_listbox", "#FFFFFF")
        cb_fg = colors.get("fg_listbox", "#000000")
        cb_bg = colors.get("bg_frame", "#FFFFFF")
        cb_arrow = colors.get("fg_heading", "#000000")
        style.configure(
            "TCombobox",
            fieldbackground=cb_field,
            background=cb_bg,
            foreground=cb_fg,
            arrowcolor=cb_arrow,
            bordercolor=colors.get("bg_frame", "#CCCCCC"),
        )
        style.map(
            "TCombobox",
            fieldbackground=[("readonly", cb_field), ("disabled", cb_field)],
            foreground=[
                ("readonly", cb_fg),
                ("disabled", colors.get("fg_text_light", cb_fg)),
            ],
            background=[("readonly", cb_bg), ("active", cb_bg)],
        )
        # Liste deroulante (popdown) : Listbox Tk classique pilote via
        # l'option DB, non stylable via ttk.Style.
        gui.root.option_add("*TCombobox*Listbox.background", cb_field)
        gui.root.option_add("*TCombobox*Listbox.foreground", cb_fg)
        gui.root.option_add("*TCombobox*Listbox.selectBackground", "#1565C0")
        gui.root.option_add("*TCombobox*Listbox.selectForeground", "#FFFFFF")
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

    _walk_and_apply(gui.root, theme, "root", has_bg=(theme.get("bg_path") is not None))

    # Re-rendre le markdown de l'onglet Aide : help_text vient de recevoir
    # bg_text/fg_textbox du theme via _walk_and_apply ci-dessus, mais les
    # couleurs internes du rendu (citations, code, tableaux, liens) sont
    # figees dans les tags Tkinter au moment du rendu -> il faut regenerer
    # le contenu pour qu'elles suivent le nouveau theme.
    if getattr(gui, "help_text", None) is not None and hasattr(
        gui, "_refresh_help_tab_content"
    ):
        try:
            gui._refresh_help_tab_content()
        except Exception:
            pass

    # 2026-08-11 -- meme raison que pour l'onglet Aide ci-dessus : les
    # panneaux "detail de mode" (mode_desc_label/_adv) sont desormais des
    # Text avec liens auto-detectes (voir RecalBoxDMD_GUI.py
    # _insert_autolink_text()), dont la couleur de lien est figee au moment
    # du rendu -- il faut regenerer le contenu pour qu'elle suive le
    # nouveau theme.
    if hasattr(gui, "_update_mode_desc"):
        try:
            gui._update_mode_desc()
        except Exception:
            pass

    # Appliquer l'image de fond (Label sous les widgets + slices sur les Frames)
    bg_path = theme.get("bg_path")
    if bg_path:
        try:
            from PIL import Image, ImageTk

            img = Image.open(bg_path)
            gui._bg_pil_img = img

            # Redimensionner pour remplir la fenêtre (~1100x750)
            w = max(gui.root.winfo_width(), 1100)
            h = max(gui.root.winfo_height(), 750)
            if w < 2:
                w = 1100
            if h < 2:
                h = 750
            resized = img.copy().resize((w, h), Image.LANCZOS)

            if not hasattr(gui, "_bg_root_label") or gui._bg_root_label is None:
                gui._bg_root_label = tk.Label(gui.root)
                gui._bg_root_label.place(relx=0, rely=0, relwidth=1, relheight=1)
                gui._bg_root_label.lower()
            gui._bg_photo = ImageTk.PhotoImage(resized)
            gui._bg_root_label.configure(image=gui._bg_photo)

            # Slicer UNE SEULE FOIS apres affichage (pas de binding resize)
            gui.root.after(400, _slice_widgets_later, gui)
        except Exception:
            pass
    else:
        gui._bg_pil_img = None
        for lbl in getattr(gui, "_slice_labels", []):
            try:
                lbl.destroy()
            except Exception:
                pass
        gui._slice_labels = []
        if hasattr(gui, "_bg_root_label") and gui._bg_root_label is not None:
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
    """Charge la préférence utilisateur depuis le fichier JSON centralisé.
    Retourne le nom du theme, 'random' ou None."""
    val = prefs.get("theme")
    if val is None:
        return None
    if val == "random":
        return "random"
    if val in list_themes():
        return val
    return None


def save_preference(name: str) -> None:
    """Sauvegarde la préférence utilisateur dans le fichier JSON centralisé."""
    prefs.set("theme", name.strip())


def clear_preference() -> None:
    """Supprime la préférence (retour au random)."""
    prefs.set("theme", "random")


def pref_is_random() -> bool:
    """Retourne True si la preference est le mode aleatoire."""
    return prefs.get("theme") == "random"
