#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$releasePath = Join-Path $root 'metadata\release.json'
$release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
$version = [string]$release.version
$clockEnabled = $version -like '0.4.0-dev39-*' -or $version -like '0.4.0-dev48-*' -or $version -like '0.4.0-dev49-*' -or $version -like '0.4.0-dev50-*' -or $version -like '0.4.0-dev51-*' -or $version -like '0.4.0-dev52-*' -or $version -like '0.4.0-dev53-*' -or $version -like '0.4.0-dev54-*' -or $version -like '0.4.0-dev55-*' -or $version -like '0.4.0-dev56-*' -or $version -like '0.4.0-dev57-*' -or $version -like '0.4.0-dev58-*' -or $version -like '0.4.0-dev59-*' -or $version -like '0.4.0-dev60-*' -or $version -like '0.4.0-dev61-*' -or $version -like '0.4.0-dev62-*' -or $version -like '0.4.0-dev63-*' -or $version -like '0.4.0-dev64-*' -or $version -like '0.4.0-dev65-*' -or $version -like '0.4.0-dev66-*' -or $version -like '0.4.0-dev67-*' -or $version -like '0.4.0-dev68-*' -or $version -like '0.4.0-dev69-*' -or $version -like '0.4.0-dev70-*' -or $version -like '0.4.0-dev71-*' -or $version -like '0.4.0-dev72-*' -or $version -like '0.4.0-dev73-*' -or $version -like '0.4.0-dev74-*'
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
    'build\Test-ReleasePackage.ps1',
    'build\ValidationHarness.cs',
    'src\ue4ss\main.lua',
    'src\ue4ss\world_map.lua',
    'src\ue4ss\diagnostics.lua',
    'src\ue4ss\bosses.lua',
    'src\ue4ss\treasures.lua',
    'src\ue4ss\config.lua',
    'src\ue4ss\world_environment.lua',
    'src\overlay\Program.cs',
    'src\overlay\Bridge\MotionBridgeReader.cs',
    'src\overlay\Bridge\MotionRecordParser.cs',
    'src\overlay\Bridge\SharedBridgeFile.cs',
    'src\overlay\Data\WorldBossCatalog.cs',
    'src\overlay\Data\WorldEncounterCatalog.cs',
    'src\overlay\Data\MoleRewardVisibilityIndex.cs',
    'src\overlay\Data\WorldTreasureCatalog.cs',
    'src\overlay\Models\OverlayModels.cs',
    'src\overlay\UI\RadarForm.cs',
    'src\overlay\Rendering\WorldStatusRenderer.cs',
    'src\overlay\SaveData\SaveDatabaseFingerprint.cs',
    'src\overlay\SaveData\SaveDatabaseKeyReader.cs',
    'src\overlay\SaveData\TreasureSaveState.cs',
    'src\overlay\SaveData\EncounterAvailabilityTracker.cs',
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

$overlaySources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' })
$installerSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' })
$luaSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\ue4ss') -Filter '*.lua' -File)
if ($overlaySources.Count -ne 41) { throw "Expected 41 Overlay C# files; found $($overlaySources.Count)." }
if ($installerSources.Count -ne 19) { throw "Expected 19 Installer C# files; found $($installerSources.Count)." }
if ($luaSources.Count -ne 9) { throw "Expected 9 UE4SS Lua files; found $($luaSources.Count)." }

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
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' -and $_.Extension -in @('.ps1','.lua','.json','.md','.txt','.cs','.cmd','.vbs','.csproj') })
if (@($sourceTextFiles | Select-String -Pattern 'RegisterHook\s*\(').Count -gt 0) {
    throw 'Experimental RegisterHook calls remain in the production source tree.'
}
if (@($sourceTextFiles | Select-String -Pattern 'FindAllOf\s*\(\s*["'']Character["'']').Count -gt 0) {
    throw 'Character enumeration remains in the production source tree.'
}
$overlayTextFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' -and $_.Extension -in @('.cs','.csproj') })
if (@($overlayTextFiles | Select-String -Pattern 'JavaScriptSerializer|System\.Web\.Extensions').Count -gt 0) {
    throw 'The removed Static JSON parser dependency remains in Overlay source.'
}
if (@($overlayTextFiles | Select-String -Pattern 'StaticStateBridgeReader|\bRadarState\b').Count -gt 0) {
    throw 'Obsolete Static Bridge model/reader symbols remain in Overlay source.'
}

