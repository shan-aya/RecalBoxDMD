"""Fenetre "Logos de theme Recalbox" du toolkit RecalBoxDMD (v14, 2026-10-03).
Historique : v2 langue des variantes = langue de l'interface (liste deroulante, en direct) ; v3 region associee (en_US->us, fr_FR/es_ES->eu), ni langue ni region de la Recalbox detectee ne remplacent plus ce choix ; v4 mise en page (langue/region/hub sur des lignes separees, explications), case hub respectee aussi a la conversion ; v5 region renommee 'Region d origine des consoles' + bouton ? d aide (langue = textes, region = consoles) ; v6 colonne 'Variantes (langues | consoles)' (conversion locale, sinon manifeste du paquet GitHub, sinon calcul sur la copie Recalbox) ; v7 case hub : coche invisible en theme sombre (fond de case blanc + coche de la couleur du texte) -> fond de case = fond du theme. v8 : a l'ouverture la fenetre interroge le hub et la Recalbox toute seule (plus de case hub) ; bouton Actualiser ; colonnes Hub / Recalbox ; bouton 'Mettre a jour les perimes' (themes plus anciens que le hub ou langue/region modifiee) ; proposition de convertir les themes installes sur la Recalbox jamais convertis. v9 : a l'ouverture, d'abord comparaison de la carte SD du DMD avec le paquet GitHub (revision par theme stockee dans <theme>/_source.json) et proposition de mise a jour de la SD. v10 : la reference des etats est la CARTE SD du DMD (le dossier de travail du toolkit est vide par Windows : transit seulement) ; conversions recopiees sur la SD ; cible affichee. v11 : les ecarts carte SD / paquet GitHub (revision par theme) remontent aussi dans le tableau (etat MAJ). v12 : 'Mettre a jour les perimes' telecharge le paquet GitHub pour les themes en retard sur le paquet (SD), convertit les autres. v13 : theme sans logo de systeme exploitable (ex. helmicretro-haigenco : illustrations 640x480) = SANS LOGOS (liste 'skipped' du manifeste + echecs de la session) au lieu de rester 'jamais converti'. v14 : titre de la fenetre aligne sur le Mode 12 « Gestion des themes Recalbox ». safe-modify, sauvegardes dans _backups/.


Verifie les themes (theme-hub + themes installes sur la Recalbox), convertit leurs logos de systeme en .raw565 pour la
SD du DMD (staging : <sd_card>/systems/_defaults/_themes/<theme>/), affiche une planche de controle, copie vers la SD.
Moteur : theme_logos.py (aucune logique ici, seulement l'interface). Ouverte par RetroBoxLEDGui._open_theme_logos_window().

Dependances (en plus du toolkit) : resvg-py, numpy, pillow. Si absentes : message explicite, rien d'autre ne casse.
"""
import os
import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

