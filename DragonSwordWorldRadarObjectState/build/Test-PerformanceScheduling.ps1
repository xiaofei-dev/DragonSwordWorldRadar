#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$script:assertions = 0
function Assert-True([bool]$condition, [string]$message) {
    $script:assertions++
    if (-not $condition) { throw $message }
}

$native = Get-Content -Raw -LiteralPath (Join-Path $root 'src\native\main.cpp')
$tracker = Get-Content -Raw -LiteralPath (Join-Path $root 'include\dswros\object_state.hpp')
$lua = Get-Content -Raw -LiteralPath (Join-Path $root 'src\ue4ss\main.lua')
$radar = Get-Content -Raw -LiteralPath (Join-Path $root 'src\overlay\UI\RadarForm.cs')
$nativeReader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\overlay\Bridge\NativeStateBridgeReader.cs')
$saveState = Get-Content -Raw -LiteralPath (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs')
$bossRule = Get-Content -Raw -LiteralPath (Join-Path $root 'src\overlay\SaveData\BossRespawnRuleResolver.cs')
$provider = Get-Content -Raw -LiteralPath (Join-Path $root 'src\installer\Core\Providers\TreasureDataProvider.cs')
$actorGenerator = Get-Content -Raw -LiteralPath (Join-Path $root 'src\installer\Core\Generation\TreasureActorCatalogGenerator.cs')
$config = Get-Content -Raw -LiteralPath (Join-Path $root 'src\ue4ss\config.lua')

# Native work is bounded and lifecycle-safe. The sole FindAllOf call is the
# once-per-class catch-up path; normal operation is BeginPlay/EndPlay driven.
foreach ($marker in @(
    'kPositionInterval = std::chrono::milliseconds{33}',
    'kDiscoveryInterval = std::chrono::milliseconds{250}',
    'RegisterBeginPlayPostCallback',
    'RegisterEndPlayPreCallback',
    'FWeakObjectPtr weak{actor}',
    'scanned_classes_.contains(name)',
    'tracker_.reset(activation_, epoch_)',
    'EEndPlayReason::Destroyed',
    'EEndPlayReason::RemovedFromWorld',
    'probe_observed_objects()',
    'Local\\DragonSwordWorldRadarObjectState.NativeState.{}')) {
    Assert-True ($native.Contains($marker)) "Missing native performance/safety marker: $marker"
}
Assert-True (([regex]::Matches($native, 'UObjectGlobals::FindAllOf\(')).Count -eq 1) 'Native source has more than one object-enumeration call site.'
foreach ($forbidden in @('UObjectArray','GetAllActorsOfClass','Sleep(','sleep_for(','static AActor*','static APawn*')) {
    Assert-True (-not $native.Contains($forbidden)) "Forbidden native hot-path/lifetime marker remains: $forbidden"
}
Assert-True ($tracker.Contains('WeakIdentity') -and $tracker.Contains('transition_active')) 'Weak identity and transition fail-closed tracking are missing.'
Assert-True ($tracker.Contains('kCompletionRadius = 3000.0')) 'Native disappearance confirmation is not proximity bounded.'
Assert-True ($tracker.Contains('kRequiredMissingSamples = 2')) 'Native disappearance confirmation is not double sampled.'
Assert-True ($native.Contains('boss-actors.tsv') -and $native.Contains('assault-actors.tsv')) 'Native encounter catalogs are not loaded.'

# Lua keeps control and UMG geometry only. Compact player movement cannot
# create filesystem writes after native coordinates become authoritative.
Assert-True ($lua.Contains('WORLD_MAP_ACTIVE_INTERVAL_MS = 8')) 'World-map presentation is not 8 ms.'
Assert-True ($lua.Contains('FAST_MOTION_INTERVAL_MS = 50')) 'Lua control bridge cadence changed unexpectedly.'
Assert-True ($lua.Contains('Native shared memory owns compact player motion.')) 'Native compact-motion ownership marker is missing.'
Assert-True (-not $lua.Contains('motion_position_epsilon')) 'Legacy player-motion file-write threshold remains.'
Assert-True (-not $lua.Contains('MOTION_Z_EPSILON')) 'Legacy Z-motion file-write threshold remains.'

# F7 owns exactly one save reconciliation attempt. There is no runtime
# fingerprint/check/cooldown scheduler, and F8 invalidates an in-flight result.
foreach ($marker in @(
    'SetNativeActivation(',
    '_activationSyncPending = false;',
    '_syncAttemptedActivation = activation;',
    'retryPolicy=next-activation-only',
    'MarkOpenedFromNative(',
    'MarkEncounterDefeatedFromNative(',
    'MergeNativeOpened(opened);',
    'MergeNativeEncounterDefeats(bossRespawns);')) {
    Assert-True ($saveState.Contains($marker)) "Missing one-shot/native-delta save marker: $marker"
}
foreach ($forbidden in @(
    'RefreshInterval',
    'SnapshotCompletionCooldown',
    '_nextRefreshUtc',
    '_nextSnapshotEligibleUtc',
    'RememberPendingChange(',
    'PendingChangeMatches(')) {
    Assert-True (-not $saveState.Contains($forbidden)) "Periodic save scheduling remains: $forbidden"
}
$consumeIndex = $saveState.IndexOf('_activationSyncPending = false;')
$processIndex = $saveState.IndexOf('GameProcessFinder.OpenTracked', $consumeIndex)
Assert-True ($consumeIndex -ge 0 -and $processIndex -gt $consumeIndex) 'F7 sync is not consumed before fallible process/save access.'

# The shared-memory reader is a fixed-size seqlock with a bounded event ring.
foreach ($marker in @(
    'private const int MappingSize = 5248;',
    'private const int EventCapacity = 128;',
    'sequenceBefore != sequenceAfter',
    'eventHead - previousEventHead - EventCapacity',
    'OpenRetryInterval =')) {
    Assert-True ($nativeReader.Contains($marker)) "Missing shared-memory reader bound: $marker"
}
foreach ($marker in @(
    'NativePositionUsable(nativeState)',
    'ApplyNativePosition(_motion, nativeState)',
    'SetNativeActivation(',
    'MarkOpenedFromNative(',
    'MarkEncounterDefeatedFromNative(')) {
    Assert-True ($radar.Contains($marker)) "Overlay does not consume native state: $marker"
}

Assert-True ($bossRule.Contains('RespawnMinutes = 120')) 'Encounter cooldown is not 120 minutes.'
Assert-True (-not $bossRule.Contains('type=DAILY')) 'Legacy daily encounter reset remains.'
foreach ($marker in @('PropTreasureBoxData.xml','treasure-actors.tsv','TreasureActorCatalogGenerator.Generate')) {
    Assert-True (($provider + $actorGenerator).Contains($marker)) "Install-time native catalog generation is missing: $marker"
}
Assert-True ($actorGenerator.Contains('unresolved > 2')) 'Install-time actor join does not fail closed.'
Assert-True ($config -match 'high_resolution_timer\s*=\s*true') 'High-resolution timer default is not enabled.'
Assert-True ($config -match 'debug_logging\s*=\s*true') 'Debug logging default is not enabled.'

Write-Host "PERFORMANCE_SCHEDULING_TESTS_OK assertions=$script:assertions nativePositionMs=33 saveSync=F7-once objectCatchup=class-once"
