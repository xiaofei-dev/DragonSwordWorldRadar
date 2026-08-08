@echo off
setlocal
cd /d "%~dp0"
title DragonSwordWorldDataProbe Diagnostics - waits for game exit

echo DragonSwordWorldDataProbe diagnostics
echo.
echo - You may start this while the game is still running.
echo - The window will wait for the game to close.
echo - After exit it waits for the automatic monitor, runs/reuses modules,
echo   packages the report, verifies the ZIP, and prints the output path.
echo - Do not click the diagnostic command a second time.
echo.

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Collect-Diagnostics.ps1" -Root "%~dp0"

echo.
pause
