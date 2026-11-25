# -*- coding: utf-8 -*-
# RecalBoxDMD_diag_gui.py -- onglet « Diagnostic » du toolkit (Wi-Fi comme la page /config/diag du DMD + moniteur serie USB + export .zip).
#
# v1 - 2026-10-31 - safe-modify - Creation. Construit par RetroBoxLEDGui._build_diag_tab() ; la langue suit gui.lang_var, les
#   couleurs gui._theme_colors(). Logique (HTTP, serie, rapport, zip) dans RecalBoxDMD_diag.py. Une panne de ce module ne doit
#   jamais empecher l'ouverture du toolkit. Le serveur du DMD est mono-connexion : UNE seule mesure a la fois ; elle s'arrete en
#   quittant l'onglet (comme la page web qui s'arrete quand elle est masquee) et a la fermeture du toolkit.
#
# v5 - 2026-11-25 - safe-modify - LIBELLES : en STABLE le bouton s'appelle « Demande d'aide… » (et non « Envoyer a Discord ») ; la fenetre d'une demande d'aide (stable ou alpha) annonce qu'elle part vers le serveur Discord du projet (salon de l'equipe de support).
#
# v7 - 2026-11-25 - safe-modify - FENETRE « TESTS ALPHA » (build alpha seulement) : bouton « Tests alpha... » du bloc Discord ; le plan de tests du Discord (docs/alpha-tests.json) se remplit depuis le toolkit, le resultat part avec les versions reellement detectees (bloc ALPHA-RESULT, module RecalBoxDMD_alpha_tests.py).
#
# v6 - 2026-11-25 - safe-modify - BLOC « DISCORD » : plus de choix + bouton « Envoyer a Discord » ; un bouton par action : « Detection des versions... » (alpha seulement) et « Demande d'aide... » (alpha et stable).
#
# v4 - 2026-11-25 - safe-modify - BLOC « DISCORD » : en build STABLE le choix « Detection (tests) » disparait, il ne reste que la demande d'aide (la detection ne sert qu'aux tests alpha ; la demande d'aide envoie deja les versions)
#
# v3 - 2026-11-24 - safe-modify - BLOC « DISCORD » (barre du bas) : envoi au serveur Discord d'une DETECTION des versions reelles (tests alpha / stable) ou d'une DEMANDE D'AIDE (rapport + journal serie, IP / MAC / SSID retires), apres un apercu de ce qui part et un clic sur Envoyer. Logique dans RecalBoxDMD_discord.py ; code d'appairage = commande /dmdlink du serveur ; panne du module = bloc absent, onglet intact.
#
# v2 - 2026-11-09 - safe-modify - LISIBILITE (profite de l'onglet agrandi, voir _set_wide_layout de la GUI v152) : trois CARTES a fond sombre fixe (signal en dBm, latence moyenne, perte de paquets) en gros caracteres colores selon le seuil ; details GROUPES en deux colonnes (reseau et liaison | sante du DMD) au lieu de 18 petites cellules ; DEUX courbes de la hauteur disponible (signal : zones bon / moyen / faible derriere la courbe, graduations adaptees a la hauteur, points, pastille de la valeur courante, dernier / min / moy / max, echelle du temps ; latence : zones, mesures perdues en rouge) ; historique sur fond sombre, deconnexions en rouge et reconnexions en vert ; adresse + boutons sur une ligne ; panneau Wi-Fi 11/20 et journal 9/20 ; barre du bas (rapport + materiel + aide) prioritaire. Aucune valeur ne depend de la longueur d'un texte : valeurs coupees en pixels (_cell_clip), tailles fixes. set_wide() reserve le coin du logo.
from __future__ import annotations

import queue
import re
import threading
import time
import tkinter as tk
from datetime import datetime
from tkinter import filedialog, messagebox, ttk

import RecalBoxDMD_diag as dg

try:                                   # v3 : un module absent ne doit jamais empecher l'onglet de s'ouvrir (le bloc Discord n'est alors pas construit)
    import RecalBoxDMD_discord as dcd
except Exception:   # noqa: BLE001
    dcd = None

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
        "rssi": "Signal Wi-Fi (dBm)", "latc": "Latence DMD → Recalbox (ms)", "pingc": "PC → DMD : ping (vert) · HTTP (ambre) (ms)", "c_rssi": "Signal Wi-Fi", "c_lat": "Latence moyenne", "c_loss": "Perte de paquets",
        "c_lat_sub": "min {mn} · max {mx} ms", "c_loss_sub": "{got}/{n} paquets reçus", "st_stats": "dernier {last} · min {mn} · moy {avg} · max {mx}", "nlast": "← {n} dernières mesures", "now": "maintenant",
        "sec_net": "Réseau et liaison", "sec_health": "Santé du DMD", "lost": "perdu",
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
        "rssi": "Wi-Fi signal (dBm)", "latc": "DMD → Recalbox latency (ms)", "pingc": "PC → DMD: ping (green) · HTTP (amber) (ms)", "c_rssi": "Wi-Fi signal", "c_lat": "Average latency", "c_loss": "Packet loss",
        "c_lat_sub": "min {mn} · max {mx} ms", "c_loss_sub": "{got}/{n} packets received", "st_stats": "last {last} · min {mn} · avg {avg} · max {mx}", "nlast": "← last {n} readings", "now": "now",
        "sec_net": "Network and link", "sec_health": "DMD health", "lost": "lost",
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
        "k_rssi": "Señal", "k_q": "Calidad", "k_ssid": "Red", "k_ch": "Canal", "k_ap": "Punto de acceso", "k_ip": "IP DMD / pasarela",
        "k_rb": "Recalbox probada", "k_loss": "Pérdida", "k_lat": "Latencia mín/med/máx", "k_jit": "Jitter", "k_age": "Último paquete",
        "k_gaps": "Cortes UDP", "k_disc": "Reconexiones Wi-Fi", "k_last": "Última desconexión", "k_heap": "Memoria libre (mín)",
        "k_up": "En marcha desde", "k_ping": "Ping PC → DMD", "k_http": "HTTP PC → DMD", "none": "sin respuesta", "never": "ninguna",
        "ago": "hace", "ev_c": "conectado al punto de acceso", "ev_d": "desconectado — ", "hist": "Historial Wi-Fi (reciente arriba)",
        "rssi": "Señal Wi-Fi (dBm)", "latc": "Latencia DMD → Recalbox (ms)", "pingc": "PC → DMD: ping (verde) · HTTP (ámbar) (ms)", "c_rssi": "Señal Wi-Fi", "c_lat": "Latencia media", "c_loss": "Pérdida de paquetes",
        "c_lat_sub": "mín {mn} · máx {mx} ms", "c_loss_sub": "{got}/{n} paquetes recibidos", "st_stats": "último {last} · mín {mn} · med {avg} · máx {mx}", "nlast": "← últimas {n} lecturas", "now": "ahora",
        "sec_net": "Red y enlace", "sec_health": "Salud del DMD", "lost": "perdido",
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
CARD_BG, CARD_EDGE, CARD_KEY, CARD_TXT = "#16213E", "#2A3558", "#9CA3AF", "#E5E7EB"      # cartes : fond sombre FIXE (comme les courbes) -> grandes valeurs lisibles sur tous les themes
CARD_KEYS = ("k_rssi", "k_q", "k_lat", "_lat_sub", "k_loss", "_loss_sub")


