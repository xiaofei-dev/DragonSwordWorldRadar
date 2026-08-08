#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$releasePath = Join-Path $root 'metadata\release.json'
$release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
$version = [string]$release.version
if ([string]::IsNullOrWhiteSpace($version)) {
    throw 'metadata/release.json has no version.'
}

$required = @(
    'Apply-Patch-And-Deploy.cmd',
    'DragonSwordWorldRadar.sln',
    'build\Apply-Patch-And-Deploy.ps1',
    'build\Build-Release.ps1',
    'build\Compile-Source.ps1',
    'build\Test-Refactor.ps1',
    'build\ValidationHarness.cs',
    'src\ue4ss\main.lua',
    'src\ue4ss\world_map.lua',
    'src\ue4ss\diagnostics.lua',
    'src\ue4ss\bosses.lua',
    'src\ue4ss\treasures.lua',
    'src\ue4ss\config.default.lua',
    'src\overlay\Program.cs',
    'src\overlay\Bridge\MotionBridgeReader.cs',
    'src\overlay\Bridge\MotionRecordParser.cs',
    'src\overlay\Bridge\SharedBridgeFile.cs',
    'src\overlay\Data\WorldBossCatalog.cs',
    'src\overlay\Data\WorldTreasureCatalog.cs',
    'src\overlay\Models\OverlayModels.cs',
    'src\overlay\UI\RadarForm.cs',
    'src\overlay\SaveData\SaveDatabaseFingerprint.cs',
    'src\overlay\SaveData\SaveDatabaseKeyReader.cs',
    'src\overlay\SaveData\TreasureSaveState.cs',
    'src\installer\Install.cmd',
    'src\installer\Install.ps1',
    'src\installer\Core\InstallationPipeline.cs',
    'src\host\DragonSwordWorldRadar.Common.ps1',
    'src\host\DragonSwordWorldRadar.Host.ps1',
    'src\host\DragonSwordWorldRadar.Watcher.ps1',
    'src\host\DragonSwordWorldRadar.Watcher.vbs',
    'src\tools\Collect-Diagnostics.cmd',
    'src\tools\Collect-Diagnostics.ps1',
    'metadata\build-validation.json',
    'metadata\source-release-map.json',
    'metadata\source-snapshot.json',
    'vendor\sqlcipher\e_sqlcipher.dll',
    'vendor\ooz\ooz.exe',
    'resources\defaults\treasure_overrides.txt',
    'resources\enabled.txt'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)) {
        throw "Missing required source file: $relative"
    }
}

$forbidden = @(
    'src\ue4ss\boss_tracker.lua',
    'src\overlay\Bridge\StaticStateBridgeReader.cs',
    'src\overlay\Models\RadarState.cs',
    'src\ue4ss\treasure_event_probe.lua',
    'src\ue4ss\boss_respawn_probe.lua',
    'src\overlay\SaveData\BossCooldownProbe.cs'
)
foreach ($relative in $forbidden) {
    if (Test-Path -LiteralPath (Join-Path $root $relative)) {
        throw "Forbidden stale or experimental file remains: $relative"
    }
}

$overlaySources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File)
$installerSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File)
$luaSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\ue4ss') -Filter '*.lua' -File)
if ($overlaySources.Count -ne 31) { throw "Expected 31 Overlay C# files; found $($overlaySources.Count)." }
if ($installerSources.Count -ne 13) { throw "Expected 13 Installer C# files; found $($installerSources.Count)." }
if ($luaSources.Count -ne 6) { throw "Expected 6 UE4SS Lua files; found $($luaSources.Count)." }

foreach ($project in @(
    'src\overlay\DragonSwordWorldRadar.Overlay.csproj',
    'src\installer\Core\DragonSwordWorldRadar.Installer.Core.csproj')) {
    $raw = Get-Content -LiteralPath (Join-Path $root $project) -Raw
    if ($raw -notmatch '<TargetFramework>net48</TargetFramework>' -or
        $raw -notmatch '<LangVersion>7\.3</LangVersion>') {
        throw "$project must preserve net48 / C# 7.3 project metadata."
    }
}

