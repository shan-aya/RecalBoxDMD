from __future__ import annotations

# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v8
#
# v8 — 2026-07-19 — safe-modify — Échelle de zoom cadrage étendue de 0..1 à
#      -1.0..+1.0, demande explicite "-100% = resize seul, 0 = crop serré,
#      +100% = zoom" (voir dmd_converter.py v68 pour le détail UI). Nouvelle
#      `_zoom_crop_size` (partagée par `compute_crop_windows` et
#      `roi_rect_for_zoom`) : 2 segments d'interpolation linéaire de part et
#      d'autre de zoom=0 — [-1, 0] entre le frame ENTIER et le crop serré
#      (même formule que l'ancienne échelle 0..1, translatée de +1),
#      [0, +1] entre le crop serré et une fenêtre RÉTRÉCIE à
#      `min_zoom_fraction` (0.35 par défaut, ~2.9× de zoom numérique max —
#      valeur choisie pour rester exploitable sans réduire la fenêtre à
#      quelques pixels). `compute_crop_windows`/`roi_rect_for_zoom` clampent
#      désormais `zoom` à [-1, 1] (au lieu de [0, 1]) ; valeur par défaut de
#      `compute_crop_windows_from_events` alignée sur le nouveau "0 = crop
#      serré" (1.0→0.0). Vérifié en réel : nouveau test dédié
#      (`test_video_zoom_scale.py`, côté dmd_converter.py) couvrant les 3
#      points clés (-1/0/+1) + monotonie + clamp aux bornes.
#
# v7 — 2026-07-19 — safe-modify — Dépendance projet changée opencv-python →
#      opencv-contrib-python (voir dmd_converter.py v67 pour le diagnostic
#      complet : décrochage réel de TrackerMIL en cours de vidéo, signalé
#      par l'utilisateur — "il ne garde pas la sélection au milieu de la
#      hauteur... comme si le scroll vers le haut ne se faisait pas").
#      Aucun changement de LOGIQUE ici (`_create_tracker` essayait déjà
#      CSRT puis KCF puis MIL) — seule la docstring est mise à jour pour
#      refléter que CSRT/KCF sont désormais le chemin normal (dépendance
#      installée), MIL n'étant plus qu'un repli de défense en profondeur.
#      Vérifié en réel : même scénario de test (vidéo à mouvement
#      diagonal) sans aucun décrochage après le changement de dépendance,
#      contre un décrochage net à ~40% du clip avant.
#
# v6 — 2026-07-19 — safe-modify — Bug réel signalé par l'utilisateur (capture
#      d'écran) juste après la mise en service du v5 : "Erreur traitement
#      vidéo: OpenCV(5.0.0) .../tracker_mil.cpp:80: error: (-215:Assertion
#      failed) !posSamples.empty() in function
#      'cv::tracking::impl::TrackerMILImpl::init'". Cause : `track_roi`
#      passait la boîte ROI initiale à `tracker.init()` sans jamais valider
#      qu'elle restait dans les bornes du frame — un point de tracking posé/
#      déplacé près du bord (désormais possible librement dans le temps
#      depuis le v5) peut produire une boîte partiellement ou totalement
#      hors cadre, faisant planter TrackerMIL avec une AssertionError C++
#      (aucun échantillon positif à extraire pour l'initialisation). Fix :
#      nouvelle `_sanitize_initial_roi` (clampe la boîte à l'intersection
#      avec le frame, rejette si < 4px sur un axe après clamp) appelée AVANT
#      `tracker.init()` ; `tracker.init()` lui-même entouré d'un try/except
#      en défense en profondeur (repli sur le comportement "aucun tracker
#      disponible", ROI fixe, plutôt qu'un crash). Vérifié en réel : suite
#      de régression complète rejouée sans régression après le fix.
#
# v5 — 2026-07-19 — safe-modify — Refonte du tracking (zones multiples +
#      zoom keyframé), demande utilisateur explicite (fonction phare de
#      l'onglet VIDEO à rendre plus flexible) :
#      - `compute_crop_windows` : `zoom` accepte désormais soit un float
#        (comportement inchangé, rétrocompatible) soit une liste (un zoom
#        par frame, pour un zoom qui varie dans le temps).
#      - Nouvelle `interpolate_zoom_keyframes` : interpolation linéaire d'un
#        scalaire (zoom) dans le temps à partir d'une liste de
#        (timestamp, zoom) — même principe que l'ancienne
#        `interpolate_roi_keyframes` mais sur une valeur simple.
#      - Nouvelle `compute_crop_windows_from_events` : orchestrateur qui
#        remplace l'ancien choix binaire tracking-global/keyframes-manuelles
#        interpolées. Segmente la séquence de frames selon une liste
#        d'événements {"t", "roi", "mode": "auto"|"manual"} triés : un
#        segment "auto" réamorce `track_roi` sur la zone du point (lissage
#        EMA conservé, absorbe le tremblement du tracker) ; un segment
#        "manual" garde le ROI FIXE jusqu'au point suivant (SANS
#        interpolation, contrairement à l'ancien comportement — cut net,
#        pas de lissage, demande explicite de l'utilisateur : "chaque
#        nouvelle zone... arrête la précédente"). Le lissage EMA de
#        `compute_crop_windows` est cumulatif sur toute la séquence qu'on
#        lui donne — appliqué PAR SEGMENT (pas globalement) pour qu'un cut
#        de zone manuelle reste un cut net, sans bavure de transition.
#      - Supprimée : `interpolate_roi_keyframes` (plus d'appelant — le
#        pipeline appelle désormais `compute_crop_windows_from_events`, qui
#        ne fait PLUS d'interpolation entre zones manuelles, remplacée par
#        un maintien fixe par palier, voir ci-dessus).
#
# v4 — 2026-07-19 — safe-modify — Lecteur vidéo intégré (demande utilisateur,
#      cadre de lecture en boucle dans l'onglet VIDEO) : nouvelles
#      `open_capture`/`read_capture_frame`/`seek_capture`/
#      `capture_position_ms`/`release_capture` — gardent un VideoCapture
#      ouvert entre appels successifs (contrairement aux autres méthodes de
#      ce module, qui ouvrent/referment à chaque appel), pour une lecture en
#      boucle fluide sans réouvrir le fichier à chaque frame.
#
# v3 — 2026-07-19 — safe-modify — Cadrage manuel multi-instants + curseur
#      zoom crop/resize (demande utilisateur, onglet VIDEO) :
#      - `compute_crop_windows` : nouveau paramètre `zoom` (0.0-1.0),
#        interpole la TAILLE de la fenêtre de crop entre le frame entier
#        (zoom=0, "resize pur") et le crop serré au ratio cible (zoom=1,
#        comportement historique) — le centre reste piloté par
#        `roi_positions` comme avant.
#      - Nouvelle `roi_rect_for_zoom` : même formule pour une frame unique,
#        utilisée pour matérialiser/redimensionner le rectangle affiché côté
#        UI sans dupliquer la logique.
#      - Nouvelle `interpolate_roi_keyframes` : interpolation linéaire d'une
#        position ROI dans le temps à partir d'une liste de (timestamp, roi)
#        — permet de faire varier le cadrage manuel au cours de la vidéo
#        sans tracker OpenCV (l'utilisateur positionne le cadre à plusieurs
#        instants, l'app mémorise et interpole entre ces points).
#
# v2 — 2026-07-19 — safe-modify — `_create_tracker` : ajout de TrackerMIL en
#      dernier repli après CSRT/KCF. Découvert en testant réellement le
#      pipeline après `pip install opencv-python` (paquet déclaré dans les
#      dépendances) : sur la version installée (opencv-python 5.0.0),
#      `dir(cv2)` ne contient NI TrackerCSRT_create NI TrackerKCF_create (ni
#      en accès direct, ni via cv2.legacy qui n'existe pas non plus dans
#      cette build) — ces 2 trackers nécessitent en réalité
#      opencv-contrib-python depuis OpenCV 4.5.1+, jamais installé par un
#      simple `pip install opencv-python`. Sans ce fix, `_create_tracker`
#      retournait systématiquement None sur une install standard et le
#      tracking ROI tombait toujours en fallback fixe, jamais testé/atteint
#      en pratique. TrackerMIL_create est en revanche bien présent dans le
#      paquet de base (vérifié dans cette même build) — moins précis/plus
#      lent que CSRT/KCF mais réellement disponible avec la seule dépendance
#      documentée.
#
# v1 — 2026-07-19 — safe-modify — Création du module, nouvel onglet VIDEO
#      (dmd_converter.py). Isole toute la logique vidéo/tracking/crop (aucune
#      dépendance Tkinter) hors du monolithe dmd_converter.py, suivant le même
#      pattern de démonolithisation progressive que dmd_engine.py/
#      dmd_manual_effects.py/dmd_pipeline_quality.py. Réutilise
#      DMDEngine.adaptive_resize/optimize_for_dmd/cleanup_dmd_frame/
#      detect_palette (dmd_engine.py) par import direct — aucune duplication
#      de logique de rendu DMD. Version de base (backup original conservé
#      dans _backups/dmd_video_engine_2026-07-19_*.bak).
# ============================================

