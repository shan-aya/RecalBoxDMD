#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Wrapper modulaire (compatibilité)

Objectif :
- Lancer l'application Tkinter existante via le monolithe
- Éviter les circular imports en ne déclenchant JAMAIS d'import depuis
  dmd_gif_converter_modulaire/__init__.py
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

os.environ.setdefault("PYTHONIOENCODING", "utf-8")
try:
    sys.stdout.reconfigure(encoding="utf-8")  # type: ignore[attr-defined]
    sys.stderr.reconfigure(encoding="utf-8")  # type: ignore[attr-defined]
except Exception:
    pass

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from dmd_converter import (
    DMDConverter,
)


def main() -> None:
    app = DMDConverter()
    app.run()


if __name__ == "__main__":
    main()
