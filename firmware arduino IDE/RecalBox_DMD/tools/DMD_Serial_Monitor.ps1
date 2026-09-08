# ============================================
# DMD_Serial_Monitor.ps1 -- Moniteur serie DMD, port auto-detecte
# ============================================
# Fenetre visible, lecture octet par octet (jamais ReadLine -- perd des
# lignes/plante sur un flux binaire ou tres rapide). Detecte automatiquement
# le port du DMD qu'il soit branche en USB (puce CP210x/CH340, port COM
# classique) ou connecte en Bluetooth SPP (nom "ESP32-GIF", voir
# bluetooth_name dans le web config du DMD) -- fonctionne sans modification
# dans les 2 cas, pas besoin de savoir a l'avance lequel est actif.
#
# v2 (2026-09-04) -- demande utilisateur ("si port USB detecte, serial
# classique complet dans le script, sinon proposition ouverture page web
# endpoint") : si AUCUN port serie n'est detecte (DMD debranche), bascule
# automatiquement sur un suivi WiFi via l'endpoint HTTP /log du firmware
# (v153+, RecalBox_DMD.ino/web_config.h) -- poll toutes les 3s, log ecrit
# pareil dans serial_logs/. Beaucoup plus limite que le Serial complet (pas
# de GIF/BOOT/etc., juste heap/RSSI/mode/tentatives connect + 10 derniers
# messages MQTT, deja le scope demande : stabilite/pertes de connexion, pas
# un diagnostic complet). Teste les 2 IP preferentielles constatees
# empiriquement sur ce reseau (192.168.0.51/.52, la Freebox alterne entre
# les 2 selon les power cycles, pas un reglage fixe cote DMD).
#
# Usage : double-clic sur DMD_Serial_Monitor.bat (ou lancer ce .ps1
# directement). Si un seul port candidat est trouve, il est utilise
# automatiquement. Si plusieurs (ou aucun) candidat evident, une liste
# numerotee de tous les ports serie disponibles est proposee.

$ErrorActionPreference = 'Stop'
$logDir = "$PSScriptRoot\serial_logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$lastPortFile = "$logDir\.last_port"

# Fallback WiFi (voir commentaire v2 ci-dessus) -- utilise uniquement si
# aucun port serie n'est detecte du tout.
function Start-WifiLogFallback {
    $candidateIPs = @('192.168.0.51', '192.168.0.52')
    Write-Host "Aucun port serie detecte -- recherche du DMD sur le reseau (IP connues : $($candidateIPs -join ', '))..." -ForegroundColor Yellow
    $dmdUri = $null
    foreach ($ip in $candidateIPs) {
        try {
            $r = Invoke-WebRequest -Uri "http://$ip/log" -UseBasicParsing -TimeoutSec 5
            if ($r.StatusCode -eq 200) { $dmdUri = "http://$ip/log"; Write-Host "DMD trouve sur $ip" -ForegroundColor Green; break }
        } catch { }
    }
    if (-not $dmdUri) {
        Write-Host "DMD introuvable (ni port serie, ni web sur $($candidateIPs -join '/')). Branche le DMD en USB, verifie l'appairage Bluetooth (ESP32-GIF), ou verifie qu'il est allume et connecte au WiFi." -ForegroundColor Red
        Read-Host "Appuie sur Entree pour fermer"
        exit 1
    }

    $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss'
    $outFile = "$logDir\dmd_wifilog_$stamp.txt"
    Write-Host "=== Suivi WiFi (/log, poll 3s) -- $dmdUri -- log: $outFile ===" -ForegroundColor Cyan
    Write-Host "(pas le Serial complet -- juste stabilite/pertes de connexion : heap/RSSI/mode/tentatives + 10 derniers messages MQTT)" -ForegroundColor DarkGray
    Write-Host "(Ctrl+C ou ferme la fenetre pour arreter)" -ForegroundColor DarkGray
    "" | Out-File -FilePath $outFile -Encoding utf8
    $lastContent = ""
    while ($true) {
        try {
            $r = Invoke-WebRequest -Uri $dmdUri -UseBasicParsing -TimeoutSec 5
            # v153 (firmware) sert desormais une mini page HTML (<meta refresh>
            # pour un navigateur ouvert en continu) au lieu de text/plain --
            # extrait le contenu de <pre>...</pre> pour un affichage propre ici.
            $raw = $r.Content
            $content = if ($raw -match '(?s)<pre>(.*?)</pre>') {
                [System.Net.WebUtility]::HtmlDecode($matches[1])
            } else {
                $raw
            }
            if ($content -ne $lastContent) {
                $ts = Get-Date -Format "HH:mm:ss.fff"
                $out = "----- $ts -----`n$content"
                Write-Host $out
                Add-Content -Path $outFile -Value $out -Encoding utf8
                $lastContent = $content
            }
        } catch {
            $ts = Get-Date -Format "HH:mm:ss.fff"
            $out = "$ts [ERREUR] DMD injoignable : $($_.Exception.Message)"
            Write-Host $out -ForegroundColor Red
            Add-Content -Path $outFile -Value $out -Encoding utf8
        }
        Start-Sleep -Seconds 3
    }
}