T = {
    "fr": {
        "title": "Gestion des thèmes Recalbox",
        "intro": "Récupère les logos des thèmes Recalbox (thèmes du hub, sans les installer sur la Recalbox, ou thèmes déjà installés), "
                 "les convertit pour le DMD et les copie sur la SD. Le DMD les affiche à la place des logos par défaut quand ce thème est actif dans Recalbox.",
        "rb_path": "Thèmes installés sur la Recalbox :",
        "browse": "Parcourir…",
        "lang": "Langue des logos (variantes) :",
        "use_hub": "Interroger le theme-hub (catalogue en ligne)",
        "region": "Région d'origine des consoles :",
        "region_help": "Région d'origine des consoles\n\nCertains thèmes dessinent le logo d'une même console différemment selon sa région de commercialisation : États-Unis (us), Europe (eu) ou Japon (jp). Exemples : Genesis (us) / Mega Drive (eu et jp), Super Nintendo / Super Famicom (jp), Saturn (jp).\n\nCe choix ne concerne QUE les logos des consoles. Il ne change pas la langue des textes des logos (Favoris, Dernier joué, collections...) : c'est le rôle de la liste « Langue ».\n\nPar défaut la région suit la langue (fr_FR et es_ES → eu, en_US → us), mais tu peux la changer. Exemple : fr_FR + jp = textes en français, consoles japonaises. Un thème qui n'a pas de variante pour un système utilise son logo de base.",
        "lang_hint": "Langue = textes des logos (Favoris, Dernier joué...). Région d'origine = logos des consoles (us / eu / jp). Elles suivent la langue de l'interface mais sont indépendantes : tu peux les changer séparément (bouton ?).",
        "hub_hint": "Coché : la liste vient du catalogue officiel en ligne (nouveaux thèmes, mises à jour) et les logos sont téléchargés depuis le hub. Décoché : seuls les thèmes installés sur la Recalbox sont listés.",
        "check": "Vérifier les thèmes",
        "refresh": "Actualiser",
        "target_sd": "Cible : carte SD {drive}",
        "target_work": "Cible : dossier de travail du toolkit (aucune carte SD du DMD détectée)",
        "sd_update_prompt": "La carte SD ({drive}) contient {n} thème(s) à mettre à jour d'après le paquet du projet sur GitHub :\n{lines}\n\nMettre à jour la carte SD maintenant ?",
        "sd_updating": "Mise à jour de la carte SD ({drive})…",
        "sd_updated": "Carte SD mise à jour : {n} fichier(s) téléchargé(s).",
        "sd_none": "Carte SD du DMD non détectée : comparaison avec le paquet GitHub ignorée.",
        "sd_nopkg": "Paquet de logos GitHub injoignable : comparaison avec la carte SD ignorée.",
        "sd_log": "Carte SD {drive} : {n} thème(s) du paquet présent(s), {m} à mettre à jour.",
        "update": "Mettre à jour les périmés",
        "col_hub": "Hub", "col_rb": "Recalbox",
        "propose_new": "{n} thème(s) installé(s) sur la Recalbox n'ont pas encore été convertis pour le DMD :\n{names}\n\nLes convertir maintenant ?",
        "all_uptodate": "Aucun thème à mettre à jour.",
        "detecting": "Recherche de la Recalbox et du hub…",
        "convert": "Convertir la sélection",
        "preview": "Aperçu",
        "copy": "Copier vers la SD…",
        "open_out": "Ouvrir le dossier",
        "col_theme": "Thème", "col_source": "Source", "col_state": "État", "col_variants": "Variantes (langues | consoles)", "col_detail": "Détail",
        "src_rb": "Recalbox", "src_hub": "hub", "src_both": "Recalbox + hub",
        "deps_missing": "Dépendances manquantes pour la conversion des logos de thème :\n{err}\n\nInstallez-les avec :\npip install resvg-py numpy pillow",
        "no_sel": "Sélectionnez au moins un thème dans la liste.",
        "checking": "Vérification en cours…",
        "check_done": "{n} thème(s) listé(s).",
        "converting": "Conversion de {name}…",
        "convert_done": "Terminé : {ok} converti(s), {ko} en échec.",
        "no_conv": "Ce thème n'a pas encore été converti : utilisez « Convertir la sélection ».",
        "no_drive": "Aucun lecteur amovible détecté. Insérez la carte SD du DMD.",
        "pick_drive": "Lecteur de la carte SD :",
        "copy_confirm": "Copier {n} thème(s) ({size} Mo) vers {drive}\\systems\\_defaults\\_themes\\ ?",
        "copying": "Copie de {name} vers {drive}…",
        "copy_done": "Copie terminée ({n} thème(s)). Éjectez la carte avant de la retirer.",
        "check_hint": "La liste réunit le hub (catalogue officiel en ligne), les thèmes installés sur la Recalbox et ce que contient la carte SD du DMD (versions lues dans chaque dossier de thème). Les conversions sont écrites sur la carte SD ; sans carte détectée, le dossier de travail du toolkit sert de transit (il peut être vidé par Windows). Orange = plus ancien que le hub ou le paquet, version inconnue, ou langue/région modifiée (à mettre à jour) ; bleu = jamais converti.",
        "ok": "OK", "close": "Fermer", "sample": "Planche de contrôle — {name}",
        "states": {"NOUVEAU": "NOUVEAU", "MAJ": "MISE À JOUR", "A JOUR": "À JOUR", "SANS LOGOS": "SANS LOGOS",
                   "DIFFERENT": "DIFFÉRENT", "ERREUR": "ERREUR"},
    },
    "en": {
        "title": "Recalbox themes management",
        "intro": "Fetches the logos of Recalbox themes (hub themes without installing them on the Recalbox, or already installed themes), "
                 "converts them for the DMD and copies them to the SD card. The DMD shows them instead of the default logos when that theme is active in Recalbox.",
        "rb_path": "Themes installed on the Recalbox:",
        "browse": "Browse…",
        "lang": "Logo language (variants):",
        "use_hub": "Query the theme-hub (online catalogue)",
        "region": "Console origin region:",
        "region_help": "Console origin region\n\nSome themes draw the same console's logo differently depending on the region it was sold in: United States (us), Europe (eu) or Japan (jp). Examples: Genesis (us) / Mega Drive (eu and jp), Super Nintendo / Super Famicom (jp), Saturn (jp).\n\nThis only affects CONSOLE logos. It does not change the language of the logo texts (Favorites, Last played, collections...): that is the job of the Language list.\n\nBy default the region follows the language (fr_FR and es_ES → eu, en_US → us), but you can change it. Example: fr_FR + jp = French texts, Japanese consoles. A theme with no variant for a system uses its base logo.",
        "lang_hint": "Language = logo texts (Favorites, Last played...). Console origin region = console logos (us / eu / jp). They follow the interface language but are independent: change them separately (? button).",
        "hub_hint": "Ticked: the list comes from the official online catalogue (new themes, updates) and logos are downloaded from the hub. Unticked: only themes installed on the Recalbox are listed.",
        "check": "Check themes",
        "refresh": "Refresh",
        "target_sd": "Target: SD card {drive}",
        "target_work": "Target: toolkit working folder (no DMD SD card detected)",
        "sd_update_prompt": "The SD card ({drive}) has {n} theme(s) to update according to the project package on GitHub:\n{lines}\n\nUpdate the SD card now?",
        "sd_updating": "Updating the SD card ({drive})…",
        "sd_updated": "SD card updated: {n} file(s) downloaded.",
        "sd_none": "DMD SD card not detected: comparison with the GitHub package skipped.",
        "sd_nopkg": "GitHub logo package unreachable: comparison with the SD card skipped.",
        "sd_log": "SD card {drive}: {n} package theme(s) present, {m} to update.",
        "update": "Update outdated",
        "col_hub": "Hub", "col_rb": "Recalbox",
        "propose_new": "{n} theme(s) installed on the Recalbox have not been converted for the DMD yet:\n{names}\n\nConvert them now?",
        "all_uptodate": "No theme to update.",
        "detecting": "Looking for the Recalbox and the hub…",
        "convert": "Convert selection",
        "preview": "Preview",
        "copy": "Copy to SD…",
        "open_out": "Open folder",
        "col_theme": "Theme", "col_source": "Source", "col_state": "Status", "col_variants": "Variants (languages | consoles)", "col_detail": "Details",
        "src_rb": "Recalbox", "src_hub": "hub", "src_both": "Recalbox + hub",
        "deps_missing": "Missing dependencies for theme logo conversion:\n{err}\n\nInstall them with:\npip install resvg-py numpy pillow",
        "no_sel": "Select at least one theme in the list.",
        "checking": "Checking…",
        "check_done": "{n} theme(s) listed.",
        "converting": "Converting {name}…",
        "convert_done": "Done: {ok} converted, {ko} failed.",
        "no_conv": "This theme has not been converted yet: use “Convert selection”.",
        "no_drive": "No removable drive detected. Insert the DMD SD card.",
        "pick_drive": "SD card drive:",
        "copy_confirm": "Copy {n} theme(s) ({size} MB) to {drive}\\systems\\_defaults\\_themes\\ ?",
        "copying": "Copying {name} to {drive}…",
        "copy_done": "Copy finished ({n} theme(s)). Eject the card before removing it.",
        "check_hint": "The list combines the hub (official online catalogue), the themes installed on the Recalbox and what the DMD SD card contains (versions read from each theme folder). Conversions are written to the SD card; with no card detected, the toolkit working folder is used as a staging area (Windows may clear it). Orange = older than the hub or the package, unknown version, or language/region changed (to update); blue = never converted.",
        "ok": "OK", "close": "Close", "sample": "Check sheet — {name}",
        "states": {"NOUVEAU": "NEW", "MAJ": "UPDATE", "A JOUR": "UP TO DATE", "SANS LOGOS": "NO LOGOS",
                   "DIFFERENT": "DIFFERENT", "ERREUR": "ERROR"},
    },
    "es": {
        "title": "Gestión de temas Recalbox",
        "intro": "Obtiene los logos de los temas de Recalbox (temas del hub sin instalarlos en la Recalbox, o temas ya instalados), "
                 "los convierte para el DMD y los copia en la SD. El DMD los muestra en lugar de los logos por defecto cuando ese tema está activo en Recalbox.",
        "rb_path": "Temas instalados en la Recalbox:",
        "browse": "Examinar…",
        "lang": "Idioma de los logos (variantes):",
        "use_hub": "Consultar el theme-hub (catálogo en línea)",
        "region": "Región de origen de las consolas:",
        "region_help": "Región de origen de las consolas\n\nAlgunos temas dibujan el logo de una misma consola de forma distinta según la región donde se vendió: Estados Unidos (us), Europa (eu) o Japón (jp). Ejemplos: Genesis (us) / Mega Drive (eu y jp), Super Nintendo / Super Famicom (jp), Saturn (jp).\n\nEsto solo afecta a los logos de las CONSOLAS. No cambia el idioma de los textos de los logos (Favoritos, Último jugado, colecciones...): eso lo decide la lista Idioma.\n\nPor defecto la región sigue al idioma (fr_FR y es_ES → eu, en_US → us), pero puedes cambiarla. Ejemplo: fr_FR + jp = textos en francés, consolas japonesas. Si un tema no tiene variante para un sistema, se usa su logo base.",
        "lang_hint": "Idioma = textos de los logos (Favoritos, Último jugado...). Región de origen = logos de las consolas (us / eu / jp). Siguen el idioma de la interfaz pero son independientes: puedes cambiarlas por separado (botón ?).",
        "hub_hint": "Marcado: la lista viene del catálogo oficial en línea (temas nuevos, actualizaciones) y los logos se descargan del hub. Desmarcado: solo se listan los temas instalados en la Recalbox.",
        "check": "Comprobar temas",
        "refresh": "Actualizar",
        "target_sd": "Destino: tarjeta SD {drive}",
        "target_work": "Destino: carpeta de trabajo del toolkit (ninguna tarjeta SD del DMD detectada)",
        "sd_update_prompt": "La tarjeta SD ({drive}) tiene {n} tema(s) por actualizar según el paquete del proyecto en GitHub:\n{lines}\n\n¿Actualizar la tarjeta SD ahora?",
        "sd_updating": "Actualizando la tarjeta SD ({drive})…",
        "sd_updated": "Tarjeta SD actualizada: {n} archivo(s) descargado(s).",
        "sd_none": "Tarjeta SD del DMD no detectada: comparación con el paquete de GitHub omitida.",
        "sd_nopkg": "Paquete de logos de GitHub inaccesible: comparación con la tarjeta SD omitida.",
        "sd_log": "Tarjeta SD {drive}: {n} tema(s) del paquete presente(s), {m} por actualizar.",
        "update": "Actualizar los obsoletos",
        "col_hub": "Hub", "col_rb": "Recalbox",
        "propose_new": "{n} tema(s) instalado(s) en la Recalbox aún no se han convertido para el DMD:\n{names}\n\n¿Convertirlos ahora?",
        "all_uptodate": "Ningún tema que actualizar.",
        "detecting": "Buscando la Recalbox y el hub…",
        "convert": "Convertir selección",
        "preview": "Vista previa",
        "copy": "Copiar a la SD…",
        "open_out": "Abrir carpeta",
        "col_theme": "Tema", "col_source": "Origen", "col_state": "Estado", "col_variants": "Variantes (idiomas | consolas)", "col_detail": "Detalle",
        "src_rb": "Recalbox", "src_hub": "hub", "src_both": "Recalbox + hub",
        "deps_missing": "Faltan dependencias para convertir los logos de tema:\n{err}\n\nInstálelas con:\npip install resvg-py numpy pillow",
        "no_sel": "Seleccione al menos un tema de la lista.",
        "checking": "Comprobando…",
        "check_done": "{n} tema(s) listado(s).",
        "converting": "Convirtiendo {name}…",
        "convert_done": "Terminado: {ok} convertido(s), {ko} con error.",
        "no_conv": "Este tema aún no se ha convertido: use «Convertir selección».",
        "no_drive": "No se detecta ninguna unidad extraíble. Inserte la tarjeta SD del DMD.",
        "pick_drive": "Unidad de la tarjeta SD:",
        "copy_confirm": "¿Copiar {n} tema(s) ({size} MB) a {drive}\\systems\\_defaults\\_themes\\ ?",
        "copying": "Copiando {name} a {drive}…",
        "copy_done": "Copia terminada ({n} tema(s)). Expulse la tarjeta antes de retirarla.",
        "check_hint": "La lista reúne el hub (catálogo oficial en línea), los temas instalados en la Recalbox y lo que contiene la tarjeta SD del DMD (versiones leídas en cada carpeta de tema). Las conversiones se escriben en la tarjeta SD; sin tarjeta detectada, la carpeta de trabajo del toolkit sirve de tránsito (Windows puede vaciarla). Naranja = más antiguo que el hub o el paquete, versión desconocida, o idioma/región modificados (a actualizar); azul = nunca convertido.",
        "ok": "OK", "close": "Cerrar", "sample": "Hoja de control — {name}",
        "states": {"NOUVEAU": "NUEVO", "MAJ": "ACTUALIZAR", "A JOUR": "AL DÍA", "SANS LOGOS": "SIN LOGOS",
                   "DIFFERENT": "DISTINTO", "ERREUR": "ERROR"},
    },
}


