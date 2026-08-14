[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$cmakeCandidates = @(@(
    (Get-Command cmake.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1),
    'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })

if (-not $cmakeCandidates) {
    throw 'CMake was not found.'
}

$cmake = $cmakeCandidates[0]
$buildDirectory = Join-Path $projectRoot 'build-native-core'
& $cmake -S $projectRoot -B $buildDirectory -A x64 -DDSWROS_BUILD_UE4SS=OFF
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
& $cmake --build $buildDirectory --config $Configuration
if ($LASTEXITCODE -ne 0) { throw "Core build failed: $LASTEXITCODE" }
& $cmake --build $buildDirectory --config $Configuration --target RUN_TESTS
if ($LASTEXITCODE -ne 0) { throw "Core tests failed: $LASTEXITCODE" }

Write-Host 'Core build and tests passed.'
