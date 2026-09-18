# ============================================
# build_release.ps1 -- Regenere le dossier _release local en une seule
# commande (firmware + PC Toolkit + scripts Recalbox)
# ============================================
# A executer depuis la RACINE du checkout "master" (celui qui a _release/,
# scripts/manual/, tools/dist*) -- PAS depuis un worktree de dev.
#
# Cree/regenere en une seule commande tout ce qui, avant ce script, se
# faisait a la main (session du 13-14/09/2026) :
#   1. Compile le firmware (compile.ps1)
#   2. Copie/renomme les binaires firmware dans DMD_firmware/, zip
#   3. Copie les scripts Recalbox (userscripts/ + dmd_helpers/ +
#      scripts/manual/) dans recalbox_scripts/, zip
#   4. Build le portable .exe (PyInstaller)
#   5. Build l'installeur .exe (Inno Setup)
#   6. Build le .msi (cx_Freeze) -- NON BLOQUANT si echec (probleme connu
#      cx_Freeze 8.7.0/Python 3.14, voir DECISIONS.md du 2026-09-14 :
#      bdist_msi ne produit rien sans lever d'erreur). L'ancien .msi de la
#      release est alors conserve tel quel, avec un avertissement clair.
#   7. Reconstruit le zip source (perimetre identique a
#      tools/RecalBoxDMD_tool_v6243/ publie sur GitHub)
#   8. Genere CHANGELOG_this_release.md (derniere entree "safe-modify" de
#      RecalBox_DMD.ino/RecalBoxDMD_GUI.py/RecalBoxDMD_tool.py -- journal
#      technique interne, distinct du CHANGELOG.md public/traduit)
#   9. Copie tout dans _release/RecalBoxDMD_<FirmwareLabel>_t<ToolkitBuild>/
#
# Ne touche PAS a GitHub (pas de commit/push) -- volontaire : la procedure
# de publication reste un geste separe et deliberement manuel (voir la
# section "Procedure de mise a jour du depot GitHub public" dans
# DECISIONS.md), ce script se limite a preparer les artefacts locaux.
#
# Usage :
#   .\build_release.ps1                        # versions par defaut (v2.0 / 6243)
#   .\build_release.ps1 -ToolkitBuild 6300      # nouveau numero de build toolkit
#   .\build_release.ps1 -SkipMsi                # saute carrement l'etape .msi (plus rapide)

param(
    [string]$FirmwareLabel = "v2.0",
    [string]$ToolkitBuild  = "6243",
    [switch]$SkipMsi
)

$ErrorActionPreference = "Stop"
$root        = $PSScriptRoot
$tools       = Join-Path $root "tools"
$releaseDir  = Join-Path $root "_release\RecalBoxDMD_${FirmwareLabel}_t${ToolkitBuild}"

