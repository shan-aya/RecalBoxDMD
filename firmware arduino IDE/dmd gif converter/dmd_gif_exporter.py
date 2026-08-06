from __future__ import annotations

# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v2
#
# v2 — 2026-07-19 — safe-modify — Refactor : logique d'encodage GIF extraite
#      de `export_frames_to_gif` vers un helper interne `_encode_gif_bytes`
#      (encode en mémoire, `io.BytesIO`, retourne les octets) partagé par
#      `export_frames_to_gif` (écrit ces octets sur disque via
#      `Path.write_bytes`) et la nouvelle `estimate_gif_size` (retourne juste
#      `len(...)`, sans écrire sur disque) — demande utilisateur : estimation
#      du poids du GIF résultant dans l'onglet VIDEO, calculée avec les
#      MÊMES paramètres qu'un export réel (pas une heuristique). Comportement
#      externe de `export_frames_to_gif` inchangé (mêmes validations, mêmes
#      octets produits — vérifié par ré-exécution des tests du pipeline
#      VIDEO existants après le refactor).
# v1 — 2026-07-19 — safe-modify — Version de base (avant refactor v2).
# ============================================

import io
from pathlib import Path
from typing import Any, Sequence

from PIL import Image

# Pillow compatibility: ADAPTIVE may not exist in some stubs.
_ADAPTIVE_RESAMPLING = getattr(Image, "ADAPTIVE", None)
if _ADAPTIVE_RESAMPLING is None:
    _ADAPTIVE_RESAMPLING = Image.Resampling.LANCZOS

# Pylance: `palette=` attend un type "Palette". En pratique, Pillow accepte
# Image.ADAPTIVE (ou une valeur équivalente). On caste pour désactiver le faux positif.
_PALETTE_VALUE: Any = _ADAPTIVE_RESAMPLING


def _encode_gif_bytes(
    frames: Sequence[Image.Image],
    *,
    fps: int,
    color_count: int,
    loop_mode: str,
    loop_count: int,
    disposal: int = 2,
    optimize: bool = False,
) -> bytes:
    """Encode une séquence de frames en GIF (mêmes règles que
    export_frames_to_gif) et retourne les octets bruts, sans rien écrire sur
    disque — utilisé à la fois par export_frames_to_gif (écriture) et
    estimate_gif_size (estimation de poids avant export)."""
    if not frames:
        raise ValueError("_encode_gif_bytes: 'frames' est vide")

    if fps <= 0:
        raise ValueError("_encode_gif_bytes: 'fps' doit être > 0")

    if color_count <= 0:
        raise ValueError("_encode_gif_bytes: 'color_count' doit être > 0")

    # Conversion palette
    p_frames: list[Image.Image] = []
    for frame in frames:
        p_frames.append(
            frame.convert(
                "P",
                palette=_PALETTE_VALUE,
                colors=color_count,
                dither=Image.Dither.NONE,
            )
        )

    loop = 0 if str(loop_mode) == "infini" else int(loop_count)
    duration_ms = int(1000 / fps)

    buf = io.BytesIO()
    p_frames[0].save(
        buf,
        format="GIF",
        save_all=True,
        append_images=p_frames[1:],
        duration=duration_ms,
        loop=loop,
        disposal=disposal,
        optimize=optimize,
    )
    return buf.getvalue()


def export_frames_to_gif(
    frames: Sequence[Image.Image],
    output_path: str | Path,
    *,
    fps: int,
    color_count: int,
    loop_mode: str,
    loop_count: int,
    disposal: int = 2,
    optimize: bool = False,
) -> None:
    """
    Exporte une séquence de frames PIL (RGB/anything convertissable) en GIF.

    - Conversion en palette (mode "P") avec palette adaptative
    - Duration = 1000/fps
    - loop: 0 si loop_mode == "infini", sinon loop_count
    """
    data = _encode_gif_bytes(
        frames,
        fps=fps,
        color_count=color_count,
        loop_mode=loop_mode,
        loop_count=loop_count,
        disposal=disposal,
        optimize=optimize,
    )

    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)


def estimate_gif_size(
    frames: Sequence[Image.Image],
    *,
    fps: int,
    color_count: int,
    loop_mode: str,
    loop_count: int,
    disposal: int = 2,
    optimize: bool = False,
) -> int:
    """Encode les frames en mémoire avec les MÊMES paramètres qu'un export
    réel et retourne la taille exacte résultante en octets, sans écrire sur
    disque — poids exact pour ces réglages, pas une heuristique."""
    return len(
        _encode_gif_bytes(
            frames,
            fps=fps,
            color_count=color_count,
            loop_mode=loop_mode,
            loop_count=loop_count,
            disposal=disposal,
            optimize=optimize,
        )
    )