"""
Moteur vidéo pour l'onglet VIDEO (dmd_converter.py) : extraction de frames,
tracking automatique d'une zone d'intérêt (ROI) avec fallback, calcul de la
fenêtre de crop par frame et réglage automatique simplifié de la qualité GIF.

Dépendance externe : opencv-contrib-python (module cv2, PAS opencv-python
— v67, nécessaire pour les trackers CSRT/KCF, bien plus robustes que
TrackerMIL). Import protégé — si absent, CV2_AVAILABLE=False et toute
méthode nécessitant réellement le décodage vidéo/tracking lève une
RuntimeError explicite (l'appelant Tkinter doit vérifier CV2_AVAILABLE
avant d'activer l'onglet, voir dmd_converter.py).
"""

from typing import Any, Dict, List, Optional, Tuple

import numpy as np
from PIL import Image

try:
    import cv2

    CV2_AVAILABLE = True
except ImportError:
    cv2 = None  # type: ignore[assignment]
    CV2_AVAILABLE = False

try:
    from .dmd_engine import DMDEngine
except ImportError:
    from dmd_engine import DMDEngine


def _require_cv2() -> None:
    if not CV2_AVAILABLE:
        raise RuntimeError(
            "opencv-contrib-python n'est pas installé "
            "(pip install opencv-contrib-python) — requis pour lire des "
            "fichiers vidéo."
        )


