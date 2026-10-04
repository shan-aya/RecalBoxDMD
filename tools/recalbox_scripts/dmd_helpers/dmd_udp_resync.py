#!/usr/bin/env python3
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v10
#
# v10 - 2026-10-04 - safe-modify - BUG REEL corrige (retour utilisateur : « quand je fais Reprendre DMD, le DMD passe sur un ecran vide ; il faut renvoyer une commande de navigation pour qu'il
#   affiche le logo » au lieu d'afficher l'etat de la RB a la reprise) : la resync (reponse au hello du DMD) ne connaissait que les JEUX (rungame / rundemo / position sur un jeu) ; quand ES
#   affichait un SYSTEME (Action=systembrowsing ou gamelistbrowsing sans jeu, ex. SystemId=lastplayed, favorites, snes...), elle repondait « default » -> le DMD reprenait une playlist vide
#   (ecran noir). Desormais : systeme affiche sans jeu -> « system <SystemId> » (meme commande que la navigation, marquee.sh), le DMD affiche le logo du systeme courant.
#
# v9 - 2026-10-04 - safe-modify - BUG REEL corrige (retour utilisateur : « theme change dans ES, le DMD garde les logos de l'ancien theme ») : v8 envoyait « themeopt » puis
#   « theme » dans la MEME milliseconde. Le DMD ne retient QU'UN paquet par passage de sa boucle de reception quand plusieurs arrivent ensemble (regle v7/v163 du firmware : le
#   premier tant que le retard est petit) : « theme » etait jete, seul « themeopt » (identique, donc sans effet) passait. Confirme au moniteur serie : 2 changements de theme, 2 paquets
#   themeopt recus, AUCUN paquet theme. Correctif : 0,5 s d'ecart entre les deux datagrammes (le firmware 2.39 traite en plus theme / themeopt directement a la reception).
#
# v8 - 2026-10-03 - safe-modify - LANGUE ET REGION A LA VOLEE (firmware v238 : commande UDP "CMD=themeopt ARG=<langue>,<region>", ex. "fr,eu"). Lit
#   system.language (2 premieres lettres, defaut "en") et emulationstation.theme.region (us / eu / jp, defaut "us" -- ES ne deduit PAS la region de la langue :
#   ThemeData.cpp, RecalboxConf.h). Envoye AVANT "theme", au HELLO du DMD et sur les PING des que la langue ou la region change : le DMD affiche alors les logos
#   de theme (sous-dossiers l_<langue>/ et r_<region>/) ET les images par defaut (_defaults/<langue>/) dans la langue / la region de la Recalbox, sans reinstallation.
#   Un firmware plus ancien ignore la commande (comportement inchange : base US / anglais).
#
# v7 - 2026-10-03 - safe-modify - THEME PAR DEFAUT : quand emulationstation.theme.folder est ABSENT de recalbox.conf (l'utilisateur n'a jamais
#   change de theme), ES utilise "recalbox-next" (RecalboxConf.h : DefineGetterSetter(ThemeFolder, ..., "recalbox-next")). Jusqu'en v6 on
#   annoncait alors un theme vide (= logos par defaut du DMD) : la majorite des utilisateurs, restes sur le theme par defaut, ne beneficiaient
#   jamais des logos de theme. On annonce maintenant "recalbox-next" ; si la carte SD n'a pas ce dossier, le firmware l'ignore et garde les
#   logos par defaut (aucun risque). Une cle presente mais VIDE reste "pas de theme".
#
# v6 - 2026-10-03 - safe-modify - THEME RECALBOX pour les logos systeme du DMD (firmware v233, commande UDP "CMD=theme ARG=<dossier>").
#   Lit emulationstation.theme.folder (et dmd.logo s'il existe : different de "theme" => theme desactive) dans
#   /recalbox/share/system/recalbox.conf. Nom assaini comme le firmware ([A-Za-z0-9._-], 31 car. max : le dossier des logos
#   sur la SD doit porter ce nom tronque). Envoye : a chaque HELLO du DMD (boot/reconnexion, AVANT la resync, pour que
#   le logo qui suit utilise deja le theme) et, sur les PING (15 s), des que la valeur change (changement de theme dans ES
#   pris en compte en <= 15 s). UDP sans accuse : un theme perdu est renvoye au prochain hello/changement.
#
# v5 - 2026-09-19 - safe-modify - BUG REEL corrige (retour utilisateur
#   externe, testeur tiers : "le DMD reste en playlist" alors que les
#   scripts sont bien installes/actives) : marquee.sh/dmd_score.sh/
#   dmd_achievement.sh visaient tous les 3 DMD_UDP_IP="192.168.0.51",
#   l'IP FIXE du DMD de developpement -- ne fonctionnait que par coincidence
#   si le DMD de l'installateur obtenait cette meme adresse. Ce script
#   recoit deja TOUS les paquets du DMD (hello au boot/reconnexion WiFi,
#   PING toutes les 15s, FEATURES a chaque sauvegarde web) et connait donc
#   deja son IP source reelle (addr[0]) a chaque appel -- ecrit desormais
#   cette IP dans DMD_IP_CACHE_PATH a CHAQUE paquet recu (avant tout
#   traitement specifique), lu par les 3 scripts shell au lieu de leur
#   ancienne IP codee en dur (repli conserve si ce fichier n'existe pas
#   encore, ex. tout debut avant le 1er hello). Auto-decouverte, se
#   corrige aussi tout seul si l'IP du DMD change (nouveau bail DHCP) des
#   le prochain PING (<=15s).
#
# v4 - 2026-09-12 - safe-modify - BUG REEL trouve en direct (retour
#   utilisateur : "j'ai un sacre melange... playlist melange avec hiscore
#   du jeu en demo") : compute_current_state() ne reconnaissait QUE
#   Action=rungame/gamelistbrowsing/systembrowsing -- AUCUN cas pour
#   "rundemo" (reactive cette session, voir marquee.sh v50/dmd_score.sh
#   v50), donc tout hello de resync recu PENDANT la veille demo (ex. juste
#   apres un reboot DMD) retombait sur le cas par defaut ("default",
#   playlist) au lieu de restaurer le jeu demo REELLEMENT affiche --
#   pendant que le round-robin ingame/hiscore (independant de l'etat du
#   DMD) continuait d'envoyer ses overlays HI-SCORE par-dessus, d'ou le
#   melange observe. Ce trou existait avant la reactivation de rundemo,
#   simplement jamais expose (ce cas ne pouvait jamais se produire tant
#   que rundemo etait un no-op). Fix : "rundemo" rejoint "rungame" dans le
#   test -- memes champs es_state.inf (GamePath/SystemId), meme
#   comportement de restauration. Se corrige aussi tout seul au prochain
#   vrai changement de jeu demo (~98s), mais laisse un melange visible
#   inutilement jusque-la sans ce fix.
#
# v3 - 2026-09-10 - safe-modify - Ping/pong DEDIE a l'alerte firmware
#   "RecalBox non connectee" (v183, RecalBox_DMD.ino) -- INDEPENDANT du
#   "HELLO" de resync ci-dessous (outil de diagnostic du gel de reception,
#   pas un mecanisme de fiabilite -- retour utilisateur explicite : ne pas
#   batir l'alerte de connectivite sur l'outil qui sert a mesurer/
#   contourner le bug qu'on cherche a resoudre). Reconnait desormais un
#   3e type de message sur ce port : "PING" (le firmware l'envoie toutes
#   les 15s, cadence alignee sur l'ancien keepalive MQTT) -> reponse
#   "PONG" immediate, brute, SANS relire es_state.inf (accuse de vie pur,
#   aussi leger/rapide que possible -- contrairement au hello qui
#   recalcule l'etat de navigation complet).
#
# v2 - 2026-09-08 - safe-modify - BUG REEL trouve en revue (pas en usage
#   reel -- retour utilisateur explicite : "qu'est-ce qu'on aurait pu
#   oublier de mettre a jour ?"). broadcastFeatureStatus() (RecalBox_DMD.ino,
#   les 8 reglages hi-score/info/description/RA de la page web du DMD) ne
#   publiait plus QUE via MQTT (marquee/status/features, lu par
#   dmd_score.sh ET dmd_achievement.sh via mosquitto_sub) -- mort depuis la
#   bascule full UDP (v161, MQTT_ENABLED=false) : tout changement de
#   reglage sur la page web du DMD n'atteignait plus jamais RB1,
#   silencieusement (les 2 scripts continuaient de lire un cache perime
#   dans FEATURES_CACHE_PATH). Fix cote firmware (RecalBox_DMD.ino v13,
#   sendUdpFeatureStatus()) : envoie desormais aussi un message
#   "FEATURES:<meme format qu'avant>" sur ce MEME port (UDP_HELLO_PORT) a
#   chaque sauvegarde web ET a chaque (re)connexion WiFi. Ce script
#   distingue maintenant les 2 types de message recus sur ce port (au lieu
#   de traiter systematiquement tout paquet comme un "hello") -- un
#   message FEATURES: est ecrit tel quel dans FEATURES_CACHE_PATH (meme
#   format/emplacement que l'ancien features_watcher() MQTT, aucun
#   changement cote dmd_score.sh/dmd_achievement.sh necessaire).
#
# v1 - 2026-09-08 - safe-modify - Creation. Piste UDP full (voir
#   TRANSPORT_PLAN_UDP.md, RecalBox_DMD.ino v161 MQTT_ENABLED=false,
#   marquee.sh v45/dmd_score.sh v48) : sans retain MQTT, un DMD qui
#   reboote/perd le WiFi en cours de session reste fige sur son dernier
#   etat jusqu'au prochain evenement REEL de navigation -- ce script comble
#   ce trou. Ecoute en permanence un "hello" UDP envoye par le DMD a chaque
#   connexion/reconnexion WiFi (voir sendUdpHello() cote firmware) ; a
#   chaque hello recu, relit /tmp/es_state.inf A NEUF (pas un etat mis en
#   cache) et renvoie l'etat REEL courant en UDP -- garantit que le DMD
#   rattrape la bonne position meme s'il a manque plusieurs changements
#   pendant sa coupure. Duplique volontairement (pas factorise) la logique
#   de decision Action=rungame/gamelistbrowsing deja presente en shell dans
#   marquee.sh (case start), meme motif que send_udp() duplique plutot que
#   factorise entre marquee.sh/dmd_score.sh : la logique shell est un
#   chemin eprouve depuis des mois, ne pas la restructurer pour ce
#   mecanisme pas encore valide en charge. PAS ENCORE TESTE SUR MATERIEL.
#
# ============================================
#
# Lance en arriere-plan par marquee.sh a son demarrage (processus persistant
# associe au singleton marquee -- meme cycle de vie que features_watcher(),
# voir marquee.sh v45). N'ecoute PAS les evenements ES : purement reactif au
# "hello" UDP du DMD, rien d'autre. Etat mis a jour ("resync") = meme
# format de payload que MQTT/marquee.sh ("CMD=<nom> ARG=<valeur>"), envoye
# directement au DMD par socket UDP brut (memes port/IP que send_udp() cote
# shell).

