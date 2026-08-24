#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v2
#
# v2 - 2026-08-24 - safe-modify - /dev/input/eventN resolu dynamiquement
#   (rb2_input_device.py) au lieu d'un chemin code en dur -- voir
#   DECISIONS.md "piege event14/reboot RB2".
#
# v1 - 2026-08-24 - safe-modify - Creation initiale. Remplace le mode
#   "lot avec parametres fixes identiques pour tous" de
#   direct_harvest_mame0278_rb2.py (v1-v7) par un traitement JEU PAR JEU,
#   piloté par l'ETAT REEL de l'ecran (diff d'images, Pillow, confirme
#   installe sur RB2) au lieu d'un dwell/credit+start envoye a un instant
#   fixe devine a l'aveugle.
#
#   Demande utilisateur explicite (2026-08-24) : (1) traiter les jeux un
#   par un, processus complet, documenter richement CHAQUE echec et sa
#   raison (pas juste "pas de .hi") ; (2) detecter reellement l'ecran
#   "INSERT COIN" (et les ecrans de selection apres credit, ex. carts
#   compilation) au lieu d'agir a l'aveugle ; (3) croiser le gamelist
#   (genre/annee) avec hiscore.dat pour adapter la methode par categorie
#   de jeu au lieu de traiter une liste dans un ordre arbitraire avec les
#   memes reglages pour tous.
#
#   Motive par un echec concret de direct_harvest_mame0278_rb2.py v6/v7 :
#   sur `darkseal`, le credit+start envoye a un instant fixe (meme
#   retente 2x, a des instants differents) n'a JAMAIS eu d'effet -- ecran
#   de cinematique IDENTIQUE avant/apres. Ce jeu a une intro NON-
#   interactive de duree fixe -- deviner un instant ne peut pas marcher,
#   il faut DETECTER le bon moment (ici : la frontiere de boucle du cycle
#   attract-mode, via hash perceptuel, cf step_wait_input_screen()).
#
#   Reutilise SANS dupliquer : LAUNCH_TEMPLATE/find_retroarch_pid/
#   wait_gone/send_credit_and_start (direct_harvest_mame0278_rb2.py),
#   read_core_ram_chunk/dump_region/udp_client + regle "socket dediee
#   READ_CORE_RAM jamais melangee au fire-and-forget" (bug reel deja
#   trouve/corrige ce soir sur rb2_ram_truth_capture.py -- une reponse
#   residuelle de SCREENSHOT avalee par le premier recv() d'un dump RAM),
#   parse_hiscore_dat/compute_offsets (parse_hiscore_dat.py).
#
#   Decouverte exploitee : /recalbox/share/roms/mame/gamelist.xml (scrape
#   EmulationStation, 610669 lignes) contient <genre>/<releasedate> pour
#   42108 roms mame0278 -- 462 genres distincts, taxonomie francaise a 2
#   niveaux. Les jeux casino/electro-mecanique (~19684 roms) sont deja
#   PRESQUE TOTALEMENT absents de priority_mame0278.txt (9/19684 en
#   overlap, verifie) car sans entree hiscore.dat -- aucun filtrage
#   supplementaire necessaire pour ca, deja regle naturellement par la
#   construction de la liste priorite.
#
#   Regles non negociables heritees de DECISIONS.md "Recolte MAME0278" :
#   ne JAMAIS couper/relancer ES/Xorg sur cette machine X11 (RB2, pas
#   d'occupation exclusive d'ecran, contrairement a RB1/framebuffer),
#   DISPLAY=:0/XDG_RUNTIME_DIR=/run/user/0 requis pour tout lancement
#   direct.
"""Recolte hi-score MAME0278 pilotee par etat d'ecran + categorisation
par genre -- traite les roms UN PAR UN avec le processus complet
(detection ecran credit -> credit -> verification effet -> confirmation
eventuelle -> capture finale -> verification .hi), journal JSONL riche
(succes ET echecs documentes avec leur raison precise).

Usage: rb2_smart_harvest.py [--romlist FILE] [--limit N] [--category CAT] [--force]
"""
import glob
import json
import os
import socket
import subprocess
import sys
import time

