@echo off
rem Double-clic : met a jour le paquet de logos de themes depuis la Recalbox (voir refresh_package_from_recalbox.ps1)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0refresh_package_from_recalbox.ps1" %*
echo.
pause
