#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$release = Get-Content -Raw -LiteralPath (Join-Path $root 'metadata\release.json') |
    ConvertFrom-Json
$version = [string]$release.version
if ([string]::IsNullOrWhiteSpace($version) -or
    [string]$release.name -ne 'DragonSwordWorldRadarObjectState' -or
    [string]$release.mod_folder -ne 'DragonSwordWorldRadarObjectState') {
    throw 'Release identity is incomplete or does not match the object-state Mod.'
}

$required = @(
    'README.md',
    'PROJECT_CONTEXT.md',
    'docs\ARCHITECTURE.md',
    'docs\PERFORMANCE_BASELINE.md',
    'CHANGELOG.md',
    'CMakeLists.txt',
    'include\dswros\object_state.hpp',
    'src\native\main.cpp',
    'tests\native_state_tests.cpp',
    'tools\Build-Core.ps1',
    'tools\Build-Native.ps1',
    'src\overlay\Bridge\NativeStateBridgeReader.cs',
    'src\overlay\SaveData\TreasureSaveState.cs',
    'src\installer\Core\Generation\TreasureActorCatalogGenerator.cs',
    'build\Compile-Source.ps1',
    'build\Test-Refactor.ps1',
    'build\Test-PerformanceScheduling.ps1',
    'build\Test-TreasureActorCatalog.ps1',
    'build\Test-ReleasePackage.ps1')
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)) {
        throw "Required source file is missing: $relative"
    }
}

$main = Get-Content -Raw -LiteralPath (Join-Path $root 'src\ue4ss\main.lua')
$native = Get-Content -Raw -LiteralPath (Join-Path $root 'src\native\main.cpp')
if (-not $main.Contains('version = "' + $version + '"') -or
    -not $native.Contains('constexpr auto kVersion = STR("' + $version + '")')) {
    throw 'Lua/native version does not match metadata/release.json.'
}
foreach ($relative in @(
    'src\host\DragonSwordWorldRadar.Host.ps1',
    'src\host\DragonSwordWorldRadar.Watcher.ps1',
    'src\host\DragonSwordWorldRadar.Watcher.vbs')) {
    if (-not (Get-Content -Raw -LiteralPath (Join-Path $root $relative)).Contains($version)) {
        throw "Host version does not match release metadata: $relative"
    }
}

$overlaySources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' })
$installerSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' })
if ($overlaySources.Count -ne 42 -or $installerSources.Count -ne 20) {
    throw "Unexpected managed source inventory: overlay=$($overlaySources.Count); installer=$($installerSources.Count)."
}

$cmake = Get-Content -Raw -LiteralPath (Join-Path $root 'CMakeLists.txt')
foreach ($marker in @(
    'DragonSwordWorldRadarObjectStateTests',
    'DSWROS_BUILD_UE4SS',
    'add_library(DragonSwordWorldRadarObjectState SHARED src/native/main.cpp)',
    'OUTPUT_NAME "main"',
    '/W4 /WX')) {
    if (-not $cmake.Contains($marker)) {
        throw "Native build contract is missing: $marker"
    }
}

$config = Get-Content -Raw -LiteralPath (Join-Path $root 'src\ue4ss\config.lua')
if ($config -notmatch 'debug_logging\s*=\s*true' -or
    $config -notmatch 'high_resolution_timer\s*=\s*true' -or
    $config -notmatch 'diagnostic_verbose\s*=\s*false') {
    throw 'Pre-release diagnostics/timer defaults are incorrect.'
}

if (@(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
        Where-Object { $_.Extension -match '^(?i)\.(exe|dll|pdb|obj)$' }).Count -ne 0) {
    throw 'Compiled artifacts are present under src/.'
}
$ooz = Join-Path $root 'vendor\ooz\ooz.exe'
if (Test-Path -LiteralPath $ooz -PathType Leaf) {
    $oozTools = @($release.bundled_tools |
        Where-Object { [string]$_.name -ieq 'ooz.exe' })
    if ($oozTools.Count -ne 1) {
        throw 'Release metadata must contain exactly one ooz.exe tool.'
    }
    $expectedOoz = [string]$oozTools[0].sha256
    $actualOoz = (Get-FileHash -LiteralPath $ooz -Algorithm SHA256).Hash
    if ($actualOoz -ine $expectedOoz) {
        throw 'Bundled ooz.exe does not match release metadata.'
    }
}

& (Join-Path $PSScriptRoot 'Test-PerformanceScheduling.ps1')
Write-Host "SOURCE_VERIFY_OK version=$version overlay=42 installer=20 native=lifecycle-shared-memory save=F7-once"