from PIL import Image, ImageChops

os.environ["DISPLAY"] = ":0"
os.environ["XDG_RUNTIME_DIR"] = "/run/user/0"

sys.path.insert(0, os.path.dirname(__file__))
from parse_hiscore_dat import parse_hiscore_dat, compute_offsets  # noqa: E402
from rb2_ram_truth_capture import udp_client as ram_socket, read_core_ram_chunk, dump_region  # noqa: E402
from rb2_input_device import detect_steam_deck_device  # noqa: E402

GAMELIST_PATH = "/recalbox/share/roms/mame/gamelist.xml"
HISCORE_DAT_PATH = "/usr/share/libretro-mame/mame0278/plugins/hiscore/hiscore.dat"
SCREENSHOT_DIR = "/recalbox/share/screenshots"
SAVE_GLOB = "/recalbox/share/saves/mame/mame0278/hiscore/{rom}.hi"
DUMP_DIR = "/tmp/smart_ramtruth"
LOG = "/tmp/rb2_smart_harvest_log.txt"
LOG_JSONL = "/tmp/rb2_smart_harvest_log.jsonl"
ATTEMPTED_FILE = "/tmp/rb2_smart_harvest_attempted.txt"

# v2 - 2026-08-24 - safe-modify - /dev/input/eventN resolu dynamiquement
# (rb2_input_device.py) au lieu d'un chemin code en dur -- voir
# DECISIONS.md "piege event14/reboot RB2".
DEVICE_PATH = detect_steam_deck_device()
LAUNCH_TEMPLATE = (
    "python3 /usr/bin/emulatorlauncher.pyc "
    "-p1index 0 -p1guid 0300f617de2800000512000010010000 -p1name \"Steam Deck\" "
    "-p1nbaxes 10 -p1nbhats 0 -p1nbbuttons 22 -p1devicepath {device} "
    "-p1physicalpath \"pci-0000:04:00.4-usb-0:3:1.2\" "
    "-system mame -rom /recalbox/share/roms/mame/mame0278/{{rom}}.zip -emulator libretro -core mame0278 "
    "-ratio auto -videobackend default -rotation 0 -resolution 1280x800 -systemtype arcade"
).format(device=DEVICE_PATH)
QUIT_WAIT_S = 15

# --- categorisation par genre -------------------------------------------
# Ordre = priorite de match (premiere sous-chaine trouvee dans le genre,
# insensible a la casse). Genre absent ou aucun match -> UNKNOWN.
GENRE_KEYWORDS = [
    ("compilation", "COMPILATION"),
    ("shoot'em up", "FAST_ACTION"),
    ("tir", "FAST_ACTION"),
    ("combat", "LONG_INTRO"),
    ("versus", "LONG_INTRO"),
    ("beat'em", "LONG_INTRO"),
    ("sport", "LONG_INTRO"),
    ("course", "RACING"),
    ("conduite", "RACING"),
    ("plateforme", "SIMPLE"),
    ("puzzle", "SIMPLE"),
    ("labyrinthe", "SIMPLE"),
    ("action", "SIMPLE"),
]

