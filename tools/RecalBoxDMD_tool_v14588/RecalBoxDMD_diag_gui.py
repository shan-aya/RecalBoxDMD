# -*- coding: utf-8 -*-
# RecalBoxDMD_diag_gui.py -- onglet « Diagnostic » du toolkit (Wi-Fi comme la page /config/diag du DMD + moniteur serie USB + export .zip).
#
# v1 - 2026-10-31 - safe-modify - Creation. Construit par RetroBoxLEDGui._build_diag_tab() ; la langue suit gui.lang_var, les
#   couleurs gui._theme_colors(). Logique (HTTP, serie, rapport, zip) dans RecalBoxDMD_diag.py. Une panne de ce module ne doit
#   jamais empecher l'ouverture du toolkit. Le serveur du DMD est mono-connexion : UNE seule mesure a la fois ; elle s'arrete en
#   quittant l'onglet (comme la page web qui s'arrete quand elle est masquee) et a la fermeture du toolkit.
from __future__ import annotations

import queue
import re
import threading
import time
import tkinter as tk
from datetime import datetime
from tkinter import filedialog, messagebox, ttk

import RecalBoxDMD_diag as dg

T = {
    "fr": {
        "intro": "Mesure Wi-Fi du DMD (identique à la page web Diagnostic), moniteur série USB et export d'un rapport à joindre à une demande d'assistance. ESP32 et Raspberry Pi.",
        "wifi": " Wi-Fi ", "usb": " USB (série) ", "ip": "DMD :", "find": "Chercher", "searching": "Recherche du DMD…",
        "found": "DMD trouvé : {kind} v{ver} ({ip})", "notfound": "DMD introuvable (voir l'onglet Logs). Saisissez son adresse.",
        "start": "Démarrer la mesure", "stop": "Arrêter", "resume": "Reprendre DMD", "st_on": "Mesure active : le DMD affiche ses mesures sur la dalle",
        "collecting": "Relevés pour le rapport (configuration du DMD, pings)…",
        "st_off": "Mesure arrêtée", "noip": "Adresse du DMD manquante", "err": "Erreur de communication : {e}",
        "k_rssi": "Signal", "k_q": "Qualité", "k_ssid": "Réseau", "k_ch": "Canal", "k_ap": "Point d'accès", "k_ip": "IP DMD / passerelle",
        "k_rb": "Recalbox testée", "k_loss": "Perte", "k_lat": "Latence min/moy/max", "k_jit": "Gigue", "k_age": "Dernier paquet",
        "k_gaps": "Coupures UDP", "k_disc": "Reconnexions Wi-Fi", "k_last": "Dernière déconnexion", "k_heap": "Mémoire libre (min)",
        "k_up": "En marche depuis", "k_ping": "Ping PC → DMD", "k_http": "HTTP PC → DMD", "none": "aucune réponse", "never": "aucune",
        "ago": "il y a", "ev_c": "connecté au point d'accès", "ev_d": "déconnecté — ", "hist": "Historique Wi-Fi (récent en haut)",
        "rssi": "Signal (dBm)", "latc": "Latence Recalbox (ms)",
        "port": "Port :", "refresh": "Actualiser", "baud": "Débit :", "connect": "Connecter", "disconnect": "Déconnecter",
        "reset": "Redémarrer la carte", "clear": "Effacer", "hide": "Masquer les lignes contenant :", "hidden": "{n} ligne(s) masquée(s)",
        "noport": "Aucun port série (branchez l'ESP32 en USB).", "opened": "Connecté à {p} ({b} bauds) — la carte n'est PAS redémarrée.",
        "closed": "Déconnecté.", "openerr": "Ouverture impossible : {e}", "nopyserial": "pyserial est absent : le moniteur USB est indisponible.",
        "rep_html": "Rapport (navigateur)", "src": "Source :", "src_usb": "USB (série)", "src_wifi": "Wi-Fi (événements)", "log": " Journal ", "follow": "Suivre", "unfollow": "Arrêter le suivi", "wlog_on": "Suivi Wi-Fi de {ip} : événements reconstitués à partir des mesures du DMD (il n'expose pas son journal série par Wi-Fi).", "wlog_off": "Suivi Wi-Fi arrêté.", "wlog_noip": "Renseignez l'adresse du DMD dans le cadre Wi-Fi (ou « Chercher »).", "rep_btn": "Aperçu texte", "copy": "Copier le rapport", "export": "Exporter (.zip)", "close": "Fermer",
        "copied": "Rapport copié dans le presse-papiers.", "exported": "Export écrit :\n{p}", "rep_title": "Rapport de diagnostic",
        "hw": "Matériel détecté : {hw}", "r_help": "Le zip contient le rapport (PC, ports USB, DMD, mesures), le journal série complet et les mesures en tableau. Les mots de passe sont masqués.",
    },
    "en": {
        "intro": "DMD Wi-Fi measurement (same as the web Diagnostic page), USB serial monitor and a report export to attach to a support request. ESP32 and Raspberry Pi.",
        "wifi": " Wi-Fi ", "usb": " USB (serial) ", "ip": "DMD:", "find": "Search", "searching": "Searching for the DMD…",
        "found": "DMD found: {kind} v{ver} ({ip})", "notfound": "DMD not found (see the Logs tab). Type its address.",
        "start": "Start measuring", "stop": "Stop", "resume": "Resume DMD", "st_on": "Measuring: the DMD shows its readings on the panel",
        "collecting": "Collecting report data (DMD configuration, pings)…",
        "st_off": "Measurement stopped", "noip": "DMD address missing", "err": "Communication error: {e}",
        "k_rssi": "Signal", "k_q": "Quality", "k_ssid": "Network", "k_ch": "Channel", "k_ap": "Access point", "k_ip": "DMD IP / gateway",
        "k_rb": "Recalbox tested", "k_loss": "Loss", "k_lat": "Latency min/avg/max", "k_jit": "Jitter", "k_age": "Last packet",
        "k_gaps": "UDP gaps", "k_disc": "Wi-Fi reconnections", "k_last": "Last disconnection", "k_heap": "Free memory (min)",
        "k_up": "Running for", "k_ping": "PC → DMD ping", "k_http": "PC → DMD HTTP", "none": "no answer", "never": "none",
        "ago": "ago", "ev_c": "connected to the access point", "ev_d": "disconnected — ", "hist": "Wi-Fi history (latest on top)",
        "rssi": "Signal (dBm)", "latc": "Recalbox latency (ms)",
        "port": "Port:", "refresh": "Refresh", "baud": "Baud:", "connect": "Connect", "disconnect": "Disconnect",
        "reset": "Reboot the board", "clear": "Clear", "hide": "Hide lines containing:", "hidden": "{n} line(s) hidden",
        "noport": "No serial port (plug the ESP32 in by USB).", "opened": "Connected to {p} ({b} baud) — the board is NOT rebooted.",
        "closed": "Disconnected.", "openerr": "Cannot open: {e}", "nopyserial": "pyserial is missing: the USB monitor is unavailable.",
        "rep_html": "Report (browser)", "src": "Source:", "src_usb": "USB (serial)", "src_wifi": "Wi-Fi (events)", "log": " Log ", "follow": "Follow", "unfollow": "Stop following", "wlog_on": "Following {ip} over Wi-Fi: events rebuilt from the DMD readings (it does not expose its serial log over Wi-Fi).", "wlog_off": "Wi-Fi following stopped.", "wlog_noip": "Enter the DMD address in the Wi-Fi frame (or press Search).", "rep_btn": "Text preview", "copy": "Copy report", "export": "Export (.zip)", "close": "Close",
        "copied": "Report copied to the clipboard.", "exported": "Export written:\n{p}", "rep_title": "Diagnostic report",
        "hw": "Detected hardware: {hw}", "r_help": "The zip holds the report (PC, USB ports, DMD, readings), the full serial log and the readings as a table. Passwords are masked.",
    },
    "es": {
        "intro": "Medición Wi-Fi del DMD (igual que la página web Diagnóstico), monitor serie USB y exportación de un informe para adjuntar a una solicitud de soporte. ESP32 y Raspberry Pi.",
        "wifi": " Wi-Fi ", "usb": " USB (serie) ", "ip": "DMD:", "find": "Buscar", "searching": "Buscando el DMD…",
        "found": "DMD encontrado: {kind} v{ver} ({ip})", "notfound": "DMD no encontrado (ver pestaña Logs). Escriba su dirección.",
        "start": "Iniciar medición", "stop": "Detener", "resume": "Reanudar DMD", "st_on": "Midiendo: el DMD muestra sus lecturas en el panel",
        "collecting": "Recopilando datos del informe (configuración del DMD, pings)…",
        "st_off": "Medición detenida", "noip": "Falta la dirección del DMD", "err": "Error de comunicación: {e}",
        "k_rssi": "Señal", "k_q": "Calidad", "k_ssid": "Red", "k_ch": "Canal", "k_ap": "Punto de acceso", "k_ip": "IP DMD / puerta de enlace",
        "k_rb": "Recalbox probada", "k_loss": "Pérdida", "k_lat": "Latencia mín/med/máx", "k_jit": "Jitter", "k_age": "Último paquete",
        "k_gaps": "Cortes UDP", "k_disc": "Reconexiones Wi-Fi", "k_last": "Última desconexión", "k_heap": "Memoria libre (mín)",
        "k_up": "En marcha desde", "k_ping": "Ping PC → DMD", "k_http": "HTTP PC → DMD", "none": "sin respuesta", "never": "ninguna",
        "ago": "hace", "ev_c": "conectado al punto de acceso", "ev_d": "desconectado — ", "hist": "Historial Wi-Fi (reciente arriba)",
        "rssi": "Señal (dBm)", "latc": "Latencia Recalbox (ms)",
        "port": "Puerto:", "refresh": "Actualizar", "baud": "Baudios:", "connect": "Conectar", "disconnect": "Desconectar",
        "reset": "Reiniciar la placa", "clear": "Borrar", "hide": "Ocultar líneas que contengan:", "hidden": "{n} línea(s) oculta(s)",
        "noport": "Ningún puerto serie (conecte el ESP32 por USB).", "opened": "Conectado a {p} ({b} baudios) — la placa NO se reinicia.",
        "closed": "Desconectado.", "openerr": "No se puede abrir: {e}", "nopyserial": "Falta pyserial: el monitor USB no está disponible.",
        "rep_html": "Informe (navegador)", "src": "Fuente:", "src_usb": "USB (serie)", "src_wifi": "Wi-Fi (eventos)", "log": " Registro ", "follow": "Seguir", "unfollow": "Dejar de seguir", "wlog_on": "Siguiendo {ip} por Wi-Fi: eventos reconstruidos a partir de las lecturas del DMD (no expone su registro serie por Wi-Fi).", "wlog_off": "Seguimiento Wi-Fi detenido.", "wlog_noip": "Escriba la dirección del DMD en el marco Wi-Fi (o pulse Buscar).", "rep_btn": "Vista de texto", "copy": "Copiar informe", "export": "Exportar (.zip)", "close": "Cerrar",
        "copied": "Informe copiado al portapapeles.", "exported": "Exportación escrita:\n{p}", "rep_title": "Informe de diagnóstico",
        "hw": "Hardware detectado: {hw}", "r_help": "El zip contiene el informe (PC, puertos USB, DMD, lecturas), el registro serie completo y las lecturas en tabla. Las contraseñas se ocultan.",
    },
}