def open_window(gui):
    """Point d'entree appele par le GUI principal. Verifie les dependances avant d'ouvrir la fenetre."""
    lang = "fr"
    try:
        lang = gui.lang_var.get()
    except Exception:
        pass
    tr = T.get(lang, T["fr"])
    try:
        import theme_logos as tl   # noqa: F401  (meme dossier que le GUI)
        import numpy  # noqa: F401
        import PIL  # noqa: F401
        import resvg_py  # noqa: F401
    except Exception as e:
        messagebox.showerror(tr["title"], tr["deps_missing"].format(err=f"{type(e).__name__}: {e}"), parent=gui.root)
        return
    return ThemeLogosWindow(gui, tr)


class ThemeLogosWindow:
    def __init__(self, gui, tr):
        import theme_logos as tl
        self.tl, self.gui, self.tr = tl, gui, tr
        self.tkmod = gui.tkmod
        self.sd_dir = Path(gui.sd_dir)
        self.work_root = self.sd_dir / "systems" / "_defaults" / "_themes"   # dossier de travail du toolkit : transit seulement (Windows peut le vider)
        self.out_root = self.work_root                                       # reference reelle des etats : carte SD du DMD (voir _resolve_target)
        self.work_default = self.sd_dir / "systems" / "_defaults"
        self.sd_root = None
        self.on_sd = False
        self._no_logos = {}      # themes dont la conversion a echoue faute de logo de systeme (cette session)
        self.default_dir = self.sd_dir / "systems" / "_defaults"
        self.known = tl.known_systems_from_dir(str(self.default_dir))
        self.q: "queue.Queue" = queue.Queue()
        self.rows = {}          # dossier du theme -> dict (etat fusionne)
        self.hub = {}           # dossier -> ligne du catalogue du hub
        self.busy = False
        colors = {}
        try:
            colors = gui._theme_colors()
        except Exception:
            pass
        self.bg = colors.get("bg_main", "#F3F3F3")
        self.fg = colors.get("fg_text", "#000000")
        self.bg_action = colors.get("bg_button_action", "#00D084")

        w = self.win = tk.Toplevel(gui.root)
        w.title(tr["title"])
        w.configure(bg=self.bg)
        w.geometry("940x640")
        w.transient(gui.root)

        tk.Label(w, text=tr["intro"], bg=self.bg, fg=self.fg, wraplength=900, justify="left").pack(anchor="w", padx=12, pady=(10, 4))

        top = tk.Frame(w, bg=self.bg)
        top.pack(fill="x", padx=12, pady=4)
        tk.Label(top, text=tr["rb_path"], bg=self.bg, fg=self.fg).grid(row=0, column=0, sticky="w")
        self.rb_var = tk.StringVar(value=tl.DEFAULT_RB_THEMES)
        tk.Entry(top, textvariable=self.rb_var, width=52).grid(row=0, column=1, padx=6, sticky="we")
        tk.Button(top, text=tr["browse"], command=self._browse_rb).grid(row=0, column=2)
        self.lang_var = tk.StringVar(value=self._lang_from_gui())   # v2 : suit la langue de l'interface (fr -> fr_FR...)
        self.region_var = tk.StringVar(value=self._region_from_lang(self.lang_var.get()))   # v3 : suit la langue (en -> us, fr/es -> eu)
        opts = tk.Frame(top, bg=self.bg)   # v4 : une ligne propre, widgets cote a cote (avant : tous superposes dans la meme cellule)
        opts.grid(row=1, column=0, columnspan=3, sticky="w", pady=(6, 0))
        tk.Label(opts, text=tr["lang"], bg=self.bg, fg=self.fg).pack(side="left")
        ttk.Combobox(opts, textvariable=self.lang_var, values=("en_US", "fr_FR", "es_ES"), width=7, state="readonly").pack(side="left", padx=(4, 14))
        tk.Label(opts, text=tr["region"], bg=self.bg, fg=self.fg).pack(side="left")
        ttk.Combobox(opts, textvariable=self.region_var, values=("us", "eu", "jp"), width=4, state="readonly").pack(side="left", padx=(4, 0))
        tk.Button(opts, text="?", width=2, command=lambda: messagebox.showinfo(tr["region"].rstrip(" :"), tr["region_help"], parent=self.win)).pack(side="left", padx=(6, 0))
        try:   # changement de langue de l'interface => meme langue pour les variantes de logos
            self._gui_lang_trace = gui.lang_var.trace_add("write", lambda *_: self._follow_gui_lang())
        except Exception:
            self._gui_lang_trace = None
        self.lang_var.trace_add("write", lambda *_: self.region_var.set(self._region_from_lang(self.lang_var.get())))   # liste de langue modifiee a la main => region associee
        tk.Label(top, text=tr["lang_hint"], bg=self.bg, fg="#555555", wraplength=880, justify="left").grid(row=2, column=0, columnspan=3, sticky="w", pady=(2, 0))
        top.columnconfigure(1, weight=1)

        bar = tk.Frame(w, bg=self.bg)
        bar.pack(fill="x", padx=12, pady=(6, 4))
        self.btn_check = self._btn(bar, tr["refresh"], self._on_check, action=True)
        self.btn_update = self._btn(bar, tr["update"], self._on_update_outdated, action=True)
        self.btn_convert = self._btn(bar, tr["convert"], self._on_convert, action=True)
        self.btn_preview = self._btn(bar, tr["preview"], self._on_preview)
        self.btn_copy = self._btn(bar, tr["copy"], self._on_copy)
        self._btn(bar, tr["open_out"], self._on_open_out)

        tk.Label(w, text=tr["check_hint"], bg=self.bg, fg="#555555", wraplength=900, justify="left").pack(anchor="w", padx=12)

        frame = tk.Frame(w, bg=self.bg)
        frame.pack(fill="both", expand=True, padx=12, pady=6)
        cols = ("theme", "hub", "rb", "state", "variants", "detail")
        self.tree = ttk.Treeview(frame, columns=cols, show="headings", selectmode="extended", height=11)
        for c, key, wd in (("theme", "col_theme", 190), ("hub", "col_hub", 60), ("rb", "col_rb", 80), ("state", "col_state", 100), ("variants", "col_variants", 190), ("detail", "col_detail", 240)):
            self.tree.heading(c, text=tr[key])
            self.tree.column(c, width=wd, anchor="w")
        self.tree.tag_configure("maj", foreground="#E08A00")   # plus ancien que le hub / langue-region modifiee
        self.tree.tag_configure("new", foreground="#3A8DFF")   # jamais converti
        sb = ttk.Scrollbar(frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=sb.set)
        self.tree.pack(side="left", fill="both", expand=True)
        sb.pack(side="left", fill="y")

        self.status = tk.StringVar(value=f"{self.out_root}")
        tk.Label(w, textvariable=self.status, bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", padx=12)
        self.log = tk.Text(w, height=8, state="disabled", bg="#101010", fg="#d0d0d0")
        self.log.pack(fill="x", padx=12, pady=(2, 10))

        w.protocol("WM_DELETE_WINDOW", self._on_close)
        self.win.after(100, self._drain)
        self._asked = set()      # themes Recalbox deja proposes a la conversion (pas de relance a chaque actualisation)
        self._post("busy", True)
        self._post("status", tr["detecting"])
        threading.Thread(target=self._detect_rb, args=(self._pkg_lang(),), daemon=True).start()

    # ------------------------------------------------------------------ utilitaires
    def _btn(self, parent, text, cmd, action=False):
        b = tk.Button(parent, text=text, command=cmd, bd=2, relief="solid", padx=10, pady=4,
                      bg=self.bg_action if action else "#FFFFFF", fg="#000000", font=("TkDefaultFont", 10, "bold"))
        b.pack(side="left", padx=(0, 8))
        return b

    def _post(self, kind, *a):
        self.q.put((kind, a))

    def _drain(self):
        try:
            while True:
                kind, a = self.q.get_nowait()
                if kind == "log":
                    self.log.config(state="normal")
                    self.log.insert("end", a[0] + "\n")
                    self.log.see("end")
                    self.log.config(state="disabled")
                elif kind == "status":
                    self.status.set(a[0])
                elif kind == "sd_updates":
                    self.win.after(300, self._propose_sd_update)
                elif kind == "rows":
                    self._fill_rows()
                    self.win.after(300, self._propose_new_rb)
                elif kind == "rb":
                    self.rb_var.set(a[0])
                elif kind == "lang":
                    self.lang_var.set(a[0])
                elif kind == "region":
                    self.region_var.set(a[0] if a[0] in ("us", "eu", "jp") else "us")
                elif kind == "busy":
                    self._set_busy(a[0])
                elif kind == "info":
                    messagebox.showinfo(self.tr["title"], a[0], parent=self.win)
                elif kind == "error":
                    messagebox.showerror(self.tr["title"], a[0], parent=self.win)
                elif kind == "preview":
                    self._show_preview(a[0])
                elif kind == "recheck":
                    self._on_check()
        except queue.Empty:
            pass
        if self.win.winfo_exists():
            self.win.after(100, self._drain)

    def _set_busy(self, on):
        self.busy = on
        for b in (self.btn_check, self.btn_update, self.btn_convert, self.btn_copy):
            b.config(state="disabled" if on else "normal")

    def _log(self, s):
        self._post("log", str(s))

    def _on_close(self):
        try:
            if getattr(self, "_gui_lang_trace", None):
                self.gui.lang_var.trace_remove("write", self._gui_lang_trace)
        except Exception:
            pass
        self.win.destroy()

    def _browse_rb(self):
        d = filedialog.askdirectory(parent=self.win, title=self.tr["rb_path"])
        if d:
            self.rb_var.set(d)

    def _on_open_out(self):
        os.makedirs(self.out_root, exist_ok=True)
        try:
            os.startfile(str(self.out_root))   # Windows
        except Exception:
            pass

    def _detect_rb(self, lang="en"):
        """Detecte le partage de la Recalbox (hors thread principal : la resolution reseau peut etre lente)."""
        try:
            host = self.tkmod.detect_recalbox_share()
        except Exception:
            host = None
        if host:
            path = rf"\\{host}\share\themes"
            self._post("rb", path)
            self._log(f"Recalbox détectée : {path}")
        self._resolve_target()
        self._sd_check(lang)       # 1) carte SD <-> paquet GitHub (propose la MAJ de la SD)
        self._post("busy", False)
        self._post("recheck")      # 2) actualisation automatique a l'ouverture (hub + Recalbox + etat des conversions)

    def _find_sd_root(self):
        """Racine ('I:\\') de la carte SD du DMD : lecteur amovible contenant systems/_defaults, sinon None."""
        try:
            drives = self.tkmod._list_removable_drives_ex()
        except Exception:
            drives = []
        for d in drives:
            letter = d[0].rstrip("\\") + "\\"
            if os.path.isdir(os.path.join(letter, "systems", "_defaults")):
                return letter
        return None

    def _resolve_target(self):
        """Fixe la reference des etats et la cible des conversions : carte SD du DMD si detectee, sinon dossier de travail. Appele a chaque actualisation."""
        root = self._find_sd_root()
        self.sd_root, self.on_sd = root, bool(root)
        if root:
            self.default_dir = Path(root) / "systems" / "_defaults"
            self.out_root = self.default_dir / "_themes"
        else:
            self.default_dir = self.work_default
            self.out_root = self.work_root
        self.known = self.tl.known_systems_from_dir(str(self.default_dir))

    def _target_text(self):
        return self.tr["target_sd"].format(drive=self.sd_root.rstrip("\\")) if self.on_sd else self.tr["target_work"]

    def _pkg_lang(self):
        """Code de langue du paquet GitHub (en / fr / es) d'apres la liste de langue de la fenetre."""
        lg = str(self.lang_var.get())[:2].lower()
        return lg if lg in ("fr", "es") else "en"

    def _sd_check(self, lang):
        """Compare les themes de la carte SD du DMD (si detectee) au paquet GitHub ; prepare la proposition de MAJ (fenetre dans le thread principal)."""
        tr = self.tr
        root = self.sd_root
        if not root:
            self._log(tr["sd_none"])
            return
        manifest = self.tkmod.fetch_theme_package_manifest()
        if manifest is None:
            self._log(tr["sd_nopkg"])
            return
        rows = self.tkmod.compare_sd_with_theme_package(root, manifest, lang)
        todo = [r for r in rows if r["state"] in ("MAJ", "INCONNU")]
        self._log(tr["sd_log"].format(drive=root.rstrip("\\"), n=len(rows), m=len(todo)))
        if todo:
            self._sd_todo = (root, todo)
            self._post("sd_updates")

    def _propose_sd_update(self):
        if self.busy:
            self.win.after(500, self._propose_sd_update)
            return
        todo = getattr(self, "_sd_todo", None)
        if not todo:
            return
        root, rows = todo
        self._sd_todo = None
        lines = "\n".join(f"• {r['theme']} — {r['detail']}" for r in rows)
        if messagebox.askyesno(self.tr["title"], self.tr["sd_update_prompt"].format(drive=root.rstrip("\\"), n=len(rows), lines=lines), parent=self.win):
            self._post("busy", True)
            threading.Thread(target=self._sd_update_worker, args=(root, [r["theme"] for r in rows], self._pkg_lang()), daemon=True).start()

    def _sd_update_worker(self, root, names, lang):
        drive = root.rstrip("\\")
        self._post("status", self.tr["sd_updating"].format(drive=drive))
        try:
            n = self.tkmod.download_theme_logos(Path(root), progress_cb=None, listen_keyboard=False, lang=lang, themes=names, force=True)
            self._post("status", self.tr["sd_updated"].format(n=n))
            self._log(self.tr["sd_updated"].format(n=n))
        except Exception as e:
            self._post("error", f"{type(e).__name__}: {e}")
        finally:
            self._post("busy", False)
            self._post("recheck")

    def _lang_from_gui(self):
        """Langue de l'interface du toolkit -> code de variante de logos (meme choix que les logos par defaut : en/fr/es)."""
        try:
            return {"fr": "fr_FR", "es": "es_ES"}.get(self.gui.lang_var.get(), "en_US")
        except Exception:
            return "en_US"

    @staticmethod
    def _region_from_lang(lang):
        """Region de logos associee a la langue : anglais -> us, francais/espagnol -> eu (modifiable a la main)."""
        return "us" if str(lang).lower().startswith("en") else "eu"

    def _follow_gui_lang(self):
        lang = self._lang_from_gui()
        self.lang_var.set(lang)
        self.region_var.set(self._region_from_lang(lang))

    def _prefer(self):
        return self.tl.prefer_keys(self.lang_var.get().strip(), self.region_var.get())

    @staticmethod
    def _variants_text(v):
        """{"lang": ["fr"], "region": ["eu", "jp"]} -> 'fr  |  eu, jp' ('?' = inconnu, '—' = aucune)."""
        if not v:
            return "?"
        return (", ".join(v.get("lang", [])) or "—") + "  |  " + (", ".join(v.get("region", [])) or "—")

    def _apply_package_state(self, merged, lang):
        """Themes de la carte SD plus anciens que le paquet GitHub (ou de version inconnue) : etat MAJ dans le tableau."""
        self.pkg_manifest = self.tkmod.fetch_theme_package_manifest()
        # themes sans logo de systeme exploitable (liste du paquet + echecs de conversion de cette session) : SANS LOGOS, pas "jamais converti"
        skipped = dict((self.pkg_manifest or {}).get("skipped", {}))
        skipped.update(getattr(self, "_no_logos", {}))
        for folder, why in skipped.items():
            m = merged.get(folder)
            if m and m["state"] == "NOUVEAU":
                m["state"], m["detail"] = "SANS LOGOS", why
        if not self.on_sd or not self.pkg_manifest:
            return
        by_dir = {self.tl.theme_dir_name(f): f for f in merged}
        for r in self.tkmod.compare_sd_with_theme_package(self.sd_root, self.pkg_manifest, lang):
            folder = by_dir.get(r["theme"])
            m = merged.get(folder) if folder else None
            if m and r["state"] in ("MAJ", "INCONNU") and m["state"] in ("A JOUR", "DIFFERENT", "SANS LOGOS", "NOUVEAU", None):
                m["state"], m["detail"] = "MAJ", "paquet GitHub : " + r["detail"]

    def _collect_variants(self, merged, rb, rb_ok):
        """Variantes (langues/regions) de chaque theme : conversion locale (_source.json), sinon manifeste du paquet GitHub, sinon calcul sur la copie de la Recalbox."""
        tl = self.tl
        pkg = {}
        try:
            import json
            import urllib.request
            req = urllib.request.Request(f"{self.tkmod.GITHUB_THEMES_RAW_BASE}/manifest.json", headers={"User-Agent": "recalbox-toolkit"})
            with urllib.request.urlopen(req, timeout=15) as resp:
                pkg = json.loads(resp.read().decode("utf-8")).get("themes", {})
        except Exception as e:
            self._log(f"manifeste du paquet de logos indisponible ({type(e).__name__}) : variantes inconnues pour les thèmes non convertis.")
        for folder, m in merged.items():
            v = None
            try:
                loc = tl._read_source_json(str(self.out_root), folder)
                v = (loc or {}).get("variants")
                if v is None:
                    v = (pkg.get(tl.theme_dir_name(folder)) or {}).get("variants")
                if v is None and rb_ok and m.get("rb"):
                    v = tl.detect_variants(tl.DirSource(os.path.join(rb, folder)), self.known)
            except Exception:
                v = None
            m["variants_txt"] = self._variants_text(v)

    def _fill_rows(self):
        for i in self.tree.get_children():
            self.tree.delete(i)
        order = {"MAJ": 0, "NOUVEAU": 1}
        for folder, r in sorted(self.rows.items(), key=lambda kv: (order.get(kv[1]["state"], 2), not kv[1]["rb"], kv[0])):
            hubc = (f"v{r['hub_version']}" if r["hub_version"] is not None else "✓") if r["hub"] else "—"
            tag = {"MAJ": "maj", "NOUVEAU": "new"}.get(r["state"])
            self.tree.insert("", "end", iid=folder, tags=(tag,) if tag else (),
                             values=(folder, hubc, "✓" if r["rb"] else "—", self.tr["states"].get(r["state"], r["state"]), r.get("variants_txt", "?"), r["detail"]))
        n = sum(1 for r in self.rows.values() if r["state"] == "MAJ")
        self.btn_update.config(text=f"{self.tr['update']} ({n})")

    def _propose_new_rb(self):
        """Themes installes sur la Recalbox (par l'utilisateur) et jamais convertis : propose de les convertir (une seule fois par theme)."""
        if self.busy or getattr(self, "_sd_todo", None):      # la proposition de MAJ de la SD passe en premier
            self.win.after(500, self._propose_new_rb)
            return
        new = sorted(f for f, r in self.rows.items() if r["rb"] and r["state"] == "NOUVEAU" and f not in self._asked)
        if not new:
            return
        self._asked.update(new)
        if messagebox.askyesno(self.tr["title"], self.tr["propose_new"].format(n=len(new), names=", ".join(new)), parent=self.win):
            self._post("busy", True)
            threading.Thread(target=self._convert_worker, args=(new, self.rb_var.get().strip(), self._prefer(), True, False), daemon=True).start()

    def _on_update_outdated(self):
        """Met a jour (reconvertit) tous les themes perimes : plus anciens que le hub, ou langue/region modifiee."""
        if self.busy:
            return
        todo = sorted(f for f, r in self.rows.items() if r["state"] == "MAJ")
        if not todo:
            messagebox.showinfo(self.tr["title"], self.tr["all_uptodate"], parent=self.win)
            return
        pkg = [f for f in todo if str(self.rows[f]["detail"]).startswith("paquet GitHub")] if self.on_sd else []
        conv = [f for f in todo if f not in pkg]
        self._post("busy", True)
        threading.Thread(target=self._update_worker, args=(pkg, conv, self.rb_var.get().strip(), self._prefer(), self._pkg_lang()), daemon=True).start()

    def _update_worker(self, pkg, conv, rb, prefer, lang):
        """1) themes en retard sur le paquet GitHub : telechargement du paquet directement sur la SD ; 2) les autres : conversion."""
        if pkg:
            names = [self.tl.theme_dir_name(f) for f in pkg]
            self._post("status", self.tr["sd_updating"].format(drive=self.sd_root.rstrip(chr(92))))
            try:
                n = self.tkmod.download_theme_logos(Path(self.sd_root), progress_cb=None, listen_keyboard=False, lang=lang, themes=names, force=True)
                self._log(self.tr["sd_updated"].format(n=n))
            except Exception as e:
                self._log(f"ÉCHEC mise à jour depuis le paquet : {type(e).__name__}: {e}")
        if conv:
            self._convert_worker(conv, rb, prefer, True, False)      # termine par busy False + actualisation
        else:
            self._post("busy", False)
            self._post("recheck")

    def _selected(self):
        sel = list(self.tree.selection())
        if not sel:
            messagebox.showinfo(self.tr["title"], self.tr["no_sel"], parent=self.win)
        return sel

    # ------------------------------------------------------------------ verifier
    def _on_check(self):
        if self.busy:
            return
        self._post("busy", True)
        self._post("status", self.tr["checking"])
        threading.Thread(target=self._check_worker, args=(self.rb_var.get().strip(), True, self._prefer(), self._pkg_lang()), daemon=True).start()

    def _check_worker(self, rb, use_hub, prefer, lang="en"):
        tl = self.tl
        try:
            self._resolve_target()      # la carte SD a pu etre inseree / retiree depuis la derniere actualisation
            rb_ok = bool(rb) and os.path.isdir(rb)
            if not rb_ok:
                self._log(f"Thèmes de la Recalbox inaccessibles ({rb}) : seul le hub sera interrogé.")
            raw = tl.check_status(str(self.out_root), rb if rb_ok else None, use_hub, self.known, log=self._log, prefer=prefer)
            prio = {"ERREUR": 5, "MAJ": 4, "NOUVEAU": 3, "DIFFERENT": 2, "A JOUR": 1, "SANS LOGOS": 0}
            merged = {}
            for folder, st, why, ver, src in raw:
                m = merged.setdefault(folder, {"state": None, "detail": "", "rb": False, "hub": False, "hub_version": None})
                m["rb" if src == "rb" else "hub"] = True
                if src == "hub":
                    m["hub_version"] = ver
                # l'etat le plus urgent prime (MAJ > NOUVEAU > ...) ; a egalite celui de la copie Recalbox
                if m["state"] is None or prio.get(st, 0) > prio.get(m["state"], 0) or (prio.get(st, 0) == prio.get(m["state"], 0) and src == "rb"):
                    m["state"], m["detail"] = st, why
            for folder, m in merged.items():
                m["source"] = "both" if (m["rb"] and m["hub"]) else "rb" if m["rb"] else "hub"
            self._apply_package_state(merged, lang)
            self._collect_variants(merged, rb, rb_ok)
            self.rows = merged
            if use_hub:
                try:
                    self.hub = {h["folder"]: h for h in tl.hub_catalog()}
                except Exception as e:
                    self._log(f"theme-hub injoignable : {e}")
            self._post("rows")
            self._post("status", self.tr["check_done"].format(n=len(merged)) + "   " + self._target_text())
        except Exception as e:
            self._post("error", f"{type(e).__name__}: {e}")
        finally:
            self._post("busy", False)

    # ------------------------------------------------------------------ convertir
    def _on_convert(self):
        if self.busy:
            return
        sel = self._selected()
        if not sel:
            return
        self._post("busy", True)
        threading.Thread(target=self._convert_worker, args=(sel, self.rb_var.get().strip(), self._prefer(), True), daemon=True).start()

    def _convert_worker(self, folders, rb, prefer, use_hub=True, show_preview=True):
        tl = self.tl
        ok = ko = 0
        last_ok = None
        for folder in folders:
            self._post("status", self.tr["converting"].format(name=folder))
            self._log(f"=== {folder}")
            try:
                out = str(self.work_root / tl.theme_dir_name(folder))
                rb_dir = os.path.join(rb, folder) if rb else ""
                hub_info, src, label, rf = None, None, "hub", None
                # reference = theme-hub (stable/public) ; copie de la Recalbox seulement si le theme n est pas dans le hub
                h = self.hub.get(folder) if use_hub else None
                if not h and use_hub:
                    try:
                        h = {x["folder"]: x for x in tl.hub_catalog()}.get(folder)
                    except Exception:
                        h = None
                if h and h.get("zips"):
                    rf = tl.RangeFile(tl.hub_zip_urls(folder, h["zips"][0]))
                    src = tl.ZipSource(rf)
                    hub_info = {"hub_version": h.get("version"), "zip": h["zips"][0]}
                elif rb_dir and os.path.isdir(rb_dir) and os.path.exists(os.path.join(rb_dir, "theme.xml")):
                    src, label = tl.DirSource(rb_dir), "rb"
                else:
                    raise RuntimeError("thème absent du hub et non installé sur la Recalbox")
                tl.convert_theme(src, out, self.known, log=self._log, hub_info=hub_info, source_label=label, prefer=prefer)
                if self.on_sd:      # recopie immediate sur la carte SD du DMD (reference des etats)
                    dst = os.path.join(self.sd_root, "systems", "_defaults", "_themes", tl.theme_dir_name(folder))
                    os.makedirs(dst, exist_ok=True)
                    copied, failed, interrupted = self.tkmod._copy_to_drive(Path(out), dst, True)
                    self._log(f"→ carte SD {self.sd_root.rstrip(chr(92))} : {copied} fichier(s) copié(s), {len(failed)} échec(s)")
                    if failed or interrupted:
                        raise RuntimeError("copie vers la carte SD incomplète")
                if rf is not None:
                    self._log(f"téléchargé : {rf.fetched / 1e6:.1f} Mo sur {rf.size / 1e6:.0f} Mo")
                ok += 1
                last_ok = folder
            except Exception as e:
                ko += 1
                self._log(f"ÉCHEC {folder} : {e}")
                if "aucun logo" in str(e):
                    self._no_logos[folder] = "aucun logo de système exploitable"
        self._post("status", self.tr["convert_done"].format(ok=ok, ko=ko))
        self._post("busy", False)
        # planche de controle du dernier theme converti, puis rafraichit l'etat (les themes convertis passent en "A JOUR")
        if last_ok and show_preview:
            self._post("preview", last_ok)
        self._post("recheck")

    # ------------------------------------------------------------------ apercu
    def _on_preview(self):
        sel = self._selected()
        if sel:
            self._show_preview(sel[0])

    def _show_preview(self, folder):
        tl = self.tl
        d = self.out_root / tl.theme_dir_name(folder)
        if not d.is_dir():
            d = self.work_root / tl.theme_dir_name(folder)
        if not d.is_dir() or not any(f.endswith(".raw565") for f in os.listdir(d)):
            messagebox.showinfo(self.tr["title"], self.tr["no_conv"], parent=self.win)
            return
        try:
            from PIL import ImageTk
            sheet, _names = tl.preview_sheet(str(d), str(self.default_dir))
        except Exception as e:
            messagebox.showerror(self.tr["title"], f"{type(e).__name__}: {e}", parent=self.win)
            return
        pw = tk.Toplevel(self.win)
        pw.title(self.tr["sample"].format(name=folder))
        pw.configure(bg=self.bg)
        canvas = tk.Canvas(pw, bg="#2d2d2d", width=min(sheet.width + 24, 900), height=min(sheet.height + 8, 700), highlightthickness=0)
        vs = ttk.Scrollbar(pw, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=vs.set, scrollregion=(0, 0, sheet.width, sheet.height))
        photo = ImageTk.PhotoImage(sheet)
        canvas.create_image(0, 0, anchor="nw", image=photo)
        canvas.image = photo           # reference (evite le ramasse-miettes)
        canvas.pack(side="left", fill="both", expand=True)
        vs.pack(side="left", fill="y")

    # ------------------------------------------------------------------ copier vers la SD
    def _on_copy(self):
        if self.busy:
            return
        sel = self._selected()
        if not sel:
            return
        names = [self.tl.theme_dir_name(f) for f in sel]
        missing = [n for n in names if not (self.work_root / n).is_dir()]
        if missing:
            messagebox.showinfo(self.tr["title"], self.tr["no_conv"] + "\n" + ", ".join(missing), parent=self.win)
            return
        try:
            drives = self.tkmod._list_removable_drives_ex()
        except Exception:
            drives = []
        if not drives:
            messagebox.showinfo(self.tr["title"], self.tr["no_drive"], parent=self.win)
            return
        letter = self._pick_drive(drives)
        if not letter:
            return
        size = sum(f.stat().st_size for n in names for f in (self.work_root / n).rglob("*") if f.is_file()) / 1e6
        if not messagebox.askyesno(self.tr["title"], self.tr["copy_confirm"].format(n=len(names), size=f"{size:.1f}", drive=letter), parent=self.win):
            return
        self._post("busy", True)
        threading.Thread(target=self._copy_worker, args=(names, letter), daemon=True).start()

    def _pick_drive(self, drives):
        if len(drives) == 1:
            return drives[0][0]
        dlg = tk.Toplevel(self.win)
        dlg.title(self.tr["pick_drive"])
        dlg.transient(self.win)
        dlg.grab_set()
        var = tk.StringVar(value=drives[0][0])
        tk.Label(dlg, text=self.tr["pick_drive"]).pack(padx=14, pady=(12, 4))
        for d in drives:
            tk.Radiobutton(dlg, text=f"{d[0]}  {d[1]}  ({d[2]}, {d[3] if len(d) > 3 else ''})", variable=var, value=d[0]).pack(anchor="w", padx=14)
        res = {"v": None}

        def ok():
            res["v"] = var.get()
            dlg.destroy()

        tk.Button(dlg, text=self.tr["ok"], command=ok, padx=14).pack(pady=12)
        self.win.wait_window(dlg)
        return res["v"]

    def _copy_worker(self, names, letter):
        n_ok = 0
        for name in names:
            self._post("status", self.tr["copying"].format(name=name, drive=letter))
            try:
                dst = f"{letter}\\systems\\_defaults\\_themes\\{name}"
                os.makedirs(dst, exist_ok=True)
                copied, failed, interrupted = self.tkmod._copy_to_drive(self.work_root / name, dst, True)
                self._log(f"{name} : {copied} fichier(s) copié(s), {len(failed)} échec(s)" + (" — INTERROMPU" if interrupted else ""))
                if not failed and not interrupted:
                    n_ok += 1
            except Exception as e:
                self._log(f"ÉCHEC copie {name} : {e}")
        self._post("status", self.tr["copy_done"].format(n=n_ok))
        self._post("busy", False)
