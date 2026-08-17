# ============================================
# compile.ps1 -- Compilation RecalBox_DMD.ino avec garde-fou gzip
# ============================================
# Regenere TOUJOURS web_config_html_gz.h depuis web_config.h avant de
# compiler -- evite le piege de process documente dans web_config.h
# (page web servie via un gzip genere separement : toute modification du
# HTML/JS oubliee de regeneration laisse le firmware servir une ancienne
# version SILENCIEUSEMENT, sans erreur de compilation). Ce script rend cet
# oubli impossible en le rendant automatique a chaque compilation.
#
# Usage : .\compile.ps1
# ============================================

$dir = $PSScriptRoot

Write-Host "=== Regeneration web_config_html_gz.h ===" -ForegroundColor Cyan
python "$dir\tools\gen_web_config_gz.py"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ECHEC generation gzip -- compilation annulee." -ForegroundColor Red
    exit 1
}

Write-Host "`n=== Compilation arduino-cli ===" -ForegroundColor Cyan
$FQBN = "esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=40,FlashMode=qio,FlashSize=4M,PartitionScheme=huge_app,DebugLevel=none,PSRAM=disabled,LoopCore=0,EventsCore=1,EraseFlash=none"
# directories.user d'arduino-cli est aligne sur le sketchbook de l'IDE
# Arduino ("d:\CROQUIS ARDUINO IDE", voir arduino-cli config) -- les
# librairies utilisees ici sont donc desormais exactement les memes que
# celles de l'IDE, plus de --libraries explicite necessaire.
arduino-cli compile --clean --fqbn $FQBN --output-dir "$dir\compiled" "$dir"
