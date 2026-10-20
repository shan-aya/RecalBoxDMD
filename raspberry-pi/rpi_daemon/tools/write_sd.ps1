# write_sd.ps1 -- ecrit Raspberry Pi OS Lite sur une carte SD (Windows, PowerShell 5.1)
# et prepare le premier demarrage sans ecran (SSH, WiFi, utilisateur, nom d'hote).
#
# DESTRUCTIF : efface tout le disque cible. Garde-fous : disque USB, modele et taille attendus,
# jamais le disque systeme, confirmation tapee. Les mots de passe sont saisis dans la fenetre
# (jamais en parametre ni dans un fichier du depot).
#
# Usage : .\write_sd.ps1 -DiskNumber 8            (s'eleve tout seul en administrateur)
#         .\write_sd.ps1 -DiskNumber 8 -DryRun    (controles + apercu, n'ecrit rien)
param(
    [Parameter(Mandatory = $true)][int]$DiskNumber,
    [string]$ImagePath = 'F:\RPI_PORT_WORK\img\2026-10-06-raspios-bookworm-armhf-lite.img',
    [string]$Hostname = 'dmdpi',
    [string]$UserName = 'pi',
    [string]$ExpectedModel = 'SanDisk SD',
    [uint64]$ExpectedSize = 15931539456,
    [string]$WifiCountry = 'FR',
    [switch]$DryRun,
    [switch]$SkipWrite   # reprise : l'image est deja ecrite, ne refait que la verification et les fichiers de boot
)
$ErrorActionPreference = 'Stop'

function Test-Admin {
    $id = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
    return $id.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}
function Read-Plain([string]$prompt) {
    $s = Read-Host -Prompt $prompt -AsSecureString
    $b = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($s)
    try { return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($b) } finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($b) }
}

# Hash SHA-512 crypt du mot de passe. Ne PAS piper la chaine vers openssl (PowerShell ajoute un CR,
# et un flux .NET donne aussi un hash faux) : fichier temporaire sans fin de ligne, supprime aussitot.
function Get-Sha512Crypt([string]$ossl, [string]$plain, [string]$salt = '') {
    $f = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName())
    try {
        [IO.File]::WriteAllBytes($f, [Text.Encoding]::UTF8.GetBytes($plain))
        $a = @('passwd', '-6')
        if ($salt) { $a += @('-salt', $salt) }
        $a += @('-in', $f)
        $h = (& $ossl @a | Out-String).Trim()
    } finally { if (Test-Path $f) { Remove-Item $f -Force } }
    if ($h -notmatch '^\$6\$') { throw 'Hash du mot de passe invalide.' }
    return $h
}

if (-not $DryRun -and -not (Test-Admin)) {
    Write-Host 'Elevation administrateur demandee (UAC)...'
    $a = "-NoProfile -ExecutionPolicy Bypass -NoExit -File `"$PSCommandPath`" -DiskNumber $DiskNumber -ImagePath `"$ImagePath`" -Hostname $Hostname -UserName $UserName -ExpectedModel `"$ExpectedModel`" -ExpectedSize $ExpectedSize -WifiCountry $WifiCountry"
    if ($SkipWrite) { $a += ' -SkipWrite' }
    Start-Process powershell -Verb RunAs -ArgumentList $a
    exit
}

# --- Garde-fous sur le disque cible ---
if (-not (Test-Path $ImagePath)) { throw "Image introuvable : $ImagePath" }
$disk = Get-Disk -Number $DiskNumber
Write-Host ("Cible : disque {0} - {1} - {2} - {3:N0} octets - table {4}" -f $disk.Number, $disk.FriendlyName, $disk.BusType, $disk.Size, $disk.PartitionStyle)
if ($disk.Number -eq 0 -or $disk.IsBoot -or $disk.IsSystem) { throw 'Refus : disque systeme.' }
if ($disk.BusType -ne 'USB') { throw "Refus : le disque n'est pas USB ($($disk.BusType))." }
if ($disk.FriendlyName -notlike "$ExpectedModel*") { throw "Refus : modele '$($disk.FriendlyName)' different de '$ExpectedModel'." }
if ([uint64]$disk.Size -ne $ExpectedSize) { throw "Refus : taille $($disk.Size) differente de $ExpectedSize." }
$img = Get-Item $ImagePath
if ($img.Length % 512 -ne 0) { throw 'Image : taille non multiple de 512.' }
Get-Partition -DiskNumber $DiskNumber -ErrorAction SilentlyContinue | ForEach-Object {
    $v = Get-Volume -Partition $_ -ErrorAction SilentlyContinue
    Write-Host ("  partition {0} lettre {1} etiquette '{2}' {3:N0} octets" -f $_.PartitionNumber, $_.DriveLetter, $v.FileSystemLabel, $_.Size)
}

