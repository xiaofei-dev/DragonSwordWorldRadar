[CmdletBinding()]
param([switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$adapterText = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'src\ue4ss\main.cpp')
$luaText = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Scripts\main.lua')
$fingerprintText = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'src\platform\windows_fingerprint.cpp')

foreach ($pattern in @('FindAllOf', 'FindObjects\s*\(', 'RegisterProcessEvent',
        'RegisterStaticConstructObject', 'RegisterAActorTick', 'ReceiveTick', 'RegisterLoadMap',
        'OnBeginOverlap', 'ExecuteWithDelay', 'ExecuteInGameThread')) {
    if ($adapterText -match $pattern -or $luaText -match $pattern) { throw "Forbidden runtime pattern found: $pattern" }
}
if ($luaText -match 'Find(First|All)Of|GameViewport|GameInstance|LocalPlayers|PlayerController|Pawn|function\s*\(') {
    throw 'Lua must remain a marker-only entry point.'
}
if ($fingerprintText -notmatch 'binary_directory / "ue4ss" / "UE4SS\.dll"') {
    throw 'Fingerprint source must read UE4SS from Win64/ue4ss.'
}
if ($luaText -notmatch 'Native input-frame pulse enabled; Lua scheduler disabled' -or
    $luaText -notmatch 'OWNER_AUTHORIZED_RUNTIME_CANARY') {
    throw 'Lua marker does not identify the native pulse canary.'
}

foreach ($pattern in @(
        'FUObjectCreateListener', 'FUObjectDeleteListener', 'AddUObjectCreateListener', 'AddUObjectDeleteListener',
        'RemoveUObjectCreateListener', 'RemoveUObjectDeleteListener', 'FWeakObjectPtr', 'std::scoped_lock',
        '/Script/Engine\.PlayerController:ServerRecvClientInputFrame',
        'binary_directory\(\) / "ue4ss" / "Mods" / "DragonSwordNativeAutoPickup"',
        'kPulseInterval = std::chrono::milliseconds\{150\}', 'RegisterPostHook\(&input_frame_post',
        'UnregisterHook\(pulse_hook_post_\)', '/Script/Engine\.Actor:K2_GetActorLocation',
        '/Script/DS\.DInteractableComponent:Server_InputInteractKeyAction', '/Script/DS\.DropItemActor',
        'GetClassPrivate\(\) == self->drop_item_class_', 'InteractComponent', 'InteractableValue', 'InteractTypeValue',
        'kRequiredInteractableValue = 2', 'kDropItemInteractType',
        'GetValuePtrByPropertyNameInChain<UObject\*>\(STR\("Pawn"\)\)',
        'GetValuePtrByPropertyNameInChain<UObject\*>\(STR\("Controller"\)\)',
        '\*controller_value != controller', 'IsA\(player_character_class_\)', 'IsA\(player_controller_class_\)',
        'next_pulse_due_ = now \+ kPulseInterval', 'radius_meters \* 100\.0', 'kCandidatesPerPulse = 8',
        'max_retries', 'retry_backoff_ms', 'action_interval_ms', 'GetParmsSize\(\)',
        'ProcessEvent\(interaction_function_', 'RegisterInitGameStatePreCallback', 'OWNER_AUTHORIZED_RUNTIME_CANARY',
        '__try', 'raw_hook_callbacks_', 'accepted_pulses_', 'throttle_rejects_', 'gate_rejections_', 'PERF_AGGREGATE')) {
    if ($adapterText -notmatch $pattern) { throw "Required native-pulse canary gate is missing: $pattern" }
}

$createCallback = [regex]::Match($adapterText,
    '(?s)void NotifyUObjectCreated\(.*?\) override \{(.*?)\n    \}\n\n    void NotifyUObjectDeleted')
if (-not $createCallback.Success) { throw 'Could not isolate NotifyUObjectCreated.' }
foreach ($pattern in @('GetValuePtr', 'ProcessEvent', 'GetOuterPrivate', 'GetFullName', 'GetName')) {
    if ($createCallback.Groups[1].Value -match $pattern) { throw "Create callback performs forbidden gameplay work: $pattern" }
}

$onUpdate = [regex]::Match($adapterText, '(?s)void on_update\(\) override \{(.*?)\n    \}\n\n    void NotifyUObjectCreated')
if (-not $onUpdate.Success) { throw 'Could not isolate on_update.' }
foreach ($pattern in @('UObject', 'ProcessEvent', 'GetValuePtrByPropertyName', 'GetOuterPrivate', 'IsA\(')) {
    if ($onUpdate.Groups[1].Value -match $pattern) { throw "on_update performs forbidden UObject work: $pattern" }
}

$configText = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'config\default.ini')
if ($configText -notmatch '(?m)^enabled_on_launch=false$' -or $configText -notmatch '(?m)^toggle_hotkey=F9$') {
    throw 'Default config must remain off and use exact F9.'
}
$contract = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'metadata\interaction-contract.json') | ConvertFrom-Json
if ($contract.schema_version -ne 4 -or $contract.status -ne 'OWNER_AUTHORIZED_RUNTIME_CANARY' -or
    $contract.active_mode_approved -ne $true -or $contract.runtime_acceptance -ne $false -or
    $contract.pulse_function -ne '/Script/Engine.PlayerController:ServerRecvClientInputFrame' -or
    $contract.pulse_interval_ms -ne 150 -or
    $contract.required_identity_rule -ne 'controller.Pawn == player && player.Controller == controller' -or
    $contract.interaction.key_action -ne 13) {
    throw 'Metadata does not match the unaccepted owner-authorized native-pulse canary.'
}

$fingerprints = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'metadata\build-fingerprints.json') | ConvertFrom-Json
if ($fingerprints.unknown_build_policy -ne 'passive_off') { throw 'Unknown build policy must remain passive_off.' }
$headerText = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'include\dsnap\windows_fingerprint.hpp')
function Read-HeaderConstant([string]$Name) {
    $match = [regex]::Match($headerText, [regex]::Escape($Name) + '\s*=\s*"([^"]+)"')
    if (-not $match.Success) { throw "Missing fingerprint constant: $Name" }
    $match.Groups[1].Value
}
if ((Read-HeaderConstant 'kExpectedGameSha256') -ne $fingerprints.game.sha256 -or
    (Read-HeaderConstant 'kExpectedUe4ssSha256') -ne $fingerprints.ue4ss.sha256 -or
    (Read-HeaderConstant 'kExpectedUe4ssGitSha') -ne $fingerprints.ue4ss.git_sha) {
    throw 'Compiled fingerprint constants do not match metadata.'
}

& (Join-Path $PSScriptRoot 'Verify-Manifest.ps1')
& (Join-Path $PSScriptRoot 'Test-PackageLayout.ps1')
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build-Core.ps1') }
Write-Host 'DragonSwordNativeAutoPickup native-pulse canary source verification passed.'