$mainLua = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\main.lua') -Raw
$worldMapLua = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\world_map.lua') -Raw
$commonHost = Get-Content -LiteralPath (Join-Path $root 'src\host\DragonSwordWorldRadar.Common.ps1') -Raw
$deployHelper = Get-Content -LiteralPath (Join-Path $root 'build\Apply-Patch-And-Deploy.ps1') -Raw
$mainVersionMarker = 'version = "' + $version + '"'
if ($mainLua -notmatch [regex]::Escape($mainVersionMarker)) {
    throw "main.lua version does not match release.json: $version"
}
if ($mainLua -notmatch [regex]::Escape('win64_path .. "\\ue4ss\\Mods\\DragonSwordWorldRadar"') -or
    $mainLua -match [regex]::Escape('win64_path .. "\\Mods\\DragonSwordWorldRadar"')) {
    throw 'Lua fallback must resolve the standard Win64/ue4ss/Mods runtime root.'
}
if ($commonHost -notmatch [regex]::Escape("Join-Path `$win64Root 'ue4ss\Mods\ooz.exe'")) {
    throw 'The installer host must use the standard UE4SS Mods-root Oodle fallback.'
}
if ($deployHelper -notmatch [regex]::Escape("'Binaries\Win64\ue4ss\Mods'")) {
    throw 'The local deployment helper must target Win64/ue4ss/Mods.'
}
foreach ($marker in @(
    'WORLD_MAP_ACTIVE_INTERVAL_MS = 8',
    'MINIMAP_UPDATE_INTERVAL_MS = 250',
    'FAST_MOTION_INTERVAL_MS = 50',
    'MOTION_PROTOCOL_VERSION = 6',
    'FAST_MOTION_HEARTBEAT_MS = 1000',
    'MOTION_POSITION_EPSILON_FLOOR = 20.0',
    'MOTION_SCREEN_PIXEL_EPSILON = 0.5',
    'motion_position_epsilon(radius)',
    'MOTION_Z_EPSILON = 10.0',
    'Protocol-v6 Motion Bridge is the sole runtime IPC channel',
    'no JSON Static Bridge is required',
    'report_async_failure',
    'is_mod_enabled_in_mods_file')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "Single-Bridge producer marker is missing: $marker"
    }
}
foreach ($obsoleteActiveConstant in @('WORLD_MAP_ACTIVE_INTERVAL_MS = 24','WORLD_MAP_ACTIVE_INTERVAL_MS = 50','FAST_MOTION_INTERVAL_MS = 24')) {
    if ($mainLua -match [regex]::Escape($obsoleteActiveConstant)) {
        throw "Obsolete 24 ms active producer constant remains: $obsoleteActiveConstant"
    }
}
foreach ($marker in @('MAX_CANDIDATES = 2','MAX_RETIRED_IDENTITIES = 256','entry.epoch == lifecycle_epoch','entry.token == candidate_token','retired_identities[identity]','notify-current-epoch','bounded-resume-scan','candidate_token = candidate_token + 1')) {
    if ($worldMapLua -notmatch [regex]::Escape($marker)) {
        throw "Bounded world-map candidate marker is missing: $marker"
    }
}
foreach ($marker in @('function WorldMap.recover_session()','function WorldMap.has_retained_candidates()','record_candidate_metrics("recover-session")')) {
    if ($worldMapLua -notmatch [regex]::Escape($marker)) {
        throw "World-map bounded recovery marker is missing: $marker"
    }
}
foreach ($marker in @('world_map.recover_session()','world_map.has_retained_candidates()')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "World-map control recovery marker is missing: $marker"
    }
}
$worldMapRecoverStart = $worldMapLua.IndexOf('function WorldMap.recover_session()')
$worldMapRecoverEnd = $worldMapLua.IndexOf('function WorldMap.has_retained_candidates()', $worldMapRecoverStart)
if ($worldMapRecoverStart -lt 0 -or $worldMapRecoverEnd -le $worldMapRecoverStart) {
    throw 'World-map recovery block is missing or malformed.'
}
$worldMapRecoverBlock = $worldMapLua.Substring($worldMapRecoverStart, $worldMapRecoverEnd - $worldMapRecoverStart)
foreach ($marker in @('cached_entry = nil','candidates = {}','candidate_token = candidate_token + 1','needs_rescan = true','wake_hint = true')) {
    if ($worldMapRecoverBlock -notmatch [regex]::Escape($marker)) {
        throw "World-map recovery safety marker is missing: $marker"
    }
}
if ($worldMapRecoverBlock -match [regex]::Escape('FindAllOf(')) {
    throw 'Active world-map recovery directly enumerates UObjects.'
}
if ($worldMapLua -match [regex]::Escape('known_layers')) {
    throw 'Unbounded retained world-map widget list remains.'
}
if (([regex]::Matches($worldMapLua,[regex]::Escape('FindAllOf("DLayerMap_C")'))).Count -ne 1) {
    throw 'World-map widget enumeration is not confined to one bounded resume scan.'
}
foreach ($marker in @('motion_loop_token','world_map_loop_token','owner_loop_token ~= motion_loop_token','owner_loop_token ~= world_map_loop_token','update_pending_token == request_token','world_motion_pending_token == request_token','record_loop_event')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "Async loop/request ownership marker is missing: $marker"
    }
}
foreach ($marker in @('CONTROL_WATCHDOG_SAMPLE_MS = 1000','CONTROL_WATCHDOG_STALE_SAMPLES = 5','MAX_AUTOMATIC_RUNTIME_RESTARTS = 3','local function observe_control_watchdog(delta_ms)','request_runtime_restart("control_watchdog_stalled")','request_runtime_restart("async_failure:" .. tostring(key))','automatic F8->F7 recovery','enter_world_transition("runtime_error_restart")')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "Confirmed-error automatic recovery marker is missing: $marker"
    }
}
if (([regex]::Matches($mainLua,[regex]::Escape('request_runtime_restart('))).Count -ne 3) {
    throw 'Runtime restart may be armed outside the function definition, confirmed async failure, and control watchdog.'
}
$controlWatchdogStart = $mainLua.IndexOf('local function observe_control_watchdog(delta_ms)')
$controlWatchdogEnd = $mainLua.IndexOf('local function purge_runtime_references()', $controlWatchdogStart)
if ($controlWatchdogStart -lt 0 -or $controlWatchdogEnd -le $controlWatchdogStart) {
    throw 'Control-watchdog block is missing or malformed.'
}
$controlWatchdogBlock = $mainLua.Substring($controlWatchdogStart, $controlWatchdogEnd - $controlWatchdogStart)
foreach ($forbidden in @('FindAllOf(','FindFirstOf(','get_player_location(','world_map.read_state(','ExecuteInGameThread(','write_fast_motion(')) {
    if ($controlWatchdogBlock -match [regex]::Escape($forbidden)) {
        throw "Control watchdog performs non-scalar work: $forbidden"
    }
}
if ($worldMapLua -match [regex]::Escape('request_runtime_restart')) {
    throw 'Temporary world-map read loss can arm whole-runtime recovery.'
}
foreach ($marker in @('require, "mole_catalog"','require, "mole_completion"','mole_completion.refresh','mole_visible_mask')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) { throw "Mole/Fly single-bridge producer marker is missing: $marker" }
}
foreach ($unsafeWorldTimeMarker in @('FindFirstOf("DGameSingleton")')) {
    if ($mainLua -match [regex]::Escape($unsafeWorldTimeMarker)) { throw "Unsafe production world-time marker remains: $unsafeWorldTimeMarker" }
}
if ($clockEnabled) {
    foreach ($marker in @('require, "world_environment"','world_environment.arm()','world_environment.cancel()','world_environment.context_lost()','local function queue_world_time_capture()')) {
        if ($mainLua -notmatch [regex]::Escape($marker)) { throw "Isolated production world-status marker is missing: $marker" }
    }
} else {
    foreach ($marker in @('require, "world_environment"','world_environment.','FindFirstOf("DGameSingleton")')) {
        if ($mainLua -match [regex]::Escape($marker)) { throw "Assault-only dev38 performs production clock work: $marker" }
    }
}
foreach ($marker in @('local function has_configured_visible_features()','return has_radar_marker_layers() or show_world_status','if not enabled or not ensure_layers_loaded() then','published_show_world_status = is_enabled == true and show_world_status')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) { throw "Whole-mod F7/F8 lifecycle marker is missing: $marker" }
}
foreach ($marker in @(
    'local world_epoch = 1',
    'local function is_lifecycle_epoch_current(epoch)',
    'purge_runtime_references()',
    'is_world_epoch_current(scheduled_epoch)',
    'local function begin_post_cooldown_probe(expected_epoch, source)',
    'complete_post_cooldown_probe = function()',
    'resume_probe_pending = true',
    'reference 1.6.1 Pawn-loss cooldown retry enabled',
    'WORLD_TRANSITION_ENTER',
    'WORLD_REFERENCE_PURGE',
    'WORLD_TRANSITION_PROBE',
    'WORLD_TRANSITION_RESUME')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "World-transition lifecycle marker is missing: $marker"
    }
}
foreach ($removedRecoveryProbe in @(
    'resolve_fresh_player_roots',
    'try_reacquire_world',
    'queue_world_recovery',
    'recovery_stable_samples',
    'recovery_identity',
    'active_world_identity',
    'GetWorld()',
    'transition_post_seen')) {
    if ($mainLua -match [regex]::Escape($removedRecoveryProbe)) {
        throw "Unsafe world-identity recovery marker remains: $removedRecoveryProbe"
    }
}
foreach ($unsafeHook in @('RegisterLoadMapPreHook','RegisterLoadMapPostHook')) {
    if ($mainLua -match [regex]::Escape($unsafeHook)) {
        throw "Unsafe LoadMap lifecycle hook remains: $unsafeHook"
    }
}
foreach ($retainedWrapper in @('local player_controller = nil','local player_pawn = nil')) {
    if ($mainLua -match [regex]::Escape($retainedWrapper)) {
        throw "Unnecessary retained UObject wrapper remains in the main lifecycle: $retainedWrapper"
    }
}
foreach ($marker in @(
    'local engine = nil',
    'local minimap_layer = nil',
    'local function resolve_player_controller()',
    'if engine == nil then',
    'engine = FindFirstOf("Engine")',
    'local function is_valid_object(object)',
    'local function read_minimap_scale()',
    'cache_hit = is_valid_object(minimap_layer)',
    'if not cache_hit then',
    'FindFirstOf("DLayerMiniMap")',
    'object:IsValid()')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) {
        throw "Reference cached-context marker is missing: $marker"
    }
}
foreach ($unsafeMinimapAccess in @('FindFirstOf("DPanelMain")','Overlay_Minimap','current_main_panel.','current_minimap_layer:IsVisible()','surface_hidden','compact_surface_visible','set_compact_surface_visible')) {
    if ($mainLua -match [regex]::Escape($unsafeMinimapAccess)) {
        throw "Unsafe parent-HUD minimap access remains: $unsafeMinimapAccess"
    }
}
if ($mainLua -match [regex]::Escape('FindAllOf("DLayerMiniMap')) {
    throw 'Minimap scale sampling must not enumerate UObjects.'
}
if (([regex]::Matches($mainLua, [regex]::Escape('FindFirstOf("Engine")'))).Count -ne 1) { throw 'Engine must be resolved only by the reference cache-miss path.' }
if (([regex]::Matches($mainLua, [regex]::Escape('FindFirstOf("DLayerMiniMap")'))).Count -ne 1) { throw 'DLayerMiniMap must be resolved only by the reference cache-miss path.' }
$minimapReadStart = $mainLua.IndexOf('local function read_minimap_scale()')
$minimapReadEnd = $mainLua.IndexOf('local function update_radar_radius(scale)', $minimapReadStart)
if ($minimapReadStart -lt 0 -or $minimapReadEnd -le $minimapReadStart) { throw 'Minimap scale reader boundaries are missing.' }
$minimapReadBlock = $mainLua.Substring($minimapReadStart, $minimapReadEnd - $minimapReadStart)
foreach ($forbiddenNestedValidation in @('is_valid_object(layer_map)','is_valid_object(map_overlay)','MINIMAP_LAYERMAP_ISVALID','MINIMAP_OVERLAY_ISVALID')) {
    if ($minimapReadBlock.Contains($forbiddenNestedValidation)) {
        throw "Nested minimap wrapper uses incompatible UObject validation: $forbiddenNestedValidation"
    }
}
foreach ($guardedNestedRead in @('local layer_map = current_minimap_layer.LayerMap','local map_overlay = layer_map.MapOverlay','tonumber(map_overlay.RenderTransform.Scale.X)')) {
    if (-not $minimapReadBlock.Contains($guardedNestedRead)) {
        throw "Guarded nested minimap read is missing: $guardedNestedRead"
    }
}
$stableLocationStart = $mainLua.IndexOf('get_player_location = function()')
$stableLocationEnd = $mainLua.IndexOf('local function is_valid_object(object)', $stableLocationStart)
$stableLocationBlock = $mainLua.Substring($stableLocationStart, $stableLocationEnd - $stableLocationStart)
foreach ($scanMarker in @('resolve_fresh_player_roots()','GetWorld()','GetFullName()','FindFirstOf(')) {
    if ($stableLocationBlock.Contains($scanMarker)) { throw "Stable player-location path performs recovery/object scan work: $scanMarker" }
}
foreach ($freshPawnMarker in @('local controller = resolve_player_controller()','local current_pawn = controller.Pawn','current_pawn:K2_GetActorLocation()')) {
    if (-not $stableLocationBlock.Contains($freshPawnMarker)) { throw "Reference current-Pawn path is missing: $freshPawnMarker" }
}
$cooldownProbeStart = $mainLua.IndexOf('local function begin_post_cooldown_probe(expected_epoch, source)')
$cooldownProbeEnd = $mainLua.IndexOf('local function activation_game_thread_callback()', $cooldownProbeStart)
$cooldownProbeBlock = $mainLua.Substring($cooldownProbeStart, $cooldownProbeEnd - $cooldownProbeStart)
foreach ($unsafeCooldownAccess in @('FindFirstOf','FindAllOf','StaticFindObject','ExecuteInGameThread','.Pawn','IsValid','GetWorld','GetFullName','K2_GetActorLocation')) {
    if ($cooldownProbeBlock.Contains($unsafeCooldownAccess)) { throw "Cooldown release performs UObject work: $unsafeCooldownAccess" }
}
if (([regex]::Matches($mainLua, [regex]::Escape('world_map.set_suspended(false, world_epoch)'))).Count -ne 1) {
    throw 'World-map access must resume exactly once, after a valid player-location sample.'
}
foreach ($obsoleteLifecycle in @('if show_world_status then ensure_loop_started() end','F7/F8 control radar markers only','Status-only updates remain on the 250 ms control path','mode = "status"','automatic world time remains active','not enabled and not show_world_status')) {
    if ($mainLua -match [regex]::Escape($obsoleteLifecycle)) { throw "Obsolete status-after-F8 lifecycle remains: $obsoleteLifecycle" }
}
foreach ($marker in @('local function has_radar_marker_layers()','return show_treasures or show_bosses or show_moles','if not has_configured_visible_features() then')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) { throw "Configured whole-mod F7 gate is missing: $marker" }
}
$missingPlayerIndex = $mainLua.IndexOf('if player_x == nil or player_y == nil then')
$missingPlayerClearIndex = $mainLua.IndexOf('world_time_available = false', $missingPlayerIndex)
$missingPlayerWriteIndex = $mainLua.IndexOf('write_disabled_motion()', $missingPlayerIndex)
if ($missingPlayerClearIndex -lt 0 -or $missingPlayerClearIndex -gt $missingPlayerWriteIndex) {
    throw 'Player-context loss does not clear published world time before the lifecycle frame.'
}
$modDisableIndex = $mainLua.IndexOf('local function disable_for_mod_switch()')
$modClearIndex = $mainLua.IndexOf('world_time_available = false', $modDisableIndex)
$modWriteIndex = $mainLua.IndexOf('write_disabled_motion()', $modDisableIndex)
if ($modClearIndex -lt 0 -or $modClearIndex -gt $modWriteIndex) {
    throw 'mods.txt disable does not clear published world time before the lifecycle frame.'
}
if ($mainLua -match [regex]::Escape('Game time is sampled once per second')) {
    throw 'Stale one-hertz Ready log claim remains.'
}
if ($clockEnabled -and $mainLua -notmatch [regex]::Escape('one isolated baseline attempt after stable context')) {
    throw 'Isolated world-status Ready diagnostic is missing.'
}
if ($clockEnabled -and
    ($mainLua -notmatch [regex]::Escape('local capture_failure = "none"') -or
     $mainLua -notmatch [regex]::Escape('failure = capture_failure'))) {
    throw 'Successful world-clock diagnostics do not clear the failure field.'
}
if ($mainLua -match [regex]::Escape('failure = capture_ok and capture_error')) {
    throw 'Ambiguous world-clock failure expression remains.'
}
if (-not $clockEnabled -and $mainLua -notmatch [regex]::Escape('World status is fail-closed')) {
    throw 'Assault-only dev38 fail-closed world-status diagnostic is missing.'
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
    'SupportedProtocolVersion = 6',
    'fixed 38-field',
    'protocolVersion != SupportedProtocolVersion',
    'sequence != trailingSequence',
    'textScale < 0.5',
    'textScale > 2.0')) {
    if ($motionParser -notmatch [regex]::Escape($marker)) {
        throw "Protocol-v6 parser marker is missing: $marker"
    }
}

