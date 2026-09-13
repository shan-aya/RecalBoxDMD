@echo off
setlocal EnableDelayedExpansion
title RecalBoxDMD Toolkit - Installation
color 0B

echo ============================================================
echo   RecalBoxDMD Toolkit - Installation et lancement
echo ============================================================
echo.
echo Ce script verifie/installe Python 3 et la bibliotheque Pillow,
echo puis lance directement l'interface graphique de l'outil.
echo.

REM ------------------------------------------------------------
REM 1) Verifier Python
REM ------------------------------------------------------------
set "PYTHON_CMD="
where python >nul 2>nul
if %ERRORLEVEL%==0 (
    for /f "tokens=2 delims= " %%v in ('python --version 2^>^&1') do set "PYVER=%%v"
    echo [OK] Python detecte : !PYVER!
    set "PYTHON_CMD=python"
    goto :python_ready
)

echo [INFO] Python n'est pas installe ou pas dans le PATH.
where winget >nul 2>nul
if %ERRORLEVEL%==0 (
    echo [INFO] Installation de Python 3 via winget, veuillez patienter...
    winget install --id Python.Python.3.12 -e --source winget --accept-package-agreements --accept-source-agreements
    if %ERRORLEVEL% NEQ 0 (
        echo [ERREUR] L'installation via winget a echoue.
        goto :python_missing
    )
    echo [INFO] Python installe. Fermez puis relancez ce script pour que le PATH soit pris en compte.
    pause
    exit /b 0
) else (
    goto :python_missing
)

:python_missing
echo.
echo [ERREUR] Impossible d'installer Python automatiquement ^(winget absent^).
echo Installez Python manuellement depuis https://www.python.org/downloads/
echo   -^> cochez bien "Add Python to PATH" pendant l'installation,
echo puis relancez ce script.
echo.
pause
exit /b 1

:python_ready

REM ------------------------------------------------------------
REM 2) Verifier / installer les dependances (Pillow, Markdown)
REM ------------------------------------------------------------
echo.
echo [INFO] Verification des bibliotheques necessaires...
%PYTHON_CMD% -c "import PIL" >nul 2>nul
if %ERRORLEVEL%==0 (
    echo [OK] Pillow deja installe.
) else (
    echo [INFO] Installation de Pillow...
    %PYTHON_CMD% -m pip install --upgrade pip >nul 2>nul
    %PYTHON_CMD% -m pip install Pillow
    if %ERRORLEVEL% NEQ 0 (
        echo [ERREUR] Echec de l'installation de Pillow. Verifiez votre connexion internet.
        pause
        exit /b 1
    )
    echo [OK] Pillow installe.
)

%PYTHON_CMD% -c "import markdown" >nul 2>nul
if %ERRORLEVEL%==0 (
    echo [OK] Markdown deja installe.
) else (
    echo [INFO] Installation de Markdown ^(rendu de l'onglet AIDE^)...
    %PYTHON_CMD% -m pip install Markdown
    if %ERRORLEVEL% NEQ 0 (
        echo [ERREUR] Echec de l'installation de Markdown. Verifiez votre connexion internet.
        pause
        exit /b 1
    )
    echo [OK] Markdown installe.
)

REM ------------------------------------------------------------
REM 3) Lancer l'outil
REM ------------------------------------------------------------
echo.
echo [INFO] Lancement de RecalBoxDMD Toolkit...
cd /d "%~dp0"
start "" %PYTHON_CMD% run_gui.py

exit /b 0
