# ============================================
# Reboot_Cycle_Test_HTTP.ps1 -- reboots repetes via HTTP + capture serie
# ============================================
# Variante de Reboot_Cycle_Test.ps1 pour une carte qui expose un endpoint
# HTTP de reboot (le DMD principal, /reboot -- voir web_config.h) plutot
# que de re-flasher a chaque cycle. Meme protocole/duree de cycle que
# Reboot_Cycle_Test.ps1 pour une comparaison directe entre les 2 cartes
# (bascule des roles demandee par l'utilisateur, 2026-09-12 nuit).
#
# Usage : .\Reboot_Cycle_Test_HTTP.ps1 -Port COM4 -RebootUrl http://192.168.0.51/reboot -TotalHours 8 -CycleSeconds 180
param(
    [Parameter(Mandatory=$true)][string]$Port,
    [Parameter(Mandatory=$true)][string]$RebootUrl,
    [double]$TotalHours = 8,
    [int]$CycleSeconds = 180
)

$logDir = "$PSScriptRoot\serial_logs\reboot_cycles_dmdprod"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$summaryFile = "$logDir\_summary.txt"

$endTime = (Get-Date).AddHours($TotalHours)
$cycleNum = 0

while ((Get-Date) -lt $endTime) {
    $cycleNum++
    $cycleStart = Get-Date
    $ts = $cycleStart.ToString("yyyy-MM-dd_HHmmss")
    $logFile = "$logDir\cycle${cycleNum}_$ts.txt"

    Write-Host "=== Cycle $cycleNum -- reboot HTTP a $ts ===" -ForegroundColor Yellow

    $rebootOk = $false
    try {
        $r = Invoke-WebRequest -Uri $RebootUrl -TimeoutSec 5 -UseBasicParsing
        $rebootOk = $true
    } catch {
        # le serveur web coupe la connexion en repondant (ESP.restart()
        # juste apres) -- une exception de connexion FERMEE ici est le
        # comportement NORMAL attendu, pas un vrai echec.
        $rebootOk = $true
    }

    if (-not $rebootOk) {
        Add-Content -Path $summaryFile -Value "$ts cycle=$cycleNum REBOOT_HTTP_ECHEC"
        Start-Sleep -Seconds 5
        continue
    }

    # Capture serie pour le reste du cycle (observation post-boot). Le
    # port peut mettre quelques secondes a redevenir lisible pendant que
    # l'ESP32 redemarre -- retente l'ouverture plutot que d'abandonner.
    $observeUntil = $cycleStart.AddSeconds($CycleSeconds)
    $sp = New-Object System.IO.Ports.SerialPort $Port, 115200
    $sp.ReadTimeout = 1000
    $gotAnomaly = $false
    $anomalyDetail = ""
    $opened = $false
    for ($try = 0; $try -lt 10 -and -not $opened; $try++) {
        try { $sp.Open(); $opened = $true } catch { Start-Sleep -Milliseconds 500 }
    }
    if ($opened) {
        try {
            while ((Get-Date) -lt $observeUntil) {
                try {
                    $line = $sp.ReadLine()
                    $stamp = (Get-Date -Format "HH:mm:ss.fff")
                    Add-Content -Path $logFile -Value "$stamp $line"
                    if ($line -match '\[UDPGAP\]|abort|PANIC|Backtrace') {
                        $gotAnomaly = $true
                        $anomalyDetail = $line
                    }
                } catch [System.TimeoutException] { }
            }
        } finally {
            if ($sp.IsOpen) { $sp.Close() }
        }
    } else {
        Add-Content -Path $summaryFile -Value "$ts cycle=$cycleNum PORT_INACCESSIBLE"
        continue
    }

    $verdict = if ($gotAnomaly) { "ANOMALIE: $anomalyDetail" } else { "sain" }
    Add-Content -Path $summaryFile -Value "$ts cycle=$cycleNum $verdict"
    Write-Host "Cycle $cycleNum -> $verdict" -ForegroundColor $(if ($gotAnomaly) { "Red" } else { "Green" })
}

Write-Host "=== Test termine, $cycleNum cycles ===" -ForegroundColor Cyan