$radar = Get-Content -LiteralPath (Join-Path $root 'src\overlay\UI\RadarForm.cs') -Raw
$bossRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\BossMarkerRenderer.cs') -Raw
$moleRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\MoleMarkerRenderer.cs') -Raw
$overlayPerformance = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Diagnostics\OverlayPerformanceTracker.cs') -Raw
$motionReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Bridge\MotionBridgeReader.cs') -Raw
$luaDiagnostics = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\diagnostics.lua') -Raw
$defaultConfigText = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\config.lua') -Raw
if ($mainLua -notmatch [regex]::Escape('if not f7_trace_active or perf_diagnostics == nil then return end') -or
    $mainLua -notmatch [regex]::Escape('if perf_diagnostics == nil then') -or
    [regex]::IsMatch($mainLua, '(?<!perf_)diagnostics\.debug\(') -or
    [regex]::IsMatch($mainLua, 'f7_trace\s*\([^\)]*\{', [Text.RegularExpressions.RegexOptions]::Singleline)) {
    throw 'Normal-mode Lua still performs eager debug/F7-trace work.'
}
if (([regex]::Matches($mainLua, 'get_player_location\(\)')).Count -ne 2) {
    throw 'Player full-chain location sampling must have one definition and exactly one 250 ms call site.'
}
foreach ($removedFastSample in @('local function queue_motion_update()','queue_motion_update()')) {
    if ($mainLua -match [regex]::Escape($removedFastSample)) { throw "Duplicated 50 ms player sampling path remains: $removedFastSample" }
}
$worldMotionStart = $mainLua.IndexOf('local function queue_world_motion_update(')
$worldMotionEnd = $mainLua.IndexOf('ensure_motion_loop_started = function()', $worldMotionStart)
$worldMotionBlock = $mainLua.Substring($worldMotionStart, $worldMotionEnd-$worldMotionStart)
if ($worldMotionBlock -match [regex]::Escape('get_player_location()') -or
    $worldMotionBlock -notmatch [regex]::Escape('latest_motion_x')) {
    throw 'Large-map transform loop does not reuse the shared 250 ms scalar player sample.'
}
foreach ($marker in @('WorldEpoch','SampleTimestampMs','ScalarMotionPredictor','MaximumPredictionMs = 250.0','StaleFreezeMs = 500.0','OVERLAY_MOTION_PREDICTION','player_location_hz')) {
    if (($motionReader + $motionParser + $radar + $overlayPerformance + $luaDiagnostics) -notmatch [regex]::Escape($marker)) {
        throw "Scalar motion/prediction contract is missing: $marker"
    }
}
foreach ($removedMarkerCacheSymbol in @(
    'EVENTRADAR_VECTOR_MARKERS',
    'EnsureCache',
    'DrawImageUnscaled',
    'CacheBlits',
    'CacheRebuilds',
    'CacheRebuildMilliseconds',
    'VectorDraws',
    'RecordMarkerCacheSnapshot')) {
    if (($bossRenderer + $moleRenderer + $radar + $overlayPerformance) -match [regex]::Escape($removedMarkerCacheSymbol)) {
        throw "Removed Boss/Mole marker-cache symbol remains: $removedMarkerCacheSymbol"
    }
}
foreach ($marker in @(
    'ActiveTimerIntervalMs = 33',
    'DiagnosticNoMotionTimerIntervalMs = 250',
    'DiagnosticModeNoPaint = 1',
    'DiagnosticModeNoMotion = 2',
    'WorldMapTimerIntervalMs = 8',
    'RadarIdleTimerIntervalMs = 75',
    'DisabledTimerIntervalMs = 125',
    'BackgroundTimerIntervalMs = 500',
    'MaintenanceIntervalMs = 1000',
    'GeometryCheckIntervalMs = 1000',
    'GameLifetimeCheckIntervalMs = 1000',
    'WindowVisibilityCheckIntervalMs = 250',
    'MotionStaleTimeoutMs = 2500',
    'MotionBridgeFallbackIntervalMs = 250',
    'MotionBridgePollingFallbackIntervalMs = 50',
    'ReferenceStatusStripGap = -6',
    'ReferenceStatusStripHeight = 46',
    'new WorldEncounterCatalog()',
    'frame.ProtocolVersion',
    'SwpNoCopyBits',
    'MAP_SURFACE_PREPARED',
    'Invalidate();',
    'Update();')) {
    if ($radar -notmatch [regex]::Escape($marker)) {
        throw "Overlay marker is missing: $marker"
    }
}
foreach ($marker in @(
    'FileSystemWatcher',
    'MotionBridgeDirtyGate',
    ': IDisposable',
    'UpdateOrRearm',
    'Interlocked.Exchange(ref _dirtyMask, 0)',
    'watcher.EnableRaisingEvents = false')) {
    if ($motionReader -notmatch [regex]::Escape($marker)) {
        throw "Event-driven Motion Bridge marker is missing: $marker"
    }
}
foreach ($marker in @(
    'ShouldForceMotionBridgeScan',
    'ApplyMotionBridgePollingFallback',
    'ProcessMotionBridgeWake',
    'SetChangeNotificationsEnabled(!worldMode)',
    'ChangeNotificationsAvailable',
    'HasPendingChanges')) {
    if (($radar + $motionReader) -notmatch [regex]::Escape($marker)) {
        throw "Compact bridge scheduling marker is missing: $marker"
    }
}
$watcherCallbackStart = $motionReader.IndexOf('private void OnBridgeChanged(')
$watcherCallbackEnd = $motionReader.IndexOf('private static bool SameContent(', $watcherCallbackStart)
if ($watcherCallbackStart -lt 0 -or $watcherCallbackEnd -le $watcherCallbackStart) {
    throw 'Motion Bridge watcher callback boundary is malformed.'
}
$watcherCallbackBlock = $motionReader.Substring(
    $watcherCallbackStart,
    $watcherCallbackEnd - $watcherCallbackStart)
