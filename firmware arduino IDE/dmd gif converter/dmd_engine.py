from __future__ import annotations

# ============================================
# safe-modify — Historique des modifications
# ============================================
# Version actuelle : v17
#
# v17 — 2026-07-20 — safe-modify — Demande explicite : "ajoute raw565 comme
#      format image supporté en entrée si pas déjà le cas" — confirmé absent
#      (aucune référence "565" nulle part dans le projet avant ce fix).
#      Nouveau `DMDEngine.load_image(path)` (remplace `Image.open(path)` à
#      tous les points d'entrée d'images utilisateur, voir dmd_converter.py
#      v77 et dmd_pipeline_quality.py) : formats PIL standard inchangés,
#      délègue simplement à `Image.open`. Extension `.raw565` : décodée par
#      la nouvelle `_load_raw565`, avec la MÊME convention EXACTE que le
#      projet firmware sœur RecalBox_DMD (`RecalBoxDMD_tool.py`,
#      `convert_png_to_raw565_only`, ET `RecalBox_DMD.ino`, constantes
#      `RAW565_W`/`RAW565_H`) — vérifiée dans ces 2 fichiers plutôt que
#      supposée : pas d'en-tête, 128×32 pixels fixes (panneau DMD, 8192
#      octets), ligne-majeur, `uint16` petit-boutiste par pixel (ESP32
#      nativement petit-boutiste, confirmé par `struct.pack("<H", ...)`
#      côté outil), empaquetage `((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3)`.
#      Décodage vectorisé numpy (réplication de bits 5/6→8 bits pour la
#      reconstruction RGB888, plus fidèle qu'un simple décalage qui
#      assombrirait l'image). `img.format = "RAW565"` assigné manuellement
#      après décodage (sinon `None`, `Image.open` le remplit normalement
#      pour les formats standard — affiché tel quel dans le panneau
#      "Informations Image" de dmd_converter.py). Taille de fichier
#      incorrecte (≠ 8192 octets) : `ValueError` explicite plutôt que de
#      deviner d'autres dimensions.
#
# v16 — 2026-07-19 — safe-modify — `render_led_style` : la simulation LED à
#      brightness=1.0 (100%) restait nettement plus SOMBRE que le rendu
#      classique (bloc plein), signalé par l'utilisateur. Mesuré : seulement
#      63% de la luminosité moyenne du rendu classique à 100%. Cause : la
#      formule linéaire symétrique de `bloom_delta`/`glow_radius_factor`/
#      `glow_blend` (v12) n'allait jamais assez loin — à brightness=1.0,
#      `effective_led_ratio` n'atteignait que 0.775 (plafonné à 0.95 de toute
#      façon), couvrant seulement ~47% de l'aire de chaque cellule (un cercle
#      inscrit dans un carré ne peut de toute façon jamais couvrir plus de
#      ~78.5% même à ratio=1.0) — très en-dessous de l'intention documentée
#      dans le docstring v12 ("les points commencent à se toucher/baver les
#      uns sur les autres à pleine puissance"), jamais vraiment atteinte en
#      pratique. Fix : formule ASYMÉTRIQUE — la moitié basse (brightness
#      0.0-0.5) reste STRICTEMENT INCHANGÉE (mêmes coefficients qu'avant,
#      aucune régression, vérifié pixel-luminosité identique) ; la moitié
#      haute (0.5-1.0) utilise des coefficients plus agressifs pour que
#      `effective_led_ratio` atteigne 1.1 (dépasse 1.0 — les points
#      chevauchent légèrement leurs voisins, réalisant enfin le "bavage"
#      décrit) et que le glow (rayon/mélange) compense le reste. Résultat
#      mesuré : brightness=1.0 atteint 100.8% de la luminosité du rendu
#      classique (contre 63% avant), progression cohérente entre 0.5 et 1.0
#      (60.4% à 0.75, 85.4% à 0.9).
#
# v15 — 2026-07-18 — safe-modify — `_pixel_perfect_dims`/`adaptive_resize` :
#      nouveau paramètre `direction` (optionnel), corrige un recadrage silencieux
#      du contenu sur les bords en mode "Mode DMD / Force pixel-perfect" + Fill.
#      Signalé par l'utilisateur sur "1stDivisionManager(Europe).png" : "le mode
#      fill semble crop l'image, il manque de l'image sur les côtés" (uniquement
#      en pixel-perfect). Cause : en fill NON pixel-perfect, l'échelle continue
#      `max(target_w/w, target_h/h)` garantit par construction qu'UN SEUL axe
#      déborde (l'autre tombe exactement sur la cible) — le rendu scroll cet axe
#      qui déborde et laisse l'autre (fixe, centré) exactement à la bonne taille,
#      sans recadrage. En pixel-perfect, l'arrondi à un facteur entier UNIQUE
#      (obligatoire pour un mapping pixel-LED exact) casse cette garantie dès que
#      le ratio du contenu s'écarte significativement de 128:32 — le facteur entier
#      qui satisfait tout juste la couverture du "petit" axe survalorise l'autre
#      axe, qui déborde LUI AUSSI. Le rendu ne scrollant qu'un seul axe (direction
#      choisie selon le ratio), l'axe fixe qui déborde est centré avec une
#      coordonnée négative — recadrage symétrique et définitif, jamais révélé par
#      le scroll. Exemple mesuré (1stDivisionManager, contenu recadré 449×284) :
#      facteur entier d=3 → 150×95 (déborde sur LES DEUX axes, 22px perdus sur les
#      côtés en direction="vertical"). Fix : si `direction` ("horizontal" ou
#      "vertical") est connue, le facteur entier est calculé UNIQUEMENT à partir
#      de l'axe FIXE (non scrollé), avec le même arrondi "sûr" que le mode fit
#      (jamais de dépassement sur cet axe précis) — l'axe qui scrolle hérite du
#      même facteur et déborde toujours (normal, c'est lui qui est censé scroller).
#      Sans `direction` connue (appelant qui ne la fournit pas), comportement
#      inchangé (ancien calcul combiné) — rétrocompatible.
#
# v14 — 2026-07-18 — safe-modify — nouvelle méthode `crop_to_visible_content`,
#      suite à un signalement utilisateur sur le fichier "4th&Inches(Europe).png" :
#      "le mode fill semble rendre une image qui pourrait tenir en resize sans
#      scroll". Cause racine : le fichier fait 400×175px mais son contenu visible
#      (bbox alpha non-transparente) n'occupe que 400×83px — 46px de marge
#      transparente en haut ET en bas (53% de l'image est vide). Aucune étape du
#      pipeline ne recadrait sur ce contenu réel avant de calculer les échelles
#      fit/fill, avec 2 conséquences cumulées : (1) le mode Resize/fit divisait par
#      la hauteur BRUTE (32/175=0.183) au lieu de la hauteur du contenu
#      (32/83=0.386), rendant le texte ~2× plus petit que nécessaire (flou/tassé,
#      confirmé visuellement malgré un score correct qui ne capture pas cette
#      perte de netteté) ; (2) le mode Fill, contraint par la largeur brute
#      (128/400=0.32, qui donne par coïncidence exactement 128px de large — aucun
#      scroll horizontal utile), calculait une hauteur de canvas de 175×0.32=56px
#      (> 32, scroll vertical déclenché) alors que le texte à cette échelle ne
#      fait que 83×0.32≈26.6px de haut — le scroll vertical ne parcourait donc que
#      du vide transparent, un simple recadrage centré statique aurait suffi.
#      `crop_to_visible_content(img, padding=2)` : si l'image a un canal alpha
#      avec transparence effective, recadre sur la bbox de CE SEUL canal (pas
#      RGBA combiné — évite de rater une transparence complète si des pixels
#      "invisibles" ont des valeurs RVB résiduelles non nulles) + une marge de
#      sécurité pour ne pas rogner un dégradé alpha en bord de forme. Sans
#      transparence effective, retourne l'image inchangée. Appliquée dans
#      `render_dmd_frame`/`score_variant` (dmd_pipeline_quality.py v17) et dans
#      `auto_analyze_and_preview` (dmd_converter.py v41) — PAS dans les
#      chargeurs MANUEL (`load_manual_image` etc.), qui doivent rester sans
#      garde-fou automatique par décision explicite antérieure de l'utilisateur
#      (voir mémoire projet, point 20). `show_original`/`update_image_info`
#      (affichage du panneau "Image Originale") ne sont pas non plus touchés —
#      l'utilisateur doit continuer à voir le fichier source tel quel.
#
# v13 — 2026-07-18 — safe-modify — cleanup_dmd_frame entièrement reconçue suite à
#      un retour utilisateur : le nettoyage dégradait visiblement les images (perte
#      de détails de texte/logo) même à power=0.3, alors que le score de qualité
#      augmentait parfois — signe d'un défaut de conception, pas d'une régression
#      (code identique depuis le tout premier commit, vérifié par diff backup).
#      Cause : l'ancienne version appliquait MedianFilter/ModeFilter à TOUS les
#      pixels sans jamais tester s'ils étaient réellement isolés (bruit) ou membres
#      d'un trait cohérent — disproportionnellement destructeur sur le canvas déjà
#      réduit à 128x32 (un trait de lettre de 1-2px y est tout le détail visible).
#      Nouvelle logique : `_remove_isolated_pixels` (nouvelle méthode) ne nettoie
#      un pixel que si TOUS ses 8 voisins directs en diffèrent au-delà d'un seuil —
#      un pixel de trait, ayant au moins 1 voisin proche le long du trait, n'est
#      jamais touché. `power` module le seuil (plus élevé = plus agressif) et le
#      nombre de passes (1 à power>=0.3, 2 à >=0.6, 3 à >=0.9), remplaçant l'ancien
#      empilement de filtres différents par types. Second fix associé (voir
#      dmd_pipeline_quality.py v16) : score_variant comparait le candidat à une
#      référence non rehaussée (biais de score qui récompensait le flou). Limite
#      connue et acceptée : un petit amas de 2-3 pixels de bruit adjacents peut
#      survivre (chaque pixel de l'amas a alors un voisin proche : l'autre pixel du
#      même amas) — à traiter par détection de composantes connexes seulement si
#      démontré nécessaire empiriquement.
#
# v12 — 2026-07-18 — safe-modify — render_led_style : `brightness` reconçu suite
#      à un retour utilisateur explicite — "ça change la couleur mais ça 'éclaire'
#      pas plus". La v11 ne faisait varier que la teinte (color_mult), inefficace
#      sur des couleurs déjà proches de leur maximum (rien à gagner à multiplier
#      un canal déjà à 255). Sur du matériel réel, une LED plus lumineuse ne
#      change pas de teinte, elle paraît PLUS GROSSE (bloom/halation) et son halo
#      s'étend davantage — c'est maintenant le mécanisme principal : le diamètre
#      effectif du point (`led_ratio` + décalage lié à `brightness`, jusqu'à ce
#      que les points se touchent à pleine puissance) et le rayon/mélange du
#      halo grossissent avec la luminosité ; la teinte ne varie plus qu'en
#      appoint mineur. brightness=0.5 reproduit toujours exactement le rendu
#      neutre précédent (aucune régression, vérifié).
# v11 — 2026-07-18 — safe-modify — render_led_style : nouveau paramètre
#      `brightness` (0.0-1.0, défaut 0.5), demandé explicitement pour simuler le
#      réglage de luminosité physique d'un panneau LED. Remplace les constantes
#      fixes de glow de la v10 (radius_factor=0.15, blend=0.25) par des formules
#      dérivées de `brightness`, calées pour reproduire EXACTEMENT ces valeurs à
#      brightness=0.5 (aucune régression du rendu par défaut) : plus lumineux =
#      couleurs poussées vers le blanc (`color_mult`) ET halo de bloom plus
#      marqué (une LED plus lumineuse "bave" davantage sur ses voisines,
#      comportement physique réel) ; moins lumineux = assombri et halo quasi
#      supprimé. Utilisé par le nouveau slider "Luminosité LED" (dmd_converter.py,
#      3 onglets) et par la fenêtre de zoom loupe (même version).
# v10 — 2026-07-18 — safe-modify — render_led_style : ajout d'un suréchantillonnage
#      interne (nouveau paramètre `supersample`, défaut 4) — le fix v9 (led_ratio
#      0.72→0.525) restait invisible en pratique, signalé par l'utilisateur ("rien
#      ne semble avoir changé, LED toujours collées"). Cause racine trouvée par
#      comparaison visuelle directe (captures zoomées pixel à pixel) : à scale=4
#      (taille réellement utilisée dans les 3 aperçus live de l'app), le rayon du
#      point est de l'ordre de 1px — `ImageDraw.ellipse` de PIL ne fait AUCUN
#      anti-crénelage, donc un cercle aussi petit se rasterise de façon
#      disproportionnée/blocage, rendant led_ratio quasi sans effet visuel quelle
#      que soit sa valeur (vérifié : 0.72 et 0.525 quasi indiscernables à scale=4).
#      Fix : dessiner les points à `scale*supersample` puis réduire au format
#      cible en LANCZOS (lisse la forme, le ratio devient enfin perceptible).
#      Glow également réduit (rayon 0.35→0.15, mélange 0.65/0.35→0.75/0.25) : à
#      l'ancien réglage, le flou comblait à lui seul l'espace entre points tout
#      juste rendu visible par le suréchantillonnage, annulant l'effet — même
#      méthode de vérification (comparaison visuelle directe avant/après).
#      Mesuré ~12ms/frame à scale=4/supersample=4, sous le budget 10fps.
# v9 — 2026-07-18 — safe-modify — render_led_style : led_ratio par défaut corrigé
#      de 0.72 à 0.525. Signalé par l'utilisateur (retour sur du matériel réel) :
#      les LED paraissaient beaucoup trop proches les unes des autres dans
#      l'aperçu par rapport à la réalité. Vérification des specs du panneau
#      utilisateur : HUB75 P4 (pas de 4 mm — confirmé par le panneau 256×128 mm
#      pour 64×32 points, 256/64 = 128/32 = 4 mm), LED type 2121 (convention de
#      nommage SMD = 2.1 mm × 2.1 mm). Ratio physique réel = taille LED / pas =
#      2.1 / 4.0 = 0.525, très inférieur à l'ancienne valeur 0.72 (choisie sans
#      référence matérielle à l'origine, v7) — d'où l'écart visuel signalé. 2
#      panneaux 64×32 côte à côte = 128×32 au total, cohérent avec la résolution
#      DMD déjà utilisée partout dans l'app (aucun changement de résolution
#      nécessaire, seul l'espacement visuel des points est corrigé).
# v8 — 2026-07-17 — safe-modify — optimize_for_dmd : protection anti-écrêtage des
#      hautes lumières. Bug réel signalé par l'utilisateur (logo "A.G.E." doré,
#      reflets métalliques) : le contraste (×1.5) + saturation (×1.3) + netteté
#      (×1.2) par défaut poussaient des pixels DÉJÀ clairs (mais pas blancs) de
#      l'original jusqu'au blanc pur (255,255,255), créant un halo disgracieux
#      absent de la source (1911 pixels blancs après optimize_for_dmd contre 18
#      juste après l'aplatissement de la transparence, sur l'image pleine
#      résolution — vérifié). Le seuil noir ne corrige que les pixels SOMBRES,
#      aucune protection n'existait côté clair. Ajout d'un plafond de gain
#      (MAX_GAIN=60) pour les pixels qui n'étaient PAS déjà proches du blanc avant
#      contraste/saturation/netteté — les vrais blancs voulus de l'image source ne
#      sont pas affectés, seul l'écrêtage artificiel est limité.
# v7 — 2026-07-17 — safe-modify — Nouvelle méthode statique render_led_style : simule
#      un rendu physique DMD/matrice de LED — chaque pixel de la frame 128×32 devient
#      un point rond (au lieu d'un simple carré agrandi), séparé de ses voisins par un
#      espace sombre (bezel), avec un léger flou de halo optionnel. Étape 1 (aperçu
#      statique à la demande) d'une demande utilisateur en 2 étapes ; l'intégration à
#      l'animation continue (étape 2) est laissée pour plus tard si le rendu convient,
#      le dessin de 4096 ellipses par frame en boucle Python n'étant pas forcément
#      assez rapide pour un affichage temps réel sans optimisation supplémentaire.
# v6 — 2026-07-17 — safe-modify — create_animation_frames : nouveau paramètre
#      sample_mid_scroll (défaut False, rétrocompatible). Bug réel signalé par
#      l'utilisateur : score_variant et les miniatures Propositions IA utilisaient
#      max_frames=1, qui renvoie la frame à offset=0 — pour un mode scroll (fill), ça
#      ne montre/ne score QUE les 128 premiers pixels de gauche de l'image redimen-
#      sionnée (souvent bien plus large), tronquant le logo et faussant le score de
#      lisibilité (comparé à une référence qui, elle, n'est pas tronquée au même
#      endroit). Avec sample_mid_scroll=True (utilisé uniquement avec max_frames=1),
#      la frame à offset=max_offset//2 (milieu du scroll) est calculée directement en
#      O(1) via le nouvel helper _render_scroll_frame_at_offset, sans boucler sur
#      toutes les frames intermédiaires (préserve le fix de perf v4). Extrait de
#      _scroll_axis_frames pour réutilisation (aucun changement de comportement pour
#      les appels existants qui ne passent pas ce paramètre).
# v5 — 2026-07-17 — safe-modify — create_animation_frames/_scroll_axis_frames :
#      scroll_speed peut désormais être < 1 (scroll plus lent que 1px/frame). Python
#      range() n'accepte pas de pas fractionnaire, donc pour scroll_speed<1 le pas
#      pixel reste 1 mais chaque frame est répétée round(1/scroll_speed) fois (frame
#      "tenue" plus longtemps = ralentit la perception du mouvement sans notion de
#      sous-pixel, qui n'aurait pas de sens sur une grille de LED physique). Pour
#      scroll_speed>=1, comportement inchangé (juste round() au lieu de int(), sans
#      incidence puisque l'UI ne produisait jusqu'ici que des entiers).
# v4 — 2026-07-17 — safe-modify — create_animation_frames/_scroll_axis_frames : ajout
#      du paramètre optionnel max_frames (défaut None, rétrocompatible) pour arrêter
#      la génération dès que ce nombre de frames est atteint. Bug de performance
#      découvert lors de l'analyse de lenteur du mode Auto/IA (7-15s par analyse) :
#      score_variant appelait create_animation_frames en mode fill/scroll juste pour
#      récupérer frames[0], régénérant TOUTE l'animation aller-retour (parfois 100+
#      frames avec Image.new+paste+filtre chacune) alors qu'une seule était utilisée.
#      Avec max_frames=1, seule la première frame est calculée. Comportement
#      strictement inchangé quand max_frames=None (défaut).
# v3 — 2026-07-15 — safe-modify — adaptive_resize : refonte de pixel_perfect. Avant ce
#      correctif, pixel_perfect se contentait de changer le filtre de rééchantillonnage
#      en NEAREST tout en gardant une échelle fractionnaire (scale = min/max(target/w,
#      target/h)) — un pixel source pouvait donc s'étaler de façon inégale sur 2-3 LED
#      selon la position dans l'image (rendu "moche" signalé par l'utilisateur). Le vrai
#      pixel-perfect impose maintenant un facteur d'échelle ENTIER : agrandissement par
#      un facteur k (NEAREST, chaque pixel source devient un bloc k×k de LED identiques)
#      ou réduction par un diviseur d (BOX, chaque LED = moyenne exacte d'un bloc d×d de
#      pixels source) si l'image dépasse la résolution du panneau. Arrondi vers le bas
#      (fit, ne doit pas déborder du cadre) ou vers le haut (fill, doit couvrir tout le
#      cadre), cohérent avec la sémantique fit/fill existante.
# v2 — 2026-07-15 — safe-modify — create_animation_frames : factorisation des branches
#      horizontal/vertical (logique de scroll aller-retour quasi identique, seul l'axe
#      change) dans un helper _scroll_axis_frames. Comportement inchangé.
# v1 — 2026-07-15 — safe-modify — Version de base (backup original conservé dans
#      _backups/dmd_engine_2026-07-15_20-04-12.bak)
# ============================================

