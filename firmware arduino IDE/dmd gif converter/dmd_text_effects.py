from __future__ import annotations

import math
import random
from typing import Any

import numpy as np
from PIL import Image


class TextEffects:
    """Effets visuels avancés pour texte."""

    @staticmethod
    def effect_3d(
        draw: Any,
        text: str,
        pos: tuple[int, int],
        font: Any,
        color: tuple[int, int, int],
        bg_color: Any,
    ) -> None:
        """Effet 3D avec ombre décalée."""
        x, y = pos
        depth = 3

        for i in range(depth, 0, -1):
            shadow_color = tuple(max(0, c - i * 30) for c in color)
            draw.text((x + i, y + i), text, fill=shadow_color, font=font)

        draw.text(pos, text, fill=color, font=font)

    @staticmethod
    def effect_fire(
        draw: Any,
        text: str,
        pos: tuple[int, int],
        font: Any,
        base_color: tuple[int, int, int] = (255, 100, 0),
    ) -> None:
        """Effet flamme avec dégradé."""
        x, y = pos
        bbox = draw.textbbox(pos, text, font=font)
        height = bbox[3] - bbox[1]

        for offset in range(height):
            progress = offset / height if height else 0
            r = int(255 * (1 - progress * 0.3))
            g = int(100 + 155 * progress)
            b = int(progress * 50)
            color = (r, g, b)
            draw.text((x, y + offset), text, fill=color, font=font)

    @staticmethod
    def effect_snow(img: Image.Image, text_img: Image.Image) -> Image.Image:
        """Effet neige tombante."""
        arr = np.array(text_img)
        h, w = arr.shape[:2]

        for _ in range(50):
            x = random.randint(0, w - 1)
            y = random.randint(0, h - 1)
            arr[y, x] = [255, 255, 255]

        return Image.fromarray(arr)

    @staticmethod
    def effect_ice(draw: Any, text: str, pos: tuple[int, int], font: Any) -> None:
        """Effet glace cristalline."""
        x, y = pos

        for dx in [-2, -1, 0, 1, 2]:
            for dy in [-2, -1, 0, 1, 2]:
                if dx != 0 or dy != 0:
                    draw.text((x + dx, y + dy), text, fill=(100, 150, 255), font=font)

        draw.text(pos, text, fill=(200, 230, 255), font=font)

    @staticmethod
    def effect_metal(draw: Any, text: str, pos: tuple[int, int], font: Any) -> None:
        """Effet métal chromé."""
        x, y = pos
        bbox = draw.textbbox(pos, text, font=font)
        width = bbox[2] - bbox[0]

        for offset in range(width):
            progress = offset / width if width else 0
            gray = int(100 + 100 * abs(math.sin(progress * math.pi * 2)))
            color = (gray, gray, gray + 50)
            draw.line([(x + offset, y), (x + offset, y + 30)], fill=color, width=1)

        draw.text(pos, text, fill=(220, 220, 255), font=font)

    @staticmethod
    def effect_neon(
        draw: Any,
        text: str,
        pos: tuple[int, int],
        font: Any,
        color: tuple[int, int, int] = (0, 255, 255),
    ) -> None:
        """Effet néon lumineux."""
        x, y = pos

        for radius in range(5, 0, -1):
            alpha = 50 + radius * 20
            glow_color = tuple(min(255, c + alpha) for c in color)
            for dx in range(-radius, radius + 1):
                for dy in range(-radius, radius + 1):
                    if dx * dx + dy * dy <= radius * radius:
                        draw.text((x + dx, y + dy), text, fill=glow_color, font=font)

        draw.text(pos, text, fill=(255, 255, 255), font=font)

    @staticmethod
    def effect_graffiti(
        draw: Any,
        text: str,
        pos: tuple[int, int],
        font: Any,
        colors: list[tuple[int, int, int]] | None = None,
    ) -> None:
        """Effet graffiti multi-couleurs."""
        if colors is None:
            colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]

        x, y = pos

        for dx in range(-3, 4):
            for dy in range(-3, 4):
                if dx != 0 or dy != 0:
                    draw.text((x + dx, y + dy), text, fill=(0, 0, 0), font=font)

        for i, char in enumerate(text):
            c = colors[i % len(colors)]
            char_x = x + i * 10
            draw.text((char_x, y), char, fill=c, font=font)

    @staticmethod
    def effect_pixel_art(img: Image.Image) -> Image.Image:
        """Effet pixel art rétro."""
        w, h = img.size
        pixelated = img.resize((w // 4, h // 4), Image.Resampling.NEAREST)
        return pixelated.resize((w, h), Image.Resampling.NEAREST)

    @staticmethod
    def effect_rainbow(draw: Any, text: str, pos: tuple[int, int], font: Any) -> None:
        """Effet arc-en-ciel."""
        x, y = pos
        colors = [
            (255, 0, 0),
            (255, 127, 0),
            (255, 255, 0),
            (0, 255, 0),
            (0, 0, 255),
            (75, 0, 130),
            (148, 0, 211),
        ]

        offset_x = 0
        for i, char in enumerate(text):
            c = colors[i % len(colors)]
            draw.text((x + offset_x, y), char, fill=c, font=font)
            bbox = draw.textbbox((x + offset_x, y), char, font=font)
            offset_x += bbox[2] - bbox[0]

    @staticmethod
    def effect_matrix(draw: Any, text: str, pos: tuple[int, int], font: Any) -> None:
        """Effet Matrix (vert sur noir)."""
        x, y = pos

        for offset in range(5):
            alpha = 255 - offset * 50
            green = max(0, alpha)
            draw.text((x, y - offset * 2), text, fill=(0, green, 0), font=font)

        draw.text(pos, text, fill=(0, 255, 0), font=font)

    @staticmethod
    def effect_gradient(
        draw: Any,
        text: str,
        pos: tuple[int, int],
        font: Any,
        color1: tuple[int, int, int],
        color2: tuple[int, int, int],
    ) -> None:
        """Dégradé de couleur."""
        x, y = pos
        bbox = draw.textbbox(pos, text, font=font)
        width = bbox[2] - bbox[0] if bbox else 1

        for offset in range(width):
            progress = offset / width if width else 0
            r = int(color1[0] + (color2[0] - color1[0]) * progress)
            g = int(color1[1] + (color2[1] - color1[1]) * progress)
            b = int(color1[2] + (color2[2] - color1[2]) * progress)
            draw.line([(x + offset, y), (x + offset, y + 30)], fill=(r, g, b), width=1)
