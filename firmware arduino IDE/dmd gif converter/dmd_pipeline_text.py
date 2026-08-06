from __future__ import annotations

# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v2
#
# v2 — 2026-07-18 — safe-modify — render_text_image : y0 (position verticale du
#      texte) n'était pas centré, fixé en dur à 8px depuis toujours quelle que
#      soit la taille de police (8-48px réglable dans l'UI) — pour une police de
#      taille par défaut (20) ça pouvait paraître à peu près centré par coïncidence,
#      mais pas pour les autres tailles. Signalé par l'utilisateur ("la preview ne
#      semble pas centrée" dans TEXTSCROLL). Remplacé par un calcul via
#      `draw.textbbox` : centre verticalement la vraie boîte englobante du texte
#      (encre réelle, pas la ligne de base théorique de la police) dans les 32px
#      de hauteur du canvas DMD.
# v1 — version de base (sans changelog, module ajouté lors d'un refactor
#      antérieur — voir _backups/ pour l'historique complet du monolithe).
# ============================================

from typing import Optional, Tuple

from PIL import Image, ImageDraw

try:
    from .dmd_text_effects import TextEffects
except ImportError:
    from dmd_text_effects import TextEffects


def render_text_image(
    *,
    text: str,
    font,
    effect: str,
    color_effect: str,
    text_color: Tuple[int, int, int],
    text_bg_color: Tuple[int, int, int],
) -> Optional[Image.Image]:
    """
    Rend le texte en image (PIL) avec effets, sans Tkinter.

    - text: texte brut
    - font: PIL.ImageFont
    - effect: sélection effet (3d/fire/ice/metal/neon/...) + éventuellement snow/pixel_art en post
    - color_effect: variante de couleur pour l'effet "normal" (rainbow/matrix/gradient/...)
    """
    if not text or text == "Votre texte ici...":
        return None

    # Créer image large pour le texte
    img = Image.new("RGB", (2000, 32), text_bg_color)
    draw = ImageDraw.Draw(img)

    # Centrage vertical réel : bbox d'encre de CE texte avec CETTE police (pas une
    # position fixe qui ne convient que par coïncidence à une taille de police).
    x0 = 10
    probe_bbox = draw.textbbox((0, 0), text, font=font)
    ink_height = probe_bbox[3] - probe_bbox[1]
    y0 = (32 - ink_height) // 2 - probe_bbox[1]

    # Appliquer effet texte
    if effect == "3d":
        TextEffects.effect_3d(draw, text, (x0, y0), font, text_color, text_bg_color)
    elif effect == "fire":
        TextEffects.effect_fire(draw, text, (x0, y0), font)
    elif effect == "ice":
        TextEffects.effect_ice(draw, text, (x0, y0), font)
    elif effect == "metal":
        TextEffects.effect_metal(draw, text, (x0, y0), font)
    elif effect == "neon":
        TextEffects.effect_neon(draw, text, (x0, y0), font, text_color)
    elif effect == "graffiti":
        TextEffects.effect_graffiti(draw, text, (x0, y0), font)
    elif effect == "outline":
        # Contour
        for dx in [-2, -1, 0, 1, 2]:
            for dy in [-2, -1, 0, 1, 2]:
                if dx != 0 or dy != 0:
                    draw.text((x0 + dx, y0 + dy), text, fill=(0, 0, 0), font=font)
        draw.text((x0, y0), text, fill=text_color, font=font)
    elif effect == "shadow":
        # Ombre
        draw.text((x0 + 2, y0 + 2), text, fill=(0, 0, 0), font=font)
        draw.text((x0, y0), text, fill=text_color, font=font)
    else:
        # Normal
        if color_effect == "rainbow":
            TextEffects.effect_rainbow(draw, text, (x0, y0), font)
        elif color_effect == "matrix":
            TextEffects.effect_matrix(draw, text, (x0, y0), font)
        elif color_effect == "fire":
            TextEffects.effect_fire(draw, text, (x0, y0), font)
        elif color_effect == "gradient":
            TextEffects.effect_gradient(
                draw, text, (x0, y0), font, text_color, (255, 255, 0)
            )
        else:
            draw.text((x0, y0), text, fill=text_color, font=font)

    # Effets post-traitement
    if effect == "snow":
        img = TextEffects.effect_snow(img, img)
    elif effect == "pixel_art":
        img = TextEffects.effect_pixel_art(img)

    # Recadrer au contenu
    bbox = img.getbbox()
    if bbox:
        img = img.crop((bbox[0] - 10, 0, bbox[2] + 10, 32))

    return img
