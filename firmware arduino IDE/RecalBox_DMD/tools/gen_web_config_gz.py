"""Regenere web_config_html_gz.h depuis les blocs WEB_CONFIG_*_HTML
de web_config.h (gzip + tableau C uint8_t PROGMEM). A relancer apres CHAQUE
modification du HTML/JS source -- voir compile.ps1/compile_and_merge.bat qui
l'appellent automatiquement.
"""
import gzip
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "web_config.h"
DST = ROOT / "web_config_html_gz.h"

src = SRC.read_text(encoding="utf-8")


def extract(name):
    pat = re.compile(
        r'static const char ' + re.escape(name) + r'\[\] PROGMEM = R"rawliteral\((.*?)\)rawliteral";',
        re.DOTALL,
    )
    m = pat.search(src)
    if not m:
        raise SystemExit(f"ERREUR: bloc {name} introuvable dans web_config.h")
    return m.group(1)


def to_c_array(data: bytes, name: str) -> str:
    lines = [f"static const uint8_t {name}[] PROGMEM = {{"]
    for i in range(0, len(data), 20):
        chunk = data[i:i + 20]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    lines.append(f"static const size_t {name}_LEN = {len(data)};")
    return "\n".join(lines)


page_names = [
    "WEB_CONFIG_MENU_HTML",
    "WEB_CONFIG_BASIC_HTML",
    "WEB_CONFIG_NETWORK_HTML",
    "WEB_CONFIG_CLOCK_HTML",
    "WEB_CONFIG_MEDIA_HTML",
    "WEB_CONFIG_AP_HTML",
]

parts = []
for name in page_names:
    html = extract(name)
    gz = gzip.compress(html.encode("utf-8"), compresslevel=9, mtime=0)
    parts.append(to_c_array(gz, f"{name}_GZ"))

out = [
    "// Fichier GENERE -- ne pas editer a la main.",
    "// Contenu HTML gzip-compresse des pages WEB_CONFIG_*_HTML (voir web_config.h).",
    "// IMPORTANT : regenere automatiquement par compile.ps1/compile_and_merge.bat -- si vous compilez",
    "// autrement (Arduino IDE, arduino-cli directement), relancez",
    "// tools/gen_web_config_gz.py a la main apres CHAQUE modification du HTML/JS",
    "// source dans web_config.h.",
    "",
]
out.extend(parts)
out.append("")

DST.write_text("\n".join(out), encoding="utf-8", newline="\n")

for name in page_names:
    html = extract(name)
    gz = gzip.compress(html.encode("utf-8"), compresslevel=9, mtime=0)
    print(f"{name}: {len(html)} octets -> gzip {len(gz)} octets")
print("Ecrit:", DST)
