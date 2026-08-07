#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json
$version = [string]$release.version
if ([string]::IsNullOrWhiteSpace($version)) {
    throw 'metadata/release.json has no version.'
}

$required = @(
    'Apply-Patch-And-Deploy.cmd',
    'build\Apply-Patch-And-Deploy.ps1',
    'src\ue4ss\main.lua',
    'src\ue4ss\world_map.lua',
    'src\ue4ss\diagnostics.lua',
    'src\ue4ss\bosses.lua',
    'src\ue4ss\boss_tracker.lua',
    'src\ue4ss\treasures.lua',
    'src\ue4ss\config.default.lua',
    'src\overlay\Program.cs',
    'src\overlay\Configuration\DebugSettings.cs',
    'src\overlay\Diagnostics\ErrorLog.cs',
    'src\overlay\UI\RadarForm.cs',
    'src\overlay\Bridge\MotionBridgeReader.cs',
    'src\overlay\Bridge\MotionRecordParser.cs',
    'src\overlay\Diagnostics\OverlayPerformanceTracker.cs',
    'src\overlay\Rendering\WorldTreasureRenderBuffer.cs',
    'src\overlay\SaveData\BossRespawnRuleResolver.cs',
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
    'build\Compile-Source.ps1',
    'build\Test-Refactor.ps1',
    'build\ValidationHarness.cs',
    'metadata\build-validation.json',
    'metadata\source-release-map.json',
    'vendor\sqlcipher\e_sqlcipher.dll',
    'vendor\ooz\ooz.exe',
    'resources\defaults\treasure_overrides.txt'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)) {
        throw "Missing required source file: $relative"
    }
}

$forbidden = @(
    'src\ue4ss\treasure_event_probe.lua',
    'src\ue4ss\boss_respawn_probe.lua',
    'src\overlay\SaveData\BossCooldownProbe.cs',
    'src\overlay\UI\RadarForm.cs.before-dev6-compiler-fix'
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
if ($luaSources.Count -ne 7) { throw "Expected 7 UE4SS Lua files; found $($luaSources.Count)." }

foreach ($project in @(
    'src\overlay\DragonSwordWorldRadar.Overlay.csproj',
    'src\installer\Core\DragonSwordWorldRadar.Installer.Core.csproj')) {
    $raw = Get-Content -LiteralPath (Join-Path $root $project) -Raw
    if ($raw -notmatch '<TargetFramework>net48</TargetFramework>' -or
        $raw -notmatch '<LangVersion>7\.3</LangVersion>') {
        throw "$project must preserve the tested net48 / C# 7.3 project metadata."
    }
}

# The csproj language setting is for IDE builds. Production installation uses
# Windows PowerShell 5.1 CodeDOM, so reject high-confidence syntax it cannot compile.
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
    Where-Object { $_.Extension -in @('.ps1','.lua','.json','.md','.txt','.cs','.cmd','.vbs') })
if (@($sourceTextFiles | Select-String -Pattern 'RegisterHook\s*\(').Count -gt 0) {
    throw 'Experimental RegisterHook calls remain in the production source tree.'
}
if (@($sourceTextFiles | Select-String -Pattern 'FindAllOf\s*\(\s*["'']Character["'']').Count -gt 0) {
    throw 'Character enumeration remains in the production source tree.'
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
    'FAST_MOTION_HEARTBEAT_MS = 1000',
    'MOD_SWITCH_CHECK_INTERVAL_MS = 5000',
    'if world_map_was_active and latest_motion_map_state ~= nil then',
    'report_async_failure',
    'is_mod_enabled_in_mods_file',
    'Overlay launch requested through hidden resident watcher',
    'request:write(tostring(os.time()))')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "Performance producer marker is missing: $marker"
    }
}
foreach ($forbiddenMarker in @(
    'WORLD_MAP_DETECT_INTERVAL_MS',
    'WORLD_MAP_EXIT_FAST_MISSING_SAMPLES',
    'WORLD_MAP_REENTRY_BLOCK_MS',
    'os.execute',
    'cmd.exe /c',
    'request:write(tostring(os.time()),')) {
    if ($mainLua -match [regex]::Escape($forbiddenMarker)) {
        throw "Stale always-on world-map detector remains: $forbiddenMarker"
    }
}