import os
import re
import socket
import sys
import time

UDP_HELLO_PORT = 5006  # doit rester identique a UDP_HELLO_PORT, RecalBox_DMD.ino
DMD_UDP_PORT = 5005  # doit rester identique a UDP_CMD_PORT, RecalBox_DMD.ino
ES_STATE_PATH = "/tmp/es_state.inf"
LOG_PATH = "/recalbox/share/system/logs/dmd_udp_resync.log"
# v2 -- meme chemin/format que FEATURES_FILE dans dmd_score.sh (voir
# features_watcher()/feat_enabled()) -- ecrit tel quel, lu par dmd_score.sh
# ET dmd_achievement.sh sans aucun changement de leur cote.
FEATURES_CACHE_PATH = "/tmp/dmd_features_cache"
FEATURES_PREFIX = "FEATURES:"
# v5 -- IP source reelle du DMD, apprise de chaque paquet recu sur ce port
# (hello/PING/FEATURES, peu importe le type) -- lue par marquee.sh/
# dmd_score.sh/dmd_achievement.sh au lieu de leur ancienne IP codee en dur.
DMD_IP_CACHE_PATH = "/tmp/dmd_udp_ip"
# v6 -- theme Recalbox (voir changelog v6).
RECALBOX_CONF_PATH = "/recalbox/share/system/recalbox.conf"
_last_theme_sent = {}  # ip du DMD -> dernier (options, theme) envoye (changement detecte sur les PING)


