# -*- coding: utf-8 -*-
"""dmd_update.latest_release : chaque canal ne voit que SES releases (alpha = pre-releases, stable = releases normales), meme si le numero de l'autre canal est plus eleve. Sans reseau."""
import io
import json
import os
import sys
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dmd_update as u  # noqa: E402


def asset(n):
    return {"name": n, "browser_download_url": "https://x/" + n}


def rel(tag, pre):
    return {"tag_name": tag, "draft": False, "prerelease": pre, "assets": [asset(u.ASSET), asset(u.ASSET + ".sha256")]}


RELS = [rel("pi-v3.96", False), rel("pi-v3.95", True), rel("pi-v3.84", False), rel("pi-v3.94", True)]


class R(io.BytesIO):
    def __enter__(self):
        return self

    def __exit__(self, *a):
        return False


urllib.request.urlopen = lambda req, timeout=None: R(json.dumps(RELS).encode())
ok = True
for label, got, want in (("alpha", u.latest_release(alpha=True), "3.95"), ("stable", u.latest_release(alpha=False), "3.96")):
    good = got is not None and got[0] == want
    print(("ok     " if good else "ECHEC  ") + "%s : %s (attendu %s)" % (label, got[0] if got else None, want))
    ok = ok and good
RELS[:] = [r for r in RELS if not r["prerelease"]]
good = u.latest_release(alpha=True) is None
print(("ok     " if good else "ECHEC  ") + "alpha sans pre-release : aucune mise a jour (jamais un stable)")
ok = ok and good
print("OK" if ok else "ECHEC")
sys.exit(0 if ok else 1)