function Get-CandidatePorts {
    $names = [System.IO.Ports.SerialPort]::GetPortNames()
    if (-not $names) { return @() }
    $pnp = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '\((COM\d+)\)' }
    $rows = foreach ($n in $names) {
        $match = $pnp | Where-Object { $_.Name -match "\($n\)" } | Select-Object -First 1
        $desc = if ($match) { $match.Name } else { "(description inconnue)" }
        $isLikely = $desc -match 'CP210|CH340|CH910|Silicon Labs|USB-SERIAL|USB Serial|ESP32|Standard Serial over Bluetooth'
        [PSCustomObject]@{ Port = $n; Description = $desc; Likely = $isLikely }
    }
    return $rows
}

Write-Host "=== DMD Serial Monitor ===" -ForegroundColor Cyan
Write-Host "Recherche des ports serie disponibles..." -ForegroundColor DarkGray
$candidates = Get-CandidatePorts

if (-not $candidates -or $candidates.Count -eq 0) {
    Start-WifiLogFallback
    exit 0
}

$likely = @($candidates | Where-Object { $_.Likely })
$selectedPort = $null

if ($likely.Count -eq 1) {
    $selectedPort = $likely[0].Port
    Write-Host "Port detecte automatiquement : $selectedPort ($($likely[0].Description))" -ForegroundColor Green
} else {
    # Plusieurs candidats probables (ou aucun net) -- propose le dernier port
    # utilise en 1er choix par defaut s'il est toujours present, sinon liste
    # complete numerotee.
    $lastPort = if (Test-Path $lastPortFile) { (Get-Content -LiteralPath $lastPortFile -Raw).Trim() } else { $null }
    Write-Host "Plusieurs ports disponibles :" -ForegroundColor Yellow
    for ($i = 0; $i -lt $candidates.Count; $i++) {
        $c = $candidates[$i]
        $tag = if ($c.Likely) { " <- probable" } elseif ($c.Port -eq $lastPort) { " <- dernier utilise" } else { "" }
        Write-Host "  [$($i+1)] $($c.Port) - $($c.Description)$tag"
    }
    $defaultIdx = if ($lastPort) { [array]::IndexOf($candidates.Port, $lastPort) + 1 } else { 0 }
    $prompt = if ($defaultIdx -gt 0) { "Choix (Entree = $defaultIdx)" } else { "Choix" }
    $choice = Read-Host $prompt
    if ([string]::IsNullOrWhiteSpace($choice) -and $defaultIdx -gt 0) { $choice = $defaultIdx }
    $idx = 0
    if (-not [int]::TryParse($choice, [ref]$idx) -or $idx -lt 1 -or $idx -gt $candidates.Count) {
        Write-Host "Choix invalide." -ForegroundColor Red
        Read-Host "Appuie sur Entree pour fermer"
        exit 1
    }
    $selectedPort = $candidates[$idx - 1].Port
}

Set-Content -LiteralPath $lastPortFile -Value $selectedPort -Encoding utf8

$stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss'
$outFile = "$logDir\dmd_serial_$stamp.txt"
$Port = $selectedPort
$Baud = 115200

try {
    $sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, "None", 8, "One"
    $sp.ReadTimeout = 500
    $sp.Open()
    $sp.DiscardInBuffer()
    Write-Host "=== $Port @ $Baud -- log: $outFile ===" -ForegroundColor Cyan
    Write-Host "(Ctrl+C ou ferme la fenetre pour arreter)" -ForegroundColor DarkGray
    "" | Out-File -FilePath $outFile -Encoding utf8
    $lineBuf = New-Object System.Text.StringBuilder
    $writeBuf = New-Object System.Text.StringBuilder
    $lastFlush = Get-Date
    while ($true) {
        try {
            $b = $sp.ReadByte()
            if ($b -eq 10) {
                $ts = Get-Date -Format "HH:mm:ss.fff"
                $line = $lineBuf.ToString().TrimEnd("`r")
                $out = "$ts $line"
                Write-Host $out
                $writeBuf.AppendLine($out) | Out-Null
                $lineBuf.Clear() | Out-Null
            } elseif ($b -ge 0) {
                $lineBuf.Append([char]$b) | Out-Null
            }
        } catch [System.TimeoutException] { }
        if (((Get-Date) - $lastFlush).TotalMilliseconds -ge 300 -and $writeBuf.Length -gt 0) {
            Add-Content -Path $outFile -Value $writeBuf.ToString().TrimEnd("`r`n") -Encoding utf8
            $writeBuf.Clear() | Out-Null
            $lastFlush = Get-Date
        }
    }
} catch {
    Write-Host "ERREUR: $($_.Exception.GetType().FullName): $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "(Port deja ouvert par un autre programme -- Arduino IDE, un autre moniteur -- ou DMD debranche ?)" -ForegroundColor Yellow
    Read-Host "Appuie sur Entree pour fermer"
}
