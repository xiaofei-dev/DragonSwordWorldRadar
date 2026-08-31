$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$version = [string]((Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json).version)
$clockEnabled = $version -like '0.4.0-dev39-*' -or $version -like '0.4.0-dev48-*' -or $version -like '0.4.0-dev49-*' -or $version -like '0.4.0-dev50-*' -or $version -like '0.4.0-dev51-*' -or $version -like '0.4.0-dev52-*' -or $version -like '0.4.0-dev53-*' -or $version -like '0.4.0-dev54-*' -or $version -like '0.4.0-dev55-*' -or $version -like '0.4.0-dev56-*' -or $version -like '0.4.0-dev57-*' -or $version -like '0.4.0-dev58-*' -or $version -like '0.4.0-dev59-*' -or $version -like '0.4.0-dev60-*' -or $version -like '0.4.0-dev61-*' -or $version -like '0.4.0-dev62-*' -or $version -like '0.4.0-dev63-*' -or $version -like '0.4.0-dev64-*' -or $version -like '0.4.0-dev65-*' -or $version -like '0.4.0-dev66-*' -or $version -like '0.4.0-dev67-*' -or $version -like '0.4.0-dev68-*' -or $version -like '0.4.0-dev69-*' -or $version -like '0.4.0-dev70-*' -or $version -like '0.4.0-dev71-*' -or $version -like '0.4.0-dev72-*' -or $version -like '0.4.0-dev73-*' -or $version -like '0.4.0-dev74-*'
$mole = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\mole_completion.lua') -Raw
$environment = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\world_environment.lua') -Raw
$worldMap = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\world_map.lua') -Raw
$main = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\main.lua') -Raw
$diagnostics = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\diagnostics.lua') -Raw
$radar = Get-Content -LiteralPath (Join-Path $root 'src\overlay\UI\RadarForm.cs') -Raw
$motionReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Bridge\MotionBridgeReader.cs') -Raw
$renderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\WorldStatusRenderer.cs') -Raw
$bossRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\BossMarkerRenderer.cs') -Raw
$moleRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\MoleMarkerRenderer.cs') -Raw
$overlayPerformance = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Diagnostics\OverlayPerformanceTracker.cs') -Raw
$debugSettings = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Configuration\DebugSettings.cs') -Raw
$defaultConfig = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\config.lua') -Raw
$installer = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.ps1') -Raw
$keyReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveDatabaseKeyReader.cs') -Raw
$snapshotReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveSnapshotReader.cs') -Raw
$saveState = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
$treasureOverrides = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureOverrides.cs') -Raw
$encounterAvailability = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\EncounterAvailabilityTracker.cs') -Raw
$nativeMethods = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Platform\NativeMethods.cs') -Raw
$ownerConfig = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\GeneratedOwnerPointerConfig.cs') -Raw
$ownerResolver = Get-Content -LiteralPath (Join-Path $root 'src\installer\Core\Generation\OwnerPointerRvaResolver.cs') -Raw
$defaultOverrides = Get-Content -LiteralPath (Join-Path $root 'resources\defaults\treasure_overrides.txt') -Raw
$baselineRoot = Join-Path $root 'dist\DragonSwordWorldRadar-0.4.0-dev15-status-autostart-layout1\DragonSwordWorldRadar'
$script:assertionCount = 0
function Assert-True([bool]$Condition, [string]$Message) {
    $script:assertionCount++
    if (-not $Condition) { throw $Message }
}
function Invoke-ControlWatchdogModel([int[]]$SerialSamples, [int]$Threshold) {
    $observed = $SerialSamples[0]
    $stale = 0
    $triggeredAt = -1
    for ($index = 1; $index -lt $SerialSamples.Count; $index++) {
        if ($SerialSamples[$index] -ne $observed) {
            $observed = $SerialSamples[$index]
            $stale = 0
            continue
        }
        $stale = [Math]::Min($Threshold, $stale + 1)
        if ($stale -ge $Threshold) {
            $triggeredAt = $index
            break
        }
    }
    return $triggeredAt
}
Assert-True ((Invoke-ControlWatchdogModel @(10,10,10,10,10,10) 5) -eq 5) 'Control watchdog does not require exactly five consecutive stale observations.'
Assert-True ((Invoke-ControlWatchdogModel @(10,10,11,11,11,12,12,12) 5) -eq -1) 'Healthy control progress can falsely trigger automatic recovery.'
Assert-True ($defaultConfig.Contains('debug_logging = true')) 'Debug logging does not default on.'
Assert-True ($main.Contains('if not f7_trace_active or perf_diagnostics == nil then return end')) 'F7 trace does not fail fast when debug diagnostics are disabled.'
Assert-True ($main.Contains('if perf_diagnostics == nil then') -and $main.Contains('f7_trace_active = false')) 'F7 still activates its crash-trace budget in normal mode.'
Assert-True (-not [regex]::IsMatch($main, 'f7_trace\s*\([^\)]*\{', [Text.RegularExpressions.RegexOptions]::Singleline)) 'F7 trace call eagerly allocates a field table before its disabled guard.'
Assert-True (-not [regex]::IsMatch($main, '(?<!perf_)diagnostics\.debug\(')) 'Normal-use diagnostics object still receives eager debug-only work.'
Assert-True ($defaultConfig.Contains('diagnostic_verbose = false')) 'In-map verbose labels are not disabled by default.'
Assert-True ($debugSettings.Contains('public static bool VerboseLabelsEnabled')) 'File diagnostics and visual labels do not expose separate settings.'
Assert-True ($radar.Contains('DebugSettings.VerboseLabelsEnabled')) 'Radar visual labels are not gated by diagnostic_verbose.'
Assert-True (-not $radar.Contains('bool showDebugCoordinates = _debugEnabled;')) 'Radar visual labels are still coupled to file debug logging.'
foreach ($marker in @('MINIMAP_SCALE_PERF','cache_validation_ms','find_called','find_ms','resolved_validation_ms','layer_map_access_ms','map_overlay_access_ms','scale_access_ms')) { Assert-True ($main.Contains($marker)) "Missing minimap scale phase diagnostic: $marker" }
foreach ($marker in @('EXPECTED_FLY_RECORD_COUNT = 33','EXPECTED_CATALOG_COUNT = 83','unsafe runtime completion queries are disabled','function M.set_context_available','function M.visible_mask','return false')) { Assert-True ($mole.Contains($marker)) "Missing Mole safety-mode contract: $marker" }
foreach ($forbidden in @('FindFirstOf','StaticFindObject','CIsClearMiniGameInStandAlone','clear_function(')) { Assert-True (-not $mole.Contains($forbidden)) "Unsafe Mole runtime query remains: $forbidden" }
foreach ($marker in @('GAME_SECONDS_PER_REAL_SECOND = 60','DEFAULT_STABLE_CONTEXT_TICKS = 8','function M.arm()','function M.cancel()','function M.context_lost()','function M.observe_context(available)','function M.capture_ready()','function M.mark_capture_queued()','function M.capture_in_game_thread(token)','function M.advance()','attempted_reads=1','retry_reads=0','baseline_wall_seconds','os.time() - baseline_wall_seconds','%SECONDS_PER_DAY')) { Assert-True ($environment.Contains($marker)) "Missing isolated one-shot world-time contract: $marker" }
Assert-True (-not $environment.Contains('os.clock(')) 'Portable Lua os.clock must not drive production world time.'
foreach ($api in @('record_mole_query','record_mole_state_change','record_mole_scheduler')) { Assert-True ($diagnostics.Contains("function Diagnostics.$api")) "Missing debug API: $api" }
Assert-True ($main.Contains('math.floor(previous.world_time_seconds / 60)')) 'Exact-seconds-only bridge suppression is missing.'
Assert-True (-not $main.Contains('Game time is sampled once per second')) 'Stale Ready log claim remains.'
Assert-True ($main.Contains('local function radar_game_thread_callback()')) 'Stable compact game-thread callback is missing.'
Assert-True ($main.Contains('task_kind = "radar"') -and $main.Contains('radar_game_thread_callback()')) 'Compact control is not owned by the serialized ProcessEvent dispatcher.'
foreach ($marker in @('local ACTIVATION_STABLE_SAMPLE_COUNT = 4','local function activation_game_thread_callback()','local function queue_activation_probe()','task_kind = "activation"','process_event_dispatch.request()','ACTIVATION_STABILITY_SAMPLE','ACTIVATION_STABLE')) { Assert-True ($main.Contains($marker)) "Missing stable F7 activation contract: $marker" }
$queueStart = $main.IndexOf('local function queue_radar_update()')
$queueEnd = $main.IndexOf('ensure_world_map_loop_started = function()', $queueStart)
$queueBlock = $main.Substring($queueStart, $queueEnd - $queueStart)
Assert-True (-not $queueBlock.Contains('ExecuteInGameThread(function()')) 'Compact control still creates a request-specific game-thread closure.'
if ($clockEnabled) {
    Assert-True ($main.Contains('one isolated baseline attempt after stable context')) 'Isolated world-status Ready contract is missing.'
    Assert-True ($main.Contains('require, "world_environment"')) 'Production main does not import the isolated clock module.'
    Assert-True ($main.Contains('local function queue_world_time_capture()')) 'Isolated clock queue is missing.'
} else {
    Assert-True (-not $main.Contains('require, "world_environment"') -and -not $main.Contains('world_environment.')) 'Assault-only dev38 must retain dev37 zero-clock production behavior.'
    Assert-True ($main.Contains('World status is fail-closed')) 'Assault-only dev38 Ready log must retain the fail-closed clock contract.'
}
Assert-True (-not $main.Contains('if show_world_status then ensure_loop_started() end')) 'World status still auto-starts before F7.'
if ($clockEnabled) {
    Assert-True ($main.Contains('F7 enables configured marker, save-state, and world-status work')) 'F7/F8 clock lifecycle Ready contract is missing.'
    Assert-True ($defaultConfig.Contains('show_world_status = true')) 'Isolated world status is not enabled by default.'
} else {
    Assert-True ($defaultConfig.Contains('show_world_status = false')) 'Assault-only dev38 must keep world status disabled.'
}
foreach ($marker in @(
    'local world_epoch = 1',
    'local function is_world_epoch_current(epoch)',
    'local function is_lifecycle_epoch_current(epoch)',
    'purge_runtime_references()',
    'begin_post_cooldown_probe',
    'complete_post_cooldown_probe',
    'resume_probe_pending = true',
    'is_world_epoch_current(scheduled_epoch)',
    'reference 1.6.1 Pawn-loss cooldown retry enabled',
    'deferred safely until the current Pawn-loss cooldown completes')) {
    Assert-True ($main.Contains($marker)) "Missing world-transition safety contract: $marker"
}
foreach ($removedRecoveryProbe in @('resolve_fresh_player_roots','try_reacquire_world','queue_world_recovery','recovery_stable_samples','recovery_identity','active_world_identity','GetWorld()','transition_post_seen')) {
    Assert-True (-not $main.Contains($removedRecoveryProbe)) "Unsafe world-identity recovery marker remains: $removedRecoveryProbe"
}
Assert-True (-not $main.Contains('RegisterLoadMapPreHook')) 'Unsafe LoadMap pre-hook remains.'
Assert-True (-not $main.Contains('RegisterLoadMapPostHook')) 'Unsafe LoadMap post-hook remains.'
Assert-True (-not $environment.Contains('local game_singleton = nil')) 'World-time sampling retains a singleton wrapper across travel.'
Assert-True ($main.Contains('local engine = nil')) 'Reference Engine cache is missing.'
Assert-True (-not $main.Contains('local player_controller = nil')) 'Main lifecycle retains an unused PlayerController wrapper.'
Assert-True (-not $main.Contains('local player_pawn = nil')) 'Main lifecycle retains a Pawn wrapper across world teardown.'
Assert-True ($main.Contains('local minimap_layer = nil')) 'Reference minimap-layer cache is missing.'
Assert-True (([regex]::Matches($environment, [regex]::Escape('FindFirstOf, "DGameSingleton"'))).Count -eq 1) 'One-shot clock has more than one DGameSingleton lookup site.'
Assert-True (([regex]::Matches($environment, [regex]::Escape('read_field(singleton, "TimeOfDay")'))).Count -eq 1) 'One-shot clock has more than one TimeOfDay read site.'
Assert-True ($environment.Contains('singleton=nil')) 'Clock capture does not explicitly drop its local singleton wrapper.'
Assert-True ($environment.Contains('return singleton:IsValid()')) 'Clock capture does not use the DataProbe-style protected validity check.'
foreach ($forbiddenClock in @('recalibrat','retry_at','next_retry','FindAllOf','StaticFindObject')) { Assert-True (-not $environment.Contains($forbiddenClock)) "Clock retry/scan path remains: $forbiddenClock" }
$updateStart = $main.IndexOf('local function update_radar_state(queue_delay_ms)')
$updateEnd = $main.IndexOf('local ensure_world_map_loop_started', $updateStart)
$updateBlock = $main.Substring($updateStart, $updateEnd-$updateStart)
foreach ($forbiddenClockAccess in @('TimeOfDay','DGameSingleton','capture_in_game_thread','world_environment.')) { Assert-True (-not $updateBlock.Contains($forbiddenClockAccess)) "Control update touches isolated clock provider: $forbiddenClockAccess" }
if ($clockEnabled) {
    $f7StartForClock = $main.IndexOf('RegisterKeyBind(Key[start_key]')
    $f7EndForClock = $main.IndexOf('RegisterKeyBind(Key[stop_key]', $f7StartForClock)
    $f7ClockBlock = $main.Substring($f7StartForClock, $f7EndForClock-$f7StartForClock)
    Assert-True (-not $f7ClockBlock.Contains('world_environment.arm()')) 'F7 arms the isolated clock before activation stability is confirmed.'
    $activationStartForClock = $main.IndexOf('local function activation_game_thread_callback()')
    $activationEndForClock = $main.IndexOf('local function queue_activation_probe()', $activationStartForClock)
    $activationClockBlock = $main.Substring($activationStartForClock, $activationEndForClock-$activationStartForClock)
    Assert-True ($activationClockBlock.Contains('world_environment.arm()')) 'Stable activation does not arm the isolated clock.'
    foreach ($forbiddenF7Access in @('TimeOfDay','DGameSingleton','FindFirstOf','ExecuteInGameThread','capture_in_game_thread')) { Assert-True (-not $f7ClockBlock.Contains($forbiddenF7Access)) "F7 directly performs clock/UObject work: $forbiddenF7Access" }
}
Assert-True ($main.Contains('world_map.initialize(') -and $main.Contains('is_lifecycle_epoch_current')) 'World-map access is not guarded by the Lua-only lifecycle epoch.'
$lifecycleGuardStart = $main.IndexOf('local function is_lifecycle_epoch_current(epoch)')
$lifecycleGuardEnd = $main.IndexOf('if world_map_markers_enabled then', $lifecycleGuardStart)
$lifecycleGuardBlock = $main.Substring($lifecycleGuardStart, $lifecycleGuardEnd - $lifecycleGuardStart)
Assert-True (-not $lifecycleGuardBlock.Contains('is_current_generation()')) 'Per-property world-map guard performs a shared generation lookup.'
foreach ($marker in @(
    'local function resolve_player_controller()',
    'if engine == nil then',
    'engine = FindFirstOf("Engine")',
    'local function is_valid_object(object)',
    'local function read_minimap_scale()',
    'cache_hit = is_valid_object(minimap_layer)',
    'if not cache_hit then',
    'FindFirstOf("DLayerMiniMap")',
    'object:IsValid()')) {
    Assert-True ($main.Contains($marker)) "Missing reference cached-context contract: $marker"
}
Assert-True (-not $main.Contains('FindFirstOf("DPanelMain")')) 'Minimap gate still queries DPanelMain.'
Assert-True (-not $main.Contains('Overlay_Minimap')) 'Minimap gate still traverses the Overlay_Minimap parent.'
foreach ($removedGate in @('current_minimap_layer:IsVisible()','surface_hidden','compact_surface_visible','set_compact_surface_visible')) { Assert-True (-not $main.Contains($removedGate)) "Custom HUD visibility gate remains: $removedGate" }
Assert-True (-not $main.Contains('FindAllOf("DLayerMiniMap')) 'Minimap scale sampling enumerates UObjects.'
Assert-True (([regex]::Matches($main, [regex]::Escape('FindFirstOf("Engine")'))).Count -eq 1) 'Engine FindFirstOf is not confined to one cache-miss path.'
Assert-True (([regex]::Matches($main, [regex]::Escape('FindFirstOf("DLayerMiniMap")'))).Count -eq 1) 'DLayerMiniMap FindFirstOf is not confined to one cache-miss path.'
$minimapReadStart = $main.IndexOf('local function read_minimap_scale()')
$minimapReadEnd = $main.IndexOf('local function update_radar_radius(scale)', $minimapReadStart)
Assert-True ($minimapReadStart -ge 0 -and $minimapReadEnd -gt $minimapReadStart) 'Minimap scale reader boundaries are missing.'
$minimapReadBlock = $main.Substring($minimapReadStart, $minimapReadEnd - $minimapReadStart)
foreach ($forbiddenNestedValidation in @('is_valid_object(layer_map)','is_valid_object(map_overlay)','MINIMAP_LAYERMAP_ISVALID','MINIMAP_OVERLAY_ISVALID')) {
    Assert-True (-not $minimapReadBlock.Contains($forbiddenNestedValidation)) "Nested minimap wrapper uses incompatible UObject validation: $forbiddenNestedValidation"
}
foreach ($guardedNestedRead in @('local layer_map = current_minimap_layer.LayerMap','local map_overlay = layer_map.MapOverlay','tonumber(map_overlay.RenderTransform.Scale.X)')) {
    Assert-True ($minimapReadBlock.Contains($guardedNestedRead)) "Missing guarded nested minimap read: $guardedNestedRead"
}
foreach ($retainedRootMarker in @('local root_retained = false','root_retained = true','root_retained = root_retained','local resolved_valid = cache_hit')) {
    Assert-True ($minimapReadBlock.Contains($retainedRootMarker)) "Missing retained validated minimap-root contract: $retainedRootMarker"
}
Assert-True (([regex]::Matches($minimapReadBlock, 'minimap_layer\s*=\s*nil')).Count -eq 1) 'Nested minimap read failure can still clear the validated root and restart one-hertz FindFirstOf.'
$nestedMinimapFailureStart = $minimapReadBlock.IndexOf('if not ok or scale == nil then')
$nestedMinimapFailureEnd = $minimapReadBlock.IndexOf('emit_minimap_scale_diagnostic(nil)', $nestedMinimapFailureStart)
Assert-True ($nestedMinimapFailureStart -ge 0 -and $nestedMinimapFailureEnd -gt $nestedMinimapFailureStart) 'Nested minimap failure block is missing.'
$nestedMinimapFailureBlock = $minimapReadBlock.Substring($nestedMinimapFailureStart, $nestedMinimapFailureEnd - $nestedMinimapFailureStart)
Assert-True (-not $nestedMinimapFailureBlock.Contains('minimap_layer = nil')) 'Nested minimap property miss still discards the validated top-level root.'
$minimapCacheModel = [ordered]@{ RootValid=$false; NestedAvailable=$false; Finds=0; Reads=0 }
function Step-MinimapCacheModel {
    if (-not $script:minimapCacheModel.RootValid) {
        $script:minimapCacheModel.Finds++
        $script:minimapCacheModel.RootValid=$true
    }
    $script:minimapCacheModel.Reads++
    return $script:minimapCacheModel.NestedAvailable
}
1..60 | ForEach-Object { $null=Step-MinimapCacheModel }
Assert-True ($minimapCacheModel.Finds -eq 1 -and $minimapCacheModel.Reads -eq 60) 'Sixty nested scale misses caused recurring DLayerMiniMap lookup.'
$minimapCacheModel.NestedAvailable=$true
Assert-True ((Step-MinimapCacheModel) -and $minimapCacheModel.Finds -eq 1) 'Nested scale recovery unnecessarily performed a new global lookup.'
$minimapCacheModel.RootValid=$false
Assert-True ((Step-MinimapCacheModel) -and $minimapCacheModel.Finds -eq 2) 'A genuinely invalid top-level minimap root did not permit one replacement lookup.'
$stableLocationStart = $main.IndexOf('get_player_location = function()')
$stableLocationEnd = $main.IndexOf('local function is_valid_object(object)', $stableLocationStart)
$stableLocationBlock = $main.Substring($stableLocationStart, $stableLocationEnd - $stableLocationStart)
foreach ($scanMarker in @('resolve_fresh_player_roots()','GetWorld()','GetFullName()','FindFirstOf(')) { Assert-True (-not $stableLocationBlock.Contains($scanMarker)) "Stable location path performs scan/recovery work: $scanMarker" }
foreach ($freshPawnMarker in @('local controller = resolve_player_controller()','local current_pawn = controller.Pawn','current_pawn:K2_GetActorLocation()')) { Assert-True ($stableLocationBlock.Contains($freshPawnMarker)) "Reference current-Pawn path is missing: $freshPawnMarker" }
$cooldownProbeStart = $main.IndexOf('local function begin_post_cooldown_probe(expected_epoch, source)')
$cooldownProbeEnd = $main.IndexOf('local function activation_game_thread_callback()', $cooldownProbeStart)
$cooldownProbeBlock = $main.Substring($cooldownProbeStart, $cooldownProbeEnd - $cooldownProbeStart)
foreach ($unsafeCooldownAccess in @('FindFirstOf','FindAllOf','StaticFindObject','ExecuteInGameThread','.Pawn','IsValid','GetWorld','GetFullName','K2_GetActorLocation')) { Assert-True (-not $cooldownProbeBlock.Contains($unsafeCooldownAccess)) "Cooldown release performs UObject work: $unsafeCooldownAccess" }
Assert-True (([regex]::Matches($main, [regex]::Escape('world_map.set_suspended(false, world_epoch)'))).Count -eq 1) 'World-map access does not resume exactly once after player-location success.'
foreach ($marker in @('access_guard','lifecycle_epoch','access_allowed()','world_map.set_suspended(true, world_epoch)')) {
    Assert-True (($worldMap.Contains($marker)) -or ($main.Contains($marker))) "World-map epoch guard is missing: $marker"
}
foreach($marker in @('MAX_CANDIDATES = 2','MAX_RETIRED_IDENTITIES = 256','entry.epoch == lifecycle_epoch','entry.token == candidate_token','retired_identities[identity]','notify-current-epoch','bounded-resume-scan','candidate_token = candidate_token + 1')){Assert-True ($worldMap.Contains($marker)) "Bounded world-map candidate marker is missing: $marker"}
foreach($marker in @('hidden_index = nil','not is_visible(entry.object)','table.remove(candidates, hidden_index)','active visible entries are never')){Assert-True ($worldMap.Contains($marker)) "Hidden world-map candidate replacement marker is missing: $marker"}
foreach($marker in @('function WorldMap.consume_wake_hint()','function WorldMap.recover_session()','function WorldMap.has_retained_candidates()','WORLD_MAP_INACTIVE_CHECK_INTERVAL_MS = 2000','world_map.consume_wake_hint()','world_map.has_retained_candidates()','world_map.recover_session()')){Assert-True (($worldMap+$main).Contains($marker)) "Inactive world-map wake/backoff marker is missing: $marker"}
Assert-True ($worldMap.Contains('record_candidate_metrics("recover-session")')) 'World-map recovery is not observable in debug diagnostics.'
$recoverStart=$worldMap.IndexOf('function WorldMap.recover_session()')
$recoverEnd=$worldMap.IndexOf('function WorldMap.has_retained_candidates()', $recoverStart)
Assert-True ($recoverStart -ge 0 -and $recoverEnd -gt $recoverStart) 'World-map recovery block is missing or malformed.'
$recoverBlock=$worldMap.Substring($recoverStart,$recoverEnd-$recoverStart)
foreach($marker in @('cached_entry = nil','candidates = {}','candidate_token = candidate_token + 1','needs_rescan = true','wake_hint = true')){Assert-True ($recoverBlock.Contains($marker)) "World-map recovery safety marker is missing: $marker"}
Assert-True (-not $recoverBlock.Contains('FindAllOf(')) 'Active world-map recovery directly enumerates UObjects.'
Assert-True (([regex]::Matches($main,[regex]::Escape('world_map.recover_session()'))).Count -eq 2) 'World-map recovery must invalidate candidates in both control and fast-loop failure paths.'
Assert-True (-not $worldMap.Contains('known_layers')) 'Unbounded retained world-map list remains.'
Assert-True (([regex]::Matches($worldMap,[regex]::Escape('FindAllOf("DLayerMap_C")'))).Count -eq 1) 'World-map widget enumeration is not limited to one bounded rescan site.'
$activeReadStart=$worldMap.IndexOf('function WorldMap.read_state()')
$activeReadEnd=$worldMap.IndexOf('function WorldMap.debug_state()',$activeReadStart)
$activeReadBlock=$worldMap.Substring($activeReadStart,$activeReadEnd-$activeReadStart)
Assert-True (-not $activeReadBlock.Contains('FindAllOf(')) 'Active world-map read_state directly enumerates UObjects.'
foreach($marker in @('TimeSpan.FromSeconds(19)','slot.Signature == _lastSlotSignature','key == _lastKey','includeTreasure = true','IncludeTreasure','OpenedAvailable','encounterTaskQueryMs','tb_unexpected_switch_week')){Assert-True (($snapshotReader+$saveState).Contains($marker)) "Change-gated save-query/task diagnostic marker is missing: $marker"}
foreach($marker in @('motion_loop_token','world_map_loop_token','owner_loop_token ~= motion_loop_token','owner_loop_token ~= world_map_loop_token','update_pending_token == request_token','world_motion_pending_token == request_token','world_motion_pending_epoch','world_motion_update_pending','max_logical_loops')){Assert-True (($main+$diagnostics).Contains($marker)) "Async ownership marker is missing: $marker"}
foreach($marker in @('CONTROL_WATCHDOG_SAMPLE_MS = 1000','CONTROL_WATCHDOG_STALE_SAMPLES = 5','MAX_AUTOMATIC_RUNTIME_RESTARTS = 3','local function observe_control_watchdog(delta_ms)','request_runtime_restart("control_watchdog_stalled")','request_runtime_restart("async_failure:" .. tostring(key))','automatic F8->F7 recovery','enter_world_transition("runtime_error_restart")')){Assert-True ($main.Contains($marker)) "Confirmed-error automatic recovery marker is missing: $marker"}
Assert-True (([regex]::Matches($main,[regex]::Escape('LoopAsync('))).Count -eq 3) 'Automatic recovery added a fourth scheduler loop.'
Assert-True (([regex]::Matches($main,[regex]::Escape('request_runtime_restart('))).Count -eq 3) 'Runtime restart may be armed outside the function definition, confirmed async failure, and control watchdog.'
$watchdogStart=$main.IndexOf('local function observe_control_watchdog(delta_ms)')
$watchdogEnd=$main.IndexOf('local function purge_runtime_references()', $watchdogStart)
Assert-True ($watchdogStart -ge 0 -and $watchdogEnd -gt $watchdogStart) 'Control-watchdog block is missing or malformed.'
$watchdogBlock=$main.Substring($watchdogStart,$watchdogEnd-$watchdogStart)
foreach($forbidden in @('FindAllOf(','FindFirstOf(','get_player_location(','world_map.read_state(','ExecuteInGameThread(','write_fast_motion(')){Assert-True (-not $watchdogBlock.Contains($forbidden)) "Control watchdog performs non-scalar work: $forbidden"}
Assert-True (-not $worldMap.Contains('request_runtime_restart')) 'Temporary world-map read loss can arm whole-runtime recovery.'
Assert-True ($main.Contains('local function has_radar_marker_layers()')) 'Central marker-layer predicate is missing.'
Assert-True ($main.Contains('return show_treasures or show_bosses or show_moles or show_assaults')) 'Marker predicate must include Assault and exclude world status/treasure decorations.'
Assert-True ($main.Contains('local function has_configured_visible_features()')) 'Configured whole-mod feature predicate is missing.'
Assert-True ($main.Contains('return has_radar_marker_layers() or show_world_status')) 'F7 does not include configured world status.'
Assert-True ($main.Contains('if not has_configured_visible_features() then')) 'F7 does not use the configured whole-mod predicate.'
Assert-True ($main.Contains('if not enabled or not ensure_layers_loaded() then')) 'Master-disabled control callback can perform provider work.'
Assert-True (-not $main.Contains('(not enabled and not show_world_status)')) 'Legacy status-after-F8 control condition remains.'
Assert-True (-not $main.Contains('mode = "status"')) 'Legacy status-only Lua mode remains.'
Assert-True (-not $radar.Contains('StatusOnlyTimerIntervalMs')) 'Legacy status-only Overlay timer remains.'
Assert-True ($radar.Contains('DisabledTimerIntervalMs = 125')) 'Dev15 disabled Overlay polling was not restored.'
Assert-True (-not $radar.Contains('String.Equals(mode, "status"')) 'Legacy status-only Overlay paint path remains.'
Assert-True ($radar.Contains('if (!motion.Enabled) return;')) 'Disabled paint guard is missing.'
Assert-True ($radar.Contains('if (!motion.Enabled)') -and $radar.Contains('return "disabled";')) 'Disabled frames can still become a visible status mode.'
Assert-True ($radar.Contains('ReferenceStatusStripGap = -6')) 'Raised status overlap is missing.'
Assert-True ($radar.Contains('ReferenceStatusStripHeight = 46')) 'Compact status strip height is missing.'
Assert-True ($radar.Contains('_compactOverlayWidth = ReferenceOverlaySize;')) 'Compact width still extends left of the minimap.'
Assert-True ($radar.Contains('_compactOverlayHeight')) 'Compact status-strip height is not applied.'
foreach ($removed in @('ReferenceStatusExtension','ApplyWindowRegionIfNeeded','WindowRegionGeometry','SetWindowRgn','CreateEllipticRgn','CombineRgn','DeleteObject','RADAR_MAP_EVIDENCE','CompactRadarMapId')) {
    Assert-True (-not $radar.Contains($removed)) "Post-dev15 Overlay symbol remains: $removed"
}
Assert-True ($renderer.Contains('StatusStripGapReference = -6f')) 'Raised renderer offset is missing.'
Assert-True ($renderer.Contains('PhaseFontReferencePixels = 13f')) 'Compact phase font contract is missing.'
Assert-True ($renderer.Contains('CalculateGroupBounds')) 'Bounded status layout helper is missing.'
Assert-True ($radar.Contains('IsWorldMapMode()')) 'Expanded world-map mode contract is missing.'
Assert-True ($main.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 8')) 'Visible world-map scalar presentation is not 8 ms.'
Assert-True ($main.Contains('FAST_MOTION_INTERVAL_MS = 50')) 'Radar producer is not 50 ms.'
Assert-True ($main.Contains('MINIMAP_UPDATE_INTERVAL_MS = 250')) 'Player/control scalar sampling is not 250 ms.'
Assert-True (([regex]::Matches($main,'get_player_location\(\)')).Count -eq 2) 'Fresh full-chain location must have one definition and one 250 ms call site.'
Assert-True (-not $main.Contains('queue_motion_update()')) 'Duplicated 50 ms player sampling call remains.'
$worldMotionStartForScalar=$main.IndexOf('world_map._main_motion_game_thread_callback = function()')
$worldMotionEndForScalar=$main.IndexOf('ensure_motion_loop_started = function()',$worldMotionStartForScalar)
$worldMotionScalarBlock=$main.Substring($worldMotionStartForScalar,$worldMotionEndForScalar-$worldMotionStartForScalar)
Assert-True (-not $worldMotionScalarBlock.Contains('get_player_location()') -and $worldMotionScalarBlock.Contains('latest_motion_x')) 'Large-map loop does not reuse the shared scalar player sample.'
foreach($forbidden in @('LoopInGameThreadAfterFrames','PauseDelayedAction','UnpauseDelayedAction','IsValidDelayedActionHandle','IsDelayedActionActive','world_map_frame_action_')){Assert-True (-not $main.Contains($forbidden)) "Unsafe EngineTick delayed-action marker remains: $forbidden"}
Assert-True (([regex]::Matches($main,[regex]::Escape('ExecuteInGameThread('))).Count -eq 1) 'Raw ExecuteInGameThread bypasses the single ProcessEvent helper.'
Assert-True ($main.Contains('ProcessEventAvailable ~= true') -and $main.Contains('ExecuteInGameThread(callback, EGameThreadMethod.ProcessEvent)') -and $main.Contains('queue_process_event_callback,') -and $main.Contains('process_event_dispatch.callback')) 'Game-thread handoffs are not explicitly serialized fail-closed ProcessEvent dispatches.'
foreach($marker in @('queue_world_motion_update','world_motion_update_pending','world_motion_update_pending = true','world_motion_update_pending = false','world_motion_pending_token == request_token','process_event_dispatch.request()')){Assert-True ($worldMotionScalarBlock.Contains($marker)) "Bounded world-map ProcessEvent marker is missing: $marker"}
$serializedDispatchStart=$main.IndexOf('process_event_dispatch.poison = function(reason, failure)')
$serializedDispatchEnd=$main.IndexOf('local function queue_radar_update()',$serializedDispatchStart)
$serializedDispatchBlock=$main.Substring($serializedDispatchStart,$serializedDispatchEnd-$serializedDispatchStart)
foreach($marker in @('task_kind = "activation"','task_kind = "radar"','task_kind = "clock"','task_kind = "world_motion"','process_event_dispatch.pending','process_event_dispatch.poisoned','process_event_dispatch.callback','queue_process_event_callback,','activation_game_thread_callback()','radar_game_thread_callback()','process_event_dispatch.clock_task()','world_map._main_motion_game_thread_callback()')){Assert-True ($serializedDispatchBlock.Contains($marker)) "Serialized ProcessEvent dispatcher marker is missing: $marker"}
Assert-True (([regex]::Matches($main,[regex]::Escape('queue_process_event_callback,')).Count -eq 1)) 'More than one WorldRadar call site can submit a UE4SS ProcessEvent callback.'
$worldPresentationStart=$main.IndexOf('ensure_world_map_loop_started = function()')
$worldPresentationEnd=$main.IndexOf('local function ensure_loop_started()',$worldPresentationStart)
$worldPresentationBlock=$main.Substring($worldPresentationStart,$worldPresentationEnd-$worldPresentationStart)
Assert-True (-not $worldPresentationBlock.Contains('ExecuteInGameThread(') -and -not $worldPresentationBlock.Contains('world_map.read_state(') -and $worldPresentationBlock.Contains('flush_latest_motion()') -and $worldPresentationBlock.Contains('queue_world_motion_update(owner_loop_token)')) 'The 8 ms expanded presentation loop does not use the bounded ProcessEvent queue.'
Assert-True ($main.Contains('record_world_map_producer_tick()')) 'World-map producer-rate diagnostics are missing.'
Assert-True ($main.Contains('local capture_failure = "none"') -and
    $main.Contains('failure = capture_failure')) 'Successful world-clock diagnostics do not clear the failure field.'
Assert-True (-not $main.Contains('failure = capture_ok and capture_error')) 'Ambiguous world-clock failure expression remains.'
Assert-True (-not $main.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 24')) '24 ms world-map producer constant remains.'
Assert-True (-not $main.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 50')) 'Dev40 50 ms world-map producer constant remains.'
Assert-True (-not $main.Contains('FAST_MOTION_INTERVAL_MS = 24')) '24 ms radar producer constant remains.'
Assert-True ($radar.Contains('ActiveTimerIntervalMs = 33')) 'Overlay active timer is not 33 ms.'
foreach ($marker in @(
    'no_paint_key = "F5"',
    'no_motion_key = "F6"',
    'request_active_diagnostic_mode("no_paint", "F5")',
    'request_active_diagnostic_mode("no_motion", "F6")',
    'request_active_diagnostic_mode("normal", "F7")',
    'diagnostic_mode = "normal"')) {
    Assert-True ($main.Contains($marker) -or $defaultConfig.Contains($marker)) "Missing inline A/B hotkey contract: $marker"
}
$debugHotkeyGuardStart = $main.IndexOf(
    'if config.debug_logging == true then',
    $main.IndexOf('local function request_active_diagnostic_mode('))
$normalHotkeyStart = $main.IndexOf(
    'RegisterKeyBind(Key[start_key]',
    $debugHotkeyGuardStart)
Assert-True ($debugHotkeyGuardStart -ge 0 -and
    $normalHotkeyStart -gt $debugHotkeyGuardStart) 'Debug hotkey guard boundary is malformed.'
$debugHotkeyBlock = $main.Substring(
    $debugHotkeyGuardStart,
    $normalHotkeyStart - $debugHotkeyGuardStart)
Assert-True ($debugHotkeyBlock.Contains('RegisterKeyBind(Key[no_paint_key]') -and
    $debugHotkeyBlock.Contains('RegisterKeyBind(Key[no_motion_key]')) 'F5/F6 are not confined to the debug_logging guard.'
foreach ($marker in @(
    'DiagnosticModeNoPaint = 1',
    'DiagnosticModeNoMotion = 2',
    'DiagnosticNoMotionTimerIntervalMs = 250',
    'if (_diagnosticMode == DiagnosticModeNoPaint)',
    '_diagnosticMode != DiagnosticModeNoMotion',
    '_motionPredictor.Clear()',
    'CloneMotionFrame(motion)')) {
    Assert-True ($radar.Contains($marker)) "Missing Overlay A/B isolation contract: $marker"
}
Assert-True ($main.Contains('MOTION_PROTOCOL_VERSION = 6')) 'A/B mode is not carried by protocol v6.'
Assert-True ($radar.Contains('WorldMapTimerIntervalMs = 8')) 'World-map presentation timer is not 8 ms.'
Assert-True (-not $radar.Contains('WorldIdleTimerIntervalMs')) 'Legacy 50 ms world-map timer remains.'
Assert-True ($radar.Contains('interval = WorldMapTimerIntervalMs;')) 'World-map mode does not deterministically select its diagnostic cadence.'
Assert-True ($radar.Contains('RecordModeTransition(') -and $overlayPerformance.Contains('worldMapPresentationHz=') -and $overlayPerformance.Contains('modeTransitions=')) 'World-map cadence/transition diagnostics are incomplete.'
foreach($marker in @('ScalarMotionPredictor','MaximumPredictionMs = 250.0','StaleFreezeMs = 500.0','MaximumSourceGapMs = 1000.0','frame.WorldEpoch != _worldEpoch','OVERLAY_MOTION_PREDICTION')){Assert-True (($radar+$overlayPerformance).Contains($marker)) "Bounded scalar prediction marker is missing: $marker"}
foreach($marker in @('PositionEpsilonFloor = 20.0','ScreenPixelEpsilon = 0.5','HeightEpsilon = 10.0','positionChanged = deltaX * deltaX','if (!positionChanged)')){Assert-True ($radar.Contains($marker)) "Subpixel Overlay prediction gate is missing: $marker"}
Assert-True ($diagnostics.Contains('player_location_hz')) 'Lua debug output does not expose full-chain sample rate.'
Assert-True ($radar.Contains('MaintenanceIntervalMs = 1000')) 'Overlay maintenance timer is not 1000 ms.'
Assert-True ($radar.Contains('WindowVisibilityCheckIntervalMs = 250')) 'Window visibility sampling is not 250 ms.'
Assert-True ($radar.Contains('_nextWindowVisibilityCheckUtc = DateTime.MinValue;')) 'Mode transitions do not force an immediate window sample.'
foreach($marker in @('MotionBridgeFallbackIntervalMs = 250','MotionBridgePollingFallbackIntervalMs = 50','ShouldForceMotionBridgeScan','ApplyMotionBridgePollingFallback','ChangeNotificationsAvailable','HasPendingChanges','ProcessMotionBridgeWake','SetChangeNotificationsEnabled(!worldMode)','TryReadLatest(')){Assert-True (($radar+$motionReader).Contains($marker)) "Event-driven compact bridge marker is missing: $marker"}
foreach($marker in @('FileSystemWatcher','MotionBridgeDirtyGate',': IDisposable','UpdateOrRearm','Interlocked.Exchange(ref _dirtyMask, 0)','watcher.EnableRaisingEvents = false')){Assert-True ($motionReader.Contains($marker)) "Motion Bridge notification gate is missing: $marker"}
$watcherCallbackStart=$motionReader.IndexOf('private void OnBridgeChanged(')
$watcherCallbackEnd=$motionReader.IndexOf('private static bool SameContent(', $watcherCallbackStart)
Assert-True ($watcherCallbackStart -ge 0 -and $watcherCallbackEnd -gt $watcherCallbackStart) 'Motion Bridge watcher callback boundary is malformed.'
$watcherCallbackBlock=$motionReader.Substring($watcherCallbackStart,$watcherCallbackEnd-$watcherCallbackStart)
foreach($forbidden in @('SharedBridgeFile.ReadInto','MotionRecordParser.TryParse','Task.Run','new Thread')){Assert-True (-not $watcherCallbackBlock.Contains($forbidden)) "Watcher callback performs forbidden work: $forbidden"}
Assert-True (-not $motionReader.Contains('System.Threading.Tasks')) 'Motion Bridge added a background parser task.'
Assert-True ($radar.Contains('RecordWindowVisibilitySample()')) 'Window visibility sample diagnostics are missing.'
Assert-True ($debugSettings.Contains('HighResolutionTimerEnabled')) 'High-resolution timer startup setting is missing.'
Assert-True ($radar.Contains('if (DebugSettings.HighResolutionTimerEnabled)')) 'timeBeginPeriod is not guarded by the A/B setting.'
Assert-True ($defaultConfig.Contains('high_resolution_timer = true')) 'High-resolution timer does not default on.'
Assert-True ($overlayPerformance.Contains('treasureDrawCalls=')) 'Treasure layer timing diagnostics are missing.'
Assert-True ($overlayPerformance.Contains('bossDrawCalls=')) 'Boss layer timing diagnostics are missing.'
Assert-True ($overlayPerformance.Contains('windowVisibilitySampleHz=')) 'Window sample-rate diagnostics are missing.'
Assert-True ($radar.Contains('RecordTreasureDraw(')) 'Treasure layer timing is not recorded.'
Assert-True ($radar.Contains('RecordEncounterDraw(')) 'Unified Boss/Assault layer timing is not recorded.'
Assert-True ($main.Contains('MOTION_SCREEN_PIXEL_EPSILON = 0.5')) 'Subpixel motion suppression is missing.'
Assert-True ($main.Contains('motion_position_epsilon(radius)')) 'Dynamic compact-motion threshold is missing.'
Assert-True ($radar.Contains('RadarTreasureSelectionIntervalMs = 1000')) 'Nearest unopened selection is not 1000 ms.'
foreach($marker in @('generated-fingerprint-bound','known-current','known-legacy','delayed-pattern-fallback','TimeSpan.FromSeconds(30)')){Assert-True ($keyReader.Contains($marker)) "Save-key route is missing: $marker"}
foreach($marker in @('ComputeGameFingerprint','^[0-9a-f]{64}$','executable_length','owner_pointer_rva','install-time-exact-executable-pattern')){Assert-True ($ownerConfig.Contains($marker)) "Generated owner-pointer validation marker is missing: $marker"}
foreach($marker in @('matches.Count == 0','matches.Count != 1','PE section exceeds file bounds','GenerateBoundConfig')){Assert-True ($ownerResolver.Contains($marker)) "Install-time owner-pointer fail-closed marker is missing: $marker"}
Assert-True ($installer.Contains('(?m)^game_fingerprint=(?<value>[0-9a-f]{64})\r?$') -and $installer.Contains('(?m)^executable_length=(?<value>[1-9][0-9]*)\r?$') -and $installer.Contains('(?m)^owner_pointer_rva=0x(?<value>[0-9A-F]+)\r?$')) 'Installer-generated owner-pointer validation is not CRLF/LF safe.'
Assert-True ($radar.Contains('WorldMapId = 100')) 'Dev15 compact map ownership is missing.'
Assert-True ($radar.Contains('_worldTreasureIndex.GetMap(WorldMapId)')) 'Dev15 compact treasure source is missing.'
Assert-True (-not $radar.Contains('RadarMapSessionState')) 'Unsafe retained world-map session state remains.'
Assert-True ($radar.Contains('_worldTreasureIndex.GetMap(map.mapId)')) 'World-map treasure rendering no longer uses frame mapId.'

# Performance acceptance is evidence-driven rather than bound to a historical
# package cadence. Independent safety gates still prohibit retained Pawn state
# and preserve the measured 250 ms fresh-current-player sample boundary.
Assert-True ([regex]::Match($main,'local WORLD_MAP_ACTIVE_INTERVAL_MS = (\d+)').Groups[1].Value -eq '8') 'World-map cadence is not 8 ms.'
foreach($required in @('_worldMapTimerResolutionAcquired','UpdateWorldMapTimerResolution(','ReleaseWorldMapTimerResolution(','RecordTimerResolutionChange','timerResolutionBalance=')){Assert-True (($radar+$overlayPerformance).Contains($required)) "Mode-scoped world-map timer-resolution marker is missing: $required"}
Assert-True (-not $main.Contains('local player_pawn = nil')) 'Candidate retains a cross-frame Pawn wrapper.'
foreach ($marker in @('record_player_location','player_root_avg_ms','actor_location_avg_ms')) { Assert-True (($main+$diagnostics).Contains($marker)) "Fresh-Pawn split timing is missing: $marker" }

# Exact dev15 restoration gates for files that have no approved deviation.
foreach ($relative in @(
    'src\overlay\Rendering\BossMarkerRenderer.cs')) {
    $currentText = Get-Content -LiteralPath (Join-Path $root $relative) -Raw
    $baselineText = Get-Content -LiteralPath (Join-Path $baselineRoot $relative) -Raw
    Assert-True ($currentText -ceq $baselineText) "Runtime file differs from authoritative dev15 baseline: $relative"
}
foreach ($marker in @('BuildHammer','BuildWave','GraphicsPath _activityIcon','_activityIcon.Dispose()')) { Assert-True ($moleRenderer.Contains($marker)) "Allocation-free MiniGame renderer marker is missing: $marker" }
Assert-True ($diagnostics.Contains('record_motion_write')) 'Lua motion-write diagnostics are missing.'
Assert-True ($overlayPerformance.Contains('OVERLAY_WORK_PERF')) 'Overlay work-duty diagnostics are missing.'
$saveState = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
$saveReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveSnapshotReader.cs') -Raw
Assert-True ($saveState.Contains('TimeSpan.FromSeconds(19)')) 'Save change-check interval is not nineteen seconds.'
Assert-True ($treasureOverrides.Contains('public void LoadOnce()') -and $treasureOverrides.Contains('private bool _loadAttempted;') -and $saveState.Contains('_overrides.LoadOnce();')) 'Treasure overrides are not loaded once at Overlay startup.'
Assert-True (-not $treasureOverrides.Contains('RefreshInterval') -and -not $saveState.Contains('_overrides.Refresh();')) 'Runtime treasure-override filesystem polling remains.'
foreach ($marker in @('SetRuntimeEnabled','ThreadPriority.BelowNormal','SAVE_REFRESH_PERF','changeCheckIntervalMs=')) { Assert-True ($saveState.Contains($marker)) "Missing optimized save-state contract: $marker" }
foreach ($marker in @('SnapshotCompletionCooldown','_nextSnapshotEligibleUtc','RememberPendingChange(slot, key);','completionCooldownMs=')) { Assert-True ($saveState.Contains($marker)) "Missing post-completion save cooldown contract: $marker" }
Assert-True (-not $radar.Contains('_nextSaveFilterLogUtc') -and $radar.Contains('_lastSaveFilterSaveVersion') -and $radar.Contains('_lastSaveFilterIndexVersion')) 'Detailed treasure diagnostics are not version-gated.'
foreach ($marker in @('ThreadModeBackgroundBegin','ThreadModeBackgroundEnd','SetThreadPriority(','GetCurrentThread()')) { Assert-True (($saveState+$nativeMethods).Contains($marker)) "Missing background save-worker scheduling contract: $marker" }
foreach ($marker in @('DatabaseCacheHits','TryGetCached','TryGetCompatibleCached','BuildCacheKey','includeTreasure','SqliteOpenReadWrite','ApplyKey(database, key)','QueryOpenedTreasureBits(database)','QueryBossRespawns(database, encounterTargetIds)','BuildActorFilterSql(encounterTargetIds)','ActorFilterSignature')) { Assert-True ($saveReader.Contains($marker)) "Missing optimized snapshot-reader contract: $marker" }
Assert-True ($saveReader.Contains('if (DebugSettings.Enabled)') -and $saveReader.Contains('QueryEncounterTaskTable(database)')) 'Diagnostic-only encounter task query is not gated by debug mode.'
Assert-True ($saveReader.Contains('WHERE OPENED_BIT_FIELD <> 0;')) 'Treasure query still returns zero-only categories.'
Assert-True ($encounterAvailability.Contains('saveVersion == _saveVersion') -and $encounterAvailability.Contains('Object.ReferenceEquals(encounters, _catalog)')) 'Encounter availability repeats unchanged save-state traversal.'
Assert-True ($encounterAvailability.Contains('DateTime nowUtc') -and $encounterAvailability.Contains('nowUtc < next')) 'Encounter paint does not share one coherent UTC sample.'
foreach ($marker in @('CacheMissIncludesTreasure = true','bool readIncludesTreasure =','TreasureRequested','TreasureQueries','changeCheckIntervalMs=')) { Assert-True (($saveReader+$saveState).Contains($marker)) "Missing change-gated cold-read contract: $marker" }
Assert-True ($saveReader.Contains('StoreCached(') -and $saveReader.Contains('readIncludesTreasure,')) 'Cache miss does not store the warmed treasure-rich shape.'
Assert-True (-not $saveReader.Contains('(includeTreasure ? "|treasure" : "|encounter")')) 'Treasure and encounter query modes still overwrite one path-owned cache entry.'
Assert-True ($saveReader.Contains('string richKey = BuildCacheKey(') -and $saveReader.Contains('!includeTreasure')) 'Encounter-only reads do not reuse a compatible treasure-rich cache entry.'
Assert-True ($saveReader.Contains('MergeRequestedSnapshot(') -and $saveReader.Contains('if (includeTreasure && source.OpenedAvailable)')) 'Encounter-only rich-cache reuse can publish treasure state or extend the treasure deadline.'
Assert-True ($saveState.Contains('bool treasureRefreshAvailable = request.IncludeTreasure') -and $saveState.Contains('&& snapshot.OpenedAvailable;')) 'Treasure publication is not guarded by the scheduled request scope.'
Assert-True (([regex]::Matches($saveReader, 'PRAGMA key')).Count -eq 1) 'SQLCipher key is applied more than once per database connection.'
$coldReadModel=[ordered]@{Enabled=$true;Loaded=$false;Fingerprint='';Reads=0}
function Try-ColdReadModel([string]$Fingerprint) {
    if(-not$script:coldReadModel.Enabled){return $false}
    if($script:coldReadModel.Loaded-and$Fingerprint-eq$script:coldReadModel.Fingerprint){return $false}
    $script:coldReadModel.Loaded=$true;$script:coldReadModel.Fingerprint=$Fingerprint;$script:coldReadModel.Reads++;return $true
}
Assert-True (Try-ColdReadModel 'A') 'Initial save snapshot did not run immediately.'
1..4|ForEach-Object{Assert-True (-not(Try-ColdReadModel 'A')) 'An unchanged fingerprint queued redundant SQLCipher work.'}
Assert-True ((Try-ColdReadModel 'B')-and$coldReadModel.Reads-eq2) 'A changed fingerprint did not release one fresh snapshot.'
$coldReadModel.Enabled=$false
Assert-True (-not(Try-ColdReadModel 'C')) 'F8-disabled state queued a save read.'

# Completion-based scheduling model: a long or failed SQLCipher worker cannot
# cause an immediate catch-up query, while the newest observed fingerprint is
# retained until it becomes eligible.
$saveCooldownModel=[ordered]@{
    Published='A';Pending=$null;Busy=$false
    EligibleUtc=[DateTime]::MinValue;Reads=0
}
function Try-QueueCooldownModel([string]$Fingerprint,[DateTime]$NowUtc) {
    if ($Fingerprint -eq $script:saveCooldownModel.Published) {
        $script:saveCooldownModel.Pending=$null
        return $false
    }
    $script:saveCooldownModel.Pending=$Fingerprint
    if ($script:saveCooldownModel.Busy -or
        $NowUtc -lt $script:saveCooldownModel.EligibleUtc) {
        return $false
    }
    $script:saveCooldownModel.Busy=$true
    $script:saveCooldownModel.Reads++
    return $true
}
function Complete-CooldownModel([string]$Fingerprint,[DateTime]$NowUtc,[bool]$Success) {
    if ($Success) {
        $script:saveCooldownModel.Published=$Fingerprint
        if ($script:saveCooldownModel.Pending -eq $Fingerprint) {
            $script:saveCooldownModel.Pending=$null
        }
    }
    $script:saveCooldownModel.Busy=$false
    $script:saveCooldownModel.EligibleUtc=$NowUtc.AddSeconds(19)
}
$modelStart=[DateTime]::Parse('2026-08-13T00:00:00Z').ToUniversalTime()
Assert-True (Try-QueueCooldownModel 'B' $modelStart) 'Changed save did not queue the first snapshot.'
Assert-True (-not (Try-QueueCooldownModel 'C' $modelStart.AddSeconds(19))) 'Busy worker queued overlapping SQLCipher work.'
Complete-CooldownModel 'B' $modelStart.AddSeconds(20) $true
Assert-True ($saveCooldownModel.Pending -eq 'C') 'Newer fingerprint was lost when the older worker completed.'
Assert-True (-not (Try-QueueCooldownModel 'C' $modelStart.AddSeconds(38))) 'Completion cooldown allowed an early catch-up snapshot.'
Assert-True (Try-QueueCooldownModel 'C' $modelStart.AddSeconds(39)) 'Pending fingerprint did not queue when completion cooldown expired.'
Complete-CooldownModel 'C' $modelStart.AddSeconds(40) $false
Assert-True (-not (Try-QueueCooldownModel 'C' $modelStart.AddSeconds(58))) 'Failed worker did not receive the same retry cooldown.'
Assert-True (Try-QueueCooldownModel 'C' $modelStart.AddSeconds(59)) 'Failed worker did not become eligible after one bounded cooldown.'
foreach ($catalogRelative in @('src\overlay\Data\WorldTreasureCatalog.cs','src\overlay\Data\WorldBossCatalog.cs')) {
    $catalogText = Get-Content -LiteralPath (Join-Path $root $catalogRelative) -Raw
    Assert-True ($catalogText.Contains('if (_hasLoaded)')) "Immutable catalog one-load gate is missing: $catalogRelative"
}
$baselineRadar = Get-Content -LiteralPath (Join-Path $baselineRoot 'src\overlay\UI\RadarForm.cs') -Raw
$queryStart = $radar.IndexOf('        private sealed class RadarTreasureQueryBuffer')
$baselineQueryStart = $baselineRadar.IndexOf('        private sealed class RadarTreasureQueryBuffer')
$queryEnd = $radar.IndexOf('        private sealed class MotionVisualSnapshot', $queryStart)
$baselineQueryEnd = $baselineRadar.IndexOf('        private sealed class MotionVisualSnapshot', $baselineQueryStart)
Assert-True ($queryStart -ge 0 -and $baselineQueryStart -ge 0 -and $queryEnd -gt $queryStart -and $baselineQueryEnd -gt $baselineQueryStart -and $radar.Substring($queryStart, $queryEnd - $queryStart) -ceq $baselineRadar.Substring($baselineQueryStart, $baselineQueryEnd - $baselineQueryStart)) 'Radar treasure query-buffer implementation differs from dev15.'
$refreshStart = $radar.IndexOf('        private bool RefreshRadarTreasureSelection(')
$refreshEnd = $radar.IndexOf('        private MotionFrame GetCurrentMotion()', $refreshStart)
$baselineRefreshStart = $baselineRadar.IndexOf('        private bool RefreshRadarTreasureSelection(')
$baselineRefreshEnd = $baselineRadar.IndexOf('        private MotionFrame GetCurrentMotion()', $baselineRefreshStart)
Assert-True ($radar.Substring($refreshStart, $refreshEnd-$refreshStart) -ceq $baselineRadar.Substring($baselineRefreshStart, $baselineRefreshEnd-$baselineRefreshStart)) 'Radar compact map/source selection differs from dev15.'

# Only the approved active/selection intervals may differ from dev15.
$allowedRadarCadence = @('ActiveTimerIntervalMs','WorldIdleTimerIntervalMs','RadarTreasureSelectionIntervalMs','MaintenanceIntervalMs','StatusOnlyTimerIntervalMs')
$currentIntervals = @{}; $baselineIntervals = @{}
[regex]::Matches($radar, 'private const int (\w*IntervalMs) = (\d+);') | ForEach-Object { $currentIntervals[$_.Groups[1].Value]=$_.Groups[2].Value }
[regex]::Matches($baselineRadar, 'private const int (\w*IntervalMs) = (\d+);') | ForEach-Object { $baselineIntervals[$_.Groups[1].Value]=$_.Groups[2].Value }
foreach ($name in $baselineIntervals.Keys) {
    if ($name -eq 'StatusOnlyTimerIntervalMs') {
        Assert-True (-not $currentIntervals.ContainsKey($name)) 'Approved whole-mod lifecycle retained the obsolete status-only timer.'
    } elseif ($allowedRadarCadence -notcontains $name) {
        Assert-True ($currentIntervals[$name] -eq $baselineIntervals[$name]) "Unapproved Radar cadence deviation: $name"
    }
}
$baselineMain = Get-Content -LiteralPath (Join-Path $baselineRoot 'scripts\main.lua') -Raw
foreach ($name in @('MINIMAP_SCALE_CHECK_INTERVAL_MS','MAP_LOAD_RESUME_DELAY_MS','MINIMAP_UPDATE_INTERVAL_MS','FAST_MOTION_HEARTBEAT_MS')) {
    $pattern = 'local ' + $name + ' = (\d+)'
    Assert-True ([regex]::Match($main,$pattern).Groups[1].Value -eq [regex]::Match($baselineMain,$pattern).Groups[1].Value) "Unapproved Lua cadence deviation: $name"
}
foreach ($source in @($bossRenderer,$moleRenderer)) {
    foreach ($removed in @('EVENTRADAR_VECTOR_MARKERS','EnsureCache','DrawImageUnscaled','CacheBlits','CacheRebuilds','CacheRebuildMilliseconds','VectorDraws','_markerCache')) {
        Assert-True (-not $source.Contains($removed)) "Removed marker-cache symbol returned: $removed"
    }
    Assert-True ($source.Contains('public void DrawMarker')) 'Direct vector DrawMarker entry point is missing.'
}
Assert-True (-not ($defaultOverrides -match '(?m)^\s*ignore\s+10220122\s*(?:#.*)?$')) 'Default overrides ignore fixed chest 10220122.'
Assert-True (-not ($defaultOverrides -match '(?im)^\s*ignore\s+11003\s*(?:#.*)?$')) 'Default overrides hide valid MiniGame/Fly reward 11003.'
Assert-True (-not $defaultOverrides.Contains('11003 overlaps 14016') -and -not $defaultOverrides.Contains('alias 11003 14016')) 'Obsolete 11003/14016 duplicate guidance remains.'
Assert-True ($installer.Contains('Overlay startup fixes one 49-record Boss/Assault encounter catalog')) 'Installer omits the unified encounter production contract.'
if ($clockEnabled) {
    Assert-True ($installer.Contains('F7 only enables configured marker, save-state, and isolated world-clock work')) 'Installer does not describe F7 isolated-clock enablement.'
} else {
    Assert-True ($installer.Contains('World status remains disabled for the Assault-only A/B')) 'Installer does not describe the dev38 zero-clock boundary.'
}
Assert-True ($installer.Contains('F8 disables all active mod work for FPS comparison')) 'Installer does not describe F8 fail-closed disablement.'
Assert-True (-not $installer.Contains('World time is automatic; F7 enables radar markers')) 'Stale installer marker-only F7 claim remains.'
Assert-True ($installer.Contains("`$_ -notmatch '^\s*ignore\s+10220122\s*(?:#.*)?$'")) 'Missing removal-only 10220122 migration.'
Assert-True ($installer.Contains("`$_ -notmatch '^\s*#\s*Known abandoned or inaccessible chest record\.\s*$'")) 'Missing exact obsolete 10220122 explanation removal.'
Assert-True ($installer.Contains("`$_ -notmatch '^\s*ignore\s+11003\s*(?:#.*)?$'")) 'Missing valid-reward 11003 migration.'
Assert-True ($installer.Contains('REMOVED obsolete treasure override rules for valid records 10220122/11003')) 'Combined valid-record migration log is missing.'
$legacyOverrides = @(
    '# user rule',
    'ignore 77777',
    'alias 11003 14016',
    'ignore 10220122 # obsolete stock variant',
    '# Known abandoned duplicate: 11003 overlaps 14016 at the same chest location.',
    '# Keep the authoritative 14016 record and suppress the offset duplicate.',
    '  IGNORE 11003 # retained duplicate rule')
$migratedOverrides = @($legacyOverrides | Where-Object {
    $_ -notmatch '^\s*ignore\s+10220122\s*(?:#.*)?$' -and
    $_ -notmatch '^\s*#\s*Known abandoned or inaccessible chest record\.\s*$' -and
    $_ -notmatch '^\s*ignore\s+11003\s*(?:#.*)?$' -and
    $_ -notmatch '^\s*#\s*Known abandoned duplicate: 11003 overlaps 14016 at the same chest location\.\s*$' -and
    $_ -notmatch '^\s*#\s*Keep the authoritative 14016 record and suppress the offset duplicate\.\s*$'
})
Assert-True ($migratedOverrides.Count -eq 3) 'Valid-record migration removed an unexpected number of user lines.'
Assert-True ($migratedOverrides -contains 'ignore 77777') 'Accepted migration removed an unrelated ignore rule.'
Assert-True ($migratedOverrides -contains 'alias 11003 14016') 'Accepted migration removed a user alias.'
Assert-True (@($migratedOverrides | Where-Object { $_ -match '^\s*ignore\s+11003' }).Count -eq 0) 'Valid-reward migration retained ignore 11003.'
$missingPlayer = $main.IndexOf('if player_x == nil or player_y == nil then')
$clearPublishedTime = $main.IndexOf('world_time_available = false', $missingPlayer)
$missingPlayerWrite = $main.IndexOf('write_disabled_motion()', $missingPlayer)
Assert-True ($missingPlayer -ge 0 -and $clearPublishedTime -gt $missingPlayer -and $clearPublishedTime -lt $missingPlayerWrite) 'Context-loss frame can serialize stale world-time availability.'
foreach ($forbiddenRuntimeModCheck in @('is_mod_enabled_in_mods_file','MOD_SWITCH_CHECK_INTERVAL_MS','IsEnabledInModsFile')) { Assert-True (-not ($main+$radar).Contains($forbiddenRuntimeModCheck)) "Runtime mods.txt polling remains: $forbiddenRuntimeModCheck" }
$f7Gate = $main.IndexOf('if not has_configured_visible_features() then')
$f7Enable = $main.IndexOf('activation_requested = true', $f7Gate)
$f7Return = $main.IndexOf('return', $f7Gate)
Assert-True ($f7Gate -ge 0 -and $f7Return -lt $f7Enable) 'F7 configured-feature gate is malformed.'
$f8Start = $main.IndexOf('RegisterKeyBind(Key[stop_key]')
$f8End = $main.IndexOf('reset_bridge_files()', $f8Start)
$f8Block = $main.Substring($f8Start, $f8End - $f8Start)
Assert-True (($f8Block.Split(@('enter_world_transition("f8")'), [System.StringSplitOptions]::None).Count - 1) -eq 1) 'F8 transition invalidation is not one-shot.'
Assert-True (-not $f8Block.Contains('world_environment.')) 'F8 still touches the unsafe world-time provider.'
Assert-True ($main.Contains('mole_completion.set_context_available(false)') -and $main.Contains('mole_completion.invalidate_runtime_handles()')) 'Transition purge does not stop and invalidate Mole/Fly work.'
Assert-True (-not $f8Block.Contains('ensure_loop_started()')) 'F8 restarts the 250 ms control loop.'
Assert-True (-not $f8Block.Contains('show_world_status =')) 'F8 mutates configured world-status choice.'
Assert-True (-not $f8Block.Contains('show_treasures =')) 'F8 mutates configured treasure choice.'
Assert-True ($main.Contains('published_show_world_status = is_enabled == true and show_world_status')) 'Disabled bridge frame does not hide configured world status.'
Assert-True ($main.Contains('published_show_moles = is_enabled == true and show_moles')) 'Disabled bridge frame does not hide configured Mole layer.'
foreach ($unfinished in @(34,17,5,1)) {
    $spacing = [Math]::Max(1,[Math]::Floor(240.0/$unfinished))
    Assert-True (($spacing*$unfinished) -le 240) "Sweep bound failed for $unfinished"
    Assert-True ($spacing -ge 1) "More than one query per callback for $unfinished"
}
function Extrapolate([double]$base,[double]$elapsed) { [Math]::Floor(($base+$elapsed*60.0)%86400.0) }
Assert-True ((Extrapolate 3600 1) -eq 3660) 'Monotonic extrapolation failed.'
Assert-True ((Extrapolate 3600 60) -eq 7200) 'One-game-hour calibration boundary failed.'
Assert-True ((Extrapolate 86370 1) -eq 30) 'Midnight wrap failed.'
Assert-True ((Extrapolate 3600 7) -eq 4020) 'Skipped/delayed callbacks did not advance from real wall elapsed.'
Assert-True (([Math]::Floor((Extrapolate 3600 0.9)/60)) -eq 60) 'Minute stability failed.'
Assert-True (([Math]::Floor((Extrapolate 3600 1.0)/60)) -eq 61) 'Minute transition failed.'

# Executable state-model checks mirroring the production callback-cadence
# contract. Direct Lua execution is not available in this source environment.
$model = [ordered]@{ Armed=$false; Attempted=$false; Synced=$false; StableTicks=0; Reads=0; Base=0 }
function Arm-ClockModel {
    $script:model.Armed=$true; $script:model.Attempted=$false
    $script:model.Synced=$false; $script:model.StableTicks=0
}
function Cancel-ClockModel {
    $script:model.Armed=$false; $script:model.Attempted=$false
    $script:model.Synced=$false; $script:model.StableTicks=0
}
function Step-ClockModel([bool]$context,$actual) {
    if (-not $script:model.Armed -or $script:model.Synced -or $script:model.Attempted) { return }
    if (-not $context) { $script:model.StableTicks=0; return }
    $script:model.StableTicks++
    if ($script:model.StableTicks -lt 8) { return }
    $script:model.Attempted=$true; $script:model.Reads++
    if ($null -ne $actual) { $script:model.Base=[int]$actual; $script:model.Synced=$true }
}
Arm-ClockModel
1..7 | ForEach-Object { Step-ClockModel $true $null }
Assert-True ($model.Reads -eq 0) 'Clock sampled before the stable-context delay.'
Step-ClockModel $true 3600
Assert-True ($model.Synced -and $model.Reads -eq 1 -and $model.Base -eq 3600) 'One-shot clock synchronization failed.'
1..400 | ForEach-Object { Step-ClockModel $true 7200 }
Assert-True ($model.Reads -eq 1) 'Clock performed a recalibration read after synchronization.'
Arm-ClockModel
1..8 | ForEach-Object { Step-ClockModel $true $null }
1..400 | ForEach-Object { Step-ClockModel $true 7200 }
Assert-True (-not $model.Synced -and $model.Reads -eq 2) 'Failed one-shot clock capture retried within one F7 activation.'
Cancel-ClockModel
1..20 | ForEach-Object { Step-ClockModel $true 7200 }
Assert-True ($model.Reads -eq 2) 'F8-cancelled clock performed provider work.'

if ($clockEnabled) {
    foreach ($marker in @('clock_capture_task_token = nil','local owns_task = clock_capture_task_token == token','stale_task = not owns_task')) { Assert-True ($main.Contains($marker)) "Clock pending-token lifecycle marker is missing: $marker" }
    $purgeStart=$main.IndexOf('local function purge_runtime_references()')
    $purgeEnd=$main.IndexOf('enter_world_transition = function(reason)', $purgeStart)
    $purgeBlock=$main.Substring($purgeStart,$purgeEnd-$purgeStart)
    Assert-True ($purgeBlock.Contains('clock_capture_task_pending = false') -and $purgeBlock.Contains('clock_capture_task_token = nil')) 'Transition purge does not clear the local clock queue gate.'
    $queueModel=[ordered]@{ProviderToken=1;LocalToken=$null;Pending=$false;NativeReads=0;FreshQueues=0}
    function Queue-ClockTaskModel { $script:queueModel.Pending=$true;$script:queueModel.LocalToken=$script:queueModel.ProviderToken;$script:queueModel.FreshQueues++;return $script:queueModel.LocalToken }
    function Invalidate-ClockTaskModel { $script:queueModel.ProviderToken++;$script:queueModel.Pending=$false;$script:queueModel.LocalToken=$null }
    function Run-ClockTaskModel([int]$Token) { if($Token-ne$script:queueModel.ProviderToken){return};$script:queueModel.NativeReads++ }
    $old=Queue-ClockTaskModel;Invalidate-ClockTaskModel;Run-ClockTaskModel $old
    $fresh=Queue-ClockTaskModel;Run-ClockTaskModel $fresh
    Assert-True ($queueModel.FreshQueues-eq 2 -and $queueModel.NativeReads-eq 1 -and $fresh-ne$old) 'Queue -> F8/travel -> stale callback -> next F7 lifecycle is incorrect.'
}

# Expanded-map lifecycle model: at most one ProcessEvent callback may remain
# queued. Travel/F8 invalidates its numeric epoch without opening the pending
# gate; only the stale callback itself drains that gate before a new queue.
$worldRequest = [ordered]@{ Epoch=1; LoopToken=1; NextToken=0; Pending=$false; PendingToken=$null; PendingEpoch=$null; Owner=$null; Queues=0; Reads=0; PeakPending=0 }
function Queue-WorldRequestModel {
    if($script:worldRequest.Pending){return $null}
    $script:worldRequest.NextToken++
    $script:worldRequest.Pending=$true
    $script:worldRequest.PendingToken=$script:worldRequest.NextToken
    $script:worldRequest.PendingEpoch=$script:worldRequest.Epoch
    $script:worldRequest.Owner=$script:worldRequest.LoopToken
    $script:worldRequest.Queues++
    $script:worldRequest.PeakPending=1
    return $script:worldRequest.PendingToken
}
function Invalidate-WorldRequestModel {
    $script:worldRequest.Epoch++
    $script:worldRequest.LoopToken++
}
function Run-WorldRequestModel([int]$Token,[int]$Epoch,[int]$Owner) {
    if($Epoch-eq$script:worldRequest.Epoch-and$Owner-eq$script:worldRequest.LoopToken){$script:worldRequest.Reads++}
    if($script:worldRequest.PendingToken-eq$Token-and$script:worldRequest.PendingEpoch-eq$Epoch-and$script:worldRequest.Owner-eq$Owner){
        $script:worldRequest.Pending=$false
        $script:worldRequest.PendingToken=$null
        $script:worldRequest.PendingEpoch=$null
        $script:worldRequest.Owner=$null
    }
}
$oldToken=Queue-WorldRequestModel;$oldEpoch=$worldRequest.PendingEpoch;$oldOwner=$worldRequest.Owner
Invalidate-WorldRequestModel
$blockedToken=Queue-WorldRequestModel
Run-WorldRequestModel $oldToken $oldEpoch $oldOwner
$newToken=Queue-WorldRequestModel;$newEpoch=$worldRequest.PendingEpoch;$newOwner=$worldRequest.Owner
Run-WorldRequestModel $newToken $newEpoch $newOwner
Assert-True ($null-eq$blockedToken-and$worldRequest.Queues-eq2-and$worldRequest.Reads-eq1-and$worldRequest.PeakPending-eq1) 'Expanded-map pending gate allowed overlap or stale UObject reads across a transition.'
foreach ($marker in @('world_map._main_motion_game_thread_callback = function()','local function queue_world_motion_update(owner_loop_token)','world_motion_update_pending','world_motion_pending_token == request_token','ExecuteInGameThread(callback, EGameThreadMethod.ProcessEvent)','task_kind = "world_motion"')) {
    Assert-True ($main.Contains($marker)) "Bounded expanded-map ProcessEvent marker is missing: $marker"
}

# Executable master-gate model: configuration ownership is independent from
# temporary runtime enablement, and context changes cannot bypass F8.
$master = [ordered]@{
    Enabled=$false; Context=$true; ConfigTreasures=$true;
    ConfigBosses=$false; ConfigMoles=$true; ConfigWorldStatus=$true;
    PublishedStatus=$false; PublishedMarkers=$false;
    ProviderRefreshes=0; ActiveTicks=0; DisabledWrites=0
}
function Enable-MasterModel {
    $script:master.Enabled=$true
    $script:master.PublishedStatus=$script:master.ConfigWorldStatus
    $script:master.PublishedMarkers=($script:master.ConfigTreasures -or $script:master.ConfigBosses -or $script:master.ConfigMoles)
}
function Disable-MasterModel {
    $script:master.Enabled=$false
    $script:master.PublishedStatus=$false
    $script:master.PublishedMarkers=$false
    $script:master.DisabledWrites++
}
function Step-MasterModel {
    if (-not $script:master.Enabled -or -not $script:master.Context) { return }
    $script:master.ProviderRefreshes++
    if ($script:master.PublishedMarkers) { $script:master.ActiveTicks++ }
}
Enable-MasterModel; Step-MasterModel; Disable-MasterModel
$configuredBefore = @($master.ConfigTreasures,$master.ConfigBosses,$master.ConfigMoles,$master.ConfigWorldStatus) -join ','
1..8 | ForEach-Object { Step-MasterModel }
$master.Context=$false; Step-MasterModel; $master.Context=$true; Step-MasterModel
Assert-True (-not $master.Enabled) 'Context reacquisition bypassed the F8 master gate.'
Assert-True (-not $master.PublishedStatus -and -not $master.PublishedMarkers) 'F8 did not hide all visible features.'
Assert-True ($master.ProviderRefreshes -eq 1 -and $master.ActiveTicks -eq 1) 'Master-disabled state performed functional provider or 50 ms work.'
Assert-True ($master.DisabledWrites -eq 1) 'Master-disabled lifecycle frame was not one-shot.'
Assert-True ((@($master.ConfigTreasures,$master.ConfigBosses,$master.ConfigMoles,$master.ConfigWorldStatus) -join ',') -eq $configuredBefore) 'F8 mutated configured layer choices.'
Enable-MasterModel
Assert-True ($master.PublishedStatus -and $master.PublishedMarkers -and -not $master.ConfigBosses) 'F7 did not restore exactly the configured visible layers.'

# Executable world-epoch model: transition entry invalidates queued work,
# cooldown ticks perform zero probes, one normal location probe is released
# after three seconds, and every failed probe rearms the complete cooldown.
$lifecycle = [ordered]@{ Epoch=1; Transition=$true; Requested=$false; Enabled=$false; Delay=0; Probes=0; Stable=0 }
function Enter-TransitionModel([string]$Reason) {
    $script:lifecycle.Epoch++; $script:lifecycle.Transition=$true
    $script:lifecycle.Enabled=$false; $script:lifecycle.Stable=0
    $script:lifecycle.Delay=if ($Reason -eq 'f8') { 0 } else { 3000 }
}
function Release-ProbeModel([int]$Epoch) {
    if ($Epoch -ne $script:lifecycle.Epoch -or -not $script:lifecycle.Transition -or -not $script:lifecycle.Requested -or $script:lifecycle.Delay -gt 0) { return $false }
    $script:lifecycle.Transition=$false; $script:lifecycle.Enabled=$false; $script:lifecycle.Stable=0
    $script:lifecycle.Probes++; return $true
}
function Accept-StableSampleModel {
    if ($script:lifecycle.Transition -or -not $script:lifecycle.Requested) { $script:lifecycle.Stable=0; return $false }
    $script:lifecycle.Stable++
    if ($script:lifecycle.Stable -ge 4) { $script:lifecycle.Enabled=$true; return $true }
    return $false
}
function F7-LifecycleModel {
    $script:lifecycle.Requested=$true
    if ($script:lifecycle.Transition -and $script:lifecycle.Delay -eq 0) { return (Release-ProbeModel $script:lifecycle.Epoch) }
    return (-not $script:lifecycle.Transition)
}
function F8-LifecycleModel { $script:lifecycle.Requested=$false; Enter-TransitionModel 'f8' }
function Step-CooldownModel {
    if ($script:lifecycle.Delay -gt 0) {
        $script:lifecycle.Delay=[Math]::Max(0,$script:lifecycle.Delay-250)
        return $false
    }
    return (Release-ProbeModel $script:lifecycle.Epoch)
}
Assert-True (F7-LifecycleModel) 'F7 did not release one normal startup location probe.'
Assert-True ($lifecycle.Probes -eq 1 -and -not $lifecycle.Enabled) 'Startup probe release enabled features before stability confirmation.'
1..3 | ForEach-Object { Assert-True (-not (Accept-StableSampleModel)) 'Activation completed before four stable samples.' }
Assert-True ((Accept-StableSampleModel) -and $lifecycle.Enabled) 'Four stable samples did not complete activation.'
Enter-TransitionModel 'player_access_failed'; $staleEpoch=$lifecycle.Epoch-1
Assert-True (-not (Release-ProbeModel $staleEpoch)) 'Stale queued epoch survived transition invalidation.'
$probesBefore=$lifecycle.Probes
1..12 | ForEach-Object { Assert-True (-not (Step-CooldownModel)) 'Cooldown released a probe before three seconds elapsed.' }
Assert-True ($lifecycle.Probes -eq $probesBefore -and $lifecycle.Delay -eq 0) 'Cooldown performed UObject-equivalent probe work.'
Assert-True ((Step-CooldownModel) -and $lifecycle.Probes -eq ($probesBefore+1)) 'Cooldown did not release exactly one normal location probe.'
Enter-TransitionModel 'player_access_failed'
Assert-True ($lifecycle.Delay -eq 3000 -and -not $lifecycle.Enabled) 'Failed recovery probe did not rearm the full cooldown.'
F8-LifecycleModel
Assert-True (-not $lifecycle.Requested -and -not $lifecycle.Enabled) 'F8 did not cancel deferred activation safely.'

# Executable bounded-candidate and loop-token models. Old wrappers may remain
# externally valid, but a scan cannot promote a retired identity into a new
# epoch. Notify creation is explicit current-epoch evidence.
$candidateModel=[ordered]@{Epoch=1;Token=1;Cap=2;Candidates=@();Retired=@{};Max=0}
function Reset-CandidateModel {
    foreach($entry in $script:candidateModel.Candidates){$script:candidateModel.Retired[$entry.Identity]=$true}
    $script:candidateModel.Candidates=@();$script:candidateModel.Epoch++;$script:candidateModel.Token++
}
function Admit-CandidateModel([string]$Identity,[string]$Source) {
    if($Source-ne'notify'-and$script:candidateModel.Retired.ContainsKey($Identity)){return $false}
    if($script:candidateModel.Candidates.Count-ge$script:candidateModel.Cap){return $false}
    $script:candidateModel.Candidates+=,[ordered]@{Identity=$Identity;Epoch=$script:candidateModel.Epoch;Token=$script:candidateModel.Token;Source=$Source}
    $script:candidateModel.Max=[Math]::Max($script:candidateModel.Max,$script:candidateModel.Candidates.Count);return $true
}
1..100|ForEach-Object{
    $old='old-'+$_;Assert-True (Admit-CandidateModel $old 'notify') 'Current notify candidate was not admitted.'
    Assert-True ((Admit-CandidateModel ('current-'+$_) 'scan')) 'Bounded scan candidate was not admitted.'
    Assert-True (-not (Admit-CandidateModel ('cap-'+$_) 'scan')) 'Candidate cap was exceeded.'
    Reset-CandidateModel
    Assert-True ($candidateModel.Candidates.Count-eq0) 'Reset retained a wrapper.'
    Assert-True (-not (Admit-CandidateModel $old 'scan')) 'Old externally-valid widget was promoted by a later scan.'
    Reset-CandidateModel
}
Assert-True ($candidateModel.Max-le2) 'Candidate model exceeded the hard cap across 100 travel/F8 cycles.'
Assert-True (Admit-CandidateModel 'new-notify-after-travel' 'notify') 'Current Notify-created candidate did not become usable.'

$loopModel=[ordered]@{CompactToken=0;WorldToken=0;CompactActive=0;WorldActive=0;Max=0;PendingToken=0}
function Start-LoopModel([string]$Kind){
    if($Kind-eq'compact'){$script:loopModel.WorldToken++;$script:loopModel.WorldActive=0;$script:loopModel.CompactToken++;$script:loopModel.CompactActive=1;$token=$script:loopModel.CompactToken}
    else{$script:loopModel.CompactToken++;$script:loopModel.CompactActive=0;$script:loopModel.WorldToken++;$script:loopModel.WorldActive=1;$token=$script:loopModel.WorldToken}
    $script:loopModel.Max=[Math]::Max($script:loopModel.Max,$script:loopModel.CompactActive+$script:loopModel.WorldActive);return $token
}
1..100|ForEach-Object{$oldCompact=Start-LoopModel 'compact';$oldWorld=Start-LoopModel 'world';$null=Start-LoopModel 'compact';Assert-True ($oldCompact-ne$loopModel.CompactToken-and$oldWorld-ne$loopModel.WorldToken) 'Old loop token survived a mode/F8 cycle.'}
Assert-True ($loopModel.Max-le1) 'Logical compact/world loop concurrency exceeded one.'
$oldPending=++$loopModel.PendingToken;$newPending=++$loopModel.PendingToken
if($oldPending-eq$loopModel.PendingToken){$loopModel.PendingToken=0}
Assert-True ($loopModel.PendingToken-eq$newPending) 'Stalled old callback cleared the new pending gate.'

# One shared ProcessEvent slot covers every task type. A callback timeout poisons
# the route once for the session and never submits a replacement to the dead queue.
Assert-True ($main.Contains('local process_event_dispatch = {') -and $main.Contains('timeout_ms = 3000')) 'Serialized ProcessEvent timeout/state table is missing.'
Assert-True ($main.Contains('PROCESS_EVENT_DISPATCH_POISONED')) 'ProcessEvent route poisoning is not observable.'
Assert-True ($main.Contains('retry_count = 0') -and $main.Contains('engine_tick_fallback = false')) 'Poisoned ProcessEvent policy does not explicitly prohibit retries and fallback.'
Assert-True (-not $main.Contains('ACTIVATION_CALLBACK_TIMEOUT_RETRY')) 'Obsolete activation-only retry still queues work to a stalled ProcessEvent route.'
$dispatchModel=[ordered]@{Pending=$true;Elapsed=0;Queues=1;Peak=1;Poisoned=$false;Retries=0;Epoch=1;ScheduledEpoch=1;Reads=0}
1..20|ForEach-Object{if(-not$dispatchModel.Pending-and-not$dispatchModel.Poisoned){$dispatchModel.Pending=$true;$dispatchModel.Queues++};$dispatchModel.Peak=[Math]::Max($dispatchModel.Peak,[int]$dispatchModel.Pending)}
Assert-True ($dispatchModel.Queues-eq1-and$dispatchModel.Peak-eq1) 'Cross-task producer pressure created more than one ProcessEvent request.'
1..12|ForEach-Object{$dispatchModel.Elapsed=[Math]::Min(3000,$dispatchModel.Elapsed+250)}
if($dispatchModel.Pending-and$dispatchModel.Elapsed-ge3000){$dispatchModel.Poisoned=$true;$dispatchModel.Retries=0}
Assert-True ($dispatchModel.Poisoned-and$dispatchModel.Retries-eq0-and$dispatchModel.Queues-eq1) 'Stalled ProcessEvent route retried or failed to poison once.'
$dispatchModel.Epoch=2
if(-not$dispatchModel.Poisoned-and$dispatchModel.ScheduledEpoch-eq$dispatchModel.Epoch){$dispatchModel.Reads++}
Assert-True ($dispatchModel.Reads-eq0) 'Poisoned stale ProcessEvent work reached a UObject read after an epoch change.'

Write-Host "PERFORMANCE_SCHEDULING_TESTS_OK assertions=$script:assertionCount; luaExecution=unavailable; stateModels=executed"
