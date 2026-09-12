# ============================================
# Generic_Serial_Logger.ps1 -- logger serie generique parametre par port
# ============================================
# Meme esprit que DMD_Serial_Monitor.ps1 (fenetre visible, lecture ligne
# par ligne, log horodate dans serial_logs/) mais parametrable sur un port
# COM explicite -- utilise pour le sketch d'isolation udp_freeze_isolation
# (2e module ESP32), en parallele du DMD de production sur un autre port.
#
# Usage : .\Generic_Serial_Logger.ps1 -Port COM3 -Label isolation
# ============================================
param(
    [Parameter(Mandatory=$true)][string]$Port,
    [string]$Label = "generic",
    [int]$Baud = 115200
)

$logDir = "$PSScriptRoot\serial_logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$ts = Get-Date -Format "yyyy-MM-dd_HHmmss"
$logFile = "$logDir\${Label}_serial_$ts.txt"

Write-Host "=== Generic Serial Logger ($Label) ===" -ForegroundColor Cyan
Write-Host "=== $Port @ $Baud -- log: $logFile ===" -ForegroundColor Cyan
Write-Host "(Ctrl+C ou ferme la fenetre pour arreter)"

$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud
$sp.ReadTimeout = 1000
$sp.Open()

while ($true) {
    try {
        $line = $sp.ReadLine()
        $stamp = (Get-Date -Format "HH:mm:ss.fff")
        $out = "$stamp $line"
        Write-Host $out
        Add-Content -Path $logFile -Value $out -Encoding utf8
    } catch [System.TimeoutException] {
        # normal, pas de ligne dispo -- on reboucle
    }
}
