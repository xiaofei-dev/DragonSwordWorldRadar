$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json
if (-not $release.version) { throw 'metadata/release.json has no version.' }

$required = @(
    'src\ue4ss\main.lua',
    'src\ue4ss\world_map.lua',
    'src\ue4ss\diagnostics.lua',
    'src\ue4ss\bosses.lua',
    'src\ue4ss\boss_tracker.lua',
    'src\ue4ss\config.default.lua',
    'src\overlay\Bridge\MotionBridgeReader.cs',
    'src\overlay\Bridge\SharedBridgeFile.cs',
    'src\overlay\Bridge\StaticStateBridgeReader.cs',
    'src\overlay\UI\RadarForm.cs',
    'src\overlay\Rendering\BossMarkerRenderer.cs',
    'src\overlay\Rendering\TreasureMarkerPalette.cs',
    'src\overlay\SaveData\SaveDatabaseFingerprint.cs',
    'src\installer\Install.cmd',
    'src\installer\Install.ps1',
    'src\installer\Core\InstallationPipeline.cs',
    'src\installer\Core\Providers\BossDataProvider.cs',
    'src\installer\Core\Generation\BossLuaGenerator.cs',
    'src\host\DragonSwordWorldRadar.Watcher.ps1',
    'src\host\DragonSwordWorldRadar.Host.ps1',
    'vendor\sqlcipher\e_sqlcipher.dll',
    'vendor\ooz\ooz.exe',
    'resources\defaults\treasure_overrides.txt'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative))) {
        throw "Missing: $relative"
    }
}

$forbidden = @(
    'src\ue4ss\treasure_event_probe.lua',
    'src\ue4ss\boss_respawn_probe.lua',
    'src\overlay\UI\RadarForm.cs.before-dev6-compiler-fix'
)
foreach ($relative in $forbidden) {
    if (Test-Path -LiteralPath (Join-Path $root $relative)) {
        throw "Forbidden stale/experimental file remains: $relative"
    }
}

$sourceTextFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Where-Object { $_.Extension -in @('.ps1','.lua','.json','.md','.txt','.cs','.cmd','.vbs') })
$hookReferences = @($sourceTextFiles | Select-String -Pattern 'RegisterHook\s*\(')
if ($hookReferences.Count -gt 0) {
    throw ('Experimental RegisterHook calls remain in the main source tree: ' + (($hookReferences.Path | Select-Object -Unique) -join ', '))
}

$configPath = Join-Path $root 'src\ue4ss\config.default.lua'
$configRaw = Get-Content -LiteralPath $configPath -Raw
$configNoComments = [regex]::Replace($configRaw, '(?m)--[^\r\n]*$', '').Trim()
if ($configNoComments -notmatch '^return\s*\{' -or $configNoComments -notmatch '\}\s*$') {
    throw 'src/ue4ss/config.default.lua is not a complete returned Lua table.'
}
if (([regex]::Matches($configNoComments, '\{')).Count -ne 1 -or
    ([regex]::Matches($configNoComments, '\}')).Count -ne 1) {
    throw 'src/ue4ss/config.default.lua has an unexpected brace count.'
}

$versionReferences = @(Get-ChildItem -LiteralPath $root -Recurse -File |
    Where-Object {
        $_.FullName -notlike (Join-Path $root 'dist\*') -and
        $_.Extension -in @('.ps1','.lua','.json','.md','.txt','.cs','.cmd')
    } |
    Select-String -Pattern ([regex]::Escape([string]$release.version)) -AllMatches)
if ($versionReferences.Count -eq 0) { throw 'Release version is not referenced by the source tree.' }

$allowedTool = [IO.Path]::GetFullPath((Join-Path $root 'vendor\ooz\ooz.exe'))
$unexpectedExe = @(Get-ChildItem -LiteralPath $root -Recurse -Filter '*.exe' -File -ErrorAction SilentlyContinue |
    Where-Object {
        $_.FullName -notlike (Join-Path $root 'dist\*') -and
        [IO.Path]::GetFullPath($_.FullName) -ne $allowedTool
    })
if ($unexpectedExe.Count -gt 0) {
    throw ('Unexpected EXE files found: ' + (($unexpectedExe.FullName) -join ', '))
}

$legacyNamespaces = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Select-String -Pattern 'namespace DragonSwordTreasureRadar' -SimpleMatch)
if ($legacyNamespaces.Count -gt 0) { throw 'Legacy DragonSwordTreasureRadar namespace remains.' }

$backupArtifacts = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Where-Object { $_.Name -match '(\.bak$|\.orig$|\.before-|~$)' })
if ($backupArtifacts.Count -gt 0) {
    throw ('Backup artifacts remain in src: ' + (($backupArtifacts.FullName) -join ', '))
}

Write-Host "Source verification passed for DragonSwordWorldRadar $($release.version)."
