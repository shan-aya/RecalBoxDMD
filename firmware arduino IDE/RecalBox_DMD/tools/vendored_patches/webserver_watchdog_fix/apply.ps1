# ============================================
# apply.ps1 -- Patch vendore : crash TASK_WDT dans WebServer::_parseRequest()
# ============================================
# Voir README.md (meme dossier) pour le detail complet de la cause racine et
# du fix retenu. Ce script est IDEMPOTENT (relancable sans risque) et
# VERIFIE le contenu avant de modifier quoi que ce soit -- il refuse de
# patcher un fichier dont le contenu ne correspond ni a l'original attendu
# ni au patch deja applique, plutot que d'ecrire a l'aveugle.
#
# A relancer apres toute (re)installation du core Arduino "esp32" 3.3.11
# (les fichiers cibles font partie du core installe, PAS du depot Git --
# voir README.md, section "Pourquoi ce patch vit hors du depot Git").
#
# Usage : .\apply.ps1 [-Arduino15Root <chemin>] [-CoreVersion <version>]
# ============================================

param(
    [string]$Arduino15Root = (Join-Path $env:LOCALAPPDATA "Arduino15"),
    [string]$CoreVersion = "3.3.11"
)

$ErrorActionPreference = "Stop"
$MARKER = "RecalBoxDMD watchdog fix"
$hadError = $false

$coreBase = Join-Path $Arduino15Root "packages\esp32\hardware\esp32\$CoreVersion"
$streamPath = Join-Path $coreBase "cores\esp32\Stream.cpp"
$webServerPath = Join-Path $coreBase "libraries\WebServer\src\WebServer.cpp"

function Test-FileExists {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path $Path)) {
        Write-Host "ECHEC : $Label introuvable a l'emplacement attendu :" -ForegroundColor Red
        Write-Host "  $Path" -ForegroundColor Red
        Write-Host "  -> core esp32 $CoreVersion absent/pas au bon chemin. Verifier -Arduino15Root/-CoreVersion." -ForegroundColor Red
        return $false
    }
    return $true
}

# --- 1. Stream.cpp : delay(1) dans timedRead()/timedPeek() ---
Write-Host "=== Stream.cpp ($streamPath) ===" -ForegroundColor Cyan
if (Test-FileExists -Path $streamPath -Label "Stream.cpp") {
    $text = [System.IO.File]::ReadAllText($streamPath)
    if ($text.Contains($MARKER)) {
        Write-Host "Deja applique -- rien a faire." -ForegroundColor Green
    } else {
        $oldTimedRead = @"
int Stream::timedRead() {
  int c;
  _startMillis = millis();
  do {
    c = read();
    if (c >= 0) {
      return c;
    }
  } while (millis() - _startMillis < _timeout);
  return -1;  // -1 indicates timeout
}
"@
        $newTimedRead = @"
// $MARKER (2026-09-14) -- voir tools/vendored_patches/webserver_watchdog_fix/README.md.
// delay(1) ajoute : la boucle d'attente busy-spinnait sans yield/delay,
// affamant la tache IDLE0 assez longtemps pour declencher le Task Watchdog
// (5s, meme coeur que loop() via LoopCore=0) -- reproductible sur un simple
// acces a la page web de config du DMD (WebServer::_parseRequest()).
int Stream::timedRead() {
  int c;
  _startMillis = millis();
  do {
    c = read();
    if (c >= 0) {
      return c;
    }
    delay(1);
  } while (millis() - _startMillis < _timeout);
  return -1;  // -1 indicates timeout
}
"@
        $oldTimedPeek = @"
int Stream::timedPeek() {
  int c;
  _startMillis = millis();
  do {
    c = peek();
    if (c >= 0) {
      return c;
    }
  } while (millis() - _startMillis < _timeout);
  return -1;  // -1 indicates timeout
}
"@
        $newTimedPeek = @"
// $MARKER (2026-09-14) -- meme raison que timedRead() ci-dessus.
int Stream::timedPeek() {
  int c;
  _startMillis = millis();
  do {
    c = peek();
    if (c >= 0) {
      return c;
    }
    delay(1);
  } while (millis() - _startMillis < _timeout);
  return -1;  // -1 indicates timeout
}
"@
        if ($text.Contains($oldTimedRead) -and $text.Contains($oldTimedPeek)) {
            $text = $text.Replace($oldTimedRead, $newTimedRead).Replace($oldTimedPeek, $newTimedPeek)
            [System.IO.File]::WriteAllText($streamPath, $text)
            Write-Host "Patche avec succes (timedRead + timedPeek)." -ForegroundColor Green
        } else {
            Write-Host "ECHEC : contenu inattendu -- ce fichier ne correspond ni a l'original connu ni au patch deja applique." -ForegroundColor Red
            Write-Host "  -> version de core differente de celle prevue (3.3.11) ? Rien modifie, verification manuelle necessaire." -ForegroundColor Red
            $hadError = $true
        }
    }
} else {
    $hadError = $true
}

# --- 2. WebServer.cpp : timeout de lecture avant _parseRequest() ---
Write-Host "`n=== WebServer.cpp ($webServerPath) ===" -ForegroundColor Cyan
if (Test-FileExists -Path $webServerPath -Label "WebServer.cpp") {
    $text = [System.IO.File]::ReadAllText($webServerPath)
    if ($text.Contains($MARKER)) {
        Write-Host "Deja applique -- rien a faire." -ForegroundColor Green
    } else {
        $oldLine = '          _currentClient.setTimeout(HTTP_MAX_SEND_WAIT); /* / 1000 removed, WifiClient setTimeout changed to ms */'
        $newLine = "          _currentClient.setTimeout(800); /* $MARKER (2026-09-14) -- voir tools/vendored_patches/webserver_watchdog_fix/README.md -- etait HTTP_MAX_SEND_WAIT (5000ms, quasi egal au Task Watchdog 5s du core, cause du crash TASK_WDT documente dans DECISIONS.md) */"
        if ($text.Contains($oldLine)) {
            $text = $text.Replace($oldLine, $newLine)
            [System.IO.File]::WriteAllText($webServerPath, $text)
            Write-Host "Patche avec succes (timeout de lecture 5000ms -> 800ms)." -ForegroundColor Green
        } else {
            Write-Host "ECHEC : ligne attendue introuvable -- ce fichier ne correspond ni a l'original connu ni au patch deja applique." -ForegroundColor Red
            Write-Host "  -> version de core differente de celle prevue (3.3.11) ? Rien modifie, verification manuelle necessaire." -ForegroundColor Red
            $hadError = $true
        }
    }
} else {
    $hadError = $true
}

Write-Host ""
if ($hadError) {
    Write-Host "Termine AVEC ERREUR(S) -- voir ci-dessus. Ne pas compiler avant resolution." -ForegroundColor Red
    exit 1
} else {
    Write-Host "OK -- recompiler maintenant via compile.ps1." -ForegroundColor Green
    exit 0
}
