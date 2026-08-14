@echo off
setlocal
cd /d "%~dp0"

set "PS_SCRIPT=%~dp0Publish-GitHub-Interactive.ps1"

if not exist "%PS_SCRIPT%" (
    echo.
    echo [ERROR] Publish-GitHub-Interactive.ps1 was not found.
    echo Put this CMD file and the PowerShell script in the repository root.
    echo.
    pause
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%"
set "RC=%ERRORLEVEL%"

echo.
if not "%RC%"=="0" (
    echo [FAILED] Publish script exited with code %RC%.
) else (
    echo [DONE] Publish workflow finished.
)
echo.
pause
exit /b %RC%
