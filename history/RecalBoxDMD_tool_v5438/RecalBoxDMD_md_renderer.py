"""
Module de rendu Markdown vers tkinter Text.
state=normal permanent. tag_bind sur les liens.
Widget bind <Button-1> retourne "break" pour stopper le curseur.
<Key> bloque la saisie.

Images (balises <img>, ex. ![alt](chemin/relatif.png)) : affichées réellement
si `base_dir` est fourni à `render_markdown_in_text()` ET que Pillow est
disponible — sinon repli sur l'ancien texte "[Image: alt](src)" (comportement
identique à avant, zéro régression pour un appelant qui ne passe pas
`base_dir` ou tourne sans Pillow installé). Seules les images LOCALES
(chemin relatif à `base_dir`) sont chargées ; les URL http(s)/data: restent
en texte, ce module ne fait aucun accès réseau.
"""

import unicodedata
import webbrowser
from html.parser import HTMLParser
from pathlib import Path
import markdown as md_lib

try:
    from PIL import Image, ImageTk

    _PIL_AVAILABLE = True
except ImportError:
    _PIL_AVAILABLE = False

# Largeur max (px) d'une image insérée dans le Text — sous-échantillonnage
# préservant le ratio si l'image source est plus large, jamais d'agrandissement.
_MAX_IMAGE_WIDTH = 700

# IMPORTANT : l'ordre des cles determine la priorite des tags Tkinter
# (un tag configure plus tard a priorite plus haute sur les attributs
# partages, ex. "font"). Les tags de BLOC doivent donc venir en premier,
# et les tags de mise en forme EN LIGNE (gras/italique/code/barre) en
# dernier : sinon un tag de bloc comme "p" ou "blockquote" ecrase le
# gras/italique de tout le texte qu'il contient (bug constate : **gras**
# et _italique_ ne s'affichaient jamais dans les paragraphes/citations).
def _build_tag_configs(md_colors):
    """Construit la config des tags Tkinter. Les polices sont fixes, les
    couleurs viennent de md_colors (derivees du theme actif, voir
    _derive_markdown_colors)."""
    return {
        "h1": {"font": ("TkDefaultFont", 16, "bold"), "spacing1": 12, "spacing3": 6},
        "h2": {"font": ("TkDefaultFont", 14, "bold"), "spacing1": 10, "spacing3": 4},
        "h3": {"font": ("TkDefaultFont", 12, "bold"), "spacing1": 8, "spacing3": 3},
        "h4": {"font": ("TkDefaultFont", 11, "bold"), "spacing1": 6, "spacing3": 2},
        "h5": {"font": ("TkDefaultFont", 10, "bold"), "spacing1": 4, "spacing3": 2},
        "h6": {"font": ("TkDefaultFont", 10, "bold"), "spacing1": 4, "spacing3": 2},
        "p": {"font": ("TkDefaultFont", 10)},
        "list": {"lmargin1": 20, "lmargin2": 30, "spacing1": 2},
        "olist": {"lmargin1": 20, "lmargin2": 30, "spacing1": 2},
        "blockquote": {
            "lmargin1": 20,
            "lmargin2": 20,
            "foreground": md_colors["blockquote_fg"],
            "font": ("TkDefaultFont", 10, "italic"),
            "background": md_colors["blockquote_bg"],
        },
        "table_header": {
            "font": ("Courier New", 9, "bold"),
            "background": md_colors["table_header_bg"],
        },
        "table_cell": {"font": ("Courier New", 9)},
        "codeblock": {
            "font": ("Courier New", 9),
            "background": md_colors["codeblock_bg"],
            "spacing1": 4,
            "spacing3": 4,
            "lmargin1": 10,
            "lmargin2": 10,
        },
        "separator": {
            "foreground": md_colors["separator_fg"],
            "font": ("Courier New", 9),
        },
        # --- tags en ligne : configures en dernier, priorite la plus haute ---
        "bold": {"font": ("TkDefaultFont", 10, "bold")},
        "italic": {"font": ("TkDefaultFont", 10, "italic")},
        "bolditalic": {"font": ("TkDefaultFont", 10, "bold italic")},
        "code": {"font": ("Courier New", 9), "background": md_colors["code_bg"]},
        "strikethrough": {"overstrike": True},
    }


