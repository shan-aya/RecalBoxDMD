#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# direct_harvest_mame0278_rb2.py v6 -- variante de direct_harvest_mame0278.py
# (RB1, JAMMA/CRT) adaptee au materiel x86/X11 (ex. Steam Deck, pas de
# CRT/JAMMA, manette integree, controleur graphique nécessitant DISPLAY).
# Ligne de lancement capturee en reel le 2026-08-23 (jeu lance normalement
# via ES) -- uniquement -system/-rom/-core changent d'un jeu a l'autre.
#
# BUG REEL trouve/corrige (2026-08-23, voir DECISIONS.md "Récolte multi-RB")
# : sans DISPLAY/XDG_RUNTIME_DIR exportes, retroarch segfaultait
# systematiquement (jump a un pointeur de fonction NULL) -- confirme via
# /proc/<pid du process openbox lance par ES>/environ (DISPLAY=:0,
# XDG_RUNTIME_DIR=/run/user/0), absents d'une session SSH nue.
#
# v2 -- 2eme BUG REEL trouve/corrige, PLUS IMPORTANT que le premier (retour
# utilisateur explicite : "est-ce que ES sur steamdeck a les memes
# limitations que le CRT rb1 en occupation exclusive de l'ecran ? qui t'a
# oblige a kill ES ?") : contrairement a RB1 (Raspberry Pi, framebuffer/KMS
# EXCLUSIF -- un seul proprietaire possible, ES DOIT etre coupe avant tout
# lancement direct), RB2 tourne sous X11 (Steam Deck), concu pour PLUSIEURS
# clients simultanes -- rien n'oblige a couper ES ici. La version v1 de ce
# script coupait/relancait ES/Xorg avant/apres chaque run (motif copie de
# RB1 sans le remettre en question) -- ces coupures/relances REPETEES du
# serveur X etaient la VRAIE cause des segfaults massifs et apparemment
# intermittents observes cette nuit (dmesg confirmait en parallele
# "[drm] Failed to add display topology, DTM TA is not initialized." --
# le pilote amdgpu ne supporte visiblement pas bien des transitions
# d'affichage X repetees rapidement). Valide en reel : 5 jeux qui
# echouaient a 100% (280zzzap/3wonders/3wondersb/4enraya/64street) dans
# l'ancien cycle "coupe tout avant, relance tout apres" reussissent 5/5
# SANS AUCUN ECHEC en laissant simplement ES actif en permanence, sans
# jamais y toucher. **Ce script ne doit donc PLUS jamais arreter/relancer
# ES ou Xorg** -- les lancements directs cohabitent normalement avec ES qui
# tourne, exactement comme n'importe quel jeu lance normalement par
# l'utilisateur.
#
# v3 -- capture d'ecran INTEGREE a la boucle de recolte (demande
# utilisateur explicite : "avec le comparatif screenshot directement plutot
# que de devoir repasser dessus apres"). Juste avant le QUIT propre, envoie
# la commande reseau RetroArch SCREENSHOT (capture le rendu GPU reel,
# contrairement a fbgrab/dev/fb0 qui ne capture qu'un contenu perime --
# voir DECISIONS.md "methode verite d'abord") -- le fichier PNG atterrit
# dans le screenshot_directory configure de RetroArch (verifier avec
# `grep screenshot_directory /recalbox/share/system/configs/retroarch/
# retroarchcustom.cfg`, meme convention que RB1 a priori :
# /recalbox/share/screenshots/), nomme "<rom>-<date>-<heure>.png". Fait
# pour TOUS les jeux qui chargent (pas seulement ceux avec .hi peuple) --
# donne directement, a la fin du lot, un jeu de captures pretes pour la
# methode "verite d'abord" (tools/build_entry_from_truth.py) SANS avoir a
# relancer quoi que ce soit.
#
# v4 -- BUG REEL corrige (2026-08-23, retour utilisateur direct : "tu as
# relance la phase 1 sur les jeux deja analyses ?") : le seul critere de
# skip etait la presence d'un .hi (`os.path.exists(save_path)`), donc tout
# rom SANS .hi (l'immense majorite, vu le taux de peuplement -- voir
# DECISIONS.md) etait RELANCE A CHAQUE REDEMARRAGE du script, meme s'il
# avait deja ete tente et avait deja une reponse definitive ("pas de .hi",
# "jamais apparu"...). Chaque pause/reprise de ce soir (tests RAM live,
# decoupage en tranches, etc.) repartait donc du debut de la liste --
# verifie sur le log : 471 tentatives pour seulement 172 roms distincts,
# certains relances jusqu'a 11 fois (`1944u`), ~63% de travail machine
# perdu en pure repetition. Fix : fichier de suivi persistant
# (ATTEMPTED_FILE, un rom par ligne, alimente a chaque etat terminal
# atteint) charge au demarrage -- un rom deja tente cette campagne est
# desormais saute au meme titre qu'un rom deja peuple, quel que soit le
# nombre de redemarrages du script.
#
# v5 -- dwell configurable (--dwell N) : plusieurs jeux du lot 1 captures
# pendant un ecran de CHARGEMENT (bloxeed noir, cpsoccer compteur "DECO
# CASSETTE SYSTEM") -- .hi peuple mais memoire non pertinente au moment
# de la capture. Teste : dwell 30s (50 roms) -> 16% de .hi peuples contre
# ~8.6% a dwell 15s sur la tranche 1 -- mieux, mais pas suffisant seul.
#
# v6 -- 2026-08-24 - safe-modify - simule un CREDIT + START via
# l'interface reseau RetroArch (PLAYER1_SELECT x2 puis PLAYER1_START,
# apres BOOT_SETTLE_S) au lieu d'attendre passivement un cycle attract-
# mode qui peut ne jamais afficher de score sur les jeux a intro longue.
# Valide en reel : sur 3wondersb (bloque sur ecran d'histoire meme a 45s
# de dwell PASSIF), credit+start fait apparaitre un vrai ecran GAMESELECT
# 10s plus tard -- la simulation d'input fonctionne reellement. Objectif :
# forcer une VRAIE partie (SCORE au HUD des les premieres secondes) au
# lieu d'esperer tomber sur le bon instant du cycle passif. Desactivable
# via --no-input pour comparaison cote a cote avec l'ancien comportement.
import subprocess, time, socket, os, sys