# --- Saisies (hors depot) ---
if ($DryRun) {
    $ssid = 'SSID_TEST'; $psk = 'PSK_TEST'; $pwHash = '$6$dryrun$hash'
} else {
    $ssid = Read-Host -Prompt 'SSID WiFi 2,4 GHz'
    $psk = Read-Plain 'Mot de passe WiFi'
    $p1 = Read-Plain "Mot de passe pour l'utilisateur $UserName sur le Pi"
    $p2 = Read-Plain 'Confirmer ce mot de passe'
    if ($p1 -ne $p2 -or $p1.Length -lt 6) { throw 'Mots de passe differents ou trop courts (6 mini).' }
    $ossl = @(
        (Get-Command openssl -ErrorAction SilentlyContinue).Source,
        'C:\Program Files\Git\usr\bin\openssl.exe',
        (Join-Path $env:USERPROFILE 'scoop\apps\git\current\usr\bin\openssl.exe')
    ) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
    if (-not $ossl) { throw 'openssl (Git for Windows) introuvable.' }
    $pwHash = Get-Sha512Crypt $ossl $p1
    $p1 = $null; $p2 = $null
}

$toml = @"
config_version = 1

[system]
hostname = "$Hostname"

[user]
name = "$UserName"
password = "$pwHash"
password_encrypted = true

[ssh]
enabled = true
password_authentication = true

[wlan]
ssid = "$ssid"
password = "$psk"
password_encrypted = false
hidden = false
country = "$WifiCountry"

[locale]
keymap = "fr"
timezone = "Europe/Paris"
"@

if ($DryRun) {
    Write-Host '--- DryRun : controles OK, rien ecrit. Apercu custom.toml (valeurs factices) ---'
    Write-Host $toml
    return
}

$bufSize = 4MB
$buf = New-Object byte[] $bufSize
if (-not $SkipWrite) {
Write-Host ''
Write-Host "ATTENTION : tout le contenu du disque $DiskNumber ($($disk.FriendlyName)) va etre EFFACE."
$c = Read-Host -Prompt 'Tapez OUI en majuscules pour continuer'
if ($c -ne 'OUI') { Write-Host 'Annule.'; return }

# --- Effacement + ecriture brute ---
Write-Host 'Effacement du disque...'
Clear-Disk -Number $DiskNumber -RemoveData -RemoveOEM -Confirm:$false
Start-Sleep -Seconds 2

Write-Host 'Ecriture de l image...'
$src = [IO.File]::OpenRead($ImagePath)
$dst = New-Object IO.FileStream("\\.\PhysicalDrive$DiskNumber", [IO.FileMode]::Open, [IO.FileAccess]::Write, [IO.FileShare]::ReadWrite, 1MB, [IO.FileOptions]::WriteThrough)
try {
    $total = $src.Length; $done = [long]0
    while (($n = $src.Read($buf, 0, $bufSize)) -gt 0) {
        $w = [int]([math]::Ceiling($n / 512.0) * 512)
        if ($w -gt $n) { [Array]::Clear($buf, $n, $w - $n) }
        $dst.Write($buf, 0, $w)
        $done += $n
        Write-Progress -Activity 'Ecriture' -Status ('{0:N0} / {1:N0} Mo' -f ($done / 1MB), ($total / 1MB)) -PercentComplete ($done * 100 / $total)
    }
    $dst.Flush()
} finally { $dst.Close(); $src.Close() }
Write-Progress -Activity 'Ecriture' -Completed
}

# --- Relecture et comparaison SHA-256 ---
Write-Host 'Verification (relecture complete)...'
$hSrc = (Get-FileHash $ImagePath -Algorithm SHA256).Hash
$rd = New-Object IO.FileStream("\\.\PhysicalDrive$DiskNumber", [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
$inc = [Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA256)
try {
    $left = $img.Length
    while ($left -gt 0) {
        $want = [int][math]::Min([long]$bufSize, [long]$left)
        $got = $rd.Read($buf, 0, $want)
        if ($got -le 0) { throw 'Lecture interrompue.' }
        $inc.AppendData($buf, 0, $got); $left -= $got
    }
} finally { $rd.Close() }
$hDst = -join ($inc.GetHashAndReset() | ForEach-Object { $_.ToString('X2') })
if ($hSrc -ne $hDst) { throw "Verification ECHOUEE : $hSrc <> $hDst" }
Write-Host "Verification OK ($hDst)"

# --- Fichiers de premier demarrage sur la partition boot ---
Update-Disk -Number $DiskNumber
Start-Sleep -Seconds 3
$boot = Get-Partition -DiskNumber $DiskNumber | Where-Object { $_.Size -lt 1GB -and $_.Type -ne 'Reserved' } | Select-Object -First 1
if (-not $boot) { throw 'Partition boot introuvable.' }
if (-not $boot.DriveLetter) { Add-PartitionAccessPath -DiskNumber $DiskNumber -PartitionNumber $boot.PartitionNumber -AssignDriveLetter; $boot = Get-Partition -DiskNumber $DiskNumber -PartitionNumber $boot.PartitionNumber }
$bl = "$($boot.DriveLetter):\"
if (-not (Test-Path (Join-Path $bl 'cmdline.txt'))) { throw "cmdline.txt absent de $bl (mauvaise partition ?)" }
[IO.File]::WriteAllText((Join-Path $bl 'custom.toml'), ($toml -replace "`r`n", "`n"), (New-Object Text.UTF8Encoding($false)))
[IO.File]::WriteAllText((Join-Path $bl 'ssh'), '')
Write-Host "custom.toml et ssh ecrits sur $bl"
Write-Host 'TERMINE. Ejectez la carte, mettez-la dans le Pi (alimente), attendez ~2 a 3 minutes au premier demarrage.'