$configLua = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\config.default.lua') -Raw
foreach ($marker in @(
    'use_logging = true',
    'debug_logging = false')) {
    if ($configLua -notmatch [regex]::Escape($marker)) {
        throw "Split logging configuration marker is missing: $marker"
    }
}

$diagnosticsLua = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\diagnostics.lua') -Raw
foreach ($marker in @(
    'DragonSwordWorldRadar.Lua.Use.log',
    'DragonSwordWorldRadar.Lua.Debug.log',
    'is_debug_enabled',
    'motion_sample_hz',
    'motion_write_skips',
    'motion_write_hz',
    'queue_over_250_ms')) {
    if ($diagnosticsLua -notmatch [regex]::Escape($marker)) {
        throw "Performance diagnostics marker is missing: $marker"
    }
}

$radar = Get-Content -LiteralPath (Join-Path $root 'src\overlay\UI\RadarForm.cs') -Raw
foreach ($marker in @(
    'ActiveTimerIntervalMs = 33',
    'WorldIdleTimerIntervalMs = 50',
    'RadarIdleTimerIntervalMs = 75',
    'DisabledTimerIntervalMs = 125',
    'StaticStatePollIntervalMs = 200',
    'MotionVisualSnapshot',
    'SetOverlayWindowVisible(false, false)',
    '_overlayWindowVisible',
    'SwShowNoActivate',
    'SwpNoCopyBits',
    'if (!_overlayWindowVisible && sizeChanged)',
    'positionFlags |= NativeMethods.SwpNoCopyBits',
    'else if (_overlayWindowVisible && sizeChanged)',
    '_hiddenSurfacePreparedForCurrentTick',
    'paintSequenceBeforeUpdate',
    'MAP_SURFACE_PREPARED',
    'needsInvalidate',
    'Invalidate();',
    'Update();',
    'targetWidth = _overlaySize;',
    'EVENTRADAR_GAME_PID',
    'IsEnabledInModsFile',
    'resetChanged')) {
    if ($radar -notmatch [regex]::Escape($marker)) {
        throw "Performance/CodeDOM marker is missing: $marker"
    }
}
if ($radar -match 'NativeMethods\.SwpNoActivate\s*\|\s*NativeMethods\.SwpNoCopyBits') {
    throw 'SWP_NOCOPYBITS must be conditional on a hidden size transition.'
}
$paintSequenceIncrementCount = ([regex]::Matches(
    $radar,
    [regex]::Escape('_paintSequence++;'))).Count
if ($paintSequenceIncrementCount -ne 1 -or
    $radar -notmatch '(?s)if \(!paintFailed\).*?_paintSequence\+\+;') {
    throw 'Hidden-prepaint confirmation must advance only after a successful paint.'
}

foreach ($forbiddenMarker in @(
    'GetRadarDrawBounds',
    'InvalidatePresentedContent',
    'WorldMapRevealDelayMs',
    '_worldMapRevealUtc',
    'world-map-warmup')) {
    if ($radar -match [regex]::Escape($forbiddenMarker)) {
        throw "Stale fixed-fullscreen radar helper remains: $forbiddenMarker"
    }
}

$performanceTracker = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Diagnostics\OverlayPerformanceTracker.cs') -Raw
foreach ($marker in @(
    'gaps50=',
    'gaps100=',
    'gaps250=',
    'overlayPaintFps_not_game_present_fps')) {
    if ($performanceTracker -notmatch [regex]::Escape($marker)) {
        throw "Overlay debug performance marker is missing: $marker"
    }
}

$errorLog = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Diagnostics\ErrorLog.cs') -Raw
foreach ($marker in @(
    'DragonSwordWorldRadar.Overlay.Use.log',
    'DragonSwordWorldRadar.Overlay.Debug.log',
    'StartSession')) {
    if ($errorLog -notmatch [regex]::Escape($marker)) {
        throw "Overlay split-log marker is missing: $marker"
    }
}

$debugSettings = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Configuration\DebugSettings.cs') -Raw
foreach ($marker in @(
    'debug_logging',
    'diagnostic_verbose',
    'StartupEnabled = LoadAtStartup()',
    'get { return StartupEnabled; }')) {
    if ($debugSettings -notmatch [regex]::Escape($marker)) {
        throw "Overlay startup-only debug setting marker is missing: $marker"
    }
}
foreach ($forbiddenMarker in @(
    'RefreshInterval',
    '_nextRefreshUtc',
    'FileInfo completedInfo',
    'lock (Sync)')) {
    if ($debugSettings -match [regex]::Escape($forbiddenMarker)) {
        throw "Runtime Debug-config polling remains: $forbiddenMarker"
    }
}
foreach ($forbiddenMarker in @(
    'RefreshDebugMode()',
    '_nextDebugModeRefreshUtc')) {
    if ($radar -match [regex]::Escape($forbiddenMarker)) {
        throw "Radar timer still polls Debug configuration: $forbiddenMarker"
    }
}

