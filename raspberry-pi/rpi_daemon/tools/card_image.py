"""Sauvegarde / restauration d'une carte SD complete (disque brut) dans un fichier image compresse. Outil de developpement.

    python card_image.py backup  --disk N --model "SanDisk SD  SDDR-189" --size 63864569856 --out carte.img.gz
    python card_image.py restore --disk N --model "..." --size ... --in carte.img.gz

Garde-fous : droits administrateur, disque amovible USB / SD / MMC uniquement, jamais le disque systeme ni le disque 0, modele ET taille exacts
(a recopier de la liste du toolkit), l'image de sauvegarde porte la taille d'origine et l'empreinte SHA-256 des octets bruts (fichier .sha256 a cote).
La restauration relit la carte et compare l'empreinte. Les zones de zeros (espace libre) se compressent tres bien : gzip niveau 1, rapide.
--limit-mb N : traite seulement les N premiers Mo (essai de l'outil)."""
import argparse
import gzip
import hashlib
import json
import subprocess
import sys
import time
from pathlib import Path

CHUNK = 4 * 1024 * 1024
SECTOR = 512


def ps(cmd: str) -> str:
    r = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command",
                        "[Console]::OutputEncoding=[Text.Encoding]::UTF8; $ErrorActionPreference='Stop'; " + cmd], capture_output=True, timeout=120)
    if r.returncode != 0:
        raise SystemExit((r.stderr or r.stdout).decode("utf-8", "replace")[:400])
    return r.stdout.decode("utf-8", "replace")


def check_disk(n: int, model: str, size: int) -> None:
    d = json.loads(ps(f"Get-Disk -Number {n} | Select-Object Number,FriendlyName,Size,@{{n='BusType';e={{$_.BusType.ToString()}}}},IsBoot,IsSystem | ConvertTo-Json -Compress"))
    if n == 0 or d.get("IsBoot") or d.get("IsSystem"):
        sys.exit("refus : disque systeme")
    if str(d["BusType"]).upper() not in ("USB", "SD", "MMC"):
        sys.exit(f"refus : pas un support amovible ({d['BusType']})")
    if str(d["FriendlyName"]).strip() != model or int(d["Size"]) != size:
        sys.exit(f"refus : le disque {n} est '{d['FriendlyName']}' {d['Size']} octets, pas ce qui est attendu")
    print(f"disque {n} : {d['FriendlyName']} {int(d['Size']):,} octets ({d['BusType']})".replace(",", " "), flush=True)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["backup", "restore"])
    ap.add_argument("--disk", type=int, required=True)
    ap.add_argument("--model", required=True)
    ap.add_argument("--size", type=int, required=True)
    ap.add_argument("--out")
    ap.add_argument("--in", dest="inp")
    ap.add_argument("--limit-mb", type=int, default=0)
    ap.add_argument("--target-file", help="essai : ecrire / lire ce fichier au lieu du disque (aucun controle de disque)")
    a = ap.parse_args()
    dev = a.target_file or rf"\\.\PhysicalDrive{a.disk}"
    if not a.target_file:
        check_disk(a.disk, a.model, a.size)
    total = a.size if not a.limit_mb else min(a.size, a.limit_mb * 1024 * 1024)
    t0 = time.time()
    if a.mode == "backup":
        out = Path(a.out)
        h = hashlib.sha256()
        done = 0
        with open(dev, "rb", buffering=0) as src, gzip.open(out, "wb", compresslevel=1) as dst:
            while done < total:
                n = min(CHUNK, total - done)
                b = src.read(n)
                if not b:
                    break
                b = b[:min(len(b), total - done)]
                h.update(b)
                dst.write(b)
                done += len(b)
                if (done // CHUNK) % 64 == 0:
                    print(f"  {done / 1e9:.1f} / {total / 1e9:.1f} Go  ({done / max(time.time() - t0, 1) / 1e6:.0f} Mo/s)", flush=True)
        Path(str(out) + ".sha256").write_text(f"{h.hexdigest()}  {done}  {a.model}\n", encoding="utf-8")
        print(f"sauvegarde : {done} octets -> {out} ({out.stat().st_size / 1e9:.2f} Go) sha256 {h.hexdigest()[:16]}... en {time.time() - t0:.0f} s", flush=True)
        return 0
    # restauration
    inp = Path(a.inp)
    want, want_len = Path(str(inp) + ".sha256").read_text(encoding="utf-8").split()[:2]
    want_len = int(want_len)
    if a.limit_mb == 0 and want_len != a.size:
        sys.exit("refus : l'image a ete prise sur un disque de taille differente")
    h = hashlib.sha256()
    done = 0
    with gzip.open(inp, "rb") as src, open(dev, "r+b", buffering=0) as dst:
        while True:
            b = src.read(CHUNK)
            if not b:
                break
            h.update(b)
            if len(b) % SECTOR:
                b += b"\0" * (SECTOR - len(b) % SECTOR)
            dst.write(b)
            done += len(b)
            if (done // CHUNK) % 64 == 0:
                print(f"  ecrit {done / 1e9:.1f} / {want_len / 1e9:.1f} Go", flush=True)
    if h.hexdigest() != want:
        sys.exit("l'image est corrompue (empreinte differente de celle de la sauvegarde)")
    print("relecture de la carte...", flush=True)
    h2 = hashlib.sha256()
    left = want_len
    with open(dev, "rb", buffering=0) as rd:
        while left > 0:
            n = min(CHUNK, left)
            b = rd.read(n + (-n % SECTOR))
            if not b:
                sys.exit("lecture interrompue")
            h2.update(b[:n])
            left -= n
    if h2.hexdigest() != want:
        sys.exit("VERIFICATION ECHOUEE : la carte ne contient pas l'image (carte defectueuse ?)")
    print(f"restauration verifiee : {want_len} octets en {time.time() - t0:.0f} s", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