foreach ($forbiddenWatcherWork in @(
    'SharedBridgeFile.ReadInto',
    'MotionRecordParser.TryParse',
    'Task.Run',
    'new Thread')) {
    if ($watcherCallbackBlock -match [regex]::Escape($forbiddenWatcherWork)) {
        throw "Watcher callback performs forbidden work: $forbiddenWatcherWork"
    }
}
if ($motionReader -match [regex]::Escape('System.Threading.Tasks')) {
    throw 'Motion Bridge added a background parser task.'
}
foreach ($marker in @(
    'no_paint_key = "F5"',
    'no_motion_key = "F6"',
    'request_active_diagnostic_mode("no_paint", "F5")',
    'request_active_diagnostic_mode("no_motion", "F6")',
    'request_active_diagnostic_mode("normal", "F7")')) {
    if ($mainLua -notmatch [regex]::Escape($marker) -and
        $defaultConfigText -notmatch [regex]::Escape($marker)) {
        throw "Inline A/B hotkey marker is missing: $marker"
    }
}
$debugHotkeyGuardStart = $mainLua.IndexOf(
    'if config.debug_logging == true then',
    $mainLua.IndexOf('local function request_active_diagnostic_mode('))
$normalHotkeyStart = $mainLua.IndexOf(
    'RegisterKeyBind(Key[start_key]',
    $debugHotkeyGuardStart)