import math
from pathlib import Path
from typing import Any, Dict, List, Tuple

import numpy as np
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter


class DMDEngine:
    """Moteur de rendu optimisé pour écrans DMD avec algorithmes adaptatifs."""

    # Dimensions fixes d'un fichier .raw565 (voir load_image/_load_raw565) —
    # mêmes constantes que TARGET_W/TARGET_H côté RecalBoxDMD_tool.py
    # (RecalBox_DMD/tools/, projet firmware sœur) : panneau DMD 128×32.
    RAW565_WIDTH = 128
    RAW565_HEIGHT = 32

    @staticmethod
    def load_image(path) -> Image.Image:
        """Charge une image source depuis n'importe quel format d'ENTRÉE
        supporté — remplace `Image.open(path)` à tous les points d'entrée
        d'images utilisateur (AUTO/MANUEL/multi-images morphing/pipeline de
        rendu). Formats standard PIL (PNG/JPG/BMP/GIF/...) : comportement
        strictement inchangé, délègue à `Image.open`. Extension `.raw565`
        (demande explicite "ajoute raw565 comme format image supporté en
        entrée") : décode le format RGB565 brut utilisé par le firmware
        RecalBox_DMD et son outil `RecalBoxDMD_tool.py`
        (`convert_png_to_raw565_only`, projet sœur) — même convention
        EXACTE, vérifiée dans ce projet plutôt que réinventée : extension
        `.raw565`, aucun en-tête, 128×32 pixels fixes (8192 octets),
        ligne-majeur, `uint16` PETIT-BOUTISTE par pixel, empaquetage
        `((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3)` (`struct.pack("<H", ...)` côté
        firmware/outil)."""
        path = Path(path)
        if path.suffix.lower() == ".raw565":
            return DMDEngine._load_raw565(path)
        return Image.open(path)

    @staticmethod
    def _load_raw565(path: Path) -> Image.Image:
        """Décode un fichier .raw565 (voir load_image) en image RGB PIL. Lève
        ValueError si la taille du fichier ne correspond pas exactement à
        128×32 pixels × 2 octets (8192 octets) — pas de tentative de deviner
        d'autres dimensions, un fichier .raw565 de taille différente indique
        presque certainement un fichier corrompu ou d'une autre origine que
        la convention RecalBox_DMD."""
        expected_bytes = DMDEngine.RAW565_WIDTH * DMDEngine.RAW565_HEIGHT * 2
        data = path.read_bytes()
        if len(data) != expected_bytes:
            raise ValueError(
                f"Fichier .raw565 invalide ({len(data)} octets, {expected_bytes} "
                f"attendus pour {DMDEngine.RAW565_WIDTH}x{DMDEngine.RAW565_HEIGHT})"
            )
        # Petit-boutiste ("<u2"), même convention que struct.pack("<H", ...)
        # côté RecalBoxDMD_tool.py (ESP32 nativement petit-boutiste).
        values = np.frombuffer(data, dtype="<u2").astype(np.uint32)
        r5 = (values >> 11) & 0x1F
        g6 = (values >> 5) & 0x3F
        b5 = values & 0x1F
        # Réplication de bits (5/6 bits -> 8 bits) : reconstruction standard
        # RGB565->RGB888, plus fidèle qu'un simple décalage qui assombrirait
        # systématiquement l'image décodée.
        r8 = ((r5 << 3) | (r5 >> 2)).astype(np.uint8)
        g8 = ((g6 << 2) | (g6 >> 4)).astype(np.uint8)
        b8 = ((b5 << 3) | (b5 >> 2)).astype(np.uint8)
        rgb = np.stack([r8, g8, b8], axis=-1).reshape(
            DMDEngine.RAW565_HEIGHT, DMDEngine.RAW565_WIDTH, 3
        )
        img = Image.fromarray(rgb, "RGB")
        img.format = "RAW565"
        return img

    @staticmethod
    def ensure_rgb_on_black(img: Image.Image) -> Image.Image:
        """Convertit une image en RGB en compositant la transparence sur noir."""
        if img.mode in ("RGBA", "LA") or img.info.get("transparency") is not None:
            img_rgba = img.convert("RGBA")
            background = Image.new("RGB", img_rgba.size, (0, 0, 0))
            background.paste(img_rgba, mask=img_rgba.split()[-1])
            return background
        return img.convert("RGB")

    @staticmethod
    def crop_to_visible_content(img: Image.Image, padding: int = 2) -> Image.Image:
        """Recadre l'image sur son contenu visible (bbox du canal alpha) si elle a
        une transparence effective — corrige les échelles fit/fill faussées par des
        marges transparentes (fréquent sur les logos "clear logo" avec un cadre de
        taille uniforme). Sans transparence effective, retourne l'image inchangée
        (aucun moyen fiable de distinguer contenu et fond sans canal alpha). Voir
        changelog v14."""
        if img.mode not in ("RGBA", "LA") and img.info.get("transparency") is None:
            return img
        alpha = img.convert("RGBA").split()[-1]
        bbox = alpha.getbbox()
        if bbox is None:
            return img
        x0, y0, x1, y1 = bbox
        if (x0, y0, x1, y1) == (0, 0, img.width, img.height):
            return img
        x0 = max(0, x0 - padding)
        y0 = max(0, y0 - padding)
        x1 = min(img.width, x1 + padding)
        y1 = min(img.height, y1 + padding)
        return img.crop((x0, y0, x1, y1))

    @staticmethod
    def detect_background_color(img: Image.Image) -> Tuple[Tuple[int, int, int], bool]:
        """
        Détecte la couleur de fond en analysant les coins de l'image.
        Retourne: (couleur_rgb, est_sombre)
        """
        arr = np.array(DMDEngine.ensure_rgb_on_black(img))
        h, w = arr.shape[:2]

        corner_size = min(10, min(h, w) // 8)
        corners = np.concatenate(
            [
                arr[:corner_size, :corner_size].reshape(-1, 3),
                arr[:corner_size, -corner_size:].reshape(-1, 3),
                arr[-corner_size:, :corner_size].reshape(-1, 3),
                arr[-corner_size:, -corner_size:].reshape(-1, 3),
            ]
        )

        bg_candidate = np.median(corners, axis=0).astype(int)

        if np.mean(bg_candidate) < 40:
            return (0, 0, 0), True

        dist_to_black = np.sqrt(np.sum(bg_candidate**2))
        if dist_to_black < 80:
            return (0, 0, 0), True

        return (int(bg_candidate[0]), int(bg_candidate[1]), int(bg_candidate[2])), False

    @staticmethod
    def detect_palette(
        img: Image.Image, max_colors: int = 16
    ) -> List[Tuple[int, int, int]]:
        """
        Détecte les couleurs dominantes de l'image.
        Retourne: liste de couleurs RGB triées par fréquence.
        """
        img_small = img.resize((128, 128), Image.Resampling.LANCZOS)
        arr = np.array(img_small.convert("RGB"))
        pixels = arr.reshape(-1, 3)

        quantized = (pixels // 32) * 32
        unique, counts = np.unique(quantized, axis=0, return_counts=True)

        sorted_indices = np.argsort(-counts)

        palette: List[Tuple[int, int, int]] = []
        for i in sorted_indices[:max_colors]:
            rgb = unique[i]
            palette.append((int(rgb[0]), int(rgb[1]), int(rgb[2])))

        return palette

    @staticmethod
    def optimize_for_dmd(img: Image.Image, settings: Dict[str, Any]) -> Image.Image:
        """Optimise l'image pour affichage DMD."""
        img = DMDEngine.ensure_rgb_on_black(img)

        if "brightness" in settings and settings["brightness"] != 1.0:
            enhancer = ImageEnhance.Brightness(img)
            img = enhancer.enhance(float(settings["brightness"]))

        # Référence AVANT contraste/saturation/netteté, pour la protection
        # anti-écrêtage ci-dessous.
        arr_before = np.array(img).astype(np.float32)

        enhancer = ImageEnhance.Contrast(img)
        img = enhancer.enhance(float(settings.get("contrast", 1.5)))

        enhancer = ImageEnhance.Color(img)
        img = enhancer.enhance(float(settings.get("saturation", 1.3)))

        enhancer = ImageEnhance.Sharpness(img)
        img = enhancer.enhance(1.2)

        # Protection anti-écrêtage des hautes lumières : le contraste/saturation/
        # netteté peuvent pousser des pixels DÉJÀ clairs (reflets métalliques,
        # dégradés) jusqu'au blanc pur, créant un halo absent de l'image source. On
        # plafonne le gain de luminosité des pixels qui n'étaient PAS déjà proches
        # du blanc avant amélioration — les vrais blancs voulus de la source ne
        # sont pas affectés.
        arr_after = np.array(img).astype(np.float32)
        MAX_GAIN = 60.0
        NEAR_WHITE_BEFORE = 220.0
        gain = arr_after - arr_before
        clip_mask = (gain > MAX_GAIN) & (arr_before < NEAR_WHITE_BEFORE)
        arr_after = np.where(clip_mask, arr_before + MAX_GAIN, arr_after)
        img = Image.fromarray(np.clip(arr_after, 0, 255).astype(np.uint8))

        arr = np.array(img)
        black_threshold = int(settings.get("black_threshold", 30))
        mask = np.all(arr < black_threshold, axis=2)
        arr[mask] = [0, 0, 0]

        img = Image.fromarray(arr)
        img = img.filter(ImageFilter.MedianFilter(size=3))

        return img

    @staticmethod
    def _pixel_perfect_dims(
        w: int, h: int, target_w: int, target_h: int, mode: str,
        direction: str = None,
    ) -> Tuple[int, int, Any]:
        """
        Calcule des dimensions de sortie en échelle ENTIÈRE par rapport à la taille
        source, pour qu'un pixel source corresponde exactement à un bloc carré
        identique de LED (pas de pixel "à cheval" sur plusieurs LED).

        - Agrandissement (image plus petite que la cible) : facteur entier k >= 1,
          rééchantillonnage NEAREST (réplication exacte de bloc k×k).
        - Réduction (image plus grande que la cible) : diviseur entier d >= 1,
          rééchantillonnage BOX (moyenne exacte d'un bloc d×d de pixels source).

        En mode "fit" l'arrondi se fait vers le bas (l'image ne doit jamais déborder
        du cadre) ; en mode "fill" vers le haut (l'image doit toujours couvrir tout le
        cadre, quitte à déborder pour être recadrée/scrollée ensuite).

        En mode "fill" avec `direction` connue ("horizontal" ou "vertical"), le
        facteur entier est calculé UNIQUEMENT à partir de l'axe FIXE (non scrollé,
        perpendiculaire à `direction`), avec le même arrondi "sûr" que "fit" —
        garantit que cet axe ne déborde JAMAIS, donc n'est jamais recadré (voir
        changelog v15). Sans direction connue, comportement combiné historique
        (peut déborder sur les 2 axes si le ratio source s'écarte trop de la cible).
        """
        if mode == "fill" and direction in ("horizontal", "vertical"):
            # Axe fixe = perpendiculaire au sens du scroll.
            if direction == "horizontal":
                fixed_src, fixed_target = h, target_h
            else:
                fixed_src, fixed_target = w, target_w
            fixed_scale = fixed_target / fixed_src
            if fixed_scale >= 1:
                k = max(1, math.floor(fixed_scale))
                return w * k, h * k, Image.Resampling.NEAREST
            d = max(1, math.ceil(1 / fixed_scale))
            new_w = max(1, round(w / d))
            new_h = max(1, round(h / d))
            return new_w, new_h, Image.Resampling.BOX

        scale = (
            min(target_w / w, target_h / h)
            if mode == "fit"
            else max(target_w / w, target_h / h)
        )

        if scale >= 1:
            k = math.floor(scale) if mode == "fit" else math.ceil(scale)
            k = max(1, k)
            return w * k, h * k, Image.Resampling.NEAREST

        d = math.ceil(1 / scale) if mode == "fit" else math.floor(1 / scale)
        d = max(1, d)
        new_w = max(1, round(w / d))
        new_h = max(1, round(h / d))
        return new_w, new_h, Image.Resampling.BOX

    @staticmethod
    def adaptive_resize(
        img: Image.Image,
        target_w: int = 128,
        target_h: int = 32,
        mode: str = "auto",
        pixel_perfect: bool = False,
        direction: str = None,
    ) -> Tuple[Image.Image, int, int]:
        """
        Redimensionnement adaptatif pour DMD.
        """
        w, h = img.size

        if mode == "auto":
            ratio = w / h
            target_ratio = target_w / target_h
            mode = "fit" if abs(ratio - target_ratio) < 0.5 else "fill"

        if pixel_perfect:
            new_w, new_h, resample = DMDEngine._pixel_perfect_dims(
                w, h, target_w, target_h, mode, direction=direction
            )
            img = img.resize((new_w, new_h), resample)
            return img, new_w, new_h

        if mode == "fit":
            scale = min(target_w / w, target_h / h)
        else:
            scale = max(target_w / w, target_h / h)

        new_w = int(w * scale)
        new_h = int(h * scale)

        img = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        return img, new_w, new_h

    @staticmethod
    def _render_scroll_frame_at_offset(
        img: Image.Image,
        bg_color: Tuple[int, int, int],
        cleanup: bool,
        cleanup_power: float,
        offset: int,
        horizontal: bool,
        fixed_coord: int,
    ) -> Image.Image:
        """Rend UNE frame de scroll à un offset donné, en O(1) (pas de boucle sur les
        offsets intermédiaires). Extrait de _scroll_axis_frames pour être réutilisable
        par create_animation_frames (paramètre sample_mid_scroll) sans reproduire le
        cycle complet aller-retour juste pour une frame d'aperçu."""
        pos = (-offset, fixed_coord) if horizontal else (fixed_coord, -offset)
        canvas = Image.new("RGB", (128, 32), bg_color)
        canvas.paste(img, pos)
        if cleanup:
            canvas = DMDEngine.cleanup_dmd_frame(canvas, cleanup_power)
        return canvas

    @staticmethod
    def _scroll_axis_frames(
        img: Image.Image,
        bg_color: Tuple[int, int, int],
        cleanup: bool,
        cleanup_power: float,
        max_offset: int,
        scroll_speed: int,
        horizontal: bool,
        fixed_coord: int,
        max_frames: "int | None" = None,
        frame_repeat: int = 1,
    ) -> List[Image.Image]:
        """Génère les frames de scroll aller-retour sur un axe (horizontal ou vertical).
        Si max_frames est fourni, s'arrête dès que ce nombre de frames est atteint
        (les frames déjà produites sont un préfixe identique à la séquence complète).
        frame_repeat répète chaque position de scroll (utilisé pour scroll_speed<1,
        où le pas pixel reste 1 mais chaque frame est tenue plus longtemps)."""
        frames: List[Image.Image] = []

        for offset in range(0, max_offset + 1, scroll_speed):
            canvas = DMDEngine._render_scroll_frame_at_offset(
                img, bg_color, cleanup, cleanup_power, offset, horizontal, fixed_coord
            )
            for _ in range(frame_repeat):
                frames.append(canvas)
                if max_frames is not None and len(frames) >= max_frames:
                    return frames

        for offset in range(max_offset, -1, -scroll_speed):
            canvas = DMDEngine._render_scroll_frame_at_offset(
                img, bg_color, cleanup, cleanup_power, offset, horizontal, fixed_coord
            )
            for _ in range(frame_repeat):
                frames.append(canvas)
                if max_frames is not None and len(frames) >= max_frames:
                    return frames

        return frames

    @staticmethod
    def create_animation_frames(
        img: Image.Image,
        settings: Dict[str, Any],
        bg_color: Tuple[int, int, int] = (0, 0, 0),
        cleanup: bool = False,
        cleanup_power: float = 1.0,
        max_frames: "int | None" = None,
        sample_mid_scroll: bool = False,
    ) -> Tuple[List[Image.Image], str]:
        """
        Génère les frames d'animation selon direction et paramètres.
        Retourne: (frames, direction_effective)

        max_frames (optionnel) : arrête la génération dès que ce nombre de frames est
        atteint, sans générer ni padder le reste de l'animation. À utiliser quand seul
        un aperçu (ex. frames[0]) est nécessaire — évite de générer un cycle de scroll
        complet (potentiellement des centaines de frames) pour n'en garder qu'une.

        sample_mid_scroll (optionnel, avec max_frames=1 uniquement) : pour les modes
        scroll (horizontal/vertical), renvoie la frame au MILIEU du scroll
        (offset=max_offset//2) au lieu du début (offset=0). offset=0 ne montre que les
        128 premiers pixels de l'image redimensionnée (souvent bien plus large en mode
        fill), ce qui tronque le contenu — trompeur pour un score ou un aperçu censé
        représenter l'ensemble de l'image. Calculé directement en O(1) via
        _render_scroll_frame_at_offset, sans boucler sur les offsets intermédiaires.
        Sans effet sur les directions "static".
        """
        w, h = img.size
        direction = str(settings["direction"])
        fps = int(settings["fps"])
        duration = float(settings.get("duration", 2.0))

        scroll_speed_raw = float(settings["scroll_speed"])
        if scroll_speed_raw <= 0:
            scroll_speed_raw = 1.0
        if scroll_speed_raw >= 1:
            scroll_speed = max(1, round(scroll_speed_raw))
            frame_repeat = 1
        else:
            # Pas de notion de sous-pixel sur une grille de LED : le pas reste 1px,
            # mais chaque position est tenue plus longtemps pour ralentir le scroll.
            scroll_speed = 1
            frame_repeat = max(1, round(1 / scroll_speed_raw))

        if direction == "auto":
            if w > 128 and h <= 32:
                direction = "horizontal"
            elif h > 32 and w <= 128:
                direction = "vertical"
            elif w > 128 and h > 32:
                direction = "horizontal" if (w - 128) > (h - 32) else "vertical"
            else:
                direction = "static"

        frames: List[Image.Image] = []

        if direction == "static" or (w <= 128 and h <= 32):
            canvas = Image.new("RGB", (128, 32), bg_color)
            x = (128 - w) // 2
            y = (32 - h) // 2
            canvas.paste(img, (x, y))
            if cleanup:
                canvas = DMDEngine.cleanup_dmd_frame(canvas, cleanup_power)
            num_frames = max(int(fps * duration), 1)
            if max_frames is not None:
                num_frames = min(num_frames, max_frames)
            frames = [canvas] * num_frames

        elif direction == "horizontal":
            max_offset = max(0, w - 128)
            y = (32 - h) // 2
            if sample_mid_scroll and max_frames == 1:
                frames = [
                    DMDEngine._render_scroll_frame_at_offset(
                        img, bg_color, cleanup, cleanup_power,
                        max_offset // 2, True, y,
                    )
                ]
            else:
                frames = DMDEngine._scroll_axis_frames(
                    img,
                    bg_color,
                    cleanup,
                    cleanup_power,
                    max_offset,
                    scroll_speed,
                    horizontal=True,
                    fixed_coord=y,
                    max_frames=max_frames,
                    frame_repeat=frame_repeat,
                )

        elif direction == "vertical":
            max_offset = max(0, h - 32)
            x = (128 - w) // 2
            if sample_mid_scroll and max_frames == 1:
                frames = [
                    DMDEngine._render_scroll_frame_at_offset(
                        img, bg_color, cleanup, cleanup_power,
                        max_offset // 2, False, x,
                    )
                ]
            else:
                frames = DMDEngine._scroll_axis_frames(
                    img,
                    bg_color,
                    cleanup,
                    cleanup_power,
                    max_offset,
                    scroll_speed,
                    horizontal=False,
                    fixed_coord=x,
                    max_frames=max_frames,
                    frame_repeat=frame_repeat,
                )

        else:
            canvas = Image.new("RGB", (128, 32), bg_color)
            canvas.paste(img, ((128 - w) // 2, (32 - h) // 2))
            frames = [canvas]

        if max_frames is None:
            if frames and len(frames) < fps * duration:
                multiplier = int((fps * duration) / len(frames)) + 1
                frames = frames * multiplier
        elif frames:
            frames = frames[:max_frames]

        return (
            frames if frames else [Image.new("RGB", (128, 32), bg_color)]
        ), direction

    @staticmethod
    def cleanup_dmd_frame(img: Image.Image, power: float = 1.0) -> Image.Image:
        """Nettoie les pixels réellement ISOLÉS (aucun des 8 voisins directs n'est
        de couleur proche) — jamais un pixel faisant partie d'un trait cohérent
        (au moins 1 voisin proche le long du trait). Voir changelog v13."""
        if img.mode != "RGB":
            img = img.convert("RGB")

        if power <= 0:
            return img

        power = max(0.0, min(1.0, float(power)))

        # Seuil de distance couleur (somme des écarts absolus RVB, 0-765)
        # au-dessus duquel un voisin est considéré "différent" -> plus power est
        # élevé, plus le seuil est bas (nettoyage plus agressif).
        threshold = 380.0 - power * 260.0
        passes = 1
        if power >= 0.6:
            passes = 2
        if power >= 0.9:
            passes = 3

        cleaned = img
        for _ in range(passes):
            cleaned = DMDEngine._remove_isolated_pixels(cleaned, threshold)
        return cleaned

    @staticmethod
    def _remove_isolated_pixels(img: Image.Image, threshold: float) -> Image.Image:
        """Remplace par la valeur du filtre médian local (3x3) UNIQUEMENT les
        pixels dont la distance couleur à CHACUN de leurs 8 voisins directs
        dépasse `threshold` — un pixel ayant ne serait-ce qu'1 voisin proche n'est
        jamais considéré isolé, ce qui préserve les traits fins."""
        arr = np.array(img).astype(np.int32)
        h, w, _ = arr.shape
        padded = np.pad(arr, ((1, 1), (1, 1), (0, 0)), mode="edge")

        min_dist = None
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy == 0 and dx == 0:
                    continue
                neighbor = padded[1 + dy: 1 + dy + h, 1 + dx: 1 + dx + w, :]
                dist = np.abs(arr - neighbor).sum(axis=2)
                min_dist = dist if min_dist is None else np.minimum(min_dist, dist)

        mask = min_dist > threshold
        if not mask.any():
            return img

        median = np.array(img.filter(ImageFilter.MedianFilter(size=3))).astype(np.int32)
        result = arr.copy()
        result[mask] = median[mask]
        return Image.fromarray(result.astype(np.uint8), mode="RGB")

    @staticmethod
    def render_led_style(
        frame_128x32: Image.Image,
        scale: int = 8,
        led_ratio: float = 0.525,
        glow: bool = True,
        supersample: int = 4,
        brightness: float = 0.5,
    ) -> Image.Image:
        """Simule un rendu physique DMD/matrice de LED à partir d'une frame 128×32 :
        chaque pixel devient un point rond (au lieu d'un carré plein), séparé de ses
        voisins par un espace sombre (bezel), avec un léger flou de halo optionnel
        pour imiter le bloom d'une vraie LED. Pixels quasi noirs (LED éteinte) non
        dessinés, pour un fond bien noir entre les points.

        scale : facteur d'agrandissement (taille de sortie = 128*scale × 32*scale).
        led_ratio : diamètre du point / taille de la cellule (0-1, 1 = pas d'espace).
            Défaut 0.525 = LED SMD 2121 (2.1 mm) sur un panneau HUB75 P4 (pas de
            4 mm) — matériel réellement utilisé par l'utilisateur (2 panneaux
            64×32 de 256×128 mm côte à côte), voir changelog v9.
        supersample : facteur de suréchantillonnage interne avant réduction en
            LANCZOS (voir changelog v10) — nécessaire pour que led_ratio se
            traduise vraiment visuellement à scale=4 (taille d'affichage réelle
            utilisée partout dans l'app) : PIL ImageDraw.ellipse n'anticrénèle
            pas, donc un point de ~1px de rayon (cas scale=4) se dessine de façon
            disproportionnée/blocage quel que soit led_ratio. Dessiner à
            scale*supersample puis réduire lisse la forme et rend enfin le ratio
            perceptible. Mesuré ~12ms/frame à scale=4/supersample=4, largement
            sous le budget d'une animation 10fps (100ms/frame).
        brightness : simulation de la luminosité physique des LED (0.0-1.0,
            défaut 0.5 = comportement neutre, identique aux constantes v10
            fixes). **v12 : ré-conçu suite à un retour utilisateur** — la v11
            ne faisait varier que la teinte des couleurs (color_mult), ce qui
            "changeait la couleur mais n'éclairait pas plus" (constat exact :
            un point déjà proche de sa couleur max ne peut visuellement pas
            devenir plus lumineux par simple multiplication RGB, l'effet était
            donc quasi invisible sur des couleurs déjà vives). Sur du matériel
            réel, une LED plus lumineuse ne change pas de teinte : elle semble
            PLUS GROSSE (bloom/halation, la lumière déborde visuellement sur
            ses voisines) et son halo s'étend et s'intensifie davantage. C'est
            maintenant le mécanisme principal : le diamètre effectif du point
            (`led_ratio` + variation liée à brightness) et le halo de flou
            grossissent avec la luminosité — jusqu'à ce que les points
            commencent à se toucher/baver les uns sur les autres à pleine
            puissance (comportement réaliste d'un panneau poussé à fond) ;
            à luminosité minimale, points réduits à de petits points nets et
            halo quasi supprimé (LED à peine alimentée). La teinte ne varie
            plus que légèrement (contribution secondaire). Formules calées
            pour que brightness=0.5 reproduise exactement le rendu neutre
            précédent (aucune régression) ; None des 2 versions (v11/v12) ne
            change quoi que ce soit à brightness=0.5.
        """
        brightness = max(0.0, min(1.0, brightness))
        color_mult = 0.7 + 0.6 * brightness
        if brightness >= 0.5:
            # Moitié haute : coefficients plus agressifs pour que les points
            # atteignent/dépassent le plein recouvrement de la cellule à
            # brightness=1.0 (voir changelog v16 — la formule d'origine
            # n'allait jamais assez loin, rendu final ~63% plus sombre que le
            # rendu classique).
            bloom_delta = (brightness - 0.5) * 1.15
            glow_radius_factor = 0.15 + (brightness - 0.5) * 0.8
            glow_blend = min(0.85, 0.25 + (brightness - 0.5) * 1.0)
        else:
            # Moitié basse : INCHANGÉE (aucune régression sous brightness=0.5).
            bloom_delta = (brightness - 0.5) * 0.5
            glow_radius_factor = max(0.02, 0.15 + (brightness - 0.5) * 0.4)
            glow_blend = max(0.02, 0.25 + (brightness - 0.5) * 0.6)
        effective_led_ratio = max(0.15, min(1.3, led_ratio + bloom_delta))

        src = np.array(frame_128x32.convert("RGB")).astype(np.float32) * color_mult
        src = np.clip(src, 0, 255).astype(np.uint8)
        h, w, _ = src.shape
        big_scale = scale * supersample
        canvas = Image.new("RGB", (w * big_scale, h * big_scale), (0, 0, 0))
        draw = ImageDraw.Draw(canvas)
        dot_r = (big_scale * effective_led_ratio) / 2

        for y in range(h):
            for x in range(w):
                r, g, b = src[y, x]
                if r < 8 and g < 8 and b < 8:
                    continue
                cx = x * big_scale + big_scale / 2
                cy = y * big_scale + big_scale / 2
                draw.ellipse(
                    [cx - dot_r, cy - dot_r, cx + dot_r, cy + dot_r],
                    fill=(int(r), int(g), int(b)),
                )

        canvas = canvas.resize((w * scale, h * scale), Image.Resampling.LANCZOS)

        if glow:
            # Rayon/mélange dérivés de `brightness` (voir docstring) — à
            # brightness=0.5 (défaut) : 0.15/0.25, identique à la v10 fixe qui
            # avait déjà été vérifiée par comparaison visuelle directe.
            blurred = canvas.filter(
                ImageFilter.GaussianBlur(radius=scale * glow_radius_factor)
            )
            canvas = Image.blend(canvas, blurred, glow_blend)

        return canvas
