@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\Run-Pak-Verification.ps1" -Root "%ROOT%"
echo.
pause