$overlayProject = Get-Content -LiteralPath (Join-Path $root 'src\overlay\DragonSwordWorldRadar.Overlay.csproj') -Raw
if ($overlayProject -match 'System\.Web\.Extensions') {
    throw 'The removed Static JSON Bridge dependency remains in the Overlay project.'
}

# Production installation uses Windows PowerShell 5.1 CodeDOM. Reject common
# syntax that its compiler does not accept even though an IDE may accept it.
$unsupported = @(
    @{ Pattern = '\bnameof\s*\('; Name = 'nameof' },
    @{ Pattern = '\?\?='; Name = 'null-coalescing assignment' },
    @{ Pattern = '\busing\s+var\b'; Name = 'using declaration' },
    @{ Pattern = '\bout\s+var\b'; Name = 'out variable declaration' },
    @{ Pattern = '\bnew\s*\(\s*\)'; Name = 'target-typed new' },
    @{ Pattern = '\binit\s*;'; Name = 'init accessor' },
    @{ Pattern = '\bswitch\s*\{'; Name = 'switch expression' }
)
foreach ($source in @($overlaySources + $installerSources)) {
    $raw = Get-Content -LiteralPath $source.FullName -Raw
    foreach ($rule in $unsupported) {
        if ($raw -match [string]$rule.Pattern) {
            throw "Unsupported production compiler syntax ($($rule.Name)) in $($source.FullName)."
        }
    }
}

$sourceTextFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Where-Object { $_.Extension -in @('.ps1','.lua','.json','.md','.txt','.cs','.cmd','.vbs','.csproj') })
if (@($sourceTextFiles | Select-String -Pattern 'RegisterHook\s*\(').Count -gt 0) {
    throw 'Experimental RegisterHook calls remain in the production source tree.'
}
if (@($sourceTextFiles | Select-String -Pattern 'FindAllOf\s*\(\s*["'']Character["'']').Count -gt 0) {
    throw 'Character enumeration remains in the production source tree.'
}
$overlayTextFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -File |
    Where-Object { $_.Extension -in @('.cs','.csproj') })
if (@($overlayTextFiles | Select-String -Pattern 'JavaScriptSerializer|System\.Web\.Extensions').Count -gt 0) {
    throw 'The removed Static JSON parser dependency remains in Overlay source.'
}
if (@($overlayTextFiles | Select-String -Pattern 'StaticStateBridgeReader|\bRadarState\b').Count -gt 0) {
    throw 'Obsolete Static Bridge model/reader symbols remain in Overlay source.'
}

$mainLua = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\main.lua') -Raw
$mainVersionMarker = 'version = "' + $version + '"'
if ($mainLua -notmatch [regex]::Escape($mainVersionMarker)) {
    throw "main.lua version does not match release.json: $version"
}
foreach ($marker in @(
    'WORLD_MAP_ACTIVE_INTERVAL_MS = 24',
    'MINIMAP_UPDATE_INTERVAL_MS = 250',
    'FAST_MOTION_INTERVAL_MS = 24',
    'MOTION_PROTOCOL_VERSION = 2',
    'FAST_MOTION_HEARTBEAT_MS = 1000',
    'MOTION_POSITION_EPSILON = 20.0',
    'MOTION_Z_EPSILON = 10.0',
    'Protocol-v2 Motion Bridge is the sole runtime IPC channel',
    'no JSON Static Bridge is required',
    'report_async_failure',
    'is_mod_enabled_in_mods_file')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "Single-Bridge producer marker is missing: $marker"
    }
}
$loopCount = ([regex]::Matches($mainLua, 'LoopAsync\s*\(')).Count
if ($loopCount -ne 3) {
    throw "Expected exactly three textual LoopAsync registrations; found $loopCount."
}
foreach ($forbiddenMarker in @(
    'require, "boss_tracker"',
    'local boss_tracker',
    'state_path_a',
    'state_path_b',
    'write_static',
    'build_static',
    'WORLD_MAP_DETECT_INTERVAL_MS',
    'WORLD_MAP_EXIT_FAST_MISSING_SAMPLES',
    'WORLD_MAP_REENTRY_BLOCK_MS')) {
    if ($mainLua -match [regex]::Escape($forbiddenMarker)) {
        throw "Obsolete or regressed producer marker remains: $forbiddenMarker"
    }
}