_DC_T = {
    "fr": {
        "dc_title": "Discord", "dc_code": "Code d'appairage :", "dc_kind_test": "Détection (tests)", "dc_kind_support": "Demande d'aide",
        "dc_send_test": "Détection des versions…", "dc_send_tests": "Tests alpha…", "dc_send_help": "Demande d'aide…", "dc_win_title_help": "Demande d'aide", "dc_win_head_help": "Votre demande d'aide sera envoyée au serveur Discord du projet (salon réservé à l'équipe de support). Voici exactement ce qui part (adresses IP, adresses MAC et nom du Wi-Fi retirés) :", "dc_hint": "Code : tapez /dmdlink sur le serveur Discord (discord.gg/CYAyGJSvp4). Rien ne part sans votre clic.",
        "dc_detecting": "Détection en cours…", "dc_sending": "Envoi…", "dc_sent": "Envoyé. Le bot vous répond en message privé sur Discord.",
        "dc_win_title": "Envoyer à Discord", "dc_win_head": "Voici exactement ce qui sera envoyé (adresses IP, adresses MAC et nom du Wi-Fi retirés) :",
        "dc_win_support": "Décrivez votre problème (facultatif, 900 caractères) :", "dc_win_att": "Pièce jointe : diagnostic.zip (rapport + journal série), {n} Ko",
        "dc_win_notes": "Non détecté : {x}", "dc_win_go": "Envoyer", "dc_win_cancel": "Annuler",
        "dc_n_no_dmd_ip": "adresse du DMD vide", "dc_n_dmd_unreachable": "DMD injoignable", "dc_n_recalbox_unreadable": "version de Recalbox illisible",
        "dc_n_no_recalbox": "aucune Recalbox liée",
        "dc_err_code": "Code d'appairage invalide : tapez /dmdlink sur le serveur Discord.", "dc_err_wait": "Patientez encore {s} s avant un nouvel envoi.",
        "dc_err_intake_unreachable": "Service d'envoi injoignable (pas de réseau ?).", "dc_err_intake_invalid": "Service d'envoi indisponible (fichier invalide).",
        "dc_err_intake_disabled": "L'envoi est suspendu par le projet pour le moment.", "dc_err_kind_unavailable": "Ce type d'envoi n'est pas disponible.",
        "dc_err_toolkit_too_old": "Cette version du toolkit est trop ancienne pour l'envoi : mettez-la à jour.",
        "dc_err_rate_limited": "Discord demande de patienter : réessayez dans une minute.", "dc_err_network": "Échec réseau pendant l'envoi.",
        "dc_err_other": "Échec de l'envoi ({c}).",
    },
    "en": {
        "dc_title": "Discord", "dc_code": "Pairing code:", "dc_kind_test": "Detection (tests)", "dc_kind_support": "Help request",
        "dc_send_test": "Detect versions…", "dc_send_tests": "Alpha tests…", "dc_send_help": "Help request…", "dc_win_title_help": "Help request", "dc_win_head_help": "Your help request will be sent to the project's Discord server (a channel reserved for the support team). This is exactly what goes out (IP addresses, MAC addresses and Wi-Fi name removed):", "dc_hint": "Code: type /dmdlink on the Discord server (discord.gg/CYAyGJSvp4). Nothing is sent without your click.",
        "dc_detecting": "Detecting…", "dc_sending": "Sending…", "dc_sent": "Sent. The bot replies to you in a Discord direct message.",
        "dc_win_title": "Send to Discord", "dc_win_head": "This is exactly what will be sent (IP addresses, MAC addresses and Wi-Fi name removed):",
        "dc_win_support": "Describe your problem (optional, 900 characters):", "dc_win_att": "Attachment: diagnostic.zip (report + serial log), {n} KB",
        "dc_win_notes": "Not detected: {x}", "dc_win_go": "Send", "dc_win_cancel": "Cancel",
        "dc_n_no_dmd_ip": "DMD address empty", "dc_n_dmd_unreachable": "DMD unreachable", "dc_n_recalbox_unreadable": "Recalbox version unreadable",
        "dc_n_no_recalbox": "no linked Recalbox",
        "dc_err_code": "Invalid pairing code: type /dmdlink on the Discord server.", "dc_err_wait": "Please wait {s} s before sending again.",
        "dc_err_intake_unreachable": "Sending service unreachable (no network?).", "dc_err_intake_invalid": "Sending service unavailable (invalid file).",
        "dc_err_intake_disabled": "Sending is suspended by the project for now.", "dc_err_kind_unavailable": "This kind of report is not available.",
        "dc_err_toolkit_too_old": "This toolkit version is too old for sending: please update it.",
        "dc_err_rate_limited": "Discord asks you to wait: try again in a minute.", "dc_err_network": "Network failure while sending.",
        "dc_err_other": "Sending failed ({c}).",
    },
    "es": {
        "dc_title": "Discord", "dc_code": "Código de emparejamiento:", "dc_kind_test": "Detección (pruebas)", "dc_kind_support": "Petición de ayuda",
        "dc_send_test": "Detectar versiones…", "dc_send_tests": "Pruebas alfa…", "dc_send_help": "Petición de ayuda…", "dc_win_title_help": "Petición de ayuda", "dc_win_head_help": "Su petición de ayuda se enviará al servidor de Discord del proyecto (un canal reservado al equipo de soporte). Esto es exactamente lo que sale (sin direcciones IP, direcciones MAC ni nombre del Wi-Fi):", "dc_hint": "Código: escriba /dmdlink en el servidor de Discord (discord.gg/CYAyGJSvp4). No se envía nada sin su clic.",
        "dc_detecting": "Detectando…", "dc_sending": "Enviando…", "dc_sent": "Enviado. El bot le responde con un mensaje privado de Discord.",
        "dc_win_title": "Enviar a Discord", "dc_win_head": "Esto es exactamente lo que se enviará (sin direcciones IP, direcciones MAC ni nombre del Wi-Fi):",
        "dc_win_support": "Describa su problema (opcional, 900 caracteres):", "dc_win_att": "Adjunto: diagnostic.zip (informe + registro serie), {n} KB",
        "dc_win_notes": "No detectado: {x}", "dc_win_go": "Enviar", "dc_win_cancel": "Cancelar",
        "dc_n_no_dmd_ip": "dirección del DMD vacía", "dc_n_dmd_unreachable": "DMD inaccesible", "dc_n_recalbox_unreadable": "versión de Recalbox ilegible",
        "dc_n_no_recalbox": "ninguna Recalbox vinculada",
        "dc_err_code": "Código de emparejamiento no válido: escriba /dmdlink en el servidor de Discord.", "dc_err_wait": "Espere {s} s antes de volver a enviar.",
        "dc_err_intake_unreachable": "Servicio de envío inaccesible (¿sin red?).", "dc_err_intake_invalid": "Servicio de envío no disponible (archivo no válido).",
        "dc_err_intake_disabled": "El proyecto ha suspendido el envío por ahora.", "dc_err_kind_unavailable": "Este tipo de envío no está disponible.",
        "dc_err_toolkit_too_old": "Esta versión del toolkit es demasiado antigua para enviar: actualícela.",
        "dc_err_rate_limited": "Discord pide esperar: inténtelo de nuevo en un minuto.", "dc_err_network": "Fallo de red durante el envío.",
        "dc_err_other": "Error de envío ({c}).",
    },
}
for _lg, _tr in _DC_T.items():
    T[_lg].update(_tr)


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
        self._fixed: list = []       # widgets a fond sombre FIXE (cartes de mesure) : ni le theme ni la palette clair / sombre n'y touchent
        self._i18n: list = []
        self._last_d: dict | None = None
        self._build()
        self.refresh_ports()
        self._pump()
        try:
            gui.nb_top.bind("<<NotebookTabChanged>>", self._on_tab_changed, add="+")
        except Exception:   # noqa: BLE001
            pass

    def set_wide(self, wide):
        """Onglet agrandi (cadre Progression retire) : reserve en bas a droite la place du logo / cafe / Discord, pour que rien ne passe dessous."""
        try:
            self._logbox.pack_configure(pady=(2, 12 if wide else 6))
        except tk.TclError:
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
        padx = kw.pop("padx", 8)
        b = tk.Button(parent, text=self.t(key), command=command, bg=(self.bg_action if action else self.bg_btn),
                      fg="#000000" if action else self.fg, bd=2, relief="solid", padx=padx, pady=2, **kw)
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
        bar = tk.Frame(p, bg=self.bg)           # packee AVANT les colonnes (side=bottom) : jamais rognee quand les colonnes demandent beaucoup de hauteur
        bar.pack(side="bottom", fill="x", padx=10, pady=(3, 3))
        cols = tk.Frame(p, bg=self.bg)
        cols.pack(fill="both", expand=True, padx=10)
        cols.columnconfigure(0, weight=11, uniform="c")      # v2 : 11 / 9 (le panneau Wi-Fi porte les valeurs et les trois courbes)
        cols.columnconfigure(1, weight=9, uniform="c")
        cols.rowconfigure(0, weight=1)
        left = tk.LabelFrame(cols, text=self.t("wifi"), bg=self.bg, fg=self.fg, bd=2, relief="solid")
        right = tk.LabelFrame(cols, text=self.t("log"), bg=self.bg, fg=self.fg, bd=2, relief="solid")
        left.grid(row=0, column=0, sticky="nsew", padx=(0, 4))
        right.grid(row=0, column=1, sticky="nsew", padx=(4, 0))
        for fr in (left, right):
            fr._no_bg_slice = True      # cadres DE DONNEES : fond plein (couleur du theme), jamais l'image decoupee derriere les valeurs ; le conteneur autour et la barre du bas, eux, suivent le theme
        self._frames = (left, right)
        self._build_wifi(left)
        self._build_usb(right)
        row = tk.Frame(bar, bg=self.bg)
        row.pack(fill="x")
        self._button(row, "rep_html", self.open_html).pack(side="left")
        self._button(row, "rep_btn", self.show_report).pack(side="left", padx=(6, 0))
        self._button(row, "copy", self.copy_report).pack(side="left", padx=6)
        self._button(row, "export", self.export, action=True).pack(side="left")
        self.hw_lbl = tk.Label(row, text="", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 9, "bold"))
        self.hw_lbl.pack(side="left", padx=14)
        if dcd is not None:
            try:
                self._build_discord(bar)
            except Exception:   # noqa: BLE001  -- le bloc Discord est un bonus : jamais d'onglet casse
                pass
        self.help_lbl = tk.Label(bar, text=self.t("r_help"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), justify="left", anchor="w", wraplength=820)
        self.help_lbl.pack(fill="x", pady=(2, 0))
        self._i18n.append((self.help_lbl, "r_help"))

    def _build_wifi(self, f):
        # v2 : adresse + boutons sur UNE ligne (le haut du panneau ne mange plus la place des courbes)
        top = tk.Frame(f, bg=self.bg)
        top.pack(fill="x", padx=6, pady=(4, 2))
        self._label(top, "ip").pack(side="left")
        self.ip_var = tk.StringVar()
        try:
            import RecalBoxDMD_prefs as prefs
            self.ip_var.set(str(prefs.get("dmd_ip") or ""))
        except Exception:   # noqa: BLE001
            pass
        ttk.Entry(top, textvariable=self.ip_var, width=14).pack(side="left", padx=3)
        self._button(top, "find", self.find, padx=6).pack(side="left")
        self.btn_run = self._button(top, "start", self.toggle, action=True, padx=6)
        self.btn_run.pack(side="left", padx=4)
        self._button(top, "resume", self.resume, padx=6).pack(side="left")
        self.status = tk.Label(f, text=self.t("st_off"), bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 9))   # ligne 2 : etat
        self.status.pack(fill="x", padx=6)

        # trois CARTES pour ce qui compte d'un coup d'oeil (signal, latence, perte) : gros caracteres, colores selon le seuil
        cards = tk.Frame(f, bg=self.bg)
        cards.pack(fill="x", padx=6, pady=(4, 2))
        for c in range(3):
            cards.columnconfigure(c, weight=1, uniform="card")
        for i, (title, main, sub) in enumerate((("c_rssi", "k_rssi", "k_q"), ("c_lat", "k_lat", "_lat_sub"), ("c_loss", "k_loss", "_loss_sub"))):
            card = tk.Frame(cards, bg=CARD_BG, highlightthickness=1, highlightbackground=CARD_EDGE)
            card.grid(row=0, column=i, sticky="nsew", padx=(0 if i == 0 else 4, 0))
            kl = tk.Label(card, text=self.t(title), bg=CARD_BG, fg=CARD_KEY, font=("TkDefaultFont", 8), anchor="w")
            kl.pack(anchor="w", padx=6, pady=(2, 0))
            self._i18n.append((kl, title))
            v = tk.Label(card, text="-", bg=CARD_BG, fg=CARD_TXT, font=("TkDefaultFont", 18, "bold"), anchor="w")
            v.pack(anchor="w", padx=6)
            sb = tk.Label(card, text="", bg=CARD_BG, fg=CARD_TXT, font=("TkDefaultFont", 9, "bold"), anchor="w")
            sb.pack(anchor="w", padx=6, pady=(0, 2))
            self.cells[main], self.cells[sub] = v, sb
            self._fixed.extend((card, kl, v, sb))
            for w in (card, kl, v, sb):
                w._no_bg_slice = True          # le theme decoupe son image de fond derriere chaque Frame / Label : pas ici (fond sombre uni)
            kl._fixed_theme_colors, v._fixed_theme_colors, sb._fixed_theme_colors = (CARD_BG, CARD_KEY), (CARD_BG, CARD_TXT), (CARD_BG, CARD_TXT)      # et il ne repeint pas leurs couleurs (voir _walk_and_apply)

        # details en DEUX groupes cote a cote : « reseau et liaison » | « sante du DMD »
        det = tk.Frame(f, bg=self.bg)
        det.pack(fill="x", padx=6, pady=(4, 0))
        det.columnconfigure(0, weight=6, uniform="grp")       # 6 / 5 : le groupe « reseau et liaison » porte les valeurs les plus longues (IP / passerelle)
        det.columnconfigure(1, weight=5, uniform="grp")
        for col, (title, keys) in enumerate((("sec_net", ("k_ssid", "k_ch", "k_ap", "k_ip", "k_rb", "k_jit", "k_age")),
                                            ("sec_health", ("k_gaps", "k_disc", "k_last", "k_heap", "k_up", "k_ping", "k_http")))):
            grp = tk.Frame(det, bg=self.bg)
            grp.grid(row=0, column=col, sticky="nw", padx=(0 if col == 0 else 6, 0))
            h = tk.Label(grp, text=self.t(title), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 9, "bold"), anchor="w")
            h.grid(row=0, column=0, columnspan=2, sticky="w")
            self._i18n.append((h, title))
            for r, k in enumerate(keys, start=1):
                self._kv(grp, r, 0, k)

        # historique (bas) puis courbes (tout le reste de la hauteur) : packes dans cet ordre pour que l'historique ne soit jamais rogne
        self.hist = tk.Listbox(f, height=2, bg=CARD_BG, fg=CARD_TXT, font=("TkDefaultFont", 9), activestyle="none", highlightthickness=1, highlightbackground=CARD_EDGE,
                               selectbackground=CARD_EDGE, selectforeground=CARD_TXT)      # fond sombre FIXE (comme cartes et courbes) : texte colore lisible sur tous les themes
        self.hist._fixed_theme_colors = (CARD_BG, CARD_TXT)
        self.hist.pack(side="bottom", fill="x", padx=6, pady=(0, 4))
        self.hist_lbl = self._label(f, "hist", font=("TkDefaultFont", 8), anchor="w")
        self.hist_lbl.pack(side="bottom", fill="x", padx=6, pady=(2, 0))
        charts = tk.Frame(f, bg=self.bg)
        charts.pack(fill="both", expand=True, padx=6, pady=(5, 0))
        self.cv_rssi, self.cv_lat = (tk.Canvas(charts, height=70, bg="#16213E", highlightthickness=0) for _i in range(2))
        for cv in (self.cv_rssi, self.cv_lat):
            cv.pack(fill="both", expand=True, pady=(0, 3))
            cv.bind("<Configure>", lambda e: self._charts_soon())

    def _kv(self, parent, row, col, key):
        """Une ligne « libelle : valeur » (libelle gris a gauche, valeur en gras a droite) ; la valeur reste dans self.cells[key]."""
        kl = tk.Label(parent, text=self.t(key), bg=self.bg, fg="#666666", font=("TkDefaultFont", 8), anchor="w")
        kl.grid(row=row, column=col, sticky="nw", pady=0)
        self._i18n.append((kl, key))
        self._keylbls.append(kl)
        v = tk.Label(parent, text="-", bg=self.bg, fg=self.fg, font=("TkDefaultFont", 9, "bold"), anchor="w")
        v.grid(row=row, column=col + 1, sticky="nw", padx=(8, 0), pady=0)
        self.cells[key] = v
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
        ttk.Combobox(self.baud_fr, textvariable=self.baud_var, values=["9600", "74880", "115200", "230400", "460800", "921600"], width=7).pack(side="left", padx=3)
        self.btn_conn = self._button(self.r2, "connect", self.toggle_serial, action=True, padx=5)      # v2 : boutons un peu plus serres (le journal est plus etroit, 9 / 20)
        self.btn_conn.pack(side="left", padx=3)
        self.btn_reset = self._button(self.r2, "reset", self.reset_board, padx=5)
        self.btn_reset.pack(side="left")
        self._button(self.r2, "clear", self.clear_log, padx=5).pack(side="left", padx=3)
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
        self._logbox = box
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
        for w in [c for k, c in self.cells.items() if k not in CARD_KEYS] + [self.status, self.ser_status]:
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
                if ch in self._fixed:
                    continue                       # carte de mesure : fond sombre fixe
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
            if k not in CARD_KEYS and str(v.cget("fg")).lower() not in (GOOD.lower(), WARN.lower(), BAD.lower()):
                v.config(fg=self.fg)
        if self._last_d is not None:        # le theme remet les cartes a la couleur de base : on redessine les mesures (couleurs de seuil)
            try:
                self._draw(self._last_d)
            except Exception:   # noqa: BLE001
                pass

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
        if k in CARD_KEYS:      # carte a fond sombre fixe : palette claire fixe, quel que soit le theme
            color = {GOOD.lower(): _PAL_DARK[0], WARN.lower(): _PAL_DARK[1], BAD.lower(): _PAL_DARK[2]}.get(str(color).lower(), CARD_TXT)
            self.cells[k].config(text=text, fg=color)
            return
        self.cells[k].config(text=text, fg=color or self.fg)

    def _cell_clip(self, k, text, max_px, color=None):
        """Valeur coupee EN PIXELS avec « … » : la hauteur d'une ligne ne depend jamais du texte (un texte sur plusieurs lignes poussait les courbes). Le texte complet est dans l'historique."""
        import tkinter.font as tkfont
        lbl = self.cells[k]
        try:
            font = tkfont.Font(root=lbl, font=lbl.cget("font"))
            if font.measure(text) > max_px:
                while text and font.measure(text + "…") > max_px:
                    text = text[:-1]
                text += "…"
        except tk.TclError:
            pass
        self._cell(k, text, color)

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
            for k in ("k_loss", "k_lat", "k_jit", "_lat_sub", "_loss_sub"):
                self._cell(k, "-" if k in ("k_loss", "k_lat", "k_jit") else "")
        else:
            loss = d.get("loss", 0)
            self._cell("k_loss", f"{loss} %", GOOD if loss == 0 else WARN if loss <= 10 else BAD)
            self._cell("_loss_sub", self.t("c_loss_sub", got=d.get("got"), n=d.get("n")), GOOD if loss == 0 else WARN if loss <= 10 else BAD)
            if not d.get("got"):
                self._cell("k_lat", "-", BAD)
                self._cell("_lat_sub", self.t("none"), BAD)
                self._cell("k_jit", "-")
            else:
                a = d.get("avg", 0)
                col = GOOD if a < 80 else WARN if a < 200 else BAD
                self._cell("k_lat", f"{a} ms", col)
                self._cell("_lat_sub", self.t("c_lat_sub", mn=d.get("min"), mx=d.get("max")), col)
                j = d.get("jit", 0)
                self._cell("k_jit", f"{j} ms", GOOD if j < 10 else WARN if j <= 40 else BAD)
        age = d.get("age", -1)
        self._cell("k_age", "-" if age < 0 else f"{age / 1000:.1f} s", None if age < 0 else GOOD if age < 2000 else WARN if age < 15000 else BAD)
        self._cell("k_gaps", d.get("gaps", 0), GOOD if d.get("gaps", 0) == 0 else WARN)
        disc = d.get("disc", 0)
        self._cell("k_disc", disc, GOOD if disc == 0 else WARN if disc < 3 else BAD)
        ups = d.get("up_s", 0)
        self._cell_clip("k_last", (f"{self.t('ago')} {dg.fmt_ago(ups - d.get('da', 0))} : {dg.reason_text(d.get('dr'), self.lang)}" if disc else self.t("never")), 125)
        self._cell("k_heap", f"{round(d.get('heap', 0) / 1024)} Ko ({round(d.get('minheap', 0) / 1024)} Ko)")
        self._cell("k_up", f"{ups // 3600} h {ups % 3600 // 60} min" if ups >= 3600 else f"{ups // 60} min")
        if self.wifi and self.wifi.samples:
            s = self.wifi.samples[-1]
            self._cell("k_ping", "-" if s["ping_ms"] is None else f"{s['ping_ms']:.0f} ms", BAD if s["ping_ms"] is None else None)
            self._cell("k_http", "-" if s["http_ms"] is None else f"{s['http_ms']:.0f} ms")
        self._charts_soon()
        self.hist.delete(0, "end")
        for i, e in enumerate(d.get("ev", [])):
            ago = f"{self.t('ago')} {dg.fmt_ago(ups - e[1])}"
            self.hist.insert("end", f"{ago} : " + (self.t("ev_d") + dg.reason_text(e[2], self.lang) if e[0] == "D" else f"{self.t('ev_c')} ({e[2]})"))
            self.hist.itemconfig(i, fg=_PAL_DARK[2] if e[0] == "D" else _PAL_DARK[0])        # deconnexion en rouge, reconnexion en vert (palette claire : fond sombre fixe)
        if self.wifi and self.wifi.load:
            self.hw_lbl.config(text=self.t("hw", hw=dg.device_kind(self.wifi.load, d)))

    # ------------------------------------------------------------------ courbes
    _BAND_GOOD, _BAND_WARN, _BAND_BAD = "#12301E", "#33301A", "#33191C"      # zones de qualite derriere les courbes (fond sombre fixe)

    def _charts_soon(self):
        """Redessine les courbes (donnees de la derniere mesure) 40 ms apres le dernier evenement : changement de taille ou nouvelle mesure."""
        if getattr(self, "_chart_job", None):
            try:
                self.parent.after_cancel(self._chart_job)
            except tk.TclError:
                pass
        self._chart_job = self.parent.after(40, self._draw_charts)

    def _draw_charts(self):
        self._chart_job = None
        d = self._last_d or {}
        rs = [None if v <= -127 else v for v in d.get("rs", [])]
        lat = [None if v < 0 else v for v in d.get("lat", [])]
        self._chart(self.cv_rssi, [(rs, "#8AB4F8")], -95, -30, self.t("rssi"), levels=(-90, -80, -70, -60, -50, -40),
                    bands=((-67, -30, self._BAND_GOOD), (-75, -67, self._BAND_WARN), (-95, -75, self._BAND_BAD)), nlast=len(rs))
        hi = max(100, int((max([v for v in lat if v is not None] or [0])) * 1.15))
        self._chart(self.cv_lat, [(lat, "#8AB4F8")], 0, hi, self.t("latc"), bands=((0, 80, self._BAND_GOOD), (80, 200, self._BAND_WARN), (200, max(hi, 201), self._BAND_BAD)),
                    lost=True, nlast=len(lat))

    def _chart(self, cv, series, lo, hi, title, levels=None, bands=None, lost=False, nlast=0, stats_series=0):
        """Courbe lisible : titre + valeurs (dernier / min / moy / max), axe gradue a gauche, zones de qualite, points, valeur courante et echelle du temps.
        series : [(valeurs, couleur)] ; None = mesure perdue (trait interrompu, repere rouge si lost)."""
        cv.delete("all")
        W, H = max(cv.winfo_width(), 200), max(cv.winfo_height(), 40)
        L, R, T0, B = 36, 12, 15, 12
        pw, ph = W - L - R, H - T0 - B
        if ph < 18 or not series or not any(v is not None for vals, _c in series for v in vals):
            cv.create_text(6, 2, anchor="nw", fill="#9CA3AF", font=("TkDefaultFont", 8, "bold"), text=title)
            return
        hi = hi if hi > lo else lo + 1

        def Y(v):
            return T0 + ph - (min(max(v, lo), hi) - lo) / (hi - lo) * ph
        for a, b, col in (bands or ()):
            cv.create_rectangle(L, Y(b), L + pw, Y(a), fill=col, outline="")
        n_ticks = max(2, min(5, int(ph / 20)))          # une graduation tous les ~20 px au plus : jamais d'etiquettes qui se chevauchent
        if levels and len(levels) > n_ticks:
            levels = list(levels)[::-(-len(levels) // n_ticks)]
        for v in (levels or [lo + (hi - lo) * i / (n_ticks - 1) for i in range(n_ticks)]):
            y = Y(v)
            cv.create_line(L, y, L + pw, y, fill="#2A3558")
            cv.create_text(L - 4, y, anchor="e", fill="#9CA3AF", font=("TkDefaultFont", 7), text=f"{v:.0f}")
        n = max(len(vals) for vals, _c in series)
        dx = pw / max(1, n - 1)
        for vals, col in series:
            pts = []
            for i, v in enumerate(vals):
                if v is None:
                    if len(pts) >= 4:
                        cv.create_line(*pts, fill=col, width=2)
                    pts = []
                    if lost:
                        cv.create_rectangle(L + i * dx - 2, T0 + ph - 8, L + i * dx + 2, T0 + ph, fill="#F87171", outline="")
                    continue
                x, y = L + i * dx, Y(v)
                pts += [x, y]
            if len(pts) >= 4:
                cv.create_line(*pts, fill=col, width=2)
            for k in range(0, len(pts), 2):
                cv.create_oval(pts[k] - 2, pts[k + 1] - 2, pts[k] + 2, pts[k + 1] + 2, fill=col, outline="")
            if pts and vals and vals[-1] is not None:        # valeur courante : pastille + etiquette au bout de la courbe
                cv.create_oval(pts[-2] - 4, pts[-1] - 4, pts[-2] + 4, pts[-1] + 4, outline="#FFFFFF", width=2)
        ref = [v for v in series[min(stats_series, len(series) - 1)][0] if v is not None]
        cv.create_text(6, 2, anchor="nw", fill="#E5E7EB", font=("TkDefaultFont", 8, "bold"), text=title)
        if ref:
            cv.create_text(W - R, 2, anchor="ne", fill="#E5E7EB", font=("TkDefaultFont", 8),
                           text=self.t("st_stats", last=f"{ref[-1]:.0f}", mn=f"{min(ref):.0f}", avg=f"{sum(ref) / len(ref):.0f}", mx=f"{max(ref):.0f}"))
        if nlast:
            cv.create_text(L, H - 1, anchor="sw", fill="#6B7280", font=("TkDefaultFont", 7), text=self.t("nlast", n=nlast))
            cv.create_text(L + pw, H - 1, anchor="se", fill="#6B7280", font=("TkDefaultFont", 7), text=self.t("now"))
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

    # ------------------------------------------------------------------ envoi vers Discord (v3)
    def _build_discord(self, bar):
        row = tk.Frame(bar, bg=self.bg)
        row.pack(fill="x", pady=(3, 0))
        self._label(row, "dc_title", font=("TkDefaultFont", 9, "bold")).pack(side="left")
        self._label(row, "dc_code").pack(side="left", padx=(8, 2))
        self.dc_code = tk.StringVar()
        try:
            import RecalBoxDMD_prefs as prefs
            self.dc_code.set(str(prefs.get("discord_code") or ""))
        except Exception:   # noqa: BLE001
            pass
        ttk.Entry(row, textvariable=self.dc_code, width=12).pack(side="left")
        # Detection des versions = tests ALPHA seulement : en STABLE il ne reste que la demande d'aide (pas de choix a faire)
        try:
            import RecalBoxDMD_tool as _tool
            stable = getattr(_tool, "BUILD_CHANNEL", "alpha") == "stable"
        except Exception:   # noqa: BLE001
            stable = False
        # Un bouton par action (pas de choix + « Envoyer ») : « Detection des versions » (alpha seulement) et « Demande d'aide »
        if not stable:
            self._button(row, "dc_send_test", lambda: self.dc_start("test"), action=True).pack(side="left", padx=(8, 0))
            self._button(row, "dc_send_tests", self.at_open, action=True).pack(side="left", padx=(6, 0))        # plan de tests alpha du Discord
        self._button(row, "dc_send_help", lambda: self.dc_start("support"), action=True).pack(side="left", padx=8)
        self.dc_status = tk.Label(row, text=self.t("dc_hint"), bg=self.bg, fg=self.fg, font=("TkDefaultFont", 8), anchor="w", justify="left")
        self.dc_status.pack(side="left", fill="x", expand=True)

    def _dc_msg(self, text, color=None):
        try:
            self.dc_status.config(text=text, fg=color or self.fg)
        except tk.TclError:
            pass
        cb = getattr(self, "_at_status_cb", None)       # fenetre « Tests alpha » ouverte : le resultat de l'envoi s'y affiche aussi
        if cb:
            try:
                cb(text, color or self.fg)
            except tk.TclError:
                self._at_status_cb = None

    def _dc_detect(self):
        """Detection des versions reelles (appel bloquant, a lancer dans un fil) -> (versions, notes). Sert a la fenetre « Tests alpha »."""
        import platform
        import RecalBoxDMD_tool as tool
        ip = self.ip_var.get().strip()
        if not ip:
            try:
                ip = (tool.find_dmd(None) or {}).get("ip", "")
            except Exception:   # noqa: BLE001
                ip = ""
            if ip:
                self._post(self.ip_var.set, ip)
        return dcd.detect(ip, self._dc_build_number(), getattr(tool, "BUILD_CHANNEL", ""), platform.platform(terse=True), tool._dmd_probe, tool.read_recalbox_version)

    def _dc_build_number(self) -> int:
        try:
            import sys
            return int(sys.modules["RecalBoxDMD_GUI"].TOOLKIT_RELEASE_VERSION)
        except Exception:   # noqa: BLE001
            return 0

    def dc_start(self, kind="support", extra=""):
        """Clic sur « Detection des versions… » (kind=test) ou « Demande d'aide… » (kind=support) : detection (et, pour une demande d'aide, releves du rapport) dans un fil, puis fenetre d'apercu."""
        self._dc_msg(self.t("dc_detecting"))
        ip0 = self.ip_var.get().strip()

        def work():
            try:
                import platform
                import RecalBoxDMD_tool as tool
                ip = ip0
                if not ip:
                    try:
                        ip = (tool.find_dmd(None) or {}).get("ip", "")
                    except Exception:   # noqa: BLE001
                        ip = ""
                    if ip:
                        self._post(self.ip_var.set, ip)
                if kind == "support" and ip:                      # releves du rapport (config du DMD, pings) : comme « Copier »
                    if self.wifi is None or self.wifi.ip != ip:
                        self.wifi = dg.WifiDiag(ip)
                    try:
                        self.wifi.collect_extra()
                    except Exception:   # noqa: BLE001
                        pass
                versions, notes = dcd.detect(ip, self._dc_build_number(), getattr(tool, "BUILD_CHANNEL", ""),
                                             platform.platform(terse=True), tool._dmd_probe, tool.read_recalbox_version)
            except Exception as e:   # noqa: BLE001
                self._post(self._dc_fail, dcd.DiscordError("other", f"{type(e).__name__}: {e}"))
                return
            self._post(self._dc_window, kind, versions, notes, extra)
        threading.Thread(target=work, daemon=True).start()

    def _dc_fail(self, err):
        code = getattr(err, "code", "other")
        s = T.get(self.lang, T["fr"]).get("dc_err_" + code) or T["fr"]["dc_err_other"]
        self._dc_msg(s.format(c=getattr(err, "detail", "") or code, s=getattr(err, "detail", "")), BAD)

    def _dc_window(self, kind, versions, notes, extra=""):
        """Apercu de ce qui va partir (consentement) + zone de message pour une demande d'aide + boutons."""
        win = tk.Toplevel(self.gui.root)
        win.title(self.t("dc_win_title_help" if kind == "support" else "dc_win_title"))
        win.configure(bg=self.bg)
        win.transient(self.gui.root)
        tk.Label(win, text=self.t("dc_win_head_help" if kind == "support" else "dc_win_head"), bg=self.bg, fg=self.fg, anchor="w", justify="left", wraplength=560).pack(fill="x", padx=10, pady=(8, 2))
        code = self.dc_code.get()
        try:
            preview = dcd.build_message(kind, code if dcd.valid_code(code) else "XXXXXXXX", versions, extra)
        except Exception:   # noqa: BLE001
            preview = ""
        box = tk.Text(win, width=70, height=10, font=("Consolas", 9))
        box.insert("1.0", preview)
        box.config(state="disabled")
        box.pack(fill="x", padx=10)
        if notes:
            tk.Label(win, text=self.t("dc_win_notes", x=", ".join(self.t("dc_n_" + n) for n in notes)), bg=self.bg, fg=WARN,
                     anchor="w", justify="left", wraplength=560).pack(fill="x", padx=10, pady=(2, 0))
        files, usertxt = [], None
        if kind == "support":
            secrets = [str((getattr(self.wifi, "load", None) or {}).get("wifi_ssid") or "")]
            try:
                args = self._report_args()
                data = dcd.build_support_zip(self._report(), args[4], secrets)
            except Exception as e:   # noqa: BLE001
                win.destroy()
                self._dc_fail(dcd.DiscordError("other", f"{type(e).__name__}: {e}"))
                return
            files = [("diagnostic.zip", data)]
            tk.Label(win, text=self.t("dc_win_att", n=max(1, len(data) // 1024)), bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", padx=10, pady=(6, 0))
            tk.Label(win, text=self.t("dc_win_support"), bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", padx=10, pady=(6, 0))
            usertxt = tk.Text(win, width=70, height=5, font=("TkDefaultFont", 9))
            usertxt.pack(fill="x", padx=10)
        btns = tk.Frame(win, bg=self.bg)
        btns.pack(fill="x", padx=10, pady=8)

        def go():
            text = usertxt.get("1.0", "end").strip() if usertxt is not None else extra
            win.destroy()
            self._dc_send(kind, versions, files, text)
        tk.Button(btns, text=self.t("dc_win_go"), command=go, bg=self.bg_action, fg="#000000", bd=2, relief="solid", padx=10).pack(side="left")
        tk.Button(btns, text=self.t("dc_win_cancel"), command=win.destroy, bd=2, relief="solid", padx=10).pack(side="right")
        self._dc_msg(self.t("dc_hint"))

    def _dc_send(self, kind, versions, files, user_text):
        code = dcd.normalize_code(self.dc_code.get())
        if not dcd.valid_code(code):
            return self._dc_fail(dcd.DiscordError("code"))
        try:
            import RecalBoxDMD_prefs as prefs
            wait = dcd.too_soon(prefs.get("discord_last_send"))
        except Exception:   # noqa: BLE001
            prefs, wait = None, 0
        if wait:
            return self._dc_fail(dcd.DiscordError("wait", str(wait)))
        self._dc_msg(self.t("dc_sending"))
        build = self._dc_build_number()

        def work():
            try:
                intake = dcd.load_intake()
                url = dcd.webhook_for(kind, intake, build)
                dcd.post(url, dcd.build_message(kind, code, versions, user_text), files)
            except dcd.DiscordError as e:
                self._post(self._dc_fail, e)
                return
            except Exception as e:   # noqa: BLE001
                self._post(self._dc_fail, dcd.DiscordError("other", f"{type(e).__name__}: {e}"))
                return
            try:
                if prefs:
                    prefs.set("discord_code", code)
                    prefs.set("discord_last_send", str(time.time()))
            except Exception:   # noqa: BLE001
                pass
            self._post(self._dc_msg, self.t("dc_sent"), GOOD)
        threading.Thread(target=work, daemon=True).start()
    # ------------------------------------------------------------------ fenetre « Tests alpha » (build alpha)
    _AT_T = {
        "fr": {"title": "Tests alpha", "loading": "Chargement du plan de tests…", "err": "Plan de tests injoignable : {e}", "list": "Tests", "res": "Résultat :", "dev": "Appareil :",
               "ok": "✅ OK", "ko": "❌ KO", "partial": "🟠 Partiel", "na": "➖ Non applicable", "comment": "Commentaire (obligatoire si KO ou partiel) :", "send": "Envoyer le résultat…",
               "sort": "Trier par :", "s_area": "Zone", "s_id": "N°", "s_title": "Titre", "sent_hint": "Confirmez l'envoi dans la fenêtre d'aperçu, puis passez au test suivant.", "detecting": "détection des versions…", "preview": "Ce qui sera envoyé (rien ne part avant votre clic sur « Envoyer ») :", "nover": "Versions pas encore détectées…", "pick": "(choisissez un test et un résultat)", "sending": "Envoi en cours…", "errd": "Détection impossible : {e}", "need": "Choisissez un test et un résultat.", "needc": "Décrivez le problème dans le commentaire (KO ou partiel).", "hint": "Les versions détectées par le toolkit sont jointes : ce test compte pour « Prêt stable »."},
        "en": {"title": "Alpha tests", "loading": "Loading the test plan…", "err": "Test plan unreachable: {e}", "list": "Tests", "res": "Result:", "dev": "Device:",
               "ok": "✅ OK", "ko": "❌ KO", "partial": "🟠 Partial", "na": "➖ Not applicable", "comment": "Comment (required for KO or partial):", "send": "Send the result…",
               "sort": "Sort by:", "s_area": "Area", "s_id": "No.", "s_title": "Title", "sent_hint": "Confirm the sending in the preview window, then go on with the next test.", "detecting": "detecting versions…", "preview": "What will be sent (nothing leaves before you click \"Send\"):", "nover": "Versions not detected yet…", "pick": "(pick a test and a result)", "sending": "Sending…", "errd": "Detection failed: {e}", "need": "Pick a test and a result.", "needc": "Describe the problem in the comment (KO or partial).", "hint": "The versions detected by the toolkit are attached: this test counts toward \"Ready for stable\"."},
        "es": {"title": "Pruebas alfa", "loading": "Cargando el plan de pruebas…", "err": "Plan de pruebas inaccesible: {e}", "list": "Pruebas", "res": "Resultado:", "dev": "Dispositivo:",
               "ok": "✅ OK", "ko": "❌ KO", "partial": "🟠 Parcial", "na": "➖ No aplicable", "comment": "Comentario (obligatorio si KO o parcial):", "send": "Enviar el resultado…",
               "sort": "Ordenar por:", "s_area": "Zona", "s_id": "N.º", "s_title": "Título", "sent_hint": "Confirme el envío en la ventana de vista previa y continúe con la siguiente prueba.", "detecting": "detectando versiones…", "preview": "Lo que se enviará (no sale nada antes de pulsar «Enviar»):", "nover": "Versiones aún sin detectar…", "pick": "(elija una prueba y un resultado)", "sending": "Enviando…", "errd": "Detección imposible: {e}", "need": "Elija una prueba y un resultado.", "needc": "Describa el problema en el comentario (KO o parcial).", "hint": "Se adjuntan las versiones detectadas por el toolkit: esta prueba cuenta para «Listo para estable»."},
    }

    def at_open(self):
        """Plan de tests alpha a remplir depuis le toolkit. Les versions reelles sont detectees a l'ouverture et l'APERCU de ce qui sera envoye est dans la fenetre
        (le bouton Envoyer vaut consentement : pas de seconde fenetre) ; la fenetre reste ouverte pour enchainer les tests."""
        import RecalBoxDMD_alpha_tests as att
        tr = self._AT_T.get(self.lang, self._AT_T["en"])
        win = tk.Toplevel(self.gui.root)
        win.title(tr["title"])
        win.configure(bg=self.bg)
        win.transient(self.gui.root)
        win.geometry("1240x800")
        win.minsize(900, 600)
        top = tk.Frame(win, bg=self.bg)
        top.pack(fill="x", padx=10, pady=(8, 2))
        status = tk.Label(top, text=tr["loading"], bg=self.bg, fg=self.fg, anchor="w")
        status.pack(side="left", fill="x", expand=True)
        tk.Label(top, text=self.t("dc_code"), bg=self.bg, fg=self.fg).pack(side="left", padx=(8, 2))
        ttk.Entry(top, textvariable=self.dc_code, width=12).pack(side="left")
        sort_var = tk.StringVar(value=tr["s_area"])
        tk.Label(top, text=tr["sort"], bg=self.bg, fg=self.fg).pack(side="left", padx=(12, 4))
        sort_cb = ttk.Combobox(top, textvariable=sort_var, values=[tr["s_area"], tr["s_id"], tr["s_title"]], state="readonly", width=14)
        sort_cb.pack(side="left")
        body = tk.PanedWindow(win, orient="horizontal", bg=self.bg, sashwidth=6)
        body.pack(fill="both", expand=True, padx=10)
        left = tk.Frame(body, bg=self.bg)
        tree = ttk.Treeview(left, show="tree", selectmode="browse")
        ysb = ttk.Scrollbar(left, orient="vertical", command=tree.yview)
        xsb = ttk.Scrollbar(left, orient="horizontal", command=tree.xview)
        tree.configure(yscrollcommand=ysb.set, xscrollcommand=xsb.set)
        xsb.pack(side="bottom", fill="x")
        ysb.pack(side="right", fill="y")
        tree.pack(side="left", fill="both", expand=True)
        right = tk.Frame(body, bg=self.bg)
        body.add(left, minsize=360, width=560)
        body.add(right, minsize=380)
        detail = tk.Text(right, height=11, wrap="word", font=("TkDefaultFont", 9))
        detail.pack(fill="both", expand=True)
        row = tk.Frame(right, bg=self.bg)
        row.pack(fill="x", pady=(6, 0))
        tk.Label(row, text=tr["res"], bg=self.bg, fg=self.fg).pack(side="left")
        res = tk.StringVar(value="__none__")      # une valeur vide est l'etat « indetermine » (-tristatevalue) de Tk : tous les boutons paraissaient coches
        for key in att.RESULTS:
            tk.Radiobutton(row, text=tr[key], variable=res, value=key, tristatevalue="__tri__", bg=self.bg, fg=self.fg, selectcolor=self.bg, activebackground=self.bg).pack(side="left", padx=4)
        tk.Label(right, text=tr["comment"], bg=self.bg, fg=self.fg, anchor="w").pack(fill="x", pady=(6, 0))
        comment = tk.Text(right, height=3, font=("TkDefaultFont", 9))
        comment.pack(fill="x")
        tk.Label(right, text=tr["preview"], bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 9, "bold")).pack(fill="x", pady=(8, 0))
        preview = tk.Text(right, height=9, wrap="word", font=("Consolas", 9), state="disabled")
        preview.pack(fill="x")
        notes_lbl = tk.Label(right, text="", bg=self.bg, fg=WARN, anchor="w", justify="left", wraplength=560)
        notes_lbl.pack(fill="x")
        tk.Label(right, text=tr["hint"], bg=self.bg, fg=self.fg, anchor="w", font=("TkDefaultFont", 8), wraplength=560, justify="left").pack(fill="x", pady=(2, 0))
        tests, by_iid = [], {}
        state = {"versions": None, "notes": [], "err": ""}
        modes = {tr["s_area"]: "area", tr["s_id"]: "id", tr["s_title"]: "title"}

        def cur_test():
            sel = tree.selection()
            return by_iid.get(sel[0]) if sel else None

        def refresh_preview(*_a):
            """Message exact qui partira : versions detectees + test + resultat + commentaire (IP, MAC et nom du Wi-Fi deja retires)."""
            t = cur_test()
            block = ""
            try:
                block = att.result_block(t["id"], res.get(), "", comment.get("1.0", "end").strip()) if (t and res.get() in att.RESULTS) else ""
                code = self.dc_code.get()
                txt = dcd.build_message("test", code if dcd.valid_code(code) else "XXXXXXXX", state["versions"] or {}, block) if state["versions"] else tr["nover"]
            except Exception:   # noqa: BLE001
                txt = tr["nover"]
            if not block:
                txt += "\n\n" + tr["pick"]
            preview.config(state="normal")
            preview.delete("1.0", "end")
            preview.insert("1.0", txt)
            preview.config(state="disabled")

        def populate():
            """Liste groupee par zone, ou plate (numero / titre) selon le tri choisi ; la largeur suit le titre le plus long."""
            tree.delete(*tree.get_children())
            by_iid.clear()
            mode = modes.get(sort_var.get(), "area")
            longest = 0
            if mode == "area":
                for label, lst in att.groups(tests, self.lang):
                    gid = tree.insert("", "end", text=f"{label} ({len(lst)})", open=True)
                    for t in lst:
                        title = att.title_of(t, self.lang)
                        longest = max(longest, len(title) + 4)
                        by_iid[tree.insert(gid, "end", text=title)] = t
            else:
                for t in att.sort_tests(tests, mode, self.lang):
                    title = att.title_of(t, self.lang)
                    longest = max(longest, len(title))
                    by_iid[tree.insert("", "end", text=title)] = t
            tree.column("#0", width=max(340, min(longest * 7 + 30, 900)), stretch=True)

        def reset_form():
            res.set("__none__")
            comment.delete("1.0", "end")
            refresh_preview()

        def show(_e=None):
            t = cur_test()
            if t is None:
                return
            detail.config(state="normal")
            detail.delete("1.0", "end")
            detail.insert("1.0", att.test_text(t, self.lang))
            detail.config(state="disabled")
            reset_form()      # un autre test : formulaire vierge (pas de resultat envoye par erreur pour le mauvais test)

        def send():
            t = cur_test()
            if t is None or res.get() not in att.RESULTS:
                status.config(text=tr["need"], fg=BAD)
                return
            txt = comment.get("1.0", "end").strip()
            if res.get() in ("ko", "partial") and not txt:
                status.config(text=tr["needc"], fg=BAD)
                return
            if not state["versions"]:
                status.config(text=state["err"] or tr["nover"], fg=BAD)
                return
            self._dc_send("test", state["versions"], [], att.result_block(t["id"], res.get(), "", txt))      # direct : l'apercu est deja dans la fenetre
            reset_form()
            status.config(text=tr["sending"], fg=self.fg)
        tk.Button(win, text=tr["send"], command=send, bg=self.bg_action, fg="#000000", bd=2, relief="solid", padx=10).pack(side="right", padx=10, pady=8)
        tree.bind("<<TreeviewSelect>>", show)
        sort_cb.bind("<<ComboboxSelected>>", lambda _e: populate())
        comment.bind("<KeyRelease>", refresh_preview)
        res.trace_add("write", refresh_preview)
        self.dc_code.trace_add("write", refresh_preview)
        self._at_status_cb = lambda text, color: status.config(text=text, fg=color) if win.winfo_exists() else None
        win.bind("<Destroy>", lambda e: setattr(self, "_at_status_cb", None) if e.widget is win else None)
        refresh_preview()

        def work():
            try:
                data = att.load_tests()
            except att.AlphaTestsError as e:
                self._post(lambda: status.config(text=tr["err"].format(e=e), fg=BAD) if win.winfo_exists() else None)
                data = None

            def fill():
                if data is None or not win.winfo_exists():
                    return
                tests.extend(data["tests"])
                populate()
                status.config(text=f"{tr['list']} : {len(tests)} — {tr['detecting']}", fg=self.fg)
            self._post(fill)
            try:
                versions, notes = self._dc_detect()
            except Exception as e:   # noqa: BLE001
                state["err"] = f"{type(e).__name__}: {e}"
                self._post(lambda: (status.config(text=tr["errd"].format(e=state["err"]), fg=BAD), refresh_preview()) if win.winfo_exists() else None)
                return
            state["versions"], state["notes"] = versions, notes

            def done():
                if not win.winfo_exists():
                    return
                refresh_preview()
                notes_lbl.config(text=self.t("dc_win_notes", x=", ".join(self.t("dc_n_" + n) for n in notes)) if notes else "")
                status.config(text=f"{tr['list']} : {len(tests)}", fg=self.fg)
            self._post(done)
        threading.Thread(target=work, daemon=True).start()
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