CATEGORY_PARAMS = {
    "FAST_ACTION": dict(load_wait_s=20, boot_settle_s=3, wait_input_timeout_s=15,
                         credit_retries=2, credit_retry_wait_s=4, confirm_steps=0,
                         post_start_dwell_s=8),
    "SIMPLE": dict(load_wait_s=20, boot_settle_s=3, wait_input_timeout_s=20,
                   credit_retries=2, credit_retry_wait_s=5, confirm_steps=0,
                   post_start_dwell_s=10),
    "LONG_INTRO": dict(load_wait_s=20, boot_settle_s=5, wait_input_timeout_s=40,
                        credit_retries=3, credit_retry_wait_s=8, confirm_steps=1,
                        post_start_dwell_s=12),
    "RACING": dict(load_wait_s=20, boot_settle_s=4, wait_input_timeout_s=30,
                    credit_retries=2, credit_retry_wait_s=6, confirm_steps=1,
                    post_start_dwell_s=10),
    "COMPILATION": dict(load_wait_s=20, boot_settle_s=5, wait_input_timeout_s=30,
                         credit_retries=2, credit_retry_wait_s=6, confirm_steps=2,
                         post_start_dwell_s=10),
    "UNKNOWN": dict(load_wait_s=25, boot_settle_s=5, wait_input_timeout_s=45,
                     credit_retries=3, credit_retry_wait_s=8, confirm_steps=2,
                     post_start_dwell_s=12),
}

# --- seuils de classification par diff d'image --------------------------
PIXEL_DIFF_THRESHOLD = 30    # ecart de gris (0-255) pour compter un pixel "change"
FROZEN_RATIO_MAX = 0.02      # < 2% change -> ecran fige (attente probable)
ACTIVE_RATIO_MIN = 0.15      # >= 15% -> ecran qui bouge activement
VERIFY_EFFECT_RATIO_MIN = 0.08  # seuil pour confirmer qu'un credit a eu un effet reel
LOOP_HAMMING_MAX = 4         # distance de Hamming (sur 64 bits) pour une repetition de boucle
FRAME_SIZE = (160, 100)      # downscale pour les diffs (rapide, lisse l'antialiasing)


def log(msg):
    line = "{} {}".format(time.strftime("%H:%M:%S"), msg)
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def iso_now():
    return time.strftime("%Y-%m-%dT%H:%M:%S")


# --- gamelist.xml : chargement + categorisation --------------------------

def load_gamelist(path=GAMELIST_PATH):
    """Parse en streaming (iterparse, fichier de 610k lignes) ->
    {rom_basename: {"genre": str|None, "releasedate": str|None}}.

    BUG REEL trouve/corrige (2026-08-24, 1er test) : `elem.clear()`
    applique a CHAQUE element (y compris les enfants <path>/<genre> au
    moment ou LEUR PROPRE evenement "end" survient, avant celui de leur
    parent <game>) effacait leur texte -- par le temps que <game> finisse
    et que find("path")/find("genre") soient appeles, les enfants etaient
    deja vides. Resultat : 0 rom charge. Fix : ne clear() QUE l'element
    <game> lui-meme, une fois ses enfants deja lus."""
    import xml.etree.ElementTree as ET
    result = {}
    for event, elem in ET.iterparse(path, events=("end",)):
        if elem.tag == "game":
            path_el = elem.find("path")
            if path_el is not None and path_el.text:
                basename = os.path.splitext(os.path.basename(path_el.text))[0]
                genre_el = elem.find("genre")
                date_el = elem.find("releasedate")
                result[basename] = {
                    "genre": genre_el.text if genre_el is not None else None,
                    "releasedate": date_el.text if date_el is not None else None,
                }
            elem.clear()
    return result


def categorize(genre):
    if not genre:
        return "UNKNOWN"
    g = genre.lower()
    for keyword, cat in GENRE_KEYWORDS:
        if keyword in g:
            return cat
    return "UNKNOWN"


def rom_params(rom, gamelist):
    info = gamelist.get(rom, {})
    genre = info.get("genre")
    releasedate = info.get("releasedate")
    category = categorize(genre)
    if category == "UNKNOWN" and releasedate:
        try:
            if int(releasedate[:4]) < 1985:
                category = "SIMPLE"
        except ValueError:
            pass
    return category, CATEGORY_PARAMS[category], genre, releasedate


# --- lancement / process --------------------------------------------------

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
    # meme motif que direct_harvest_mame0278_rb2.py v6/v7 (valide en
    # reel) -- 2 credits (certaines bornes 2 joueurs en exigent 2) puis 1
    # start, fire-and-forget UDP.
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


