@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-Diagnostics.ps1"
set "RC=%ERRORLEVEL%"
echo.
if "%RC%"=="0" (
  echo Diagnostics completed successfully.
) else (
  echo Diagnostics failed with exit code %RC%.
)
echo Press any key to close this window.
pause >nul
exit /b %RC%