$bossRuleResolver = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\BossRespawnRuleResolver.cs') -Raw
foreach ($marker in @(
    'ResetHour = 9',
    'ResetMinute = 0',
    'localReset=09:00',
    'source=built-in-original')) {
    if ($bossRuleResolver -notmatch [regex]::Escape($marker)) {
        throw "Fixed Boss rule marker is missing: $marker"
    }
}
foreach ($forbiddenMarker in @(
    'Directory.GetFiles',
    'File.ReadAllBytes',
    'Regex.',
    'TryDetectReadableOverride',
    'ScanInterval',
    'public void Refresh()')) {
    if ($bossRuleResolver -match [regex]::Escape($forbiddenMarker)) {
        throw "Runtime Boss PAK scanning remains: $forbiddenMarker"
    }
}
$saveStateSource = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
if ($saveStateSource -match [regex]::Escape('_bossRuleResolver.Refresh()')) {
    throw 'Save worker still invokes the removed Boss rule scanner.'
}

$program = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Program.cs') -Raw
foreach ($marker in @(
    'ErrorLog.StartSession()',
    'Overlay session started')) {
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

$watcherVbs = Get-Content -LiteralPath (Join-Path $root 'src\host\DragonSwordWorldRadar.Watcher.vbs') -Raw
foreach ($marker in @(
    'shell.Run(command, 0, True)',
    'WATCHER_STOP_REQUESTED',
    'OVERLAY_PROCESS_EXIT',
    'NormalizeStamp',
    'Len(normalized) > 32')) {
    if ($watcherVbs -notmatch [regex]::Escape($marker)) {
        throw "Resident hidden watcher marker is missing: $marker"
    }
}
$exitDoCount = ([regex]::Matches($watcherVbs, 'Exit Do')).Count
if ($exitDoCount -ne 1) {
    throw "Watcher must exit only for watcher.stop; found $exitDoCount Exit Do statements."
}

$keyReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveDatabaseKeyReader.cs') -Raw
foreach ($marker in @(
    'DateTime.UtcNow.AddSeconds(30)',
    '_fallbackScanAttempted = true',
    'Task.Run(',
    'TryDetectOwnerPointerRva',
    'completedProcessId == game.Id',
    'CurrentOwnerPointerRva != _ownerPointerRva')) {
    if ($keyReader -notmatch [regex]::Escape($marker)) {
        throw "Save-key startup optimization marker is missing: $marker"
    }
}
if ($keyReader -match [regex]::Escape('IsKeyInitializationPending')) {
    throw 'Save-key fallback must not be permanently suppressed by a transient key-not-ready state.'
}

foreach ($marker in @(
    'BuildExceptionSignature',
    'IsExpectedStartupNotReady',
    'Save-state initialization pending')) {
    if ($saveStateSource -notmatch [regex]::Escape($marker)) {
        throw "Save-state startup log marker is missing: $marker"
    }
}

$installCmd = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.cmd') -Raw
if ($installCmd -notmatch '(?i)powershell\.exe' -or $installCmd -match '(?i)\bpwsh(?:\.exe)?\b') {
    throw 'Install.cmd must use Windows PowerShell and must not use PowerShell Core.'
}
$install = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.ps1') -Raw
foreach ($marker in @('OVERLAY_COMPILE_OK','INSTALLER_COMPILE_OK','MOD_SETTING_PRESERVED','RESIDENT_WATCHER_READY','INSTALL_COMPLETE','DragonSwordWorldRadar.lnk','wscript.exe','-WindowStyle Hidden')) {
    if ($install -notmatch [regex]::Escape($marker)) {
        throw "Installer marker is missing: $marker"
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
        $_.FullName -notlike (Join-Path $root '.git\*') -and
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

Write-Host "Source verification passed for DragonSwordWorldRadar $version; overlay=31; installer=13; lua=7."