def _hex_to_rgb(hexcolor):
    h = (hexcolor or "").lstrip("#")
    if len(h) == 3:
        h = "".join(c * 2 for c in h)
    if len(h) != 6:
        return (0, 0, 0)
    try:
        return tuple(int(h[i : i + 2], 16) for i in (0, 2, 4))
    except ValueError:
        return (0, 0, 0)


def _rgb_to_hex(rgb):
    return "#%02X%02X%02X" % tuple(max(0, min(255, round(c))) for c in rgb)


def _mix(c1, c2, t):
    return tuple(a + (b - a) * t for a, b in zip(c1, c2))


def _luminance(rgb):
    def chan(c):
        c = c / 255.0
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4

    r, g, b = rgb
    return 0.2126 * chan(r) + 0.7152 * chan(g) + 0.0722 * chan(b)


def _contrast(rgb1, rgb2):
    l1, l2 = _luminance(rgb1) + 0.05, _luminance(rgb2) + 0.05
    return max(l1, l2) / min(l1, l2)


def _ensure_contrast(fg_rgb, bg_rgb, min_ratio=4.5):
    """Rapproche fg_rgb du blanc ou du noir (selon ce qui aide) par petits
    pas jusqu'a atteindre min_ratio de contraste avec bg_rgb. Garantit la
    lisibilite d'un accent derive quel que soit le theme, sans lui faire
    perdre sa teinte d'origine plus que necessaire."""
    if _contrast(fg_rgb, bg_rgb) >= min_ratio:
        return fg_rgb
    target = (255, 255, 255) if _luminance(bg_rgb) < 0.5 else (0, 0, 0)
    best = fg_rgb
    for i in range(1, 21):
        candidate = _mix(fg_rgb, target, i / 20)
        best = candidate
        if _contrast(candidate, bg_rgb) >= min_ratio:
            break
    return best


