@echo off
setlocal
cd /d "%~dp0"

set "PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%PS%" (
  echo Windows PowerShell 5.1 was not found.
  pause
  exit /b 1
)

"%PS%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build\Apply-Patch-And-Deploy.ps1"
set "code=%ERRORLEVEL%"
if not "%code%"=="0" pause
exit /b %code%