def log(msg):
    ts = time.strftime("%H:%M:%S")
    line = f"{ts} {msg}"
    try:
        with open(LOG_PATH, "a") as f:
            f.write(line + "\n")
    except Exception:
        pass


def extract_field(content, key):
    # meme motif que extract_field() en shell (marquee.sh/dmd_score.sh) :
    # une ligne "KEY=valeur", valeur prend tout le reste de la ligne.
    m = re.search(rf"^{re.escape(key)}=(.*)$", content, re.MULTILINE)
    return m.group(1).strip() if m else ""


def normalize_rom(game_path):
    # meme motif que basename+sed dans marquee.sh (retire l'extension,
    # retire les espaces) -- pas de garantie de correspondance parfaite
    # avec normalize_system()/le detail des conventions ES, suffisant pour
    # ce mecanisme de secours (resync post-coupure, pas le chemin principal).
    base = os.path.basename(game_path)
    base = re.sub(r"\.[^.]*$", "", base)
    base = base.replace(" ", "")
    return base


def compute_current_state():
    """Relit es_state.inf A NEUF et retourne une liste de (cmd, arg) a
    envoyer -- meme decision que le case start) de marquee.sh (v17/v32/v33,
    voir son commentaire complet)."""
    try:
        with open(ES_STATE_PATH, "r", errors="replace") as f:
            content = f.read()
    except Exception as e:
        log(f"ERREUR lecture {ES_STATE_PATH}: {e}")
        return [("default", "1")]

    action = extract_field(content, "Action")
    game_path = extract_field(content, "GamePath")
    system_id = extract_field(content, "SystemId")

    if action in ("rungame", "rundemo") and game_path:
        rom = normalize_rom(game_path)
        if system_id and rom:
            return [("game", f"{system_id}/{rom}"), ("ingame", "1")]
        return [("default", "1")]

    if action in ("systembrowsing", "gamelistbrowsing") and game_path and not os.path.isdir(game_path):
        rom = normalize_rom(game_path)
        if system_id and rom:
            return [("game", f"{system_id}/{rom}")]
        return [("default", "1")]

    # v10 -- systeme affiche sans jeu (liste des systemes, ou liste de jeux dont le curseur est sur un dossier) : meme commande que la navigation en direct
    if action in ("systembrowsing", "gamelistbrowsing"):
        sysid = re.sub(r"[^A-Za-z0-9_.-]", "", system_id)
        if sysid:
            return [("system", sysid)]

    return [("default", "1")]