def quit_and_cleanup(pid, proc):
    send_quit()
    time.sleep(1)
    ended = wait_gone(pid, QUIT_WAIT_S)
    if not ended:
        try:
            os.kill(pid, 9)
        except OSError:
            pass
        time.sleep(2)
    try:
        proc.wait(timeout=5)
    except Exception:
        pass


def mark_attempted(rom):
    with open(ATTEMPTED_FILE, "a") as f:
        f.write(rom + "\n")


# --- capture + diff d'ecran (Pillow, directement sur RB2) -----------------

def latest_screenshot(rom, after_ts):
    pattern = os.path.join(SCREENSHOT_DIR, "{}-*.png".format(rom))
    candidates = [p for p in glob.glob(pattern) if os.path.getmtime(p) >= after_ts]
    if not candidates:
        return None
    return max(candidates, key=os.path.getmtime)


def capture_frame(rom):
    """Envoie SCREENSHOT, attend l'ecriture, ouvre le PNG en niveaux de
    gris downscale -> (chemin, image PIL) ou (None, None)/(chemin, None)
    si echec de capture/ouverture."""
    ts_before = time.time()
    send_screenshot()
    time.sleep(1.0)  # laisse RetroArch ecrire le fichier (motif deja valide ce soir)
    path = latest_screenshot(rom, ts_before - 1)
    if not path:
        return None, None
    try:
        img = Image.open(path).convert("L").resize(FRAME_SIZE, Image.BILINEAR)
    except Exception:
        return path, None
    return path, img


def diff_ratio(img_a, img_b):
    diff = ImageChops.difference(img_a, img_b)
    bw = diff.point(lambda p: 255 if p > PIXEL_DIFF_THRESHOLD else 0)
    hist = bw.histogram()
    changed = hist[255] if len(hist) > 255 else 0
    total = bw.size[0] * bw.size[1]
    return changed / total if total else 0.0


def classify_pair(img_a, img_b):
    r = diff_ratio(img_a, img_b)
    if r < FROZEN_RATIO_MAX:
        return "FROZEN", r
    if r >= ACTIVE_RATIO_MIN:
        return "ACTIVE", r
    return "BLINK", r


def ahash(img):
    small = img.resize((8, 8), Image.BILINEAR)
    pixels = list(small.getdata())
    avg = sum(pixels) / 64.0
    bits = 0
    for v in pixels:
        bits = (bits << 1) | (1 if v > avg else 0)
    return bits


def hamming(a, b):
    return bin(a ^ b).count("1")


# --- etapes de la machine a etats -----------------------------------------

def step_wait_input_screen(rom, params):
    """Attend soit un ecran stable (FROZEN/BLINK, 2 classifications
    consecutives), soit une repetition de boucle attract-mode (hash
    perceptuel -- resout le cas darkseal : cinematique NON-interactive de
    duree fixe, ou seule la fin de boucle a un sens comme instant pour le
    credit). Si rien n'est detecte avant le timeout, tente quand meme un
    essai "a l'aveugle" (preserve la chance de succes de v6/v7) mais le
    marque explicitement comme tel dans le journal."""
    timeout = params["wait_input_timeout_s"]
    t0 = time.time()
    frames = []  # [(t, path, img, hash), ...]
    prev_state = None
    samples = []
    last_path = None
    while time.time() - t0 < timeout:
        path, img = capture_frame(rom)
        if img is None:
            time.sleep(2)
            continue
        last_path = path
        h = ahash(img)

        loop_hit = next((f for f in frames if hamming(h, f[3]) <= LOOP_HAMMING_MAX), None)

        state, ratio = (None, None)
        if frames:
            state, ratio = classify_pair(frames[-1][2], img)
        samples.append({"t": round(time.time() - t0, 1), "state": state,
                         "ratio": round(ratio, 3) if ratio is not None else None,
                         "screenshot": path})

        if loop_hit is not None:
            return {"detection": "loop_boundary", "screenshot": path,
                    "elapsed": round(time.time() - t0, 1), "samples": samples}
        if state in ("FROZEN", "BLINK") and prev_state in ("FROZEN", "BLINK"):
            return {"detection": "stable_screen", "screenshot": path,
                     "elapsed": round(time.time() - t0, 1), "samples": samples}

        frames.append((time.time() - t0, path, img, h))
        prev_state = state
        time.sleep(2)

    return {"detection": "blind_fallback", "screenshot": last_path,
            "elapsed": round(time.time() - t0, 1), "samples": samples}


