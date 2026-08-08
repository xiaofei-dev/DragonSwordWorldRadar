$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Build = Join-Path $Root "build"

cmake -S $Root -B $Build -A x64
cmake --build $Build --config Release

$Dll = Join-Path $Build "bin\Release\DBConnectionProbe.dll"
if (-not (Test-Path $Dll)) {
    $Dll = Join-Path $Build "bin\DBConnectionProbe.dll"
}
if (-not (Test-Path $Dll)) {
    throw "DBConnectionProbe.dll was not produced."
}

$Out = Join-Path $Root "bin"
New-Item -ItemType Directory -Force -Path $Out | Out-Null
Copy-Item -Force $Dll (Join-Path $Out "DBConnectionProbe.dll")
Write-Host "Built: $(Join-Path $Out 'DBConnectionProbe.dll')"