def _bgr_frame_to_pil(frame_bgr: "np.ndarray") -> Image.Image:
    """Convertit une frame OpenCV (BGR, np.ndarray) en image PIL RGB."""
    return Image.fromarray(cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB))


def _pil_to_bgr(img: Image.Image) -> "np.ndarray":
    """Convertit une image PIL RGB en frame OpenCV (BGR, np.ndarray), format
    attendu par les trackers cv2 (init/update)."""
    arr = np.array(img.convert("RGB"))
    return cv2.cvtColor(arr, cv2.COLOR_RGB2BGR)


class VideoEngine:
    """Moteur vidéo statique (aucun état Tkinter) pour l'onglet VIDEO."""

    # ------------------------------------------------------------------
    # Lecture / échantillonnage vidéo
    # ------------------------------------------------------------------

    @staticmethod
    def probe_video(path: str) -> Dict[str, Any]:
        """Ouvre la vidéo et retourne ses métadonnées de base.
        Lève RuntimeError si le fichier ne peut pas être ouvert (codec non
        supporté, fichier corrompu ou non-vidéo)."""
        _require_cv2()
        cap = cv2.VideoCapture(path)
        try:
            if not cap.isOpened():
                raise RuntimeError(
                    "Impossible de lire ce fichier vidéo (codec non supporté ?)"
                )
            fps = cap.get(cv2.CAP_PROP_FPS) or 0.0
            frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0)
            width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH) or 0)
            height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT) or 0)
            # Certains conteneurs/codecs ne donnent pas un frame_count fiable
            # (webcam, streams) — fallback prudent si fps connu mais pas la
            # durée, sinon 0 (bornera plus tard trim_end à 0 côté UI).
            duration = (frame_count / fps) if fps > 0 and frame_count > 0 else 0.0
            if fps <= 0:
                fps = 25.0  # valeur de repli raisonnable, rare en pratique
            return {
                "fps": float(fps),
                "frame_count": frame_count,
                "width": width,
                "height": height,
                "duration": float(duration),
            }
        finally:
            cap.release()

    @staticmethod
    def extract_reference_frame(path: str, t_seconds: float) -> Image.Image:
        """Extrait la frame à un instant donné (secondes), utilisée comme
        image de référence pour le dessin du rectangle ROI."""
        _require_cv2()
        cap = cv2.VideoCapture(path)
        try:
            if not cap.isOpened():
                raise RuntimeError(
                    "Impossible de lire ce fichier vidéo (codec non supporté ?)"
                )
            cap.set(cv2.CAP_PROP_POS_MSEC, max(0.0, t_seconds) * 1000.0)
            ok, frame = cap.read()
            if not ok:
                # Repli : première frame du fichier (fin de vidéo/seek imprécis
                # sur certains conteneurs).
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                ok, frame = cap.read()
            if not ok:
                raise RuntimeError("Impossible d'extraire une frame de cette vidéo")
            return _bgr_frame_to_pil(frame)
        finally:
            cap.release()

    @staticmethod
    def extract_thumbnails(path: str, count: int = 40) -> List[Image.Image]:
        """Extrait `count` vignettes réparties uniformément sur toute la durée
        de la vidéo (frise de trim)."""
        _require_cv2()
        meta = VideoEngine.probe_video(path)
        duration = meta["duration"]
        if duration <= 0:
            return [VideoEngine.extract_reference_frame(path, 0.0)]
        timestamps = [duration * i / max(1, count - 1) for i in range(count)]
        return VideoEngine.extract_frames_at(path, timestamps)

    @staticmethod
    def sample_frame_timestamps(
        trim_start: float, trim_end: float, native_fps: float, target_fps: float
    ) -> List[float]:
        """Sous-échantillonnage temporel : retourne exactement
        round(target_fps * (trim_end - trim_start)) timestamps régulièrement
        espacés dans [trim_start, trim_end], indépendamment du fps natif de la
        vidéo source (une frame vidéo peut être piochée plusieurs fois si le
        fps cible dépasse le fps natif)."""
        span = max(0.0, trim_end - trim_start)
        n_frames = max(1, round(target_fps * span))
        if n_frames == 1:
            return [trim_start]
        step = span / n_frames
        return [trim_start + step * i for i in range(n_frames)]

    @staticmethod
    def extract_frames_at(path: str, timestamps: List[float]) -> List[Image.Image]:
        """Extrait une frame par timestamp (secondes), dans l'ordre. Seek
        direct par timestamp (CAP_PROP_POS_MSEC) — suffisant pour ce cas
        d'usage (quelques dizaines de frames), pas d'optimisation par lecture
        séquentielle nécessaire."""
        _require_cv2()
        cap = cv2.VideoCapture(path)
        try:
            if not cap.isOpened():
                raise RuntimeError(
                    "Impossible de lire ce fichier vidéo (codec non supporté ?)"
                )
            frames: List[Image.Image] = []
            last_frame: Optional[Image.Image] = None
            for t in timestamps:
                cap.set(cv2.CAP_PROP_POS_MSEC, max(0.0, t) * 1000.0)
                ok, frame = cap.read()
                if ok:
                    last_frame = _bgr_frame_to_pil(frame)
                    frames.append(last_frame)
                elif last_frame is not None:
                    # Fin de flux/seek imprécis en tout dernier timestamp :
                    # répète la dernière frame valide plutôt que planter tout
                    # le pipeline pour 1 frame manquante en bout de trim.
                    frames.append(last_frame)
                else:
                    raise RuntimeError("Impossible d'extraire une frame de cette vidéo")
            return frames
        finally:
            cap.release()

    # ------------------------------------------------------------------
    # Lecture continue (lecteur intégré de l'onglet VIDEO) — contrairement
    # aux méthodes ci-dessus qui ouvrent/referment un VideoCapture par appel,
    # celles-ci gardent un VideoCapture OUVERT entre appels successifs pour
    # une lecture en boucle fluide sans réouvrir le fichier à chaque frame.
    # L'appelant (dmd_converter.py) ne manipule l'objet retourné par
    # `open_capture` que via ces fonctions — aucun import cv2 direct côté UI,
    # cohérent avec le reste de l'architecture de ce module.
    # ------------------------------------------------------------------

    @staticmethod
    def open_capture(path: str):
        """Ouvre un VideoCapture pour lecture continue. L'appelant DOIT le
        libérer via `release_capture` (nouveau chargement de vidéo, ou
        fermeture de l'app) pour ne pas fuir le descripteur de fichier."""
        _require_cv2()
        cap = cv2.VideoCapture(path)
        if not cap.isOpened():
            cap.release()
            raise RuntimeError("Impossible de lire ce fichier vidéo (codec non supporté ?)")
        return cap

    @staticmethod
    def read_capture_frame(cap) -> Optional[Image.Image]:
        """Lit la frame suivante. À la fin du flux, boucle automatiquement
        (seek à 0 puis relit) — c'est le comportement de lecture en boucle
        demandé pour le lecteur intégré. Retourne None seulement si la
        lecture échoue même après la tentative de bouclage (flux invalide)."""
        ok, frame = cap.read()
        if not ok:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            ok, frame = cap.read()
            if not ok:
                return None
        return _bgr_frame_to_pil(frame)

    @staticmethod
    def seek_capture(cap, ms: float) -> None:
        cap.set(cv2.CAP_PROP_POS_MSEC, max(0.0, ms))

    @staticmethod
    def capture_position_ms(cap) -> float:
        return float(cap.get(cv2.CAP_PROP_POS_MSEC))

    @staticmethod
    def release_capture(cap) -> None:
        if cap is not None:
            cap.release()

    # ------------------------------------------------------------------
    # Tracking ROI + fenêtre de crop
    # ------------------------------------------------------------------

    @staticmethod
    def _create_tracker():
        """Essaie CSRT (précis, plus robuste) puis KCF (rapide, moins
        précis), aux 2 emplacements API connus selon la version d'OpenCV
        installée. Dépendance du projet DÉSORMAIS opencv-contrib-python
        (v67, PAS opencv-python — décision utilisateur suite à un bug réel
        de décrochage du tracker en cours de vidéo, diagnostiqué comme une
        limitation de TrackerMIL) : CSRT/KCF y sont bien présents et sont
        maintenant le chemin normal, pas un cas hypothétique. TrackerMIL
        reste en DERNIER repli pur défense en profondeur, pour le cas où
        l'environnement d'exécution aurait malgré tout un simple
        opencv-python (dépendance absente d'un `pip install opencv-python`
        depuis OpenCV 4.5.1+, `dir(cv2)` n'y expose que
        Tracker{MIL,DaSiamRPN,Nano,Vit}) — nettement moins robuste que
        CSRT/KCF (décrochages fréquents en cours de suivi, voir
        dmd_converter.py v67), à éviter si possible. Retourne None si
        vraiment aucun tracker n'est disponible — l'appelant se rabat alors
        sur un ROI fixe."""
        if not CV2_AVAILABLE:
            return None
        for factory_name in ("TrackerCSRT_create", "TrackerKCF_create", "TrackerMIL_create"):
            for ns in (cv2, getattr(cv2, "legacy", None)):
                if ns is not None and hasattr(ns, factory_name):
                    try:
                        return getattr(ns, factory_name)()
                    except Exception:
                        continue
        return None

    @staticmethod
    def _is_plausible_box(
        box: Tuple[float, float, float, float], frame_size: Tuple[int, int]
    ) -> bool:
        """Rejette une boîte de tracking dégénérée (taille qui explose/
        s'effondre, ou sort presque entièrement du cadre) — un tracker
        CSRT/KCF peut renvoyer ok=True avec une boîte aberrante juste avant de
        vraiment décrocher."""
        x, y, w, h = box
        fw, fh = frame_size
        if w <= 2 or h <= 2 or w > fw * 1.5 or h > fh * 1.5:
            return False
        cx, cy = x + w / 2, y + h / 2
        return -w < cx < fw + w and -h < cy < fh + h

    @staticmethod
    def _lerp_roi_towards_center(
        box: Tuple[int, int, int, int], frame_size: Tuple[int, int], step: float = 0.05
    ) -> Tuple[int, int, int, int]:
        """Relâche progressivement une boîte ROI vers le centre du frame
        (utilisé après un décrochage durable du tracker, pour ne pas rester
        bloqué indéfiniment sur une position obsolète après une coupe de
        plan/occlusion totale)."""
        x, y, w, h = box
        fw, fh = frame_size
        target_x, target_y = (fw - w) / 2, (fh - h) / 2
        new_x = x + (target_x - x) * step
        new_y = y + (target_y - y) * step
        return int(new_x), int(new_y), w, h

    @staticmethod
    def _sanitize_initial_roi(
        box: Tuple[int, int, int, int], frame_size: Tuple[int, int]
    ) -> Optional[Tuple[int, int, int, int]]:
        """Valide/clampe la boîte ROI initiale AVANT de l'utiliser pour
        amorcer un tracker OpenCV — une boîte dégénérée ou entièrement hors
        cadre fait planter `tracker.init()` avec une AssertionError C++ non
        rattrapable proprement par un simple try/except (ex. TrackerMIL :
        "posSamples.empty()", crash réel observé une fois les points de
        tracking devenus librement déplaçables dans le temps via la refonte
        multi-segments — un point posé/déplacé près du bord peut produire
        une boîte partiellement ou totalement hors cadre). Retourne None si
        la boîte reste inexploitable même après clamp (l'appelant renonce
        alors au tracking réel pour ce segment plutôt que de planter)."""
        x, y, w, h = box
        fw, fh = frame_size
        x0 = max(0, min(fw, x))
        y0 = max(0, min(fh, y))
        x1 = max(0, min(fw, x + w))
        y1 = max(0, min(fh, y + h))
        cw, ch = x1 - x0, y1 - y0
        if cw < 4 or ch < 4:
            return None
        return (x0, y0, cw, ch)

    @staticmethod
    def track_roi(
        frames: List[Image.Image], initial_roi: Tuple[int, int, int, int]
    ) -> List[Tuple[int, int, int, int]]:
        """Suit `initial_roi` (dessiné par l'utilisateur sur frames[0]) sur
        toute la séquence de frames source, via un tracker OpenCV (CSRT puis
        KCF en repli). Retourne une position (x, y, w, h) par frame (même
        longueur que `frames`).

        - Échec isolé du tracker (ou boîte implausible) : on garde la
          DERNIÈRE position connue, pas de saut ni d'interpolation
          immédiate — préserve la continuité visuelle.
        - Échec DURABLE (>= MAX_CONSECUTIVE_FAILURES d'affilée) : relâchement
          progressif de la position vers le centre du frame, pour ne pas
          rester bloqué indéfiniment sur une zone qui ne correspond plus au
          sujet.
        - Si cv2 absent, une seule frame, ou aucun tracker disponible :
          fallback trivial, ROI fixe sur toute la séquence.
        """
        if not frames:
            return []
        positions: List[Tuple[int, int, int, int]] = [tuple(initial_roi)]  # type: ignore[list-item]

        if not CV2_AVAILABLE or len(frames) <= 1:
            return positions * len(frames)

        tracker = VideoEngine._create_tracker()
        if tracker is None:
            return positions * len(frames)

        safe_initial = VideoEngine._sanitize_initial_roi(initial_roi, frames[0].size)
        if safe_initial is None:
            return positions * len(frames)
        try:
            tracker.init(_pil_to_bgr(frames[0]), tuple(int(v) for v in safe_initial))
        except Exception:
            # Défense en profondeur : même une boîte a priori valide peut
            # faire planter certains trackers OpenCV pour d'autres raisons
            # (image invalide, etc.) — repli sur le comportement "aucun
            # tracker disponible" plutôt qu'un crash.
            return positions * len(frames)

        last_good = tuple(int(v) for v in safe_initial)
        consecutive_failures = 0
        MAX_CONSECUTIVE_FAILURES = 5

        for frame in frames[1:]:
            if consecutive_failures < MAX_CONSECUTIVE_FAILURES:
                ok, box = tracker.update(_pil_to_bgr(frame))
            else:
                ok, box = False, None

            if ok and VideoEngine._is_plausible_box(box, frame.size):
                last_good = tuple(int(v) for v in box)
                consecutive_failures = 0
            else:
                consecutive_failures += 1
                if consecutive_failures >= MAX_CONSECUTIVE_FAILURES:
                    last_good = VideoEngine._lerp_roi_towards_center(
                        last_good, frame.size, step=0.05
                    )
            positions.append(last_good)

        return positions

    @staticmethod
    def compute_crop_windows(
        frame_sizes: List[Tuple[int, int]],
        roi_positions: List[Tuple[int, int, int, int]],
        canvas_w: int = 128,
        canvas_h: int = 32,
        smoothing: float = 0.25,
        zoom: float | List[float] = 0.0,
    ) -> List[Tuple[int, int, int, int]]:
        """Calcule, pour chaque frame, une fenêtre de crop (x0, y0, w, h)
        centrée sur la position ROI (trackée ou interpolée depuis des
        keyframes manuelles), avec lissage EMA du CENTRE (pas de la boîte
        entière) pour absorber le tremblement frame-à-frame du tracker sans
        introduire de lag perceptible (smoothing=0.0 pour désactiver — utile
        pour une position déjà déterministe comme une interpolation de
        keyframes, où un lissage ajouterait un retard non voulu par rapport
        à l'instant exact choisi par l'utilisateur). La fenêtre est toujours
        clampée pour ne jamais sortir du frame source.

        `zoom` (-1.0..+1.0, demande explicite "-100% = resize seul, 0 =
        crop serré, +100% zoom") contrôle le compromis crop/redimensionnement
        ET le zoom numérique au-delà du crop serré (curseur "Zoom cadrage"
        côté UI) : à zoom=-1.0, la fenêtre couvre le frame ENTIER (aucun
        crop, résultat identique à un redimensionnement classique une fois
        passé par adaptive_resize en mode "fit") ; à zoom=0.0, la fenêtre
        est le plus petit rectangle au ratio canvas_w:canvas_h qui tient
        dans le frame (crop serré, comportement historique de l'ancien
        zoom=1.0) ; à zoom=+1.0, la fenêtre est PLUS PETITE que le crop
        serré (zoom numérique — voir `_zoom_crop_size`, sujet agrandi) ;
        valeurs intermédiaires : interpolation linéaire de la taille de
        fenêtre entre les 2 segments (-1..0 et 0..+1), toujours centrée sur
        le ROI. Ignore silencieusement les tailles (w, h) de
        `roi_positions` — seul leur CENTRE compte, la taille de fenêtre
        réelle est toujours dérivée de `zoom` (comportement inchangé depuis
        la version d'origine, qui ignorait déjà w/h de la même façon).

        `zoom` accepte aussi une LISTE (un zoom par frame, même longueur que
        `roi_positions`) pour un zoom qui varie dans le temps (voir
        `interpolate_zoom_keyframes`) — rétrocompatible : un float unique
        continue de s'appliquer uniformément à toutes les frames comme
        avant."""
        if not roi_positions:
            return []
        if isinstance(zoom, (list, tuple)):
            zooms = [max(-1.0, min(1.0, z)) for z in zoom]
        else:
            zooms = [max(-1.0, min(1.0, zoom))] * len(roi_positions)
        centers = [(x + w / 2, y + h / 2) for (x, y, w, h) in roi_positions]
        smoothed = [centers[0]]
        for cx, cy in centers[1:]:
            px, py = smoothed[-1]
            smoothed.append(
                (smoothing * px + (1 - smoothing) * cx, smoothing * py + (1 - smoothing) * cy)
            )

        windows: List[Tuple[int, int, int, int]] = []
        target_ratio = canvas_w / canvas_h
        for (fw, fh), (cx, cy), z in zip(frame_sizes, smoothed, zooms):
            crop_w, crop_h = VideoEngine._zoom_crop_size(fw, fh, z, target_ratio)
            x0 = max(0.0, min(fw - crop_w, cx - crop_w / 2))
            y0 = max(0.0, min(fh - crop_h, cy - crop_h / 2))
            windows.append((int(x0), int(y0), int(crop_w), int(crop_h)))
        return windows

    @staticmethod
    def _zoom_crop_size(
        fw: float, fh: float, zoom: float, target_ratio: float,
        min_zoom_fraction: float = 0.35,
    ) -> Tuple[float, float]:
        """Calcule la taille (crop_w, crop_h) de la fenêtre de crop pour un
        niveau de `zoom` (-1.0..+1.0) donné — logique PARTAGÉE par
        `compute_crop_windows` et `roi_rect_for_zoom`, en 2 segments
        d'interpolation linéaire de part et d'autre de zoom=0 (demande
        explicite "-100% = resize seul, 0 = crop serré, +100% zoom") :

        - `tight_w/tight_h` = le plus petit rectangle au ratio
          `target_ratio` qui tient dans le frame (crop serré, "zoom=0").
        - zoom ∈ [-1, 0] : interpole entre le frame ENTIER (zoom=-1, aucun
          crop) et `tight_w/tight_h` (zoom=0) — même formule que l'ancienne
          échelle 0..1, translatée de +1.
        - zoom ∈ [0, +1] : interpole entre `tight_w/tight_h` (zoom=0) et une
          fenêtre RÉTRÉCIE à `min_zoom_fraction` de sa taille (zoom=+1,
          zoom numérique — valeur par défaut 0.35, soit un facteur
          d'agrandissement d'environ 2.9× au maximum, choisie pour rester
          exploitable sans réduire la fenêtre à quelques pixels sur des
          zones déjà petites)."""
        tight_w = min(float(fw), fh * target_ratio)
        tight_h = tight_w / target_ratio
        if zoom <= 0.0:
            t = zoom + 1.0  # -1..0 -> 0..1
            crop_w = fw + (tight_w - fw) * t
            crop_h = fh + (tight_h - fh) * t
        else:
            zoomed_w = tight_w * min_zoom_fraction
            zoomed_h = tight_h * min_zoom_fraction
            crop_w = tight_w + (zoomed_w - tight_w) * zoom
            crop_h = tight_h + (zoomed_h - tight_h) * zoom
        return crop_w, crop_h

    @staticmethod
    def roi_rect_for_zoom(
        frame_size: Tuple[int, int],
        center: Tuple[float, float],
        zoom: float,
        canvas_w: int = 128,
        canvas_h: int = 32,
    ) -> Tuple[int, int, int, int]:
        """Calcule le rectangle ROI (x, y, w, h) centré sur `center` pour un
        niveau de zoom donné (-1.0..+1.0) — même formule que
        `compute_crop_windows` (voir `_zoom_crop_size`) mais pour une frame
        unique, utilisée pour matérialiser/redimensionner en direct le
        rectangle affiché dans l'UI (voir dmd_converter.py, onglet VIDEO)
        sans repasser par toute la logique multi-frames."""
        fw, fh = frame_size
        zoom = max(-1.0, min(1.0, zoom))
        target_ratio = canvas_w / canvas_h
        w, h = VideoEngine._zoom_crop_size(fw, fh, zoom, target_ratio)
        cx, cy = center
        x = max(0.0, min(fw - w, cx - w / 2))
        y = max(0.0, min(fh - h, cy - h / 2))
        return (int(x), int(y), int(w), int(h))

    @staticmethod
    def interpolate_zoom_keyframes(
        frame_timestamps: List[float],
        keyframes: List[Tuple[float, float]],
    ) -> List[float]:
        """Interpolation linéaire d'un zoom (scalaire) dans le temps, à
        partir d'une liste de (timestamp_secondes, zoom) — pas nécessairement
        triée. Même logique de clamp/interpolation que l'ancienne
        `interpolate_roi_keyframes` (retirée, remplacée par
        `compute_crop_windows_from_events` — voir plus bas), mais sur une
        valeur simple plutôt qu'un rectangle : constante égale à la
        première/dernière valeur avant/après leurs bornes, interpolée
        linéairement entre 2 keyframes consécutives sinon. Avec une seule
        keyframe, retourne cette valeur pour tous les timestamps."""
        if not keyframes:
            return []
        kf = sorted(keyframes, key=lambda k: k[0])
        values: List[float] = []
        for t in frame_timestamps:
            if t <= kf[0][0] or len(kf) == 1:
                values.append(kf[0][1])
                continue
            if t >= kf[-1][0]:
                values.append(kf[-1][1])
                continue
            for i in range(len(kf) - 1):
                t0, z0 = kf[i]
                t1, z1 = kf[i + 1]
                if t0 <= t <= t1:
                    frac = (t - t0) / (t1 - t0) if t1 > t0 else 0.0
                    values.append(z0 + (z1 - z0) * frac)
                    break
        return values

    @staticmethod
    def compute_crop_windows_from_events(
        frames: List[Image.Image],
        timestamps: List[float],
        events: List[Dict[str, Any]],
        zoom: float | List[float] = 0.0,
        canvas_w: int = 128,
        canvas_h: int = 32,
    ) -> List[Tuple[int, int, int, int]]:
        """Orchestrateur central du cadrage vidéo (remplace l'ancien choix
        binaire tracking-global / keyframes-manuelles-interpolées). Segmente
        `frames`/`timestamps` selon `events` (liste de {"t", "roi",
        "mode": "auto"|"manual"}, pas nécessairement triée), et calcule la
        fenêtre de crop finale par frame :

        - Segment "auto" : réamorce `track_roi` sur `roi` (seedé sur la
          première frame du segment), lissage EMA conservé (smoothing=0.25,
          absorbe le tremblement du tracker) — réutilise `track_roi` TEL
          QUEL, aucune modification de l'algorithme de suivi.
        - Segment "manual" : `roi` reste FIXE pour tout le segment (aucune
          interpolation vers le segment suivant), smoothing=0.0 — le
          changement de zone doit rester un cut net, pas un lissage de
          transition (demande explicite : "chaque nouvelle zone... arrête
          la précédente").
        - Avant le premier événement : traité comme faisant partie du
          premier segment (même position/mode que le premier événement,
          même convention de clamp que l'ancienne `interpolate_roi_keyframes`).
        - `events` ou `frames` vide : retourne [] (l'appelant gère le cas
          "pas de cadrage du tout", comme avant).

        Le lissage EMA de `compute_crop_windows` est cumulatif SUR LA
        SÉQUENCE QU'ON LUI DONNE — appliqué ici PAR SEGMENT (pas en une
        seule passe sur toute la vidéo), pour qu'un changement de zone
        manuelle reste un cut net plutôt que de "baver" sur quelques frames
        de transition à la frontière."""
        if not events or not frames:
            return []
        ev = sorted(events, key=lambda e: e["t"])
        zoom_list = zoom if isinstance(zoom, (list, tuple)) else [zoom] * len(frames)

        # Index du segment (dans `ev`) auquel appartient chaque frame : le
        # dernier événement dont t <= timestamp de la frame (ou le premier
        # événement si la frame est antérieure à tous les événements).
        segment_idx_per_frame: List[int] = []
        ev_i = 0
        for t in timestamps:
            while ev_i + 1 < len(ev) and ev[ev_i + 1]["t"] <= t:
                ev_i += 1
            segment_idx_per_frame.append(ev_i)

        windows: List[Tuple[int, int, int, int]] = []
        start = 0
        for i in range(1, len(frames) + 1):
            if i == len(frames) or segment_idx_per_frame[i] != segment_idx_per_frame[start]:
                seg_event = ev[segment_idx_per_frame[start]]
                seg_frames = frames[start:i]
                seg_sizes = [f.size for f in seg_frames]
                seg_zoom = zoom_list[start:i]
                if seg_event["mode"] == "auto":
                    roi_positions = VideoEngine.track_roi(seg_frames, seg_event["roi"])
                    seg_windows = VideoEngine.compute_crop_windows(
                        seg_sizes, roi_positions, canvas_w, canvas_h,
                        smoothing=0.25, zoom=seg_zoom,
                    )
                else:
                    roi_positions = [seg_event["roi"]] * len(seg_frames)
                    seg_windows = VideoEngine.compute_crop_windows(
                        seg_sizes, roi_positions, canvas_w, canvas_h,
                        smoothing=0.0, zoom=seg_zoom,
                    )
                windows.extend(seg_windows)
                start = i
        return windows

    # ------------------------------------------------------------------
    # Qualité automatique (moteur dédié, simplifié — voir plan : ne réutilise
    # PAS score_variant/le sweep multi-variantes de dmd_pipeline_quality.py)
    # ------------------------------------------------------------------

    @staticmethod
    def auto_quality_settings(sample_frames: List[Image.Image]) -> Dict[str, Any]:
        """Calcule un réglage contraste/saturation/luminosité/couleurs GIF en
        UNE passe sur quelques frames représentatives (pas de sweep
        multi-variantes scoré). Bornes calées sur les défauts déjà en usage
        dans DMDEngine.optimize_for_dmd (contrast=1.5, saturation=1.3,
        black_threshold=30) pour rester dans des plages déjà validées
        visuellement par l'utilisateur ailleurs dans l'app."""
        if not sample_frames:
            return {
                "contrast": 1.5,
                "saturation": 1.3,
                "brightness": 1.0,
                "black_threshold": 30,
                "color_count": 64,
            }

        mid = sample_frames[len(sample_frames) // 2]
        palette = DMDEngine.detect_palette(mid, max_colors=256)
        if len(palette) <= 8:
            color_count = 8
        elif len(palette) <= 16:
            color_count = 16
        else:
            color_count = 64

        # Mesure luminance/contraste/saturation moyens sur l'ensemble des
        # frames échantillon (numpy), ajustement proportionnel borné.
        luminances: List[float] = []
        stds: List[float] = []
        sats: List[float] = []
        for frame in sample_frames:
            rgb = np.array(frame.convert("RGB")).astype(np.float32)
            luma = 0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2]
            luminances.append(float(luma.mean()))
            stds.append(float(luma.std()))
            hsv = np.array(frame.convert("HSV")).astype(np.float32)
            sats.append(float(hsv[..., 1].mean()))

        mean_luma = float(np.mean(luminances))
        mean_std = float(np.mean(stds))
        mean_sat = float(np.mean(sats))

        def clamp(v: float, lo: float, hi: float) -> float:
            return max(lo, min(hi, v))

        # Luminance cible ~128 (milieu de plage) : image sombre -> éclaircir,
        # jusqu'à +30%.
        brightness = clamp(1.0 + (128.0 - mean_luma) / 256.0, 0.85, 1.3)

        # Écart-type de luminance cible ~60 (contraste "normal") : image plate
        # -> plus de contraste, jusqu'à 1.6 (défaut app 1.5).
        contrast = clamp(1.0 + (60.0 - mean_std) / 100.0, 1.0, 1.6)

        # Saturation moyenne HSV cible ~110/255 : image désaturée -> plus de
        # saturation, jusqu'à 1.4 (défaut app 1.3).
        saturation = clamp(1.0 + (110.0 - mean_sat) / 220.0, 1.0, 1.4)

        # Fond bruité proche du noir (beaucoup de pixels 10-40) -> seuil noir
        # légèrement relevé pour un fond plus propre en petit format.
        black_threshold = 30
        arr_mid = np.array(mid.convert("RGB")).astype(np.float32)
        near_black_ratio = float(
            np.mean((arr_mid.mean(axis=2) > 10) & (arr_mid.mean(axis=2) < 40))
        )
        if near_black_ratio > 0.15:
            black_threshold = 45

        return {
            "contrast": contrast,
            "saturation": saturation,
            "brightness": brightness,
            "black_threshold": black_threshold,
            "color_count": color_count,
        }
