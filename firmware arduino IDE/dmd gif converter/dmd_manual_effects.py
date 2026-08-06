from __future__ import annotations

# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v6
#
# v6 — 2026-07-17 — safe-modify — Ajout de apply_over_frames : applique la
#      transformation par-frame d'un effet (couleur ou géométrique) à une séquence de
#      frames DÉJÀ RENDUES (typiquement un scroll ou un rendu pixel-perfect) au lieu
#      d'une image statique. Demandé par l'utilisateur pour que les propositions
#      artistiques du mode Auto/IA gardent le scrolling/pixel-perfect de la
#      proposition retenue au lieu de les remplacer par une animation d'effet
#      indépendante. Reprend la logique par-frame exacte de fade_effect/
#      color_shift_effect/glitch_effect/wave_effect/pulse_effect/zoom_effect/
#      spiral_effect (mêmes effets, source différente), avec des amplitudes de
#      zoom/rotation réduites pour ces 3 derniers car appliquées à un cadre 128×32
#      déjà composé plutôt qu'à un petit sprite (les plages d'origine, ex. zoom
#      0.5-1.0, videraient une grande partie de l'écran). N'affecte pas les méthodes
#      existantes utilisées par le mode manuel (aucune modifiée).
# v5 — 2026-07-15 — safe-modify — flash_effect : flash_interval = num_frames //
#      (flashes*2) pouvait valoir 0 quand flashes est grand par rapport à num_frames
#      (fps*duration), ce qui faisait dégénérer l'effet en écran blanc fixe pendant
#      toute l'animation (is_flash=True systématique via le fallback), sans jamais
#      montrer l'image. Plancher à 1 ajouté : dans ce cas l'effet alterne au rythme le
#      plus rapide possible (1 frame sur 2) au lieu de ne plus rien montrer.
# v4 — 2026-07-15 — safe-modify — color_shift_effect : correction d'un OverflowError
#      (numpy récent refuse `uint8_array % 256` quand une addition dépasse la plage) en
#      castant en int avant l'opération modulo. L'effet plantait systématiquement avant
#      ce correctif.
# v3 — 2026-07-15 — safe-modify — Factorisation du bloc "canvas noir 128x32 + centrage
#      de l'image" dupliqué dans ~13 méthodes d'effets, dans un helper _centered_canvas.
#      Comportement visuel inchangé.
# v2 — 2026-07-15 — safe-modify — Vectorisation numpy de wave_effect : la double boucle
#      Python pixel-par-pixel (y × x) est remplacée par de l'indexation numpy vectorisée.
#      Comportement visuel inchangé, gain de performance.
# v1 — 2026-07-15 — safe-modify — Version de base (backup original conservé dans
#      _backups/dmd_manual_effects_2026-07-15_19-18-05.bak)
# ============================================

import math
import random
from typing import List, Tuple

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

try:
    from .dmd_engine import DMDEngine
except ImportError:
    from dmd_engine import DMDEngine


