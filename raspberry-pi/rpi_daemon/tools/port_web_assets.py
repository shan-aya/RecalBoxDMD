"""Extrait du firmware les pages de la configuration web (web_config.h) vers rpi_daemon/web/ : un fichier .html par page,
cfg.css, cfg.js (le meme bundle que le firmware) et native.js. Le demon les sert tels quels (meme interface que sur l'ESP32).

Meme regle que tools/gen_web_config_gz.py (qui produit les tableaux gzip du firmware) : les marqueurs @@CFGCSS@@ / @@CFGJS@@ des pages sont
remplaces par le hash du contenu de cfg.css / cfg.js (cache navigateur longue duree). A relancer apres chaque modification de ces pages
dans le firmware ; les fichiers generes sont commites (le Pi n'a pas besoin de ce script pour fonctionner).

Usage : python port_web_assets.py
"""
import hashlib
import re
from pathlib import Path

here = Path(__file__).resolve().parent
fw = here.parents[1]
src = (fw / "web_config.h").read_text(encoding="utf-8")
out = here.parent / "web"
out.mkdir(exist_ok=True)

shared = fw / "web_shared"
css = (shared / "cfg.css").read_bytes().replace(b"\r\n", b"\n")
js = b"".join(
    (shared / f).read_bytes().replace(b"\r\n", b"\n") + b"\n"
    for f in ("cfg.js", "cfg_help.js", "cfg_i18n.js", "cfg_i18n_extra.js")
    if (shared / f).exists()
)
css_hash = hashlib.sha1(css).hexdigest()[:8]
js_hash = hashlib.sha1(js).hexdigest()[:8]

PAGES = {  # bloc du firmware -> fichier
    "WEB_CONFIG_MENU_HTML": "menu.html",
    "WEB_CONFIG_SETTINGS_HTML": "settings.html",
    "WEB_CONFIG_BASIC_HTML": "basic.html",
    "WEB_CONFIG_PLAYLIST_HTML": "playlist.html",
    "WEB_CONFIG_NETWORK_HTML": "network.html",
    "WEB_CONFIG_CLOCK_HTML": "clock.html",
    "WEB_CONFIG_BUTTON_HTML": "button.html",
    "WEB_CONFIG_DECO_HTML": "widgets.html",
    "WEB_CONFIG_PLACE_HTML": "place.html",
    "WEB_CONFIG_EVENTS_HTML": "events.html",
    "WEB_CONFIG_QUOTES_HTML": "quotes.html",
    "WEB_CONFIG_CLOCKCOLORS_HTML": "clockcolors.html",
    "WEB_CONFIG_MEDIA_HTML": "media.html",
    "WEB_CONFIG_BRIGHT_HTML": "brightness.html",  # planificateur de luminosite  # page du point d'acces de configuration (premier demarrage / secours)
}


def block(name, delim="rawliteral"):
    m = re.search(r"static const char " + re.escape(name) + r'\[\] PROGMEM = R"' + delim + r"\((.*?)\)" + delim + '";', src, re.DOTALL)
    if not m:
        raise SystemExit(f"bloc {name} introuvable dans web_config.h")
    return m.group(1)


total = 0
for name, fn in PAGES.items():
    html = block(name).replace("@@CFGCSS@@", css_hash).replace("@@CFGJS@@", js_hash)
    (out / fn).write_text(html, encoding="utf-8", newline="\n")
    total += len(html)
(out / "cfg.css").write_bytes(css)
(out / "cfg.js").write_bytes(js)
(out / "native.js").write_text(block("NATIVE_JS", "nativejs"), encoding="utf-8", newline="\n")
print(f"{len(PAGES)} pages + cfg.css ({css_hash}) + cfg.js ({js_hash}) + native.js -> {out} ({total} octets de HTML)")
