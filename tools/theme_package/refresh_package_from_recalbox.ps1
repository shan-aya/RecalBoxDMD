# ============================================
# refresh_package_from_recalbox.ps1 -- Met a jour le PAQUET de logos de themes (carte SD/systems/_defaults/_themes)
# depuis les themes INSTALLES sur la Recalbox (la Recalbox est la reference : le theme-hub GitLab n'est plus alimente).
# ============================================
# Outil du MAINTENEUR (pas de l'utilisateur final). A lancer depuis le checkout "main" du depot.
# Ne fait AUCUN git add / commit / push : il construit et montre ce qui a change ; la publication reste un geste manuel.
#
# Prerequis : la Recalbox allumee et son partage reseau accessible (\\RECALBOX\share\themes), Python avec resvg-py, numpy, pillow
#             (versions de tools/theme_package/requirements.txt : le rendu doit rester reproductible).
# Usage :
#   .\tools\theme_package\refresh_package_from_recalbox.ps1
#   .\tools\theme_package\refresh_package_from_recalbox.ps1 -Rb "\\192.168.0.35\share\themes"
#   .\tools\theme_package\refresh_package_from_recalbox.ps1 -Only "bounitos-midnight,serviettzky-dashboard-X"
#
# v1 - 2026-10-05 - Creation (decision utilisateur : les themes se basent sur la Recalbox, le hub est en retard).

param(
    [string]$Rb      = "\\RECALBOX\share\themes",
    [string]$Only    = "",   # vide = TOUS les themes (catalogue a jour de media.recalbox.com + themes installes sur la Recalbox) : le nombre de themes evolue
    [string]$Exclude = ""    # themes a ne pas publier, separes par des virgules
)

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$root = (Resolve-Path (Join-Path $here "..\..")).Path
$out  = Join-Path $root "carte SD\systems\_defaults\_themes"
$known = Join-Path $root "carte SD\systems\_defaults"

function Step($m) { Write-Host "`n=== $m ===" -ForegroundColor Cyan }

Step "1/4 Verifications"
if (-not (Test-Path (Join-Path $root ".git"))) { throw "Ce script doit tourner depuis un checkout du depot (dossier .git introuvable au-dessus de $here)." }
if (-not (Test-Path $out))   { throw "Dossier du paquet introuvable : $out" }
if (-not (Test-Path $Rb))    { throw "Partage de la Recalbox inaccessible : $Rb (Recalbox allumee ? ou utilise -Rb \\IP\share\themes)" }
$py = (Get-Command python -ErrorAction SilentlyContinue)
if (-not $py) { throw "Python introuvable dans le PATH." }
& python -c "import resvg_py, numpy, PIL" 2>$null
if ($LASTEXITCODE -ne 0) { throw "Modules manquants : pip install -r tools\theme_package\requirements.txt" }
Write-Host "Recalbox : $Rb"
Write-Host ("Themes   : " + $(if ($Only) { $Only } else { "tous (hub + Recalbox)" }) + $(if ($Exclude) { " ; exclus : $Exclude" } else { "" }))

Step "2/4 Versions installees sur la Recalbox"
foreach ($d in (Get-ChildItem $Rb -Directory | Sort-Object Name)) {
    $x = Join-Path $d.FullName "theme.xml"
    if ((Test-Path $x) -and ((-not $Only) -or (($Only -split ",") -contains $d.Name))) {
        $m = [regex]::Match((Get-Content $x -TotalCount 12 -Encoding UTF8 | Out-String), '<theme\b[^>]*\bversion="([^"]+)"')
        "{0,-36} v{1}" -f $d.Name, $(if ($m.Success) { $m.Groups[1].Value } else { "?" })
    }
}

Step "3/4 Construction du paquet (--prefer-rb, incremental)"
$summary = Join-Path $env:TEMP "rb_pkg_summary.md"
$env:PYTHONIOENCODING = "utf-8"
Push-Location $root
try {
    $args2 = @((Join-Path $here "build_theme_package.py"), $out, "--known", $known, "--rb", $Rb, "--prefer-rb", "--incremental", "--summary", $summary)
    if ($Only)    { $args2 += @("--only", $Only) }
    if ($Exclude) { $args2 += @("--exclude", $Exclude) }
    & python -W ignore @args2
    if ($LASTEXITCODE -ne 0) { throw "Echec de la construction du paquet (code $LASTEXITCODE)." }

    Step "4/4 Ce qui a change"
    if (Test-Path $summary) { Get-Content $summary -Encoding UTF8 }
    $st = git status --short -- "carte SD/systems/_defaults/_themes" | ForEach-Object {
        $p = $_.Substring(3).Trim('"')
        if ($p -match '_themes/([^/]+)/') { $Matches[1] } else { $p }
    } | Group-Object | Sort-Object Name
    if ($st) {
        Write-Host "`nFichiers modifies par theme :" -ForegroundColor Green
        $st | ForEach-Object { "{0,5}  {1}" -f $_.Count, $_.Name }
        Write-Host "`nProchaine etape (MANUELLE) : verifier un theme dans le toolkit (Mode 12 > Apercu), puis git add / commit / push." -ForegroundColor Cyan
    } else {
        Write-Host "`nAucun changement : le paquet est deja a jour avec la Recalbox." -ForegroundColor Green
    }
} finally { Pop-Location }
