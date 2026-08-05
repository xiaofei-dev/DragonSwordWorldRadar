$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json
if (-not $release.version) { throw 'metadata/release.json has no version.' }

$required = @(
    'src\ue4ss\main.lua',
    'src\ue4ss\world_map.lua',
    'src\ue4ss\diagnostics.lua',
    'src\overlay\UI\RadarForm.cs',
    'src\installer\Install.ps1',
    'src\installer\Core\InstallationPipeline.cs',
    'src\host\DragonSwordWorldRadar.Watcher.ps1',
    'src\host\DragonSwordWorldRadar.Host.ps1',
    'vendor\sqlcipher\e_sqlcipher.dll',
    'resources\defaults\treasure_overrides.txt'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative))) { throw "Missing: $relative" }
}

$versionReferences = @(Get-ChildItem -LiteralPath $root -Recurse -File |
    Where-Object { $_.Extension -in @('.ps1','.lua','.json','.md','.txt','.cs','.cmd') } |
    Select-String -Pattern ([regex]::Escape([string]$release.version)) -AllMatches)
if ($versionReferences.Count -eq 0) { throw 'Release version is not referenced by the source tree.' }

$customExe = @(Get-ChildItem -LiteralPath $root -Recurse -Filter '*.exe' -File -ErrorAction SilentlyContinue)
if ($customExe.Count -gt 0) { throw ('Custom EXE files found: ' + (($customExe.FullName) -join ', ')) }
$legacyNamespaces = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Select-String -Pattern 'namespace DragonSwordTreasureRadar' -SimpleMatch)
if ($legacyNamespaces.Count -gt 0) { throw 'Legacy DragonSwordTreasureRadar namespace remains.' }
Write-Host "Source verification passed for DragonSwordWorldRadar $($release.version)."