class ManualEffects:
    """Collection d'effets avancés pour l'édition manuelle."""

    @staticmethod
    def _centered_canvas(
        content: Image.Image,
        bg_color: Tuple[int, int, int] = (0, 0, 0),
        extra_offset: Tuple[int, int] = (0, 0),
    ) -> Image.Image:
        """Crée un canvas DMD 128x32 avec `content` collé au centre (+ décalage optionnel)."""
        canvas = Image.new("RGB", (128, 32), bg_color)
        w, h = content.size
        x = (128 - w) // 2 + extra_offset[0]
        y = (32 - h) // 2 + extra_offset[1]
        canvas.paste(content, (x, y))
        return canvas

    @staticmethod
    def apply_over_frames(
        effect_name: str,
        base_frames: List[Image.Image],
        duration: float = 2.0,
        fps: int = 10,
    ) -> List[Image.Image]:
        """Applique la transformation par-frame d'un effet à une séquence de frames
        déjà rendues (ex. scroll ou pixel-perfect) au lieu d'une image statique —
        chaque frame de sortie combine la position de `base_frames[i % len(base_frames)]`
        avec la transformation de l'effet à l'instant i. `duration`/`fps` pilotent
        uniquement le cycle de l'effet (peuvent différer des paramètres globaux pour
        obtenir une meilleure animation), pas la longueur du scroll de base."""
        num_frames = max(1, int(fps * duration))
        frames: List[Image.Image] = []

        for i in range(num_frames):
            progress = i / num_frames
            src = base_frames[i % len(base_frames)]

            if effect_name == "fade_effect":
                canvas = ImageEnhance.Brightness(src).enhance(progress).convert("RGB")

            elif effect_name == "color_shift_effect":
                angle = progress * 360
                hsv = src.convert("HSV")
                h, s, v = hsv.split()
                h_arr = (np.array(h).astype(int) + int(angle)) % 256
                h = Image.fromarray(h_arr.astype("uint8"))
                canvas = Image.merge("HSV", (h, s, v)).convert("RGB")

            elif effect_name == "glitch_effect":
                if random.random() < 0.3:
                    arr = np.array(src).copy()
                    shift = random.randint(5, 15)
                    arr[:, :, 0] = np.roll(arr[:, :, 0], shift, axis=1)
                    arr[:, :, 2] = np.roll(arr[:, :, 2], -shift, axis=1)
                    canvas = Image.fromarray(arr)
                else:
                    canvas = src.copy()

            elif effect_name == "wave_effect":
                arr = np.array(src)
                h, w = arr.shape[0], arr.shape[1]
                y_idx = np.arange(h)
                x_idx = np.arange(w)
                phase = progress * 2 * math.pi
                offset = (5 * np.sin(phase + y_idx * 0.5)).astype(int)
                new_x = (x_idx[None, :] + offset[:, None]) % w
                canvas = Image.fromarray(arr[y_idx[:, None], new_x])

            elif effect_name == "pulse_effect":
                # Amplitude réduite vs pulse_effect (mode manuel, sprite) : ici la
                # source est déjà un cadre 128x32 complet, un zoom trop fort viderait
                # l'écran au lieu de faire "pulser" le contenu.
                scale = 0.9 + 0.1 * abs(math.sin(progress * math.pi * 2))
                w, h = src.size
                new_w, new_h = max(1, int(w * scale)), max(1, int(h * scale))
                scaled = src.resize((new_w, new_h), Image.Resampling.LANCZOS)
                canvas = ManualEffects._centered_canvas(scaled)

            elif effect_name == "zoom_effect":
                scale = 0.9 + 0.1 * progress
                w, h = src.size
                new_w, new_h = max(1, int(w * scale)), max(1, int(h * scale))
                scaled = src.resize((new_w, new_h), Image.Resampling.LANCZOS)
                canvas = ManualEffects._centered_canvas(scaled)

            elif effect_name == "spiral_effect":
                angle = progress * 360 * 2
                scale = 0.9 + 0.1 * progress
                w, h = src.size
                new_w, new_h = max(1, int(w * scale)), max(1, int(h * scale))
                scaled = src.resize((new_w, new_h), Image.Resampling.LANCZOS)
                rotated = scaled.rotate(angle, expand=True, fillcolor=(0, 0, 0))
                canvas = ManualEffects._centered_canvas(rotated)

            else:
                canvas = src.copy()

            frames.append(canvas)

        return frames

    @staticmethod
    def scroll_effect(
        img: Image.Image,
        direction: str = "horizontal",
        speed: int = 2,
        duration: float = 2.0,
        fps: int = 10,
    ) -> List[Image.Image]:
        """Scroll classique (liste de frames)."""
        return DMDEngine.create_animation_frames(
            img,
            {
                "direction": direction,
                "scroll_speed": speed,
                "duration": duration,
                "fps": fps,
            },
            bg_color=(0, 0, 0),
        )[0]

    @staticmethod
    def fade_effect(
        img: Image.Image,
        duration: float = 2.0,
        fps: int = 10,
        fade_in: bool = True,
    ) -> List[Image.Image]:
        """Fade in/out."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            alpha = i / num_frames if fade_in else 1 - (i / num_frames)
            frame = img.copy()

            enhancer = ImageEnhance.Brightness(frame)
            frame = enhancer.enhance(alpha)

            canvas = ManualEffects._centered_canvas(frame)
            frames.append(canvas)

        return frames

    @staticmethod
    def zoom_effect(
        img: Image.Image,
        duration: float = 2.0,
        fps: int = 10,
        zoom_in: bool = True,
    ) -> List[Image.Image]:
        """Zoom in/out."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            progress = i / num_frames
            scale = 0.5 + progress * 0.5 if zoom_in else 1.0 - progress * 0.5

            w, h = img.size
            new_w, new_h = int(w * scale), int(h * scale)
            scaled = img.resize((new_w, new_h), Image.Resampling.LANCZOS)

            canvas = ManualEffects._centered_canvas(scaled)
            frames.append(canvas)

        return frames

    @staticmethod
    def rotate_effect(
        img: Image.Image, duration: float = 2.0, fps: int = 10
    ) -> List[Image.Image]:
        """Rotation 360°."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            angle = (i / num_frames) * 360
            rotated = img.rotate(angle, expand=True, fillcolor=(0, 0, 0))

            canvas = ManualEffects._centered_canvas(rotated)
            frames.append(canvas)

        return frames

    @staticmethod
    def wave_effect(
        img: Image.Image,
        duration: float = 2.0,
        fps: int = 10,
        amplitude: int = 5,
    ) -> List[Image.Image]:
        """Effet vague."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)
        w, h = img.size
        arr = np.array(img)

        y_idx = np.arange(h)
        x_idx = np.arange(w)

        for i in range(num_frames):
            phase = (i / num_frames) * 2 * math.pi
            offset = (amplitude * np.sin(phase + y_idx * 0.5)).astype(int)

            new_x = (x_idx[None, :] + offset[:, None]) % w
            result = arr[y_idx[:, None], new_x]

            frame = Image.fromarray(result)
            canvas = ManualEffects._centered_canvas(frame)
            frames.append(canvas)

        return frames

    @staticmethod
    def bounce_effect(
        img: Image.Image, duration: float = 2.0, fps: int = 10
    ) -> List[Image.Image]:
        """Rebond."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            progress = i / num_frames
            y_offset = int(abs(math.sin(progress * math.pi * 2)) * 10)

            canvas = ManualEffects._centered_canvas(img, extra_offset=(0, -y_offset))
            frames.append(canvas)

        return frames

    @staticmethod
    def flash_effect(
        img: Image.Image,
        duration: float = 1.0,
        fps: int = 10,
        flashes: int = 3,
    ) -> List[Image.Image]:
        """Flash stroboscopique."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)
        flash_interval = (
            max(1, num_frames // (flashes * 2)) if flashes > 0 else num_frames
        )

        for i in range(num_frames):
            is_flash = (i // flash_interval) % 2 == 0

            if is_flash:
                canvas = Image.new("RGB", (128, 32), (255, 255, 255))
            else:
                canvas = ManualEffects._centered_canvas(img)
            frames.append(canvas)

        return frames

    @staticmethod
    def slide_effect(
        img: Image.Image,
        direction: str = "left",
        duration: float = 1.0,
        fps: int = 10,
    ) -> List[Image.Image]:
        """Slide depuis un côté."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)
        w, h = img.size

        for i in range(num_frames):
            progress = i / num_frames
            canvas = Image.new("RGB", (128, 32), (0, 0, 0))

            if direction == "left":
                x = int(-w + progress * (128 + w)) - 128 // 2
                y = (32 - h) // 2
            elif direction == "right":
                x = int(128 - progress * (128 + w)) + 128 // 2
                y = (32 - h) // 2
            elif direction == "top":
                x = (128 - w) // 2
                y = int(-h + progress * (32 + h)) - 32 // 2
            else:  # bottom
                x = (128 - w) // 2
                y = int(32 - progress * (32 + h)) + 32 // 2

            canvas.paste(img, (x, y))
            frames.append(canvas)

        return frames

    @staticmethod
    def spiral_effect(
        img: Image.Image, duration: float = 2.0, fps: int = 10
    ) -> List[Image.Image]:
        """Spirale."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            progress = i / num_frames
            angle = progress * 360 * 2
            scale = 0.3 + progress * 0.7

            w, h = img.size
            new_w, new_h = int(w * scale), int(h * scale)
            scaled = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
            rotated = scaled.rotate(angle, expand=True, fillcolor=(0, 0, 0))

            canvas = ManualEffects._centered_canvas(rotated)
            frames.append(canvas)

        return frames

    @staticmethod
    def shake_effect(
        img: Image.Image,
        duration: float = 1.0,
        fps: int = 10,
        intensity: int = 5,
    ) -> List[Image.Image]:
        """Tremblement."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            x_offset = random.randint(-intensity, intensity)
            y_offset = random.randint(-intensity, intensity)

            canvas = ManualEffects._centered_canvas(
                img, extra_offset=(x_offset, y_offset)
            )
            frames.append(canvas)

        return frames

    @staticmethod
    def pulse_effect(
        img: Image.Image, duration: float = 2.0, fps: int = 10
    ) -> List[Image.Image]:
        """Pulsation."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            progress = i / num_frames
            scale = 0.8 + 0.4 * abs(math.sin(progress * math.pi * 2))

            w, h = img.size
            new_w, new_h = int(w * scale), int(h * scale)
            scaled = img.resize((new_w, new_h), Image.Resampling.LANCZOS)

            canvas = ManualEffects._centered_canvas(scaled)
            frames.append(canvas)

        return frames

    @staticmethod
    def glitch_effect(
        img: Image.Image, duration: float = 1.0, fps: int = 10
    ) -> List[Image.Image]:
        """Effet glitch."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)
        arr = np.array(img)

        for i in range(num_frames):
            if random.random() < 0.3:
                glitched = arr.copy()

                shift = random.randint(5, 15)
                glitched[:, :, 0] = np.roll(glitched[:, :, 0], shift, axis=1)
                glitched[:, :, 2] = np.roll(glitched[:, :, 2], -shift, axis=1)

                frame = Image.fromarray(glitched)
            else:
                frame = img.copy()

            canvas = ManualEffects._centered_canvas(frame)
            frames.append(canvas)

        return frames

    @staticmethod
    def pixelate_effect(
        img: Image.Image, duration: float = 2.0, fps: int = 10
    ) -> List[Image.Image]:
        """Pixelisation progressive."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            progress = i / num_frames
            pixel_size = int(1 + progress * 10)

            w, h = img.size
            small = img.resize(
                (w // pixel_size, h // pixel_size), Image.Resampling.NEAREST
            )
            pixelated = small.resize((w, h), Image.Resampling.NEAREST)

            canvas = ManualEffects._centered_canvas(pixelated)
            frames.append(canvas)

        return frames

    @staticmethod
    def blur_transition_effect(
        img: Image.Image,
        duration: float = 2.0,
        fps: int = 10,
    ) -> List[Image.Image]:
        """Transition floue."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)

        for i in range(num_frames):
            progress = i / num_frames
            radius = int(progress * 10)

            blurred = (
                img.filter(ImageFilter.GaussianBlur(radius=radius))
                if radius > 0
                else img.copy()
            )

            canvas = ManualEffects._centered_canvas(blurred)
            frames.append(canvas)

        return frames

    @staticmethod
    def color_shift_effect(
        img: Image.Image,
        duration: float = 2.0,
        fps: int = 10,
    ) -> List[Image.Image]:
        """Décalage de couleurs."""
        frames: List[Image.Image] = []
        num_frames = int(fps * duration)
        arr = np.array(img)

        for i in range(num_frames):
            progress = i / num_frames
            angle = progress * 360

            shifted = arr.copy()
            hsv = Image.fromarray(shifted).convert("HSV")
            h, s, v = hsv.split()

            h_arr = np.array(h).astype(int)
            h_arr = (h_arr + int(angle)) % 256
            h = Image.fromarray(h_arr.astype("uint8"))

            shifted_img = Image.merge("HSV", (h, s, v)).convert("RGB")

            canvas = ManualEffects._centered_canvas(shifted_img)
            frames.append(canvas)

        return frames