os.environ["DISPLAY"] = ":0"
os.environ["XDG_RUNTIME_DIR"] = "/run/user/0"

ROM_LIST_FILE = sys.argv[1] if len(sys.argv) > 1 else "/tmp/priority_mame0278.txt"
LOG = "/tmp/direct_harvest_mame0278_log.txt"
# v4 -- fichier de suivi des roms DEJA TENTEES cette campagne (2026-08-23,
# bug reel trouve : le seul critere de skip etait la presence d'un .hi,
# donc chaque redemarrage du script -- pause pour un test, decoupage en
# tranches, etc. -- refaisait tourner TOUS les jeux depuis le debut de la
# liste, meme ceux deja tentes sans .hi. Verifie sur le log de ce soir :
# 471 tentatives pour seulement 172 roms distincts, certains relances
# jusqu'a 11 fois. Ce fichier est un journal simple (1 rom par ligne,
# ajoute des qu'un rom atteint un etat terminal, quel qu'il soit) --
# persiste entre les runs, contrairement au calcul depuis un fichier de
# liste qu'il faudrait reconstruire manuellement a chaque fois.
ATTEMPTED_FILE = "/tmp/direct_harvest_mame0278_attempted.txt"
LAUNCH_TEMPLATE = (
    "python3 /usr/bin/emulatorlauncher.pyc "
    "-p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name \"Steam Deck\" "
    "-p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath /dev/input/event14 "
    "-p1physicalpath \"pci-0000:04:00.4-usb-0:3:1.2\" "
    "-system mame -rom /recalbox/share/roms/mame/mame0278/{rom}.zip -emulator libretro -core mame0278 "
    "-ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade"
)
LOAD_WAIT_S = 20
BOOT_SETTLE_S = 4  # v6 -- laisse le jeu finir son boot avant d'envoyer credit+start
DWELL_S = 15
QUIT_WAIT_S = 15
SAVE_GLOB = "/recalbox/share/saves/mame/mame0278/hiscore/{rom}.hi"
NO_INPUT = False  # v6 -- desactivable via --no-input pour comparaison


def log(msg):
    line = "{} {}".format(time.strftime("%H:%M:%S"), msg)
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def find_retroarch_pid(rom):
    try:
        out = subprocess.check_output(
            "ps -o pid,args -ww | grep -- '/{}.zip' | grep retroarch | grep -v grep".format(rom),
            shell=True, text=True)
    except subprocess.CalledProcessError:
        return None
    line = out.strip().splitlines()[0] if out.strip() else ""
    if not line:
        return None
    return int(line.split()[0])


def send_udp(cmd):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.sendto(cmd, ("127.0.0.1", 55355))
        s.close()
    except Exception as e:
        log("!! erreur envoi {}: {}".format(cmd, e))


def send_screenshot():
    send_udp(b"SCREENSHOT\n")


def send_quit():
    send_udp(b"QUIT\n")


def send_credit_and_start():
    # v6 - 2026-08-24 - safe-modify - simule un credit + start via
    # l'interface reseau RetroArch (meme port 55355, commandes fire-and-
    # forget -- pas de reponse, contrairement a READ_CORE_RAM/GET_STATUS).
    # Valide en reel ce soir : sur 3wondersb (bloque sur ecran d'histoire
    # meme a 45s de dwell PASSIF), credit+start fait apparaitre un vrai
    # ecran GAMESELECT 10s plus tard -- la simulation d'input fonctionne
    # reellement, pas juste acceptee silencieusement sans effet. Objectif :
    # forcer une VRAIE partie (SCORE au HUD des les premieres secondes)
    # au lieu d'esperer tomber sur le bon instant du cycle attract-mode
    # passif (methode des lots 1/2 de la nuit precedente, peu fiable sur
    # les jeux a intro longue). 2 credits envoyes (certains jeux/bornes
    # 2 joueurs exigent 2 credits pour demarrer) puis 1 start.
    for _ in range(2):
        send_udp(b"PLAYER1_SELECT\n")
        time.sleep(0.5)
    time.sleep(0.5)
    send_udp(b"PLAYER1_START\n")