def _tk_color_to_rgb(tw, color):
    """Resout une couleur Tk (nom comme 'white' ou hex) en RGB 0-255 via le
    widget, pour accepter n'importe quelle valeur renvoyee par tw.cget()."""
    try:
        r, g, b = tw.winfo_rgb(color)
        return (r // 256, g // 256, b // 256)
    except Exception:
        return (255, 255, 255)


def _derive_markdown_colors(tw, bg_color, fg_color):
    """Derive la palette d'accents markdown (citations, code, tableaux,
    liens, separateurs) a partir des couleurs de fond/texte du theme
    actuellement applique au widget. Fidele au theme (aucune teinte figee
    n'est injectee, tout part de bg/fg) tout en garantissant un contraste
    suffisant (~4.5:1 texte, ~4.0:1 pour le texte secondaire des citations)."""
    bg = _tk_color_to_rgb(tw, bg_color)
    fg = _tk_color_to_rgb(tw, fg_color)
    dark = _luminance(bg) < 0.5

    panel_soft = _mix(bg, fg, 0.08)
    panel_code = _mix(bg, fg, 0.12)
    panel_strong = _mix(bg, fg, 0.16)
    border = _mix(bg, fg, 0.30)
    quote_fg = _ensure_contrast(_mix(fg, bg, 0.20), panel_soft, min_ratio=4.0)

    link_seed = (109, 168, 255) if dark else (6, 69, 173)
    anchor_seed = (110, 220, 210) if dark else (11, 111, 161)
    link = _ensure_contrast(link_seed, bg, min_ratio=4.5)
    anchor = _ensure_contrast(anchor_seed, bg, min_ratio=4.5)

    return {
        "blockquote_bg": _rgb_to_hex(panel_soft),
        "blockquote_fg": _rgb_to_hex(quote_fg),
        "code_bg": _rgb_to_hex(panel_code),
        "codeblock_bg": _rgb_to_hex(panel_soft),
        "table_header_bg": _rgb_to_hex(panel_strong),
        "separator_fg": _rgb_to_hex(border),
        "link": _rgb_to_hex(link),
        "anchor": _rgb_to_hex(anchor),
    }

# Plages Unicode affichees sur 2 colonnes dans une police a chasse fixe
# (emoji, pictogrammes...) meme quand unicodedata.east_asian_width() ne
# les classe pas "Wide"/"Fullwidth". Sert a aligner correctement les
# tableaux markdown qui contiennent des emoji (ex. tableau materiel).
_WIDE_RANGES = (
    (0x1F300, 0x1FAFF),
    (0x2600, 0x27BF),
    (0x2B00, 0x2BFF),
    (0x1F1E6, 0x1F1FF),
)
# Caracteres de largeur nulle (accents combinants, selecteurs de variante,
# joker de liaison emoji) a ne pas compter dans la largeur affichee.
_ZERO_WIDTH_RANGES = (
    (0x0300, 0x036F),
    (0x200B, 0x200F),
    (0xFE00, 0xFE0F),
    (0x1F3FB, 0x1F3FF),
)


def _char_width(ch: str) -> int:
    cp = ord(ch)
    if cp == 0x200D:
        return 0
    for lo, hi in _ZERO_WIDTH_RANGES:
        if lo <= cp <= hi:
            return 0
    for lo, hi in _WIDE_RANGES:
        if lo <= cp <= hi:
            return 2
    if unicodedata.east_asian_width(ch) in ("W", "F"):
        return 2
    return 1


def _display_width(s: str) -> int:
    """Largeur d'affichage approximative (police a chasse fixe), en tenant
    compte des emoji/pictogrammes qui occupent 2 colonnes."""
    return sum(_char_width(c) for c in s)


def _resolve_emphasis(tags):
    """Fusionne "bold" + "italic" actifs simultanement (ex. ***texte***,
    **_texte_**, _**texte**_) en un seul tag "bolditalic" : Tkinter ne
    combine pas deux tags qui definissent chacun "font", il n'en garde
    qu'un (le plus prioritaire), donc sans fusion l'un des deux styles
    est silencieusement perdu."""
    if "bold" in tags and "italic" in tags:
        tags = [t for t in tags if t not in ("bold", "italic")]
        tags.append("bolditalic")
    return tags


def _strip_run_whitespace(runs):
    """Enleve les espaces de debut/fin d'une liste de (texte, tags) tout en
    conservant la mise en forme (gras/italique/lien) du texte utile."""
    runs = list(runs)
    while runs and runs[0][0].strip() == "":
        runs.pop(0)
    if runs:
        text, tags = runs[0]
        stripped = text.lstrip()
        if stripped != text:
            runs[0] = (stripped, tags)
    while runs and runs[-1][0].strip() == "":
        runs.pop()
    if runs:
        text, tags = runs[-1]
        stripped = text.rstrip()
        if stripped != text:
            runs[-1] = (stripped, tags)
    return runs


class _Builder(HTMLParser):
    """Parse HTML et insere dans un Text avec tags."""

    def __init__(self, tw, on_external_link, on_anchor_link, md_colors, base_dir=None):
        super().__init__(convert_charrefs=True)
        self.tw = tw
        self.on_url = on_external_link
        self.on_anchor = on_anchor_link
        self._md_colors = md_colors
        self._base_dir = base_dir
        # Références PhotoImage conservées ici (pas seulement locales à cette
        # méthode) : Tkinter ne garde aucune référence forte sur une image
        # affichée via image_create — sans ça, le garbage collector Python la
        # libère dès la fin de handle_starttag et l'image disparaît de
        # l'écran au prochain repaint (piège Tkinter classique).
        self._images = []
        self.tags = []
        self.buf = ""
        self._in_pre = False
        self._in_table = False
        self._list_stack = []
        self._ol_n = []
        self._heading_id = None
        self._link_n = 0
        self._anchor_n = 0
        self._anchor_map = {}
        self._link_map = {}
        self._table_rows = []
        self._table_cur_row = []
        self._table_cur_runs = []
        self._table_cur_celltype = "cell"

        for name, cfg in _build_tag_configs(md_colors).items():
            try:
                self.tw.tag_configure(name, **cfg)
            except Exception:
                pass

    def _insert_with_tags(self, text, tag_list):
        if not text:
            return
        if text == "\n":
            self.tw.insert("end", text)
            return
        start = self.tw.index("end-1c")
        self.tw.insert("end", text)
        if tag_list:
            tag_list = _resolve_emphasis(tag_list)
            end = self.tw.index("end-1c")
            for t in tag_list:
                try:
                    self.tw.tag_add(t, start, end)
                except Exception:
                    pass

    def _emit(self, text, tags=None):
        self._insert_with_tags(text, tags or [])

    def _flush(self):
        if not self.buf:
            return
        text = self.buf
        self.buf = ""
        self._insert_with_tags(text, self.tags)

    def _load_image(self, src):
        """Charge `src` (chemin relatif à `self._base_dir`) en PhotoImage
        Tkinter, sous-échantillonné à `_MAX_IMAGE_WIDTH` si besoin (ratio
        préservé, jamais agrandi). Retourne None si Pillow est absent,
        `base_dir` n'a pas été fourni, `src` est une URL distante
        (http/https/data:, jamais chargée par ce module) ou si le fichier
        est introuvable/illisible — dans tous ces cas l'appelant retombe sur
        le texte "[Image: alt](src)" historique."""
        if not _PIL_AVAILABLE or not self._base_dir or not src:
            return None
        if src.startswith(("http://", "https://", "data:")):
            return None
        try:
            path = (Path(self._base_dir) / src).resolve()
            img = Image.open(path)
            img.load()
            if img.width > _MAX_IMAGE_WIDTH:
                ratio = _MAX_IMAGE_WIDTH / img.width
                new_size = (_MAX_IMAGE_WIDTH, max(1, round(img.height * ratio)))
                img = img.resize(new_size, Image.Resampling.LANCZOS)
            return ImageTk.PhotoImage(img)
        except Exception:
            return None

    def _flush_table_cell(self):
        runs = _strip_run_whitespace(self._table_cur_runs)
        plain = "".join(text for text, _tags in runs)
        self._table_cur_row.append((self._table_cur_celltype, plain, runs))
        self._table_cur_runs = []

    def handle_starttag(self, tag, attrs):
        d = dict(attrs)
        if tag in ("h1", "h2", "h3", "h4", "h5", "h6"):
            self._flush()
            self._heading_id = d.get("id")
            self.tags.append(tag)
            if self._heading_id:
                try:
                    pos = self.tw.index("end-1c")
                    mark_name = "anchor_" + self._heading_id
                    self.tw.mark_set(mark_name, pos)
                    # Gravite "left" obligatoire : par defaut (gravite "right"),
                    # une mark derive vers l'avant a chaque insertion faite a sa
                    # position (ici tout le texte inséré ensuite via insert("end", ...)).
                    # Sans ce fix, toutes les marks d'ancrage finissent par se
                    # retrouver collees a la fin du document une fois le rendu termine.
                    self.tw.mark_gravity(mark_name, "left")
                except Exception:
                    pass
            return

        if tag == "p":
            self._flush()
            self._emit("\n")
            self.tags.append("p")
            return

        if tag == "br":
            self._flush()
            self._emit("\n")
            return

        if tag == "hr":
            self._flush()
            self._emit("\n" + "─" * 60 + "\n", ["separator"])
            return

        if tag in ("strong", "b"):
            self._flush()
            self.tags.append("bold")
            return

        if tag in ("em", "i"):
            self._flush()
            self.tags.append("italic")
            return

        if tag == "del":
            self._flush()
            self.tags.append("strikethrough")
            return

        if tag == "code" and not self._in_pre:
            self._flush()
            self.tags.append("code")
            return

        if tag == "pre":
            self._in_pre = True
            self._flush()
            self._emit("\n")
            return

        if tag == "code" and self._in_pre:
            self._flush()
            self.tags.append("codeblock")
            return

        if tag == "blockquote":
            self._flush()
            self._emit("\n")
            self.tags.append("blockquote")
            return

        if tag in ("ul", "ol"):
            self._list_stack.append(tag)
            self._ol_n.append(0)
            self._flush()
            self._emit("\n")
            return

        if tag == "li":
            self._flush()
            depth = max(len(self._list_stack) - 1, 0)
            indent = "  " * depth
            parent = self._list_stack[-1] if self._list_stack else "ul"
            if parent == "ol":
                if self._ol_n:
                    self._ol_n[-1] += 1
                    num = self._ol_n[-1]
                else:
                    num = 1
                self._emit(indent + str(num) + ". ", ["olist"])
            else:
                self._emit(indent + "• ", ["list"])
            self.tags.append("list" if parent != "ol" else "olist")
            return

        if tag == "a":
            href = d.get("href") or ""
            if href.startswith("#"):
                self._anchor_n += 1
                anchor_id = href[1:]
                tagname = "at_" + str(self._anchor_n)
                self.tw.tag_configure(
                    tagname, foreground=self._md_colors["anchor"], underline=True
                )
                self._flush()
                self.tags.append(tagname)
                self._anchor_map[tagname] = anchor_id
                self.tw.tag_bind(
                    tagname,
                    "<Button-1>",
                    lambda e, a=anchor_id: (
                        self.on_anchor(a) if self.on_anchor else "break"
                    ),
                )
            else:
                self._link_n += 1
                tagname = "ln_" + str(self._link_n)
                self.tw.tag_configure(
                    tagname, foreground=self._md_colors["link"], underline=True
                )
                self._flush()
                self.tags.append(tagname)
                self._link_map[tagname] = href
                self.tw.tag_bind(
                    tagname,
                    "<Button-1>",
                    lambda e, u=href: self.on_url(u) if self.on_url else "break",
                )
            return

        if tag == "img":
            src = d.get("src") or ""
            alt = d.get("alt") or "image"
            self._flush()
            photo = self._load_image(src)
            if photo is not None:
                self._emit("\n")
                self.tw.image_create("end", image=photo)
                self._images.append(photo)
                self._emit("\n")
            else:
                # Repli texte : image distante (http/https/data:), fichier
                # local introuvable/corrompu, Pillow absent, ou base_dir non
                # fourni par l'appelant — comportement historique inchangé.
                self._emit("[Image: " + alt + "](" + src + ")", ["italic"])
            return

        if tag == "table":
            self._flush()
            self._table_rows = []
            self._table_cur_row = []
            self._table_cur_runs = []
            self._table_cur_celltype = "cell"
            self._in_table = True
            self._emit("\n")
            return

        if tag == "tr":
            self._flush()
            self._table_cur_row = []
            self._table_cur_runs = []
            self._table_cur_celltype = "cell"
            return

        if tag in ("th", "td"):
            self._flush()
            self._table_cur_celltype = "header" if tag == "th" else "cell"
            self._table_cur_runs = []
            return

        if tag == "span":
            style = (d.get("style") or "").strip()
            mapping = {
                "font-weight: bold; font-style: italic": "bolditalic",
                "font-weight: bold;font-style: italic": "bolditalic",
                "font-weight: bold": "bold",
                "font-style: italic": "italic",
                "text-decoration: line-through": "strikethrough",
            }
            t = mapping.get(style)
            if t:
                self._flush()
                self.tags.append(t)
            return

    def handle_endtag(self, tag):
        if tag == "div":
            return

        if tag in ("h1", "h2", "h3", "h4", "h5", "h6"):
            self._flush()
            if self.tags:
                self.tags.pop()
            if self._heading_id:
                self._heading_id = None
            self._emit("\n\n")
            return

        if tag == "p":
            self._flush()
            if self.tags and self.tags[-1] == "p":
                self.tags.pop()
            self._emit("\n")
            return

        if tag in ("strong", "b"):
            self._flush()
            if self.tags and self.tags[-1] == "bold":
                self.tags.pop()
            return

        if tag in ("em", "i"):
            self._flush()
            if self.tags and self.tags[-1] == "italic":
                self.tags.pop()
            return

        if tag == "del":
            self._flush()
            if self.tags and self.tags[-1] == "strikethrough":
                self.tags.pop()
            return

        if tag == "code" and not self._in_pre:
            self._flush()
            if self.tags and self.tags[-1] == "code":
                self.tags.pop()
            return

        if tag == "code" and self._in_pre:
            self._flush()
            if self.tags and self.tags[-1] == "codeblock":
                self.tags.pop()
            return

        if tag == "pre":
            self._in_pre = False
            self._flush()
            self._emit("\n")
            return

        if tag == "blockquote":
            self._flush()
            if self.tags and self.tags[-1] == "blockquote":
                self.tags.pop()
            self._emit("\n")
            return

        if tag == "li":
            self._flush()
            if self.tags:
                self.tags.pop()
            self._emit("\n")
            return

        if tag in ("ul", "ol"):
            if self._list_stack:
                self._list_stack.pop()
            if self._ol_n:
                self._ol_n.pop()
            self._flush()
            self._emit("\n")
            return

        if tag == "a":
            self._flush()
            if self.tags:
                self.tags.pop()
            return

        if tag in ("th", "td"):
            self._flush()
            # NB : th/td ne pousse pas de tag sur self.tags a l'ouverture
            # (voir handle_starttag), donc rien a depiler ici.
            self._flush_table_cell()
            return

        if tag == "tr":
            self._flush()
            if self._table_cur_row:
                self._table_rows.append(self._table_cur_row)
            self._table_cur_row = []
            self._table_cur_runs = []
            self._table_cur_celltype = "cell"
            return

        if tag == "table":
            self._flush()
            if any(text.strip() for text, _tags in self._table_cur_runs):
                self._flush_table_cell()
            if self._table_cur_row:
                self._table_rows.append(self._table_cur_row)
            if not self._table_rows:
                self._in_table = False
                return
            num_cols = max(len(row) for row in self._table_rows)
            col_widths = [0] * num_cols
            for row in self._table_rows:
                for idx, (_ctype, plain, _runs) in enumerate(row):
                    if idx < num_cols:
                        col_widths[idx] = max(col_widths[idx], _display_width(plain))
            col_widths = [w + 2 for w in col_widths]

            top_border = "┌" + "┬".join("─" * w for w in col_widths) + "┐"
            mid_border = "├" + "┼".join("─" * w for w in col_widths) + "┤"
            bottom_border = "└" + "┴".join("─" * w for w in col_widths) + "┘"

            self._emit(top_border + "\n", ["separator"])
            for row_idx, row in enumerate(self._table_rows):
                is_header_row = row_idx == 0 and any(
                    ct == "header" for ct, _plain, _runs in row
                )
                base_tag = "table_header" if is_header_row else "table_cell"
                self._emit("│", ["separator"])
                for idx in range(num_cols):
                    if idx < len(row):
                        _ctype, plain, runs = row[idx]
                    else:
                        plain, runs = "", []
                    width = col_widths[idx]
                    pad = max(width - _display_width(plain), 0)
                    left_n = pad // 2
                    right_n = pad - left_n
                    if left_n:
                        self._emit(" " * left_n, [base_tag])
                    for text, extra_tags in runs:
                        self._emit(text, [base_tag] + list(extra_tags))
                    if right_n:
                        self._emit(" " * right_n, [base_tag])
                    self._emit("│", ["separator"])
                self._emit("\n")
                if is_header_row:
                    self._emit(mid_border + "\n", ["separator"])
            self._emit(bottom_border + "\n", ["separator"])
            self._in_table = False
            self._table_rows = []
            self._table_cur_row = []
            self._table_cur_runs = []
            return

        if tag == "span":
            self._flush()
            if self.tags:
                self.tags.pop()
            return

    def handle_data(self, data):
        if self._in_table:
            # Capture le texte AVEC les tags actifs (gras/italique/lien...)
            # pour ne plus perdre la mise en forme a l'interieur des cellules.
            self._table_cur_runs.append((data, tuple(self.tags)))
        else:
            self.buf += data


def render_markdown_in_text(tw, md_src, on_external_link=None, on_anchor_link=None, base_dir=None):
    """Affiche le markdown dans un widget Text.
    state=normal permanent. tag_bind sur les liens (haute priorite).
    Widget bind <Button-1> retourne "break" pour stopper le class_bind Text.
    <Key> retourne "break" pour bloquer la saisie.

    `base_dir` (str/Path, optionnel) : dossier de resolution des images
    LOCALES referencees en Markdown (ex. ![alt](images/x.png), resolu comme
    base_dir/images/x.png) — typiquement le dossier du fichier .md source.
    Sans base_dir (valeur par defaut) ou sans Pillow installe, les balises
    <img> retombent sur l'ancien texte "[Image: alt](src)" (comportement
    identique a avant l'ajout du support image)."""
    if on_external_link is None:

        def _ext(url):
            try:
                webbrowser.open_new_tab(url)
            except Exception:
                pass

        on_external_link = _ext

    if on_anchor_link is None:

        def _anc(aid):
            try:
                mark = "anchor_" + aid
                if mark in tw.mark_names():
                    tw.see(mark)
                    return
                for mn in tw.mark_names():
                    if mn.startswith("anchor_"):
                        hid = mn[7:]
                        if aid == hid or hid.startswith(aid) or aid.startswith(hid):
                            tw.see(mn)
                            return
            except Exception:
                pass

        on_anchor_link = _anc

    # Nettoyer les anciens binds
    for seq in ("<Button-1>", "<ButtonRelease-1>", "<Key>"):
        try:
            tw.unbind(seq)
        except Exception:
            pass

    tw.configure(state="normal")
    tw.delete("1.0", "end")

    # Nettoyer les marks d'ancrage d'un rendu precedent (ex. changement de
    # langue dans l'onglet Aide) : sans ca, les marks s'accumulent a chaque
    # appel et peuvent fausser la navigation si une ancre n'existe plus.
    for mn in tw.mark_names():
        if mn.startswith("anchor_"):
            try:
                tw.mark_unset(mn)
            except Exception:
                pass

    # Deriver la palette markdown (citations/code/tableaux/liens) depuis les
    # couleurs bg/fg deja appliquees au widget par le theme actif, pour que
    # le rendu reste fidele au theme tout en restant lisible.
    md_colors = _derive_markdown_colors(tw, tw.cget("bg"), tw.cget("fg"))

    html = md_lib.markdown(
        md_src,
        extensions=["extra", "smarty", "sane_lists", "toc"],
        extension_configs={"toc": {"anchorlink": False, "permalink": False}},
    )

    p = _Builder(tw, on_external_link, on_anchor_link, md_colors, base_dir=base_dir)
    p.feed(html)

    # Stocker les maps sur tw
    tw._anchor_map = p._anchor_map
    tw._link_map = p._link_map
    # Références PhotoImage conservées sur tw (pas seulement sur p, qui sort
    # de portée à la fin de cette fonction) — sans ça le garbage collector
    # libère les images dès le retour de render_markdown_in_text.
    tw._md_images = p._images

    # Bind widget <Button-1> : bloque le curseur de classe Text
    # (en retournant "break", il empeche le class_bind de placer le curseur)
    # les tag_bind sur les liens sont traites AVANT ce widget bind
    tw.bind("<Button-1>", lambda e: "break")
    tw.bind("<Key>", lambda e: "break")

    tw.configure(state="normal")