$motionParser = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Bridge\MotionRecordParser.cs') -Raw
foreach ($marker in @(
    'SupportedProtocolVersion = 2',
    'fixed 27-field',
    'protocolVersion != SupportedProtocolVersion',
    'sequence != trailingSequence',
    'textScale < 0.5',
    'textScale > 2.0')) {
    if ($motionParser -notmatch [regex]::Escape($marker)) {
        throw "Protocol-v2 parser marker is missing: $marker"
    }
}

$radar = Get-Content -LiteralPath (Join-Path $root 'src\overlay\UI\RadarForm.cs') -Raw
foreach ($marker in @(
    'ActiveTimerIntervalMs = 24',
    'WorldIdleTimerIntervalMs = 50',
    'RadarIdleTimerIntervalMs = 75',
    'DisabledTimerIntervalMs = 125',
    'BackgroundTimerIntervalMs = 500',
    'MaintenanceIntervalMs = 500',
    'GeometryCheckIntervalMs = 1000',
    'GameLifetimeCheckIntervalMs = 1000',
    'MotionStaleTimeoutMs = 2500',
    'new WorldBossCatalog()',
    'frame.ProtocolVersion',
    'SwpNoCopyBits',
    'MAP_SURFACE_PREPARED',
    'Invalidate();',
    'Update();')) {
    if ($radar -notmatch [regex]::Escape($marker)) {
        throw "Overlay marker is missing: $marker"
    }
}
foreach ($forbiddenMarker in @(
    'StaticStatePollIntervalMs',
    'StaticStateBridgeReader',
    'JavaScriptSerializer',
    'WorldMapRevealDelayMs',
    '_worldMapRevealUtc',
    'world-map-warmup')) {
    if ($radar -match [regex]::Escape($forbiddenMarker)) {
        throw "Obsolete Overlay marker remains: $forbiddenMarker"
    }
}

$bossCatalog = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Data\WorldBossCatalog.cs') -Raw
foreach ($marker in @(
    'ExpectedBossCount = 9',
    'duplicate boss ID',
    'changed while it was being read')) {
    if ($bossCatalog -notmatch [regex]::Escape($marker)) {
        throw "World-Boss catalog validation marker is missing: $marker"
    }
}

$fingerprint = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveDatabaseFingerprint.cs') -Raw
if ($fingerprint -match [regex]::Escape('Flush(true)')) {
    throw 'Save snapshot still forces a physical-disk Flush(true).'
}
if ($fingerprint -notmatch [regex]::Escape('output.Flush();')) {
    throw 'Expected normal buffered save-snapshot Flush() marker is missing.'
}

$keyReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveDatabaseKeyReader.cs') -Raw
foreach ($marker in @(
    'TimeSpan.FromSeconds(30)',
    'DateTime.UtcNow.Add(PatternScanDelay)',
    '_patternScanAttempted = true',
    'DetectOwnerPointerRva')) {
    if ($keyReader -notmatch [regex]::Escape($marker)) {
        throw "Save-key startup optimization marker is missing: $marker"
    }
}

$saveState = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
if ($saveState -notmatch [regex]::Escape('public bool HasLoadedSaveState')) {
    throw 'Initial save-state visibility gate is missing.'
}

$program = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Program.cs') -Raw
foreach ($marker in @('ErrorLog.StartSession()','Overlay session started')) {
    if ($program -notmatch [regex]::Escape($marker)) {
        throw "Overlay session-log marker is missing: $marker"
    }
}

foreach ($relative in @(
    'src\host\DragonSwordWorldRadar.Host.ps1',
    'src\host\DragonSwordWorldRadar.Watcher.ps1',
    'src\host\DragonSwordWorldRadar.Watcher.vbs')) {
    $hostText = Get-Content -LiteralPath (Join-Path $root $relative) -Raw
    if ($hostText -notmatch [regex]::Escape($version)) {
        throw "Host version does not match release.json: $relative"
    }
}

$hostScriptText = Get-Content -LiteralPath (Join-Path $root 'src\host\DragonSwordWorldRadar.Host.ps1') -Raw
if ($hostScriptText -match 'System\.Web\.Extensions|JavaScriptSerializer') {
    throw 'Host still loads the removed JSON serializer assembly.'
}