def step_verify_credit_effect(rom, pre_img, params):
    attempts = []
    wait_s = params["credit_retry_wait_s"]
    post_img, post_path = None, None
    for attempt in range(1, params["credit_retries"] + 1):
        send_credit_and_start()
        time.sleep(wait_s)
        post_path, post_img = capture_frame(rom)
        if post_img is None:
            attempts.append({"attempt": attempt, "ok": False, "ratio": None, "error": "no_screenshot"})
            continue
        ratio = diff_ratio(pre_img, post_img) if pre_img is not None else 1.0
        ok = ratio >= VERIFY_EFFECT_RATIO_MIN
        attempts.append({"attempt": attempt, "ok": ok, "ratio": round(ratio, 3), "screenshot": post_path})
        if ok:
            return {"ok": True, "attempts": attempts, "post_img": post_img, "post_screenshot": post_path}
        pre_img = post_img
        wait_s = min(wait_s * 1.5, 15)
    return {"ok": False, "attempts": attempts, "post_img": post_img, "post_screenshot": post_path}


def step_post_credit_classify(rom, post_img, params):
    path, img2 = capture_frame(rom)
    if img2 is None or post_img is None:
        return {"state": "UNKNOWN", "ratio": None, "screenshot": path, "img": img2}
    state, ratio = classify_pair(post_img, img2)
    return {"state": state, "ratio": round(ratio, 3), "screenshot": path, "img": img2}


def step_confirm_loop(rom, current_img, params):
    steps = []
    for i in range(1, params["confirm_steps"] + 1):
        send_udp(b"PLAYER1_START\n")
        time.sleep(3)
        path, new_img = capture_frame(rom)
        if new_img is None or current_img is None:
            steps.append({"step": i, "ok": False, "error": "no_screenshot"})
            continue
        state, ratio = classify_pair(current_img, new_img)
        active = (state == "ACTIVE")
        steps.append({"step": i, "ok": active, "state": state, "ratio": round(ratio, 3), "screenshot": path})
        current_img = new_img
        if active:
            return {"ok": True, "steps": steps, "final_img": new_img}
    return {"ok": False, "steps": steps, "final_img": current_img}


def try_ram_dump(rom):
    """Sonde READ_CORE_RAM (16 octets) ; si le driver repond, fait un
    dump complet 0x0000-0x10000 (meme convention que
    rb2_ram_truth_capture.py) dans DUMP_DIR. Best-effort, jamais
    bloquant."""
    os.makedirs(DUMP_DIR, exist_ok=True)
    dump_path = os.path.join(DUMP_DIR, "{}.bin".format(rom))
    s = ram_socket()
    try:
        probe = read_core_ram_chunk(s, 0, 16)
        if not probe:
            return False, None
        dump_region(s, 0x0000, 0x10000, dump_path)
        return True, dump_path
    finally:
        s.close()


def step_hi_check(rom, hiscore_entries):
    save_path = SAVE_GLOB.format(rom=rom)
    blocks = hiscore_entries.get(rom)
    expected = None
    if blocks:
        offsets = compute_offsets(blocks)
        if offsets:
            last_off, last_len = offsets[-1]
            expected = last_off + last_len
    if not os.path.exists(save_path):
        return {"hi_status": "not_produced", "hi_size": None,
                "hi_size_matches_expected": None, "expected_size": expected}
    size = os.path.getsize(save_path)
    with open(save_path, "rb") as f:
        data = f.read()
    populated = any(b != 0 for b in data)
    matches = (expected == size) if expected is not None else None
    return {"hi_status": "populated" if populated else "zero", "hi_size": size,
            "hi_size_matches_expected": matches, "expected_size": expected}