def wait_gone(pid, timeout_s):
    waited = 0
    while waited < timeout_s:
        try:
            os.kill(pid, 0)
        except OSError:
            return True
        time.sleep(1)
        waited += 1
    return False


def mark_attempted(rom):
    with open(ATTEMPTED_FILE, "a") as f:
        f.write(rom + "\n")


def harvest_one(rom, attempted):
    save_path = SAVE_GLOB.format(rom=rom)
    if os.path.exists(save_path):
        log("(skip) {} -- .hi deja present".format(rom))
        return
    if rom in attempted:
        log("(skip) {} -- deja tente cette campagne (voir {})".format(rom, ATTEMPTED_FILE))
        return

    cmd = LAUNCH_TEMPLATE.format(rom=rom)
    proc = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    pid = None
    waited = 0
    while waited < LOAD_WAIT_S:
        pid = find_retroarch_pid(rom)
        if pid:
            break
        time.sleep(1)
        waited += 1

    if not pid:
        log(">>> {} : retroarch jamais apparu ({}s) -- abandon".format(rom, LOAD_WAIT_S))
        try:
            proc.terminate()
        except Exception:
            pass
        time.sleep(2)
        mark_attempted(rom)
        return

    log(">>> {} lance (pid={})".format(rom, pid))
    time.sleep(BOOT_SETTLE_S)

    if not NO_INPUT:
        send_credit_and_start()

    log("    attente {}s ({})".format(DWELL_S, "credit+start envoyes" if not NO_INPUT else "dwell passif seul"))
    time.sleep(DWELL_S)

    send_screenshot()
    time.sleep(1)  # laisse RetroArch ecrire le fichier avant le QUIT

    send_quit()
    ended_cleanly = wait_gone(pid, QUIT_WAIT_S)
    if not ended_cleanly:
        log("!! {} n'a pas quitte apres QUIT ({}s) -- kill forcé".format(rom, QUIT_WAIT_S))
        try:
            os.kill(pid, 9)
        except OSError:
            pass
        time.sleep(2)

    try:
        proc.wait(timeout=5)
    except Exception:
        pass

    if os.path.exists(save_path):
        size = os.path.getsize(save_path)
        with open(save_path, "rb") as f:
            data = f.read()
        populated = any(b != 0 for b in data)
        log("==> {} : .hi present ({} octets, {})".format(
            rom, size, "PEUPLE" if populated else "zero"))
    else:
        log("==> {} : pas de .hi".format(rom))
    mark_attempted(rom)


def main():
    global DWELL_S, NO_INPUT
    if "--no-input" in sys.argv:
        # v6 -- desactive le credit+start (dwell passif seul, comportement
        # v5) -- utile pour comparer les deux methodes cote a cote.
        NO_INPUT = True
    if "--dwell" in sys.argv:
        # v5 - 2026-08-23 - safe-modify - dwell configurable (retour
        # utilisateur : plusieurs jeux du lot 1 captures pendant un
        # ecran de CHARGEMENT -- bloxeed (noir), cpsoccer (compteur
        # "DECO CASSETTE SYSTEM" pas encore a 000) -- .hi peuple mais
        # memoire non pertinente au moment de la capture. Hypothese a
        # tester : un dwell plus long avant SCREENSHOT/QUIT laisse le
        # temps au chargement de se terminer sur ces jeux specifiques
        # (diversite materielle MAME bien plus large que FBNeo, ex.
        # systemes a cassette simulee -- voir DECISIONS.md). Defaut
        # inchange (15s) tant que l'hypothese n'est pas validee.
        DWELL_S = int(sys.argv[sys.argv.index("--dwell") + 1])
    with open(ROM_LIST_FILE) as f:
        roms = [l.strip() for l in f if l.strip()]
    attempted = set()
    if os.path.exists(ATTEMPTED_FILE):
        with open(ATTEMPTED_FILE) as f:
            attempted = set(l.strip() for l in f if l.strip())
    log("=== direct_harvest_mame0278_rb2 demarre, {} roms en file, {} deja tentees cette campagne ===".format(
        len(roms), len(attempted)))
    for rom in roms:
        try:
            harvest_one(rom, attempted)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
        time.sleep(2)
    log("=== direct_harvest_mame0278_rb2 termine ===")
    # v2 -- ES n'a JAMAIS ete coupe (voir commentaire d'en-tete), rien a
    # relancer.


if __name__ == "__main__":
    main()