$watcherVbs = Get-Content -LiteralPath (Join-Path $root 'src\host\DragonSwordWorldRadar.Watcher.vbs') -Raw
foreach ($marker in @(
    'shell.Run(command, 0, True)',
    'WATCHER_STOP_REQUESTED',
    'OVERLAY_PROCESS_EXIT',
    'NormalizeStamp')) {
    if ($watcherVbs -notmatch [regex]::Escape($marker)) {
        throw "Per-session hidden watcher marker is missing: $marker"
    }
}

$installCmd = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.cmd') -Raw
if ($installCmd -notmatch '(?i)powershell\.exe' -or $installCmd -match '(?i)\bpwsh(?:\.exe)?\b') {
    throw 'Install.cmd must use Windows PowerShell and must not use PowerShell Core.'
}
$install = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.ps1') -Raw
foreach ($marker in @(
    'OVERLAY_COMPILE_OK',
    'INSTALLER_COMPILE_OK',
    'REMOVED_OBSOLETE',
    'SESSION_WATCHER_READY',
    'INSTALL_COMPLETE')) {
    if ($install -notmatch [regex]::Escape($marker)) {
        throw "Installer marker is missing: $marker"
    }
}
foreach ($obsolete in @(
    'src\overlay\Bridge\StaticStateBridgeReader.cs',
    'src\overlay\Models\RadarState.cs',
    'scripts\boss_tracker.lua')) {
    if ($install -notmatch [regex]::Escape($obsolete)) {
        throw "Installer upgrade cleanup entry is missing: $obsolete"
    }
}

$collector = Get-Content -LiteralPath (Join-Path $root 'src\tools\Collect-Diagnostics.ps1') -Raw
if ($collector -match "'radar_state_a\.json'|'radar_state_b\.json'|'radar_state\.json'") {
    throw 'Diagnostics still treats legacy Static Bridge files as active captures.'
}
foreach ($marker in @('radar_motion_a.dat','radar_motion_b.dat','WorldBossCatalog.cs')) {
    if ($collector -notmatch [regex]::Escape($marker)) {
        throw "Diagnostics marker is missing: $marker"
    }
}

foreach ($jsonFile in @(Get-ChildItem -LiteralPath (Join-Path $root 'metadata') -Filter '*.json' -File)) {
    try { $null = Get-Content -LiteralPath $jsonFile.FullName -Raw | ConvertFrom-Json -ErrorAction Stop }
    catch { throw "Invalid JSON metadata: $($jsonFile.FullName): $($_.Exception.Message)" }
}

$allowedTool = [IO.Path]::GetFullPath((Join-Path $root 'vendor\ooz\ooz.exe'))
$unexpectedExe = @(Get-ChildItem -LiteralPath $root -Recurse -Filter '*.exe' -File -ErrorAction SilentlyContinue |
    Where-Object {
        $_.FullName -notlike (Join-Path $root 'dist\*') -and
        $_.FullName -notlike (Join-Path $root 'runtime\*') -and
        [IO.Path]::GetFullPath($_.FullName) -ne $allowedTool
    })
if ($unexpectedExe.Count -gt 0) {
    throw ('Unexpected EXE files found: ' + (($unexpectedExe.FullName) -join ', '))
}
$tool = @($release.bundled_tools | Where-Object { [string]$_.name -ieq 'ooz.exe' })
if ($tool.Count -ne 1) { throw 'release.json must define exactly one ooz.exe entry.' }
$actualToolHash = (Get-FileHash -LiteralPath $allowedTool -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualToolHash -ne ([string]$tool[0].sha256).ToLowerInvariant()) {
    throw 'vendor/ooz/ooz.exe does not match release.json.'
}

$backupArtifacts = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Where-Object { $_.Name -match '(\.bak$|\.orig$|\.before-|~$)' })
if ($backupArtifacts.Count -gt 0) {
    throw ('Backup artifacts remain in src: ' + (($backupArtifacts.FullName) -join ', '))
}

Write-Host "Source verification passed for DragonSwordWorldRadar $version; overlay=31; installer=13; lua=6; motionProtocol=2; fields=27."