def finalize_record(record, final_state, t_start):
    record["final_state"] = final_state
    record["fail_reason"] = None if final_state.startswith("hi_produced") else final_state
    record["ts_end"] = iso_now()
    record["duration_s"] = round(time.time() - t_start, 1)


def append_jsonl(record):
    with open(LOG_JSONL, "a", encoding="utf-8") as f:
        f.write(json.dumps(record, ensure_ascii=False) + "\n")


# --- orchestration par jeu -------------------------------------------------

def harvest_one(rom, gamelist, hiscore_entries, attempted):
    save_path = SAVE_GLOB.format(rom=rom)
    if os.path.exists(save_path):
        log("(skip) {} -- .hi deja present".format(rom))
        return None
    if rom in attempted:
        log("(skip) {} -- deja traite par ce script (voir {})".format(rom, ATTEMPTED_FILE))
        return None

    category, params, genre, releasedate = rom_params(rom, gamelist)
    record = {"rom": rom, "genre": genre, "releasedate": releasedate, "category": category,
              "params": params, "ts_start": iso_now(), "steps": []}
    t_start = time.time()

    def add_step(step_name, **kw):
        # BUG REEL trouve/corrige (2026-08-24, 1er test complet) :
        # parametre nomme "state" entrait en collision avec le champ
        # "state" (FROZEN/BLINK/ACTIVE) qu'on veut logger pour
        # POST_CREDIT_CLASSIFY -- "got multiple values for argument
        # 'state'". Renomme en step_name, cle "step" au lieu de "state"
        # dans l'entree.
        entry = {"step": step_name, "t": round(time.time() - t_start, 1)}
        entry.update(kw)
        record["steps"].append(entry)

    cmd = LAUNCH_TEMPLATE.format(rom=rom)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    pid = None
    waited = 0
    while waited < params["load_wait_s"]:
        pid = find_retroarch_pid(rom)
        if pid:
            break
        time.sleep(1)
        waited += 1

    if not pid:
        add_step("LAUNCH", ok=False, detail="retroarch jamais apparu")
        try:
            proc.terminate()
        except Exception:
            pass
        time.sleep(2)
        finalize_record(record, "retroarch_never_appeared", t_start)
        mark_attempted(rom)
        log(">>> {} : retroarch jamais apparu -- abandon".format(rom))
        return record

    add_step("LAUNCH", ok=True, detail="pid={}".format(pid))
    log(">>> {} lance (pid={}), categorie={}, genre={}".format(rom, pid, category, genre))

    try:
        time.sleep(params["boot_settle_s"])
        add_step("BOOT_SETTLE", ok=True)

        wis = step_wait_input_screen(rom, params)
        add_step("WAIT_INPUT_SCREEN", ok=True, **wis)
        log("    ecran d'attente: {} ({}s)".format(wis["detection"], wis["elapsed"]))

        pre_path, pre_img = capture_frame(rom)
        add_step("PRE_CREDIT_CAPTURE", ok=pre_img is not None, screenshot=pre_path)

        vce = step_verify_credit_effect(rom, pre_img, params)
        add_step("VERIFY_CREDIT_EFFECT", ok=vce["ok"], attempts=vce["attempts"])

        if not vce["ok"]:
            finalize_record(record, "credit_no_effect_after_{}_attempts".format(params["credit_retries"]), t_start)
            mark_attempted(rom)
            log("==> {} : ECHEC credit sans effet".format(rom))
            return record

        post_img = vce["post_img"]
        pcc = step_post_credit_classify(rom, post_img, params)
        add_step("POST_CREDIT_CLASSIFY", ok=True, state=pcc["state"], ratio=pcc["ratio"],
                  screenshot=pcc["screenshot"])

        active_img = pcc.get("img") or post_img
        if pcc["state"] != "ACTIVE" and params["confirm_steps"] > 0:
            cl = step_confirm_loop(rom, active_img, params)
            add_step("CONFIRM_LOOP", ok=cl["ok"], steps=cl["steps"])
            if not cl["ok"]:
                finalize_record(record, "stuck_on_confirm_screen_after_{}_steps".format(params["confirm_steps"]), t_start)
                mark_attempted(rom)
                log("==> {} : ECHEC bloque sur ecran de confirmation".format(rom))
                return record
            active_img = cl["final_img"]

        time.sleep(params["post_start_dwell_s"])
        add_step("POST_START_DWELL", ok=True)

        final_path, final_img = capture_frame(rom)
        add_step("FINAL_CAPTURE", ok=final_img is not None, screenshot=final_path)
        record["screenshot_final"] = final_path

        ram_supported, dump_path = None, None
        if not os.path.exists(save_path):
            ram_supported, dump_path = try_ram_dump(rom)
            add_step("RAM_DUMP_PROBE", ok=True, supported=ram_supported, path=dump_path)
        record["ram_dump_supported"] = ram_supported

        hi = step_hi_check(rom, hiscore_entries)
        record.update(hi)
        final_state = ("hi_produced_{}".format(hi["hi_status"])
                        if hi["hi_status"] != "not_produced" else "hi_not_produced")
        finalize_record(record, final_state, t_start)
        mark_attempted(rom)
        log("==> {} : {} (.hi {})".format(rom, final_state, hi["hi_status"]))
        return record
    finally:
        # filet de securite : quel que soit le chemin de sortie (y compris
        # une exception imprevue), ne jamais laisser retroarch orphelin --
        # idempotent si deja quitte (wait_gone renvoie True immediatement).
        quit_and_cleanup(pid, proc)


