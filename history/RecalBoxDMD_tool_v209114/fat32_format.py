# -*- coding: utf-8 -*-
"""Formatage FAT32 integre (toute taille de carte, y compris > 32 Go que Windows refuse de formater en FAT32).

Windows (Format-Volume, diskpart, « format ») plafonne le FAT32 a 32 Go : une carte de 64 Go ou plus est livree en exFAT et l'utilisateur ne
peut pas la reformater sans outil tiers. Ce module ecrit lui-meme les structures FAT32 (secteur de boot + copie, FSInfo, 2 FAT, racine) sur le
VOLUME d'une carte amovible (formatage rapide : la zone de donnees n'est pas effacee).

Securite (le formatage est destructif) :
  * l'ecriture se fait dans un processus ADMINISTRATEUR relance par le toolkit (UAC), comme l'ecriture de la carte du Pi ;
  * la tache nomme la lettre, la taille et le modele CONFIRMES par l'utilisateur ; le processus eleve les re-controle et refuse si le support
    n'est pas un disque amovible (USB / SD / MMC), si c'est le disque systeme, si la taille ou le modele ont change ;
  * le volume est verrouille (FSCTL_LOCK_VOLUME) : refus si un programme l'utilise encore.

Testable sans carte : format_fat32() ecrit dans n'importe quel objet qui a write_at(offset, octets) (ex. un fichier image)."""
from __future__ import annotations

import json
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

SECTOR_DEFAULT = 512
RESERVED_SECTORS = 32
NUM_FATS = 2
ROOT_CLUSTER = 2
FSINFO_SECTOR = 1
BACKUP_BOOT_SECTOR = 6
ALLOWED_BUS = ("USB", "SD", "MMC")
MIN_VOLUME_BYTES = 1000 ** 3            # en dessous : pas une carte du DMD
MAX_VOLUME_BYTES = 2 * 1000 ** 4        # limite pratique du FAT32 (2 To avec secteurs de 512 o)
JOB_FLAG = "--fat32-format-job"


