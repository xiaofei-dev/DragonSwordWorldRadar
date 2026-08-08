@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0installer\Install.ps1"
set "code=%ERRORLEVEL%"
if not "%code%"=="0" pause
exit /b %code%
