[CmdletBinding()]
param([switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$adapter = Get-Content -Raw (Join-Path $projectRoot 'src\ue4ss\main.cpp')
$lua = Get-Content -Raw (Join-Path $projectRoot 'Scripts\main.lua')
$cmake = Get-Content -Raw (Join-Path $projectRoot 'CMakeLists.txt')
$nativeBuild = Get-Content -Raw (Join-Path $projectRoot 'tools\Build-Native.ps1')

foreach ($pattern in @('FindAllOf','FindFirstOf','FindObjects\s*\(','RegisterProcessEvent','RegisterAActorTick',
        'ReceiveTick','RegisterLoadMap','SendInput','ExecuteWithDelay','ExecuteInGameThread','ForEachUObjectInRange','ForEachUObject\s*\(')) {
    if ($adapter -match $pattern -or $lua -match $pattern) { throw "Forbidden runtime pattern: $pattern" }
}
foreach ($pattern in @('SetObjectPropertyValue','CANARY_ACTION_INVOKED','ACTION_INVOKED_PENDING','invoke_contract_',
        'process_vitality_candidates','process_single_target_canary','capture_vitality_candidate',
        'Vitality_Leave_01_C_','auto_pickup\.contract','persist_confirmed_contract','validate_calibration_delete',
        'action_invocation_total_us_','actions_invoked_','CANARY_PERF','OWNER_AUTHORIZED_RUNTIME_CANARY')) {
    if ($adapter -match $pattern) { throw "Mutation or rejected discovery path remains: $pattern" }
}
if ($adapter -notmatch '0\.6\.0-dropitem-closed-loop-diagnostic') { throw 'Wrong adapter version.' }
if ($adapter -notmatch 'OWNER_AUTHORIZED_READ_ONLY_DIAGNOSTIC') { throw 'Read-only diagnostic label is missing.' }
foreach ($pattern in @('configuration_result_\.value\.action_interval_ms',
        'configuration_result_\.value\.max_retries','configuration_result_\.value\.retry_backoff_ms')) {
    if ($adapter -match $pattern) { throw "Native diagnostic reads a legacy action setting: $pattern" }
}
if ($cmake -match '/WX-' -or $cmake -notmatch '/W4 /WX /wd4324 /external:anglebrackets /external:W0') { throw 'Strict native /W4 /WX is missing.' }
if ($lua -match 'function\s*\(|Find(All|First)Of|GameViewport|ProcessEvent') { throw 'Lua must remain marker-only.' }
foreach ($pattern in @('patternSleuthBindLockBytes','try \{','finally \{','WriteAllBytes\(\$patternSleuthBindLock',
        'Assert-GitCommit \$resolvedUE4SS \$expectedCommits\.UE4SS ''RE-UE4SS after build''')) {
    if ($nativeBuild -notmatch $pattern) { throw "Pinned SDK post-build restoration invariant missing: $pattern" }
}

foreach ($pattern in @('GetAsyncKeyState\(VK_F9\)','f9_toggle_requested_','RegisterEngineTickPostCallback',
        'RegisterInitGameStatePreCallback','UObjectArray::GetNumElements\(\)','UObjectArray::IndexToObject\(index\)',
        'item->IsValid\(false\)','!item->IsUnreachable\(\)','capture_drop_item_guarded',
        'RF_ClassDefaultObject','RF_ArchetypeObject','RF_DefaultSubObject',
        'kDiscoveryBatchBudget = std::chrono::microseconds\{2000\}','kCalibrationWindow = std::chrono::seconds\{60\}',
        'update_diagnostic_lock\(context\)','eligible_count != 1','diagnostic_candidate_identity_',
        'diagnostic_component_identity_','OnBeginOverlap','OnEndOverlap','DropItemActor:AniPickUp',
        'DropItemActor:SetDestroy','trace_interaction_call_guarded\(spec_index, context, "pre"\)',
        'trace_interaction_call_guarded\(spec_index, context, "post", true\)','correlated_pre_pending_',
        'parameter_locked_relation',
        'receiver_field_locked_relation','RemoveUObjectCreateListener','RemoveUObjectDeleteListener',
        'runtime_state_\.reset_off\(\)','diagnostic_cancelled=1')) {
    if ($adapter -notmatch $pattern) { throw "Required read-only diagnostic invariant missing: $pattern" }
}

$f9 = [regex]::Match($adapter, '(?s)void f9_keydown\(\) noexcept \{(.*?)\n    \}')
if (-not $f9.Success) { throw 'Could not isolate F9 callback.' }
foreach ($forbidden in @('UObject','StaticFindObject','RegisterHook','UnregisterHook','register_object_listeners','start_discovery','candidates_')) {
    if ($f9.Groups[1].Value -match $forbidden) { throw "F9 callback violates scalar-only affinity: $forbidden" }
}
$onUpdate = [regex]::Match($adapter, '(?s)void on_update\(\) override \{(.*?)\n    \}\n\n    void NotifyUObjectCreated')
if (-not $onUpdate.Success) { throw 'Could not isolate on_update.' }
foreach ($forbidden in @('StaticFindObject','RegisterHook','UnregisterHook','AddUObject','RemoveUObject','IndexToObject',
        'start_discovery','clear_activation_state','ProcessEvent')) {
    if ($onUpdate.Groups[1].Value -match $forbidden) { throw "on_update violates scalar-only affinity: $forbidden" }
}
$create = [regex]::Match($adapter, '(?s)void NotifyUObjectCreated\(.*?\) override \{(.*?)\n    \}\n\n    void NotifyUObjectDeleted')
foreach ($forbidden in @('GetValuePtr','ProcessEvent','GetOuterPrivate','logger_\.write')) {
    if ($create.Groups[1].Value -match $forbidden) { throw "Create callback performs forbidden work: $forbidden" }
}
$shutdown = [regex]::Match($adapter, '(?s)void OnUObjectArrayShutdown\(\) override \{(.*?)\n    \}\n\nprivate:')
foreach ($required in @('uobject_array_shutdown_seen_\.store\(true','shutting_down_\.store\(true','callback_gate_\.invalidate',
        'abandon_calibration_hooks_after_uobject_shutdown','unregister_object_listeners')) {
    if ($shutdown.Groups[1].Value -notmatch $required) { throw "Shutdown cleanup missing: $required" }
}
if ($shutdown.Groups[1].Value -match 'unregister_calibration_hooks') { throw 'Shutdown must not dereference saved UFunction pointers.' }

$metadata = Get-Content -Raw (Join-Path $projectRoot 'metadata\interaction-contract.json') | ConvertFrom-Json
if ($metadata.schema_version -ne 15 -or $metadata.status -ne 'DROPITEM_CLOSED_LOOP_DIAGNOSTIC' -or
    $metadata.active_mode_approved -ne $false -or $metadata.runtime_acceptance -ne $false -or
    $metadata.automatic_action.enabled -ne $false -or $metadata.automatic_action.target_field_writes -ne $false -or
    $metadata.automatic_action.process_event_pickup_calls -ne $false -or $metadata.automatic_action.contract_io -ne $false -or
    $metadata.window_seconds -ne 60) { throw 'Interaction metadata does not match the read-only diagnostic.' }
$defaultConfig = Get-Content -Raw (Join-Path $projectRoot 'config\default.ini')
if ($defaultConfig -notmatch 'read_only_diagnostic=true' -or
    $defaultConfig -match '(?m)^(action_interval_ms|max_retries|retry_backoff_ms)=') {
    throw 'Default config exposes a rejected action/canary setting.'
}
$fingerprints = Get-Content -Raw (Join-Path $projectRoot 'metadata\build-fingerprints.json') | ConvertFrom-Json
if ($fingerprints.source_build.version -ne '0.6.0-dropitem-closed-loop-diagnostic' -or
    $fingerprints.source_build.deployed -ne $true -or $fingerprints.source_build.runtime_validated -ne $false -or
    ($fingerprints.source_build.installed_path -replace '\\', '/') -notmatch 'Win64/ue4ss/Mods/DragonSwordNativeAutoPickup$') {
    throw 'Build metadata does not identify the deployed but runtime-unvalidated diagnostic.'
}

& (Join-Path $PSScriptRoot 'Verify-Manifest.ps1')
& (Join-Path $PSScriptRoot 'Test-PackageLayout.ps1')
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build-Core.ps1') }
Write-Host 'DragonSwordNativeAutoPickup 0.6.0 read-only diagnostic source verification passed.'
