"""
Module de rendu Markdown vers tkinter Text.
Utilise "markdown" 3.x pour la conversion MD -> HTML,
puis applique les tags sur le texte insere.

Usage:
    from RecalBoxDMD_md_renderer import render_markdown_in_text
"""

import webbrowser
from html.parser import HTMLParser

import markdown as md_lib

# Tags pre-configures
TAG_CONFIGS = {
    "h1": {"font": ("TkDefaultFont", 16, "bold"), "spacing1": 12, "spacing3": 6},
    "h2": {"font": ("TkDefaultFont", 14, "bold"), "spacing1": 10, "spacing3": 4},
    "h3": {"font": ("TkDefaultFont", 12, "bold"), "spacing1": 8, "spacing3": 3},
    "h4": {"font": ("TkDefaultFont", 11, "bold"), "spacing1": 6, "spacing3": 2},
    "h5": {"font": ("TkDefaultFont", 10, "bold"), "spacing1": 4, "spacing3": 2},
    "h6": {"font": ("TkDefaultFont", 10, "bold"), "spacing1": 4, "spacing3": 2},
    "bold": {"font": ("TkDefaultFont", 10, "bold")},
    "italic": {"font": ("TkDefaultFont", 10, "italic")},
    "bolditalic": {"font": ("TkDefaultFont", 10, "bold italic")},
    "code": {"font": ("Courier New", 9), "background": "#F0F0F0"},
    "codeblock": {
        "font": ("Courier New", 9),
        "background": "#F5F5F5",
        "spacing1": 4,
        "spacing3": 4,
        "lmargin1": 10,
        "lmargin2": 10,
    },
    "p": {"font": ("TkDefaultFont", 10)},
    "list": {"lmargin1": 20, "lmargin2": 30, "spacing1": 2},
    "olist": {"lmargin1": 20, "lmargin2": 30, "spacing1": 2},
    "blockquote": {
        "lmargin1": 20,
        "lmargin2": 20,
        "foreground": "#555555",
        "font": ("TkDefaultFont", 10, "italic"),
        "background": "#F0F5F0",
    },
    "table_header": {
        "font": ("TkDefaultFont", 10, "bold"),
        "background": "#E0E0E0",
    },
    "table_cell": {"font": ("TkDefaultFont", 9)},
    "separator": {"foreground": "#CCCCCC"},
    "strikethrough": {"overstrike": True},
}


class _Builder(HTMLParser):
    """Parse HTML et insere dans un Text avec tags."""

    def __init__(self, tw, on_external_link, on_anchor_link):
        super().__init__(convert_charrefs=True)
        self.tw = tw
        self._on_url = on_external_link
        self._on_anchor = on_anchor_link
        self.tags = []
        self.buf = ""
        self._in_pre = False
        self._in_table = False
        self._list_stack = []
        self._heading_anchor_ids = set()
        self._ol_n = []
        self._heading_id = None
        self._link_n = 0
        self._anchor_n = 0

        for name, cfg in TAG_CONFIGS.items():
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

    def handle_starttag(self, tag, attrs):
        d = dict(attrs)
        if tag == "div" and d.get("class") == "toc":
            return
        if tag in ("h1", "h2", "h3", "h4", "h5", "h6"):
            self._flush()
            self._heading_id = d.get("id")
            self.tags.append(tag)
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
            self._emit("\n" + "\u2500" * 60 + "\n", ["separator"])
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
                self._emit(indent + "\u2022 ", ["list"])
            self.tags.append("list" if parent != "ol" else "olist")
            return
        # Links
        if tag == "a":
            href = d.get("href") or ""
            if href.startswith("#"):
                self._anchor_n += 1
                anchor_id = href[1:]
                tagname = f"at_{self._anchor_n}"
                self.tw.tag_configure(tagname, foreground="#0066CC", underline=True)

                # Capturer anchor_id par valeur via default arg
                def _on_click_anchor(_e, _aid=anchor_id):
                    self._on_anchor(_aid)

                self.tw.tag_bind(tagname, "<Button-1>", _on_click_anchor)
                self._flush()
                self.tags.append(tagname)
            else:
                self._link_n += 1
                tagname = f"ln_{self._link_n}"
                self.tw.tag_configure(
                    tagname, foreground="#0000EE", underline=True, cursor="hand2"
                )

                # Capturer href par valeur via default arg
                def _on_click_link(_e, _url=href):
                    self._on_url(_url)

                self.tw.tag_bind(tagname, "<Button-1>", _on_click_link)
                self._flush()
                self.tags.append(tagname)
            return
        if tag == "img":
            src = d.get("src") or ""
            alt = d.get("alt") or "image"
            self._flush()
            self._emit("[Image: " + alt + "](" + src + ")", ["italic"])
            return
        # Tables
        if tag == "table":
            self._flush()
            self._in_table = True
            self._emit("\n")
            return
        if tag == "tr":
            self._flush()
            return
        if tag in ("th", "td"):
            self._flush()
            self.tags.append("table_header" if tag == "th" else "table_cell")
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
                try:
                    self.tw.mark_set("anchor_" + self._heading_id, "end-1c")
                    self._heading_anchor_ids.add(self._heading_id)
                except Exception:
                    pass
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
        # Tables
        if tag in ("th", "td"):
            self._flush()
            if self.tags:
                self.tags.pop()
            self._emit("  │  ")  # separateur visuel entre colonnes
            return
        if tag == "tr":
            self._flush()
            self._emit("\n")
            return
        if tag == "table":
            self._flush()
            self._in_table = False
            return
        if tag == "span":
            self._flush()
            if self.tags:
                self.tags.pop()
            return

    def handle_data(self, data):
        if self._in_table and data.strip() == "":
            return
        self.buf += data


def render_markdown_in_text(tw, md_src, on_external_link=None, on_anchor_link=None):
    """Affiche le markdown dans un widget Text."""
    if on_external_link is None:

        def _ext_link(url):
            try:
                webbrowser.open_new_tab(url)
            except Exception:
                pass

        on_external_link = _ext_link
    if on_anchor_link is None:

        def _anchor(aid):
            try:
                mark = "anchor_" + aid
                if mark in tw.mark_names():
                    tw.see(mark)
                else:
                    try:
                        found = None
                        for mn in tw.mark_names():
                            if mn.startswith("anchor_"):
                                hid = mn[7:]
                                if (
                                    aid == hid
                                    or aid.startswith(hid)
                                    or hid.startswith(aid)
                                ):
                                    found = mn
                                    break
                        if found:
                            tw.see(found)
                    except Exception:
                        pass
            except Exception:
                pass

        on_anchor_link = _anchor

    tw.configure(state="normal")
    tw.delete("1.0", "end")

    html = md_lib.markdown(
        md_src,
        extensions=["extra", "smarty", "sane_lists", "toc"],
        extension_configs={"toc": {"anchorlink": False, "permalink": False}},
    )

    p = _Builder(tw, on_external_link, on_anchor_link)
    p.feed(html)

    # Garder state="normal" pour que les liens (tag_bind) fonctionnent
    tw.configure(state="normal")
