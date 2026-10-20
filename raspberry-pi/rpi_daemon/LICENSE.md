# Licence de rpi_daemon

Le demon Raspberry Pi (ce dossier, `rpi_daemon/`) est distribue sous **GPL-2.0-or-later** (texte : `LICENSE`).

Pourquoi : il est lie a la bibliotheque rpi-rgb-led-matrix de Henner Zeller (GPL-2.0-or-later), ce qui impose la GPL a l'ensemble
du binaire. Le reste du depot (firmware ESP32, toolkit) reste sous la licence MIT du depot ; seul ce dossier differe.
Les sources completes du demon sont dans ce depot : un binaire precompile peut donc etre distribue.

Composants tiers : rpi-rgb-led-matrix (GPL-2.0+), Adafruit GFX / TomThumb (BSD, `src/display/LICENSE-Adafruit-GFX.txt`),
cpp-httplib (MIT), giflib, libpng, zlib (licences permissives).