def update_dmd_ip_cache(ip):
    # v5 -- best-effort, appele a CHAQUE paquet recu -- voir DMD_IP_CACHE_PATH.
    try:
        with open(DMD_IP_CACHE_PATH, "w") as f:
            f.write(ip)
    except Exception as e:
        log(f"ERREUR ecriture {DMD_IP_CACHE_PATH}: {e}")


THEME_PACKET_GAP_S = 0.5   # v9 -- ecart entre CMD=themeopt et CMD=theme
DEFAULT_THEME_FOLDER = "recalbox-next"   # valeur par defaut de emulationstation.theme.folder dans Recalbox 9/10/11


def compute_theme():
    """Theme a annoncer au DMD (chaine vide = logos par defaut).
    Regle (choix utilisateur : automatique) : dmd.logo absent ou "theme" => on suit emulationstation.theme.folder ;
    dmd.logo = autre chose (recalbox/custom) => pas de theme. Lignes commentees (; ou #) ignorees."""
    wanted = ("emulationstation.theme.folder", "dmd.logo")
    values = {}
    try:
        with open(RECALBOX_CONF_PATH, "r", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line or line[0] in ";#" or "=" not in line:
                    continue
                key, val = line.split("=", 1)
                if key.strip() in wanted:
                    values[key.strip()] = val.strip()
    except Exception as e:
        log(f"ERREUR lecture {RECALBOX_CONF_PATH}: {e}")
        return ""
    logo_mode = values.get("dmd.logo", "").lower()
    if logo_mode and logo_mode != "theme":
        return ""
    # meme assainissement que le firmware (CMD=theme) : caracteres autorises puis 31 max
    # v7 : cle absente => theme par defaut d'ES ; cle presente mais vide => rien
    folder = re.sub(r"[^A-Za-z0-9._-]", "", values.get("emulationstation.theme.folder", DEFAULT_THEME_FOLDER))[:31]
    return "" if folder in ("", ".", "..") else folder


def compute_theme_options():
    """'<langue>,<region>' a annoncer au DMD (ex. 'fr,eu'), chaine vide si recalbox.conf est illisible.
    langue = 2 premieres lettres de system.language (defaut en) ; region = emulationstation.theme.region si us/eu/jp, sinon us."""
    values = {}
    try:
        with open(RECALBOX_CONF_PATH, "r", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line or line[0] in ";#" or "=" not in line:
                    continue
                key, val = line.split("=", 1)
                if key.strip() in ("system.language", "emulationstation.theme.region"):
                    values[key.strip()] = val.strip().lower()
    except Exception as e:
        log(f"ERREUR lecture {RECALBOX_CONF_PATH}: {e}")
        return ""
    m = re.match(r"^([a-z]{2})", values.get("system.language", "en_us"))
    lang = m.group(1) if m else "en"
    region = values.get("emulationstation.theme.region", "us")
    return f"{lang},{region if region in ('us', 'eu', 'jp') else 'us'}"


def send_theme(sock, dmd_ip, force=False):
    """Envoie CMD=themeopt (langue, region) puis CMD=theme si force ou si l'un des deux a change depuis le dernier envoi a ce DMD."""
    opts = compute_theme_options()
    theme = compute_theme()
    state = (opts, theme)
    changed = _last_theme_sent.get(dmd_ip) != state
    if force or changed:
        if opts:
            send_udp(sock, dmd_ip, "themeopt", opts)      # AVANT le theme : une seule selection cote DMD
            time.sleep(THEME_PACKET_GAP_S)                # v9 : jamais 2 paquets dans le meme passage de la boucle de reception du DMD (un seul est retenu)
        send_udp(sock, dmd_ip, "theme", theme)
        if changed:
            log(f"theme pour {dmd_ip}: {theme or '(defaut)'} ; langue/region: {opts or '(inconnues)'}")
        _last_theme_sent[dmd_ip] = state


def send_udp(sock, dmd_ip, cmd, arg):
    payload = f"CMD={cmd} ARG={arg}".encode("utf-8", "replace")
    sock.sendto(payload, (dmd_ip, DMD_UDP_PORT))


def main():
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listen_sock.bind(("0.0.0.0", UDP_HELLO_PORT))
    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    log(f"demarre, ecoute UDP {UDP_HELLO_PORT}, repond a l'IP source de chaque paquet recu")

    while True:
        try:
            data, addr = listen_sock.recvfrom(256)
        except Exception as e:
            log(f"ERREUR recvfrom: {e}")
            time.sleep(1)
            continue

        # v5 -- apprend l'IP reelle du DMD de CHAQUE paquet recu, peu importe
        # son type -- voir DMD_IP_CACHE_PATH.
        update_dmd_ip_cache(addr[0])

        # v2 -- distingue desormais 2 types de message sur ce port (avant,
        # v1 traitait TOUT paquet comme un "hello") :
        # - "FEATURES:<...>" : reglages hi-score/info/description/RA, ecrits
        #   tels quels dans FEATURES_CACHE_PATH (voir sa declaration).
        # - tout le reste (typiquement "HELLO") : comportement v1 inchange,
        #   resync de l'etat de navigation courant.
        text = data.decode("utf-8", "replace")
        # v3 -- ping/pong dedie, voir changelog v3 -- reponse immediate,
        # brute (pas via send_udp(), qui formate en "CMD=.../ARG=...").
        if text == "PING":
            send_sock.sendto(b"PONG", (addr[0], DMD_UDP_PORT))
            send_theme(send_sock, addr[0])  # v6 -- pris en compte si le theme a change (<= 15 s)
            continue
        if text.startswith(FEATURES_PREFIX):
            payload = text[len(FEATURES_PREFIX):]
            try:
                with open(FEATURES_CACHE_PATH, "w") as f:
                    f.write(payload + "\n")
                log(f"features de {addr[0]} -> cache mis a jour: {payload}")
            except Exception as e:
                log(f"ERREUR ecriture {FEATURES_CACHE_PATH}: {e}")
            continue

        send_theme(send_sock, addr[0], force=True)  # v6 -- AVANT la resync : le logo systeme qui suit utilise deja le theme
        cmds = compute_current_state()
        log(f"hello de {addr[0]} -> resync {cmds}")
        for cmd, arg in cmds:
            send_udp(send_sock, addr[0], cmd, arg)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