# ---------------------------------------------------------------------------------------------------------------- structures FAT32
def cluster_sectors(total_bytes: int, bps: int = SECTOR_DEFAULT) -> int:
    """Secteurs par cluster : 4 Ko jusqu'a 8 Go, 8 Ko jusqu'a 16 Go, 16 Ko jusqu'a 32 Go, 32 Ko au-dela (valeurs de Windows / guiformat)."""
    gib = total_bytes / (1024 ** 3)
    size = 4096 if gib <= 8 else 8192 if gib <= 16 else 16384 if gib <= 32 else 32768
    return max(1, size // bps)


def fat_size_sectors(total_sectors: int, spc: int, bps: int = SECTOR_DEFAULT) -> int:
    """Formule de la specification Microsoft (FAT32) : nombre de secteurs d'UNE FAT."""
    t1 = total_sectors - RESERVED_SECTORS
    t2 = (256 * spc + NUM_FATS) // 2 if bps == 512 else ((bps // 2) * spc + NUM_FATS) // 2
    return (t1 + t2 - 1) // t2


def build_boot_sector(total_sectors: int, hidden: int, label: str, volume_id: int, spc: int, fat_sz: int, bps: int = SECTOR_DEFAULT) -> bytes:
    b = bytearray(bps)
    b[0:3] = b"\xEB\x58\x90"
    b[3:11] = b"MSWIN4.1"
    struct.pack_into("<HBHBHHBHHHII", b, 11,
                     bps, spc, RESERVED_SECTORS, NUM_FATS, 0, 0, 0xF8, 0, 63, 255, hidden & 0xFFFFFFFF, total_sectors)
    struct.pack_into("<IHHIHH", b, 36, fat_sz, 0, 0, ROOT_CLUSTER, FSINFO_SECTOR, BACKUP_BOOT_SECTOR)
    b[64] = 0x80                   # numero de lecteur
    b[66] = 0x29                   # signature de boot etendue
    struct.pack_into("<I", b, 67, volume_id & 0xFFFFFFFF)
    b[71:82] = label.upper().encode("ascii", "replace")[:11].ljust(11, b" ")
    b[82:90] = b"FAT32   "
    # petit code de boot : affiche rien, boucle (disque non amorcable)
    b[90:92] = b"\xEB\xFE"
    b[510:512] = b"\x55\xAA"
    return bytes(b)


def build_fsinfo(free_clusters: int, next_free: int, bps: int = SECTOR_DEFAULT) -> bytes:
    b = bytearray(bps)
    struct.pack_into("<I", b, 0, 0x41615252)
    struct.pack_into("<I", b, 484, 0x61417272)
    struct.pack_into("<II", b, 488, free_clusters & 0xFFFFFFFF, next_free)
    struct.pack_into("<I", b, 508, 0xAA550000)
    return bytes(b)


def plan(total_bytes: int, bps: int = SECTOR_DEFAULT) -> dict:
    """Parametres du volume : secteurs, cluster, FAT. Leve ValueError si la taille ne convient pas au FAT32."""
    total_sectors = total_bytes // bps
    if total_sectors >= 2 ** 32:
        raise ValueError("volume trop grand pour le FAT32 (secteurs sur 32 bits)")
    spc = cluster_sectors(total_bytes, bps)
    fat_sz = fat_size_sectors(total_sectors, spc, bps)
    data_sectors = total_sectors - RESERVED_SECTORS - NUM_FATS * fat_sz
    clusters = data_sectors // spc
    if clusters < 65525 or clusters > 0x0FFFFFF0:
        raise ValueError(f"nombre de clusters {clusters} hors limites FAT32")
    return {"total_sectors": total_sectors, "spc": spc, "fat_sz": fat_sz, "clusters": clusters, "bps": bps}


def format_fat32(dev, total_bytes: int, hidden: int, label: str, log=None, bps: int = SECTOR_DEFAULT) -> dict:
    """Ecrit un systeme de fichiers FAT32 vide sur `dev` (objet avec write_at(offset, data)). Formatage rapide : la zone de donnees est conservee."""
    log = log or (lambda s: None)
    p = plan(total_bytes, bps)
    spc, fat_sz = p["spc"], p["fat_sz"]
    vol_id = int(time.time()) & 0xFFFFFFFF
    boot = build_boot_sector(p["total_sectors"], hidden, label, vol_id, spc, fat_sz, bps)
    fsinfo = build_fsinfo(p["clusters"] - 1, 3, bps)
    sec2 = bytearray(bps)
    sec2[510:512] = b"\x55\xAA"
    log(f"FAT32 : {p['total_sectors']} secteurs, cluster {spc * bps // 1024} Ko, {p['clusters']} clusters, FAT de {fat_sz} secteurs")
    # 1) zone reservee mise a zero (efface aussi un ancien secteur de boot exFAT : 24 secteurs), puis secteurs de boot / FSInfo / copies
    dev.write_at(0, bytes(RESERVED_SECTORS * bps))
    for base in (0, BACKUP_BOOT_SECTOR):
        dev.write_at(base * bps, boot)
        dev.write_at((base + 1) * bps, fsinfo)
        dev.write_at((base + 2) * bps, bytes(sec2))
    # 2) les deux FAT : mises a zero par blocs, puis les 3 premieres entrees
    fat_bytes = fat_sz * bps
    zero = bytes(min(fat_bytes, 4 * 1024 * 1024))
    for f in range(NUM_FATS):
        start = (RESERVED_SECTORS + f * fat_sz) * bps
        pos = 0
        while pos < fat_bytes:
            n = min(len(zero), fat_bytes - pos)
            dev.write_at(start + pos, zero[:n])
            pos += n
        first = bytearray(bps)       # 1er secteur de la FAT (ecriture alignee sur le secteur)
        first[0:12] = struct.pack("<III", 0x0FFFFFF8, 0xFFFFFFFF, 0x0FFFFFFF)   # media, fin de chaine, racine (cluster 2) = 1 cluster
        dev.write_at(start, bytes(first))
    # 3) racine : un cluster vide contenant l'etiquette de volume
    root_off = (RESERVED_SECTORS + NUM_FATS * fat_sz) * bps
    root = bytearray(spc * bps)
    root[0:11] = label.upper().encode("ascii", "replace")[:11].ljust(11, b" ")
    root[11] = 0x08
    dev.write_at(root_off, bytes(root))
    log("structures FAT32 ecrites")
    return p


# ---------------------------------------------------------------------------------------------------------------- acces au volume (Windows)
class _VolumeWriter:
    """Poignee de VOLUME (\\\\.\\X:) verrouillee et demontee ; ecritures alignees sur les secteurs."""

    def __init__(self, letter: str):
        import ctypes
        from ctypes import wintypes
        self.ct = ctypes
        self.k32 = ctypes.WinDLL("kernel32", use_last_error=True)
        k = self.k32
        k.CreateFileW.restype = wintypes.HANDLE
        k.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
        k.DeviceIoControl.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD,
                                      ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
        k.SetFilePointerEx.argtypes = [wintypes.HANDLE, ctypes.c_longlong, ctypes.POINTER(ctypes.c_longlong), wintypes.DWORD]
        k.WriteFile.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
        k.CloseHandle.argtypes = [wintypes.HANDLE]
        h = k.CreateFileW(f"\\\\.\\{letter.rstrip(':').upper()}:", 0xC0000000, 3, None, 3, 0, None)   # GENERIC_READ|WRITE, SHARE_READ|WRITE, OPEN_EXISTING
        if h in (None, wintypes.HANDLE(-1).value):
            raise OSError(f"ouverture du volume {letter} impossible (erreur {ctypes.get_last_error()})")
        self.h = h
        self._locked = False

    def _ioctl(self, code: int, out_size: int = 0):
        buf = self.ct.create_string_buffer(out_size) if out_size else None
        from ctypes import wintypes
        ret = wintypes.DWORD(0)
        ok = self.k32.DeviceIoControl(self.h, code, None, 0, buf, out_size, self.ct.byref(ret), None)
        return ok, (buf.raw[:ret.value] if buf is not None else b"")

    def geometry(self) -> tuple[int, int, int]:
        """(taille en octets, octets par secteur, secteurs caches = debut de la partition en secteurs)."""
        ok, g = self._ioctl(0x70000, 32)                 # IOCTL_DISK_GET_DRIVE_GEOMETRY
        bps = struct.unpack_from("<I", g, 20)[0] if ok and len(g) >= 24 else SECTOR_DEFAULT
        ok, ln = self._ioctl(0x7405C, 8)                 # IOCTL_DISK_GET_LENGTH_INFO
        if not ok:
            raise OSError("taille du volume illisible")
        size = struct.unpack("<q", ln[:8])[0]
        ok, pi = self._ioctl(0x70048, 144)               # IOCTL_DISK_GET_PARTITION_INFO_EX
        start = struct.unpack_from("<q", pi, 8)[0] if ok and len(pi) >= 16 else 0
        return size, bps, start // bps

    def lock(self):
        for _ in range(10):
            ok, _o = self._ioctl(0x00090018)             # FSCTL_LOCK_VOLUME
            if ok:
                self._locked = True
                break
            time.sleep(0.5)
        if not self._locked:
            raise OSError("volume utilise par un autre programme (fermez l'Explorateur / les fenetres de la carte) puis recommencez")
        self._ioctl(0x00090020)                           # FSCTL_DISMOUNT_VOLUME

    def write_at(self, offset: int, data: bytes):
        from ctypes import wintypes
        if offset % SECTOR_DEFAULT or len(data) % SECTOR_DEFAULT:
            raise ValueError("ecriture non alignee sur les secteurs")
        if not self.k32.SetFilePointerEx(self.h, offset, None, 0):
            raise OSError(f"positionnement impossible (erreur {self.ct.get_last_error()})")
        done = wintypes.DWORD(0)
        pos = 0
        while pos < len(data):
            chunk = data[pos:pos + 1024 * 1024]
            if not self.k32.WriteFile(self.h, chunk, len(chunk), self.ct.byref(done), None) or done.value != len(chunk):
                raise OSError(f"ecriture impossible a l'octet {offset + pos} (erreur {self.ct.get_last_error()})")
            pos += len(chunk)

    def close(self):
        try:
            if self._locked:
                self._ioctl(0x0009001C)                   # FSCTL_UNLOCK_VOLUME
        finally:
            self.k32.CloseHandle(self.h)


# ---------------------------------------------------------------------------------------------------------------- tache elevee
def _drive_info(letter: str) -> dict:
    """Disque physique du volume : numero, modele, taille, bus, systeme (PowerShell Get-Partition / Get-Disk)."""
    import RecalBoxDMD_pi as pi
    l = letter.rstrip(":").upper()
    out = pi._ps(f"$p=Get-Partition -DriveLetter {l}; $d=Get-Disk -Number $p.DiskNumber; "
                 "[pscustomobject]@{Number=$d.Number;Model=$d.FriendlyName;Size=[int64]$d.Size;Bus=$d.BusType.ToString();IsBoot=$d.IsBoot;IsSystem=$d.IsSystem;"
                 "Style=$d.PartitionStyle.ToString();PartSize=[int64]$p.Size} | ConvertTo-Json -Compress")
    return json.loads(out)


def describe_drive(letter: str) -> dict | None:
    """Pour la confirmation de l'utilisateur : modele / taille du disque, ou None si le lecteur n'est pas un support amovible formatable."""
    try:
        d = _drive_info(letter)
    except Exception:   # noqa: BLE001
        return None
    if d.get("IsBoot") or d.get("IsSystem") or int(d.get("Number", 0)) == 0 or str(d.get("Bus", "")).upper() not in ALLOWED_BUS:
        return None
    return d


def write_job(job: dict) -> Path:
    import RecalBoxDMD_pi as pi
    p = pi.jobs_dir() / f"fat32_{int(time.time())}_{os.getpid()}.json"
    p.write_text(json.dumps(job), encoding="utf-8")
    return p


def launch_elevated(job_path: Path) -> bool:
    """Relance le toolkit en administrateur (UAC) avec --fat32-format-job <fichier>. False si l'utilisateur refuse."""
    import ctypes
    if getattr(sys, "frozen", False):
        exe, params = sys.executable, f'{JOB_FLAG} "{job_path}"'
    else:
        exe, params = sys.executable, f'"{Path(__file__).resolve().parent / "run_gui.py"}" {JOB_FLAG} "{job_path}"'
    return ctypes.windll.shell32.ShellExecuteW(None, "runas", exe, params, None, 0) > 32


def read_progress(job_path: Path):
    """(lignes du journal, fini?, resultat) : le processus eleve ecrit <job>.log et <job>.done (« OK » ou « ERR: ... »)."""
    lg, dn = Path(str(job_path) + ".log"), Path(str(job_path) + ".done")
    lines = lg.read_text(encoding="utf-8", errors="replace").splitlines() if lg.is_file() else []
    res = dn.read_text(encoding="utf-8").strip() if dn.is_file() else None
    return lines, res is not None, res


def helper_main(job_path: str) -> int:
    """Point d'entree du processus ELEVE."""
    import RecalBoxDMD_pi as pi
    jp = Path(job_path)
    lg, dn = Path(str(jp) + ".log"), Path(str(jp) + ".done")

    def log(s):
        with open(lg, "a", encoding="utf-8") as f:
            f.write(str(s).replace("\n", " ") + "\n")
    try:
        job = json.loads(jp.read_text(encoding="utf-8"))
        try:
            jp.unlink()
        except OSError:
            pass
        letter = job["letter"].rstrip(":").upper()
        label = (job.get("label") or "RECALBOXDMD")[:11]
        if not pi.is_admin():
            raise RuntimeError("droits administrateur requis")
        d = _drive_info(letter)
        log(f"cible : {letter}: disque {d['Number']} {d['Model']} {int(d['Size']):,} octets ({d['Bus']})".replace(",", " "))
        if int(d["Number"]) == 0 or d.get("IsBoot") or d.get("IsSystem"):
            raise RuntimeError("refus : disque systeme")
        if str(d["Bus"]).upper() not in ALLOWED_BUS:
            raise RuntimeError(f"refus : le disque n'est pas un support amovible ({d['Bus']})")
        if str(d["Model"]).strip() != job["expected_model"] or int(d["Size"]) != int(job["expected_size"]):
            raise RuntimeError("refus : le disque n'est plus celui qui a ete confirme (modele ou taille differents)")
        if os.environ.get("SystemDrive", "C:").rstrip(":").upper() == letter:
            raise RuntimeError("refus : lecteur systeme")
        if not (MIN_VOLUME_BYTES <= int(d["Size"]) <= MAX_VOLUME_BYTES):
            raise RuntimeError("refus : taille de disque inattendue")
        log("PCT 10")
        vol = _VolumeWriter(letter)
        try:
            size, bps, hidden = vol.geometry()
            if abs(size - int(d["PartSize"])) > 1024 * 1024:
                raise RuntimeError("refus : taille du volume incoherente")
            vol.lock()
            log("volume verrouille")
            log("PCT 30")
            format_fat32(vol, size, hidden, label, log, bps)
        finally:
            vol.close()
        log("PCT 90")
        if str(d.get("Style", "")).upper() == "MBR":      # type de partition « FAT32 LBA » (0x0C) : une carte exFAT portait 0x07
            try:
                pi._ps(f"Set-Partition -DriveLetter {letter} -MbrType 12")
            except Exception as e:   # noqa: BLE001
                log(f"type de partition non modifie ({e}) : sans consequence")
        log("PCT 100")
        dn.write_text("OK", encoding="utf-8")
        return 0
    except Exception as e:   # noqa: BLE001
        log(f"ERREUR: {type(e).__name__}: {e}")
        dn.write_text(f"ERR: {type(e).__name__}: {e}", encoding="utf-8")
        return 1