function Step($msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Warn($msg) { Write-Host "AVERTISSEMENT: $msg" -ForegroundColor Yellow }

# Extrait l'entree de changelog LA PLUS RECENTE de l'en-tete "safe-modify"
# d'un fichier source (convention deja en place dans RecalBox_DMD.ino/
# RecalBoxDMD_GUI.py/RecalBoxDMD_tool.py -- voir la skill safe-modify).
# Format de chaque entree : "// v<N> - <date> - safe-modify - ..." ou
# "# v<N> - <date> - safe-modify - ..." -- le separateur est parfois un
# tiret simple, parfois un tiret cadratin ("—") selon le fichier, d'ou le
# [-—] dans le motif. Chaque entree s'etend jusqu'au debut de la
# suivante (les entrees sont toujours triees de la plus recente a la plus
# ancienne) -- ne depend d'aucun tag/historique git, fonctionne meme entre
# 2 branches sans ancetre commun.
function Get-LatestChangelogEntry {
    param([string]$FilePath, [string]$Label)
    if (-not (Test-Path $FilePath)) { return $null }
    $content = Get-Content $FilePath -Raw -Encoding UTF8
    $pattern = '(?m)^(?://|#)\s*v(\d+)\s*[-—]\s*(\d{4}-\d{2}-\d{2})\s*[-—]\s*safe-modify\s*[-—].*$'
    $ms = [regex]::Matches($content, $pattern)
    if ($ms.Count -eq 0) { return $null }
    $first = $ms[0]
    $endIdx = if ($ms.Count -gt 1) { $ms[1].Index } else { $content.Length }
    $entryText = $content.Substring($first.Index, $endIdx - $first.Index).TrimEnd()
    $lines = ($entryText -split "`r?`n") | ForEach-Object { $_ -replace '^\s*(//|#)\s?', '' }
    [PSCustomObject]@{
        Label   = $Label
        Version = "v$($first.Groups[1].Value)"
        Date    = $first.Groups[2].Value
        Text    = ($lines -join "`n").TrimEnd()
    }
}

if (-not (Test-Path (Join-Path $root "compile.ps1"))) {
    throw "compile.ps1 introuvable a la racine ($root) -- ce script doit tourner depuis le checkout master, pas un worktree de dev."
}
New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null

# --- 1) Firmware ---
Step "1/9 Compilation firmware"
# v1 - 2026-09-18 - safe-modify - BUG REEL trouve en test materiel (cycle de
# republication build 6301) : arduino-cli emet une note informative
# ("#pragma message: Compiling for original ESP32...") sur stderr -- sans
# rapport avec un echec de compilation (compile.ps1 seul, sans le
# $ErrorActionPreference="Stop" global de CE script, l'a toujours ignoree
# sans souci sur des dizaines de compilations cette session). Mais SOUS ce
# $ErrorActionPreference="Stop" (tete de ce script), PowerShell remonte
# cette simple note comme une exception terminale (NativeCommandError),
# stoppant le cycle de release AVANT que la compilation elle-meme n'ait
# fini -- confirme : le .bin de sortie datait encore du cycle precedent
# apres cet echec silencieux. Fix : ErrorActionPreference assoupli
# localement le temps de cet appel precis (compile.ps1 ecrit deja son
# propre message d'echec + $LASTEXITCODE fiable en cas de vraie erreur,
# verifie juste apres comme avant).
$prevEAP = $ErrorActionPreference
$ErrorActionPreference = "Continue"
& (Join-Path $root "compile.ps1")
$ErrorActionPreference = $prevEAP
if ($LASTEXITCODE -ne 0) { throw "Echec compilation firmware (voir sortie ci-dessus)." }

Step "2/9 Copie des binaires firmware"
$fwSrc = Join-Path $root "compiled"
$fwDst = Join-Path $releaseDir "DMD_firmware"
# Repart de zero -- evite qu'un ancien nom de fichier (ex. changement de
# FirmwareLabel entre 2 executions) reste indefiniment a cote du nouveau.
if (Test-Path $fwDst) { Remove-Item $fwDst -Recurse -Force }
New-Item -ItemType Directory -Path $fwDst -Force | Out-Null
Copy-Item (Join-Path $fwSrc "RecalBox_DMD.ino.bin")            (Join-Path $fwDst "RecalBoxDMD_${FirmwareLabel}_app.bin")    -Force
Copy-Item (Join-Path $fwSrc "RecalBox_DMD.ino.merged.bin")     (Join-Path $fwDst "RecalBoxDMD_${FirmwareLabel}_merged.bin") -Force
Copy-Item (Join-Path $fwSrc "RecalBox_DMD.ino.bootloader.bin") (Join-Path $fwDst "bootloader.bin")  -Force
Copy-Item (Join-Path $fwSrc "RecalBox_DMD.ino.partitions.bin") (Join-Path $fwDst "partitions.bin")  -Force
$bootApp0Dst = Join-Path $fwDst "boot_app0.bin"
if (-not (Test-Path $bootApp0Dst)) {
    $bootApp0Src = Get-ChildItem "$env:LOCALAPPDATA\Arduino15\packages\esp32" -Recurse -Filter "boot_app0.bin" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($bootApp0Src) { Copy-Item $bootApp0Src.FullName $bootApp0Dst -Force }
    else { Warn "boot_app0.bin introuvable (ni dans la release existante, ni dans le core ESP32 installe) -- a ajouter manuellement." }
}

Step "Zip DMD_firmware.zip"
$fwZip = Join-Path $releaseDir "DMD_firmware.zip"
if (Test-Path $fwZip) { Remove-Item $fwZip -Force }
Compress-Archive -Path (Join-Path $fwDst "*") -DestinationPath $fwZip

# --- 2) Scripts Recalbox ---
Step "3/9 Copie des scripts Recalbox"
$scriptsDst = Join-Path $releaseDir "recalbox_scripts"
$helpersDst = Join-Path $scriptsDst "dmd_helpers"
$manualDst  = Join-Path $scriptsDst "manual"
# Repart de zero a chaque fois (au lieu de copier PAR-DESSUS) -- sinon
# d'anciennes versions de scripts (noms perimes d'un rename anterieur,
# fichiers "_disabled_stale_..." etc.) restent indefiniment dans la
# release, exactement le genre de residu deja rencontre plusieurs fois
# cette session (v5438 vs v6243, anciens noms de scripts manuels...).
if (Test-Path $scriptsDst) { Remove-Item $scriptsDst -Recurse -Force }
New-Item -ItemType Directory -Path $scriptsDst, $helpersDst, $manualDst -Force | Out-Null
Get-ChildItem (Join-Path $tools "userscripts") -Filter "*.sh" -File | Copy-Item -Destination $scriptsDst -Force
Get-ChildItem (Join-Path $tools "userscripts\dmd_helpers") -File | Copy-Item -Destination $helpersDst -Force
Get-ChildItem (Join-Path $root "scripts\manual") -File | Copy-Item -Destination $manualDst -Force

Step "Zip RB_scripts.zip"
$rbZip = Join-Path $releaseDir "RB_scripts.zip"
if (Test-Path $rbZip) { Remove-Item $rbZip -Force }
Compress-Archive -Path (Join-Path $scriptsDst "*") -DestinationPath $rbZip

# --- 3) PC Toolkit : portable exe (PyInstaller) ---
# v1 -- verifie que TOOLKIT_RELEASE_VERSION (constante affichee dans le
# bandeau de la fenetre, voir son commentaire dans RecalBoxDMD_GUI.py) est
# bien synchronisee a la main avec -ToolkitBuild avant de packager -- sans
# ca, le bandeau afficherait un numero perime des la prochaine release
# (aucune injection automatique, cf. justification dans le commentaire de
# la constante). Avertissement seul, non bloquant (peut etre volontaire en
# cours de test local avec -ToolkitBuild par defaut).
$guiPy = Join-Path $tools "RecalBoxDMD_GUI.py"
$verMatch = Select-String -Path $guiPy -Pattern 'TOOLKIT_RELEASE_VERSION\s*=\s*"([^"]+)"' | Select-Object -First 1
if ($verMatch -and $verMatch.Matches[0].Groups[1].Value -ne $ToolkitBuild) {
    Warn "TOOLKIT_RELEASE_VERSION dans RecalBoxDMD_GUI.py ($($verMatch.Matches[0].Groups[1].Value)) ne correspond pas a -ToolkitBuild ($ToolkitBuild) -- le bandeau de la fenetre affichera un numero perime. Mets a jour la constante avant de publier."
} elseif (-not $verMatch) {
    Warn "TOOLKIT_RELEASE_VERSION introuvable dans RecalBoxDMD_GUI.py -- verification de coherence sautee."
}

Step "4/9 Build du portable .exe (PyInstaller)"
# v1 -- meme fix qu'a l'etape 1/9 (voir son commentaire) : PyInstaller ecrit
# ses logs INFO normaux sur stderr, remontes en exception terminale sous
# $ErrorActionPreference="Stop" -- $LASTEXITCODE reste la seule source de
# verite fiable pour un vrai echec, verifie juste apres comme avant.
Push-Location $tools
try {
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    python -m PyInstaller --clean --noconfirm RecalBoxDMD_GUI.spec
    $ErrorActionPreference = $prevEAP
    if ($LASTEXITCODE -ne 0) { throw "Echec PyInstaller (code $LASTEXITCODE)." }
} finally { Pop-Location }

# --- 4) Installeur .exe (Inno Setup) ---
Step "5/9 Build de l'installeur .exe (Inno Setup)"
$iscc = Get-ChildItem "C:\Program Files (x86)\Inno Setup 6\ISCC.exe", "C:\Program Files\Inno Setup 6\ISCC.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $iscc) { throw "ISCC.exe introuvable -- Inno Setup 6 doit etre installe (https://jrsoftware.org/isinfo.php)." }
Push-Location $tools
try {
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & $iscc.FullName "RecalBoxDMD_Setup.iss"
    $ErrorActionPreference = $prevEAP
    if ($LASTEXITCODE -ne 0) { throw "Echec Inno Setup (code $LASTEXITCODE)." }
} finally { Pop-Location }

# --- 5) .msi (cx_Freeze) -- non bloquant ---
$msiOk = $false
if (-not $SkipMsi) {
    Step "6/9 Build du .msi (cx_Freeze) -- non bloquant si echec"
    Push-Location $tools
    try {
        python setup_msi.py bdist_msi
        $msiPath = Join-Path $tools "dist_msi\RecalBoxDMD Toolkit-1.0.0-win64.msi"
        if ($LASTEXITCODE -eq 0 -and (Test-Path $msiPath) -and ((Get-Item $msiPath).LastWriteTime -gt (Get-Date).AddMinutes(-10))) {
            $msiOk = $true
        } else {
            Warn "Le build .msi n'a rien produit de recent (probleme connu cx_Freeze 8.7.0/Python 3.14, voir DECISIONS.md 2026-09-14). L'ancien .msi de la release, s'il existe, est conserve tel quel."
        }
    } finally { Pop-Location }
} else {
    Step "6/9 .msi saute (-SkipMsi)"
}

# --- 6) Zip source (meme perimetre que tools/RecalBoxDMD_tool_v6243/ publie sur GitHub) ---
Step "7/9 Construction du zip source"
$stageDir = Join-Path $env:TEMP "rbdmd_source_stage_$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Path $stageDir -Force | Out-Null
try {
    foreach ($f in @("HELP.md","HELP.fr.md","HELP.es.md","RecalBoxDMD_GUI.py","RecalBoxDMD_md_renderer.py","RecalBoxDMD_prefs.py","RecalBoxDMD_themes.py","RecalBoxDMD_tool.py","install_and_run.bat","run_gui.py")) {
        $src = Join-Path $tools $f
        if (Test-Path $src) { Copy-Item $src (Join-Path $stageDir $f) -Force }
        else { Warn "$f absent de tools/, ignore dans le zip source." }
    }
    Copy-Item (Join-Path $tools "assets") (Join-Path $stageDir "assets") -Recurse -Force
    Copy-Item (Join-Path $tools "themes") (Join-Path $stageDir "themes") -Recurse -Force
    Get-ChildItem $stageDir -Recurse -Filter "__pycache__" -Directory | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    $sourceZip = Join-Path $env:TEMP "RecalBoxDMD-${ToolkitBuild}-source.zip"
    if (Test-Path $sourceZip) { Remove-Item $sourceZip -Force }
    Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $sourceZip
} finally {
    Remove-Item $stageDir -Recurse -Force -ErrorAction SilentlyContinue
}

# --- 7) Changelog de cette release ---
Step "8/9 Generation du changelog"
$entries = @(
    Get-LatestChangelogEntry (Join-Path $root "RecalBox_DMD.ino")         "Firmware (RecalBox_DMD.ino)"
    Get-LatestChangelogEntry (Join-Path $tools "RecalBoxDMD_GUI.py")      "PC Toolkit - interface (RecalBoxDMD_GUI.py)"
    Get-LatestChangelogEntry (Join-Path $tools "RecalBoxDMD_tool.py")     "PC Toolkit - coeur (RecalBoxDMD_tool.py)"
) | Where-Object { $_ }
$changelogPath = Join-Path $releaseDir "CHANGELOG_this_release.md"
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# Changelog -- RecalBoxDMD ${FirmwareLabel} / toolkit build ${ToolkitBuild}")
$lines.Add("")
$lines.Add("Genere automatiquement le $(Get-Date -Format 'yyyy-MM-dd HH:mm') par build_release.ps1 -- extrait de la derniere entree de l'en-tete `"safe-modify`" de chaque fichier source. Journal technique interne (pas le CHANGELOG.md public/traduit) -- destine a savoir rapidement ce qui a change dans CE build precis.")
$lines.Add("")
if ($entries.Count -eq 0) {
    $lines.Add("(Aucune entree de changelog trouvee -- verifier que RecalBox_DMD.ino/RecalBoxDMD_GUI.py/RecalBoxDMD_tool.py suivent toujours la convention safe-modify.)")
} else {
    foreach ($e in $entries) {
        $lines.Add("## $($e.Label) -- $($e.Version) ($($e.Date))")
        $lines.Add("")
        $lines.Add($e.Text)
        $lines.Add("")
    }
}
[System.IO.File]::WriteAllLines($changelogPath, $lines.ToArray(), (New-Object System.Text.UTF8Encoding($false)))
Write-Host "Changelog ecrit : $changelogPath"

# --- 8) Assemblage final dans Windows_Tools/ ---
Step "9/9 Copie des outils Windows dans la release"
$wtDst = Join-Path $releaseDir "Windows_Tools"
New-Item -ItemType Directory -Path $wtDst -Force | Out-Null
# Retire les artefacts VERSIONNES d'un run precedent avec un autre
# -ToolkitBuild (ex. RecalBoxDMD-6243-portable.exe qui trainerait a cote
# d'un nouveau RecalBoxDMD-6300-portable.exe) -- ne touche pas aux fichiers
# non versionnes de ce dossier (ex. RecalBoxDMD_prefs.json), qui ne sont
# pas regeneres par ce script et doivent survivre au nettoyage.
Get-ChildItem (Join-Path $wtDst "*") -File -Include "RecalBoxDMD-*-portable.exe", "RecalBoxDMD_Toolkit_*_Setup.exe", "RecalBoxDMD-*-source.zip", "RecalBoxDMD Toolkit-*-win64.msi" | Remove-Item -Force
Copy-Item (Join-Path $tools "dist\RecalBoxDMD_GUI.exe")                        (Join-Path $wtDst "RecalBoxDMD-${ToolkitBuild}-portable.exe")      -Force
Copy-Item (Join-Path $tools "dist_installer\RecalBoxDMD_Toolkit_Setup.exe")    (Join-Path $wtDst "RecalBoxDMD_Toolkit_${ToolkitBuild}_Setup.exe") -Force
Copy-Item $sourceZip                                                          (Join-Path $wtDst "RecalBoxDMD-${ToolkitBuild}-source.zip")        -Force
Remove-Item $sourceZip -Force -ErrorAction SilentlyContinue
if ($msiOk) {
    Copy-Item (Join-Path $tools "dist_msi\RecalBoxDMD Toolkit-1.0.0-win64.msi") (Join-Path $wtDst "RecalBoxDMD Toolkit-${ToolkitBuild}-win64.msi") -Force
}

Step "Termine"
Write-Host "Release regeneree dans : $releaseDir" -ForegroundColor Green
Get-ChildItem $releaseDir -Recurse -File | Select-Object @{n='Fichier';e={$_.FullName.Substring($releaseDir.Length+1)}}, @{n='Taille';e={"{0:N1} MB" -f ($_.Length/1MB)}}, LastWriteTime | Format-Table -AutoSize
if (-not $msiOk -and -not $SkipMsi) {
    Write-Host "Rappel : le .msi n'a PAS ete regenere ce coup-ci (voir avertissement plus haut) -- celui deja present dans la release, s'il existe, date d'avant." -ForegroundColor Yellow
}
Write-Host "`nProchaine etape (manuelle, volontairement) : mettre a jour le depot GitHub public si besoin -- voir la procedure dans DECISIONS.md." -ForegroundColor Cyan