if ($debugHotkeyGuardStart -lt 0 -or
    $normalHotkeyStart -le $debugHotkeyGuardStart) {
    throw 'Debug hotkey guard boundary is malformed.'
}
$debugHotkeyBlock = $mainLua.Substring(
    $debugHotkeyGuardStart,
    $normalHotkeyStart - $debugHotkeyGuardStart)
foreach ($debugHotkeyMarker in @(
    'RegisterKeyBind(Key[no_paint_key]',
    'RegisterKeyBind(Key[no_motion_key]')) {
    if ($debugHotkeyBlock -notmatch
        [regex]::Escape($debugHotkeyMarker)) {
        throw "F5/F6 are not confined to the debug_logging guard: $debugHotkeyMarker"
    }
}
foreach ($marker in @(
    'if (_diagnosticMode == DiagnosticModeNoPaint)',
    '_diagnosticMode != DiagnosticModeNoMotion',
    '_motionPredictor.Clear()',
    'CloneMotionFrame(motion)',
    'frame.DiagnosticMode = diagnosticMode')) {
    if (($radar + $motionParser) -notmatch [regex]::Escape($marker)) {
        throw "Inline A/B isolation marker is missing: $marker"
    }
}
if ($radar -match [regex]::Escape('ActiveTimerIntervalMs = 24')) {
    throw 'Obsolete 24 ms Overlay active timer remains.'
}
if ($radar -match [regex]::Escape('WorldIdleTimerIntervalMs')) {
    throw 'Legacy 50 ms world-map presentation timer remains.'
}
foreach ($requiredTimerMarker in @('_worldMapTimerResolutionAcquired','UpdateWorldMapTimerResolution(','ReleaseWorldMapTimerResolution(','RecordTimerResolutionChange','timerResolutionBalance=')) {
    if (($radar + $overlayPerformance) -notmatch [regex]::Escape($requiredTimerMarker)) {
        throw "Mode-scoped world-map timer-resolution marker is missing: $requiredTimerMarker"
    }
}
foreach ($marker in @('record_world_map_producer_tick','world_map_producer_hz','worldMapPresentationHz=','modeTransitions=')) {
    if (($mainLua + $luaDiagnostics + $radar + $overlayPerformance) -notmatch [regex]::Escape($marker)) {
        throw "World-map high-rate diagnostic marker is missing: $marker"
    }
}
$saveStateText = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
if ($saveStateText -notmatch [regex]::Escape('TimeSpan.FromMilliseconds(2000)')) {
    throw 'Save-state metadata sampling is not bounded to two seconds.'
}
foreach ($marker in @('SaveChangeDebounce','TimeSpan.FromSeconds(4)','SetRuntimeEnabled','ThreadPriority.BelowNormal','SAVE_REFRESH_PERF')) {
    if ($saveStateText -notmatch [regex]::Escape($marker)) { throw "Optimized save-state marker is missing: $marker" }
}
$nativeMethodsText = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Platform\NativeMethods.cs') -Raw
foreach ($marker in @('ThreadModeBackgroundBegin','ThreadModeBackgroundEnd','SetThreadPriority(','GetCurrentThread()')) {
    if (($saveStateText + $nativeMethodsText) -notmatch [regex]::Escape($marker)) { throw "Background save-worker scheduling marker is missing: $marker" }
}
$saveReaderText = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveSnapshotReader.cs') -Raw
foreach ($marker in @('DatabaseCacheHits','TryGetCached','TryGetCompatibleCached','BuildCacheKey','includeTreasure','SqliteOpenReadWrite','ApplyKey(database, key)','QueryOpenedTreasureBits(database)','QueryBossRespawns(database, encounterTargetIds)','BuildActorFilterSql(encounterTargetIds)','ActorFilterSignature')) {
    if ($saveReaderText -notmatch [regex]::Escape($marker)) { throw "Optimized snapshot-reader marker is missing: $marker" }
}
if ($saveReaderText -notmatch [regex]::Escape('if (DebugSettings.Enabled)') -or
    $saveReaderText -notmatch [regex]::Escape('QueryEncounterTaskTable(database)') -or
    $saveReaderText -notmatch [regex]::Escape('WHERE OPENED_BIT_FIELD <> 0;')) {
    throw 'Normal save snapshot still performs avoidable diagnostic or zero-row work.'
}
foreach ($marker in @('CacheMissIncludesTreasure = true','bool readIncludesTreasure =','TreasureRequested','TreasureQueries','coalescingWindowMs=')) {
    if (($saveReaderText + $saveStateText) -notmatch [regex]::Escape($marker)) { throw "Cold-read coalescing marker is missing: $marker" }
}
foreach ($marker in @('SnapshotReadInterval =','TimeSpan.FromSeconds(45)','_nextSnapshotReadUtc','now < _nextSnapshotReadUtc','includeTreasure = true')) {
    if ($saveStateText -notmatch [regex]::Escape($marker)) { throw "Bounded save-read scheduler marker is missing: $marker" }
}
if (-not $saveReaderText.Contains('StoreCached(') -or -not $saveReaderText.Contains('readIncludesTreasure,')) {
    throw 'Cache miss does not store the warmed treasure-rich shape.'
}
if (([regex]::Matches($saveReaderText, 'PRAGMA key')).Count -ne 1) {
    throw 'SQLCipher key must be applied exactly once per database connection.'
}
foreach ($catalogRelative in @('src\overlay\Data\WorldTreasureCatalog.cs','src\overlay\Data\WorldBossCatalog.cs','src\overlay\Data\WorldEncounterCatalog.cs')) {
    $catalogText = Get-Content -LiteralPath (Join-Path $root $catalogRelative) -Raw
    if ($catalogText -notmatch [regex]::Escape('if (_hasLoaded)')) {
        throw "Immutable catalog one-load gate is missing: $catalogRelative"
    }
}
if ($overlayPerformance -notmatch [regex]::Escape('OVERLAY_WORK_PERF')) {
    throw 'Debug Overlay work-duty metrics are missing.'
}
foreach ($marker in @(
    'RecordTreasureDraw',
    'RecordBossDraw',
    'RecordEncounterDraw',
    'treasureDrawCalls=',
    'bossDrawCalls=',
    'windowVisibilitySampleHz=')) {
    if ($overlayPerformance -notmatch [regex]::Escape($marker)) {
        throw "Secondary Overlay diagnostic marker is missing: $marker"
    }
}
foreach ($marker in @(
    '_nextWindowVisibilityCheckUtc = DateTime.MinValue;',
    'RecordWindowVisibilitySample();',
    'if (DebugSettings.HighResolutionTimerEnabled)')) {
    if ($radar -notmatch [regex]::Escape($marker)) {
        throw "Secondary Overlay scheduling marker is missing: $marker"
    }
}
$debugSettingsText = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Configuration\DebugSettings.cs') -Raw
$defaultConfigText = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\config.lua') -Raw
if (Test-Path -LiteralPath (Join-Path $root 'src\ue4ss\config.default.lua')) {
    throw 'A second default configuration file remains; config.lua must be authoritative.'
}
if ($debugSettingsText -notmatch [regex]::Escape('HighResolutionTimerEnabled')) {
    throw 'High-resolution timer startup setting parser is missing.'
}
if ($defaultConfigText -notmatch [regex]::Escape('high_resolution_timer = false')) {
    throw 'High-resolution timer must default off.'
}
if ($defaultConfigText -notmatch '(?m)^\s*show_assaults\s*=\s*true\s*,' -or
    $mainLua -notmatch [regex]::Escape('local show_assaults = config.show_assaults ~= false') -or
    $debugSettingsText -notmatch [regex]::Escape('LoadBooleanAtStartup("show_assaults", true)') -or
    ($defaultConfigText + $mainLua + $debugSettingsText) -match 'assault_performance_ab_isolation') {
    throw 'Production Assault enablement is not effective in both Lua and Overlay.'
}
foreach ($obsoleteEncounterPath in @('_worldAssaults','_worldBosses','_assaultAvailability','_bossAvailability','ConfigureAssaultTargets')) {
    if ($radar -match [regex]::Escape($obsoleteEncounterPath)) {
        throw "Separate Boss/Assault runtime path remains: $obsoleteEncounterPath"
    }
}
foreach ($marker in @('_worldEncounters.Load();','ConfigureEncounterTargets(','_encounterAvailability.Refresh(','WORLD_ENCOUNTER_LIFECYCLE')) {
    if ($radar -notmatch [regex]::Escape($marker)) {
        throw "Unified encounter runtime marker is missing: $marker"
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

$moleCatalog = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Data\WorldMoleCatalog.cs') -Raw
foreach ($marker in @('ExpectedCount = 83','invalid or duplicate record','GetMap','contiguous mask bits')) {
    if ($moleCatalog -notmatch [regex]::Escape($marker)) { throw "Mole/Fly catalog invariant is missing: $marker" }
}
$moleCompletion = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\mole_completion.lua') -Raw
foreach ($marker in @('EXPECTED_FLY_RECORD_COUNT = 33','EXPECTED_CATALOG_COUNT = 83','unsafe runtime completion queries are disabled','function M.set_context_available','function M.visible_mask')) {
    if ($moleCompletion -notmatch [regex]::Escape($marker)) { throw "Mole/Fly safety-mode invariant is missing: $marker" }
}
$moleRewardVisibility = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Data\MoleRewardVisibilityIndex.cs') -Raw
foreach ($marker in @('FirstMiniGameId = 11001','LastMiniGameId = 11034','IsRawOpened','mole.RewardSaveId','mole.MaskBit')) {
    if ($moleRewardVisibility -notmatch [regex]::Escape($marker)) { throw "Mole reward visibility invariant is missing: $marker" }
}
$moleProvider = Get-Content -LiteralPath (Join-Path $root 'src\installer\Core\Providers\MoleDataProvider.cs') -Raw
foreach ($marker in @('DT_MiniGame_G5_','BindRewardTreasureIds','treasures.lua','reward_save_id','rewards.Count != 83','AdditionalMiniGameLuaGenerator.Append','gameId == 13009 && saveId == 13008','rewards.Add(13010, sharedWaveReward)')) {
    if ($moleProvider -notmatch [regex]::Escape($marker)) { throw "Install-generated Mole reward mapping invariant is missing: $marker" }
}
$moleCatalogSource = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Data\WorldMoleCatalog.cs') -Raw
foreach ($marker in @('reward_save_id','RewardSaveId = rewardSaveId')) {
    if ($moleCatalogSource -notmatch [regex]::Escape($marker)) { throw "Runtime Mole reward mapping parser is missing: $marker" }
}
foreach ($forbiddenMarker in @('FindFirstOf','StaticFindObject','CIsClearMiniGameInStandAlone','clear_function(')) {
    if ($moleCompletion -match [regex]::Escape($forbiddenMarker)) { throw "Mole/Fly safety mode contains a forbidden runtime query marker: $forbiddenMarker" }
}
$diagnosticsConfigureIndex = $mainLua.IndexOf('diagnostics.configure({')
$moleInitializeIndex = $mainLua.IndexOf('completion_module.initialize')
if ($diagnosticsConfigureIndex -lt 0 -or $moleInitializeIndex -le $diagnosticsConfigureIndex) {
    throw 'Mole completion must initialize after diagnostics configuration so debug performance recording is available.'
}
$moleRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\MoleMarkerRenderer.cs') -Raw
foreach ($marker in @('winged upward-arrow','BuildWing','BuildArrow','BuildHammer','BuildWave','GraphicsPath')) {
    if ($moleRenderer -notmatch [regex]::Escape($marker)) { throw "Winged Fly marker invariant is missing: $marker" }
}
$environmentSampler = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\world_environment.lua') -Raw
foreach ($marker in @('GAME_SECONDS_PER_REAL_SECOND = 60','DEFAULT_STABLE_CONTEXT_TICKS = 8','DGameSingleton','TimeOfDay','function M.arm()','function M.cancel()','function M.context_lost()','function M.observe_context(available)','function M.capture_in_game_thread(token)','attempted_reads=1','retry_reads=0','baseline_wall_seconds = os.time()','os.time() - baseline_wall_seconds','return singleton:IsValid()')) {
    if ($environmentSampler -notmatch [regex]::Escape($marker)) { throw "World-time sampler invariant is missing: $marker" }
}
if ($environmentSampler -match 'LoopAsync\s*\(' -or $environmentSampler -match 'FindAllOf\s*\(') {
    throw 'World-time sampler must not add a scheduler or enumerate UObjects.'
}
if (([regex]::Matches($environmentSampler, [regex]::Escape('FindFirstOf, "DGameSingleton"'))).Count -ne 1 -or
    ([regex]::Matches($environmentSampler, [regex]::Escape('read_field(singleton, "TimeOfDay")'))).Count -ne 1) {
    throw 'Isolated clock must have exactly one DGameSingleton lookup and one TimeOfDay read site.'
}
foreach ($forbidden in @('recalibrat','next_retry','retry_at','FindAllOf','StaticFindObject')) {
    if ($environmentSampler -match [regex]::Escape($forbidden)) { throw "Clock retry/scan path remains: $forbidden" }
}
$updateStart = $mainLua.IndexOf('local function update_radar_state(queue_delay_ms)')
$updateEnd = $mainLua.IndexOf('local ensure_world_map_loop_started', $updateStart)
$updateBlock = $mainLua.Substring($updateStart, $updateEnd-$updateStart)
foreach ($forbidden in @('TimeOfDay','DGameSingleton','capture_in_game_thread','world_environment.')) {
    if ($updateBlock -match [regex]::Escape($forbidden)) { throw "update_radar_state touches isolated clock provider: $forbidden" }
}
if ($clockEnabled) {
    $f7Start = $mainLua.IndexOf('RegisterKeyBind(Key[start_key]')
    $f7End = $mainLua.IndexOf('RegisterKeyBind(Key[stop_key]', $f7Start)
    $f7Block = $mainLua.Substring($f7Start, $f7End-$f7Start)
    if ($f7Block -match [regex]::Escape('world_environment.arm()')) { throw 'F7 arms the isolated clock before activation stability is confirmed.' }
    $activationStart = $mainLua.IndexOf('local function activation_game_thread_callback()')
    $activationEnd = $mainLua.IndexOf('local function queue_activation_probe()', $activationStart)
    $activationBlock = $mainLua.Substring($activationStart, $activationEnd-$activationStart)
    if ($activationBlock -notmatch [regex]::Escape('world_environment.arm()')) { throw 'Stable activation does not arm the isolated clock.' }
    foreach ($forbidden in @('TimeOfDay','DGameSingleton','FindFirstOf','ExecuteInGameThread','capture_in_game_thread')) {
        if ($f7Block -match [regex]::Escape($forbidden)) { throw "F7 directly performs clock/UObject work: $forbidden" }
    }
}
foreach ($marker in @('local ACTIVATION_STABLE_SAMPLE_COUNT = 4','local function activation_game_thread_callback()','local function queue_activation_probe()','ExecuteInGameThread(activation_game_thread_callback)','ACTIVATION_STABILITY_SAMPLE','ACTIVATION_STABLE')) {
    if ($mainLua -notmatch [regex]::Escape($marker)) { throw "Activation stability marker is missing: $marker" }
}
foreach ($marker in @('if (!motion.Enabled) return;','if (motion.ShowWorldStatus)','ClientSize.Width - _overlaySize / 2f','return "disabled";')) {
    if ($radar -notmatch [regex]::Escape($marker)) { throw "Master-gated world-status Overlay marker is missing: $marker" }
}
if ($radar -match [regex]::Escape('StatusOnlyTimerIntervalMs') -or
    $radar -match [regex]::Escape('String.Equals(mode, "status"')) {
    throw 'Obsolete active Overlay status-only timer or paint path remains.'
}
$nativeMethods = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Platform\NativeMethods.cs') -Raw
foreach ($removedRegionMarker in @('CreateRectRgn','CreateEllipticRgn','CombineRgn','SetWindowRgn','DeleteObject','RgnOr = 2','WindowRegionGeometry','ApplyWindowRegionIfNeeded')) {
    if (($nativeMethods + $radar) -match [regex]::Escape($removedRegionMarker)) { throw "Post-dev15 composition-region symbol remains: $removedRegionMarker" }
}
if ($environmentSampler -match 'os\.clock\s*\(' -or $environmentSampler -notmatch [regex]::Escape('math.max(0, os.time() - baseline_wall_seconds)')) {
    throw 'World-time extrapolation must use nonnegative os.time wall elapsed and never process CPU time.'
}
foreach ($retiredWeatherRead in @('DsEnvironmentManager','CurrentWeatherState','CurrentWeatherBTState')) {
    if ($environmentSampler -match [regex]::Escape($retiredWeatherRead)) {
        throw "Unproven weather sampling was reintroduced: $retiredWeatherRead"
    }
}
$worldStatusRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\WorldStatusRenderer.cs') -Raw
foreach ($marker in @('FormatClock','FormatPhase','MORNING','AFTERNOON','EVENING','NIGHT')) {
    if ($worldStatusRenderer -notmatch [regex]::Escape($marker)) { throw "World status renderer invariant is missing: $marker" }
}
foreach ($marker in @('StatusGroupReferenceWidth = 138f','StatusStripGapReference = -6f','PhaseFontReferencePixels = 13f','CalculateGroupBounds','clientHeight - groupHeight - padding')) {
    if ($worldStatusRenderer -notmatch [regex]::Escape($marker)) { throw "World status layout marker is missing: $marker" }
}
if ($worldStatusRenderer -match 'WeatherState|WeatherBtState|DrawWeatherGlyph') {
    throw 'World status renderer still exposes unproven weather state.'
}

$treasureCatalog = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Data\WorldTreasureCatalog.cs') -Raw
foreach ($marker in @('(true|false|nil)','match.Groups[4].Value')) {
    if ($treasureCatalog -notmatch [regex]::Escape($marker)) {
        throw "Shared Lua catalog parser is missing its Boolean-literal contract: $marker"
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
    'GeneratedOwnerPointerConfig.LoadForGame',
    'generated-fingerprint-bound',
    'known-current',
    'known-legacy',
    'delayed-pattern-fallback',
    'TimeSpan.FromSeconds(30)',
    'DateTime.UtcNow.Add(PatternScanDelay)',
    '_patternScanAttempted = true',
    'DetectOwnerPointerRva')) {
    if ($keyReader -notmatch [regex]::Escape($marker)) {
        throw "Save-key startup optimization marker is missing: $marker"
    }
}

$saveState = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
foreach ($marker in @(
    'public bool HasLoadedSaveState',
    'IsStartupInitializationPending',
    'Save-state initialization pending',
    'Save-state snapshot ready after startup wait')) {
    if ($saveState -notmatch [regex]::Escape($marker)) {
        throw "Save-state startup safety marker is missing: $marker"
    }
}
foreach ($marker in @('bool initialSnapshot = !_hasLoadedSaveState','&& !_initialDebounceBypassConsumed;','_initialDebounceBypassConsumed = true;','if (!initialSnapshot','fingerprint before/after its copy')) {
    if ($saveState -notmatch [regex]::Escape($marker)) {
        throw "Initial snapshot debounce safety marker is missing: $marker"
    }
}

$generatedOwnerConfig = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\GeneratedOwnerPointerConfig.cs') -Raw
$ownerResolver = Get-Content -LiteralPath (Join-Path $root 'src\installer\Core\Generation\OwnerPointerRvaResolver.cs') -Raw
foreach ($marker in @('ComputeGameFingerprint','owner_pointer_rva','install-time-exact-executable-pattern','matches.Count == 0','matches.Count != 1','GenerateBoundConfig')) {
    if (($generatedOwnerConfig + $ownerResolver) -notmatch [regex]::Escape($marker)) {
        throw "Generated owner-pointer safety marker is missing: $marker"
    }
}
if (($generatedOwnerConfig + $ownerResolver) -match '(?i)sqlcipher[_ -]?key\s*=') {
    throw 'Generated owner-pointer data must not persist the SQLCipher key.'
}

$visibilityIndex = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Data\WorldTreasureVisibilityIndex.cs') -Raw
foreach ($marker in @('catalogVersion == _catalogVersion','saveVersion == _saveVersion','if (hasSave)','IList<WorldTreasure> points = catalog.Points','WorldTreasure treasure = points[index]','saveState.IsOpened(','_catalogVersion = catalogVersion','_saveVersion = saveVersion')) {
    if ($visibilityIndex -notmatch [regex]::Escape($marker)) {
        throw "Startup fail-closed treasure visibility marker is missing: $marker"
    }
}
if ($visibilityIndex -match [regex]::Escape('_hasSave')) {
    throw 'Post-dev15 treasure visibility cache-contract state remains: _hasSave'
}
if ($visibilityIndex -match [regex]::Escape('Startup is display-fail-open') -or
    $visibilityIndex -match [regex]::Escape('publish the complete catalog')) {
    throw 'Obsolete startup fail-open treasure-index policy remains.'
}

$defaultOverrides = Get-Content -LiteralPath (Join-Path $root 'resources\defaults\treasure_overrides.txt') -Raw
if ($defaultOverrides -match '(?m)^\s*ignore\s+10220122\s*(?:#.*)?$') {
    throw 'Obsolete default treasure override ignore 10220122 is still active.'
}
if ($defaultOverrides -match '(?im)^\s*ignore\s+11003\s*(?:#.*)?$' -or
    $defaultOverrides -match [regex]::Escape('11003 overlaps 14016') -or
    $defaultOverrides -match [regex]::Escape('alias 11003 14016')) {
    throw 'Obsolete default 11003/14016 duplicate override guidance remains.'
}
$installScript = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.ps1') -Raw
if ($mainLua -match [regex]::Escape('Protocol-v3 Motion Bridge') -or
    $installScript -match [regex]::Escape('single-motion-bridge-v3')) {
    throw 'An active runtime or installer string still identifies the current bridge as protocol v3.'
}
if ($installScript -notmatch [regex]::Escape('REMOVED obsolete treasure override rules for valid records 10220122/11003')) {
    throw 'Valid-record override migration log is missing.'
}
foreach ($marker in @(
    "`$_ -notmatch '^\s*ignore\s+10220122\s*(?:#.*)?$'",
    "`$_ -notmatch '^\s*#\s*Known abandoned or inaccessible chest record\.\s*$'",
    "`$_ -notmatch '^\s*ignore\s+11003\s*(?:#.*)?$'",
    "`$_ -notmatch '^\s*#\s*Known abandoned duplicate: 11003 overlaps 14016 at the same chest location\.\s*$'",
    "`$_ -notmatch '^\s*#\s*Keep the authoritative 14016 record and suppress the offset duplicate\.\s*$'")) {
    if ($installScript -notmatch [regex]::Escape($marker)) {
        throw "Accepted removal-only installer migration marker is missing: $marker"
    }
}
foreach ($marker in @('WorldMapId = 100','_worldTreasureIndex.GetMap(WorldMapId)','_worldTreasureIndex.GetMap(map.mapId)')) {
    if ($radar -notmatch [regex]::Escape($marker)) {
        throw "Restored dev15 treasure-map contract is missing: $marker"
    }
}
foreach ($postDev15TreasureSymbol in @('CompactRadarMapId','RadarMapSessionState','_radarMapSession','RADAR_MAP_EVIDENCE','_mapId != mapId','_hasSave')) {
    if ($radar -match [regex]::Escape($postDev15TreasureSymbol)) { throw "Post-dev15 treasure/cache symbol remains: $postDev15TreasureSymbol" }
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
foreach ($marker in @('F8 disables all active mod work for FPS comparison','Overlay startup fixes one 49-record Boss/Assault encounter catalog')) {
    if ($install -notmatch [regex]::Escape($marker)) {
        throw "Installer whole-mod completion guidance is missing: $marker"
    }
}
$clockInstallerMarker = if ($clockEnabled) {
    'F7 only enables configured marker, save-state, and isolated world-clock work'
} else {
    'World status remains disabled for the Assault-only A/B'
}
if ($install -notmatch [regex]::Escape($clockInstallerMarker)) {
    throw "Installer clock-stage guidance is missing: $clockInstallerMarker"
}
foreach ($obsoleteInstallerClaim in @('World time is automatic; F7 enables radar markers','F8 disables radar markers')) {
    if ($install -match [regex]::Escape($obsoleteInstallerClaim)) {
        throw "Obsolete installer F7/F8 guidance remains: $obsoleteInstallerClaim"
    }
}
foreach ($obsolete in @(
    'src\overlay\Bridge\StaticStateBridgeReader.cs',
    'src\overlay\Models\RadarState.cs',
    'src\overlay\SaveData\BossAvailabilityTracker.cs',
    'src\overlay\SaveData\AssaultAvailabilityTracker.cs',
    'scripts\boss_tracker.lua')) {
    if ($install -notmatch [regex]::Escape($obsolete)) {
        throw "Installer upgrade cleanup entry is missing: $obsolete"
    }
}

$collector = Get-Content -LiteralPath (Join-Path $root 'src\tools\Collect-Diagnostics.ps1') -Raw
if ($collector -match "'radar_state_a\.json'|'radar_state_b\.json'|'radar_state\.json'") {
    throw 'Diagnostics still treats legacy Static Bridge files as active captures.'
}
foreach ($marker in @(
    'radar_motion_a.dat',
    'radar_motion_b.dat',
    'WorldBossCatalog.cs',
    'WorldEncounterCatalog.cs',
    'world_environment.lua',
    'WorldStatusRenderer.cs')) {
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

$worldStatusGate = if ($clockEnabled) { 'isolated-one-shot-wall-time-no-retry' } else { 'disconnected-zero-read' }
Write-Host "Source verification passed for DragonSwordWorldRadar $version; overlay=41; installer=19; lua=9; motionProtocol=6; fields=38; inlineAB=debug-only-F5-no-paint_F6-no-motion_F7-normal_F8-off; playerSamples=250ms-scalar-only; compactPrediction=33ms-event-driven; bridgeFallback=250ms; visibleWorldMap=8ms; worldCandidates=epoch-bound-cap2; loopOwnership=tokenized; installBoundSaveRva=true; encounters=49-startup-fixed-unified; miniGames=83; worldStatus=$worldStatusGate."
