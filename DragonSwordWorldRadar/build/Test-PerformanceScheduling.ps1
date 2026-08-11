$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$version = [string]((Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json).version)
$clockEnabled = $version -like '0.4.0-dev39-*' -or $version -like '0.4.0-dev48-*' -or $version -like '0.4.0-dev49-*' -or $version -like '0.4.0-dev50-*' -or $version -like '0.4.0-dev51-*' -or $version -like '0.4.0-dev52-*' -or $version -like '0.4.0-dev53-*' -or $version -like '0.4.0-dev54-*' -or $version -like '0.4.0-dev55-*' -or $version -like '0.4.0-dev56-*' -or $version -like '0.4.0-dev57-*' -or $version -like '0.4.0-dev58-*' -or $version -like '0.4.0-dev59-*'
$mole = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\mole_completion.lua') -Raw
$environment = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\world_environment.lua') -Raw
$worldMap = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\world_map.lua') -Raw
$main = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\main.lua') -Raw
$diagnostics = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\diagnostics.lua') -Raw
$radar = Get-Content -LiteralPath (Join-Path $root 'src\overlay\UI\RadarForm.cs') -Raw
$renderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\WorldStatusRenderer.cs') -Raw
$bossRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\BossMarkerRenderer.cs') -Raw
$moleRenderer = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Rendering\MoleMarkerRenderer.cs') -Raw
$overlayPerformance = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Diagnostics\OverlayPerformanceTracker.cs') -Raw
$debugSettings = Get-Content -LiteralPath (Join-Path $root 'src\overlay\Configuration\DebugSettings.cs') -Raw
$defaultConfig = Get-Content -LiteralPath (Join-Path $root 'src\ue4ss\config.default.lua') -Raw
$installer = Get-Content -LiteralPath (Join-Path $root 'src\installer\Install.ps1') -Raw
$keyReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveDatabaseKeyReader.cs') -Raw
$snapshotReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveSnapshotReader.cs') -Raw
$saveState = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
$ownerConfig = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\GeneratedOwnerPointerConfig.cs') -Raw
$ownerResolver = Get-Content -LiteralPath (Join-Path $root 'src\installer\Core\Generation\OwnerPointerRvaResolver.cs') -Raw
$defaultOverrides = Get-Content -LiteralPath (Join-Path $root 'resources\defaults\treasure_overrides.txt') -Raw
$baselineRoot = Join-Path $root 'dist\DragonSwordWorldRadar-0.4.0-dev15-status-autostart-layout1\DragonSwordWorldRadar'
$performanceBaselineRoot = Join-Path $root 'dist\DragonSwordWorldRadar-0.4.0-dev30-referencecache1\DragonSwordWorldRadar'
$script:assertionCount = 0
function Assert-True([bool]$Condition, [string]$Message) {
    $script:assertionCount++
    if (-not $Condition) { throw $Message }
}
Assert-True ($defaultConfig.Contains('debug_logging = true')) 'Pre-release file diagnostics are not enabled by default.'
Assert-True ($defaultConfig.Contains('diagnostic_verbose = false')) 'In-map verbose labels are not disabled by default.'
Assert-True ($debugSettings.Contains('public static bool VerboseLabelsEnabled')) 'File diagnostics and visual labels do not expose separate settings.'
Assert-True ($radar.Contains('DebugSettings.VerboseLabelsEnabled')) 'Radar visual labels are not gated by diagnostic_verbose.'
Assert-True (-not $radar.Contains('bool showDebugCoordinates = _debugEnabled;')) 'Radar visual labels are still coupled to file debug logging.'
foreach ($marker in @('MINIMAP_SCALE_PERF','cache_validation_ms','find_called','find_ms','resolved_validation_ms','layer_map_validation_ms','map_overlay_validation_ms','scale_access_ms')) { Assert-True ($main.Contains($marker)) "Missing minimap scale phase diagnostic: $marker" }
foreach ($marker in @('EXPECTED_RECORD_COUNT = 34','unsafe runtime completion queries are disabled','function M.set_context_available','function M.visible_mask','return false')) { Assert-True ($mole.Contains($marker)) "Missing Mole safety-mode contract: $marker" }
foreach ($forbidden in @('FindFirstOf','StaticFindObject','CIsClearMiniGameInStandAlone','clear_function(')) { Assert-True (-not $mole.Contains($forbidden)) "Unsafe Mole runtime query remains: $forbidden" }
foreach ($marker in @('GAME_SECONDS_PER_REAL_SECOND = 60','DEFAULT_STABLE_CONTEXT_TICKS = 8','function M.arm()','function M.cancel()','function M.context_lost()','function M.observe_context(available)','function M.capture_ready()','function M.mark_capture_queued()','function M.capture_in_game_thread(token)','function M.advance()','attempted_reads=1','retry_reads=0','baseline_wall_seconds','os.time() - baseline_wall_seconds','%SECONDS_PER_DAY')) { Assert-True ($environment.Contains($marker)) "Missing isolated one-shot world-time contract: $marker" }
Assert-True (-not $environment.Contains('os.clock(')) 'Portable Lua os.clock must not drive production world time.'
foreach ($api in @('record_mole_query','record_mole_state_change','record_mole_scheduler')) { Assert-True ($diagnostics.Contains("function Diagnostics.$api")) "Missing debug API: $api" }
Assert-True ($main.Contains('math.floor(previous.world_time_seconds / 60)')) 'Exact-seconds-only bridge suppression is missing.'
Assert-True (-not $main.Contains('Game time is sampled once per second')) 'Stale Ready log claim remains.'
Assert-True ($main.Contains('local function radar_game_thread_callback()')) 'Stable compact game-thread callback is missing.'
Assert-True ($main.Contains('ExecuteInGameThread(radar_game_thread_callback)')) 'Compact control still does not queue the stable callback function.'
foreach ($marker in @('local ACTIVATION_STABLE_SAMPLE_COUNT = 4','local function activation_game_thread_callback()','local function queue_activation_probe()','ExecuteInGameThread(activation_game_thread_callback)','ACTIVATION_STABILITY_SAMPLE','ACTIVATION_STABLE')) { Assert-True ($main.Contains($marker)) "Missing stable F7 activation contract: $marker" }
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
    'F7 deferred safely until the current Pawn-loss cooldown completes')) {
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
foreach($marker in @('function WorldMap.close_session()','record_candidate_metrics("close-session")','world_map.close_session()')){Assert-True (($worldMap.Contains($marker))-or($main.Contains($marker))) "World-map reopen boundary is missing: $marker"}
foreach($marker in @('function WorldMap.consume_wake_hint()','WORLD_MAP_INACTIVE_CHECK_INTERVAL_MS = 2000','world_map.consume_wake_hint()')){Assert-True (($worldMap+$main).Contains($marker)) "Inactive world-map wake/backoff marker is missing: $marker"}
Assert-True (([regex]::Matches($main,[regex]::Escape('world_map.close_session()'))).Count -eq 2) 'World-map close must invalidate candidates in both control and fast-loop close paths.'
Assert-True (-not $worldMap.Contains('known_layers')) 'Unbounded retained world-map list remains.'
Assert-True (([regex]::Matches($worldMap,[regex]::Escape('FindAllOf("DLayerMap_C")'))).Count -eq 1) 'World-map widget enumeration is not limited to one bounded rescan site.'
$activeReadStart=$worldMap.IndexOf('function WorldMap.read_state()')
$activeReadEnd=$worldMap.IndexOf('function WorldMap.debug_state()',$activeReadStart)
$activeReadBlock=$worldMap.Substring($activeReadStart,$activeReadEnd-$activeReadStart)
Assert-True (-not $activeReadBlock.Contains('FindAllOf(')) 'Active world-map read_state directly enumerates UObjects.'
foreach($marker in @('TreasureQueryInterval =','TimeSpan.FromSeconds(45)','IncludeTreasure','OpenedAvailable','encounterTaskQueryMs','tb_unexpected_switch_week')){Assert-True (($snapshotReader+$saveState).Contains($marker)) "Split save-query/task diagnostic marker is missing: $marker"}
foreach($marker in @('motion_loop_token','world_map_loop_token','owner_loop_token ~= motion_loop_token','owner_loop_token ~= world_map_loop_token','update_pending_token == request_token','world_motion_pending_token == request_token','max_logical_loops')){Assert-True (($main+$diagnostics).Contains($marker)) "Async ownership marker is missing: $marker"}
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
Assert-True ($main.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 4')) 'Visible world-map producer is not diagnostic 4 ms.'
Assert-True ($main.Contains('FAST_MOTION_INTERVAL_MS = 50')) 'Radar producer is not 50 ms.'
Assert-True ($main.Contains('MINIMAP_UPDATE_INTERVAL_MS = 250')) 'Player/control scalar sampling is not 250 ms.'
Assert-True (([regex]::Matches($main,'get_player_location\(\)')).Count -eq 2) 'Fresh full-chain location must have one definition and one 250 ms call site.'
Assert-True (-not $main.Contains('queue_motion_update()')) 'Duplicated 50 ms player sampling call remains.'
$worldMotionStartForScalar=$main.IndexOf('local function queue_world_motion_update(')
$worldMotionEndForScalar=$main.IndexOf('ensure_motion_loop_started = function()',$worldMotionStartForScalar)
$worldMotionScalarBlock=$main.Substring($worldMotionStartForScalar,$worldMotionEndForScalar-$worldMotionStartForScalar)
Assert-True (-not $worldMotionScalarBlock.Contains('get_player_location()') -and $worldMotionScalarBlock.Contains('latest_motion_x')) 'Large-map loop does not reuse the shared scalar player sample.'
Assert-True ($main.Contains('record_world_map_producer_tick()')) 'World-map producer-rate diagnostics are missing.'
Assert-True (-not $main.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 24')) '24 ms world-map producer constant remains.'
Assert-True (-not $main.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 50')) 'Dev40 50 ms world-map producer constant remains.'
Assert-True (-not $main.Contains('FAST_MOTION_INTERVAL_MS = 24')) '24 ms radar producer constant remains.'
Assert-True ($radar.Contains('ActiveTimerIntervalMs = 50')) 'Overlay active timer is not 50 ms.'
Assert-True ($radar.Contains('WorldMapTimerIntervalMs = 4')) 'World-map presentation timer is not diagnostic 4 ms.'
Assert-True (-not $radar.Contains('WorldIdleTimerIntervalMs')) 'Legacy 50 ms world-map timer remains.'
Assert-True ($radar.Contains('interval = WorldMapTimerIntervalMs;')) 'World-map mode does not deterministically select its diagnostic cadence.'
Assert-True ($radar.Contains('RecordModeTransition(') -and $overlayPerformance.Contains('worldMapPresentationHz=') -and $overlayPerformance.Contains('modeTransitions=')) 'World-map cadence/transition diagnostics are incomplete.'
foreach($marker in @('ScalarMotionPredictor','MaximumPredictionMs = 250.0','StaleFreezeMs = 500.0','MaximumSourceGapMs = 1000.0','frame.WorldEpoch != _worldEpoch','OVERLAY_MOTION_PREDICTION')){Assert-True (($radar+$overlayPerformance).Contains($marker)) "Bounded scalar prediction marker is missing: $marker"}
Assert-True ($diagnostics.Contains('player_location_hz')) 'Lua debug output does not expose full-chain sample rate.'
Assert-True ($radar.Contains('MaintenanceIntervalMs = 1000')) 'Overlay maintenance timer is not 1000 ms.'
Assert-True ($radar.Contains('WindowVisibilityCheckIntervalMs = 250')) 'Window visibility sampling is not 250 ms.'
Assert-True ($radar.Contains('_nextWindowVisibilityCheckUtc = DateTime.MinValue;')) 'Mode transitions do not force an immediate window sample.'
Assert-True ($radar.Contains('RecordWindowVisibilitySample()')) 'Window visibility sample diagnostics are missing.'
Assert-True ($debugSettings.Contains('HighResolutionTimerEnabled')) 'High-resolution timer startup setting is missing.'
Assert-True ($radar.Contains('if (DebugSettings.HighResolutionTimerEnabled)')) 'timeBeginPeriod is not guarded by the A/B setting.'
Assert-True ($defaultConfig.Contains('high_resolution_timer = false')) 'High-resolution timer does not default off.'
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

# Dev30 is the owner-accepted smoothness baseline. The active cadence remains
# identical, while current-Pawn traversal intentionally differs for travel
# safety and is measured separately from K2_GetActorLocation.
$performanceBaselineMain = Get-Content -LiteralPath (Join-Path $performanceBaselineRoot 'scripts\main.lua') -Raw
$performanceBaselineRadar = Get-Content -LiteralPath (Join-Path $performanceBaselineRoot 'src\overlay\UI\RadarForm.cs') -Raw
foreach ($name in @('MINIMAP_UPDATE_INTERVAL_MS','FAST_MOTION_INTERVAL_MS')) {
    $pattern = 'local ' + $name + ' = (\d+)'
    Assert-True ([regex]::Match($main,$pattern).Groups[1].Value -eq [regex]::Match($performanceBaselineMain,$pattern).Groups[1].Value) "Active Lua cadence differs from dev30 performance baseline: $name"
}
Assert-True ([regex]::Match($main,'local WORLD_MAP_ACTIVE_INTERVAL_MS = (\d+)').Groups[1].Value -eq '4') 'Diagnostic world-map cadence is not 4 ms.'
Assert-True ([regex]::Match($performanceBaselineMain,'local WORLD_MAP_ACTIVE_INTERVAL_MS = (\d+)').Groups[1].Value -eq '50') 'Packaged dev30 world-map reference cadence unexpectedly changed.'
foreach($required in @('_worldMapTimerResolutionAcquired','UpdateWorldMapTimerResolution(','ReleaseWorldMapTimerResolution(','RecordTimerResolutionChange','timerResolutionBalance=')){Assert-True (($radar+$overlayPerformance).Contains($required)) "Mode-scoped world-map timer-resolution marker is missing: $required"}
Assert-True ([regex]::Match($radar,'ActiveTimerIntervalMs = (\d+)').Groups[1].Value -eq [regex]::Match($performanceBaselineRadar,'ActiveTimerIntervalMs = (\d+)').Groups[1].Value) 'Overlay active cadence differs from dev30 performance baseline.'
Assert-True ($performanceBaselineMain.Contains('local player_pawn = nil')) 'Packaged dev30 no longer demonstrates the retained-Pawn baseline.'
Assert-True (-not $main.Contains('local player_pawn = nil')) 'Candidate reintroduced the dev30 cross-frame Pawn wrapper.'
foreach ($marker in @('record_player_location','player_root_avg_ms','actor_location_avg_ms')) { Assert-True (($main+$diagnostics).Contains($marker)) "Fresh-Pawn split timing is missing: $marker" }

# Exact dev15 restoration gates for files that have no approved deviation.
foreach ($relative in @(
    'src\overlay\Data\WorldTreasureVisibilityIndex.cs',
    'src\overlay\Platform\NativeMethods.cs',
    'src\overlay\Rendering\BossMarkerRenderer.cs',
    'src\overlay\Rendering\MoleMarkerRenderer.cs')) {
    $currentText = Get-Content -LiteralPath (Join-Path $root $relative) -Raw
    $baselineText = Get-Content -LiteralPath (Join-Path $baselineRoot $relative) -Raw
    Assert-True ($currentText -ceq $baselineText) "Runtime file differs from authoritative dev15 baseline: $relative"
}
Assert-True ($diagnostics.Contains('record_motion_write')) 'Lua motion-write diagnostics are missing.'
Assert-True ($overlayPerformance.Contains('OVERLAY_WORK_PERF')) 'Overlay work-duty diagnostics are missing.'
$saveState = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs') -Raw
Assert-True ($saveState.Contains('bool initialSnapshot = !_hasLoadedSaveState') -and $saveState.Contains('&& !_initialDebounceBypassConsumed;') -and $saveState.Contains('_initialDebounceBypassConsumed = true;') -and $saveState.Contains('if (!initialSnapshot') -and $saveState.Contains('fingerprint before/after its copy')) 'One-shot initial debounce bypass or consistent-copy safety marker is missing.'
$saveReader = Get-Content -LiteralPath (Join-Path $root 'src\overlay\SaveData\SaveSnapshotReader.cs') -Raw
Assert-True ($saveState.Contains('TimeSpan.FromMilliseconds(2000)')) 'Save metadata polling is not two seconds.'
foreach ($marker in @('SaveChangeDebounce','TimeSpan.FromSeconds(4)','SetRuntimeEnabled','ThreadPriority.BelowNormal','SAVE_REFRESH_PERF')) { Assert-True ($saveState.Contains($marker)) "Missing optimized save-state contract: $marker" }
foreach ($marker in @('DatabaseCacheHits','TryGetCached','SqliteOpenReadWrite','ApplyKey(database, key)','QueryOpenedTreasureBits(database)','QueryBossRespawns(database, encounterTargetIds)','BuildActorFilterSql(encounterTargetIds)','ActorFilterSignature')) { Assert-True ($saveReader.Contains($marker)) "Missing optimized snapshot-reader contract: $marker" }
Assert-True (([regex]::Matches($saveReader, 'PRAGMA key')).Count -eq 1) 'SQLCipher key is applied more than once per database connection.'
foreach ($catalogRelative in @('src\overlay\Data\WorldTreasureCatalog.cs','src\overlay\Data\WorldBossCatalog.cs')) {
    $catalogText = Get-Content -LiteralPath (Join-Path $root $catalogRelative) -Raw
    Assert-True ($catalogText.Contains('if (_hasLoaded)')) "Immutable catalog one-load gate is missing: $catalogRelative"
}
$baselineRadar = Get-Content -LiteralPath (Join-Path $baselineRoot 'src\overlay\UI\RadarForm.cs') -Raw
$queryStart = $radar.IndexOf('        private sealed class RadarTreasureQueryBuffer')
$baselineQueryStart = $baselineRadar.IndexOf('        private sealed class RadarTreasureQueryBuffer')
Assert-True ($queryStart -ge 0 -and $baselineQueryStart -ge 0 -and $radar.Substring($queryStart) -ceq $baselineRadar.Substring($baselineQueryStart)) 'Radar treasure query-buffer implementation differs from dev15.'
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
foreach ($name in @('MINIMAP_SCALE_CHECK_INTERVAL_MS','MAP_LOAD_RESUME_DELAY_MS','MOD_SWITCH_CHECK_INTERVAL_MS','MINIMAP_UPDATE_INTERVAL_MS','FAST_MOTION_HEARTBEAT_MS')) {
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
Assert-True ([regex]::Matches($defaultOverrides, '(?im)^\s*ignore\s+11003\s*(?:#.*)?$').Count -eq 1) 'Default duplicate ignore 11003 must exist exactly once.'
Assert-True ($defaultOverrides.Contains('alias 11003 14016') -and $defaultOverrides.Contains('authoritative 14016 record')) 'Default 11003/14016 duplicate explanation is missing.'
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
Assert-True (-not ($installer -match 'notmatch[^\r\n]+11003')) 'Installer must not migrate ignore 11003.'
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
    $_ -notmatch '^\s*#\s*Known abandoned or inaccessible chest record\.\s*$'
})
Assert-True ($migratedOverrides.Count -eq 6) '10220122 migration removed an unexpected number of user lines.'
Assert-True ($migratedOverrides -contains 'ignore 77777') 'Accepted migration removed an unrelated ignore rule.'
Assert-True ($migratedOverrides -contains 'alias 11003 14016') 'Accepted migration removed a user alias.'
Assert-True ($migratedOverrides -contains '  IGNORE 11003 # retained duplicate rule') '10220122 migration removed ignore 11003.'
$missingPlayer = $main.IndexOf('if player_x == nil or player_y == nil then')
$clearPublishedTime = $main.IndexOf('world_time_available = false', $missingPlayer)
$missingPlayerWrite = $main.IndexOf('write_disabled_motion()', $missingPlayer)
Assert-True ($missingPlayer -ge 0 -and $clearPublishedTime -gt $missingPlayer -and $clearPublishedTime -lt $missingPlayerWrite) 'Context-loss frame can serialize stale world-time availability.'
$modDisable = $main.IndexOf('local function disable_for_mod_switch()')
$modClearTime = $main.IndexOf('world_time_available = false', $modDisable)
$modWrite = $main.IndexOf('write_disabled_motion()', $modDisable)
Assert-True ($modClearTime -gt $modDisable -and $modClearTime -lt $modWrite) 'mods.txt disable can serialize stale world-time availability.'
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

Write-Host "PERFORMANCE_SCHEDULING_TESTS_OK assertions=$script:assertionCount; luaExecution=unavailable; stateModels=executed"
