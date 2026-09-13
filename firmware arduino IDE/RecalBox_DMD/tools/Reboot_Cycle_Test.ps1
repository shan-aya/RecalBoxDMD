# ============================================
# Reboot_Cycle_Test.ps1 -- reboots repetes + capture serie par cycle
# ============================================
# Teste l'hypothese "certains boots partent mal des le depart" (retour
# utilisateur, 2026-09-12 nuit) plutot que "degradation avec l'uptime".
# Reboote la carte (re-upload du binaire deja compile, declenche un hard
# reset) toutes les $CycleSeconds secondes, capture le serial dans un
# fichier DEDIE par cycle -- permet de verifier facilement si [UDPGAP]
# apparait tres tot apres CE boot precis, ou si la carte reste saine.
#
# Le trafic ping externe (necessaire pour que [UDPGAP] ait un sens, sinon
# "personne ne parle a la carte" ressemble a tort a une coupure) est deja
# fourni par le script Python de test long (dual_ping_long_test.py, ping
# toutes les 5s vers 192.168.0.142) qui tourne en parallele -- pas besoin
# de le dupliquer ici.
#
# Usage : .\Reboot_Cycle_Test.ps1 -Port COM3 -SketchDir <dossier> -FQBN <fqbn> -TotalHours 8 -CycleSeconds 180
param(
    [Parameter(Mandatory=$true)][string]$Port,
    [Parameter(Mandatory=$true)][string]$SketchDir,
    [Parameter(Mandatory=$true)][string]$FQBN,
    [double]$TotalHours = 8,
    [int]$CycleSeconds = 180
)

$logDir = "$PSScriptRoot\serial_logs\reboot_cycles"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$summaryFile = "$logDir\_summary.txt"

$endTime = (Get-Date).AddHours($TotalHours)
$cycleNum = 0

while ((Get-Date) -lt $endTime) {
    $cycleNum++
    $cycleStart = Get-Date
    $ts = $cycleStart.ToString("yyyy-MM-dd_HHmmss")
    $logFile = "$logDir\cycle${cycleNum}_$ts.txt"

    Write-Host "=== Cycle $cycleNum -- reboot a $ts ===" -ForegroundColor Yellow

    # Reboot via re-upload (declenche un hard reset RTS a la fin, deja
    # valide fonctionnel dans ce projet). Le port doit etre libre --
    # aucun logger persistant ne doit tourner en parallele sur ce port.
    $uploadOutput = & arduino-cli upload --fqbn $FQBN --port $Port --input-dir "$SketchDir\compiled" $SketchDir 2>&1
    $uploadOk = $LASTEXITCODE -eq 0

    if (-not $uploadOk) {
        Add-Content -Path $summaryFile -Value "$ts cycle=$cycleNum UPLOAD_ECHEC"
        Write-Host "Echec upload, retry dans 5s" -ForegroundColor Red
        Start-Sleep -Seconds 5
        continue
    }

    # Capture serie pour le reste du cycle (observation post-boot).
    $observeUntil = $cycleStart.AddSeconds($CycleSeconds)
    $sp = New-Object System.IO.Ports.SerialPort $Port, 115200
    $sp.ReadTimeout = 1000
    $gotAnomaly = $false
    $anomalyDetail = ""
    try {
        $sp.Open()
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

    $verdict = if ($gotAnomaly) { "ANOMALIE: $anomalyDetail" } else { "sain" }
    Add-Content -Path $summaryFile -Value "$ts cycle=$cycleNum $verdict"
    Write-Host "Cycle $cycleNum -> $verdict" -ForegroundColor $(if ($gotAnomaly) { "Red" } else { "Green" })
}

Write-Host "=== Test termine, $cycleNum cycles ===" -ForegroundColor Cyan