_PAL_LIGHT = ("#1B8A3A", "#B7791F", "#C62828")
_PAL_DARK = ("#4ADE80", "#FACC15", "#F87171")   # fonds sombres du theme : les verts / rouges clairs restent lisibles
GOOD, WARN, BAD = _PAL_LIGHT
MAX_WIDGET_LINES = 3000


class DiagTab:
    def __init__(self, gui, parent):
        self.gui, self.parent = gui, parent
        c = {}
        try:
            c = gui._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        self.bg = c.get("bg_main", "#F3F3F3")
        self.fg = c.get("fg_text", "#000000")
        self.bg_action = c.get("bg_button_action", "#FFD400")
        self.bg_btn = c.get("bg_button_normal", "#FFFFFF")
        self.wifi: dg.WifiDiag | None = None
        self.ser: dg.SerialSession | None = None
        self.wlog: dg.WifiLogger | None = None
        self.load: dict | None = None
        self.ports: list[dict] = []
        self.q: queue.Queue = queue.Queue()
        self.cells: dict[str, tk.Label] = {}
        self._keylbls: list = []     # libelles de cle des mesures (gris clair du theme)
        self._plain: list = []       # autres libelles dont la couleur de texte suit le theme
        self._i18n: list = []
        self._last_d: dict | None = None
        self._build()
        self.refresh_ports()
        self._pump()
        try:
            gui.nb_top.bind("<<NotebookTabChanged>>", self._on_tab_changed, add="+")
        except Exception:   # noqa: BLE001
            pass

    # ------------------------------------------------------------------ utilitaires
    def t(self, key, **kw):
        try:
            lang = self.gui.lang_var.get()
        except Exception:   # noqa: BLE001
            lang = "fr"
        s = T.get(lang, T["fr"]).get(key) or T["fr"][key]
        return s.format(**kw) if kw else s

    @property
    def lang(self) -> str:
        try:
            v = self.gui.lang_var.get()
        except Exception:   # noqa: BLE001
            v = "fr"
        return v if v in T else "fr"

    def _label(self, parent, key, **kw):
        w = tk.Label(parent, text=self.t(key), bg=self.bg, fg=self.fg, **kw)
        self._i18n.append((w, key))
        return w

    def _button(self, parent, key, command, action=False, **kw):
        b = tk.Button(parent, text=self.t(key), command=command, bg=(self.bg_action if action else self.bg_btn),
                      fg="#000000" if action else self.fg, bd=2, relief="solid", padx=8, pady=2, **kw)
        self._i18n.append((b, key))
        return b

    def _post(self, fn, *a):
        try:
            self.q.put(("call", fn, a))
        except Exception:   # noqa: BLE001
            pass

    # ------------------------------------------------------------------ construction
    def _build(self):
        p = self.parent
        tk.Label(p, text=self.t("intro"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), justify="left", anchor="w",
                 wraplength=1040).pack(fill="x", padx=10, pady=(4, 2))
        self._i18n.append((p.winfo_children()[-1], "intro"))
        cols = tk.Frame(p, bg=self.bg)
        cols.pack(fill="both", expand=True, padx=10)
        cols._no_bg_slice = True        # fond uni : le theme decoupe l'image de fond dans chaque Frame (carres blancs derriere les libelles)
        cols.columnconfigure(0, weight=1, uniform="c")
        cols.columnconfigure(1, weight=1, uniform="c")
        cols.rowconfigure(0, weight=1)
        left = tk.LabelFrame(cols, text=self.t("wifi"), bg=self.bg, fg=self.fg, bd=2, relief="solid")
        right = tk.LabelFrame(cols, text=self.t("log"), bg=self.bg, fg=self.fg, bd=2, relief="solid")
        left.grid(row=0, column=0, sticky="nsew", padx=(0, 4))
        right.grid(row=0, column=1, sticky="nsew", padx=(4, 0))
        self._frames = (left, right)
        self._build_wifi(left)
        self._build_usb(right)
        bar = tk.Frame(p, bg=self.bg)
        bar.pack(fill="x", padx=10, pady=(3, 3))
        bar._no_bg_slice = True
        self._button(bar, "rep_html", self.open_html).pack(side="left")
        self._button(bar, "rep_btn", self.show_report).pack(side="left", padx=(6, 0))
        self._button(bar, "copy", self.copy_report).pack(side="left", padx=6)
        self._button(bar, "export", self.export, action=True).pack(side="left")
        self.hw_lbl = tk.Label(bar, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8))
        self.hw_lbl.pack(side="right")

    def _build_wifi(self, f):
        top = tk.Frame(f, bg=self.bg)
        top.pack(fill="x", padx=6, pady=(4, 2))
        self._label(top, "ip").pack(side="left")
        self.ip_var = tk.StringVar()
        try:
            import RecalBoxDMD_prefs as prefs
            self.ip_var.set(str(prefs.get("dmd_ip") or ""))
        except Exception:   # noqa: BLE001
            pass
        ttk.Entry(top, textvariable=self.ip_var, width=22).pack(side="left", padx=4)      # ligne 1 : l'adresse du DMD, seule
        btns = tk.Frame(f, bg=self.bg)                                                    # ligne 2 : tous les boutons
        btns.pack(fill="x", padx=6, pady=(0, 2))
        self._button(btns, "find", self.find).pack(side="left")
        self.btn_run = self._button(btns, "start", self.toggle, action=True)
        self.btn_run.pack(side="left", padx=6)
        self._button(btns, "resume", self.resume).pack(side="left")
        self.status = tk.Label(f, text=self.t("st_off"), bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8))   # ligne 3 : etat
        self.status.pack(fill="x", padx=6)
        grid = tk.Frame(f, bg=self.bg)
        grid.pack(fill="x", padx=6, pady=2)
        keys = ["k_rssi", "k_q", "k_ssid", "k_ch", "k_ap", "k_ip", "k_rb", "k_loss", "k_lat", "k_jit", "k_age", "k_gaps", "k_disc",
                "k_last", "k_heap", "k_up", "k_ping", "k_http"]
        for i, k in enumerate(keys):
            r, c = divmod(i, 3)
            cell = tk.Frame(grid, bg=self.bg)
            cell.grid(row=r, column=c, sticky="w", padx=(0, 8), pady=0)
            kl = tk.Label(cell, text=self.t(k), bg=self.bg, fg="#666666", font=("TkDefaultFont", 7), anchor="w")
            kl.pack(anchor="w")
            self._i18n.append((kl, k))
            self._keylbls.append(kl)
            v = tk.Label(cell, text="-", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 9, "bold"), anchor="w")
            v.pack(anchor="w")
            self.cells[k] = v
        for c in range(3):
            grid.columnconfigure(c, weight=1, uniform="g")
        self.cv_rssi = tk.Canvas(f, height=28, bg="#16213E", highlightthickness=0)
        self.cv_rssi.pack(fill="x", padx=6, pady=(4, 0))
        self.cv_lat = tk.Canvas(f, height=28, bg="#16213E", highlightthickness=0)
        self.cv_lat.pack(fill="x", padx=6, pady=(2, 0))
        self.hist_lbl = self._label(f, "hist", font=("TkDefaultFont", 7), anchor="w")
        self.hist_lbl.pack(fill="x", padx=6, pady=(2, 0))
        self.hist = tk.Listbox(f, height=2, bg="#FFFFFF", fg="#000000", font=("TkDefaultFont", 8), activestyle="none")
        self.hist.pack(fill="x", padx=6, pady=(0, 3))

    def _build_usb(self, f):
        self.src_var = tk.StringVar(value="usb")
        rs = tk.Frame(f, bg=self.bg)                                   # source du journal : cable USB, ou Wi-Fi (DMD sans cable)
        rs.pack(fill="x", padx=6, pady=(4, 0))
        self._label(rs, "src").pack(side="left")
        for val, key in (("usb", "src_usb"), ("wifi", "src_wifi")):
            rb = tk.Radiobutton(rs, text=self.t(key), variable=self.src_var, value=val, command=self._on_src, bg=self.bg, fg=self.fg,
                                selectcolor=self.bg, activebackground=self.bg)
            rb.pack(side="left", padx=(8, 0))
            self._i18n.append((rb, key))
        self.r1 = tk.Frame(f, bg=self.bg)
        self.r1.pack(fill="x", padx=6, pady=(4, 2))
        self._label(self.r1, "port").pack(side="left")
        self.port_var = tk.StringVar()
        self.port_cb = ttk.Combobox(self.r1, textvariable=self.port_var, width=30, state="readonly")
        self.port_cb.pack(side="left", padx=4)
        self._button(self.r1, "refresh", self.refresh_ports).pack(side="left")
        self.r2 = tk.Frame(f, bg=self.bg)
        self.r2.pack(fill="x", padx=6, pady=2)
        self.baud_fr = tk.Frame(self.r2, bg=self.bg)
        self.baud_fr.pack(side="left")
        self._label(self.baud_fr, "baud").pack(side="left")
        self.baud_var = tk.StringVar(value="115200")
        ttk.Combobox(self.baud_fr, textvariable=self.baud_var, values=["9600", "74880", "115200", "230400", "460800", "921600"], width=8).pack(side="left", padx=4)
        self.btn_conn = self._button(self.r2, "connect", self.toggle_serial, action=True)
        self.btn_conn.pack(side="left", padx=4)
        self.btn_reset = self._button(self.r2, "reset", self.reset_board)
        self.btn_reset.pack(side="left")
        self._button(self.r2, "clear", self.clear_log).pack(side="left", padx=4)
        self.r3 = tk.Frame(f, bg=self.bg)
        self.r3.pack(fill="x", padx=6, pady=2)
        self._label(self.r3, "hide", font=("TkDefaultFont", 8)).pack(side="left")
        self.hide_var = tk.StringVar(value="WIFIPS")
        ttk.Entry(self.r3, textvariable=self.hide_var, width=18).pack(side="left", padx=4)
        self.hidden_lbl = tk.Label(self.r3, text="", bg=self.bg, fg="#666666", font=("TkDefaultFont", 8))
        self.hidden_lbl.pack(side="left")
        self.ser_status = tk.Label(f, text="", bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8), wraplength=480, justify="left")
        self.ser_status.pack(fill="x", padx=6)
        box = tk.Frame(f, bg=self.bg)
        box.pack(fill="both", expand=True, padx=6, pady=(2, 6))
        self.log = tk.Text(box, height=8, wrap="none", bg="#101418", fg="#D8DEE9", insertbackground="#D8DEE9",
                           font=("Consolas", 8), state="disabled")
        sb = ttk.Scrollbar(box, orient="vertical", command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y")
        self.log.pack(side="left", fill="both", expand=True)
        self.log.tag_configure("crash", foreground="#FF6B6B")
        if dg.serial is None:
            self.ser_status.config(text=self.t("nopyserial"), fg=BAD)

    def _on_src(self):
        wifi = self.src_var.get() == "wifi"
        if wifi and self.ser:                                          # une seule source a la fois
            self.close_serial()
        if not wifi and self.wlog:
            self._stop_wlog()
        for w in (self.r1, self.r3, self.baud_fr, self.btn_reset):
            w.pack_forget()
        if not wifi:
            self.r1.pack(fill="x", padx=6, pady=(4, 2), before=self.r2)
            self.baud_fr.pack(side="left", before=self.btn_conn)
            self.btn_reset.pack(side="left", after=self.btn_conn)
            self.r3.pack(fill="x", padx=6, pady=2, before=self.ser_status)
        self._refresh_conn_btn()
        self.ser_status.config(text="", fg=self.fg)

    def _refresh_conn_btn(self):
        wifi = self.src_var.get() == "wifi"
        on = bool(self.wlog and self.wlog.running) if wifi else bool(self.ser and self.ser.is_open)
        try:
            self.btn_conn.config(text=self.t(("unfollow" if on else "follow") if wifi else ("disconnect" if on else "connect")))
        except tk.TclError:
            pass

    def _toggle_wlog(self):
        if self.wlog and self.wlog.running:
            self._stop_wlog()
            return
        ip = self.ip_var.get().strip()
        if not ip:
            self.ser_status.config(text=self.t("wlog_noip"), fg=BAD)
            return
        w = dg.WifiLogger(ip, on_line=lambda ln: self.q.put(("line", ln)), lang=self.lang)
        self.wlog = w
        w.start()
        self._refresh_conn_btn()
        self.ser_status.config(text=self.t("wlog_on", ip=ip), fg=GOOD)

    def _stop_wlog(self):
        w, self.wlog = self.wlog, None
        if w:
            self._kept = (list(w.lines), 0, [], f"Wi-Fi {w.ip}", 0)      # le journal reste exportable apres l'arret
            threading.Thread(target=w.stop, daemon=True).start()
        self._refresh_conn_btn()
        try:
            self.ser_status.config(text=self.t("wlog_off"), fg=self.fg)
        except tk.TclError:
            pass

    # ------------------------------------------------------------------ langue / theme
    def apply_language(self):
        for w, key in self._i18n:
            try:
                w.config(text=self.t(key))
            except tk.TclError:
                pass
        try:
            self._frames[0].config(text=self.t("wifi"))
            self._frames[1].config(text=self.t("log"))
            self.btn_run.config(text=self.t("stop" if self.wifi and self.wifi.running else "start"))
            self._refresh_conn_btn()
            self.status.config(text=self.t("st_on" if self.wifi and self.wifi.running else "st_off"))
        except tk.TclError:
            pass

    def _apply_palette(self):
        global GOOD, WARN, BAD
        h = self.bg.lstrip("#")
        try:
            lum = 0.299 * int(h[0:2], 16) + 0.587 * int(h[2:4], 16) + 0.114 * int(h[4:6], 16)
        except (ValueError, IndexError):
            lum = 255
        new = _PAL_DARK if lum < 140 else _PAL_LIGHT
        old = (GOOD, WARN, BAD)
        if new == old:
            return
        m = {o.lower(): n for o, n in zip(old, new)}
        GOOD, WARN, BAD = new
        for w in list(self.cells.values()) + [self.status, self.ser_status]:
            try:
                fg = str(w.cget("fg")).lower()
                if fg in m:
                    w.config(fg=m[fg])
            except tk.TclError:
                pass

    def apply_theme(self):
        """Le theme est applique APRES la construction : fonds des conteneurs / libelles = bg_main courant (les zones de mesure et le
        journal serie gardent leur fond sombre fixe). Appele apres un changement de theme et a chaque affichage de l'onglet."""
        c = {}
        try:
            c = self.gui._theme_colors()
        except Exception:   # noqa: BLE001
            pass
        self.bg = c.get("bg_main", self.bg)
        self.fg = c.get("fg_text", self.fg)
        light = c.get("fg_text_light", "#666666")
        self._apply_palette()

        def walk(w):
            for ch in w.winfo_children():
                cls = ch.winfo_class()
                try:
                    if cls in ("Frame", "Labelframe"):
                        ch.config(bg=self.bg)
                        if cls == "Labelframe":
                            ch.config(fg=self.fg)
                    elif cls == "Label":
                        ch.config(bg=self.bg)
                        if ch in self._keylbls:
                            ch.config(fg=light)
                        elif ch.cget("fg") in ("#000000", "black", "#F3F3F3") or ch in self._plain:
                            ch.config(fg=self.fg)
                except tk.TclError:
                    pass
                walk(ch)
        walk(self.parent)
        for k, v in self.cells.items():
            if str(v.cget("fg")).lower() not in (GOOD.lower(), WARN.lower(), BAD.lower()):
                v.config(fg=self.fg)

    # ------------------------------------------------------------------ Wi-Fi
    def find(self):
        self.status.config(text=self.t("searching"), fg=self.fg)
        known = self.ip_var.get().strip() or None

        def work():
            try:
                import RecalBoxDMD_tool as tool
                r = tool.find_dmd(known)
            except Exception:   # noqa: BLE001
                r = None
            self._post(self._found, r)
        threading.Thread(target=work, daemon=True).start()

    def _found(self, r):
        if not r:
            self.status.config(text=self.t("notfound"), fg=BAD)
            return
        self.ip_var.set(r["ip"])
        kind = "Raspberry Pi" if r.get("is_pi") or r.get("platform") == "rpi" else "ESP32"
        self.status.config(text=self.t("found", kind=kind, ver=r.get("version", "?"), ip=r["ip"]), fg=GOOD)

    def toggle(self):
        if self.wifi and self.wifi.running:
            w, self.wifi = self.wifi, self.wifi
            threading.Thread(target=lambda: w.stop(resume=False), daemon=True).start()
            self.btn_run.config(text=self.t("start"))
            self.status.config(text=self.t("st_off"), fg=self.fg)
            return
        ip = self.ip_var.get().strip()
        if not ip:
            self.status.config(text=self.t("noip"), fg=BAD)
            return
        self.wifi = dg.WifiDiag(ip, on_sample=lambda d: self.q.put(("sample", d)), on_error=lambda e: self.q.put(("err", e)))
        self.wifi.start()
        self.btn_run.config(text=self.t("stop"))
        self.status.config(text=self.t("st_on"), fg=GOOD)

    def resume(self):
        ip = self.ip_var.get().strip()
        w = self.wifi
        if w and w.running:
            self.btn_run.config(text=self.t("start"))
            self.status.config(text=self.t("st_off"), fg=self.fg)
            threading.Thread(target=lambda: w.stop(resume=True), daemon=True).start()
        elif ip:
            threading.Thread(target=lambda: dg.http_json(ip, "/dmd-resume", "POST", 4), daemon=True).start()

    def stop_all(self, resume: bool = True):
        """Fermeture du toolkit : arrete la mesure Wi-Fi (le DMD reprend), le suivi Wi-Fi et ferme le port serie."""
        self.stop_measure(resume)
        if self.ser:
            self.close_serial()
        if self.wlog:
            self._stop_wlog()

    def stop_measure(self, resume: bool = True):
        """Arrete SEULEMENT la mesure Wi-Fi (le DMD reprend) : appele en quittant l'onglet. Le journal USB / le suivi Wi-Fi continuent en arriere-plan."""
        w = self.wifi
        if w and w.running:
            threading.Thread(target=lambda: w.stop(resume=resume), daemon=True).start()
            try:
                self.btn_run.config(text=self.t("start"))
                self.status.config(text=self.t("st_off"), fg=self.fg)
            except tk.TclError:
                pass

    def _on_tab_changed(self, _e=None):
        try:
            if self.gui.nb_top.select() != str(self.parent):
                w = self.wifi
                if w and w.running:       # comme la page web : la mesure s'arrete des que l'onglet est masque
                    self.stop_measure(resume=True)
            else:
                self.parent.after(450, self.apply_theme)   # apres le decoupage du theme (root.after(400) dans themes.apply)
        except Exception:   # noqa: BLE001
            pass

    def _cell(self, k, text, color=None):
        self.cells[k].config(text=text, fg=color or self.fg)

    def _draw(self, d):
        self._last_d = d
        up = bool(d.get("up"))
        if not up:
            self._cell("k_rssi", "-", BAD)
            self._cell("k_q", dg.QUALITY[self.lang][5], BAD)
        else:
            lvl, qi = dg.quality(d["rssi"])
            col = {"good": GOOD, "warn": WARN, "bad": BAD}[lvl]
            self._cell("k_rssi", f"{d['rssi']} dBm", col)
            self._cell("k_q", dg.QUALITY[self.lang][qi], col)
        self._cell("k_ssid", d.get("ssid") or "-")
        self._cell("k_ch", d.get("ch") if up else "-")
        self._cell("k_ap", d.get("bssid") or "-")
        self._cell("k_ip", f"{d.get('ip')} / {d.get('gw')}" if up else "-")
        self._cell("k_rb", d.get("rb") or "-")
        if not d.get("rb") or not d.get("n"):
            for k in ("k_loss", "k_lat", "k_jit"):
                self._cell(k, "-")
        else:
            loss = d.get("loss", 0)
            self._cell("k_loss", f"{loss} % ({d.get('got')}/{d.get('n')})", GOOD if loss == 0 else WARN if loss <= 10 else BAD)
            if not d.get("got"):
                self._cell("k_lat", self.t("none"), BAD)
                self._cell("k_jit", "-")
            else:
                a = d.get("avg", 0)
                self._cell("k_lat", f"{d.get('min')} / {a} / {d.get('max')} ms", GOOD if a < 80 else WARN if a < 200 else BAD)
                j = d.get("jit", 0)
                self._cell("k_jit", f"{j} ms", GOOD if j < 10 else WARN if j <= 40 else BAD)
        age = d.get("age", -1)
        self._cell("k_age", "-" if age < 0 else f"{age / 1000:.1f} s", None if age < 0 else GOOD if age < 2000 else WARN if age < 15000 else BAD)
        self._cell("k_gaps", d.get("gaps", 0), GOOD if d.get("gaps", 0) == 0 else WARN)
        disc = d.get("disc", 0)
        self._cell("k_disc", disc, GOOD if disc == 0 else WARN if disc < 3 else BAD)
        ups = d.get("up_s", 0)
        self._cell("k_last", (f"{dg.reason_text(d.get('dr'), self.lang)} ({self.t('ago')} {dg.fmt_ago(ups - d.get('da', 0))})" if disc else self.t("never")))
        self._cell("k_heap", f"{round(d.get('heap', 0) / 1024)} Ko ({round(d.get('minheap', 0) / 1024)} Ko)")
        self._cell("k_up", f"{ups // 3600} h {ups % 3600 // 60} min" if ups >= 3600 else f"{ups // 60} min")
        if self.wifi and self.wifi.samples:
            s = self.wifi.samples[-1]
            self._cell("k_ping", "-" if s["ping_ms"] is None else f"{s['ping_ms']:.0f} ms", BAD if s["ping_ms"] is None else None)
            self._cell("k_http", "-" if s["http_ms"] is None else f"{s['http_ms']:.0f} ms")
        self._spark(self.cv_rssi, [None if v <= -127 else v for v in d.get("rs", [])], -95, -30, False)
        self._spark(self.cv_lat, [None if v < 0 else v for v in d.get("lat", [])], 0, max(100, d.get("max", 0)), True)
        self.hist.delete(0, "end")
        for e in d.get("ev", []):
            ago = f"{self.t('ago')} {dg.fmt_ago(ups - e[1])}"
            self.hist.insert("end", f"{ago} : " + (self.t("ev_d") + dg.reason_text(e[2], self.lang) if e[0] == "D" else f"{self.t('ev_c')} ({e[2]})"))
        if self.wifi and self.wifi.load:
            self.hw_lbl.config(text=self.t("hw", hw=dg.device_kind(self.wifi.load, d)))

    def _spark(self, cv, vals, lo, hi, lost_mark):
        cv.delete("all")
        w, h = max(cv.winfo_width(), 200), int(cv.cget("height"))
        if not vals:
            return
        for fr in (0.25, 0.5, 0.75):
            cv.create_line(0, h * fr, w, h * fr, fill="#2A3558")
        dx = w / max(19, len(vals) - 1)
        pts, pen = [], False
        for i, v in enumerate(vals):
            if v is None:
                if len(pts) >= 4:
                    cv.create_line(*pts, fill="#8AB4F8", width=2)
                pts = []
                if lost_mark:
                    cv.create_rectangle(i * dx - 2, h - 10, i * dx + 2, h - 2, fill="#F87171", outline="")
                continue
            y = h - 4 - (min(max(v, lo), hi) - lo) / (hi - lo) * (h - 8)
            pts += [i * dx, y]
        if len(pts) >= 4:
            cv.create_line(*pts, fill="#8AB4F8", width=2)
        cv.create_text(4, 2, anchor="nw", fill="#9CA3AF", font=("TkDefaultFont", 7), text=self.t("rssi" if cv is self.cv_rssi else "latc"))

    # ------------------------------------------------------------------ USB
    def refresh_ports(self):
        self.ports = dg.list_serial_ports()
        labels = [f"{p['device']} — {p['chip'] or p['description']}" for p in self.ports]
        self.port_cb["values"] = labels
        if labels and not self.port_var.get():
            self.port_var.set(labels[0])
        elif not labels:
            self.port_var.set("")
            if dg.serial is not None:
                self.ser_status.config(text=self.t("noport"), fg=self.fg)

    def _port_device(self):
        v = self.port_var.get()
        return v.split(" — ")[0].strip() if v else ""

    def toggle_serial(self):
        if self.src_var.get() == "wifi":
            self._toggle_wlog()
            return
        if self.ser and self.ser.is_open:
            self.close_serial()
            return
        dev = self._port_device()
        if not dev or dg.serial is None:
            self.ser_status.config(text=self.t("noport" if dg.serial else "nopyserial"), fg=BAD)
            return
        try:
            baud = int(self.baud_var.get())
        except ValueError:
            baud = 115200
        s = dg.SerialSession(dev, baud, on_line=lambda ln: self.q.put(("line", ln)))
        s.hide = [h.strip() for h in self.hide_var.get().split(",") if h.strip()]
        try:
            s.open()
        except Exception as e:   # noqa: BLE001
            self.ser_status.config(text=self.t("openerr", e=f"{type(e).__name__}: {e}"), fg=BAD)
            return
        self.ser = s
        self.btn_conn.config(text=self.t("disconnect"))
        self.ser_status.config(text=self.t("opened", p=dev, b=baud), fg=GOOD)

    def close_serial(self):
        s, self.ser = self.ser, None
        if s:
            self._kept = (list(s.lines), s.hidden, s.hide, s.port, s.baud)    # le journal reste exportable apres la deconnexion
            threading.Thread(target=s.close, daemon=True).start()
        try:
            self.btn_conn.config(text=self.t("connect"))
            self.ser_status.config(text=self.t("closed"), fg=self.fg)
        except tk.TclError:
            pass

    def reset_board(self):
        if self.ser and self.ser.is_open:
            threading.Thread(target=self.ser.reset_board, daemon=True).start()

    def clear_log(self):
        self.log.config(state="normal")
        self.log.delete("1.0", "end")
        self.log.config(state="disabled")
        if self.ser:
            self.ser.lines.clear()
            self.ser.hidden = 0
        if self.wlog:
            self.wlog.lines.clear()

    def _append_lines(self, lines):
        at_end = self.log.yview()[1] >= 0.999
        self.log.config(state="normal")
        for ln in lines:
            crash = any(p.lower() in ln.lower() for p in dg.CRASH_PATTERNS)
            self.log.insert("end", ln + "\n", ("crash",) if crash else ())
        n = int(self.log.index("end-1c").split(".")[0])
        if n > MAX_WIDGET_LINES:                        # l'affichage est plafonne ; le journal complet reste dans la session (export)
            self.log.delete("1.0", f"{n - MAX_WIDGET_LINES + 500}.0")
        self.log.config(state="disabled")
        if at_end:
            self.log.see("end")

    # ------------------------------------------------------------------ boucle d'interface (fils -> Tk, par lots)
    def _pump(self):
        try:
            if not self.parent.winfo_exists():
                return
        except tk.TclError:
            return
        try:                               # le theme (aleatoire ou choisi) est applique apres la construction : suivre ses couleurs
            if self.gui._theme_colors().get("bg_main", self.bg) != self.bg:
                self.apply_theme()
        except Exception:   # noqa: BLE001
            pass
        lines, last_sample, t0 = [], None, time.perf_counter()
        try:
            while time.perf_counter() - t0 < 0.05:
                kind, *rest = self.q.get_nowait()
                if kind == "line":
                    lines.append(rest[0])
                elif kind == "sample":
                    last_sample = rest[0]
                elif kind == "err":
                    self.status.config(text=self.t("err", e=rest[0]), fg=BAD)
                elif kind == "call":
                    rest[0](*rest[1])
        except queue.Empty:
            pass
        if lines:
            self._append_lines(lines)
        if last_sample is not None:
            self._draw(last_sample)
        if self.ser:
            self.ser.hide = [h.strip() for h in self.hide_var.get().split(",") if h.strip()]
            n_hid = self.ser.hidden + self.ser.garbage
            self.hidden_lbl.config(text=self.t("hidden", n=n_hid) if n_hid else "")
            if self.ser.error and self.ser.is_open is False:
                self.close_serial()
        self.parent.after(100, self._pump)

    # ------------------------------------------------------------------ rapport / export
    def _report(self) -> str:
        return dg.build_report(*self._report_args(), lang=self.lang)

    def _report_html(self) -> str:
        return dg.build_report_html(*self._report_args(), lang=self.lang)

    def _report_args(self):
        if self.ser:
            lines, hidden, hide, port, baud = list(self.ser.lines), self.ser.hidden, self.ser.hide, self.ser.port, self.ser.baud
        elif self.wlog:
            lines, hidden, hide, port, baud = list(self.wlog.lines), 0, [], f"Wi-Fi {self.wlog.ip}", 0
        elif getattr(self, "_kept", None):
            lines, hidden, hide, port, baud = self._kept
        else:
            lines, hidden, hide, port, baud = [], 0, [], "", 0
        try:                      # version lue dans le module du GUI (le titre de la fenetre change : suffixe de cible Pi)
            import sys
            tool_id = "build " + str(sys.modules["RecalBoxDMD_GUI"].TOOLKIT_RELEASE_VERSION)
        except Exception:   # noqa: BLE001
            try:
                tool_id = self.gui.root.title()
            except Exception:   # noqa: BLE001
                tool_id = ""
        return (dg.pc_info(tool_id), dg.list_serial_ports(), self.ip_var.get().strip(), self.wifi, lines, port, baud, hidden, hide)

    def _collect(self, then):
        """Releves ponctuels du rapport (config du DMD, pings PC -> DMD / passerelle / Recalbox) dans un fil, puis `then()` dans l'interface.
        Adresse du DMD vide : on le CHERCHE sur le reseau (sinon le rapport n'aurait aucune partie Wi-Fi quand seul l'USB a servi)."""
        ip0 = self.ip_var.get().strip()
        self.status.config(text=self.t("collecting"), fg=self.fg)

        def work():
            ip = ip0
            if not ip:
                try:
                    import RecalBoxDMD_tool as tool
                    r = tool.find_dmd(None)
                    ip = (r or {}).get("ip", "")
                except Exception:   # noqa: BLE001
                    ip = ""
                if ip:
                    self._post(self.ip_var.set, ip)
            if ip:
                if self.wifi is None or self.wifi.ip != ip:
                    self.wifi = dg.WifiDiag(ip)
                try:
                    self.wifi.collect_extra()
                except Exception:   # noqa: BLE001
                    pass
            self._post(then)
        threading.Thread(target=work, daemon=True).start()

    def show_report(self):
        self._collect(self._show_report)

    def _show_report(self):
        win = tk.Toplevel(self.gui.root)
        win.title(self.t("rep_title"))
        win.configure(bg=self.bg)
        txt = tk.Text(win, width=110, height=32, font=("Consolas", 9), wrap="none")
        txt.pack(fill="both", expand=True, padx=8, pady=8)
        txt.insert("1.0", self._report())
        row = tk.Frame(win, bg=self.bg)
        row.pack(fill="x", padx=8, pady=(0, 8))
        tk.Button(row, text=self.t("copy"), command=lambda: self._clip(txt.get("1.0", "end"))).pack(side="left")
        tk.Button(row, text=self.t("close"), command=win.destroy).pack(side="right")

    def _clip(self, text):
        self.gui.root.clipboard_clear()
        self.gui.root.clipboard_append(text)
        self.status.config(text=self.t("copied"), fg=GOOD)

    def copy_report(self):
        self._collect(lambda: self._clip(self._report()))

    def export(self):
        self._collect(self._export)

    def _export(self):
        rep = self._report()
        name = f"diagnostic_dmd_{datetime.now():%Y%m%d_%H%M%S}.zip"
        path = filedialog.asksaveasfilename(parent=self.gui.root, defaultextension=".zip", initialfile=name, filetypes=[("ZIP", "*.zip")])
        if not path:
            return
        lines = self._report_args()[4]
        try:
            dg.export_zip(path, rep, self.wifi, lines, html=self._report_html())
        except Exception as e:   # noqa: BLE001
            messagebox.showerror(self.t("rep_title"), f"{type(e).__name__}: {e}", parent=self.gui.root)
            return
        self.status.config(text=self.t("st_off") if not (self.wifi and self.wifi.running) else self.t("st_on"), fg=self.fg)
        messagebox.showinfo(self.t("rep_title"), self.t("exported", p=path), parent=self.gui.root)

    def open_html(self):
        self._collect(self._open_html)

    def _open_html(self):
        import os
        import tempfile
        p = os.path.join(tempfile.gettempdir(), f"diagnostic_dmd_{datetime.now():%Y%m%d_%H%M%S}.html")
        try:
            with open(p, "w", encoding="utf-8") as fh:
                fh.write(self._report_html())
            os.startfile(p)   # type: ignore[attr-defined]
        except Exception as e:   # noqa: BLE001
            messagebox.showerror(self.t("rep_title"), f"{type(e).__name__}: {e}", parent=self.gui.root)
