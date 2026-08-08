@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Start-Monitor.ps1" -Root "%~dp0"
echo.
pause