def main():
    romlist_path = "/tmp/priority_mame0278.txt"
    limit = None
    only_category = None
    force = False
    args = sys.argv[1:]
    if "--romlist" in args:
        romlist_path = args[args.index("--romlist") + 1]
    if "--limit" in args:
        limit = int(args[args.index("--limit") + 1])
    if "--category" in args:
        only_category = args[args.index("--category") + 1]
    if "--force" in args:
        force = True

    log("=== rb2_smart_harvest : chargement gamelist.xml ===")
    gamelist = load_gamelist()
    log("    {} roms avec metadonnees gamelist".format(len(gamelist)))

    hiscore_entries = parse_hiscore_dat(HISCORE_DAT_PATH)
    log("    {} entrees hiscore.dat".format(len(hiscore_entries)))

    with open(romlist_path) as f:
        roms = [l.strip() for l in f if l.strip()]

    attempted = set()
    if not force and os.path.exists(ATTEMPTED_FILE):
        with open(ATTEMPTED_FILE) as f:
            attempted = set(l.strip() for l in f if l.strip())

    if only_category:
        roms = [r for r in roms if rom_params(r, gamelist)[0] == only_category]
        log("    filtre categorie={} -> {} roms".format(only_category, len(roms)))

    if limit:
        roms = roms[:limit]

    log("=== rb2_smart_harvest demarre, {} roms en file, {} deja tentees ===".format(
        len(roms), len(attempted)))

    for rom in roms:
        try:
            record = harvest_one(rom, gamelist, hiscore_entries, attempted)
            if record:
                append_jsonl(record)
        except Exception as e:
            log("!! exception sur {}: {}".format(rom, e))
            # volontairement PAS mark_attempted -- une exception imprevue
            # doit rester retentable au prochain run, contrairement a un
            # etat terminal caracterise (meme regle que le filtre "casino
            # deja exclu naturellement" : ne jamais deviner/supposer sans
            # preuve, ici la preuve d'un etat terminal propre).
        time.sleep(2)
    log("=== rb2_smart_harvest termine ===")


if __name__ == "__main__":
    main()
